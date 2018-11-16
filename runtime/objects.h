#pragma once

#include "globals.h"
#include "utils.h"
#include "view.h"

namespace python {

/*
class Object;
  class Boolean;

  class Fixnum;
  class HeapObject;
    class Code;
    class Module;
    class ObjectArray;
    class ByteArray;
    class String;
    // TODO: method objects, etc.
*/
// 0bXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXx0 number {31,63}-bit signed numbers
// 0bXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX01 object: mov dst, [src-1]
// 0bXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX11 immediate small strings, true and false,
// header word mark

// 0b0011 header
// 0b0111 boolean
// 0b1011 none
// 0b1111 string

// small string format
// 7 bytes of char
// 1 byte of length + tag
// LLL TTTT

// xxxxxx00 even small integer
// xxxxxx10  odd small integer
// xxxxxx01 object
// xxxxxx11 other immediate

// xxxxx011 header
// xxxx0111 small string
// xxxx1111

// [ header ]
// [ length ]
// [ elem 0 ]
// [   ...  ]
// [ elem N ]

// [ header ]
// [ attrs  ]
// [ elem 0 ]
// [   ...  ]
// [ elem N ]

// clang-format off
#define INTRINSIC_IMMEDIATE_CLASS_NAMES(V) \
  V(SmallInteger)                        \
  V(SmallString)                         \
  V(Boolean)                             \
  V(None)

#define INTRINSIC_HEAP_CLASS_NAMES(V) \
  V(Object)                         \
  V(ByteArray)                      \
  V(Code)                           \
  V(ClassMethod)                    \
  V(Dictionary)                     \
  V(Double)                         \
  V(Ellipsis)                       \
  V(Function)                       \
  V(LargeInteger)                   \
  V(LargeString)                    \
  V(Layout)                         \
  V(List)                           \
  V(ListIterator)                   \
  V(Module)                         \
  V(ObjectArray)                    \
  V(Range)                          \
  V(RangeIterator)                  \
  V(Slice)                          \
  V(Type)                           \
  V(ValueCell)                      \
  V(WeakRef)

#define INTRINSIC_CLASS_NAMES(V)     \
  INTRINSIC_IMMEDIATE_CLASS_NAMES(V) \
  INTRINSIC_HEAP_CLASS_NAMES(V)
// clang-format on

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
enum IntrinsicLayoutId {
  // Immediate objects - note that the SmallInteger class is also aliased to
  // all even integers less than 32, so that classes of immediate objects can
  // be looked up simply by using the low 5 bits of the immediate value. This
  // implies that all other immediate class ids must be odd.
  kSmallInteger = 0,
  kBoolean = 7,
  kNone = 15,
  // there is no class associated with the Error object type, this is here as a
  // placeholder.
  kError = 23,
  kSmallString = 31,

  // Heap objects
  kObject = 32,
  kBoundMethod,
  kByteArray,
  kClassMethod,
  kCode,
  kComplex,
  kDictionary,
  kDouble,
  kEllipsis,
  kFunction,
  kLargeInteger,
  kLargeString,
  kLayout,
  kList,
  kListIterator,
  kModule,
  kNotImplemented,
  kObjectArray,
  kRange,
  kRangeIterator,
  kSet,
  kSlice,
  kSuper,
  kType,
  kValueCell,
  kWeakRef,

  kLastId = kWeakRef,
};

class Object {
 public:
  // Getters and setters.
  inline bool isObject();
  inline word layoutId();

  // Immediate objects
  inline bool isSmallInteger();
  inline bool isSmallString();
  inline bool isHeader();
  inline bool isNone();
  inline bool isError();
  inline bool isBoolean();

  // Heap objects
  inline bool isHeapObject();
  inline bool isBoundMethod();
  inline bool isByteArray();
  inline bool isClass();
  inline bool isClassMethod();
  inline bool isCode();
  inline bool isComplex();
  inline bool isDictionary();
  inline bool isDouble();
  inline bool isEllipsis();
  inline bool isFunction();
  inline bool isInstance();
  inline bool isInteger();
  inline bool isLargeInteger();
  inline bool isLargeString();
  inline bool isLayout();
  inline bool isList();
  inline bool isListIterator();
  inline bool isModule();
  inline bool isNotImplemented();
  inline bool isObjectArray();
  inline bool isRange();
  inline bool isRangeIterator();
  inline bool isSet();
  inline bool isSlice();
  inline bool isSuper();
  inline bool isValueCell();
  inline bool isWeakRef();

  // superclass objects
  inline bool isString();

  static inline bool equals(Object* lhs, Object* rhs);

  // Casting.
  static inline Object* cast(Object* object);

  // Constants

  // The bottom five bits of immediate objects are used as the class id when
  // indexing into the class table in the runtime.
  static const uword kImmediateClassTableIndexMask = (1 << 5) - 1;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Object);
};

// Generic wrapper around SmallInteger/LargeInteger.
class Integer : public Object {
 public:
  // Getters and setters.
  inline word asWord();
  inline void* asCPointer();

  // Casting.
  static inline Integer* cast(Object* object);

 private:
  DISALLOW_COPY_AND_ASSIGN(Integer);
};

// Immediate objects

class SmallInteger : public Object {
 public:
  // Getters and setters.
  inline word value();
  inline void* asCPointer();

  // Conversion.
  static inline SmallInteger* fromWord(word value);
  static inline constexpr bool isValid(word value) {
    return (value >= kMinValue) && (value <= kMaxValue);
  }

  template <typename T>
  static inline SmallInteger* fromFunctionPointer(T pointer);

  template <typename T>
  inline T asFunctionPointer();

  // Casting.
  static inline SmallInteger* cast(Object* object);

  // Tags.
  static const int kTag = 0;
  static const int kTagSize = 1;
  static const uword kTagMask = (1 << kTagSize) - 1;

  // Constants
  static const word kMinValue = -(1L << (kBitsPerPointer - (kTagSize + 1)));
  static const word kMaxValue = (1L << (kBitsPerPointer - (kTagSize + 1))) - 1;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(SmallInteger);
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
  inline word hashCode();
  inline Header* withHashCode(word value);

  inline ObjectFormat format();

  inline word layoutId();
  inline Header* withLayoutId(word layout_id);

  inline word count();

  inline bool hasOverflow();

  static inline Header*
  from(word count, word hash, word layout_id, ObjectFormat format);

  // Casting.
  static inline Header* cast(Object* object);

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

class Boolean : public Object {
 public:
  // Getters and setters.
  inline bool value();

  // Conversion.
  static inline Boolean* fromBool(bool value);

  // Casting.
  static inline Boolean* cast(Object* object);

  // Tags.
  static const int kTag = 7; // 0b00111
  static const int kTagSize = 5;
  static const uword kTagMask = (1 << kTagSize) - 1;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Boolean);
};

class None : public Object {
 public:
  // Singletons.
  inline static None* object();

  // Casting.
  static inline None* cast(Object* object);

  // Tags.
  static const int kTag = 15; // 0b01111
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
  inline static Error* object();

  // Casting.
  static inline Error* cast(Object* object);

  // Tagging.
  static const int kTag = 23; // 0b10111
  static const int kTagSize = 5;
  static const uword kTagMask = (1 << kTagSize) - 1;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Error);
};

// Super class of common string functionality
class String : public Object {
 public:
  // Getters and setters.
  inline byte charAt(word index);
  inline word length();
  inline void copyTo(byte* dst, word length);

  // Equality checks.
  inline word compare(Object* string);
  inline bool equals(Object* that);
  inline bool equalsCString(const char* c_string);

  // Conversion to an unescaped C string.  The underlying memory is allocated
  // with malloc and must be freed by the caller.
  inline char* toCString();

  // Casting.
  static inline String* cast(Object* object);

 public:
  DISALLOW_IMPLICIT_CONSTRUCTORS(String);
};

class SmallString : public Object {
 public:
  // Conversion.
  static Object* fromCString(const char* value);
  static Object* fromBytes(View<byte> data);

  // Tagging.
  static const int kTag = 31; // 0b11111
  static const int kTagSize = 5;
  static const uword kTagMask = (1 << kTagSize) - 1;

  static const word kMaxLength = kWordSize - 1;

 private:
  // Interface methods are private: strings should be manipulated via the
  // String class, which delegates to LargeString/SmallString appropriately.

  // Getters and setters.
  inline word length();
  inline byte charAt(word index);
  inline void copyTo(byte* dst, word length);

  // Conversion to an unescaped C string.  The underlying memory is allocated
  // with malloc and must be freed by the caller.
  char* toCString();

  // Casting.
  static inline SmallString* cast(Object* object);

  friend class Heap;
  friend class Object;
  friend class Runtime;
  friend class String;

  DISALLOW_IMPLICIT_CONSTRUCTORS(SmallString);
};

// Heap objects

class HeapObject : public Object {
 public:
  // Getters and setters.
  inline uword address();
  inline uword baseAddress();
  inline Header* header();
  inline void setHeader(Header* header);
  inline word headerOverflow();
  inline void
  setHeaderAndOverflow(word count, word hash, word id, ObjectFormat format);
  inline word headerCountOrOverflow();
  inline word size();

