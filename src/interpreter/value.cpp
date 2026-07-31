#include "value.h"
#include <format>
#include <string>

std::string inspect(const Value &value) {
  switch (value.kind) {
  case ValueKind::Number:
    return std::format("{}", value.numValue);
  case ValueKind::Str:
    return value.strValue;
  case ValueKind::Bool:
    return value.boolValue ? "true" : "false";
  case ValueKind::Null:
    return "null";
  }
  return "null";
}
