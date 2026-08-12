#pragma once

#include "interpreter/value.h"
#include <memory>
#include <string>
#include <unordered_map>

struct Environment {
  std::unordered_map<std::string, Value> store;
  std::shared_ptr<Environment> outer;
  Value get(std::string name);
  void set(std::string, Value value);
};

std::shared_ptr<Environment>
newEnclosedEnvironment(std::shared_ptr<Environment> outer);
