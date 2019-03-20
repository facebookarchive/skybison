#pragma once

#include <limits>

#include "globals.h"
#include "utils.h"
#include "view.h"

namespace python {

class Frame;
template <typename T>
class Handle;

// Python types that store their value directly in a RawObject.
#define INTRINSIC_IMMEDIATE_CLASS_NAMES(V)                                     \
  V(SmallInt)                                                                  \
  V(SmallStr)                                                                  \
  V(Bool)                                                                      \
  V(NoneType)

// Python types that hold a pointer to heap-allocated data in a RawObject.
#define INTRINSIC_HEAP_CLASS_NAMES(V)                                          \
  V(Object)                                                                    \
  V(BoundMethod)                                                               \
  V(Bytes)                                                                     \
  V(ByteArray)                                                                 \
  V(ClassMethod)                                                               \
  V(Code)                                                                      \
  V(Complex)                                                                   \
  V(Coroutine)                                                                 \
  V(Dict)                                                                      \
  V(DictItemIterator)                                                          \
  V(DictItems)                                                                 \
  V(DictKeyIterator)                                                           \
  V(DictKeys)                                                                  \
  V(DictValueIterator)                                                         \
  V(DictValues)                                                                \
  V(Ellipsis)                                                                  \
  V(ExceptionState)                                                            \
  V(Float)                                                                     \
  V(FrozenSet)                                                                 \
  V(Function)                                                                  \
  V(Generator)                                                                 \
  V(HeapFrame)                                                                 \
  V(Int)                                                                       \
  V(LargeInt)                                                                  \
  V(LargeStr)                                                                  \
  V(Layout)                                                                    \
  V(List)                                                                      \
  V(ListIterator)                                                              \
  V(Module)                                                                    \
  V(NotImplemented)                                                            \
  V(Property)                                                                  \
  V(Range)                                                                     \
  V(RangeIterator)                                                             \
  V(SeqIterator)                                                               \
  V(Set)                                                                       \
  V(SetIterator)                                                               \
  V(Slice)                                                                     \
  V(StaticMethod)                                                              \
  V(Str)                                                                       \
  V(StrIterator)                                                               \
  V(Super)                                                                     \
  V(Traceback)                                                                 \
  V(Tuple)                                                                     \
  V(TupleIterator)                                                             \
  V(UnboundValue)                                                              \
  V(Type)                                                                      \
  V(ValueCell)                                                                 \
  V(WeakRef)

// Heap-allocated Python types in the BaseException hierarchy.
#define INTRINSIC_EXCEPTION_CLASS_NAMES(V)                                     \
  V(ArithmeticError)                                                           \
  V(AssertionError)                                                            \
  V(AttributeError)                                                            \
  V(BaseException)                                                             \
  V(BlockingIOError)                                                           \
  V(BrokenPipeError)                                                           \
  V(BufferError)                                                               \
  V(BytesWarning)                                                              \
  V(ChildProcessError)                                                         \
  V(ConnectionAbortedError)                                                    \
  V(ConnectionError)                                                           \
  V(ConnectionRefusedError)                                                    \
  V(ConnectionResetError)                                                      \
  V(DeprecationWarning)                                                        \
  V(EOFError)                                                                  \
  V(Exception)                                                                 \
  V(FileExistsError)                                                           \
  V(FileNotFoundError)                                                         \
  V(FloatingPointError)                                                        \
  V(FutureWarning)                                                             \
  V(GeneratorExit)                                                             \
  V(ImportError)                                                               \
  V(ImportWarning)                                                             \
  V(IndentationError)                                                          \
  V(IndexError)                                                                \
  V(InterruptedError)                                                          \
  V(IsADirectoryError)                                                         \
  V(KeyboardInterrupt)                                                         \
  V(KeyError)                                                                  \
  V(LookupError)                                                               \
  V(MemoryError)                                                               \
  V(ModuleNotFoundError)                                                       \
  V(NameError)                                                                 \
  V(NotADirectoryError)                                                        \
  V(NotImplementedError)                                                       \
  V(OSError)                                                                   \
  V(OverflowError)                                                             \
  V(PendingDeprecationWarning)                                                 \
  V(PermissionError)                                                           \
  V(ProcessLookupError)                                                        \
  V(RecursionError)                                                            \
  V(ReferenceError)                                                            \
  V(ResourceWarning)                                                           \
  V(RuntimeError)                                                              \
  V(RuntimeWarning)                                                            \
  V(StopAsyncIteration)                                                        \
  V(StopIteration)                                                             \
  V(SyntaxError)                                                               \
  V(SyntaxWarning)                                                             \
  V(SystemError)                                                               \
  V(SystemExit)                                                                \
  V(TabError)                                                                  \
  V(TimeoutError)                                                              \
  V(TypeError)                                                                 \
  V(UnboundLocalError)                                                         \
  V(UnicodeDecodeError)                                                        \
  V(UnicodeEncodeError)                                                        \
  V(UnicodeError)                                                              \
  V(UnicodeTranslateError)                                                     \
  V(UnicodeWarning)                                                            \
  V(UserWarning)                                                               \
  V(ValueError)                                                                \
  V(Warning)                                                                   \
  V(ZeroDivisionError)

#define INTRINSIC_CLASS_NAMES(V)                                               \
  INTRINSIC_IMMEDIATE_CLASS_NAMES(V)                                           \
  INTRINSIC_HEAP_CLASS_NAMES(V)                                                \
  INTRINSIC_EXCEPTION_CLASS_NAMES(V)

// This enumerates layout ids of intrinsic classes. Notably, the layout of an
// instance of an intrinsic class does not change.
//
// An instance of an intrinsic class that has an immediate representation
// cannot have attributes added.  An instance of an intrinsic class that is heap
// allocated has a predefined number in-object attributes in the base
// instance.  For some of those types, the language forbids adding new
// attributes.  For the types which are permitted to have attributes added,
// these types must include a hidden attribute that indirects to attribute
// storage.
//
// NB: If you add something here make sure you add it to the appropriate macro
// above
enum class LayoutId : word {
  // Immediate objects - note that the SmallInt class is also aliased to all
  // even integers less than 32, so that classes of immediate objects can be
  // looked up simply by using the low 5 bits of the immediate value. This
  // implies that all other immediate class ids must be odd.
  kSmallInt = 0,
  kBool = 7,
  kNoneType = 15,
  // There is no RawType associated with the RawError object type, this is here
  // as a placeholder.
  kError = 23,
  kSmallStr = 31,

// clang-format off
  // Heap objects
#define LAYOUT_ID(name) k##name,
  INTRINSIC_HEAP_CLASS_NAMES(LAYOUT_ID)
  INTRINSIC_EXCEPTION_CLASS_NAMES(LAYOUT_ID)
#undef LAYOUT_ID

  // Mark the first and last Exception LayoutIds, to allow range comparisons.
#define GET_FIRST(name) k##name + 0 *
  kFirstException = INTRINSIC_EXCEPTION_CLASS_NAMES(GET_FIRST) 0,
#undef GET_FIRST
#define GET_LAST(name) 0 + k##name *
  kLastException = INTRINSIC_EXCEPTION_CLASS_NAMES(GET_LAST) 1,
#undef GET_LAST
  // clang-format on

  kLastBuiltinId = kLastException,
  kSentinelId = kLastBuiltinId + 1,
};

// Map from type to its corresponding LayoutId:
// ObjectLayoutId<RawSmallInt>::value == LayoutId::kSmallInt, etc.
template <typename T>
struct ObjectLayoutId;
#define CASE(ty)                                                               \
  template <>                                                                  \
  struct ObjectLayoutId<class Raw##ty> {                                       \
    static constexpr LayoutId value = LayoutId::k##ty;                         \
  };
INTRINSIC_CLASS_NAMES(CASE)
#undef CASE

// Add functionality common to all RawObject subclasses, split into two parts
// since some types manually define cast() but want everything else.
#define RAW_OBJECT_COMMON_NO_CAST(ty)                                          \
  /* TODO(T34683229): Once Handle<T> doesn't inherit from T, delete this.      \
   * Right now it exists to prevent implicit conversion of Handle<T> to T. */  \
  template <typename T>                                                        \
  Raw##ty(const Handle<T>&) = delete;                                          \
  DISALLOW_HEAP_ALLOCATION()

#define RAW_OBJECT_COMMON(ty)                                                  \
  static Raw##ty cast(RawObject object) {                                      \
    DCHECK(object.is##ty(), "invalid cast, expected " #ty);                    \
    return object.rawCast<Raw##ty>();                                          \
  }                                                                            \
  RAW_OBJECT_COMMON_NO_CAST(ty)

class RawObject {
 public:
  // TODO(bsimmers): Delete this. The default constructor gives you a
  // zero-initialized RawObject, which is equivalent to
  // RawSmallInt::fromWord(0). This behavior can be confusing and surprising, so
  // we should just require all Objects to be explicitly initialized.
  RawObject() = default;

  explicit RawObject(uword raw);

  // Getters and setters.
  uword raw() const;
  bool isObject() const;
  LayoutId layoutId() const;

  // Immediate objects
  bool isBool() const;
  bool isError() const;
  bool isHeader() const;
  bool isNoneType() const;
  bool isSmallInt() const;
  bool isSmallStr() const;

  // Heap objects
  bool isBaseException() const;
  bool isBoundMethod() const;
  bool isByteArray() const;
  bool isBytes() const;
  bool isType() const;
  bool isClassMethod() const;
  bool isCode() const;
  bool isComplex() const;
  bool isCoroutine() const;
  bool isDict() const;
  bool isDictItemIterator() const;
  bool isDictItems() const;
  bool isDictKeyIterator() const;
  bool isDictKeys() const;
  bool isDictValueIterator() const;
  bool isDictValues() const;
  bool isEllipsis() const;
  bool isException() const;
  bool isExceptionState() const;
  bool isFloat() const;
  bool isHeapFrame() const;
  bool isFrozenSet() const;
  bool isFunction() const;
  bool isGenerator() const;
  bool isHeapObject() const;
  bool isHeapObjectWithLayout(LayoutId layout_id) const;
  bool isImportError() const;
  bool isIndexError() const;
  bool isInstance() const;
  bool isKeyError() const;
  bool isLargeInt() const;
  bool isLargeStr() const;
  bool isLayout() const;
  bool isList() const;
  bool isListIterator() const;
  bool isLookupError() const;
  bool isModule() const;
  bool isModuleNotFoundError() const;
  bool isNotImplemented() const;
  bool isNotImplementedError() const;
  bool isProperty() const;
  bool isRange() const;
  bool isRangeIterator() const;
  bool isGeneratorBase() const;
  bool isRuntimeError() const;
  bool isSeqIterator() const;
  bool isSet() const;
  bool isSetIterator() const;
  bool isSlice() const;
  bool isStaticMethod() const;
  bool isStopIteration() const;
  bool isStrIterator() const;
  bool isSuper() const;
  bool isSystemExit() const;
  bool isTraceback() const;
  bool isTuple() const;
  bool isTupleIterator() const;
  bool isUnboundValue() const;
  bool isUnicodeDecodeError() const;
  bool isUnicodeEncodeError() const;
  bool isUnicodeError() const;
  bool isUnicodeTranslateError() const;
  bool isValueCell() const;
  bool isWeakRef() const;

  // superclass objects
  bool isInt() const;
  bool isSetBase() const;
  bool isStr() const;

  static bool equals(RawObject lhs, RawObject rhs);

  bool operator==(const RawObject& other) const;
  bool operator!=(const RawObject& other) const;

  // Constants

  // The bottom five bits of immediate objects are used as the class id when
  // indexing into the class table in the runtime.
  static const uword kImmediateTypeTableIndexMask = (1 << 5) - 1;

  RAW_OBJECT_COMMON(Object);

  // Cast this RawObject to another Raw* type with no runtime checks. Only used
  // in a few limited situations; most code should use Raw*::cast() instead.
  template <typename T>
  T rawCast() const;

 private:
  // Zero-initializing raw_ gives RawSmallInt::fromWord(0).
  uword raw_{};
};

// CastError and OptInt<T> represent the result of a call to RawInt::asInt<T>():
// If error == CastError::None, value contains the result. Otherwise, error
// indicates why the value didn't fit in T.
enum class CastError {
  None,
  Underflow,
  Overflow,
};

template <typename T>
class OptInt {
 public:
  static OptInt valid(T i) { return {i, CastError::None}; }
  static OptInt underflow() { return {0, CastError::Underflow}; }
  static OptInt overflow() { return {0, CastError::Overflow}; }

  T value;
  CastError error;
};

// Generic wrapper around RawSmallInt/RawLargeInt/RawBool.
class RawInt : public RawObject {
 public:
  // Getters and setters.
  word asWord() const;
  // Returns the value as a word if it fits into a word.
  // Otherwise, returns kMinWord for negative values or kMaxWord for positive
  // values.
  word asWordSaturated() const;
  void* asCPtr() const;

  // If this fits in T, get its value as a T. If not, indicate what went wrong.
  template <typename T>
  OptInt<T> asInt() const;

  // Returns a positive value if 'this' is greater than 'other', zero if it
  // is the same, a negavite value if smaller. The value does not have to be
  // -1, 0, or 1.
  word compare(RawInt other) const;

  word bitLength() const;

  bool isNegative() const;
  bool isPositive() const;
  bool isZero() const;

  RAW_OBJECT_COMMON(Int);

  // Indexing into digits
  uword digitAt(word index) const;

  // Number of digits
  word numDigits() const;

  // Copies digits bytewise to `dst`. Returns number of bytes copied.
  word copyTo(byte* dst, word max_length) const;
};

// Immediate objects

class RawSmallInt : public RawObject {
 public:
  // Getters and setters.
  word value() const;
  void* asCPtr() const;

  // If this fits in T, get its value as a T. If not, indicate what went wrong.
  template <typename T>
  if_signed_t<T, OptInt<T>> asInt() const;
  template <typename T>
  if_unsigned_t<T, OptInt<T>> asInt() const;

  // Conversion.
  static RawSmallInt fromWord(word value);
  static RawSmallInt fromWordTruncated(word value);
  static constexpr bool isValid(word value) {
    return (value >= kMinValue) && (value <= kMaxValue);
  }

  template <typename T>
  static RawSmallInt fromFunctionPointer(T pointer);

  // Tags.
  static const int kTag = 0;
  static const int kTagSize = 1;
  static const uword kTagMask = (1 << kTagSize) - 1;

  // Constants
  static const word kMinValue = -(1L << (kBitsPerPointer - (kTagSize + 1)));
  static const word kMaxValue = (1L << (kBitsPerPointer - (kTagSize + 1))) - 1;

  RAW_OBJECT_COMMON(SmallInt);
};

enum class ObjectFormat {
  // Arrays that do not contain objects, one per element width
  kDataArray8 = 0,
  kDataArray16 = 1,
  kDataArray32 = 2,
  kDataArray64 = 3,
  kDataArray128 = 4,
  // Arrays that contain objects
  kObjectArray = 5,
  // Instances that do not contain objects
  kDataInstance = 6,
  // Instances that contain objects
  kObjectInstance = 7,
};

/**
 * RawHeader objects
 *
 * Headers are located in first logical word of a heap allocated object and
 * contain metadata related to the object its part of.  A header is not
 * really object that the user will interact with directly.  Nevertheless, we
 * tag them as immediate object.  This allows the runtime to identify the start
 * of an object when scanning the heap.
 *
 * Headers encode the following information
 *
 * Name      Size  Description
 * ----------------------------------------------------------------------------
 * Tag          3   tag for a header object
 * Format       3   enumeration describing the object encoding
 * LayoutId    20   identifier for the layout, allowing 2^20 unique layouts
 * Hash        30   bits to use for an identity hash code
 * Count        8   number of array elements or instance variables
 */
class RawHeader : public RawObject {
 public:
  word hashCode() const;
  RawHeader withHashCode(word value) const;

  ObjectFormat format() const;

  LayoutId layoutId() const;
  RawHeader withLayoutId(LayoutId layout_id) const;

  word count() const;

  bool hasOverflow() const;

  static RawHeader from(word count, word hash, LayoutId layout_id,
                        ObjectFormat format);

  // Tags.
  static const int kTag = 3;
  static const int kTagSize = 3;
  static const uword kTagMask = (1 << kTagSize) - 1;

  static const int kFormatSize = 3;
  static const int kFormatOffset = 3;
  static const uword kFormatMask = (1 << kFormatSize) - 1;

  static const int kLayoutIdSize = 20;
  static const int kLayoutIdOffset = 6;
  static const uword kLayoutIdMask = (1 << kLayoutIdSize) - 1;

  static const int kHashCodeOffset = 26;
  static const int kHashCodeSize = 30;
  static const uword kHashCodeMask = (1 << kHashCodeSize) - 1U;

  static const int kCountOffset = 56;
  static const int kCountSize = 8;
  static const uword kCountMask = (1 << kCountSize) - 1;

