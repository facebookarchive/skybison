#pragma once

#include "globals.h"
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

enum Layout {
  CLASS = 0xf00f,
  CODE,
  OBJECT_ARRAY,
  BYTE_ARRAY,
  STRING,
  FUNCTION,
  MODULE,
  DICTIONARY,
  LIST
};

class Object {
 public:
  inline bool isObject() {
    return true;
  }

  // The constructor for Handle<T> calls T::cast, thus an implementation is
  // required in order to create a Handle<Object>.
  static inline Object* cast(Object* object) {
    return object;
  }

  // Immediate
  inline bool isSmallInteger();
  inline bool isHeaderWord();
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

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Object);
};

class Class;

// Immediate objects

class SmallInteger : public Object {
 public:
  static inline SmallInteger* fromWord(word value);
  static inline SmallInteger* cast(Object* object);

  static const int kTag = 0;
  static const int kTagSize = 1;
  static const uword kTagMask = (1 << kTagSize) - 1;

  inline word value();

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(SmallInteger);
};

class HeaderWord : public Object {
 public:
  static inline HeaderWord* fromClass(Class* a_class);
  static inline HeaderWord* cast(Object* object);

  static const int kTag = 3;
  static const int kTagSize = 3;
  static const uword kTagMask = (1 << kTagSize) - 1;

  inline Class* toClass();

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(HeaderWord);
};

class Boolean : public Object {
 public:
  static inline Boolean* fromBool(bool value);
  static inline Boolean* cast(Object* object);

  static const int kTag = 7; // 0b00111
  static const int kTagSize = 5;
  static const uword kTagMask = (1 << kTagSize) - 1;

  inline bool value();

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Boolean);
};

class None : public Object {
 public:
  static inline None* cast(Object* object);

  static const int kTag = 15; // 0b01111
  static const int kTagSize = 5;
  static const uword kTagMask = (1 << kTagSize) - 1;

  inline static None* object();

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(None);
};

class Ellipsis : public Object {
 public:
  static inline Ellipsis* cast(Object* object);

  static const int kTag = 23; // 0b10111
  static const int kTagSize = 5;
  static const uword kTagMask = (1 << kTagSize) - 1;

  inline static Ellipsis* object();

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Ellipsis);
};

// Heap objects

class HeapObject : public Object {
 public:
  // Setters and getters.
  inline uword address();
  inline static HeapObject* fromAddress(uword address);
  inline int size();

  inline Class* getClass();
  inline void setClass(Class* a_class);
  inline void initialize(int size, Object* value);

  inline static HeapObject* cast(Object* object);

  inline bool isForwarding();
  inline Object* forward();
  inline void forwardTo(Object* object);

  static const int kTag = 1;
  static const int kTagSize = 2;
  static const uword kTagMask = (1 << kTagSize) - 1;
  static const uword kIsForwarded = -3;

  // Layout.
  static const int kClassOffset = 0;
  static const int kSize = kClassOffset + kPointerSize;

 protected:
  inline Object* at(int offset);
  inline void atPut(int offset, Object* value);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(HeapObject);
};

class Class : public HeapObject {
 public:
  inline Layout layout();

  inline static Class* cast(Object* object);

  inline static int allocationSize() {
    return Class::kSize;
  }

  inline void initialize(Layout layout, int size, bool isArray, bool isRoot);

  inline word instanceSize();

  inline bool isRoot();

  static const int kLayoutOffset = HeapObject::kSize;
  static const int kInstanceSizeOffset = kLayoutOffset + kPointerSize;
  static const int kIsRootOffset = kInstanceSizeOffset + kPointerSize;
  static const int kSize = kIsRootOffset + kPointerSize;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Class);
};

// Abstract
class Array : public HeapObject {
 public:
  // Setters and getters.
  inline word length();
  inline void setLength(word length);

  // Layout.
  static const int kLengthOffset = HeapObject::kSize;
  static const int kSize = kLengthOffset + kPointerSize;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Array);
};

class ByteArray : public Array {
 public:
  inline byte byteAt(int index);
  inline void setByteAt(int index, byte value);

  inline static ByteArray* cast(Object* object);
  inline static int allocationSize(int length);
  inline void initialize(int length);

  static const int kElementSize = 1;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ByteArray);
};

class ObjectArray : public Array {
 public:
  inline Object* get(int index);
  inline void set(int index, Object* value);

  inline static ObjectArray* cast(Object* object);

  inline static int allocationSize(int length);

  inline void initialize(int length, int size, Object* value);

  inline void copyTo(ObjectArray* dst);

  static const int kElementSize = kPointerSize;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ObjectArray);
};

class String : public Array {
 public:
  static inline String* cast(Object* object);
  inline void initialize(word length);
  inline static int allocationSize(int length);