  // Conversion.
  inline static HeapObject* fromAddress(uword address);

  // Casting.
  inline static HeapObject* cast(Object* object);

  // Sizing
  inline static word headerSize(word count);

  inline void initialize(word size, Object* value);

  // Garbage collection.
  inline bool isRoot();
  inline bool isForwarding();
  inline Object* forward();
  inline void forwardTo(Object* object);

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
  inline Object* instanceVariableAt(word offset);
  inline void instanceVariableAtPut(word offset, Object* value);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(HeapObject);

  friend class Runtime;
};

template <typename T>
class Handle;

class Thread;

class Class : public HeapObject {
 public:
  enum Flag {
    kListSubclass = 1,
    kDictSubclass = 2,
  };

  // Getters and setters.
  inline Object* instanceLayout();
  inline void setInstanceLayout(Object* layout);

  inline Object* mro();
  inline void setMro(Object* object_array);

  inline Object* name();
  inline void setName(Object* name);

  inline Object* flags();
  inline void setFlags(Object* value);
  inline void setFlag(Flag flag);
  inline bool hasFlag(Flag flag);

  inline Object* dictionary();
  inline void setDictionary(Object* name);

  // builtin base related
  inline Object* builtinBaseClass();
  inline void setBuiltinBaseClass(Object* base);

  inline bool isIntrinsicOrExtension();

  // Casting.
  inline static Class* cast(Object* object);

  // Sizing.
  inline static word allocationSize();

  // Layout.
  static const int kMroOffset = HeapObject::kSize;
  static const int kInstanceLayoutOffset = kMroOffset + kPointerSize;
  static const int kNameOffset = kInstanceLayoutOffset + kPointerSize;
  static const int kFlagsOffset = kNameOffset + kPointerSize;
  static const int kDictionaryOffset = kFlagsOffset + kPointerSize;
  static const int kBuiltinBaseClassOffset = kDictionaryOffset + kPointerSize;
  static const int kSize = kBuiltinBaseClassOffset + kPointerSize;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Class);
};

class Array : public HeapObject {
 public:
  inline word length();

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Array);
};

class ByteArray : public Array {
 public:
  // Getters and setters.
  inline byte byteAt(word index);
  inline void byteAtPut(word index, byte value);

  // Casting.
  inline static ByteArray* cast(Object* object);

  // Sizing.
  inline static word allocationSize(word length);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ByteArray);
};

class ObjectArray : public Array {
 public:
  // Getters and setters.
  inline Object* at(word index);
  inline void atPut(word index, Object* value);

  // Casting.
  inline static ObjectArray* cast(Object* object);

  // Sizing.
  inline static word allocationSize(word length);

  inline void copyTo(Object* dst);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ObjectArray);
};

class LargeString : public Array {
 public:
  // Sizing.
  inline static word allocationSize(word length);

 private:
  // Interface methods are private: strings should be manipulated via the
  // String class, which delegates to LargeString/SmallString appropriately.

  // Getters and setters.
  inline byte charAt(word index);
  void copyTo(byte* bytes, word length);

  // Equality checks.
  bool equals(Object* that);

  // Conversion to an unescaped C string.  The underlying memory is allocated
  // with malloc and must be freed by the caller.
  char* toCString();

  // Casting.
  static inline LargeString* cast(Object* object);

  friend class Heap;
  friend class Object;
  friend class Runtime;
  friend class String;

  DISALLOW_IMPLICIT_CONSTRUCTORS(LargeString);
};

// Currently a 64 bit signed integer - will eventually be extended to
// support arbitrary precision.
// TODO: arbitrary precision support.
class LargeInteger : public HeapObject {
 public:
  // Getters and setters.
  // TODO: when arbitrary precision is supported, how should these methods
  // behave?
  inline word asWord();

  // LargeInteger is also used for storing native pointers.
  inline void* asCPointer();

  // Casting.
  static inline LargeInteger* cast(Object* object);

  // Sizing.
  static inline word allocationSize();

  // Allocation.
  inline void initialize(word value);

  // Layout.
  static const int kValueOffset = HeapObject::kSize;
  static const int kSize = kValueOffset + kPointerSize;

 private:
  DISALLOW_COPY_AND_ASSIGN(LargeInteger);
};

class Double : public HeapObject {
 public:
  // Getters and setters.
  inline double value();

  // Casting.
  static inline Double* cast(Object* object);

  // Sizing.
  static inline word allocationSize();

  // Allocation.
  inline void initialize(double value);

  // Layout.
  static const int kValueOffset = HeapObject::kSize;
  static const int kSize = kValueOffset + kDoubleSize;

 private:
  DISALLOW_COPY_AND_ASSIGN(Double);
};

class Complex : public HeapObject {
 public:
  // Getters
  inline double real();
  inline double imag();

  // Casting.
  static inline Complex* cast(Object* object);

  // Sizing.
  static inline word allocationSize();

  // Allocation.
  inline void initialize(double real, double imag);

  // Layout.
  static const int kRealOffset = HeapObject::kSize;
  static const int kImagOffset = kRealOffset + kDoubleSize;
  static const int kSize = kImagOffset + kDoubleSize;

 private:
  DISALLOW_COPY_AND_ASSIGN(Complex);
};

class Range : public HeapObject {
 public:
  // Getters and setters.
  inline word start();
  inline void setStart(word value);

  inline word stop();
  inline void setStop(word value);

  inline word step();
  inline void setStep(word value);

  // Casting.
  static inline Range* cast(Object* object);

  // Sizing.
  static inline word allocationSize();

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
  inline void setRange(Object* range);

  // Iteration.
  inline Object* next();

  // Sizing.
  static inline word allocationSize();

  // Casting.
  static inline RangeIterator* cast(Object* object);

  // Layout.
  static const int kRangeOffset = HeapObject::kSize;
  static const int kCurValueOffset = kRangeOffset + kPointerSize;
  static const int kSize = kCurValueOffset + kPointerSize;

 private:
  static inline bool isOutOfRange(word cur, word stop, word step);

  DISALLOW_IMPLICIT_CONSTRUCTORS(RangeIterator);
};

class Slice : public HeapObject {
 public:
  // Getters and setters.
  inline Object* start();
  inline void setStart(Object* value);

  inline Object* stop();
  inline void setStop(Object* value);

  inline Object* step();
  inline void setStep(Object* value);

  // Returns the correct start, stop, and step word values from this slice
  void unpack(word* start, word* stop, word* step);

  // Takes in the length of a list and the start, stop, and step values
  // Returns the length of the new list and the corrected start and stop values
  static word adjustIndices(word length, word* start, word* stop, word step);

  // Casting.
  static inline Slice* cast(Object* object);

  // Sizing.
  static inline word allocationSize();

  // Layout.
  static const int kStartOffset = HeapObject::kSize;
  static const int kStopOffset = kStartOffset + kPointerSize;
  static const int kStepOffset = kStopOffset + kPointerSize;
  static const int kSize = kStepOffset + kPointerSize;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Slice);
};

class ListIterator : public HeapObject {
 public:
  // Getters and setters.
  inline word index();
  inline void setIndex(word index);

  inline Object* list();
  inline void setList(Object* list);

  // Iteration.
  inline Object* next();

  // Sizing.
  static inline word allocationSize();

  // Casting.
  static inline ListIterator* cast(Object* object);

  // Layout.
  static const int kListOffset = HeapObject::kSize;
  static const int kIndexOffset = kListOffset + kPointerSize;
  static const int kSize = kIndexOffset + kPointerSize;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ListIterator);
};

class Code : public HeapObject {
 public:
  // Getters and setters.
  inline word argcount();
  inline void setArgcount(word value);

  inline word cell2arg();
  inline void setCell2arg(word value);

  inline Object* cellvars();
  inline void setCellvars(Object* value);
  inline word numCellvars();

  inline Object* code();
  inline void setCode(Object* value);

  inline Object* consts();
  inline void setConsts(Object* value);

  inline Object* filename();
  inline void setFilename(Object* value);

  inline word firstlineno();
  inline void setFirstlineno(word value);

  inline word flags();
  inline void setFlags(word value);

  inline Object* freevars();
  inline void setFreevars(Object* value);
  inline word numFreevars();

  inline word kwonlyargcount();
  inline void setKwonlyargcount(word value);

  inline Object* lnotab();
  inline void setLnotab(Object* value);

  inline Object* name();
  inline void setName(Object* value);

  inline Object* names();
  inline void setNames(Object* value);

  inline word nlocals();
  inline void setNlocals(word value);

  inline word stacksize();
  inline void setStacksize(word value);

  inline Object* varnames();
  inline void setVarnames(Object* value);

  // Casting.
  inline static Code* cast(Object* obj);

  // Sizing.
  inline static word allocationSize();

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

  // A dictionary containing parameter annotations
  inline Object* annotations();
  inline void setAnnotations(Object* annotations);