  static const int kCountOverflowFlag = (1 << kCountSize) - 1;
  static const int kCountMax = kCountOverflowFlag - 1;

  static const int kSize = kPointerSize;

  // Constants
  static const word kMaxLayoutId = (1L << (kLayoutIdSize + 1)) - 1;

  RAW_OBJECT_COMMON(Header);
};

class RawBool : public RawObject {
 public:
  // Getters and setters.
  bool value() const;

  // Singletons
  static RawBool trueObj();
  static RawBool falseObj();

  // Conversion.
  static RawBool fromBool(bool value);
  static RawBool negate(RawObject value);

  // Tags.
  static const int kTag = 7;  // 0b00111
  static const int kTagSize = 5;
  static const uword kTagMask = (1 << kTagSize) - 1;

  RAW_OBJECT_COMMON(Bool);
};

class RawNoneType : public RawObject {
 public:
  // Singletons.
  static RawNoneType object();

  // Tags.
  static const int kTag = 15;  // 0b01111
  static const int kTagSize = 5;
  static const uword kTagMask = (1 << kTagSize) - 1;

  RAW_OBJECT_COMMON(NoneType);
};

// RawError is a special object type, internal to the runtime. It is used to
// signal that an error has occurred inside the runtime or native code, e.g. an
// exception has been thrown.
class RawError : public RawObject {
 public:
  // Singletons.
  static RawError object();

  // Tagging.
  static const int kTag = 23;  // 0b10111
  static const int kTagSize = 5;
  static const uword kTagMask = (1 << kTagSize) - 1;

  RAW_OBJECT_COMMON(Error);
};

// Super class of common string functionality
class RawStr : public RawObject {
 public:
  // Singletons.
  static RawStr empty();

  // Getters and setters.
  byte charAt(word index) const;
  word length() const;
  void copyTo(byte* dst, word length) const;

  // Equality checks.
  word compare(RawObject string) const;
  word compareCStr(const char* c_str) const;
  bool equals(RawObject that) const;
  bool equalsCStr(const char* c_str) const;

  // Codepoints
  int32 codePointAt(word index, word* length) const;
  word codePointLength() const;

  // Counts forward through the code points of the string, starting at the
  // specified code unit index. Returns the code unit index at the offset,
  // or length() if the offset reaches the end of the string.
  word offsetByCodePoints(word index, word count) const;

  // Conversion to an unescaped C string.  The underlying memory is allocated
  // with malloc and must be freed by the caller.
  char* toCStr() const;

  RAW_OBJECT_COMMON(Str);
};

class RawSmallStr : public RawObject {
 public:
  // Conversion.
  static RawObject fromCodePoint(int32 code_point);
  static RawObject fromCStr(const char* value);
  static RawObject fromBytes(View<byte> data);

  // Tagging.
  static const int kTag = 31;  // 0b11111
  static const int kTagSize = 5;
  static const uword kTagMask = (1 << kTagSize) - 1;

  static const word kMaxLength = kWordSize - 1;

  RAW_OBJECT_COMMON(SmallStr);

 private:
  // Interface methods are private: strings should be manipulated via the
  // RawStr class, which delegates to RawLargeStr/RawSmallStr appropriately.

  // Getters and setters.
  word length() const;
  byte charAt(word index) const;
  void copyTo(byte* dst, word length) const;

  // Codepoints
  word codePointLength() const;

  // Conversion to an unescaped C string.  The underlying memory is allocated
  // with malloc and must be freed by the caller.
  char* toCStr() const;

  friend class Heap;
  friend class RawObject;
  friend class RawStr;
  friend class Runtime;
};

// Heap objects

class RawHeapObject : public RawObject {
 public:
  // Getters and setters.
  uword address() const;
  uword baseAddress() const;
  RawHeader header() const;
  void setHeader(RawHeader header) const;
  word headerOverflow() const;
  void setHeaderAndOverflow(word count, word hash, LayoutId id,
                            ObjectFormat format) const;
  word headerCountOrOverflow() const;
  word size() const;

  // Conversion.
  static RawHeapObject fromAddress(uword address);

  // Sizing
  static word headerSize(word count);

  // Garbage collection.
  bool isRoot() const;
  bool isForwarding() const;
  RawObject forward() const;
  void forwardTo(RawObject object) const;

  // Tags.
  static const int kTag = 1;
  static const int kTagSize = 2;
  static const uword kTagMask = (1 << kTagSize) - 1;

  static const uword kIsForwarded = static_cast<uword>(-3);

  // Layout.
  static const int kHeaderOffset = -kPointerSize;
  static const int kHeaderOverflowOffset = kHeaderOffset - kPointerSize;
  static const int kSize = kHeaderOffset + kPointerSize;

  static const word kMinimumSize = kPointerSize * 2;

  RAW_OBJECT_COMMON(HeapObject);

 protected:
  RawObject instanceVariableAt(word offset) const;
  void instanceVariableAtPut(word offset, RawObject value) const;

 private:
  // Instance initialization should only done by the Heap.
  void initialize(word size, RawObject value) const;

  friend class Heap;
  friend class Runtime;
};

class RawBaseException : public RawHeapObject {
 public:
  // Getters and setters.
  RawObject args() const;
  void setArgs(RawObject args) const;

  // traceback, cause, and context can all be UnboundValue to indicate that they
  // are "NULL" rather than the normal unset value of None. The only code that
  // cares about the distinction is a handful of C-API functions, so the
  // standard getters transparently replace UnboundValue with None. The
  // *OrUnbound getters return the value as it's stored in memory, and are used
  // in the few C-API functions that care about the distinction.
  RawObject traceback() const;
  RawObject tracebackOrUnbound() const;
  void setTraceback(RawObject traceback) const;

  RawObject cause() const;
  RawObject causeOrUnbound() const;
  void setCause(RawObject cause) const;

  RawObject context() const;
  RawObject contextOrUnbound() const;
  void setContext(RawObject context) const;

  static const int kArgsOffset = RawHeapObject::kSize;
  static const int kTracebackOffset = kArgsOffset + kPointerSize;
  static const int kCauseOffset = kTracebackOffset + kPointerSize;
  static const int kContextOffset = kCauseOffset + kPointerSize;
  static const int kSize = kContextOffset + kPointerSize;

  RAW_OBJECT_COMMON(BaseException);
};

class RawException : public RawBaseException {
 public:
  RAW_OBJECT_COMMON(Exception);
};

class RawStopIteration : public RawBaseException {
 public:
  // Getters and setters.
  RawObject value() const;
  void setValue(RawObject value) const;

  static const int kValueOffset = RawBaseException::kSize;
  static const int kSize = kValueOffset + kPointerSize;

  RAW_OBJECT_COMMON(StopIteration);
};

class RawSystemExit : public RawBaseException {
 public:
  RawObject code() const;
  void setCode(RawObject code) const;

  static const int kCodeOffset = RawBaseException::kSize;
  static const int kSize = kCodeOffset + kPointerSize;

  RAW_OBJECT_COMMON(SystemExit);
};

class RawRuntimeError : public RawException {
 public:
  RAW_OBJECT_COMMON(RuntimeError);
};

class RawNotImplementedError : public RawRuntimeError {
 public:
  RAW_OBJECT_COMMON(NotImplementedError);
};

class RawImportError : public RawException {
 public:
  // Getters and setters
  RawObject msg() const;
  void setMsg(RawObject msg) const;

  RawObject name() const;
  void setName(RawObject name) const;

  RawObject path() const;
  void setPath(RawObject path) const;

  static const int kMsgOffset = RawBaseException::kSize;
  static const int kNameOffset = kMsgOffset + kPointerSize;
  static const int kPathOffset = kNameOffset + kPointerSize;
  static const int kSize = kPathOffset + kPointerSize;

  RAW_OBJECT_COMMON(ImportError);
};

class RawModuleNotFoundError : public RawImportError {
 public:
  RAW_OBJECT_COMMON(ModuleNotFoundError);
};

class RawLookupError : public RawException {
 public:
  RAW_OBJECT_COMMON(LookupError);
};

class RawIndexError : public RawLookupError {
 public:
  RAW_OBJECT_COMMON(IndexError);
};

class RawKeyError : public RawLookupError {
 public:
  RAW_OBJECT_COMMON(KeyError);
};

class RawUnicodeError : public RawException {
 public:
  RawObject encoding() const;
  void setEncoding(RawObject encoding) const;

  RawObject object() const;
  void setObject(RawObject object) const;

  RawObject start() const;
  void setStart(RawObject object) const;

  RawObject end() const;
  void setEnd(RawObject end) const;

  RawObject reason() const;
  void setReason(RawObject reason) const;

  static const int kEncodingOffset = RawBaseException::kSize;
  static const int kObjectOffset = kEncodingOffset + kPointerSize;
  static const int kStartOffset = kObjectOffset + kPointerSize;
  static const int kEndOffset = kStartOffset + kPointerSize;
  static const int kReasonOffset = kEndOffset + kPointerSize;
  static const int kSize = kReasonOffset + kPointerSize;

  RAW_OBJECT_COMMON(UnicodeError);
};

class RawUnicodeDecodeError : public RawUnicodeError {
 public:
  RAW_OBJECT_COMMON(UnicodeDecodeError);
};

class RawUnicodeEncodeError : public RawUnicodeError {
 public:
  RAW_OBJECT_COMMON(UnicodeEncodeError);
};

class RawUnicodeTranslateError : public RawUnicodeError {
 public:
  RAW_OBJECT_COMMON(UnicodeTranslateError);
};

class RawType : public RawHeapObject {
 public:
  enum Flag : word {
    // If you add a flag, keep in mind that bits 0-7 are reserved to hold a
    // LayoutId.
  };

  enum class ExtensionSlot {
    kFlags,
    kBasicSize,
    kItemSize,
    kMapAssSubscript,
    kMapLength,
    kMapSubscript,
    kNumberAbsolute,
    kNumberAdd,
    kNumberAnd,
    kNumberBool,
    kNumberDivmod,
    kNumberFloat,
    kNumberFloorDivide,
    kNumberIndex,
    kNumberInplaceAdd,
    kNumberInplaceAnd,
    kNumberInplaceFloorDivide,
    kNumberInplaceLshift,
    kNumberInplaceMultiply,
    kNumberInplaceOr,
    kNumberInplacePower,
    kNumberInplaceRemainder,
    kNumberInplaceRshift,
    kNumberInplaceSubtract,
    kNumberInplaceTrueDivide,
    kNumberInplaceXor,
    kNumberInt,
    kNumberInvert,
    kNumberLshift,
    kNumberMultiply,
    kNumberNegative,
    kNumberOr,
    kNumberPositive,
    kNumberPower,
    kNumberRemainder,
    kNumberRshift,
    kNumberSubtract,
    kNumberTrueDivide,
    kNumberXor,
    kSequenceAssItem,
    kSequenceConcat,
    kSequenceContains,
    kSequenceInplaceConcat,
    kSequenceInplaceRepeat,
    kSequenceItem,
    kSequenceLength,
    kSequenceRepeat,
    kAlloc,
    kBase,
    kBases,
    kCall,
    kClear,
    kDealloc,
    kDel,
    kDescrGet,
    kDescrSet,
    kDoc,
    kGetattr,
    kGetattro,
    kHash,
    kInit,
    kIsGc,
    kIter,
    kIternext,
    kMethods,
    kNew,
    kRepr,
    kRichcompare,
    kSetattr,
    kSetattro,
    kStr,
    kTraverse,
    kMembers,
    kGetset,
    kFree,
    kNumberMatrixMultiply,
    kNumberInplaceMatrixMultiply,
    kAsyncAwait,
    kAsyncAiter,
    kAsyncAnext,
    kFinalize,
    kEnd,
  };

  // Getters and setters.
  RawObject instanceLayout() const;
  void setInstanceLayout(RawObject layout) const;

  RawObject mro() const;
  void setMro(RawObject object_array) const;

  RawObject name() const;
  void setName(RawObject name) const;

  // Flags.
  //
  // Bits 0-7 contain the LayoutId of the builtin base type. For builtin types,
  // this is the type itself, except for subtypes of int and str, which have
  // kInt and kStr, respectively. For user-defined types, it is the LayoutId of
  // the first builtin base class (kObject for most types).
  //
  // Bits 8+ are a bitmask of flags describing certain properties of the type.
  Flag flags() const;
  bool hasFlag(Flag bit) const;
  LayoutId builtinBase() const;
  void setFlagsAndBuiltinBase(Flag value, LayoutId base) const;
  void setBuiltinBase(LayoutId base) const;

  RawObject dict() const;
  void setDict(RawObject name) const;

  bool isBuiltin() const;

  RawObject extensionSlots() const;
  void setExtensionSlots(RawObject slots) const;

  bool isBaseExceptionSubclass() const;

  // Seal the attributes of the type. Sets the layout's overflowAttributes to
  // RawNoneType::object().
  void sealAttributes() const;

  // Layout.
  static const int kMroOffset = RawHeapObject::kSize;
  static const int kInstanceLayoutOffset = kMroOffset + kPointerSize;
  static const int kNameOffset = kInstanceLayoutOffset + kPointerSize;
  static const int kFlagsOffset = kNameOffset + kPointerSize;
  static const int kDictOffset = kFlagsOffset + kPointerSize;
  static const int kExtensionSlotsOffset = kDictOffset + kPointerSize;
  static const int kSize = kExtensionSlotsOffset + kPointerSize;

  static const int kBuiltinBaseMask = 0xff;

  RAW_OBJECT_COMMON(Type);

 private:
  void setFlags(Flag value) const;
};

class RawArray : public RawHeapObject {
 public:
  word length() const;

  RAW_OBJECT_COMMON_NO_CAST(Array);
};

class RawBytes : public RawArray {
 public:
  // Getters and setters.
  byte byteAt(word index) const;
  void byteAtPut(word index, byte value) const;
  void copyTo(byte* dst, word length) const;
  word replaceFromWith(word start, RawObject src, word length) const;

  // Returns a positive value if 'this' is greater that 'other', a negative
  // value if 'this' is less that 'other', and zero if they are the same.
  // Does not guarantee to return -1, 0, or 1.
  word compare(RawBytes other) const;

  // Sizing.
  static word allocationSize(word length);

  RAW_OBJECT_COMMON(Bytes);
};

class RawTuple : public RawArray {
 public:
  // Getters and setters.
  RawObject at(word index) const;
  void atPut(word index, RawObject value) const;

  // Sizing.
  static word allocationSize(word length);

  void copyTo(RawObject dst) const;

  void replaceFromWith(word start, RawObject array) const;

  bool contains(RawObject object) const;

  RAW_OBJECT_COMMON(Tuple);
};

class RawUserTupleBase : public RawHeapObject {
 public:
  // Getters and setters.
  RawObject tupleValue() const;
  void setTupleValue(RawObject value) const;

  // RawLayout.
  static const int kTupleOffset = RawHeapObject::kSize;
  static const int kSize = kTupleOffset + kPointerSize;

  RAW_OBJECT_COMMON_NO_CAST(UserTupleBase);
};

class RawLargeStr : public RawArray {
 public:
  // Sizing.
  static word allocationSize(word length);

  static const int kDataOffset = RawHeapObject::kSize;

  RAW_OBJECT_COMMON(LargeStr);

 private:
  // Interface methods are private: strings should be manipulated via the
  // RawStr class, which delegates to RawLargeStr/RawSmallStr appropriately.

  // Getters and setters.
  byte charAt(word index) const;
  void copyTo(byte* bytes, word length) const;

  // Equality checks.
  bool equals(RawObject that) const;

  // Codepoints
  word codePointLength() const;

  // Conversion to an unescaped C string.  The underlying memory is allocated
  // with malloc and must be freed by the caller.
  char* toCStr() const;

  friend class Heap;
  friend class RawObject;
  friend class RawStr;
  friend class Runtime;
};

// Arbitrary precision signed integer, with 64 bit digits in two's complement
// representation
class RawLargeInt : public RawHeapObject {
 public:
  // Getters and setters.
  word asWord() const;

  // Return whether or not this RawLargeInt obeys the following invariants:
  // - numDigits() >= 1
  // - The value does not fit in a RawSmallInt
  // - Negative numbers do not have redundant sign-extended digits
  // - Positive numbers do not have redundant zero-extended digits
  bool isValid() const;

  // RawLargeInt is also used for storing native pointers.
  void* asCPtr() const;

  // If this fits in T, get its value as a T. If not, indicate what went wrong.
  template <typename T>
  if_signed_t<T, OptInt<T>> asInt() const;
  template <typename T>
  if_unsigned_t<T, OptInt<T>> asInt() const;

  // Sizing.
  static word allocationSize(word num_digits);

  // Indexing into digits
  uword digitAt(word index) const;
  void digitAtPut(word index, uword digit) const;

  bool isNegative() const;
  bool isPositive() const;

