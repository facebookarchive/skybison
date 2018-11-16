#pragma once

#include "globals.h"
#include "handles.h"
#include "utils.h"

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

enum class ClassId {
  kObject = 1,

  // Immediate
  kSmallInteger,
  kBoolean,
  kNone,
  kEllipsis,

  // Heap object
  kClass,
  kByteArray,
  kObjectArray,
  kString,
  kCode,
  kFunction,
  kDictionary,

  // Composite
  kModule,
  kList,
};

class Object {
 public:
  // Getters and setters.

  inline bool isObject();

  // Immediate
  inline bool isSmallInteger();
  inline bool isHeader();
  inline bool isNone();
  inline bool isEllipsis();
  inline bool isBoolean();

  // Indirect
  inline bool isHeapObject();
  inline bool isClass();
  inline bool isCode();
  inline bool isByteArray();
  inline bool isObjectArray();
  inline bool isString();
  inline bool isFunction();
  inline bool isModule();
  inline bool isDictionary();
  inline bool isList();

  static inline bool equals(Object* lhs, Object* rhs);

  // Casting.
  static inline Object* cast(Object* object);
  template <typename T>
  inline T* cast();

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

  // Casting.
  static inline SmallInteger* cast(Object* object);

  // Tags.
  static const int kTag = 0;
  static const int kTagSize = 1;
  static const uword kTagMask = (1 << kTagSize) - 1;

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

class Ellipsis : public Object {
 public:
  // Singletons.
  inline static Ellipsis* object();

  // Casting.
  static inline Ellipsis* cast(Object* object);

  // Tagging.
  static const int kTag = 23; // 0b10111
  static const int kTagSize = 5;
  static const uword kTagMask = (1 << kTagSize) - 1;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Ellipsis);
};

// Heap objects

class HeapObject : public Object {
 public:
  // Getters and setters.
  inline uword address();
  inline Header* header();
  inline word size();
  inline void setHeader(Header* header);

  // Conversion.
  inline static HeapObject* fromAddress(uword address);

  // Casting.
  inline static HeapObject* cast(Object* object);

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
  static const int kHeaderOffset = 0;
  static const int kSize = kHeaderOffset + kPointerSize;

 protected:
  inline Object* instanceVariableAt(word offset);
  inline void instanceVariableAtPut(word offset, Object* value);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(HeapObject);
};

class Class : public HeapObject {
 public:
  // Getters and setters.
  inline Object* superClass();

  // Casting.
  inline static Class* cast(Object* object);

  // Sizing.
  inline static int allocationSize();
  inline static int bodySize();

  // Allocation.
  inline void initialize(Object* super_class);

  // Layout.
  static const int kSuperClassOffset = HeapObject::kSize;
  static const int kSize = kSuperClassOffset + kPointerSize;

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

  // Allocation.
  inline void initialize(word length);

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

class String : public Array {
 public:
  // Getters and setters.
  inline byte charAt(word index);
  inline void charAtPut(word index, byte value);

  // Equality checks.
  bool equals(Object* that);
  bool equalsCString(const char* c_string);

  // Casting.
  static inline String* cast(Object* object);

  // Sizing.
  inline static word allocationSize(word length);

  // Allocation.
  inline void initialize(word length);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(String);
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
  inline static word bodySize();

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

  // Returns a pointer to a trampoline that is responsible for executing the
  // function when it is invoked via CALL_FUNCTION
  inline Object* entry();
  inline void setEntry(Object* entry);

  // Returns a pointer to a trampoline that is responsible for executing the
  // function when it is invoked via CALL_FUNCTION_KW
  inline Object* entryKw();
  inline void setEntryKw(Object* entryKw);

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
  inline static word bodySize();

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
  static const int kSize = kEntryKwOffset;

 private:
  DISALLOW_COPY_AND_ASSIGN(Function);
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
  inline static word bodySize();

  // Allocation.
  inline void initialize(Object* name, Object* dict);

  // Layout.
  static const int kNameOffset = HeapObject::kSize;
  static const int kDictionaryOffset = kNameOffset + kPointerSize;
  static const int kSize = kDictionaryOffset + kPointerSize;

