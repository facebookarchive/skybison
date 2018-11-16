#include "marshal.h"

#include <cassert>

#include "heap.h"
#include "runtime.h"

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

Marshal::Reader::Reader(Runtime* runtime, const char* buffer)
    : runtime_(runtime),
      start_(reinterpret_cast<const byte*>(buffer)),
      length_(strlen(buffer)),
      refs_(nullptr),
      pos_(0),
      depth_(0) {
  end_ = start_ + length_;
}

const byte* Marshal::Reader::readString(int length) {
  const byte* result = &start_[pos_];
  pos_ += length;
  return result;
}

byte Marshal::Reader::readByte() {
  byte result = -1;
  const byte* buffer = readString(1);
  if (buffer != nullptr) {
    result = buffer[0];
  }
  return result;
}

int16 Marshal::Reader::readShort() {
  int16 result = -1;
  const byte* buffer = readString(sizeof(result));
  if (buffer != nullptr) {
    result = buffer[0];
    result |= buffer[1] << 8;
  }
  return result;
}

int32 Marshal::Reader::readLong() {
  int32 result = -1;
  const byte* buffer = readString(4);
  if (buffer != nullptr) {
    result = buffer[0];
    result |= buffer[1] << 8;
    result |= buffer[2] << 16;
    result |= buffer[3] << 24;
  }
  return result;
}

template <typename T>
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

Object* Marshal::Reader::readObject() {
  ScopedCounter<int> counter(&depth_);
  Object* result = nullptr;
  byte code = readByte();
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

    case TYPE_STRING: // Misnomer, should be TYPE_BYTES
      result = readTypeString();
      break;

    case TYPE_ASCII_INTERNED:
      assert(0);
      break;

    case TYPE_SHORT_ASCII_INTERNED:
      result = readTypeShortAscii();
      break;

    case TYPE_SHORT_ASCII:
      result = readTypeShortAscii();
      break;

    case TYPE_INTERNED:
      assert(0);
      break;

    case TYPE_UNICODE:
      assert(0);
      break;

    case TYPE_SMALL_TUPLE:
      result = readTypeSmallTuple();
      break;

    case TYPE_TUPLE:
      result = readTypeTuple();
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
      result = readTypeCode();
      break;

    case TYPE_REF:
      result = readTypeRef();
      break;

    default:
      assert(0);
  }
  return result;
}

int Marshal::Reader::addRef(Object* value) {
  if (refs_ == nullptr) {
    // Lazily create a new refs list.
    refs_ = runtime_->createList();
  }
  List::appendAndGrow(List::cast(refs_), value, runtime_);
  return 0;
}

void Marshal::Reader::setRef(int index, Object* value) {
  List::cast(refs_)->set(index, value);
}

Object* Marshal::Reader::getRef(int index) {
  return List::cast(refs_)->get(index);
}

Object* Marshal::Reader::readTypeString() {
  int length = readLong();
  const byte* data = readString(length);
  Object* result = runtime_->createByteArray(length);
  if (isRef_) {
    addRef(result);
  }
  for (int i = 0; i < length; i++) {
    ByteArray::cast(result)->setByteAt(i, data[i]);
  }
  return result;
}

Object* Marshal::Reader::readTypeShortAscii() {
  int length = readByte();
  const byte* data = readString(length);
  Object* result = runtime_->createString(length);
  if (isRef_) {
    addRef(result);
  }
  for (int i = 0; i < length; i++) {
    String::cast(result)->setCharAt(i, data[i]);
  }
  return result;
}

Object* Marshal::Reader::readTypeSmallTuple() {
  int32 n = readByte();
  return doTupleElements(n);
}

Object* Marshal::Reader::readTypeTuple() {
  int n = readLong();
  return doTupleElements(n);
}

Object* Marshal::Reader::doTupleElements(int32 length) {
  Object* result = runtime_->createObjectArray(length);
  if (isRef_) {
    addRef(result);
  }
  for (int i = 0; i < length; i++) {
    Object* value = readObject();
    ObjectArray::cast(result)->set(i, value);
  }
  return result;
}

Object* Marshal::Reader::readTypeCode() {
  int index = -1;
  if (isRef_) {
    index = addRef(None::object());
  }
  int argcount = readLong();
  int kwonlyargcount = readLong();
  int nlocals = readLong();
  int stacksize = readLong();
  int flags = readLong();
  Object* code = readObject();
  Object* consts = readObject();
  Object* names = readObject();
  Object* varnames = readObject();
  Object* freevars = readObject();
  Object* cellvars = readObject();
  Object* filename = readObject();
  Object* name = readObject();
  int firstlineno = readLong();
  Object* lnotab = readObject();
  Object* result = runtime_->createCode(
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
    setRef(index, result);
  }
  return result;
}

Object* Marshal::Reader::readTypeRef() {
  int32 n = readLong();
  return getRef(n);
}

} // namespace python