  word bitLength() const;

  // Number of digits
  word numDigits() const;

  // Copies digits bytewise to `dst`. Returns number of bytes copied.
  word copyTo(byte* dst, word max_length) const;

  // Copy 'bytes' array into digits; if the array is too small set remaining
  // data to 'sign_extension' byte.
  void copyFrom(View<byte> bytes, byte sign_extension) const;

  // Layout.
  static const int kValueOffset = RawHeapObject::kSize;
  static const int kSize = kValueOffset + kPointerSize;

  RAW_OBJECT_COMMON(LargeInt);

 private:
  friend class RawInt;
  friend class Runtime;
};

class RawFloat : public RawHeapObject {
 public:
  // Getters and setters.
  double value() const;

  // Layout.
  static const int kValueOffset = RawHeapObject::kSize;
  static const int kSize = kValueOffset + kDoubleSize;

  RAW_OBJECT_COMMON(Float);

 private:
  // Instance initialization should only done by the Heap.
  void initialize(double value) const;

  friend class Heap;
};

class RawUserFloatBase : public RawHeapObject {
 public:
  // Getters and setters.
  RawObject floatValue() const;
  void setFloatValue(RawObject value) const;

  // RawLayout.
  static const int kFloatOffset = RawHeapObject::kSize;
  static const int kSize = kFloatOffset + kPointerSize;

  RAW_OBJECT_COMMON_NO_CAST(UserFloatBase);
};

class RawComplex : public RawHeapObject {
 public:
  // Getters
  double real() const;
  double imag() const;

  // Layout.
  static const int kRealOffset = RawHeapObject::kSize;
  static const int kImagOffset = kRealOffset + kDoubleSize;
  static const int kSize = kImagOffset + kDoubleSize;

  RAW_OBJECT_COMMON(Complex);

 private:
  // Instance initialization should only done by the Heap.
  void initialize(double real, double imag) const;

  friend class Heap;
};

class RawProperty : public RawHeapObject {
 public:
  // Getters and setters
  RawObject getter() const;
  void setGetter(RawObject function) const;

  RawObject setter() const;
  void setSetter(RawObject function) const;

  RawObject deleter() const;
  void setDeleter(RawObject function) const;

  // Layout.
  static const int kGetterOffset = RawHeapObject::kSize;
  static const int kSetterOffset = kGetterOffset + kPointerSize;
  static const int kDeleterOffset = kSetterOffset + kPointerSize;
  static const int kSize = kDeleterOffset + kPointerSize;

  RAW_OBJECT_COMMON(Property);
};

class RawRange : public RawHeapObject {
 public:
  // Getters and setters.
  word start() const;
  void setStart(word value) const;

  word stop() const;
  void setStop(word value) const;

  word step() const;
  void setStep(word value) const;

  // Layout.
  static const int kStartOffset = RawHeapObject::kSize;
  static const int kStopOffset = kStartOffset + kPointerSize;
  static const int kStepOffset = kStopOffset + kPointerSize;
  static const int kSize = kStepOffset + kPointerSize;

  RAW_OBJECT_COMMON(Range);
};

class RawRangeIterator : public RawHeapObject {
 public:
  // Getters and setters.

  // Bind the iterator to a specific range. The binding should not be changed
  // after creation.
  void setRange(RawObject range) const;

  // Iteration.
  RawObject next() const;

  // Number of unconsumed values in the range iterator
  word pendingLength() const;

  // Layout.
  static const int kRangeOffset = RawHeapObject::kSize;
  static const int kCurValueOffset = kRangeOffset + kPointerSize;
  static const int kSize = kCurValueOffset + kPointerSize;

  RAW_OBJECT_COMMON(RangeIterator);

 private:
  static bool isOutOfRange(word cur, word stop, word step);
};

class RawSlice : public RawHeapObject {
 public:
  // Getters and setters.
  RawObject start() const;
  void setStart(RawObject value) const;

  RawObject stop() const;
  void setStop(RawObject value) const;

  RawObject step() const;
  void setStep(RawObject value) const;

  // Returns the correct start, stop, and step word values from this slice
  void unpack(word* start, word* stop, word* step) const;

  // Takes in the length of a list and the start, stop, and step values
  // Returns the length of the new list and the corrected start and stop values
  static word adjustIndices(word length, word* start, word* stop, word step);

  // Layout.
  static const int kStartOffset = RawHeapObject::kSize;
  static const int kStopOffset = kStartOffset + kPointerSize;
  static const int kStepOffset = kStopOffset + kPointerSize;
  static const int kSize = kStepOffset + kPointerSize;

  RAW_OBJECT_COMMON(Slice);
};

class RawStaticMethod : public RawHeapObject {
 public:
  // Getters and setters
  RawObject function() const;
  void setFunction(RawObject function) const;

  // Layout.
  static const int kFunctionOffset = RawHeapObject::kSize;
  static const int kSize = kFunctionOffset + kPointerSize;

  RAW_OBJECT_COMMON(StaticMethod);
};

class RawIteratorBase : public RawHeapObject {
 public:
  // Getters and setters.
  word index() const;
  void setIndex(word index) const;

  RawObject iterable() const;
  void setIterable(RawObject iterable) const;

  // Layout.
  static const int kIterableOffset = RawHeapObject::kSize;
  static const int kIndexOffset = kIterableOffset + kPointerSize;
  static const int kSize = kIndexOffset + kPointerSize;
};

class RawSeqIterator : public RawIteratorBase {
 public:
  RAW_OBJECT_COMMON(SeqIterator);
};

class RawStrIterator : public RawIteratorBase {
 public:
  RAW_OBJECT_COMMON(StrIterator);
};

class RawListIterator : public RawIteratorBase {
 public:
  RAW_OBJECT_COMMON(ListIterator);
};

class RawSetIterator : public RawIteratorBase {
 public:
  // Getters and setters
  word consumedCount() const;
  void setConsumedCount(word consumed) const;

  // Layout.
  static const int kConsumedCountOffset = RawIteratorBase::kSize;
  static const int kSize = kConsumedCountOffset + kPointerSize;

  RAW_OBJECT_COMMON(SetIterator);
};

class RawTupleIterator : public RawIteratorBase {
 public:
  // Getters and setters.
  word tupleLength() const;
  void setTupleLength(word length) const;

  // Layout.
  static const int kTupleLengthOffset = RawIteratorBase::kSize;
  static const int kSize = kTupleLengthOffset + kPointerSize;

  RAW_OBJECT_COMMON(TupleIterator);
};

class RawUnboundValue : public RawHeapObject {
 public:
  // Layout.
  // kPaddingOffset is not used, but the GC expects the object to be
  // at least one word.
  static const int kPaddingOffset = RawHeapObject::kSize;
  static const int kSize = kPaddingOffset + kPointerSize;

  RAW_OBJECT_COMMON(UnboundValue);
};

class RawCode : public RawHeapObject {
 public:
  // Matching CPython
  enum Flags {
    OPTIMIZED = 0x0001,
    NEWLOCALS = 0x0002,
    VARARGS = 0x0004,
    VARKEYARGS = 0x0008,
    NESTED = 0x0010,
    GENERATOR = 0x0020,
    NOFREE = 0x0040,  // Shortcut for no free or cell vars
    COROUTINE = 0x0080,
    ITERABLE_COROUTINE = 0x0100,
    ASYNC_GENERATOR = 0x0200,
    SIMPLE_CALL = 0x0400,  // Pyro addition; speeds detection of fast call
  };

  // Getters and setters.
  word argcount() const;
  void setArgcount(word value) const;
  word totalArgs() const;

  RawObject cell2arg() const;
  void setCell2arg(RawObject args) const;

  RawObject cellvars() const;
  void setCellvars(RawObject value) const;
  word numCellvars() const;

  RawObject code() const;
  void setCode(RawObject value) const;

  RawObject consts() const;
  void setConsts(RawObject value) const;

  RawObject filename() const;
  void setFilename(RawObject value) const;

  word firstlineno() const;
  void setFirstlineno(word value) const;

  word flags() const;
  void setFlags(word value) const;

  RawObject freevars() const;
  void setFreevars(RawObject value) const;
  word numFreevars() const;

  word kwonlyargcount() const;
  void setKwonlyargcount(word value) const;

  RawObject lnotab() const;
  void setLnotab(RawObject value) const;

  RawObject name() const;
  void setName(RawObject value) const;

  RawObject names() const;
  void setNames(RawObject value) const;

  word nlocals() const;
  void setNlocals(word value) const;

  // The total number of variables in this code object: normal locals, cell
  // variables, and free variables.
  word totalVars() const;

  word stacksize() const;
  void setStacksize(word value) const;

  RawObject varnames() const;
  void setVarnames(RawObject value) const;

  // Returns true if the code is for a coroutine.
  bool hasCoroutine() const;

  // Returns true if the code is for a generator function.
  bool hasGenerator() const;

  // Returns true if the code is for an iterable coroutine.
  bool hasIterableCoroutine() const;

  // Returns true if the code is for a coroutine or a generator function.
  bool hasCoroutineOrGenerator() const;

  // Returns true if the code has free variables or cell variables.
  bool hasFreevarsOrCellvars() const;

  // Returns true if the code has varargs.
  bool hasVarargs() const;

  // Returns true if the code has varkeyword arguments.
  bool hasVarkeyargs() const;

  // Returns true if the code has varargs or varkeyword arguments.
  bool hasVarargsOrVarkeyargs() const;

  // Layout.
  static const int kArgcountOffset = RawHeapObject::kSize;
  static const int kKwonlyargcountOffset = kArgcountOffset + kPointerSize;
  static const int kNlocalsOffset = kKwonlyargcountOffset + kPointerSize;
  static const int kStacksizeOffset = kNlocalsOffset + kPointerSize;
  static const int kFlagsOffset = kStacksizeOffset + kPointerSize;
  static const int kFirstlinenoOffset = kFlagsOffset + kPointerSize;
  static const int kCodeOffset = kFirstlinenoOffset + kPointerSize;
  static const int kConstsOffset = kCodeOffset + kPointerSize;
  static const int kNamesOffset = kConstsOffset + kPointerSize;
  static const int kVarnamesOffset = kNamesOffset + kPointerSize;
  static const int kFreevarsOffset = kVarnamesOffset + kPointerSize;
  static const int kCellvarsOffset = kFreevarsOffset + kPointerSize;
  static const int kCell2argOffset = kCellvarsOffset + kPointerSize;
  static const int kFilenameOffset = kCell2argOffset + kPointerSize;
  static const int kNameOffset = kFilenameOffset + kPointerSize;
  static const int kLnotabOffset = kNameOffset + kPointerSize;
  static const int kSize = kLnotabOffset + kPointerSize;

  RAW_OBJECT_COMMON(Code);
};

class Frame;
class Thread;

/**
 * A function object.
 *
 * This may contain a user-defined function or a built-in function.
 *
 * RawFunction objects have a set of pre-defined attributes, only some of which
 * are writable outside of the runtime. The full set is defined at
 *
 *     https://docs.python.org/3/reference/datamodel.html
 */
class RawFunction : public RawHeapObject {
 public:
  /**
   * An entry point into a function.
   *
   * The entry point is called with the current thread, the caller's stack
   * frame, and the number of arguments that have been pushed onto the stack.
   */
  using Entry = RawObject (*)(Thread*, Frame*, word);

  // Getters and setters.

  // A dict containing parameter annotations
  RawObject annotations() const;
  void setAnnotations(RawObject annotations) const;

  // The code object backing this function or None
  RawObject code() const;
  void setCode(RawObject code) const;

  // A tuple of cell objects that contain bindings for the function's free
  // variables. Read-only to user code.
  RawObject closure() const;
  void setClosure(RawObject closure) const;

  // A tuple containing default values for arguments with defaults. Read-only
  // to user code.
  RawObject defaults() const;
  void setDefaults(RawObject defaults) const;
  bool hasDefaults() const;

  // The function's docstring
  RawObject doc() const;
  void setDoc(RawObject doc) const;

  // Returns the entry to be used when the function is invoked via
  // CALL_FUNCTION
  Entry entry() const;
  void setEntry(Entry thunk) const;

  // Returns the entry to be used when the function is invoked via
  // CALL_FUNCTION_KW
  Entry entryKw() const;
  void setEntryKw(Entry thunk) const;

  // Returns the entry to be used when the function is invoked via
  // CALL_FUNCTION_EX
  inline Entry entryEx() const;
  inline void setEntryEx(Entry thunk) const;

  // The dict that holds this function's global namespace. User-code
  // cannot change this
  RawObject globals() const;
  void setGlobals(RawObject globals) const;

  // A dict containing defaults for keyword-only parameters
  RawObject kwDefaults() const;
  void setKwDefaults(RawObject kw_defaults) const;

  // The name of the module the function was defined in
  RawObject module() const;
  void setModule(RawObject module) const;

  // The function's name
  RawObject name() const;
  void setName(RawObject name) const;

  // The function's qualname
  RawObject qualname() const;
  void setQualname(RawObject qualname) const;

  // The pre-computed object array provided fast globals access.
  // fastGlobals[arg] == globals[names[arg]]
  RawObject fastGlobals() const;
  void setFastGlobals(RawObject fast_globals) const;

  // The function's dictionary
  RawObject dict() const;
  void setDict(RawObject dict) const;

  // Layout.
  static const int kDocOffset = RawHeapObject::kSize;
  static const int kNameOffset = kDocOffset + kPointerSize;
  static const int kQualnameOffset = kNameOffset + kPointerSize;
  static const int kModuleOffset = kQualnameOffset + kPointerSize;
  static const int kDefaultsOffset = kModuleOffset + kPointerSize;
  static const int kCodeOffset = kDefaultsOffset + kPointerSize;
  static const int kAnnotationsOffset = kCodeOffset + kPointerSize;
  static const int kKwDefaultsOffset = kAnnotationsOffset + kPointerSize;
  static const int kClosureOffset = kKwDefaultsOffset + kPointerSize;
  static const int kGlobalsOffset = kClosureOffset + kPointerSize;
  static const int kEntryOffset = kGlobalsOffset + kPointerSize;
  static const int kEntryKwOffset = kEntryOffset + kPointerSize;
  static const int kEntryExOffset = kEntryKwOffset + kPointerSize;
  static const int kFastGlobalsOffset = kEntryExOffset + kPointerSize;
  static const int kDictOffset = kFastGlobalsOffset + kPointerSize;
  static const int kSize = kDictOffset + kPointerSize;

  RAW_OBJECT_COMMON(Function);
};

class RawInstance : public RawHeapObject {
 public:
  // Sizing.
  static word allocationSize(word num_attributes);

  RAW_OBJECT_COMMON(Instance);
};

class RawModule : public RawHeapObject {
 public:
  // Setters and getters.
  RawObject name() const;
  void setName(RawObject name) const;

  RawObject dict() const;
  void setDict(RawObject dict) const;

  // Contains the numeric address of mode definition object for C-API modules or
  // zero if the module was not defined through the C-API.
  RawObject def() const;
  void setDef(RawObject def) const;

  // Layout.
  static const int kNameOffset = RawHeapObject::kSize;
  static const int kDictOffset = kNameOffset + kPointerSize;
  static const int kDefOffset = kDictOffset + kPointerSize;
  static const int kSize = kDefOffset + kPointerSize;

  RAW_OBJECT_COMMON(Module);
};

class RawNotImplemented : public RawHeapObject {
 public:
  // Layout.
  // kPaddingOffset is not used, but the GC expects the object to be
  // at least one word.
  static const int kPaddingOffset = RawHeapObject::kSize;
  static const int kSize = kPaddingOffset + kPointerSize;

  RAW_OBJECT_COMMON(NotImplemented);
};

/**
 * A mutable array of bytes.
 *
 * Invariant: All allocated bytes past the end of the array are 0.
 *
 * RawLayout:
 *   [RawType pointer]
 *   [Bytes          ] - Pointer to a RawBytes with the underlying data.
 *   [NumItems       ] - Number of bytes currently in the array.
 */
class RawByteArray : public RawHeapObject {
 public:
  // Getters and setters
  byte byteAt(word index) const;
  void byteAtPut(word index, byte value) const;
  RawObject bytes() const;
  void setBytes(RawObject new_bytes) const;
  word numItems() const;
  void setNumItems(word num_bytes) const;
  void downsize(word new_length) const;

  // The size of the underlying bytes
  word capacity() const;

  // Layout
  static const int kBytesOffset = RawHeapObject::kSize;
  static const int kNumItemsOffset = kBytesOffset + kPointerSize;
  static const int kSize = kNumItemsOffset + kPointerSize;

  RAW_OBJECT_COMMON(ByteArray);
};

