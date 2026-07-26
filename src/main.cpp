#include "lexer/lexer.h"
#include "parser/parser.h"
#include <iostream>
#include <vector>

int main() {
  std::string input;
  while (true) {
    std::cout << ">> ";
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
        std::cout << "\t" << error << "\n";
      }
      continue;
    }
    std::cout << "parsed " << parser.parserResult.statements.size()
              << " statement(s)\n";
  }
  std::cout << "\n";
  return 0;
}
