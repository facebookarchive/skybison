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

  Object* newByteArray(word length);

  Object* newCode(
      int32 argcount,
      int32 kwonlyargcount,
      int32 nlocals,
      int32 stacksize,
      int32 flags,
      Object* code,
      Object* consts,
      Object* names,
      Object* varnames,
      Object* freevars,
      Object* cellvars,
      Object* filename,
      Object* name,
      int32 firstlineno,
      Object* lnotab);

  Object* newDictionary();

  Object* newList();

  Object* newModule(Object* name);

  Object* newObjectArray(word length);

  Object* newString(word length);
  Object* newStringFromCString(const char* c_string);

  void collectGarbage();

  Heap* heap() {
    return &heap_;
  };

  void visitRoots(PointerVisitor* visitor);

  void addModule(Object* module);

  Object* modules() {
    return modules_;
  };

 private:
  void initializeThreads();
  void initializeClasses();
  void initializeInstances();
  void initializeModules();

  void createBuiltinsModule();

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
  Object* empty_string_;

  // the equivalent of sys.modules in python
  Object* modules_;

  Thread* threads_;

  DISALLOW_COPY_AND_ASSIGN(Runtime);
};

} // namespace python
