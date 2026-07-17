#pragma once

#include <string>
enum class TokenType {
  Unknown,
  EndOfFile,

  Identifier,
  Number,
  Plus,
  Minus,
  Star,
  Slash,
  LParen,
  RParen,
  LBrace,
  RBrace,
  Comma,
  Semicolon,
  Equal,
  NotEqual,
  LessThan,
  GreaterThan,
  LessThanOrEqual,
  GreaterThanOrEqual,
  And,
  Or,
  BANG,
  Assign,

  // Keywords
  If,
  Else,
  While,
  Var,

  // Operators
  Function,
  Return,
};

struct Token {
  TokenType type;
  std::string literal;
};

std::string tokenTypeToString(TokenType type);
