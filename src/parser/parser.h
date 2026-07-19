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
  CALL         // myFunction(X)
};

// Expression parse functions store their node in ParserResult.expressions and
// return its index (-1 means none). A binary function receives the index of its
// already parsed left operand.
using UnaryParseFn = std::function<int()>;
using BinaryParseFn = std::function<int(int)>;

struct ParserResult {
  std::vector<Statement> statements;
  std::vector<Expression> expressions;
};

struct Parser {
  std::vector<Token> &tokens;
  size_t current = 0;
  ParserResult parserResult;
  std::vector<std::string> errors;
  // Pratt parse functions, looked up by the token type they start with
  std::unordered_map<TokenType, UnaryParseFn> unaryFns;
  std::unordered_map<TokenType, BinaryParseFn> binaryFns;

  Parser(std::vector<Token> &tokens) : tokens(tokens) {
    registerUnary(TokenType::Identifier, [this] { return parseIdentifier(); });
    registerUnary(TokenType::Number, [this] { return parseNumber(); });

    auto unary = [this] { return parseUnary(); };
    registerUnary(TokenType::BANG, unary);
    registerUnary(TokenType::Minus, unary);

    auto binary = [this](int leftExprIndex) {
      return parseBinary(leftExprIndex);
    };
    registerBinary(TokenType::Plus, binary);
    registerBinary(TokenType::Minus, binary);
    registerBinary(TokenType::Slash, binary);
    registerBinary(TokenType::Star, binary);
    registerBinary(TokenType::Equal, binary);
    registerBinary(TokenType::NotEqual, binary);
    registerBinary(TokenType::LessThan, binary);
    registerBinary(TokenType::GreaterThan, binary);
  }
  Token currentToken();
  Token nextToken();
  bool nextTokenIs(TokenType type);
  bool currentTokenIs(TokenType type);
  Precedence currentPrecendence();
  Precedence nextPrecendence();
  void parse();
  void parseVarStatement();
  void parseExpressionStatment();
  int parseExpression(Precedence precedence);
  int parseIdentifier();
  int parseNumber();
  int parseUnary();
  int parseBinary(int leftExprIndex);
  void registerUnary(TokenType unaryType, UnaryParseFn fn);
  void registerBinary(TokenType binaryType, BinaryParseFn fn);
};

std::string expectedTokenError(TokenType expected, TokenType got);
