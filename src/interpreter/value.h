#pragma once
#include <string>

enum class ValueKind { Number, Str, Bool, Null, Error };

struct Value {
  ValueKind kind = ValueKind::Null;
  double numValue = 0;
  std::string strValue = "";
  bool boolValue = false;
};

std::string inspect(const Value &value);
std::string valueKindToString(ValueKind kind);
