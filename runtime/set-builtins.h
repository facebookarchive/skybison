#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

RawObject setAdd(Thread* thread, const Set& set, const Object& key);

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
  static RawObject intersection(Thread* thread, Frame* frame, word nargs);
  static RawObject isDisjoint(Thread* thread, Frame* frame, word nargs);
};

class SetBuiltins
    : public SetBaseBuiltins,
      public Builtins<SetBuiltins, SymbolId::kSet, LayoutId::kSet> {
 public:
  static RawObject dunderIand(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderInit(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNew(Thread* thread, Frame* frame, word nargs);
  static RawObject add(Thread* thread, Frame* frame, word nargs);
  static RawObject pop(Thread* thread, Frame* frame, word nargs);

  static const BuiltinAttribute kAttributes[];
  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(SetBuiltins);
};

class FrozenSetBuiltins
    : public SetBaseBuiltins,
      public Builtins<FrozenSetBuiltins, SymbolId::kFrozenSet,
                      LayoutId::kFrozenSet> {
 public:
  static RawObject dunderNew(Thread* thread, Frame* frame, word nargs);

  static const BuiltinAttribute kAttributes[];
  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(FrozenSetBuiltins);
};

class SetIteratorBuiltins
    : public Builtins<SetIteratorBuiltins, SymbolId::kSetIterator,
                      LayoutId::kSetIterator> {
 public:
  static RawObject dunderIter(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderLengthHint(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNext(Thread* thread, Frame* frame, word nargs);

  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(SetIteratorBuiltins);
};

}  // namespace python
