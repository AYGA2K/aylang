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

  // Var initializer, return value, or expression statement expression
  Expression expression;
};
