#include "evaluator.h"
#include "garbage-colector/root.h"
#include "interpreter/environment.h"
#include "interpreter/value.h"
#include "parser/expression.h"
#include "parser/parser.h"
#include "parser/statement.h"
#include <array>
#include <cmath>
#include <cstddef>
#include <print>
#include <string>
#include <utility>
#include <vector>

inline constexpr std::array<std::string, 5> builtinFuncs = {
    "print", "len", "push", "set", "has"};

bool isBuiltIn(std::string name) {
  for (std::string builtin : builtinFuncs) {
    if (builtin == name) {
      return true;
    }
  }
  return false;
}

// fromProgramStatement is the index of the next statement to evaluate
// (how many statements were already evaluated), so we can use one parser
// instance in the repl.
Value Evaluator::evalStatements(size_t fromProgramStatement) {
  gc.globalEnv = globalEnv;
  Value result;
  for (size_t i = fromProgramStatement;
       i < parserResult.programStatementsIndexes.size(); i++) {
    int index = parserResult.programStatementsIndexes[i];
    result = evalStatement(index, globalEnv);
    if (isError(result) || returning ||
        parserResult.statements[index].kind == StatementKind::RETURN) {
      // A top level return ends this run, so the repl's next line starts clean.
      returning = false;
      return result;
    }
  }
  return result;
}

Value Evaluator::evalExpression(int index, ObjEnv *env) {
  // A statement that failed to parse holds no expression
  if (index == -1) {
    return makeError("invalid expression");
  }
  const Expression &expr = parserResult.expressions[index];
  switch (expr.kind) {
  case ExpressionKind::LITERAL_NUMBER:
    return makeNumber(expr.numValue);
  case ExpressionKind::LITERAL_BOOL:
    return makeBool(expr.boolValue);
  case ExpressionKind::LITERAL_NULL:
    return makeNull();
  case ExpressionKind::UNARY: {
    Value value = evalExpression(expr.subExprIndex, env);
    if (isError(value)) {
      return value;
    }
    return evalPrefixExpression(expr.unaryOperator, value);
  }
  case ExpressionKind::BINARY: {
    Value left = evalExpression(expr.leftExprIndex, env);
    // A failed operand is reported as it happened, instead of turning into
    // an "unknown operator" about the error value itself.
    if (isError(left)) {
      return left;
    }
    // Evaluate right only when it can change the answer
    if (expr.binaryOperator == BinaryOperator::AND && !isTruthy(left)) {
      return makeBool(false);
    }
    if (expr.binaryOperator == BinaryOperator::OR && isTruthy(left)) {
      return makeBool(true);
    }
    Rooted rootLeft(left); // it must survive eval right expression
    Value right = evalExpression(expr.rightExprIndex, env);
    if (isError(right)) {
      return right;
    }
    return evalInfixExpression(expr.binaryOperator, left, right);
  }
  case ExpressionKind::ASSIGN: {
    Value value = evalExpression(expr.rightExprIndex, env);
    if (isError(value)) {
      return value;
    }
    if (!envAssign(env, expr.literal, value)) {
      return makeError("identifier not found: " + expr.literal);
    }
    return value;
  }
  case ExpressionKind::LITERAL_STRING:
    return makeString(expr.literal);

  case ExpressionKind::IDENTIFIER: {
    return envGet(env, expr.literal);
  }
  case ExpressionKind::FUNCTION:
    return evalFunctionExpression(expr.literal, expr.parameters,
                                  expr.bodyStmtIndex, env);
  case ExpressionKind::CALL:
    return evalCallExpression(expr.functionExprIndex, expr.expressionsIndexes,
                              env);
  case ExpressionKind::LITERAL_ARRAY:
    return evalArray(index, env);
  case ExpressionKind::INDEX:
    return evalIndex(index, env);
  case ExpressionKind::LITERAL_HASH:
    return evalHashMap(index, env);
  case ExpressionKind::STAR:
    break;
  }
  return {};
}

