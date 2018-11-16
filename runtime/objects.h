#pragma once

#include "globals.h"
#include "utils.h"
#include "view.h"

#include <limits>
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

// Add functionality common to all Object subclasses. This only contains cast()
// for now, but we have plans to add more in the future, hence the more generic
// name.
#define RAW_OBJECT_COMMON(ty)                                                  \
  static ty* cast(Object* object) {                                            \
    DCHECK(object->is##ty(), "invalid cast, expected " #ty);                   \
    return reinterpret_cast<ty*>(object);                                      \
  }

class Object {
 public:
  // Getters and setters.
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

  static bool equals(Object* lhs, Object* rhs);

  // Constants

  // The bottom five bits of immediate objects are used as the class id when
  // indexing into the class table in the runtime.
  static const uword kImmediateClassTableIndexMask = (1 << 5) - 1;

  RAW_OBJECT_COMMON(Object)

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Object);
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

  word compare(Int* other);

  double floatValue();

  word bitLength();

  bool isNegative();
  bool isPositive();
  bool isZero();

  RAW_OBJECT_COMMON(Int)

 private:
  static word compareDigits(View<uword> lhs, View<uword> rhs);
  DISALLOW_COPY_AND_ASSIGN(Int);
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
  static SmallInt* fromWord(word value);
  static constexpr bool isValid(word value) {
    return (value >= kMinValue) && (value <= kMaxValue);
  }

  template <typename T>
  static SmallInt* fromFunctionPointer(T pointer);

  template <typename T>
  T asFunctionPointer();

  // Tags.
  static const int kTag = 0;
  static const int kTagSize = 1;
  static const uword kTagMask = (1 << kTagSize) - 1;

  // Constants
  static const word kMinValue = -(1L << (kBitsPerPointer - (kTagSize + 1)));
  static const word kMaxValue = (1L << (kBitsPerPointer - (kTagSize + 1))) - 1;

  RAW_OBJECT_COMMON(SmallInt)

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(SmallInt);
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
  Header* withHashCode(word value);

  ObjectFormat format();

  LayoutId layoutId();
  Header* withLayoutId(LayoutId layout_id);

  word count();

  bool hasOverflow();

  static Header* from(word count, word hash, LayoutId layout_id,
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

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Header);
};

class Bool : public Object {
 public:
  // Getters and setters.
  bool value();

  // Singletons
  static Bool* trueObj();
  static Bool* falseObj();

  // Conversion.
  static Bool* fromBool(bool value);
  static Bool* negate(Object* value);

  // Tags.
  static const int kTag = 7;  // 0b00111
  static const int kTagSize = 5;
  static const uword kTagMask = (1 << kTagSize) - 1;

  RAW_OBJECT_COMMON(Bool)

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Bool);
};

class NoneType : public Object {
 public:
  // Singletons.
  static NoneType* object();

  // Tags.
  static const int kTag = 15;  // 0b01111
  static const int kTagSize = 5;
  static const uword kTagMask = (1 << kTagSize) - 1;

  RAW_OBJECT_COMMON(NoneType)

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(NoneType);
};

// Error is a special object type, internal to the runtime. It is used to signal
// that an error has occurred inside the runtime or native code, e.g. an
// exception has been thrown.
class Error : public Object {
 public:
  // Singletons.
  static Error* object();

  // Tagging.
  static const int kTag = 23;  // 0b10111
  static const int kTagSize = 5;
  static const uword kTagMask = (1 << kTagSize) - 1;

  RAW_OBJECT_COMMON(Error)

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Error);
};

// Super class of common string functionality
class Str : public Object {
 public:
  // Getters and setters.
  byte charAt(word index);
  word length();
  void copyTo(byte* dst, word length);

  // Equality checks.
  word compare(Object* string);
  bool equals(Object* that);
  bool equalsCStr(const char* c_str);

  // Conversion to an unescaped C string.  The underlying memory is allocated
  // with malloc and must be freed by the caller.
  char* toCStr();

  RAW_OBJECT_COMMON(Str)

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Str);
};

class SmallStr : public Object {
 public:
  // Conversion.
  static Object* fromCStr(const char* value);
  static Object* fromBytes(View<byte> data);

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

  DISALLOW_IMPLICIT_CONSTRUCTORS(SmallStr);
};

// Heap objects

class HeapObject : public Object {
 public:
  // Getters and setters.
  uword address();
  uword baseAddress();
  Header* header();
  void setHeader(Header* header);
  word headerOverflow();
  void setHeaderAndOverflow(word count, word hash, LayoutId id,
                            ObjectFormat format);
  word headerCountOrOverflow();
  word size();

  // Conversion.
  static HeapObject* fromAddress(uword address);

  // Sizing
  static word headerSize(word count);

  void initialize(word size, Object* value);

  // Garbage collection.
  bool isRoot();
  bool isForwarding();
  Object* forward();
  void forwardTo(Object* object);

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
  Object* instanceVariableAt(word offset);
  void instanceVariableAtPut(word offset, Object* value);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(HeapObject);

  friend class Runtime;
};

class BaseException : public HeapObject {
 public:
  // Getters and setters.
  Object* args();
  void setArgs(Object* args);

  Object* traceback();
  void setTraceback(Object* traceback);

  Object* cause();
  void setCause(Object* cause);

  Object* context();
  void setContext(Object* context);

  static const int kArgsOffset = HeapObject::kSize;
  static const int kTracebackOffset = kArgsOffset + kPointerSize;
  static const int kCauseOffset = kTracebackOffset + kPointerSize;
  static const int kContextOffset = kCauseOffset + kPointerSize;
  static const int kSize = kContextOffset + kPointerSize;

  RAW_OBJECT_COMMON(BaseException)

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(BaseException);
};

class Exception : public BaseException {
 public:
  RAW_OBJECT_COMMON(Exception)

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Exception);
};

class StopIteration : public BaseException {
 public:
  // Getters and setters.
  Object* value();
  void setValue(Object* value);

  static const int kValueOffset = BaseException::kSize;
  static const int kSize = kValueOffset + kPointerSize;

  RAW_OBJECT_COMMON(StopIteration)

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(StopIteration);
};

class SystemExit : public BaseException {
 public:
  Object* code();
  void setCode(Object* code);

  static const int kCodeOffset = BaseException::kSize;
  static const int kSize = kCodeOffset + kPointerSize;

  RAW_OBJECT_COMMON(SystemExit)

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(SystemExit);
};

class RuntimeError : public Exception {
 public:
  RAW_OBJECT_COMMON(RuntimeError)

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(RuntimeError);
};

class NotImplementedError : public RuntimeError {
 public:
  RAW_OBJECT_COMMON(NotImplementedError)

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(NotImplementedError);
};

class ImportError : public Exception {
 public:
  // Getters and setters
  Object* msg();
  void setMsg(Object* msg);

  Object* name();
  void setName(Object* name);

  Object* path();
  void setPath(Object* path);

  static const int kMsgOffset = BaseException::kSize;
  static const int kNameOffset = kMsgOffset + kPointerSize;
  static const int kPathOffset = kNameOffset + kPointerSize;
  static const int kSize = kPathOffset + kPointerSize;

  RAW_OBJECT_COMMON(ImportError)

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ImportError);
};

class ModuleNotFoundError : public ImportError {
 public:
  RAW_OBJECT_COMMON(ModuleNotFoundError)

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ModuleNotFoundError);
};

class LookupError : public Exception {
 public:
  RAW_OBJECT_COMMON(LookupError)

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(LookupError);
};

class IndexError : public LookupError {
 public:
  RAW_OBJECT_COMMON(IndexError)

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(IndexError);
};

