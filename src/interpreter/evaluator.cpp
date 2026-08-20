#include "evaluator.h"
#include "interpreter/environment.h"
#include "interpreter/value.h"
#include "parser/expression.h"
#include "parser/parser.h"
#include "parser/statement.h"
#include <cstddef>
#include <memory>
#include <print>
#include <string>
#include <vector>

inline constexpr std::array<std::string, 2> builtinFuncs = {"print", "len"};

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

Value Evaluator::evalExpression(int index, std::shared_ptr<Environment> env) {
  // A statement that failed to parse holds no expression
  if (index == -1) {
    return Value{.kind = ValueKind::Error, .strValue = "invalid expression"};
  }
  const Expression &expr = parserResult.expressions[index];
  switch (expr.kind) {
  case ExpressionKind::LITERAL_NUMBER:
    return Value{.kind = ValueKind::Number, .numValue = expr.numValue};
  case ExpressionKind::LITERAL_BOOL:
    return Value{.kind = ValueKind::Bool, .boolValue = expr.boolValue};
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
    return Value{.kind = ValueKind::String, .strValue = expr.literal};

  case ExpressionKind::IDENTIFIER: {
    return env->get(expr.literal);
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
    return evalArrayIndex(index, env);
  case ExpressionKind::STAR:
    break;
  }
  return {};
}

Value Evaluator::evalStatement(int index, std::shared_ptr<Environment> env) {
  if (index == -1) {
    return Value{.kind = ValueKind::Error, .strValue = "invalid statement"};
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
    value.numValue = -value.numValue;
    return value;
  }
  if (oper == UnaryOperator::NOT && isBool(value)) {
    value.boolValue = !value.boolValue;
    return value;
  }
  std::string message = "unknown operator: " + unaryOperatorToString(oper) +
                        valueKindToString(value.kind);
  return Value{.kind = ValueKind::Error, .strValue = message};
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
    return Value{.kind = ValueKind::Bool,
                 .boolValue = compare(oper, leftValue, rightValue)};
  case BinaryOperator::AND:
  case BinaryOperator::OR:
    if (isNumeric(leftValue) && isNumeric(rightValue)) {
      // A number is true when nonzero.
      bool left = asNumber(leftValue) != 0;
      bool right = asNumber(rightValue) != 0;
      bool result = oper == BinaryOperator::AND ? left && right : left || right;
      return Value{.kind = ValueKind::Bool, .boolValue = result};
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
        return Value{.kind = ValueKind::Number, .numValue = left + right};
      }

      if (oper == BinaryOperator::SUBTRACT) {
        return Value{.kind = ValueKind::Number, .numValue = left - right};
      }

      if (oper == BinaryOperator::MULTIPLY) {
        return Value{.kind = ValueKind::Number, .numValue = left * right};
      }

      if (oper == BinaryOperator::DIVIDE) {
        return Value{.kind = ValueKind::Number, .numValue = left / right};
      }
    }
    if (oper == BinaryOperator::ADD && isString(leftValue) &&
        isString(rightValue)) {
      return Value{.kind = ValueKind::String,
                   .strValue = leftValue.strValue + rightValue.strValue};
    }

    if (oper == BinaryOperator::ADD && isArray(leftValue) &&
        isArray(rightValue)) {
      auto values = leftValue.values;
      values.insert(values.end(), rightValue.values.begin(),
                    rightValue.values.end());
      return Value{.kind = ValueKind::Array, .values = values};
    }
    break;
  }
  std::string message =
      "unknown operator: " + valueKindToString(leftValue.kind) + " " +
      binaryOperatorToString(oper) + " " + valueKindToString(rightValue.kind);
  return Value{.kind = ValueKind::Error, .strValue = message};
}

Value Evaluator::evalIfStatement(int index, std::shared_ptr<Environment> env) {
  const Statement &stmt = parserResult.statements[index];
  Value conditionValue = evalExpression(stmt.conditionExprIndex, env);
  if (isError(conditionValue)) {
    return conditionValue;
  }
  if (isTruthy(conditionValue)) {
    return evalBlockStatement(stmt.consequenceStmtIndex, env);
  } else if (stmt.alternativeStmtIndex != -1) {
    return evalBlockStatement(stmt.alternativeStmtIndex, env);
  }
  return {};
}

