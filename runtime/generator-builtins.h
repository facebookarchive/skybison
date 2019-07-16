#pragma once

#include "runtime.h"

namespace python {

// Get the RawGeneratorBase corresponding to the given Frame, assuming it is
// executing in a resumed GeneratorBase.
RawGeneratorBase generatorFromStackFrame(Frame* frame);

template <typename T, SymbolId name, LayoutId type>
class GeneratorBaseBuiltins : public Builtins<T, name, type> {
 public:
  using Base = GeneratorBaseBuiltins;

  static RawObject send(Thread* thread, Frame* frame, word nargs);
  static RawObject genThrow(Thread* thread, Frame* frame, word nargs);

  static_assert(type == LayoutId::kGenerator || type == LayoutId::kCoroutine ||
                    type == LayoutId::kAsyncGenerator,
                "unsupported type");

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(GeneratorBaseBuiltins);
};

class GeneratorBuiltins
    : public GeneratorBaseBuiltins<GeneratorBuiltins, SymbolId::kGenerator,
                                   LayoutId::kGenerator> {
 public:
  static RawObject dunderNext(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderIter(Thread* thread, Frame* frame, word nargs);

  static const BuiltinMethod kBuiltinMethods[];
  static const BuiltinAttribute kAttributes[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(GeneratorBuiltins);
};

class CoroutineBuiltins
    : public GeneratorBaseBuiltins<CoroutineBuiltins, SymbolId::kCoroutine,
                                   LayoutId::kCoroutine> {
 public:
  static const BuiltinMethod kBuiltinMethods[];
  static const BuiltinAttribute kAttributes[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(CoroutineBuiltins);
};

class AsyncGeneratorBuiltins
    : public GeneratorBaseBuiltins<AsyncGeneratorBuiltins,
                                   SymbolId::kAsyncGenerator,
                                   LayoutId::kAsyncGenerator> {
 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(AsyncGeneratorBuiltins);
};

}  // namespace python
