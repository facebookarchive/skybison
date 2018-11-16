#pragma once

#include "bytecode.h"
#include "callback.h"
#include "capi-handles.h"
#include "handles.h"
#include "heap.h"
#include "interpreter.h"
#include "layout.h"
#include "symbols.h"
#include "tracked-allocation.h"
#include "view.h"

namespace python {

class AttributeInfo;
class Heap;
class Object;
class ObjectArray;
class PointerVisitor;
class Thread;

struct BuiltinAttribute {
  SymbolId name;
  int offset;
};

struct BuiltinMethod {
  SymbolId name;
  Object* (*address)(Thread* thread, Frame* frame, word nargs);
};

class Runtime {
 public:
  class NewValueCellCallback : public Callback<Object*> {
   public:
    explicit NewValueCellCallback(Runtime* runtime) : runtime_(runtime) {}
    Object* call() { return runtime_->newValueCell(); }
    Runtime* runtime_;
  };

  Runtime();
  explicit Runtime(word size);
  ~Runtime();

  Object* newBoundMethod(const Handle<Object>& function,
                         const Handle<Object>& self);

  Object* newByteArray(word length, byte fill);
  Object* newByteArrayWithAll(View<byte> array);

  Object* newClass();
  Object* newClassWithMetaclass(LayoutId metaclass_id);

  Object* newClassMethod();

  Object* newCode();

  Object* newComplex(double real, double imag);

  Object* newDict();
  Object* newDict(word initial_size);

  Object* newFloat(double value);

  Object* newSet();

  Object* newBuiltinFunction(SymbolId name, Function::Entry entry,
                             Function::Entry entry_kw,
                             Function::Entry entry_ex);
  Object* newFunction();

  Object* newInstance(const Handle<Layout>& layout);

  Object* newInt(word value);

  Object* newIntWithDigits(View<uword> digits);

  Object* newLayout();

  Object* newList();

  Object* newListIterator(const Handle<Object>& list);

  Object* newModule(const Handle<Object>& name);

  // Returns an Int that stores the numerical address of the pointer.
  Object* newIntFromCPointer(void* ptr);

  Object* newObjectArray(word length);

  Object* newProperty(const Handle<Object>& getter,
                      const Handle<Object>& setter,
                      const Handle<Object>& deleter);

  Object* newRange(word start, word stop, word step);

  Object* newRangeIterator(const Handle<Object>& iterable);

  Object* newSlice(const Handle<Object>& start, const Handle<Object>& stop,
                   const Handle<Object>& step);

  Object* newStaticMethod();

  Object* newStringFromCString(const char* c_string);
  // Creates a new string constructed from a format and a list of arguments,
  // similar to sprintf.
  Object* newStringFromFormat(const char* fmt, ...) FORMAT_ATTRIBUTE(2, 3);
  Object* newStringWithAll(View<byte> code_units);

  Object* newSuper();

  void processCallbacks();

  Object* stringConcat(const Handle<String>& a, const Handle<String>& b);

  // Rudimentary support for '%' operator
  Object* stringFormat(Thread* thread, const Handle<String>& fmt,
                       const Handle<ObjectArray>& args);

  Object* newValueCell();

  Object* newWeakRef();

  Object* internString(const Handle<Object>& string);
  Object* internStringFromCString(const char* c_string);

  void collectGarbage();

  Object* run(const char* buffer);

  Object* runFromCString(const char* c_string);

  Object* hash(Object* object);
  word siphash24(View<byte> array);

  uword random();

  void setArgv(int argc, const char** argv);

  Heap* heap() { return &heap_; }

  void visitRoots(PointerVisitor* visitor);

  void addModule(const Handle<Module>& module);
  Object* moduleAddGlobal(const Handle<Module>& module, SymbolId name,
                          const Handle<Object>& value);

  Object* moduleAddBuiltinFunction(const Handle<Module>& module, SymbolId name,
                                   const Function::Entry entry,
                                   const Function::Entry entry_kw,
                                   const Function::Entry entry_ex);

