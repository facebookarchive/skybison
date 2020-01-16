#pragma once

#include "bytecode.h"
#include "callback.h"
#include "handles.h"
#include "heap.h"
#include "interpreter-gen.h"
#include "interpreter.h"
#include "layout.h"
#include "symbols.h"
#include "view.h"

namespace py {

class AttributeInfo;
class Heap;
class RawObject;
class RawTuple;
class PointerVisitor;
class Thread;

struct BuiltinAttribute {
  SymbolId name;
  int offset;
  AttributeFlags flags;
};

struct ListEntry {
  ListEntry* prev;
  ListEntry* next;
};

using AtExitFn = void (*)();

using DictEq = RawObject (*)(Thread*, RawObject, RawObject);

using NativeMethodType = RawObject (*)(Thread* thread, Frame* frame,
                                       word nargs);

struct BuiltinMethod {
  SymbolId name;
  NativeMethodType address;
};

enum class ReadOnly : bool {
  ReadWrite,
  ReadOnly,
};

class Runtime {
 public:
  class NewValueCellCallback : public Callback<RawObject> {
   public:
    explicit NewValueCellCallback(Runtime* runtime) : runtime_(runtime) {}
    RawObject call() {
      RawObject result = runtime_->newValueCell();
      ValueCell::cast(result).makePlaceholder();
      return result;
    }
    Runtime* runtime_;
  };

  Runtime();
  Runtime(word heap_size);
  ~Runtime();

  RawObject newAsyncGenerator();

  RawObject newBoundMethod(const Object& function, const Object& self);

  RawObject newByteArray();
  RawObject newByteArrayIterator(Thread* thread, const ByteArray& bytearray);

  RawObject newBytes(word length, byte fill);
  RawObject newBytesWithAll(View<byte> array);
  RawObject newBytesIterator(Thread* thread, const Bytes& bytes);

  RawObject newType();

  RawObject newTypeWithMetaclass(LayoutId metaclass_id);

  RawObject newTypeProxy(const Type& type);

  RawObject newClassMethod();

  // TODO(T55871582): Remove code paths that can raise from the Runtime
  RawObject newCode(word argcount, word posonlyargcount, word kwonlyargcount,
                    word nlocals, word stacksize, word flags,
                    const Object& code, const Object& consts,
                    const Object& names, const Object& varnames,
                    const Object& freevars, const Object& cellvars,
                    const Object& filename, const Object& name,
                    word firstlineno, const Object& lnotab);

  RawObject newBuiltinCode(word argcount, word posonlyargcount,
                           word kwonlyargcount, word flags,
                           Function::Entry entry, const Object& parameter_names,
                           const Object& name_str);

  RawObject newComplex(double real, double imag);

  RawObject newCoroutine();

  RawObject newDict();
  RawObject newDictWithSize(word initial_size);

  RawObject newDictItemIterator(Thread* thread, const Dict& dict);
  RawObject newDictItems(Thread* thread, const Dict& dict);
  RawObject newDictKeyIterator(Thread* thread, const Dict& dict);
  RawObject newDictKeys(Thread* thread, const Dict& dict);
  RawObject newDictValueIterator(Thread* thread, const Dict& dict);
  RawObject newDictValues(Thread* thread, const Dict& dict);

  RawObject newFloat(double value);

  RawObject newSet();

  RawObject emptyFrozenSet();
  RawObject newFrozenSet();

  RawObject newFunctionWithCode(Thread* thread, const Object& qualname,
                                const Code& code, const Object& module_obj);

  RawObject newFunctionWithCustomEntry(Thread* thread, const Object& name,
                                       const Object& code,
                                       Function::Entry entry,
                                       Function::Entry entry_kw,
                                       Function::Entry entry_ex);

  RawObject newExceptionState();

  RawObject newGenerator();

  RawObject newHeapFrame(const Function& function);

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

  // Create a new MemoryView object. Initializes the view format to "B".
  RawObject newMemoryView(Thread* thread, const Object& buffer, word length,
                          ReadOnly read_only);
  // Create a new MemoryView object from a C pointer. Initializes the view
  // format to "B".
  RawObject newMemoryViewFromCPtr(Thread* thread, void* ptr, word length,
                                  ReadOnly read_only);

