
#include "parser/parser.h"
#include "lexer/token.h"
#include "parser/expression.h"
#include "parser/statement.h"
#include <string>
#include <vector>

bool Parser::currentTokenIs(TokenType type) {
  return currentToken().type == type;
}

bool Parser::nextTokenIs(TokenType type) { return nextToken().type == type; }

Token Parser::currentToken() {
  if (current >= tokens.size() ||
      tokens[current].type == TokenType::EndOfFile) {
    return Token{.type = TokenType::EndOfFile, .literal = ""};
  }
  return tokens[current];
}

Token Parser::nextToken() {
  if (current + 1 >= tokens.size() ||
      tokens[current + 1].type == TokenType::EndOfFile) {
    return Token{.type = TokenType::EndOfFile, .literal = ""};
  }
  return tokens[current + 1];
}

std::vector<Statement> Parser::parse() {
  while (currentToken().type != TokenType::EndOfFile) {
    switch (currentToken().type) {
    case TokenType::Var:
      parseVarStatement();
      break;
    case TokenType::If:
    case TokenType::Else:
    case TokenType::While:
    case TokenType::Return:
      // Not implemented yet
      break;
    default:
      parseExpressionStatment();
      break;
    }
    current++;
  }
  return parserResult.statements;
}

Statement Parser::parseExpressionStatment() {
  Statement statement;
  statement.kind = StatementKind::EXPRESSION;
  statement.expression = parseExpression(Precedence::LOWEST);
  if (nextTokenIs(TokenType::Semicolon)) {
    current++;
  }
  parserResult.statements.push_back(statement);
  return statement;
}

// Parses "var name;" or "var name = expression;"
Statement Parser::parseVarStatement() {
  current++; // skip "var"
  Statement statement;
  statement.kind = StatementKind::VAR;
  if (!currentTokenIs(TokenType::Identifier)) {
    errors.push_back(
        expectedTokenError(TokenType::Identifier, currentToken().type));
    return statement;
  }
  statement.name = currentToken().literal;
  current++;
  // Declaration without initializer: "var name;"
  if (currentTokenIs(TokenType::Semicolon)) {
    parserResult.statements.push_back(statement);
    return statement;
  }
  if (!currentTokenIs(TokenType::Assign)) {
    errors.push_back(
        expectedTokenError(TokenType::Assign, currentToken().type));
    return statement;
  }
  current++; // skip "="
  statement.expression = parseExpression(Precedence::LOWEST);
  if (nextTokenIs(TokenType::Semicolon)) {
    current++;
  }
  parserResult.statements.push_back(statement);
  return statement;
}

Expression Parser::parseExpression(Precedence precedence) {
  auto prefix = prefixFns[currentToken().type];
  if (!prefix) {
    return {};
  }
  auto leftExpt = prefix();
  return *leftExpt;
}

Expression Parser::parseIdentifier() {
  Expression expression;
  expression.kind = ExpressionKind::IDENTIFIER;
  expression.stringValue = currentToken().literal;
  return expression;
}

Expression Parser::parseNumber() {
  Expression expression;
  expression.kind = ExpressionKind::LITERAL_NUMBER;
  expression.numValue = std::stod(currentToken().literal);
  return expression;
}

// Parses "!operand" or "-operand"
Expression Parser::parsePrefix() {
  Expression expression;
  expression.kind = ExpressionKind::PREFIX;
  if (currentToken().type == TokenType::BANG) {
    expression.unaryOperator = UnaryOperator::NOT;
  } else if (currentToken().type == TokenType::Minus) {
    expression.unaryOperator = UnaryOperator::NEGATE;
  }
  current++; // skip the operator
  // Child expressions live in parserResult.expressions, referenced by index
  Expression rightExpr = parseExpression(Precedence::PREFIX);
  expression.operandExprIndex = parserResult.expressions.size();
  parserResult.expressions.push_back(rightExpr);
  return expression;
}

std::string expectedTokenError(TokenType expected, TokenType got) {
  return "Expected next token to be " + tokenTypeToString(expected) + ", got " +

         tokenTypeToString(got);
}

void Parser::registerPrefix(TokenType prefixType, PrefixParseFn fn) {
  prefixFns[prefixType] = fn;
}

void Parser::registerInfix(TokenType infixType, InfixParseFn fn) {
  infixFns[infixType] = fn;
}
