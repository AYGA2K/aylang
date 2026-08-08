#include "environment.h"
#include "interpreter/value.h"
#include <string>

Value Environment::get(std::string name) {
  if (store.contains(name)) {
    return store[name];
  }
  return {};
}

void Environment::set(std::string name, Value value) { store[name] = value; }
