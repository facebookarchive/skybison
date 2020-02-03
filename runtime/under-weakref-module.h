#pragma once

#include "modules.h"
#include "symbols.h"

namespace py {

RawObject FUNC(_weakref, _weakref_hash)(Thread* thread, Frame* frame,
                                        word nargs);
RawObject FUNC(_weakref, _weakref_set_hash)(Thread* thread, Frame* frame,
                                            word nargs);

class UnderWeakrefModule {
 public:
  static void initialize(Thread* thread, const Module& module);

 private:
  static const BuiltinFunction kBuiltinFunctions[];
  static const BuiltinType kBuiltinTypes[];
};

}  // namespace py
