#include "environment.h"
#include "interpreter/value.h"
#include <string>

Value Environment::get(std::string name) {
  if (store.contains(name)) {
    return store[name];
  }
  if (outer) {
    return outer->get(name);
  }
  return Value{.kind = ValueKind::Error,
               .strValue = "identifier not found: " + name};
}

void Environment::set(std::string name, Value value) { store[name] = value; }

std::shared_ptr<Environment>
newEnclosedEnvironment(std::shared_ptr<Environment> outer) {
  auto environment = std::make_shared<Environment>();
  environment->outer = outer;
  return environment;
}
