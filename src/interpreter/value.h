#pragma once
#include "parser/expression.h"
#include <memory>
#include <string>
#include <vector>

enum class ValueKind {
  Number,
  String,
  Bool,
  Null,
  Error,
  Function,
  BUILTIN,
  Array,
  HashMap
};
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
  // Array
  std::vector<std::shared_ptr<Value>> values =
      std::vector<std::shared_ptr<Value>>();
};

std::string inspect(const Value &value);
std::string valueKindToString(ValueKind kind);
bool isNumeric(const Value &value);
double asNumber(const Value &value);
bool compare(BinaryOperator oper, const Value &leftValue,
             const Value &rightValue);
bool isTruthy(const Value &value);
bool isError(const Value &value);
bool isNumber(const Value &value);
bool isString(const Value &value);
bool isBool(const Value &value);
bool isNull(const Value &value);
bool isArray(const Value &value);
bool isFunction(const Value &value);
