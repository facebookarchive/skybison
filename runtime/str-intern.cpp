#include "str-intern.h"

#include "handles.h"
#include "runtime.h"
#include "thread.h"

namespace py {

word internSetComputeRemaining(word data_length) {
  return (data_length * 2) / 3;
}

RawObject internSetGrow(Thread* thread, RawMutableTuple data_raw,
                        word* remaining_out) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Tuple old_data(&scope, data_raw);
  word old_capacity = old_data.length();
  word new_capacity = old_capacity * 2;
  word new_remaining = internSetComputeRemaining(new_capacity);
  DCHECK(Utils::isPowerOfTwo(new_capacity), "must be power of two");
  word mask = new_capacity - 1;
  MutableTuple new_data(&scope, runtime->newMutableTuple(new_capacity));
  for (word i = 0, length = old_data.length(); i < length; i++) {
    RawObject slot = old_data.at(i);
    if (slot.isNoneType()) {
      continue;
    }
    word hash = LargeStr::cast(slot).header().hashCode();
    word index = hash & mask;
    word num_probes = 0;
    while (!new_data.at(index).isNoneType()) {
      num_probes++;
      index = (index + num_probes) & mask;
    }
    new_data.atPut(index, slot);
    new_remaining--;
  }
  DCHECK(new_remaining > 0, "must have remaining slots");
  *remaining_out = new_remaining;
  return *new_data;
}

bool internSetContains(RawMutableTuple data, RawLargeStr str) {
  word hash = str.header().hashCode();
  if (hash == Header::kUninitializedHash) {
    return false;
  }
  DCHECK(Utils::isPowerOfTwo(data.length()), "length must be power of two");
  word mask = data.length() - 1;
  word idx = hash & mask;
  word num_probes = 0;
  for (;;) {
    RawObject slot = data.at(idx);
    if (slot == str) {
      return true;
    }
    if (slot.isNoneType()) {
      return false;
    }
    num_probes++;
    idx = (idx + num_probes) & mask;
  }
}

}  // namespace py
