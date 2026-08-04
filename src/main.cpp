#include "interpreter/evaluator.h"
#include "interpreter/value.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include <cstdlib>
#include <print>
#include <readline/history.h>
#include <readline/readline.h>
#include <string>
#include <vector>

static bool readLine(const char *prompt, std::string &line) {
  char *buffer = readline(prompt);
  if (buffer == nullptr) {
    return false;
  }
  line = buffer;
  std::free(buffer);
  return true;
}

int main() {
  std::string input;
  while (readLine(">> ", input)) {
    if (input == "exit") {
      break;
    }
    if (input.empty()) {
      continue;
    }
    add_history(input.c_str());
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