  RawObject newModule(const Object& name);
  RawObject newModuleById(SymbolId name);

  RawObject newModuleProxy(const Module& module);

  RawObject newMutableBytesUninitialized(word size);

  // Returns an Int that stores the numerical address of the pointer.
  RawObject newIntFromCPtr(void* ptr);

  // Returns the singleton empty mutablebytes. Guaranteed to not allocate.
  RawObject emptyMutableBytes();

  // Returns the singleton empty slice. Guaranteed to not allocate.
  RawObject emptySlice();

  // Returns the singleton empty tuple. Guaranteed to not allocate.
  RawObject emptyTuple();

  // Return a new, None-initialized, mutable tuple of the given length.
  RawObject newMutableTuple(word length);

  // Return a new, None-initialized tuple of the given length.
  RawObject newTuple(word length);

  RawObject newProperty(const Object& getter, const Object& setter,
                        const Object& deleter);

  RawObject newRange(const Object& start, const Object& stop,
                     const Object& step);

  RawObject newLongRangeIterator(const Int& start, const Int& stop,
                                 const Int& step);

  RawObject newRangeIterator(word start, word step, word length);

  RawObject newSetIterator(const Object& set);

  RawObject newSlice(const Object& start, const Object& stop,
                     const Object& step);

  RawObject newStaticMethod();

  RawObject newStrArray();

  RawObject newStrFromByteArray(const ByteArray& array);
  RawObject newStrFromCStr(const char* c_str);
  RawObject strFromStrArray(const StrArray& array);

  // Creates a new string constructed from a format and a list of arguments,
  // similar to sprintf.
  // %c formats an ASCII character
  // %w formats a word
  // %C formats a unicode code point
  // %S formats a Str object
  // %T gets the type name of an object and formats that
  // %Y formats a SymbolId
  RawObject newStrFromFmt(const char* fmt, ...);
  RawObject newStrFromFmtV(Thread* thread, const char* fmt, va_list args);
  RawObject newStrFromUTF32(View<int32_t> code_units);
  RawObject newStrWithAll(View<byte> code_units);

  RawObject newStrIterator(const Object& str);

  RawObject newSuper();

  RawObject newTupleIterator(const Tuple& tuple, word length);

  void processCallbacks();
  void processFinalizers();

  RawObject strConcat(Thread* thread, const Str& left, const Str& right);
  NODISCARD RawObject strJoin(Thread* thread, const Str& sep,
                              const Tuple& items, word allocated);

  // Creates a new Str containing `str` repeated `count` times.
  RawObject strRepeat(Thread* thread, const Str& str, word count);

  RawObject strSlice(Thread* thread, const Str& str, word start, word stop,
                     word step);

  RawObject strSubstr(Thread* thread, const Str& str, word start, word length);

  RawObject newValueCell();

  RawObject newWeakLink(Thread* thread, const Object& referent,
                        const Object& prev, const Object& next);

  RawObject newWeakRef(Thread* thread, const Object& referent,
                       const Object& callback);

  RawObject ellipsis() { return ellipsis_; }

  RawObject moduleDunderGetattribute() { return module_dunder_getattribute_; }
  RawObject objectDunderGetattribute() { return object_dunder_getattribute_; }
  RawObject objectDunderInit() { return object_dunder_init_; }
  RawObject objectDunderNew() { return object_dunder_new_; }
  RawObject objectDunderSetattr() { return object_dunder_setattr_; }
  RawObject strDunderEq() { return str_dunder_eq_; }
  RawObject strDunderHash() { return str_dunder_hash_; }
  RawValueCell sysStderr() { return ValueCell::cast(sys_stderr_); }
  RawValueCell sysStdout() { return ValueCell::cast(sys_stdout_); }
  RawObject typeDunderGetattribute() { return type_dunder_getattribute_; }

  void createBuiltinsModule(Thread* thread);
  void createEmptyBuiltinsModule(Thread* thread);
  void createSysModule(Thread* thread);
  void createUnderBuiltinsModule(Thread* thread);

  static RawObject internStr(Thread* thread, const Object& str);
  static RawObject internStrFromAll(Thread* thread, View<byte> bytes);
  static RawObject internStrFromCStr(Thread* thread, const char* c_str);
  static RawObject internLargeStr(Thread* thread, const Object& str);
  // This function should only be used for `CHECK()`/`DCHECK()`. It is as slow
  // as the whole `internStr()` operation and will always return true for small
  // strings, even when the user did not explicitly intern them.
  static bool isInternedStr(Thread* thread, const Object& str);