/**
 * A simple dict that uses open addressing and linear probing.
 *
 * RawLayout:
 *
 *   [RawType pointer]
 *   [NumItems     ] - Number of items currently in the dict
 *   [Items        ] - Pointer to an RawTuple that stores the underlying
 * data.
 *
 * RawDict entries are stored in buckets as a triple of (hash, key, value).
 * Empty buckets are stored as (RawNoneType, RawNoneType, RawNoneType).
 * Tombstone buckets are stored as (RawNoneType, <not RawNoneType>, <Any>).
 *
 */
class RawDict : public RawHeapObject {
 public:
  class Bucket;

  // Getters and setters.
  // The RawTuple backing the dict
  RawObject data() const;
  void setData(RawObject data) const;

  // Number of items currently in the dict
  word numItems() const;
  void setNumItems(word num_items) const;

  // Layout.
  static const int kNumItemsOffset = RawHeapObject::kSize;
  static const int kDataOffset = kNumItemsOffset + kPointerSize;
  static const int kSize = kDataOffset + kPointerSize;

  RAW_OBJECT_COMMON(Dict);
};

typedef bool (*DictEq)(RawObject a, RawObject b);

// Helper class for manipulating buckets in the RawTuple that backs the
// dict
class RawDict::Bucket {
 public:
  // none of these operations do bounds checking on the backing array
  static word getIndex(RawTuple data, RawObject hash) {
    word nbuckets = data.length() / kNumPointers;
    DCHECK(Utils::isPowerOfTwo(nbuckets), "%ld is not a power of 2", nbuckets);
    word value = RawSmallInt::cast(hash).value();
    return (value & (nbuckets - 1)) * kNumPointers;
  }

  static bool hasKey(RawTuple data, word index, RawObject that_key,
                     DictEq pred) {
    return !hash(data, index).isNoneType() && pred(key(data, index), that_key);
  }

  static RawObject hash(RawTuple data, word index) {
    return data.at(index + kHashOffset);
  }

  static bool isEmpty(RawTuple data, word index) {
    return hash(data, index).isNoneType() && key(data, index).isNoneType();
  }

  static bool isTombstone(RawTuple data, word index) {
    return hash(data, index).isNoneType() && !key(data, index).isNoneType();
  }

  static RawObject key(RawTuple data, word index) {
    return data.at(index + kKeyOffset);
  }

  static void set(RawTuple data, word index, RawObject hash, RawObject key,
                  RawObject value) {
    data.atPut(index + kHashOffset, hash);
    data.atPut(index + kKeyOffset, key);
    data.atPut(index + kValueOffset, value);
  }

  static void setTombstone(RawTuple data, word index) {
    set(data, index, RawNoneType::object(), RawError::object(),
        RawNoneType::object());
  }

  static RawObject value(RawTuple data, word index) {
    return data.at(index + kValueOffset);
  }

  static bool nextItem(RawTuple data, word* idx) {
    // Calling next on an invalid index should not advance that index.
    if (*idx >= data.length()) {
      return false;
    }
    do {
      *idx += kNumPointers;
    } while (*idx < data.length() && isEmptyOrTombstone(data, *idx));
    return *idx < data.length();
  }

  // Layout.
  static const word kHashOffset = 0;
  static const word kKeyOffset = kHashOffset + 1;
  static const word kValueOffset = kKeyOffset + 1;
  static const word kNumPointers = kValueOffset + 1;
  static const word kFirst = -kNumPointers;

 private:
  static bool isEmptyOrTombstone(RawTuple data, word index) {
    return isEmpty(data, index) || isTombstone(data, index);
  }

  DISALLOW_HEAP_ALLOCATION();
};

class RawDictIteratorBase : public RawIteratorBase {
 public:
  // Getters and setters.
  /*
   * This looks similar to index but is different and required in order to
   * implement interators properly. We cannot use index in __length_hint__
   * because index describes the position inside the internal buckets list of
   * our implementation of dict -- not the logical number of iterms. Therefore
   * we need an additional piece of state that refers to the logical number of
   * items seen so far.
   */
  word numFound() const;
  void setNumFound(word num_found) const;

  // Layout
  static const int kNumFoundOffset = RawIteratorBase::kSize;
  static const int kSize = kNumFoundOffset + kPointerSize;
};

class RawDictViewBase : public RawHeapObject {
 public:
  // Getters and setters
  RawObject dict() const;
  void setDict(RawObject dict) const;

  // Layout
  static const int kDictOffset = RawHeapObject::kSize;
  static const int kSize = kDictOffset + kPointerSize;
};

class RawDictItemIterator : public RawDictIteratorBase {
 public:
  RAW_OBJECT_COMMON(DictItemIterator);
};

class RawDictItems : public RawDictViewBase {
 public:
  RAW_OBJECT_COMMON(DictItems);
};

class RawDictKeyIterator : public RawDictIteratorBase {
 public:
  RAW_OBJECT_COMMON(DictKeyIterator);
};

class RawDictKeys : public RawDictViewBase {
 public:
  RAW_OBJECT_COMMON(DictKeys);
};

class RawDictValueIterator : public RawDictIteratorBase {
 public:
  RAW_OBJECT_COMMON(DictValueIterator);
};

class RawDictValues : public RawDictViewBase {
 public:
  RAW_OBJECT_COMMON(DictValues);
};

/**
 * A simple set implementation. Used by set and frozenset.
 */
class RawSetBase : public RawHeapObject {
 public:
  class Bucket;

  // Getters and setters.
  // The RawTuple backing the set
  RawObject data() const;
  void setData(RawObject data) const;

  // Number of items currently in the set
  word numItems() const;
  void setNumItems(word num_items) const;

  // Layout.
  static const int kNumItemsOffset = RawHeapObject::kSize;
  static const int kDataOffset = kNumItemsOffset + kPointerSize;
  static const int kSize = kDataOffset + kPointerSize;

  RAW_OBJECT_COMMON(SetBase);
};

class RawSet : public RawSetBase {
 public:
  RAW_OBJECT_COMMON(Set);
};

class RawFrozenSet : public RawSetBase {
 public:
  RAW_OBJECT_COMMON(FrozenSet);
};

// Helper class for manipulating buckets in the RawTuple that backs the
// set
class RawSetBase::Bucket {
 public:
  // none of these operations do bounds checking on the backing array
  static word getIndex(RawTuple data, RawObject hash) {
    word nbuckets = data.length() / kNumPointers;
    DCHECK(Utils::isPowerOfTwo(nbuckets), "%ld not a power of 2", nbuckets);
    word value = RawSmallInt::cast(hash).value();
    return (value & (nbuckets - 1)) * kNumPointers;
  }

  static RawObject hash(RawTuple data, word index) {
    return data.at(index + kHashOffset);
  }

  static bool hasKey(RawTuple data, word index, RawObject that_key) {
    return !hash(data, index).isNoneType() &&
           RawObject::equals(key(data, index), that_key);
  }

  static bool isEmpty(RawTuple data, word index) {
    return hash(data, index).isNoneType() && key(data, index).isNoneType();
  }

  static bool isTombstone(RawTuple data, word index) {
    return hash(data, index).isNoneType() && !key(data, index).isNoneType();
  }

  static RawObject key(RawTuple data, word index) {
    return data.at(index + kKeyOffset);
  }

  static void set(RawTuple data, word index, RawObject hash, RawObject key) {
    data.atPut(index + kHashOffset, hash);
    data.atPut(index + kKeyOffset, key);
  }

  static void setTombstone(RawTuple data, word index) {
    set(data, index, RawNoneType::object(), RawError::object());
  }

  static bool nextItem(RawTuple data, word* idx) {
    // Calling next on an invalid index should not advance that index.
    if (*idx >= data.length()) {
      return false;
    }
    do {
      *idx += kNumPointers;
    } while (*idx < data.length() && isEmptyOrTombstone(data, *idx));
    return *idx < data.length();
  }

  // Layout.
  static const word kHashOffset = 0;
  static const word kKeyOffset = kHashOffset + 1;
  static const word kNumPointers = kKeyOffset + 1;
  static const word kFirst = -kNumPointers;

 private:
  static bool isEmptyOrTombstone(RawTuple data, word index) {
    return isEmpty(data, index) || isTombstone(data, index);
  }

  DISALLOW_HEAP_ALLOCATION();
};

/**
 * A growable array
 *
 * RawLayout:
 *
 *   [RawType pointer]
 *   [Length       ] - Number of elements currently in the list
 *   [Elems        ] - Pointer to an RawTuple that contains list elements
 */
class RawList : public RawHeapObject {
 public:
  // Getters and setters.
  RawObject at(word index) const;
  void atPut(word index, RawObject value) const;
  RawObject items() const;
  void setItems(RawObject new_items) const;
  word numItems() const;
  void setNumItems(word num_items) const;

  // Return the total number of elements that may be held without growing the
  // list
  word capacity() const;

  // Layout.
  static const int kItemsOffset = RawHeapObject::kSize;
  static const int kAllocatedOffset = kItemsOffset + kPointerSize;
  static const int kSize = kAllocatedOffset + kPointerSize;

  RAW_OBJECT_COMMON(List);
};

class RawValueCell : public RawHeapObject {
 public:
  // Getters and setters
  RawObject value() const;
  void setValue(RawObject object) const;

  bool isUnbound() const;
  void makeUnbound() const;

  // Layout.
  static const int kValueOffset = RawHeapObject::kSize;
  static const int kSize = kValueOffset + kPointerSize;

  RAW_OBJECT_COMMON(ValueCell);
};

class RawEllipsis : public RawHeapObject {
 public:
  // Layout.
  // kPaddingOffset is not used, but the GC expects the object to be
  // at least one word.
  static const int kPaddingOffset = RawHeapObject::kSize;
  static const int kSize = kPaddingOffset + kPointerSize;

  RAW_OBJECT_COMMON(Ellipsis);
};

class RawWeakRef : public RawHeapObject {
 public:
  // Getters and setters

  // The object weakly referred to by this instance.
  RawObject referent() const;
  void setReferent(RawObject referent) const;

  // A callable object invoked with the referent as an argument when the
  // referent is deemed to be "near death" and only reachable through this weak
  // reference.
  RawObject callback() const;
  void setCallback(RawObject callable) const;

  // Singly linked list of weak reference objects.  This field is used during
  // garbage collection to represent the set of weak references that had been
  // discovered by the initial trace with an otherwise unreachable referent.
  RawObject link() const;
  void setLink(RawObject reference) const;

  static void enqueueReference(RawObject reference, RawObject* list);
  static RawObject dequeueReference(RawObject* list);
  static RawObject spliceQueue(RawObject tail1, RawObject tail2);

  // Layout.
  static const int kReferentOffset = RawHeapObject::kSize;
  static const int kCallbackOffset = kReferentOffset + kPointerSize;
  static const int kLinkOffset = kCallbackOffset + kPointerSize;
  static const int kSize = kLinkOffset + kPointerSize;

  RAW_OBJECT_COMMON(WeakRef);
};

/**
 * A RawBoundMethod binds a RawFunction and its first argument (called `self`).
 *
 * These are typically created as a temporary object during a method call,
 * though they may be created and passed around freely.
 *
 * Consider the following snippet of python code:
 *
 *   class Foo:
 *     def bar(self):
 *       return self
 *   f = Foo()
 *   f.bar()
 *
 * The Python 3.6 bytecode produced for the line `f.bar()` is:
 *
 *   LOAD_FAST                0 (f)
 *   LOAD_ATTR                1 (bar)
 *   CALL_FUNCTION            0
 *
 * The LOAD_ATTR for `f.bar` creates a `RawBoundMethod`, which is then called
 * directly by the subsequent CALL_FUNCTION opcode.
 */
class RawBoundMethod : public RawHeapObject {
 public:
  // Getters and setters

  // The function to which "self" is bound
  RawObject function() const;
  void setFunction(RawObject function) const;

  // The instance of "self" being bound
  RawObject self() const;
  void setSelf(RawObject self) const;

  // Layout.
  static const int kFunctionOffset = RawHeapObject::kSize;
  static const int kSelfOffset = kFunctionOffset + kPointerSize;
  static const int kSize = kSelfOffset + kPointerSize;

  RAW_OBJECT_COMMON(BoundMethod);
};

class RawClassMethod : public RawHeapObject {
 public:
  // Getters and setters
  RawObject function() const;
  void setFunction(RawObject function) const;

  // Layout.
  static const int kFunctionOffset = RawHeapObject::kSize;
  static const int kSize = kFunctionOffset + kPointerSize;

  RAW_OBJECT_COMMON(ClassMethod);
};

/**
 * A RawLayout describes the in-memory shape of an instance.
 *
 * RawInstance attributes are split into two classes: in-object attributes,
 * which exist directly in the instance, and overflow attributes, which are
 * stored in an object array pointed to by the last word of the instance.
 * Graphically, this looks like:
 *
 *   RawInstance                                   RawTuple
 *   +---------------------------+     +------->+--------------------------+
 *   | First in-object attribute |     |        | First overflow attribute |
 *   +---------------------------+     |        +--------------------------+
 *   |            ...            |     |        |           ...            |
 *   +---------------------------+     |        +--------------------------+
 *   | Last in-object attribute  |     |        | Last overflow attribute  |
 *   +---------------------------+     |        +--------------------------+
 *   | Overflow Attributes       +-----+
 *   +---------------------------+
 *
 * Each instance is associated with a layout (whose id is stored in the header
 * word). The layout acts as a roadmap for the instance; it describes where to
 * find each attribute.
 *
 * In general, instances of the same class will have the same shape. Idiomatic
 * Python typically initializes attributes in the same order for instances of
 * the same class. Ideally, we would be able to share the same concrete
 * RawLayout between two instances of the same shape. This both reduces memory
 * overhead and enables effective caching of attribute location.
 *
 * To achieve structural sharing, layouts form an immutable DAG. Every class
 * has a root layout that contains only in-object attributes. When an instance
 * is created, it is assigned the root layout of its class. When a shape
 * altering mutation to the instance occurs (e.g. adding an attribute), the
 * current layout is searched for a corresponding edge. If such an edge exists,
 * it is followed and the instance is assigned the resulting layout. If there
 * is no such edge, a new layout is created, an edge is inserted between
 * the two layouts, and the instance is assigned the new layout.
 *
 */
class RawLayout : public RawHeapObject {
 public:
  // Getters and setters.
  LayoutId id() const;
  void setId(LayoutId id) const;

  // Returns the class whose instances are described by this layout
  RawObject describedType() const;
  void setDescribedType(RawObject type) const;

  // Set the number of in-object attributes that may be stored on an instance
  // described by this layout.
  //
  // N.B. - This will always be larger than or equal to the length of the
  // RawTuple returned by inObjectAttributes().
  void setNumInObjectAttributes(word count) const;
  word numInObjectAttributes() const;

  // Returns an RawTuple describing the attributes stored directly in
  // in the instance.
  //
  // Each item in the object array is a two element tuple. Each tuple is
  // composed of the following elements, in order:
  //
  //   1. The attribute name (RawStr)
  //   2. The attribute info (AttributeInfo)
  RawObject inObjectAttributes() const;
  void setInObjectAttributes(RawObject attributes) const;

  // Returns an RawTuple describing the attributes stored in the overflow
  // array of the instance.
  //
  // Each item in the object array is a two element tuple. Each tuple is
  // composed of the following elements, in order:
  //
  //   1. The attribute name (RawStr)
  //   2. The attribute info (AttributeInfo)
  RawObject overflowAttributes() const;
  void setOverflowAttributes(RawObject attributes) const;

  // Returns a flattened list of tuples. Each tuple is composed of the
  // following elements, in order:
  //
  //   1. The attribute name (RawStr)
  //   2. The layout that would result if an attribute with that name
  //      was added.
  RawObject additions() const;
  void setAdditions(RawObject additions) const;

  // Returns a flattened list of tuples. Each tuple is composed of the
  // following elements, in order:
  //
  //   1. The attribute name (RawStr)
  //   2. The layout that would result if an attribute with that name
  //      was deleted.
  RawObject deletions() const;
  void setDeletions(RawObject deletions) const;

  // Returns the number of bytes in an instance described by this layout,
  // including the overflow array. Computed from the number of in-object
  // attributes and possible overflow slot.
  word instanceSize() const;

  // Return the offset, in bytes, of the overflow slot
  word overflowOffset() const;

  // Seal the attributes of the layout. Sets overflowAttributes to
  // RawNoneType::object().
  void sealAttributes() const;

  // Layout.
  static const int kDescribedTypeOffset = RawHeapObject::kSize;
  static const int kInObjectAttributesOffset =
      kDescribedTypeOffset + kPointerSize;
  static const int kOverflowAttributesOffset =
      kInObjectAttributesOffset + kPointerSize;
  static const int kAdditionsOffset = kOverflowAttributesOffset + kPointerSize;
  static const int kDeletionsOffset = kAdditionsOffset + kPointerSize;
  static const int kNumInObjectAttributesOffset =
      kDeletionsOffset + kPointerSize;
  static const int kSize = kNumInObjectAttributesOffset + kPointerSize;

