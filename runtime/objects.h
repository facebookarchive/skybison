#pragma once

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
  V(BaseException)                                                             \
  V(BoundMethod)                                                               \
  V(ByteArray)                                                                 \
  V(ClassMethod)                                                               \
  V(Code)                                                                      \
  V(Dict)                                                                      \
  V(Float)                                                                     \
  V(Ellipsis)                                                                  \
  V(Exception)                                                                 \
  V(Function)                                                                  \
  V(ImportError)                                                               \
  V(Int)                                                                       \
  V(LargeInt)                                                                  \
  V(LargeStr)                                                                  \
  V(Layout)                                                                    \
  V(List)                                                                      \
  V(ListIterator)                                                              \
  V(Module)                                                                    \
  V(ObjectArray)                                                               \
  V(Property)                                                                  \
  V(Range)                                                                     \
  V(RangeIterator)                                                             \
  V(Set)                                                                       \
  V(Slice)                                                                     \
  V(StaticMethod)                                                              \
  V(StopIteration)                                                             \
  V(Str)                                                                       \
  V(SystemExit)                                                                \
  V(Type)                                                                      \
  V(TypeError)                                                                 \
  V(ValueCell)                                                                 \
  V(WeakRef)

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
  kAttributeError,
  kBaseException,
  kBoundMethod,
  kByteArray,
  kClassMethod,
  kCode,
  kComplex,
  kCoro,
  kDict,
  kFloat,
  kEllipsis,
  kException,
  kFunction,
  kImportError,
  kInt,
  kIndexError,
  kKeyError,
  kLargeInt,
  kLargeStr,
  kLayout,
  kList,
  kListIterator,
  kLookupError,
  kModule,
  kModuleNotFoundError,
  kNameError,
  kNotImplemented,
  kNotImplementedError,
  kObjectArray,
  kProperty,
  kRange,
  kRangeIterator,
  kRuntimeError,
  kSet,
  kSetIterator,
  kSlice,
  kStaticMethod,
  kStopIteration,
  kStr,
  kSystemExit,
  kSuper,
  kTupleIterator,
  kType,
  kTypeError,
  kValueCell,
  kValueError,
  kWeakRef,

  kLastBuiltinId = kWeakRef,
};

class Object {
 public:
  // Getters and setters.
  bool isObject();
  LayoutId layoutId();

  // Immediate objects
  bool isBool();
  bool isError();
  bool isHeader();
  bool isNone();
  bool isSmallInt();
  bool isSmallStr();

  // Heap objects
  bool isBaseException();
  bool isBoundMethod();
  bool isByteArray();
  bool isType();
  bool isClassMethod();
  bool isCode();
  bool isComplex();
  bool isCoro();
  bool isDict();
  bool isFloat();
  bool isEllipsis();
  bool isException();
  bool isFunction();
  bool isHeapObject();
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

  // Casting.
  static Object* cast(Object* object);

  // Constants

  // The bottom five bits of immediate objects are used as the class id when
  // indexing into the class table in the runtime.
  static const uword kImmediateClassTableIndexMask = (1 << 5) - 1;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Object);
};

// Generic wrapper around SmallInt/LargeInt.
class Int : public Object {
 public:
  // Getters and setters.
  word asWord();
  void* asCPointer();

  word compare(Int* other);

  double floatValue();

  word highestBit();

  bool isNegative();
  bool isPositive();
  bool isZero();

  // Casting.
  static Int* cast(Object* object);

 private:
  static word compareDigits(View<uword> lhs, View<uword> rhs);
  DISALLOW_COPY_AND_ASSIGN(Int);
};

// Immediate objects

class SmallInt : public Object {
 public:
  // Getters and setters.
  word value();
  void* asCPointer();

  // Conversion.
  static SmallInt* fromWord(word value);
  static constexpr bool isValid(word value) {
    return (value >= kMinValue) && (value <= kMaxValue);
  }

  template <typename T>
  static SmallInt* fromFunctionPointer(T pointer);

  template <typename T>
  T asFunctionPointer();

  // Casting.
  static SmallInt* cast(Object* object);

  // Tags.
  static const int kTag = 0;
  static const int kTagSize = 1;
  static const uword kTagMask = (1 << kTagSize) - 1;

  // Constants
  static const word kMinValue = -(1L << (kBitsPerPointer - (kTagSize + 1)));
  static const word kMaxValue = (1L << (kBitsPerPointer - (kTagSize + 1))) - 1;

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

  // Casting.
  static Header* cast(Object* object);

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

  // Casting.
  static Bool* cast(Object* object);

  // Tags.
  static const int kTag = 7;  // 0b00111
  static const int kTagSize = 5;
  static const uword kTagMask = (1 << kTagSize) - 1;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Bool);
};

class None : public Object {
 public:
  // Singletons.
  static None* object();

  // Casting.
  static None* cast(Object* object);

  // Tags.
  static const int kTag = 15;  // 0b01111
  static const int kTagSize = 5;
  static const uword kTagMask = (1 << kTagSize) - 1;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(None);
};

// Error is a special object type, internal to the runtime. It is used to signal
// that an error has occurred inside the runtime or native code, e.g. an
// exception has been thrown.
class Error : public Object {
 public:
  // Singletons.
  static Error* object();

  // Casting.
  static Error* cast(Object* object);

