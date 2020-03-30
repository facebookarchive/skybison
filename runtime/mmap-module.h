#pragma once

#include "frame.h"
#include "globals.h"
#include "modules.h"
#include "objects.h"
#include "runtime.h"

namespace py {

class MmapModule {
 public:
  static void initialize(Thread* thread, const Module& module);

 private:
  static const BuiltinType kBuiltinTypes[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(MmapModule);
};

class MmapBuiltins : public Builtins<MmapBuiltins, ID(mmap), LayoutId::kMmap> {
 public:
  static const BuiltinAttribute kAttributes[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(MmapBuiltins);
};

}  // namespace py
