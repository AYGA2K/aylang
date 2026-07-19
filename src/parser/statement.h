#pragma once
#include "parser/expression.h"
#include <string>

enum class StatementKind {
  VAR,
  RETURN,
  EXPRESSION,
};

struct Statement {
  StatementKind kind;

  // Var Statement
  std::string name;

  // Index into ParserResult.expressions for the var initializer, return value,
  // or expression statement expression (-1 means none).
  int expressionIndex = -1;
};
