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
    {TokenType::LessThanOrEqual, Precedence::LESSGREATER},
    {TokenType::GreaterThanOrEqual, Precedence::LESSGREATER},
    {TokenType::Minus, Precedence::SUM},
    {TokenType::Plus, Precedence::SUM},
    {TokenType::Slash, Precedence::PRODUCT},
    {TokenType::Star, Precedence::PRODUCT},
    {TokenType::LParen, Precedence::CALL},
    {TokenType::LBrack, Precedence::INDEX},
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

Precedence Parser::nextPrecedence() {
  auto it = precedences.find(nextToken().type);
  if (it == precedences.end()) {
    return Precedence::LOWEST;
  }
  return it->second;
}

Precedence Parser::currentPrecedence() {
  auto it = precedences.find(currentToken().type);
  if (it == precedences.end()) {
    return Precedence::LOWEST;
  }
  return it->second;
}

void Parser::parse() {
  while (currentToken().type != TokenType::EndOfFile) {
    int index = parseStatement();
    if (index != -1) {
      parserResult.programStatementsIndexes.push_back(index);
    }
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
    parseIfStatement();
    break;
  case TokenType::Else:
  case TokenType::While:
    errors.push_back("Unexpected token " +
                     tokenTypeToString(currentToken().type));
    return -1;
  case TokenType::Return:
    parseReturnStatement();
    break;
  case TokenType::Semicolon:
    current++;
    break;
  default:
    parseExpressionStatement();
    break;
  }
  // Failed to parse the statement
  if (statementsBefore == parserResult.statements.size()) {
    return -1;
  }
  return static_cast<int>(parserResult.statements.size()) - 1;
}

