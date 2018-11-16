#pragma once

#include "callback.h"
#include "handles.h"
#include "heap.h"
#include "symbols.h"
#include "view.h"

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
  explicit Runtime(word size);
  ~Runtime();

  Object* newBoundMethod(
      const Handle<Object>& function,
      const Handle<Object>& self);

  Object* newByteArray(word length, byte fill);
  Object* newByteArrayWithAll(View<byte> array);

  Object* newClass();
  Object* newClassWithId(ClassId class_id);

  Object* newClassMethod();

  Object* newCode();

  Object* newDictionary();
  Object* newDictionary(word initialSize);

  Object* newDouble(double value);

  Object* newSet();

  Object* newBuiltinFunction(Function::Entry entry, Function::Entry entryKw);
  Object* newFunction();

  Object* newInstance(ClassId class_id);

  Object* newInteger(word value);

  Object* newList();

  Object* newModule(const Handle<Object>& name);

  // Returns a LargeInteger that stores the value in ptr.
  Object* newIntegerFromCPointer(void* ptr);

  Object* newObjectArray(word length);

  Object* newRange(word start, word stop, word step);

  Object* newRangeIterator(const Handle<Object>& iterable);

  Object* newStringFromCString(const char* c_string);
  Object* newStringWithAll(View<byte> code_units);

  Object* stringConcat(const Handle<String>& a, const Handle<String>& b);

  Object* newValueCell();

  Object* newWeakRef();

  Object* internString(const Handle<Object>& string);

  void collectGarbage();

  Object* run(const char* buffer);

  Object* hash(Object* object);
  word siphash24(View<byte> array);

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
      const Function::Entry entry,
      const Function::Entry entryKw);

  Object* findModule(const Handle<Object>& name);

  Object* moduleAt(const Handle<Module>& module, const Handle<Object>& key);
  void moduleAtPut(
      const Handle<Module>& module,
      const Handle<Object>& key,
      const Handle<Object>& value);

  Object* importModule(const Handle<Object>& name);

  // importModuleFromBuffer is exposed for use by the tests. We may be able to
  // remove this later.
  Object* importModuleFromBuffer(
      const char* buffer,
      const Handle<Object>& name);

  Object* classAt(ClassId class_id);
  Object* classOf(Object* object);

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

  Symbols* symbols() {
    return symbols_;
  }

  Object* getIter(const Handle<Object>& iterable);

  // Ensures that array has enough space for an atPut at index. If so, returns
  // array. If not, allocates and returns a new array with sufficient capacity
  // and identical contents.
  ObjectArray* ensureCapacity(const Handle<ObjectArray>& array, word index);

  // Appends an element to the end of the list.
  void listAdd(const Handle<List>& list, const Handle<Object>& value);

  // Inserts an element to the specified index of the list.
  // When index >= len(list) it is equivalent to appending to the list.
  void
  listInsert(const Handle<List>& list, const Handle<Object>& value, word index);

  // Deletes an element in the specified list index.
  void listPop(const Handle<List>& list, word index);

  // Return a new list that is composed of list repeated ntimes
  Object* listReplicate(Thread* thread, const Handle<List>& list, word ntimes);

  // Associate a value with the supplied key.
  //
  // This handles growing the backing ObjectArray if needed.
  void dictionaryAtPut(
      const Handle<Dictionary>& dict,
      const Handle<Object>& key,
      const Handle<Object>& value);

  // Look up the value associated with key. Returns Error::object() if the
  // key was not found.
  Object* dictionaryAt(
      const Handle<Dictionary>& dict,
      const Handle<Object>& key);

  // Looks up and returns the value associated with the key.  If the key is
  // absent, calls thunk and inserts its result as the value.
  Object* dictionaryAtIfAbsentPut(
      const Handle<Dictionary>& dict,
      const Handle<Object>& key,
      Callback<Object*>* thunk);

  // Stores value in a ValueCell stored at key in dict. Careful to
  // reuse an existing value cell if one exists since it may be shared.
  Object* dictionaryAtPutInValueCell(
      const Handle<Dictionary>& dict,
      const Handle<Object>& key,
      const Handle<Object>& value);

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

  ObjectArray* dictionaryKeys(const Handle<Dictionary>& dict);

  // Set related function, based on dictionary.
  // Add a value to set and return the object in set.
  Object* setAdd(const Handle<Set>& set, const Handle<Object>& value);

  // Returns true if the set contains the specified value.
  bool setIncludes(const Handle<Set>& set, const Handle<Object>& value);

  // Delete a key from the set, returns true if the key existed.
  bool setRemove(const Handle<Set>& set, const Handle<Object>& value);

  NewValueCellCallback* newValueCellCallback() {
    return &new_value_cell_callback_;
  }

  static char* compile(const char* src);

  // Performs a simple scan of the bytecode and collects all attributes that
  // are set via `self.<attribute> =` into attributes.
  void collectAttributes(
      const Handle<Code>& code,
      const Handle<Dictionary>& attributes);

  // Constructs the mapping of where attributes are stored in instances.
  //
  // The result is an ObjectArray of attribute names. For a given attribute,
  // its offset in an instance is the same as the offset of its name in the
  // returned ObjectArray.
  //
  // This is computed by scanning the constructors of every klass in klass's
  // MRO.
  ObjectArray* computeInstanceAttributeMap(const Handle<Class>& klass);

  // Returns klass's __init__ method, or None
  Object* classConstructor(const Handle<Class>& klass);

  // Looks up name in the dict of each entry in klass's MRO.
  //
  // This is equivalent to CPython's PyType_Lookup. Returns the Error object if
  // the name wasn't found.
  Object* lookupNameInMro(
      Thread* thread,
      const Handle<Class>& klass,
      const Handle<Object>& name);

  // Implements `receiver.name`
  Object* attributeAt(
      Thread* thread,
      const Handle<Object>& receiver,
      const Handle<Object>& name);

  // Implements `receiver.name = value`
  Object* attributeAtPut(
      Thread* thread,
      const Handle<Object>& receiver,
      const Handle<Object>& name,
      const Handle<Object>& value);

  // Pre-computes fast_globals for functions.
  Object* computeFastGlobals(
      const Handle<Code>& code,
      const Handle<Dictionary>& globals,
      const Handle<Dictionary>& builtins);

  // Converts the offset in code's bytecode into the corresponding line number
  // in the backing source file.
  word
  codeOffsetToLineNum(Thread* thread, const Handle<Code>& code, word offset);

  // Return true if subclass is a subclass of superclass
  Object* isSubClass(
      const Handle<Class>& subclass,
      const Handle<Class>& superclass);

  // Return true if obj is an instance of a subclass of klass
  Object* isInstance(const Handle<Object>& obj, const Handle<Class>& klass);

  static const int kDictionaryGrowthFactor = 2;
  // Initial size of the dictionary. According to comments in CPython's
  // dictobject.c this accommodates the majority of dictionaries without needing
  // a resize (obviously this depends on the load factor used to resize the
  // dict).
  static const int kInitialDictionaryCapacity = 8;

  // Initial data of the set.
  static const int kSetGrowthFactor = 2;
  static const int kInitialSetCapacity = 8;

  // Explicitly seed the random number generator
  void seedRandom(uword random_state[2], uword hash_secret[2]) {
    random_state_[0] = random_state[0];
    random_state_[1] = random_state[1];
    hash_secret_[0] = hash_secret[0];
    hash_secret_[1] = hash_secret[1];
  }

 private:
  void initializeThreads();
  void initializeClasses();
  void initializeHeapClasses();
  void initializeImmediateClasses();
  void initializePrimitiveInstances();
  void initializeInterned();
  void initializeModules();
  void initializeRandom();
  void initializeSymbols();

  template <typename... Args>
  void initializeHeapClass(const char* name, Args... args);
  void initializeListClass();
  void initializeSmallIntClass();
  void initializeClassMethodClass();

  void createBuiltinsModule();
  void createSysModule();
  Object* createMainModule();

  Object* executeModule(const char* buffer, const Handle<Module>& module);

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

  bool setLookup(
      const Handle<ObjectArray>& data,
      const Handle<Object>& key,
      const Handle<Object>& key_hash,
      word* index);

  ObjectArray* setGrow(const Handle<ObjectArray>& data);

  void moduleAddBuiltinPrint(const Handle<Module>& module);

  // Helper function to add builtin functions to classes
  void classAddBuiltinFunction(
      const Handle<Class>& klass,
      Object* name,
      Function::Entry entry,
      Function::Entry entryKw);

  // Generic attribute lookup code used for class objects
  Object* classGetAttr(
      Thread* thread,
      const Handle<Object>& receiver,
      const Handle<Object>& name);

  // Generic attribute setting code used for class objects
  Object* classSetAttr(
      Thread* thread,
      const Handle<Object>& receiver,
      const Handle<Object>& name,
      const Handle<Object>& value);

  // Generic attribute lookup code used for instance objects
  Object* instanceGetAttr(
      Thread* thread,
      const Handle<Object>& receiver,
      const Handle<Object>& name);

  // Generic attribute setting code used for instance objects
  Object* instanceSetAttr(
      Thread* thread,
      const Handle<Object>& receiver,
      const Handle<Object>& name,
      const Handle<Object>& value);

  // Generic attribute lookup code used for module objects
  Object* moduleGetAttr(
      Thread* thread,
      const Handle<Object>& receiver,
      const Handle<Object>& name);

  // Generic attribute setting code used for module objects
  Object* moduleSetAttr(
      Thread* thread,
      const Handle<Object>& receiver,
      const Handle<Object>& name,
      const Handle<Object>& value);

  // Returns whether object's class provides a __set__ method
  bool isDataDescriptor(Thread* thread, const Handle<Object>& object);

  // Returns whether object's class provides a __get__ method
  bool isNonDataDescriptor(Thread* thread, const Handle<Object>& object);

  // The size ensureCapacity grows to if array is empty
  static const int kInitialEnsuredCapacity = 4;

  Heap heap_;

  // Classes
  Object* class_table_;

  // Cached instances
  Object* empty_byte_array_;
  Object* empty_object_array_;
  Object* ellipsis_;
  Object* build_class_;
  Object* print_default_end_;

  // Interned strings
  Object* interned_;

  // Modules
  Object* modules_;

  Thread* threads_;

  uword random_state_[2];
  uword hash_secret_[2];

  NewValueCellCallback new_value_cell_callback_;

  Symbols* symbols_;

  DISALLOW_COPY_AND_ASSIGN(Runtime);
};

} // namespace python
