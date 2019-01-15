#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

// Extends a list from an iterator.
// Returns either the extended list or an Error object.
RawObject listExtend(Thread* thread, const List& dst, const Object& iterable);

// Inserts an element to the specified index of the list.
// When index >= len(list) it is equivalent to appending to the list.
void listInsert(Thread* thread, const List& list, const Object& value,
                word index);

// Removes and returns an element from the specified list index.
// Expects index to be within [0, len(list)]
RawObject listPop(const List& list, word index);

// Return a new list that is composed of list repeated ntimes
RawObject listReplicate(Thread* thread, const List& list, word ntimes);

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
