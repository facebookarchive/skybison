#pragma once

#include "runtime.h"

namespace python {

class GeneratorBaseBuiltins {
 public:
  static void initialize(Runtime* runtime);

  template <LayoutId target>
  static RawObject send(Thread* thread, Frame* frame, word nargs);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(GeneratorBaseBuiltins);
};

class GeneratorBuiltins {
 public:
  static void initialize(Runtime* runtime);

  static RawObject dunderNext(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderIter(Thread* thread, Frame* frame, word nargs);

 private:
  static const NativeMethod kNativeMethods[];
};

class CoroutineBuiltins {
 public:
  static void initialize(Runtime* runtime);

 private:
  static const NativeMethod kNativeMethods[];
};

}  // namespace python
