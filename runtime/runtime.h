#pragma once

#include "handles.h"
#include "heap.h"

namespace python {

class Heap;
class Object;
class ObjectArray;
class PointerVisitor;
class Thread;

class Runtime {
 public:
  Runtime();
  ~Runtime();

  Object* newByteArray(word length);
  Object* newByteArrayFromCString(const char* c_string, word length);

  Object* newCode();

  Object* newDictionary();

  Object* newFunction();

  Object* newList();

  Object* newModule(Object* name);

  Object* newObjectArray(word length);

  Object* newString(word length);
  Object* newStringFromCString(const char* c_string);

  void collectGarbage();

  Object* hash(Object* object);

  uword random();

  Heap* heap() {
    return &heap_;
  };

  void visitRoots(PointerVisitor* visitor);

  void addModule(Object* module);

  Object* modules() {
    return modules_;
  };

  // Ensures that array has enough space for an atPut at index. If so, returns
  // array. If not, allocates and returns a new array with sufficient capacity
  // and identical contents.
  ObjectArray* ensureCapacity(const Handle<ObjectArray>& array, word index);

  void appendToList(const Handle<List>& list, const Handle<Object>& value);

 private:
  void initializeThreads();
  void initializeClasses();
  void initializeInstances();
  void initializeModules();
  void initializeRandom();

  void createBuiltinsModule();

  void visitRuntimeRoots(PointerVisitor* visitor);
  void visitThreadRoots(PointerVisitor* visitor);

  Object* identityHash(Object* object);
  Object* immediateHash(Object* object);
  Object* valueHash(Object* object);

  // The size ensureCapacity grows to if array is empty
  static const int kInitialEnsuredCapacity = 4;

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

  uword random_state_[2];
  uword hash_secret_[2];

  DISALLOW_COPY_AND_ASSIGN(Runtime);
};

} // namespace python
