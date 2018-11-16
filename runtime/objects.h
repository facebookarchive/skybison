#pragma once

#include <limits>

#include "globals.h"
#include "utils.h"
#include "view.h"

namespace python {

#define INTRINSIC_IMMEDIATE_CLASS_NAMES(V)                                     \
  V(SmallInt)                                                                  \
  V(SmallStr)                                                                  \
  V(Bool)                                                                      \
  V(NoneType)

#define INTRINSIC_HEAP_CLASS_NAMES(V)                                          \
  V(Object)                                                                    \
  V(ArithmeticError)                                                           \
  V(AssertionError)                                                            \
  V(AttributeError)                                                            \
  V(BaseException)                                                             \
  V(BlockingIOError)                                                           \
  V(BoundMethod)                                                               \
  V(BrokenPipeError)                                                           \
  V(BufferError)                                                               \
  V(Bytes)                                                                     \
  V(BytesWarning)                                                              \
  V(ChildProcessError)                                                         \
  V(ClassMethod)                                                               \
  V(Code)                                                                      \
  V(Complex)                                                                   \
  V(ConnectionAbortedError)                                                    \
  V(ConnectionError)                                                           \
  V(ConnectionRefusedError)                                                    \
  V(ConnectionResetError)                                                      \
  V(Coroutine)                                                                 \
  V(DeprecationWarning)                                                        \
  V(Dict)                                                                      \
  V(EOFError)                                                                  \
  V(Ellipsis)                                                                  \
  V(Exception)                                                                 \
  V(FileExistsError)                                                           \
  V(FileNotFoundError)                                                         \
  V(Float)                                                                     \
  V(FloatingPointError)                                                        \
  V(Function)                                                                  \
  V(FutureWarning)                                                             \
  V(Generator)                                                                 \
  V(GeneratorExit)                                                             \
  V(HeapFrame)                                                                 \
  V(ImportError)                                                               \
  V(ImportWarning)                                                             \
  V(IndentationError)                                                          \
  V(IndexError)                                                                \
  V(Int)                                                                       \
  V(InterruptedError)                                                          \
  V(IsADirectoryError)                                                         \
  V(KeyError)                                                                  \
  V(KeyboardInterrupt)                                                         \
  V(LargeInt)                                                                  \
  V(LargeStr)                                                                  \
  V(Layout)                                                                    \
  V(List)                                                                      \
  V(ListIterator)                                                              \
  V(LookupError)                                                               \
  V(MemoryError)                                                               \
  V(Module)                                                                    \
  V(ModuleNotFoundError)                                                       \
  V(NameError)                                                                 \
  V(NotADirectoryError)                                                        \
  V(NotImplemented)                                                            \
  V(NotImplementedError)                                                       \
  V(OSError)                                                                   \
  V(ObjectArray)                                                               \
  V(OverflowError)                                                             \
  V(PendingDeprecationWarning)                                                 \
  V(PermissionError)                                                           \
  V(ProcessLookupError)                                                        \
  V(Property)                                                                  \
  V(Range)                                                                     \
  V(RangeIterator)                                                             \
  V(RecursionError)                                                            \
  V(ReferenceError)                                                            \
  V(ResourceWarning)                                                           \
  V(RuntimeError)                                                              \
  V(RuntimeWarning)                                                            \
  V(Set)                                                                       \
  V(SetIterator)                                                               \
  V(Slice)                                                                     \
  V(StaticMethod)                                                              \
  V(StopAsyncIteration)                                                        \
  V(StopIteration)                                                             \
  V(Str)                                                                       \
  V(Super)                                                                     \
  V(SyntaxError)                                                               \
  V(SyntaxWarning)                                                             \
  V(SystemError)                                                               \
  V(SystemExit)                                                                \
  V(TabError)                                                                  \
  V(TimeoutError)                                                              \
  V(TupleIterator)                                                             \
  V(Type)                                                                      \
  V(TypeError)                                                                 \
  V(UnboundLocalError)                                                         \
  V(UnicodeDecodeError)                                                        \
  V(UnicodeEncodeError)                                                        \
  V(UnicodeError)                                                              \
  V(UnicodeTranslateError)                                                     \
  V(UnicodeWarning)                                                            \
  V(UserWarning)                                                               \
  V(ValueCell)                                                                 \
  V(ValueError)                                                                \
  V(Warning)                                                                   \
  V(WeakRef)                                                                   \
  V(ZeroDivisionError)

#define INTRINSIC_CLASS_NAMES(V)                                               \
  INTRINSIC_IMMEDIATE_CLASS_NAMES(V)                                           \
  INTRINSIC_HEAP_CLASS_NAMES(V)

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
  // Immediate objects - note that the SmallInt class is also aliased to
  // all even integers less than 32, so that classes of immediate objects can
  // be looked up simply by using the low 5 bits of the immediate value. This
  // implies that all other immediate class ids must be odd.
  kSmallInt = 0,
  kBool = 7,
  kNoneType = 15,
  // there is no class associated with the Error object type, this is here as a
  // placeholder.
  kError = 23,
  kSmallStr = 31,

  // Heap objects
  kObject = 32,
  kArithmeticError,
  kAssertionError,
  kAttributeError,
  kBaseException,
  kBlockingIOError,
  kBoundMethod,
  kBrokenPipeError,
  kBufferError,
  kBytes,
  kBytesWarning,
  kChildProcessError,
  kClassMethod,
  kCode,
  kComplex,
  kConnectionAbortedError,
  kConnectionError,
  kConnectionRefusedError,
  kConnectionResetError,
  kCoroutine,
  kDeprecationWarning,
  kDict,
  kEOFError,
  kEllipsis,
  kException,
  kFileExistsError,
  kFileNotFoundError,
  kFloat,
  kFloatingPointError,
  kFunction,
  kFutureWarning,
  kGenerator,
  kGeneratorExit,
  kHeapFrame,
  kImportError,
  kImportWarning,
  kIndentationError,
  kIndexError,
  kInt,
  kInterruptedError,
  kIsADirectoryError,
  kKeyError,
  kKeyboardInterrupt,
  kLargeInt,
  kLargeStr,
  kLayout,
  kList,
  kListIterator,
  kLookupError,
  kMemoryError,
  kModule,
  kModuleNotFoundError,
  kNameError,
  kNotADirectoryError,
  kNotImplemented,
  kNotImplementedError,
  kOSError,
  kObjectArray,
  kOverflowError,
  kPendingDeprecationWarning,
  kPermissionError,
  kProcessLookupError,
  kProperty,
  kRange,
  kRangeIterator,
  kRecursionError,
  kReferenceError,
  kResourceWarning,
  kRuntimeError,
  kRuntimeWarning,
  kSet,
  kSetIterator,
  kSlice,
  kStaticMethod,
  kStopAsyncIteration,
  kStopIteration,
  kStr,
  kSuper,
  kSyntaxError,
  kSyntaxWarning,
  kSystemError,
  kSystemExit,
  kTabError,
  kTimeoutError,
  kTupleIterator,
  kType,
  kTypeError,
  kUnboundLocalError,
  kUnicodeDecodeError,
  kUnicodeEncodeError,
  kUnicodeError,
  kUnicodeTranslateError,
  kUnicodeWarning,
  kUserWarning,
  kValueCell,
  kValueError,
  kWarning,
  kWeakRef,
  kZeroDivisionError,

  kLastBuiltinId = kZeroDivisionError,
};

// Map from type to its corresponding LayoutId:
// ObjectLayoutId<SmallInt>::value == LayoutId::kSmallInt, etc.
template <typename T>
struct ObjectLayoutId;
#define CASE(ty)                                                               \
  template <>                                                                  \
  struct ObjectLayoutId<class ty> {                                            \
    static constexpr LayoutId value = LayoutId::k##ty;                         \
  };
INTRINSIC_CLASS_NAMES(CASE)
#undef CASE

// Add functionality common to all Object subclasses, split into two parts since
// some types manually define cast() but want everything else.
#define RAW_OBJECT_COMMON_NO_CAST(ty)                                          \
  /* TODO(T34683229) The const_cast here is temporary for a migration */       \
  Raw##ty* operator->() const { return const_cast<Raw##ty*>(this); }           \
  DISALLOW_HEAP_ALLOCATION();

#define RAW_OBJECT_COMMON(ty)                                                  \
  RAW_OBJECT_COMMON_NO_CAST(ty)                                                \
  static Raw##ty cast(RawObject object) {                                      \
    DCHECK(object.is##ty(), "invalid cast, expected " #ty);                    \
    return bit_cast<Raw##ty>(object);                                          \
  }

// TODO(T34683229): These typedefs are temporary as part of an in-progress
// migration.
#define RAW_ALIAS(ty) using Raw##ty = class ty
RAW_ALIAS(Object);
RAW_ALIAS(Int);
RAW_ALIAS(SmallInt);
RAW_ALIAS(Header);
RAW_ALIAS(Bool);
RAW_ALIAS(NoneType);
RAW_ALIAS(Error);
RAW_ALIAS(Str);
RAW_ALIAS(SmallStr);
RAW_ALIAS(HeapObject);
RAW_ALIAS(BaseException);
RAW_ALIAS(Exception);
RAW_ALIAS(StopIteration);
RAW_ALIAS(SystemExit);
RAW_ALIAS(RuntimeError);
RAW_ALIAS(NotImplementedError);
RAW_ALIAS(ImportError);
RAW_ALIAS(ModuleNotFoundError);
RAW_ALIAS(LookupError);
RAW_ALIAS(IndexError);
RAW_ALIAS(KeyError);
RAW_ALIAS(Type);
RAW_ALIAS(Array);
RAW_ALIAS(Bytes);
RAW_ALIAS(ObjectArray);
RAW_ALIAS(LargeStr);
RAW_ALIAS(LargeInt);
RAW_ALIAS(Float);
RAW_ALIAS(Complex);
RAW_ALIAS(Property);
RAW_ALIAS(Range);
RAW_ALIAS(RangeIterator);
RAW_ALIAS(Slice);
RAW_ALIAS(StaticMethod);
RAW_ALIAS(ListIterator);
RAW_ALIAS(SetIterator);
RAW_ALIAS(TupleIterator);
RAW_ALIAS(Code);
RAW_ALIAS(Function);
RAW_ALIAS(Instance);
RAW_ALIAS(Module);
RAW_ALIAS(NotImplemented);
RAW_ALIAS(Dict);
RAW_ALIAS(Set);
RAW_ALIAS(List);
RAW_ALIAS(ValueCell);
RAW_ALIAS(Ellipsis);
RAW_ALIAS(WeakRef);
RAW_ALIAS(BoundMethod);
RAW_ALIAS(ClassMethod);
RAW_ALIAS(Layout);
RAW_ALIAS(Super);
RAW_ALIAS(GeneratorBase);
RAW_ALIAS(Generator);
RAW_ALIAS(Coroutine);
RAW_ALIAS(HeapFrame);
#undef RAW_ALIAS

class Object {
 public:
  // TODO(bsimmers): Delete this. The default constructor gives you a
  // zero-initialized Object, which is equivalent to SmallInt::fromWord(0). This
  // behavior can be confusing and surprising, so we should just require all
  // Objects to be explicitly initialized.
  Object() = default;

  explicit Object(uword raw);

  // Getters and setters.
  uword raw() const;
  bool isObject();
  LayoutId layoutId();

  // Immediate objects
  bool isBool();
  bool isError();
  bool isHeader();
  bool isNoneType();
  bool isSmallInt();
  bool isSmallStr();

  // Heap objects
  bool isBaseException();
  bool isBoundMethod();
  bool isBytes();
  bool isType();
  bool isClassMethod();
  bool isCode();
  bool isComplex();
  bool isCoroutine();
  bool isDict();
  bool isEllipsis();
  bool isException();
  bool isFloat();
  bool isHeapFrame();
  bool isFunction();
  bool isGenerator();
  bool isHeapObject();
  bool isHeapObjectWithLayout(LayoutId layout_id);
  bool isImportError();
  bool isIndexError();
  bool isInstance();
  bool isKeyError();
  bool isLargeInt();
  bool isLargeStr();
  bool isLayout();
  bool isList();
  bool isListIterator();
  bool isLookupError();
  bool isModule();
  bool isModuleNotFoundError();
  bool isNotImplemented();
  bool isNotImplementedError();
  bool isObjectArray();
  bool isProperty();
  bool isRange();
  bool isRangeIterator();
  bool isGeneratorBase();
  bool isRuntimeError();
  bool isSet();
  bool isSetIterator();
  bool isSlice();
  bool isStaticMethod();
  bool isStopIteration();
  bool isSuper();
  bool isSystemExit();
  bool isTupleIterator();
  bool isValueCell();
  bool isWeakRef();

