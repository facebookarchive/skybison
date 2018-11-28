#include "marshal.h"

#include <cstdlib>
#include <cstring>
#include <memory>

#include "heap.h"
#include "runtime.h"
#include "utils.h"
#include "view.h"

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

Marshal::Reader::Reader(HandleScope* scope, Runtime* runtime,
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
  double result;
  const byte* buffer = readBytes(sizeof(result));
  std::memcpy(&result, buffer, sizeof(result));
  return result;
}

template <typename T>
class ScopedCounter {
 public:
  explicit ScopedCounter(T* counter) : counter_(counter) { (*counter_)++; }
  ~ScopedCounter() { (*counter_)--; }

 public:
  T* counter_;
  DISALLOW_COPY_AND_ASSIGN(ScopedCounter);
};

RawObject Marshal::Reader::readObject() {
  ScopedCounter<int> counter(&depth_);
  RawObject result;
  byte code = readByte();
  byte flag = code & FLAG_REF;
  byte type = code & ~FLAG_REF;
  isRef_ = flag;
  switch (type) {
    case TYPE_NULL:
      break;

    case TYPE_NONE:
      result = NoneType::object();
      break;

    case TYPE_STOPITER:
      UNIMPLEMENTED("TYPE_STOPITER");
      break;

    case TYPE_ELLIPSIS:
      UNIMPLEMENTED("TYPE_ELLIPSIS");
      break;

    case TYPE_FALSE:
      result = Bool::falseObj();
      break;

    case TYPE_TRUE:
      result = Bool::trueObj();
      break;

    case TYPE_INT: {
      // NB: this will continue to work as long as SmallInt can contain the
      // full range of 32 bit signed integer values. Notably, this will break if
      // we need to support 32 bit machines.
      word n = readLong();
      if (!SmallInt::isValid(n)) {
        UNIMPLEMENTED("value '%ld' outside range supported by RawSmallInt", n);
      }
      result = SmallInt::fromWord(n);
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
      result = runtime_->newFloat(n);
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

    case TYPE_STRING:  // Misnomer, should be TYPE_BYTES
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
      result = readLongObject();
      break;

    default:
      UNREACHABLE("unknown type '%c' (flags=%x)", type, flag);
  }
  return result;
}

word Marshal::Reader::addRef(RawObject value) {
  HandleScope scope;
  Object value_handle(&scope, value);
  word result = refs_->numItems();
  runtime_->listAdd(refs_, value_handle);
  return result;
}

void Marshal::Reader::setRef(word index, RawObject value) {
  refs_->atPut(index, value);
}

RawObject Marshal::Reader::getRef(word index) { return refs_->at(index); }

word Marshal::Reader::numRefs() { return refs_->numItems(); }

RawObject Marshal::Reader::readTypeString() {
  int32 length = readLong();
  const byte* data = readBytes(length);
  RawObject result = runtime_->newBytesWithAll(View<byte>(data, length));
  if (isRef_) {
    addRef(result);
  }
  return result;
}

RawObject Marshal::Reader::readTypeAscii() {
  word length = readLong();
  if (length < 0) {
    return Thread::currentThread()->raiseValueErrorWithCStr(
        "bad marshal data (string size out of range)");
  }
  return readStr(length);
}

RawObject Marshal::Reader::readTypeAsciiInterned() {
  word length = readLong();
  if (length < 0) {
    return Thread::currentThread()->raiseValueErrorWithCStr(
        "bad marshal data (string size out of range)");
  }
  return readAndInternStr(length);
}

RawObject Marshal::Reader::readTypeShortAscii() {
  word length = readByte();
  return readStr(length);
}

RawObject Marshal::Reader::readTypeShortAsciiInterned() {
  word length = readByte();
  return readAndInternStr(length);
}

RawObject Marshal::Reader::readStr(word length) {
  const byte* data = readBytes(length);
  RawObject result = runtime_->newStrWithAll(View<byte>(data, length));
  if (isRef_) {
    addRef(result);
  }
  return result;
}

RawObject Marshal::Reader::readAndInternStr(word length) {
  const byte* data = readBytes(length);
  HandleScope scope;
  // TODO(T25820368): Intern strings iff the string isn't already part of the
  // intern table.
  Object str(&scope, runtime_->newStrWithAll(View<byte>(data, length)));
  RawObject result = runtime_->internStr(str);
  if (isRef_) {
    addRef(result);
  }
  return result;
}

RawObject Marshal::Reader::readTypeSmallTuple() {
  int32 n = readByte();
  return doTupleElements(n);
}

RawObject Marshal::Reader::readTypeTuple() {
  int32 n = readLong();
  return doTupleElements(n);
}

RawObject Marshal::Reader::doTupleElements(int32 length) {
  RawObject result = runtime_->newTuple(length);
  if (isRef_) {
    addRef(result);
  }
  for (int32 i = 0; i < length; i++) {
    RawObject value = readObject();
    RawTuple::cast(result)->atPut(i, value);
  }
  return result;
}

RawObject Marshal::Reader::readTypeCode() {
  word index = -1;
  if (isRef_) {
    index = addRef(NoneType::object());
  }
  HandleScope scope;
  Code result(&scope, runtime_->newCode());
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

RawObject Marshal::Reader::readTypeRef() {
  int32 n = readLong();
  return getRef(n);
}

RawObject Marshal::Reader::readLongObject() {
  int32 n = readLong();
  if (n == 0) {
    RawObject zero = SmallInt::fromWord(0);
    if (isRef_) {
      addRef(zero);
    }
    return zero;
  }
  if (n < kMinInt32 || n > kMaxInt32) {
    return Thread::currentThread()->raiseValueErrorWithCStr(
        "bad marshal data (string size out of range)");
  }
  word bits_consumed = 0;
  word n_bits = std::abs(n) * kBitsPerLongDigit;
  word num_digits = ((n_bits + kBitsPerWord + 1) / kBitsPerWord) + 1;
  std::unique_ptr<uword[]> digits{new uword[num_digits]};
  word digits_idx = 0;
  uword buf = 0;
  word word_offset = 0;
  while (bits_consumed < n_bits) {
    int16 digit = readShort();
    if (digit < 0) {
      return Thread::currentThread()->raiseValueErrorWithCStr(
          "bad marshal data (negative long digit)");
    }
    auto unsigned_digit = static_cast<uword>(digit);
    if (word_offset + kBitsPerLongDigit <= kBitsPerWord) {
      buf |= unsigned_digit << word_offset;
      word_offset += kBitsPerLongDigit;
      if (word_offset == kBitsPerWord) {
        digits[digits_idx++] = buf;
        buf = 0;
        word_offset = 0;
      }
    } else {
      word extra_bits = (word_offset + kBitsPerLongDigit) % kBitsPerWord;
      word bits_to_include = kBitsPerLongDigit - extra_bits;
      buf |= (unsigned_digit & ((1 << bits_to_include) - 1)) << word_offset;
      digits[digits_idx++] = buf;
      buf = (unsigned_digit >> bits_to_include) & ((1 << extra_bits) - 1);
      word_offset = extra_bits;
    }
    bits_consumed += kBitsPerLongDigit;
  }
  if (word_offset > 0 && buf != 0) {
    digits[digits_idx++] = buf;
  } else if ((digits[digits_idx - 1] >> (kBitsPerWord - 1)) &&
             (n > 0 || digits[digits_idx - 1] << 1)) {
    // Zero extend if the MSB is set in the top digit and either the result is
    // positive or the top digit has at least one other bit set (in which case
    // we need the extra digit for the negation).
    digits[digits_idx++] = 0;
  }
  if (n < 0) {
    uword carry = 1;
    for (word i = 0; i < digits_idx; i++) {
      uword digit = digits[i];
      carry = __builtin_uaddl_overflow(~digit, carry, &digit);
      digits[i] = digit;
    }
    DCHECK(carry == 0, "Carry should be zero");
  }

  RawObject result =
      runtime_->newIntWithDigits(View<uword>(digits.get(), digits_idx));
  if (isRef_) {
    addRef(result);
  }
  return result;
}

}  // namespace python
