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
  V(Dictionary)                     \
  V(Ellipsis)                       \
  V(Function)                       \
  V(Integer)                        \
  V(LargeString)                    \
  V(List)                           \
  V(Module)                         \
  V(ObjectArray)                    \
  V(Range)                          \
  V(RangeIterator)                  \
  V(Type)                           \
  V(ValueCell)

#define INTRINSIC_CLASS_NAMES(V)     \
  INTRINSIC_IMMEDIATE_CLASS_NAMES(V) \
  INTRINSIC_HEAP_CLASS_NAMES(V)
// clang-format on

// NB: If you add something here make sure you add it to the appropriate macro
// above
enum ClassId {
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
  kCode,
  kDictionary,
  kEllipsis,
  kFunction,
  kInteger,
  kLargeString,
  kList,
  kModule,
  kObjectArray,
  kRange,
  kRangeIterator,
  kSet,
  kType,
  kValueCell,

  kLastId = kValueCell,
};

class Object {
 public:
  // Getters and setters.
  inline bool isObject();
  inline ClassId classId();

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
  inline bool isCode();
  inline bool isDictionary();
  inline bool isEllipsis();
  inline bool isFunction();
  inline bool isInstance();
  inline bool isList();
  inline bool isModule();
  inline bool isObjectArray();
  inline bool isLargeString();
  inline bool isRange();
  inline bool isRangeIterator();
  inline bool isSet();
  inline bool isValueCell();

  // superclass objects
  inline bool isString();

  static inline bool equals(Object* lhs, Object* rhs);

  // Casting.
  static inline Object* cast(Object* object);
  template <typename T>
  inline T* cast();

  // Constants

  // The bottom five bits of immediate objects are used as the class id when
  // indexing into the class table in the runtime.
  static const uword kImmediateClassTableIndexMask = (1 << 5) - 1;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Object);
};

// Immediate objects

class SmallInteger : public Object {
 public:
  // Getters and setters.
  inline word value();

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
 * Class       20   identifier for the class, allowing 2^20 unique classes
 * Hash        30   bits to use for an identity hash code
 * Count        8   number of array elements or instance variables
 */
class Header : public Object {
 public:
  inline word hashCode();
  inline Header* withHashCode(word value);

  inline ObjectFormat format();

  inline ClassId classId();

  inline word count();

  inline bool hasOverflow();

  static inline Header*
  from(word count, word hash, ClassId id, ObjectFormat format);

  // Casting.
  static inline Header* cast(Object* object);

  // Tags.
  static const int kTag = 3;
  static const int kTagSize = 3;
  static const uword kTagMask = (1 << kTagSize) - 1;

  static const int kFormatSize = 3;
  static const int kFormatOffset = 3;
  static const uword kFormatMask = (1 << kFormatSize) - 1;

  static const int kClassIdSize = 20;
  static const int kClassIdOffset = 6;
  static const uword kClassIdMask = (1 << kClassIdSize) - 1;

  static const int kHashCodeOffset = 26;
  static const int kHashCodeSize = 30;
  static const uword kHashCodeMask = (1 << kHashCodeSize) - 1U;

  static const int kCountOffset = 56;
  static const int kCountSize = 8;
  static const uword kCountMask = (1 << kCountSize) - 1;

  static const int kCountOverflowFlag = (1 << kCountSize) - 1;
  static const int kCountMax = kCountOverflowFlag - 1;

  static const int kSize = kPointerSize;

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
  setHeaderAndOverflow(word count, word hash, ClassId id, ObjectFormat format);
  inline word headerCountOrOverflow();
  inline word size();

  // Conversion.
  inline static HeapObject* fromAddress(uword address);

  // Casting.
  inline static HeapObject* cast(Object* object);

  // Sizing
  inline static word headerSize(word count);

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
};

template <typename T>
class Handle;

class Thread;

class Class : public HeapObject {
 public:
  // Getters and setters.
  inline ClassId id();
  inline Object* mro();
  inline void setMro(Object* object_array);
  inline Object* name();
  inline void setName(Object* name);
  inline Object* dictionary();
  inline void setDictionary(Object* name);
  // Number of attributes in instances of this class when allocated
  inline void setInstanceSize(word instance_size);
  inline word instanceSize();

  inline bool isIntrinsicOrExtension();

  // Casting.
  inline static Class* cast(Object* object);

  // Sizing.
  inline static word allocationSize();

  // Allocation.
  inline void initialize(Object* dictionary);