  // superclass objects
  bool isInt();
  bool isStr();

  static bool equals(RawObject lhs, RawObject rhs);

  bool operator==(const RawObject& other) const;
  bool operator!=(const RawObject& other) const;

  // Constants

  // The bottom five bits of immediate objects are used as the class id when
  // indexing into the class table in the runtime.
  static const uword kImmediateClassTableIndexMask = (1 << 5) - 1;

  RAW_OBJECT_COMMON(Object)

 private:
  // Zero-initializing raw_ gives SmallInt::fromWord(0).
  uword raw_{};
};

// CastError and OptInt<T> represent the result of a call to Int::asInt<T>(): If
// error == CastError::None, value contains the result. Otherwise, error
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

// Generic wrapper around SmallInt/LargeInt.
class Int : public Object {
 public:
  // Getters and setters.
  word asWord();
  void* asCPtr();

  // If this fits in T, get its value as a T. If not, indicate what went wrong.
  template <typename T>
  OptInt<T> asInt();

  word compare(RawInt other);

  double floatValue();

  word bitLength();

  bool isNegative();
  bool isPositive();
  bool isZero();

  RAW_OBJECT_COMMON(Int)

  // Indexing into digits
  word digitAt(word index);

  // Number of digits
  word numDigits();
};

// Immediate objects

class SmallInt : public Object {
 public:
  // Getters and setters.
  word value();
  void* asCPtr();

  // If this fits in T, get its value as a T. If not, indicate what went wrong.
  template <typename T>
  if_signed_t<T, OptInt<T>> asInt();
  template <typename T>
  if_unsigned_t<T, OptInt<T>> asInt();

  // Conversion.
  static RawSmallInt fromWord(word value);
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

  RAW_OBJECT_COMMON(SmallInt)
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
 * Header objects
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
 * Layout      20   identifier for the layout, allowing 2^20 unique layouts
 * Hash        30   bits to use for an identity hash code
 * Count        8   number of array elements or instance variables
 */
class Header : public Object {
 public:
  word hashCode();
  RawHeader withHashCode(word value);

  ObjectFormat format();

  LayoutId layoutId();
  RawHeader withLayoutId(LayoutId layout_id);

  word count();

  bool hasOverflow();

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

  RAW_OBJECT_COMMON(Header)
};

class Bool : public Object {
 public:
  // Getters and setters.
  bool value();

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

  RAW_OBJECT_COMMON(Bool)
};

class NoneType : public Object {
 public:
  // Singletons.
  static RawNoneType object();

  // Tags.
  static const int kTag = 15;  // 0b01111
  static const int kTagSize = 5;
  static const uword kTagMask = (1 << kTagSize) - 1;

  RAW_OBJECT_COMMON(NoneType)
};

// Error is a special object type, internal to the runtime. It is used to signal
// that an error has occurred inside the runtime or native code, e.g. an
// exception has been thrown.
class Error : public Object {
 public:
  // Singletons.
  static RawError object();

  // Tagging.
  static const int kTag = 23;  // 0b10111
  static const int kTagSize = 5;
  static const uword kTagMask = (1 << kTagSize) - 1;

  RAW_OBJECT_COMMON(Error)
};

// Super class of common string functionality
class Str : public Object {
 public:
  // Getters and setters.
  byte charAt(word index);
  word length();
  void copyTo(byte* dst, word length);

  // Equality checks.
  word compare(RawObject string);
  bool equals(RawObject that);
  bool equalsCStr(const char* c_str);

  // Conversion to an unescaped C string.  The underlying memory is allocated
  // with malloc and must be freed by the caller.
  char* toCStr();

  RAW_OBJECT_COMMON(Str)
};

class SmallStr : public Object {
 public:
  // Conversion.
  static RawObject fromCStr(const char* value);
  static RawObject fromBytes(View<byte> data);

  // Tagging.
  static const int kTag = 31;  // 0b11111
  static const int kTagSize = 5;
  static const uword kTagMask = (1 << kTagSize) - 1;

  static const word kMaxLength = kWordSize - 1;

  RAW_OBJECT_COMMON(SmallStr)

 private:
  // Interface methods are private: strings should be manipulated via the
  // Str class, which delegates to LargeStr/SmallStr appropriately.

  // Getters and setters.
  word length();
  byte charAt(word index);
  void copyTo(byte* dst, word length);

  // Conversion to an unescaped C string.  The underlying memory is allocated
  // with malloc and must be freed by the caller.
  char* toCStr();

  friend class Heap;
  friend class Object;
  friend class Runtime;
  friend class Str;
};

// Heap objects

class HeapObject : public Object {
 public:
  // Getters and setters.
  uword address();
  uword baseAddress();
  RawHeader header();
  void setHeader(RawHeader header);
  word headerOverflow();
  void setHeaderAndOverflow(word count, word hash, LayoutId id,
                            ObjectFormat format);
  word headerCountOrOverflow();
  word size();

  // Conversion.
  static RawHeapObject fromAddress(uword address);

  // Sizing
  static word headerSize(word count);

  void initialize(word size, RawObject value);

  // Garbage collection.
  bool isRoot();
  bool isForwarding();
  RawObject forward();
  void forwardTo(RawObject object);

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

  RAW_OBJECT_COMMON(HeapObject)

 protected:
  RawObject instanceVariableAt(word offset);
  void instanceVariableAtPut(word offset, RawObject value);

 private:
  friend class Runtime;
};

class BaseException : public HeapObject {
 public:
  // Getters and setters.
  RawObject args();
  void setArgs(RawObject args);

  RawObject traceback();
  void setTraceback(RawObject traceback);

  RawObject cause();
  void setCause(RawObject cause);

  RawObject context();
  void setContext(RawObject context);

  static const int kArgsOffset = HeapObject::kSize;
  static const int kTracebackOffset = kArgsOffset + kPointerSize;
  static const int kCauseOffset = kTracebackOffset + kPointerSize;
  static const int kContextOffset = kCauseOffset + kPointerSize;
  static const int kSize = kContextOffset + kPointerSize;

  RAW_OBJECT_COMMON(BaseException)
};

class Exception : public BaseException {
 public:
  RAW_OBJECT_COMMON(Exception)
};

class StopIteration : public BaseException {
 public:
  // Getters and setters.
  RawObject value();
  void setValue(RawObject value);

  static const int kValueOffset = BaseException::kSize;
  static const int kSize = kValueOffset + kPointerSize;

  RAW_OBJECT_COMMON(StopIteration)
};

class SystemExit : public BaseException {
 public:
  RawObject code();
  void setCode(RawObject code);

  static const int kCodeOffset = BaseException::kSize;
  static const int kSize = kCodeOffset + kPointerSize;

  RAW_OBJECT_COMMON(SystemExit)
};

class RuntimeError : public Exception {
 public:
  RAW_OBJECT_COMMON(RuntimeError)
};

class NotImplementedError : public RuntimeError {
 public:
  RAW_OBJECT_COMMON(NotImplementedError)
};

class ImportError : public Exception {
 public:
  // Getters and setters
  RawObject msg();
  void setMsg(RawObject msg);

  RawObject name();
  void setName(RawObject name);

  RawObject path();
  void setPath(RawObject path);

  static const int kMsgOffset = BaseException::kSize;
  static const int kNameOffset = kMsgOffset + kPointerSize;
  static const int kPathOffset = kNameOffset + kPointerSize;
  static const int kSize = kPathOffset + kPointerSize;

  RAW_OBJECT_COMMON(ImportError)
};

class ModuleNotFoundError : public ImportError {
 public:
  RAW_OBJECT_COMMON(ModuleNotFoundError)
};

class LookupError : public Exception {
 public:
  RAW_OBJECT_COMMON(LookupError)
};

class IndexError : public LookupError {
 public:
  RAW_OBJECT_COMMON(IndexError)
};

class KeyError : public LookupError {
 public:
  RAW_OBJECT_COMMON(KeyError)
};

class Type : public HeapObject {
 public:
  enum Flag : word {
    kBaseExceptionSubclass = 1 << 0,
    kComplexSubclass = 1 << 1,
    kDictSubclass = 1 << 2,
    kFloatSubclass = 1 << 3,
    kIntSubclass = 1 << 4,
    kListSubclass = 1 << 5,
    kSetSubclass = 1 << 6,
    kStopIterationSubclass = 1 << 7,
    kStrSubclass = 1 << 8,
    kSystemExitSubclass = 1 << 9,
    kTupleSubclass = 1 << 10,
    kTypeSubclass = 1 << 11,
    kLast = kTypeSubclass,
  };
  static_assert(Flag::kLast < SmallInt::kMaxValue,
                "Flags must be encodable in a SmallInt");

  // Getters and setters.
  RawObject instanceLayout();
  void setInstanceLayout(RawObject layout);

  RawObject mro();
  void setMro(RawObject object_array);

  RawObject name();
  void setName(RawObject name);

  RawObject flags();
  void setFlags(RawObject value);
  void setFlag(Flag flag);
  bool hasFlag(Flag flag);

  RawObject dict();
  void setDict(RawObject name);

  // Int holding a pointer to a PyTypeObject
  // Only set on classes that were initialized through PyType_Ready
  RawObject extensionType();
  void setExtensionType(RawObject pytype);

  // builtin base related
  RawObject builtinBaseClass();
  void setBuiltinBaseClass(RawObject base);

  bool isIntrinsicOrExtension();

  // Casting.
  static RawType cast(RawObject object);

  // Layout.
  static const int kMroOffset = HeapObject::kSize;
  static const int kInstanceLayoutOffset = kMroOffset + kPointerSize;
  static const int kNameOffset = kInstanceLayoutOffset + kPointerSize;
  static const int kFlagsOffset = kNameOffset + kPointerSize;
  static const int kDictOffset = kFlagsOffset + kPointerSize;
  static const int kBuiltinBaseClassOffset = kDictOffset + kPointerSize;
  static const int kExtensionTypeOffset =
      kBuiltinBaseClassOffset + kPointerSize;
  static const int kSize = kExtensionTypeOffset + kPointerSize;

  RAW_OBJECT_COMMON_NO_CAST(Type)
};

class Array : public HeapObject {
 public:
  word length();

  RAW_OBJECT_COMMON_NO_CAST(Array)
};

class Bytes : public Array {
 public:
  // Getters and setters.
  byte byteAt(word index);
  void byteAtPut(word index, byte value);

  // Sizing.
  static word allocationSize(word length);

  RAW_OBJECT_COMMON(Bytes)
};

class ObjectArray : public Array {
 public:
  // Getters and setters.
  RawObject at(word index);
  void atPut(word index, RawObject value);

  // Sizing.
  static word allocationSize(word length);

  void copyTo(RawObject dst);

  void replaceFromWith(word start, RawObject array);

  bool contains(RawObject object);

  RAW_OBJECT_COMMON(ObjectArray)
};

class LargeStr : public Array {
 public:
  // Sizing.
  static word allocationSize(word length);

  static const int kDataOffset = HeapObject::kSize;

  RAW_OBJECT_COMMON(LargeStr)

 private:
  // Interface methods are private: strings should be manipulated via the
  // Str class, which delegates to LargeStr/SmallStr appropriately.

  // Getters and setters.
  byte charAt(word index);
  void copyTo(byte* bytes, word length);

  // Equality checks.
  bool equals(RawObject that);

  // Conversion to an unescaped C string.  The underlying memory is allocated
  // with malloc and must be freed by the caller.
  char* toCStr();

  friend class Heap;
  friend class Object;
  friend class Runtime;
  friend class Str;
};

// Arbitrary precision signed integer, with 64 bit digits in two's complement
// representation
class LargeInt : public HeapObject {
 public:
  // Getters and setters.
  word asWord();

  // Return whether or not this LargeInt obeys the following invariants:
  // - numDigits() >= 1
  // - The value does not fit in a SmallInt
  // - Negative numbers do not have redundant sign-extended digits
  // - Positive numbers do not have redundant zero-extended digits
  bool isValid();

  // LargeInt is also used for storing native pointers.
  void* asCPtr();

  // If this fits in T, get its value as a T. If not, indicate what went wrong.
  template <typename T>
  if_signed_t<T, OptInt<T>> asInt();
  template <typename T>
  if_unsigned_t<T, OptInt<T>> asInt();

  // Sizing.
  static word allocationSize(word num_digits);

  // Indexing into digits
  word digitAt(word index);
  void digitAtPut(word index, word digit);

  bool isNegative();
  bool isPositive();

  word bitLength();

  // Number of digits
  word numDigits();

  // Layout.
  static const int kValueOffset = HeapObject::kSize;
  static const int kSize = kValueOffset + kPointerSize;

  RAW_OBJECT_COMMON(LargeInt)

