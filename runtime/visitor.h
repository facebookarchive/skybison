#pragma once

namespace python {

class Object;

class PointerVisitor {
 public:
  virtual void visitPointer(RawObject* pointer) = 0;
  virtual ~PointerVisitor() = default;
};

}  // namespace python