Value Evaluator::evalStatement(int index, ObjEnv *env) {
  if (index == -1) {
    return makeError("invalid statement");
  }
  const Statement &stmt = parserResult.statements[index];
  switch (stmt.kind) {
  case StatementKind::BLOCK:
    return evalBlockStatement(index, env);
  case StatementKind::IF:
    return evalIfStatement(index, env);
  case StatementKind::LET:
    return evalLetStatement(index, env);
  case StatementKind::RETURN: {
    Value value = evalExpression(stmt.expressionIndex, env);
    returning = true;
    return value;
  }
  case StatementKind::EXPRESSION:
    return evalExpression(stmt.expressionIndex, env);
    break;
  }
  return {};
}

Value Evaluator::evalPrefixExpression(UnaryOperator oper, Value &value) {
  if (oper == UnaryOperator::NEGATE && isNumber(value)) {
    value.num = -value.num;
    return value;
  }
  if (oper == UnaryOperator::NOT && isBool(value)) {
    value.boolean = !value.boolean;
    return value;
  }
  std::string message = "unknown operator: " + unaryOperatorToString(oper) +
                        valueKindToString(kindOf(value));
  return makeError(message);
}

Value Evaluator::evalInfixExpression(BinaryOperator oper,
                                     const Value &leftValue,
                                     const Value &rightValue) {
  switch (oper) {
  case BinaryOperator::NOT_EQUAL:
  case BinaryOperator::EQUAL:
  case BinaryOperator::LESS_THAN:
  case BinaryOperator::LESS_THAN_OR_EQUAL:
  case BinaryOperator::GREATER_THAN:
  case BinaryOperator::GREATER_THAN_OR_EQUAL:
    return makeBool(compare(oper, leftValue, rightValue));
  case BinaryOperator::AND:
  case BinaryOperator::OR: {
    bool left = isTruthy(leftValue);
    bool right = isTruthy(rightValue);
    return makeBool(oper == BinaryOperator::AND ? left && right
                                                : left || right);
  }
  case BinaryOperator::ADD:
  case BinaryOperator::SUBTRACT:
  case BinaryOperator::MULTIPLY:
  case BinaryOperator::DIVIDE:
  case BinaryOperator::MODULO:
    if (isNumeric(leftValue) && isNumeric(rightValue)) {
      double left = asNumber(leftValue);
      double right = asNumber(rightValue);
      if (oper == BinaryOperator::ADD) {
        return makeNumber(left + right);
      }

      if (oper == BinaryOperator::SUBTRACT) {
        return makeNumber(left - right);
      }

      if (oper == BinaryOperator::MULTIPLY) {
        return makeNumber(left * right);
      }

      if (oper == BinaryOperator::DIVIDE) {
        return makeNumber(left / right);
      }

      if (oper == BinaryOperator::MODULO) {
        return makeNumber(std::fmod(left, right));
      }
    }
    if (oper == BinaryOperator::ADD && isString(leftValue) &&
        isString(rightValue)) {
      return makeString(asString(leftValue)->chars +
                        asString(rightValue)->chars);
    }

    if (oper == BinaryOperator::ADD && isArray(leftValue) &&
        isArray(rightValue)) {
      // Concatenation builds a fresh array, so neither operand is mutated.
      std::vector<Value> items = asArray(leftValue)->items;
      const std::vector<Value> &rightItems = asArray(rightValue)->items;
      items.insert(items.end(), rightItems.begin(), rightItems.end());
      return makeArray(std::move(items));
    }
    break;
  }
  std::string message =
      "unknown operator: " + valueKindToString(kindOf(leftValue)) + " " +
      binaryOperatorToString(oper) + " " +
      valueKindToString(kindOf(rightValue));
  return makeError(message);
}

Value Evaluator::evalIfStatement(int index, ObjEnv *env) {
  const Statement &stmt = parserResult.statements[index];
  Value conditionValue = evalExpression(stmt.conditionExprIndex, env);
  if (isError(conditionValue)) {
    return conditionValue;
  }
  if (isTruthy(conditionValue)) {
    return evalBlockStatement(stmt.consequenceStmtIndex, env);
  } else if (stmt.alternativeStmtIndex != -1) {
    // "else if" alternatives are IF statements, plain "else" ones are blocks
    return evalStatement(stmt.alternativeStmtIndex, env);
  }
  return {};
}