  Object* findModule(const Handle<Object>& name);

  Object* moduleAt(const Handle<Module>& module, const Handle<Object>& key);
  void moduleAtPut(const Handle<Module>& module, const Handle<Object>& key,
                   const Handle<Object>& value);

  Object* importModule(const Handle<Object>& name);

  // importModuleFromBuffer is exposed for use by the tests. We may be able to
  // remove this later.
  Object* importModuleFromBuffer(const char* buffer,
                                 const Handle<Object>& name);

  Object* typeOf(Object* object);

  Object* typeAt(LayoutId layout_id);
  Object* layoutAt(LayoutId layout_id);
  void layoutAtPut(LayoutId layout_id, Object* object);

  // Bootstrapping primitive for creating a built-in class that has built-in
  // attributes.
  Object* addEmptyBuiltinClass(SymbolId name, LayoutId subclass_id,
                               LayoutId superclass_id);
  Object* addBuiltinClass(SymbolId name, LayoutId subclass_id,
                          LayoutId superclass_id,
                          View<BuiltinAttribute> attributes);

  LayoutId reserveLayoutId();

  SymbolId binaryOperationSelector(Interpreter::BinaryOp op);
  SymbolId swappedBinaryOperationSelector(Interpreter::BinaryOp op);

  SymbolId inplaceOperationSelector(Interpreter::BinaryOp op);

  SymbolId comparisonSelector(CompareOp op);
  SymbolId swappedComparisonSelector(CompareOp op);

  // Return true if the selector does not appear in the MRO object below object.
  bool isMethodOverloaded(Thread* thread, const Handle<Type>& type,
                          SymbolId selector);

  Object* buildClass() { return build_class_; }

  Object* interned() { return interned_; }

  Object* modules() { return modules_; }

  Object* notImplemented() { return not_implemented_; }

  Object* apiHandles() { return api_handles_; }

  Object* extensionTypes() { return extension_types_; }

  Symbols* symbols() { return symbols_; }

  Object* getIter(const Handle<Object>& iterable);

  // Ensures that array has enough space for an atPut at index. If so, returns
  // array. If not, allocates and returns a new array with sufficient capacity
  // and identical contents.
  void listEnsureCapacity(const Handle<List>& list, word index);

  // Appends an element to the end of the list.
  void listAdd(const Handle<List>& list, const Handle<Object>& value);

  // Extends a list from an iterator.
  void listExtend(const Handle<List>& dest, const Handle<Object>& iterable);

  // Inserts an element to the specified index of the list.
  // When index >= len(list) it is equivalent to appending to the list.
  void listInsert(const Handle<List>& list, const Handle<Object>& value,
                  word index);

  // Removes and returns an element from the specified list index.
  // Expects index to be within [0, len(list)]
  Object* listPop(const Handle<List>& list, word index);

  // Return a new list that is composed of list repeated ntimes
  Object* listReplicate(Thread* thread, const Handle<List>& list, word ntimes);

  // Associate a value with the supplied key.
  //
  // This handles growing the backing ObjectArray if needed.
  void dictAtPut(const Handle<Dict>& dict, const Handle<Object>& key,
                 const Handle<Object>& value);

  // Look up the value associated with key. Returns Error::object() if the
  // key was not found.
  Object* dictAt(const Handle<Dict>& dict, const Handle<Object>& key);

  // Looks up and returns the value associated with the key.  If the key is
  // absent, calls thunk and inserts its result as the value.
  Object* dictAtIfAbsentPut(const Handle<Dict>& dict, const Handle<Object>& key,
                            Callback<Object*>* thunk);

  // Stores value in a ValueCell stored at key in dict. Careful to
  // reuse an existing value cell if one exists since it may be shared.
  Object* dictAtPutInValueCell(const Handle<Dict>& dict,
                               const Handle<Object>& key,
                               const Handle<Object>& value);

  // Returns true if the dict contains the specified key.
  bool dictIncludes(const Handle<Dict>& dict, const Handle<Object>& key);

