#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace py {

// Creates a new dict with shallow copies of the given dict's elements.
RawObject dictCopy(Thread* thread, const Dict& dict);

// Update a dict from another dict or an iterator, overwriting existing keys if
// duplicates exist.
//
// Returns None on success, or an Error object if an exception was raised.
RawObject dictMergeOverride(Thread* thread, const Dict& dict,
                            const Object& mapping);

// Merges a dict with another dict or a mapping, raising an exception on
// duplicate keys.
//
// Returns None on success, or an Error object if an exception was raised.
RawObject dictMergeError(Thread* thread, const Dict& dict,
                         const Object& mapping);

// Update a dict from another dict or a mapping, ignoring existing keys if
// duplicates exist.
//
// Returns None on success, or an Error object if an exception was raised.
RawObject dictMergeIgnore(Thread* thread, const Dict& dict,
                          const Object& mapping);

// Returns next item in the dict as (key, value) tuple (Tuple)
// Returns Error::object() if there are no more objects
RawObject dictItemIteratorNext(Thread* thread, const DictItemIterator& iter);

// Returns next key in the dict
// Returns Error::object() if there are no more objects
RawObject dictKeyIteratorNext(Thread* thread, const DictKeyIterator& iter);

// Returns next value in the dict
// Returns Error::object() if there are no more objects
RawObject dictValueIteratorNext(Thread* thread, const DictValueIterator& iter);

class DictBuiltins
    : public Builtins<DictBuiltins, SymbolId::kDict, LayoutId::kDict> {
 public:
  static RawObject clear(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderDelItem(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderEq(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderLen(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderIter(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNew(Thread* thread, Frame* frame, word nargs);

  static RawObject items(Thread* thread, Frame* frame, word nargs);
  static RawObject keys(Thread* thread, Frame* frame, word nargs);
  static RawObject values(Thread* thread, Frame* frame, word nargs);

  static const BuiltinAttribute kAttributes[];
  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(DictBuiltins);
};

class DictItemIteratorBuiltins
    : public Builtins<DictItemIteratorBuiltins, SymbolId::kDictItemIterator,
                      LayoutId::kDictItemIterator> {
 public:
  static RawObject dunderIter(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderLengthHint(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNext(Thread* thread, Frame* frame, word nargs);

  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(DictItemIteratorBuiltins);
};

class DictItemsBuiltins
    : public Builtins<DictItemsBuiltins, SymbolId::kDictItems,
                      LayoutId::kDictItems> {
 public:
  static RawObject dunderIter(Thread* thread, Frame* frame, word nargs);

  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(DictItemsBuiltins);
};

class DictKeyIteratorBuiltins
    : public Builtins<DictKeyIteratorBuiltins, SymbolId::kDictKeyIterator,
                      LayoutId::kDictKeyIterator> {
 public:
  static RawObject dunderIter(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderLengthHint(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNext(Thread* thread, Frame* frame, word nargs);

  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(DictKeyIteratorBuiltins);
};

class DictKeysBuiltins : public Builtins<DictKeysBuiltins, SymbolId::kDictKeys,
                                         LayoutId::kDictKeys> {
 public:
  static RawObject dunderIter(Thread* thread, Frame* frame, word nargs);

  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(DictKeysBuiltins);
};

class DictValueIteratorBuiltins
    : public Builtins<DictValueIteratorBuiltins, SymbolId::kDictValueIterator,
                      LayoutId::kDictValueIterator> {
 public:
  static RawObject dunderIter(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderLengthHint(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNext(Thread* thread, Frame* frame, word nargs);

  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(DictValueIteratorBuiltins);
};

class DictValuesBuiltins
    : public Builtins<DictValuesBuiltins, SymbolId::kDictValues,
                      LayoutId::kDictValues> {
 public:
  static RawObject dunderIter(Thread* thread, Frame* frame, word nargs);

  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(DictValuesBuiltins);
};

}  // namespace py