  RAW_OBJECT_COMMON(Layout);
};

class RawSuper : public RawHeapObject {
 public:
  // getters and setters
  RawObject type() const;
  void setType(RawObject tp) const;
  RawObject object() const;
  void setObject(RawObject obj) const;
  RawObject objectType() const;
  void setObjectType(RawObject tp) const;

  // Layout.
  static const int kTypeOffset = RawHeapObject::kSize;
  static const int kObjectOffset = kTypeOffset + kPointerSize;
  static const int kObjectTypeOffset = kObjectOffset + kPointerSize;
  static const int kSize = kObjectTypeOffset + kPointerSize;

  RAW_OBJECT_COMMON(Super);
};

/**
 * A Frame in a HeapObject, with space allocated before and after for stack and
 * locals, respectively. It looks almost exactly like the ascii art diagram for
 * Frame (from frame.h), except that there is a fixed amount of space allocated
 * for the value stack, which comes from stacksize() on the Code object this is
 * created from:
 *
 *   +----------------------+  <--+
 *   | Arg 0                |     |
 *   | ...                  |     |
 *   | Arg N                |     |
 *   | Local 0              |     | frame()-code()->totalVars() * kPointerSize
 *   | ...                  |     |
 *   | Local N              |     |
 *   +----------------------+  <--+
 *   |                      |     |
 *   | Frame                |     | Frame::kSize
 *   |                      |     |
 *   +----------------------+  <--+  <-- frame()
 *   |                      |     |
 *   | Value stack          |     | maxStackSize() * kPointerSize
 *   |                      |     |
 *   +----------------------+  <--+
 *   | maxStackSize         |
 *   +----------------------+
 */
class RawHeapFrame : public RawHeapObject {
 public:
  // The Frame contained in this HeapFrame.
  Frame* frame() const;

  // The size of the embedded frame + stack and locals, in words.
  word numFrameWords() const;

  // Get or set the number of words allocated for the value stack. Used to
  // derive a pointer to the Frame inside this HeapFrame.
  word maxStackSize() const;
  void setMaxStackSize(word offset) const;

  // Sizing.
  static word numAttributes(word extra_words);

  // Layout.
  static const int kMaxStackSizeOffset = RawHeapObject::kSize;
  static const int kFrameOffset = kMaxStackSizeOffset + kPointerSize;

  // Number of words that aren't the Frame.
  static const int kNumOverheadWords = kFrameOffset / kPointerSize;

  RAW_OBJECT_COMMON(HeapFrame);
};

/**
 * The exception currently being handled. Every Generator and Coroutine has its
 * own exception state that is installed while it's running, to allow yielding
 * from an except block without losing track of the caught exception.
 *
 * TODO(T38009294): This class won't exist forever. Think very hard about
 * adding any more bits of state to it.
 */
class RawExceptionState : public RawHeapObject {
 public:
  // Getters and setters.
  RawObject type() const;
  RawObject value() const;
  RawObject traceback() const;

  void setType(RawObject type) const;
  void setValue(RawObject value) const;
  void setTraceback(RawObject tb) const;

  RawObject previous() const;
  void setPrevious(RawObject prev) const;

  // Layout.
  static const int kTypeOffset = RawHeapObject::kSize + kPointerSize;
  static const int kValueOffset = kTypeOffset + kPointerSize;
  static const int kTracebackOffset = kValueOffset + kPointerSize;
  static const int kPreviousOffset = kTracebackOffset + kPointerSize;
  static const int kSize = kPreviousOffset + kPointerSize;

  RAW_OBJECT_COMMON(ExceptionState);
};

/**
 * Base class containing functionality needed by all objects representing a
 * suspended execution frame: RawGenerator, RawCoroutine, and AsyncGenerator.
 */
class RawGeneratorBase : public RawHeapObject {
 public:
  // Get or set the RawHeapFrame embedded in this RawGeneratorBase.
  RawObject heapFrame() const;
  void setHeapFrame(RawObject obj) const;

  RawObject exceptionState() const;
  void setExceptionState(RawObject obj) const;

  // Layout.
  static const int kFrameOffset = RawHeapObject::kSize;
  static const int kExceptionStateOffset = kFrameOffset + kPointerSize;
  static const int kSize = kExceptionStateOffset + kPointerSize;

  RAW_OBJECT_COMMON(GeneratorBase);
};

class RawGenerator : public RawGeneratorBase {
 public:
  static const int kYieldFromOffset = RawGeneratorBase::kSize;
  static const int kSize = kYieldFromOffset + kPointerSize;

  RAW_OBJECT_COMMON(Generator);
};

class RawCoroutine : public RawGeneratorBase {
 public:
  // Layout.
  static const int kAwaitOffset = RawGeneratorBase::kSize;
  static const int kOriginOffset = kAwaitOffset + kPointerSize;
  static const int kSize = kOriginOffset + kPointerSize;

  RAW_OBJECT_COMMON(Coroutine);
};

class RawTraceback : public RawHeapObject {
 public:
  // Layout.
  static const int kNextOffset = RawHeapObject::kSize;
  static const int kFrameOffset = kNextOffset + kPointerSize;
  static const int kLastiOffset = kFrameOffset + kPointerSize;
  static const int kLinenoOffset = kLastiOffset + kPointerSize;
  static const int kSize = kLinenoOffset + kPointerSize;

  RAW_OBJECT_COMMON(Traceback);
};

// RawObject

inline RawObject::RawObject(uword raw) : raw_{raw} {}

inline uword RawObject::raw() const { return raw_; }

inline bool RawObject::isObject() const { return true; }

inline LayoutId RawObject::layoutId() const {
  if (isHeapObject()) {
    return RawHeapObject::cast(*this).header().layoutId();
  }
  if (isSmallInt()) {
    return LayoutId::kSmallInt;
  }
  return static_cast<LayoutId>(raw() & kImmediateTypeTableIndexMask);
}

inline bool RawObject::isType() const {
  return isHeapObjectWithLayout(LayoutId::kType);
}

inline bool RawObject::isClassMethod() const {
  return isHeapObjectWithLayout(LayoutId::kClassMethod);
}

inline bool RawObject::isSmallInt() const {
  return (raw() & RawSmallInt::kTagMask) == RawSmallInt::kTag;
}

inline bool RawObject::isSmallStr() const {
  return (raw() & RawSmallStr::kTagMask) == RawSmallStr::kTag;
}

inline bool RawObject::isHeader() const {
  return (raw() & RawHeader::kTagMask) == RawHeader::kTag;
}

inline bool RawObject::isBool() const {
  return (raw() & RawBool::kTagMask) == RawBool::kTag;
}

inline bool RawObject::isNoneType() const {
  return (raw() & RawNoneType::kTagMask) == RawNoneType::kTag;
}

inline bool RawObject::isError() const {
  return (raw() & RawError::kTagMask) == RawError::kTag;
}

inline bool RawObject::isHeapObject() const {
  return (raw() & RawHeapObject::kTagMask) == RawHeapObject::kTag;
}

inline bool RawObject::isHeapObjectWithLayout(LayoutId layout_id) const {
  if (!isHeapObject()) {
    return false;
  }
  return RawHeapObject::cast(*this).header().layoutId() == layout_id;
}

inline bool RawObject::isLayout() const {
  return isHeapObjectWithLayout(LayoutId::kLayout);
}

inline bool RawObject::isBaseException() const {
  return isHeapObjectWithLayout(LayoutId::kBaseException);
}

inline bool RawObject::isException() const {
  return isHeapObjectWithLayout(LayoutId::kException);
}

inline bool RawObject::isExceptionState() const {
  return isHeapObjectWithLayout(LayoutId::kExceptionState);
}

inline bool RawObject::isBoundMethod() const {
  return isHeapObjectWithLayout(LayoutId::kBoundMethod);
}

inline bool RawObject::isByteArray() const {
  return isHeapObjectWithLayout(LayoutId::kByteArray);
}

inline bool RawObject::isBytes() const {
  return isHeapObjectWithLayout(LayoutId::kBytes);
}

inline bool RawObject::isTuple() const {
  return isHeapObjectWithLayout(LayoutId::kTuple);
}

inline bool RawObject::isCode() const {
  return isHeapObjectWithLayout(LayoutId::kCode);
}

inline bool RawObject::isComplex() const {
  return isHeapObjectWithLayout(LayoutId::kComplex);
}

inline bool RawObject::isCoroutine() const {
  return isHeapObjectWithLayout(LayoutId::kCoroutine);
}

inline bool RawObject::isLargeStr() const {
  return isHeapObjectWithLayout(LayoutId::kLargeStr);
}

inline bool RawObject::isFrozenSet() const {
  return isHeapObjectWithLayout(LayoutId::kFrozenSet);
}

inline bool RawObject::isFunction() const {
  return isHeapObjectWithLayout(LayoutId::kFunction);
}

inline bool RawObject::isInstance() const {
  if (!isHeapObject()) {
    return false;
  }
  return RawHeapObject::cast(*this).header().layoutId() >
         LayoutId::kLastBuiltinId;
}

inline bool RawObject::isKeyError() const {
  return isHeapObjectWithLayout(LayoutId::kKeyError);
}

inline bool RawObject::isDict() const {
  return isHeapObjectWithLayout(LayoutId::kDict);
}

inline bool RawObject::isDictItemIterator() const {
  return isHeapObjectWithLayout(LayoutId::kDictItemIterator);
}

inline bool RawObject::isDictItems() const {
  return isHeapObjectWithLayout(LayoutId::kDictItems);
}

inline bool RawObject::isDictKeyIterator() const {
  return isHeapObjectWithLayout(LayoutId::kDictKeyIterator);
}

inline bool RawObject::isDictKeys() const {
  return isHeapObjectWithLayout(LayoutId::kDictKeys);
}

inline bool RawObject::isDictValueIterator() const {
  return isHeapObjectWithLayout(LayoutId::kDictValueIterator);
}

inline bool RawObject::isDictValues() const {
  return isHeapObjectWithLayout(LayoutId::kDictValues);
}

inline bool RawObject::isFloat() const {
  return isHeapObjectWithLayout(LayoutId::kFloat);
}

inline bool RawObject::isHeapFrame() const {
  return isHeapObjectWithLayout(LayoutId::kHeapFrame);
}

inline bool RawObject::isSeqIterator() const {
  return isHeapObjectWithLayout(LayoutId::kSeqIterator);
}

inline bool RawObject::isSetBase() const { return isSet() || isFrozenSet(); }

inline bool RawObject::isSet() const {
  return isHeapObjectWithLayout(LayoutId::kSet);
}

inline bool RawObject::isSetIterator() const {
  return isHeapObjectWithLayout(LayoutId::kSetIterator);
}

inline bool RawObject::isSuper() const {
  return isHeapObjectWithLayout(LayoutId::kSuper);
}

inline bool RawObject::isModule() const {
  return isHeapObjectWithLayout(LayoutId::kModule);
}

inline bool RawObject::isList() const {
  return isHeapObjectWithLayout(LayoutId::kList);
}

inline bool RawObject::isListIterator() const {
  return isHeapObjectWithLayout(LayoutId::kListIterator);
}

inline bool RawObject::isLookupError() const {
  return isHeapObjectWithLayout(LayoutId::kLookupError);
}

inline bool RawObject::isValueCell() const {
  return isHeapObjectWithLayout(LayoutId::kValueCell);
}

inline bool RawObject::isEllipsis() const {
  return isHeapObjectWithLayout(LayoutId::kEllipsis);
}

inline bool RawObject::isGenerator() const {
  return isHeapObjectWithLayout(LayoutId::kGenerator);
}

inline bool RawObject::isLargeInt() const {
  return isHeapObjectWithLayout(LayoutId::kLargeInt);
}

inline bool RawObject::isInt() const {
  return isSmallInt() || isLargeInt() || isBool();
}

inline bool RawObject::isNotImplemented() const {
  return isHeapObjectWithLayout(LayoutId::kNotImplemented);
}

inline bool RawObject::isNotImplementedError() const {
  return isHeapObjectWithLayout(LayoutId::kNotImplementedError);
}

inline bool RawObject::isProperty() const {
  return isHeapObjectWithLayout(LayoutId::kProperty);
}

inline bool RawObject::isRange() const {
  return isHeapObjectWithLayout(LayoutId::kRange);
}

inline bool RawObject::isRangeIterator() const {
  return isHeapObjectWithLayout(LayoutId::kRangeIterator);
}

inline bool RawObject::isGeneratorBase() const {
  return isGenerator() || isCoroutine();
}

inline bool RawObject::isRuntimeError() const {
  return isHeapObjectWithLayout(LayoutId::kRuntimeError);
}

inline bool RawObject::isSlice() const {
  return isHeapObjectWithLayout(LayoutId::kSlice);
}

inline bool RawObject::isStaticMethod() const {
  return isHeapObjectWithLayout(LayoutId::kStaticMethod);
}

inline bool RawObject::isStopIteration() const {
  return isHeapObjectWithLayout(LayoutId::kStopIteration);
}

inline bool RawObject::isStr() const { return isSmallStr() || isLargeStr(); }

inline bool RawObject::isStrIterator() const {
  return isHeapObjectWithLayout(LayoutId::kStrIterator);
}

inline bool RawObject::isSystemExit() const {
  return isHeapObjectWithLayout(LayoutId::kSystemExit);
}

inline bool RawObject::isTraceback() const {
  return isHeapObjectWithLayout(LayoutId::kTraceback);
}

inline bool RawObject::isTupleIterator() const {
  return isHeapObjectWithLayout(LayoutId::kTupleIterator);
}

inline bool RawObject::isUnboundValue() const {
  return isHeapObjectWithLayout(LayoutId::kUnboundValue);
}

inline bool RawObject::isUnicodeDecodeError() const {
  return isHeapObjectWithLayout(LayoutId::kUnicodeDecodeError);
}

inline bool RawObject::isUnicodeEncodeError() const {
  return isHeapObjectWithLayout(LayoutId::kUnicodeEncodeError);
}

inline bool RawObject::isUnicodeError() const {
  return isHeapObjectWithLayout(LayoutId::kUnicodeError);
}

inline bool RawObject::isUnicodeTranslateError() const {
  return isHeapObjectWithLayout(LayoutId::kUnicodeTranslateError);
}

inline bool RawObject::isImportError() const {
  return isHeapObjectWithLayout(LayoutId::kImportError);
}

inline bool RawObject::isIndexError() const {
  return isHeapObjectWithLayout(LayoutId::kIndexError);
}

inline bool RawObject::isWeakRef() const {
  return isHeapObjectWithLayout(LayoutId::kWeakRef);
}

inline bool RawObject::isModuleNotFoundError() const {
  return isHeapObjectWithLayout(LayoutId::kModuleNotFoundError);
}

inline bool RawObject::equals(RawObject lhs, RawObject rhs) {
  return (lhs == rhs) ||
         (lhs.isLargeStr() && RawLargeStr::cast(lhs).equals(rhs));
}

inline bool RawObject::operator==(const RawObject& other) const {
  return raw() == other.raw();
}

inline bool RawObject::operator!=(const RawObject& other) const {
  return !operator==(other);
}

template <typename T>
T RawObject::rawCast() const {
  return *static_cast<const T*>(this);
}

// RawInt

inline word RawInt::asWord() const {
  if (isSmallInt()) {
    return RawSmallInt::cast(*this).value();
  }
  if (isBool()) {
    return RawBool::cast(*this).value();
  }
  return RawLargeInt::cast(*this).asWord();
}

inline word RawInt::asWordSaturated() const {
  if (numDigits() == 1) return asWord();
  return isNegative() ? kMinWord : kMaxWord;
}

inline void* RawInt::asCPtr() const {
  if (isSmallInt()) {
    return RawSmallInt::cast(*this).asCPtr();
  }
  return RawLargeInt::cast(*this).asCPtr();
}

template <typename T>
OptInt<T> RawInt::asInt() const {
  if (isSmallInt()) return RawSmallInt::cast(*this).asInt<T>();
  return RawLargeInt::cast(*this).asInt<T>();
}

inline word RawInt::bitLength() const {
  if (isSmallInt()) {
    uword self = static_cast<uword>(std::abs(RawSmallInt::cast(*this).value()));
    return Utils::highestBit(self);
  }
  if (isBool()) {
    return RawBool::cast(*this) == RawBool::trueObj() ? 1 : 0;
  }
  return RawLargeInt::cast(*this).bitLength();
}

inline bool RawInt::isPositive() const {
  if (isSmallInt()) {
    return RawSmallInt::cast(*this).value() > 0;
  }
  if (isBool()) {
    return RawBool::cast(*this) == RawBool::trueObj();
  }
  return RawLargeInt::cast(*this).isPositive();
}

inline bool RawInt::isNegative() const {
  if (isSmallInt()) {
    return RawSmallInt::cast(*this).value() < 0;
  }
  if (isBool()) {
    return false;
  }
  return RawLargeInt::cast(*this).isNegative();
}