  // Delete a key from the dict.
  //
  // Returns true if the key existed and sets the previous value in value.
  // Returns false otherwise.
  bool dictRemove(const Handle<Dict>& dict, const Handle<Object>& key,
                  Object** value);

  // Support explicit hash value of key to do dictAtPut.
  void dictAtPutWithHash(const Handle<Dict>& dict, const Handle<Object>& key,
                         const Handle<Object>& value,
                         const Handle<Object>& key_hash);

  // Support explicit hash value of key to do dictAt.
  Object* dictAtWithHash(const Handle<Dict>& dict, const Handle<Object>& key,
                         const Handle<Object>& key_hash);

  ObjectArray* dictKeys(const Handle<Dict>& dict);

  // Set related function, based on dict.
  // Add a value to set and return the object in set.
  Object* setAdd(const Handle<Set>& set, const Handle<Object>& value);

  Object* setAddWithHash(const Handle<Set>& set, const Handle<Object>& value,
                         const Handle<Object>& key_hash);

  // Returns true if the set contains the specified value.
  bool setIncludes(const Handle<Set>& set, const Handle<Object>& value);

  // Delete a key from the set, returns true if the key existed.
  bool setRemove(const Handle<Set>& set, const Handle<Object>& value);

  // Update a set from an iterator.
  void setUpdate(const Handle<Set>& set, const Handle<Object>& iterable);

  NewValueCellCallback* newValueCellCallback() {
    return &new_value_cell_callback_;
  }

  static char* compile(const char* src);

  // Performs a simple scan of the bytecode and collects all attributes that
  // are set via `self.<attribute> =` into attributes.
  void collectAttributes(const Handle<Code>& code,
                         const Handle<Dict>& attributes);

  // Constructs the initial layout for instances of type.
  //
  // The layout contains the set of in-object attributes. This is computed by
  // scanning the constructors of every klass in klass's MRO.
  Object* computeInitialLayout(Thread* thread, const Handle<Type>& klass,
                               LayoutId base_layout_id);

  // Returns type's __init__ method, or None
  Object* classConstructor(const Handle<Type>& type);

  // Looks up name in the dict of each entry in type's MRO.
  //
  // This is equivalent to CPython's PyType_Lookup. Returns the Error object if
  // the name wasn't found.
  Object* lookupNameInMro(Thread* thread, const Handle<Type>& type,
                          const Handle<Object>& name);

  // Looks up symbol name in the dict of each entry in type's MRO.
  Object* lookupSymbolInMro(Thread* thread, const Handle<Type>& type,
                            SymbolId symbol);

  // Implements `receiver.name`
  Object* attributeAt(Thread* thread, const Handle<Object>& receiver,
                      const Handle<Object>& name);

  // Implements `receiver.name = value`
  Object* attributeAtPut(Thread* thread, const Handle<Object>& receiver,
                         const Handle<Object>& name,
                         const Handle<Object>& value);

  // Implements `del receiver.name`
  Object* attributeDel(Thread* thread, const Handle<Object>& receiver,
                       const Handle<Object>& name);

  // Attribute lookup primitive for instances.
  //
  // This operates directly on the instance and does not respect Python
  // semantics for attribute lookup. Returns Error::object() if the attribute
  // isn't found.
  Object* instanceAt(Thread* thread, const Handle<HeapObject>& instance,
                     const Handle<Object>& name);

  // Attribute setting primitive for instances.
  //
  // This operates directly on the instance and does not respect Python
  // semantics for attribute storage. This handles mutating the instance's
  // layout if the attribute does not already exist on the instance.
  Object* instanceAtPut(Thread* thread, const Handle<HeapObject>& instance,
                        const Handle<Object>& name,
                        const Handle<Object>& value);

  // Attribute deletion primitive for instances.
  //
  // This operates directly on the instance and does not respect Python
  // semantics for attribute deletion. This handles mutating the layout if the
  // attribute exists. Returns Error::object() if the attribute is not found.
  Object* instanceDel(Thread* thread, const Handle<HeapObject>& instance,
                      const Handle<Object>& name);

