#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace py {

// Associate a value with the supplied key.
//
// This handles growing the backing Tuple if needed.
void dictAtPut(Thread* thread, const Dict& dict, const Object& key, word hash,
               const Object& value);

// Does the same as `dictAtPut` but only works for `key` being a `str`
// instance. It must not be used with instances of a `str` subclass.
void dictAtPutByStr(Thread* thread, const Dict& dict, const Object& name,
                    const Object& value);

// Does the same as `dictAtPut` but uses symbol `id` (converted to a string)
// as key.
void dictAtPutById(Thread* thread, const Dict& dict, SymbolId id,
                   const Object& value);

// Look up the value associated with `key`. Returns the associated value or
// `Error::notFound()`.
RawObject dictAt(Thread* thread, const Dict& dict, const Object& key,
                 word hash);

// Look up the value associated with `name`. `name` must be an instance of
// `str` but not of a subclass. Returns the associated value or
// `Error::notFound()`.
RawObject dictAtByStr(Thread* thread, const Dict& dict, const Object& name);

// Look up the value associated with `id`. Returns the associated value or
// `Error::notFound()`.
RawObject dictAtById(Thread* thread, const Dict& dict, SymbolId id);

// Looks up and returns the value associated with the key.  If the key is
// absent, calls thunk and inserts its result as the value.
RawObject dictAtIfAbsentPut(Thread* thread, const Dict& dict, const Object& key,
                            word hash, Callback<RawObject>* thunk);

// Stores value in a ValueCell associated with `key`. Reuses an existing
// value cell when possible.
RawObject dictAtPutInValueCell(Thread* thread, const Dict& dict,
                               const Object& key, word hash,
                               const Object& value);

// Stores value in a ValueCell associated with `name`. Reuses an existing
// value cell when possible.
RawObject dictAtPutInValueCellByStr(Thread* thread, const Dict& dict,
                                    const Object& name, const Object& value);

// Remove all items from a Dict.
void dictClear(Thread* thread, const Dict& dict);

// Returns true if the dict contains the specified key.
bool dictIncludes(Thread* thread, const Dict& dict, const Object& key,
                  word hash);

// Returns true if the dict contains an entry associated with `name`.
// `name` must by a `str` instance (but no subclass).
bool dictIncludesByStr(Thread* thread, const Dict& dict, const Object& name);

// TODO(T46009010): Make this into a private function after making
// 'builtins._dict_setitem' into a dict builtin function
void dictEnsureCapacity(Thread* thread, const Dict& dict);

// Try to remove entry associated with `key` from `dict`.
// Returns the value that was associated before deletion or
// `Error:notFound()`.
RawObject dictRemove(Thread* thread, const Dict& dict, const Object& key,
                     word hash);

// Try to remove entry associated with `name` from `dict`.
// Returns the value that was associated before deletion or
// `Error:notFound()`.
RawObject dictRemoveByStr(Thread* thread, const Dict& dict, const Object& name);

RawObject dictKeys(Thread* thread, const Dict& dict);

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

RawObject METH(dict, clear)(Thread* thread, Frame* frame, word nargs);
RawObject METH(dict, __delitem__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(dict, __eq__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(dict, __len__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(dict, __iter__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(dict, __new__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(dict, items)(Thread* thread, Frame* frame, word nargs);
RawObject METH(dict, keys)(Thread* thread, Frame* frame, word nargs);
RawObject METH(dict, values)(Thread* thread, Frame* frame, word nargs);

class DictBuiltins : public Builtins<DictBuiltins, ID(dict), LayoutId::kDict> {
 public:
  static const BuiltinAttribute kAttributes[];
  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(DictBuiltins);
};

RawObject METH(dict_itemiterator, __iter__)(Thread* thread, Frame* frame,
                                            word nargs);
RawObject METH(dict_itemiterator, __length_hint__)(Thread* thread, Frame* frame,
                                                   word nargs);
RawObject METH(dict_itemiterator, __next__)(Thread* thread, Frame* frame,
                                            word nargs);

class DictItemIteratorBuiltins
    : public Builtins<DictItemIteratorBuiltins, ID(dict_itemiterator),
                      LayoutId::kDictItemIterator> {
 public:
  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(DictItemIteratorBuiltins);
};

RawObject METH(dict_items, __iter__)(Thread* thread, Frame* frame, word nargs);

class DictItemsBuiltins
    : public Builtins<DictItemsBuiltins, ID(dict_items), LayoutId::kDictItems> {
 public:
  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(DictItemsBuiltins);
};

RawObject METH(dict_keyiterator, __iter__)(Thread* thread, Frame* frame,
                                           word nargs);
RawObject METH(dict_keyiterator, __length_hint__)(Thread* thread, Frame* frame,
                                                  word nargs);
RawObject METH(dict_keyiterator, __next__)(Thread* thread, Frame* frame,
                                           word nargs);

class DictKeyIteratorBuiltins
    : public Builtins<DictKeyIteratorBuiltins, ID(dict_keyiterator),
                      LayoutId::kDictKeyIterator> {
 public:
  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(DictKeyIteratorBuiltins);
};

RawObject METH(dict_keys, __iter__)(Thread* thread, Frame* frame, word nargs);

class DictKeysBuiltins
    : public Builtins<DictKeysBuiltins, ID(dict_keys), LayoutId::kDictKeys> {
 public:
  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(DictKeysBuiltins);
};

RawObject METH(dict_valueiterator, __iter__)(Thread* thread, Frame* frame,
                                             word nargs);
RawObject METH(dict_valueiterator, __length_hint__)(Thread* thread,
                                                    Frame* frame, word nargs);
RawObject METH(dict_valueiterator, __next__)(Thread* thread, Frame* frame,
                                             word nargs);

class DictValueIteratorBuiltins
    : public Builtins<DictValueIteratorBuiltins, ID(dict_valueiterator),
                      LayoutId::kDictValueIterator> {
 public:
  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(DictValueIteratorBuiltins);
};

RawObject METH(dict_values, __iter__)(Thread* thread, Frame* frame, word nargs);

class DictValuesBuiltins : public Builtins<DictValuesBuiltins, ID(dict_values),
                                           LayoutId::kDictValues> {
 public:
  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(DictValuesBuiltins);
};

}  // namespace py