inline bool RawInt::isZero() const {
  if (isSmallInt()) {
    return RawSmallInt::cast(*this).value() == 0;
  }
  if (isBool()) {
    return RawBool::cast(*this) == RawBool::falseObj();
  }
  // A RawLargeInt can never be zero
  DCHECK(isLargeInt(), "RawObject must be a RawLargeInt");
  return false;
}

inline word RawInt::numDigits() const {
  if (isSmallInt() || isBool()) {
    return 1;
  }
  return RawLargeInt::cast(*this).numDigits();
}

inline uword RawInt::digitAt(word index) const {
  if (isSmallInt()) {
    DCHECK(index == 0, "RawSmallInt digit index out of bounds");
    return RawSmallInt::cast(*this).value();
  }
  if (isBool()) {
    DCHECK(index == 0, "RawBool digit index out of bounds");
    return RawBool::cast(*this).value();
  }
  return RawLargeInt::cast(*this).digitAt(index);
}

// RawSmallInt

inline word RawSmallInt::value() const {
  return static_cast<word>(raw()) >> kTagSize;
}

inline void* RawSmallInt::asCPtr() const {
  return reinterpret_cast<void*>(value());
}

template <typename T>
if_signed_t<T, OptInt<T>> RawSmallInt::asInt() const {
  static_assert(sizeof(T) <= sizeof(word), "T must not be larger than word");

  auto const value = this->value();
  if (value > std::numeric_limits<T>::max()) return OptInt<T>::overflow();
  if (value < std::numeric_limits<T>::min()) return OptInt<T>::underflow();
  return OptInt<T>::valid(value);
}

template <typename T>
if_unsigned_t<T, OptInt<T>> RawSmallInt::asInt() const {
  static_assert(sizeof(T) <= sizeof(word), "T must not be larger than word");
  auto const max = std::numeric_limits<T>::max();
  auto const value = this->value();

  if (value < 0) return OptInt<T>::underflow();
  if (max >= RawSmallInt::kMaxValue || static_cast<uword>(value) <= max) {
    return OptInt<T>::valid(value);
  }
  return OptInt<T>::overflow();
}

inline RawSmallInt RawSmallInt::fromWord(word value) {
  DCHECK(RawSmallInt::isValid(value), "invalid cast");
  return cast(RawObject{static_cast<uword>(value) << kTagSize});
}

inline RawSmallInt RawSmallInt::fromWordTruncated(word value) {
  return cast(RawObject{static_cast<uword>(value) << kTagSize});
}

template <typename T>
inline RawSmallInt RawSmallInt::fromFunctionPointer(T pointer) {
  // The bit pattern for a function pointer object must be indistinguishable
  // from that of a small integer object.
  return cast(RawObject{reinterpret_cast<uword>(pointer)});
}

// RawSmallStr

inline word RawSmallStr::length() const {
  return (raw() >> kTagSize) & kMaxLength;
}

inline byte RawSmallStr::charAt(word index) const {
  DCHECK_INDEX(index, length());
  return raw() >> (kBitsPerByte * (index + 1));
}

inline void RawSmallStr::copyTo(byte* dst, word length) const {
  DCHECK_BOUND(length, this->length());
  for (word i = 0; i < length; ++i) {
    *dst++ = charAt(i);
  }
}

// RawHeader

inline word RawHeader::count() const {
  return static_cast<word>((raw() >> kCountOffset) & kCountMask);
}

inline bool RawHeader::hasOverflow() const {
  return count() == kCountOverflowFlag;
}

inline word RawHeader::hashCode() const {
  return static_cast<word>((raw() >> kHashCodeOffset) & kHashCodeMask);
}

inline RawHeader RawHeader::withHashCode(word value) const {
  auto header = raw();
  header &= ~(kHashCodeMask << kHashCodeOffset);
  header |= (value & kHashCodeMask) << kHashCodeOffset;
  return cast(RawObject{header});
}

inline LayoutId RawHeader::layoutId() const {
  return static_cast<LayoutId>((raw() >> kLayoutIdOffset) & kLayoutIdMask);
}

inline RawHeader RawHeader::withLayoutId(LayoutId layout_id) const {
  DCHECK_BOUND(static_cast<word>(layout_id), kMaxLayoutId);
  auto header = raw();
  header &= ~(kLayoutIdMask << kLayoutIdOffset);
  header |= (static_cast<word>(layout_id) & kLayoutIdMask) << kLayoutIdOffset;
  return cast(RawObject{header});
}

inline ObjectFormat RawHeader::format() const {
  return static_cast<ObjectFormat>((raw() >> kFormatOffset) & kFormatMask);
}

inline RawHeader RawHeader::from(word count, word hash, LayoutId id,
                                 ObjectFormat format) {
  DCHECK(
      (count >= 0) && ((count <= kCountMax) || (count == kCountOverflowFlag)),
      "bounds violation, %ld not in 0..%d", count, kCountMax);
  uword result = RawHeader::kTag;
  result |= ((count > kCountMax) ? kCountOverflowFlag : count) << kCountOffset;
  result |= hash << kHashCodeOffset;
  result |= static_cast<uword>(id) << kLayoutIdOffset;
  result |= static_cast<uword>(format) << kFormatOffset;
  return cast(RawObject{result});
}

// None

inline RawNoneType RawNoneType::object() {
  return RawObject{kTag}.rawCast<RawNoneType>();
}

// RawError

inline RawError RawError::object() {
  return RawObject{kTag}.rawCast<RawError>();
}

// RawBool

inline RawBool RawBool::trueObj() { return fromBool(true); }

inline RawBool RawBool::falseObj() { return fromBool(false); }

inline RawBool RawBool::negate(RawObject value) {
  DCHECK(value.isBool(), "not a boolean instance");
  return (value == trueObj()) ? falseObj() : trueObj();
}

inline RawBool RawBool::fromBool(bool value) {
  return cast(RawObject{(static_cast<uword>(value) << kTagSize) | kTag});
}

inline bool RawBool::value() const {
  return (raw() >> kTagSize) ? true : false;
}

// RawHeapObject

inline uword RawHeapObject::address() const {
  return raw() - RawHeapObject::kTag;
}

inline uword RawHeapObject::baseAddress() const {
  uword result = address() - RawHeader::kSize;
  if (header().hasOverflow()) {
    result -= kPointerSize;
  }
  return result;
}

inline RawHeader RawHeapObject::header() const {
  return RawHeader::cast(instanceVariableAt(kHeaderOffset));
}

inline void RawHeapObject::setHeader(RawHeader header) const {
  instanceVariableAtPut(kHeaderOffset, header);
}

inline word RawHeapObject::headerOverflow() const {
  DCHECK(header().hasOverflow(), "expected Overflow");
  return RawSmallInt::cast(instanceVariableAt(kHeaderOverflowOffset)).value();
}

inline void RawHeapObject::setHeaderAndOverflow(word count, word hash,
                                                LayoutId id,
                                                ObjectFormat format) const {
  if (count > RawHeader::kCountMax) {
    instanceVariableAtPut(kHeaderOverflowOffset, RawSmallInt::fromWord(count));
    count = RawHeader::kCountOverflowFlag;
  }
  setHeader(RawHeader::from(count, hash, id, format));
}

inline RawHeapObject RawHeapObject::fromAddress(uword address) {
  DCHECK((address & kTagMask) == 0, "invalid cast, expected heap address");
  return cast(RawObject{address + kTag});
}

inline word RawHeapObject::headerCountOrOverflow() const {
  if (header().hasOverflow()) {
    return headerOverflow();
  }
  return header().count();
}

inline word RawHeapObject::size() const {
  word count = headerCountOrOverflow();
  word result = headerSize(count);
  switch (header().format()) {
    case ObjectFormat::kDataArray8:
      result += count;
      break;
    case ObjectFormat::kDataArray16:
      result += count * 2;
      break;
    case ObjectFormat::kDataArray32:
      result += count * 4;
      break;
    case ObjectFormat::kDataArray64:
      result += count * 8;
      break;
    case ObjectFormat::kDataArray128:
      result += count * 16;
      break;
    case ObjectFormat::kObjectArray:
    case ObjectFormat::kDataInstance:
    case ObjectFormat::kObjectInstance:
      result += count * kPointerSize;
      break;
  }
  return Utils::maximum(Utils::roundUp(result, kPointerSize), kMinimumSize);
}

inline word RawHeapObject::headerSize(word count) {
  word result = kPointerSize;
  if (count > RawHeader::kCountMax) {
    result += kPointerSize;
  }
  return result;
}

inline void RawHeapObject::initialize(word size, RawObject value) const {
  for (word offset = RawHeapObject::kSize; offset < size;
       offset += kPointerSize) {
    instanceVariableAtPut(offset, value);
  }
}

inline bool RawHeapObject::isRoot() const {
  return header().format() == ObjectFormat::kObjectArray ||
         header().format() == ObjectFormat::kObjectInstance;
}

inline bool RawHeapObject::isForwarding() const {
  return *reinterpret_cast<uword*>(address() + kHeaderOffset) == kIsForwarded;
}

inline RawObject RawHeapObject::forward() const {
  // When a heap object is forwarding, its second word is the forwarding
  // address.
  return *reinterpret_cast<RawObject*>(address() + kHeaderOffset +
                                       kPointerSize);
}

inline void RawHeapObject::forwardTo(RawObject object) const {
  // Overwrite the header with the forwarding marker.
  *reinterpret_cast<uword*>(address() + kHeaderOffset) = kIsForwarded;
  // Overwrite the second word with the forwarding addressing.
  *reinterpret_cast<RawObject*>(address() + kHeaderOffset + kPointerSize) =
      object;
}

inline RawObject RawHeapObject::instanceVariableAt(word offset) const {
  return *reinterpret_cast<RawObject*>(address() + offset);
}

inline void RawHeapObject::instanceVariableAtPut(word offset,
                                                 RawObject value) const {
  *reinterpret_cast<RawObject*>(address() + offset) = value;
}

// RawBaseException

inline RawObject RawBaseException::args() const {
  return instanceVariableAt(kArgsOffset);
}

inline void RawBaseException::setArgs(RawObject args) const {
  instanceVariableAtPut(kArgsOffset, args);
}

inline RawObject RawBaseException::traceback() const {
  RawObject o = tracebackOrUnbound();
  return o.isUnboundValue() ? RawNoneType::object() : o;
}

inline RawObject RawBaseException::tracebackOrUnbound() const {
  return instanceVariableAt(kTracebackOffset);
}

inline void RawBaseException::setTraceback(RawObject traceback) const {
  instanceVariableAtPut(kTracebackOffset, traceback);
}

inline RawObject RawBaseException::cause() const {
  RawObject o = causeOrUnbound();
  return o.isUnboundValue() ? RawNoneType::object() : o;
}

inline RawObject RawBaseException::causeOrUnbound() const {
  return instanceVariableAt(kCauseOffset);
}

inline void RawBaseException::setCause(RawObject cause) const {
  return instanceVariableAtPut(kCauseOffset, cause);
}

inline RawObject RawBaseException::context() const {
  RawObject o = contextOrUnbound();
  return o.isUnboundValue() ? RawNoneType::object() : o;
}

inline RawObject RawBaseException::contextOrUnbound() const {
  return instanceVariableAt(kContextOffset);
}

inline void RawBaseException::setContext(RawObject context) const {
  return instanceVariableAtPut(kContextOffset, context);
}

// RawStopIteration

inline RawObject RawStopIteration::value() const {
  return instanceVariableAt(kValueOffset);
}

inline void RawStopIteration::setValue(RawObject value) const {
  instanceVariableAtPut(kValueOffset, value);
}

// RawSystemExit

inline RawObject RawSystemExit::code() const {
  return instanceVariableAt(kCodeOffset);
}

inline void RawSystemExit::setCode(RawObject code) const {
  instanceVariableAtPut(kCodeOffset, code);
}

// RawImportError

inline RawObject RawImportError::msg() const {
  return instanceVariableAt(kMsgOffset);
}

inline void RawImportError::setMsg(RawObject msg) const {
  instanceVariableAtPut(kMsgOffset, msg);
}

inline RawObject RawImportError::name() const {
  return instanceVariableAt(kNameOffset);
}

inline void RawImportError::setName(RawObject name) const {
  instanceVariableAtPut(kNameOffset, name);
}

inline RawObject RawImportError::path() const {
  return instanceVariableAt(kPathOffset);
}

inline void RawImportError::setPath(RawObject path) const {
  instanceVariableAtPut(kPathOffset, path);
}

// RawType

inline RawObject RawType::mro() const { return instanceVariableAt(kMroOffset); }

inline void RawType::setMro(RawObject object_array) const {
  instanceVariableAtPut(kMroOffset, object_array);
}

inline RawObject RawType::instanceLayout() const {
  return instanceVariableAt(kInstanceLayoutOffset);
}

inline void RawType::setInstanceLayout(RawObject layout) const {
  instanceVariableAtPut(kInstanceLayoutOffset, layout);
}

inline RawObject RawType::name() const {
  return instanceVariableAt(kNameOffset);
}

inline void RawType::setName(RawObject name) const {
  instanceVariableAtPut(kNameOffset, name);
}

inline RawType::Flag RawType::flags() const {
  return static_cast<Flag>(
      RawSmallInt::cast(instanceVariableAt(kFlagsOffset)).value());
}

inline void RawType::setFlagsAndBuiltinBase(Flag value, LayoutId base) const {
  auto raw_base = static_cast<int>(base);
  DCHECK((raw_base & kBuiltinBaseMask) == raw_base,
         "Builtin base LayoutId too high");
  setFlags(static_cast<Flag>((value & ~kBuiltinBaseMask) | raw_base));
}

inline void RawType::setFlags(Flag value) const {
  instanceVariableAtPut(kFlagsOffset, RawSmallInt::fromWord(value));
}

inline void RawType::setBuiltinBase(LayoutId base) const {
  auto raw = static_cast<int>(base);
  DCHECK((raw & kBuiltinBaseMask) == raw, "Builtin base LayoutId too high");
  setFlags(static_cast<Flag>((flags() & ~kBuiltinBaseMask) | raw));
}

inline bool RawType::hasFlag(Flag bit) const { return (flags() & bit) != 0; }

inline LayoutId RawType::builtinBase() const {
  return static_cast<LayoutId>(flags() & kBuiltinBaseMask);
}

inline RawObject RawType::dict() const {
  return instanceVariableAt(kDictOffset);
}

inline void RawType::setDict(RawObject dict) const {
  instanceVariableAtPut(kDictOffset, dict);
}

inline RawObject RawType::extensionSlots() const {
  return instanceVariableAt(kExtensionSlotsOffset);
}

inline void RawType::setExtensionSlots(RawObject slots) const {
  instanceVariableAtPut(kExtensionSlotsOffset, slots);
}

inline bool RawType::isBuiltin() const {
  return RawLayout::cast(instanceLayout()).id() <= LayoutId::kLastBuiltinId;
}

inline bool RawType::isBaseExceptionSubclass() const {
  LayoutId base = builtinBase();
  return base >= LayoutId::kFirstException && base <= LayoutId::kLastException;
}

inline void RawType::sealAttributes() const {
  RawLayout layout = RawLayout::cast(instanceLayout());
  DCHECK(layout.additions().isList(), "Additions must be list");
  DCHECK(RawList::cast(layout.additions()).numItems() == 0,
         "Cannot seal a layout with outgoing edges");
  DCHECK(layout.deletions().isList(), "Deletions must be list");
  DCHECK(RawList::cast(layout.deletions()).numItems() == 0,
         "Cannot seal a layout with outgoing edges");
  DCHECK(layout.overflowAttributes().isTuple(),
         "OverflowAttributes must be tuple");
  DCHECK(RawTuple::cast(layout.overflowAttributes()).length() == 0,
         "Cannot seal a layout with outgoing edges");
  layout.sealAttributes();
}

// RawArray

inline word RawArray::length() const {
  DCHECK(isBytes() || isTuple() || isLargeStr(), "invalid array type");
  return headerCountOrOverflow();
}

// RawBytes

inline word RawBytes::allocationSize(word length) {
  DCHECK(length >= 0, "invalid length %ld", length);
  word size = headerSize(length) + length;
  return Utils::maximum(kMinimumSize, Utils::roundUp(size, kPointerSize));
}

inline byte RawBytes::byteAt(word index) const {
  DCHECK_INDEX(index, length());
  return *reinterpret_cast<byte*>(address() + index);
}

inline void RawBytes::byteAtPut(word index, byte value) const {
  DCHECK_INDEX(index, length());
  *reinterpret_cast<byte*>(address() + index) = value;
}

inline void RawBytes::copyTo(byte* dst, word length) const {
  DCHECK_BOUND(length, this->length());
  std::memcpy(dst, reinterpret_cast<const byte*>(address()), length);
}

// RawTuple

