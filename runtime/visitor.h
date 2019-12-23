#pragma once

namespace py {

class RawObject;
class RawHeapObject;

class PointerVisitor {
 public:
  virtual void visitPointer(RawObject* pointer) = 0;
  virtual ~PointerVisitor() = default;
};

class HeapObjectVisitor {
 public:
  virtual void visitHeapObject(RawHeapObject object) = 0;
  virtual ~HeapObjectVisitor() = default;
};

}  // namespace py
