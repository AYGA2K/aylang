#include "value.h"
#include "garbage-colector/gc.h"
#include <cstddef>
#include <format>
#include <string>
#include <utility>
#include <vector>

Value makeString(std::string chars) {
  ObjString *obj = allocateObj<ObjString>(ObjKind::String);
  obj->chars = std::move(chars);
  return makeObj(obj);
}

Value makeError(std::string message) {
  ObjError *obj = allocateObj<ObjError>(ObjKind::Error);
  obj->message = std::move(message);
  return makeObj(obj);
}

Value makeArray(std::vector<Value> items) {
  ObjArray *obj = allocateObj<ObjArray>(ObjKind::Array);
  obj->items = std::move(items);
  return makeObj(obj);
}

Value makeHashMap(std::vector<Value> entries) {
  ObjHashMap *obj = allocateObj<ObjHashMap>(ObjKind::HashMap);
  obj->entries = std::move(entries);
  return makeObj(obj);
}

Value makeFunction(const std::vector<std::string> &parameters,
                   int bodyStmtIndex, ObjEnv *env) {
  ObjFunction *obj = allocateObj<ObjFunction>(ObjKind::Function);
  obj->parameters = parameters;
  obj->bodyStmtIndex = bodyStmtIndex;
  obj->env = env;
  return makeObj(obj);
}

void hashMapSet(ObjHashMap *hashMap, const Value &key, const Value &value) {
  std::vector<Value> &entries = hashMap->entries;
  for (size_t indx = 0; indx + 1 < entries.size(); indx += 2) {
    if (compare(BinaryOperator::EQUAL, entries[indx], key)) {
      entries[indx + 1] = value;
      return;
    }
  }
  entries.push_back(key);
  entries.push_back(value);
}

ValueKind kindOf(const Value &value) {
  switch (value.tag) {
  case Tag::Null:
    return ValueKind::Null;
  case Tag::Number:
    return ValueKind::Number;
  case Tag::Bool:
    return ValueKind::Bool;
  case Tag::Obj:
    break;
  }
  switch (value.obj->kind) {
  case ObjKind::String:
    return ValueKind::String;
  case ObjKind::Error:
    return ValueKind::Error;
  case ObjKind::Array:
    return ValueKind::Array;
  case ObjKind::HashMap:
    return ValueKind::HashMap;
  case ObjKind::Function:
    return ValueKind::Function;
  case ObjKind::Env:
    break;
  }
  return ValueKind::Null;
}

static std::string buildArrString(const Value &value) {
  const std::vector<Value> &items = asArray(value)->items;
  std::string format = "[";
  for (size_t i = 0; i < items.size(); i++) {
    if (i > 0) {
      format += ",";
    }
    format += inspect(items[i]);
  }
  format += "]";
  return format;
}

static std::string buildHashMapString(const Value &value) {
  const std::vector<Value> &entries = asHashMap(value)->entries;
  std::string format = "{";
  for (size_t indx = 0; indx < entries.size(); indx += 2) {
    format += inspect(entries[indx]);
    format += ":";
    format += inspect(entries[indx + 1]);
    if (indx < entries.size() - 2) {
      format += ",";
    }
  }
  format += "}";
  return format;
}

std::string inspect(const Value &value) {
  switch (kindOf(value)) {
  case ValueKind::Number:
    return std::format("{}", value.num);
  case ValueKind::String:
    return asString(value)->chars;
  case ValueKind::Error:
    return asError(value)->message;
  case ValueKind::Bool:
    return value.boolean ? "true" : "false";
  case ValueKind::Null:
    return "null";
  case ValueKind::Array:
    return buildArrString(value);
  case ValueKind::Function:
    break;
  case ValueKind::HashMap:
    return buildHashMapString(value);
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
    return "Function";
  case ValueKind::Array:
    return "Array";
  case ValueKind::HashMap:
    return "HashMap";
  }
  return "Unknown";
}

// Booleans compare as numbers: false is 0, true is 1.
bool isNumeric(const Value &value) { return isNumber(value) || isBool(value); }

double asNumber(const Value &value) {
  return isBool(value) ? value.boolean : value.num;
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
  if (kindOf(leftValue) != kindOf(rightValue)) {
    return false;
  }
  if (isString(leftValue)) {
    return compareOrdered(oper, asString(leftValue)->chars,
                          asString(rightValue)->chars);
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
  if (isBool(value) && !value.boolean) {
    return false;
  }
  return true;
}
