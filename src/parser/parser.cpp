#include "parser/parser.h"
#include "lexer/token.h"
#include "parser/expression.h"
#include "parser/statement.h"
#include <string>
#include <vector>

std::unordered_map<TokenType, Precedence> precedences{
    {TokenType::Equal, Precedence::EQUALS},
    {TokenType::NotEqual, Precedence::EQUALS},
    {TokenType::LessThan, Precedence::LESSGREATER},
    {TokenType::GreaterThan, Precedence::LESSGREATER},
    {TokenType::Minus, Precedence::SUM},
    {TokenType::Plus, Precedence::SUM},
    {TokenType::Slash, Precedence::PRODUCT},
    {TokenType::Star, Precedence::PRODUCT},
};

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

Precedence Parser::nextPrecendence() {
  auto it = precedences.find(nextToken().type);
  if (it == precedences.end()) {
    return Precedence::LOWEST;
  }
  return it->second;
}

Precedence Parser::currentPrecendence() {
  auto it = precedences.find(currentToken().type);
  if (it == precedences.end()) {
    return Precedence::LOWEST;
  }
  return it->second;
}

void Parser::parse() {
  while (currentToken().type != TokenType::EndOfFile) {
    parseStatement();
    current++;
  }
}

// Returns the index of the parsed statement in parserResult.statements (-1
// means none).
int Parser::parseStatement() {
  size_t statementsBefore = parserResult.statements.size();
  switch (currentToken().type) {
  case TokenType::Var:
    parseVarStatement();
    break;
  case TokenType::If:
    parseIfExpression();
    break;
  case TokenType::Else:
  case TokenType::While:
    errors.push_back("Unexpected token " +
                     tokenTypeToString(currentToken().type));
    return -1;
  case TokenType::Return:
    // parseReturnStatment();
  default:
    parseExpressionStatment();
    break;
  }
  if (parserResult.statements.size() == statementsBefore) {
    errors.push_back("No statement produced for " +
                     tokenTypeToString(currentToken().type));
    return -1;
  }
  return parserResult.statements.size() - 1;
}

void Parser::parseExpressionStatment() {
  Statement statement;
  statement.kind = StatementKind::EXPRESSION;
  statement.expressionIndex = parseExpression(Precedence::LOWEST);
  if (nextTokenIs(TokenType::Semicolon)) {
    current++;
  }
  parserResult.statements.push_back(statement);
}

// Parses "var name;" or "var name = expression;"
void Parser::parseVarStatement() {
  current++; // skip "var"
  Statement statement;
  statement.kind = StatementKind::VAR;
  if (!currentTokenIs(TokenType::Identifier)) {
    errors.push_back(
        expectedTokenError(TokenType::Identifier, currentToken().type));
    return;
  }
  statement.name = currentToken().literal;
  current++;
  // Declaration without initializer: "var name;"
  if (currentTokenIs(TokenType::Semicolon)) {
    parserResult.statements.push_back(statement);
    return;
  }
  if (!currentTokenIs(TokenType::Assign)) {
    errors.push_back(
        expectedTokenError(TokenType::Assign, currentToken().type));
    return;
  }
  current++; // skip "="
  statement.expressionIndex = parseExpression(Precedence::LOWEST);
  if (nextTokenIs(TokenType::Semicolon)) {
    current++;
  }
  parserResult.statements.push_back(statement);
}

int Parser::parseExpression(Precedence precedence) {
  auto prefix = prefixFns[currentToken().type];
  if (!prefix) {
    errors.push_back("No prefix parse function for " +
                     tokenTypeToString(currentToken().type) + " found");
    return -1;
  }
  int leftExprIndex = prefix();
  while (!nextTokenIs(TokenType::Semicolon) && nextPrecendence() > precedence) {
    current++;
    auto infix = infixFns[currentToken().type];
    if (!infix) {
      return leftExprIndex;
    }
    leftExprIndex = infix(leftExprIndex);
  }
  return leftExprIndex;
}

int Parser::parseIdentifier() {
  Expression expression;
  expression.kind = ExpressionKind::IDENTIFIER;
  expression.stringValue = currentToken().literal;
  parserResult.expressions.push_back(expression);
  return parserResult.expressions.size() - 1;
}

int Parser::parseNumber() {
  Expression expression;
  expression.kind = ExpressionKind::LITERAL_NUMBER;
  expression.numValue = std::stod(currentToken().literal);
  parserResult.expressions.push_back(expression);
  return parserResult.expressions.size() - 1;
}

// Parses "!operand" or "-operand"
int Parser::parseUnary() {
  Expression expression;
  expression.kind = ExpressionKind::UNARY;
  if (currentToken().type == TokenType::BANG) {
    expression.unaryOperator = UnaryOperator::NOT;
  } else if (currentToken().type == TokenType::Minus) {
    expression.unaryOperator = UnaryOperator::NEGATE;
  }
  current++; // skip the operator
  // Child expressions live in parserResult.expressions, referenced by index
  expression.operandExprIndex = parseExpression(Precedence::UNARY);
  parserResult.expressions.push_back(expression);
  return parserResult.expressions.size() - 1;
}