  void collectGarbage();

  // Compute hash value suitable for `RawObject::operator==` (aka `a is b`)
  // equality tests.
  word hash(RawObject object);
  // Compute hash value for objects with byte payload. This is a helper to
  // implement `xxxHash()` functions.
  word valueHash(RawObject object);

  word siphash24(View<byte> array);

  uword random();

  Heap* heap() { return &heap_; }

  Interpreter* interpreter() { return interpreter_.get(); }

  // Tracks extension native non-GC and GC objects.
  // Returns true if an untracked entry becomes tracked, false, otherwise.
  bool trackNativeObject(ListEntry* entry);
  bool trackNativeGcObject(ListEntry* entry);

  // Untracks extension native non-GC and GC objects.
  // Returns true if a tracked entry becomes untracked, false, otherwise.
  bool untrackNativeObject(ListEntry* entry);
  bool untrackNativeGcObject(ListEntry* entry);

  // Return the head of the tracked native objects list.
  ListEntry* trackedNativeObjects();
  ListEntry* trackedNativeGcObjects();
  RawObject* finalizableReferences();

  void visitRoots(PointerVisitor* visitor);

  void addModule(const Module& module);
  bool moduleListAtPut(Thread* thread, const Module& module, word index);

  void moduleAddBuiltinFunction(Thread* thread, const Module& module,
                                SymbolId name, Function::Entry entry);
  void moduleAddBuiltinType(Thread* thread, const Module& module, SymbolId name,
                            LayoutId layout_id);

  RawObject findModule(const Object& name);
  RawObject findModuleById(SymbolId name);
  RawObject moduleListAt(Thread* thread, word index);
  RawObject lookupNameInModule(Thread* thread, SymbolId module_name,
                               SymbolId name);

  // Write the traceback to the given file object. If success, return None.
  // Else, return Error.
  RawObject printTraceback(Thread* thread, word fd);

  // importModuleFromCode is exposed for use by the tests. We may be able to
  // remove this later.
  NODISCARD RawObject importModuleFromCode(const Code& code,
                                           const Object& name);

  // Stores the implicit bases for a new class.
  RawObject implicitBases() { return implicit_bases_; }
  void initializeImplicitBases();

  // Gets the internal notion of type, rather than the user-visible type.
  RawObject concreteTypeAt(LayoutId layout_id);
  inline RawObject concreteTypeOf(RawObject object) {
    return concreteTypeAt(object.layoutId());
  }

  // Sets the internal type for layouts with a different describedType.
  void setLargeBytesType(const Type& type);
  void setLargeIntType(const Type& type);
  void setLargeStrType(const Type& type);
  void setSmallBytesType(const Type& type);
  void setSmallIntType(const Type& type);
  void setSmallStrType(const Type& type);

  RawObject typeOf(RawObject object) {
    return Layout::cast(layoutAt(object.layoutId())).describedType();
  }

  RawObject typeAt(LayoutId layout_id);
  RawObject layoutAt(LayoutId layout_id) {
    DCHECK(layout_id != LayoutId::kError, "Error has no Layout");
    return Tuple::cast(layouts_).at(static_cast<word>(layout_id));
  }
  void layoutAtPut(LayoutId layout_id, RawObject object);
  // Get layout for `layout_id` and attempt not to crash for invalid ids. This
  // is for debug dumpers. Do not use for other purposes!
  RawObject layoutAtSafe(LayoutId layout_id);

  // Raw access to the MutableTuple of Layouts. Intended only for use by the GC.
  RawObject layouts() { return layouts_; }
  void setLayouts(RawObject layouts) { layouts_ = layouts; }

  // Bootstrapping primitive for creating a built-in class that has built-in
  // attributes and/or methods.
  RawObject addEmptyBuiltinType(SymbolId name, LayoutId subclass_id,
                                LayoutId superclass_id);
  RawObject addBuiltinType(SymbolId name, LayoutId subclass_id,
                           LayoutId superclass_id,
                           const BuiltinAttribute attrs[],
                           const BuiltinMethod builtins[]);

