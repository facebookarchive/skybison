#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

class DictBuiltins {
 public:
  static void initialize(Runtime* runtime);

  static RawObject dunderContains(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderDelItem(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderEq(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderGetItem(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderLen(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderItems(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderIter(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderKeys(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNew(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderSetItem(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderValues(Thread* thread, Frame* frame, word nargs);

 private:
  static const BuiltinAttribute kAttributes[];
  static const BuiltinMethod kMethods[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(DictBuiltins);
};

class DictItemIteratorBuiltins {
 public:
  static void initialize(Runtime* runtime);

  static RawObject dunderIter(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderLengthHint(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNext(Thread* thread, Frame* frame, word nargs);

 private:
  static const BuiltinMethod kMethods[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(DictItemIteratorBuiltins);
};

class DictItemsBuiltins {
 public:
  static void initialize(Runtime* runtime);

  static RawObject dunderIter(Thread* thread, Frame* frame, word nargs);

 private:
  static const BuiltinMethod kMethods[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(DictItemsBuiltins);
};

class DictKeyIteratorBuiltins {
 public:
  static void initialize(Runtime* runtime);

  static RawObject dunderIter(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderLengthHint(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNext(Thread* thread, Frame* frame, word nargs);

 private:
  static const BuiltinMethod kMethods[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(DictKeyIteratorBuiltins);
};

class DictKeysBuiltins {
 public:
  static void initialize(Runtime* runtime);

  static RawObject dunderIter(Thread* thread, Frame* frame, word nargs);

 private:
  static const BuiltinMethod kMethods[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(DictKeysBuiltins);
};

class DictValueIteratorBuiltins {
 public:
  static void initialize(Runtime* runtime);

  static RawObject dunderIter(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderLengthHint(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNext(Thread* thread, Frame* frame, word nargs);

 private:
  static const BuiltinMethod kMethods[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(DictValueIteratorBuiltins);
};

class DictValuesBuiltins {
 public:
  static void initialize(Runtime* runtime);

  static RawObject dunderIter(Thread* thread, Frame* frame, word nargs);

 private:
  static const BuiltinMethod kMethods[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(DictValuesBuiltins);
};

}  // namespace python
