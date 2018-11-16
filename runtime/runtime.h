#pragma once

#include "callback.h"
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
  class NewValueCellCallback : public Callback<Object*> {
   public:
    explicit NewValueCellCallback(Runtime* runtime) : runtime_(runtime) {}
    Object* call() {
      return runtime_->newValueCell();
    }
    Runtime* runtime_;
  };

  Runtime();
  ~Runtime();

  Object* newByteArray(word length);
  Object* newByteArrayWithAll(const byte* data, word length);

  Object* newClass();
  Object* newClassWithId(ClassId class_id);

  Object* newCode();

  Object* newDictionary();

  Object* newBuiltinFunction(Function::Entry entry);
  Object* newFunction();

  Object* newList();

  Object* newModule(const Handle<Object>& name);

  Object* newObjectArray(word length);

  Object* newString(word length);
  Object* newStringFromCString(const char* c_string);

  Object* newValueCell();

  Object* internString(const Handle<Object>& string);

  void collectGarbage();

  Object* run(const char* buffer);

  Object* hash(Object* object);
  word siphash24(const byte* src, word size);

  // Determines whether or not object is a truthy value. Exceptions are
  // propagated using the normal exception handling mechanisms.
  bool isTruthy(Object* object);

  uword random();

  Heap* heap() {
    return &heap_;
  }

  void visitRoots(PointerVisitor* visitor);

  void addModule(const Handle<Module>& module);
  void moduleAddGlobal(
      const Handle<Module>& module,
      const Handle<Object>& key,
      const Handle<Object>& value);

  Object* moduleAddBuiltinFunction(
      const Handle<Module>& module,
      const char* name,
      const Function::Entry thunk);

  Object* findModule(const char* name);

  Object* classAt(ClassId class_id);

  ClassId newClassId();

  Object* buildClass() {
    return build_class_;
  }

  Object* interned() {
    return interned_;
  }

  Object* modules() {
    return modules_;
  }

  // Ensures that array has enough space for an atPut at index. If so, returns
  // array. If not, allocates and returns a new array with sufficient capacity
  // and identical contents.
  ObjectArray* ensureCapacity(const Handle<ObjectArray>& array, word index);

  void appendToList(const Handle<List>& list, const Handle<Object>& value);

  // Associate a value with the supplied key.
  //
  // This handles growing the backing ObjectArray if needed.
  void dictionaryAtPut(
      const Handle<Dictionary>& dict,
      const Handle<Object>& key,
      const Handle<Object>& value);

  // Look up the value associated with key.
  //
  // Returns true if the key was found and sets the associated value in value.
  // Returns false otherwise.
  bool dictionaryAt(
      const Handle<Dictionary>& dict,
      const Handle<Object>& key,
      Object** value);

  // Looks up and returns the value associated with the key.  If the key is
  // absent, calls thunk and inserts its result as the value.
  Object* dictionaryAtIfAbsentPut(
      const Handle<Dictionary>& dict,
      const Handle<Object>& key,
      Callback<Object*>* thunk);

  // Returns true if the dictionary contains the specified key.
  bool dictionaryIncludes(
      const Handle<Dictionary>& dict,
      const Handle<Object>& key);

  // Delete a key from the dictionary.
  //
  // Returns true if the key existed and sets the previous value in value.
  // Returns false otherwise.
  bool dictionaryRemove(
      const Handle<Dictionary>& dict,
      const Handle<Object>& key,
      Object** value);

  NewValueCellCallback* newValueCellCallback() {
    return &new_value_cell_callback_;
  }

  static const int kDictionaryGrowthFactor = 2;
  // Initial size of the dictionary. According to comments in CPython's
  // dictobject.c this accommodates the majority of dictionaries without needing
  // a resize (obviously this depends on the load factor used to resize the
  // dict).
  static const int kInitialDictionaryCapacity = 8;

 private:
  void initializeThreads();
  void initializeClasses();
  void initializeHeapClasses();
  void initializeImmediateClasses();
  void initializeInstances();
  void initializeInterned();
  void initializeModules();
  void initializeRandom();

  void createBuiltinsModule();
  void createSysModule();
  Object* createMainModule();

  void visitRuntimeRoots(PointerVisitor* visitor);
  void visitThreadRoots(PointerVisitor* visitor);

  Object* identityHash(Object* object);
  Object* immediateHash(Object* object);
  Object* valueHash(Object* object);

  Object* createMro(const ClassId* superclasses, word length);

  ObjectArray* dictionaryGrow(const Handle<ObjectArray>& data);

  // Looks up the supplied key
  //
  // If the key is found, this function returns true and sets index to the
  // index of the bucket that contains the value. If the key is not found, this
  // function returns false and sets index to the location where the key would
  // be inserted. If the dictionary is full, it sets index to -1.
  bool dictionaryLookup(
      const Handle<ObjectArray>& data,
      const Handle<Object>& key,
      const Handle<Object>& key_hash,
      word* index);

  // The size ensureCapacity grows to if array is empty
  static const int kInitialEnsuredCapacity = 4;

  Heap heap_;

  // Classes
  Object* class_table_;

  // Cached instances
  Object* empty_byte_array_;
  Object* empty_object_array_;
  Object* empty_string_;
  Object* build_class_;

  // Interned strings
  Object* interned_;

  // Modules
  Object* modules_;

  Thread* threads_;

  uword random_state_[2];
  uword hash_secret_[2];

  NewValueCellCallback new_value_cell_callback_;

  DISALLOW_COPY_AND_ASSIGN(Runtime);
};

} // namespace python