  // Looks up the named attribute in the layout.
  //
  // If the attribute is found this returns true and sets info.
  // Returns false otherwise.
  bool layoutFindAttribute(Thread* thread, const Handle<Layout>& layout,
                           const Handle<Object>& name, AttributeInfo* info);

  // Add the attribute to the overflow array.
  //
  // This returns a new layout by either following a pre-existing edge or
  // adding one.
  Object* layoutAddAttribute(Thread* thread, const Handle<Layout>& layout,
                             const Handle<Object>& name, word flags);

  // Delete the named attribute from the layout.
  //
  // If the attribute exists, this returns a new layout by either following
  // a pre-existing edge or adding one.
  //
  // If the attribute doesn't exist, Error::object() is returned.
  Object* layoutDeleteAttribute(Thread* thread, const Handle<Layout>& layout,
                                const Handle<Object>& name);

  // Pre-computes fast_globals for functions.
  Object* computeFastGlobals(const Handle<Code>& code,
                             const Handle<Dict>& globals,
                             const Handle<Dict>& builtins);

  LayoutId computeBuiltinBaseClass(const Handle<Type>& klass);

  // Adds a builtin function with a positional entry point definition
  // using the default keyword and splatting entry points.
  void classAddBuiltinFunction(const Handle<Type>& type, SymbolId name,
                               Function::Entry entry);

  // Adds a builtin function with positional and keyword entry point
  // definitions, using the default splatting entry point.
  void classAddBuiltinFunctionKw(const Handle<Type>& type, SymbolId name,
                                 Function::Entry entry,
                                 Function::Entry entry_kw);

  // Adds a builtin function with positional, keyword & splatting entry point
  // definitions
  void classAddBuiltinFunctionKwEx(const Handle<Type>& type, SymbolId name,
                                   Function::Entry entry,
                                   Function::Entry entry_kw,
                                   Function::Entry entry_ex);

  // Helper function to add extension functions to extension classes
  void classAddExtensionFunction(const Handle<Type>& type, SymbolId name,
                                 void* c_function);

  // Converts the offset in code's bytecode into the corresponding line number
  // in the backing source file.
  word codeOffsetToLineNum(Thread* thread, const Handle<Code>& code,
                           word offset);

  // Return true if subclass is a subclass of superclass
  Object* isSubClass(const Handle<Type>& subclass,
                     const Handle<Type>& superclass);

  bool hasSubClassFlag(Object* instance, Type::Flag flag) {
    return Type::cast(typeAt(instance->layoutId()))->hasFlag(flag);
  }

  // Returns whether or not instance is an instance of Type or a subclass of
  // Type
  //
  // This is equivalent to PyType_Check.
  bool isInstanceOfClass(Object* instance) {
    if (instance->isType()) {
      return true;
    }
    // The reinterpret_cast here is needed to avoid self-recursion when this is
    // called by Type::cast(). It is safe, as typeOf() is guaranteed to return
    // a Type*.
    return reinterpret_cast<Type*>(typeOf(instance))
        ->hasFlag(Type::Flag::kTypeSubclass);
  }

  bool isInstanceOfList(Object* instance) {
    if (instance->isList()) {
      return true;
    }
    return Type::cast(typeOf(instance))->hasFlag(Type::Flag::kListSubclass);
  }

  // Return true if obj is an instance of a subclass of klass
  Object* isInstance(const Handle<Object>& obj, const Handle<Type>& klass);

  // Clear the allocated memory from all extension related objects
  void deallocExtensions();

  static const int kDictGrowthFactor = 2;
  // Initial size of the dict. According to comments in CPython's
  // dictobject.c this accommodates the majority of dictionaries without needing
  // a resize (obviously this depends on the load factor used to resize the
  // dict).
  static const int kInitialDictCapacity = 8;

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

