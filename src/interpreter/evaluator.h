#pragma once
#include "environment.h"
#include "parser/expression.h"
#include "parser/parser.h"
#include "value.h"
#include <memory>
#include <vector>

struct Evaluator {
  ParserResult parserResult;
  std::shared_ptr<Environment> globalEnv = std::make_shared<Environment>();
  // Returns the value of the last statement.
  Value evalStatements(size_t fromProgramStatement = 0);
  Value evalStatement(int index, std::shared_ptr<Environment> env);
  Value evalExpression(int index, std::shared_ptr<Environment> env);
  Value evalPrefixExpression(UnaryOperator oper, Value &rightValue);
  Value evalInfixExpression(BinaryOperator oper, const Value &leftValue,
                            const Value &rightValue);
  Value evalIfStatement(int index, std::shared_ptr<Environment> env);
  Value evalBlockStatement(int index, std::shared_ptr<Environment> env);
  Value evalVarStatement(int index, std::shared_ptr<Environment> env);
  Value evalFunctionExpression(const std::string &name,
                               const std::vector<std::string> &parameters,
                               int bodyStmtIndex,
                               std::shared_ptr<Environment> env);

  std::vector<Value> evalExpressions(const std::vector<int> &argExprIndexes,
                                     std::shared_ptr<Environment> env);
  Value evalCallExpression(int functionExprIndex,
                           const std::vector<int> &argExprIndexes,
                           std::shared_ptr<Environment> env);

  Value applyFunction(Value &function, std::vector<Value> &args);
  Value evaluateBuiltinFuncs(std::string funcName,
                             const std::vector<int> &argExprIndexes,
                             std::shared_ptr<Environment> env);
  Value evalArray(int index, std::shared_ptr<Environment> env);
};
