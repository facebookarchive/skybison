#pragma once

#include "frame.h"
#include "globals.h"
#include "modules.h"
#include "objects.h"
#include "runtime.h"

namespace py {

RawObject delAttribute(Thread* thread, const Object& self, const Object& name);
RawObject getAttribute(Thread* thread, const Object& self, const Object& name);
RawObject hasAttribute(Thread* thread, const Object& self, const Object& name);
RawObject setAttribute(Thread* thread, const Object& self, const Object& name,
                       const Object& value);

RawObject compile(Thread* thread, const Object& source, const Object& filename,
                  SymbolId mode, word flags, int optimize);

RawObject FUNC(builtins, __build_class__)(Thread* thread, Frame* frame,
                                          word nargs);
RawObject FUNC(builtins, __import__)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(builtins, bin)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(builtins, callable)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(builtins, chr)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(builtins, delattr)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(builtins, getattr)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(builtins, hasattr)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(builtins, hash)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(builtins, hex)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(builtins, id)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(builtins, oct)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(builtins, ord)(Thread* thread, Frame* frame, word nargs);
RawObject FUNC(builtins, setattr)(Thread* thread, Frame* frame, word nargs);

class BuiltinsModule {
 public:
  static void initialize(Thread* thread, const Module& module);

 private:
  static const BuiltinFunction kBuiltinFunctions[];
  static const BuiltinType kBuiltinTypes[];
  static const SymbolId kIntrinsicIds[];
};

}  // namespace py
