#pragma once

namespace py {

class RawObject;

class PointerVisitor {
 public:
  virtual void visitPointer(RawObject* pointer) = 0;
  virtual ~PointerVisitor() = default;
};

}  // namespace py