inline word RawTuple::allocationSize(word length) {
  DCHECK(length >= 0, "invalid length %ld", length);
  word size = headerSize(length) + length * kPointerSize;
  return Utils::maximum(kMinimumSize, Utils::roundUp(size, kPointerSize));
}

inline RawObject RawTuple::at(word index) const {
  DCHECK_INDEX(index, length());
  return instanceVariableAt(index * kPointerSize);
}

inline void RawTuple::atPut(word index, RawObject value) const {
  DCHECK_INDEX(index, length());
  instanceVariableAtPut(index * kPointerSize, value);
}

// RawUserTupleBase

inline RawObject RawUserTupleBase::tupleValue() const {
  return instanceVariableAt(kTupleOffset);
}

inline void RawUserTupleBase::setTupleValue(RawObject value) const {
  DCHECK(value.isTuple(), "Only tuple type is permitted as a value");
  instanceVariableAtPut(kTupleOffset, value);
}

// RawUnicodeError

inline RawObject RawUnicodeError::encoding() const {
  return instanceVariableAt(kEncodingOffset);
}

inline void RawUnicodeError::setEncoding(RawObject value) const {
  DCHECK(value.isStr(), "Only string type is permitted as a value");
  instanceVariableAtPut(kEncodingOffset, value);
}

inline RawObject RawUnicodeError::object() const {
  return instanceVariableAt(kObjectOffset);
}

inline void RawUnicodeError::setObject(RawObject value) const {
  // TODO(T39229519): Allow bytearrays to be stored as well
  DCHECK(value.isBytes(), "Only bytes type is permitted as a value");
  instanceVariableAtPut(kObjectOffset, value);
}

inline RawObject RawUnicodeError::start() const {
  return instanceVariableAt(kStartOffset);
}

inline void RawUnicodeError::setStart(RawObject value) const {
  DCHECK(value.isInt(), "Only int type is permitted as a value");
  instanceVariableAtPut(kStartOffset, value);
}

inline RawObject RawUnicodeError::end() const {
  return instanceVariableAt(kEndOffset);
}

inline void RawUnicodeError::setEnd(RawObject value) const {
  DCHECK(value.isInt(), "Only int type is permitted as a value");
  instanceVariableAtPut(kEndOffset, value);
}

inline RawObject RawUnicodeError::reason() const {
  return instanceVariableAt(kReasonOffset);
}

inline void RawUnicodeError::setReason(RawObject value) const {
  DCHECK(value.isStr(), "Only string type is permitted as a value");
  instanceVariableAtPut(kReasonOffset, value);
}

// RawCode

inline word RawCode::argcount() const {
  return RawSmallInt::cast(instanceVariableAt(kArgcountOffset)).value();
}

inline void RawCode::setArgcount(word value) const {
  instanceVariableAtPut(kArgcountOffset, RawSmallInt::fromWord(value));
}

inline RawObject RawCode::cell2arg() const {
  return instanceVariableAt(kCell2argOffset);
}

inline word RawCode::totalArgs() const {
  uword f = flags();
  word res = argcount() + kwonlyargcount();
  if (f & VARARGS) {
    res++;
  }
  if (f & VARKEYARGS) {
    res++;
  }
  return res;
}

inline void RawCode::setCell2arg(RawObject value) const {
  instanceVariableAtPut(kCell2argOffset, value);
}

inline RawObject RawCode::cellvars() const {
  return instanceVariableAt(kCellvarsOffset);
}

inline void RawCode::setCellvars(RawObject value) const {
  instanceVariableAtPut(kCellvarsOffset, value);
}

inline word RawCode::numCellvars() const {
  RawObject object = cellvars();
  DCHECK(object.isNoneType() || object.isTuple(), "not an object array");
  if (object.isNoneType()) {
    return 0;
  }
  return RawTuple::cast(object).length();
}

inline RawObject RawCode::code() const {
  return instanceVariableAt(kCodeOffset);
}

inline void RawCode::setCode(RawObject value) const {
  instanceVariableAtPut(kCodeOffset, value);
}

inline RawObject RawCode::consts() const {
  return instanceVariableAt(kConstsOffset);
}

inline void RawCode::setConsts(RawObject value) const {
  instanceVariableAtPut(kConstsOffset, value);
}

inline RawObject RawCode::filename() const {
  return instanceVariableAt(kFilenameOffset);
}

inline void RawCode::setFilename(RawObject value) const {
  instanceVariableAtPut(kFilenameOffset, value);
}

inline word RawCode::firstlineno() const {
  return RawSmallInt::cast(instanceVariableAt(kFirstlinenoOffset)).value();
}

inline void RawCode::setFirstlineno(word value) const {
  instanceVariableAtPut(kFirstlinenoOffset, RawSmallInt::fromWord(value));
}

inline word RawCode::flags() const {
  return RawSmallInt::cast(instanceVariableAt(kFlagsOffset)).value();
}

inline void RawCode::setFlags(word value) const {
  if ((kwonlyargcount() == 0) && (value & NOFREE) &&
      !(value & (VARARGS | VARKEYARGS))) {
    // Set up shortcut for detecting fast case for calls
    // TODO(buzbee): move into equivalent of CPython's codeobject.c:PyCode_New()
    value |= SIMPLE_CALL;
  }
  instanceVariableAtPut(kFlagsOffset, RawSmallInt::fromWord(value));
}

inline RawObject RawCode::freevars() const {
  return instanceVariableAt(kFreevarsOffset);
}

inline void RawCode::setFreevars(RawObject value) const {
  instanceVariableAtPut(kFreevarsOffset, value);
}

inline word RawCode::numFreevars() const {
  RawObject object = freevars();
  DCHECK(object.isNoneType() || object.isTuple(), "not an object array");
  if (object.isNoneType()) {
    return 0;
  }
  return RawTuple::cast(object).length();
}

inline word RawCode::kwonlyargcount() const {
  return RawSmallInt::cast(instanceVariableAt(kKwonlyargcountOffset)).value();
}

inline void RawCode::setKwonlyargcount(word value) const {
  instanceVariableAtPut(kKwonlyargcountOffset, RawSmallInt::fromWord(value));
}

inline RawObject RawCode::lnotab() const {
  return instanceVariableAt(kLnotabOffset);
}

inline void RawCode::setLnotab(RawObject value) const {
  instanceVariableAtPut(kLnotabOffset, value);
}

inline RawObject RawCode::name() const {
  return instanceVariableAt(kNameOffset);
}

inline void RawCode::setName(RawObject value) const {
  instanceVariableAtPut(kNameOffset, value);
}

inline RawObject RawCode::names() const {
  return instanceVariableAt(kNamesOffset);
}

inline void RawCode::setNames(RawObject value) const {
  instanceVariableAtPut(kNamesOffset, value);
}

inline word RawCode::nlocals() const {
  return RawSmallInt::cast(instanceVariableAt(kNlocalsOffset)).value();
}

inline void RawCode::setNlocals(word value) const {
  instanceVariableAtPut(kNlocalsOffset, RawSmallInt::fromWord(value));
}

inline word RawCode::totalVars() const {
  return nlocals() + numCellvars() + numFreevars();
}

inline word RawCode::stacksize() const {
  return RawSmallInt::cast(instanceVariableAt(kStacksizeOffset)).value();
}

inline void RawCode::setStacksize(word value) const {
  instanceVariableAtPut(kStacksizeOffset, RawSmallInt::fromWord(value));
}

inline RawObject RawCode::varnames() const {
  return instanceVariableAt(kVarnamesOffset);
}

inline void RawCode::setVarnames(RawObject value) const {
  instanceVariableAtPut(kVarnamesOffset, value);
}

inline bool RawCode::hasCoroutine() const { return (flags() & COROUTINE) != 0; }

inline bool RawCode::hasCoroutineOrGenerator() const {
  return hasCoroutine() || hasGenerator();
}

inline bool RawCode::hasFreevarsOrCellvars() const {
  return (flags() & NOFREE) == 0;
}

inline bool RawCode::hasGenerator() const { return (flags() & GENERATOR) != 0; }

inline bool RawCode::hasIterableCoroutine() const {
  return (flags() & ITERABLE_COROUTINE) != 0;
}

inline bool RawCode::hasVarargs() const { return (flags() & VARARGS) != 0; }

inline bool RawCode::hasVarkeyargs() const {
  return (flags() & VARKEYARGS) != 0;
}

inline bool RawCode::hasVarargsOrVarkeyargs() const {
  return hasVarargs() || hasVarkeyargs();
}

// RawLargeInt

inline word RawLargeInt::asWord() const {
  DCHECK(numDigits() == 1, "RawLargeInt cannot fit in a word");
  return static_cast<word>(digitAt(0));
}

inline void* RawLargeInt::asCPtr() const {
  DCHECK(numDigits() == 1, "Large integer cannot fit in a pointer");
  DCHECK(isPositive(), "Cannot cast a negative value to a C pointer");
  return reinterpret_cast<void*>(asWord());
}

template <typename T>
if_signed_t<T, OptInt<T>> RawLargeInt::asInt() const {
  static_assert(sizeof(T) <= sizeof(word), "T must not be larger than word");

  if (numDigits() > 1) {
    auto const high_digit = static_cast<word>(digitAt(numDigits() - 1));
    return high_digit < 0 ? OptInt<T>::underflow() : OptInt<T>::overflow();
  }
  if (numDigits() == 1) {
    auto const value = asWord();
    if (value <= std::numeric_limits<T>::max() &&
        value >= std::numeric_limits<T>::min()) {
      return OptInt<T>::valid(value);
    }
  }
  return OptInt<T>::overflow();
}

template <typename T>
if_unsigned_t<T, OptInt<T>> RawLargeInt::asInt() const {
  static_assert(sizeof(T) <= sizeof(word), "T must not be larger than word");

  if (isNegative()) return OptInt<T>::underflow();
  if (static_cast<size_t>(bitLength()) > sizeof(T) * kBitsPerByte) {
    return OptInt<T>::overflow();
  }
  // No T accepted by this function needs more than one digit.
  return OptInt<T>::valid(digitAt(0));
}

inline bool RawLargeInt::isNegative() const {
  word highest_digit = digitAt(numDigits() - 1);
  return highest_digit < 0;
}

inline bool RawLargeInt::isPositive() const {
  word highest_digit = digitAt(numDigits() - 1);
  return highest_digit >= 0;
}

inline uword RawLargeInt::digitAt(word index) const {
  DCHECK_INDEX(index, numDigits());
  return reinterpret_cast<uword*>(address() + kValueOffset)[index];
}

inline void RawLargeInt::digitAtPut(word index, uword digit) const {
  DCHECK_INDEX(index, numDigits());
  reinterpret_cast<uword*>(address() + kValueOffset)[index] = digit;
}

inline word RawLargeInt::numDigits() const { return headerCountOrOverflow(); }

inline word RawLargeInt::allocationSize(word num_digits) {
  word size = headerSize(num_digits) + num_digits * kPointerSize;
  return Utils::maximum(kMinimumSize, Utils::roundUp(size, kPointerSize));
}

// RawFloat

inline double RawFloat::value() const {
  return *reinterpret_cast<double*>(address() + kValueOffset);
}

inline void RawFloat::initialize(double value) const {
  *reinterpret_cast<double*>(address() + kValueOffset) = value;
}

// RawComplex
inline double RawComplex::real() const {
  return *reinterpret_cast<double*>(address() + kRealOffset);
}

inline double RawComplex::imag() const {
  return *reinterpret_cast<double*>(address() + kImagOffset);
}

inline void RawComplex::initialize(double real, double imag) const {
  *reinterpret_cast<double*>(address() + kRealOffset) = real;
  *reinterpret_cast<double*>(address() + kImagOffset) = imag;
}

// RawUserFloatBase

inline RawObject RawUserFloatBase::floatValue() const {
  return instanceVariableAt(kFloatOffset);
}

inline void RawUserFloatBase::setFloatValue(RawObject value) const {
  DCHECK(value.isFloat(), "Only float type is permitted as a value");
  instanceVariableAtPut(kFloatOffset, value);
}

// RawRange

inline word RawRange::start() const {
  return RawSmallInt::cast(instanceVariableAt(kStartOffset)).value();
}

inline void RawRange::setStart(word value) const {
  instanceVariableAtPut(kStartOffset, RawSmallInt::fromWord(value));
}

inline word RawRange::stop() const {
  return RawSmallInt::cast(instanceVariableAt(kStopOffset)).value();
}

inline void RawRange::setStop(word value) const {
  instanceVariableAtPut(kStopOffset, RawSmallInt::fromWord(value));
}

inline word RawRange::step() const {
  return RawSmallInt::cast(instanceVariableAt(kStepOffset)).value();
}

inline void RawRange::setStep(word value) const {
  instanceVariableAtPut(kStepOffset, RawSmallInt::fromWord(value));
}

// RawProperty

inline RawObject RawProperty::getter() const {
  return instanceVariableAt(kGetterOffset);
}

inline void RawProperty::setGetter(RawObject function) const {
  instanceVariableAtPut(kGetterOffset, function);
}

inline RawObject RawProperty::setter() const {
  return instanceVariableAt(kSetterOffset);
}

inline void RawProperty::setSetter(RawObject function) const {
  instanceVariableAtPut(kSetterOffset, function);
}

inline RawObject RawProperty::deleter() const {
  return instanceVariableAt(kDeleterOffset);
}

inline void RawProperty::setDeleter(RawObject function) const {
  instanceVariableAtPut(kDeleterOffset, function);
}

// RawRangeIterator

inline void RawRangeIterator::setRange(RawObject range) const {
  auto r = RawRange::cast(range);
  instanceVariableAtPut(kRangeOffset, r);
  instanceVariableAtPut(kCurValueOffset, RawSmallInt::fromWord(r.start()));
}

// RawSlice

inline RawObject RawSlice::start() const {
  return instanceVariableAt(kStartOffset);
}

inline void RawSlice::setStart(RawObject value) const {
  instanceVariableAtPut(kStartOffset, value);
}

inline RawObject RawSlice::stop() const {
  return instanceVariableAt(kStopOffset);
}

inline void RawSlice::setStop(RawObject value) const {
  instanceVariableAtPut(kStopOffset, value);
}

inline RawObject RawSlice::step() const {
  return instanceVariableAt(kStepOffset);
}

inline void RawSlice::setStep(RawObject value) const {
  instanceVariableAtPut(kStepOffset, value);
}

// RawStaticMethod

inline RawObject RawStaticMethod::function() const {
  return instanceVariableAt(kFunctionOffset);
}

inline void RawStaticMethod::setFunction(RawObject function) const {
  instanceVariableAtPut(kFunctionOffset, function);
}

// RawByteArray

inline byte RawByteArray::byteAt(word index) const {
  DCHECK_INDEX(index, numItems());
  return RawBytes::cast(bytes()).byteAt(index);
}

inline void RawByteArray::byteAtPut(word index, byte value) const {
  DCHECK_INDEX(index, numItems());
  RawBytes::cast(bytes()).byteAtPut(index, value);
}

inline word RawByteArray::numItems() const {
  return RawSmallInt::cast(instanceVariableAt(kNumItemsOffset)).value();
}

inline void RawByteArray::setNumItems(word num_bytes) const {
  instanceVariableAtPut(kNumItemsOffset, RawSmallInt::fromWord(num_bytes));
}

inline RawObject RawByteArray::bytes() const {
  return instanceVariableAt(kBytesOffset);
}

inline void RawByteArray::setBytes(RawObject new_bytes) const {
  instanceVariableAtPut(kBytesOffset, new_bytes);
}

inline word RawByteArray::capacity() const {
  return RawBytes::cast(bytes()).length();
}

// RawDict

inline word RawDict::numItems() const {
  return RawSmallInt::cast(instanceVariableAt(kNumItemsOffset)).value();
}

inline void RawDict::setNumItems(word num_items) const {
  instanceVariableAtPut(kNumItemsOffset, RawSmallInt::fromWord(num_items));
}

inline RawObject RawDict::data() const {
  return instanceVariableAt(kDataOffset);
}

inline void RawDict::setData(RawObject data) const {
  instanceVariableAtPut(kDataOffset, data);
}

// RawDictIteratorBase

inline word RawDictIteratorBase::numFound() const {
  return RawSmallInt::cast(instanceVariableAt(kNumFoundOffset)).value();
}

inline void RawDictIteratorBase::setNumFound(word num_found) const {
  instanceVariableAtPut(kNumFoundOffset, RawSmallInt::fromWord(num_found));
}

// RawDictViewBase

inline RawObject RawDictViewBase::dict() const {
  return instanceVariableAt(kDictOffset);
}

inline void RawDictViewBase::setDict(RawObject dict) const {
  instanceVariableAtPut(kDictOffset, dict);
}

// RawFunction

inline RawObject RawFunction::annotations() const {
  return instanceVariableAt(kAnnotationsOffset);
}