  LayoutId reserveLayoutId(Thread* thread);

  word reserveModuleId();

  SymbolId binaryOperationSelector(Interpreter::BinaryOp op);
  SymbolId swappedBinaryOperationSelector(Interpreter::BinaryOp op);

  SymbolId inplaceOperationSelector(Interpreter::BinaryOp op);

  SymbolId comparisonSelector(CompareOp op);
  SymbolId swappedComparisonSelector(CompareOp op);

  RawObject buildClass() { return build_class_; }

  RawObject displayHook() { return display_hook_; }

  RawObject dunderImport() { return dunder_import_; }

  RawObject modules() { return modules_; }

  RawObject apiHandles() { return api_handles_; }

  RawObject apiCaches() { return api_caches_; }

  void setAtExit(AtExitFn at_exit) { at_exit_ = at_exit; }
  void atExit() {
    if (at_exit_ != nullptr) at_exit_();
  }

  Symbols* symbols() { return symbols_; }

  // Provides a growth strategy for mutable sequences. Grows by a factor of 1.5,
  // scaling up to the requested capacity if the initial factor is insufficient.
  // Always grows the sequence.
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

  // Returns a copy of a bytes object
  RawObject bytesCopy(Thread* thread, const Bytes& src);

  // Makes a new copy of the `original` bytes with the specified `size`.
  // If the new length is less than the old length, truncate the bytes to fit.
  // If the new length is greater than the old length, pad with trailing zeros.
  RawObject bytesCopyWithSize(Thread* thread, const Bytes& original,
                              word new_length);

  // Checks whether the specified range of `self` ends with the given `suffix`.
  // Returns `Bool::trueObj()` if the suffix matches, else `Bool::falseObj()`.
  RawObject bytesEndsWith(const Bytes& self, word self_len, const Bytes& suffix,
                          word suffix_len, word start, word end);

  // Returns a new Bytes from the first `length` int-like elements in the tuple.
  RawObject bytesFromTuple(Thread* thread, const Tuple& items, word length);

  // Concatenates an iterable of bytes-like objects with a separator. Returns
  // Bytes or MutableBytes, depending on `sep`'s type.
  RawObject bytesJoin(Thread* thread, const Bytes& sep, word sep_length,
                      const Tuple& src, word src_length);

  // Creates a new Bytes or MutableBytes (depending on `source`'s type)
  // containing the first `length` bytes of the `source` repeated `count` times.
  // Specifies `length` explicitly to allow for ByteArrays with extra allocated
  // space.
  RawObject bytesRepeat(Thread* thread, const Bytes& source, word length,
                        word count);

  // Returns a new Bytes that contains the specified slice of self.
  RawObject bytesSlice(Thread* thread, const Bytes& self, word start, word stop,
                       word step);

  // Checks whether the specified range of self starts with the given prefix.
  // Returns Bool::trueObj() if the suffix matches, else Bool::falseObj().
  RawObject bytesStartsWith(const Bytes& self, word self_len,
                            const Bytes& prefix, word prefix_len, word start,
                            word end);

  // Returns a new Bytes containing the Bytes or MutableBytes subsequence of
  // self with the given start index and length.
  RawObject bytesSubseq(Thread* thread, const Bytes& self, word start,
                        word length);

  // Returns a new Bytes or MutableBytes copy of `self` with all of
  // the bytes in `del` removed, where the remaining bytes are mapped using
  // `table`.
  RawObject bytesTranslate(Thread* thread, const Bytes& self, word length,
                           const Bytes& table, word table_len, const Bytes& del,
                           word del_len);

  // Ensures that the list has at least the desired capacity.
  // Allocates if the existing capacity is insufficient.
  void listEnsureCapacity(Thread* thread, const List& list, word min_capacity);

  // Appends an element to the end of the list.
  void listAdd(Thread* thread, const List& list, const Object& value);

  // Create a MutableBytes object from a given Bytes
  RawObject mutableBytesFromBytes(Thread* thread, const Bytes& bytes);
  RawObject mutableBytesWith(word length, byte value);

  RawObject tupleSubseq(Thread* thread, const Tuple& self, word start,
                        word length);

  NewValueCellCallback* newValueCellCallback() {
    return &new_value_cell_callback_;
  }

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

