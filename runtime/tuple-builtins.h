#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

class TupleBuiltins {
 public:
  static void initialize(Runtime* runtime);

  static Object* dunderEq(Thread* thread, Frame* frame, word nargs);
  static Object* dunderGetItem(Thread* thread, Frame* frame, word nargs);
  static Object* dunderIter(Thread* thread, Frame* frame, word nargs);
  static Object* dunderLen(Thread* thread, Frame* frame, word nargs);
  static Object* dunderMul(Thread* thread, Frame* frame, word nargs);
  static Object* dunderNew(Thread* thread, Frame* frame, word nargs);

  static Object* slice(Thread* thread, ObjectArray* tuple, Slice* slice);

 private:
  static const BuiltinMethod kMethods[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(TupleBuiltins);
};

class TupleIteratorBuiltins {
 public:
  static void initialize(Runtime* runtime);

  static Object* dunderIter(Thread* thread, Frame* frame, word nargs);
  static Object* dunderLengthHint(Thread* thread, Frame* frame, word nargs);
  static Object* dunderNext(Thread* thread, Frame* frame, word nargs);

 private:
  static const BuiltinMethod kMethods[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(TupleIteratorBuiltins);
};

}  // namespace python