  // Tagging.
  static const int kTag = 23;  // 0b10111
  static const int kTagSize = 5;
  static const uword kTagMask = (1 << kTagSize) - 1;

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

  // Casting.
  static Str* cast(Object* object);

 public:
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

  // Casting.
  static SmallStr* cast(Object* object);

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

  // Casting.
  static HeapObject* cast(Object* object);

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

  // Casting.
  static BaseException* cast(Object* object);

  // Sizing.
  static word allocationSize();

  static const int kArgsOffset = HeapObject::kSize;
  static const int kTracebackOffset = kArgsOffset + kPointerSize;
  static const int kCauseOffset = kTracebackOffset + kPointerSize;
  static const int kContextOffset = kCauseOffset + kPointerSize;
  static const int kSize = kContextOffset + kPointerSize;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(BaseException);
};

class Exception : public BaseException {
 public:
  // Casting.
  static Exception* cast(Object* object);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Exception);
};

class StopIteration : public BaseException {
 public:
  // Getters and setters.
  Object* value();
  void setValue(Object* value);

  // Casting.
  static StopIteration* cast(Object* object);

  static const int kValueOffset = BaseException::kSize;
  static const int kSize = kValueOffset + kPointerSize;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(StopIteration);
};

class SystemExit : public BaseException {
 public:
  Object* code();
  void setCode(Object* code);

  // Casting.
  static SystemExit* cast(Object* object);

  static const int kCodeOffset = BaseException::kSize;
  static const int kSize = kCodeOffset + kPointerSize;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(SystemExit);
};

class RuntimeError : public Exception {
 public:
  // Casting.
  static RuntimeError* cast(Object* object);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(RuntimeError);
};

class NotImplementedError : public RuntimeError {
 public:
  // Casting.
  static NotImplementedError* cast(Object* object);

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

  // Casting.
  static ImportError* cast(Object* object);

  static const int kMsgOffset = BaseException::kSize;
  static const int kNameOffset = kMsgOffset + kPointerSize;
  static const int kPathOffset = kNameOffset + kPointerSize;
  static const int kSize = kPathOffset + kPointerSize;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ImportError);
};

class ModuleNotFoundError : public ImportError {
 public:
  // Casting.
  static ModuleNotFoundError* cast(Object* object);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ModuleNotFoundError);
};

class LookupError : public Exception {
 public:
  // Casting.
  static LookupError* cast(Object* object);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(LookupError);
};

class IndexError : public LookupError {
 public:
  // Casting.
  static IndexError* cast(Object* object);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(IndexError);
};

class KeyError : public LookupError {
 public:
  // Casting.
  static KeyError* cast(Object* object);

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

  // Sizing.
  static word allocationSize();

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

class ByteArray : public Array {
 public:
  // Getters and setters.
  byte byteAt(word index);
  void byteAtPut(word index, byte value);

  // Casting.
  static ByteArray* cast(Object* object);

  // Sizing.
  static word allocationSize(word length);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ByteArray);
};

class ObjectArray : public Array {
 public:
  // Getters and setters.
  Object* at(word index);
  void atPut(word index, Object* value);

  // Casting.
  static ObjectArray* cast(Object* object);

  // Sizing.
  static word allocationSize(word length);

  void copyTo(Object* dst);

  void replaceFromWith(word start, Object* array);

  bool contains(Object* object);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ObjectArray);
};

class LargeStr : public Array {
 public:
  // Casting.
  static LargeStr* cast(Object* object);

  // Sizing.
  static word allocationSize(word length);

  static const int kDataOffset = HeapObject::kSize;

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

  // LargeInt is also used for storing native pointers.
  void* asCPointer();

  // Sizing.
  static word allocationSize(word num_digits);

  // Casting.
  static LargeInt* cast(Object* object);

  // Indexing into digits
  uword digitAt(word index);
  void digitAtPut(word index, uword digit);

  // Backing array of unsigned words
  View<uword> digits();

  bool isNegative();
  bool isPositive();

  word highestBit();

  // Number of digits
  word numDigits();

  // Layout.
  static const int kValueOffset = HeapObject::kSize;
  static const int kSize = kValueOffset + kPointerSize;

 private:
  friend class Int;
  friend class Runtime;

  DISALLOW_COPY_AND_ASSIGN(LargeInt);
};

class Float : public HeapObject {
 public:
  // Getters and setters.
  double value();

  // Casting.
  static Float* cast(Object* object);

  // Sizing.
  static word allocationSize();

  // Allocation.
  void initialize(double value);

  // Layout.
  static const int kValueOffset = HeapObject::kSize;
  static const int kSize = kValueOffset + kDoubleSize;

 private:
  DISALLOW_COPY_AND_ASSIGN(Float);
};

class Complex : public HeapObject {
 public:
  // Getters
  double real();
  double imag();

  // Casting.
  static Complex* cast(Object* object);

  // Sizing.
  static word allocationSize();

  // Allocation.
  void initialize(double real, double imag);

  // Layout.
  static const int kRealOffset = HeapObject::kSize;
  static const int kImagOffset = kRealOffset + kDoubleSize;
  static const int kSize = kImagOffset + kDoubleSize;

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

  // Casting
  static Property* cast(Object* object);

  // Sizing
  static word allocationSize();

