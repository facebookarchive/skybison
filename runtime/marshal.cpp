#include "marshal.h"

#include <cstdlib>
#include <cstring>

#include "heap.h"
#include "runtime.h"
#include "utils.h"

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

Marshal::Reader::Reader(
    HandleScope* scope,
    Runtime* runtime,
    const char* buffer)
    : runtime_(runtime),
      refs_(scope, runtime->newList()),
      start_(reinterpret_cast<const byte*>(buffer)),
      depth_(0),
      length_(std::strlen(buffer)),
      pos_(0) {
  end_ = start_ + length_;
}

const byte* Marshal::Reader::readBytes(int length) {
  const byte* result = &start_[pos_];
  pos_ += length;
  return result;
}

byte Marshal::Reader::readByte() {
  byte result = 0xFF;
  const byte* buffer = readBytes(1);
  if (buffer != nullptr) {
    result = buffer[0];
  }
  return result;
}

int16 Marshal::Reader::readShort() {
  int16 result = -1;
  const byte* buffer = readBytes(sizeof(result));
  if (buffer != nullptr) {
    result = buffer[0];
    result |= buffer[1] << 8;
  }
  return result;
}

int32 Marshal::Reader::readLong() {
  int32 result = -1;
  const byte* buffer = readBytes(4);
  if (buffer != nullptr) {
    result = buffer[0];
    result |= buffer[1] << 8;
    result |= buffer[2] << 16;
    result |= buffer[3] << 24;
  }
  return result;
}

double Marshal::Reader::readBinaryFloat() {
  const byte* buffer = readBytes(8);
  return *reinterpret_cast<const double*>(buffer);
}

template <typename T>
class ScopedCounter {
 public:
  explicit ScopedCounter(T* counter) : counter_(counter) {
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
      UNIMPLEMENTED("TYPE_STOPITER");
      break;

    case TYPE_ELLIPSIS:
      UNIMPLEMENTED("TYPE_ELLIPSIS");
      break;

    case TYPE_FALSE:
      result = Boolean::fromBool(false);
      break;

    case TYPE_TRUE:
      result = Boolean::fromBool(true);
      break;

    case TYPE_INT: {
      // NB: this will continue to work as long as SmallInteger can contain the
      // full range of 32 bit signed integer values. Notably, this will break if
      // we need to support 32 bit machines.
      word n = readLong();
      if (!SmallInteger::isValid(n)) {
        UNIMPLEMENTED("value '%ld' outside range supported by SmallInteger", n);
      }
      result = SmallInteger::fromWord(n);
      if (isRef_) {
        addRef(result);
      }
      break;
    }

    case TYPE_FLOAT:
      UNIMPLEMENTED("TYPE_FLOAT");
      break;

    case TYPE_BINARY_FLOAT: {
      double n = readBinaryFloat();
      result = runtime_->newDouble(n);
      if (isRef_) {
        addRef(result);
      }
      break;
    }

    case TYPE_COMPLEX:
      UNIMPLEMENTED("TYPE_COMPLEX");
      break;

    case TYPE_BINARY_COMPLEX: {
      double real = readBinaryFloat();
      double imag = readBinaryFloat();
      result = runtime_->newComplex(real, imag);
      if (isRef_) {
        addRef(result);
      }
      break;
    }

    case TYPE_STRING: // Misnomer, should be TYPE_BYTES
      result = readTypeString();
      break;

    case TYPE_INTERNED:
    case TYPE_ASCII_INTERNED:
      result = readTypeAsciiInterned();
      break;

    case TYPE_UNICODE:
    case TYPE_ASCII: {
      result = readTypeAscii();
      break;
    }

    case TYPE_SHORT_ASCII_INTERNED:
      result = readTypeShortAsciiInterned();
      break;

    case TYPE_SHORT_ASCII:
      result = readTypeShortAscii();
      break;

    case TYPE_SMALL_TUPLE:
      result = readTypeSmallTuple();
      break;

    case TYPE_TUPLE:
      result = readTypeTuple();
      break;

    case TYPE_LIST:
      UNIMPLEMENTED("TYPE_LIST");
      break;

    case TYPE_DICT:
      UNIMPLEMENTED("TYPE_DICT");
      break;

    case TYPE_SET:
      UNIMPLEMENTED("TYPE_SET");
      break;

    case TYPE_FROZENSET:
      UNIMPLEMENTED("TYPE_FROZENSET");
      break;

    case TYPE_CODE:
      result = readTypeCode();
      break;

    case TYPE_REF:
      result = readTypeRef();
      break;

    case TYPE_LONG:
      UNIMPLEMENTED("TYPE_LONG");
      break;

    default:
      UNREACHABLE("unknown type '%c' (flags=%x)", type, flag);
  }
  return result;
}

