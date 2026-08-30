#pragma once
#include "interpreter/value.h"

// During a collection every object is in one of three states. Obj::marked and
// membership in pendingScan encode all three between them. The colors are the
// standard tricolor names for these states:
//
//   marked  in pendingScan  color  meaning
//   false   no              white  not reached; presumed garbage
//   true    yes             gray   reached; its references not scanned yet
//   true    no              black  reached; its references marked too

struct GC {
  Obj *objects = nullptr;       // head of the chain of every live object
  ObjEnv *globalEnv = nullptr;  // the one root that is always live
  std::vector<Obj *> grayStack; // marked, references not scanned yet
  size_t bytesAllocated = 0;    // how much we think is live
  size_t nextGC = 1024 * 1024;  // collect once bytesAllocated passes this
};
extern GC gc;

void collectGarbage();
