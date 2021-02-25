#pragma once

#include "globals.h"
#include "handles-decl.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace py {

// Add `LargeStr` object `str` to the intern set `data`. Sets `*result` to
// an equal existing string in the table and returns `false` or inserts `str`
// to the table and returns `true`. The return value should be used to count
// down a "remaining" counter and grow the set when necessary.
bool internSetAdd(Thread* thread, RawMutableTuple data, const Object& str,
                  RawObject* result);

// Sets `*result` to an existing string object equal to `bytes` and returns
// `false` or creates a new `LargeStr` from `bytes`, inserts it into the table
// and returns `true`. The return value should be used to count down a
// "remaining" counter and grow the set when necessary.
bool internSetAddFromAll(Thread* thread, RawMutableTuple data, View<byte> bytes,
                         RawObject* result);

word internSetComputeRemaining(word data_length);

bool internSetContains(RawMutableTuple data, RawLargeStr str);

RawObject internSetGrow(Thread* thread, RawMutableTuple data,
                        word* remaining_out);

inline bool internSetAddFromAll(Thread* thread, RawMutableTuple data,
                                View<byte> bytes, RawObject* result) {
  DCHECK(bytes.length() > SmallStr::kMaxLength, "only need to intern LargeStr");
  Runtime* runtime = thread->runtime();
  word hash = runtime->bytesHash(bytes);
  DCHECK(Utils::isPowerOfTwo(data.length()), "table size must be power of two");
  word mask = data.length() - 1;
  word index = hash & mask;
  word num_probes = 0;
  for (;;) {
    RawObject slot = data.at(index);
    if (slot.isNoneType()) {
      RawLargeStr new_str = LargeStr::cast(runtime->newStrWithAll(bytes));
      new_str.setHeader(new_str.header().withHashCode(hash));
      data.atPut(index, new_str);
      *result = new_str;
      return true;
    }
    if (LargeStr::cast(slot).equalsBytes(bytes)) {
      *result = slot;
      return false;
    }

    num_probes++;
    index = (index + num_probes) & mask;
  }
}

inline bool internSetAdd(Thread* thread, RawMutableTuple data,
                         const Object& str, RawObject* result) {
  DCHECK(str.isLargeStr(), "expected large string");
  Runtime* runtime = thread->runtime();
  word hash = runtime->valueHash(*str);
  DCHECK(Utils::isPowerOfTwo(data.length()), "table size must be power of two");
  word mask = data.length() - 1;
  word index = hash & mask;
  word num_probes = 0;
  for (;;) {
    RawObject slot = data.at(index);
    if (slot == str) {
      *result = *str;
      return false;
    }
    if (slot.isNoneType()) {
      data.atPut(index, *str);
      *result = *str;
      return true;
    }
    if (LargeStr::cast(slot).equals(LargeStr::cast(*str))) {
      *result = slot;
      return false;
    }

    num_probes++;
    index = (index + num_probes) & mask;
  }
}

}  // namespace py