  // The code object backing this function or None
  inline Object* code();
  inline void setCode(Object* code);

  // A tuple of cell objects that contain bindings for the function's free
  // variables. Read-only to user code.
  inline Object* closure();
  inline void setClosure(Object* closure);

  // A tuple containing default values for arguments with defaults. Read-only
  // to user code.
  inline Object* defaults();
  inline void setDefaults(Object* defaults);
  inline bool hasDefaults();

  // The function's docstring
  inline Object* doc();
  inline void setDoc(Object* doc);

  // Returns the entry to be used when the function is invoked via
  // CALL_FUNCTION
  inline Entry entry();
  inline void setEntry(Entry entry);

  // Returns the entry to be used when the fucntion is invoked via
  // CALL_FUNCTION_KW
  inline Entry entryKw();
  inline void setEntryKw(Entry entryKw);

  // The dictionary that holds this function's global namespace. User-code
  // cannot change this
  inline Object* globals();
  inline void setGlobals(Object* globals);

  // A dictionary containing defaults for keyword-only parameters
  inline Object* kwDefaults();
  inline void setKwDefaults(Object* kwDefaults);

  // The name of the module the function was defined in
  inline Object* module();
  inline void setModule(Object* module);

  // The function's name
  inline Object* name();
  inline void setName(Object* name);

  // The function's qualname
  inline Object* qualname();
  inline void setQualname(Object* qualname);

  // The pre-computed object array provided fast globals access.
  // fastGlobals[arg] == globals[names[arg]]
  inline Object* fastGlobals();
  inline void setFastGlobals(Object* fast_globals);

  // Casting.
  inline static Function* cast(Object* object);

  // Sizing.
  inline static word allocationSize();

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
  static const int kFastGlobalsOffset = kEntryKwOffset + kPointerSize;
  static const int kSize = kFastGlobalsOffset + kPointerSize;

 private:
  DISALLOW_COPY_AND_ASSIGN(Function);
};

class Instance : public HeapObject {
 public:
  // Casting.
  inline static Instance* cast(Object* object);

  // Sizing.
  inline static word allocationSize(word num_attributes);

 private:
  DISALLOW_COPY_AND_ASSIGN(Instance);
};

class Module : public HeapObject {
 public:
  // Setters and getters.
  inline Object* name();
  inline void setName(Object* name);

  inline Object* dictionary();
  inline void setDictionary(Object* dictionary);

  // Casting.
  inline static Module* cast(Object* object);

  // Allocation.
  inline static word allocationSize();

  // Layout.
  static const int kNameOffset = HeapObject::kSize;
  static const int kDictionaryOffset = kNameOffset + kPointerSize;
  static const int kSize = kDictionaryOffset + kPointerSize;

 private:
  DISALLOW_COPY_AND_ASSIGN(Module);
};

class NotImplemented : public HeapObject {
 public:
  // Casting.
  static inline NotImplemented* cast(Object* object);

  // Sizing.
  static inline word allocationSize();

  // Layout
  // kPaddingOffset is not used, but the GC expects the object to be
  // at least one word.
  static const int kPaddingOffset = HeapObject::kSize;
  static const int kSize = kPaddingOffset + kPointerSize;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(NotImplemented);
};

/**
 * A simple dictionary that uses open addressing and linear probing.
 *
 * Layout:
 *
 *   [Class pointer]
 *   [NumItems     ] - Number of items currently in the dictionary
 *   [Items        ] - Pointer to an ObjectArray that stores the underlying
 * data.
 *
 * Dictionary entries are stored in buckets as a triple of (hash, key, value).
 * Empty buckets are stored as (None, None, None).
 * Tombstone buckets are stored as (None, <not None>, <Any>).
 *
 */
class Dictionary : public HeapObject {
 public:
  // Getters and setters.
  inline static Dictionary* cast(Object* object);

  // The ObjectArray backing the dictionary
  inline Object* data();
  inline void setData(Object* data);

  // Number of items currently in the dictionary
  inline word numItems();
  inline void setNumItems(word numItems);

  // Sizing.
  inline static word allocationSize();

  // Layout.
  static const int kNumItemsOffset = HeapObject::kSize;
  static const int kDataOffset = kNumItemsOffset + kPointerSize;
  static const int kSize = kDataOffset + kPointerSize;

 private:
  DISALLOW_COPY_AND_ASSIGN(Dictionary);
};

/**
 * A simple set implementation.
 */
class Set : public HeapObject {
 public:
  // Getters and setters.
  inline static Set* cast(Object* object);

  // The ObjectArray backing the set
  inline Object* data();
  inline void setData(Object* data);

  // Number of items currently in the set
  inline word numItems();
  inline void setNumItems(word numItems);

  // Sizing.
  inline static word allocationSize();

  // Layout.
  static const int kNumItemsOffset = HeapObject::kSize;
  static const int kDataOffset = kNumItemsOffset + kPointerSize;
  static const int kSize = kDataOffset + kPointerSize;

 private:
  DISALLOW_COPY_AND_ASSIGN(Set);
};

/**
 * A growable array
 *
 * Layout:
 *
 *   [Class pointer]
 *   [Length       ] - Number of elements currently in the list
 *   [Elems        ] - Pointer to an ObjectArray that contains list elements
 */
class List : public HeapObject {
 public:
  // Getters and setters.
  inline Object* at(word index);
  inline void atPut(word index, Object* value);
  inline Object* items();
  inline void setItems(Object* items);
  inline word allocated();
  inline void setAllocated(word allocated);

  // Return the total number of elements that may be held without growing the
  // list
  inline word capacity();

  // Casting.
  static inline List* cast(Object* object);

  // Sizing.
  static inline word allocationSize();

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
  inline Object* value();
  inline void setValue(Object* object);

  inline bool isUnbound();
  inline void makeUnbound();

  // Casting.
  static inline ValueCell* cast(Object* object);

  // Sizing.
  static inline word allocationSize();

  // Layout.
  static const int kValueOffset = HeapObject::kSize;
  static const int kSize = kValueOffset + kPointerSize;

 private:
  DISALLOW_COPY_AND_ASSIGN(ValueCell);
};

class Ellipsis : public HeapObject {
 public:
  // Casting.
  static inline Ellipsis* cast(Object* object);

  // Sizing.
  static inline word allocationSize();

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
  inline Object* referent();
  inline void setReferent(Object* referent);

  // A callable object invoked with the referent as an argument when the
  // referent is deemed to be "near death" and only reachable through this weak
  // reference.
  inline Object* callback();
  inline void setCallback(Object* callable);

  // Singly linked list of weak reference objects.  This field is used during
  // garbage collection to represent the set of weak references that had been
  // discovered by the initial trace with an otherwise unreachable referent.
  inline Object* link();
  inline void setLink(Object* reference);

  // Casting.
  static inline WeakRef* cast(Object* object);

  // Sizing.
  static inline word allocationSize();

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
  inline Object* function();
  inline void setFunction(Object* function);

  // The instance of "self" being bound
  inline Object* self();
  inline void setSelf(Object* self);

  // Casting
  static inline BoundMethod* cast(Object* object);

  // Sizing
  static inline word allocationSize();

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
  inline Object* function();
  inline void setFunction(Object* function);

  // Casting
  static inline ClassMethod* cast(Object* object);

  // Sizing
  static inline word allocationSize();

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
 *   | Delegate?                 |
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
  inline word id();

  // Returns the class whose instances are described by this layout
  inline Object* describedClass();
  inline void setDescribedClass(Object* klass);

  // Set the number of in-object attributes that may be stored on an instance
  // described by this layout.
  //
  // N.B. - This will always be less than or equal to the length of the
  // ObjectArray returned by inObjectAttributes().
  inline void setNumInObjectAttributes(word count);
  inline word numInObjectAttributes();

  // Returns an ObjectArray describing the attributes stored directly in
  // in the instance.
  //
  // Each item in the object array is a two element tuple. Each tuple is
  // composed of the following elements, in order:
  //
  //   1. The attribute name (String)
  //   2. The attribute info (AttributeInfo)
  inline Object* inObjectAttributes();
  inline void setInObjectAttributes(Object* attributes);

  // Returns an ObjectArray describing the attributes stored in the overflow
  // array of the instance.
  //
  // Each item in the object array is a two element tuple. Each tuple is
  // composed of the following elements, in order:
  //
  //   1. The attribute name (String)
  //   2. The attribute info (AttributeInfo)
  inline Object* overflowAttributes();
  inline void setOverflowAttributes(Object* attributes);

  // Returns a flattened list of tuples. Each tuple is composed of the
  // following elements, in order:
  //
  //   1. The attribute name (String)
  //   2. The layout that would result if an attribute with that name
  //      was added.
  inline Object* additions();
  inline void setAdditions(Object* additions);

  // Returns a flattened list of tuples. Each tuple is composed of the
  // following elements, in order:
  //
  //   1. The attribute name (String)
  //   2. The layout that would result if an attribute with that name
  //      was deleted.
  inline Object* deletions();
  inline void setDeletions(Object* deletions);

