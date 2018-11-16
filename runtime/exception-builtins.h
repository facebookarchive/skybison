#pragma once

#include "globals.h"
#include "runtime.h"

namespace python {

class BaseExceptionBuiltins {
 public:
  static void initialize(Runtime* runtime);

  static Object* dunderInit(Thread* thread, Frame* frame, word nargs);

 private:
  static const BuiltinAttribute kAttributes[];
  static const BuiltinMethod kMethods[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(BaseExceptionBuiltins);
};

class StopIterationBuiltins {
 public:
  static void initialize(Runtime* runtime);

  static Object* dunderInit(Thread* thread, Frame* frame, word nargs);

 private:
  static const BuiltinAttribute kAttributes[];
  static const BuiltinMethod kMethods[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(StopIterationBuiltins);
};

class SystemExitBuiltins {
 public:
  static void initialize(Runtime* runtime);

  static Object* dunderInit(Thread* thread, Frame* frame, word nargs);

 private:
  static const BuiltinAttribute kAttributes[];
  static const BuiltinMethod kMethods[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(SystemExitBuiltins);
};

}  // namespace python
