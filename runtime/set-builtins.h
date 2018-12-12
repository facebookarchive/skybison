#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

RawObject setAdd(Thread* thread, const Set& set, const Object& key);

class SetBaseBuiltins {
 public:
  static RawObject dunderAnd(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderContains(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderEq(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderGe(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderGt(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderIter(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderLe(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderLen(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderLt(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNe(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNew(Thread* thread, Frame* frame, word nargs);
  static RawObject intersection(Thread* thread, Frame* frame, word nargs);
  static RawObject isDisjoint(Thread* thread, Frame* frame, word nargs);
};

class SetBuiltins : public SetBaseBuiltins {
 public:
  static void initialize(Runtime* runtime);

  static RawObject dunderIand(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderInit(Thread* thread, Frame* frame, word nargs);
  static RawObject add(Thread* thread, Frame* frame, word nargs);
  static RawObject pop(Thread* thread, Frame* frame, word nargs);

 private:
  static const BuiltinAttribute kAttributes[];
  static const BuiltinMethod kMethods[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(SetBuiltins);
};

class FrozenSetBuiltins : public SetBaseBuiltins {
 public:
  static void initialize(Runtime* runtime);

 private:
  static const BuiltinAttribute kAttributes[];
  static const BuiltinMethod kMethods[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(FrozenSetBuiltins);
};

class SetIteratorBuiltins {
 public:
  static void initialize(Runtime* runtime);

  static RawObject dunderIter(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderLengthHint(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNext(Thread* thread, Frame* frame, word nargs);

 private:
  static const BuiltinMethod kMethods[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(SetIteratorBuiltins);
};

}  // namespace python
