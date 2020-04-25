#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace py {

// Allocate dict.indices() of size `num_indices` and dict.data() accordingly.
void dictAllocateArrays(Thread* thread, const Dict& dict, word indices_len);

// Return true if an item is found at `*index` or after and sets index to the
// next index to probe, key_out to the found key and value_out to the found
// value. Returns false otherwise.
bool dictNextItem(const Dict& dict, word* index, Object* key_out,
                  Object* value_out);

bool dictNextItemHash(const Dict& dict, word* index, Object* key_out,
                      Object* value_out, word* hash_out);

bool dictNextKey(const Dict& dict, word* index, Object* key_out);

bool dictNextValue(const Dict& dict, word* index, Object* value_out);

bool dictNextKeyHash(const Dict& dict, word* index, Object* key_out,
                     word* hash_out);

// Associate a value with the supplied key.
//
// This handles growing the backing Tuple if needed.
RawObject dictAtPut(Thread* thread, const Dict& dict, const Object& key,
                    word hash, const Object& value);

// Does the same as `dictAtPut` but only works for `key` being a `str`
// instance. It must not be used with instances of a `str` subclass.
// `dict` is supposed to contain only `str` keys.
void dictAtPutByStr(Thread* thread, const Dict& dict, const Object& name,
                    const Object& value);

// Does the same as `dictAtPut` but uses symbol `id` (converted to a string)
// as key.
// `dict` is supposed to contain only `str` keys.
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

// Stores value in a ValueCell associated with `name`. Reuses an existing
// value cell when possible.
RawObject dictAtPutInValueCellByStr(Thread* thread, const Dict& dict,
                                    const Object& name, const Object& value);

// Remove all items from a Dict.
void dictClear(Thread* thread, const Dict& dict);

// Returns true if the dict contains the specified key.
RawObject dictIncludes(Thread* thread, const Dict& dict, const Object& key,
                       word hash);

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

class DictBuiltins : public Builtins<DictBuiltins, ID(dict), LayoutId::kDict> {
 public:
  static const BuiltinAttribute kAttributes[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(DictBuiltins);
};

class DictItemIteratorBuiltins
    : public Builtins<DictItemIteratorBuiltins, ID(dict_itemiterator),
                      LayoutId::kDictItemIterator> {
 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(DictItemIteratorBuiltins);
};

class DictItemsBuiltins
    : public Builtins<DictItemsBuiltins, ID(dict_items), LayoutId::kDictItems> {
 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(DictItemsBuiltins);
};

class DictKeyIteratorBuiltins
    : public Builtins<DictKeyIteratorBuiltins, ID(dict_keyiterator),
                      LayoutId::kDictKeyIterator> {
 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(DictKeyIteratorBuiltins);
};

class DictKeysBuiltins
    : public Builtins<DictKeysBuiltins, ID(dict_keys), LayoutId::kDictKeys> {
 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(DictKeysBuiltins);
};

class DictValueIteratorBuiltins
    : public Builtins<DictValueIteratorBuiltins, ID(dict_valueiterator),
                      LayoutId::kDictValueIterator> {
 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(DictValueIteratorBuiltins);
};

class DictValuesBuiltins : public Builtins<DictValuesBuiltins, ID(dict_values),
                                           LayoutId::kDictValues> {
 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(DictValuesBuiltins);
};

}  // namespace py
