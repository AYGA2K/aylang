#pragma once
#include "environment.h"
#include "parser/expression.h"
#include "parser/parser.h"
#include "value.h"
#include <vector>

struct Evaluator {
  ParserResult parserResult;
  ObjEnv *globalEnv = newEnvironment();
  bool returning = false; // Set by a return statement and cleared once the call
                          // it belongs to ends.
  // Returns the value of the last statement.
  Value evalStatements(size_t fromProgramStatement = 0);
  Value evalStatement(int index, ObjEnv *env);
  Value evalExpression(int index, ObjEnv *env);
  Value evalPrefixExpression(UnaryOperator oper, Value &rightValue);
  Value evalInfixExpression(BinaryOperator oper, const Value &leftValue,
                            const Value &rightValue);
  Value evalIfStatement(int index, ObjEnv *env);
  Value evalBlockStatement(int index, ObjEnv *env);
  Value evalLetStatement(int index, ObjEnv *env);
  Value evalFunctionExpression(const std::string &name,
                               const std::vector<std::string> &parameters,
                               int bodyStmtIndex, ObjEnv *env);

  std::vector<Value> evalExpressions(const std::vector<int> &argExprIndexes,
                                     ObjEnv *env);
  Value evalCallExpression(int functionExprIndex,
                           const std::vector<int> &argExprIndexes, ObjEnv *env);

  Value applyFunction(Value &function, std::vector<Value> &args);
  Value evalBuiltinFuncs(std::string funcName,
                         const std::vector<int> &argExprIndexes, ObjEnv *env);
  Value evalPrint(const std::vector<int> &argExprIndexes, ObjEnv *env);
  Value evalLen(const std::vector<int> &argExprIndexes, ObjEnv *env);
  Value evalPush(const std::vector<int> &argExprIndexes, ObjEnv *env);
  Value evalSet(const std::vector<int> &argExprIndexes, ObjEnv *env);
  Value lookupVariableArg(const std::string &funcName, int argExprIndex,
                          ObjEnv *env);

  Value evalArray(int index, ObjEnv *env);

  Value evalHashMap(int index, ObjEnv *env);

  Value evalIndex(int index, ObjEnv *env);

  Value evalArrayIndex(const Value &array, const Value &indexValue);

  Value evalHashMapIndex(const Value &hashMap, const Value &keyValue);
};
