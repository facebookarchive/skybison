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
class RawObject;
class RawTuple;
class PointerVisitor;
class Thread;

struct BuiltinAttribute {
  SymbolId name;
  int offset;
};

using NativeMethodType = RawObject (*)(Thread* thread, Frame* frame,
                                       word nargs);

struct NativeMethod {
  SymbolId name;
  NativeMethodType address;
};

struct BuiltinMethod {
  SymbolId name;
  NativeMethodType address;
};

enum class SetLookupType { Lookup, Insertion };

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

  RawObject newBoundMethod(const Object& function, const Object& self);

  RawObject newByteArray();

  RawObject newBytes(word length, byte fill);
  RawObject newBytesWithAll(View<byte> array);

  RawObject newType();
  RawObject newTypeWithMetaclass(LayoutId metaclass_id);

  RawObject newClassMethod();

  RawObject newEmptyCode();
  RawObject newCode(word argcount, word kwonlyargcount, word nlocals,
                    word stacksize, word flags, const Object& code,
                    const Object& consts, const Object& names,
                    const Tuple& varnames, const Tuple& freevars,
                    const Tuple& cellvars, const Object& filename,
                    const Object& name, word firstlineno, const Object& lnotab);

  RawObject newComplex(double real, double imag);

  RawObject newCoroutine();

  RawObject newDict();
  RawObject newDictWithSize(word initial_size);

  RawObject newDictItemIterator(const Dict& dict);
  RawObject newDictItems(const Dict& dict);
  RawObject newDictKeyIterator(const Dict& dict);
  RawObject newDictKeys(const Dict& dict);
  RawObject newDictValueIterator(const Dict& dict);
  RawObject newDictValues(const Dict& dict);

  RawObject newFloat(double value);

  RawObject newSet();

  RawObject emptyFrozenSet();
  RawObject newFrozenSet();

  RawObject newNativeFunction(SymbolId name, const Str& qualname,
                              Function::Entry entry, Function::Entry entry_kw,
                              Function::Entry entry_ex);

  RawObject newBuiltinFunction(SymbolId name, const Str& qualname,
                               Function::Entry entry);

  RawObject newExceptionState();

  RawObject newFunction();

  RawObject newGenerator();

  RawObject newHeapFrame(const Code& code);

  RawObject newInstance(const Layout& layout);

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

  RawObject newListIterator(const Object& list);

  RawObject newSeqIterator(const Object& sequence);

  RawObject newModule(const Object& name);

  // Returns an Int that stores the numerical address of the pointer.
  RawObject newIntFromCPtr(void* ptr);

  RawObject newTuple(word length);

  RawObject newProperty(const Object& getter, const Object& setter,
                        const Object& deleter);

  RawObject newRange(word start, word stop, word step);

  RawObject newRangeIterator(const Object& iterable);

  RawObject newSetIterator(const Object& set);

  RawObject newSlice();

  RawObject newStaticMethod();

  RawObject newStrFromByteArray(const ByteArray& array);
  RawObject newStrFromCStr(const char* c_str);
  // Creates a new string constructed from a format and a list of arguments,
  // similar to sprintf.
  RawObject newStrFromFormat(const char* fmt, ...) FORMAT_ATTRIBUTE(2, 3);
  RawObject newStrFromUTF32(View<int32> code_units);
  RawObject newStrWithAll(View<byte> code_units);

  RawObject newStrIterator(const Object& iterable);

  RawObject newSuper();

  RawObject newTupleIterator(const Tuple& tuple, word length);

  void processCallbacks();

  RawObject strConcat(Thread* thread, const Str& left, const Str& right);
  RawObject strJoin(Thread* thread, const Str& sep, const Tuple& items,
                    word allocated);
  RawObject strSubstr(Thread* thread, const Str& str, word start, word length);

  RawObject newValueCell();

  RawObject newWeakRef();

  void createBuiltinsModule();
  void createImportlibModule();
  void createImportModule();
  void createMarshalModule();
  void createOperatorModule();
  void createSysModule();
  void createTimeModule();
  void createUnderIoModule();
  void createWarningsModule();
  void createWeakRefModule();

  RawObject internStr(const Object& str);
  RawObject internStrFromCStr(const char* c_str);

  void collectGarbage();

  RawObject run(const char* buffer);

  RawObject hash(RawObject object);
  word siphash24(View<byte> array);

  uword random();

  void setArgv(int argc, const char** argv);

  Heap* heap() { return &heap_; }

  void visitRoots(PointerVisitor* visitor);

  void addModule(const Module& module);
  RawObject moduleAddGlobal(const Module& module, SymbolId name,
                            const Object& value);

  RawObject moduleAddBuiltinFunction(const Module& module, SymbolId name,
                                     Function::Entry entry);

  RawObject moduleAddNativeFunction(const Module& module, SymbolId name,
                                    Function::Entry entry,
                                    Function::Entry entry_kw,
                                    Function::Entry entry_ex);

  RawObject findModule(const Object& name);
  RawObject findModuleById(SymbolId name);
  RawObject lookupNameInModule(Thread* thread, SymbolId module_name,
                               SymbolId name);

  RawObject moduleDictAt(const Dict& module_dict, const Object& key);
  RawObject moduleDictAtPut(const Dict& dict, const Object& key,
                            const Object& value);
  RawObject moduleAt(const Module& module, const Object& key);
  RawObject moduleAtById(const Module& module, SymbolId key);
  void moduleAtPut(const Module& module, const Object& key,
                   const Object& value);

  RawObject importModule(const Object& name);

  // importModuleFromBuffer is exposed for use by the tests. We may be able to
  // remove this later.
  RawObject importModuleFromBuffer(const char* buffer, const Object& name);

  RawObject typeOf(RawObject object);

  RawObject typeDictAt(const Dict& dict, const Object& key);
  RawObject typeDictAtPut(const Dict& dict, const Object& key,
                          const Object& value);
  RawObject typeAt(LayoutId layout_id);
  RawObject layoutAt(LayoutId layout_id);
  void layoutAtPut(LayoutId layout_id, RawObject object);

  // Bootstrapping primitive for creating a built-in class that has built-in
  // attributes and/or methods.
  RawObject addEmptyBuiltinType(SymbolId name, LayoutId subclass_id,
                                LayoutId superclass_id);
  RawObject addBuiltinTypeWithBuiltinMethods(SymbolId name,
                                             LayoutId subclass_id,
                                             LayoutId superclass_id,
                                             const BuiltinMethod methods[]);
  RawObject addBuiltinType(SymbolId name, LayoutId subclass_id,
                           LayoutId superclass_id,
                           const BuiltinAttribute attrs[],
                           const NativeMethod methods[],
                           const BuiltinMethod builtins[]);

  LayoutId reserveLayoutId();

  SymbolId binaryOperationSelector(Interpreter::BinaryOp op);
  SymbolId swappedBinaryOperationSelector(Interpreter::BinaryOp op);
  bool shouldReverseBinaryOperation(Thread* thread, const Object& left,
                                    const Object& left_reversed_method,
                                    const Object& right,
                                    const Object& right_reversed_method);

  SymbolId inplaceOperationSelector(Interpreter::BinaryOp op);

  SymbolId comparisonSelector(CompareOp op);
  SymbolId swappedComparisonSelector(CompareOp op);

  RawObject iteratorLengthHint(Thread* thread, const Object& iterator);

  // Return true if the selector does not appear in the MRO object below object.
  bool isMethodOverloaded(Thread* thread, const Type& type, SymbolId selector);

  RawObject buildClass() { return build_class_; }

  RawObject displayHook() { return display_hook_; }

  RawObject interned() { return interned_; }

  RawObject modules() { return modules_; }

  RawObject notImplemented() { return not_implemented_; }

  RawObject apiHandles() { return api_handles_; }

  RawObject apiCaches() { return api_caches_; }

  Symbols* symbols() { return symbols_; }

  RawObject unboundValue() { return unbound_value_; }

  // Provides a growth strategy for mutable sequences. Grows by a factor of 2,
  // scaling up to the smallest power of two satisfying the required miniumum
  // if the initial factor is insufficient. Always grows the sequence.
  word newCapacity(word curr_capacity, word min_capacity);

  // Ensures that the byte array has at least the desired capacity.
  // Allocates if the existing capacity is insufficient.
  void byteArrayEnsureCapacity(Thread* thread, const ByteArray& array,
                               word min_capacity);

  // Appends multiple bytes to the end of the array.
  void byteArrayExtend(Thread* thread, const ByteArray& array, View<byte> view);
  void byteArrayIadd(Thread* thread, const ByteArray& array, const Bytes& bytes,
                     word length);

  // Returns a new Bytes containing the elements of self and then other.
  RawObject bytesConcat(Thread* thread, const Bytes& self, const Bytes& other);

  // Returns a new Bytes containing the subsequence of self
  // with the given start index and length.
  RawObject bytesSubseq(Thread* thread, const Bytes& self, word start,
                        word length);

  // Ensures that the list has at least the desired capacity.
  // Allocates if the existing capacity is insufficient.
  void listEnsureCapacity(const List& list, word min_capacity);

  // Appends an element to the end of the list.
  void listAdd(const List& list, const Object& value);

  // Associate a value with the supplied key.
  //
  // This handles growing the backing Tuple if needed.
  void dictAtPut(const Dict& dict, const Object& key, const Object& value);

  // Look up the value associated with key. Returns Error::object() if the
  // key was not found.
  RawObject dictAt(const Dict& dict, const Object& key);

  // Looks up and returns the value associated with the key.  If the key is
  // absent, calls thunk and inserts its result as the value.
  RawObject dictAtIfAbsentPut(const Dict& dict, const Object& key,
                              Callback<RawObject>* thunk);

  // Stores value in a ValueCell stored at key in dict. Careful to
  // reuse an existing value cell if one exists since it may be shared.
  RawObject dictAtPutInValueCell(const Dict& dict, const Object& key,
                                 const Object& value);

  // Returns true if the dict contains the specified key.
  bool dictIncludes(const Dict& dict, const Object& key);

  // Returns true if the dict contains the specified key with the specified
  // hash.
  bool dictIncludesWithHash(const Dict& dict, const Object& key,
                            const Object& key_hash);

  // Delete a key from the dict.
  //
  // Returns true if the key existed and sets the previous value in value.
  // Returns false otherwise.
  RawObject dictRemove(const Dict& dict, const Object& key);

  // Delete a key from the dict.
  //
  // Returns true if the key existed and sets the previous value in value.
  // Returns false otherwise.
  RawObject dictRemoveWithHash(const Dict& dict, const Object& key,
                               const Object& key_hash);

  // Support explicit hash value of key to do dictAtPut.
  void dictAtPutWithHash(const Dict& dict, const Object& key,
                         const Object& value, const Object& key_hash);

  // Support explicit hash value of key to do dictAt.
  RawObject dictAtWithHash(const Dict& dict, const Object& key,
                           const Object& key_hash);

  RawObject dictItems(Thread* thread, const Dict& dict);
  RawObject dictKeys(const Dict& dict);
  RawObject dictValues(Thread* thread, const Dict& dict);

  // Set related function, based on dict.
  // Add a value to set and return the object in set.
  RawObject setAdd(const SetBase& set, const Object& value);

  RawObject setAddWithHash(const SetBase& set, const Object& value,
                           const Object& key_hash);

  // Returns true if the set contains the specified value.
  bool setIncludes(const SetBase& set, const Object& value);

  // Compute the set intersection between a set and an iterator
  // Returns either a new set with the intersection or an Error object.
  RawObject setIntersection(Thread* thread, const SetBase& set,
                            const Object& iterable);

  // Delete a key from the set, returns true if the key existed.
  bool setRemove(const Set& set, const Object& value);

  // Update a set from an iterator
  // Returns either the updated set or an Error object.
  RawObject setUpdate(Thread* thread, const SetBase& set,
                      const Object& iterable);

  // Resume a GeneratorBase, passing it the given value and returning either the
  // yielded value or Error on termination.
  RawObject genSend(Thread* thread, const GeneratorBase& gen,
                    const Object& value);

  // Save the current Frame to the given generator.
  void genSave(Thread* thread, const GeneratorBase& gen);

  // Get the RawGeneratorBase corresponding to the given Frame, assuming it is
  // executing in a resumed GeneratorBase.
  RawGeneratorBase genFromStackFrame(Frame* frame);

  NewValueCellCallback* newValueCellCallback() {
    return &new_value_cell_callback_;
  }

  // Compile the given source code string into a marshaled code object. If
  // PYRO_COMPILE_CACHE is set in the environment or ~/.pyro-compile-cache is a
  // writable directory, the result will be cached on disk to speed up future
  // calls with the same source.
  static std::unique_ptr<char[]> compile(View<char> src);

  static std::unique_ptr<char[]> compileFromCStr(const char* src);

  // Like compile(), but bypass the cache and return the length of the resuling
  // buffer in len.
  static std::unique_ptr<char[]> compileWithLen(View<char> src, word* len);

  // Performs a simple scan of the bytecode and collects all attributes that
  // are set via `self.<attribute> =` into attributes.
  void collectAttributes(const Code& code, const Dict& attributes);

  // Constructs the initial layout for instances of type.
  //
  // The layout contains the set of in-object attributes. This is computed by
  // scanning the constructors of every type in type's MRO.
  RawObject computeInitialLayout(Thread* thread, const Type& type,
                                 LayoutId base_layout_id);

  // Returns type's __init__ method, or None
  RawObject classConstructor(const Type& type);

  // Looks up name in the dict of each entry in type's MRO.
  //
  // This is equivalent to CPython's PyType_Lookup. Returns the Error object if
  // the name wasn't found.
  RawObject lookupNameInMro(Thread* thread, const Type& type,
                            const Object& name);

  // Looks up symbol name in the dict of each entry in type's MRO.
  RawObject lookupSymbolInMro(Thread* thread, const Type& type,
                              SymbolId symbol);

  // Implements `receiver.name`
  RawObject attributeAt(Thread* thread, const Object& receiver,
                        const Object& name);

  // Implements `receiver.name = value`
  RawObject attributeAtPut(Thread* thread, const Object& receiver,
                           const Object& name, const Object& value);

  // Implements `del receiver.name`
  RawObject attributeDel(Thread* thread, const Object& receiver,
                         const Object& name);

  // Attribute lookup primitive for instances.
  //
  // This operates directly on the instance and does not respect Python
  // semantics for attribute lookup. Returns Error::object() if the attribute
  // isn't found.
  RawObject instanceAt(Thread* thread, const HeapObject& instance,
                       const Object& name);

  // Attribute setting primitive for instances.
  //
  // This operates directly on the instance and does not respect Python
  // semantics for attribute storage. This handles mutating the instance's
  // layout if the attribute does not already exist on the instance.
  RawObject instanceAtPut(Thread* thread, const HeapObject& instance,
                          const Object& name, const Object& value);

  // Attribute deletion primitive for instances.
  //
  // This operates directly on the instance and does not respect Python
  // semantics for attribute deletion. This handles mutating the layout if the
  // attribute exists. Returns Error::object() if the attribute is not found.
  RawObject instanceDel(Thread* thread, const HeapObject& instance,
                        const Object& name);

  // Looks up the named attribute in the layout.
  //
  // If the attribute is found this returns true and sets info.
  // Returns false otherwise.
  bool layoutFindAttribute(Thread* thread, const Layout& layout,
                           const Object& name, AttributeInfo* info);

  // Add the attribute to the overflow array.
  //
  // This returns a new layout by either following a pre-existing edge or
  // adding one.
  RawObject layoutAddAttribute(Thread* thread, const Layout& layout,
                               const Object& name, word flags);

  // Delete the named attribute from the layout.
  //
  // If the attribute exists, this returns a new layout by either following
  // a pre-existing edge or adding one.
  //
  // If the attribute doesn't exist, Error::object() is returned.
  RawObject layoutDeleteAttribute(Thread* thread, const Layout& layout,
                                  const Object& name);

  // Pre-computes fast_globals for functions.
  RawObject computeFastGlobals(const Code& code, const Dict& globals,
                               const Dict& builtins);

  RawObject computeBuiltinBase(Thread* thread, const Type& type);

  // Adds a builtin function with a positional entry point definition
  // using the default keyword and splatting entry points.
  void typeAddNativeFunction(const Type& type, SymbolId name,
                             Function::Entry entry);

  // Adds a builtin function with positional and keyword entry point
  // definitions, using the default splatting entry point.
  void typeAddNativeFunctionKw(const Type& type, SymbolId name,
                               Function::Entry entry, Function::Entry entry_kw);

  // Adds a builtin function with positional, keyword & splatting entry point
  // definitions
  void typeAddNativeFunctionKwEx(const Type& type, SymbolId name,
                                 Function::Entry entry,
                                 Function::Entry entry_kw,
                                 Function::Entry entry_ex);

  void typeAddBuiltinFunction(const Type& type, SymbolId name,
                              Function::Entry entry);

  // Helper function to add extension functions to extension classes
  void classAddExtensionFunction(const Type& type, SymbolId name,
                                 void* c_function);

  // Converts the offset in code's bytecode into the corresponding line number
  // in the backing source file.
  word codeOffsetToLineNum(Thread* thread, const Code& code, word offset);

  // Return true if subclass is a subclass of superclass
  bool isSubclass(const Type& subclass, const Type& superclass);

  // Return true if obj is an instance of a subclass of type
  bool isInstance(const Object& obj, const Type& type);

  // For commonly-subclassed builtin types, define isInstanceOfFoo(RawObject)
  // that does a check including subclasses (unlike RawObject::isFoo(), which
  // only gets exact types).
