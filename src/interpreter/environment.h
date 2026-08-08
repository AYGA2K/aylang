#pragma once

#include "interpreter/value.h"
#include <string>
#include <unordered_map>

struct Environment {
  std::unordered_map<std::string, Value> store;
  Value get(std::string name);
  void set(std::string, Value value);
};
