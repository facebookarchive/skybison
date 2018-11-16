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
  static inline Object* hash(Object* object);

  // Casting.
  static inline Object* cast(Object* object);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Object);
};

// Immediate objects

class SmallInteger : public Object {
 public:
  // Getters and setters.
  inline word value();
  inline Object* hash();

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
  inline Object* hashCode();

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
  inline Object* hash();

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
  // Getters and setters.
  inline Object* hash();

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
  // Getters and setters.
  inline Object* hash();

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
  Object* hash();

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
  inline int cell2arg();
  inline Object* cellvars();
  inline Object* code();
  inline Object* consts();
  inline Object* filename();
  inline int firstlineno();
  inline int flags();
  inline Object* freevars();
  inline int kwonlyargcount();
  inline Object* lnotab();
  inline Object* name();
  inline Object* names();
  inline int nlocals();
  inline int stacksize();
  inline Object* varnames();

  // Casting.
  inline static Code* cast(Object* obj);

  // Sizing.
  inline static word allocationSize();
  inline static word bodySize();

  // Does this really need to take so many arguments?
  inline void initialize(
      int argcount,
      int kwonlyargcount,
      int nlocals,
      int stacksize,
      int flags,
      Object* code,
      Object* consts,
      Object* names,
      Object* varnames,
      Object* freevars,
      Object* cellvars,
      Object* filename,
      Object* name,
      int firstlineno,
      Object* lnotab);

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
  static const int kZombieframeOffset = kLnotabOffset + kPointerSize;
  static const int kWeakreflistOffset = kZombieframeOffset + kPointerSize;
  static const int kExtraOffset = kWeakreflistOffset + kPointerSize;
  static const int kSize = kExtraOffset + kPointerSize;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Code);
};

class Function : public HeapObject {
 public:
  // Getters and setters.
  inline Object* code();

  // Casting.
  inline static Function* cast(Object* object);

  // Sizing.
  inline static word allocationSize();
  inline static word bodySize();

  // Allocation.
  inline void initialize();

  // Layout.
  static const int kCodeOffset = HeapObject::kSize + kPointerSize;
  static const int kSize = kCodeOffset + kPointerSize;

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