  // Returns whether object's class provides a __set__ method
  bool isDataDescriptor(Thread* thread, const Handle<Object>& object);

  // Returns whether object's class provides a __get__ method
  bool isNonDataDescriptor(Thread* thread, const Handle<Object>& object);

  // Returns whether object's class provides a __delete__ method
  bool isDeleteDescriptor(Thread* thread, const Handle<Object>& object);

  // Returns a pointer of an initialized PyTypeObject
  ApiTypeHandle* builtinTypeHandles(ExtensionTypes type) {
    return builtin_type_handles_[static_cast<int>(type)];
  }

  // Saves an initialized PyTypeObject
  void addBuiltinTypeHandle(ApiTypeHandle* type_handle) {
    builtin_type_handles_.push_back(type_handle);
  }

  // Linked list of all tracked allocations
  TrackedAllocation** trackedAllocations() { return &tracked_allocations_; }

  // Clear the allocated memory from all extension related objects
  void freeTrackedAllocations();

  // Creates an layout with a new ID and no attributes.
  Object* layoutCreateEmpty(Thread* thread);

 private:
  void initializeThreads();
  void initializeClasses();
  void initializeExceptionClasses();
  void initializeLayouts();
  void initializeHeapClasses();
  void initializeImmediateClasses();
  void initializePrimitiveInstances();
  void initializeInterned();
  void initializeModules();
  void initializeApiHandles();
  void initializeRandom();
  void initializeSymbols();

  void initializeBoolClass();
  void initializeClassMethodClass();
  void initializeFloatClass();
  void initializeFunctionClass();
  void initializeObjectClass();
  void initializeObjectArrayClass();
  void initializePropertyClass();
  void initializeRefClass();
  void initializeSetClass();
  void initializeStaticMethodClass();
  void initializeStrClass();
  void initializeSuperClass();
  void initializeTypeClass();

  void createBuiltinsModule();
  void createSysModule();
  void createTimeModule();
  void createWeakRefModule();
  Object* createMainModule();

  Object* executeModule(const char* buffer, const Handle<Module>& module);

  void visitRuntimeRoots(PointerVisitor* visitor);
  void visitThreadRoots(PointerVisitor* visitor);

  Object* identityHash(Object* object);
  Object* immediateHash(Object* object);
  Object* valueHash(Object* object);

  Object* createMro(const Handle<Layout>& subclass_layout,
                    LayoutId superclass_id);

  ObjectArray* dictGrow(const Handle<ObjectArray>& data);

  // Looks up the supplied key
  //
  // If the key is found, this function returns true and sets index to the
  // index of the bucket that contains the value. If the key is not found, this
  // function returns false and sets index to the location where the key would
  // be inserted. If the dict is full, it sets index to -1.
  bool dictLookup(const Handle<ObjectArray>& data, const Handle<Object>& key,
                  const Handle<Object>& key_hash, word* index);

  bool setLookup(const Handle<ObjectArray>& data, const Handle<Object>& key,
                 const Handle<Object>& key_hash, word* index);

  ObjectArray* setGrow(const Handle<ObjectArray>& data);

  // Generic attribute lookup code used for class objects
  Object* classGetAttr(Thread* thread, const Handle<Object>& receiver,
                       const Handle<Object>& name);

  // Generic attribute setting code used for class objects
  Object* classSetAttr(Thread* thread, const Handle<Object>& receiver,
                       const Handle<Object>& name, const Handle<Object>& value);

  // Generic attribute deletion code used for class objects
  Object* classDelAttr(Thread* thread, const Handle<Object>& receiver,
                       const Handle<Object>& name);

  // Generic attribute lookup code used for instance objects
  Object* instanceGetAttr(Thread* thread, const Handle<Object>& receiver,
                          const Handle<Object>& name);

  // Generic attribute setting code used for instance objects
  Object* instanceSetAttr(Thread* thread, const Handle<Object>& receiver,
                          const Handle<Object>& name,
                          const Handle<Object>& value);