 private:
  friend class Int;
  friend class Runtime;
};

class Float : public HeapObject {
 public:
  // Getters and setters.
  double value();

  // Allocation.
  void initialize(double value);

  // Layout.
  static const int kValueOffset = HeapObject::kSize;
  static const int kSize = kValueOffset + kDoubleSize;

  RAW_OBJECT_COMMON(Float)
};

class Complex : public HeapObject {
 public:
  // Getters
  double real();
  double imag();

  // Allocation.
  void initialize(double real, double imag);

  // Layout.
  static const int kRealOffset = HeapObject::kSize;
  static const int kImagOffset = kRealOffset + kDoubleSize;
  static const int kSize = kImagOffset + kDoubleSize;

  RAW_OBJECT_COMMON(Complex)
};

class Property : public HeapObject {
 public:
  // Getters and setters
  RawObject getter();
  void setGetter(RawObject function);

  RawObject setter();
  void setSetter(RawObject function);

  RawObject deleter();
  void setDeleter(RawObject function);

  // Layout
  static const int kGetterOffset = HeapObject::kSize;
  static const int kSetterOffset = kGetterOffset + kPointerSize;
  static const int kDeleterOffset = kSetterOffset + kPointerSize;
  static const int kSize = kDeleterOffset + kPointerSize;

  RAW_OBJECT_COMMON(Property)
};

class Range : public HeapObject {
 public:
  // Getters and setters.
  word start();
  void setStart(word value);

  word stop();
  void setStop(word value);

  word step();
  void setStep(word value);

  // Layout.
  static const int kStartOffset = HeapObject::kSize;
  static const int kStopOffset = kStartOffset + kPointerSize;
  static const int kStepOffset = kStopOffset + kPointerSize;
  static const int kSize = kStepOffset + kPointerSize;

  RAW_OBJECT_COMMON(Range)
};

class RangeIterator : public HeapObject {
 public:
  // Getters and setters.

  // Bind the iterator to a specific range. The binding should not be changed
  // after creation.
  void setRange(RawObject range);

  // Iteration.
  RawObject next();

  // Number of unconsumed values in the range iterator
  word pendingLength();

  // Layout.
  static const int kRangeOffset = HeapObject::kSize;
  static const int kCurValueOffset = kRangeOffset + kPointerSize;
  static const int kSize = kCurValueOffset + kPointerSize;

  RAW_OBJECT_COMMON(RangeIterator)

 private:
  static bool isOutOfRange(word cur, word stop, word step);
};

class Slice : public HeapObject {
 public:
  // Getters and setters.
  RawObject start();
  void setStart(RawObject value);

  RawObject stop();
  void setStop(RawObject value);

  RawObject step();
  void setStep(RawObject value);

  // Returns the correct start, stop, and step word values from this slice
  void unpack(word* start, word* stop, word* step);

  // Takes in the length of a list and the start, stop, and step values
  // Returns the length of the new list and the corrected start and stop values
  static word adjustIndices(word length, word* start, word* stop, word step);

  // Layout.
  static const int kStartOffset = HeapObject::kSize;
  static const int kStopOffset = kStartOffset + kPointerSize;
  static const int kStepOffset = kStopOffset + kPointerSize;
  static const int kSize = kStepOffset + kPointerSize;

  RAW_OBJECT_COMMON(Slice)
};

class StaticMethod : public HeapObject {
 public:
  // Getters and setters
  RawObject function();
  void setFunction(RawObject function);

  // Layout
  static const int kFunctionOffset = HeapObject::kSize;
  static const int kSize = kFunctionOffset + kPointerSize;

  RAW_OBJECT_COMMON(StaticMethod)
};

class ListIterator : public HeapObject {
 public:
  // Getters and setters.
  word index();
  void setIndex(word index);

  RawObject list();
  void setList(RawObject list);

  // Iteration.
  RawObject next();

  // Layout.
  static const int kListOffset = HeapObject::kSize;
  static const int kIndexOffset = kListOffset + kPointerSize;
  static const int kSize = kIndexOffset + kPointerSize;

  RAW_OBJECT_COMMON(ListIterator)
};

class SetIterator : public HeapObject {
 public:
  // Getters and setters
  word consumedCount();
  void setConsumedCount(word consumed);

  word index();
  void setIndex(word index);

  RawObject set();
  void setSet(RawObject set);

  // Iteration.
  RawObject next();

  // Number of unconsumed values in the set iterator
  word pendingLength();

  // Layout
  static const int kSetOffset = HeapObject::kSize;
  static const int kIndexOffset = kSetOffset + kPointerSize;
  static const int kConsumedCountOffset = kIndexOffset + kPointerSize;
  static const int kSize = kConsumedCountOffset + kPointerSize;

  RAW_OBJECT_COMMON(SetIterator)
};

class TupleIterator : public HeapObject {
 public:
  // Getters and setters.
  word index();

  void setIndex(word index);

  RawObject tuple();

  void setTuple(RawObject tuple);

  // Iteration.
  RawObject next();

  // Layout.
  static const int kTupleOffset = HeapObject::kSize;
  static const int kIndexOffset = kTupleOffset + kPointerSize;
  static const int kSize = kIndexOffset + kPointerSize;

  RAW_OBJECT_COMMON(TupleIterator)
};

class Code : public HeapObject {
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
  word argcount();
  void setArgcount(word value);
  word totalArgs();

  word cell2arg();
  void setCell2arg(word value);

  RawObject cellvars();
  void setCellvars(RawObject value);
  word numCellvars();

  RawObject code();
  void setCode(RawObject value);

  RawObject consts();
  void setConsts(RawObject value);

  RawObject filename();
  void setFilename(RawObject value);

  word firstlineno();
  void setFirstlineno(word value);

  word flags();
  void setFlags(word value);

  RawObject freevars();
  void setFreevars(RawObject value);
  word numFreevars();

  word kwonlyargcount();
  void setKwonlyargcount(word value);

  RawObject lnotab();
  void setLnotab(RawObject value);

  RawObject name();
  void setName(RawObject value);

  RawObject names();
  void setNames(RawObject value);

  word nlocals();
  void setNlocals(word value);

  // The total number of variables in this code object: normal locals, cell
  // variables, and free variables.
  word totalVars();

  word stacksize();
  void setStacksize(word value);

  RawObject varnames();
  void setVarnames(RawObject value);

  // Layout.
  static const int kArgcountOffset = HeapObject::kSize;
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

  RAW_OBJECT_COMMON(Code)
};

class Frame;
class Thread;

/**
 * A function object.
 *
 * This may contain a user-defined function or a built-in function.
 *
 * Function objects have a set of pre-defined attributes, only some of which
 * are writable outside of the runtime. The full set is defined at
 *
 *     https://docs.python.org/3/reference/datamodel.html
 */
class Function : public HeapObject {
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
  RawObject annotations();
  void setAnnotations(RawObject annotations);

  // The code object backing this function or None
  RawObject code();
  void setCode(RawObject code);

  // A tuple of cell objects that contain bindings for the function's free
  // variables. Read-only to user code.
  RawObject closure();
  void setClosure(RawObject closure);

  // A tuple containing default values for arguments with defaults. Read-only
  // to user code.
  RawObject defaults();
  void setDefaults(RawObject defaults);
  bool hasDefaults();

  // The function's docstring
  RawObject doc();
  void setDoc(RawObject doc);

  // Returns the entry to be used when the function is invoked via
  // CALL_FUNCTION
  Entry entry();
  void setEntry(Entry entry);

  // Returns the entry to be used when the function is invoked via
  // CALL_FUNCTION_KW
  Entry entryKw();
  void setEntryKw(Entry entry_kw);

  // Returns the entry to be used when the function is invoked via
  // CALL_FUNCTION_EX
  inline Entry entryEx();
  inline void setEntryEx(Entry entry_ex);

  // The dict that holds this function's global namespace. User-code
  // cannot change this
  RawObject globals();
  void setGlobals(RawObject globals);

  // A dict containing defaults for keyword-only parameters
  RawObject kwDefaults();
  void setKwDefaults(RawObject kw_defaults);

  // The name of the module the function was defined in
  RawObject module();
  void setModule(RawObject module);

  // The function's name
  RawObject name();
  void setName(RawObject name);

  // The function's qualname
  RawObject qualname();
  void setQualname(RawObject qualname);

  // The pre-computed object array provided fast globals access.
  // fastGlobals[arg] == globals[names[arg]]
  RawObject fastGlobals();
  void setFastGlobals(RawObject fast_globals);

  // Layout.
  static const int kDocOffset = HeapObject::kSize;
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
  static const int kSize = kFastGlobalsOffset + kPointerSize;

  RAW_OBJECT_COMMON(Function)
};

class Instance : public HeapObject {
 public:
  // Sizing.
  static word allocationSize(word num_attributes);

  RAW_OBJECT_COMMON(Instance)
};

class Module : public HeapObject {
 public:
  // Setters and getters.
  RawObject name();
  void setName(RawObject name);

  RawObject dict();
  void setDict(RawObject dict);

  // Contains the numeric address of mode definition object for C-API modules or
  // zero if the module was not defined through the C-API.
  RawObject def();
  void setDef(RawObject def);

  // Layout.
  static const int kNameOffset = HeapObject::kSize;
  static const int kDictOffset = kNameOffset + kPointerSize;
  static const int kDefOffset = kDictOffset + kPointerSize;
  static const int kSize = kDefOffset + kPointerSize;

  RAW_OBJECT_COMMON(Module)
};

class NotImplemented : public HeapObject {
 public:
  // Layout
  // kPaddingOffset is not used, but the GC expects the object to be
  // at least one word.
  static const int kPaddingOffset = HeapObject::kSize;
  static const int kSize = kPaddingOffset + kPointerSize;

  RAW_OBJECT_COMMON(NotImplemented)
};

/**
 * A simple dict that uses open addressing and linear probing.
 *
 * Layout:
 *
 *   [Type pointer]
 *   [NumItems     ] - Number of items currently in the dict
 *   [Items        ] - Pointer to an ObjectArray that stores the underlying
 * data.
 *
 * Dict entries are stored in buckets as a triple of (hash, key, value).
 * Empty buckets are stored as (NoneType, NoneType, NoneType).
 * Tombstone buckets are stored as (NoneType, <not NoneType>, <Any>).
 *
 */
class Dict : public HeapObject {
 public:
  class Bucket;

  // Getters and setters.
  // The ObjectArray backing the dict
  RawObject data();
  void setData(RawObject data);

  // Number of items currently in the dict
  word numItems();
  void setNumItems(word num_items);

  // Layout.
  static const int kNumItemsOffset = HeapObject::kSize;
  static const int kDataOffset = kNumItemsOffset + kPointerSize;
  static const int kSize = kDataOffset + kPointerSize;

  RAW_OBJECT_COMMON(Dict)
};

// Helper class for manipulating buckets in the ObjectArray that backs the
// dict
class Dict::Bucket {
 public:
  // none of these operations do bounds checking on the backing array
  static word getIndex(RawObjectArray data, RawObject hash) {
    word nbuckets = data->length() / kNumPointers;
    DCHECK(Utils::isPowerOfTwo(nbuckets), "%ld is not a power of 2", nbuckets);
    word value = SmallInt::cast(hash)->value();
    return (value & (nbuckets - 1)) * kNumPointers;
  }

  static bool hasKey(RawObjectArray data, word index, RawObject that_key) {
    return !hash(data, index)->isNoneType() &&
           Object::equals(key(data, index), that_key);
  }

  static RawObject hash(RawObjectArray data, word index) {
    return data->at(index + kHashOffset);
  }

  static bool isEmpty(RawObjectArray data, word index) {
    return hash(data, index)->isNoneType() && key(data, index)->isNoneType();
  }

  static bool isFilled(RawObjectArray data, word index) {
    return !(isEmpty(data, index) || isTombstone(data, index));
  }

  static bool isTombstone(RawObjectArray data, word index) {
    return hash(data, index)->isNoneType() && !key(data, index)->isNoneType();
  }

  static RawObject key(RawObjectArray data, word index) {
    return data->at(index + kKeyOffset);
  }

  static void set(RawObjectArray data, word index, RawObject hash,
                  RawObject key, RawObject value) {
    data->atPut(index + kHashOffset, hash);
    data->atPut(index + kKeyOffset, key);
    data->atPut(index + kValueOffset, value);
  }

  static void setTombstone(RawObjectArray data, word index) {
    set(data, index, NoneType::object(), Error::object(), NoneType::object());
  }

  static RawObject value(RawObjectArray data, word index) {
    return data->at(index + kValueOffset);
  }

  // Layout.
  static const word kHashOffset = 0;
  static const word kKeyOffset = kHashOffset + 1;
  static const word kValueOffset = kKeyOffset + 1;
  static const word kNumPointers = kValueOffset + 1;