  // Layout
  static const int kGetterOffset = HeapObject::kSize;
  static const int kSetterOffset = kGetterOffset + kPointerSize;
  static const int kDeleterOffset = kSetterOffset + kPointerSize;
  static const int kSize = kDeleterOffset + kPointerSize;

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

  // Casting.
  static Range* cast(Object* object);

  // Sizing.
  static word allocationSize();

  // Layout.
  static const int kStartOffset = HeapObject::kSize;
  static const int kStopOffset = kStartOffset + kPointerSize;
  static const int kStepOffset = kStopOffset + kPointerSize;
  static const int kSize = kStepOffset + kPointerSize;

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

  // Sizing.
  static word allocationSize();

  // Casting.
  static RangeIterator* cast(Object* object);

  // Layout.
  static const int kRangeOffset = HeapObject::kSize;
  static const int kCurValueOffset = kRangeOffset + kPointerSize;
  static const int kSize = kCurValueOffset + kPointerSize;

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

  // Casting.
  static Slice* cast(Object* object);

  // Sizing.
  static word allocationSize();

  // Layout.
  static const int kStartOffset = HeapObject::kSize;
  static const int kStopOffset = kStartOffset + kPointerSize;
  static const int kStepOffset = kStopOffset + kPointerSize;
  static const int kSize = kStepOffset + kPointerSize;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Slice);
};

class StaticMethod : public HeapObject {
 public:
  // Getters and setters
  Object* function();
  void setFunction(Object* function);

  // Casting
  static StaticMethod* cast(Object* object);

  // Sizing
  static word allocationSize();

  // Layout
  static const int kFunctionOffset = HeapObject::kSize;
  static const int kSize = kFunctionOffset + kPointerSize;

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

  // Sizing.
  static word allocationSize();

  // Casting.
  static ListIterator* cast(Object* object);

  // Layout.
  static const int kListOffset = HeapObject::kSize;
  static const int kIndexOffset = kListOffset + kPointerSize;
  static const int kSize = kIndexOffset + kPointerSize;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ListIterator);
};

class SetIterator : public HeapObject {
 public:
  // Getters and setters
  word index();
  void setIndex(word index);

  Object* set();
  void setSet(Object* set);

  // Iteration.
  Object* next();

  // Number of unconsumed values in the set iterator
  word pendingLength();

  // Sizing.
  static word allocationSize();

  // Cast
  static SetIterator* cast(Object* object);

  // Layout
  static const int kSetOffset = HeapObject::kSize;
  static const int kIndexOffset = kSetOffset + kPointerSize;
  static const int kConsumedCountOffset = kSetOffset + kPointerSize;
  static const int kSize = kConsumedCountOffset + kPointerSize;

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

  // Sizing.
  static word allocationSize();

  // Casting.
  static TupleIterator* cast(Object* object);

  // Iteration.
  Object* next();

  // Layout.
  static const int kTupleOffset = HeapObject::kSize;
  static const int kIndexOffset = kTupleOffset + kPointerSize;
  static const int kSize = kIndexOffset + kPointerSize;

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

  word stacksize();
  void setStacksize(word value);

  Object* varnames();
  void setVarnames(Object* value);

  // Casting.
  static Code* cast(Object* obj);

  // Sizing.
  static word allocationSize();

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

  // Casting.
  static Function* cast(Object* object);

  // Sizing.
  static word allocationSize();

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

 private:
  DISALLOW_COPY_AND_ASSIGN(Function);
};

class Instance : public HeapObject {
 public:
  // Casting.
  static Instance* cast(Object* object);

  // Sizing.
  static word allocationSize(word num_attributes);

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

  // Casting.
  static Module* cast(Object* object);

  // Allocation.
  static word allocationSize();

  // Layout.
  static const int kNameOffset = HeapObject::kSize;
  static const int kDictOffset = kNameOffset + kPointerSize;
  static const int kDefOffset = kDictOffset + kPointerSize;
  static const int kSize = kDefOffset + kPointerSize;

 private:
  DISALLOW_COPY_AND_ASSIGN(Module);
};

class NotImplemented : public HeapObject {
 public:
  // Casting.
  static NotImplemented* cast(Object* object);

  // Sizing.
  static word allocationSize();

  // Layout
  // kPaddingOffset is not used, but the GC expects the object to be
  // at least one word.
  static const int kPaddingOffset = HeapObject::kSize;
  static const int kSize = kPaddingOffset + kPointerSize;

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
 * Empty buckets are stored as (None, None, None).
 * Tombstone buckets are stored as (None, <not None>, <Any>).
 *
 */
class Dict : public HeapObject {
 public:
  class Bucket;

  // Getters and setters.
  static Dict* cast(Object* object);

  // The ObjectArray backing the dict
  Object* data();
  void setData(Object* data);

  // Number of items currently in the dict
  word numItems();
  void setNumItems(word num_items);

  // Sizing.
  static word allocationSize();

  // Layout.
  static const int kNumItemsOffset = HeapObject::kSize;
  static const int kDataOffset = kNumItemsOffset + kPointerSize;
  static const int kSize = kDataOffset + kPointerSize;

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
    return !hash(data, index)->isNone() &&
           Object::equals(key(data, index), that_key);
  }

  static Object* hash(ObjectArray* data, word index) {
    return data->at(index + kHashOffset);
  }

  static bool isEmpty(ObjectArray* data, word index) {
    return hash(data, index)->isNone() && key(data, index)->isNone();
  }

