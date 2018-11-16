#pragma once

#include "runtime.h"

namespace python {

class GeneratorBaseBuiltins {
 public:
  static void initialize(Runtime* runtime);

  template <LayoutId target>
  static Object* send(Thread* thread, Frame* frame, word nargs);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(GeneratorBaseBuiltins);
};

class GeneratorBuiltins {
 public:
  static void initialize(Runtime* runtime);

  static Object* dunderNext(Thread* thread, Frame* frame, word nargs);
  static Object* dunderIter(Thread* thread, Frame* frame, word nargs);

 private:
  static const BuiltinMethod kMethods[];
};

class CoroutineBuiltins {
 public:
  static void initialize(Runtime* runtime);

 private:
  static const BuiltinMethod kMethods[];
};

}  // namespace python