Value Evaluator::evalBlockStatement(int index, ObjEnv *env) {
  const Statement &stmt = parserResult.statements[index];
  Value returnedValue;
  for (int index : stmt.statementsIndexes) {
    returnedValue = evalStatement(index, env);
    if (isError(returnedValue) || returning ||
        parserResult.statements[index].kind == StatementKind::RETURN) {
      return returnedValue;
    }
  }
  return returnedValue;
}

Value Evaluator::evalLetStatement(int index, ObjEnv *env) {
  const Statement &stmt = parserResult.statements[index];
  // If the variable has no initializer it gets null as value
  if (stmt.expressionIndex < 0) {
    envSet(env, stmt.name, makeNull());
    return {};
  }
  Value val = evalExpression(stmt.expressionIndex, env);
  envSet(env, stmt.name, val);
  return val;
}

Value Evaluator::evalFunctionExpression(
    const std::string &name, const std::vector<std::string> &parameters,
    int bodyStmtIndex, ObjEnv *env) {
  Value value = makeFunction(parameters, bodyStmtIndex, env);
  if (!name.empty()) {
    envSet(env, name, value);
  }
  return value;
}

std::vector<Value>
Evaluator::evalExpressions(const std::vector<int> &argExprIndexes,
                           ObjEnv *env) {
  std::vector<Value> result;
  RootedVector rootResult(result); // earlier args survive later ones
  for (int index : argExprIndexes) {
    Value value = evalExpression(index, env);
    if (isError(value)) {
      return {value};
    }
    result.push_back(value);
  }
  return result;
}

// We get the function's environment and give the params values from the args
ObjEnv *extendFunctionEnv(Value &function, std::vector<Value> &args) {
  ObjFunction *fn = asFunction(function);
  // Using the function's env instead of the caller's is what makes closures
  // work
  ObjEnv *env = newEnclosedEnvironment(fn->env);
  for (size_t i = 0; i < args.size(); i++) {
    // Map the function parameters to their values from the arguments
    envSet(env, fn->parameters[i], args[i]);
  }
  return env;
}

Value Evaluator::applyFunction(Value &function, std::vector<Value> &args) {
  ObjFunction *fn = asFunction(function);
  if (fn->parameters.size() != args.size()) {
    std::string message = "wrong number of arguments: got " +
                          std::to_string(args.size()) + ", want " +
                          std::to_string(fn->parameters.size());
    return makeError(message);
  }
  ObjEnv *extendedEnv = extendFunctionEnv(function, args);
  RootedObj rootEnv(extendedEnv); // holds the whole call alive
  Value evaluted = evalStatement(fn->bodyStmtIndex, extendedEnv);
  // The return belongs to this call, so the caller carries on normally.
  returning = false;
  return evaluted;
}

static Value wrongArgCountError(size_t got, size_t want) {
  std::string message = "wrong number of arguments: got " +
                        std::to_string(got) + ", want " + std::to_string(want);
  return makeError(message);
}

static bool argsFailed(const std::vector<Value> &args) {
  return args.size() == 1 && isError(args[0]);
}

static Value hashMapKeyError(const Value &key) {
  return makeError("hash map key must be a number, string, bool or null: " +
                   valueKindToString(kindOf(key)));
}

Value Evaluator::lookupVariableArg(const std::string &funcName,
                                   int argExprIndex, ObjEnv *env) {
  const Expression &expr = parserResult.expressions[argExprIndex];
  if (expr.kind != ExpressionKind::IDENTIFIER) {
    return makeError("first argument to " + funcName +
                     " must be an identifier");
  }
  return envGet(env, expr.literal);
}

Value Evaluator::evalPrint(const std::vector<int> &argExprIndexes,
                           ObjEnv *env) {
  std::vector<Value> args = evalExpressions(argExprIndexes, env);
  if (argsFailed(args)) {
    return args[0];
  }
  std::string printedString;
  for (size_t indx = 0; indx < args.size(); indx++) {
    if (indx > 0) {
      printedString += " ";
    }
    printedString += inspect(args[indx]);
  }
  std::println("{}", printedString);
  return {};
}