  static bool isFilled(ObjectArray* data, word index) {
    return !(isEmpty(data, index) || isTombstone(data, index));
  }

  static bool isTombstone(ObjectArray* data, word index) {
    return hash(data, index)->isNone() && !key(data, index)->isNone();
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
    set(data, index, None::object(), Error::object(), None::object());
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
  static Set* cast(Object* object);

  // The ObjectArray backing the set
  Object* data();
  void setData(Object* data);

  // Number of items currently in the set
  word numItems();
  void setNumItems(word num_items);

  // Sizing.
  static word allocationSize();

  // Layout.
  static const int kNumItemsOffset = HeapObject::kSize;
  static const int kDataOffset = kNumItemsOffset + kPointerSize;
  static const int kSize = kDataOffset + kPointerSize;

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
    return !hash(data, index)->isNone() &&
           Object::equals(key(data, index), that_key);
  }

  static bool isEmpty(ObjectArray* data, word index) {
    return hash(data, index)->isNone() && key(data, index)->isNone();
  }

  static bool isTombstone(ObjectArray* data, word index) {
    return hash(data, index)->isNone() && !key(data, index)->isNone();
  }

  static Object* key(ObjectArray* data, word index) {
    return data->at(index + kKeyOffset);
  }

  static void set(ObjectArray* data, word index, Object* hash, Object* key) {
    data->atPut(index + kHashOffset, hash);
    data->atPut(index + kKeyOffset, key);
  }

  static void setTombstone(ObjectArray* data, word index) {
    set(data, index, None::object(), Error::object());
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

  // Sizing.
  static word allocationSize();

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

  // Casting.
  static ValueCell* cast(Object* object);

  // Sizing.
  static word allocationSize();

  // Layout.
  static const int kValueOffset = HeapObject::kSize;
  static const int kSize = kValueOffset + kPointerSize;

 private:
  DISALLOW_COPY_AND_ASSIGN(ValueCell);
};

class Ellipsis : public HeapObject {
 public:
  // Casting.
  static Ellipsis* cast(Object* object);

  // Sizing.
  static word allocationSize();

  // Layout
  // kPaddingOffset is not used, but the GC expects the object to be
  // at least one word.
  static const int kPaddingOffset = HeapObject::kSize;
  static const int kSize = kPaddingOffset + kPointerSize;

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

  // Casting.
  static WeakRef* cast(Object* object);

  // Sizing.
  static word allocationSize();

  // Layout.
  static const int kReferentOffset = HeapObject::kSize;
  static const int kCallbackOffset = kReferentOffset + kPointerSize;
  static const int kLinkOffset = kCallbackOffset + kPointerSize;
  static const int kSize = kLinkOffset + kPointerSize;

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

  // Casting
  static BoundMethod* cast(Object* object);

  // Sizing
  static word allocationSize();

  // Layout
  static const int kFunctionOffset = HeapObject::kSize;
  static const int kSelfOffset = kFunctionOffset + kPointerSize;
  static const int kSize = kSelfOffset + kPointerSize;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(BoundMethod);
};

class ClassMethod : public HeapObject {
 public:
  // Getters and setters
  Object* function();
  void setFunction(Object* function);

  // Casting
  static ClassMethod* cast(Object* object);

  // Sizing
  static word allocationSize();

  // Layout
  static const int kFunctionOffset = HeapObject::kSize;
  static const int kSize = kFunctionOffset + kPointerSize;

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

  // Casting
  static Layout* cast(Object* object);

  // Sizing
  static word allocationSize();

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

  // Casting
  static Super* cast(Object* object);

  // Sizing
  static word allocationSize();

  // Layout
  static const int kTypeOffset = HeapObject::kSize;
  static const int kObjectOffset = kTypeOffset + kPointerSize;
  static const int kObjectTypeOffset = kObjectOffset + kPointerSize;
  static const int kSize = kObjectTypeOffset + kPointerSize;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Super);
};

class Coro : public HeapObject {
 public:
  // Casting.
  static Coro* cast(Object* object);

  // Sizing.
  static word allocationSize();

  // Layout
  static const int kAwaitOffset = HeapObject::kSize;
  static const int kFrameOffset = kAwaitOffset + kPointerSize;
  static const int kIsRunningOffset = kFrameOffset + kPointerSize;
  static const int kCodeOffset = kIsRunningOffset + kPointerSize;
  static const int kOriginOffset = kCodeOffset + kPointerSize;
  static const int kSize = kOriginOffset + kPointerSize;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Coro);
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

inline bool Object::isType() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() == LayoutId::kType;
}

inline bool Object::isClassMethod() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() == LayoutId::kClassMethod;
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

inline bool Object::isNone() {
  uword tag = reinterpret_cast<uword>(this) & None::kTagMask;
  return tag == None::kTag;
}

inline bool Object::isError() {
  uword tag = reinterpret_cast<uword>(this) & None::kTagMask;
  return tag == Error::kTag;
}

inline bool Object::isHeapObject() {
  uword tag = reinterpret_cast<uword>(this) & HeapObject::kTagMask;
  return tag == HeapObject::kTag;
}

inline bool Object::isLayout() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() == LayoutId::kLayout;
}

inline bool Object::isBaseException() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() ==
         LayoutId::kBaseException;
}

inline bool Object::isException() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() == LayoutId::kException;
}

inline bool Object::isBoundMethod() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() == LayoutId::kBoundMethod;
}