 private:
  DISALLOW_HEAP_ALLOCATION();
};

/**
 * A simple set implementation.
 */
class Set : public HeapObject {
 public:
  class Bucket;

  // Getters and setters.
  // The ObjectArray backing the set
  RawObject data();
  void setData(RawObject data);

  // Number of items currently in the set
  word numItems();
  void setNumItems(word num_items);

  // Layout.
  static const int kNumItemsOffset = HeapObject::kSize;
  static const int kDataOffset = kNumItemsOffset + kPointerSize;
  static const int kSize = kDataOffset + kPointerSize;

  RAW_OBJECT_COMMON(Set)
};

// Helper class for manipulating buckets in the ObjectArray that backs the
// set
class Set::Bucket {
 public:
  // none of these operations do bounds checking on the backing array
  static word getIndex(RawObjectArray data, RawObject hash) {
    word nbuckets = data->length() / kNumPointers;
    DCHECK(Utils::isPowerOfTwo(nbuckets), "%ld not a power of 2", nbuckets);
    word value = SmallInt::cast(hash)->value();
    return (value & (nbuckets - 1)) * kNumPointers;
  }

  static RawObject hash(RawObjectArray data, word index) {
    return data->at(index + kHashOffset);
  }

  static bool hasKey(RawObjectArray data, word index, RawObject that_key) {
    return !hash(data, index)->isNoneType() &&
           Object::equals(key(data, index), that_key);
  }

  static bool isEmpty(RawObjectArray data, word index) {
    return hash(data, index)->isNoneType() && key(data, index)->isNoneType();
  }

  static bool isTombstone(RawObjectArray data, word index) {
    return hash(data, index)->isNoneType() && !key(data, index)->isNoneType();
  }

  static RawObject key(RawObjectArray data, word index) {
    return data->at(index + kKeyOffset);
  }

  static void set(RawObjectArray data, word index, RawObject hash,
                  RawObject key) {
    data->atPut(index + kHashOffset, hash);
    data->atPut(index + kKeyOffset, key);
  }

  static void setTombstone(RawObjectArray data, word index) {
    set(data, index, NoneType::object(), Error::object());
  }

  // Layout.
  static const word kHashOffset = 0;
  static const word kKeyOffset = kHashOffset + 1;
  static const word kNumPointers = kKeyOffset + 1;

 private:
  DISALLOW_HEAP_ALLOCATION();
};

/**
 * A growable array
 *
 * Layout:
 *
 *   [Type pointer]
 *   [Length       ] - Number of elements currently in the list
 *   [Elems        ] - Pointer to an ObjectArray that contains list elements
 */
class List : public HeapObject {
 public:
  // Getters and setters.
  RawObject at(word index);
  void atPut(word index, RawObject value);
  RawObject items();
  void setItems(RawObject new_items);
  word allocated();
  void setAllocated(word new_allocated);

  // Return the total number of elements that may be held without growing the
  // list
  word capacity();

  // Casting.
  static RawList cast(RawObject object);

  // Layout.
  static const int kItemsOffset = HeapObject::kSize;
  static const int kAllocatedOffset = kItemsOffset + kPointerSize;
  static const int kSize = kAllocatedOffset + kPointerSize;

  RAW_OBJECT_COMMON_NO_CAST(List)
};

class ValueCell : public HeapObject {
 public:
  // Getters and setters
  RawObject value();
  void setValue(RawObject object);

  bool isUnbound();
  void makeUnbound();

  // Layout.
  static const int kValueOffset = HeapObject::kSize;
  static const int kSize = kValueOffset + kPointerSize;

  RAW_OBJECT_COMMON(ValueCell)
};

class Ellipsis : public HeapObject {
 public:
  // Layout
  // kPaddingOffset is not used, but the GC expects the object to be
  // at least one word.
  static const int kPaddingOffset = HeapObject::kSize;
  static const int kSize = kPaddingOffset + kPointerSize;

  RAW_OBJECT_COMMON(Ellipsis)
};

class WeakRef : public HeapObject {
 public:
  // Getters and setters

  // The object weakly referred to by this instance.
  RawObject referent();
  void setReferent(RawObject referent);

  // A callable object invoked with the referent as an argument when the
  // referent is deemed to be "near death" and only reachable through this weak
  // reference.
  RawObject callback();
  void setCallback(RawObject callable);

  // Singly linked list of weak reference objects.  This field is used during
  // garbage collection to represent the set of weak references that had been
  // discovered by the initial trace with an otherwise unreachable referent.
  RawObject link();
  void setLink(RawObject reference);

  static void enqueueReference(RawObject reference, RawObject* list);
  static RawObject dequeueReference(RawObject* list);
  static RawObject spliceQueue(RawObject tail1, RawObject tail2);

  // Layout.
  static const int kReferentOffset = HeapObject::kSize;
  static const int kCallbackOffset = kReferentOffset + kPointerSize;
  static const int kLinkOffset = kCallbackOffset + kPointerSize;
  static const int kSize = kLinkOffset + kPointerSize;

  RAW_OBJECT_COMMON(WeakRef)
};

/**
 * A BoundMethod binds a Function and its first argument (called `self`).
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
 * The LOAD_ATTR for `f.bar` creates a `BoundMethod`, which is then called
 * directly by the subsequent CALL_FUNCTION opcode.
 */
class BoundMethod : public HeapObject {
 public:
  // Getters and setters

  // The function to which "self" is bound
  RawObject function();
  void setFunction(RawObject function);

  // The instance of "self" being bound
  RawObject self();
  void setSelf(RawObject self);

  // Layout
  static const int kFunctionOffset = HeapObject::kSize;
  static const int kSelfOffset = kFunctionOffset + kPointerSize;
  static const int kSize = kSelfOffset + kPointerSize;

  RAW_OBJECT_COMMON(BoundMethod)
};

class ClassMethod : public HeapObject {
 public:
  // Getters and setters
  RawObject function();
  void setFunction(RawObject function);

  // Layout
  static const int kFunctionOffset = HeapObject::kSize;
  static const int kSize = kFunctionOffset + kPointerSize;

  RAW_OBJECT_COMMON(ClassMethod)
};

/**
 * A Layout describes the in-memory shape of an instance.
 *
 * Instance attributes are split into two classes: in-object attributes, which
 * exist directly in the instance, and overflow attributes, which are stored
 * in an object array pointed to by the last word of the instance. Graphically,
 * this looks like:
 *
 *   Instance                                   ObjectArray
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
 * the same class. Ideally, we would be able to share the same concrete Layout
 * between two instances of the same shape. This both reduces memory overhead
 * and enables effective caching of attribute location.
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
class Layout : public HeapObject {
 public:
  // Getters and setters.
  LayoutId id();
  void setId(LayoutId id);

  // Returns the class whose instances are described by this layout
  RawObject describedClass();
  void setDescribedClass(RawObject type);

  // Set the number of in-object attributes that may be stored on an instance
  // described by this layout.
  //
  // N.B. - This will always be larger than or equal to the length of the
  // ObjectArray returned by inObjectAttributes().
  void setNumInObjectAttributes(word count);
  word numInObjectAttributes();

  // Returns an ObjectArray describing the attributes stored directly in
  // in the instance.
  //
  // Each item in the object array is a two element tuple. Each tuple is
  // composed of the following elements, in order:
  //
  //   1. The attribute name (Str)
  //   2. The attribute info (AttributeInfo)
  RawObject inObjectAttributes();
  void setInObjectAttributes(RawObject attributes);

  // Returns an ObjectArray describing the attributes stored in the overflow
  // array of the instance.
  //
  // Each item in the object array is a two element tuple. Each tuple is
  // composed of the following elements, in order:
  //
  //   1. The attribute name (Str)
  //   2. The attribute info (AttributeInfo)
  RawObject overflowAttributes();
  void setOverflowAttributes(RawObject attributes);

  // Returns a flattened list of tuples. Each tuple is composed of the
  // following elements, in order:
  //
  //   1. The attribute name (Str)
  //   2. The layout that would result if an attribute with that name
  //      was added.
  RawObject additions();
  void setAdditions(RawObject additions);

  // Returns a flattened list of tuples. Each tuple is composed of the
  // following elements, in order:
  //
  //   1. The attribute name (Str)
  //   2. The layout that would result if an attribute with that name
  //      was deleted.
  RawObject deletions();
  void setDeletions(RawObject deletions);

  // Returns the number of words in an instance described by this layout,
  // including the overflow array.
  word instanceSize();
  void setInstanceSize(word size);

  // Return the offset, in bytes, of the overflow slot
  word overflowOffset();

  // Layout
  static const int kDescribedClassOffset = HeapObject::kSize;
  static const int kInObjectAttributesOffset =
      kDescribedClassOffset + kPointerSize;
  static const int kOverflowAttributesOffset =
      kInObjectAttributesOffset + kPointerSize;
  static const int kAdditionsOffset = kOverflowAttributesOffset + kPointerSize;
  static const int kDeletionsOffset = kAdditionsOffset + kPointerSize;
  static const int kInstanceSizeOffset = kDeletionsOffset + kPointerSize;
  static const int kOverflowOffsetOffset = kInstanceSizeOffset + kPointerSize;
  static const int kNumInObjectAttributesOffset =
      kOverflowOffsetOffset + kPointerSize;
  static const int kSize = kNumInObjectAttributesOffset + kPointerSize;

  RAW_OBJECT_COMMON(Layout)

 private:
  void setOverflowOffset(word offset);
};

class Super : public HeapObject {
 public:
  // getters and setters
  RawObject type();
  void setType(RawObject tp);
  RawObject object();
  void setObject(RawObject obj);
  RawObject objectType();
  void setObjectType(RawObject tp);

  // Layout
  static const int kTypeOffset = HeapObject::kSize;
  static const int kObjectOffset = kTypeOffset + kPointerSize;
  static const int kObjectTypeOffset = kObjectOffset + kPointerSize;
  static const int kSize = kObjectTypeOffset + kPointerSize;

  RAW_OBJECT_COMMON(Super)
};

/**
 * Base class containing functionality needed by all objects representing a
 * suspended execution frame: Generator, Coroutine, and AsyncGenerator.
 */
class GeneratorBase : public HeapObject {
 public:
  // Get or set the HeapFrame embedded in this GeneratorBase.
  RawObject heapFrame();
  void setHeapFrame(RawObject obj);

  // Layout.
  static const int kFrameOffset = HeapObject::kSize;
  static const int kIsRunningOffset = kFrameOffset + kPointerSize;
  static const int kCodeOffset = kIsRunningOffset + kPointerSize;
  static const int kSize = kCodeOffset + kPointerSize;

  RAW_OBJECT_COMMON(GeneratorBase)
};

class Generator : public GeneratorBase {
 public:
  static const int kYieldFromOffset = GeneratorBase::kSize;
  static const int kSize = kYieldFromOffset + kPointerSize;

  RAW_OBJECT_COMMON(Generator)
};

class Coroutine : public GeneratorBase {
 public:
  // Layout.
  static const int kAwaitOffset = GeneratorBase::kSize;
  static const int kOriginOffset = kAwaitOffset + kPointerSize;
  static const int kSize = kOriginOffset + kPointerSize;

  RAW_OBJECT_COMMON(Coroutine)
};

// Object

inline Object::Object(uword raw) : raw_{raw} {}

inline uword Object::raw() const { return raw_; }

inline bool Object::isObject() { return true; }

inline LayoutId Object::layoutId() {
  if (isHeapObject()) {
    return HeapObject::cast(*this)->header()->layoutId();
  }
  if (isSmallInt()) {
    return LayoutId::kSmallInt;
  }
  return static_cast<LayoutId>(raw() & kImmediateClassTableIndexMask);
}

inline bool Object::isType() { return isHeapObjectWithLayout(LayoutId::kType); }

inline bool Object::isClassMethod() {
  return isHeapObjectWithLayout(LayoutId::kClassMethod);
}

inline bool Object::isSmallInt() {
  return (raw() & SmallInt::kTagMask) == SmallInt::kTag;
}

inline bool Object::isSmallStr() {
  return (raw() & SmallStr::kTagMask) == SmallStr::kTag;
}

inline bool Object::isHeader() {
  return (raw() & Header::kTagMask) == Header::kTag;
}

inline bool Object::isBool() { return (raw() & Bool::kTagMask) == Bool::kTag; }

inline bool Object::isNoneType() {
  return (raw() & NoneType::kTagMask) == NoneType::kTag;
}

inline bool Object::isError() {
  return (raw() & Error::kTagMask) == Error::kTag;
}

inline bool Object::isHeapObject() {
  return (raw() & HeapObject::kTagMask) == HeapObject::kTag;
}

inline bool Object::isHeapObjectWithLayout(LayoutId layout_id) {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(*this)->header()->layoutId() == layout_id;
}

inline bool Object::isLayout() {
  return isHeapObjectWithLayout(LayoutId::kLayout);
}

inline bool Object::isBaseException() {
  return isHeapObjectWithLayout(LayoutId::kBaseException);
}

inline bool Object::isException() {
  return isHeapObjectWithLayout(LayoutId::kException);
}

inline bool Object::isBoundMethod() {
  return isHeapObjectWithLayout(LayoutId::kBoundMethod);
}

inline bool Object::isBytes() {
  return isHeapObjectWithLayout(LayoutId::kBytes);
}

inline bool Object::isObjectArray() {
  return isHeapObjectWithLayout(LayoutId::kObjectArray);
}

inline bool Object::isCode() { return isHeapObjectWithLayout(LayoutId::kCode); }

inline bool Object::isComplex() {
  return isHeapObjectWithLayout(LayoutId::kComplex);
}

inline bool Object::isCoroutine() {
  return isHeapObjectWithLayout(LayoutId::kCoroutine);
}

inline bool Object::isLargeStr() {
  return isHeapObjectWithLayout(LayoutId::kLargeStr);
}

inline bool Object::isFunction() {
  return isHeapObjectWithLayout(LayoutId::kFunction);
}

inline bool Object::isInstance() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(*this)->header()->layoutId() >
         LayoutId::kLastBuiltinId;
}