class KeyError : public LookupError {
 public:
  RAW_OBJECT_COMMON(KeyError)

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(KeyError);
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
  Object* instanceLayout();
  void setInstanceLayout(Object* layout);

  Object* mro();
  void setMro(Object* object_array);

  Object* name();
  void setName(Object* name);

  Object* flags();
  void setFlags(Object* value);
  void setFlag(Flag flag);
  bool hasFlag(Flag flag);

  Object* dict();
  void setDict(Object* name);

  // Int holding a pointer to a PyTypeObject
  // Only set on classes that were initialized through PyType_Ready
  Object* extensionType();
  void setExtensionType(Object* pytype);

  // builtin base related
  Object* builtinBaseClass();
  void setBuiltinBaseClass(Object* base);

  bool isIntrinsicOrExtension();

  // Casting.
  static Type* cast(Object* object);

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

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Type);
};

class Array : public HeapObject {
 public:
  word length();

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Array);
};

class Bytes : public Array {
 public:
  // Getters and setters.
  byte byteAt(word index);
  void byteAtPut(word index, byte value);

  // Sizing.
  static word allocationSize(word length);

  RAW_OBJECT_COMMON(Bytes)

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Bytes);
};

class ObjectArray : public Array {
 public:
  // Getters and setters.
  Object* at(word index);
  void atPut(word index, Object* value);

  // Sizing.
  static word allocationSize(word length);

  void copyTo(Object* dst);

  void replaceFromWith(word start, Object* array);

  bool contains(Object* object);

  RAW_OBJECT_COMMON(ObjectArray)

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ObjectArray);
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
  bool equals(Object* that);

  // Conversion to an unescaped C string.  The underlying memory is allocated
  // with malloc and must be freed by the caller.
  char* toCStr();

  friend class Heap;
  friend class Object;
  friend class Runtime;
  friend class Str;

  DISALLOW_IMPLICIT_CONSTRUCTORS(LargeStr);
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
  uword digitAt(word index);
  void digitAtPut(word index, uword digit);

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

  // Backing array of unsigned words. Extremely GC-unsafe; use with care.
  View<uword> digits();

  DISALLOW_COPY_AND_ASSIGN(LargeInt);
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

 private:
  DISALLOW_COPY_AND_ASSIGN(Float);
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

 private:
  DISALLOW_COPY_AND_ASSIGN(Complex);
};

class Property : public HeapObject {
 public:
  // Getters and setters
  Object* getter();
  void setGetter(Object* function);

  Object* setter();
  void setSetter(Object* function);

  Object* deleter();
  void setDeleter(Object* function);

  // Layout
  static const int kGetterOffset = HeapObject::kSize;
  static const int kSetterOffset = kGetterOffset + kPointerSize;
  static const int kDeleterOffset = kSetterOffset + kPointerSize;
  static const int kSize = kDeleterOffset + kPointerSize;

  RAW_OBJECT_COMMON(Property)

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Property);
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

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Range);
};

class RangeIterator : public HeapObject {
 public:
  // Getters and setters.

  // Bind the iterator to a specific range. The binding should not be changed
  // after creation.
  void setRange(Object* range);

  // Iteration.
  Object* next();

  // Number of unconsumed values in the range iterator
  word pendingLength();

  // Layout.
  static const int kRangeOffset = HeapObject::kSize;
  static const int kCurValueOffset = kRangeOffset + kPointerSize;
  static const int kSize = kCurValueOffset + kPointerSize;

  RAW_OBJECT_COMMON(RangeIterator)

 private:
  static bool isOutOfRange(word cur, word stop, word step);

  DISALLOW_IMPLICIT_CONSTRUCTORS(RangeIterator);
};

class Slice : public HeapObject {
 public:
  // Getters and setters.
  Object* start();
  void setStart(Object* value);

  Object* stop();
  void setStop(Object* value);

  Object* step();
  void setStep(Object* value);

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

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Slice);
};

class StaticMethod : public HeapObject {
 public:
  // Getters and setters
  Object* function();
  void setFunction(Object* function);

  // Layout
  static const int kFunctionOffset = HeapObject::kSize;
  static const int kSize = kFunctionOffset + kPointerSize;

  RAW_OBJECT_COMMON(StaticMethod)

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(StaticMethod);
};

class ListIterator : public HeapObject {
 public:
  // Getters and setters.
  word index();
  void setIndex(word index);

  Object* list();
  void setList(Object* list);

  // Iteration.
  Object* next();

  // Layout.
  static const int kListOffset = HeapObject::kSize;
  static const int kIndexOffset = kListOffset + kPointerSize;
  static const int kSize = kIndexOffset + kPointerSize;

  RAW_OBJECT_COMMON(ListIterator)

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ListIterator);
};

class SetIterator : public HeapObject {
 public:
  // Getters and setters
  word consumedCount();
  void setConsumedCount(word consumed);

  word index();
  void setIndex(word index);

  Object* set();
  void setSet(Object* set);

  // Iteration.
  Object* next();

  // Number of unconsumed values in the set iterator
  word pendingLength();

  // Layout
  static const int kSetOffset = HeapObject::kSize;
  static const int kIndexOffset = kSetOffset + kPointerSize;
  static const int kConsumedCountOffset = kIndexOffset + kPointerSize;
  static const int kSize = kConsumedCountOffset + kPointerSize;

  RAW_OBJECT_COMMON(SetIterator)

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(SetIterator);
};

class TupleIterator : public HeapObject {
 public:
  // Getters and setters.
  word index();

  void setIndex(word index);

  Object* tuple();

  void setTuple(Object* tuple);

  // Iteration.
  Object* next();

  // Layout.
  static const int kTupleOffset = HeapObject::kSize;
  static const int kIndexOffset = kTupleOffset + kPointerSize;
  static const int kSize = kIndexOffset + kPointerSize;

  RAW_OBJECT_COMMON(TupleIterator)

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(TupleIterator);
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

  Object* cellvars();
  void setCellvars(Object* value);
  word numCellvars();

  Object* code();
  void setCode(Object* value);

  Object* consts();
  void setConsts(Object* value);

  Object* filename();
  void setFilename(Object* value);

  word firstlineno();
  void setFirstlineno(word value);

  word flags();
  void setFlags(word value);

  Object* freevars();
  void setFreevars(Object* value);
  word numFreevars();

  word kwonlyargcount();
  void setKwonlyargcount(word value);

  Object* lnotab();
  void setLnotab(Object* value);

  Object* name();
  void setName(Object* value);

  Object* names();
  void setNames(Object* value);

  word nlocals();
  void setNlocals(word value);

  // The total number of variables in this code object: normal locals, cell
  // variables, and free variables.
  word totalVars();

  word stacksize();
  void setStacksize(word value);

  Object* varnames();
  void setVarnames(Object* value);

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

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Code);
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
  using Entry = Object* (*)(Thread*, Frame*, word);

  // Getters and setters.

  // A dict containing parameter annotations
  Object* annotations();
  void setAnnotations(Object* annotations);

  // The code object backing this function or None
  Object* code();
  void setCode(Object* code);

  // A tuple of cell objects that contain bindings for the function's free
  // variables. Read-only to user code.
  Object* closure();
  void setClosure(Object* closure);

  // A tuple containing default values for arguments with defaults. Read-only
  // to user code.
  Object* defaults();
  void setDefaults(Object* defaults);
  bool hasDefaults();

  // The function's docstring
  Object* doc();
  void setDoc(Object* doc);

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
  Object* globals();
  void setGlobals(Object* globals);

  // A dict containing defaults for keyword-only parameters
  Object* kwDefaults();
  void setKwDefaults(Object* kw_defaults);

  // The name of the module the function was defined in
  Object* module();
  void setModule(Object* module);

  // The function's name
  Object* name();
  void setName(Object* name);

  // The function's qualname
  Object* qualname();
  void setQualname(Object* qualname);

  // The pre-computed object array provided fast globals access.
  // fastGlobals[arg] == globals[names[arg]]
  Object* fastGlobals();
  void setFastGlobals(Object* fast_globals);

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

 private:
  DISALLOW_COPY_AND_ASSIGN(Function);
};

