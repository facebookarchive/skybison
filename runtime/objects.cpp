#include "objects.h"

#include "frame.h"
#include "runtime.h"
#include "thread.h"

#include <cstring>

namespace python {

// RawSmallStr

RawObject RawSmallStr::fromCStr(const char* value) {
  word len = strlen(value);
  return fromBytes(View<byte>(reinterpret_cast<const byte*>(value), len));
}

RawObject RawSmallStr::fromBytes(View<byte> data) {
  word length = data.length();
  DCHECK_BOUND(length, kMaxLength);
  uword result = 0;
  for (word i = length; i > 0;) {
    i -= 1;
    result = (result << kBitsPerByte) | data.get(i);
  }
  result = (result << kBitsPerByte) | (length << kTagSize) | kTag;
  return cast(RawObject{result});
}

char* RawSmallStr::toCStr() const {
  word length = this->length();
  byte* result = static_cast<byte*>(std::malloc(length + 1));
  CHECK(result != nullptr, "out of memory");
  copyTo(result, length);
  result[length] = '\0';
  return reinterpret_cast<char*>(result);
}

// RawSmallInt

const word RawSmallInt::kMinValue;
const word RawSmallInt::kMaxValue;

// RawByteArray

void RawByteArray::downsize(word new_length) const {
  word original_length = numItems();
  DCHECK_BOUND(new_length, original_length);
  byte* dst = reinterpret_cast<byte*>(RawBytes::cast(bytes()).address());
  std::memset(dst + new_length, 0, original_length - new_length);
  setNumItems(new_length);
}

// RawBytes

word RawBytes::compare(RawBytes that) const {
  word this_len = this->length();
  word that_len = that->length();
  word len = Utils::minimum(this_len, that_len);
  auto b1 = reinterpret_cast<void*>(this->address());
  auto b2 = reinterpret_cast<void*>(that->address());
  word diff = std::memcmp(b1, b2, len);
  if (diff != 0 || this_len == that_len) return diff;
  return this_len < that_len ? -1 : 1;
}

// RawLargeStr

bool RawLargeStr::equals(RawObject that) const {
  if (*this == that) {
    return true;
  }
  if (!that->isLargeStr()) {
    return false;
  }
  auto that_str = RawLargeStr::cast(that);
  if (length() != that_str->length()) {
    return false;
  }
  auto s1 = reinterpret_cast<void*>(address());
  auto s2 = reinterpret_cast<void*>(that_str->address());
  return std::memcmp(s1, s2, length()) == 0;
}

void RawLargeStr::copyTo(byte* dst, word length) const {
  DCHECK_BOUND(length, this->length());
  std::memcpy(dst, reinterpret_cast<const byte*>(address()), length);
}

char* RawLargeStr::toCStr() const {
  word length = this->length();
  byte* result = static_cast<byte*>(std::malloc(length + 1));
  CHECK(result != nullptr, "out of memory");
  copyTo(result, length);
  result[length] = '\0';
  return reinterpret_cast<char*>(result);
}

// RawInt

word RawInt::compare(RawInt that) const {
  if (this->isSmallInt() && that->isSmallInt()) {
    return this->asWord() - that->asWord();
  }
  // compare with large ints always returns -1, 0, or 1
  if (this->isNegative() != that->isNegative()) {
    return this->isNegative() ? -1 : 1;
  }

  word left_digits = this->numDigits();
  word right_digits = that->numDigits();

  if (left_digits > right_digits) {
    return 1;
  }
  if (left_digits < right_digits) {
    return -1;
  }
  for (word i = left_digits - 1; i >= 0; i--) {
    uword left_digit = this->digitAt(i);
    uword right_digit = that->digitAt(i);
    if (left_digit > right_digit) {
      return 1;
    }
    if (left_digit < right_digit) {
      return -1;
    }
  }
  return 0;
}

word RawInt::copyTo(byte* dst, word max_length) const {
  if (isLargeInt()) {
    return RawLargeInt::cast(*this)->copyTo(dst, max_length);
  }

  DCHECK(isSmallInt() || isBool(), "not an integer");
  uword val = isSmallInt() ? RawSmallInt::cast(*this)->value()
                           : RawBool::cast(*this)->value();
  word memcpy_length = std::min(word{kWordSize}, max_length);
  std::memcpy(dst, &val, memcpy_length);
  return memcpy_length;
}

// RawLargeInt

bool RawLargeInt::isValid() const {
  word digits = numDigits();
  if (digits <= 0) {
    return false;
  }
  if (digits == 1) {
    // Enforce a canonical representation for all ints.
    return !RawSmallInt::isValid(digitAt(0));
  }

  word high_digit = digitAt(digits - 1);
  word next_digit = digitAt(digits - 2);

  // Redundant sign-extension for negative values.
  if (high_digit == -1 && next_digit < 0) {
    return false;
  }

  // Redundant zero-extension for positive values.
  if (high_digit == 0 && next_digit >= 0) {
    return false;
  }

  return true;
}

word RawLargeInt::bitLength() const {
  word num_digits = numDigits();
  word high_digit = digitAt(num_digits - 1);

  if (high_digit < 0) {
    // We're negative. Calculate what high_digit would be after negation.
    uword carry = [&] {
      for (word i = 0; i < num_digits - 1; i++) {
        if (digitAt(i) != 0) return 0;
      }
      return 1;
    }();
    high_digit = ~high_digit + carry;
  }
  return (num_digits - 1) * kBitsPerWord + Utils::highestBit(high_digit);
}

word RawLargeInt::copyTo(byte* dst, word copy_length) const {
  word length = numDigits() * kWordSize;
  auto digits = reinterpret_cast<uword*>(address() + kValueOffset);
  word memcpy_size = std::min(length, copy_length);
  std::memcpy(dst, digits, memcpy_size);
  return memcpy_size;
}

void RawLargeInt::copyFrom(View<byte> bytes, byte sign_extension) const {
  auto dst = reinterpret_cast<char*>(address() + kValueOffset);
  word bytes_len = bytes.length();
  DCHECK(bytes_len <= numDigits() * kWordSize, "too many bytes");
  std::memcpy(dst, bytes.data(), bytes_len);
  std::memset(dst + bytes_len, sign_extension,
              (numDigits() * kWordSize) - bytes_len);
}

// RawTuple

bool RawTuple::contains(RawObject object) const {
  word len = length();
  for (word i = 0; i < len; i++) {
    if (at(i) == object) {
      return true;
    }
  }
  return false;
}

void RawTuple::copyTo(RawObject array) const {
  RawTuple dst = RawTuple::cast(array);
  word len = length();
  DCHECK_BOUND(len, dst->length());
  for (word i = 0; i < len; i++) {
    RawObject elem = at(i);
    dst->atPut(i, elem);
  }
}

void RawTuple::replaceFromWith(word start, RawObject array) const {
  RawTuple src = RawTuple::cast(array);
  word count = Utils::minimum(this->length() - start, src->length());
  word stop = start + count;
  for (word i = start, j = 0; i < stop; i++, j++) {
    atPut(i, src->at(j));
  }
}

// RawRangeIterator

bool RawRangeIterator::isOutOfRange(word cur, word stop, word step) {
  DCHECK(step != 0,
         "invalid step");  // should have been checked in Builtins::range().

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

word RawRangeIterator::pendingLength() const {
  RawRange range = RawRange::cast(instanceVariableAt(kRangeOffset));
  word stop = range->stop();
  word step = range->step();
  word current =
      RawSmallInt::cast(instanceVariableAt(kCurValueOffset))->value();
  if (isOutOfRange(current, stop, step)) {
    return 0;
  }
  return std::abs((stop - current) / step);
}

RawObject RawRangeIterator::next() const {
  RawSmallInt ret = RawSmallInt::cast(instanceVariableAt(kCurValueOffset));
  word cur = ret->value();

  RawRange range = RawRange::cast(instanceVariableAt(kRangeOffset));
  word stop = range->stop();
  word step = range->step();

  // TODO(jeethu): range overflow is unchecked. Since a correct implementation
  // has to support arbitrary precision anyway, there's no point in checking
  // for overflow.
  if (isOutOfRange(cur, stop, step)) {
    // TODO(bsimmers): Use RawStopIteration for control flow.
    return RawError::object();
  }

  instanceVariableAtPut(kCurValueOffset, RawSmallInt::fromWord(cur + step));
  return ret;
}

// RawSlice

void RawSlice::unpack(word* start, word* stop, word* step) const {
  if (this->step()->isNoneType()) {
    *step = 1;
  } else {
    // TODO(T27897506): CPython uses _PyEval_SliceIndex to convert any
    //       integer to eval any object into a valid index. For now, it'll
    //       assume that all indices are SmallInts.
    *step = RawSmallInt::cast(this->step())->value();
    if (*step == 0) {
      UNIMPLEMENTED("Throw ValueError. slice step cannot be zero");
    }
    // Here *step might be -RawSmallInt::kMaxValue-1; in this case we replace
    // it with -RawSmallInt::kMaxValue.  This doesn't affect the semantics,
    // and it guards against later undefined behaviour resulting from code that
    // does "step = -step" as part of a slice reversal.
    if (*step < -RawSmallInt::kMaxValue) {
      *step = -RawSmallInt::kMaxValue;
    }
  }

  if (this->start()->isNoneType()) {
    *start = (*step < 0) ? RawSmallInt::kMaxValue : 0;
  } else {
    *start = RawSmallInt::cast(this->start())->value();
  }

  if (this->stop()->isNoneType()) {
    *stop = (*step < 0) ? RawSmallInt::kMinValue : RawSmallInt::kMaxValue;
  } else {
    *stop = RawSmallInt::cast(this->stop())->value();
  }
}

word RawSlice::adjustIndices(word length, word* start, word* stop, word step) {
  DCHECK(step != 0, "Step should be non zero");

  if (*start < 0) {
    *start += length;
    if (*start < 0) {
      *start = (step < 0) ? -1 : 0;
    }
  } else if (*start >= length) {
    *start = (step < 0) ? length - 1 : length;
  }

  if (*stop < 0) {
    *stop += length;
    if (*stop < 0) {
      *stop = (step < 0) ? -1 : 0;
    }
  } else if (*stop >= length) {
    *stop = (step < 0) ? length - 1 : length;
  }

  if (step < 0) {
    if (*stop < *start) {
      return (*start - *stop - 1) / (-step) + 1;
    }
  } else {
    if (*start < *stop) {
      return (*stop - *start - 1) / step + 1;
    }
  }
  return 0;
}

// RawStr

word RawStr::compare(RawObject string) const {
  RawStr that = RawStr::cast(string);
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

word RawStr::compareCStr(const char* c_str) const {
  word c_length = std::strlen(c_str);
  word length = Utils::minimum(this->length(), c_length);
  for (word i = 0; i < length; i++) {
    word diff = charAt(i) - static_cast<byte>(c_str[i]);
    if (diff != 0) {
      return (diff > 0) ? 1 : -1;
    }
  }
  word diff = this->length() - c_length;
  return (diff > 0) ? 1 : ((diff < 0) ? -1 : 0);
}

bool RawStr::equalsCStr(const char* c_str) const {
  const char* cp = c_str;
  const word len = length();
  for (word i = 0; i < len; i++, cp++) {
    char ch = *cp;
    if (ch == '\0' || ch != charAt(i)) {
      return false;
    }
  }
  return *cp == '\0';
}

// RawWeakRef

void RawWeakRef::enqueueReference(RawObject reference, RawObject* tail) {
  if (*tail == RawNoneType::object()) {
    RawWeakRef::cast(reference)->setLink(reference);
  } else {
    RawObject head = RawWeakRef::cast(*tail)->link();
    RawWeakRef::cast(*tail)->setLink(reference);
    RawWeakRef::cast(reference)->setLink(head);
  }
  *tail = reference;
}

RawObject RawWeakRef::dequeueReference(RawObject* tail) {
  DCHECK(*tail != RawNoneType::object(), "empty queue");
  RawObject head = RawWeakRef::cast(*tail)->link();
  if (head == *tail) {
    *tail = RawNoneType::object();
  } else {
    RawObject next = RawWeakRef::cast(head)->link();
    RawWeakRef::cast(*tail)->setLink(next);
  }
  RawWeakRef::cast(head)->setLink(RawNoneType::object());
  return head;
}

// Append tail2 to tail1 and return the new tail.
RawObject RawWeakRef::spliceQueue(RawObject tail1, RawObject tail2) {
  if (tail1 == RawNoneType::object() && tail2 == RawNoneType::object()) {
    return RawNoneType::object();
  }
  if (tail1 == RawNoneType::object()) {
    return tail2;
  }
  if (tail2 == RawNoneType::object()) {
    return tail1;
  }
  // merge two list, tail1 -> head2 -> ... -> tail2 -> head1
  RawObject head1 = RawWeakRef::cast(tail1)->link();
  RawObject head2 = RawWeakRef::cast(tail2)->link();
  RawWeakRef::cast(tail1)->setLink(head2);
  RawWeakRef::cast(tail2)->setLink(head1);
  return tail2;
}

// RawHeapFrame

word RawHeapFrame::numAttributes(word extra_words) {
  return kNumOverheadWords + Frame::kSize / kPointerSize + extra_words;
}

}  // namespace python
