#pragma once
#include "parser/expression.h"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

// A Value is either a null, a number or a bool stored in the Value itself, or a
// pointer to a heap object. tag says which one, and Obj::kind says which kind
// of object.
enum class Tag : uint8_t { Null, Number, Bool, Obj };
enum class ObjKind : uint8_t { String, Error, Array, HashMap, Function, Env };

// The flat view of a value's type, holding the names that appear in error
// messages. kindOf() folds Tag and ObjKind into it.
enum class ValueKind {
  Number,
  String,
  Bool,
  Null,
  Error,
  Function,
  Array,
  HashMap
};

struct Obj;

struct Value {
  Tag tag = Tag::Null;
  union {
    double num;
    bool boolean;
    Obj *obj = nullptr;
  };
};

struct Obj {
  ObjKind kind;
  bool marked;
  Obj *next = nullptr;
};

struct ObjString : Obj {
  std::string chars;
};
struct ObjError : Obj {
  std::string message;
};
struct ObjArray : Obj {
  std::vector<Value> items;
};

// entries[i] is a key and entries[i + 1] is the value it maps to.
struct ObjHashMap : Obj {
  std::vector<Value> entries;
};
struct ObjEnv : Obj {
  std::unordered_map<std::string, Value> store;
  ObjEnv *outer = nullptr;
};
struct ObjFunction : Obj {
  std::vector<std::string> parameters;
  int bodyStmtIndex = -1;
  ObjEnv *env = nullptr;
};

inline Value makeNull() { return Value{}; }
inline Value makeNumber(double n) {
  Value v;
  v.tag = Tag::Number;
  v.num = n;
  return v;
}
inline Value makeBool(bool b) {
  Value v;
  v.tag = Tag::Bool;
  v.boolean = b;
  return v;
}
inline Value makeObj(Obj *o) {
  Value v;
  v.tag = Tag::Obj;
  v.obj = o;
  return v;
}

Value makeString(std::string chars);
Value makeError(std::string message);
Value makeArray(std::vector<Value> items);
Value makeHashMap(std::vector<Value> entries);
Value makeFunction(const std::vector<std::string> &parameters,
                   int bodyStmtIndex, ObjEnv *env);

inline bool isObjKind(const Value &v, ObjKind k) {
  return v.tag == Tag::Obj && v.obj->kind == k;
}
inline bool isNumber(const Value &v) { return v.tag == Tag::Number; }
inline bool isBool(const Value &v) { return v.tag == Tag::Bool; }
inline bool isNull(const Value &v) { return v.tag == Tag::Null; }
inline bool isString(const Value &v) { return isObjKind(v, ObjKind::String); }
inline bool isError(const Value &v) { return isObjKind(v, ObjKind::Error); }
inline bool isArray(const Value &v) { return isObjKind(v, ObjKind::Array); }
inline bool isHashMap(const Value &v) { return isObjKind(v, ObjKind::HashMap); }
inline bool isFunction(const Value &v) {
  return isObjKind(v, ObjKind::Function);
}

inline ObjString *asString(const Value &v) {
  return static_cast<ObjString *>(v.obj);
}
inline ObjError *asError(const Value &v) {
  return static_cast<ObjError *>(v.obj);
}
inline ObjArray *asArray(const Value &v) {
  return static_cast<ObjArray *>(v.obj);
}
inline ObjHashMap *asHashMap(const Value &v) {
  return static_cast<ObjHashMap *>(v.obj);
}
inline ObjFunction *asFunction(const Value &v) {
  return static_cast<ObjFunction *>(v.obj);
}

ValueKind kindOf(const Value &value);
std::string inspect(const Value &value);
std::string valueKindToString(ValueKind kind);
bool isNumeric(const Value &value);
double asNumber(const Value &value);
bool compare(BinaryOperator oper, const Value &leftValue,
             const Value &rightValue);
bool isTruthy(const Value &value);