 private:
  DISALLOW_COPY_AND_ASSIGN(Module);
};

class Runtime;

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
 * Dictionary entries are stored as a triple of (hash, key, value).
 * Empty buckets are stored as (None, None, None).
 * Tombstone buckets are stored as (None, <not None>, <Any>).
 *
 */
class Dictionary : public HeapObject {
 public:
  // Getters and setters.
  inline static Dictionary* cast(Object* object);

  // Number of items currently in the dictionary
  inline word numItems();

  // Total number of items that could be stored in the ObjectArray backing the
  // dictionary
  inline word capacity();

  // Look up the value associated with key.
  //
  // Returns true if the key was found and sets the associated value in value.
  // Returns false otherwise.
  static bool at(Object* dict, Object* key, Object* hash, Object** value);

  // Associate a value with the supplied key.
  //
  // This handles growing the backing ObjectArray if needed.
  static void atPut(
      Object* dict,
      Object* key,
      Object* hash,
      Object* value,
      Runtime* runtime);

  // Delete a key from the dictionary.
  //
  // Returns true if the key existed and sets the previous value in value.
  // Returns false otherwise.
  static bool remove(Object* dict, Object* key, Object* hash, Object** value);

  // Sizing.
  inline static word allocationSize();
  inline static word bodySize();

  // Allocation.
  inline void initialize(Object* items);

  // Bucket layout.
  static const int kBucketHashOffset = 0;
  static const int kBucketKeyOffset = kBucketHashOffset + 1;
  static const int kBucketValueOffset = kBucketKeyOffset + 1;
  static const int kPointersPerBucket = kBucketValueOffset + 1;

  // Initial size of the dictionary. According to comments in CPython's
  // dictobject.c this accomodates the majority of dictionaries without needing
  // a resize (obviously this depends on the load factor used to resize the
  // dict).
  static const int kInitialItemsSize = 8 * kPointersPerBucket;

  // Layout.
  static const int kNumItemsOffset = HeapObject::kSize;
  static const int kItemsOffset = kNumItemsOffset + kPointerSize;
  static const int kSize = kItemsOffset + kPointerSize;

 private:
  inline Object* items();
  inline void setItems(Object* items);
  inline void setNumItems(word numItems);

  // Looks up the supplied key in the dictionary.
  //
  // If the key is found, this function returns true and sets bucket to the
  // index of the bucket that contains the value. If the key is not found, this
  // function returns false and sets bucket to the location where the key would
  // be inserted. If the dictionary is full, it sets bucket to -1.
  static bool lookup(Dictionary* dict, Object* key, Object* hash, int* bucket);
  static void grow(Dictionary* dict, Runtime* runtime);

  DISALLOW_COPY_AND_ASSIGN(Dictionary);
};

class Runtime;

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
  static inline word bodySize();

  // Allocation
  inline void initialize(Object* items);

  // Layout.
  static const int kItemsOffset = HeapObject::kSize;
  static const int kAllocatedOffset = kItemsOffset + kPointerSize;
  static const int kSize = kAllocatedOffset + kPointerSize;

 private:
  DISALLOW_COPY_AND_ASSIGN(List);
};

// Object

bool Object::isObject() {
  return true;
}

bool Object::isClass() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->classId() == ClassId::kClass;
}

bool Object::isSmallInteger() {
  uword tag = reinterpret_cast<uword>(this) & SmallInteger::kTagMask;
  return tag == SmallInteger::kTag;
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

bool Object::isEllipsis() {
  int tag = reinterpret_cast<uword>(this) & None::kTagMask;
  return tag == Ellipsis::kTag;
}

bool Object::isHeapObject() {
  uword tag = reinterpret_cast<uword>(this) & HeapObject::kTagMask;
  return tag == HeapObject::kTag;
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

bool Object::isString() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->classId() == ClassId::kString;
}

bool Object::isFunction() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->classId() == ClassId::kFunction;
}

bool Object::isDictionary() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->header()->classId() == ClassId::kDictionary;
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