inline bool Object::isByteArray() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() == LayoutId::kByteArray;
}

inline bool Object::isObjectArray() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() == LayoutId::kObjectArray;
}

inline bool Object::isCode() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() == LayoutId::kCode;
}

inline bool Object::isComplex() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() == LayoutId::kComplex;
}

inline bool Object::isCoro() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() == LayoutId::kCoro;
}

inline bool Object::isLargeStr() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() == LayoutId::kLargeStr;
}

inline bool Object::isFunction() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() == LayoutId::kFunction;
}

inline bool Object::isInstance() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() >
         LayoutId::kLastBuiltinId;
}

inline bool Object::isKeyError() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() == LayoutId::kKeyError;
}

inline bool Object::isDict() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() == LayoutId::kDict;
}

inline bool Object::isFloat() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() == LayoutId::kFloat;
}

inline bool Object::isSet() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() == LayoutId::kSet;
}

inline bool Object::isSetIterator() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() == LayoutId::kSetIterator;
}

inline bool Object::isSuper() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() == LayoutId::kSuper;
}

inline bool Object::isModule() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() == LayoutId::kModule;
}

inline bool Object::isList() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() == LayoutId::kList;
}

inline bool Object::isListIterator() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() ==
         LayoutId::kListIterator;
}

inline bool Object::isLookupError() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() == LayoutId::kLookupError;
}

inline bool Object::isValueCell() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() == LayoutId::kValueCell;
}

inline bool Object::isEllipsis() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() == LayoutId::kEllipsis;
}

inline bool Object::isLargeInt() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() == LayoutId::kLargeInt;
}

inline bool Object::isInt() { return isSmallInt() || isLargeInt(); }

inline bool Object::isNotImplemented() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() ==
         LayoutId::kNotImplemented;
}

inline bool Object::isNotImplementedError() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() ==
         LayoutId::kNotImplementedError;
}

inline bool Object::isProperty() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() == LayoutId::kProperty;
}

inline bool Object::isRange() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() == LayoutId::kRange;
}

inline bool Object::isRangeIterator() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() ==
         LayoutId::kRangeIterator;
}

inline bool Object::isRuntimeError() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() ==
         LayoutId::kRuntimeError;
}

inline bool Object::isSlice() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() == LayoutId::kSlice;
}

inline bool Object::isStaticMethod() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() ==
         LayoutId::kStaticMethod;
}

inline bool Object::isStopIteration() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() ==
         LayoutId::kStopIteration;
}

inline bool Object::isStr() { return isSmallStr() || isLargeStr(); }

inline bool Object::isSystemExit() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() == LayoutId::kSystemExit;
}

inline bool Object::isTupleIterator() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() ==
         LayoutId::kTupleIterator;
}

inline bool Object::isImportError() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() == LayoutId::kImportError;
}

inline bool Object::isIndexError() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() == LayoutId::kIndexError;
}

inline bool Object::isWeakRef() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() == LayoutId::kWeakRef;
}

inline bool Object::isModuleNotFoundError() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() ==
         LayoutId::kModuleNotFoundError;
}

inline bool Object::equals(Object* lhs, Object* rhs) {
  return (lhs == rhs) ||
         (lhs->isLargeStr() && LargeStr::cast(lhs)->equals(rhs));
}

inline Object* Object::cast(Object* object) { return object; }

// Int

inline Int* Int::cast(Object* object) {
  DCHECK(object->isInt(), "invalid cast");
  return reinterpret_cast<Int*>(object);
}

inline word Int::asWord() {
  if (isSmallInt()) {
    return SmallInt::cast(this)->value();
  }
  return LargeInt::cast(this)->asWord();
}

inline void* Int::asCPointer() {
  if (isSmallInt()) {
    return SmallInt::cast(this)->asCPointer();
  }
  return LargeInt::cast(this)->asCPointer();
}