Value Evaluator::evalLen(const std::vector<int> &argExprIndexes, ObjEnv *env) {
  if (argExprIndexes.size() != 1) {
    return wrongArgCountError(argExprIndexes.size(), 1);
  }
  std::vector<Value> args = evalExpressions(argExprIndexes, env);
  if (argsFailed(args)) {
    return args[0];
  }
  const Value arg = args[0];
  if (!isString(arg) && !isArray(arg) && !isHashMap(arg)) {
    std::string message =
        "argument to len is not supported: " + valueKindToString(kindOf(arg));
    return makeError(message);
  }
  if (isString(arg)) {
    return makeNumber(static_cast<double>(asString(arg)->chars.size()));
  }

  if (isHashMap(arg)) {
    size_t length = asHashMap(arg)->entries.size() / 2;
    return makeNumber(static_cast<double>(length));
  }

  return makeNumber(static_cast<double>(asArray(arg)->items.size()));
}

Value Evaluator::evalPush(const std::vector<int> &argExprIndexes, ObjEnv *env) {
  if (argExprIndexes.size() != 2) {
    return wrongArgCountError(argExprIndexes.size(), 2);
  }
  Value array = lookupVariableArg("push", argExprIndexes[0], env);
  Rooted rootArray(array); // survives evaluating the pushed value
  if (isError(array)) {
    return array;
  }
  if (!isArray(array)) {
    std::string message =
        "argument to push is not an array: " + valueKindToString(kindOf(array));
    return makeError(message);
  }
  Value pushedValue = evalExpression(argExprIndexes[1], env);
  if (isError(pushedValue)) {
    return pushedValue;
  }
  asArray(array)->items.push_back(pushedValue);
  return pushedValue;
}

Value Evaluator::evalSet(const std::vector<int> &argExprIndexes, ObjEnv *env) {
  if (argExprIndexes.size() != 3) {
    return wrongArgCountError(argExprIndexes.size(), 3);
  }
  Value hashMap = lookupVariableArg("set", argExprIndexes[0], env);
  Rooted rootHashMap(hashMap); // survives evaluating the key and the value
  if (isError(hashMap)) {
    return hashMap;
  }
  if (!isHashMap(hashMap)) {
    std::string message = "argument to set is not a hashMap: " +
                          valueKindToString(kindOf(hashMap));
    return makeError(message);
  }
  Value key = evalExpression(argExprIndexes[1], env);
  Rooted rootKey(key); // survives evaluating the value
  if (isError(key)) {
    return key;
  }
  if (!isValidHashMapKey(key)) {
    return hashMapKeyError(key);
  }
  Value value = evalExpression(argExprIndexes[2], env);
  if (isError(value)) {
    return value;
  }
  hashMapSet(asHashMap(hashMap), key, value);
  return value;
}

Value Evaluator::evalHas(const std::vector<int> &argExprIndexes, ObjEnv *env) {
  if (argExprIndexes.size() != 2) {
    return wrongArgCountError(argExprIndexes.size(), 2);
  }
  Value hashMap = lookupVariableArg("has", argExprIndexes[0], env);
  Rooted rootHashMap(hashMap); // survives evaluating the key
  if (isError(hashMap)) {
    return hashMap;
  }
  if (!isHashMap(hashMap)) {
    std::string message = "argument to has is not a hashMap: " +
                          valueKindToString(kindOf(hashMap));
    return makeError(message);
  }
  Value key = evalExpression(argExprIndexes[1], env);
  if (isError(key)) {
    return key;
  }

  if (!isValidHashMapKey(key)) {
    return hashMapKeyError(key);
  }

  return makeBool(hashMapHas(asHashMap(hashMap), key));
}

Value Evaluator::evalBuiltinFuncs(std::string funcName,
                                  const std::vector<int> &argExprIndexes,
                                  ObjEnv *env) {
  if (funcName == "print") {
    return evalPrint(argExprIndexes, env);
  }
  if (funcName == "len") {
    return evalLen(argExprIndexes, env);
  }
  if (funcName == "push") {
    return evalPush(argExprIndexes, env);
  }
  if (funcName == "set") {
    return evalSet(argExprIndexes, env);
  }
  if (funcName == "has") {
    return evalHas(argExprIndexes, env);
  }
  return {};
}

