#include "environment.h"
#include "interpreter/value.h"
#include <string>

Value Environment::get(std::string name) {
  if (store.contains(name)) {
    return store[name];
  }
  if (outer && outer->get(name).kind != ValueKind::Null) {
    return outer->get(name);
  }
  return {};
}

void Environment::set(std::string name, Value value) { store[name] = value; }

std::shared_ptr<Environment>
newEnclosedEnvironment(std::shared_ptr<Environment> outer) {
  auto environment = std::make_shared<Environment>();
  environment->outer = outer;
  return environment;
}