  static void appendAndGrow(
      const Handle<List>& list,
      const Handle<Object>& value,
      Runtime* runtime);

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

Object* Object::hash(Object* object) {
  // Until we have proper support for classes and methods so we can dispatch on
  // __hash__, this is the "dumping ground" for hash related functionality.
  //
  // TODO: Need to support hashing of arbitrary objects using their identity
  // hash code.
  if (object->isString()) {
    return String::cast(object)->hash();
  } else if (object->isSmallInteger()) {
    return SmallInteger::cast(object)->hash();
  } else if (object->isBoolean()) {
    return Boolean::cast(object)->hash();
  } else if (object->isNone()) {
    return None::cast(object)->hash();
  } else if (object->isEllipsis()) {
    return Ellipsis::cast(object)->hash();
  }
  assert(0);
  return None::object();
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

Object* SmallInteger::hash() {
  return this;
}

// Header

Header* Header::cast(Object* object) {
  assert(object->isHeader());
  return reinterpret_cast<Header*>(object);
}

word Header::count() {
  uword header = reinterpret_cast<uword>(this);
  return (header >> kCountOffset) & kCountMask;
}

Object* Header::hashCode() {
  uword header = reinterpret_cast<uword>(this);
  return SmallInteger::fromWord((header >> kHashCodeOffset) & kHashCodeMask);
}

ClassId Header::classId() {
  uword header = reinterpret_cast<uword>(this);
  return static_cast<ClassId>((header >> kClassIdOffset) & kClassIdMask);
}

ObjectFormat Header::format() {
  uword header = reinterpret_cast<uword>(this);
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

Object* None::hash() {
  return SmallInteger::fromWord(reinterpret_cast<word>(this));
}

// Ellipsis

Ellipsis* Ellipsis::object() {
  return reinterpret_cast<Ellipsis*>(kTag);
}

Ellipsis* Ellipsis::cast(Object* object) {
  assert(object->isEllipsis());
  return reinterpret_cast<Ellipsis*>(object);
}

Object* Ellipsis::hash() {
  return SmallInteger::fromWord(reinterpret_cast<word>(this));
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

Object* Boolean::hash() {
  return SmallInteger::fromWord(value());
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

void Code::initialize(
    int argcount,
    int kwonlyargcount,
    int nlocals,
    int stacksize,
    int flags,
    Object* code,
    Object* consts,
    Object* names,
    Object* varnames,
    Object* freevars,
    Object* cellvars,
    Object* filename,
    Object* name,
    int firstlineno,
    Object* lnotab) {
  instanceVariableAtPut(kArgcountOffset, SmallInteger::fromWord(argcount));
  instanceVariableAtPut(
      kKwonlyargcountOffset, SmallInteger::fromWord(kwonlyargcount));
  instanceVariableAtPut(kNlocalsOffset, SmallInteger::fromWord(nlocals));
  instanceVariableAtPut(kStacksizeOffset, SmallInteger::fromWord(stacksize));
  instanceVariableAtPut(kFlagsOffset, SmallInteger::fromWord(flags));
  instanceVariableAtPut(kCodeOffset, code);
  instanceVariableAtPut(kConstsOffset, consts);
  instanceVariableAtPut(kNamesOffset, names);
  instanceVariableAtPut(kVarnamesOffset, varnames);
  instanceVariableAtPut(kFreevarsOffset, freevars);
  instanceVariableAtPut(kCellvarsOffset, cellvars);
  instanceVariableAtPut(kFilenameOffset, filename);
  instanceVariableAtPut(kNameOffset, name);
  instanceVariableAtPut(
      kFirstlinenoOffset, SmallInteger::fromWord(firstlineno));
  instanceVariableAtPut(kLnotabOffset, lnotab);
}

word Code::argcount() {
  return SmallInteger::cast(instanceVariableAt(kArgcountOffset))->value();
}

int Code::kwonlyargcount() {
  return SmallInteger::cast(instanceVariableAt(kKwonlyargcountOffset))->value();
}

Object* Code::code() {
  return instanceVariableAt(kCodeOffset);
}

Object* Code::cellvars() {
  return instanceVariableAt(kCellvarsOffset);
}

Object* Code::filename() {
  return instanceVariableAt(kFilenameOffset);
}

Object* Code::lnotab() {
  return instanceVariableAt(kLnotabOffset);
}

int Code::flags() {
  return SmallInteger::cast(instanceVariableAt(kFlagsOffset))->value();
}

int Code::firstlineno() {
  return SmallInteger::cast(instanceVariableAt(kFirstlinenoOffset))->value();
}

int Code::cell2arg() {
  return SmallInteger::cast(instanceVariableAt(kCell2argOffset))->value();
}

Object* Code::freevars() {
  return instanceVariableAt(kFreevarsOffset);
}

int Code::stacksize() {
  return SmallInteger::cast(instanceVariableAt(kStacksizeOffset))->value();
}

int Code::nlocals() {
  return SmallInteger::cast(instanceVariableAt(kNlocalsOffset))->value();
}

Object* Code::name() {
  return instanceVariableAt(kNameOffset);
}

Object* Code::names() {
  return instanceVariableAt(kNamesOffset);
}

Object* Code::consts() {
  return instanceVariableAt(kConstsOffset);
}

Object* Code::varnames() {
  return instanceVariableAt(kVarnamesOffset);
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

word Function::allocationSize() {
  return Utils::roundUp(Function::kSize, kPointerSize);
}

Function* Function::cast(Object* object) {
  assert(object->isFunction());
  return reinterpret_cast<Function*>(object);
}

Object* Function::code() {
  return instanceVariableAt(Function::kCodeOffset);
}

void Function::initialize() {
  // ???
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