  // Layout.
  static const int kMroOffset = HeapObject::kSize;
  static const int kNameOffset = kMroOffset + kPointerSize;
  static const int kDictionaryOffset = kNameOffset + kPointerSize;
  static const int kInstanceSizeOffset = kDictionaryOffset + kPointerSize;
  static const int kGetAttrOffset = kInstanceSizeOffset + kPointerSize;
  static const int kSetAttrOffset = kGetAttrOffset + kPointerSize;
  static const int kDescriptorGetOffset = kSetAttrOffset + kPointerSize;
  static const int kDescriptorSetOffset = kDescriptorGetOffset + kPointerSize;
  static const int kSize = kDescriptorSetOffset + kPointerSize;

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

  // Allocation
  inline void initialize(word size, Object* value);

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
  inline void charAtPut(word index, byte value);

  // Equality checks.
  bool equals(Object* that);
  bool equalsCString(const char* c_string);

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

  // Allocation.
  inline void initialize(Object* empty_object_array);

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

  // Casting.
  inline static Function* cast(Object* object);

  // Sizing.
  inline static word allocationSize();

  // Allocation.
  inline void initialize();

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
  static const int kSize = kEntryKwOffset + kPointerSize;

 private:
  DISALLOW_COPY_AND_ASSIGN(Function);
};

class Instance : public HeapObject {
 public:
  inline Object* attributeAt(word index);
  inline void attributeAtPut(word index, Object* value);

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
  inline Object* dictionary();

  // Casting.
  inline static Module* cast(Object* object);

  // Allocation.
  inline static word allocationSize();

  // Allocation.
  inline void initialize(Object* name, Object* dict);

  // Layout.
  static const int kNameOffset = HeapObject::kSize;
  static const int kDictionaryOffset = kNameOffset + kPointerSize;
  static const int kSize = kDictionaryOffset + kPointerSize;

 private:
  DISALLOW_COPY_AND_ASSIGN(Module);
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

  // Allocation.
  inline void initialize(Object* items);

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

  // Allocation.
  inline void initialize(Object* items);

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

  // Allocation
  inline void initialize(Object* items);

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

  // Casting.
  static inline ValueCell* cast(Object* object);

  // Sizing.
  static inline word allocationSize();

  // Allocation.
  inline void initialize();

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

// Object

bool Object::isObject() {
  return true;
}

ClassId Object::classId() {
  if (isHeapObject()) {
    return HeapObject::cast(this)->header()->classId();
  }
  if (isSmallInteger()) {
    return ClassId::kSmallInteger;
  }
  return static_cast<ClassId>(
      reinterpret_cast<uword>(this) & kImmediateClassTableIndexMask);
}

bool Object::isClass() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->classId() == ClassId::kType;
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

bool Object::isBoundMethod() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->classId() == ClassId::kBoundMethod;
}

bool Object::isByteArray() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->classId() == ClassId::kByteArray;
}

bool Object::isObjectArray() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->classId() == ClassId::kObjectArray;
}

bool Object::isCode() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->classId() == ClassId::kCode;
}

bool Object::isLargeString() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->classId() == ClassId::kLargeString;
}

bool Object::isFunction() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->classId() == ClassId::kFunction;
}

bool Object::isInstance() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->classId() > ClassId::kLastId;
}

bool Object::isDictionary() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->classId() == ClassId::kDictionary;
}

bool Object::isSet() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->classId() == ClassId::kSet;
}

bool Object::isModule() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->classId() == ClassId::kModule;
}

bool Object::isList() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->classId() == ClassId::kList;
}

bool Object::isValueCell() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->classId() == ClassId::kValueCell;
}

bool Object::isEllipsis() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->classId() == ClassId::kEllipsis;
}

bool Object::isRange() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->classId() == ClassId::kRange;
}

bool Object::isRangeIterator() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->classId() == ClassId::kRangeIterator;
}

bool Object::isString() {
  return isSmallString() || isLargeString();
}

bool Object::equals(Object* lhs, Object* rhs) {
  return (lhs == rhs) ||
      (lhs->isLargeString() && LargeString::cast(lhs)->equals(rhs));
}

Object* Object::cast(Object* object) {
  return object;
}

template <typename T>
T* Object::cast() {
  return T::cast(this);
}

// SmallInteger

SmallInteger* SmallInteger::cast(Object* object) {
  assert(object->isSmallInteger());
  return reinterpret_cast<SmallInteger*>(object);
}

word SmallInteger::value() {
  return reinterpret_cast<word>(this) >> kTagSize;
}

SmallInteger* SmallInteger::fromWord(word value) {
  assert(SmallInteger::isValid(value));
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
  assert(0 <= index);
  assert(index < length());
  return reinterpret_cast<word>(this) >> (kBitsPerByte * (index + 1));
}

