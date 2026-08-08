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
  case ExpressionKind::LITERAL_STRING:
    return Value{.kind = ValueKind::String, .strValue = expr.literal};
  case ExpressionKind::IDENTIFIER: {
    return environment.get(expr.literal);
  }
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
    return evalVarStatement(index);
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
    if (oper == BinaryOperator::ADD && leftValue.kind == ValueKind::String &&
        rightValue.kind == ValueKind::String) {
      return Value{.kind = ValueKind::String,
                   .strValue = leftValue.strValue + rightValue.strValue};
    }
    break;
  }
  std::string message =
      "unknown operator: " + valueKindToString(leftValue.kind) + " " +
      binaryOperatorToString(oper) + " " + valueKindToString(rightValue.kind);
  return Value{.kind = ValueKind::Error, .strValue = message};
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

Value Evaluator::evalVarStatement(int index) {
  const Statement &stmt = parserResult.statements[index];
  // If the variable has no initializer it gets null as value
  if (stmt.expressionIndex < 0) {
    environment.set(stmt.name, Value{});
    return {};
  }
  Value val = evalExpression(stmt.expressionIndex);
  environment.set(stmt.name, val);
  return val;
}
