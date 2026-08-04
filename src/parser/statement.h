#pragma once
#include <string>
#include <vector>

enum class StatementKind {
  VAR,
  RETURN,
  EXPRESSION,
  BLOCK,
  IF,
};

struct Statement {
  StatementKind kind;

  // Var Statement
  std::string name;

  // Index into ParserResult.expressions for the var initializer, return value,
  // or expression statement expression (-1 means none).
  int expressionIndex = -1;

  // Block statements indexes
  std::vector<int> statementsIndexes;

  // If Statement
  int conditionExprIndex = -1;
  int consequenceStmtIndex = -1;
  int alternativeStmtIndex = -1;
};