  // Implements `receiver.name`
  NODISCARD RawObject attributeAt(Thread* thread, const Object& object,
                                  const Object& name);
  NODISCARD RawObject attributeAtById(Thread* thread, const Object& receiver,
                                      SymbolId id);
  NODISCARD RawObject attributeAtByCStr(Thread* thread, const Object& receiver,
                                        const char* name);

  // Implements `del receiver.name`
  NODISCARD RawObject attributeDel(Thread* thread, const Object& receiver,
                                   const Object& name);

  // Looks up the named attribute in the layout.
  //
  // If the attribute is found this returns true and sets info.
  // Returns false otherwise.
  static bool layoutFindAttribute(RawLayout layout, const Object& name,
                                  AttributeInfo* info);

  // Creates a new layout by adding empty slots to the base_layout
  // to match the NativeProxy layout
  RawObject createNativeProxyLayout(Thread* thread, const Layout& base_layout);

  // Add the attribute to the overflow array.
  //
  // This returns a new layout by either following a pre-existing edge or
  // adding one.
  RawObject layoutAddAttribute(Thread* thread, const Layout& layout,
                               const Object& name, word flags,
                               AttributeInfo* info);

  // Create a new tuple for the name, info pair and return a new tuple
  // containing entries + entry.
  RawObject layoutAddAttributeEntry(Thread* thread, const Tuple& entries,
                                    const Object& name, AttributeInfo info);

  // Delete the named attribute from the layout.
  //
  // If the attribute exists, this returns a new layout by either following
  // a pre-existing edge or adding one.
  //
  // If the attribute doesn't exist, Error::object() is returned.
  RawObject layoutDeleteAttribute(Thread* thread, const Layout& layout,
                                  const Object& name, AttributeInfo info);

  void typeAddBuiltinFunction(const Type& type, SymbolId name,
                              Function::Entry entry);

  void* nativeProxyPtr(RawObject object);

  void setNativeProxyPtr(RawObject object, void* c_ptr);

  // For commonly-subclassed builtin types, define isInstanceOfFoo(RawObject)
  // that does a check including subclasses (unlike RawObject::isFoo(), which
  // only gets exact types).
#define DEFINE_IS_INSTANCE(ty)                                                 \
  bool isInstanceOf##ty(RawObject obj) {                                       \
    if (obj.is##ty()) return true;                                             \
    return typeOf(obj).rawCast<RawType>().builtinBase() == LayoutId::k##ty;    \
  }
  DEFINE_IS_INSTANCE(BufferedReader)
  DEFINE_IS_INSTANCE(ByteArray)
  DEFINE_IS_INSTANCE(Bytes)
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
  DEFINE_IS_INSTANCE(StringIO)
  DEFINE_IS_INSTANCE(SystemExit)
  DEFINE_IS_INSTANCE(Tuple)
  DEFINE_IS_INSTANCE(Type)
  DEFINE_IS_INSTANCE(UnicodeDecodeError)
  DEFINE_IS_INSTANCE(UnicodeEncodeError)
  DEFINE_IS_INSTANCE(UnicodeError)
  DEFINE_IS_INSTANCE(UnicodeTranslateError)
  DEFINE_IS_INSTANCE(WeakRef)
#undef DEFINE_IS_INSTANCE

  // User-defined subclasses of immediate types have no corresponding LayoutId,
  // so we detect them by looking for an object that is a subclass of a
  // particular immediate type but not exactly that type.
#define DEFINE_IS_USER_INSTANCE(ty)                                            \
  bool isInstanceOfUser##ty##Base(RawObject obj) {                             \
    return !obj.is##ty() &&                                                    \
           typeOf(obj).rawCast<RawType>().builtinBase() == LayoutId::k##ty;    \
  }
  DEFINE_IS_USER_INSTANCE(Bytes)
  DEFINE_IS_USER_INSTANCE(Complex)
  DEFINE_IS_USER_INSTANCE(Float)
  DEFINE_IS_USER_INSTANCE(Int)
  DEFINE_IS_USER_INSTANCE(Str)
  DEFINE_IS_USER_INSTANCE(Tuple)
#undef DEFINE_IS_USER_INSTANCE