inline bool Object::isKeyError() {
  return isHeapObjectWithLayout(LayoutId::kKeyError);
}

inline bool Object::isDict() { return isHeapObjectWithLayout(LayoutId::kDict); }

inline bool Object::isFloat() {
  return isHeapObjectWithLayout(LayoutId::kFloat);
}

inline bool Object::isHeapFrame() {
  return isHeapObjectWithLayout(LayoutId::kHeapFrame);
}

inline bool Object::isSet() { return isHeapObjectWithLayout(LayoutId::kSet); }

inline bool Object::isSetIterator() {
  return isHeapObjectWithLayout(LayoutId::kSetIterator);
}

inline bool Object::isSuper() {
  return isHeapObjectWithLayout(LayoutId::kSuper);
}

inline bool Object::isModule() {
  return isHeapObjectWithLayout(LayoutId::kModule);
}

inline bool Object::isList() { return isHeapObjectWithLayout(LayoutId::kList); }

inline bool Object::isListIterator() {
  return isHeapObjectWithLayout(LayoutId::kListIterator);
}

inline bool Object::isLookupError() {
  return isHeapObjectWithLayout(LayoutId::kLookupError);
}

inline bool Object::isValueCell() {
  return isHeapObjectWithLayout(LayoutId::kValueCell);
}

inline bool Object::isEllipsis() {
  return isHeapObjectWithLayout(LayoutId::kEllipsis);
}

inline bool Object::isGenerator() {
  return isHeapObjectWithLayout(LayoutId::kGenerator);
}

inline bool Object::isLargeInt() {
  return isHeapObjectWithLayout(LayoutId::kLargeInt);
}

inline bool Object::isInt() { return isSmallInt() || isLargeInt() || isBool(); }

inline bool Object::isNotImplemented() {
  return isHeapObjectWithLayout(LayoutId::kNotImplemented);
}

inline bool Object::isNotImplementedError() {
  return isHeapObjectWithLayout(LayoutId::kNotImplementedError);
}

inline bool Object::isProperty() {
  return isHeapObjectWithLayout(LayoutId::kProperty);
}

inline bool Object::isRange() {
  return isHeapObjectWithLayout(LayoutId::kRange);
}

inline bool Object::isRangeIterator() {
  return isHeapObjectWithLayout(LayoutId::kRangeIterator);
}

inline bool Object::isGeneratorBase() { return isGenerator() || isCoroutine(); }

inline bool Object::isRuntimeError() {
  return isHeapObjectWithLayout(LayoutId::kRuntimeError);
}

inline bool Object::isSlice() {
  return isHeapObjectWithLayout(LayoutId::kSlice);
}

inline bool Object::isStaticMethod() {
  return isHeapObjectWithLayout(LayoutId::kStaticMethod);
}

inline bool Object::isStopIteration() {
  return isHeapObjectWithLayout(LayoutId::kStopIteration);
}

inline bool Object::isStr() { return isSmallStr() || isLargeStr(); }

inline bool Object::isSystemExit() {
  return isHeapObjectWithLayout(LayoutId::kSystemExit);
}

inline bool Object::isTupleIterator() {
  return isHeapObjectWithLayout(LayoutId::kTupleIterator);
}

inline bool Object::isImportError() {
  return isHeapObjectWithLayout(LayoutId::kImportError);
}

inline bool Object::isIndexError() {
  return isHeapObjectWithLayout(LayoutId::kIndexError);
}

inline bool Object::isWeakRef() {
  return isHeapObjectWithLayout(LayoutId::kWeakRef);
}

inline bool Object::isModuleNotFoundError() {
  return isHeapObjectWithLayout(LayoutId::kModuleNotFoundError);
}

inline bool Object::equals(RawObject lhs, RawObject rhs) {
  return (lhs == rhs) ||
         (lhs->isLargeStr() && LargeStr::cast(lhs)->equals(rhs));
}

inline bool Object::operator==(const Object& other) const {
  return raw() == other.raw();
}

inline bool Object::operator!=(const Object& other) const {
  return !operator==(other);
}

// Int

inline word Int::asWord() {
  if (isSmallInt()) {
    return SmallInt::cast(*this)->value();
  }
  return LargeInt::cast(*this)->asWord();
}

inline void* Int::asCPtr() {
  if (isSmallInt()) {
    return SmallInt::cast(*this)->asCPtr();
  }
  return LargeInt::cast(*this)->asCPtr();
}

template <typename T>
OptInt<T> Int::asInt() {
  if (isSmallInt()) return SmallInt::cast(*this)->asInt<T>();
  return LargeInt::cast(*this)->asInt<T>();
}

inline word Int::compare(RawInt that) {
  if (this->isSmallInt() && that->isSmallInt()) {
    return this->asWord() - that->asWord();
  }
  if (this->isNegative() != that->isNegative()) {
    return this->isNegative() ? -1 : 1;
  }

  word left_digits = this->numDigits();
  word right_digits = that->numDigits();

  if (left_digits > right_digits) {
    return 1;
  }
  if (left_digits < right_digits) {
    return -1;
  }
  for (word i = left_digits - 1; i >= 0; i--) {
    word left_digit = this->digitAt(i), right_digit = that->digitAt(i);
    if (left_digit > right_digit) {
      return 1;
    }
    if (left_digit < right_digit) {
      return -1;
    }
  }
  return 0;
}

inline double Int::floatValue() {
  if (isSmallInt()) {
    return static_cast<double>(asWord());
  }
  if (isBool()) {
    return Bool::cast(*this) == Bool::trueObj() ? 1.0 : 0.0;
  }
  RawLargeInt large_int = LargeInt::cast(*this);
  if (large_int->numDigits() == 1) {
    return static_cast<double>(asWord());
  }
  // TODO(T30610701): Handle arbitrary precision LargeInts
  UNIMPLEMENTED("LargeInts with > 1 digit");
}

inline word Int::bitLength() {
  if (isSmallInt()) {
    uword self = static_cast<uword>(std::abs(SmallInt::cast(*this)->value()));
    return Utils::highestBit(self);
  }
  if (isBool()) {
    return Bool::cast(*this) == Bool::trueObj() ? 1 : 0;
  }
  return LargeInt::cast(*this)->bitLength();
}

inline bool Int::isPositive() {
  if (isSmallInt()) {
    return SmallInt::cast(*this)->value() > 0;
  }
  if (isBool()) {
    return Bool::cast(*this) == Bool::trueObj();
  }
  return LargeInt::cast(*this)->isPositive();
}

inline bool Int::isNegative() {
  if (isSmallInt()) {
    return SmallInt::cast(*this)->value() < 0;
  }
  if (isBool()) {
    return false;
  }
  return LargeInt::cast(*this)->isNegative();
}

inline bool Int::isZero() {
  if (isSmallInt()) {
    return SmallInt::cast(*this)->value() == 0;
  }
  if (isBool()) {
    return Bool::cast(*this) == Bool::falseObj();
  }
  // A LargeInt can never be zero
  DCHECK(isLargeInt(), "Object must be a LargeInt");
  return false;
}

inline word Int::numDigits() {
  if (isSmallInt() || isBool()) {
    return 1;
  }
  return LargeInt::cast(*this)->numDigits();
}

inline word Int::digitAt(word index) {
  if (isSmallInt()) {
    DCHECK(index == 0, "SmallInt digit index out of bounds");
    return SmallInt::cast(*this)->value();
  }
  if (isBool()) {
    DCHECK(index == 0, "Bool digit index out of bounds");
    return Bool::cast(*this) == Bool::trueObj() ? 1 : 0;
  }
  return LargeInt::cast(*this)->digitAt(index);
}

// SmallInt

inline word SmallInt::value() { return static_cast<word>(raw()) >> kTagSize; }

inline void* SmallInt::asCPtr() { return reinterpret_cast<void*>(value()); }

template <typename T>
if_signed_t<T, OptInt<T>> SmallInt::asInt() {
  static_assert(sizeof(T) <= sizeof(word), "T must not be larger than word");

  auto const value = this->value();
  if (value > std::numeric_limits<T>::max()) return OptInt<T>::overflow();
  if (value < std::numeric_limits<T>::min()) return OptInt<T>::underflow();
  return OptInt<T>::valid(value);
}

template <typename T>
if_unsigned_t<T, OptInt<T>> SmallInt::asInt() {
  static_assert(sizeof(T) <= sizeof(word), "T must not be larger than word");
  auto const max = std::numeric_limits<T>::max();
  auto const value = this->value();

  if (value < 0) return OptInt<T>::underflow();
  if (max >= SmallInt::kMaxValue || static_cast<uword>(value) <= max) {
    return OptInt<T>::valid(value);
  }
  return OptInt<T>::overflow();
}

inline RawSmallInt SmallInt::fromWord(word value) {
  DCHECK(SmallInt::isValid(value), "invalid cast");
  return cast(RawObject{static_cast<uword>(value) << kTagSize});
}

template <typename T>
inline RawSmallInt SmallInt::fromFunctionPointer(T pointer) {
  // The bit pattern for a function pointer object must be indistinguishable
  // from that of a small integer object.
  return cast(RawObject{reinterpret_cast<uword>(pointer)});
}

// SmallStr

inline word SmallStr::length() { return (raw() >> kTagSize) & kMaxLength; }

inline byte SmallStr::charAt(word index) {
  DCHECK_INDEX(index, length());
  return raw() >> (kBitsPerByte * (index + 1));
}

inline void SmallStr::copyTo(byte* dst, word length) {
  DCHECK_BOUND(length, this->length());
  for (word i = 0; i < length; ++i) {
    *dst++ = charAt(i);
  }
}

// Header

inline word Header::count() {
  return static_cast<word>((raw() >> kCountOffset) & kCountMask);
}

inline bool Header::hasOverflow() { return count() == kCountOverflowFlag; }

inline word Header::hashCode() {
  return static_cast<word>((raw() >> kHashCodeOffset) & kHashCodeMask);
}

inline RawHeader Header::withHashCode(word value) {
  auto header = raw();
  header &= ~(kHashCodeMask << kHashCodeOffset);
  header |= (value & kHashCodeMask) << kHashCodeOffset;
  return cast(RawObject{header});
}

inline LayoutId Header::layoutId() {
  return static_cast<LayoutId>((raw() >> kLayoutIdOffset) & kLayoutIdMask);
}

inline RawHeader Header::withLayoutId(LayoutId layout_id) {
  DCHECK_BOUND(static_cast<word>(layout_id), kMaxLayoutId);
  auto header = raw();
  header &= ~(kLayoutIdMask << kLayoutIdOffset);
  header |= (static_cast<word>(layout_id) & kLayoutIdMask) << kLayoutIdOffset;
  return cast(RawObject{header});
}

inline ObjectFormat Header::format() {
  return static_cast<ObjectFormat>((raw() >> kFormatOffset) & kFormatMask);
}

inline RawHeader Header::from(word count, word hash, LayoutId id,
                              ObjectFormat format) {
  DCHECK(
      (count >= 0) && ((count <= kCountMax) || (count == kCountOverflowFlag)),
      "bounds violation, %ld not in 0..%d", count, kCountMax);
  uword result = Header::kTag;
  result |= ((count > kCountMax) ? kCountOverflowFlag : count) << kCountOffset;
  result |= hash << kHashCodeOffset;
  result |= static_cast<uword>(id) << kLayoutIdOffset;
  result |= static_cast<uword>(format) << kFormatOffset;
  return cast(RawObject{result});
}

