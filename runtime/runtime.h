#pragma once

#include "bytecode.h"
#include "callback.h"
#include "capi-handles.h"
#include "handles.h"
#include "heap.h"
#include "interpreter.h"
#include "layout.h"
#include "symbols.h"
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
  RawObject (*address)(Thread* thread, Frame* frame, word nargs);
};

enum class DictUpdateType { Update, Merge };

enum class SetLookupType { Lookup, Insertion };

enum class StrStripDirection { Left, Right, Both };

class Runtime {
 public:
  class NewValueCellCallback : public Callback<RawObject> {
   public:
    explicit NewValueCellCallback(Runtime* runtime) : runtime_(runtime) {}
    RawObject call() { return runtime_->newValueCell(); }
    Runtime* runtime_;
  };

  Runtime();
  explicit Runtime(word size);
  ~Runtime();

  RawObject newBoundMethod(const Handle<Object>& function,
                           const Handle<Object>& self);

  RawObject newBytes(word length, byte fill);
  RawObject newBytesWithAll(View<byte> array);

  RawObject newClass();
  RawObject newClassWithMetaclass(LayoutId metaclass_id);

  RawObject newClassMethod();

  RawObject newCode();

  RawObject newComplex(double real, double imag);

  RawObject newCoroutine();

  RawObject newDict();
  RawObject newDictWithSize(word initial_size);

  RawObject newFloat(double value);

  RawObject newSet();
  RawObject newSetWithSize(word initial_size);

  RawObject newBuiltinFunction(SymbolId name, Function::Entry entry,
                               Function::Entry entry_kw,
                               Function::Entry entry_ex);
  RawObject newFunction();

  RawObject newGenerator();

  RawObject newHeapFrame(const Handle<Code>& code);

  RawObject newInstance(const Handle<Layout>& layout);

  // Create a new Int from a signed value.
  RawObject newInt(word value);

  // Create a new Int from an unsigned value.
  RawObject newIntFromUnsigned(uword value);

  // Create a new Int from a sequence of digits, which will be interpreted as a
  // signed, two's-complement number. The digits must satisfy the invariants
  // listed on the LargeInt class.
  RawObject newIntWithDigits(View<uword> digits);

  RawObject newLayout();

  RawObject newList();

  RawObject newListIterator(const Handle<Object>& list);

  RawObject newModule(const Handle<Object>& name);

  // Returns an Int that stores the numerical address of the pointer.
  RawObject newIntFromCPtr(void* ptr);

  RawObject newObjectArray(word length);

  RawObject newProperty(const Handle<Object>& getter,
                        const Handle<Object>& setter,
                        const Handle<Object>& deleter);

  RawObject newRange(word start, word stop, word step);

  RawObject newRangeIterator(const Handle<Object>& iterable);

  RawObject newSetIterator(const Handle<Object>& set);

  RawObject newSlice(const Handle<Object>& start, const Handle<Object>& stop,
                     const Handle<Object>& step);

  RawObject newStaticMethod();

  RawObject newStrFromCStr(const char* c_str);
  // Creates a new string constructed from a format and a list of arguments,
  // similar to sprintf.
  RawObject newStrFromFormat(const char* fmt, ...) FORMAT_ATTRIBUTE(2, 3);
  RawObject newStrWithAll(View<byte> code_units);

  RawObject newSuper();

  RawObject newTupleIterator(const Handle<Object>& iterable);

  void processCallbacks();

  RawObject strConcat(const Handle<Str>& left, const Handle<Str>& right);
  RawObject strJoin(Thread* thread, const Handle<Str>& sep,
                    const Handle<ObjectArray>& items, word allocated);
  RawObject strStripSpace(const Handle<Str>& src,
                          const StrStripDirection direction);
  RawObject strStrip(const Handle<Str>& src, const Handle<Str>& str,
                     StrStripDirection direction);

  RawObject newValueCell();

  RawObject newWeakRef();

  void createBuiltinsModule();
  void createSysModule();
  void createTimeModule();
  void createWeakRefModule();

  RawObject internStr(const Handle<Object>& str);
  RawObject internStrFromCStr(const char* c_str);

  void collectGarbage();

  RawObject run(const char* buffer);

  RawObject runFromCStr(const char* c_str);

  RawObject hash(RawObject object);
  word siphash24(View<byte> array);

  uword random();

  void setArgv(int argc, const char** argv);

  Heap* heap() { return &heap_; }

  void visitRoots(PointerVisitor* visitor);

