#include "environment.h"
#include "interpreter/value.h"
#include <string>

ObjEnv *newEnvironment() {
  ObjEnv *env = new ObjEnv();
  env->kind = ObjKind::Env;
  return env;
}

ObjEnv *newEnclosedEnvironment(ObjEnv *outer) {
  ObjEnv *environment = newEnvironment();
  environment->outer = outer;
  return environment;
}

// Walks outward through the enclosing environments until the name is found
Value envGet(ObjEnv *env, const std::string &name) {
  for (ObjEnv *e = env; e != nullptr; e = e->outer) {
    auto it = e->store.find(name);
    if (it != e->store.end()) {
      return it->second;
    }
  }
  return makeError("identifier not found: " + name);
}

void envSet(ObjEnv *env, const std::string &name, const Value &value) {
  env->store[name] = value;
}