// None

inline RawNoneType NoneType::object() {
  return bit_cast<RawNoneType>(static_cast<uword>(kTag));
}

// Error

inline RawError Error::object() {
  return bit_cast<RawError>(static_cast<uword>(kTag));
}

// Bool

inline RawBool Bool::trueObj() { return fromBool(true); }

inline RawBool Bool::falseObj() { return fromBool(false); }

inline RawBool Bool::negate(RawObject value) {
  DCHECK(value->isBool(), "not a boolean instance");
  return (value == trueObj()) ? falseObj() : trueObj();
}

inline RawBool Bool::fromBool(bool value) {
  return cast(RawObject{(static_cast<uword>(value) << kTagSize) | kTag});
}

inline bool Bool::value() { return (raw() >> kTagSize) ? true : false; }

// HeapObject

inline uword HeapObject::address() { return raw() - HeapObject::kTag; }

inline uword HeapObject::baseAddress() {
  uword result = address() - Header::kSize;
  if (header()->hasOverflow()) {
    result -= kPointerSize;
  }
  return result;
}

inline RawHeader HeapObject::header() {
  return Header::cast(instanceVariableAt(kHeaderOffset));
}

inline void HeapObject::setHeader(RawHeader header) {
  instanceVariableAtPut(kHeaderOffset, header);
}

inline word HeapObject::headerOverflow() {
  DCHECK(header()->hasOverflow(), "expected Overflow");
  return SmallInt::cast(instanceVariableAt(kHeaderOverflowOffset))->value();
}

inline void HeapObject::setHeaderAndOverflow(word count, word hash, LayoutId id,
                                             ObjectFormat format) {
  if (count > Header::kCountMax) {
    instanceVariableAtPut(kHeaderOverflowOffset, SmallInt::fromWord(count));
    count = Header::kCountOverflowFlag;
  }
  setHeader(Header::from(count, hash, id, format));
}

inline RawHeapObject HeapObject::fromAddress(uword address) {
  DCHECK((address & kTagMask) == 0, "invalid cast, expected heap address");
  return cast(RawObject{address + kTag});
}

inline word HeapObject::headerCountOrOverflow() {
  if (header()->hasOverflow()) {
    return headerOverflow();
  }
  return header()->count();
}

