#include "evaluator.h"
#include "interpreter/value.h"
#include "parser/expression.h"
#include "parser/statement.h"
#include <vector>

Value Evaluator::evalStatements() {
  Value result;
  for (int index : parserResult.programStatementsIndexes) {
    result = evalStatement(index);
    if (result.kind == ValueKind::Error ||
        parserResult.statements[index].kind == StatementKind::RETURN) {
      return result;
    }
  }
  return result;
}

Value Evaluator::evalExpression(int index) {
  const Expression &expr = parserResult.expressions[index];
  switch (expr.kind) {
  case ExpressionKind::LITERAL_NUMBER:
    return Value{.kind = ValueKind::Number, .numValue = expr.numValue};
  case ExpressionKind::LITERAL_BOOL:
    return Value{.kind = ValueKind::Bool, .boolValue = expr.boolValue};
  case ExpressionKind::UNARY: {
    Value value = evalExpression(expr.operandExprIndex);
    return evalPrefixExpression(expr.unaryOperator, value);
  }
  case ExpressionKind::BINARY: {
    Value left = evalExpression(expr.leftExprIndex);
    Value right = evalExpression(expr.rightExprIndex);
    return evalInfixExpression(expr.binaryOperator, left, right);
  }
  case ExpressionKind::IDENTIFIER:
  case ExpressionKind::LITERAL_STRING:
  case ExpressionKind::STAR:
  case ExpressionKind::FUNCTION:
  case ExpressionKind::CALL:
    break;
  }
  return {};
}

Value Evaluator::evalStatement(int index) {
  const Statement &stmt = parserResult.statements[index];
  switch (stmt.kind) {
  case StatementKind::BLOCK:
    return evalBlockStatement(index);
  case StatementKind::IF:
    return evalIfStatement(index);
  case StatementKind::VAR:
  case StatementKind::RETURN:
  case StatementKind::EXPRESSION:
    return evalExpression(stmt.expressionIndex);
    break;
  }
  return {};
}

Value Evaluator::evalPrefixExpression(UnaryOperator oper, Value &value) {
  if (oper == UnaryOperator::NEGATE && value.kind == ValueKind::Number) {
    value.numValue = -value.numValue;
    return value;
  }
  if (oper == UnaryOperator::NOT && value.kind == ValueKind::Bool) {
    value.boolValue = !value.boolValue;
    return value;
  }
  std::string message = "unknown operator: " + unaryOperatorToString(oper) +
                        valueKindToString(value.kind);
  return Value{.kind = ValueKind::Error, .strValue = message};
}

// Booleans compare as numbers: false is 0, true is 1.
static bool isNumeric(const Value &value) {
  return value.kind == ValueKind::Number || value.kind == ValueKind::Bool;
}

static double asNumber(const Value &value) {
  return value.kind == ValueKind::Bool ? value.boolValue : value.numValue;
}

bool compare(BinaryOperator oper, const Value &leftValue,
             const Value &rightValue) {
  if (isNumeric(leftValue) && isNumeric(rightValue)) {
    double left = asNumber(leftValue);
    double right = asNumber(rightValue);
    switch (oper) {
    case BinaryOperator::EQUAL:
      return left == right;
    case BinaryOperator::NOT_EQUAL:
      return left != right;
    case BinaryOperator::LESS_THAN:
      return left < right;
    case BinaryOperator::LESS_THAN_OR_EQUAL:
      return left <= right;
    case BinaryOperator::GREATER_THAN:
      return left > right;
    case BinaryOperator::GREATER_THAN_OR_EQUAL:
      return left >= right;
    default:
      break;
    }
    return false;
  }
  if (leftValue.kind != rightValue.kind) {
    return false;
  }
  if (leftValue.kind == ValueKind::Str) {
    return leftValue.strValue == rightValue.strValue;
  }
  if (leftValue.kind == ValueKind::Null) {
    return true;
  }
  return false;
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
      // A number is true when nonzero.
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
    break;
  }
  std::string message =
      "unknown operator: " + valueKindToString(leftValue.kind) + " " +
      binaryOperatorToString(oper) + " " + valueKindToString(rightValue.kind);
  return Value{.kind = ValueKind::Error, .strValue = message};
}

bool isTruthy(const Value &value) {
  if (value.kind == ValueKind::Null) {
    return false;
  }
  if (value.kind == ValueKind::Bool && !value.boolValue) {
    return false;
  }
  return true;
}

Value Evaluator::evalIfStatement(int index) {
  const Statement &stmt = parserResult.statements[index];
  Value conditionValue = evalExpression(stmt.conditionExprIndex);
  if (conditionValue.kind == ValueKind::Error) {
    return conditionValue;
  }
  if (isTruthy(conditionValue)) {
    return evalBlockStatement(stmt.consequenceStmtIndex);
  } else if (stmt.alternativeStmtIndex != -1) {
    return evalBlockStatement(stmt.alternativeStmtIndex);
  }
  return {};
}

Value Evaluator::evalBlockStatement(int index) {
  const Statement &stmt = parserResult.statements[index];
  Value returnedValue;
  for (int index : stmt.statementsIndexes) {
    returnedValue = evalStatement(index);
    if (returnedValue.kind == ValueKind::Error ||
        parserResult.statements[index].kind == StatementKind::RETURN) {
      return returnedValue;
    }
  }
  return returnedValue;
}
