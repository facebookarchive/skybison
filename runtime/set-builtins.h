#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace py {

// Add `value` to set if there is no equivalent value present yet. Returns
// `value` if it was added or the existing equivalent value.
RawObject setAdd(Thread* thread, const SetBase& set, const Object& value,
                 word hash);

bool setIncludes(Thread* thread, const SetBase& set, const Object& key,
                 word hash);

// Compute the set intersection between a set and an iterator
// Returns either a new set with the intersection or an Error object.
NODISCARD RawObject setIntersection(Thread* thread, const SetBase& set,
                                    const Object& iterable);

// Delete the value equivalent to `key` from the set.
// Returns true if a value was removed, false if no equivalent value existed.
bool setRemove(Thread* thread, const Set& set, const Object& key, word hash);

// Update a set from an iterator
// Returns either the updated set or an Error object.
NODISCARD RawObject setUpdate(Thread* thread, const SetBase& dst,
                              const Object& iterable);

// Returns a shallow copy of a set
RawObject setCopy(Thread* thread, const SetBase& set);

// Returns true if set and other contain the same set of values
bool setEquals(Thread* thread, const SetBase& set, const SetBase& other);

// Returns true if every element of set is in other
// and the elements in set and other are not the same.
// This is analogous to the < operator on Python sets.
bool setIsProperSubset(Thread* thread, const SetBase& set,
                       const SetBase& other);

// Returns true if every element of set is in other.
// This is analogous to the <= operator on Python sets.
bool setIsSubset(Thread* thread, const SetBase& set, const SetBase& other);

// Remove an arbitrary object from the set and return a reference to it.
// Returns None on success and raises + returns KeyError if the set is empty.
RawObject setPop(Thread* thread, const Set& set);

// Return the next item from the iterator, or Error if there are no items left.
RawObject setIteratorNext(Thread* thread, const SetIterator& iter);

RawSmallInt frozensetHash(Thread* thread, const Object& frozenset);

RawObject METH(set, __and__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(set, __contains__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(set, __eq__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(set, __ge__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(set, __gt__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(set, __iand__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(set, __init__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(set, __iter__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(set, __le__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(set, __len__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(set, __lt__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(set, __ne__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(set, __new__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(set, add)(Thread* thread, Frame* frame, word nargs);
RawObject METH(set, clear)(Thread* thread, Frame* frame, word nargs);
RawObject METH(set, copy)(Thread* thread, Frame* frame, word nargs);
RawObject METH(set, discard)(Thread* thread, Frame* frame, word nargs);
RawObject METH(set, intersection)(Thread* thread, Frame* frame, word nargs);
RawObject METH(set, isdisjoint)(Thread* thread, Frame* frame, word nargs);
RawObject METH(set, pop)(Thread* thread, Frame* frame, word nargs);
RawObject METH(set, remove)(Thread* thread, Frame* frame, word nargs);
RawObject METH(set, update)(Thread* thread, Frame* frame, word nargs);

class SetBuiltins : public Builtins<SetBuiltins, ID(set), LayoutId::kSet> {
 public:
  static const BuiltinAttribute kAttributes[];
  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(SetBuiltins);
};

RawObject METH(frozenset, __and__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(frozenset, __contains__)(Thread* thread, Frame* frame,
                                        word nargs);
RawObject METH(frozenset, __eq__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(frozenset, __ge__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(frozenset, __gt__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(frozenset, __hash__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(frozenset, __iter__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(frozenset, __le__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(frozenset, __len__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(frozenset, __lt__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(frozenset, __ne__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(frozenset, __new__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(frozenset, copy)(Thread* thread, Frame* frame, word nargs);
RawObject METH(frozenset, intersection)(Thread* thread, Frame* frame,
                                        word nargs);
RawObject METH(frozenset, isdisjoint)(Thread* thread, Frame* frame, word nargs);

class FrozenSetBuiltins
    : public Builtins<FrozenSetBuiltins, ID(frozenset), LayoutId::kFrozenSet> {
 public:
  static const BuiltinAttribute kAttributes[];
  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(FrozenSetBuiltins);
};

RawObject METH(set_iterator, __iter__)(Thread* thread, Frame* frame,
                                       word nargs);
RawObject METH(set_iterator, __length_hint__)(Thread* thread, Frame* frame,
                                              word nargs);
RawObject METH(set_iterator, __next__)(Thread* thread, Frame* frame,
                                       word nargs);

class SetIteratorBuiltins
    : public Builtins<SetIteratorBuiltins, ID(set_iterator),
                      LayoutId::kSetIterator> {
 public:
  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(SetIteratorBuiltins);
};

}  // namespace py
