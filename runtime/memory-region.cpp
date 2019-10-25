// Copyright (c) 2013, the Dart project authors and Facebook, Inc. and its
// affiliates. Please see the AUTHORS-Dart file for details. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE-Dart file.

#include "memory-region.h"

#include <cstring>

#include "utils.h"

namespace py {

void MemoryRegion::copyFrom(uword offset, MemoryRegion from) const {
  DCHECK(size() >= from.size(), "source cannot be larger than destination");
  DCHECK(offset <= (size() - from.size()), "offset is too large");
  std::memmove(reinterpret_cast<void*>(start() + offset), from.pointer(),
               from.size());
}

}  // namespace py