  inline byte charAt(int index);
  inline void setCharAt(int index, byte value);
  inline bool equals(Object* that);

  static const int kElementSize = 1;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(String);
};

class Code : public HeapObject {
 public:
  inline static Code* cast(Object* obj);

  static int allocationSize() {
    return Utils::roundUp(kSize, kPointerSize);
  }

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

  inline int argcount();

  inline Object* code();

  inline Object* cellvars();

  inline Object* freevars();

  inline int stacksize();

  inline int nlocals();

  inline Object* consts();

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
  inline static Function* cast(Object* object);
  inline Object* code();
  inline static int allocationSize();
  inline void initialize();

  static const int kCodeOffset = HeapObject::kSize + kPointerSize;
  static const int kSize = kCodeOffset + kPointerSize;

 private:
  DISALLOW_COPY_AND_ASSIGN(Function);
};

class Module : public HeapObject {
 public:
  inline static Module* cast(Object* object);
  inline Object* name();
  inline static int allocationSize();
  inline void initialize();

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
  inline static Dictionary* cast(Object* object);
  inline static int allocationSize();
  inline void initialize(Object* items);

  // Number of items currently in the dictionary
  inline word numItems();

  // Total number of items that could be stored in the ObjectArray backing the
  // dictionary
  inline word capacity();

  // Look up the value associated with key.
  //
  // Returns true if the key was found and sets the associated value in value.
  // Returns false otherwise.
  static bool itemAt(Object* dict, Object* key, word hash, Object** value);

  // Associate a value with the supplied key.
  //
  // This handles growing the backing ObjectArray if needed.
  static void itemAtPut(
      Object* dict,
      Object* key,
      word hash,
      Object* value,
      Runtime* runtime);

  // Delete a key from the dictionary.
  //
  // Returns true if the key existed and sets the previous value in value.
  // Returns false otherwise.
  static bool
  itemAtRemove(Object* dict, Object* key, word hash, Object** value);

  static const int kBucketHashOffset = 0;
  static const int kBucketKeyOffset = kBucketHashOffset + 1;
  static const int kBucketValueOffset = kBucketKeyOffset + 2;
  static const int kPointersPerBucket = kBucketValueOffset + 1;

  // Initial size of the dictionary. According to comments in CPython's
  // dictobject.c this accomodates the majority of dictionaries without needing
  // a resize (obviously this depends on the load factor used to resize the
  // dict).
  static const int kInitialItemsSize = 8 * kPointersPerBucket;
  static const int kNumItemsOffset = HeapObject::kSize;
  static const int kItemsOffset = kNumItemsOffset + kPointerSize;
  static const int kSize = kItemsOffset + kPointerSize;

 private:
  ObjectArray* items();
  void setItems(ObjectArray* items);
  void setNumItems(word numItems);

  // Looks up the supplied key in the dictionary.
  //
  // If the key is found, this function returns true and sets bucket to the
  // index of the bucket that contains the value. If the key is not found, this
  // function returns false and sets bucket to the location where the key would
  // be inserted. If the dictionary is full, it sets bucket to -1.
  static bool lookup(Dictionary* dict, Object* key, word hash, int* bucket);
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
class List : public Array {
 public:
  static List* cast(Object* object);
  static int allocationSize();
  void initialize(Object* elements);

  // Return the total number of elements that may be held without growing the
  // list
  word capacity();

  Object* get(int index);
  void set(int index, Object* value);

  static void appendAndGrow(List* list, Object* value, Runtime* runtime);

  ObjectArray* elems();
  void setElems(ObjectArray* elems);

  static const int kElemsOffset = Array::kSize;
  static const int kSize = kElemsOffset + kPointerSize;

 private:
  DISALLOW_COPY_AND_ASSIGN(List);
};

// Object

bool Object::isClass() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->getClass()->layout() == Layout::CLASS;
}

bool Object::isSmallInteger() {
  int tag = reinterpret_cast<uword>(this) & SmallInteger::kTagMask;
  return tag == SmallInteger::kTag;
}

bool Object::isHeaderWord() {
  int tag = reinterpret_cast<uword>(this) & HeaderWord::kTagMask;
  return tag == HeaderWord::kTag;
}

bool Object::isBoolean() {
  int tag = reinterpret_cast<uword>(this) & Boolean::kTagMask;
  return tag == Boolean::kTag;
}

bool Object::isNone() {
  int tag = reinterpret_cast<uword>(this) & None::kTagMask;
  return tag == None::kTag;
}

bool Object::isEllipsis() {
  int tag = reinterpret_cast<uword>(this) & None::kTagMask;
  return tag == Ellipsis::kTag;
}

bool Object::isHeapObject() {
  int tag = reinterpret_cast<uword>(this) & HeapObject::kTagMask;
  return tag == HeapObject::kTag;
}

