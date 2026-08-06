#include "parser/expression.h"

std::string unaryOperatorToString(UnaryOperator oper) {
  switch (oper) {
  case UnaryOperator::NOT:
    return "!";
  case UnaryOperator::NEGATE:
    return "-";
  }
  return "";
}

std::string binaryOperatorToString(BinaryOperator oper) {
  switch (oper) {
  case BinaryOperator::EQUAL:
    return "==";
  case BinaryOperator::NOT_EQUAL:
    return "!=";
  case BinaryOperator::LESS_THAN:
    return "<";
  case BinaryOperator::LESS_THAN_OR_EQUAL:
    return "<=";
  case BinaryOperator::GREATER_THAN:
    return ">";
  case BinaryOperator::GREATER_THAN_OR_EQUAL:
    return ">=";
  case BinaryOperator::AND:
    return "&&";
  case BinaryOperator::OR:
    return "||";
  case BinaryOperator::ADD:
    return "+";
  case BinaryOperator::SUBTRACT:
    return "-";
  case BinaryOperator::MULTIPLY:
    return "*";
  case BinaryOperator::DIVIDE:
    return "/";
  }
  return "";
}
