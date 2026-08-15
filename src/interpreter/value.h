#pragma once
#include "parser/expression.h"
#include <memory>
#include <string>
#include <vector>

enum class ValueKind { Number, String, Bool, Null, Error, Function, BUILTIN };
struct Environment;

struct Value {
  ValueKind kind = ValueKind::Null;
  double numValue = 0;
  std::string strValue = "";
  bool boolValue = false;
  // Function
  std::vector<std::string> parameters = std::vector<std::string>();
  int bodyStmtIndex = -1;
  std::shared_ptr<Environment> env = nullptr;
};

std::string inspect(const Value &value);
std::string valueKindToString(ValueKind kind);
bool isNumeric(const Value &value);
double asNumber(const Value &value);
bool compare(BinaryOperator oper, const Value &leftValue,
             const Value &rightValue);
bool isTruthy(const Value &value);
bool isError(const Value &value);