bool Object::isByteArray() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->getClass()->layout() == Layout::BYTE_ARRAY;
}

bool Object::isObjectArray() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->getClass()->layout() == Layout::OBJECT_ARRAY;
}

bool Object::isCode() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->getClass()->layout() == Layout::CODE;
}

bool Object::isString() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->getClass()->layout() == Layout::STRING;
}

bool Object::isFunction() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->getClass()->layout() == Layout::FUNCTION;
}

bool Object::isDictionary() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->getClass()->layout() == Layout::DICTIONARY;
}

bool Object::isModule() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->getClass()->layout() == Layout::MODULE;
}

bool Object::isList() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->getClass()->layout() == Layout::LIST;
}

bool Object::equals(Object* lhs, Object* rhs) {
  return (lhs == rhs) || (lhs->isString() && String::cast(lhs)->equals(rhs));
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

HeaderWord* HeaderWord::cast(Object* object) {
  assert(object->isHeaderWord());
  return reinterpret_cast<HeaderWord*>(object);
}

HeaderWord* HeaderWord::fromClass(Class* a_class) {
  return reinterpret_cast<HeaderWord*>(a_class->address() | HeaderWord::kTag);
}

Class* HeaderWord::toClass() {
  uword address = reinterpret_cast<uword>(this) - HeaderWord::kTag;
  return reinterpret_cast<Class*>(HeapObject::fromAddress(address));
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

HeapObject* HeapObject::fromAddress(uword address) {
  assert((address & kTagMask) == 0);
  return reinterpret_cast<HeapObject*>(address + kTag);
}

int HeapObject::size() {
  word result = getClass()->instanceSize();
  if (result < 0) {
    result *= -SmallInteger::cast(at(Array::kLengthOffset))->value();
    result += Array::kSize;
  }
  return result;
}

HeapObject* HeapObject::cast(Object* obj) {
  assert(obj->isHeapObject());
  return reinterpret_cast<HeapObject*>(obj);
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
  atPut(kPointerSize, object);
}

void HeapObject::initialize(int size, Object* value) {
  for (int offset = HeapObject::kSize; offset < size; offset += kPointerSize) {
    atPut(offset, value);
  }
}

Object* HeapObject::at(int offset) {
  return *reinterpret_cast<Object**>(address() + offset);
}

void HeapObject::atPut(int offset, Object* value) {
  *reinterpret_cast<Object**>(address() + offset) = value;
}

Class* HeapObject::getClass() {
  return HeaderWord::cast(at(kClassOffset))->toClass();
}

void HeapObject::setClass(Class* a_class) {
  atPut(kClassOffset, HeaderWord::fromClass(a_class));
}

// Class

void Class::initialize(Layout layout, int size, bool isArray, bool isRoot) {
  atPut(kLayoutOffset, SmallInteger::fromWord(layout));
  atPut(kInstanceSizeOffset, SmallInteger::fromWord(size * (isArray ? -1 : 1)));
  atPut(kIsRootOffset, Boolean::fromBool(isRoot));
}

Layout Class::layout() {
  Object* hopefully_a_small_integer = at(kLayoutOffset);
  return static_cast<Layout>(
      SmallInteger::cast(hopefully_a_small_integer)->value());
}

Class* Class::cast(Object* object) {
  assert(object->isClass());
  return reinterpret_cast<Class*>(object);
}

word Class::instanceSize() {
  return SmallInteger::cast(at(kInstanceSizeOffset))->value();
}

bool Class::isRoot() {
  return Boolean::cast(at(kIsRootOffset))->value();
}

// Array

word Array::length() {
  return SmallInteger::cast(at(Array::kLengthOffset))->value();
}

void Array::setLength(word length) {
  atPut(kLengthOffset, SmallInteger::fromWord(length));
}

// ByteArray

int ByteArray::allocationSize(int length) {
  return Utils::roundUp(length + Array::kSize, kPointerSize);
}

void ByteArray::initialize(int length) {
  setLength(length);
  memset(reinterpret_cast<void*>(address() + Array::kSize), 0, length);
}

byte ByteArray::byteAt(int index) {
  assert(index >= 0);
  assert(index < length());
  return *reinterpret_cast<byte*>(address() + Array::kSize + index);
}

void ByteArray::setByteAt(int index, byte value) {
  assert(index >= 0);
  assert(index < length());
  *reinterpret_cast<byte*>(address() + Array::kSize + index) = value;
}

ByteArray* ByteArray::cast(Object* object) {
  assert(object->isByteArray());
  return reinterpret_cast<ByteArray*>(object);
}

// ObjectArray

int ObjectArray::allocationSize(int length) {
  return Utils::roundUp(length * kPointerSize + Array::kSize, kPointerSize);
}

void ObjectArray::initialize(int length, int size, Object* value) {
  setLength(length);
  for (int offset = Array::kSize; offset < size; offset += kPointerSize) {
    atPut(offset, value);
  }
}

ObjectArray* ObjectArray::cast(Object* object) {
  assert(object->isObjectArray());
  return reinterpret_cast<ObjectArray*>(object);
}

Object* ObjectArray::get(int index) {
  assert(index >= 0);
  assert(index < length());
  return at(Array::kSize + (index * kPointerSize));
}

void ObjectArray::set(int index, Object* value) {
  assert(index >= 0);
  assert(index < length());
  atPut(Array::kSize + (index * kPointerSize), value);
}

void ObjectArray::copyTo(ObjectArray* dst) {
  word len = length();
  assert(len <= dst->length());
  for (int i = 0; i < len; i++) {
    Object* elem = get(i);
    dst->set(i, elem);
  }
}

// Code

Code* Code::cast(Object* obj) {
  assert(obj->isCode());
  return reinterpret_cast<Code*>(obj);
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
  atPut(kArgcountOffset, SmallInteger::fromWord(argcount));
  atPut(kKwonlyargcountOffset, SmallInteger::fromWord(kwonlyargcount));
  atPut(kNlocalsOffset, SmallInteger::fromWord(nlocals));
  atPut(kStacksizeOffset, SmallInteger::fromWord(stacksize));
  atPut(kFlagsOffset, SmallInteger::fromWord(flags));
  atPut(kCodeOffset, code);
  atPut(kConstsOffset, consts);
  atPut(kNamesOffset, names);
  atPut(kVarnamesOffset, varnames);
  atPut(kFreevarsOffset, freevars);
  atPut(kCellvarsOffset, cellvars);
  atPut(kFilenameOffset, filename);
  atPut(kNameOffset, name);
  atPut(kFirstlinenoOffset, SmallInteger::fromWord(firstlineno));
  atPut(kLnotabOffset, lnotab);
}

int Code::argcount() {
  return SmallInteger::cast(at(kArgcountOffset))->value();
}

Object* Code::code() {
  return at(kCodeOffset);
}

Object* Code::cellvars() {
  return at(kCellvarsOffset);
}

Object* Code::freevars() {
  return at(kFreevarsOffset);
}

int Code::stacksize() {
  return SmallInteger::cast(at(kStacksizeOffset))->value();
}

int Code::nlocals() {
  return SmallInteger::cast(at(kNlocalsOffset))->value();
}

Object* Code::consts() {
  return at(kConstsOffset);
}

// Function

int Function::allocationSize() {
  return Utils::roundUp(Function::kSize, kPointerSize);
}

Function* Function::cast(Object* object) {
  assert(object->isFunction());
  return reinterpret_cast<Function*>(object);
}

Object* Function::code() {
  return at(Function::kCodeOffset);
}

void Function::initialize() {
  // ???
}

// Module

int Module::allocationSize() {
  return Utils::roundUp(Module::kSize, kPointerSize);
}

Module* Module::cast(Object* object) {
  assert(object->isModule());
  return reinterpret_cast<Module*>(object);
}

Object* Module::name() {
  return at(Module::kNameOffset);
}

void Module::initialize() {
  // ???
}

// String

String* String::cast(Object* object) {
  assert(object->isString());
  return reinterpret_cast<String*>(object);
}

void String::initialize(word length) {
  setLength(length);
}

int String::allocationSize(int length) {
  return Utils::roundUp(Array::kSize + length, kPointerSize);
}

byte String::charAt(int index) {
  assert(index >= 0);
  assert(index < length());
  return *reinterpret_cast<byte*>(address() + String::kSize + index);
}

void String::setCharAt(int index, byte value) {
  assert(index >= 0);
  assert(index < length());
  *reinterpret_cast<byte*>(address() + String::kSize + index) = value;
}

bool String::equals(Object* that) {
  if (!that->isString()) {
    return false;
  }
  String* thatStr = String::cast(that);
  if (length() != thatStr->length()) {
    return false;
  }
  auto s1 = reinterpret_cast<void*>(address() + String::kSize);
  auto s2 = reinterpret_cast<void*>(thatStr->address() + String::kSize);
  return memcmp(s1, s2, length()) == 0;
}

// Dictionary

int Dictionary::allocationSize() {
  return Utils::roundUp(Module::kSize, kPointerSize);
}

void Dictionary::initialize(Object* items) {
  atPut(kNumItemsOffset, SmallInteger::fromWord(0));
  atPut(kItemsOffset, items);
}

Dictionary* Dictionary::cast(Object* object) {
  assert(object->isDictionary());
  return reinterpret_cast<Dictionary*>(object);
}

word Dictionary::numItems() {
  return SmallInteger::cast(at(kNumItemsOffset))->value();
};

word Dictionary::capacity() {
  return items()->length() / kPointersPerBucket;
}

} // namespace python