bool Object::equals(Object* lhs, Object* rhs) {
  return (lhs == rhs) || (lhs->isString() && String::cast(lhs)->equals(rhs));
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
  return reinterpret_cast<SmallInteger*>(value << kTagSize);
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
  uword result = Header::kTag;
  result |= ((count >= kCountMask) ? kCountMask : count) << kCountOffset;
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

// Ellipsis

Ellipsis* Ellipsis::object() {
  return reinterpret_cast<Ellipsis*>(kTag);
}

Ellipsis* Ellipsis::cast(Object* object) {
  assert(object->isEllipsis());
  return reinterpret_cast<Ellipsis*>(object);
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

Header* HeapObject::header() {
  return Header::cast(instanceVariableAt(kHeaderOffset));
}

HeapObject* HeapObject::fromAddress(uword address) {
  assert((address & kTagMask) == 0);
  return reinterpret_cast<HeapObject*>(address + kTag);
}

word HeapObject::size() {
  word result = header()->count();
  switch (header()->format()) {
    case ObjectFormat::kDataArray8:
      break;
    case ObjectFormat::kDataArray16:
      result <<= 1;
      break;
    case ObjectFormat::kDataArray32:
      result <<= 2;
      break;
    case ObjectFormat::kDataArray64:
      result <<= 3;
      break;
    case ObjectFormat::kDataArray128:
      result <<= 4;
      break;
    case ObjectFormat::kObjectArray:
      result *= kPointerSize;
      break;
    case ObjectFormat::kDataInstance:
    case ObjectFormat::kObjectInstance:
      result *= kPointerSize;
      break;
  }
  return Utils::roundUp(result + HeapObject::kSize, kPointerSize * 2);
}

void HeapObject::setHeader(Header* header) {
  instanceVariableAtPut(kHeaderOffset, header);
}

HeapObject* HeapObject::cast(Object* object) {
  assert(object->isHeapObject());
  return reinterpret_cast<HeapObject*>(object);
}

bool HeapObject::isRoot() {
  return header()->format() == ObjectFormat::kObjectArray ||
      header()->format() == ObjectFormat::kObjectInstance;
}

bool HeapObject::isForwarding() {
  return *reinterpret_cast<uword*>(address()) == kIsForwarded;
}

Object* HeapObject::forward() {
  // When a heap object is forwarding, its second word is the forwarding
  // address.
  return *reinterpret_cast<Object**>(address() + kPointerSize);
}

void HeapObject::forwardTo(Object* object) {
  // Overwrite the header with the forwarding marker.
  *reinterpret_cast<uword*>(address()) = kIsForwarded;
  // Overwrite the second word with the forwarding addressing.
  instanceVariableAtPut(kPointerSize, object);
}

Object* HeapObject::instanceVariableAt(word offset) {
  return *reinterpret_cast<Object**>(address() + offset);
}

void HeapObject::instanceVariableAtPut(word offset, Object* value) {
  *reinterpret_cast<Object**>(address() + offset) = value;
}

// Class

int Class::allocationSize() {
  return Class::kSize;
}

int Class::bodySize() {
  return Class::kSize - HeapObject::kSize;
}

void Class::initialize(Object* super_class) {
  instanceVariableAtPut(kSuperClassOffset, super_class);
}

Class* Class::cast(Object* object) {
  assert(object->isClass());
  return reinterpret_cast<Class*>(object);
}

// Array

word Array::length() {
  assert(isByteArray() || isObjectArray() || isString());
  return header()->count();
}

// ByteArray

word ByteArray::allocationSize(word length) {
  assert(length >= 0);
  return Utils::roundUp(length + ByteArray::kSize, kPointerSize);
}

void ByteArray::initialize(word length) {
  assert(length >= 0);
  memset(reinterpret_cast<void*>(address() + ByteArray::kSize), 0, length);
}

byte ByteArray::byteAt(word index) {
  assert(index >= 0);
  assert(index < length());
  return *reinterpret_cast<byte*>(address() + ByteArray::kSize + index);
}

void ByteArray::byteAtPut(word index, byte value) {
  assert(index >= 0);
  assert(index < length());
  *reinterpret_cast<byte*>(address() + ByteArray::kSize + index) = value;
}

ByteArray* ByteArray::cast(Object* object) {
  assert(object->isByteArray());
  return reinterpret_cast<ByteArray*>(object);
}

// ObjectArray

word ObjectArray::allocationSize(word length) {
  assert(length >= 0);
  return Utils::roundUp(
      length * kPointerSize + ObjectArray::kSize, kPointerSize);
}

void ObjectArray::initialize(word size, Object* value) {
  assert(size >= 0);
  for (word offset = ObjectArray::kSize; offset < size;
       offset += kPointerSize) {
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
  return instanceVariableAt(ObjectArray::kSize + (index * kPointerSize));
}

void ObjectArray::atPut(word index, Object* value) {
  assert(index >= 0);
  assert(index < length());
  instanceVariableAtPut(ObjectArray::kSize + (index * kPointerSize), value);
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
  return Utils::roundUp(kSize, kPointerSize);
}

word Code::bodySize() {
  return Code::kSize - HeapObject::kSize;
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

// Dictionary

word Dictionary::allocationSize() {
  return Utils::roundUp(Module::kSize, kPointerSize);
}

word Dictionary::bodySize() {
  return Dictionary::kSize - HeapObject::kSize;
}

void Dictionary::initialize(Object* items) {
  instanceVariableAtPut(kNumItemsOffset, SmallInteger::fromWord(0));
  instanceVariableAtPut(kItemsOffset, items);
}

Dictionary* Dictionary::cast(Object* object) {
  assert(object->isDictionary());
  return reinterpret_cast<Dictionary*>(object);
}

word Dictionary::numItems() {
  return SmallInteger::cast(instanceVariableAt(kNumItemsOffset))->value();
};

word Dictionary::capacity() {
  return ObjectArray::cast(items())->length() / kPointersPerBucket;
}

void Dictionary::setNumItems(word numItems) {
  instanceVariableAtPut(kNumItemsOffset, SmallInteger::fromWord(numItems));
}

Object* Dictionary::items() {
  return instanceVariableAt(kItemsOffset);
}

void Dictionary::setItems(Object* items) {
  assert((ObjectArray::cast(items)->length() % kPointersPerBucket) == 0);
  assert(Utils::isPowerOfTwo(
      ObjectArray::cast(items)->length() / kPointersPerBucket));
  instanceVariableAtPut(kItemsOffset, items);
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

Object* Function::entry() {
  return instanceVariableAt(kEntryOffset);
}

void Function::setEntry(Object* entry) {
  return instanceVariableAtPut(kEntryOffset, entry);
}

Object* Function::entryKw() {
  return instanceVariableAt(kEntryKwOffset);
}

void Function::setEntryKw(Object* entryKw) {
  return instanceVariableAtPut(kEntryKwOffset, entryKw);
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
  return Utils::roundUp(Function::kSize, kPointerSize);
}

void Function::initialize() {
  for (word offset = HeapObject::kSize; offset < Function::kSize;
       offset += kPointerSize) {
    instanceVariableAtPut(offset, None::object());
  }
}

word Function::bodySize() {
  return Function::kSize - HeapObject::kSize;
}

// List

word List::allocationSize() {
  return Utils::roundUp(List::kSize, kPointerSize);
}

word List::bodySize() {
  return List::kSize - HeapObject::kSize;
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
  return ObjectArray::cast(items())->at(index);
}

// Module

word Module::allocationSize() {
  return Utils::roundUp(Module::kSize, kPointerSize);
}

word Module::bodySize() {
  return Module::kSize - HeapObject::kSize;
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
  assert(object->isString());
  return reinterpret_cast<String*>(object);
}

void String::initialize(word length) {
  assert(length >= 0);
  std::memset(reinterpret_cast<void*>(address() + String::kSize), 0, length);
}

word String::allocationSize(word length) {
  assert(length >= 0);
  return Utils::roundUp(String::kSize + length, kPointerSize);
}

byte String::charAt(word index) {
  assert(index >= 0);
  assert(index < length());
  return *reinterpret_cast<byte*>(address() + String::kSize + index);
}

void String::charAtPut(word index, byte value) {
  assert(index >= 0);
  assert(index < length());
  *reinterpret_cast<byte*>(address() + String::kSize + index) = value;
}

} // namespace python
