#pragma once

#include "globals.h"
#include "runtime.h"

namespace py {

RawObject memoryviewItemsize(Thread* thread, const MemoryView& view);
RawObject memoryviewSetitem(Thread* thread, const MemoryView& view,
                            const Int& index, const Object& value);
RawObject memoryviewSetslice(Thread* thread, const MemoryView& view, word start,
                             word stop, word step, word slice_len,
                             const Object& value);

RawObject METH(memoryview, cast)(Thread* thread, Frame* frame, word nargs);
RawObject METH(memoryview, __getitem__)(Thread* thread, Frame* frame,
                                        word nargs);
RawObject METH(memoryview, __len__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(memoryview, __new__)(Thread* thread, Frame* frame, word nargs);

class MemoryViewBuiltins : public Builtins<MemoryViewBuiltins, ID(memoryview),
                                           LayoutId::kMemoryView> {
 public:
  static const BuiltinAttribute kAttributes[];
  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(MemoryViewBuiltins);
};

}  // namespace py
