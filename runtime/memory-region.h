// Copyright (c) 2013, the Dart project authors and Facebook, Inc. and its
// affiliates. Please see the AUTHORS-Dart file for details. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE-Dart file.

#pragma once

#include "globals.h"

namespace py {

class MemoryRegion {
 public:
  MemoryRegion(void* pointer, word size) : pointer_(pointer), size_(size) {}

  void* pointer() const { return pointer_; }
  uword size() const { return size_; }

  void copyFrom(uword offset, MemoryRegion from) const;

 private:
  uword start() const { return reinterpret_cast<uword>(pointer_); }
  uword end() const { return start() + size_; }

  void* pointer_;
  uword size_;
};

}  // namespace py