inline void RawFunction::setAnnotations(RawObject annotations) const {
  return instanceVariableAtPut(kAnnotationsOffset, annotations);
}

inline RawObject RawFunction::closure() const {
  return instanceVariableAt(kClosureOffset);
}

inline void RawFunction::setClosure(RawObject closure) const {
  return instanceVariableAtPut(kClosureOffset, closure);
}

inline RawObject RawFunction::code() const {
  return instanceVariableAt(kCodeOffset);
}

inline void RawFunction::setCode(RawObject code) const {
  return instanceVariableAtPut(kCodeOffset, code);
}

inline RawObject RawFunction::defaults() const {
  return instanceVariableAt(kDefaultsOffset);
}

inline void RawFunction::setDefaults(RawObject defaults) const {
  return instanceVariableAtPut(kDefaultsOffset, defaults);
}

inline bool RawFunction::hasDefaults() const {
  return !defaults().isNoneType();
}

inline RawObject RawFunction::doc() const {
  return instanceVariableAt(kDocOffset);
}

inline void RawFunction::setDoc(RawObject doc) const {
  instanceVariableAtPut(kDocOffset, doc);
}

inline RawFunction::Entry RawFunction::entry() const {
  RawObject object = instanceVariableAt(kEntryOffset);
  DCHECK(object.isSmallInt(), "entry address must look like a RawSmallInt");
  return bit_cast<RawFunction::Entry>(object);
}

inline void RawFunction::setEntry(RawFunction::Entry thunk) const {
  auto object = RawSmallInt::fromFunctionPointer(thunk);
  instanceVariableAtPut(kEntryOffset, object);
}

inline RawFunction::Entry RawFunction::entryKw() const {
  RawObject object = instanceVariableAt(kEntryKwOffset);
  DCHECK(object.isSmallInt(), "entryKw address must look like a RawSmallInt");
  return bit_cast<RawFunction::Entry>(object);
}

inline void RawFunction::setEntryKw(RawFunction::Entry thunk) const {
  auto object = RawSmallInt::fromFunctionPointer(thunk);
  instanceVariableAtPut(kEntryKwOffset, object);
}

RawFunction::Entry RawFunction::entryEx() const {
  RawObject object = instanceVariableAt(kEntryExOffset);
  DCHECK(object.isSmallInt(), "entryEx address must look like a RawSmallInt");
  return bit_cast<RawFunction::Entry>(object);
}

void RawFunction::setEntryEx(RawFunction::Entry thunk) const {
  auto object = RawSmallInt::fromFunctionPointer(thunk);
  instanceVariableAtPut(kEntryExOffset, object);
}

inline RawObject RawFunction::globals() const {
  return instanceVariableAt(kGlobalsOffset);
}

inline void RawFunction::setGlobals(RawObject globals) const {
  return instanceVariableAtPut(kGlobalsOffset, globals);
}

inline RawObject RawFunction::kwDefaults() const {
  return instanceVariableAt(kKwDefaultsOffset);
}

inline void RawFunction::setKwDefaults(RawObject kw_defaults) const {
  return instanceVariableAtPut(kKwDefaultsOffset, kw_defaults);
}

inline RawObject RawFunction::module() const {
  return instanceVariableAt(kModuleOffset);
}

inline void RawFunction::setModule(RawObject module) const {
  return instanceVariableAtPut(kModuleOffset, module);
}

inline RawObject RawFunction::name() const {
  return instanceVariableAt(kNameOffset);
}

inline void RawFunction::setName(RawObject name) const {
  instanceVariableAtPut(kNameOffset, name);
}

inline RawObject RawFunction::qualname() const {
  return instanceVariableAt(kQualnameOffset);
}

inline void RawFunction::setQualname(RawObject qualname) const {
  instanceVariableAtPut(kQualnameOffset, qualname);
}

inline RawObject RawFunction::fastGlobals() const {
  return instanceVariableAt(kFastGlobalsOffset);
}

inline void RawFunction::setFastGlobals(RawObject fast_globals) const {
  return instanceVariableAtPut(kFastGlobalsOffset, fast_globals);
}

inline RawObject RawFunction::dict() const {
  return instanceVariableAt(kDictOffset);
}

inline void RawFunction::setDict(RawObject dict) const {
  instanceVariableAtPut(kDictOffset, dict);
}

// RawInstance

inline word RawInstance::allocationSize(word num_attr) {
  DCHECK(num_attr >= 0, "invalid number of attributes %ld", num_attr);
  word size = headerSize(num_attr) + num_attr * kPointerSize;
  return Utils::maximum(kMinimumSize, Utils::roundUp(size, kPointerSize));
}

// RawList

inline RawObject RawList::items() const {
  return instanceVariableAt(kItemsOffset);
}

inline void RawList::setItems(RawObject new_items) const {
  instanceVariableAtPut(kItemsOffset, new_items);
}

inline word RawList::capacity() const {
  return RawTuple::cast(items()).length();
}

inline word RawList::numItems() const {
  return RawSmallInt::cast(instanceVariableAt(kAllocatedOffset)).value();
}

inline void RawList::setNumItems(word num_items) const {
  instanceVariableAtPut(kAllocatedOffset, RawSmallInt::fromWord(num_items));
}

inline void RawList::atPut(word index, RawObject value) const {
  DCHECK_INDEX(index, numItems());
  RawObject items = instanceVariableAt(kItemsOffset);
  RawTuple::cast(items).atPut(index, value);
}

inline RawObject RawList::at(word index) const {
  DCHECK_INDEX(index, numItems());
  return RawTuple::cast(items()).at(index);
}

// RawModule

inline RawObject RawModule::name() const {
  return instanceVariableAt(kNameOffset);
}

inline void RawModule::setName(RawObject name) const {
  instanceVariableAtPut(kNameOffset, name);
}

inline RawObject RawModule::dict() const {
  return instanceVariableAt(kDictOffset);
}

inline void RawModule::setDict(RawObject dict) const {
  instanceVariableAtPut(kDictOffset, dict);
}

inline RawObject RawModule::def() const {
  return instanceVariableAt(kDefOffset);
}

inline void RawModule::setDef(RawObject dict) const {
  instanceVariableAtPut(kDefOffset, dict);
}

// RawStr

inline RawStr RawStr::empty() {
  return RawSmallStr::fromCStr("").rawCast<RawStr>();
}

inline byte RawStr::charAt(word index) const {
  if (isSmallStr()) {
    return RawSmallStr::cast(*this).charAt(index);
  }
  DCHECK(isLargeStr(), "unexpected type");
  return RawLargeStr::cast(*this).charAt(index);
}

inline word RawStr::length() const {
  if (isSmallStr()) {
    return RawSmallStr::cast(*this).length();
  }
  DCHECK(isLargeStr(), "unexpected type");
  return RawLargeStr::cast(*this).length();
}

inline bool RawStr::equals(RawObject that) const {
  if (isSmallStr()) {
    return *this == that;
  }
  DCHECK(isLargeStr(), "unexpected type");
  return RawLargeStr::cast(*this).equals(that);
}

inline void RawStr::copyTo(byte* dst, word length) const {
  if (isSmallStr()) {
    RawSmallStr::cast(*this).copyTo(dst, length);
    return;
  }
  DCHECK(isLargeStr(), "unexpected type");
  return RawLargeStr::cast(*this).copyTo(dst, length);
}

inline char* RawStr::toCStr() const {
  if (isSmallStr()) {
    return RawSmallStr::cast(*this).toCStr();
  }
  DCHECK(isLargeStr(), "unexpected type");
  return RawLargeStr::cast(*this).toCStr();
}

inline word RawStr::codePointLength() const {
  if (isSmallStr()) {
    return RawSmallStr::cast(*this).codePointLength();
  }
  DCHECK(isLargeStr(), "unexpected type");
  return RawLargeStr::cast(*this).codePointLength();
}

// RawLargeStr

inline word RawLargeStr::allocationSize(word length) {
  DCHECK(length > RawSmallStr::kMaxLength, "length %ld overflows", length);
  word size = headerSize(length) + length;
  return Utils::maximum(kMinimumSize, Utils::roundUp(size, kPointerSize));
}

inline byte RawLargeStr::charAt(word index) const {
  DCHECK_INDEX(index, length());
  return *reinterpret_cast<byte*>(address() + index);
}

// RawValueCell

inline RawObject RawValueCell::value() const {
  return instanceVariableAt(kValueOffset);
}

inline void RawValueCell::setValue(RawObject object) const {
  instanceVariableAtPut(kValueOffset, object);
}

inline bool RawValueCell::isUnbound() const { return *this == value(); }

inline void RawValueCell::makeUnbound() const { setValue(*this); }

// RawSetBase

inline word RawSetBase::numItems() const {
  return RawSmallInt::cast(instanceVariableAt(kNumItemsOffset)).value();
}

inline void RawSetBase::setNumItems(word num_items) const {
  instanceVariableAtPut(kNumItemsOffset, RawSmallInt::fromWord(num_items));
}

inline RawObject RawSetBase::data() const {
  return instanceVariableAt(kDataOffset);
}

inline void RawSetBase::setData(RawObject data) const {
  instanceVariableAtPut(kDataOffset, data);
}

// RawBoundMethod

inline RawObject RawBoundMethod::function() const {
  return instanceVariableAt(kFunctionOffset);
}

inline void RawBoundMethod::setFunction(RawObject function) const {
  instanceVariableAtPut(kFunctionOffset, function);
}

inline RawObject RawBoundMethod::self() const {
  return instanceVariableAt(kSelfOffset);
}

inline void RawBoundMethod::setSelf(RawObject self) const {
  instanceVariableAtPut(kSelfOffset, self);
}

// RawClassMethod

inline RawObject RawClassMethod::function() const {
  return instanceVariableAt(kFunctionOffset);
}

inline void RawClassMethod::setFunction(RawObject function) const {
  instanceVariableAtPut(kFunctionOffset, function);
}

// RawWeakRef

inline RawObject RawWeakRef::referent() const {
  return instanceVariableAt(kReferentOffset);
}

inline void RawWeakRef::setReferent(RawObject referent) const {
  instanceVariableAtPut(kReferentOffset, referent);
}

inline RawObject RawWeakRef::callback() const {
  return instanceVariableAt(kCallbackOffset);
}

inline void RawWeakRef::setCallback(RawObject callable) const {
  instanceVariableAtPut(kCallbackOffset, callable);
}

inline RawObject RawWeakRef::link() const {
  return instanceVariableAt(kLinkOffset);
}

inline void RawWeakRef::setLink(RawObject reference) const {
  instanceVariableAtPut(kLinkOffset, reference);
}

// RawLayout

inline LayoutId RawLayout::id() const {
  return static_cast<LayoutId>(header().hashCode());
}

inline void RawLayout::setId(LayoutId id) const {
  setHeader(header().withHashCode(static_cast<word>(id)));
}

inline void RawLayout::setDescribedType(RawObject type) const {
  instanceVariableAtPut(kDescribedTypeOffset, type);
}

inline RawObject RawLayout::describedType() const {
  return instanceVariableAt(kDescribedTypeOffset);
}

inline void RawLayout::setInObjectAttributes(RawObject attributes) const {
  instanceVariableAtPut(kInObjectAttributesOffset, attributes);
}

inline RawObject RawLayout::inObjectAttributes() const {
  return instanceVariableAt(kInObjectAttributesOffset);
}

inline void RawLayout::setOverflowAttributes(RawObject attributes) const {
  instanceVariableAtPut(kOverflowAttributesOffset, attributes);
}

inline word RawLayout::instanceSize() const {
  word instance_size_in_words =
      numInObjectAttributes() + !overflowAttributes().isNoneType();
  return instance_size_in_words * kPointerSize;
}

inline RawObject RawLayout::overflowAttributes() const {
  return instanceVariableAt(kOverflowAttributesOffset);
}

inline void RawLayout::setAdditions(RawObject additions) const {
  instanceVariableAtPut(kAdditionsOffset, additions);
}

inline RawObject RawLayout::additions() const {
  return instanceVariableAt(kAdditionsOffset);
}

inline void RawLayout::setDeletions(RawObject deletions) const {
  instanceVariableAtPut(kDeletionsOffset, deletions);
}

inline RawObject RawLayout::deletions() const {
  return instanceVariableAt(kDeletionsOffset);
}

inline word RawLayout::overflowOffset() const {
  return numInObjectAttributes() * kPointerSize;
}

inline word RawLayout::numInObjectAttributes() const {
  return RawSmallInt::cast(instanceVariableAt(kNumInObjectAttributesOffset))
      .value();
}

inline void RawLayout::setNumInObjectAttributes(word count) const {
  instanceVariableAtPut(kNumInObjectAttributesOffset,
                        RawSmallInt::fromWord(count));
}

inline void RawLayout::sealAttributes() const {
  setOverflowAttributes(RawNoneType::object());
}

// RawSetIterator

inline word RawSetIterator::consumedCount() const {
  return RawSmallInt::cast(instanceVariableAt(kConsumedCountOffset)).value();
}

inline void RawSetIterator::setConsumedCount(word consumed) const {
  instanceVariableAtPut(kConsumedCountOffset, RawSmallInt::fromWord(consumed));
}

// RawIteratorBase

inline RawObject RawIteratorBase::iterable() const {
  return instanceVariableAt(kIterableOffset);
}

inline void RawIteratorBase::setIterable(RawObject iterable) const {
  instanceVariableAtPut(kIterableOffset, iterable);
}

inline word RawIteratorBase::index() const {
  return RawSmallInt::cast(instanceVariableAt(kIndexOffset)).value();
}

inline void RawIteratorBase::setIndex(word index) const {
  instanceVariableAtPut(kIndexOffset, RawSmallInt::fromWord(index));
}

// RawSuper

inline RawObject RawSuper::type() const {
  return instanceVariableAt(kTypeOffset);
}

inline void RawSuper::setType(RawObject tp) const {
  DCHECK(tp.isType(), "expected type");
  instanceVariableAtPut(kTypeOffset, tp);
}

inline RawObject RawSuper::object() const {
  return instanceVariableAt(kObjectOffset);
}

inline void RawSuper::setObject(RawObject obj) const {
  instanceVariableAtPut(kObjectOffset, obj);
}

inline RawObject RawSuper::objectType() const {
  return instanceVariableAt(kObjectTypeOffset);
}

inline void RawSuper::setObjectType(RawObject tp) const {
  DCHECK(tp.isType(), "expected type");
  instanceVariableAtPut(kObjectTypeOffset, tp);
}

// RawTupleIterator

inline word RawTupleIterator::tupleLength() const {
  return RawSmallInt::cast(instanceVariableAt(kTupleLengthOffset)).value();
}

inline void RawTupleIterator::setTupleLength(word length) const {
  instanceVariableAtPut(kTupleLengthOffset, RawSmallInt::fromWord(length));
}

// RawExceptionState

inline RawObject RawExceptionState::type() const {
  return instanceVariableAt(kTypeOffset);
}

inline RawObject RawExceptionState::value() const {
  return instanceVariableAt(kValueOffset);
}

inline RawObject RawExceptionState::traceback() const {
  return instanceVariableAt(kTracebackOffset);
}

inline void RawExceptionState::setType(RawObject type) const {
  instanceVariableAtPut(kTypeOffset, type);
}

inline void RawExceptionState::setValue(RawObject value) const {
  instanceVariableAtPut(kValueOffset, value);
}

inline void RawExceptionState::setTraceback(RawObject tb) const {
  instanceVariableAtPut(kTracebackOffset, tb);
}

inline RawObject RawExceptionState::previous() const {
  return instanceVariableAt(kPreviousOffset);
}

inline void RawExceptionState::setPrevious(RawObject prev) const {
  instanceVariableAtPut(kPreviousOffset, prev);
}

// RawGeneratorBase

inline RawObject RawGeneratorBase::heapFrame() const {
  return instanceVariableAt(kFrameOffset);
}

inline void RawGeneratorBase::setHeapFrame(RawObject obj) const {
  instanceVariableAtPut(kFrameOffset, obj);
}

inline RawObject RawGeneratorBase::exceptionState() const {
  return instanceVariableAt(kExceptionStateOffset);
}

inline void RawGeneratorBase::setExceptionState(RawObject obj) const {
  instanceVariableAtPut(kExceptionStateOffset, obj);
}

// RawHeapFrame

inline Frame* RawHeapFrame::frame() const {
  return reinterpret_cast<Frame*>(address() + kFrameOffset +
                                  maxStackSize() * kPointerSize);
}

inline word RawHeapFrame::numFrameWords() const {
  return header().count() - kNumOverheadWords;
}

inline word RawHeapFrame::maxStackSize() const {
  return RawSmallInt::cast(instanceVariableAt(kMaxStackSizeOffset)).value();
}

inline void RawHeapFrame::setMaxStackSize(word offset) const {
  instanceVariableAtPut(kMaxStackSizeOffset, RawSmallInt::fromWord(offset));
}

}  // namespace python
