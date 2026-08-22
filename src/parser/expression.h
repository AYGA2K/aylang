#pragma once

#include <string>
#include <vector>

enum class ExpressionKind {
  IDENTIFIER,
  LITERAL_NUMBER,
  LITERAL_STRING,
  LITERAL_BOOL,
  LITERAL_ARRAY,
  LITERAL_HASH,
  INDEX,
  BINARY,
  UNARY,
  STAR,
  FUNCTION,
  CALL
};

enum class BinaryOperator {
  EQUAL,
  NOT_EQUAL,
  LESS_THAN,
  LESS_THAN_OR_EQUAL,
  GREATER_THAN,
  GREATER_THAN_OR_EQUAL,
  AND,
  OR,
  ADD,
  SUBTRACT,
  MULTIPLY,
  DIVIDE,
};

enum class UnaryOperator {
  NOT,
  NEGATE,
};

// Which fields are meaningful depends on kind.
// Child expressions are referenced by index (-1 means none).
struct Expression {
  ExpressionKind kind;

  double numValue;
  bool boolValue;

  // This is used for variables name, for string values, and for function names
  std::string literal;

  // Binary Expression
  BinaryOperator binaryOperator;
  int leftExprIndex = -1;
  int rightExprIndex = -1;

  // Unary Expression, Index Expression
  UnaryOperator unaryOperator;
  int subExprIndex = -1;

  // Functions
  std::vector<std::string> parameters;
  int bodyStmtIndex;

  // Call expression
  int functionExprIndex = -1;
  // params for functions, values for arrays, and flattened key/value pairs
  // for hashes
  std::vector<int> expressionsIndexes;
};

std::string unaryOperatorToString(UnaryOperator oper);
std::string binaryOperatorToString(BinaryOperator oper);
