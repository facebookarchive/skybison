#pragma once

#include "heap.h"

namespace python {

class Heap;
class Object;
class PointerVisitor;
class Thread;

class Runtime {
 public:
  Runtime();
  ~Runtime();

  Object* createByteArray(intptr_t length);

  Object* createCode(
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
      Object* lnotab);

  Object* createObjectArray(intptr_t length);

  Object* createString(intptr_t length);

  Object* createList();

  void collectGarbage();

  Heap* heap() {
    return &heap_;
  };

  void visitRoots(PointerVisitor* visitor);

  Object* modules() {
    return modules_;
  };

 private:
  void initializeThreads();
  void initializeClasses();
  void initializeInstances();
  void initializeModules();

  void visitRuntimeRoots(PointerVisitor* visitor);
  void visitThreadRoots(PointerVisitor* visitor);

  Heap heap_;

  Object* byte_array_class_;
  Object* class_class_;
  Object* code_class_;
  Object* dictionary_class_;
  Object* function_class_;
  Object* list_class_;
  Object* module_class_;
  Object* object_array_class_;
  Object* string_class_;

  Object* empty_byte_array_;
  Object* empty_object_array_;

  // the equivalent of sys.modules in python
  Object* modules_;

  Thread* threads_;

  DISALLOW_COPY_AND_ASSIGN(Runtime);
};

} // namespace python
