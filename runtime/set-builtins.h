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

// Return true if an item is found at `*index` or after and sets index to the
// next index to probe, hash_out to the hash and value_out to the found
// value. Returns false otherwise.
bool setNextItem(const SetBase& set, word* index, RawObject* value_out);

bool setNextItemHash(const SetBase& set, word* index, RawObject* value_out,
                     word* hash_out);

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

void initializeSetTypes(Thread* thread);

}  // namespace py