class Instance : public HeapObject {
 public:
  // Sizing.
  static word allocationSize(word num_attributes);

  RAW_OBJECT_COMMON(Instance)

 private:
  DISALLOW_COPY_AND_ASSIGN(Instance);
};

class Module : public HeapObject {
 public:
  // Setters and getters.
  Object* name();
  void setName(Object* name);

  Object* dict();
  void setDict(Object* dict);

  // Contains the numeric address of mode definition object for C-API modules or
  // zero if the module was not defined through the C-API.
  Object* def();
  void setDef(Object* def);

  // Layout.
  static const int kNameOffset = HeapObject::kSize;
  static const int kDictOffset = kNameOffset + kPointerSize;
  static const int kDefOffset = kDictOffset + kPointerSize;
  static const int kSize = kDefOffset + kPointerSize;

  RAW_OBJECT_COMMON(Module)

 private:
  DISALLOW_COPY_AND_ASSIGN(Module);
};

class NotImplemented : public HeapObject {
 public:
  // Layout
  // kPaddingOffset is not used, but the GC expects the object to be
  // at least one word.
  static const int kPaddingOffset = HeapObject::kSize;
  static const int kSize = kPaddingOffset + kPointerSize;

  RAW_OBJECT_COMMON(NotImplemented)

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(NotImplemented);
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
  Object* data();
  void setData(Object* data);

  // Number of items currently in the dict
  word numItems();
  void setNumItems(word num_items);

  // Layout.
  static const int kNumItemsOffset = HeapObject::kSize;
  static const int kDataOffset = kNumItemsOffset + kPointerSize;
  static const int kSize = kDataOffset + kPointerSize;

  RAW_OBJECT_COMMON(Dict)

 private:
  DISALLOW_COPY_AND_ASSIGN(Dict);
};

// Helper class for manipulating buckets in the ObjectArray that backs the
// dict
class Dict::Bucket {
 public:
  // none of these operations do bounds checking on the backing array
  static word getIndex(ObjectArray* data, Object* hash) {
    word nbuckets = data->length() / kNumPointers;
    DCHECK(Utils::isPowerOfTwo(nbuckets), "%ld is not a power of 2", nbuckets);
    word value = SmallInt::cast(hash)->value();
    return (value & (nbuckets - 1)) * kNumPointers;
  }

  static bool hasKey(ObjectArray* data, word index, Object* that_key) {
    return !hash(data, index)->isNoneType() &&
           Object::equals(key(data, index), that_key);
  }

  static Object* hash(ObjectArray* data, word index) {
    return data->at(index + kHashOffset);
  }

  static bool isEmpty(ObjectArray* data, word index) {
    return hash(data, index)->isNoneType() && key(data, index)->isNoneType();
  }

  static bool isFilled(ObjectArray* data, word index) {
    return !(isEmpty(data, index) || isTombstone(data, index));
  }

  static bool isTombstone(ObjectArray* data, word index) {
    return hash(data, index)->isNoneType() && !key(data, index)->isNoneType();
  }

  static Object* key(ObjectArray* data, word index) {
    return data->at(index + kKeyOffset);
  }

  static void set(ObjectArray* data, word index, Object* hash, Object* key,
                  Object* value) {
    data->atPut(index + kHashOffset, hash);
    data->atPut(index + kKeyOffset, key);
    data->atPut(index + kValueOffset, value);
  }

  static void setTombstone(ObjectArray* data, word index) {
    set(data, index, NoneType::object(), Error::object(), NoneType::object());
  }

  static Object* value(ObjectArray* data, word index) {
    return data->at(index + kValueOffset);
  }

  // Layout.
  static const word kHashOffset = 0;
  static const word kKeyOffset = kHashOffset + 1;
  static const word kValueOffset = kKeyOffset + 1;
  static const word kNumPointers = kValueOffset + 1;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Bucket);
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
  Object* data();
  void setData(Object* data);

  // Number of items currently in the set
  word numItems();
  void setNumItems(word num_items);

  // Layout.
  static const int kNumItemsOffset = HeapObject::kSize;
  static const int kDataOffset = kNumItemsOffset + kPointerSize;
  static const int kSize = kDataOffset + kPointerSize;

  RAW_OBJECT_COMMON(Set)

 private:
  DISALLOW_COPY_AND_ASSIGN(Set);
};

// Helper class for manipulating buckets in the ObjectArray that backs the
// set
class Set::Bucket {
 public:
  // none of these operations do bounds checking on the backing array
  static word getIndex(ObjectArray* data, Object* hash) {
    word nbuckets = data->length() / kNumPointers;
    DCHECK(Utils::isPowerOfTwo(nbuckets), "%ld not a power of 2", nbuckets);
    word value = SmallInt::cast(hash)->value();
    return (value & (nbuckets - 1)) * kNumPointers;
  }

  static Object* hash(ObjectArray* data, word index) {
    return data->at(index + kHashOffset);
  }

  static bool hasKey(ObjectArray* data, word index, Object* that_key) {
    return !hash(data, index)->isNoneType() &&
           Object::equals(key(data, index), that_key);
  }

  static bool isEmpty(ObjectArray* data, word index) {
    return hash(data, index)->isNoneType() && key(data, index)->isNoneType();
  }

  static bool isTombstone(ObjectArray* data, word index) {
    return hash(data, index)->isNoneType() && !key(data, index)->isNoneType();
  }

  static Object* key(ObjectArray* data, word index) {
    return data->at(index + kKeyOffset);
  }

  static void set(ObjectArray* data, word index, Object* hash, Object* key) {
    data->atPut(index + kHashOffset, hash);
    data->atPut(index + kKeyOffset, key);
  }

  static void setTombstone(ObjectArray* data, word index) {
    set(data, index, NoneType::object(), Error::object());
  }

  // Layout.
  static const word kHashOffset = 0;
  static const word kKeyOffset = kHashOffset + 1;
  static const word kNumPointers = kKeyOffset + 1;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Bucket);
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
  Object* at(word index);
  void atPut(word index, Object* value);
  Object* items();
  void setItems(Object* new_items);
  word allocated();
  void setAllocated(word new_allocated);

  // Return the total number of elements that may be held without growing the
  // list
  word capacity();

  // Casting.
  static List* cast(Object* object);

  // Layout.
  static const int kItemsOffset = HeapObject::kSize;
  static const int kAllocatedOffset = kItemsOffset + kPointerSize;
  static const int kSize = kAllocatedOffset + kPointerSize;

 private:
  DISALLOW_COPY_AND_ASSIGN(List);
};

class ValueCell : public HeapObject {
 public:
  // Getters and setters
  Object* value();
  void setValue(Object* object);

  bool isUnbound();
  void makeUnbound();

  // Layout.
  static const int kValueOffset = HeapObject::kSize;
  static const int kSize = kValueOffset + kPointerSize;

  RAW_OBJECT_COMMON(ValueCell)

 private:
  DISALLOW_COPY_AND_ASSIGN(ValueCell);
};

class Ellipsis : public HeapObject {
 public:
  // Layout
  // kPaddingOffset is not used, but the GC expects the object to be
  // at least one word.
  static const int kPaddingOffset = HeapObject::kSize;
  static const int kSize = kPaddingOffset + kPointerSize;

