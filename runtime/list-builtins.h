#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

void listReverse(Thread* thread, const List& list);

class ListBuiltins {
 public:
  static void initialize(Runtime* runtime);

  static RawObject append(Thread* thread, Frame* frame, word nargs);
  static RawObject extend(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderAdd(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderDelItem(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderGetItem(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderIter(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderLen(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderMul(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNew(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderSetItem(Thread* thread, Frame* frame, word nargs);
  static RawObject insert(Thread* thread, Frame* frame, word nargs);
  static RawObject pop(Thread* thread, Frame* frame, word nargs);
  static RawObject remove(Thread* thread, Frame* frame, word nargs);
  static RawObject slice(Thread* thread, RawList list, RawSlice slice);

 private:
  static const BuiltinAttribute kAttributes[];
  static const BuiltinMethod kMethods[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(ListBuiltins);
};

class ListIteratorBuiltins {
 public:
  static void initialize(Runtime* runtime);

  static RawObject dunderIter(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderLengthHint(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNext(Thread* thread, Frame* frame, word nargs);

 private:
  static const BuiltinMethod kMethods[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(ListIteratorBuiltins);
};

}  // namespace python