  // Returns the number of words in an instance described by this layout,
  // including the overflow array.
  inline word instanceSize();
  inline void setInstanceSize(word num_words);

  // Return the offset, in bytes, of the overflow slot
  inline word overflowOffset();

  // Return the offset, in bytes, of the delegate slot, if one
  // exists. Otherwise, return None.
  //
  // Instances of classes that descend from variable-sized intrinsic types
  // (e.g. tuple, string) have an extra slot at the end of the instance that
  // points to a delegate object of the appropriate type. Methods that operate
  // expect to operate on the variable sized data are forwarded to the
  // delegate.
  inline word delegateOffset();
  inline bool hasDelegateSlot();
  inline void addDelegateSlot();

  // Casting
  inline static Layout* cast(Object* object);

  // Sizing
  static inline word allocationSize();

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
  static const int kDelegateOffsetOffset = kOverflowOffsetOffset + kPointerSize;
  static const int kNumInObjectAttributesOffset =
      kDelegateOffsetOffset + kPointerSize;
  static const int kSize = kNumInObjectAttributesOffset + kPointerSize;

 private:
  inline void setDelegateOffset(word offset);
  inline void setOverflowOffset(word offset);
  inline void updateInstanceSize();

  DISALLOW_IMPLICIT_CONSTRUCTORS(Layout);
};

class Super : public HeapObject {
 public:
  // getters and setters
  inline Object* type();
  inline void setType(Object* tp);
  inline Object* object();
  inline void setObject(Object* obj);
  inline Object* objectType();
  inline void setObjectType(Object* tp);

  // Casting
  static inline Super* cast(Object* object);

  // Sizing
  static inline word allocationSize();

  // Layout
  static const int kTypeOffset = HeapObject::kSize;
  static const int kObjectOffset = kTypeOffset + kPointerSize;
  static const int kObjectTypeOffset = kObjectOffset + kPointerSize;
  static const int kSize = kObjectTypeOffset + kPointerSize;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Super);
};

// Object

bool Object::isObject() {
  return true;
}

word Object::layoutId() {
  if (isHeapObject()) {
    return HeapObject::cast(this)->header()->layoutId();
  }
  if (isSmallInteger()) {
    return IntrinsicLayoutId::kSmallInteger;
  }
  return reinterpret_cast<uword>(this) & kImmediateClassTableIndexMask;
}

bool Object::isClass() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() ==
      IntrinsicLayoutId::kType;
}

bool Object::isClassMethod() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() ==
      IntrinsicLayoutId::kClassMethod;
}

bool Object::isSmallInteger() {
  uword tag = reinterpret_cast<uword>(this) & SmallInteger::kTagMask;
  return tag == SmallInteger::kTag;
}

bool Object::isSmallString() {
  uword tag = reinterpret_cast<uword>(this) & SmallString::kTagMask;
  return tag == SmallString::kTag;
}

bool Object::isHeader() {
  uword tag = reinterpret_cast<uword>(this) & Header::kTagMask;
  return tag == Header::kTag;
}

bool Object::isBoolean() {
  uword tag = reinterpret_cast<uword>(this) & Boolean::kTagMask;
  return tag == Boolean::kTag;
}

bool Object::isNone() {
  uword tag = reinterpret_cast<uword>(this) & None::kTagMask;
  return tag == None::kTag;
}

bool Object::isError() {
  uword tag = reinterpret_cast<uword>(this) & None::kTagMask;
  return tag == Error::kTag;
}

bool Object::isHeapObject() {
  uword tag = reinterpret_cast<uword>(this) & HeapObject::kTagMask;
  return tag == HeapObject::kTag;
}

bool Object::isLayout() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() ==
      IntrinsicLayoutId::kLayout;
}

bool Object::isBoundMethod() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() ==
      IntrinsicLayoutId::kBoundMethod;
}

bool Object::isByteArray() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() ==
      IntrinsicLayoutId::kByteArray;
}

bool Object::isObjectArray() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() ==
      IntrinsicLayoutId::kObjectArray;
}

bool Object::isCode() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() ==
      IntrinsicLayoutId::kCode;
}

bool Object::isComplex() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() ==
      IntrinsicLayoutId::kComplex;
}

bool Object::isLargeString() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() ==
      IntrinsicLayoutId::kLargeString;
}

bool Object::isFunction() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() ==
      IntrinsicLayoutId::kFunction;
}

bool Object::isInstance() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() >
      IntrinsicLayoutId::kLastId;
}

bool Object::isDictionary() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() ==
      IntrinsicLayoutId::kDictionary;
}

bool Object::isDouble() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() ==
      IntrinsicLayoutId::kDouble;
}

bool Object::isSet() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() ==
      IntrinsicLayoutId::kSet;
}

bool Object::isSuper() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() ==
      IntrinsicLayoutId::kSuper;
}

bool Object::isModule() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() ==
      IntrinsicLayoutId::kModule;
}

bool Object::isList() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() ==
      IntrinsicLayoutId::kList;
}

bool Object::isListIterator() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() ==
      IntrinsicLayoutId::kListIterator;
}

bool Object::isValueCell() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() ==
      IntrinsicLayoutId::kValueCell;
}

bool Object::isEllipsis() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() ==
      IntrinsicLayoutId::kEllipsis;
}

bool Object::isLargeInteger() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() ==
      IntrinsicLayoutId::kLargeInteger;
}

bool Object::isInteger() {
  return isSmallInteger() || isLargeInteger();
}

bool Object::isNotImplemented() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() ==
      IntrinsicLayoutId::kNotImplemented;
}

bool Object::isRange() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() ==
      IntrinsicLayoutId::kRange;
}

bool Object::isRangeIterator() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() ==
      IntrinsicLayoutId::kRangeIterator;
}

bool Object::isSlice() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() ==
      IntrinsicLayoutId::kSlice;
}

bool Object::isString() {
  return isSmallString() || isLargeString();
}

bool Object::isWeakRef() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->layoutId() ==
      IntrinsicLayoutId::kWeakRef;
}

bool Object::equals(Object* lhs, Object* rhs) {
  return (lhs == rhs) ||
      (lhs->isLargeString() && LargeString::cast(lhs)->equals(rhs));
}

Object* Object::cast(Object* object) {
  return object;
}

// Integer

Integer* Integer::cast(Object* object) {
  DCHECK(object->isInteger(), "invalid cast");
  return reinterpret_cast<Integer*>(object);
}

word Integer::asWord() {
  if (isSmallInteger()) {
    return SmallInteger::cast(this)->value();
  }
  return LargeInteger::cast(this)->asWord();
}

void* Integer::asCPointer() {
  if (isSmallInteger()) {
    return SmallInteger::cast(this)->asCPointer();
  }
  return LargeInteger::cast(this)->asCPointer();
}

// SmallInteger

SmallInteger* SmallInteger::cast(Object* object) {
  DCHECK(object->isSmallInteger(), "invalid cast");
  return reinterpret_cast<SmallInteger*>(object);
}

word SmallInteger::value() {
  return reinterpret_cast<word>(this) >> kTagSize;
}

void* SmallInteger::asCPointer() {
  return reinterpret_cast<void*>(value());
}

SmallInteger* SmallInteger::fromWord(word value) {
  DCHECK(SmallInteger::isValid(value), "invalid cast");
  return reinterpret_cast<SmallInteger*>(value << kTagSize);
}

template <typename T>
SmallInteger* SmallInteger::fromFunctionPointer(T pointer) {
  // The bit pattern for a function pointer object must be indistinguishable
  // from that of a small integer object.
  auto object = reinterpret_cast<Object*>(reinterpret_cast<uword>(pointer));
  return SmallInteger::cast(object);
}

template <typename T>
T SmallInteger::asFunctionPointer() {
  return reinterpret_cast<T>(reinterpret_cast<uword>(this));
}

// SmallString

word SmallString::length() {
  return (reinterpret_cast<word>(this) >> kTagSize) & kMaxLength;
}

byte SmallString::charAt(word index) {
  DCHECK_INDEX(index, length());
  return reinterpret_cast<word>(this) >> (kBitsPerByte * (index + 1));
}

void SmallString::copyTo(byte* dst, word length) {
  DCHECK_BOUND(length, this->length());
  for (word i = 0; i < length; ++i) {
    *dst++ = charAt(i);
  }
}

SmallString* SmallString::cast(Object* object) {
  DCHECK(object->isSmallString(), "invalid cast");
  return reinterpret_cast<SmallString*>(object);
}

// Header

Header* Header::cast(Object* object) {
  DCHECK(object->isHeader(), "invalid cast");
  return reinterpret_cast<Header*>(object);
}

word Header::count() {
  auto header = reinterpret_cast<uword>(this);
  return (header >> kCountOffset) & kCountMask;
}

bool Header::hasOverflow() {
  return count() == kCountOverflowFlag;
}

word Header::hashCode() {
  auto header = reinterpret_cast<uword>(this);
  return (header >> kHashCodeOffset) & kHashCodeMask;
}

