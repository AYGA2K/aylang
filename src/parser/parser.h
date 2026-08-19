#pragma once

#include "lexer/token.h"
#include "parser/expression.h"
#include "parser/statement.h"
#include <cstddef>
#include <functional>
#include <unordered_map>
#include <vector>

enum class Precedence {
  LOWEST,
  EQUALS,      // ==
  LESSGREATER, // > or <
  SUM,         // +
  PRODUCT,     // *
  UNARY,       // -X or !X
  CALL,        // myFunction(X)
  INDEX        // array[index]
};

// Expression parse functions store their node in ParserResult.expressions and
// return its index (-1 means none). An infix function receives the index of its
// already parsed left operand.
using PrefixParseFn = std::function<int()>;
using InfixParseFn = std::function<int(int)>;

struct ParserResult {
  std::vector<Statement> statements;
  std::vector<Expression> expressions;
  // Indexes of statements which will be evaluated: block statements are not
  // included
  std::vector<int> programStatementsIndexes;
};

struct Parser {
  std::vector<Token> &tokens;
  size_t current = 0;
  ParserResult parserResult;
  std::vector<std::string> errors;
  // Pratt parse functions, looked up by the token type they start with
  std::unordered_map<TokenType, PrefixParseFn> prefixFns;
  std::unordered_map<TokenType, InfixParseFn> infixFns;

  Parser(std::vector<Token> &tokens) : tokens(tokens) {
    registerPrefix(TokenType::Identifier, [this] { return parseIdentifier(); });
    registerPrefix(TokenType::Number, [this] { return parseNumber(); });
    registerPrefix(TokenType::String, [this] { return parseString(); });
    registerPrefix(TokenType::False, [this] { return parseBoolean(); });
    registerPrefix(TokenType::True, [this] { return parseBoolean(); });
    registerPrefix(TokenType::LParen,
                   [this] { return parseGroupedExpression(); });

    registerPrefix(TokenType::LBrack, [this] { return parseArray(); });

    registerPrefix(TokenType::Function, [this] { return parseFunction(); });

    auto unary = [this] { return parseUnary(); };
    registerPrefix(TokenType::BANG, unary);
    registerPrefix(TokenType::Minus, unary);

    auto binary = [this](int leftExprIndex) {
      return parseBinary(leftExprIndex);
    };
    registerInfix(TokenType::Plus, binary);
    registerInfix(TokenType::Minus, binary);
    registerInfix(TokenType::Slash, binary);
    registerInfix(TokenType::Star, binary);
    registerInfix(TokenType::Equal, binary);
    registerInfix(TokenType::NotEqual, binary);
    registerInfix(TokenType::LessThan, binary);
    registerInfix(TokenType::GreaterThan, binary);
    registerInfix(TokenType::LessThanOrEqual, binary);
    registerInfix(TokenType::GreaterThanOrEqual, binary);

    registerInfix(TokenType::LParen, [this](int leftExprIndex) {
      return parseCallExpression(leftExprIndex);
    });

    registerInfix(TokenType::LBrack, [this](int leftExprIndex) {
      return parseIndexExpression(leftExprIndex);
    });
  }
  Token currentToken();
  Token nextToken();
  bool nextTokenIs(TokenType type);
  bool currentTokenIs(TokenType type);
  Precedence currentPrecedence();
  Precedence nextPrecedence();
  void parse();
  int parseStatement();
  void parseVarStatement();
  void parseReturnStatement();
  void parseExpressionStatement();
  int parseExpression(Precedence precedence);
  int parseIdentifier();
  int parseNumber();
  int parseString();
  int parseUnary();
  int parseBinary(int leftExprIndex);
  int parseBoolean();
  int parseGroupedExpression();
  int parseIndexExpression(int leftExprIndex);
  int parseIfStatement();
  int parseBlockStatement();
  int parseFunction();
  int parseArray();
  std::vector<int> parseArrayValues();
  std::vector<std::string> parseFunctionParams();
  int parseCallExpression(int leftExprIndex);
  std::vector<int> parseCallParams();
  void registerPrefix(TokenType tokenType, PrefixParseFn fn);
  void registerInfix(TokenType tokenType, InfixParseFn fn);
};

std::string expectedTokenError(TokenType expected, TokenType got);
