#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

class SetBuiltins {
 public:
  static void initialize(Runtime* runtime);

  static Object* add(Thread* thread, Frame* frame, word nargs);
  static Object* dunderAnd(Thread* thread, Frame* frame, word nargs);
  static Object* dunderContains(Thread* thread, Frame* frame, word nargs);
  static Object* dunderIand(Thread* thread, Frame* frame, word nargs);
  static Object* dunderInit(Thread* thread, Frame* frame, word nargs);
  static Object* dunderIter(Thread* thread, Frame* frame, word nargs);
  static Object* dunderLen(Thread* thread, Frame* frame, word nargs);
  static Object* dunderNew(Thread* thread, Frame* frame, word nargs);
  static Object* intersection(Thread* thread, Frame* frame, word nargs);
  static Object* isDisjoint(Thread* thread, Frame* frame, word nargs);
  static Object* pop(Thread* thread, Frame* frame, word nargs);

 private:
  static const BuiltinMethod kMethods[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(SetBuiltins);
};

class SetIteratorBuiltins {
 public:
  static void initialize(Runtime* runtime);

  static Object* dunderIter(Thread* thread, Frame* frame, word nargs);
  static Object* dunderLengthHint(Thread* thread, Frame* frame, word nargs);
  static Object* dunderNext(Thread* thread, Frame* frame, word nargs);

 private:
  static const BuiltinMethod kMethods[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(SetIteratorBuiltins);
};

}  // namespace python
