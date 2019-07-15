#include "objects.h"

#include "frame.h"
#include "runtime.h"
#include "thread.h"

#include <cstring>

namespace python {

// RawSmallBytes

RawObject RawSmallBytes::fromBytes(View<byte> data) {
  word length = data.length();
  DCHECK_BOUND(length, kMaxLength);
  uword result = 0;
  for (word i = length - 1; i >= 0; i--) {
    result = (result << kBitsPerByte) | data.get(i);
  }
  return RawObject{result << kBitsPerByte | length << kImmediateTagBits |
                   kSmallBytesTag};
}

// RawSmallStr

RawObject RawSmallStr::fromCodePoint(int32_t code_point) {
  DCHECK_BOUND(code_point, kMaxUnicode);
  uword cp = static_cast<uword>(code_point);
  // 0xxxxxxx
  if (cp <= kMaxASCII) {  // 01111111
    return RawObject{cp << 8 | (1 << kImmediateTagBits) | kSmallStrTag};
  }
  uword result = cp & 0x3F;  // 00111111
  cp >>= 6;
  result <<= kBitsPerByte;
  // 110xxxxx 10xxxxxx
  if (cp <= 0x1F) {  // 00011111
    result |= cp;
    result |= 0x80C0;  // 10xxxxxx 110xxxxx
    result <<= kBitsPerByte;
    return RawObject{result | (2 << kImmediateTagBits) | kSmallStrTag};
  }

  result |= cp & 0x3F;  // 00111111
  cp >>= 6;
  result <<= kBitsPerByte;
  // 1110xxxx 10xxxxxx 10xxxxxx
  if (cp <= 0xF) {  // 00001111
    result |= cp;
    result |= 0x8080E0;  // 10xxxxxx 10xxxxxx 1110xxxx
    result <<= kBitsPerByte;
    return RawObject{result | (3 << kImmediateTagBits) | kSmallStrTag};
  }
  result |= cp & 0x3F;  // 00111111
  cp >>= 6;
  result <<= kBitsPerByte;
  // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
  result |= cp;
  result |= 0x808080F0;  // 10xxxxxx 10xxxxxx 10xxxxxx 11110xxx
  result <<= kBitsPerByte;
  return RawObject{result | (4 << kImmediateTagBits) | kSmallStrTag};
}

RawObject RawSmallStr::fromCStr(const char* value) {
  word len = strlen(value);
  return fromBytes(View<byte>(reinterpret_cast<const byte*>(value), len));
}

RawObject RawSmallStr::fromBytes(View<byte> data) {
  word length = data.length();
  DCHECK_BOUND(length, kMaxLength);
  uword result = 0;
  for (word i = length - 1; i >= 0; i--) {
    result = (result << kBitsPerByte) | data.get(i);
  }
  return RawObject{result << kBitsPerByte | length << kImmediateTagBits |
                   kSmallStrTag};
}

char* RawSmallStr::toCStr() const {
  word length = this->length();
  byte* result = static_cast<byte*>(std::malloc(length + 1));
  CHECK(result != nullptr, "out of memory");
  copyTo(result, length);
  result[length] = '\0';
  return reinterpret_cast<char*>(result);
}

word RawSmallStr::codePointLength() const {
  uword block = raw() >> kBitsPerByte;
  uword mask_0 = ~uword{0} / 0xFF;  // 0x010101...
  uword mask_7 = mask_0 << 7;       // 0x808080...
  block = ((block & mask_7) >> 7) & ((~block) >> 6);
  // TODO(cshapiro): evaluate using popcount instead of multiplication
  word num_trailing = (block * mask_0) >> ((kWordSize - 1) * kBitsPerByte);
  return length() - num_trailing;
}

// RawSmallInt

const word RawSmallInt::kMinValue;
const word RawSmallInt::kMaxValue;

// RawByteArray

word RawByteArray::compare(RawBytes that, word that_len) {
  DCHECK_BOUND(that_len, that.length());
  word this_len = this->numItems();
  word len = Utils::minimum(this_len, that_len);
  for (word i = 0; i < len; i++) {
    word diff = this->byteAt(i) - that.byteAt(i);
    if (diff != 0) return diff;
  }
  return this_len - that_len;
}

void RawByteArray::downsize(word new_length) const {
  word original_length = numItems();
  DCHECK_BOUND(new_length, original_length);
  if (original_length == 0) return;
  byte* dst = reinterpret_cast<byte*>(RawLargeBytes::cast(bytes()).address());
  std::memset(dst + new_length, 0, original_length - new_length);
  setNumItems(new_length);
}

// RawBytes

word RawBytes::compare(RawBytes that) const {
  word this_len = this->length();
  word that_len = that.length();
  word len = Utils::minimum(this_len, that_len);
  for (word i = 0; i < len; i++) {
    word diff = this->byteAt(i) - that.byteAt(i);
    if (diff != 0) return diff;
  }
  return this_len - that_len;
}

// RawLargeStr

bool RawLargeStr::equals(RawObject that) const {
  if (!that.isLargeStr()) {
    return false;
  }
  auto that_str = RawLargeStr::cast(that);
  if (length() != that_str.length()) {
    return false;
  }
  auto s1 = reinterpret_cast<void*>(address());
  auto s2 = reinterpret_cast<void*>(that_str.address());
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

word RawLargeStr::codePointLength() const {
  // This is a vectorized loop for processing code units in groups the size of a
  // machine word.  The garbage collector ensures the following invariants that
  // simplify the algorithm, eliminating the need for a scalar pre-loop or a
  // scalar-post loop:
  //
  //   1) The base address of instance data is always word aligned
  //   2) The allocation sizes are always rounded-up to the next word
  //   3) Unused bytes at the end of an allocation are always zero
  //
  // This algorithm works by counting the number of UTF-8 trailing bytes found
  // in the string from the total number of byte in the string.  Because the
  // unused bytes at the end of a string are zero they are conveniently ignored
  // by the counting.
  word length = this->length();
  word size_in_words = (length + kWordSize - 1) >> kWordSizeLog2;
  word result = length;
  const uword* data = reinterpret_cast<const uword*>(address());
  uword mask_0 = ~uword{0} / 0xFF;  // 0x010101...
  uword mask_7 = mask_0 << 7;       // 0x808080...
  for (word i = 0; i < size_in_words; i++) {
    // Read an entire word of code units.
    uword block = data[i];
    // The bit pattern 0b10xxxxxx identifies a UTF-8 trailing byte.  For each
    // byte in a word, we isolate bit 6 and 7 and logically and the complement
    // of bit 6 with bit 7.  That leaves one set bit for each trailing byte in a
    // word.
    block = ((block & mask_7) >> 7) & ((~block) >> 6);
    // Count the number of bits leftover in the word.  That is equal to the
    // number of trailing bytes.
    // TODO(cshapiro): evaluate using popcount instead of multiplication
    word num_trailing = (block * mask_0) >> ((kWordSize - 1) * kBitsPerByte);
    // Finally, subtract the number of trailing bytes from the number of bytes
    // in the string leaving just the number of ASCII code points and UTF-8
    // leading bytes in the count.
    result -= num_trailing;
  }
  return result;
}

// RawInt

word RawInt::compare(RawInt that) const {
  if (this->isSmallInt() && that.isSmallInt()) {
    return this->asWord() - that.asWord();
  }
  // compare with large ints always returns -1, 0, or 1
  bool is_negative = this->isNegative();
  if (is_negative != that.isNegative()) {
    return is_negative ? -1 : 1;
  }

  word left_digits = this->numDigits();
  word right_digits = that.numDigits();

  if (left_digits > right_digits) {
    return is_negative ? -1 : 1;
  }
  if (left_digits < right_digits) {
    return is_negative ? 1 : -1;
  }
  for (word i = left_digits - 1; i >= 0; i--) {
    uword left_digit = this->digitAt(i);
    uword right_digit = that.digitAt(i);
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
    return RawLargeInt::cast(*this).copyTo(dst, max_length);
  }

  DCHECK(isSmallInt() || isBool(), "not an integer");
  uword val = isSmallInt() ? RawSmallInt::cast(*this).value()
                           : RawBool::cast(*this).value();
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

void RawLargeInt::copyFrom(RawBytes bytes, byte sign_extension) const {
  auto dst = reinterpret_cast<byte*>(address() + kValueOffset);
  word bytes_len = bytes.length();
  DCHECK(bytes_len <= numDigits() * kWordSize, "too many bytes");
  bytes.copyTo(dst, bytes_len);
  std::memset(dst + bytes_len, sign_extension,
              (numDigits() * kWordSize) - bytes_len);
}

// RawMutableBytes

word RawMutableBytes::compareWithBytes(View<byte> that) {
  word this_len = this->length();
  word that_len = that.length();
  word len = Utils::minimum(this_len, that_len);
  for (word i = 0; i < len; i++) {
    word diff = this->byteAt(i) - that.get(i);
    if (diff != 0) return diff;
  }
  return this_len - that_len;
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

void RawTuple::copyTo(RawObject dst) const {
  RawTuple dst_tuple = RawTuple::cast(dst);
  word len = length();
  DCHECK_BOUND(len, dst_tuple.length());
  for (word i = 0; i < len; i++) {
    RawObject elem = at(i);
    dst_tuple.atPut(i, elem);
  }
}

void RawTuple::fill(RawObject value) const {
  DCHECK(header().hashCode() == RawHeader::kUninitializedHash,
         "tuple has been hashed and cannot be modified");
  if (value.isNoneType()) {
    std::memset(reinterpret_cast<byte*>(address()), -1, length() * kWordSize);
    return;
  }
  for (word i = 0; i < length(); i++) {
    atPut(i, value);
  }
}

void RawTuple::replaceFromWith(word start, RawObject array) const {
  RawTuple src = RawTuple::cast(array);
  word count = Utils::minimum(this->length() - start, src.length());
  word stop = start + count;
  for (word i = start, j = 0; i < stop; i++, j++) {
    atPut(i, src.at(j));
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
  word stop = range.stop();
  word step = range.step();
  word current = RawSmallInt::cast(instanceVariableAt(kCurValueOffset)).value();
  if (isOutOfRange(current, stop, step)) {
    return 0;
  }
  return std::abs((stop - current) / step);
}

RawObject RawRangeIterator::next() const {
  RawSmallInt ret = RawSmallInt::cast(instanceVariableAt(kCurValueOffset));
  word cur = ret.value();

  RawRange range = RawRange::cast(instanceVariableAt(kRangeOffset));
  word stop = range.stop();
  word step = range.step();

  // TODO(jeethu): range overflow is unchecked. Since a correct implementation
  // has to support arbitrary precision anyway, there's no point in checking
  // for overflow.
  if (isOutOfRange(cur, stop, step)) {
    return RawError::noMoreItems();
  }

  instanceVariableAtPut(kCurValueOffset, RawSmallInt::fromWord(cur + step));
  return ret;
}

// RawSlice

void RawSlice::unpack(word* start, word* stop, word* step) const {
  if (this->step().isNoneType()) {
    *step = 1;
  } else {
    // TODO(T27897506): CPython uses _PyEval_SliceIndex to convert any
    //       integer to eval any object into a valid index. For now, it'll
    //       assume that all indices are SmallInts.
    *step = RawSmallInt::cast(this->step()).value();
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

  if (this->start().isNoneType()) {
    *start = (*step < 0) ? RawSmallInt::kMaxValue : 0;
  } else {
    *start = RawSmallInt::cast(this->start()).value();
  }

  if (this->stop().isNoneType()) {
    *stop = (*step < 0) ? RawSmallInt::kMinValue : RawSmallInt::kMaxValue;
  } else {
    *stop = RawSmallInt::cast(this->stop()).value();
  }
}

word RawSlice::length(word start, word stop, word step) {
  if (step < 0) {
    if (stop < start) {
      return (start - stop - 1) / (-step) + 1;
    }
  } else if (start < stop) {
    return (stop - start - 1) / step + 1;
  }
  return 0;
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

  return RawSlice::length(*start, *stop, step);
}

void RawSlice::adjustSearchIndices(word* start, word* end, word length) {
  if (*start < 0) {
    *start = Utils::maximum(*start + length, word{0});
  }
  if (*end < 0) {
    *end = Utils::maximum(*end + length, word{0});
  } else if (*end > length) {
    *end = length;
  }
}

// RawStr

word RawStr::compare(RawObject string) const {
  RawStr that = RawStr::cast(string);
  word length = Utils::minimum(this->length(), that.length());
  for (word i = 0; i < length; i++) {
    word diff = this->charAt(i) - that.charAt(i);
    if (diff != 0) {
      return (diff > 0) ? 1 : -1;
    }
  }
  word diff = this->length() - that.length();
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
    byte ch = static_cast<byte>(*cp);
    if (ch == '\0' || ch != charAt(i)) {
      return false;
    }
  }
  return *cp == '\0';
}

template <typename T, typename F>
static inline int32_t decodeCodePoint(T src, F at, word src_length, word index,
                                      word* length) {
  DCHECK_INDEX(index, src_length);
  byte ch0 = (src->*at)(index);
  if (ch0 <= kMaxASCII) {
    *length = 1;
    return ch0;
  }
  DCHECK_INDEX(index + 1, src_length);
  byte ch1 = (src->*at)(index + 1) & byte{0x3F};
  if ((ch0 & 0xE0) == 0xC0) {
    *length = 2;
    return ((ch0 & 0x1F) << 6) | ch1;
  }
  DCHECK_INDEX(index + 2, src_length);
  byte ch2 = (src->*at)(index + 2) & byte{0x3F};
  if ((ch0 & 0xF0) == 0xE0) {
    *length = 3;
    return ((ch0 & 0xF) << 12) | (ch1 << 6) | ch2;
  }
  DCHECK((ch0 & 0xF8) == 0xF0, "invalid code unit");
  DCHECK_INDEX(index + 2, src_length);
  byte ch3 = (src->*at)(index + 3) & byte{0x3F};
  *length = 4;
  return ((ch0 & 0x7) << 18) | (ch1 << 12) | (ch2 << 6) | ch3;
}

int32_t RawStr::codePointAt(word index, word* length) const {
  return decodeCodePoint(this, &RawStr::charAt, this->length(), index, length);
}

word RawStr::offsetByCodePoints(word index, word count) const {
  word len = length();
  while (count-- && index < len) {
    byte ch = charAt(index);
    if (ch <= kMaxASCII) {
      index++;
    } else if ((ch & 0xE0) == 0xC0) {
      index += 2;
    } else if ((ch & 0xF0) == 0xE0) {
      index += 3;
    } else {
      DCHECK((ch & 0xF8) == 0xF0, "invalid code unit");
      index += 4;
    }
  }
  return Utils::minimum(index, len);
}

// RawStrArray

int32_t RawStrArray::codePointAt(word index, word* length) const {
  RawMutableBytes buffer = RawMutableBytes::cast(items());
  return decodeCodePoint(&buffer, &RawMutableBytes::byteAt, numItems(), index,
                         length);
}

// RawWeakRef

void RawWeakRef::enqueueReference(RawObject reference, RawObject* tail) {
  if (*tail == RawNoneType::object()) {
    RawWeakRef::cast(reference).setLink(reference);
  } else {
    RawObject head = RawWeakRef::cast(*tail).link();
    RawWeakRef::cast(*tail).setLink(reference);
    RawWeakRef::cast(reference).setLink(head);
  }
  *tail = reference;
}

RawObject RawWeakRef::dequeueReference(RawObject* tail) {
  DCHECK(*tail != RawNoneType::object(), "empty queue");
  RawObject head = RawWeakRef::cast(*tail).link();
  if (head == *tail) {
    *tail = RawNoneType::object();
  } else {
    RawObject next = RawWeakRef::cast(head).link();
    RawWeakRef::cast(*tail).setLink(next);
  }
  RawWeakRef::cast(head).setLink(RawNoneType::object());
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
  RawObject head1 = RawWeakRef::cast(tail1).link();
  RawObject head2 = RawWeakRef::cast(tail2).link();
  RawWeakRef::cast(tail1).setLink(head2);
  RawWeakRef::cast(tail2).setLink(head1);
  return tail2;
}

// RawHeapFrame

word RawHeapFrame::numAttributes(word extra_words) {
  return kNumOverheadWords + Frame::kSize / kPointerSize + extra_words;
}

word RawHeapFrame::virtualPC() const { return frame()->virtualPC(); }

void RawHeapFrame::setVirtualPC(word value) const {
  return frame()->setVirtualPC(value);
}

RawObject* RawHeapFrame::valueStackTop() const {
  return frame()->stashedValueStackTop();
}

RawObject RawHeapFrame::popValue() const { return frame()->stashedPopValue(); }

void RawHeapFrame::stashInternalPointers(Frame* original_frame) const {
  frame()->stashInternalPointers(original_frame);
}

}  // namespace python
