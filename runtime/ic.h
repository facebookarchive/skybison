// Inline caches.
#pragma once

#include "handles.h"
#include "objects.h"
#include "runtime.h"

namespace python {

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

// Prepares bytecode for caching: Adds a rewritten variant of the bytecode to
// `function`. It has the arguments of opcodes that use the cache replaced with
// a cache index. The previous arguments are moved to a separate tuple and can
// be retrieved with `icOriginalArg()`. Also adds a correctly sized `caches`
// tuple to `function`.
void icRewriteBytecode(Thread* thread, const Function& function);

// Returns the original argument of bytecode operations that were rewritten by
// `rewriteBytecode()`.
inline word icOriginalArg(const Function& function, word index) {
  return RawSmallInt::cast(
             RawTuple::cast(function.originalArguments()).at(index))
      .value();
}

// Looks for a cached value matching `layout_id` at `index`. Returns a cached
// value or an `Error` if no cached value was found.
inline RawObject icLookup(const Tuple& caches, word index, LayoutId layout_id) {
  RawSmallInt key = RawSmallInt::fromWord(static_cast<word>(layout_id));
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

// Sets a cache entry to the given `layout_id` as key and `value` as value.
void icUpdate(Thread* thread, const Tuple& caches, word index,
              LayoutId layout_id, const Object& value);

}  // namespace python
