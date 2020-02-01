#pragma once

#include "runtime.h"

namespace py {

// Get the RawGeneratorBase corresponding to the given Frame, assuming it is
// executing in a resumed GeneratorBase.
RawGeneratorBase generatorFromStackFrame(Frame* frame);

RawObject METH(generator, __next__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(generator, __iter__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(generator, send)(Thread* thread, Frame* frame, word nargs);
RawObject METH(generator, throw)(Thread* thread, Frame* frame, word nargs);

class GeneratorBuiltins
    : public Builtins<GeneratorBuiltins, ID(generator), LayoutId::kGenerator> {
 public:
  static const BuiltinMethod kBuiltinMethods[];
  static const BuiltinAttribute kAttributes[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(GeneratorBuiltins);
};

RawObject METH(coroutine, send)(Thread* thread, Frame* frame, word nargs);

class CoroutineBuiltins
    : public Builtins<CoroutineBuiltins, ID(coroutine), LayoutId::kCoroutine> {
 public:
  static const BuiltinMethod kBuiltinMethods[];
  static const BuiltinAttribute kAttributes[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(CoroutineBuiltins);
};

class AsyncGeneratorBuiltins
    : public Builtins<AsyncGeneratorBuiltins, ID(async_generator),
                      LayoutId::kAsyncGenerator> {
 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(AsyncGeneratorBuiltins);
};

}  // namespace py
