#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace py {

// Extends a list from an exact tuple or exact list. Modifies dst in-place.
// Returns either the None or an Error object if allocation failed.
RawObject listExtend(Thread* thread, const List& dst, const Object& iterable);

// Inserts an element to the specified index of the list.
// When index >= len(list) it is equivalent to appending to the list.
void listInsert(Thread* thread, const List& list, const Object& value,
                word index);

// Removes and returns an element from the specified list index.
// Expects index to be within [0, len(list)]
RawObject listPop(Thread* thread, const List& list, word index);

// Return a new list that is composed of list repeated ntimes
RawObject listReplicate(Thread* thread, const List& list, word ntimes);

void listReverse(Thread* thread, const List& list);

// Returns a new list by slicing the given list
RawObject listSlice(Thread* thread, const List& list, word start, word stop,
                    word step);

// Sort a list in place.
// Returns None when there has been no error, or throws a TypeError and returns
// Error otherwise.
RawObject listSort(Thread* thread, const List& list);

// Return the next item from the iterator, or Error if there are no items left.
RawObject listIteratorNext(Thread* thread, const ListIterator& iter);

RawObject METH(list, append)(Thread* thread, Frame* frame, word nargs);
RawObject METH(list, clear)(Thread* thread, Frame* frame, word nargs);
RawObject METH(list, __add__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(list, __contains__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(list, __iter__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(list, __imul__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(list, __len__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(list, __mul__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(list, __new__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(list, insert)(Thread* thread, Frame* frame, word nargs);
RawObject METH(list, pop)(Thread* thread, Frame* frame, word nargs);
RawObject METH(list, remove)(Thread* thread, Frame* frame, word nargs);
RawObject METH(list, sort)(Thread* thread, Frame* frame, word nargs);

class ListBuiltins : public Builtins<ListBuiltins, ID(list), LayoutId::kList> {
 public:
  static const BuiltinAttribute kAttributes[];
  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ListBuiltins);
};

RawObject METH(list_iterator, __iter__)(Thread* thread, Frame* frame,
                                        word nargs);
RawObject METH(list_iterator, __length_hint__)(Thread* thread, Frame* frame,
                                               word nargs);
RawObject METH(list_iterator, __next__)(Thread* thread, Frame* frame,
                                        word nargs);

class ListIteratorBuiltins
    : public Builtins<ListIteratorBuiltins, ID(list_iterator),
                      LayoutId::kListIterator> {
 public:
  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ListIteratorBuiltins);
};

}  // namespace py