#define DEFINE_IS_INSTANCE(ty)                                                 \
  bool isInstanceOf##ty(RawObject obj) {                                       \
    if (obj.is##ty()) return true;                                             \
    return typeOf(obj).rawCast<RawType>().builtinBase() == LayoutId::k##ty;    \
  }
  DEFINE_IS_INSTANCE(Bytes)
  DEFINE_IS_INSTANCE(ByteArray)
  DEFINE_IS_INSTANCE(Complex)
  DEFINE_IS_INSTANCE(Dict)
  DEFINE_IS_INSTANCE(Float)
  DEFINE_IS_INSTANCE(FrozenSet)
  DEFINE_IS_INSTANCE(ImportError)
  DEFINE_IS_INSTANCE(Int)
  DEFINE_IS_INSTANCE(List)
  DEFINE_IS_INSTANCE(Module)
  DEFINE_IS_INSTANCE(Set)
  DEFINE_IS_INSTANCE(StopIteration)
  DEFINE_IS_INSTANCE(Str)
  DEFINE_IS_INSTANCE(SystemExit)
  DEFINE_IS_INSTANCE(Tuple)
  DEFINE_IS_INSTANCE(Type)
#undef DEFINE_IS_INSTANCE

  // User-defined subclasses of immediate types have no corresponding LayoutId,
  // so we detect them by looking for an object that is a subclass of a
  // particular immediate type but not exactly that type.
