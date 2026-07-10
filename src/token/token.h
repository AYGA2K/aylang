#pragma once

#include <string>
enum class TokenType {
  Unknown,
  EndOfFile,
  Identifier,
  Integer,
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
  Not,
  Assign,

  // Keywords
  If,
  Else,
  While,

  // Operators
  Function,
  Return,
};

struct Token {
  TokenType type;
  std::string Literal;
};
