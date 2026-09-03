#pragma once
#include "interpreter/value.h"
#include <cstddef>
#include <vector>

// During a collection every object is in one of three states. Obj::marked and
// membership in pendingScan encode all three between them. The colors are the
// standard tricolor names for these states:
//
//   marked  in pendingScan  color  meaning
//   false   no              white  not reached; presumed garbage
//   true    yes             gray   reached; its references not scanned yet
//   true    no              black  reached; its references marked too

struct GC {
  Obj *objects = nullptr;          // head of the chain of every live object
  ObjEnv *globalEnv = nullptr;     // the one root that is always live
  std::vector<Obj *> grayStack;    // marked, references not scanned yet
  size_t bytesAllocated = 0;       // how much we think is live
  size_t nextGC = 1024 * 1024;     // collect once bytesAllocated passes this
  std::vector<Value *> valueRoots; // one local Value
  std::vector<std::vector<Value> *> vectorRoots; // a local vector of Values
  std::vector<Obj *> objRoots; // a bare object pointer (call environment)
};
extern GC gc;

void collectGarbage();
void maybeCollect(size_t size);

// Every heap object of the language goes through here:
// we collect first if this allocation would push us past the threshold, then
// link the new object into the chain of live objects. Collecting before the
// object exists keeps the sweep from freeing it while nothing points to it yet.
template <typename T> T *allocateObj(ObjKind kind) {
  maybeCollect(sizeof(T));
  T *obj = new T();
  obj->kind = kind;
  obj->marked = false;
  obj->next = gc.objects; // link into the list
  gc.objects = obj;
  gc.bytesAllocated += sizeof(T);
  return obj;
}