int Parser::parseBinary(int leftExprIndex) {
  Expression expression;
  expression.kind = ExpressionKind::BINARY;
  expression.leftExprIndex = leftExprIndex;
  TokenType type = currentToken().type;
  if (type == TokenType::Plus) {
    expression.binaryOperator = BinaryOperator::ADD;
  } else if (type == TokenType::Minus) {
    expression.binaryOperator = BinaryOperator::SUBTRACT;
  } else if (type == TokenType::Star) {
    expression.binaryOperator = BinaryOperator::MULTIPLY;
  } else if (type == TokenType::Slash) {
    expression.binaryOperator = BinaryOperator::DIVIDE;
  } else if (type == TokenType::Equal) {
    expression.binaryOperator = BinaryOperator::EQUAL;
  } else if (type == TokenType::NotEqual) {
    expression.binaryOperator = BinaryOperator::NOT_EQUAL;
  } else if (type == TokenType::LessThan) {
    expression.binaryOperator = BinaryOperator::LESS_THAN;
  } else if (type == TokenType::GreaterThan) {
    expression.binaryOperator = BinaryOperator::GREATER_THAN;
  }
  Precedence precendence = currentPrecendence();
  current++;
  expression.rightExprIndex = parseExpression(precendence);
  parserResult.expressions.push_back(expression);
  return parserResult.expressions.size() - 1;
}

int Parser::parseBoolean() {
  Expression expression;
  expression.kind = ExpressionKind::LITERAL_BOOL;
  expression.boolValue = currentToken().type == TokenType::True ? true : false;
  parserResult.expressions.push_back(expression);
  return parserResult.expressions.size() - 1;
}

int Parser::parseGroupedExpression() {
  current++; // skip "("
  int exprIndex = parseExpression(Precedence::LOWEST);
  if (!nextTokenIs(TokenType::RParen)) {
    errors.push_back(expectedTokenError(TokenType::RParen, nextToken().type));
    return -1;
  }
  current++; // consume ")"
  return exprIndex;
}

int Parser::parseIfExpression() {
  if (!nextTokenIs(TokenType::LParen)) {
    errors.push_back(expectedTokenError(TokenType::LParen, nextToken().type));
    return -1;
  }
  Expression expression;
  expression.kind = ExpressionKind::IF;
  current++;
  expression.conditionExprIndex = parseExpression(Precedence::LOWEST);
  if (!currentTokenIs(TokenType::RParen)) {
    errors.push_back(
        expectedTokenError(TokenType::RParen, currentToken().type));
    return -1;
  }
  expression.consquenceStmtIndex = parseBlockStatement();
  if (currentTokenIs(TokenType::Else)) {
    expression.alternativeStmtIndex = parseBlockStatement();
  }
  parserResult.expressions.push_back(expression);
  return parserResult.expressions.size() - 1;
}

int Parser::parseBlockStatement() {
  if (!nextTokenIs(TokenType::LBrace)) {
    errors.push_back(expectedTokenError(TokenType::LBrace, nextToken().type));
    return -1;
  }
  current++; // move to "{"
  Statement statement;
  statement.kind = StatementKind::BLOCK;
  current++; // move past "{"
  while (!currentTokenIs(TokenType::RBrace) &&
         !currentTokenIs(TokenType::EndOfFile)) {
    int index = parseStatement();
    statement.statementsIndexes.push_back(index);
    current++;
  }
  if (!currentTokenIs(TokenType::RBrace)) {
    errors.push_back(
        expectedTokenError(TokenType::RBrace, currentToken().type));
  }
  current++; // move past "}"
  parserResult.statements.push_back(statement);
  return parserResult.statements.size() - 1;
}

int Parser::parseFunction() {
  if (!nextTokenIs(TokenType::LParen)) {
    errors.push_back(expectedTokenError(TokenType::LParen, nextToken().type));
    return -1;
  }
  current++; // move to "("
  Expression expression;
  expression.kind = ExpressionKind::FUNCTION;
  expression.parameters = parseFunctionParams();
  expression.bodyStmtIndex = parseBlockStatement();
  parserResult.expressions.push_back(expression);
  return parserResult.expressions.size() - 1;
}

std::vector<std::string> Parser::parseFunctionParams() {
  std::vector<std::string> params;
  if (!currentTokenIs(TokenType::LParen)) {
    errors.push_back(expectedTokenError(TokenType::LParen, currentToken().type));
    return params;
  }
  current++; // move past "("
  
  // No params => "()"
  if (currentTokenIs(TokenType::RParen)) {
      return params;
  }
  
  if (!currentTokenIs(TokenType::Identifier)) {
    errors.push_back(
        expectedTokenError(TokenType::Identifier, currentToken().type));
    return params;
  }
  params.push_back(currentToken().literal);
  current++;
  while (currentTokenIs(TokenType::Comma)) {
    current++;
    if (!currentTokenIs(TokenType::Identifier)) {
      errors.push_back(
          expectedTokenError(TokenType::Identifier, currentToken().type));
      return params;
    }
    params.push_back(currentToken().literal);
    current++;
  }
  if (!currentTokenIs(TokenType::RParen)) {
    errors.push_back(expectedTokenError(TokenType::RParen, currentToken().type));
  }
  return params;
}

std::string expectedTokenError(TokenType expected, TokenType got) {
  return "Expected next token to be " + tokenTypeToString(expected) + ", got " +
         tokenTypeToString(got);
}

void Parser::registerPrefix(TokenType tokenType, PrefixParseFn fn) {
  prefixFns[tokenType] = fn;
}

void Parser::registerInfix(TokenType tokenType, InfixParseFn fn) {
  infixFns[tokenType] = fn;
}