Header* Header::withHashCode(word value) {
  auto header = reinterpret_cast<uword>(this);
  header &= ~(kHashCodeMask << kHashCodeOffset);
  header |= (value & kHashCodeMask) << kHashCodeOffset;
  return reinterpret_cast<Header*>(header);
}

word Header::layoutId() {
  auto header = reinterpret_cast<uword>(this);
  return (header >> kLayoutIdOffset) & kLayoutIdMask;
}

Header* Header::withLayoutId(word layout_id) {
  DCHECK_BOUND(layout_id, kMaxLayoutId);
  auto header = reinterpret_cast<uword>(this);
  header &= ~(kLayoutIdMask << kLayoutIdOffset);
  header |= (layout_id & kLayoutIdMask) << kLayoutIdOffset;
  return reinterpret_cast<Header*>(header);
}

ObjectFormat Header::format() {
  auto header = reinterpret_cast<uword>(this);
  return static_cast<ObjectFormat>((header >> kFormatOffset) & kFormatMask);
}

Header* Header::from(word count, word hash, word id, ObjectFormat format) {
  DCHECK(
      (count >= 0) && ((count <= kCountMax) || (count == kCountOverflowFlag)),
      "bounds violation, %ld not in 0..%d",
      count,
      kCountMax);
  uword result = Header::kTag;
  result |= ((count > kCountMax) ? kCountOverflowFlag : count) << kCountOffset;
  result |= hash << kHashCodeOffset;
  result |= static_cast<uword>(id) << kLayoutIdOffset;
  result |= static_cast<uword>(format) << kFormatOffset;
  return reinterpret_cast<Header*>(result);
}

// None

None* None::object() {
  return reinterpret_cast<None*>(kTag);
}

None* None::cast(Object* object) {
  DCHECK(object->isNone(), "invalid cast, expected None");
  return reinterpret_cast<None*>(object);
}

// Error

Error* Error::object() {
  return reinterpret_cast<Error*>(kTag);
}

Error* Error::cast(Object* object) {
  DCHECK(object->isError(), "invalid cast, expected Error");
  return reinterpret_cast<Error*>(object);
}

// Boolean

Boolean* Boolean::fromBool(bool value) {
  return reinterpret_cast<Boolean*>(
      (static_cast<uword>(value) << kTagSize) | kTag);
}

Boolean* Boolean::cast(Object* object) {
  DCHECK(object->isBoolean(), "invalid cast, expected Boolean");
  return reinterpret_cast<Boolean*>(object);
}

bool Boolean::value() {
  return (reinterpret_cast<uword>(this) >> kTagSize) ? true : false;
}

// HeapObject

uword HeapObject::address() {
  return reinterpret_cast<uword>(this) - HeapObject::kTag;
}

uword HeapObject::baseAddress() {
  word result = address() - Header::kSize;
  if (header()->hasOverflow()) {
    result -= kPointerSize;
  }
  return result;
}

Header* HeapObject::header() {
  return Header::cast(instanceVariableAt(kHeaderOffset));
}

void HeapObject::setHeader(Header* header) {
  instanceVariableAtPut(kHeaderOffset, header);
}

word HeapObject::headerOverflow() {
  DCHECK(header()->hasOverflow(), "expected Overflow");
  return SmallInteger::cast(instanceVariableAt(kHeaderOverflowOffset))->value();
}

void HeapObject::setHeaderAndOverflow(
    word count,
    word hash,
    word id,
    ObjectFormat format) {
  if (count > Header::kCountMax) {
    instanceVariableAtPut(kHeaderOverflowOffset, SmallInteger::fromWord(count));
    count = Header::kCountOverflowFlag;
  }
  setHeader(Header::from(count, hash, id, format));
}

HeapObject* HeapObject::fromAddress(uword address) {
  DCHECK((address & kTagMask) == 0, "invalid cast, expected heap address");
  return reinterpret_cast<HeapObject*>(address + kTag);
}

word HeapObject::headerCountOrOverflow() {
  if (header()->hasOverflow()) {
    return headerOverflow();
  }
  return header()->count();
}

