// Copyright (c) 2013, the Dart project authors and Facebook, Inc. and its
// affiliates. Please see the AUTHORS-Dart file for details. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE-Dart file.

#pragma once

#include "globals.h"
#include "memory-region.h"
#include "utils.h"
#include "vector.h"

namespace python {

// Forward declarations.
namespace x64 {
class Assembler;
}
class AssemblerBuffer;

class WARN_UNUSED Label {
 public:
  Label() : position_(0), unresolved_(0) {
#ifndef NDEBUG
    for (int i = 0; i < kMaxUnresolvedBranches; i++) {
      unresolved_near_positions_[i] = -1;
    }
#endif
  }

  ~Label() {
    // Assert if label is being destroyed with unresolved branches pending.
    DCHECK(!isLinked(), "assert()");
    DCHECK(!hasNear(), "assert()");
  }

  // Returns the position for bound and linked labels. Cannot be used
  // for unused labels.
  word position() const {
    DCHECK(!isUnused(), "assert()");
    return isBound() ? -position_ - kBias : position_ - kBias;
  }

  word linkPosition() const {
    DCHECK(isLinked(), "assert()");
    return position_ - kBias;
  }

  word nearPosition() {
    DCHECK(hasNear(), "assert()");
    return unresolved_near_positions_[--unresolved_];
  }

  bool isBound() const { return position_ < 0; }
  bool isUnused() const { return position_ == 0 && unresolved_ == 0; }
  bool isLinked() const { return position_ > 0; }
  bool hasNear() const { return unresolved_ != 0; }

 private:
  static const int kMaxUnresolvedBranches = 20;
  // Zero position_ means unused (neither bound nor linked to).
  // Thus we offset actual positions by the given bias to prevent zero
  // positions from occurring.
  static const int kBias = 4;

  word position_;
  word unresolved_;
  word unresolved_near_positions_[kMaxUnresolvedBranches];

  void reinitialize() { position_ = 0; }

  void bindTo(word position) {
    DCHECK(!isBound(), "assert()");
    DCHECK(!hasNear(), "assert()");
    position_ = -position - kBias;
    DCHECK(isBound(), "assert()");
  }

  void linkTo(word position) {
    DCHECK(!isBound(), "assert()");
    position_ = position + kBias;
    DCHECK(isLinked(), "assert()");
  }

  void nearLinkTo(word position) {
    DCHECK(!isBound(), "assert()");
    DCHECK(unresolved_ < kMaxUnresolvedBranches, "assert()");
    unresolved_near_positions_[unresolved_++] = position;
  }

  friend class x64::Assembler;
  DISALLOW_COPY_AND_ASSIGN(Label);
};

// Assembler fixups are positions in generated code that hold relocation
// information that needs to be processed before finalizing the code
// into executable memory.
class AssemblerFixup {
 public:
  virtual void process(MemoryRegion region, word position) = 0;

  // It would be ideal if the destructor method could be made private,
  // but the g++ compiler complains when this is subclassed.
  virtual ~AssemblerFixup() { UNREACHABLE("~AssemblerFixup"); }

 private:
  AssemblerFixup* previous_;
  word position_;

  AssemblerFixup* previous() const { return previous_; }
  void setPrevious(AssemblerFixup* previous) { previous_ = previous; }

  word position() const { return position_; }
  void setPosition(word position) { position_ = position; }

  friend class AssemblerBuffer;
};

// Assembler buffers are used to emit binary code. They grow on demand.
class AssemblerBuffer {
 public:
  AssemblerBuffer();
  ~AssemblerBuffer();

  // Basic support for emitting, loading, and storing.
  template <typename T>
  void emit(T value) {
    DCHECK(hasEnsuredCapacity(), "assert()");
    *reinterpret_cast<T*>(cursor_) = value;
    cursor_ += sizeof(T);
  }

