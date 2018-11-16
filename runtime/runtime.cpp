#include "runtime.h"

#include "globals.h"
#include "handles.h"
#include "heap.h"
#include "thread.h"
#include "visitor.h"

namespace python {

Runtime::Runtime() : heap_(64 * MiB) {
  initializeThreads();
  initializeClasses();
  initializeInstances();
  initializeModules();
}

Runtime::~Runtime() {
  for (Thread* thread = threads_; thread != nullptr; thread = thread->next()) {
    if (thread == Thread::currentThread()) {
      Thread::setCurrentThread(nullptr);
    } else {
      assert(0); // Not implemented.
    }
    delete thread;
  }
}

Object* Runtime::newByteArray(intptr_t length) {
  if (length == 0) {
    return empty_byte_array_;
  }
  return heap()->createByteArray(Runtime::byte_array_class_, length);
}

Object* Runtime::newCode(
    int argcount,
    int kwonlyargcount,
    int nlocals,
    int stacksize,
    int flags,
    Object* code,
    Object* consts,
    Object* names,
    Object* varnames,
    Object* freevars,
    Object* cellvars,
    Object* filename,
    Object* name,
    int firstlineno,
    Object* lnotab) {
  return heap()->createCode(
      code_class_,
      argcount,
      kwonlyargcount,
      nlocals,
      stacksize,
      flags,
      code,
      consts,
      names,
      varnames,
      freevars,
      cellvars,
      filename,
      name,
      firstlineno,
      lnotab);
}

Object* Runtime::newDictionary() {
  Object* items = newObjectArray(Dictionary::kInitialItemsSize);
  assert(items != nullptr);
  return heap()->createDictionary(dictionary_class_, items);
}

Object* Runtime::newList() {
  return heap()->createList(list_class_, empty_object_array_);
}

Object* Runtime::newModule(Object* name) {
  Object* dict = newDictionary();
  assert(dict != nullptr);
  return heap()->createModule(module_class_, name, dict);
}

Object* Runtime::newObjectArray(intptr_t length) {
  if (length == 0) {
    return empty_object_array_;
  }
  return heap()->createObjectArray(object_array_class_, length, None::object());
}

Object* Runtime::newString(intptr_t length) {
  return heap()->createString(string_class_, length);
}

Object* Runtime::newStringFromCString(const char* c_string) {
  intptr_t length = strlen(c_string);
  Object* result = newString(length);
  for (intptr_t i = 0; i < length; i++) {
    String::cast(result)->charAtPut(
        i, *reinterpret_cast<const byte*>(c_string + i));
  }
  return result;
}

void Runtime::initializeClasses() {
  class_class_ = heap()->createClassClass();

  byte_array_class_ = heap()->createClass(
      class_class_,
      Layout::BYTE_ARRAY,
      ByteArray::kElementSize,
      true /* isArray */,
      false /* isRoot */);

  code_class_ = heap()->createClass(
      class_class_,
      Layout::CODE,
      Code::kSize,
      false /* isArray */,
      true /* isRoot */);

  dictionary_class_ = heap()->createClass(
      class_class_,
      Layout::DICTIONARY,
      Dictionary::kSize,
      false /* isArray */,
      true /* isRoot */);

  function_class_ = heap()->createClass(
      class_class_,
      Layout::FUNCTION,
      Function::kSize,
      false /* isArray */,
      true /* isRoot */);

  list_class_ = heap()->createClass(
      class_class_,
      Layout::LIST,
      List::kSize,
      false /* isArray */,
      true /* isRoot */);

  module_class_ = heap()->createClass(
      class_class_,
      Layout::MODULE,
      Module::kSize,
      false /* isArray */,
      true /* isRoot */);

  object_array_class_ = heap()->createClass(
      class_class_,
      Layout::OBJECT_ARRAY,
      ObjectArray::kElementSize,
      true /* isArray */,
      true /* isRoot */);

  string_class_ = heap()->createClass(
      class_class_,
      Layout::STRING,
      String::kElementSize,
      true /* isArray */,
      false /* isRoot */);
}

class ScavengeVisitor : public PointerVisitor {
 public:
  explicit ScavengeVisitor(Heap* heap) : heap_(heap) {}

  void visitPointer(Object** pointer) override {
    heap_->scavengePointer(pointer);
  }

 private:
  Heap* heap_;
};

void Runtime::collectGarbage() {
  heap()->flip();
  ScavengeVisitor visitor(heap());
  visitRoots(&visitor);
  heap()->scavenge();
}

void Runtime::initializeThreads() {
  Thread* main_thread = new Thread(Thread::kDefaultStackSize);
  threads_ = main_thread;
  Thread::setCurrentThread(main_thread);
}

void Runtime::initializeInstances() {
  empty_byte_array_ = heap()->createByteArray(byte_array_class_, 0);
  empty_object_array_ =
      heap()->createObjectArray(object_array_class_, 0, None::object());
  empty_string_ = heap()->createString(string_class_, 0);
}

void Runtime::visitRoots(PointerVisitor* visitor) {
  visitRuntimeRoots(visitor);
  visitThreadRoots(visitor);
}

void Runtime::visitRuntimeRoots(PointerVisitor* visitor) {
  // Visit classes
  visitor->visitPointer(&byte_array_class_);
  visitor->visitPointer(&class_class_);
  visitor->visitPointer(&code_class_);
  visitor->visitPointer(&dictionary_class_);
  visitor->visitPointer(&function_class_);
  visitor->visitPointer(&list_class_);
  visitor->visitPointer(&module_class_);
  visitor->visitPointer(&object_array_class_);
  visitor->visitPointer(&string_class_);

  // Visit instances
  visitor->visitPointer(&empty_byte_array_);
  visitor->visitPointer(&empty_object_array_);
  visitor->visitPointer(&empty_string_);

  // Visit modules
  visitor->visitPointer(&modules_);
}

void Runtime::visitThreadRoots(PointerVisitor* visitor) {
  for (Thread* thread = threads_; thread != nullptr; thread = thread->next()) {
    thread->handles()->visitPointers(visitor);
  }
}

void Runtime::addModule(Object* module) {
  Object* name = Module::cast(module)->name();
  Dictionary::atPut(modules(), name, Object::hash(name), module, this);
}

void Runtime::initializeModules() {
  modules_ = newDictionary();
  createBuiltinsModule();
}

void Runtime::createBuiltinsModule() {
  Object* name = newStringFromCString("builtins");
  assert(name != nullptr);
  Object* builtins = newModule(name);
  assert(builtins != nullptr);
  // Fill in builtins...
  addModule(builtins);
}

} // namespace python