#define DEFINE_IS_USER_INSTANCE(ty)                                            \
  bool isInstanceOfUser##ty##Base(RawObject obj) {                             \
    return !obj.is##ty() &&                                                    \
           RawType::cast(typeOf(obj)).builtinBase() == LayoutId::k##ty;        \
  }
  DEFINE_IS_USER_INSTANCE(Float)
  DEFINE_IS_USER_INSTANCE(Tuple)
#undef DEFINE_IS_USER_INSTANCE

  // BaseException must be handled specially because it has builtin subclasses
  // that are visible to managed code.
  bool isInstanceOfBaseException(RawObject obj) {
    return RawType::cast(typeOf(obj)).isBaseExceptionSubclass();
  }

  // SetBase must also be handled specially because many builtin functions
  // accept set or frozenset, despite them not having a common ancestor.
  bool isInstanceOfSetBase(RawObject instance) {
    if (instance.isSetBase()) {
      return true;
    }
    LayoutId builtin_base = RawType::cast(typeOf(instance)).builtinBase();
    return builtin_base == LayoutId::kSet ||
           builtin_base == LayoutId::kFrozenSet;
  }

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
  void seedRandom(const uword random_state[2], const uword hash_secret[2]);

  // Returns whether object's class provides a __set__ method
  bool isDataDescriptor(Thread* thread, const Object& object);

  // Returns whether object's class provides a __get__ method
  bool isNonDataDescriptor(Thread* thread, const Object& object);

  // Returns whether object's class provides a __delete__ method
  bool isDeleteDescriptor(Thread* thread, const Object& object);

  // Return whether object's class supports the sequence protocol
  bool isSequence(Thread* thread, const Object& obj);

  // Return whether object's class provides a __getitem__ method
  bool isMapping(Thread* thread, const Object& obj);

  // Clear the allocated memory from all extension related objects
  void freeApiHandles();

  // Creates an layout with a new ID and no attributes.
  RawObject layoutCreateEmpty(Thread* thread);

  // Import all the public module's symbols to the given dict
  void moduleImportAllFrom(const Dict& dict, const Module& module);

  // Converts bytes object into an int object. The conversion is performed
  // with the specified endianness. `is_signed` specifies whether the highest
  // bit is considered a sign.
  RawObject bytesToInt(Thread* thread, const Bytes& bytes, endian endianness,
                       bool is_signed);

  // Returns the sum of `left` and `right`.
  RawObject intAdd(Thread* thread, const Int& left, const Int& right);

  // Returns the result of bitwise AND of the arguments
  RawObject intBinaryAnd(Thread* thread, const Int& left, const Int& right);

  // Returns the result of bitwise left shift of the arguments
  RawObject intBinaryLshift(Thread* thread, const Int& num, const Int& amount);

  // Returns the result of bitwise logical OR of the arguments
  RawObject intBinaryOr(Thread* thread, const Int& left, const Int& right);

  // Returns the result of bitwise right shift of `num` by `shift`.
  RawObject intBinaryRshift(Thread* thread, const Int& num, const Int& amount);

  // Returns the result of bitwise XOR of the arguments
  RawObject intBinaryXor(Thread* thread, const Int& left, const Int& right);

  // Returns a copy of `value` with all bits flipped.
  RawObject intInvert(Thread* thread, const Int& value);

  // Returns the product of `left` and `right`.
  RawObject intMultiply(Thread* thread, const Int& left, const Int& right);

  // Returns `0 - value`.
  RawObject intNegate(Thread* thread, const Int& value);

  // Returns the result of subtracting `right` from `left`.
  RawObject intSubtract(Thread* thread, const Int& left, const Int& right);

  // Converts `num` into a bytes object with the given length. This function
  // expects the length to be big enough to hold the number and does not check
  // for overflow.
  RawObject intToBytes(Thread* thread, const Int& num, word length,
                       endian endianness);

  // Given a possibly invalid LargeInt remove redundant sign- and
  // zero-extension and convert to a SmallInt when possible.
  RawObject normalizeLargeInt(const LargeInt& large_int);

  // Generate a unique number for successively initialized native modules. We
  // don't index modules the same way as CPython, but we keep this to get a
  // unique module index and thereby maintain some CPython invariants for
  // testing.
  word nextModuleIndex();

 private:
  void initializeThreads();
  void initializeTypes();
  void initializeExceptionTypes();
  void initializeLayouts();
  void initializeHeapTypes();
  void initializeImmediateTypes();
  void initializePrimitiveInstances();
  void initializeInterned();
  void initializeModules();
  void initializeApiData();
  void initializeRandom();
  void initializeSymbols();

  RawObject createMainModule();

  RawObject executeModule(const char* buffer, const Module& module);

  void visitRuntimeRoots(PointerVisitor* visitor);
  void visitThreadRoots(PointerVisitor* visitor);

  RawObject identityHash(RawObject object);
  RawObject immediateHash(RawObject object);
  RawObject valueHash(RawObject object);

  RawObject createMro(const Layout& subclass_layout, LayoutId superclass_id);

  RawTuple dictGrow(const Tuple& data);

  // Looks up the supplied key
  //
  // If the key is found, this function returns true and sets index to the
  // index of the bucket that contains the value. If the key is not found, this
  // function returns false and sets index to the location where the key would
  // be inserted. If the dict is full, it sets index to -1.
  bool dictLookup(const Tuple& data, const Object& key, const Object& key_hash,
                  word* index, DictEq pred);

  template <SetLookupType type>
  word setLookup(const Tuple& data, const Object& key, const Object& key_hash);

  RawTuple setGrow(const Tuple& data);

  // Generic attribute lookup code used for class objects
  RawObject classGetAttr(Thread* thread, const Object& receiver,
                         const Object& name);

  // Generic attribute setting code used for class objects
  RawObject classSetAttr(Thread* thread, const Object& receiver,
                         const Object& name, const Object& value);

  // Generic attribute deletion code used for class objects
  RawObject classDelAttr(Thread* thread, const Object& receiver,
                         const Object& name);

  // Generic attribute lookup code used for instance objects
  RawObject instanceGetAttr(Thread* thread, const Object& receiver,
                            const Object& name);

  // Generic attribute setting code used for instance objects
  RawObject instanceSetAttr(Thread* thread, const Object& receiver,
                            const Object& name, const Object& value);

  // Generic attribute deletion code used for instance objects
  RawObject instanceDelAttr(Thread* thread, const Object& receiver,
                            const Object& name);

  // Generic attribute lookup code used for module objects
  RawObject moduleGetAttr(Thread* thread, const Object& receiver,
                          const Object& name);

  // Generic attribute setting code used for module objects
  RawObject moduleSetAttr(Thread* thread, const Object& receiver,
                          const Object& name, const Object& value);

  // Generic attribute deletion code used for module objects
  RawObject moduleDelAttr(Thread* thread, const Object& receiver,
                          const Object& name);

  // Specialized attribute lookup code used for super objects
  RawObject superGetAttr(Thread* thread, const Object& receiver,
                         const Object& name);

  // Generic attribute lookup code used for function objects
  RawObject functionGetAttr(Thread* thread, const Object& receiver,
                            const Object& name);

  // Generic attribute setting code used for function objects
  RawObject functionSetAttr(Thread* thread, const Object& receiver,
                            const Object& name, const Object& value);

  // helper function add builtin types
  void moduleAddBuiltinType(const Module& module, SymbolId name,
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
                               const Tuple& dst, word index);

  // Check if the layout's overflow attributes point to a dict offset
  //
  // This is a useful common case for types like function, type, etc, that all
  // overflow their custom attributes to a __dict__ on the instance. The
  // layout's OverflowAttrbutesOffset is expected to be a SmallInt.
  bool layoutHasDictOverflow(const Layout& layout);

  // Get the overflow dict from the overflow attribute pointer
  RawObject layoutGetOverflowDict(Thread* thread, const HeapObject& instance,
                                  const Layout& layout);
  // Appends the edge to the list of edges.
  //
  // edges is expected to be a list of edges (label, layout pairs) corresponding
  // to a class of shape altering mutations (e.g. attribute addition).
  void layoutAddEdge(const List& edges, const Object& label,
                     const Object& layout);

  // Create a new tuple for the name, info pair and return a new tuple
  // containing entries + entry.
  RawObject layoutAddAttributeEntry(Thread* thread, const Tuple& entries,
                                    const Object& name, AttributeInfo info);

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
  RawObject layoutFollowEdge(const List& edges, const Object& label);

  // Creates a new layout that will be a child layout of the supplied parent.
  //
  // The new layout shares the in-object and overflow attributes with the
  // parent and contains no outgoing edges.
  RawObject layoutCreateChild(Thread* thread, const Layout& parent);

  // Transform the pendingExceptionValue into a reasonable exit code
  //
  // Handles SystemExit thrown both from native and from managed code.
  word handleSysExit(Thread* thread);

  // Joins the type's name and attribute's name to produce a qualname
  RawObject newQualname(const Type& type, SymbolId name);

  // The size newCapacity grows to if array is empty
  static const int kInitialEnsuredCapacity = 4;

  Heap heap_;

  // A List of Layout objects, indexed by layout id.
  RawObject layouts_;

  // Cached instances
  RawObject empty_bytes_;
  RawObject empty_frozen_set_;
  RawObject empty_tuple_;
  RawObject ellipsis_;
  RawObject not_implemented_;
  RawObject build_class_;
  RawObject display_hook_;
  RawObject unbound_value_;

  // Interned strings
  RawObject interned_;

  // Modules
  RawObject modules_;

  // ApiHandles
  RawObject api_handles_;

  // Some API functions promise to cache their return value and return the same
  // value for repeated invocations on a specific PyObject. Those value are
  // cached here.
  RawObject api_caches_;

  // Weak reference callback list
  RawObject callbacks_;

  Thread* threads_;

  uword random_state_[2];
  uword hash_secret_[2];

  NewValueCellCallback new_value_cell_callback_;

  Symbols* symbols_;

  word max_module_index_ = 0;

  friend class ApiHandle;

  DISALLOW_COPY_AND_ASSIGN(Runtime);
};

class BuiltinsBase {
 public:
  static void postInitialize(Runtime*, const Type&) {}

  static const BuiltinAttribute kAttributes[];
  static const BuiltinMethod kBuiltinMethods[];
  static const NativeMethod kNativeMethods[];

 protected:
  static const SymbolId kName;
  static const LayoutId kType;
  static const LayoutId kSuperType;
};

template <class T, SymbolId name, LayoutId type,
          LayoutId supertype = LayoutId::kObject>
class Builtins : public BuiltinsBase {
 public:
  static void initialize(Runtime* runtime) {
    HandleScope scope;
    Type new_type(&scope, runtime->addBuiltinType(
                              T::kName, T::kType, T::kSuperType, T::kAttributes,
                              T::kNativeMethods, T::kBuiltinMethods));
    new_type.sealAttributes();
    T::postInitialize(runtime, new_type);
  }

 protected:
  static const SymbolId kName = name;
  static const LayoutId kType = type;
  static const LayoutId kSuperType = supertype;
};

}  // namespace python
