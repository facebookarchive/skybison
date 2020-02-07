#pragma once

namespace py {

class RawObject;
class RawHeapObject;

enum class PointerKind {
  kRuntime,
  kThread,
  kHandle,
  kStack,
  kApiHandle,
  kUnknown,
  kLayout,
};

class PointerVisitor {
 public:
  virtual void visitPointer(RawObject* pointer, PointerKind kind) = 0;
  virtual ~PointerVisitor() = default;
};

class HeapObjectVisitor {
 public:
  virtual void visitHeapObject(RawHeapObject object) = 0;
  virtual ~HeapObjectVisitor() = default;
};

}  // namespace py
