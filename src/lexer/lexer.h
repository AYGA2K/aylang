#pragma once

#include "token.h"
#include <string>

struct Lexer {
  std::string input;
  std::size_t current = 0;
  int line = 1;

  char readChar();
  char peekChar();
  char peekNext();
  Token nextToken();
};

// Returns the keyword TokenType for `literal`, or Identifier if it is not a
// keyword.
TokenType lookupIdentifier(const std::string &literal);