  void addModule(const Handle<Module>& module);
  RawObject moduleAddGlobal(const Handle<Module>& module, SymbolId name,
                            const Handle<Object>& value);

  RawObject moduleAddBuiltinFunction(const Handle<Module>& module,
                                     SymbolId name, const Function::Entry entry,
                                     const Function::Entry entry_kw,
                                     const Function::Entry entry_ex);

  RawObject findModule(const Handle<Object>& name);

  RawObject moduleAt(const Handle<Module>& module, const Handle<Object>& key);
  void moduleAtPut(const Handle<Module>& module, const Handle<Object>& key,
                   const Handle<Object>& value);

  RawObject importModule(const Handle<Object>& name);

  // importModuleFromBuffer is exposed for use by the tests. We may be able to
  // remove this later.
  RawObject importModuleFromBuffer(const char* buffer,
                                   const Handle<Object>& name);

  RawObject typeOf(RawObject object);

  RawObject typeAt(LayoutId layout_id);
  RawObject layoutAt(LayoutId layout_id);
  void layoutAtPut(LayoutId layout_id, RawObject object);

  // Bootstrapping primitive for creating a built-in class that has built-in
  // attributes and/or methods.
  RawObject addEmptyBuiltinClass(SymbolId name, LayoutId subclass_id,
                                 LayoutId superclass_id);
  RawObject addBuiltinClass(SymbolId name, LayoutId subclass_id,
                            LayoutId superclass_id,
                            View<BuiltinAttribute> attributes);
  RawObject addBuiltinClass(SymbolId name, LayoutId subclass_id,
                            LayoutId superclass_id,
                            View<BuiltinMethod> methods);
  RawObject addBuiltinClass(SymbolId name, LayoutId subclass_id,
                            LayoutId superclass_id,
                            View<BuiltinAttribute> attributes,
                            View<BuiltinMethod> methods);

  LayoutId reserveLayoutId();

  SymbolId binaryOperationSelector(Interpreter::BinaryOp op);
  SymbolId swappedBinaryOperationSelector(Interpreter::BinaryOp op);

  SymbolId inplaceOperationSelector(Interpreter::BinaryOp op);

  SymbolId comparisonSelector(CompareOp op);
  SymbolId swappedComparisonSelector(CompareOp op);

  bool isIteratorExhausted(Thread* thread, const Handle<Object>& iterator);

  // Return true if the selector does not appear in the MRO object below object.
  bool isMethodOverloaded(Thread* thread, const Handle<Type>& type,
                          SymbolId selector);

  RawObject buildClass() { return build_class_; }

  RawObject displayHook() { return display_hook_; }

  RawObject interned() { return interned_; }

  RawObject modules() { return modules_; }

  RawObject notImplemented() { return not_implemented_; }

  RawObject apiHandles() { return api_handles_; }

  Symbols* symbols() { return symbols_; }

  // Ensures that array has enough space for an atPut at index. If so, returns
  // array. If not, allocates and returns a new array with sufficient capacity
  // and identical contents.
  void listEnsureCapacity(const Handle<List>& list, word index);

  // Appends an element to the end of the list.
  void listAdd(const Handle<List>& list, const Handle<Object>& value);

  // Extends a list from an iterator.
  // Returns either the extended list or an Error object.
  RawObject listExtend(Thread* thread, const Handle<List>& dst,
                       const Handle<Object>& iterable);

  // Inserts an element to the specified index of the list.
  // When index >= len(list) it is equivalent to appending to the list.
  void listInsert(const Handle<List>& list, const Handle<Object>& value,
                  word index);

  // Removes and returns an element from the specified list index.
  // Expects index to be within [0, len(list)]
  RawObject listPop(const Handle<List>& list, word index);

  // Return a new list that is composed of list repeated ntimes
  RawObject listReplicate(Thread* thread, const Handle<List>& list,
                          word ntimes);

  // Associate a value with the supplied key.
  //
  // This handles growing the backing ObjectArray if needed.
  void dictAtPut(const Handle<Dict>& dict, const Handle<Object>& key,
                 const Handle<Object>& value);

  // Look up the value associated with key. Returns Error::object() if the
  // key was not found.
  RawObject dictAt(const Handle<Dict>& dict, const Handle<Object>& key);

  // Looks up and returns the value associated with the key.  If the key is
  // absent, calls thunk and inserts its result as the value.
  RawObject dictAtIfAbsentPut(const Handle<Dict>& dict,
                              const Handle<Object>& key,
                              Callback<RawObject>* thunk);