  RAW_OBJECT_COMMON(Ellipsis)

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Ellipsis);
};

class WeakRef : public HeapObject {
 public:
  // Getters and setters

  // The object weakly referred to by this instance.
  Object* referent();
  void setReferent(Object* referent);

  // A callable object invoked with the referent as an argument when the
  // referent is deemed to be "near death" and only reachable through this weak
  // reference.
  Object* callback();
  void setCallback(Object* callable);

  // Singly linked list of weak reference objects.  This field is used during
  // garbage collection to represent the set of weak references that had been
  // discovered by the initial trace with an otherwise unreachable referent.
  Object* link();
  void setLink(Object* reference);

  static void enqueueReference(Object* reference, Object** list);
  static Object* dequeueReference(Object** list);
  static Object* spliceQueue(Object* tail1, Object* tail2);

  // Layout.
  static const int kReferentOffset = HeapObject::kSize;
  static const int kCallbackOffset = kReferentOffset + kPointerSize;
  static const int kLinkOffset = kCallbackOffset + kPointerSize;
  static const int kSize = kLinkOffset + kPointerSize;

  RAW_OBJECT_COMMON(WeakRef)

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(WeakRef);
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
  Object* function();
  void setFunction(Object* function);

  // The instance of "self" being bound
  Object* self();
  void setSelf(Object* self);

  // Layout
  static const int kFunctionOffset = HeapObject::kSize;
  static const int kSelfOffset = kFunctionOffset + kPointerSize;
  static const int kSize = kSelfOffset + kPointerSize;

  RAW_OBJECT_COMMON(BoundMethod)

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(BoundMethod);
};

class ClassMethod : public HeapObject {
 public:
  // Getters and setters
  Object* function();
  void setFunction(Object* function);

  // Layout
  static const int kFunctionOffset = HeapObject::kSize;
  static const int kSize = kFunctionOffset + kPointerSize;

  RAW_OBJECT_COMMON(ClassMethod)

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ClassMethod);
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
  Object* describedClass();
  void setDescribedClass(Object* type);

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
  Object* inObjectAttributes();
  void setInObjectAttributes(Object* attributes);

  // Returns an ObjectArray describing the attributes stored in the overflow
  // array of the instance.
  //
  // Each item in the object array is a two element tuple. Each tuple is
  // composed of the following elements, in order:
  //
  //   1. The attribute name (Str)
  //   2. The attribute info (AttributeInfo)
  Object* overflowAttributes();
  void setOverflowAttributes(Object* attributes);

  // Returns a flattened list of tuples. Each tuple is composed of the
  // following elements, in order:
  //
  //   1. The attribute name (Str)
  //   2. The layout that would result if an attribute with that name
  //      was added.
  Object* additions();
  void setAdditions(Object* additions);

  // Returns a flattened list of tuples. Each tuple is composed of the
  // following elements, in order:
  //
  //   1. The attribute name (Str)
  //   2. The layout that would result if an attribute with that name
  //      was deleted.
  Object* deletions();
  void setDeletions(Object* deletions);

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

  DISALLOW_IMPLICIT_CONSTRUCTORS(Layout);
};

class Super : public HeapObject {
 public:
  // getters and setters
  Object* type();
  void setType(Object* tp);
  Object* object();
  void setObject(Object* obj);
  Object* objectType();
  void setObjectType(Object* tp);

  // Layout
  static const int kTypeOffset = HeapObject::kSize;
  static const int kObjectOffset = kTypeOffset + kPointerSize;
  static const int kObjectTypeOffset = kObjectOffset + kPointerSize;
  static const int kSize = kObjectTypeOffset + kPointerSize;

  RAW_OBJECT_COMMON(Super)

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Super);
};

/**
 * Base class containing functionality needed by all objects representing a
 * suspended execution frame: Generator, Coroutine, and AsyncGenerator.
 */
class GeneratorBase : public HeapObject {
 public:
  // Get or set the HeapFrame embedded in this GeneratorBase.
  Object* heapFrame();
  void setHeapFrame(Object* obj);

  // Layout.
  static const int kFrameOffset = HeapObject::kSize;
  static const int kIsRunningOffset = kFrameOffset + kPointerSize;
  static const int kCodeOffset = kIsRunningOffset + kPointerSize;
  static const int kSize = kCodeOffset + kPointerSize;

