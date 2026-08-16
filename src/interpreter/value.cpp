#include "value.h"
#include <format>
#include <string>

std::string inspect(const Value &value) {
  switch (value.kind) {
  case ValueKind::Number:
    return std::format("{}", value.numValue);
  case ValueKind::String:
  case ValueKind::Error:
    return value.strValue;
  case ValueKind::Bool:
    return value.boolValue ? "true" : "false";
  case ValueKind::Null:
    return "null";
    break;
  case ValueKind::BUILTIN:
  case ValueKind::Function:
    break;
  }
  return "null";
}

std::string valueKindToString(ValueKind kind) {
  switch (kind) {
  case ValueKind::Number:
    return "Number";
  case ValueKind::String:
    return "Str";
  case ValueKind::Bool:
    return "Bool";
  case ValueKind::Null:
    return "Null";
  case ValueKind::Error:
    return "Error";
  case ValueKind::Function:
  case ValueKind::BUILTIN:
    return "Function";
    break;
  }
  return "Unknown";
}

// Booleans compare as numbers: false is 0, true is 1.
bool isNumeric(const Value &value) {
  return value.kind == ValueKind::Number || value.kind == ValueKind::Bool;
}

double asNumber(const Value &value) {
  return value.kind == ValueKind::Bool ? value.boolValue : value.numValue;
}

template <typename T>
static bool compareOrdered(BinaryOperator oper, const T &left, const T &right) {
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
    return false;
  }
}

bool compare(BinaryOperator oper, const Value &leftValue,
             const Value &rightValue) {
  if (isNumeric(leftValue) && isNumeric(rightValue)) {
    return compareOrdered(oper, asNumber(leftValue), asNumber(rightValue));
  }
  if (leftValue.kind != rightValue.kind) {
    return false;
  }
  if (leftValue.kind == ValueKind::String) {
    return compareOrdered(oper, leftValue.strValue, rightValue.strValue);
  }
  if (leftValue.kind == ValueKind::Null) {
    return true;
  }
  return false;
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

bool isError(const Value &value) { return value.kind == ValueKind::Error; }
