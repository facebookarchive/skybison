#include "marshal.h"

#include <cassert>

#include "heap.h"

namespace python {

enum {
  TYPE_NULL = '0',
  TYPE_NONE = 'N',
  TYPE_FALSE = 'F',
  TYPE_TRUE = 'T',
  TYPE_STOPITER = 'S',
  TYPE_ELLIPSIS = '.',
  TYPE_INT = 'i',
  TYPE_FLOAT = 'f',
  TYPE_BINARY_FLOAT = 'g',
  TYPE_COMPLEX = 'x',
  TYPE_BINARY_COMPLEX = 'y',
  TYPE_LONG = 'l',
  TYPE_STRING = 's',
  TYPE_INTERNED = 't',
  TYPE_REF = 'r',
  TYPE_TUPLE = '(',
  TYPE_LIST = '[',
  TYPE_DICT = '{',
  TYPE_CODE = 'c',
  TYPE_UNICODE = 'u',
  TYPE_UNKNOWN = '?',
  TYPE_SET = '<',
  TYPE_FROZENSET = '>',
  FLAG_REF = '\x80', /* with a type, add obj to index */

  TYPE_ASCII = 'a',
  TYPE_ASCII_INTERNED = 'A',
  TYPE_SMALL_TUPLE = ')',
  TYPE_SHORT_ASCII = 'z',
  TYPE_SHORT_ASCII_INTERNED = 'Z',
};


Marshal::Reader::Reader(const char* buffer) 
    : start_(reinterpret_cast<const byte*>(buffer))
    , length_(strlen(buffer))
    , refs_(nullptr)
    , pos_(0)
    , depth_(0) {
  end_ = start_ + length_;
}

const byte* Marshal::Reader::ReadString(int length) {
  const byte* result = &start_[pos_];
  pos_ += length;
  return result;
}


byte Marshal::Reader::ReadByte() {
  byte result = -1;
  const byte* buffer = ReadString(1);
  if (buffer != nullptr) {
    result = buffer[0];
  }
  return result;  
}


int16 Marshal::Reader::ReadShort() {
  int16 result = -1;
  const byte* buffer = ReadString(sizeof(result));
  if (buffer != nullptr) {
    result = buffer[0];
    result |= buffer[1] <<  8;
  }
  return result;  
}


int32 Marshal::Reader::ReadLong() {
  int32 result = -1;
  const byte* buffer = ReadString(4);
  if (buffer != nullptr) {
    result = buffer[0];
    result |= buffer[1] <<  8;
    result |= buffer[2] << 16;
    result |= buffer[3] << 24;
  }
  return result;  
}


template<typename T>
class ScopedCounter {
 public:
  ScopedCounter(T* counter) : counter_(counter) {
    (*counter_)++;
  }
  ~ScopedCounter() {
    (*counter_)--;
  }
 public:
  T* counter_;
  DISALLOW_COPY_AND_ASSIGN(ScopedCounter);
};


Object* Marshal::Reader::ReadObject() {
  ScopedCounter<int> counter(&depth_);
  Object* result = nullptr;
  byte code = ReadByte();
  byte flag = code & FLAG_REF;
  byte type = code & ~FLAG_REF;
  isRef_ = flag;
  switch (type) {
    case TYPE_NULL:
      break;

    case TYPE_NONE:
      result = None::object();
      break;

    case TYPE_STOPITER:
      assert(0);
      break;

    case TYPE_ELLIPSIS:
      assert(0);
      break;

    case TYPE_FALSE:
      result = Boolean::fromBool(false);
      break;

    case TYPE_TRUE:
      result = Boolean::fromBool(true);
      break;

    case TYPE_INT:
      assert(0);
      break;

    case TYPE_FLOAT:
      assert(0);
      break;

    case TYPE_BINARY_FLOAT:
      assert(0);
      break;

    case TYPE_COMPLEX:
      assert(0);
      break;

    case TYPE_BINARY_COMPLEX:
      assert(0);
      break;

    case TYPE_STRING:  // Misnomer, should be TYPE_BYTES
      result = ReadTypeString();
      break;

    case TYPE_ASCII_INTERNED:
      assert(0);
      break;

    case TYPE_SHORT_ASCII_INTERNED:
      result = ReadTypeShortAscii();
      break;

    case TYPE_SHORT_ASCII:
      result = ReadTypeShortAscii();
      break;

    case TYPE_INTERNED:
      assert(0);
      break;

    case TYPE_UNICODE:
      assert(0);
      break;

    case TYPE_SMALL_TUPLE:
      result = ReadTypeSmallTuple();
      break;

    case TYPE_TUPLE:
      result = ReadTypeTuple();
      break;

    case TYPE_LIST:
      assert(0);
      break;

    case TYPE_DICT:
      assert(0);
      break;

    case TYPE_SET:
      assert(0);
      break;

    case TYPE_FROZENSET:
      assert(0);
      break;

    case TYPE_CODE:
      result = ReadTypeCode();
      break;

    case TYPE_REF:
      result = ReadTypeRef();
      break;

    default:
      assert(0);
  }
  return result;
}

int Marshal::Reader::AddRef(Object* value) {
  if (refs_ == nullptr) {
    // Lazily create a new refs array.
    refs_ = Heap::CreateObjectArray(1);
    ObjectArray::cast(refs_)->set(0, value);
    return 0;
  } else {
    // Grow the refs array
    int length = ObjectArray::cast(refs_)->length();
    Object* new_array = Heap::CreateObjectArray(length + 1);
    for (int i = 0; i < length; i++) {
      Object* value = ObjectArray::cast(refs_)->get(i);
      ObjectArray::cast(new_array)->set(i, value);
    }
    ObjectArray::cast(new_array)->set(length, value);
    refs_ = new_array;
    return length;
  }
}

void Marshal::Reader::SetRef(int index, Object* value) {
   ObjectArray::cast(refs_)->set(index, value);
 }

Object* Marshal::Reader::GetRef(int index) {
  return ObjectArray::cast(refs_)->get(index);
}

Object* Marshal::Reader::ReadTypeString() {
  int length = ReadLong();
  const byte* data = ReadString(length);
  Object* result = Heap::CreateByteArray(length);
  if (isRef_) {
    AddRef(result);
  }
  for (int i = 0; i < length; i++) {
    ByteArray::cast(result)->setByteAt(i, data[i]);
  }
  return result;
}

Object* Marshal::Reader::ReadTypeShortAscii() {
  int length = ReadByte();
  const byte* data = ReadString(length);
  Object* result = Heap::CreateString(length);
  if (isRef_) {
    AddRef(result);
  }
  for (int i = 0; i < length; i++) {
    String::cast(result)->setCharAt(i, data[i]);
  }
  return result;
}

Object* Marshal::Reader::ReadTypeSmallTuple() {
  int32 n = ReadByte();
  return DoTupleElements(n);
}

Object* Marshal::Reader::ReadTypeTuple() {
  int n = ReadLong();
  return DoTupleElements(n);
}

Object* Marshal::Reader::DoTupleElements(int32 length) {
  Object* result = Heap::CreateObjectArray(length);
  if (isRef_) {
    AddRef(result);
  }
  for (int i = 0; i < length; i++) {
    Object* value = ReadObject();
    ObjectArray::cast(result)->set(i, value);
  }
  return result;
}

Object* Marshal::Reader::ReadTypeCode() {
  int index = -1;
  if (isRef_) {
    index = AddRef(None::object());
  }
  int argcount = ReadLong();
  int kwonlyargcount = ReadLong();
  int nlocals = ReadLong();
  int stacksize = ReadLong();
  int flags = ReadLong();
  Object* code = ReadObject();
  Object* consts = ReadObject();
  Object* names = ReadObject();
  Object* varnames = ReadObject();
  Object* freevars = ReadObject();
  Object* cellvars = ReadObject();
  Object* filename = ReadObject();
  Object* name = ReadObject();
  int firstlineno = ReadLong();
  Object* lnotab = ReadObject();
  Object* result = Heap::CreateCode(
      argcount,
      kwonlyargcount,
      nlocals,
      stacksize,
      flags,
      code,
      consts,
      names,
      varnames,
      freevars,
      cellvars,
      filename,
      name,
      firstlineno,
      lnotab);
  if (index != -1) {
    SetRef(index, result);
  }
  return result;
}

Object* Marshal::Reader::ReadTypeRef() {
  int32 n = ReadLong();
  return GetRef(n);
}

}  // namespace python
