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
  DICTIONARY
};

class Object {
 public:
  // Immediate
  inline bool isSmallInteger();
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

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Object);
};

class Boolean : public Object {
 public:
  static inline Boolean* fromBool(bool value);
  static inline Boolean* cast(Object* object);

  static const int kTag = 7; // 0b0111
  static const int kTagSize = 4;
  static const uword kTagMask = (1 << kTagSize) - 1;

  inline bool value();

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Boolean);
};

class None : public Object {
 public:
  static inline None* cast(Object* object);

  static const int kTag = 3; // 0b0011
  static const int kTagSize = 4;
  static const uword kTagMask = (1 << kTagSize) - 1;

  inline static None* object();

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(None);
};

class Ellipsis : public Object {
 public:
  static inline Ellipsis* cast(Object* object);

  static const int kTag = 11; // 0b1011
  static const int kTagSize = 4;
  static const uword kTagMask = (1 << kTagSize) - 1;

  inline static Ellipsis* object();

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Ellipsis);
};

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

class Class;

class HeapObject : public Object {
 public:
  // Setters and getters.
  inline uword address();
  inline static HeapObject* fromAddress(uword address);

  inline Class* getClass();
  inline void setClass(Class* a_class);
  inline void initialize(int size, Object* value);

  inline static HeapObject* cast(Object* object);

  static const int kTag = 1;
  static const int kTagSize = 2;
  static const uword kTagMask = (1 << kTagSize) - 1;

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
  inline void setLayout(Layout layout);

  inline static Class* cast(Object* object);
  inline static int allocationSize() {
    return Class::kSize;
  }

  static const int kLayoutOffset = HeapObject::kSize;
  static const int kSize = kLayoutOffset + kPointerSize;

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

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ObjectArray);
};

class String : public Array {
 public:
  static inline String* cast(Object* object);
  inline byte charAt(int index);
  inline void setCharAt(int index, byte value);

  inline static int allocationSize(int length);

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
  inline Object* address();
  inline Object* code();
  inline static int allocationSize();
  inline void initialize();

  static const int kAddressOffset = HeapObject::kSize;
  static const int kCodeOffset = kAddressOffset + kPointerSize;
  static const int kSize = kCodeOffset + kPointerSize;

 private:
  DISALLOW_COPY_AND_ASSIGN(Function);
};

class Module : public HeapObject {
 public:
  inline static Module* cast(Object* object);
  Object* name();
  inline static int allocationSize();
  inline void initialize();

  static const int kNameOffset = HeapObject::kSize;
  static const int kDictionaryOffset = kNameOffset + kPointerSize;
  static const int kSize = kDictionaryOffset + kPointerSize;

 private:
  DISALLOW_COPY_AND_ASSIGN(Module);
};

class Dictionary : public HeapObject {
 public:
  inline static Dictionary* cast(Object* object);
  Object* data();
  inline static int allocationSize();
  inline static void initialize();

  static const int kDataOffset = HeapObject::kSize;
  static const int kSize = kDataOffset + kPointerSize;

 private:
  DISALLOW_COPY_AND_ASSIGN(Dictionary);
};

// Object

bool Object::isClass() {
  if (!isHeapObject()) {
    return false;
  }
  return HeapObject::cast(this)->getClass()->layout() == Layout::CLASS;
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
  return tag == None::kTag;
}

bool Object::isSmallInteger() {
  int tag = reinterpret_cast<uword>(this) & SmallInteger::kTagMask;
  return tag == SmallInteger::kTag;
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

// HeapObject

uword HeapObject::address() {
  return reinterpret_cast<uword>(this) - HeapObject::kTag;
}

HeapObject* HeapObject::fromAddress(uword address) {
  assert((address & kTagMask) == 0);
  return reinterpret_cast<HeapObject*>(address + kTag);
}

HeapObject* HeapObject::cast(Object* obj) {
  assert(obj->isHeapObject());
  return reinterpret_cast<HeapObject*>(obj);
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
  return reinterpret_cast<Class*>(at(kClassOffset));
}

void HeapObject::setClass(Class* a_class) {
  atPut(kClassOffset, a_class);
}

// Class

Layout Class::layout() {
  return static_cast<Layout>(SmallInteger::cast(at(kLayoutOffset))->value());
}

void Class::setLayout(Layout layout) {
  atPut(kLayoutOffset, SmallInteger::fromWord(layout));
}

Class* Class::cast(Object* object) {
  assert(object->isClass());
  return reinterpret_cast<Class*>(object);
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

Object* Function::address() {
  return at(Function::kAddressOffset);
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

void Module::initialize() {
  // ???
}

// Dictionary

int Dictionary::allocationSize() {
  return Utils::roundUp(Module::kSize, kPointerSize);
}

Dictionary* Dictionary::cast(Object* object) {
  assert(object->isDictionary());
  return reinterpret_cast<Dictionary*>(object);
}

void Dictionary::initialize() {
  // ???
}

// String

String* String::cast(Object* object) {
  assert(object->isString());
  return reinterpret_cast<String*>(object);
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

} // namespace python
