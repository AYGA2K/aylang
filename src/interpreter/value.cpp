#include "value.h"
#include <cstddef>
#include <format>
#include <string>

static std::string buildArrString(const Value &value) {
  std::string format = "[";
  for (size_t i = 0; i < value.values.size(); i++) {
    if (i > 0) {
      format += ",";
    }
    format += inspect(*value.values[i]);
  }
  format += "]";
  return format;
}
static std::string buildHashMapString(const Value &value){
    std::string format = "{";
    for (size_t indx = 0; indx<value.values.size(); indx+=2) {
        format += inspect(*value.values[indx]);
        format += ":";
        format += inspect(*value.values[indx+1]);
        if (indx < value.values.size() - 2) {
            format += ",";
        }
    }
    format += "}";
    return format;
}
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
  case ValueKind::Array:
    return buildArrString(value);
  case ValueKind::BUILTIN:
  case ValueKind::Function:
    break;
  case ValueKind::HashMap:
      return buildHashMapString(value);
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
  case ValueKind::Array:
    return "Array";
  case ValueKind::HashMap:
      return "HashMap";
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
  if (isString(leftValue)) {
    return compareOrdered(oper, leftValue.strValue, rightValue.strValue);
  }
  if (isNull(leftValue)) {
    return true;
  }
  return false;
}

bool isTruthy(const Value &value) {
  if (isNull(value)) {
    return false;
  }
  if (isBool(value) && !value.boolValue) {
    return false;
  }
  return true;
}

bool isError(const Value &value) { return value.kind == ValueKind::Error; }

bool isNumber(const Value &value) { return value.kind == ValueKind::Number; }

bool isString(const Value &value) { return value.kind == ValueKind::String; }

bool isBool(const Value &value) { return value.kind == ValueKind::Bool; }

bool isNull(const Value &value) { return value.kind == ValueKind::Null; }

bool isArray(const Value &value) { return value.kind == ValueKind::Array; }

bool isFunction(const Value &value) {
  return value.kind == ValueKind::Function;
}
