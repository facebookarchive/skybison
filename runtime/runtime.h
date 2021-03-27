#pragma once

#include "bytecode.h"
#include "capi.h"
#include "handles.h"
#include "heap.h"
#include "interpreter-gen.h"
#include "interpreter.h"
#include "layout.h"
#include "modules.h"
#include "mutex.h"
#include "symbols.h"
#include "view.h"

namespace py {

class AttributeInfo;
class Heap;
class RawObject;
class RawTuple;
class PointerVisitor;
class Thread;

struct ListEntry {
  ListEntry* prev;
  ListEntry* next;
};

enum LayoutTypeTransition {
  kFrom = 0,
  kTo = 1,
  kResult = 2,
  kTransitionSize,
};

using AtExitFn = void (*)(void*);

using DictEq = RawObject (*)(Thread*, RawObject, RawObject);

enum class ReadOnly : bool {
  ReadWrite,
  ReadOnly,
};

struct RandomState {
  uint64_t state[2];
  uint64_t siphash24_secret;
  uint64_t extra_secret[3];
};
RandomState randomState();
RandomState randomStateFromSeed(uint64_t seed);

class Runtime {
 public:
  Runtime(word heap_size, Interpreter* interpreter, RandomState random_seed);
  ~Runtime();

  // Completes the runtime initialization. Should be called after
  // `initializeSys`.
  RawObject initialize(Thread* thread);

  bool initialized() { return initialized_; }

  // TODO(T75349221): Make createXXX functions private or remove them
  RawObject createLargeInt(word num_digits);
  RawObject createLargeStr(word length);

  RawObject newBoundMethod(const Object& function, const Object& self);

  RawObject newBytearray();
  RawObject newBytearrayIterator(Thread* thread, const Bytearray& bytearray);

  RawObject newBytes(word length, byte fill);
  RawObject newBytesWithAll(View<byte> array);
  RawObject newBytesIterator(Thread* thread, const Bytes& bytes);

  RawObject newTraceback();

  RawObject newType();

  RawObject newTypeWithMetaclass(LayoutId metaclass_id);

  RawObject newTypeProxy(const Type& type);

  RawObject newCell();

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
                           BuiltinFunction function,
                           const Object& parameter_names,
                           const Object& name_str);

  RawObject newComplex(double real, double imag);

  RawObject newCoroutine();

  RawObject newDeque();

  RawObject newDequeIterator(const Deque& deque, word index);
  RawObject newDequeReverseIterator(const Deque& deque, word index);

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

  RawObject newFunction(Thread* thread, const Object& name, const Object& code,
                        word flags, word argcount, word total_args,
                        word total_vars, const Object& stacksize_or_builtin,
                        Function::Entry entry, Function::Entry entry_kw,
                        Function::Entry entry_ex);

  RawObject newFunctionWithCode(Thread* thread, const Object& qualname,
                                const Code& code, const Object& module_obj);

  RawObject newExceptionState();

  RawObject newFrameProxy(Thread* thread, const Object& function,
                          const Object& lasti);

  RawObject newGenerator();

  RawObject newGeneratorFrame(const Function& function);

  RawObject newInstance(const Layout& layout);
  RawObject newInstanceWithSize(LayoutId layout_id, word object_size);

  // Create a new Int from a signed value.
  RawObject newInt(word value);

  // Create a new Int from an unsigned value.
  RawObject newIntFromUnsigned(uword value);

  // Create a new Int from a sequence of digits, which will be interpreted as a
  // signed, two's-complement number. The digits must satisfy the invariants
  // listed on the LargeInt class.
  RawObject newIntWithDigits(View<uword> digits);

  RawObject newLayout(LayoutId id);

  RawObject newList();

  RawObject newListIterator(const Object& list);

  RawObject newSeqIterator(const Object& sequence);

  RawObject newSlotDescriptor(const Type& type, const Object& name);

  // Create a new MemoryView object. Initializes the view format to "B".
  RawObject newMemoryView(Thread* thread, const Object& obj,
                          const Object& buffer, word length,
                          ReadOnly read_only);
  // Create a new MemoryView object from a C pointer. Initializes the view
  // format to "B".
  RawObject newMemoryViewFromCPtr(Thread* thread, const Object& obj, void* ptr,
                                  word length, ReadOnly read_only);

