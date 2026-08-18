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

static bool isDeclarationStatement(const ParserResult &parserResult,
                                   int statementIndex) {
  const Statement &statement = parserResult.statements[statementIndex];
  if (statement.kind == StatementKind::VAR) {
    return true;
  }
  return statement.kind == StatementKind::EXPRESSION &&
         statement.expressionIndex >= 0 &&
         parserResult.expressions[statement.expressionIndex].kind ==
             ExpressionKind::FUNCTION;
}

int main() {
  std::string input;
  Evaluator evaluator;
  std::vector<Token> tokens;
  Parser parser{tokens};
  size_t evaluatedStatements = 0;
  while (readLine(">> ", input)) {
    if (input == "exit") {
      break;
    }
    if (input.empty()) {
      continue;
    }
    add_history(input.c_str());
    Lexer lexer{.input = input};
    std::vector<Token> lineTokens = lexer.tokenize();
    size_t tokensBefore = tokens.size();
    size_t statementsBefore = parser.parserResult.statements.size();
    size_t expressionsBefore = parser.parserResult.expressions.size();
    size_t programStatementsBefore =
        parser.parserResult.programStatementsIndexes.size();
    tokens.insert(tokens.end(), lineTokens.begin(), lineTokens.end());
    parser.parse();
    if (!parser.errors.empty()) {
      for (const std::string &error : parser.errors) {
        std::println("{}", error);
      }
      parser.errors.clear();
      // Undo this line so the next one starts clean.
      tokens.resize(tokensBefore);
      parser.current = tokensBefore;
      parser.parserResult.statements.resize(statementsBefore);
      parser.parserResult.expressions.resize(expressionsBefore);
      parser.parserResult.programStatementsIndexes.resize(
          programStatementsBefore);
      continue;
    }
    // A trailing ";" can leave current one past the end, so reset it here.
    parser.current = tokens.size();
    evaluator.parserResult = parser.parserResult;
    Value result = evaluator.evalStatements(evaluatedStatements);
    const std::vector<int> &programStatementsIndexes =
        evaluator.parserResult.programStatementsIndexes;
    bool isDeclaration =
        !programStatementsIndexes.empty() &&
        isDeclarationStatement(evaluator.parserResult,
                               programStatementsIndexes.back());
    // Declarations and statements evaluating to null (like a print call) have
    // nothing worth echoing back.
    if (!isDeclaration && result.kind != ValueKind::Null) {
      std::println("{}", inspect(result));
    }
    evaluatedStatements = programStatementsIndexes.size();
  }
  std::println("");
  return 0;
}