word HeapObject::size() {
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

HeapObject* HeapObject::cast(Object* object) {
  DCHECK(object->isHeapObject(), "invalid cast, expected heap object");
  return reinterpret_cast<HeapObject*>(object);
}

word HeapObject::headerSize(word count) {
  word result = kPointerSize;
  if (count > Header::kCountMax) {
    result += kPointerSize;
  }
  return result;
}

void HeapObject::initialize(word size, Object* value) {
  for (word offset = HeapObject::kSize; offset < size; offset += kPointerSize) {
    instanceVariableAtPut(offset, value);
  }
}

bool HeapObject::isRoot() {
  return header()->format() == ObjectFormat::kObjectArray ||
      header()->format() == ObjectFormat::kObjectInstance;
}

bool HeapObject::isForwarding() {
  return *reinterpret_cast<uword*>(address() + kHeaderOffset) == kIsForwarded;
}

Object* HeapObject::forward() {
  // When a heap object is forwarding, its second word is the forwarding
  // address.
  return *reinterpret_cast<Object**>(address() + kHeaderOffset + kPointerSize);
}

void HeapObject::forwardTo(Object* object) {
  // Overwrite the header with the forwarding marker.
  *reinterpret_cast<uword*>(address() + kHeaderOffset) = kIsForwarded;
  // Overwrite the second word with the forwarding addressing.
  *reinterpret_cast<Object**>(address() + kHeaderOffset + kPointerSize) =
      object;
}

Object* HeapObject::instanceVariableAt(word offset) {
  return *reinterpret_cast<Object**>(address() + offset);
}

void HeapObject::instanceVariableAtPut(word offset, Object* value) {
  *reinterpret_cast<Object**>(address() + offset) = value;
}

// Class

word Class::allocationSize() {
  return Header::kSize + Class::kSize;
}

Object* Class::mro() {
  return instanceVariableAt(kMroOffset);
}

void Class::setMro(Object* object_array) {
  instanceVariableAtPut(kMroOffset, object_array);
}

Object* Class::instanceLayout() {
  return instanceVariableAt(kInstanceLayoutOffset);
}

void Class::setInstanceLayout(Object* layout) {
  instanceVariableAtPut(kInstanceLayoutOffset, layout);
}

Object* Class::name() {
  return instanceVariableAt(kNameOffset);
}

void Class::setName(Object* name) {
  instanceVariableAtPut(kNameOffset, name);
}

Object* Class::flags() {
  return instanceVariableAt(kFlagsOffset);
}

void Class::setFlags(Object* value) {
  instanceVariableAtPut(kFlagsOffset, value);
}

void Class::setFlag(Class::Flag bit) {
  word f = SmallInteger::cast(flags())->value();
  Object* new_flag = SmallInteger::fromWord(f | (1 << bit));
  instanceVariableAtPut(kFlagsOffset, new_flag);
}

bool Class::hasFlag(Class::Flag bit) {
  word f = SmallInteger::cast(flags())->value();
  return (f & (1 << bit)) != 0;
}

Object* Class::dictionary() {
  return instanceVariableAt(kDictionaryOffset);
}

void Class::setDictionary(Object* dictionary) {
  instanceVariableAtPut(kDictionaryOffset, dictionary);
}

Object* Class::builtinBaseClass() {
  return instanceVariableAt(kBuiltinBaseClassOffset);
}

void Class::setBuiltinBaseClass(Object* base) {
  instanceVariableAtPut(kBuiltinBaseClassOffset, base);
}

Class* Class::cast(Object* object) {
  DCHECK(object->isClass(), "invalid cast, expected class");
  return reinterpret_cast<Class*>(object);
}

bool Class::isIntrinsicOrExtension() {
  return Layout::cast(instanceLayout())->id() <= IntrinsicLayoutId::kLastId;
}

// Array

word Array::length() {
  DCHECK(
      isByteArray() || isObjectArray() || isLargeString(),
      "invalid array type");
  return headerCountOrOverflow();
}

// ByteArray

word ByteArray::allocationSize(word length) {
  DCHECK(length >= 0, "invalid length %ld", length);
  word size = headerSize(length) + length;
  return Utils::maximum(kMinimumSize, Utils::roundUp(size, kPointerSize));
}

byte ByteArray::byteAt(word index) {
  DCHECK_INDEX(index, length());
  return *reinterpret_cast<byte*>(address() + index);
}

void ByteArray::byteAtPut(word index, byte value) {
  DCHECK_INDEX(index, length());
  *reinterpret_cast<byte*>(address() + index) = value;
}

ByteArray* ByteArray::cast(Object* object) {
  DCHECK(object->isByteArray(), "invalid cast, expected byte array");
  return reinterpret_cast<ByteArray*>(object);
}

// ObjectArray

word ObjectArray::allocationSize(word length) {
  DCHECK(length >= 0, "invalid length %ld", length);
  word size = headerSize(length) + length * kPointerSize;
  return Utils::maximum(kMinimumSize, Utils::roundUp(size, kPointerSize));
}

ObjectArray* ObjectArray::cast(Object* object) {
  DCHECK(object->isObjectArray(), "invalid cast, expected object array");
  return reinterpret_cast<ObjectArray*>(object);
}

Object* ObjectArray::at(word index) {
  DCHECK_INDEX(index, length());
  return instanceVariableAt(index * kPointerSize);
}

void ObjectArray::atPut(word index, Object* value) {
  DCHECK_INDEX(index, length());
  instanceVariableAtPut(index * kPointerSize, value);
}

void ObjectArray::copyTo(Object* array) {
  word len = length();
  ObjectArray* dst = ObjectArray::cast(array);
  DCHECK_BOUND(len, dst->length());
  for (int i = 0; i < len; i++) {
    Object* elem = at(i);
    dst->atPut(i, elem);
  }
}

// Code

Code* Code::cast(Object* obj) {
  DCHECK(obj->isCode(), "invalid cast, expecting code object");
  return reinterpret_cast<Code*>(obj);
}

word Code::allocationSize() {
  return Header::kSize + Code::kSize;
}

word Code::argcount() {
  return SmallInteger::cast(instanceVariableAt(kArgcountOffset))->value();
}

void Code::setArgcount(word value) {
  instanceVariableAtPut(kArgcountOffset, SmallInteger::fromWord(value));
}

word Code::cell2arg() {
  return SmallInteger::cast(instanceVariableAt(kCell2argOffset))->value();
}

void Code::setCell2arg(word value) {
  instanceVariableAtPut(kCell2argOffset, SmallInteger::fromWord(value));
}

Object* Code::cellvars() {
  return instanceVariableAt(kCellvarsOffset);
}

void Code::setCellvars(Object* value) {
  instanceVariableAtPut(kCellvarsOffset, value);
}

word Code::numCellvars() {
  Object* object = cellvars();
  DCHECK(object->isNone() || object->isObjectArray(), "not an object array");
  if (object->isNone()) {
    return 0;
  }
  return ObjectArray::cast(object)->length();
}

Object* Code::code() {
  return instanceVariableAt(kCodeOffset);
}

void Code::setCode(Object* value) {
  instanceVariableAtPut(kCodeOffset, value);
}

Object* Code::consts() {
  return instanceVariableAt(kConstsOffset);
}

void Code::setConsts(Object* value) {
  instanceVariableAtPut(kConstsOffset, value);
}

Object* Code::filename() {
  return instanceVariableAt(kFilenameOffset);
}

void Code::setFilename(Object* value) {
  instanceVariableAtPut(kFilenameOffset, value);
}

word Code::firstlineno() {
  return SmallInteger::cast(instanceVariableAt(kFirstlinenoOffset))->value();
}

void Code::setFirstlineno(word value) {
  instanceVariableAtPut(kFirstlinenoOffset, SmallInteger::fromWord(value));
}

word Code::flags() {
  return SmallInteger::cast(instanceVariableAt(kFlagsOffset))->value();
}

void Code::setFlags(word value) {
  instanceVariableAtPut(kFlagsOffset, SmallInteger::fromWord(value));
}

Object* Code::freevars() {
  return instanceVariableAt(kFreevarsOffset);
}

void Code::setFreevars(Object* value) {
  instanceVariableAtPut(kFreevarsOffset, value);
}

word Code::numFreevars() {
  Object* object = freevars();
  DCHECK(object->isNone() || object->isObjectArray(), "not an object array");
  if (object->isNone()) {
    return 0;
  }
  return ObjectArray::cast(object)->length();
}

word Code::kwonlyargcount() {
  return SmallInteger::cast(instanceVariableAt(kKwonlyargcountOffset))->value();
}

void Code::setKwonlyargcount(word value) {
  instanceVariableAtPut(kKwonlyargcountOffset, SmallInteger::fromWord(value));
}

Object* Code::lnotab() {
  return instanceVariableAt(kLnotabOffset);
}

void Code::setLnotab(Object* value) {
  instanceVariableAtPut(kLnotabOffset, value);
}

Object* Code::name() {
  return instanceVariableAt(kNameOffset);
}

void Code::setName(Object* value) {
  instanceVariableAtPut(kNameOffset, value);
}

Object* Code::names() {
  return instanceVariableAt(kNamesOffset);
}

void Code::setNames(Object* value) {
  instanceVariableAtPut(kNamesOffset, value);
}

word Code::nlocals() {
  return SmallInteger::cast(instanceVariableAt(kNlocalsOffset))->value();
}

void Code::setNlocals(word value) {
  instanceVariableAtPut(kNlocalsOffset, SmallInteger::fromWord(value));
}

word Code::stacksize() {
  return SmallInteger::cast(instanceVariableAt(kStacksizeOffset))->value();
}

void Code::setStacksize(word value) {
  instanceVariableAtPut(kStacksizeOffset, SmallInteger::fromWord(value));
}

Object* Code::varnames() {
  return instanceVariableAt(kVarnamesOffset);
}

void Code::setVarnames(Object* value) {
  instanceVariableAtPut(kVarnamesOffset, value);
}

// LargeInteger

word LargeInteger::asWord() {
  return *reinterpret_cast<word*>(address() + kValueOffset);
}

void* LargeInteger::asCPointer() {
  return *reinterpret_cast<void**>(address() + kValueOffset);
}

void LargeInteger::initialize(word value) {
  *reinterpret_cast<word*>(address() + kValueOffset) = value;
}

word LargeInteger::allocationSize() {
  return Header::kSize + LargeInteger::kSize;
}

LargeInteger* LargeInteger::cast(Object* object) {
  DCHECK(object->isLargeInteger(), "not a large integer");
  return reinterpret_cast<LargeInteger*>(object);
}

// Double

double Double::value() {
  return *reinterpret_cast<double*>(address() + kValueOffset);
}

word Double::allocationSize() {
  return Header::kSize + Double::kSize;
}

Double* Double::cast(Object* object) {
  DCHECK(object->isDouble(), "not a double");
  return reinterpret_cast<Double*>(object);
}

void Double::initialize(double value) {
  *reinterpret_cast<double*>(address() + kValueOffset) = value;
}

// Complex
double Complex::real() {
  return *reinterpret_cast<double*>(address() + kRealOffset);
}

double Complex::imag() {
  return *reinterpret_cast<double*>(address() + kImagOffset);
}

word Complex::allocationSize() {
  return Header::kSize + Complex::kSize;
}

Complex* Complex::cast(Object* object) {
  DCHECK(object->isComplex(), "not a complex");
  return reinterpret_cast<Complex*>(object);
}

void Complex::initialize(double real, double imag) {
  *reinterpret_cast<double*>(address() + kRealOffset) = real;
  *reinterpret_cast<double*>(address() + kImagOffset) = imag;
}

// Range

word Range::start() {
  return SmallInteger::cast(instanceVariableAt(kStartOffset))->value();
}

void Range::setStart(word value) {
  instanceVariableAtPut(kStartOffset, SmallInteger::fromWord(value));
}

word Range::stop() {
  return SmallInteger::cast(instanceVariableAt(kStopOffset))->value();
}

void Range::setStop(word value) {
  instanceVariableAtPut(kStopOffset, SmallInteger::fromWord(value));
}

word Range::step() {
  return SmallInteger::cast(instanceVariableAt(kStepOffset))->value();
}

void Range::setStep(word value) {
  instanceVariableAtPut(kStepOffset, SmallInteger::fromWord(value));
}

word Range::allocationSize() {
  return Header::kSize + Range::kSize;
}

Range* Range::cast(Object* object) {
  DCHECK(object->isRange(), "invalid cast, expected range");
  return reinterpret_cast<Range*>(object);
}

// ListIterator

word ListIterator::index() {
  return SmallInteger::cast(instanceVariableAt(kIndexOffset))->value();
}

void ListIterator::setIndex(word index) {
  instanceVariableAtPut(kIndexOffset, SmallInteger::fromWord(index));
}

Object* ListIterator::list() {
  return instanceVariableAt(kListOffset);
}

void ListIterator::setList(Object* list) {
  instanceVariableAtPut(kListOffset, list);
}

Object* ListIterator::next() {
  word idx = index();
  auto underlying = List::cast(list());
  if (idx >= underlying->allocated()) {
    return Error::object();
  }

  Object* item = underlying->at(idx);
  setIndex(idx + 1);
  return item;
}

word ListIterator::allocationSize() {
  return Header::kSize + ListIterator::kSize;
}

ListIterator* ListIterator::cast(Object* object) {
  DCHECK(object->isListIterator(), "invalid cast, expected list iterator");
  return reinterpret_cast<ListIterator*>(object);
}

// RangeIterator

void RangeIterator::setRange(Object* range) {
  auto r = Range::cast(range);
  instanceVariableAtPut(kRangeOffset, r);
  instanceVariableAtPut(kCurValueOffset, SmallInteger::fromWord(r->start()));
}

bool RangeIterator::isOutOfRange(word cur, word stop, word step) {
  DCHECK(
      step != 0, "invalid step"); // should have been checked in builtinRange().

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

Object* RangeIterator::next() {
  auto ret = SmallInteger::cast(instanceVariableAt(kCurValueOffset));
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

  instanceVariableAtPut(kCurValueOffset, SmallInteger::fromWord(cur + step));
  return ret;
}

word RangeIterator::allocationSize() {
  return Header::kSize + RangeIterator::kSize;
}

RangeIterator* RangeIterator::cast(Object* object) {
  DCHECK(object->isRangeIterator(), "invalid cast, expected range interator");
  return reinterpret_cast<RangeIterator*>(object);
}

// Slice

Object* Slice::start() {
  return instanceVariableAt(kStartOffset);
}

void Slice::setStart(Object* value) {
  instanceVariableAtPut(kStartOffset, value);
}

Object* Slice::stop() {
  return instanceVariableAt(kStopOffset);
}

void Slice::setStop(Object* value) {
  instanceVariableAtPut(kStopOffset, value);
}

Object* Slice::step() {
  return instanceVariableAt(kStepOffset);
}

void Slice::setStep(Object* value) {
  instanceVariableAtPut(kStepOffset, value);
}

word Slice::allocationSize() {
  return Header::kSize + Slice::kSize;
}

Slice* Slice::cast(Object* object) {
  DCHECK(object->isSlice(), "invalid cast, expected slice");
  return reinterpret_cast<Slice*>(object);
}

// Dictionary

word Dictionary::allocationSize() {
  return Header::kSize + Dictionary::kSize;
}

Dictionary* Dictionary::cast(Object* object) {
  DCHECK(object->isDictionary(), "invalid cast, expected dictionary");
  return reinterpret_cast<Dictionary*>(object);
}

word Dictionary::numItems() {
  return SmallInteger::cast(instanceVariableAt(kNumItemsOffset))->value();
}

void Dictionary::setNumItems(word numItems) {
  instanceVariableAtPut(kNumItemsOffset, SmallInteger::fromWord(numItems));
}

Object* Dictionary::data() {
  return instanceVariableAt(kDataOffset);
}

void Dictionary::setData(Object* data) {
  instanceVariableAtPut(kDataOffset, data);
}

// Function

Object* Function::annotations() {
  return instanceVariableAt(kAnnotationsOffset);
}

void Function::setAnnotations(Object* annotations) {
  return instanceVariableAtPut(kAnnotationsOffset, annotations);
}

Object* Function::closure() {
  return instanceVariableAt(kClosureOffset);
}

void Function::setClosure(Object* closure) {
  return instanceVariableAtPut(kClosureOffset, closure);
}

Object* Function::code() {
  return instanceVariableAt(kCodeOffset);
}

void Function::setCode(Object* code) {
  return instanceVariableAtPut(kCodeOffset, code);
}

Object* Function::defaults() {
  return instanceVariableAt(kDefaultsOffset);
}

void Function::setDefaults(Object* defaults) {
  return instanceVariableAtPut(kDefaultsOffset, defaults);
}

bool Function::hasDefaults() {
  return !defaults()->isNone();
}

Object* Function::doc() {
  return instanceVariableAt(kDocOffset);
}

void Function::setDoc(Object* doc) {
  instanceVariableAtPut(kDocOffset, doc);
}

Function::Entry Function::entry() {
  Object* object = instanceVariableAt(kEntryOffset);
  return SmallInteger::cast(object)->asFunctionPointer<Function::Entry>();
}

void Function::setEntry(Function::Entry entry) {
  auto object = SmallInteger::fromFunctionPointer(entry);
  instanceVariableAtPut(kEntryOffset, object);
}

Function::Entry Function::entryKw() {
  Object* object = instanceVariableAt(kEntryKwOffset);
  return SmallInteger::cast(object)->asFunctionPointer<Function::Entry>();
}

void Function::setEntryKw(Function::Entry entryKw) {
  auto object = SmallInteger::fromFunctionPointer(entryKw);
  instanceVariableAtPut(kEntryKwOffset, object);
}

Object* Function::globals() {
  return instanceVariableAt(kGlobalsOffset);
}

void Function::setGlobals(Object* globals) {
  return instanceVariableAtPut(kGlobalsOffset, globals);
}

Object* Function::kwDefaults() {
  return instanceVariableAt(kKwDefaultsOffset);
}

void Function::setKwDefaults(Object* kwDefaults) {
  return instanceVariableAtPut(kKwDefaultsOffset, kwDefaults);
}

Object* Function::module() {
  return instanceVariableAt(kModuleOffset);
}

void Function::setModule(Object* module) {
  return instanceVariableAtPut(kModuleOffset, module);
}

Object* Function::name() {
  return instanceVariableAt(kNameOffset);
}

void Function::setName(Object* name) {
  instanceVariableAtPut(kNameOffset, name);
}

Object* Function::qualname() {
  return instanceVariableAt(kQualnameOffset);
}

void Function::setQualname(Object* qualname) {
  instanceVariableAtPut(kQualnameOffset, qualname);
}

Object* Function::fastGlobals() {
  return instanceVariableAt(kFastGlobalsOffset);
}

void Function::setFastGlobals(Object* fast_globals) {
  return instanceVariableAtPut(kFastGlobalsOffset, fast_globals);
}

Function* Function::cast(Object* object) {
  DCHECK(object->isFunction(), "invalid cast, expected function");
  return reinterpret_cast<Function*>(object);
}

word Function::allocationSize() {
  return Header::kSize + Function::kSize;
}

// Instance

word Instance::allocationSize(word num_attr) {
  DCHECK(num_attr >= 0, "invalid number of attributes %ld", num_attr);
  word size = headerSize(num_attr) + num_attr * kPointerSize;
  return Utils::maximum(kMinimumSize, Utils::roundUp(size, kPointerSize));
}

Instance* Instance::cast(Object* object) {
  DCHECK(object->isInstance(), "invalid cast, expected instance");
  return reinterpret_cast<Instance*>(object);
}

// List

word List::allocationSize() {
  return Header::kSize + List::kSize;
}

List* List::cast(Object* object) {
  DCHECK(object->isList(), "invalid cast, expected list");
  return reinterpret_cast<List*>(object);
}

Object* List::items() {
  return instanceVariableAt(kItemsOffset);
}

void List::setItems(Object* new_items) {
  instanceVariableAtPut(kItemsOffset, new_items);
}

word List::capacity() {
  return ObjectArray::cast(items())->length();
}

word List::allocated() {
  return SmallInteger::cast(instanceVariableAt(kAllocatedOffset))->value();
}

void List::setAllocated(word new_allocated) {
  instanceVariableAtPut(
      kAllocatedOffset, SmallInteger::fromWord(new_allocated));
}

void List::atPut(word index, Object* value) {
  DCHECK_INDEX(index, allocated());
  Object* items = instanceVariableAt(kItemsOffset);
  ObjectArray::cast(items)->atPut(index, value);
}

Object* List::at(word index) {
  DCHECK_INDEX(index, allocated());
  return ObjectArray::cast(items())->at(index);
}

// Module

word Module::allocationSize() {
  return Header::kSize + Module::kSize;
}

Module* Module::cast(Object* object) {
  DCHECK(object->isModule(), "invalid cast, expected module");
  return reinterpret_cast<Module*>(object);
}

Object* Module::name() {
  return instanceVariableAt(kNameOffset);
}

void Module::setName(Object* name) {
  instanceVariableAtPut(kNameOffset, name);
}

Object* Module::dictionary() {
  return instanceVariableAt(kDictionaryOffset);
}

void Module::setDictionary(Object* dictionary) {
  instanceVariableAtPut(kDictionaryOffset, dictionary);
}

// NotImplemented

word NotImplemented::allocationSize() {
  return Header::kSize + NotImplemented::kSize;
}

NotImplemented* NotImplemented::cast(Object* object) {
  DCHECK(object->isNotImplemented(), "invalid cast, expected NotImplemented");
  return reinterpret_cast<NotImplemented*>(object);
}

// String

bool String::equalsCString(const char* c_string) {
  const char* cp = c_string;
  const word len = length();
  for (word i = 0; i < len; i++, cp++) {
    char ch = *cp;
    if (ch == '\0' || ch != charAt(i)) {
      return false;
    }
  }
  return *cp == '\0';
}

String* String::cast(Object* object) {
  DCHECK(
      object->isLargeString() || object->isSmallString(),
      "invalid cast, expected string");
  return reinterpret_cast<String*>(object);
}

byte String::charAt(word index) {
  if (isSmallString()) {
    return SmallString::cast(this)->charAt(index);
  }
  DCHECK(isLargeString(), "unexpected type");
  return LargeString::cast(this)->charAt(index);
}

word String::length() {
  if (isSmallString()) {
    return SmallString::cast(this)->length();
  }
  DCHECK(isLargeString(), "unexpected type");
  return LargeString::cast(this)->length();
}

word String::compare(Object* string) {
  String* that = String::cast(string);
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

bool String::equals(Object* that) {
  if (isSmallString()) {
    return this == that;
  }
  DCHECK(isLargeString(), "unexpected type");
  return LargeString::cast(this)->equals(that);
}

void String::copyTo(byte* dst, word length) {
  if (isSmallString()) {
    SmallString::cast(this)->copyTo(dst, length);
    return;
  }
  DCHECK(isLargeString(), "unexpected type");
  return LargeString::cast(this)->copyTo(dst, length);
}

char* String::toCString() {
  if (isSmallString()) {
    return SmallString::cast(this)->toCString();
  }
  DCHECK(isLargeString(), "unexpected type");
  return LargeString::cast(this)->toCString();
}

// LargeString

LargeString* LargeString::cast(Object* object) {
  DCHECK(object->isLargeString(), "unexpected type");
  return reinterpret_cast<LargeString*>(object);
}

word LargeString::allocationSize(word length) {
  DCHECK(length > SmallString::kMaxLength, "length %ld overflows", length);
  word size = headerSize(length) + length;
  return Utils::maximum(kMinimumSize, Utils::roundUp(size, kPointerSize));
}

byte LargeString::charAt(word index) {
  DCHECK_INDEX(index, length());
  return *reinterpret_cast<byte*>(address() + index);
}

// ValueCell

Object* ValueCell::value() {
  return instanceVariableAt(kValueOffset);
}

void ValueCell::setValue(Object* object) {
  instanceVariableAtPut(kValueOffset, object);
}

bool ValueCell::isUnbound() {
  return this == value();
}

void ValueCell::makeUnbound() {
  setValue(this);
}

ValueCell* ValueCell::cast(Object* object) {
  DCHECK(object->isValueCell(), "invalid cast");
  return reinterpret_cast<ValueCell*>(object);
}

word ValueCell::allocationSize() {
  return Header::kSize + ValueCell::kSize;
}

// Ellipsis

word Ellipsis::allocationSize() {
  return Header::kSize + Ellipsis::kSize;
}

Ellipsis* Ellipsis::cast(Object* object) {
  DCHECK(object->isEllipsis(), "invalid cast");
  return reinterpret_cast<Ellipsis*>(object);
}

// Set

word Set::allocationSize() {
  return Header::kSize + Set::kSize;
}

Set* Set::cast(Object* object) {
  DCHECK(object->isSet(), "invalid cast");
  return reinterpret_cast<Set*>(object);
}

word Set::numItems() {
  return SmallInteger::cast(instanceVariableAt(kNumItemsOffset))->value();
}

void Set::setNumItems(word numItems) {
  instanceVariableAtPut(kNumItemsOffset, SmallInteger::fromWord(numItems));
}

Object* Set::data() {
  return instanceVariableAt(kDataOffset);
}

void Set::setData(Object* data) {
  instanceVariableAtPut(kDataOffset, data);
}

// BoundMethod

word BoundMethod::allocationSize() {
  return Header::kSize + BoundMethod::kSize;
}

Object* BoundMethod::function() {
  return instanceVariableAt(kFunctionOffset);
}

void BoundMethod::setFunction(Object* function) {
  instanceVariableAtPut(kFunctionOffset, function);
}

Object* BoundMethod::self() {
  return instanceVariableAt(kSelfOffset);
}

void BoundMethod::setSelf(Object* self) {
  instanceVariableAtPut(kSelfOffset, self);
}

BoundMethod* BoundMethod::cast(Object* object) {
  DCHECK(object->isBoundMethod(), "invalid cast");
  return reinterpret_cast<BoundMethod*>(object);
}

// ClassMethod

Object* ClassMethod::function() {
  return instanceVariableAt(kFunctionOffset);
}

void ClassMethod::setFunction(Object* function) {
  instanceVariableAtPut(kFunctionOffset, function);
}

ClassMethod* ClassMethod::cast(Object* object) {
  DCHECK(object->isClassMethod(), "invalid cast");
  return reinterpret_cast<ClassMethod*>(object);
}

word ClassMethod::allocationSize() {
  return Header::kSize + ClassMethod::kSize;
}

// WeakRef

WeakRef* WeakRef::cast(Object* object) {
  DCHECK(object->isWeakRef(), "invalid cast");
  return reinterpret_cast<WeakRef*>(object);
}

word WeakRef::allocationSize() {
  return Header::kSize + WeakRef::kSize;
}

Object* WeakRef::referent() {
  return instanceVariableAt(kReferentOffset);
}

void WeakRef::setReferent(Object* referent) {
  instanceVariableAtPut(kReferentOffset, referent);
}

Object* WeakRef::callback() {
  return instanceVariableAt(kCallbackOffset);
}

void WeakRef::setCallback(Object* callable) {
  instanceVariableAtPut(kCallbackOffset, callable);
}

Object* WeakRef::link() {
  return instanceVariableAt(kLinkOffset);
}

void WeakRef::setLink(Object* reference) {
  instanceVariableAtPut(kLinkOffset, reference);
}

// Layout

word Layout::id() {
  return header()->hashCode();
}

void Layout::setDescribedClass(Object* klass) {
  instanceVariableAtPut(kDescribedClassOffset, klass);
}

Object* Layout::describedClass() {
  return instanceVariableAt(kDescribedClassOffset);
}

void Layout::setInObjectAttributes(Object* attributes) {
  instanceVariableAtPut(kInObjectAttributesOffset, attributes);
}

Object* Layout::inObjectAttributes() {
  return instanceVariableAt(kInObjectAttributesOffset);
}

void Layout::setOverflowAttributes(Object* attributes) {
  instanceVariableAtPut(kOverflowAttributesOffset, attributes);
}

word Layout::instanceSize() {
  return SmallInteger::cast(instanceVariableAt(kInstanceSizeOffset))->value();
}

void Layout::setInstanceSize(word size) {
  instanceVariableAtPut(kInstanceSizeOffset, SmallInteger::fromWord(size));
}

Object* Layout::overflowAttributes() {
  return instanceVariableAt(kOverflowAttributesOffset);
}

void Layout::setAdditions(Object* additions) {
  instanceVariableAtPut(kAdditionsOffset, additions);
}

Object* Layout::additions() {
  return instanceVariableAt(kAdditionsOffset);
}

void Layout::setDeletions(Object* deletions) {
  instanceVariableAtPut(kDeletionsOffset, deletions);
}

Object* Layout::deletions() {
  return instanceVariableAt(kDeletionsOffset);
}

word Layout::allocationSize() {
  return Header::kSize + Layout::kSize;
}

Layout* Layout::cast(Object* object) {
  DCHECK(object->isLayout(), "invalid cast");
  return reinterpret_cast<Layout*>(object);
}

word Layout::overflowOffset() {
  return SmallInteger::cast(instanceVariableAt(kOverflowOffsetOffset))->value();
}

void Layout::setOverflowOffset(word offset) {
  instanceVariableAtPut(kOverflowOffsetOffset, SmallInteger::fromWord(offset));
}

word Layout::delegateOffset() {
  return SmallInteger::cast(instanceVariableAt(kDelegateOffsetOffset))->value();
}

void Layout::setDelegateOffset(word offset) {
  instanceVariableAtPut(kDelegateOffsetOffset, SmallInteger::fromWord(offset));
}

bool Layout::hasDelegateSlot() {
  return !instanceVariableAt(kDelegateOffsetOffset)->isNone();
}

void Layout::addDelegateSlot() {
  if (hasDelegateSlot()) {
    return;
  }
  setDelegateOffset(overflowOffset() + kPointerSize);
  updateInstanceSize();
}

word Layout::numInObjectAttributes() {
  return SmallInteger::cast(instanceVariableAt(kNumInObjectAttributesOffset))
      ->value();
}

void Layout::setNumInObjectAttributes(word count) {
  instanceVariableAtPut(
      kNumInObjectAttributesOffset, SmallInteger::fromWord(count));
  setOverflowOffset(count * kPointerSize);
  if (hasDelegateSlot()) {
    setDelegateOffset(overflowOffset() + kPointerSize);
  }
  updateInstanceSize();
}

void Layout::updateInstanceSize() {
  word num_slots = numInObjectAttributes() + 1;
  if (hasDelegateSlot()) {
    num_slots += 1;
  }
  setInstanceSize(num_slots);
}

// Super

Object* Super::type() {
  return instanceVariableAt(kTypeOffset);
}

void Super::setType(Object* tp) {
  DCHECK(tp->isClass(), "expected type");
  instanceVariableAtPut(kTypeOffset, tp);
}

Object* Super::object() {
  return instanceVariableAt(kObjectOffset);
}

void Super::setObject(Object* obj) {
  instanceVariableAtPut(kObjectOffset, obj);
}

Object* Super::objectType() {
  return instanceVariableAt(kObjectTypeOffset);
}

void Super::setObjectType(Object* tp) {
  DCHECK(tp->isClass(), "expected type");
  instanceVariableAtPut(kObjectTypeOffset, tp);
}

Super* Super::cast(Object* object) {
  DCHECK(object->isSuper(), "invalid cast, expected super");
  return reinterpret_cast<Super*>(object);
}

word Super::allocationSize() {
  return Header::kSize + Super::kSize;
}

} // namespace python
