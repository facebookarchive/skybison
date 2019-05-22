// Inline caches.
#pragma once

#include "handles.h"
#include "objects.h"

namespace python {

// Bitset indicating how a cached binary operation needs to be called.
enum IcBinopFlags : uint8_t {
  IC_BINOP_NONE = 0,
  // Swap arguments when calling the method.
  IC_BINOP_REFLECTED = 1 << 0,
  // Retry alternative method when method returns `NotImplemented`.  Should try
  // the non-reflected op if the `IC_BINOP_REFLECTED` flag is set and vice
  // versa.
  IC_BINOP_NOTIMPLEMENTED_RETRY = 1 << 1,
  // This flag is set when the cached method is an in-place operation (such as
  // __iadd__).
  IC_INPLACE_BINOP_RETRY = 1 << 2,
};

// Looks for a cache entry with a `layout_id` key. Returns the cached value.
// Returns `ErrorNotFound` if none was found.
RawObject icLookup(const Tuple& caches, word index, LayoutId layout_id);

// Looks for a cache entry with `left_layout_id` and `right_layout_id` as key.
// Returns the cached value comprising of an object reference and flags. Returns
// `ErrorNotFound` if none was found.
RawObject icLookupBinop(const Tuple& caches, word index,
                        LayoutId left_layout_id, LayoutId right_layout_id,
                        IcBinopFlags* flags_out);

// Returns the original argument of bytecode operations that were rewritten by
// `rewriteBytecode()`.
word icOriginalArg(RawFunction function, word index);

// Prepares bytecode for caching: Adds a rewritten variant of the bytecode to
// `function`. It has the arguments of opcodes that use the cache replaced with
// a cache index. The previous arguments are moved to a separate tuple and can
// be retrieved with `icOriginalArg()`. Also adds a correctly sized `caches`
// tuple to `function`.
void icRewriteBytecode(Thread* thread, const Function& function);

// Sets a cache entry to the given `layout_id` as key and `value` as value.
void icUpdate(Thread* thread, const Tuple& caches, word index,
              LayoutId layout_id, const Object& value);

// Sets a cache entry to a `left_layout_id` and `right_layout_id` key with
// the given `value` and `flags` as value.
void icUpdateBinop(Thread* thread, const Tuple& caches, word index,
                   LayoutId left_layout_id, LayoutId right_layout_id,
                   const Object& value, IcBinopFlags flags);

// Cache layout:
//  The caches for the caching opcodes of a function are joined together in a
//  tuple object.
//
//  0: cache 0  (used by the first opcode using an inline cache)
//    - 0: entry0 layout_id: layout id to match as SmallInt
//    - 1: entry0 target: cached value
//    - 2: entry1 layout_id
//    - 3: entry1 target
//    - 4: entry2 layout_id
//    - 5: entry2 target
//    - 6: entry3 layout_id
//    - 7: entry3 target
//  8: cache 1  (used by the second opcode using an inline cache)
//    - 8: entry0 layout_id
//    - 9: entry0 target
//      ...
//  n * kIcPointersPerCache: cache n

const int kIcPointersPerEntry = 2;
const int kIcEntriesPerCache = 4;
const int kIcPointersPerCache = kIcEntriesPerCache * kIcPointersPerEntry;

const int kIcEntryKeyOffset = 0;
const int kIcEntryValueOffset = 1;

inline RawObject icLookup(const Tuple& caches, word index, LayoutId layout_id) {
  RawSmallInt key = SmallInt::fromWord(static_cast<word>(layout_id));
  for (word i = index * kIcPointersPerCache, end = i + kIcPointersPerCache;
       i < end; i += kIcPointersPerEntry) {
    RawObject entry_key = caches.at(i + kIcEntryKeyOffset);
    if (entry_key == key) {
      return caches.at(i + kIcEntryValueOffset);
    }
    // Stop the search if we found an empty entry.
    if (entry_key.isNoneType()) {
      break;
    }
  }
  return Error::notFound();
}

inline RawObject icLookupBinop(const Tuple& caches, word index,
                               LayoutId left_layout_id,
                               LayoutId right_layout_id,
                               IcBinopFlags* flags_out) {
  static_assert(Header::kLayoutIdSize * 2 + kBitsPerByte <= SmallInt::kBits,
                "Two layout ids and flags overflow a SmallInt");
  word key_high_bits = static_cast<word>(left_layout_id)
                           << Header::kLayoutIdSize |
                       static_cast<word>(right_layout_id);
  for (word i = index * kIcPointersPerCache, end = i + kIcPointersPerCache;
       i < end; i += kIcPointersPerEntry) {
    RawObject entry_key = caches.at(i + kIcEntryKeyOffset);
    // Stop the search if we found an empty entry.
    if (entry_key.isNoneType()) {
      break;
    }
    word entry_key_value = SmallInt::cast(entry_key).value();
    if (entry_key_value >> 8 == key_high_bits) {
      *flags_out = static_cast<IcBinopFlags>(entry_key_value & 0xff);
      return caches.at(i + kIcEntryValueOffset);
    }
  }
  return Error::notFound();
}

inline word icOriginalArg(RawFunction function, word index) {
  return SmallInt::cast(RawTuple::cast(function.originalArguments()).at(index))
      .value();
}

}  // namespace python
