#pragma once

#include "garbage-colector/gc.h"
#include "interpreter/value.h"

// roots one Value
struct Rooted {
  explicit Rooted(Value &value) { gc.valueRoots.push_back(&value); }
  ~Rooted() { gc.valueRoots.pop_back(); }
  Rooted(const Rooted &) = delete; // disable copying
};

// roots a vector of Values
struct RootedVector {
  explicit RootedVector(std::vector<Value> &values) {
    gc.vectorRoots.push_back(&values);
  }
  ~RootedVector() { gc.vectorRoots.pop_back(); }
  RootedVector(const RootedVector &) = delete;
};

// roots one object
struct RootedObj {
  explicit RootedObj(Obj *obj) { gc.objRoots.push_back(obj); }
  ~RootedObj() { gc.objRoots.pop_back(); }
  RootedObj(const RootedObj &) = delete;
};