inline word HeapObject::size() {
  word count = headerCountOrOverflow();
  word result = headerSize(count);
  switch (header()->format()) {
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

inline word HeapObject::headerSize(word count) {
  word result = kPointerSize;
  if (count > Header::kCountMax) {
    result += kPointerSize;
  }
  return result;
}

inline void HeapObject::initialize(word size, RawObject value) {
  for (word offset = HeapObject::kSize; offset < size; offset += kPointerSize) {
    instanceVariableAtPut(offset, value);
  }
}

inline bool HeapObject::isRoot() {
  return header()->format() == ObjectFormat::kObjectArray ||
         header()->format() == ObjectFormat::kObjectInstance;
}

inline bool HeapObject::isForwarding() {
  return *reinterpret_cast<uword*>(address() + kHeaderOffset) == kIsForwarded;
}

inline RawObject HeapObject::forward() {
  // When a heap object is forwarding, its second word is the forwarding
  // address.
  return *reinterpret_cast<RawObject*>(address() + kHeaderOffset +
                                       kPointerSize);
}

inline void HeapObject::forwardTo(RawObject object) {
  // Overwrite the header with the forwarding marker.
  *reinterpret_cast<uword*>(address() + kHeaderOffset) = kIsForwarded;
  // Overwrite the second word with the forwarding addressing.
  *reinterpret_cast<RawObject*>(address() + kHeaderOffset + kPointerSize) =
      object;
}

inline RawObject HeapObject::instanceVariableAt(word offset) {
  return *reinterpret_cast<RawObject*>(address() + offset);
}

inline void HeapObject::instanceVariableAtPut(word offset, RawObject value) {
  *reinterpret_cast<RawObject*>(address() + offset) = value;
}

// BaseException

inline RawObject BaseException::args() {
  return instanceVariableAt(kArgsOffset);
}

inline void BaseException::setArgs(RawObject args) {
  instanceVariableAtPut(kArgsOffset, args);
}

inline RawObject BaseException::traceback() {
  return instanceVariableAt(kTracebackOffset);
}

inline void BaseException::setTraceback(RawObject traceback) {
  instanceVariableAtPut(kTracebackOffset, traceback);
}

inline RawObject BaseException::cause() {
  return instanceVariableAt(kCauseOffset);
}

inline void BaseException::setCause(RawObject cause) {
  return instanceVariableAtPut(kCauseOffset, cause);
}

inline RawObject BaseException::context() {
  return instanceVariableAt(kContextOffset);
}

inline void BaseException::setContext(RawObject context) {
  return instanceVariableAtPut(kContextOffset, context);
}

// StopIteration

inline RawObject StopIteration::value() {
  return instanceVariableAt(kValueOffset);
}

inline void StopIteration::setValue(RawObject value) {
  instanceVariableAtPut(kValueOffset, value);
}

// SystemExit

inline RawObject SystemExit::code() { return instanceVariableAt(kCodeOffset); }

inline void SystemExit::setCode(RawObject code) {
  instanceVariableAtPut(kCodeOffset, code);
}

// ImportError

inline RawObject ImportError::msg() { return instanceVariableAt(kMsgOffset); }

inline void ImportError::setMsg(RawObject msg) {
  instanceVariableAtPut(kMsgOffset, msg);
}

inline RawObject ImportError::name() { return instanceVariableAt(kNameOffset); }

inline void ImportError::setName(RawObject name) {
  instanceVariableAtPut(kNameOffset, name);
}

inline RawObject ImportError::path() { return instanceVariableAt(kPathOffset); }

inline void ImportError::setPath(RawObject path) {
  instanceVariableAtPut(kPathOffset, path);
}

// Type

inline RawObject Type::mro() { return instanceVariableAt(kMroOffset); }

inline void Type::setMro(RawObject object_array) {
  instanceVariableAtPut(kMroOffset, object_array);
}

inline RawObject Type::instanceLayout() {
  return instanceVariableAt(kInstanceLayoutOffset);
}

inline void Type::setInstanceLayout(RawObject layout) {
  instanceVariableAtPut(kInstanceLayoutOffset, layout);
}

inline RawObject Type::name() { return instanceVariableAt(kNameOffset); }

inline void Type::setName(RawObject name) {
  instanceVariableAtPut(kNameOffset, name);
}

inline RawObject Type::flags() { return instanceVariableAt(kFlagsOffset); }

inline void Type::setFlags(RawObject value) {
  instanceVariableAtPut(kFlagsOffset, value);
}

inline void Type::setFlag(Type::Flag bit) {
  word f = SmallInt::cast(flags())->value();
  RawObject new_flag = SmallInt::fromWord(f | bit);
  instanceVariableAtPut(kFlagsOffset, new_flag);
}

inline bool Type::hasFlag(Type::Flag bit) {
  word f = SmallInt::cast(flags())->value();
  return (f & bit) != 0;
}

inline RawObject Type::dict() { return instanceVariableAt(kDictOffset); }

inline void Type::setDict(RawObject dict) {
  instanceVariableAtPut(kDictOffset, dict);
}

inline RawObject Type::builtinBaseClass() {
  return instanceVariableAt(kBuiltinBaseClassOffset);
}

inline RawObject Type::extensionType() {
  return instanceVariableAt(kExtensionTypeOffset);
}

inline void Type::setExtensionType(RawObject pytype) {
  instanceVariableAtPut(kExtensionTypeOffset, pytype);
}

inline void Type::setBuiltinBaseClass(RawObject base) {
  instanceVariableAtPut(kBuiltinBaseClassOffset, base);
}

inline bool Type::isIntrinsicOrExtension() {
  return Layout::cast(instanceLayout())->id() <= LayoutId::kLastBuiltinId;
}

// Array

inline word Array::length() {
  DCHECK(isBytes() || isObjectArray() || isLargeStr(), "invalid array type");
  return headerCountOrOverflow();
}

// Bytes

inline word Bytes::allocationSize(word length) {
  DCHECK(length >= 0, "invalid length %ld", length);
  word size = headerSize(length) + length;
  return Utils::maximum(kMinimumSize, Utils::roundUp(size, kPointerSize));
}

inline byte Bytes::byteAt(word index) {
  DCHECK_INDEX(index, length());
  return *reinterpret_cast<byte*>(address() + index);
}

inline void Bytes::byteAtPut(word index, byte value) {
  DCHECK_INDEX(index, length());
  *reinterpret_cast<byte*>(address() + index) = value;
}

// ObjectArray

inline word ObjectArray::allocationSize(word length) {
  DCHECK(length >= 0, "invalid length %ld", length);
  word size = headerSize(length) + length * kPointerSize;
  return Utils::maximum(kMinimumSize, Utils::roundUp(size, kPointerSize));
}

inline RawObject ObjectArray::at(word index) {
  DCHECK_INDEX(index, length());
  return instanceVariableAt(index * kPointerSize);
}

inline void ObjectArray::atPut(word index, RawObject value) {
  DCHECK_INDEX(index, length());
  instanceVariableAtPut(index * kPointerSize, value);
}

inline void ObjectArray::copyTo(RawObject array) {
  RawObjectArray dst = ObjectArray::cast(array);
  word len = length();
  DCHECK_BOUND(len, dst->length());
  for (word i = 0; i < len; i++) {
    RawObject elem = at(i);
    dst->atPut(i, elem);
  }
}

inline void ObjectArray::replaceFromWith(word start, RawObject array) {
  RawObjectArray src = ObjectArray::cast(array);
  word count = Utils::minimum(this->length() - start, src->length());
  word stop = start + count;
  for (word i = start, j = 0; i < stop; i++, j++) {
    atPut(i, src->at(j));
  }
}

inline bool ObjectArray::contains(RawObject object) {
  word len = length();
  for (word i = 0; i < len; i++) {
    if (at(i) == object) {
      return true;
    }
  }
  return false;
}

// Code

inline word Code::argcount() {
  return SmallInt::cast(instanceVariableAt(kArgcountOffset))->value();
}

inline void Code::setArgcount(word value) {
  instanceVariableAtPut(kArgcountOffset, SmallInt::fromWord(value));
}

inline word Code::cell2arg() {
  return SmallInt::cast(instanceVariableAt(kCell2argOffset))->value();
}

inline word Code::totalArgs() {
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

inline void Code::setCell2arg(word value) {
  instanceVariableAtPut(kCell2argOffset, SmallInt::fromWord(value));
}

inline RawObject Code::cellvars() {
  return instanceVariableAt(kCellvarsOffset);
}

inline void Code::setCellvars(RawObject value) {
  instanceVariableAtPut(kCellvarsOffset, value);
}

inline word Code::numCellvars() {
  RawObject object = cellvars();
  DCHECK(object->isNoneType() || object->isObjectArray(),
         "not an object array");
  if (object->isNoneType()) {
    return 0;
  }
  return ObjectArray::cast(object)->length();
}

inline RawObject Code::code() { return instanceVariableAt(kCodeOffset); }

inline void Code::setCode(RawObject value) {
  instanceVariableAtPut(kCodeOffset, value);
}

inline RawObject Code::consts() { return instanceVariableAt(kConstsOffset); }

inline void Code::setConsts(RawObject value) {
  instanceVariableAtPut(kConstsOffset, value);
}

inline RawObject Code::filename() {
  return instanceVariableAt(kFilenameOffset);
}

inline void Code::setFilename(RawObject value) {
  instanceVariableAtPut(kFilenameOffset, value);
}

inline word Code::firstlineno() {
  return SmallInt::cast(instanceVariableAt(kFirstlinenoOffset))->value();
}

inline void Code::setFirstlineno(word value) {
  instanceVariableAtPut(kFirstlinenoOffset, SmallInt::fromWord(value));
}

inline word Code::flags() {
  return SmallInt::cast(instanceVariableAt(kFlagsOffset))->value();
}

inline void Code::setFlags(word value) {
  if ((kwonlyargcount() == 0) && (value & NOFREE) &&
      !(value & (VARARGS | VARKEYARGS))) {
    // Set up shortcut for detecting fast case for calls
    // TODO: move into equivalent of CPython's codeobject.c:PyCode_New()
    value |= SIMPLE_CALL;
  }
  instanceVariableAtPut(kFlagsOffset, SmallInt::fromWord(value));
}

inline RawObject Code::freevars() {
  return instanceVariableAt(kFreevarsOffset);
}

inline void Code::setFreevars(RawObject value) {
  instanceVariableAtPut(kFreevarsOffset, value);
}

inline word Code::numFreevars() {
  RawObject object = freevars();
  DCHECK(object->isNoneType() || object->isObjectArray(),
         "not an object array");
  if (object->isNoneType()) {
    return 0;
  }
  return ObjectArray::cast(object)->length();
}

inline word Code::kwonlyargcount() {
  return SmallInt::cast(instanceVariableAt(kKwonlyargcountOffset))->value();
}

inline void Code::setKwonlyargcount(word value) {
  instanceVariableAtPut(kKwonlyargcountOffset, SmallInt::fromWord(value));
}

inline RawObject Code::lnotab() { return instanceVariableAt(kLnotabOffset); }

inline void Code::setLnotab(RawObject value) {
  instanceVariableAtPut(kLnotabOffset, value);
}

inline RawObject Code::name() { return instanceVariableAt(kNameOffset); }

inline void Code::setName(RawObject value) {
  instanceVariableAtPut(kNameOffset, value);
}

inline RawObject Code::names() { return instanceVariableAt(kNamesOffset); }

inline void Code::setNames(RawObject value) {
  instanceVariableAtPut(kNamesOffset, value);
}

inline word Code::nlocals() {
  return SmallInt::cast(instanceVariableAt(kNlocalsOffset))->value();
}

inline void Code::setNlocals(word value) {
  instanceVariableAtPut(kNlocalsOffset, SmallInt::fromWord(value));
}

inline word Code::totalVars() {
  return nlocals() + numCellvars() + numFreevars();
}

inline word Code::stacksize() {
  return SmallInt::cast(instanceVariableAt(kStacksizeOffset))->value();
}

inline void Code::setStacksize(word value) {
  instanceVariableAtPut(kStacksizeOffset, SmallInt::fromWord(value));
}

inline RawObject Code::varnames() {
  return instanceVariableAt(kVarnamesOffset);
}

inline void Code::setVarnames(RawObject value) {
  instanceVariableAtPut(kVarnamesOffset, value);
}

// LargeInt

inline word LargeInt::asWord() {
  DCHECK(numDigits() == 1, "LargeInt cannot fit in a word");
  return static_cast<word>(digitAt(0));
}

inline void* LargeInt::asCPtr() {
  DCHECK(numDigits() == 1, "Large integer cannot fit in a pointer");
  DCHECK(isPositive(), "Cannot cast a negative value to a C pointer");
  return reinterpret_cast<void*>(asWord());
}

template <typename T>
if_signed_t<T, OptInt<T>> LargeInt::asInt() {
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
if_unsigned_t<T, OptInt<T>> LargeInt::asInt() {
  static_assert(sizeof(T) <= sizeof(word), "T must not be larger than word");

  if (isNegative()) return OptInt<T>::underflow();
  if (static_cast<size_t>(bitLength()) > sizeof(T) * kBitsPerByte) {
    return OptInt<T>::overflow();
  }
  // No T accepted by this function needs more than one digit.
  return OptInt<T>::valid(digitAt(0));
}

inline bool LargeInt::isNegative() {
  word highest_digit = digitAt(numDigits() - 1);
  return highest_digit < 0;
}

inline bool LargeInt::isPositive() {
  word highest_digit = digitAt(numDigits() - 1);
  return highest_digit >= 0;
}

inline word LargeInt::digitAt(word index) {
  DCHECK_INDEX(index, numDigits());
  return reinterpret_cast<word*>(address() + kValueOffset)[index];
}

inline void LargeInt::digitAtPut(word index, word digit) {
  DCHECK_INDEX(index, numDigits());
  reinterpret_cast<word*>(address() + kValueOffset)[index] = digit;
}

inline word LargeInt::numDigits() { return headerCountOrOverflow(); }

inline word LargeInt::allocationSize(word num_digits) {
  word size = headerSize(num_digits) + num_digits * kPointerSize;
  return Utils::maximum(kMinimumSize, Utils::roundUp(size, kPointerSize));
}

// Float

inline double Float::value() {
  return *reinterpret_cast<double*>(address() + kValueOffset);
}

inline void Float::initialize(double value) {
  *reinterpret_cast<double*>(address() + kValueOffset) = value;
}

// Complex
inline double Complex::real() {
  return *reinterpret_cast<double*>(address() + kRealOffset);
}

inline double Complex::imag() {
  return *reinterpret_cast<double*>(address() + kImagOffset);
}

inline void Complex::initialize(double real, double imag) {
  *reinterpret_cast<double*>(address() + kRealOffset) = real;
  *reinterpret_cast<double*>(address() + kImagOffset) = imag;
}

// Range

inline word Range::start() {
  return SmallInt::cast(instanceVariableAt(kStartOffset))->value();
}

inline void Range::setStart(word value) {
  instanceVariableAtPut(kStartOffset, SmallInt::fromWord(value));
}

inline word Range::stop() {
  return SmallInt::cast(instanceVariableAt(kStopOffset))->value();
}

inline void Range::setStop(word value) {
  instanceVariableAtPut(kStopOffset, SmallInt::fromWord(value));
}

inline word Range::step() {
  return SmallInt::cast(instanceVariableAt(kStepOffset))->value();
}

inline void Range::setStep(word value) {
  instanceVariableAtPut(kStepOffset, SmallInt::fromWord(value));
}

// ListIterator

inline word ListIterator::index() {
  return SmallInt::cast(instanceVariableAt(kIndexOffset))->value();
}

inline void ListIterator::setIndex(word index) {
  instanceVariableAtPut(kIndexOffset, SmallInt::fromWord(index));
}

inline RawObject ListIterator::list() {
  return instanceVariableAt(kListOffset);
}

inline void ListIterator::setList(RawObject list) {
  instanceVariableAtPut(kListOffset, list);
}

inline RawObject ListIterator::next() {
  word idx = index();
  auto underlying = List::cast(list());
  if (idx >= underlying->allocated()) {
    return Error::object();
  }

  RawObject item = underlying->at(idx);
  setIndex(idx + 1);
  return item;
}

// Property

inline RawObject Property::getter() {
  return instanceVariableAt(kGetterOffset);
}

inline void Property::setGetter(RawObject function) {
  instanceVariableAtPut(kGetterOffset, function);
}

inline RawObject Property::setter() {
  return instanceVariableAt(kSetterOffset);
}

inline void Property::setSetter(RawObject function) {
  instanceVariableAtPut(kSetterOffset, function);
}

inline RawObject Property::deleter() {
  return instanceVariableAt(kDeleterOffset);
}

inline void Property::setDeleter(RawObject function) {
  instanceVariableAtPut(kDeleterOffset, function);
}

// RangeIterator

inline void RangeIterator::setRange(RawObject range) {
  auto r = Range::cast(range);
  instanceVariableAtPut(kRangeOffset, r);
  instanceVariableAtPut(kCurValueOffset, SmallInt::fromWord(r->start()));
}

inline bool RangeIterator::isOutOfRange(word cur, word stop, word step) {
  DCHECK(step != 0,
         "invalid step");  // should have been checked in builtinRange().

  if (step < 0) {
    if (cur <= stop) {
      return true;
    }
  } else if (step > 0) {
    if (cur >= stop) {
      return true;
    }
  }
  return false;
}

inline word RangeIterator::pendingLength() {
  RawRange range = Range::cast(instanceVariableAt(kRangeOffset));
  word stop = range->stop();
  word step = range->step();
  word current = SmallInt::cast(instanceVariableAt(kCurValueOffset))->value();
  if (isOutOfRange(current, stop, step)) {
    return 0;
  }
  return std::abs((stop - current) / step);
}

inline RawObject RangeIterator::next() {
  auto ret = SmallInt::cast(instanceVariableAt(kCurValueOffset));
  auto cur = ret->value();

  auto range = Range::cast(instanceVariableAt(kRangeOffset));
  auto stop = range->stop();
  auto step = range->step();

  // TODO: range overflow is unchecked. Since a correct implementation
  // has to support arbitrary precision anyway, there's no point in checking
  // for overflow.
  if (isOutOfRange(cur, stop, step)) {
    // TODO: Use StopIteration for control flow.
    return Error::object();
  }

  instanceVariableAtPut(kCurValueOffset, SmallInt::fromWord(cur + step));
  return ret;
}

// Slice

inline RawObject Slice::start() { return instanceVariableAt(kStartOffset); }

inline void Slice::setStart(RawObject value) {
  instanceVariableAtPut(kStartOffset, value);
}

inline RawObject Slice::stop() { return instanceVariableAt(kStopOffset); }

inline void Slice::setStop(RawObject value) {
  instanceVariableAtPut(kStopOffset, value);
}

inline RawObject Slice::step() { return instanceVariableAt(kStepOffset); }

inline void Slice::setStep(RawObject value) {
  instanceVariableAtPut(kStepOffset, value);
}

// StaticMethod

inline RawObject StaticMethod::function() {
  return instanceVariableAt(kFunctionOffset);
}

inline void StaticMethod::setFunction(RawObject function) {
  instanceVariableAtPut(kFunctionOffset, function);
}

// Dict

inline word Dict::numItems() {
  return SmallInt::cast(instanceVariableAt(kNumItemsOffset))->value();
}

inline void Dict::setNumItems(word num_items) {
  instanceVariableAtPut(kNumItemsOffset, SmallInt::fromWord(num_items));
}

inline RawObject Dict::data() { return instanceVariableAt(kDataOffset); }

inline void Dict::setData(RawObject data) {
  instanceVariableAtPut(kDataOffset, data);
}

// Function

inline RawObject Function::annotations() {
  return instanceVariableAt(kAnnotationsOffset);
}

inline void Function::setAnnotations(RawObject annotations) {
  return instanceVariableAtPut(kAnnotationsOffset, annotations);
}

inline RawObject Function::closure() {
  return instanceVariableAt(kClosureOffset);
}

inline void Function::setClosure(RawObject closure) {
  return instanceVariableAtPut(kClosureOffset, closure);
}

inline RawObject Function::code() { return instanceVariableAt(kCodeOffset); }

inline void Function::setCode(RawObject code) {
  return instanceVariableAtPut(kCodeOffset, code);
}

inline RawObject Function::defaults() {
  return instanceVariableAt(kDefaultsOffset);
}

inline void Function::setDefaults(RawObject defaults) {
  return instanceVariableAtPut(kDefaultsOffset, defaults);
}

inline bool Function::hasDefaults() { return !defaults()->isNoneType(); }

inline RawObject Function::doc() { return instanceVariableAt(kDocOffset); }

inline void Function::setDoc(RawObject doc) {
  instanceVariableAtPut(kDocOffset, doc);
}

inline Function::Entry Function::entry() {
  RawObject object = instanceVariableAt(kEntryOffset);
  DCHECK(object->isSmallInt(), "entry address must look like a SmallInt");
  return bit_cast<Function::Entry>(object);
}

inline void Function::setEntry(Function::Entry entry) {
  auto object = SmallInt::fromFunctionPointer(entry);
  instanceVariableAtPut(kEntryOffset, object);
}

inline Function::Entry Function::entryKw() {
  RawObject object = instanceVariableAt(kEntryKwOffset);
  DCHECK(object->isSmallInt(), "entryKw address must look like a SmallInt");
  return bit_cast<Function::Entry>(object);
}

inline void Function::setEntryKw(Function::Entry entry_kw) {
  auto object = SmallInt::fromFunctionPointer(entry_kw);
  instanceVariableAtPut(kEntryKwOffset, object);
}

Function::Entry Function::entryEx() {
  RawObject object = instanceVariableAt(kEntryExOffset);
  DCHECK(object->isSmallInt(), "entryEx address must look like a SmallInt");
  return bit_cast<Function::Entry>(object);
}

void Function::setEntryEx(Function::Entry entry_ex) {
  auto object = SmallInt::fromFunctionPointer(entry_ex);
  instanceVariableAtPut(kEntryExOffset, object);
}

inline RawObject Function::globals() {
  return instanceVariableAt(kGlobalsOffset);
}

inline void Function::setGlobals(RawObject globals) {
  return instanceVariableAtPut(kGlobalsOffset, globals);
}

inline RawObject Function::kwDefaults() {
  return instanceVariableAt(kKwDefaultsOffset);
}

inline void Function::setKwDefaults(RawObject kw_defaults) {
  return instanceVariableAtPut(kKwDefaultsOffset, kw_defaults);
}

inline RawObject Function::module() {
  return instanceVariableAt(kModuleOffset);
}

inline void Function::setModule(RawObject module) {
  return instanceVariableAtPut(kModuleOffset, module);
}

inline RawObject Function::name() { return instanceVariableAt(kNameOffset); }

inline void Function::setName(RawObject name) {
  instanceVariableAtPut(kNameOffset, name);
}

inline RawObject Function::qualname() {
  return instanceVariableAt(kQualnameOffset);
}

inline void Function::setQualname(RawObject qualname) {
  instanceVariableAtPut(kQualnameOffset, qualname);
}

inline RawObject Function::fastGlobals() {
  return instanceVariableAt(kFastGlobalsOffset);
}

inline void Function::setFastGlobals(RawObject fast_globals) {
  return instanceVariableAtPut(kFastGlobalsOffset, fast_globals);
}

// Instance

inline word Instance::allocationSize(word num_attr) {
  DCHECK(num_attr >= 0, "invalid number of attributes %ld", num_attr);
  word size = headerSize(num_attr) + num_attr * kPointerSize;
  return Utils::maximum(kMinimumSize, Utils::roundUp(size, kPointerSize));
}

// List

inline RawObject List::items() { return instanceVariableAt(kItemsOffset); }

inline void List::setItems(RawObject new_items) {
  instanceVariableAtPut(kItemsOffset, new_items);
}

inline word List::capacity() { return ObjectArray::cast(items())->length(); }

inline word List::allocated() {
  return SmallInt::cast(instanceVariableAt(kAllocatedOffset))->value();
}

inline void List::setAllocated(word new_allocated) {
  instanceVariableAtPut(kAllocatedOffset, SmallInt::fromWord(new_allocated));
}

inline void List::atPut(word index, RawObject value) {
  DCHECK_INDEX(index, allocated());
  RawObject items = instanceVariableAt(kItemsOffset);
  ObjectArray::cast(items)->atPut(index, value);
}

inline RawObject List::at(word index) {
  DCHECK_INDEX(index, allocated());
  return ObjectArray::cast(items())->at(index);
}

// Module

inline RawObject Module::name() { return instanceVariableAt(kNameOffset); }

inline void Module::setName(RawObject name) {
  instanceVariableAtPut(kNameOffset, name);
}

inline RawObject Module::dict() { return instanceVariableAt(kDictOffset); }

inline void Module::setDict(RawObject dict) {
  instanceVariableAtPut(kDictOffset, dict);
}

inline RawObject Module::def() { return instanceVariableAt(kDefOffset); }

inline void Module::setDef(RawObject dict) {
  instanceVariableAtPut(kDefOffset, dict);
}

// Str

inline bool Str::equalsCStr(const char* c_str) {
  const char* cp = c_str;
  const word len = length();
  for (word i = 0; i < len; i++, cp++) {
    char ch = *cp;
    if (ch == '\0' || ch != charAt(i)) {
      return false;
    }
  }
  return *cp == '\0';
}

inline byte Str::charAt(word index) {
  if (isSmallStr()) {
    return SmallStr::cast(*this)->charAt(index);
  }
  DCHECK(isLargeStr(), "unexpected type");
  return LargeStr::cast(*this)->charAt(index);
}

inline word Str::length() {
  if (isSmallStr()) {
    return SmallStr::cast(*this)->length();
  }
  DCHECK(isLargeStr(), "unexpected type");
  return LargeStr::cast(*this)->length();
}

inline word Str::compare(RawObject string) {
  RawStr that = Str::cast(string);
  word length = Utils::minimum(this->length(), that->length());
  for (word i = 0; i < length; i++) {
    word diff = this->charAt(i) - that->charAt(i);
    if (diff != 0) {
      return (diff > 0) ? 1 : -1;
    }
  }
  word diff = this->length() - that->length();
  return (diff > 0) ? 1 : ((diff < 0) ? -1 : 0);
}

inline bool Str::equals(RawObject that) {
  if (isSmallStr()) {
    return *this == that;
  }
  DCHECK(isLargeStr(), "unexpected type");
  return LargeStr::cast(*this)->equals(that);
}

inline void Str::copyTo(byte* dst, word length) {
  if (isSmallStr()) {
    SmallStr::cast(*this)->copyTo(dst, length);
    return;
  }
  DCHECK(isLargeStr(), "unexpected type");
  return LargeStr::cast(*this)->copyTo(dst, length);
}

inline char* Str::toCStr() {
  if (isSmallStr()) {
    return SmallStr::cast(*this)->toCStr();
  }
  DCHECK(isLargeStr(), "unexpected type");
  return LargeStr::cast(*this)->toCStr();
}

// LargeStr

inline word LargeStr::allocationSize(word length) {
  DCHECK(length > SmallStr::kMaxLength, "length %ld overflows", length);
  word size = headerSize(length) + length;
  return Utils::maximum(kMinimumSize, Utils::roundUp(size, kPointerSize));
}

inline byte LargeStr::charAt(word index) {
  DCHECK_INDEX(index, length());
  return *reinterpret_cast<byte*>(address() + index);
}

// ValueCell

inline RawObject ValueCell::value() { return instanceVariableAt(kValueOffset); }

inline void ValueCell::setValue(RawObject object) {
  instanceVariableAtPut(kValueOffset, object);
}

inline bool ValueCell::isUnbound() { return *this == value(); }

inline void ValueCell::makeUnbound() { setValue(*this); }

// Set

inline word Set::numItems() {
  return SmallInt::cast(instanceVariableAt(kNumItemsOffset))->value();
}

inline void Set::setNumItems(word num_items) {
  instanceVariableAtPut(kNumItemsOffset, SmallInt::fromWord(num_items));
}

inline RawObject Set::data() { return instanceVariableAt(kDataOffset); }

inline void Set::setData(RawObject data) {
  instanceVariableAtPut(kDataOffset, data);
}

// BoundMethod

inline RawObject BoundMethod::function() {
  return instanceVariableAt(kFunctionOffset);
}

inline void BoundMethod::setFunction(RawObject function) {
  instanceVariableAtPut(kFunctionOffset, function);
}

inline RawObject BoundMethod::self() { return instanceVariableAt(kSelfOffset); }

inline void BoundMethod::setSelf(RawObject self) {
  instanceVariableAtPut(kSelfOffset, self);
}

// ClassMethod

inline RawObject ClassMethod::function() {
  return instanceVariableAt(kFunctionOffset);
}

inline void ClassMethod::setFunction(RawObject function) {
  instanceVariableAtPut(kFunctionOffset, function);
}

// WeakRef

inline RawObject WeakRef::referent() {
  return instanceVariableAt(kReferentOffset);
}

inline void WeakRef::setReferent(RawObject referent) {
  instanceVariableAtPut(kReferentOffset, referent);
}

inline RawObject WeakRef::callback() {
  return instanceVariableAt(kCallbackOffset);
}

inline void WeakRef::setCallback(RawObject callable) {
  instanceVariableAtPut(kCallbackOffset, callable);
}

inline RawObject WeakRef::link() { return instanceVariableAt(kLinkOffset); }

inline void WeakRef::setLink(RawObject reference) {
  instanceVariableAtPut(kLinkOffset, reference);
}

// Layout

inline LayoutId Layout::id() {
  return static_cast<LayoutId>(header()->hashCode());
}

inline void Layout::setId(LayoutId id) {
  setHeader(header()->withHashCode(static_cast<word>(id)));
}

inline void Layout::setDescribedClass(RawObject type) {
  instanceVariableAtPut(kDescribedClassOffset, type);
}

inline RawObject Layout::describedClass() {
  return instanceVariableAt(kDescribedClassOffset);
}

inline void Layout::setInObjectAttributes(RawObject attributes) {
  instanceVariableAtPut(kInObjectAttributesOffset, attributes);
}

inline RawObject Layout::inObjectAttributes() {
  return instanceVariableAt(kInObjectAttributesOffset);
}

inline void Layout::setOverflowAttributes(RawObject attributes) {
  instanceVariableAtPut(kOverflowAttributesOffset, attributes);
}

inline word Layout::instanceSize() {
  return SmallInt::cast(instanceVariableAt(kInstanceSizeOffset))->value();
}

inline void Layout::setInstanceSize(word size) {
  instanceVariableAtPut(kInstanceSizeOffset, SmallInt::fromWord(size));
}

inline RawObject Layout::overflowAttributes() {
  return instanceVariableAt(kOverflowAttributesOffset);
}

inline void Layout::setAdditions(RawObject additions) {
  instanceVariableAtPut(kAdditionsOffset, additions);
}

inline RawObject Layout::additions() {
  return instanceVariableAt(kAdditionsOffset);
}

inline void Layout::setDeletions(RawObject deletions) {
  instanceVariableAtPut(kDeletionsOffset, deletions);
}

inline RawObject Layout::deletions() {
  return instanceVariableAt(kDeletionsOffset);
}

inline word Layout::overflowOffset() {
  return SmallInt::cast(instanceVariableAt(kOverflowOffsetOffset))->value();
}

inline void Layout::setOverflowOffset(word offset) {
  instanceVariableAtPut(kOverflowOffsetOffset, SmallInt::fromWord(offset));
}

inline word Layout::numInObjectAttributes() {
  return SmallInt::cast(instanceVariableAt(kNumInObjectAttributesOffset))
      ->value();
}

inline void Layout::setNumInObjectAttributes(word count) {
  instanceVariableAtPut(kNumInObjectAttributesOffset,
                        SmallInt::fromWord(count));
  setOverflowOffset(count * kPointerSize);
  setInstanceSize(numInObjectAttributes() + 1);
}

// SetIterator

inline RawObject SetIterator::set() { return instanceVariableAt(kSetOffset); }

inline void SetIterator::setSet(RawObject set) {
  instanceVariableAtPut(kSetOffset, set);
  instanceVariableAtPut(kIndexOffset, SmallInt::fromWord(0));
  instanceVariableAtPut(kConsumedCountOffset, SmallInt::fromWord(0));
}

inline word SetIterator::consumedCount() {
  return SmallInt::cast(instanceVariableAt(kConsumedCountOffset))->value();
}

inline void SetIterator::setConsumedCount(word consumed) {
  instanceVariableAtPut(kConsumedCountOffset, SmallInt::fromWord(consumed));
}

inline word SetIterator::index() {
  return SmallInt::cast(instanceVariableAt(kIndexOffset))->value();
}

inline void SetIterator::setIndex(word index) {
  instanceVariableAtPut(kIndexOffset, SmallInt::fromWord(index));
}

inline RawObject SetIterator::next() {
  word idx = index();
  RawSet underlying = Set::cast(set());
  RawObjectArray data = ObjectArray::cast(underlying->data());
  word length = data->length();
  // Find the next non empty bucket
  while (idx < length && (Set::Bucket::isTombstone(data, idx) ||
                          Set::Bucket::isEmpty(data, idx))) {
    idx += Set::Bucket::kNumPointers;
  }
  if (idx >= length) {
    return Error::object();
  }
  setConsumedCount(consumedCount() + 1);
  word new_idx = (idx + Set::Bucket::kNumPointers);
  setIndex(new_idx);
  return Set::Bucket::key(data, idx);
}

inline word SetIterator::pendingLength() {
  RawSet set = Set::cast(instanceVariableAt(kSetOffset));
  return set->numItems() - consumedCount();
}

// Super

inline RawObject Super::type() { return instanceVariableAt(kTypeOffset); }

inline void Super::setType(RawObject tp) {
  DCHECK(tp->isType(), "expected type");
  instanceVariableAtPut(kTypeOffset, tp);
}

inline RawObject Super::object() { return instanceVariableAt(kObjectOffset); }

inline void Super::setObject(RawObject obj) {
  instanceVariableAtPut(kObjectOffset, obj);
}

inline RawObject Super::objectType() {
  return instanceVariableAt(kObjectTypeOffset);
}

inline void Super::setObjectType(RawObject tp) {
  DCHECK(tp->isType(), "expected type");
  instanceVariableAtPut(kObjectTypeOffset, tp);
}

// TupleIterator

inline RawObject TupleIterator::tuple() {
  return instanceVariableAt(kTupleOffset);
}

inline void TupleIterator::setTuple(RawObject tuple) {
  instanceVariableAtPut(kTupleOffset, tuple);
  instanceVariableAtPut(kIndexOffset, SmallInt::fromWord(0));
}

inline word TupleIterator::index() {
  return SmallInt::cast(instanceVariableAt(kIndexOffset))->value();
}

inline void TupleIterator::setIndex(word index) {
  instanceVariableAtPut(kIndexOffset, SmallInt::fromWord(index));
}

inline RawObject TupleIterator::next() {
  word idx = index();
  RawObjectArray underlying = ObjectArray::cast(tuple());
  if (idx >= underlying->length()) {
    return Error::object();
  }

  RawObject item = underlying->at(idx);
  setIndex(idx + 1);
  return item;
}

// GeneratorBase

inline RawObject GeneratorBase::heapFrame() {
  return instanceVariableAt(kFrameOffset);
}

inline void GeneratorBase::setHeapFrame(RawObject obj) {
  instanceVariableAtPut(kFrameOffset, obj);
}

}  // namespace python