void SmallString::copyTo(byte* dst, word length) {
  assert(length >= 0);
  assert(length <= this->length());
  for (word i = 0; i < length; ++i) {
    *dst++ = charAt(i);
  }
}

SmallString* SmallString::cast(Object* object) {
  assert(object->isSmallString());
  return reinterpret_cast<SmallString*>(object);
}

// Header

Header* Header::cast(Object* object) {
  assert(object->isHeader());
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

ClassId Header::classId() {
  auto header = reinterpret_cast<uword>(this);
  return static_cast<ClassId>((header >> kClassIdOffset) & kClassIdMask);
}

ObjectFormat Header::format() {
  auto header = reinterpret_cast<uword>(this);
  return static_cast<ObjectFormat>((header >> kFormatOffset) & kFormatMask);
}

Header* Header::from(word count, word hash, ClassId id, ObjectFormat format) {
  assert(count >= 0);
  assert(count <= kCountMax || count == kCountOverflowFlag);
  uword result = Header::kTag;
  result |= ((count > kCountMax) ? kCountOverflowFlag : count) << kCountOffset;
  result |= hash << kHashCodeOffset;
  result |= static_cast<uword>(id) << kClassIdOffset;
  result |= static_cast<uword>(format) << kFormatOffset;
  return reinterpret_cast<Header*>(result);
}

// None

None* None::object() {
  return reinterpret_cast<None*>(kTag);
}

None* None::cast(Object* object) {
  assert(object->isNone());
  return reinterpret_cast<None*>(object);
}

// Error

Error* Error::object() {
  return reinterpret_cast<Error*>(kTag);
}

Error* Error::cast(Object* object) {
  assert(object->isError());
  return reinterpret_cast<Error*>(object);
}

// Boolean

Boolean* Boolean::fromBool(bool value) {
  return reinterpret_cast<Boolean*>(
      (static_cast<uword>(value) << kTagSize) | kTag);
}

Boolean* Boolean::cast(Object* object) {
  assert(object->isBoolean());
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
  assert(header()->hasOverflow());
  return SmallInteger::cast(instanceVariableAt(kHeaderOverflowOffset))->value();
}

void HeapObject::setHeaderAndOverflow(
    word count,
    word hash,
    ClassId id,
    ObjectFormat format) {
  if (count > Header::kCountMax) {
    instanceVariableAtPut(kHeaderOverflowOffset, SmallInteger::fromWord(count));
    count = Header::kCountOverflowFlag;
  }
  setHeader(Header::from(count, hash, id, format));
}

HeapObject* HeapObject::fromAddress(uword address) {
  assert((address & kTagMask) == 0);
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
  assert(object->isHeapObject());
  return reinterpret_cast<HeapObject*>(object);
}

word HeapObject::headerSize(word count) {
  word result = kPointerSize;
  if (count > Header::kCountMax) {
    result += kPointerSize;
  }
  return result;
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

void Class::initialize(Object* dictionary) {
  setInstanceSize(0);
  setDictionary(dictionary);
}

ClassId Class::id() {
  return static_cast<ClassId>(header()->hashCode());
}

Object* Class::mro() {
  return instanceVariableAt(kMroOffset);
}

void Class::setMro(Object* object_array) {
  instanceVariableAtPut(kMroOffset, object_array);
}

Object* Class::name() {
  return instanceVariableAt(kNameOffset);
}

void Class::setName(Object* name) {
  instanceVariableAtPut(kNameOffset, name);
}

Object* Class::dictionary() {
  return instanceVariableAt(kDictionaryOffset);
}

void Class::setDictionary(Object* dictionary) {
  instanceVariableAtPut(kDictionaryOffset, dictionary);
}

void Class::setInstanceSize(word instance_size) {
  instanceVariableAtPut(
      kInstanceSizeOffset, SmallInteger::fromWord(instance_size));
}

word Class::instanceSize() {
  return SmallInteger::cast(instanceVariableAt(kInstanceSizeOffset))->value();
}

Class* Class::cast(Object* object) {
  assert(object->isClass());
  return reinterpret_cast<Class*>(object);
}

bool Class::isIntrinsicOrExtension() {
  return id() <= ClassId::kLastId;
}

// Array

word Array::length() {
  assert(isByteArray() || isObjectArray() || isLargeString());
  return headerCountOrOverflow();
}

// ByteArray

word ByteArray::allocationSize(word length) {
  assert(length >= 0);
  word size = headerSize(length) + length;
  return Utils::maximum(kMinimumSize, Utils::roundUp(size, kPointerSize));
}

byte ByteArray::byteAt(word index) {
  assert(index >= 0);
  assert(index < length());
  return *reinterpret_cast<byte*>(address() + index);
}

void ByteArray::byteAtPut(word index, byte value) {
  assert(index >= 0);
  assert(index < length());
  *reinterpret_cast<byte*>(address() + index) = value;
}

ByteArray* ByteArray::cast(Object* object) {
  assert(object->isByteArray());
  return reinterpret_cast<ByteArray*>(object);
}

// ObjectArray

word ObjectArray::allocationSize(word length) {
  assert(length >= 0);
  word size = headerSize(length) + length * kPointerSize;
  return Utils::maximum(kMinimumSize, Utils::roundUp(size, kPointerSize));
}

void ObjectArray::initialize(word size, Object* value) {
  assert(size >= 0);
  for (word offset = 0; offset < size; offset += kPointerSize) {
    instanceVariableAtPut(offset, value);
  }
}

ObjectArray* ObjectArray::cast(Object* object) {
  assert(object->isObjectArray());
  return reinterpret_cast<ObjectArray*>(object);
}

Object* ObjectArray::at(word index) {
  assert(index >= 0);
  assert(index < length());
  return instanceVariableAt(index * kPointerSize);
}

void ObjectArray::atPut(word index, Object* value) {
  assert(index >= 0);
  assert(index < length());
  instanceVariableAtPut(index * kPointerSize, value);
}

void ObjectArray::copyTo(Object* array) {
  word len = length();
  ObjectArray* dst = ObjectArray::cast(array);
  assert(len <= dst->length());
  for (int i = 0; i < len; i++) {
    Object* elem = at(i);
    dst->atPut(i, elem);
  }
}

// Code

Code* Code::cast(Object* obj) {
  assert(obj->isCode());
  return reinterpret_cast<Code*>(obj);
}

word Code::allocationSize() {
  return Header::kSize + Code::kSize;
}

// TODO: initialize instance variables with the None object.
void Code::initialize(Object* empty_object_array) {
  instanceVariableAtPut(kArgcountOffset, SmallInteger::fromWord(0));
  instanceVariableAtPut(kKwonlyargcountOffset, SmallInteger::fromWord(0));
  instanceVariableAtPut(kNlocalsOffset, SmallInteger::fromWord(0));
  instanceVariableAtPut(kStacksizeOffset, SmallInteger::fromWord(0));
  instanceVariableAtPut(kFlagsOffset, SmallInteger::fromWord(0));
  instanceVariableAtPut(kCodeOffset, None::object());
  instanceVariableAtPut(kConstsOffset, None::object());
  instanceVariableAtPut(kNamesOffset, None::object());
  instanceVariableAtPut(kVarnamesOffset, None::object());
  instanceVariableAtPut(kFreevarsOffset, empty_object_array);
  instanceVariableAtPut(kCellvarsOffset, empty_object_array);
  instanceVariableAtPut(kFilenameOffset, None::object());
  instanceVariableAtPut(kNameOffset, None::object());
  instanceVariableAtPut(kFirstlinenoOffset, SmallInteger::fromWord(0));
  instanceVariableAtPut(kLnotabOffset, None::object());
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
  assert(object->isNone() || object->isObjectArray());
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
  assert(object->isNone() || object->isObjectArray());
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
  assert(object->isRange());
  return reinterpret_cast<Range*>(object);
}

// RangeIterator

void RangeIterator::setRange(Object* range) {
  auto r = Range::cast(range);
  instanceVariableAtPut(kRangeOffset, r);
  instanceVariableAtPut(kCurValueOffset, SmallInteger::fromWord(r->start()));
}

bool RangeIterator::isOutOfRange(word cur, word stop, word step) {
  assert(step != 0); // should have been checked in builtinRange().

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
  assert(object->isRangeIterator());
  return reinterpret_cast<RangeIterator*>(object);
}

// Dictionary

word Dictionary::allocationSize() {
  return Header::kSize + Dictionary::kSize;
}

void Dictionary::initialize(Object* data) {
  instanceVariableAtPut(kNumItemsOffset, SmallInteger::fromWord(0));
  instanceVariableAtPut(kDataOffset, data);
}

Dictionary* Dictionary::cast(Object* object) {
  assert(object->isDictionary());
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

Function* Function::cast(Object* object) {
  assert(object->isFunction());
  return reinterpret_cast<Function*>(object);
}

word Function::allocationSize() {
  return Header::kSize + Function::kSize;
}

void Function::initialize() {
  for (word offset = 0; offset < Function::kSize; offset += kPointerSize) {
    instanceVariableAtPut(offset, None::object());
  }
}

// Instance

Object* Instance::attributeAt(word offset) {
  return instanceVariableAt(offset);
}

void Instance::attributeAtPut(word offset, Object* value) {
  return instanceVariableAtPut(offset, value);
}

word Instance::allocationSize(word num_attr) {
  assert(num_attr >= 0);
  word size = headerSize(num_attr) + num_attr * kPointerSize;
  return Utils::maximum(kMinimumSize, Utils::roundUp(size, kPointerSize));
}

Instance* Instance::cast(Object* object) {
  assert(object->isInstance());
  return reinterpret_cast<Instance*>(object);
}

// List

word List::allocationSize() {
  return Header::kSize + List::kSize;
}

List* List::cast(Object* object) {
  assert(object->isList());
  return reinterpret_cast<List*>(object);
}

void List::initialize(Object* items) {
  instanceVariableAtPut(kAllocatedOffset, SmallInteger::fromWord(0));
  instanceVariableAtPut(kItemsOffset, items);
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
  assert(index >= 0);
  assert(index < allocated());
  Object* items = instanceVariableAt(kItemsOffset);
  ObjectArray::cast(items)->atPut(index, value);
}

Object* List::at(word index) {
  assert(index >= 0);
  assert(index < allocated());
  return ObjectArray::cast(items())->at(index);
}

// Module

word Module::allocationSize() {
  return Header::kSize + Module::kSize;
}

Module* Module::cast(Object* object) {
  assert(object->isModule());
  return reinterpret_cast<Module*>(object);
}

void Module::initialize(Object* name, Object* dict) {
  instanceVariableAtPut(kNameOffset, name);
  instanceVariableAtPut(kDictionaryOffset, dict);
}

Object* Module::name() {
  return instanceVariableAt(kNameOffset);
}

Object* Module::dictionary() {
  return instanceVariableAt(kDictionaryOffset);
}

// String

String* String::cast(Object* object) {
  assert(object->isLargeString() || object->isSmallString());
  return reinterpret_cast<String*>(object);
}

byte String::charAt(word index) {
  if (isSmallString()) {
    return SmallString::cast(this)->charAt(index);
  }
  assert(isLargeString());
  return LargeString::cast(this)->charAt(index);
}

word String::length() {
  if (isSmallString()) {
    return SmallString::cast(this)->length();
  }
  assert(isLargeString());
  return LargeString::cast(this)->length();
}

bool String::equals(Object* that) {
  if (isSmallString()) {
    return this == that;
  }
  assert(isLargeString());
  return LargeString::cast(this)->equals(that);
}

bool String::equalsCString(const char* c_string) {
  if (isSmallString()) {
    return this == SmallString::fromCString(c_string);
  }
  assert(isLargeString());
  return LargeString::cast(this)->equalsCString(c_string);
}

void String::copyTo(byte* dst, word length) {
  if (isSmallString()) {
    SmallString::cast(this)->copyTo(dst, length);
    return;
  }
  assert(isLargeString());
  return LargeString::cast(this)->copyTo(dst, length);
}

char* String::toCString() {
  if (isSmallString()) {
    return SmallString::cast(this)->toCString();
  }
  assert(isLargeString());
  return LargeString::cast(this)->toCString();
}

// LargeString

LargeString* LargeString::cast(Object* object) {
  assert(object->isLargeString());
  return reinterpret_cast<LargeString*>(object);
}

word LargeString::allocationSize(word length) {
  assert(length > SmallString::kMaxLength);
  word size = headerSize(length) + length;
  return Utils::maximum(kMinimumSize, Utils::roundUp(size, kPointerSize));
}

byte LargeString::charAt(word index) {
  assert(index >= 0);
  assert(index < length());
  return *reinterpret_cast<byte*>(address() + index);
}

// ValueCell

Object* ValueCell::value() {
  return instanceVariableAt(kValueOffset);
}

void ValueCell::setValue(Object* object) {
  instanceVariableAtPut(kValueOffset, object);
}

ValueCell* ValueCell::cast(Object* object) {
  assert(object->isValueCell());
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
  assert(object->isEllipsis());
  return reinterpret_cast<Ellipsis*>(object);
}

// Set

word Set::allocationSize() {
  return Header::kSize + Set::kSize;
}

void Set::initialize(Object* data) {
  instanceVariableAtPut(kNumItemsOffset, SmallInteger::fromWord(0));
  instanceVariableAtPut(kDataOffset, data);
}

Set* Set::cast(Object* object) {
  assert(object->isSet());
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
  assert(object->isBoundMethod());
  return reinterpret_cast<BoundMethod*>(object);
}

} // namespace python
