#include "runtime.h"

#include "globals.h"
#include "handle.h"
#include "heap.h"

namespace python {

Object* Runtime::byteArrayClass_= nullptr;
Object* Runtime::objectArrayClass_= nullptr;
Object* Runtime::codeClass_ = nullptr;
Object* Runtime::classClass_ = nullptr;
Object* Runtime::stringClass_ = nullptr;

Object* Runtime::functionClass_ = nullptr;
Object* Runtime::moduleClass_ = nullptr;
Object* Runtime::dictionaryClass_ = nullptr;

void Runtime::allocateClasses() {
  // Create the class Class by hand to accommodate circularity.
  Object* raw = Heap::Allocate(Class::allocationSize());
  assert(raw != nullptr);
  Class* result = reinterpret_cast<Class*>(raw);
  result->setClass(result);
  result->setLayout(Layout::CLASS);
  classClass_ = result;

  byteArrayClass_ = Heap::CreateClass(Layout::BYTE_ARRAY);
  objectArrayClass_ = Heap::CreateClass(Layout::OBJECT_ARRAY);
  codeClass_ = Heap::CreateClass(Layout::CODE);
  stringClass_ = Heap::CreateClass(Layout::STRING);
  functionClass_ = Heap::CreateClass(Layout::FUNCTION);
}

void Runtime::initialize() {
  Handles::Initialize();
  Heap::Initialize(100 * MiB);
  allocateClasses();
}

}  // namespace python
