#include "lexer.h"
#include "token.h"
#include <cctype>
#include <string>
#include <unordered_map>
#include <vector>

TokenType lookupIdentifier(const std::string &literal) {
  static const std::unordered_map<std::string, TokenType> keywords = {
      {"if", TokenType::If},         {"else", TokenType::Else},
      {"while", TokenType::While},   {"fn", TokenType::Function},
      {"return", TokenType::Return}, {"var", TokenType::Var},
  };

  auto it = keywords.find(literal);
  if (it != keywords.end()) {
    return it->second;
  }
  return TokenType::Identifier;
}

char Lexer::readChar() {
  if (current >= input.size()) {
    return '\0';
  }
  return input[current++];
}

char Lexer::peekChar() {
  if (current >= input.size()) {
    return '\0';
  }
  return input[current];
}

char Lexer::peekNext() {
  if (current + 1 >= input.size()) {
    return '\0';
  }
  return input[current + 1];
}

std::vector<Token> Lexer::tokenize() {
  std::vector<Token> tokens;
  while (current < input.size()) {
    Token token = nextToken();
    tokens.push_back(token);
  }
  return tokens;
}

Token Lexer::nextToken() {
  while (peekChar() == ' ' || peekChar() == '\t' || peekChar() == '\r' ||
         peekChar() == '\n') {
    if (peekChar() == '\n')
      line++;
    current++;
  }
  char c = readChar();
  switch (c) {
  case '\0':
    return Token{.type = TokenType::EndOfFile, .literal = ""};
  case '(':
    return Token{.type = TokenType::LParen, .literal = "("};
  case ')':
    return Token{.type = TokenType::RParen, .literal = ")"};
  case '{':
    return Token{.type = TokenType::LBrace, .literal = "{"};
  case '}':
    return Token{.type = TokenType::RBrace, .literal = "}"};
  case '+':
    return Token{.type = TokenType::Plus, .literal = "+"};
  case '-':
    return Token{.type = TokenType::Minus, .literal = "-"};
  case '*':
    return Token{.type = TokenType::Star, .literal = "*"};
  case '/':
    if (peekChar() == '/') {
      while (peekChar() != '\0' && peekChar() != '\n') {
        current++;
      }
      if (peekChar() == '\n') {
        current++;
        line++;
      }
      return nextToken();
    }
    return Token{.type = TokenType::Slash, .literal = "/"};
  case ',':
    return Token{.type = TokenType::Comma, .literal = ","};
  case ';':
    return Token{.type = TokenType::Semicolon, .literal = ";"};
  case '=':
    if (peekChar() == '=') {
      current++;
      return Token{.type = TokenType::Equal, .literal = "=="};
    }
    return Token{.type = TokenType::Assign, .literal = "="};
  case '!':
    if (peekChar() == '=') {
      current++;
      return Token{.type = TokenType::NotEqual, .literal = "!="};
    }
    return Token{.type = TokenType::Not, .literal = "!"};
  case '&':
    if (peekChar() == '&') {
      current++;
      return Token{.type = TokenType::And, .literal = "&&"};
    }
    return Token{.type = TokenType::Unknown, .literal = "&"};
  case '|':
    if (peekChar() == '|') {
      current++;
      return Token{.type = TokenType::Or, .literal = "||"};
    }
    return Token{.type = TokenType::Unknown, .literal = "|"};
  case '<':
    if (peekChar() == '=') {
      current++;
      return Token{.type = TokenType::LessThanOrEqual, .literal = "<="};
    }
    return Token{.type = TokenType::LessThan, .literal = "<"};
  case '>':
    if (peekChar() == '=') {
      current++;
      return Token{.type = TokenType::GreaterThanOrEqual, .literal = ">="};
    }
    return Token{.type = TokenType::GreaterThan, .literal = ">"};
  default:
    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
      std::size_t start = current - 1;
      while (std::isalnum(static_cast<unsigned char>(peekChar())) ||
             peekChar() == '_') {
        current++;
      }
      std::string literal = input.substr(start, current - start);
      return Token{.type = lookupIdentifier(literal), .literal = literal};
    }
    if (std::isdigit(static_cast<unsigned char>(c))) {
      std::size_t start = current - 1;
      while (std::isdigit(static_cast<unsigned char>(peekChar()))) {
        current++;
      }
      if (peekChar() == '.' &&
          std::isdigit(static_cast<unsigned char>(peekNext()))) {
        current++;
        while (std::isdigit(static_cast<unsigned char>(peekChar()))) {
          current++;
        }
      }
      std::string literal = input.substr(start, current - start);
      return Token{.type = TokenType::Number, .literal = literal};
    }
    return Token{.type = TokenType::Unknown, .literal = std::string(1, c)};
  }
}
