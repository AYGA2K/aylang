#include "interpreter/evaluator.h"
#include "interpreter/value.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include <iostream>
#include <print>
#include <vector>

int main() {
  std::string input;
  while (true) {
    std::print(">> ");
    if (!std::getline(std::cin, input) || input == "exit") {
      break;
    }
    if (input.empty()) {
      continue;
    }
    Lexer lexer{.input = input};
    std::vector<Token> tokens = lexer.tokenize();
    Parser parser{tokens};
    parser.parse();
    if (!parser.errors.empty()) {
      for (const std::string &error : parser.errors) {
        std::println("{}", error);
      }
      continue;
    }
    Evaluator evaluator{.parserResult = parser.parserResult};
    std::println("{}", inspect(evaluator.evalStatements()));
  }
  std::println("");
  return 0;
}