Value Evaluator::evalCallExpression(int functionExprIndex,
                                    const std::vector<int> &argExprIndexes,
                                    ObjEnv *env) {
  const std::string funcName =
      parserResult.expressions[functionExprIndex].literal;
  if (isBuiltIn(funcName)) {
    return evalBuiltinFuncs(funcName, argExprIndexes, env);
  }
  Value function = envGet(env, funcName);
  Rooted rootFunction(function); // survives evaluating the arguments
  if (isError(function)) {
    return function;
  }
  if (!isFunction(function)) {
    std::string message =
        funcName + " is not a function: " + valueKindToString(kindOf(function));
    return makeError(message);
  }
  std::vector<Value> args = evalExpressions(argExprIndexes, env);
  RootedVector rootArgs(args); // survive the call itself
  if (args.size() == 1 && isError(args[0])) {
    return args[0];
  }
  return applyFunction(function, args);
}

Value Evaluator::evalArray(int index, ObjEnv *env) {
  Expression expr = parserResult.expressions[index];
  Value result = makeArray({}); // allocate the empty array first
  Rooted rootResult(result);    // now it is reachable from a root
  ObjArray *array = asArray(result);
  for (int indx : expr.expressionsIndexes) {
    array->items.push_back(evalExpression(indx, env));
  }
  return result;
}

Value Evaluator::evalHashMap(int index, ObjEnv *env) {
  Expression expr = parserResult.expressions[index];
  Value result = makeHashMap({}); // allocate the empty map first
  Rooted rootResult(result);      // now it is reachable from a root
  // expressionsIndexes holds flattened key/value pairs
  const std::vector<int> &pairs = expr.expressionsIndexes;
  for (size_t indx = 0; indx + 1 < pairs.size(); indx += 2) {
    Value key = evalExpression(pairs[indx], env);
    Rooted rootKey(key); // survives evaluating the value
    if (isError(key)) {
      return key;
    }
    if (!isValidHashMapKey(key)) {
      return hashMapKeyError(key);
    }
    Value value = evalExpression(pairs[indx + 1], env);
    // A repeated key in the literal keeps its last value
    hashMapSet(asHashMap(result), key, value);
  }
  return result;
}

Value Evaluator::evalIndex(int index, ObjEnv *env) {
  Expression indexexpr = parserResult.expressions[index];
  Value indexed = envGet(env, indexexpr.literal);
  if (isError(indexed)) {
    return indexed;
  }
  Value indexValue = evalExpression(indexexpr.subExprIndex, env);
  if (isError(indexValue)) {
    return indexValue;
  }
  if (isArray(indexed)) {
    return evalArrayIndex(indexed, indexValue);
  }
  if (isHashMap(indexed)) {
    return evalHashMapIndex(indexed, indexValue);
  }
  return makeError("variable is not an array or a hashmap");
}

Value Evaluator::evalArrayIndex(const Value &array, const Value &indexValue) {
  if (!isNumber(indexValue)) {
    return makeError("index must be a number");
  }
  if (indexValue.num < 0) {
    return makeError("index must be greater or equal than zero");
  }
  const std::vector<Value> &items = asArray(array)->items;
  size_t arrIndex = indexValue.num;
  if (arrIndex >= items.size()) {
    return makeError("index is bigger than array size");
  }
  return items[arrIndex];
}

Value Evaluator::evalHashMapIndex(const Value &hashMap, const Value &keyValue) {
  if (!isValidHashMapKey(keyValue)) {
    return hashMapKeyError(keyValue);
  }
  const std::vector<Value> &entries = asHashMap(hashMap)->entries;
  for (size_t indx = 0; indx + 1 < entries.size(); indx += 2) {
    if (compare(BinaryOperator::EQUAL, entries[indx], keyValue)) {
      return entries[indx + 1];
    }
  }
  return makeError("key not found: " + inspect(keyValue));
}
