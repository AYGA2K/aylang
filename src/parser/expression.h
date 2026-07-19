#pragma once

#include <string>

enum class ExpressionKind {
  IDENTIFIER,
  LITERAL_NUMBER,
  LITERAL_STRING,
  LITERAL_BOOL,
  BINARY,
  UNARY,
  STAR,
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
  std::string stringValue;
  bool boolValue;
  std::string varName;

  // Binary Expression
  BinaryOperator binaryOperator;
  int leftExprIndex = -1;
  int rightExprIndex = -1;

  // Unary Expression
  UnaryOperator unaryOperator;
  int operandExprIndex = -1;
};
