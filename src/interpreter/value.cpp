#include "value.h"
#include <format>
#include <string>

std::string inspect(const Value &value) {
  switch (value.kind) {
  case ValueKind::Number:
    return std::format("{}", value.numValue);
  case ValueKind::Str:
  case ValueKind::Error:
    return value.strValue;
  case ValueKind::Bool:
    return value.boolValue ? "true" : "false";
  case ValueKind::Null:
    return "null";
    break;
  }
  return "null";
}

std::string valueKindToString(ValueKind kind) {
  switch (kind) {
  case ValueKind::Number:
    return "Number";
  case ValueKind::Str:
    return "Str";
  case ValueKind::Bool:
    return "Bool";
  case ValueKind::Null:
    return "Null";
  case ValueKind::Error:
    return "Error";
  }
  return "Unknown";
}