inline word Int::compare(Int* that) {
  if (this->isSmallInt() && that->isSmallInt()) {
    return this->asWord() - that->asWord();
  }
  if (this->isNegative() != that->isNegative()) {
    return this->isNegative() ? -1 : 1;
  }

  uword digit;
  View<uword> lhs(&digit, 1);
  if (this->isSmallInt()) {
    digit = this->asWord();
  } else {
    lhs = LargeInt::cast(this)->digits();
  }

  View<uword> rhs(&digit, 1);
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

inline word Int::highestBit() {
  if (isSmallInt()) {
    uword self = static_cast<uword>(std::abs(SmallInt::cast(this)->value()));
    return Utils::highestBit(self);
  }
  return LargeInt::cast(this)->highestBit();
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

inline SmallInt* SmallInt::cast(Object* object) {
  DCHECK(object->isSmallInt(), "invalid cast");
  return reinterpret_cast<SmallInt*>(object);
}

inline word SmallInt::value() {
  return reinterpret_cast<word>(this) >> kTagSize;
}

inline void* SmallInt::asCPointer() { return reinterpret_cast<void*>(value()); }

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

inline SmallStr* SmallStr::cast(Object* object) {
  DCHECK(object->isSmallStr(), "invalid cast");
  return reinterpret_cast<SmallStr*>(object);
}

// Header

inline Header* Header::cast(Object* object) {
  DCHECK(object->isHeader(), "invalid cast");
  return reinterpret_cast<Header*>(object);
}

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

inline None* None::object() { return reinterpret_cast<None*>(kTag); }

inline None* None::cast(Object* object) {
  DCHECK(object->isNone(), "invalid cast, expected None");
  return reinterpret_cast<None*>(object);
}

// Error

inline Error* Error::object() { return reinterpret_cast<Error*>(kTag); }

inline Error* Error::cast(Object* object) {
  DCHECK(object->isError(), "invalid cast, expected Error");
  return reinterpret_cast<Error*>(object);
}

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

inline Bool* Bool::cast(Object* object) {
  DCHECK(object->isBool(), "invalid cast, expected Bool");
  return reinterpret_cast<Bool*>(object);
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

inline HeapObject* HeapObject::cast(Object* object) {
  DCHECK(object->isHeapObject(), "invalid cast, expected heap object");
  return reinterpret_cast<HeapObject*>(object);
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

inline BaseException* BaseException::cast(Object* object) {
  DCHECK(object->isBaseException(), "invalid cast, expected BaseException");
  return reinterpret_cast<BaseException*>(object);
}

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

// Exception

inline Exception* Exception::cast(Object* object) {
  DCHECK(object->isException(), "invalid cast, expected Exception");
  return reinterpret_cast<Exception*>(object);
}

// StopIteration

inline StopIteration* StopIteration::cast(Object* object) {
  DCHECK(object->isStopIteration(), "invalid cast, expected StopIteration");
  return reinterpret_cast<StopIteration*>(object);
}

inline Object* StopIteration::value() {
  return instanceVariableAt(kValueOffset);
}

inline void StopIteration::setValue(Object* value) {
  instanceVariableAtPut(kValueOffset, value);
}

// SystemExit

inline SystemExit* SystemExit::cast(Object* object) {
  DCHECK(object->isSystemExit(), "invalid cast, expected SystemExit");
  return reinterpret_cast<SystemExit*>(object);
}

inline Object* SystemExit::code() { return instanceVariableAt(kCodeOffset); }

inline void SystemExit::setCode(Object* code) {
  instanceVariableAtPut(kCodeOffset, code);
}

// RuntimeError

inline RuntimeError* RuntimeError::cast(Object* object) {
  DCHECK(object->isRuntimeError(), "invalid cast, expected RuntimeError");
  return reinterpret_cast<RuntimeError*>(object);
}

// NotImplementedError

inline NotImplementedError* NotImplementedError::cast(Object* object) {
  DCHECK(object->isNotImplementedError(),
         "invalid cast, expected NotImplementedError");
  return reinterpret_cast<NotImplementedError*>(object);
}

// ImportError

inline ImportError* ImportError::cast(Object* object) {
  DCHECK(object->isImportError(), "invalid cast, expected ImportError");
  return reinterpret_cast<ImportError*>(object);
}

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

// ModuleNotFoundError

inline ModuleNotFoundError* ModuleNotFoundError::cast(Object* object) {
  DCHECK(object->isModuleNotFoundError(),
         "invalid cast, expected ModuleNotFoundError");
  return reinterpret_cast<ModuleNotFoundError*>(object);
}

// LookupError

inline LookupError* LookupError::cast(Object* object) {
  DCHECK(object->isLookupError(), "invalid cast, expected LookupError");
  return reinterpret_cast<LookupError*>(object);
}

// IndexError

inline IndexError* IndexError::cast(Object* object) {
  DCHECK(object->isIndexError(), "invalid cast, expected IndexError");
  return reinterpret_cast<IndexError*>(object);
}

// KeyError

inline KeyError* KeyError::cast(Object* object) {
  DCHECK(object->isKeyError(), "invalid cast, expected KeyError");
  return reinterpret_cast<KeyError*>(object);
}

// Type

inline word Type::allocationSize() { return Header::kSize + Type::kSize; }

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
  DCHECK(isByteArray() || isObjectArray() || isLargeStr(),
         "invalid array type");
  return headerCountOrOverflow();
}

// ByteArray

inline word ByteArray::allocationSize(word length) {
  DCHECK(length >= 0, "invalid length %ld", length);
  word size = headerSize(length) + length;
  return Utils::maximum(kMinimumSize, Utils::roundUp(size, kPointerSize));
}

inline byte ByteArray::byteAt(word index) {
  DCHECK_INDEX(index, length());
  return *reinterpret_cast<byte*>(address() + index);
}

inline void ByteArray::byteAtPut(word index, byte value) {
  DCHECK_INDEX(index, length());
  *reinterpret_cast<byte*>(address() + index) = value;
}

inline ByteArray* ByteArray::cast(Object* object) {
  DCHECK(object->isByteArray(), "invalid cast, expected byte array");
  return reinterpret_cast<ByteArray*>(object);
}

// ObjectArray

inline word ObjectArray::allocationSize(word length) {
  DCHECK(length >= 0, "invalid length %ld", length);
  word size = headerSize(length) + length * kPointerSize;
  return Utils::maximum(kMinimumSize, Utils::roundUp(size, kPointerSize));
}

inline ObjectArray* ObjectArray::cast(Object* object) {
  DCHECK(object->isObjectArray(), "invalid cast, expected object array");
  return reinterpret_cast<ObjectArray*>(object);
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

inline Code* Code::cast(Object* obj) {
  DCHECK(obj->isCode(), "invalid cast, expecting code object");
  return reinterpret_cast<Code*>(obj);
}

inline word Code::allocationSize() { return Header::kSize + Code::kSize; }

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
  DCHECK(object->isNone() || object->isObjectArray(), "not an object array");
  if (object->isNone()) {
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
  DCHECK(object->isNone() || object->isObjectArray(), "not an object array");
  if (object->isNone()) {
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

inline void* LargeInt::asCPointer() {
  DCHECK(numDigits() == 1, "Large integer cannot fit in a pointer");
  DCHECK(isPositive(), "Cannot cast a negative value to a C pointer");
  return reinterpret_cast<void*>(asWord());
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

inline word LargeInt::highestBit() {
  word num_digits = numDigits();
  word highest_bit = Utils::highestBit(digitAt(num_digits - 1));
  return highest_bit + ((num_digits - 1) * kBitsPerPointer);
}

inline word LargeInt::numDigits() { return headerCountOrOverflow(); }

inline word LargeInt::allocationSize(word num_digits) {
  word size = headerSize(num_digits) + num_digits * kPointerSize;
  return Utils::maximum(kMinimumSize, Utils::roundUp(size, kPointerSize));
}

inline LargeInt* LargeInt::cast(Object* object) {
  DCHECK(object->isLargeInt(), "not a large integer");
  return reinterpret_cast<LargeInt*>(object);
}

inline View<uword> LargeInt::digits() {
  return View<uword>(reinterpret_cast<uword*>(address() + kValueOffset),
                     headerCountOrOverflow());
}

// Float

inline double Float::value() {
  return *reinterpret_cast<double*>(address() + kValueOffset);
}

inline word Float::allocationSize() { return Header::kSize + Float::kSize; }

inline Float* Float::cast(Object* object) {
  DCHECK(object->isFloat(), "not a float");
  return reinterpret_cast<Float*>(object);
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

inline word Complex::allocationSize() { return Header::kSize + Complex::kSize; }

inline Complex* Complex::cast(Object* object) {
  DCHECK(object->isComplex(), "not a complex");
  return reinterpret_cast<Complex*>(object);
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

inline word Range::allocationSize() { return Header::kSize + Range::kSize; }

inline Range* Range::cast(Object* object) {
  DCHECK(object->isRange(), "invalid cast, expected range");
  return reinterpret_cast<Range*>(object);
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

inline word ListIterator::allocationSize() {
  return Header::kSize + ListIterator::kSize;
}

inline ListIterator* ListIterator::cast(Object* object) {
  DCHECK(object->isListIterator(), "invalid cast, expected list iterator");
  return reinterpret_cast<ListIterator*>(object);
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

inline Property* Property::cast(Object* object) {
  DCHECK(object->isProperty(), "invalid cast");
  return reinterpret_cast<Property*>(object);
}

inline word Property::allocationSize() {
  return Header::kSize + Property::kSize;
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

inline word RangeIterator::allocationSize() {
  return Header::kSize + RangeIterator::kSize;
}

inline RangeIterator* RangeIterator::cast(Object* object) {
  DCHECK(object->isRangeIterator(), "invalid cast, expected range interator");
  return reinterpret_cast<RangeIterator*>(object);
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

inline word Slice::allocationSize() { return Header::kSize + Slice::kSize; }

inline Slice* Slice::cast(Object* object) {
  DCHECK(object->isSlice(), "invalid cast, expected slice");
  return reinterpret_cast<Slice*>(object);
}

// StaticMethod

inline Object* StaticMethod::function() {
  return instanceVariableAt(kFunctionOffset);
}

inline void StaticMethod::setFunction(Object* function) {
  instanceVariableAtPut(kFunctionOffset, function);
}

inline StaticMethod* StaticMethod::cast(Object* object) {
  DCHECK(object->isStaticMethod(), "invalid cast");
  return reinterpret_cast<StaticMethod*>(object);
}

inline word StaticMethod::allocationSize() {
  return Header::kSize + StaticMethod::kSize;
}

// Dict

inline word Dict::allocationSize() { return Header::kSize + Dict::kSize; }

inline Dict* Dict::cast(Object* object) {
  DCHECK(object->isDict(), "invalid cast, expected dict");
  return reinterpret_cast<Dict*>(object);
}

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

inline bool Function::hasDefaults() { return !defaults()->isNone(); }

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

inline Function* Function::cast(Object* object) {
  DCHECK(object->isFunction(), "invalid cast, expected function");
  return reinterpret_cast<Function*>(object);
}

inline word Function::allocationSize() {
  return Header::kSize + Function::kSize;
}

// Instance

inline word Instance::allocationSize(word num_attr) {
  DCHECK(num_attr >= 0, "invalid number of attributes %ld", num_attr);
  word size = headerSize(num_attr) + num_attr * kPointerSize;
  return Utils::maximum(kMinimumSize, Utils::roundUp(size, kPointerSize));
}

inline Instance* Instance::cast(Object* object) {
  DCHECK(object->isInstance(), "invalid cast, expected instance");
  return reinterpret_cast<Instance*>(object);
}

// List

inline word List::allocationSize() { return Header::kSize + List::kSize; }

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

inline word Module::allocationSize() { return Header::kSize + Module::kSize; }

inline Module* Module::cast(Object* object) {
  DCHECK(object->isModule(), "invalid cast, expected module");
  return reinterpret_cast<Module*>(object);
}

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

// NotImplemented

inline word NotImplemented::allocationSize() {
  return Header::kSize + NotImplemented::kSize;
}

inline NotImplemented* NotImplemented::cast(Object* object) {
  DCHECK(object->isNotImplemented(), "invalid cast, expected NotImplemented");
  return reinterpret_cast<NotImplemented*>(object);
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

inline Str* Str::cast(Object* object) {
  DCHECK(object->isLargeStr() || object->isSmallStr(),
         "invalid cast, expected string");
  return reinterpret_cast<Str*>(object);
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

inline LargeStr* LargeStr::cast(Object* object) {
  DCHECK(object->isLargeStr(), "unexpected type");
  return reinterpret_cast<LargeStr*>(object);
}

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

inline ValueCell* ValueCell::cast(Object* object) {
  DCHECK(object->isValueCell(), "invalid cast");
  return reinterpret_cast<ValueCell*>(object);
}

inline word ValueCell::allocationSize() {
  return Header::kSize + ValueCell::kSize;
}

// Ellipsis

inline word Ellipsis::allocationSize() {
  return Header::kSize + Ellipsis::kSize;
}

inline Ellipsis* Ellipsis::cast(Object* object) {
  DCHECK(object->isEllipsis(), "invalid cast");
  return reinterpret_cast<Ellipsis*>(object);
}

// Set

inline word Set::allocationSize() { return Header::kSize + Set::kSize; }

inline Set* Set::cast(Object* object) {
  DCHECK(object->isSet(), "invalid cast");
  return reinterpret_cast<Set*>(object);
}

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

inline word BoundMethod::allocationSize() {
  return Header::kSize + BoundMethod::kSize;
}

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

inline BoundMethod* BoundMethod::cast(Object* object) {
  DCHECK(object->isBoundMethod(), "invalid cast");
  return reinterpret_cast<BoundMethod*>(object);
}

// ClassMethod

inline Object* ClassMethod::function() {
  return instanceVariableAt(kFunctionOffset);
}

inline void ClassMethod::setFunction(Object* function) {
  instanceVariableAtPut(kFunctionOffset, function);
}

inline ClassMethod* ClassMethod::cast(Object* object) {
  DCHECK(object->isClassMethod(), "invalid cast");
  return reinterpret_cast<ClassMethod*>(object);
}

inline word ClassMethod::allocationSize() {
  return Header::kSize + ClassMethod::kSize;
}

// WeakRef

inline WeakRef* WeakRef::cast(Object* object) {
  DCHECK(object->isWeakRef(), "invalid cast");
  return reinterpret_cast<WeakRef*>(object);
}

inline word WeakRef::allocationSize() { return Header::kSize + WeakRef::kSize; }

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

inline word Layout::allocationSize() { return Header::kSize + Layout::kSize; }

inline Layout* Layout::cast(Object* object) {
  DCHECK(object->isLayout(), "invalid cast");
  return reinterpret_cast<Layout*>(object);
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

inline SetIterator* SetIterator::cast(Object* object) {
  DCHECK(object->isSetIterator(), "invalid cast, expected set_iterator");
  return reinterpret_cast<SetIterator*>(object);
}

inline word SetIterator::allocationSize() {
  return Header::kSize + SetIterator::kSize;
}

inline Object* SetIterator::set() { return instanceVariableAt(kSetOffset); }

inline void SetIterator::setSet(Object* set) {
  instanceVariableAtPut(kSetOffset, set);
  instanceVariableAtPut(kIndexOffset, SmallInt::fromWord(0));
  instanceVariableAtPut(kConsumedCountOffset, SmallInt::fromWord(0));
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
  if (idx >= data->length()) {
    return Error::object();
  }
  // Find the next non empty bucket
  while (idx < data->length() && (Set::Bucket::isTombstone(data, idx) ||
                                  Set::Bucket::isEmpty(data, idx))) {
    idx++;
  }
  setIndex(idx);
  if (idx < data->length()) {
    word consumed =
        SmallInt::cast(instanceVariableAt(kConsumedCountOffset))->value();
    instanceVariableAtPut(kConsumedCountOffset,
                          SmallInt::fromWord(consumed + 1));
    return Set::Bucket::key(data, idx);
  } else {
    return Error::object();
  }
}

inline word SetIterator::pendingLength() {
  Set* set = Set::cast(instanceVariableAt(kSetOffset));
  return set->numItems() -
         SmallInt::cast(instanceVariableAt(kConsumedCountOffset))->value();
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

inline Super* Super::cast(Object* object) {
  DCHECK(object->isSuper(), "invalid cast, expected super");
  return reinterpret_cast<Super*>(object);
}

inline word Super::allocationSize() { return Header::kSize + Super::kSize; }

// TupleIterator

inline TupleIterator* TupleIterator::cast(Object* object) {
  DCHECK(object->isTupleIterator(), "invalid cast, expected tuple_iterator");
  return reinterpret_cast<TupleIterator*>(object);
}

inline word TupleIterator::allocationSize() {
  return Header::kSize + TupleIterator::kSize;
}

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

// Coro

inline word Coro::allocationSize() { return Header::kSize + Coro::kSize; }

inline Coro* Coro::cast(Object* object) {
  DCHECK(object->isCoro(), "invalid cast, expected Coro");
  return reinterpret_cast<Coro*>(object);
}

}  // namespace python
