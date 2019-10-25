// Copyright (c) 2013, the Dart project authors and Facebook, Inc. and its
// affiliates. Please see the AUTHORS-Dart file for details. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE-Dart file.

#include "assembler-utils.h"

#include <cstdlib>

#include "assembler-x64.h"
#include "memory-region.h"
#include "utils.h"

namespace py {

#ifndef NDEBUG
AssemblerBuffer::EnsureCapacity::EnsureCapacity(AssemblerBuffer* buffer) {
  if (buffer->cursor() >= buffer->limit()) buffer->extendCapacity();
  // In debug mode, we save the assembler buffer along with the gap
  // size before we start emitting to the buffer. This allows us to
  // check that any single generated instruction doesn't overflow the
  // limit implied by the minimum gap size.
  buffer_ = buffer;
  gap_ = computeGap();
  // Make sure that extending the capacity leaves a big enough gap
  // for any kind of instruction.
  DCHECK(gap_ >= kMinimumGap, "assert()");
  // Mark the buffer as having ensured the capacity.
  DCHECK(!buffer->hasEnsuredCapacity(), "assert()");  // Cannot nest.
  buffer->has_ensured_capacity_ = true;
}

AssemblerBuffer::EnsureCapacity::~EnsureCapacity() {
  // Unmark the buffer, so we cannot emit after this.
  buffer_->has_ensured_capacity_ = false;
  // Make sure the generated instruction doesn't take up more
  // space than the minimum gap.
  word delta = gap_ - computeGap();
  DCHECK(delta <= kMinimumGap, "assert()");
}
#endif

AssemblerBuffer::AssemblerBuffer() {
  static const word initial_buffer_capacity = 4 * kKiB;
  contents_ = reinterpret_cast<uword>(std::malloc(initial_buffer_capacity));
  cursor_ = contents_;
  limit_ = computeLimit(contents_, initial_buffer_capacity);
  fixup_ = nullptr;
#ifndef NDEBUG
  has_ensured_capacity_ = false;
  fixups_processed_ = false;
#endif

  // Verify internal state.
  DCHECK(capacity() == initial_buffer_capacity, "assert()");
  DCHECK(size() == 0, "assert()");
}

AssemblerBuffer::~AssemblerBuffer() {
  std::free(reinterpret_cast<void*>(contents_));
  cursor_ = 0;
}

void AssemblerBuffer::processFixups(MemoryRegion region) {
  AssemblerFixup* fixup = fixup_;
  while (fixup != nullptr) {
    fixup->process(region, fixup->position());
    fixup = fixup->previous();
  }
}

void AssemblerBuffer::finalizeInstructions(MemoryRegion instructions) {
  // Copy the instructions from the buffer.
  MemoryRegion from(reinterpret_cast<void*>(contents()), size());
  instructions.copyFrom(0, from);
  // Process fixups in the instructions.
  processFixups(instructions);
#ifndef NDEBUG
  fixups_processed_ = true;
#endif
}

void AssemblerBuffer::extendCapacity() {
  word old_size = size();
  word old_capacity = capacity();
  word new_capacity = Utils::minimum(old_capacity * 2, old_capacity + 1 * kMiB);
  if (new_capacity < old_capacity) {
    UNREACHABLE("Unexpected overflow in AssemblerBuffer::ExtendCapacity");
  }

  // Allocate the new data area and copy contents of the old one to it.
  uword new_contents = reinterpret_cast<uword>(std::malloc(new_capacity));
  memmove(reinterpret_cast<void*>(new_contents),
          reinterpret_cast<void*>(contents_), old_size);

  // Compute the relocation delta and switch to the new contents area.
  word delta = new_contents - contents_;
  std::free(reinterpret_cast<void*>(contents_));
  contents_ = new_contents;

  // Update the cursor and recompute the limit.
  cursor_ += delta;
  limit_ = computeLimit(new_contents, new_capacity);

  // Verify internal state.
  DCHECK(capacity() == new_capacity, "assert()");
  DCHECK(size() == old_size, "assert()");
}

}  // namespace py