  // Stores value in a ValueCell stored at key in dict. Careful to
  // reuse an existing value cell if one exists since it may be shared.
  RawObject dictAtPutInValueCell(const Handle<Dict>& dict,
                                 const Handle<Object>& key,
                                 const Handle<Object>& value);

  // Returns true if the dict contains the specified key.
  bool dictIncludes(const Handle<Dict>& dict, const Handle<Object>& key);

  // Delete a key from the dict.
  //
  // Returns true if the key existed and sets the previous value in value.
  // Returns false otherwise.
  RawObject dictRemove(const Handle<Dict>& dict, const Handle<Object>& key);

  // Support explicit hash value of key to do dictAtPut.
  void dictAtPutWithHash(const Handle<Dict>& dict, const Handle<Object>& key,
                         const Handle<Object>& value,
                         const Handle<Object>& key_hash);

  // Support explicit hash value of key to do dictAt.
  RawObject dictAtWithHash(const Handle<Dict>& dict, const Handle<Object>& key,
                           const Handle<Object>& key_hash);

  RawObjectArray dictKeys(const Handle<Dict>& dict);

  // Set related function, based on dict.
  // Add a value to set and return the object in set.
  RawObject setAdd(const Handle<Set>& set, const Handle<Object>& value);

  RawObject setAddWithHash(const Handle<Set>& set, const Handle<Object>& value,
                           const Handle<Object>& key_hash);

  // Return a shallow copy of a set
  RawObject setCopy(const Handle<Set>& set);

  // Returns true if the set contains the specified value.
  bool setIncludes(const Handle<Set>& set, const Handle<Object>& value);

  // Compute the set intersection between a set and an iterator
  // Returns either a new set with the intersection or an Error object.
  RawObject setIntersection(Thread* thread, const Handle<Set>& set,
                            const Handle<Object>& iterable);

  // Delete a key from the set, returns true if the key existed.
  bool setRemove(const Handle<Set>& set, const Handle<Object>& value);

  // Update a set from an iterator
  // Returns either the updated set or an Error object.
  RawObject setUpdate(Thread* thread, const Handle<Set>& set,
                      const Handle<Object>& iterable);

  // Update a dictionary from another dictionary or an iterator.
  // Returns either the updated dict or an Error object.
  RawObject dictUpdate(Thread* thread, const Handle<Dict>& dict,
                       const Handle<Object>& mapping);

  // Merges a dictionary with another dictionary or a mapping.
  // Returns either the merged dictionary or an Error object.
  // throws a TypeError if the keys are not strings or
  // if any of the mappings have the same key repeated in them.
  RawObject dictMerge(Thread* thread, const Handle<Dict>& dict,
                      const Handle<Object>& mapping);

  // Resume a GeneratorBase, passing it the given value and returning either the
  // yielded value or Error on termination.
  RawObject genSend(Thread* thread, const Handle<GeneratorBase>& gen,
                    const Handle<Object>& value);

  // Save the current Frame to the given generator and pop the Frame off of the
  // stack.
  void genSave(Thread* thread, const Handle<GeneratorBase>& gen);

  // Get the RawGeneratorBase corresponding to the given Frame, assuming it is
  // executing in a resumed GeneratorBase.
  RawGeneratorBase genFromStackFrame(Frame* frame);

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
  RawObject computeInitialLayout(Thread* thread, const Handle<Type>& klass,
                                 LayoutId base_layout_id);

  // Returns type's __init__ method, or None
  RawObject classConstructor(const Handle<Type>& type);

  // Looks up name in the dict of each entry in type's MRO.
  //
  // This is equivalent to CPython's PyType_Lookup. Returns the Error object if
  // the name wasn't found.
  RawObject lookupNameInMro(Thread* thread, const Handle<Type>& type,
                            const Handle<Object>& name);

  // Looks up symbol name in the dict of each entry in type's MRO.
  RawObject lookupSymbolInMro(Thread* thread, const Handle<Type>& type,
                              SymbolId symbol);

  // Implements `receiver.name`
  RawObject attributeAt(Thread* thread, const Handle<Object>& receiver,
                        const Handle<Object>& name);

  // Implements `receiver.name = value`
  RawObject attributeAtPut(Thread* thread, const Handle<Object>& receiver,
                           const Handle<Object>& name,
                           const Handle<Object>& value);

  // Implements `del receiver.name`
  RawObject attributeDel(Thread* thread, const Handle<Object>& receiver,
                         const Handle<Object>& name);

