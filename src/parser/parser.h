#pragma once

#include "lexer/token.h"
#include "parser/expression.h"
#include "parser/statement.h"
#include <cstddef>
#include <functional>
#include <memory>
#include <vector>

enum class Precedence {
  LOWEST,
  EQUALS,      // ==
  LESSGREATER, // > or <
  SUM,         // +
  PRODUCT,     // *
  PREFIX,      // -X or !X
  CALL         // myFunction(X)
};

using ExprPtr = std::unique_ptr<Expression>;

using PrefixParseFn = std::function<ExprPtr()>;
using InfixParseFn = std::function<ExprPtr(ExprPtr)>;

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
  std::unordered_map<TokenType, PrefixParseFn> prefixFns;
  std::unordered_map<TokenType, InfixParseFn> infixFns;

  Parser(std::vector<Token> &tokens) : tokens(tokens) {
    registerPrefix(TokenType::Identifier, [this] {
      return std::make_unique<Expression>(parseIdentifier());
    });
    registerPrefix(TokenType::Number, [this] {
      return std::make_unique<Expression>(parseNumber());
    });

    registerPrefix(TokenType::BANG, [this] {
      return std::make_unique<Expression>(parsePrefix());
    });

    registerPrefix(TokenType::Minus, [this] {
      return std::make_unique<Expression>(parsePrefix());
    });
  }
  Token currentToken();
  Token nextToken();
  bool nextTokenIs(TokenType type);
  bool currentTokenIs(TokenType type);
  std::vector<Statement> parse();
  Statement parseVarStatement();
  Statement parseExpressionStatment();
  Expression parseExpression(Precedence precedence);
  Expression parseIdentifier();
  Expression parseNumber();
  Expression parsePrefix();
  void registerPrefix(TokenType prefixType, PrefixParseFn fn);
  void registerInfix(TokenType InfixType, InfixParseFn fn);
};

std::string expectedTokenError(TokenType expected, TokenType got);
