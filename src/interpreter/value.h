#pragma once
#include "parser/expression.h"
#include <string>

enum class ValueKind { Number, String, Bool, Null, Error };

struct Value {
  ValueKind kind = ValueKind::Null;
  double numValue = 0;
  std::string strValue = "";
  bool boolValue = false;
};

std::string inspect(const Value &value);
std::string valueKindToString(ValueKind kind);
bool isNumeric(const Value &value);
double asNumber(const Value &value);
bool compare(BinaryOperator oper, const Value &leftValue,
             const Value &rightValue);
bool isTruthy(const Value &value);