  template <typename T>
  void remit() {
    DCHECK(size() >= static_cast<word>(sizeof(T)), "assert()");
    cursor_ -= sizeof(T);
  }

  // Return address to code at |position| bytes.
  uword address(word position) { return contents_ + position; }

  template <typename T>
  T load(word position) {
    DCHECK(position >= 0, "assert()");
    DCHECK(position <= (size() - static_cast<word>(sizeof(T))), "assert()");
    return *reinterpret_cast<T*>(contents_ + position);
  }

  template <typename T>
  void store(word position, T value) {
    DCHECK(position >= 0, "assert()");
    DCHECK(position <= (size() - static_cast<word>(sizeof(T))), "assert()");
    *reinterpret_cast<T*>(contents_ + position) = value;
  }

  const Vector<word>& pointerOffsets() const {
#ifndef NDEBUG
    DCHECK(fixups_processed_, "assert()");
#endif
    return pointer_offsets_;
  }

  // Emit a fixup at the current location.
  void emitFixup(AssemblerFixup* fixup) {
    fixup->setPrevious(fixup_);
    fixup->setPosition(size());
    fixup_ = fixup;
  }

  // Count the fixups that produce a pointer offset, without processing
  // the fixups.
  word countPointerOffsets() const;

  // Get the size of the emitted code.
  word size() const { return cursor_ - contents_; }
  uword contents() const { return contents_; }

  // Copy the assembled instructions into the specified memory block
  // and apply all fixups.
  void finalizeInstructions(MemoryRegion instructions);

  // To emit an instruction to the assembler buffer, the EnsureCapacity helper
  // must be used to guarantee that the underlying data area is big enough to
  // hold the emitted instruction. Usage:
  //
  //     AssemblerBuffer buffer;
  //     AssemblerBuffer::EnsureCapacity ensured(&buffer);
  //     ... emit bytes for single instruction ...

#ifndef NDEBUG
  class EnsureCapacity {
   public:
    explicit EnsureCapacity(AssemblerBuffer* buffer);
    ~EnsureCapacity();

   private:
    AssemblerBuffer* buffer_;
    word gap_;

    word computeGap() { return buffer_->capacity() - buffer_->size(); }
  };

  bool has_ensured_capacity_;
  bool hasEnsuredCapacity() const { return has_ensured_capacity_; }
#else
  class EnsureCapacity {
   public:
    explicit EnsureCapacity(AssemblerBuffer* buffer) {
      if (buffer->cursor() >= buffer->limit()) buffer->extendCapacity();
    }
  };

  // When building the C++ tests, assertion code is enabled. To allow
  // asserting that the user of the assembler buffer has ensured the
  // capacity needed for emitting, we add a dummy method in non-debug mode.
  bool hasEnsuredCapacity() const { return true; }
#endif

  // Returns the position in the instruction stream.
  word getPosition() const { return cursor_ - contents_; }

  void reset() { cursor_ = contents_; }

 private:
  // The limit is set to kMinimumGap bytes before the end of the data area.
  // This leaves enough space for the longest possible instruction and allows
  // for a single, fast space check per instruction.
  static const word kMinimumGap = 32;

  uword contents_;
  uword cursor_;
  uword limit_;
  AssemblerFixup* fixup_;
  Vector<word> pointer_offsets_;
#ifndef NDEBUG
  bool fixups_processed_;
#endif

  uword cursor() const { return cursor_; }
  uword limit() const { return limit_; }
  word capacity() const {
    DCHECK(limit_ >= contents_, "assert()");
    return (limit_ - contents_) + kMinimumGap;
  }

  // Process the fixup chain.
  void processFixups(MemoryRegion region);

  // Compute the limit based on the data area and the capacity. See
  // description of kMinimumGap for the reasoning behind the value.
  static uword computeLimit(uword data, word capacity) {
    return data + capacity - kMinimumGap;
  }

  void extendCapacity();

  friend class AssemblerFixup;
};

}  // namespace python

#include "assembler-x64.h"
