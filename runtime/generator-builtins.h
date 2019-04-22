#pragma once

#include "runtime.h"

namespace python {

class GeneratorBaseBuiltins {
 public:
  template <LayoutId target>
  static RawObject send(Thread* thread, Frame* frame, word nargs);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(GeneratorBaseBuiltins);
};

class GeneratorBuiltins
    : public Builtins<GeneratorBuiltins, SymbolId::kGenerator,
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
    : public Builtins<CoroutineBuiltins, SymbolId::kCoroutine,
                      LayoutId::kCoroutine> {
 public:
  static const BuiltinMethod kBuiltinMethods[];
  static const BuiltinAttribute kAttributes[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(CoroutineBuiltins);
};

}  // namespace python