Value Evaluator::evalBlockStatement(int index,
                                    std::shared_ptr<Environment> env) {
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

Value Evaluator::evalVarStatement(int index, std::shared_ptr<Environment> env) {
  const Statement &stmt = parserResult.statements[index];
  // If the variable has no initializer it gets null as value
  if (stmt.expressionIndex < 0) {
    env->set(stmt.name, Value{});
    return {};
  }
  Value val = evalExpression(stmt.expressionIndex, env);
  env->set(stmt.name, val);
  return val;
}

Value Evaluator::evalFunctionExpression(
    const std::string &name, const std::vector<std::string> &parameters,
    int bodyStmtIndex, std::shared_ptr<Environment> env) {
  Value value;
  value.kind = ValueKind::Function;
  value.parameters = parameters;
  value.bodyStmtIndex = bodyStmtIndex;
  value.env = env;
  if (!name.empty()) {
    env->set(name, value);
  }
  return value;
}

std::vector<Value>
Evaluator::evalExpressions(const std::vector<int> &argExprIndexes,
                           std::shared_ptr<Environment> env) {
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
std::shared_ptr<Environment> extendFunctionEnv(Value &function,
                                               std::vector<Value> &args) {
  // Using function.env instead of the caller's env is what makes closures work
  auto env = newEnclosedEnvironment(function.env);
  for (size_t i = 0; i < args.size(); i++) {
    // Map the function parameters to their values from the arguments
    env->set(function.parameters[i], args[i]);
  }
  return env;
}

Value Evaluator::applyFunction(Value &function, std::vector<Value> &args) {
  if (function.parameters.size() != args.size()) {
    std::string message = "wrong number of arguments: got " +
                          std::to_string(args.size()) + ", want " +
                          std::to_string(function.parameters.size());
    return Value{.kind = ValueKind::Error, .strValue = message};
  }
  auto extendedEnv = extendFunctionEnv(function, args);
  Value evaluted = evalStatement(function.bodyStmtIndex, extendedEnv);
  return evaluted;
}

Value Evaluator::evaluateBuiltinFuncs(std::string funcName,
                                      const std::vector<int> &argExprIndexes,
                                      std::shared_ptr<Environment> env) {
  if (funcName == "print") {
    std::vector<Value> args = evalExpressions(argExprIndexes, env);
    if (args.size() == 1 && isError(args[0])) {
      return args[0];
    }
    std::string printedString;
    for (const Value &arg : args) {
      printedString += inspect(arg);
    }
    std::println("{}", printedString);
    return {};
  }
  if (funcName == "len") {
    if (argExprIndexes.size() != 1) {
      std::string message = "wrong number of arguments: got " +
                            std::to_string(argExprIndexes.size()) + ", want 1";
      return Value{.kind = ValueKind::Error, .strValue = message};
    }
    std::vector<Value> args = evalExpressions(argExprIndexes, env);
    if (args.size() == 1 && isError(args[0])) {
      return args[0];
    }
    if (!isString(args[0]) && !isArray(args[0])) {
      std::string message = "argument to len is not supported: " +
                            valueKindToString(args[0].kind);
      return Value{.kind = ValueKind::Error, .strValue = message};
    }
    if (isString(args[0])) {
      return Value{.kind = ValueKind::Number,
                   .numValue = static_cast<double>(args[0].strValue.size())};
    }
    return Value{.kind = ValueKind::Number,
                 .numValue = static_cast<double>(args[0].values.size())};
  }
  return {};
}
Value Evaluator::evalCallExpression(int functionExprIndex,
                                    const std::vector<int> &argExprIndexes,
                                    std::shared_ptr<Environment> env) {
  const std::string funcName =
      parserResult.expressions[functionExprIndex].literal;
  if (isBuiltIn(funcName)) {
    return evaluateBuiltinFuncs(funcName, argExprIndexes, env);
  }
  Value function = env->get(funcName);
  if (isError(function)) {
    return function;
  }
  if (!isFunction(function)) {
    std::string message =
        funcName + " is not a function: " + valueKindToString(function.kind);
    return Value{.kind = ValueKind::Error, .strValue = message};
  }
  std::vector<Value> args = evalExpressions(argExprIndexes, env);
  if (args.size() == 1 && isError(args[0])) {
    return args[0];
  }
  return applyFunction(function, args);
}

Value Evaluator::evalArray(int index, std::shared_ptr<Environment> env) {
  Expression expr = parserResult.expressions[index];
  Value value{.kind = ValueKind::Array};
  for (int indx : expr.expressionsIndexes) {
    value.values.push_back(std::make_shared<Value>(evalExpression(indx, env)));
  }
  return value;
}

Value Evaluator::evalArrayIndex(int index, std::shared_ptr<Environment> env) {
  Expression indexexpr = parserResult.expressions[index];
  Value indexValue = evalExpression(indexexpr.subExprIndex, env);
  if (isError(indexValue)) {
    return indexValue;
  }
  if (!isNumber(indexValue)) {
    return Value{.kind = ValueKind::Error,
                 .strValue = "index must be a number"};
  }
  if (indexValue.numValue < 0) {
    return Value{.kind = ValueKind::Error,
                 .strValue = "index must be greater or equal than zero"};
  }
  size_t arrIndex = indexValue.numValue;
  Value array = env->get(indexexpr.literal);
  if (!isArray(array)) {
    return Value{.kind = ValueKind::Error,
                 .strValue = "variable is not an array"};
  }
  if (arrIndex >= array.values.size()) {
    return Value{.kind = ValueKind::Error,
                 .strValue = "index is bigger than array size"};
  }
  Value value = *array.values[arrIndex];
  return value;
}