  // Attribute lookup primitive for instances.
  //
  // This operates directly on the instance and does not respect Python
  // semantics for attribute lookup. Returns Error::object() if the attribute
  // isn't found.
  RawObject instanceAt(Thread* thread, const Handle<HeapObject>& instance,
                       const Handle<Object>& name);

  // Attribute setting primitive for instances.
  //
  // This operates directly on the instance and does not respect Python
  // semantics for attribute storage. This handles mutating the instance's
  // layout if the attribute does not already exist on the instance.
  RawObject instanceAtPut(Thread* thread, const Handle<HeapObject>& instance,
                          const Handle<Object>& name,
                          const Handle<Object>& value);

  // Attribute deletion primitive for instances.
  //
  // This operates directly on the instance and does not respect Python
  // semantics for attribute deletion. This handles mutating the layout if the
  // attribute exists. Returns Error::object() if the attribute is not found.
  RawObject instanceDel(Thread* thread, const Handle<HeapObject>& instance,
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
  RawObject layoutAddAttribute(Thread* thread, const Handle<Layout>& layout,
                               const Handle<Object>& name, word flags);

  // Delete the named attribute from the layout.
  //
  // If the attribute exists, this returns a new layout by either following
  // a pre-existing edge or adding one.
  //
  // If the attribute doesn't exist, Error::object() is returned.
  RawObject layoutDeleteAttribute(Thread* thread, const Handle<Layout>& layout,
                                  const Handle<Object>& name);

  // Pre-computes fast_globals for functions.
  RawObject computeFastGlobals(const Handle<Code>& code,
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
  RawObject isSubClass(const Handle<Type>& subclass,
                       const Handle<Type>& superclass);

  bool hasSubClassFlag(RawObject instance, Type::Flag flag) {
    return Type::cast(typeAt(instance->layoutId()))->hasFlag(flag);
  }

  // Returns whether or not instance is an instance of Type or a subclass of
  // Type
  //
  // This is equivalent to PyType_Check.
  bool isInstanceOfClass(RawObject instance) {
    if (instance->isType()) {
      return true;
    }
    // The bit_cast here is needed to avoid self-recursion when this is called
    // by Type::cast(). It is safe, as typeOf() is guaranteed to return a
    // RawType.
    return bit_cast<RawType>(typeOf(instance))
        ->hasFlag(Type::Flag::kTypeSubclass);
  }

  bool isInstanceOfList(RawObject instance) {
    if (instance->isList()) {
      return true;
    }
    return Type::cast(typeOf(instance))->hasFlag(Type::Flag::kListSubclass);
  }

  // Return true if obj is an instance of a subclass of klass
  RawObject isInstance(const Handle<Object>& obj, const Handle<Type>& klass);

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

  // Clear the allocated memory from all extension related objects
  void freeApiHandles();

  // Creates an layout with a new ID and no attributes.
  RawObject layoutCreateEmpty(Thread* thread);

  // Import all the public module's symbols to the given dict
  void moduleImportAllFrom(const Handle<Dict>& dict,
                           const Handle<Module>& module);

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

  void initializeClassMethodClass();
  void initializeFunctionClass();
  void initializeObjectClass();
  void initializePropertyClass();
  void initializeRefClass();

  void initializeStaticMethodClass();
  void initializeSuperClass();
  void initializeTypeClass();

  static bool isAsciiSpace(byte ch);

  RawObject createMainModule();

  RawObject executeModule(const char* buffer, const Handle<Module>& module);

  void visitRuntimeRoots(PointerVisitor* visitor);
  void visitThreadRoots(PointerVisitor* visitor);

  RawObject identityHash(RawObject object);
  RawObject immediateHash(RawObject object);
  RawObject valueHash(RawObject object);

  RawObject createMro(const Handle<Layout>& subclass_layout,
                      LayoutId superclass_id);

  RawObjectArray dictGrow(const Handle<ObjectArray>& data);

  // Looks up the supplied key
  //
  // If the key is found, this function returns true and sets index to the
  // index of the bucket that contains the value. If the key is not found, this
  // function returns false and sets index to the location where the key would
  // be inserted. If the dict is full, it sets index to -1.
  bool dictLookup(const Handle<ObjectArray>& data, const Handle<Object>& key,
                  const Handle<Object>& key_hash, word* index);

  template <SetLookupType type>
  bool setLookup(const Handle<ObjectArray>& data, const Handle<Object>& key,
                 const Handle<Object>& key_hash, word* index);

  RawObjectArray setGrow(const Handle<ObjectArray>& data);

  // Generic attribute lookup code used for class objects
  RawObject classGetAttr(Thread* thread, const Handle<Object>& receiver,
                         const Handle<Object>& name);

  // Generic attribute setting code used for class objects
  RawObject classSetAttr(Thread* thread, const Handle<Object>& receiver,
                         const Handle<Object>& name,
                         const Handle<Object>& value);

  // Generic attribute deletion code used for class objects
  RawObject classDelAttr(Thread* thread, const Handle<Object>& receiver,
                         const Handle<Object>& name);

  // Generic attribute lookup code used for instance objects
  RawObject instanceGetAttr(Thread* thread, const Handle<Object>& receiver,
                            const Handle<Object>& name);

  // Generic attribute setting code used for instance objects
  RawObject instanceSetAttr(Thread* thread, const Handle<Object>& receiver,
                            const Handle<Object>& name,
                            const Handle<Object>& value);

  // Generic attribute deletion code used for instance objects
  RawObject instanceDelAttr(Thread* thread, const Handle<Object>& receiver,
                            const Handle<Object>& name);

  // Generic attribute lookup code used for module objects
  RawObject moduleGetAttr(Thread* thread, const Handle<Object>& receiver,
                          const Handle<Object>& name);

  // Generic attribute setting code used for module objects
  RawObject moduleSetAttr(Thread* thread, const Handle<Object>& receiver,
                          const Handle<Object>& name,
                          const Handle<Object>& value);

  // Generic attribute deletion code used for module objects
  RawObject moduleDelAttr(Thread* thread, const Handle<Object>& receiver,
                          const Handle<Object>& name);

  // Specialized attribute lookup code used for super objects
  RawObject superGetAttr(Thread* thread, const Handle<Object>& receiver,
                         const Handle<Object>& name);

  // helper function add builtin types
  void moduleAddBuiltinType(const Handle<Module>& module, SymbolId name,
                            LayoutId layout_id);

  // Creates a layout that is a subclass of a built-in class and zero or more
  // additional built-in attributes.
  RawObject layoutCreateSubclassWithBuiltins(LayoutId subclass_id,
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
  RawObject layoutAddAttributeEntry(Thread* thread,
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
  RawObject layoutFollowEdge(const Handle<List>& edges,
                             const Handle<Object>& label);

  // Creates a new layout that will be a child layout of the supplied parent.
  //
  // The new layout shares the in-object and overflow attributes with the
  // parent and contains no outgoing edges.
  RawObject layoutCreateChild(Thread* thread, const Handle<Layout>& parent);

  // Generic version of dictUpdate, used by both dictUpdate and dictMerge
  // if merge is true, checks that keys are strings and are not repeated.
  // in the merge case, if either of the checks fail, returns by throwing a
  // python TypeError exception.
  template <DictUpdateType type>
  RawObject dictUpdate(Thread* thread, const Handle<Dict>& dict,
                       const Handle<Object>& mapping);

  RawObject strSubstr(const Handle<Str>& str, word start, word length);

  // Returns the length of the maximum initial span of src composed
  // of code points found in str. Analogous to the C string API function
  // strspn().
  word strSpan(const Handle<Str>& src, const Handle<Str>& str);

  // Returns the length of the maximum final span of src composed
  // of code points found in str. Right handed version of
  // Runtime::strSpan(). The rend argument is the index at which to stop
  // scanning left, and could be set to 0 to scan the whole string.
  word strRSpan(const Handle<Str>& src, const Handle<Str>& str, word rend);

  // The size listEnsureCapacity grows to if array is empty
  static const int kInitialEnsuredCapacity = 4;

  Heap heap_;

  // An ObjectArray of Layout objects, indexed by layout id.
  RawObject layouts_;

  // Cached instances
  RawObject empty_byte_array_;
  RawObject empty_object_array_;
  RawObject ellipsis_;
  RawObject not_implemented_;
  RawObject build_class_;
  RawObject display_hook_;

  // Interned strings
  RawObject interned_;

  // Modules
  RawObject modules_;

  // ApiHandles
  RawObject api_handles_;

  // Weak reference callback list
  RawObject callbacks_;

  Thread* threads_;

  uword random_state_[2];
  uword hash_secret_[2];

  NewValueCellCallback new_value_cell_callback_;

  Symbols* symbols_;

  DISALLOW_COPY_AND_ASSIGN(Runtime);
};

}  // namespace python