void Parser::parseExpressionStatement() {
  Statement statement;
  statement.kind = StatementKind::EXPRESSION;
  statement.expressionIndex = parseExpression(Precedence::LOWEST);
  if (nextTokenIs(TokenType::Semicolon)) {
    current++;
  }
  if (statement.expressionIndex == -1) {
    return;
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

void Parser::parseReturnStatement() {
  current++; // skip "return"
  Statement statement;
  statement.kind = StatementKind::RETURN;
  statement.expressionIndex = parseExpression(Precedence::LOWEST);
  if (nextTokenIs(TokenType::Semicolon)) {
    current++;
  }
  if (statement.expressionIndex == -1) {
    return;
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
  while (!nextTokenIs(TokenType::Semicolon) && nextPrecedence() > precedence) {
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
  expression.literal = currentToken().literal;
  parserResult.expressions.push_back(expression);
  return static_cast<int>(parserResult.expressions.size()) - 1;
}

int Parser::parseNumber() {
  Expression expression;
  expression.kind = ExpressionKind::LITERAL_NUMBER;
  expression.numValue = std::stod(currentToken().literal);
  parserResult.expressions.push_back(expression);
  return static_cast<int>(parserResult.expressions.size()) - 1;
}

int Parser::parseString() {
  Expression expression;
  expression.kind = ExpressionKind::LITERAL_STRING;
  expression.literal = currentToken().literal;
  parserResult.expressions.push_back(expression);
  return static_cast<int>(parserResult.expressions.size()) - 1;
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
  expression.subExprIndex = parseExpression(Precedence::UNARY);
  parserResult.expressions.push_back(expression);
  return static_cast<int>(parserResult.expressions.size()) - 1;
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
  } else if (type == TokenType::LessThanOrEqual) {
    expression.binaryOperator = BinaryOperator::LESS_THAN_OR_EQUAL;
  } else if (type == TokenType::GreaterThanOrEqual) {
    expression.binaryOperator = BinaryOperator::GREATER_THAN_OR_EQUAL;
  }
  Precedence precedence = currentPrecedence();
  current++;
  expression.rightExprIndex = parseExpression(precedence);
  parserResult.expressions.push_back(expression);
  return static_cast<int>(parserResult.expressions.size()) - 1;
}

int Parser::parseBoolean() {
  Expression expression;
  expression.kind = ExpressionKind::LITERAL_BOOL;
  expression.boolValue = currentToken().type == TokenType::True ? true : false;
  parserResult.expressions.push_back(expression);
  return static_cast<int>(parserResult.expressions.size()) - 1;
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

int Parser::parseIfStatement() {
  if (!nextTokenIs(TokenType::LParen)) {
    errors.push_back(expectedTokenError(TokenType::LParen, nextToken().type));
    return -1;
  }
  Statement statement;
  statement.kind = StatementKind::IF;
  current++; // move to "("
  statement.conditionExprIndex = parseExpression(Precedence::LOWEST);
  if (!currentTokenIs(TokenType::RParen)) {
    errors.push_back(
        expectedTokenError(TokenType::RParen, currentToken().type));
    return -1;
  }
  statement.consequenceStmtIndex = parseBlockStatement();
  if (nextTokenIs(TokenType::Else)) {
    current++; // move to "else"
    statement.alternativeStmtIndex = parseBlockStatement();
  }
  parserResult.statements.push_back(statement);
  return static_cast<int>(parserResult.statements.size()) - 1;
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
    if (index != -1) {
      statement.statementsIndexes.push_back(index);
    }
    current++;
  }
  if (!currentTokenIs(TokenType::RBrace)) {
    errors.push_back(
        expectedTokenError(TokenType::RBrace, currentToken().type));
  }
  parserResult.statements.push_back(statement);
  return static_cast<int>(parserResult.statements.size()) - 1;
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
  return static_cast<int>(parserResult.expressions.size()) - 1;
}

std::vector<std::string> Parser::parseFunctionParams() {
  std::vector<std::string> params;
  if (!currentTokenIs(TokenType::LParen)) {
    errors.push_back(
        expectedTokenError(TokenType::LParen, currentToken().type));
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
    errors.push_back(
        expectedTokenError(TokenType::RParen, currentToken().type));
  }
  return params;
}

int Parser::parseCallExpression(int leftExprIndex) {
  if (leftExprIndex < 0) {
    errors.push_back("Expected an expression before the call parentheses");
    return -1;
  }
  Expression expression;
  expression.kind = ExpressionKind::CALL;
  expression.functionExprIndex = leftExprIndex;
  expression.expressionsIndexes = parseCallParams();
  parserResult.expressions.push_back(expression);
  return static_cast<int>(parserResult.expressions.size()) - 1;
}

std::vector<int> Parser::parseCallParams() {
  std::vector<int> params;
  current++; // move past "("

  // No params => "()"
  if (currentTokenIs(TokenType::RParen)) {
    return params;
  }

  params.push_back(parseExpression(Precedence::LOWEST));
  while (nextTokenIs(TokenType::Comma)) {
    current += 2; // move past the current param and the comma
    params.push_back(parseExpression(Precedence::LOWEST));
  }
  if (!nextTokenIs(TokenType::RParen)) {
    errors.push_back(expectedTokenError(TokenType::RParen, nextToken().type));
    return params;
  }
  current++; // move to ")"
  return params;
}

int Parser::parseIndexExpression(int leftExprIndex) {
  if (leftExprIndex < 0) {
    errors.push_back("Expected an expression before the index brackets");
    return -1;
  }
  Expression literalExpr = parserResult.expressions[leftExprIndex];
  if (literalExpr.kind != ExpressionKind::IDENTIFIER) {
    errors.push_back("Expected an identifier before the index brackets");
    return -1;
  }
  Expression expression;
  expression.kind = ExpressionKind::INDEX;
  expression.literal = literalExpr.literal;
  current++; // move past "["
  int indexExpr = parseExpression(Precedence::LOWEST);
  if (indexExpr == -1) {
    return -1;
  }
  if (!nextTokenIs(TokenType::RBrack)) {
    errors.push_back(expectedTokenError(TokenType::RBrack, nextToken().type));
    return -1;
  }
  current++; // move to "]"
  expression.subExprIndex = indexExpr;
  parserResult.expressions.push_back(expression);
  return static_cast<int>(parserResult.expressions.size()) - 1;
}

int Parser::parseArray() {
  Expression expression;
  expression.kind = ExpressionKind::LITERAL_ARRAY;
  expression.expressionsIndexes = parseArrayValues();
  parserResult.expressions.push_back(expression);
  return static_cast<int>(parserResult.expressions.size()) - 1;
}

std::vector<int> Parser::parseArrayValues() {
  std::vector<int> values;
  current++; // move past "["
  // empty array => "[]"
  if (currentTokenIs(TokenType::RBrack)) {
    return values;
  }
  values.push_back(parseExpression(Precedence::LOWEST));
  while (nextTokenIs(TokenType::Comma)) {
    current += 2; // move past the current value and the comma
    values.push_back(parseExpression(Precedence::LOWEST));
  }

  if (!nextTokenIs(TokenType::RBrack)) {
    errors.push_back(expectedTokenError(TokenType::RBrack, nextToken().type));
    return values;
  }
  current++; // move to "]"
  return values;
}

int Parser::parseHashLiteral() {
  current++; // move past "{"
  Expression expression;
  expression.kind = ExpressionKind::LITERAL_HASH;
  // empty => "{}"
  if (currentTokenIs(TokenType::RBrace)) {
    parserResult.expressions.push_back(expression);
    return static_cast<int>(parserResult.expressions.size()) - 1;
  }
  int keyIndex = parseExpression(Precedence::LOWEST);
  if (keyIndex == -1) {
    return -1;
  }
  if (!nextTokenIs(TokenType::Colon)) {
    errors.push_back(expectedTokenError(TokenType::Colon, nextToken().type));
    return -1;
  }
  current += 2; // move past the key and ":"
  int valIndex = parseExpression(Precedence::LOWEST);
  if (valIndex == -1) {
    return -1;
  }
  expression.pairs[keyIndex] = valIndex;

  while (nextTokenIs(TokenType::Comma)) {
    current += 2; // move past the current value and the comma
    keyIndex = parseExpression(Precedence::LOWEST);
    if (keyIndex == -1) {
      return -1;
    }
    if (!nextTokenIs(TokenType::Colon)) {
      errors.push_back(
          expectedTokenError(TokenType::Colon, nextToken().type));
      return -1;
    }
    current += 2; // move past the key and ":"
    valIndex = parseExpression(Precedence::LOWEST);
    if (valIndex == -1) {
      return -1;
    }
    expression.pairs[keyIndex] = valIndex;
  }

  if (!nextTokenIs(TokenType::RBrace)) {
    errors.push_back(expectedTokenError(TokenType::RBrace, nextToken().type));
    return -1;
  }
  current++; // move to "}"
  parserResult.expressions.push_back(expression);
  return static_cast<int>(parserResult.expressions.size()) - 1;
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
