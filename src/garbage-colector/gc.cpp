#include "gc.h"
#include "interpreter/value.h"
#include <cstddef>
#include <vector>

void maybeCollect(size_t size) {
  if (gc.bytesAllocated + size > gc.nextGC) {
    collectGarbage();
  }
}
template <typename T> T *allocateObj(ObjKind kind) {
  maybeCollect(sizeof(T)); // we collect before creating brand new obj because
                           // doing it after will make the sweep free it
                           // because nothing points to it yet
  T *obj = new T();
  obj->kind = kind;
  obj->marked = false;
  obj->next = gc.objects; // link into the list
  gc.objects = obj;
  gc.bytesAllocated += sizeof(T);
  return obj;
}

inline Value makeStringObj(std::string s) {
  ObjString *obj = allocateObj<ObjString>(ObjKind::String);
  obj->chars = std::move(s);
  return makeObj(obj);
}

inline Value makeArrayObj(std::vector<Value> items) {
  ObjArray *obj = allocateObj<ObjArray>(ObjKind::Array);
  obj->items = std::move(items);
  return makeObj(obj);
}

inline Value makeHashMapObj(std::vector<Value> entries) {
  ObjHashMap *obj = allocateObj<ObjHashMap>(ObjKind::Array);
  obj->entries = std::move(entries);
  return makeObj(obj);
}

static void markObject(Obj *obj) {
  if (obj == nullptr || obj->marked) {
    return;
  }
  obj->marked = true;
  gc.grayStack.push_back(obj);
}

static void markValue(const Value &value) {
  if (value.tag == Tag::Obj) {
    markObject(value.obj);
  }
}

static void scanObject(Obj *obj) {
  switch (obj->kind) {
  case ObjKind::String:
  case ObjKind::Error:
    break; // hold no other values
  case ObjKind::Array:
    for (const Value &v : static_cast<ObjArray *>(obj)->items) {
      markValue(v);
    }
    break;
  case ObjKind::HashMap:
    for (const Value &v : static_cast<ObjHashMap *>(obj)->entries) {
      markValue(v);
    }
    break;
  case ObjKind::Function:
    markObject(static_cast<ObjFunction *>(obj)->env);
    break;
  case ObjKind::Env: {
    ObjEnv *env = static_cast<ObjEnv *>(obj);
    for (const auto &[name, value] : env->store) {
      markValue(value);
    }
    markObject(env->outer);
    break;
  }
  }
}

static size_t sizeOfObject(Obj *obj) {
  switch (obj->kind) {
  case ObjKind::String:
    return sizeof(ObjString);
  case ObjKind::Error:
    return sizeof(ObjError);
  case ObjKind::Array:
    return sizeof(ObjArray);
  case ObjKind::HashMap:
    return sizeof(ObjHashMap);
  case ObjKind::Function:
    return sizeof(ObjFunction);
  case ObjKind::Env:
    return sizeof(ObjEnv);
  }
  return 0;
}

static void freeObject(Obj *obj) {
  switch (obj->kind) {
  case ObjKind::String:
    delete static_cast<ObjString *>(obj);
    break;
  case ObjKind::Error:
    delete static_cast<ObjError *>(obj);
    break;
  case ObjKind::Array:
    delete static_cast<ObjArray *>(obj);
    break;
  case ObjKind::HashMap:
    delete static_cast<ObjHashMap *>(obj);
    break;
  case ObjKind::Function:
    delete static_cast<ObjFunction *>(obj);
    break;
  case ObjKind::Env:
    delete static_cast<ObjEnv *>(obj);
    break;
  }
}

static void sweep() {
  Obj **link = &gc.objects;
  while (*link != nullptr) {
    Obj *obj = *link;
    if (obj->marked) {
      obj->marked = false; // reset
      link = &obj->next;   // move link to the next object's next pointer
    } else {
      gc.bytesAllocated -= sizeOfObject(obj);
      *link = obj->next; // unlink the current obj from the chain
      freeObject(obj);
    }
  }
}

void markRoots() { markObject(gc.globalEnv); }

void collectGarbage() {
  markRoots();
  while (!gc.grayStack.empty()) {
    Obj *obj = gc.grayStack.back();
    gc.grayStack.pop_back();
    scanObject(obj);
  }
  sweep();
  gc.nextGC = gc.bytesAllocated * 2; // let the heap grow geometrically
}
