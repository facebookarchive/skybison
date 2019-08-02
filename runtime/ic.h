// Inline caches.
#pragma once

#include "handles.h"
#include "objects.h"

namespace python {

// Bitset indicating how a cached binary operation needs to be called.
enum IcBinopFlags : uint8_t {
  IC_BINOP_NONE = 0,
  // Swap arguments when calling the method.
  IC_BINOP_REFLECTED = 1 << 0,
  // Retry alternative method when method returns `NotImplemented`.  Should try
  // the non-reflected op if the `IC_BINOP_REFLECTED` flag is set and vice
  // versa.
  IC_BINOP_NOTIMPLEMENTED_RETRY = 1 << 1,
  // This flag is set when the cached method is an in-place operation (such as
  // __iadd__).
  IC_INPLACE_BINOP_RETRY = 1 << 2,
};

// Looks for a cache entry for an attribute with a `layout_id` key.
// Returns the cached value. Returns `ErrorNotFound` if none was found.
RawObject icLookupAttr(RawTuple caches, word index, LayoutId layout_id);

// Looks for a cache entry with `left_layout_id` and `right_layout_id` as key.
// Returns the cached value comprising of an object reference and flags. Returns
// `ErrorNotFound` if none was found.
RawObject icLookupBinop(RawTuple caches, word index, LayoutId left_layout_id,
                        LayoutId right_layout_id, IcBinopFlags* flags_out);

// Looks for a cache entry for a global variable.
// Returns a ValueCell in case of cache hit.
// Returns the NoneType object otherwise.
RawObject icLookupGlobalVar(RawTuple caches, word index);

// Returns the original argument of bytecode operations that were rewritten by
// `rewriteBytecode()`.
word icOriginalArg(RawFunction function, word index);

// Prepares bytecode for caching: Adds a rewritten variant of the bytecode to
// `function`. It has the arguments of opcodes that use the cache replaced with
// a cache index. The previous arguments are moved to a separate tuple and can
// be retrieved with `icOriginalArg()`. Also adds a correctly sized `caches`
// tuple to `function`.
void icRewriteBytecode(Thread* thread, const Function& function);

// Sets a cache entry for an attribute to the given `layout_id` as key and
// `value` as value.
void icUpdateAttr(RawTuple caches, word index, LayoutId layout_id,
                  RawObject value);

// Insert dependent into dependentLink of the given value_cell. Returns true if
// depdent didn't exist in dependencyLink, and false otherwise.
bool icInsertDependentToValueCellDependencyLink(Thread* thread,
                                                const Object& dependent,
                                                const ValueCell& value_cell);

// Perform the same lookup operation as typeLookupNameInMro as we're inserting
// dependent into the ValueCell in each visited type dictionary.
void icInsertDependencyForTypeLookupInMro(Thread* thread, const Type& type,
                                          const Object& name_str,
                                          const Object& dependent);

// Delete dependent in ValueCell's dependencyLink.
void icDeleteDependentInValueCell(Thread* thread, const ValueCell& value_cell,
                                  const Object& dependent);

// Deletes a function dependency on an attribute affected by a change to some
// type.
void icDoesCacheNeedInvalidationAfterUpdate(Thread* thread,
                                            const Type& cached_type,
                                            const Str& attribute_name,
                                            const Type& updated_type,
                                            const Object& dependent);

// Returns true if a cached attribute from type cached_type is affected by
// an update to type[attribute_name] during MRO lookups.
// with the given mro. assuming that type[attribute_name] exists.
//
// Consider the following example:
//
// class A:
//   def foo(self): return 1
//
// class B(A):
//   pass
//
// class C(B):
//   def foo(self): return 3
//
// When B.foo is cached, an update to A.foo affects the cache, but not the one
// to C.foo.
bool icIsCachedAttributeAffectedByUpdatedType(Thread* thread,
                                              const Type& cached_type,
                                              const Str& attribute_name,
                                              const Type& updated_type);

// Delete caches for attribute_name to be shadowed by an update to
// type[attribute_name] in dependent's cache entries, and delete obsolete
// dependencies between dependent and other type attributes in caches' mro.
void icDeleteCacheForTypeAttrInDependent(Thread* thread,
                                         const Type& updated_type,
                                         const Str& attribute_name,
                                         bool data_descriptor,
                                         const Function& dependent);

// Invalidate caches to be shadowed by a type attribute update made to
// type[attribute_name]. data_descriptor is set to true when the newly assigned
// value to the attribute is a data descriptor. This function is expected to be
// called after the type attribute update is already made.
//
// Refer to https://fb.quip.com/q568ASVbNIad for the details of this process.
void icInvalidateCachesForTypeAttr(Thread* thread, const Type& type,
                                   const Str& attribute_name,
                                   bool data_descriptor);

// Sets a cache entry to a `left_layout_id` and `right_layout_id` key with
// the given `value` and `flags` as value.
void icUpdateBinop(RawTuple caches, word index, LayoutId left_layout_id,
                   LayoutId right_layout_id, RawObject value,
                   IcBinopFlags flags);

// Sets a cache entry for a global variable.
void icUpdateGlobalVar(Thread* thread, const Function& function, word index,
                       const ValueCell& value_cell);

// Invalide global variable cache entries referring to value_cell.
void icInvalidateGlobalVar(Thread* thread, const ValueCell& value_cell);

// Cache layout:
//  The caches for the caching opcodes of a function are joined together in a
//  tuple object.
//
//  +-Global Variable Cache Section -------------------------------------------
//  | 0: global variable cache 0 (for global var with name == code.names.at(0)
//  | ...
//  | k - 1: global variable cache k - 1 where k == code.names.length()
//  +-Method Cache Section -----------------------------------------------------
//  | k: cache 0  (used by the first opcode using an inline cache)
//  |   - 0: entry0 layout_id: layout id to match as SmallInt
//  |   - 1: entry0 target: cached value
//  |   - 2: entry1 layout_id
//  |   - 3: entry1 target
//  |   - 4: entry2 layout_id
//  |   - 5: entry2 target
//  |   - 6: entry3 layout_id
//  |   - 7: entry3 target
//  | k + 8: cache 1  (used by the second opcode using an inline cache)
//  |   - 8: entry0 layout_id
//  |   - 9: entry0 target
//  |     ...
//  | k + n * kIcPointersPerCache: cache n
//  |
//  +--------------------------------------------------------------------------
const int kIcPointersPerEntry = 2;
const int kIcEntriesPerCache = 4;
const int kIcPointersPerCache = kIcEntriesPerCache * kIcPointersPerEntry;

const int kIcEntryKeyOffset = 0;
const int kIcEntryValueOffset = 1;

inline RawObject icLookupAttr(RawTuple caches, word index, LayoutId layout_id) {
  RawSmallInt key = SmallInt::fromWord(static_cast<word>(layout_id));
  for (word i = index * kIcPointersPerCache, end = i + kIcPointersPerCache;
       i < end; i += kIcPointersPerEntry) {
    RawObject entry_key = caches.at(i + kIcEntryKeyOffset);
    if (entry_key == key) {
      return caches.at(i + kIcEntryValueOffset);
    }
  }
  return Error::notFound();
}

inline RawObject icLookupBinop(RawTuple caches, word index,
                               LayoutId left_layout_id,
                               LayoutId right_layout_id,
                               IcBinopFlags* flags_out) {
  static_assert(Header::kLayoutIdBits * 2 + kBitsPerByte <= SmallInt::kBits,
                "Two layout ids and flags overflow a SmallInt");
  word key_high_bits = static_cast<word>(left_layout_id)
                           << Header::kLayoutIdBits |
                       static_cast<word>(right_layout_id);
  for (word i = index * kIcPointersPerCache, end = i + kIcPointersPerCache;
       i < end; i += kIcPointersPerEntry) {
    RawObject entry_key = caches.at(i + kIcEntryKeyOffset);
    // Stop the search if we found an empty entry.
    if (entry_key.isNoneType()) {
      break;
    }
    word entry_key_value = SmallInt::cast(entry_key).value();
    if (entry_key_value >> 8 == key_high_bits) {
      *flags_out = static_cast<IcBinopFlags>(entry_key_value & 0xff);
      return caches.at(i + kIcEntryValueOffset);
    }
  }
  return Error::notFound();
}

inline RawObject icLookupGlobalVar(RawTuple caches, word index) {
  return caches.at(index);
}

inline word icOriginalArg(RawFunction function, word index) {
  return SmallInt::cast(RawTuple::cast(function.originalArguments()).at(index))
      .value();
}

}  // namespace python