  RAW_OBJECT_COMMON(GeneratorBase)

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(GeneratorBase);
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

inline bool Object::isObject() { return true; }

inline LayoutId Object::layoutId() {
  if (isHeapObject()) {
    return HeapObject::cast(this)->header()->layoutId();
  }
  if (isSmallInt()) {
    return LayoutId::kSmallInt;
  }
  return static_cast<LayoutId>(reinterpret_cast<uword>(this) &
                               kImmediateClassTableIndexMask);
}

inline bool Object::isType() { return isHeapObjectWithLayout(LayoutId::kType); }

inline bool Object::isClassMethod() {
  return isHeapObjectWithLayout(LayoutId::kClassMethod);
}

inline bool Object::isSmallInt() {
  uword tag = reinterpret_cast<uword>(this) & SmallInt::kTagMask;
  return tag == SmallInt::kTag;
}

inline bool Object::isSmallStr() {
  uword tag = reinterpret_cast<uword>(this) & SmallStr::kTagMask;
  return tag == SmallStr::kTag;
}

inline bool Object::isHeader() {
  uword tag = reinterpret_cast<uword>(this) & Header::kTagMask;
  return tag == Header::kTag;
}

inline bool Object::isBool() {
  uword tag = reinterpret_cast<uword>(this) & Bool::kTagMask;
  return tag == Bool::kTag;
}

inline bool Object::isNoneType() {
  uword tag = reinterpret_cast<uword>(this) & NoneType::kTagMask;
  return tag == NoneType::kTag;
}

inline bool Object::isError() {
  uword tag = reinterpret_cast<uword>(this) & NoneType::kTagMask;
  return tag == Error::kTag;
}

inline bool Object::isHeapObject() {
  uword tag = reinterpret_cast<uword>(this) & HeapObject::kTagMask;
  return tag == HeapObject::kTag;
}

inline bool Object::isHeapObjectWithLayout(LayoutId layout_id) {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() == layout_id;
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
  return HeapObject::cast(this)->header()->layoutId() >
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

inline bool Object::isInt() { return isSmallInt() || isLargeInt(); }

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

inline bool Object::equals(Object* lhs, Object* rhs) {
  return (lhs == rhs) ||
         (lhs->isLargeStr() && LargeStr::cast(lhs)->equals(rhs));
}

// Int

inline word Int::asWord() {
  if (isSmallInt()) {
    return SmallInt::cast(this)->value();
  }
  return LargeInt::cast(this)->asWord();
}

inline void* Int::asCPtr() {
  if (isSmallInt()) {
    return SmallInt::cast(this)->asCPtr();
  }
  return LargeInt::cast(this)->asCPtr();
}

template <typename T>
OptInt<T> Int::asInt() {
  if (isSmallInt()) return SmallInt::cast(this)->asInt<T>();
  return LargeInt::cast(this)->asInt<T>();
}

inline word Int::compare(Int* that) {
  if (this->isSmallInt() && that->isSmallInt()) {
    return this->asWord() - that->asWord();
  }
  if (this->isNegative() != that->isNegative()) {
    return this->isNegative() ? -1 : 1;
  }

  word digit;
  View<uword> lhs(reinterpret_cast<uword*>(&digit), 1);
  if (this->isSmallInt()) {
    digit = this->asWord();
  } else {
    lhs = LargeInt::cast(this)->digits();
  }

  View<uword> rhs(reinterpret_cast<uword*>(&digit), 1);
  if (that->isSmallInt()) {
    digit = that->asWord();
  } else {
    rhs = LargeInt::cast(that)->digits();
  }
  return compareDigits(lhs, rhs);
}

inline word Int::compareDigits(View<uword> lhs, View<uword> rhs) {
  if (lhs.length() > rhs.length()) {
    return 1;
  }
  if (lhs.length() < rhs.length()) {
    return -1;
  }
  for (word i = lhs.length() - 1; i >= 0; i--) {
    if (lhs.get(i) > rhs.get(i)) {
      return 1;
    }
    if (lhs.get(i) < rhs.get(i)) {
      return -1;
    }
  }
  return 0;
}

inline double Int::floatValue() {
  if (isSmallInt()) {
    return static_cast<double>(asWord());
  }
  LargeInt* large_int = LargeInt::cast(this);
  if (large_int->numDigits() == 1) {
    return static_cast<double>(asWord());
  }
  // TODO(T30610701): Handle arbitrary precision LargeInts
  UNIMPLEMENTED("LargeInts with > 1 digit");
}

inline word Int::bitLength() {
  if (isSmallInt()) {
    uword self = static_cast<uword>(std::abs(SmallInt::cast(this)->value()));
    return Utils::highestBit(self);
  }
  return LargeInt::cast(this)->bitLength();
}

inline bool Int::isPositive() {
  if (isSmallInt()) {
    return SmallInt::cast(this)->value() > 0;
  }
  return LargeInt::cast(this)->isPositive();
}

inline bool Int::isNegative() {
  if (isSmallInt()) {
    return SmallInt::cast(this)->value() < 0;
  }
  return LargeInt::cast(this)->isNegative();
}

inline bool Int::isZero() {
  if (isSmallInt()) {
    return SmallInt::cast(this)->value() == 0;
  }
  // A LargeInt can never be zero
  return false;
}

// SmallInt

inline word SmallInt::value() {
  return reinterpret_cast<word>(this) >> kTagSize;
}

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

inline SmallInt* SmallInt::fromWord(word value) {
  DCHECK(SmallInt::isValid(value), "invalid cast");
  return reinterpret_cast<SmallInt*>(value << kTagSize);
}

template <typename T>
inline SmallInt* SmallInt::fromFunctionPointer(T pointer) {
  // The bit pattern for a function pointer object must be indistinguishable
  // from that of a small integer object.
  auto object = reinterpret_cast<Object*>(reinterpret_cast<uword>(pointer));
  return SmallInt::cast(object);
}

template <typename T>
inline T SmallInt::asFunctionPointer() {
  return reinterpret_cast<T>(reinterpret_cast<uword>(this));
}

// SmallStr

inline word SmallStr::length() {
  return (reinterpret_cast<word>(this) >> kTagSize) & kMaxLength;
}

inline byte SmallStr::charAt(word index) {
  DCHECK_INDEX(index, length());
  return reinterpret_cast<word>(this) >> (kBitsPerByte * (index + 1));
}

inline void SmallStr::copyTo(byte* dst, word length) {
  DCHECK_BOUND(length, this->length());
  for (word i = 0; i < length; ++i) {
    *dst++ = charAt(i);
  }
}

// Header

inline word Header::count() {
  auto header = reinterpret_cast<uword>(this);
  return (header >> kCountOffset) & kCountMask;
}

inline bool Header::hasOverflow() { return count() == kCountOverflowFlag; }

inline word Header::hashCode() {
  auto header = reinterpret_cast<uword>(this);
  return (header >> kHashCodeOffset) & kHashCodeMask;
}

inline Header* Header::withHashCode(word value) {
  auto header = reinterpret_cast<uword>(this);
  header &= ~(kHashCodeMask << kHashCodeOffset);
  header |= (value & kHashCodeMask) << kHashCodeOffset;
  return reinterpret_cast<Header*>(header);
}

inline LayoutId Header::layoutId() {
  auto header = reinterpret_cast<uword>(this);
  return static_cast<LayoutId>((header >> kLayoutIdOffset) & kLayoutIdMask);
}

inline Header* Header::withLayoutId(LayoutId layout_id) {
  DCHECK_BOUND(static_cast<word>(layout_id), kMaxLayoutId);
  auto header = reinterpret_cast<uword>(this);
  header &= ~(kLayoutIdMask << kLayoutIdOffset);
  header |= (static_cast<word>(layout_id) & kLayoutIdMask) << kLayoutIdOffset;
  return reinterpret_cast<Header*>(header);
}

inline ObjectFormat Header::format() {
  auto header = reinterpret_cast<uword>(this);
  return static_cast<ObjectFormat>((header >> kFormatOffset) & kFormatMask);
}

inline Header* Header::from(word count, word hash, LayoutId id,
                            ObjectFormat format) {
  DCHECK(
      (count >= 0) && ((count <= kCountMax) || (count == kCountOverflowFlag)),
      "bounds violation, %ld not in 0..%d", count, kCountMax);
  uword result = Header::kTag;
  result |= ((count > kCountMax) ? kCountOverflowFlag : count) << kCountOffset;
  result |= hash << kHashCodeOffset;
  result |= static_cast<uword>(id) << kLayoutIdOffset;
  result |= static_cast<uword>(format) << kFormatOffset;
  return reinterpret_cast<Header*>(result);
}

// None

inline NoneType* NoneType::object() {
  return reinterpret_cast<NoneType*>(kTag);
}

// Error

inline Error* Error::object() { return reinterpret_cast<Error*>(kTag); }

// Bool

inline Bool* Bool::trueObj() { return fromBool(true); }

inline Bool* Bool::falseObj() { return fromBool(false); }

inline Bool* Bool::negate(Object* value) {
  DCHECK(value->isBool(), "not a boolean instance");
  return (value == trueObj()) ? falseObj() : trueObj();
}

inline Bool* Bool::fromBool(bool value) {
  return reinterpret_cast<Bool*>((static_cast<uword>(value) << kTagSize) |
                                 kTag);
}

inline bool Bool::value() {
  return (reinterpret_cast<uword>(this) >> kTagSize) ? true : false;
}

// HeapObject

inline uword HeapObject::address() {
  return reinterpret_cast<uword>(this) - HeapObject::kTag;
}

inline uword HeapObject::baseAddress() {
  uword result = address() - Header::kSize;
  if (header()->hasOverflow()) {
    result -= kPointerSize;
  }
  return result;
}

inline Header* HeapObject::header() {
  return Header::cast(instanceVariableAt(kHeaderOffset));
}

inline void HeapObject::setHeader(Header* header) {
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

inline HeapObject* HeapObject::fromAddress(uword address) {
  DCHECK((address & kTagMask) == 0, "invalid cast, expected heap address");
  return reinterpret_cast<HeapObject*>(address + kTag);
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

inline void HeapObject::initialize(word size, Object* value) {
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

inline Object* HeapObject::forward() {
  // When a heap object is forwarding, its second word is the forwarding
  // address.
  return *reinterpret_cast<Object**>(address() + kHeaderOffset + kPointerSize);
}

inline void HeapObject::forwardTo(Object* object) {
  // Overwrite the header with the forwarding marker.
  *reinterpret_cast<uword*>(address() + kHeaderOffset) = kIsForwarded;
  // Overwrite the second word with the forwarding addressing.
  *reinterpret_cast<Object**>(address() + kHeaderOffset + kPointerSize) =
      object;
}

inline Object* HeapObject::instanceVariableAt(word offset) {
  return *reinterpret_cast<Object**>(address() + offset);
}

inline void HeapObject::instanceVariableAtPut(word offset, Object* value) {
  *reinterpret_cast<Object**>(address() + offset) = value;
}

// BaseException

inline Object* BaseException::args() { return instanceVariableAt(kArgsOffset); }

inline void BaseException::setArgs(Object* args) {
  instanceVariableAtPut(kArgsOffset, args);
}

inline Object* BaseException::traceback() {
  return instanceVariableAt(kTracebackOffset);
}

inline void BaseException::setTraceback(Object* traceback) {
  instanceVariableAtPut(kTracebackOffset, traceback);
}

inline Object* BaseException::cause() {
  return instanceVariableAt(kCauseOffset);
}

inline void BaseException::setCause(Object* cause) {
  return instanceVariableAtPut(kCauseOffset, cause);
}

inline Object* BaseException::context() {
  return instanceVariableAt(kContextOffset);
}

inline void BaseException::setContext(Object* context) {
  return instanceVariableAtPut(kContextOffset, context);
}

// StopIteration

inline Object* StopIteration::value() {
  return instanceVariableAt(kValueOffset);
}

inline void StopIteration::setValue(Object* value) {
  instanceVariableAtPut(kValueOffset, value);
}

// SystemExit

inline Object* SystemExit::code() { return instanceVariableAt(kCodeOffset); }

inline void SystemExit::setCode(Object* code) {
  instanceVariableAtPut(kCodeOffset, code);
}

// ImportError

inline Object* ImportError::msg() { return instanceVariableAt(kMsgOffset); }

inline void ImportError::setMsg(Object* msg) {
  instanceVariableAtPut(kMsgOffset, msg);
}

inline Object* ImportError::name() { return instanceVariableAt(kNameOffset); }

inline void ImportError::setName(Object* name) {
  instanceVariableAtPut(kNameOffset, name);
}

inline Object* ImportError::path() { return instanceVariableAt(kPathOffset); }

inline void ImportError::setPath(Object* path) {
  instanceVariableAtPut(kPathOffset, path);
}

// Type

inline Object* Type::mro() { return instanceVariableAt(kMroOffset); }

inline void Type::setMro(Object* object_array) {
  instanceVariableAtPut(kMroOffset, object_array);
}

inline Object* Type::instanceLayout() {
  return instanceVariableAt(kInstanceLayoutOffset);
}

inline void Type::setInstanceLayout(Object* layout) {
  instanceVariableAtPut(kInstanceLayoutOffset, layout);
}

inline Object* Type::name() { return instanceVariableAt(kNameOffset); }

inline void Type::setName(Object* name) {
  instanceVariableAtPut(kNameOffset, name);
}

inline Object* Type::flags() { return instanceVariableAt(kFlagsOffset); }

inline void Type::setFlags(Object* value) {
  instanceVariableAtPut(kFlagsOffset, value);
}

inline void Type::setFlag(Type::Flag bit) {
  word f = SmallInt::cast(flags())->value();
  Object* new_flag = SmallInt::fromWord(f | bit);
  instanceVariableAtPut(kFlagsOffset, new_flag);
}

inline bool Type::hasFlag(Type::Flag bit) {
  word f = SmallInt::cast(flags())->value();
  return (f & bit) != 0;
}

inline Object* Type::dict() { return instanceVariableAt(kDictOffset); }

inline void Type::setDict(Object* dict) {
  instanceVariableAtPut(kDictOffset, dict);
}

inline Object* Type::builtinBaseClass() {
  return instanceVariableAt(kBuiltinBaseClassOffset);
}

inline Object* Type::extensionType() {
  return instanceVariableAt(kExtensionTypeOffset);
}

inline void Type::setExtensionType(Object* pytype) {
  instanceVariableAtPut(kExtensionTypeOffset, pytype);
}

inline void Type::setBuiltinBaseClass(Object* base) {
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

inline Object* ObjectArray::at(word index) {
  DCHECK_INDEX(index, length());
  return instanceVariableAt(index * kPointerSize);
}

inline void ObjectArray::atPut(word index, Object* value) {
  DCHECK_INDEX(index, length());
  instanceVariableAtPut(index * kPointerSize, value);
}

inline void ObjectArray::copyTo(Object* array) {
  ObjectArray* dst = ObjectArray::cast(array);
  word len = length();
  DCHECK_BOUND(len, dst->length());
  for (word i = 0; i < len; i++) {
    Object* elem = at(i);
    dst->atPut(i, elem);
  }
}

inline void ObjectArray::replaceFromWith(word start, Object* array) {
  ObjectArray* src = ObjectArray::cast(array);
  word count = Utils::minimum(this->length() - start, src->length());
  word stop = start + count;
  for (word i = start, j = 0; i < stop; i++, j++) {
    atPut(i, src->at(j));
  }
}

inline bool ObjectArray::contains(Object* object) {
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

inline Object* Code::cellvars() { return instanceVariableAt(kCellvarsOffset); }

inline void Code::setCellvars(Object* value) {
  instanceVariableAtPut(kCellvarsOffset, value);
}

inline word Code::numCellvars() {
  Object* object = cellvars();
  DCHECK(object->isNoneType() || object->isObjectArray(),
         "not an object array");
  if (object->isNoneType()) {
    return 0;
  }
  return ObjectArray::cast(object)->length();
}

inline Object* Code::code() { return instanceVariableAt(kCodeOffset); }

inline void Code::setCode(Object* value) {
  instanceVariableAtPut(kCodeOffset, value);
}

inline Object* Code::consts() { return instanceVariableAt(kConstsOffset); }

inline void Code::setConsts(Object* value) {
  instanceVariableAtPut(kConstsOffset, value);
}

inline Object* Code::filename() { return instanceVariableAt(kFilenameOffset); }

inline void Code::setFilename(Object* value) {
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

inline Object* Code::freevars() { return instanceVariableAt(kFreevarsOffset); }

inline void Code::setFreevars(Object* value) {
  instanceVariableAtPut(kFreevarsOffset, value);
}

inline word Code::numFreevars() {
  Object* object = freevars();
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

inline Object* Code::lnotab() { return instanceVariableAt(kLnotabOffset); }

inline void Code::setLnotab(Object* value) {
  instanceVariableAtPut(kLnotabOffset, value);
}

inline Object* Code::name() { return instanceVariableAt(kNameOffset); }

inline void Code::setName(Object* value) {
  instanceVariableAtPut(kNameOffset, value);
}

inline Object* Code::names() { return instanceVariableAt(kNamesOffset); }

inline void Code::setNames(Object* value) {
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

inline Object* Code::varnames() { return instanceVariableAt(kVarnamesOffset); }

inline void Code::setVarnames(Object* value) {
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
  uword highest_digit = digitAt(numDigits() - 1);
  return static_cast<word>(highest_digit) < 0;
}

inline bool LargeInt::isPositive() {
  uword highest_digit = digitAt(numDigits() - 1);
  return static_cast<word>(highest_digit) >= 0;
}

inline uword LargeInt::digitAt(word index) {
  DCHECK_INDEX(index, numDigits());
  return reinterpret_cast<uword*>(address() + kValueOffset)[index];
}

inline void LargeInt::digitAtPut(word index, uword digit) {
  DCHECK_INDEX(index, numDigits());
  reinterpret_cast<uword*>(address() + kValueOffset)[index] = digit;
}

inline word LargeInt::numDigits() { return headerCountOrOverflow(); }

inline word LargeInt::allocationSize(word num_digits) {
  word size = headerSize(num_digits) + num_digits * kPointerSize;
  return Utils::maximum(kMinimumSize, Utils::roundUp(size, kPointerSize));
}

inline View<uword> LargeInt::digits() {
  return View<uword>(reinterpret_cast<uword*>(address() + kValueOffset),
                     headerCountOrOverflow());
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

inline Object* ListIterator::list() { return instanceVariableAt(kListOffset); }

inline void ListIterator::setList(Object* list) {
  instanceVariableAtPut(kListOffset, list);
}

inline Object* ListIterator::next() {
  word idx = index();
  auto underlying = List::cast(list());
  if (idx >= underlying->allocated()) {
    return Error::object();
  }

  Object* item = underlying->at(idx);
  setIndex(idx + 1);
  return item;
}

// Property

inline Object* Property::getter() { return instanceVariableAt(kGetterOffset); }

inline void Property::setGetter(Object* function) {
  instanceVariableAtPut(kGetterOffset, function);
}

inline Object* Property::setter() { return instanceVariableAt(kSetterOffset); }

inline void Property::setSetter(Object* function) {
  instanceVariableAtPut(kSetterOffset, function);
}

inline Object* Property::deleter() {
  return instanceVariableAt(kDeleterOffset);
}

inline void Property::setDeleter(Object* function) {
  instanceVariableAtPut(kDeleterOffset, function);
}

// RangeIterator

inline void RangeIterator::setRange(Object* range) {
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
  Range* range = Range::cast(instanceVariableAt(kRangeOffset));
  word stop = range->stop();
  word step = range->step();
  word current = SmallInt::cast(instanceVariableAt(kCurValueOffset))->value();
  if (isOutOfRange(current, stop, step)) {
    return 0;
  }
  return std::abs((stop - current) / step);
}

inline Object* RangeIterator::next() {
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

inline Object* Slice::start() { return instanceVariableAt(kStartOffset); }

inline void Slice::setStart(Object* value) {
  instanceVariableAtPut(kStartOffset, value);
}

inline Object* Slice::stop() { return instanceVariableAt(kStopOffset); }

inline void Slice::setStop(Object* value) {
  instanceVariableAtPut(kStopOffset, value);
}

inline Object* Slice::step() { return instanceVariableAt(kStepOffset); }

inline void Slice::setStep(Object* value) {
  instanceVariableAtPut(kStepOffset, value);
}

// StaticMethod

inline Object* StaticMethod::function() {
  return instanceVariableAt(kFunctionOffset);
}

inline void StaticMethod::setFunction(Object* function) {
  instanceVariableAtPut(kFunctionOffset, function);
}

// Dict

inline word Dict::numItems() {
  return SmallInt::cast(instanceVariableAt(kNumItemsOffset))->value();
}

inline void Dict::setNumItems(word num_items) {
  instanceVariableAtPut(kNumItemsOffset, SmallInt::fromWord(num_items));
}

inline Object* Dict::data() { return instanceVariableAt(kDataOffset); }

inline void Dict::setData(Object* data) {
  instanceVariableAtPut(kDataOffset, data);
}

// Function

inline Object* Function::annotations() {
  return instanceVariableAt(kAnnotationsOffset);
}

inline void Function::setAnnotations(Object* annotations) {
  return instanceVariableAtPut(kAnnotationsOffset, annotations);
}

inline Object* Function::closure() {
  return instanceVariableAt(kClosureOffset);
}

inline void Function::setClosure(Object* closure) {
  return instanceVariableAtPut(kClosureOffset, closure);
}

inline Object* Function::code() { return instanceVariableAt(kCodeOffset); }

inline void Function::setCode(Object* code) {
  return instanceVariableAtPut(kCodeOffset, code);
}

inline Object* Function::defaults() {
  return instanceVariableAt(kDefaultsOffset);
}

inline void Function::setDefaults(Object* defaults) {
  return instanceVariableAtPut(kDefaultsOffset, defaults);
}

inline bool Function::hasDefaults() { return !defaults()->isNoneType(); }

inline Object* Function::doc() { return instanceVariableAt(kDocOffset); }

inline void Function::setDoc(Object* doc) {
  instanceVariableAtPut(kDocOffset, doc);
}

inline Function::Entry Function::entry() {
  Object* object = instanceVariableAt(kEntryOffset);
  return SmallInt::cast(object)->asFunctionPointer<Function::Entry>();
}

inline void Function::setEntry(Function::Entry entry) {
  auto object = SmallInt::fromFunctionPointer(entry);
  instanceVariableAtPut(kEntryOffset, object);
}

inline Function::Entry Function::entryKw() {
  Object* object = instanceVariableAt(kEntryKwOffset);
  return SmallInt::cast(object)->asFunctionPointer<Function::Entry>();
}

inline void Function::setEntryKw(Function::Entry entry_kw) {
  auto object = SmallInt::fromFunctionPointer(entry_kw);
  instanceVariableAtPut(kEntryKwOffset, object);
}

inline Object* Function::globals() {
  return instanceVariableAt(kGlobalsOffset);
}

Function::Entry Function::entryEx() {
  Object* object = instanceVariableAt(kEntryExOffset);
  return SmallInt::cast(object)->asFunctionPointer<Function::Entry>();
}

void Function::setEntryEx(Function::Entry entry_ex) {
  auto object = SmallInt::fromFunctionPointer(entry_ex);
  instanceVariableAtPut(kEntryExOffset, object);
}

inline void Function::setGlobals(Object* globals) {
  return instanceVariableAtPut(kGlobalsOffset, globals);
}

inline Object* Function::kwDefaults() {
  return instanceVariableAt(kKwDefaultsOffset);
}

inline void Function::setKwDefaults(Object* kw_defaults) {
  return instanceVariableAtPut(kKwDefaultsOffset, kw_defaults);
}

inline Object* Function::module() { return instanceVariableAt(kModuleOffset); }

inline void Function::setModule(Object* module) {
  return instanceVariableAtPut(kModuleOffset, module);
}

inline Object* Function::name() { return instanceVariableAt(kNameOffset); }

inline void Function::setName(Object* name) {
  instanceVariableAtPut(kNameOffset, name);
}

inline Object* Function::qualname() {
  return instanceVariableAt(kQualnameOffset);
}

inline void Function::setQualname(Object* qualname) {
  instanceVariableAtPut(kQualnameOffset, qualname);
}

inline Object* Function::fastGlobals() {
  return instanceVariableAt(kFastGlobalsOffset);
}

inline void Function::setFastGlobals(Object* fast_globals) {
  return instanceVariableAtPut(kFastGlobalsOffset, fast_globals);
}

// Instance

inline word Instance::allocationSize(word num_attr) {
  DCHECK(num_attr >= 0, "invalid number of attributes %ld", num_attr);
  word size = headerSize(num_attr) + num_attr * kPointerSize;
  return Utils::maximum(kMinimumSize, Utils::roundUp(size, kPointerSize));
}

// List

inline Object* List::items() { return instanceVariableAt(kItemsOffset); }

inline void List::setItems(Object* new_items) {
  instanceVariableAtPut(kItemsOffset, new_items);
}

inline word List::capacity() { return ObjectArray::cast(items())->length(); }

inline word List::allocated() {
  return SmallInt::cast(instanceVariableAt(kAllocatedOffset))->value();
}

inline void List::setAllocated(word new_allocated) {
  instanceVariableAtPut(kAllocatedOffset, SmallInt::fromWord(new_allocated));
}

inline void List::atPut(word index, Object* value) {
  DCHECK_INDEX(index, allocated());
  Object* items = instanceVariableAt(kItemsOffset);
  ObjectArray::cast(items)->atPut(index, value);
}

inline Object* List::at(word index) {
  DCHECK_INDEX(index, allocated());
  return ObjectArray::cast(items())->at(index);
}

// Module

inline Object* Module::name() { return instanceVariableAt(kNameOffset); }

inline void Module::setName(Object* name) {
  instanceVariableAtPut(kNameOffset, name);
}

inline Object* Module::dict() { return instanceVariableAt(kDictOffset); }

inline void Module::setDict(Object* dict) {
  instanceVariableAtPut(kDictOffset, dict);
}

inline Object* Module::def() { return instanceVariableAt(kDefOffset); }

inline void Module::setDef(Object* dict) {
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
    return SmallStr::cast(this)->charAt(index);
  }
  DCHECK(isLargeStr(), "unexpected type");
  return LargeStr::cast(this)->charAt(index);
}

inline word Str::length() {
  if (isSmallStr()) {
    return SmallStr::cast(this)->length();
  }
  DCHECK(isLargeStr(), "unexpected type");
  return LargeStr::cast(this)->length();
}

inline word Str::compare(Object* string) {
  Str* that = Str::cast(string);
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

inline bool Str::equals(Object* that) {
  if (isSmallStr()) {
    return this == that;
  }
  DCHECK(isLargeStr(), "unexpected type");
  return LargeStr::cast(this)->equals(that);
}

inline void Str::copyTo(byte* dst, word length) {
  if (isSmallStr()) {
    SmallStr::cast(this)->copyTo(dst, length);
    return;
  }
  DCHECK(isLargeStr(), "unexpected type");
  return LargeStr::cast(this)->copyTo(dst, length);
}

inline char* Str::toCStr() {
  if (isSmallStr()) {
    return SmallStr::cast(this)->toCStr();
  }
  DCHECK(isLargeStr(), "unexpected type");
  return LargeStr::cast(this)->toCStr();
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

inline Object* ValueCell::value() { return instanceVariableAt(kValueOffset); }

inline void ValueCell::setValue(Object* object) {
  instanceVariableAtPut(kValueOffset, object);
}

inline bool ValueCell::isUnbound() { return this == value(); }

inline void ValueCell::makeUnbound() { setValue(this); }

// Set

inline word Set::numItems() {
  return SmallInt::cast(instanceVariableAt(kNumItemsOffset))->value();
}

inline void Set::setNumItems(word num_items) {
  instanceVariableAtPut(kNumItemsOffset, SmallInt::fromWord(num_items));
}

inline Object* Set::data() { return instanceVariableAt(kDataOffset); }

inline void Set::setData(Object* data) {
  instanceVariableAtPut(kDataOffset, data);
}

// BoundMethod

inline Object* BoundMethod::function() {
  return instanceVariableAt(kFunctionOffset);
}

inline void BoundMethod::setFunction(Object* function) {
  instanceVariableAtPut(kFunctionOffset, function);
}

inline Object* BoundMethod::self() { return instanceVariableAt(kSelfOffset); }

inline void BoundMethod::setSelf(Object* self) {
  instanceVariableAtPut(kSelfOffset, self);
}

// ClassMethod

inline Object* ClassMethod::function() {
  return instanceVariableAt(kFunctionOffset);
}

inline void ClassMethod::setFunction(Object* function) {
  instanceVariableAtPut(kFunctionOffset, function);
}

// WeakRef

inline Object* WeakRef::referent() {
  return instanceVariableAt(kReferentOffset);
}

inline void WeakRef::setReferent(Object* referent) {
  instanceVariableAtPut(kReferentOffset, referent);
}

inline Object* WeakRef::callback() {
  return instanceVariableAt(kCallbackOffset);
}

inline void WeakRef::setCallback(Object* callable) {
  instanceVariableAtPut(kCallbackOffset, callable);
}

inline Object* WeakRef::link() { return instanceVariableAt(kLinkOffset); }

inline void WeakRef::setLink(Object* reference) {
  instanceVariableAtPut(kLinkOffset, reference);
}

// Layout

inline LayoutId Layout::id() {
  return static_cast<LayoutId>(header()->hashCode());
}

inline void Layout::setId(LayoutId id) {
  setHeader(header()->withHashCode(static_cast<word>(id)));
}

inline void Layout::setDescribedClass(Object* type) {
  instanceVariableAtPut(kDescribedClassOffset, type);
}

inline Object* Layout::describedClass() {
  return instanceVariableAt(kDescribedClassOffset);
}

inline void Layout::setInObjectAttributes(Object* attributes) {
  instanceVariableAtPut(kInObjectAttributesOffset, attributes);
}

inline Object* Layout::inObjectAttributes() {
  return instanceVariableAt(kInObjectAttributesOffset);
}

inline void Layout::setOverflowAttributes(Object* attributes) {
  instanceVariableAtPut(kOverflowAttributesOffset, attributes);
}

inline word Layout::instanceSize() {
  return SmallInt::cast(instanceVariableAt(kInstanceSizeOffset))->value();
}

inline void Layout::setInstanceSize(word size) {
  instanceVariableAtPut(kInstanceSizeOffset, SmallInt::fromWord(size));
}

inline Object* Layout::overflowAttributes() {
  return instanceVariableAt(kOverflowAttributesOffset);
}

inline void Layout::setAdditions(Object* additions) {
  instanceVariableAtPut(kAdditionsOffset, additions);
}

inline Object* Layout::additions() {
  return instanceVariableAt(kAdditionsOffset);
}

inline void Layout::setDeletions(Object* deletions) {
  instanceVariableAtPut(kDeletionsOffset, deletions);
}

inline Object* Layout::deletions() {
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

inline Object* SetIterator::set() { return instanceVariableAt(kSetOffset); }

inline void SetIterator::setSet(Object* set) {
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

inline Object* SetIterator::next() {
  word idx = index();
  Set* underlying = Set::cast(set());
  ObjectArray* data = ObjectArray::cast(underlying->data());
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
  Set* set = Set::cast(instanceVariableAt(kSetOffset));
  return set->numItems() - consumedCount();
}

// Super

inline Object* Super::type() { return instanceVariableAt(kTypeOffset); }

inline void Super::setType(Object* tp) {
  DCHECK(tp->isType(), "expected type");
  instanceVariableAtPut(kTypeOffset, tp);
}

inline Object* Super::object() { return instanceVariableAt(kObjectOffset); }

inline void Super::setObject(Object* obj) {
  instanceVariableAtPut(kObjectOffset, obj);
}

inline Object* Super::objectType() {
  return instanceVariableAt(kObjectTypeOffset);
}

inline void Super::setObjectType(Object* tp) {
  DCHECK(tp->isType(), "expected type");
  instanceVariableAtPut(kObjectTypeOffset, tp);
}

// TupleIterator

inline Object* TupleIterator::tuple() {
  return instanceVariableAt(kTupleOffset);
}

inline void TupleIterator::setTuple(Object* tuple) {
  instanceVariableAtPut(kTupleOffset, tuple);
  instanceVariableAtPut(kIndexOffset, SmallInt::fromWord(0));
}

inline word TupleIterator::index() {
  return SmallInt::cast(instanceVariableAt(kIndexOffset))->value();
}

inline void TupleIterator::setIndex(word index) {
  instanceVariableAtPut(kIndexOffset, SmallInt::fromWord(index));
}

inline Object* TupleIterator::next() {
  word idx = index();
  ObjectArray* underlying = ObjectArray::cast(tuple());
  if (idx >= underlying->length()) {
    return Error::object();
  }

  Object* item = underlying->at(idx);
  setIndex(idx + 1);
  return item;
}

// GeneratorBase

inline Object* GeneratorBase::heapFrame() {
  return instanceVariableAt(kFrameOffset);
}

inline void GeneratorBase::setHeapFrame(Object* obj) {
  instanceVariableAtPut(kFrameOffset, obj);
}

}  // namespace python
