#pragma once

namespace python {

class Object;

class PointerVisitor {
 public:
  virtual void visitPointer(Object** pointer) = 0;
  virtual ~PointerVisitor() = default;
};

}  // namespace python
