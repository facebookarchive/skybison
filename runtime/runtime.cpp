#include "runtime.h"

#include "globals.h"
#include "handle.h"
#include "heap.h"

namespace python {

Object* Runtime::byteArrayClass_ = nullptr;
Object* Runtime::objectArrayClass_ = nullptr;
Object* Runtime::codeClass_ = nullptr;
Object* Runtime::classClass_ = nullptr;
Object* Runtime::stringClass_ = nullptr;

Object* Runtime::functionClass_ = nullptr;
Object* Runtime::moduleClass_ = nullptr;
Object* Runtime::dictionaryClass_ = nullptr;

void Runtime::allocateClasses() {
  // Create the class Class by hand to accommodate circularity.
  Object* raw = Heap::allocate(Class::allocationSize());
  assert(raw != nullptr);
  Class* result = reinterpret_cast<Class*>(raw);
  result->setClass(result);
  result->setLayout(Layout::CLASS);
  classClass_ = result;

  byteArrayClass_ = Heap::createClass(Layout::BYTE_ARRAY);
  objectArrayClass_ = Heap::createClass(Layout::OBJECT_ARRAY);
  codeClass_ = Heap::createClass(Layout::CODE);
  stringClass_ = Heap::createClass(Layout::STRING);
  functionClass_ = Heap::createClass(Layout::FUNCTION);
}

void Runtime::initialize() {
  Handles::initialize();
  Heap::initialize(100 * MiB);
  allocateClasses();
}

} // namespace python
