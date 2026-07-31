#pragma once
#include <string>

enum class ValueKind {
  Number,
  Str,
  Bool,
  Null,
};

struct Value {
  ValueKind kind = ValueKind::Null;
  double numValue = 0;
  std::string strValue = "";
  bool boolValue = false;
};

std::string inspect(const Value &value);
