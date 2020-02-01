#pragma once

#include "runtime.h"

namespace py {

// Get the RawGeneratorBase corresponding to the given Frame, assuming it is
// executing in a resumed GeneratorBase.
RawGeneratorBase generatorFromStackFrame(Frame* frame);

class GeneratorBuiltins
    : public Builtins<GeneratorBuiltins, ID(generator), LayoutId::kGenerator> {
 public:
  static RawObject dunderNext(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderIter(Thread* thread, Frame* frame, word nargs);
  static RawObject send(Thread* thread, Frame* frame, word nargs);
  static RawObject genThrow(Thread* thread, Frame* frame, word nargs);

  static const BuiltinMethod kBuiltinMethods[];
  static const BuiltinAttribute kAttributes[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(GeneratorBuiltins);
};

class CoroutineBuiltins
    : public Builtins<CoroutineBuiltins, ID(coroutine), LayoutId::kCoroutine> {
 public:
  static RawObject send(Thread* thread, Frame* frame, word nargs);

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
