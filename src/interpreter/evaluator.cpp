#include "evaluator.h"
#include "interpreter/environment.h"
#include "interpreter/value.h"
#include "parser/expression.h"
#include "parser/parser.h"
#include "parser/statement.h"
#include <array>
#include <cstddef>
#include <print>
#include <string>
#include <utility>
#include <vector>

inline constexpr std::array<std::string, 3> builtinFuncs = {"print", "len",
                                                            "push"};

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
  Value result;
  for (size_t i = fromProgramStatement;
       i < parserResult.programStatementsIndexes.size(); i++) {
    int index = parserResult.programStatementsIndexes[i];
    result = evalStatement(index, globalEnv);
    if (isError(result) ||
        parserResult.statements[index].kind == StatementKind::RETURN) {
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
  case ExpressionKind::UNARY: {
    Value value = evalExpression(expr.subExprIndex, env);
    return evalPrefixExpression(expr.unaryOperator, value);
  }
  case ExpressionKind::BINARY: {
    Value left = evalExpression(expr.leftExprIndex, env);
    Value right = evalExpression(expr.rightExprIndex, env);
    return evalInfixExpression(expr.binaryOperator, left, right);
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
  case StatementKind::VAR:
    return evalVarStatement(index, env);
  case StatementKind::RETURN:
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
  case BinaryOperator::OR:
    if (isNumeric(leftValue) && isNumeric(rightValue)) {
      // A number is true when nonzero.
      bool left = asNumber(leftValue) != 0;
      bool right = asNumber(rightValue) != 0;
      bool result = oper == BinaryOperator::AND ? left && right : left || right;
      return makeBool(result);
    }
    break;
  case BinaryOperator::ADD:
  case BinaryOperator::SUBTRACT:
  case BinaryOperator::MULTIPLY:
  case BinaryOperator::DIVIDE:
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
    if (isError(returnedValue) ||
        parserResult.statements[index].kind == StatementKind::RETURN) {
      return returnedValue;
    }
  }
  return returnedValue;
}

Value Evaluator::evalVarStatement(int index, ObjEnv *env) {
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
  Value evaluted = evalStatement(fn->bodyStmtIndex, extendedEnv);
  return evaluted;
}

Value Evaluator::evalBuiltinFuncs(std::string funcName,
                                  const std::vector<int> &argExprIndexes,
                                  ObjEnv *env) {
  if (funcName == "print") {
    std::vector<Value> args = evalExpressions(argExprIndexes, env);
    if (args.size() == 1 && isError(args[0])) {
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
  if (funcName == "len") {
    if (argExprIndexes.size() != 1) {
      std::string message = "wrong number of arguments: got " +
                            std::to_string(argExprIndexes.size()) + ", want 1";
      return makeError(message);
    }
    std::vector<Value> args = evalExpressions(argExprIndexes, env);
    if (args.size() == 1 && isError(args[0])) {
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
  if (funcName == "push") {
    if (argExprIndexes.size() != 2) {
      std::string message = "wrong number of arguments: got " +
                            std::to_string(argExprIndexes.size()) + ", want 2";
      return makeError(message);
    }
    const Expression &arrayExpr = parserResult.expressions[argExprIndexes[0]];
    if (arrayExpr.kind != ExpressionKind::IDENTIFIER) {
      return makeError("first argument to push must be an identifier");
    }
    Value array = envGet(env, arrayExpr.literal);
    if (isError(array)) {
      return array;
    }
    if (!isArray(array)) {
      std::string message = "argument to push is not an array: " +
                            valueKindToString(kindOf(array));
      return makeError(message);
    }
    Value pushedValue = evalExpression(argExprIndexes[1], env);
    if (isError(pushedValue)) {
      return pushedValue;
    }
    asArray(array)->items.push_back(pushedValue);
    return pushedValue;
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
  if (isError(function)) {
    return function;
  }
  if (!isFunction(function)) {
    std::string message =
        funcName + " is not a function: " + valueKindToString(kindOf(function));
    return makeError(message);
  }
  std::vector<Value> args = evalExpressions(argExprIndexes, env);
  if (args.size() == 1 && isError(args[0])) {
    return args[0];
  }
  return applyFunction(function, args);
}

Value Evaluator::evalArray(int index, ObjEnv *env) {
  Expression expr = parserResult.expressions[index];
  std::vector<Value> items;
  for (int indx : expr.expressionsIndexes) {
    items.push_back(evalExpression(indx, env));
  }
  return makeArray(std::move(items));
}

Value Evaluator::evalHashMap(int index, ObjEnv *env) {
  Expression expr = parserResult.expressions[index];
  std::vector<Value> entries;
  for (int indx : expr.expressionsIndexes) {
    entries.push_back(evalExpression(indx, env));
  }
  return makeHashMap(std::move(entries));
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
  const std::vector<Value> &entries = asHashMap(hashMap)->entries;
  for (size_t indx = 0; indx + 1 < entries.size(); indx += 2) {
    if (compare(BinaryOperator::EQUAL, entries[indx], keyValue)) {
      return entries[indx + 1];
    }
  }
  return {};
}