word Marshal::Reader::addRef(Object* value) {
  HandleScope scope;
  Handle<Object> valueHandle(&scope, value);
  word result = refs_->allocated();
  runtime_->listAdd(refs_, valueHandle);
  return result;
}

void Marshal::Reader::setRef(word index, Object* value) {
  refs_->atPut(index, value);
}

Object* Marshal::Reader::getRef(word index) {
  return refs_->at(index);
}

word Marshal::Reader::numRefs() {
  return refs_->allocated();
}

Object* Marshal::Reader::readTypeString() {
  int32 length = readLong();
  const byte* data = readBytes(length);
  Object* result = runtime_->newByteArrayWithAll(View<byte>(data, length));
  if (isRef_) {
    addRef(result);
  }
  return result;
}

Object* Marshal::Reader::readTypeAscii() {
  word length = readLong();
  if (length < 0) {
    return Thread::currentThread()->throwValueErrorFromCString(
        "bad marshal data (string size out of range)");
  }
  return readString(length);
}

Object* Marshal::Reader::readTypeAsciiInterned() {
  word length = readLong();
  if (length < 0) {
    return Thread::currentThread()->throwValueErrorFromCString(
        "bad marshal data (string size out of range)");
  }
  return readAndInternString(length);
}

Object* Marshal::Reader::readTypeShortAscii() {
  word length = readByte();
  return readString(length);
}

Object* Marshal::Reader::readTypeShortAsciiInterned() {
  word length = readByte();
  return readAndInternString(length);
}

Object* Marshal::Reader::readString(word length) {
  const byte* data = readBytes(length);
  Object* result = runtime_->newStringWithAll(View<byte>(data, length));
  if (isRef_) {
    addRef(result);
  }
  return result;
}

Object* Marshal::Reader::readAndInternString(word length) {
  const byte* data = readBytes(length);
  HandleScope scope;
  // TODO(T25820368): Intern strings iff the string isn't already part of the
  // intern table.
  Handle<Object> str(
      &scope, runtime_->newStringWithAll(View<byte>(data, length)));
  Object* result = runtime_->internString(str);
  if (isRef_) {
    addRef(result);
  }
  return result;
}

Object* Marshal::Reader::readTypeSmallTuple() {
  int32 n = readByte();
  return doTupleElements(n);
}

Object* Marshal::Reader::readTypeTuple() {
  int32 n = readLong();
  return doTupleElements(n);
}

Object* Marshal::Reader::doTupleElements(int32 length) {
  Object* result = runtime_->newObjectArray(length);
  if (isRef_) {
    addRef(result);
  }
  for (int32 i = 0; i < length; i++) {
    Object* value = readObject();
    ObjectArray::cast(result)->atPut(i, value);
  }
  return result;
}

Object* Marshal::Reader::readTypeCode() {
  word index = -1;
  if (isRef_) {
    index = addRef(None::object());
  }
  HandleScope scope;
  Handle<Code> result(&scope, runtime_->newCode());
  result->setArgcount(readLong());
  result->setKwonlyargcount(readLong());
  result->setNlocals(readLong());
  result->setStacksize(readLong());
  result->setFlags(readLong());
  result->setCode(readObject());
  result->setConsts(readObject());
  result->setNames(readObject());
  result->setVarnames(readObject());
  result->setFreevars(readObject());
  result->setCellvars(readObject());
  result->setFilename(readObject());
  result->setName(readObject());
  result->setFirstlineno(readLong());
  result->setLnotab(readObject());
  if (isRef_) {
    DCHECK(index != -1, "unexpected addRef result");
    setRef(index, *result);
  }
  return *result;
}

Object* Marshal::Reader::readTypeRef() {
  int32 n = readLong();
  return getRef(n);
}

} // namespace python
