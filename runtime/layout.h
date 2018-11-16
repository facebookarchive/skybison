#pragma once

#include "objects.h"
#include "utils.h"

namespace python {

// AttributeInfo packs attribute metadata into a SmallInteger.
class AttributeInfo {
 public:
  explicit AttributeInfo(Object* value) {
    DCHECK(value->isSmallInteger(), "expected small integer");
    value_ = reinterpret_cast<word>(value);
  }

  AttributeInfo() : value_(SmallInteger::kTag) {}

  AttributeInfo(word offset, word flags) : value_(SmallInteger::kTag) {
    DCHECK(isValidOffset(offset), "offset %ld too large (max is %ld)", offset,
           kMaxOffset);
    value_ |= (offset << kOffsetOffset);
    value_ |= (flags << kFlagsOffset);
  }

  // Getters and setters.

  // Retrieve the offset at which the attribute is stored.
  //
  // Check the kInObject flag to determine whether to retrieve the attribute
  // from the instance directly or from the overflow attributes.
  //
  // NB: For in-object attributes, this is the offset, in bytes, from the start
  // of the instance. For overflow attributes, this is the index into the
  // overflow array.
  word offset();
  static bool isValidOffset(word offset) { return offset <= kMaxOffset; }

  enum Flag {
    kNone = 0,

    // When set, this indicates that the attribute is stored directly on the
    // instance. When unset, this indicates that the attribute is stored in
    // the overflow array attached to the instance.
    kInObject = 1,

    // Only applies to in-object attributes. When set, it indicates that the
    // attribute has been deleted.
    kDeleted = 2,
  };

  word flags();
  bool testFlag(Flag flag);

  bool isInObject() { return testFlag(Flag::kInObject); }

  bool isOverflow() { return !testFlag(Flag::kInObject); }

  bool isDeleted() { return testFlag(Flag::kDeleted); }

  // Casting.
  SmallInteger* asSmallInteger();

  // Tags.
  static const int kOffsetSize = 30;
  static const int kOffsetOffset = SmallInteger::kTagSize;
  static const uword kOffsetMask = (1 << kOffsetSize) - 1;

  static const int kFlagsSize = 33;
  static const int kFlagsOffset = kOffsetOffset + kOffsetSize;
  static const uword kFlagsMask = (1UL << kFlagsSize) - 1;

  static_assert(
      SmallInteger::kTagSize + kOffsetSize + kFlagsSize == kBitsPerPointer,
      "Number of bits used by AttributeInfo must fit in a SmallInteger");

  // Constants
  static const word kMaxOffset = (1L << (kOffsetSize + 1)) - 1;

 private:
  uword value_;
};

inline word AttributeInfo::offset() {
  return (value_ >> kOffsetOffset) & kOffsetMask;
}

inline word AttributeInfo::flags() {
  return (value_ >> kFlagsOffset) & kFlagsMask;
}

inline bool AttributeInfo::testFlag(Flag flag) {
  return value_ & (static_cast<uword>(flag) << kFlagsOffset);
}

inline SmallInteger* AttributeInfo::asSmallInteger() {
  auto smi = reinterpret_cast<SmallInteger*>(value_);
  DCHECK(smi->isSmallInteger(), "expected small integer");
  return smi;
}

}  // namespace python
