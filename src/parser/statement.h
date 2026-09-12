#pragma once
#include <string>
#include <vector>

enum class StatementKind {
  LET,
  RETURN,
  EXPRESSION,
  BLOCK,
  IF,
  WHILE,
};

struct Statement {
  StatementKind kind;

  // Let Statement
  std::string name;

  // Index into ParserResult.expressions for the let initializer, return value,
  // or expression statement expression (-1 means none).
  int expressionIndex = -1;

  // Block statements indexes
  std::vector<int> statementsIndexes;

  // If Statement, and the condition of a While Statement
  int conditionExprIndex = -1;
  int consequenceStmtIndex = -1;
  int alternativeStmtIndex = -1;

  // While Statement
  int bodyStmtIndex = -1;
};