  // Generic attribute deletion code used for instance objects
  Object* instanceDelAttr(Thread* thread, const Handle<Object>& receiver,
                          const Handle<Object>& name);

  // Generic attribute lookup code used for module objects
  Object* moduleGetAttr(Thread* thread, const Handle<Object>& receiver,
                        const Handle<Object>& name);

  // Generic attribute setting code used for module objects
  Object* moduleSetAttr(Thread* thread, const Handle<Object>& receiver,
                        const Handle<Object>& name,
                        const Handle<Object>& value);

  // Generic attribute deletion code used for module objects
  Object* moduleDelAttr(Thread* thread, const Handle<Object>& receiver,
                        const Handle<Object>& name);

  // Specialized attribute lookup code used for super objects
  Object* superGetAttr(Thread* thread, const Handle<Object>& receiver,
                       const Handle<Object>& name);

  // helper function add builtin types
  void moduleAddBuiltinType(const Handle<Module>& module, SymbolId name,
                            LayoutId layout_id);

  // Creates a layout that is a subclass of a built-in class and zero or more
  // additional built-in attributes.
  Object* layoutCreateSubclassWithBuiltins(LayoutId subclass_id,
                                           LayoutId superclass_id,
                                           View<BuiltinAttribute> attributes);

  // Appends attribute entries for fixed attributes to an array of in-object
  // attribute entries starting at a specific index.  Useful for constructing
  // the in-object attributes array for built-in classes with fixed attributes.
  void appendBuiltinAttributes(View<BuiltinAttribute> attributes,
                               const Handle<ObjectArray>& dst, word index);

  // Appends the edge to the list of edges.
  //
  // edges is expected to be a list of edges (label, layout pairs) corresponding
  // to a class of shape altering mutations (e.g. attribute addition).
  void layoutAddEdge(const Handle<List>& edges, const Handle<Object>& label,
                     const Handle<Object>& layout);

  // Create a new tuple for the name, info pair and return a new tuple
  // containing entries + entry.
  Object* layoutAddAttributeEntry(Thread* thread,
                                  const Handle<ObjectArray>& entries,
                                  const Handle<Object>& name,
                                  AttributeInfo info);

  // Follow the edge with the supplied label, if one exists.
  //
  // Edges is expected to be a list composed of flattened two tuples. The
  // elements of each tuple are expected to be, in order,
  //
  //   1. The label
  //   2. The layout that would be reached by following the edge.
  //
  // If an edge with the supplied label exists the corresponding layout is
  // returned. If no edge with the supplied label exists Error::object() is
  // returned.
  Object* layoutFollowEdge(const Handle<List>& edges,
                           const Handle<Object>& label);

  // Creates a new layout that will be a child layout of the supplied parent.
  //
  // The new layout shares the in-object and overflow attributes with the
  // parent and contains no outgoing edges.
  Object* layoutCreateChild(Thread* thread, const Handle<Layout>& parent);

  // The size listEnsureCapacity grows to if array is empty
  static const int kInitialEnsuredCapacity = 4;

  Heap heap_;

  // An ObjectArray of Layout objects, indexed by layout id.
  Object* layouts_;

  // Cached instances
  Object* empty_byte_array_;
  Object* empty_object_array_;
  Object* ellipsis_;
  Object* not_implemented_;
  Object* build_class_;
  Object* print_default_end_;

  // Interned strings
  Object* interned_;

  // Modules
  Object* modules_;

  // ApiHandles
  Object* api_handles_;

  // Extension Types
  Object* extension_types_;

  // Weak reference callback list
  Object* callbacks_;

  Thread* threads_;

  uword random_state_[2];
  uword hash_secret_[2];

  NewValueCellCallback new_value_cell_callback_;

  Symbols* symbols_;

  Vector<ApiTypeHandle*> builtin_type_handles_;

  TrackedAllocation* tracked_allocations_;

  DISALLOW_COPY_AND_ASSIGN(Runtime);
};

}  // namespace python
