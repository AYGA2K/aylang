#pragma once
#include "parser/expression.h"
#include "parser/parser.h"
#include "value.h"
#include <cstddef>

struct Evaluator {
  ParserResult parserResult;
  // Returns the value of the last statement.
  Value evalStatements();
  Value evalStatement(size_t index);
  Value evalExpression(size_t index);
  Value evalPrefixExpression(UnaryOperator oper, Value rightValue);
  Value evalInfixExpression(BinaryOperator oper, Value leftValue,
                            Value rightValue);
};
