#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

class ListBuiltins {
 public:
  static void initialize(Runtime* runtime);

  static Object* append(Thread* thread, Frame* frame, word nargs);
  static Object* extend(Thread* thread, Frame* frame, word nargs);
  static Object* dunderAdd(Thread* thread, Frame* frame, word nargs);
  static Object* dunderDelItem(Thread* thread, Frame* frame, word nargs);
  static Object* dunderGetItem(Thread* thread, Frame* frame, word nargs);
  static Object* dunderIter(Thread* thread, Frame* frame, word nargs);
  static Object* dunderLen(Thread* thread, Frame* frame, word nargs);
  static Object* dunderMul(Thread* thread, Frame* frame, word nargs);
  static Object* dunderNew(Thread* thread, Frame* frame, word nargs);
  static Object* dunderSetItem(Thread* thread, Frame* frame, word nargs);
  static Object* insert(Thread* thread, Frame* frame, word nargs);
  static Object* pop(Thread* thread, Frame* frame, word nargs);
  static Object* remove(Thread* thread, Frame* frame, word nargs);
  static Object* slice(Thread* thread, List* list, Slice* slice);

 private:
  static const BuiltinAttribute kAttributes[];
  static const BuiltinMethod kMethods[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(ListBuiltins);
};

class ListIteratorBuiltins {
 public:
  static void initialize(Runtime* runtime);

  static Object* dunderIter(Thread* thread, Frame* frame, word nargs);
  static Object* dunderLengthHint(Thread* thread, Frame* frame, word nargs);
  static Object* dunderNext(Thread* thread, Frame* frame, word nargs);

 private:
  static const BuiltinMethod kMethods[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(ListIteratorBuiltins);
};

}  // namespace python