  bool isNativeProxy(RawObject obj) {
    return typeOf(obj).rawCast<RawType>().hasFlag(
        RawType::Flag::kIsNativeProxy);
  }

  // BaseException must be handled specially because it has builtin subclasses
  // that are visible to managed code.
  bool isInstanceOfBaseException(RawObject obj) {
    return Type::cast(typeOf(obj)).isBaseExceptionSubclass();
  }

  // SetBase must also be handled specially because many builtin functions
  // accept set or frozenset, despite them not having a common ancestor.
  bool isInstanceOfSetBase(RawObject instance) {
    if (instance.isSetBase()) {
      return true;
    }
    LayoutId builtin_base = Type::cast(typeOf(instance)).builtinBase();
    return builtin_base == LayoutId::kSet ||
           builtin_base == LayoutId::kFrozenSet;
  }

  bool isInstanceOfUnicodeErrorBase(RawObject instance) {
    return isInstanceOfUnicodeDecodeError(instance) ||
           isInstanceOfUnicodeEncodeError(instance) ||
           isInstanceOfUnicodeTranslateError(instance);
  }

  inline bool isByteslike(RawObject obj) {
    // TODO(T38246066): support bytes-like objects other than bytes, bytearray
    return isInstanceOfBytes(obj) || isInstanceOfByteArray(obj) ||
           obj.isMemoryView();
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

  static const word kInitialInternSetCapacity = 8192;

  static const word kInitialLayoutTupleCapacity = 1024;

  // Explicitly seed the random number generator
  void seedRandom(const uword random_state[2], const uword hash_secret[2]);

  // Returns whether object's class provides a __call__ method
  //
  // If its type defines a __call__, it is also callable (even if __call__ is
  // not actually callable).
  // Note that this does not include __call__ defined on the particular
  // instance, only __call__ defined on the type.
  bool isCallable(Thread* thread, const Object& obj);

  // Returns whether object's class provides a __delete__ method
  bool isDeleteDescriptor(Thread* thread, const Object& object);

  // Returns whether object's class provides a __next__ method
  bool isIterator(Thread* thread, const Object& obj);

  // Return whether object's class supports the sequence protocol
  bool isSequence(Thread* thread, const Object& obj);

  // Return whether object's class provides a __getitem__ method
  bool isMapping(Thread* thread, const Object& obj);

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

  // Computes the floor of the quotient of dividing `left` by `right` and
  // `left` modulo `right` so that `left = quotient * right + modulo`.
  // Note that this is different from C++ division (which truncates).
  // Writes the results to the handles pointed to by `quotient` or `modulo`.
  // It is allowed to specify a nullptr for any of them.
  // Returns true on success, false on division by zero.
  bool intDivideModulo(Thread* thread, const Int& dividend, const Int& divisor,
                       Object* quotient, Object* modulo);

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
  RawObject normalizeLargeInt(Thread* thread, const LargeInt& large_int);

  // Replace the occurences of oldstr get replaced for newstr in src up
  // to maxcount. If no replacement happens, returns src itself, unmodified.
  RawObject strReplace(Thread* thread, const Str& src, const Str& oldstr,
                       const Str& newstr, word count);

  void strArrayAddASCII(Thread* thread, const StrArray& array, byte code_point);
  void strArrayAddStr(Thread* thread, const StrArray& array, const Str& str);
  void strArrayAddStrArray(Thread* thread, const StrArray& array,
                           const StrArray& str);

  // Ensures that the str array has at least the desired capacity.
  // Allocates if the existing capacity is insufficient.
  void strArrayEnsureCapacity(Thread* thread, const StrArray& array,
                              word min_capacity);

  static word nextModuleIndex();

  // If the importlib module has already been initialized and added, return it.
  // Else, create and add it to the runtime.
  NODISCARD RawObject findOrCreateImportlibModule(Thread* thread);

  // If the main module has already been initialized and added, return it.
  // Else, create and add it to the runtime.
  RawObject findOrCreateMainModule();

  // Execute the code object that represents the code for the top-level module
  // (eg the result of compiling some_file.py). Return the result.
  NODISCARD RawObject executeModule(const Code& code, const Module& module);

  // Execute a frozen module by marshalling it into a code object and then
  // executing it.
  RawObject executeFrozenModule(const char* buffer, const Module& module);

  static int heapOffset() { return OFFSETOF(Runtime, heap_); }

  static int layoutsOffset() { return OFFSETOF(Runtime, layouts_); }

  // Equivalent to `o0 is o1 or o0 == o1` optimized for LargeStr, SmallStr and
  // SmallInt objects.
  static RawObject objectEquals(Thread* thread, RawObject o0, RawObject o1);

  void* parserGrammar() { return parser_grammar_; }
  void setParserGrammar(void* grammar, void (*grammar_free)(void*)) {
    parser_grammar_ = grammar;
    parser_grammar_free_func_ = grammar_free;
  }

 private:
  void initializeApiData();
  void initializeExceptionTypes();
  void initializeHeapTypes();
  void initializeImmediateTypes();
  void initializeInterned();
  void initializeInterpreter();
  void initializeLayouts();
  void initializeModules();
  void initializePrimitiveInstances();
  void initializeRandom();
  void initializeSymbols();
  void initializeThreads();
  void initializeTypes();

  void createImportlibModule(Thread* thread);
  RawObject createMainModule();

  void visitRuntimeRoots(PointerVisitor* visitor);
  void visitThreadRoots(PointerVisitor* visitor);

  word identityHash(RawObject object);
  word immediateHash(RawObject object);

  void growInternSet(Thread* thread);

  RawObject createMro(const Layout& subclass_layout, LayoutId superclass_id);

  RawObject newFunction(Thread* thread, const Object& name, const Object& code,
                        word flags, word argcount, word total_args,
                        word total_vars, word stacksize, Function::Entry entry,
                        Function::Entry entry_kw, Function::Entry entry_ex);

  // Generic attribute deletion code used for class objects
  // TODO(T55871582): Remove code paths that can raise from the Runtime
  NODISCARD RawObject classDelAttr(Thread* thread, const Object& receiver,
                                   const Object& name);

  // Generic attribute deletion code used for instance objects
  // TODO(T55871582): Remove code paths that can raise from the Runtime
  NODISCARD RawObject instanceDelAttr(Thread* thread, const Object& receiver,
                                      const Object& name);

  // Generic attribute deletion code used for module objects
  // TODO(T55871582): Remove code paths that can raise from the Runtime
  NODISCARD RawObject moduleDelAttr(Thread* thread, const Object& receiver,
                                    const Object& name);

  // Creates a layout that is a subclass of a built-in class and zero or more
  // additional built-in attributes.
  RawObject layoutCreateSubclassWithBuiltins(LayoutId subclass_id,
                                             LayoutId superclass_id,
                                             View<BuiltinAttribute> attributes);

  // Appends attribute entries for fixed attributes to an array of in-object
  // attribute entries starting at a specific index.  Useful for constructing
  // the in-object attributes array for built-in classes with fixed attributes.
  void appendBuiltinAttributes(View<BuiltinAttribute> attributes,
                               const Tuple& dst, word start_index);

  // Creates a new layout that will be a child layout of the supplied parent.
  //
  // The new layout shares the in-object and overflow attributes with the
  // parent and contains no outgoing edges.
  RawObject layoutCreateChild(Thread* thread, const Layout& layout);

  RawObject modulesByIndex() { return modules_by_index_; }

  // Joins the type's name and attribute's name to produce a qualname
  RawObject newQualname(Thread* thread, const Type& type, SymbolId name);

  // Inserts an entry into the linked list as the new root
  static bool listEntryInsert(ListEntry* entry, ListEntry** root);

  // Removes an entry from the linked list
  static bool listEntryRemove(ListEntry* entry, ListEntry** root);

  word numTrackedApiHandles() { return Dict::cast(api_handles_).numItems(); }

  word numTrackedNativeObjects() { return num_tracked_native_objects_; }

  word numTrackedNativeGcObjects() { return num_tracked_native_gc_objects_; }

  // Clear all active handle scopes
  void clearHandleScopes();

  // Clear the allocated memory from all extension related objects
  void freeApiHandles();

  // The size newCapacity grows to if array is empty. Must be large enough to
  // guarantee a LargeBytes/LargeStr for ByteArray/StrArray.
  static const int kInitialEnsuredCapacity = kWordSize * 2;
  static_assert(kInitialEnsuredCapacity > SmallStr::kMaxLength,
                "array must be backed by a heap type");

  Heap heap_;

  std::unique_ptr<Interpreter> interpreter_;

  // TODO(T46009495): This is a temporary list tracking native objects to
  // correctly free their memory at runtime destruction. However, this should
  // be moved to a lower level abstraction in the C-API such as PyObject_Malloc
  // Linked list of tracked extension objects.
  ListEntry* tracked_native_objects_ = nullptr;
  word num_tracked_native_objects_ = 0;

  ListEntry* tracked_native_gc_objects_ = nullptr;
  word num_tracked_native_gc_objects_ = 0;

  // List of native instances which can be finalizable through tp_dealloc
  RawObject finalizable_references_ = NoneType::object();

  // A MutableTuple of Layout objects, indexed by layout id.
  RawObject layouts_ = NoneType::object();
  // The number of layout objects in layouts_.
  word num_layouts_ = 0;

  // The last module ID given out.
  word max_module_id_ = 0;

  // Internal-only types, for which the Layout has a different described type
  RawObject large_bytes_ = NoneType::object();
  RawObject large_int_ = NoneType::object();
  RawObject large_str_ = NoneType::object();
  RawObject small_bytes_ = NoneType::object();
  RawObject small_int_ = NoneType::object();
  RawObject small_str_ = NoneType::object();

  // Cached instances
  RawObject build_class_ = NoneType::object();
  RawObject display_hook_ = NoneType::object();
  RawObject dunder_import_ = NoneType::object();
  RawObject ellipsis_ = NoneType::object();
  RawObject empty_frozen_set_ = NoneType::object();
  RawObject empty_mutable_bytes_ = NoneType::object();
  RawObject empty_slice_ = NoneType::object();
  RawObject empty_tuple_ = NoneType::object();
  RawObject implicit_bases_ = NoneType::object();
  RawObject module_dunder_getattribute_ = NoneType::object();
  RawObject object_dunder_getattribute_ = NoneType::object();
  RawObject object_dunder_init_ = NoneType::object();
  RawObject object_dunder_new_ = NoneType::object();
  RawObject object_dunder_setattr_ = NoneType::object();
  RawObject str_dunder_eq_ = NoneType::object();
  RawObject str_dunder_hash_ = NoneType::object();
  RawObject sys_stderr_ = NoneType::object();
  RawObject sys_stdout_ = NoneType::object();
  RawObject type_dunder_getattribute_ = NoneType::object();

  // Interned strings
  RawObject interned_ = NoneType::object();
  word interned_remaining_ = 0;

  // Modules
  RawObject modules_ = NoneType::object();
  RawObject modules_by_index_ = NoneType::object();

  // ApiHandles
  RawObject api_handles_ = NoneType::object();

  // Some API functions promise to cache their return value and return the same
  // value for repeated invocations on a specific PyObject. Those value are
  // cached here.
  RawObject api_caches_ = NoneType::object();

  // Weak reference callback list
  RawObject callbacks_ = NoneType::object();

  Thread* threads_;

  uword random_state_[2];
  uword hash_secret_[2];

  NewValueCellCallback new_value_cell_callback_;

  Symbols* symbols_;

  void* parser_grammar_ = nullptr;
  void (*parser_grammar_free_func_)(void*) = nullptr;

  // atexit C Function
  AtExitFn at_exit_ = nullptr;

  static word next_module_index_;

  friend class ApiHandle;

  DISALLOW_COPY_AND_ASSIGN(Runtime);
};

inline RawObject Runtime::internStr(Thread* thread, const Object& str) {
  if (str.isSmallStr()) {
    return *str;
  }
  return internLargeStr(thread, str);
}

class BuiltinsBase {
 public:
  static void postInitialize(Runtime*, const Type&) {}

  static const BuiltinAttribute kAttributes[];
  static const BuiltinMethod kBuiltinMethods[];

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
    Type new_type(&scope,
                  runtime->addBuiltinType(T::kName, T::kType, T::kSuperType,
                                          T::kAttributes, T::kBuiltinMethods));
    new_type.sealAttributes();
    T::postInitialize(runtime, new_type);
  }

 protected:
  static const SymbolId kName = name;
  static const LayoutId kType = type;
  static const LayoutId kSuperType = supertype;
};

}  // namespace py