  // Create a new Mmap object.
  RawObject newMmap();

  RawObject newModule(const Object& name);

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

  // Return a new, zero-initialized, mutable tuple of the given length.
  RawObject newMutableTuple(word length);

  // Return a new tuple containing the specified arguments.
  RawObject newTupleWith1(const Object& item1);
  RawObject newTupleWith2(const Object& item1, const Object& item2);
  RawObject newTupleWith3(const Object& item1, const Object& item2,
                          const Object& item3);
  RawObject newTupleWith4(const Object& item1, const Object& item2,
                          const Object& item3, const Object& item4);
  RawObject newTupleWithN(word num_items, const Object* item1, ...);

  RawObject newPointer(void* cptr, word length);

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

  // Constructors from _contextvars
  RawObject newContext(const Dict& data);
  RawObject newContextVar(const Str& name, const Object& default_value);
  RawObject newToken(const Context& ctx, const ContextVar& ctx_var,
                     const Object& old_value);

  void processCallbacks();
  void processFinalizers();

  RawObject strConcat(Thread* thread, const Str& left, const Str& right);
  RawObject strJoin(Thread* thread, const Str& sep, const Tuple& items,
                    word allocated);

  // Creates a new Str containing `str` repeated `count` times.
  RawObject strRepeat(Thread* thread, const Str& str, word count);

  RawObject strSlice(Thread* thread, const Str& str, word start, word stop,
                     word step);

  RawObject newValueCell();

  RawObject newWeakLink(Thread* thread, const Object& referent,
                        const Object& prev, const Object& next);

  RawObject newWeakRef(Thread* thread, const Object& referent);

  RawObject ellipsis() { return ellipsis_; }

  word builtinsModuleId() { return builtins_module_id_; }
  void setBuiltinsModuleId(word builtins_module_id) {
    builtins_module_id_ = builtins_module_id;
  }

  RawObject moduleDunderGetattribute() { return module_dunder_getattribute_; }
  RawObject objectDunderClass() { return object_dunder_class_; }
  RawObject objectDunderGetattribute() { return object_dunder_getattribute_; }
  RawObject objectDunderHash() { return object_dunder_hash_; }
  RawObject objectDunderInit() { return object_dunder_init_; }
  RawObject objectDunderNew() { return object_dunder_new_; }
  RawObject objectDunderSetattr() { return object_dunder_setattr_; }
  RawObject strDunderEq() { return str_dunder_eq_; }
  RawObject strDunderHash() { return str_dunder_hash_; }
  RawValueCell sysStderr() { return ValueCell::cast(sys_stderr_); }
  RawValueCell sysStdin() { return ValueCell::cast(sys_stdin_); }
  RawValueCell sysStdout() { return ValueCell::cast(sys_stdout_); }
  RawObject typeDunderGetattribute() { return type_dunder_getattribute_; }

  RawObject profilingNewThread() { return profiling_new_thread_; }
  RawObject profilingCall() { return profiling_call_; }
  RawObject profilingReturn() { return profiling_return_; }

  void setProfiling(const Object& new_thread_func, const Object& call_func,
                    const Object& return_func);

  void reinitInterpreter();

  void builtinTypeCreated(Thread* thread, const Type& type);

  void cacheBuildClass(Thread* thread, const Module& builtins);
  void cacheSysInstances(Thread* thread, const Module& sys);

  static RawObject internStr(Thread* thread, const Object& str);
  static RawObject internStrFromAll(Thread* thread, View<byte> bytes);
  static RawObject internStrFromCStr(Thread* thread, const char* c_str);
  static RawObject internLargeStr(Thread* thread, const Object& str);
  // This function should only be used for `CHECK()`/`DCHECK()`. It is as slow
  // as the whole `internStr()` operation and will always return true for small
  // strings, even when the user did not explicitly intern them.
  static bool isInternedStr(Thread* thread, const Object& str);

  void collectGarbage();

  // Creates a new thread and adds it to the runtime.
  Thread* newThread();

  // Removes the specified thread from the runtime and deletes it.
  void deleteThread(Thread* thread);

  // Compute hash value suitable for `RawObject::operator==` (aka `a is b`)
  // equality tests.
  word hash(RawObject object);
  // Compute hash value for objects with byte payload. This is a helper to
  // implement `xxxHash()` functions.
  word valueHash(RawObject object);

