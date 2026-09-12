#pragma once

#include "interpreter/value.h"
#include <string>

ObjEnv *newEnvironment();
ObjEnv *newEnclosedEnvironment(ObjEnv *outer);

Value envGet(ObjEnv *env, const std::string &name);
void envSet(ObjEnv *env, const std::string &name, const Value &value);
// Rebinds an existing name where it lives, so a call can update a variable
// of an enclosing scope.
bool envAssign(ObjEnv *env, const std::string &name, const Value &value);