  word identityHash(RawObject object);

  word bytesHash(View<byte> array);

  uword random();

  Heap* heap() { return &heap_; }

  Interpreter* interpreter() { return interpreter_.get(); }

  // Tracks extension native objects.
  // Returns true if an untracked entry becomes tracked, false, otherwise.
  bool trackNativeObject(ListEntry* entry);

  // Untracks extension native objects.
  // Returns true if a tracked entry becomes untracked, false, otherwise.
  bool untrackNativeObject(ListEntry* entry);

  // Return the head of the tracked native objects list.
  ListEntry* trackedNativeObjects();
  RawObject* finalizableReferences();

  void visitRoots(PointerVisitor* visitor);

  RawObject findModule(const Object& name);
  RawObject findModuleById(SymbolId name);
  RawObject lookupNameInModule(Thread* thread, SymbolId module_name,
                               SymbolId name);

  // Write the traceback to the given file object. If success, return None.
  // Else, return Error.
  RawObject printTraceback(Thread* thread, word fd);

  // Returns the implicit bases -- (object,) -- for a new class.
  RawObject implicitBases();

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

  RawType typeOf(RawObject object) {
    return Layout::cast(layoutOf(object)).describedType().rawCast<RawType>();
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
  RawObject layoutOf(RawObject obj) {
    if (obj.isHeapObject()) {
      return layoutAt(RawHeapObject::cast(obj).header().layoutId());
    }
    return layoutAt(
        static_cast<LayoutId>(obj.raw() & Object::kImmediateTagMask));
  }

  // Raw access to the MutableTuple of Layouts. Intended only for use by the GC.
  RawObject layouts() { return layouts_; }
  void setLayouts(RawObject layouts) { layouts_ = layouts; }

  // Raw access to the Tuple of layout transitions while setting __class__.
  // Intended only for use by the GC.
  RawObject layoutTypeTransitions() { return layout_type_transitions_; }
  void setLayoutTypeTransitions(RawObject value) {
    layout_type_transitions_ = value;
  }

  void layoutSetTupleOverflow(RawLayout layout);

  LayoutId reserveLayoutId(Thread* thread);

  word reserveModuleId();

  RawObject buildClass() { return build_class_; }

  RawObject displayHook() { return display_hook_; }

  // Returns modules mapping (aka `sys.modules`).
  RawObject modules() { return modules_; }

  CAPIState* capiState() { return reinterpret_cast<CAPIState*>(capi_state_); }

  void setAtExit(AtExitFn func, void* ctx) {
    DCHECK(at_exit_ == nullptr,
           "setAtExit should not override existing at_exit function");
    at_exit_ = func;
    at_exit_context_ = ctx;
  }

  void callAtExit() {
    if (at_exit_ == nullptr) return;
    at_exit_(at_exit_context_);
    at_exit_ = nullptr;
    at_exit_context_ = nullptr;
  }

  Symbols* symbols() { return symbols_; }

  // Provides a growth strategy for mutable sequences. Grows by a factor of 1.5,
  // scaling up to the requested capacity if the initial factor is insufficient.
  // Always grows the sequence.
  static word newCapacity(word curr_capacity, word min_capacity);

  // Ensures that the byte array has at least the desired capacity.
  // Allocates if the existing capacity is insufficient.
  void bytearrayEnsureCapacity(Thread* thread, const Bytearray& array,
                               word min_capacity);

  // Appends multiple bytes to the end of the array.
  void bytearrayExtend(Thread* thread, const Bytearray& array, View<byte> view);
  void bytearrayIadd(Thread* thread, const Bytearray& array, const Bytes& bytes,
                     word length);

  // Returns a new Bytes containing the elements of `left` and `right`.
  RawObject bytesConcat(Thread* thread, const Bytes& left, const Bytes& right);

  // Returns a copy of a bytes object
  RawObject bytesCopy(Thread* thread, const Bytes& src);

  // Makes a new copy of the `original` bytes with the specified `size`.
  // If the new length is less than the old length, truncate the bytes to fit.
  // If the new length is greater than the old length, pad with trailing zeros.
  RawObject bytesCopyWithSize(Thread* thread, const Bytes& original,
                              word new_length);

  // Checks whether the specified range of `bytes` ends with the given `suffix`.
  // Returns `Bool::trueObj()` if the suffix matches, else `Bool::falseObj()`.
  RawObject bytesEndsWith(const Bytes& bytes, word bytes_len,
                          const Bytes& suffix, word suffix_len, word start,
                          word end);

  // Returns a new Bytes from the first `length` int-like elements in the tuple.
  RawObject bytesFromTuple(Thread* thread, const Tuple& items, word length);

  // Creates a new Bytes or MutableBytes (depending on `source`'s type)
  // containing the first `length` bytes of the `source` repeated `count` times.
  // Specifies `length` explicitly to allow for Bytearrays with extra allocated
  // space.
  RawObject bytesRepeat(Thread* thread, const Bytes& source, word length,
                        word count);

  // Replace the occurances of old_bytes with new_bytes in src up to max_count.
  // If no instances of old_bytes exist, old_bytes == new_bytes, or max_count is
  // zero, return src unmodified.
  // NOTE: a negative max_count value is used to signify that all instaces of
  // old_bytes should be replaced
  RawObject bytesReplace(Thread* thread, const Bytes& src,
                         const Bytes& old_bytes, word old_len,
                         const Bytes& new_bytes, word new_len, word max_count);

  // Returns a new Bytes that contains the specified slice of bytes.
  RawObject bytesSlice(Thread* thread, const Bytes& bytes, word start,
                       word stop, word step);

  // Checks whether the specified range of bytes starts with the given prefix.
  // Returns Bool::trueObj() if the suffix matches, else Bool::falseObj().
  RawObject bytesStartsWith(const Bytes& bytes, word bytes_len,
                            const Bytes& prefix, word prefix_len, word start,
                            word end);

  // Returns a new Bytes or MutableBytes copy of `bytes` with all of
  // the bytes in `del` removed, where the remaining bytes are mapped using
  // `table`.
  RawObject bytesTranslate(Thread* thread, const Bytes& bytes, word length,
                           const Bytes& table, word table_len, const Bytes& del,
                           word del_len);

  // Returns the repr-string for a byteslike object. The caller must provide the
  // size of the resulting string, accounting for escaping, and the correct
  // delimiter character, which is either a single- or double-quote.
  RawObject byteslikeRepr(Thread* thread, const Byteslike& byteslike,
                          word result_length, byte delimiter);

  // Ensures that the list has at least the desired capacity.
  // Allocates if the existing capacity is insufficient.
  void listEnsureCapacity(Thread* thread, const List& list, word min_capacity);

  // Appends an element to the end of the list.
  void listAdd(Thread* thread, const List& list, const Object& value);

  // Create a MutableBytes object from a given Bytes
  RawObject mutableBytesFromBytes(Thread* thread, const Bytes& bytes);
  RawObject mutableBytesWith(word length, byte value);

  RawObject tupleSubseq(Thread* thread, const Tuple& tuple, word start,
                        word length);

  // Creates a layout that is a subclass of a built-in class and zero or more
  // additional built-in attributes.
  RawObject layoutCreateSubclassWithBuiltins(Thread* thread,
                                             LayoutId subclass_id,
                                             LayoutId superclass_id,
                                             View<BuiltinAttribute> attributes,
                                             word size);

  RawObject layoutNewAttribute(const Object& name, AttributeInfo info);

  // Performs a simple scan of the bytecode and collects all attributes that
  // are set via `self.<attribute> =` into attributes.
  void collectAttributes(const Code& code, const Dict& attributes);

  // Returns type's __init__ method, or None
  RawObject classConstructor(const Type& type);

  // Implements `receiver.name`
  NODISCARD RawObject attributeAt(Thread* thread, const Object& receiver,
                                  const Object& name);
  NODISCARD RawObject attributeAtSetLocation(Thread* thread,
                                             const Object& receiver,
                                             const Object& name,
                                             LoadAttrKind* kind,
                                             Object* location_out);
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

  // Creates a copy of a layout with a new layout id.
  //
  // The new layout shares the in-object and overflow attributes and contains
  // no outgoing edges.
  RawObject layoutCreateCopy(Thread* thread, const Layout& layout);

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

  // Change the described type of a layout. Follow an edge if it exists, or
  // create a new layout otherwise.
  // This is used when assigning to __class__ on an instance.
  RawObject layoutSetDescribedType(Thread* thread, const Layout& from,
                                   const Type& to);

  // Delete the named attribute from the layout.
  //
  // If the attribute exists, this returns a new layout by either following
  // a pre-existing edge or adding one.
  //
  // If the attribute doesn't exist, Error::object() is returned.
  RawObject layoutDeleteAttribute(Thread* thread, const Layout& layout,
                                  const Object& name, AttributeInfo info);

  // Return the dict overflow-only layout derived from `type`, for instances
  // having `num_in_object_attr` in-object attributes.
  RawObject typeDictOnlyLayout(Thread* thread, const Type& type,
                               word num_in_object_attr);

  // For commonly-subclassed builtin types, define isInstanceOfFoo(RawObject)
  // that does a check including subclasses (unlike RawObject::isFoo(), which
  // only gets exact types).
#define DEFINE_IS_INSTANCE(ty)                                                 \
  bool isInstanceOf##ty(RawObject obj) {                                       \
    if (obj.is##ty()) return true;                                             \
    return typeOf(obj).rawCast<RawType>().builtinBase() == LayoutId::k##ty;    \
  }
  DEFINE_IS_INSTANCE(Array)
  DEFINE_IS_INSTANCE(BufferedReader)
  DEFINE_IS_INSTANCE(BufferedWriter)
  DEFINE_IS_INSTANCE(Bytearray)
  DEFINE_IS_INSTANCE(Bytes)
  DEFINE_IS_INSTANCE(BytesIO)
  DEFINE_IS_INSTANCE(ClassMethod)
  DEFINE_IS_INSTANCE(Complex)
  DEFINE_IS_INSTANCE(Deque)
  DEFINE_IS_INSTANCE(Dict)
  DEFINE_IS_INSTANCE(FileIO)
  DEFINE_IS_INSTANCE(Float)
  DEFINE_IS_INSTANCE(FrozenSet)
  DEFINE_IS_INSTANCE(ImportError)
  DEFINE_IS_INSTANCE(Int)
  DEFINE_IS_INSTANCE(List)
  DEFINE_IS_INSTANCE(Mmap)
  DEFINE_IS_INSTANCE(Module)
  DEFINE_IS_INSTANCE(Property)
  DEFINE_IS_INSTANCE(Set)
  DEFINE_IS_INSTANCE(StaticMethod)
  DEFINE_IS_INSTANCE(StopIteration)
  DEFINE_IS_INSTANCE(Str)
  DEFINE_IS_INSTANCE(StringIO)
  DEFINE_IS_INSTANCE(SystemExit)
  DEFINE_IS_INSTANCE(TextIOWrapper)
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
  DEFINE_IS_USER_INSTANCE(WeakRef)
#undef DEFINE_IS_USER_INSTANCE

  bool isInstanceOfNativeProxy(RawObject obj) {
    // Note that this reports true when the object can be used safely via
    // `RawNativeProxy` or assigned to a `NativeProxy` handle. The function
    // name is required for `Hanlde<>` to work. It is misleading in that we
    // consider native proxy to be more of a property of the type than is
    // orthogonal to the subtyping relationships.
    return typeOf(obj).rawCast<RawType>().hasNativeData();
  }

  // BaseException must be handled specially because it has builtin subclasses
  // that are visible to managed code.
  bool isInstanceOfBaseException(RawObject obj) {
    return typeOf(obj).rawCast<RawType>().isBaseExceptionSubclass();
  }

  // SetBase must also be handled specially because many builtin functions
  // accept set or frozenset, despite them not having a common ancestor.
  bool isInstanceOfSetBase(RawObject instance) {
    if (instance.isSetBase()) {
      return true;
    }
    LayoutId builtin_base = typeOf(instance).rawCast<RawType>().builtinBase();
    return builtin_base == LayoutId::kSet ||
           builtin_base == LayoutId::kFrozenSet;
  }

  bool isInstanceOfUnicodeErrorBase(RawObject instance) {
    return isInstanceOfUnicodeDecodeError(instance) ||
           isInstanceOfUnicodeEncodeError(instance) ||
           isInstanceOfUnicodeTranslateError(instance);
  }

  inline bool isByteslike(RawObject obj) {
    return isInstanceOfBytes(obj) || isInstanceOfBytearray(obj) ||
           obj.isMemoryView() || obj.isArray();
  }

  // Clear the allocated memory from all extension related objects
  void deallocExtensions();

  // Initial data of the set.
  static const int kSetGrowthFactor = 2;
  static const int kInitialSetCapacity = 8;

  static const word kInitialInternSetCapacity = 8192;

  static const word kInitialLayoutTupleCapacity = 1024;

  void setRandomState(RandomState random_state) {
    random_state_ = random_state;
  }

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

  // Converts bytes object into an int object. The conversion is performed
  // with the specified endianness. `is_signed` specifies whether the highest
  // bit is considered a sign.
  RawObject bytesToInt(Thread* thread, const Bytes& bytes, endian endianness,
                       bool is_signed);

  static uint64_t hashWithKey(const Bytes& bytes, uint64_t key);

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
  void strArrayAddCodePoint(Thread* thread, const StrArray& array,
                            int32_t code_point);
  void strArrayAddStr(Thread* thread, const StrArray& array, const Str& str);
  void strArrayAddStrArray(Thread* thread, const StrArray& array,
                           const StrArray& str);

  // Ensures that the str array has at least the desired capacity.
  // Allocates if the existing capacity is insufficient.
  void strArrayEnsureCapacity(Thread* thread, const StrArray& array,
                              word min_capacity);

  static word nextModuleIndex();

  static int heapOffset() { return OFFSETOF(Runtime, heap_); }

  static int layoutsOffset() { return OFFSETOF(Runtime, layouts_); }

  // Equivalent to `o0 is o1 or o0 == o1` optimized for LargeStr, SmallStr and
  // SmallInt objects.
  static RawObject objectEquals(Thread* thread, RawObject o0, RawObject o1);

  // The exec_prefix is a directory prefix where platform-dependent Python files
  // are installed (e.g. compiled .so files)
  static wchar_t* execPrefix() { return exec_prefix_; }
  static void setExecPrefix(const wchar_t* exec_prefix);

  // Getter/setter for the module search path.
  static wchar_t* moduleSearchPath() { return module_search_path_; }
  static void setModuleSearchPath(const wchar_t* module_search_path);

  // The prefix is a directory prefix where platform-independent Python files
  // are installed (e.g. .py library files)
  static wchar_t* prefix() { return prefix_; }
  static void setPrefix(const wchar_t* prefix);

  static wchar_t* programName();
  static void setProgramName(const wchar_t* program_name);

  const byte* hashSecret(size_t size);

  // Sets up the signal handlers.
  void initializeSignals(Thread* thread, const Module& under_signal);

  void finalizeSignals(Thread* thread);

  RawObject handlePendingSignals(Thread* thread);
  void setPendingSignal(Thread* thread, int signum);

  RawObject signalCallback(word signum);
  RawObject setSignalCallback(word signum, const Object& callback);

  bool isFinalizing() { return is_finalizing_; }

  Thread* mainThread() { return main_thread_; }

  void populateEntryAsm(const Function& function);

 private:
  Runtime(word heap_size);

  void initializeHeapTypes(Thread* thread);
  void initializeInterned(Thread* thread);
  void initializeLayouts();
  void initializeModules(Thread* thread);
  void initializePrimitiveInstances();
  void initializeSymbols(Thread* thread);
  void initializeTypes(Thread* thread);

  void internSetGrow(Thread* thread);

  void visitRuntimeRoots(PointerVisitor* visitor);
  void visitThreadRoots(PointerVisitor* visitor);

  word siphash24(View<byte> array);

  RawObject createLargeBytes(word length);
  RawObject createMutableBytes(word length);

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

  // Appends attribute entries for fixed attributes to an array of in-object
  // attribute entries starting at a specific index.  Useful for constructing
  // the in-object attributes array for built-in classes with fixed attributes.
  void appendBuiltinAttributes(Thread* thread,
                               View<BuiltinAttribute> attributes,
                               const MutableTuple& dst, word start_index);

  // Joins the type's name and attribute's name to produce a qualname
  RawObject newQualname(Thread* thread, const Type& type, SymbolId name);

  // Inserts an entry into the linked list as the new root
  static bool listEntryInsert(ListEntry* entry, ListEntry** root);

  // Removes an entry from the linked list
  static bool listEntryRemove(ListEntry* entry, ListEntry** root);

  static word immediateHash(RawObject object);

  word numTrackedNativeObjects() { return num_tracked_native_objects_; }

  // Clear all active handle scopes
  void clearHandleScopes();

  // Clear the allocated memory from all extension related objects
  void freeApiHandles();

  bool is_finalizing_ = false;

  // The size newCapacity grows to if array is empty. Must be large enough to
  // guarantee a LargeBytes/LargeStr for Bytearray/StrArray.
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

  // List of native instances which can be finalizable through tp_dealloc
  RawObject finalizable_references_ = NoneType::object();

  // A MutableTuple of Layout objects, indexed by layout id.
  RawObject layouts_ = NoneType::object();
  // The number of layout objects in layouts_.
  word num_layouts_ = 0;

  // A Tuple of (A, B, C) triples representing transitions from a layout A to a
  // class B, resulting in final cached layout C.
  RawObject layout_type_transitions_ = NoneType::object();

  // The last module ID given out.
  word max_module_id_ = 0;

  // The ID of builtins module.
  // TODO(T64005113): Remove this once we mark individual functions.
  word builtins_module_id_ = -1;

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
  RawObject ellipsis_ = NoneType::object();
  RawObject empty_frozen_set_ = NoneType::object();
  RawObject empty_mutable_bytes_ = NoneType::object();
  RawObject empty_slice_ = NoneType::object();
  RawObject empty_tuple_ = NoneType::object();
  RawObject module_dunder_getattribute_ = NoneType::object();
  RawObject object_dunder_class_ = NoneType::object();
  RawObject object_dunder_getattribute_ = NoneType::object();
  RawObject object_dunder_hash_ = NoneType::object();
  RawObject object_dunder_init_ = NoneType::object();
  RawObject object_dunder_new_ = NoneType::object();
  RawObject object_dunder_setattr_ = NoneType::object();
  RawObject str_dunder_eq_ = NoneType::object();
  RawObject str_dunder_hash_ = NoneType::object();
  RawObject sys_stderr_ = NoneType::object();
  RawObject sys_stdin_ = NoneType::object();
  RawObject sys_stdout_ = NoneType::object();
  RawObject type_dunder_getattribute_ = NoneType::object();

  RawObject profiling_new_thread_ = NoneType::object();
  RawObject profiling_call_ = NoneType::object();
  RawObject profiling_return_ = NoneType::object();

  // Interned strings
  RawObject interned_ = NoneType::object();
  word interned_remaining_ = 0;

  // Modules
  RawObject modules_ = NoneType::object();

  // C-API State
  char capi_state_[kCAPIStateSize];

  // Weak reference callback list
  RawObject callbacks_ = NoneType::object();

  // Quick check if any signals have been tripped.
  volatile bool is_signal_pending_ = false;

  // Tuple mapping each signal to either SIG_DFL, SIG_IGN, None,
  // or a Python object to be called when handling the signal.
  RawObject signal_callbacks_ = NoneType::object();

  void* signal_stack_ = nullptr;

  // File descriptor for writing when a signal is received.
  int wakeup_fd_ = -1;

  Thread* main_thread_ = nullptr;
  Mutex threads_mutex_;

  RandomState random_state_;

  Symbols* symbols_;

  // atexit thunk (to be passed into pylifecycle and called with atexit module)
  AtExitFn at_exit_ = nullptr;
  void* at_exit_context_ = nullptr;

  bool initialized_ = false;

  static word next_module_index_;

  static wchar_t exec_prefix_[];
  static wchar_t module_search_path_[];
  static wchar_t prefix_[];
  static wchar_t program_name_[];

  DISALLOW_COPY_AND_ASSIGN(Runtime);
};

inline RawObject Runtime::emptyMutableBytes() { return empty_mutable_bytes_; }

inline RawObject Runtime::emptySlice() { return empty_slice_; }

inline RawObject Runtime::emptyTuple() { return empty_tuple_; }

inline RawObject Runtime::internStr(Thread* thread, const Object& str) {
  if (str.isSmallStr()) {
    return *str;
  }
  return internLargeStr(thread, str);
}

}  // namespace py
