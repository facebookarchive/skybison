// Inline caches.
#pragma once

#include "bytecode.h"
#include "handles.h"
#include "interpreter.h"
#include "objects.h"

namespace py {

// Returns true if function is found on a value cell's dependency list.
bool icDependentIncluded(RawObject dependent, RawObject link);

// Looks for a cache entry for an attribute with a `layout_id` key.
// Returns the cached value. Returns `ErrorNotFound` if none was found.
RawObject icLookupAttr(RawTuple caches, word index, LayoutId layout_id);

// Looks for a cache entry with `left_layout_id` and `right_layout_id` as key.
// Returns the cached value comprising of an object reference and flags. Returns
// `ErrorNotFound` if none was found.
RawObject icLookupBinaryOp(RawTuple caches, word index, LayoutId left_layout_id,
                           LayoutId right_layout_id, BinaryOpFlags* flags_out);

// Looks for a cache entry for a global variable.
// Returns a ValueCell in case of cache hit.
// Returns the NoneType object otherwise.
RawObject icLookupGlobalVar(RawTuple caches, word index);

// Sets a cache entry for an attribute to the given `layout_id` as key and
// `value` as value.
void icUpdateAttr(Thread* thread, const Tuple& caches, word index,
                  LayoutId layout_id, const Object& value, const Object& name,
                  const Function& dependent);

bool icIsCacheEmpty(const Tuple& caches, word index);

void icUpdateAttrModule(Thread* thread, const Tuple& caches, word index,
                        const Object& receiver, const ValueCell& value_cell,
                        const Function& dependent);

void icUpdateAttrType(Thread* thread, const Tuple& caches, word index,
                      const Object& receiver, const Object& selector,
                      const Object& value, const Function& dependent);

// Insert dependent into dependentLink of the given value_cell. Returns true if
// depdent didn't exist in dependencyLink, and false otherwise.
bool icInsertDependentToValueCellDependencyLink(Thread* thread,
                                                const Object& dependent,
                                                const ValueCell& value_cell);

// Insert dependencies for `a binary_op b` where layout_id(a) == left_layout_id,
// layout_id(b) == right_layout_id.
void icInsertBinaryOpDependencies(Thread* thread, const Function& dependent,
                                  LayoutId left_layout_id,
                                  LayoutId right_layout_id,
                                  Interpreter::BinaryOp op);

// Insert dependencies for `a compare_op b` where layout_id(a) == left_layout_id
// ,layout_id(b) == right_layout_id.
void icInsertCompareOpDependencies(Thread* thread, const Function& dependent,
                                   LayoutId left_layout_id,
                                   LayoutId right_layout_id, CompareOp op);

// Insert dependencies for `a inplace_op b` where layout_id(a) == left_layout_id
// ,layout_id(b) == right_layout_id, and op is an inplace operation (e.g., +=).
void icInsertInplaceOpDependencies(Thread* thread, const Function& dependent,
                                   LayoutId left_layout_id,
                                   LayoutId right_layout_id,
                                   Interpreter::BinaryOp op);

class IcIterator;

enum class AttributeKind { kDataDescriptor, kNotADataDescriptor };

// Try evicting the attribute cache entry pointed-to by `it` and its
// dependencies to dependent.
void icEvictAttr(Thread* thread, const IcIterator& it, const Type& updated_type,
                 const Object& updated_attr, AttributeKind attribute_kind,
                 const Function& dependent);

// Delete dependent from the MRO of cached_layout_id up to the type defining
// `attr`.
void icDeleteDependentToDefiningType(Thread* thread, const Function& dependent,
                                     LayoutId cached_layout_id,
                                     const Object& attr);

// icEvictBinaryOp tries evicting the binary op cache pointed-to by `it` and
// deletes the evicted cache' dependencies.
//
// - Invalidation condition
// A binop cache for a op b gets invalidated when either type(A).op gets updated
// or type(B).rop gets updated. For example,  an update to A.__ge__ invalidates
// all caches created for a >= other or other <= a where type(a) = A.
//
// - Deleting dependencies
// After a binop cache is evicted in function f, we need to delete f's
// dependencies on type attributes referenced by that cache.
//
// When a binop cache gets invalidated due to either one of the operand's type
// update, the dependencies between f and the directly updated operand't type
// are deleted first.
//
// We also delete dependencies for f created from another operand's type if that
// operand type is not used in f.
//
// The following code snippet shows a concrete example of such dependency
// deletion.
//
// 1 def cache_binop(a, b):
// 2   if b <= 5:
// 3     pass
// 4   return a >= b
// 5
// 6 cache_binop(A(), B())
//
// After executing the code above, A.__ge__'s dependency list will contain
// cache_binop and B.__le__ will do so too.
//
// When A.__ge__ = something, that created cache is evicted, and we delete the
// dependency from A.__ge__ to cache_binop directly. Since the cache is evicted,
// cache_binop may not depend on B.__le__ anymore. To confirm if we can delete
// the dependency from B.__le__ to cache_binop, we scan all caches of
// cache_binop to make sure if any of caches in cache_binop references B.__le__,
// and only if not, we delete the dependency.
//
// In the example, since cache_binop still caches B.__le__ at line 2 we cannot
// delete this dependency. If line 2 didn't exist, we could delete it.
void icEvictBinaryOp(Thread* thread, const IcIterator& it,
                     const Type& updated_type, const Object& updated_attr,
                     const Function& dependent);

// Similar to icEvictBinopCache, but handle updates to dunder functions for
// inplace operations (e.g., iadd, imul, and etc.).
// TODO(T54575269): Pass SymbolId for updated_attr.
void icEvictInplaceOp(Thread* thread, const IcIterator& it,
                      const Type& updated_type, const Object& updated_attr,
                      const Function& dependent);

// Delete dependent in ValueCell's dependencyLink.
void icDeleteDependentInValueCell(Thread* thread, const ValueCell& value_cell,
                                  const Object& dependent);

// Delete dependencies from cached_type to new_defining_type to dependent.
void icDeleteDependentFromInheritingTypes(Thread* thread,
                                          LayoutId cached_layout_id,
                                          const Object& attr_name,
                                          const Type& new_defining_type,
                                          const Object& dependent);

// Returns the highest supertype of `cached_type` where all types between
// `cached_type` and that supertype in `cached_type`'s MRO are not cached in
// dependent. In case such a supertype is not found, returns ErrorNotFound. If a
// type is returned, it's safe to delete `dependent` from all types between
// `cached_type` and the returned type.
RawObject icHighestSuperTypeNotInMroOfOtherCachedTypes(
    Thread* thread, LayoutId cached_layout_id, const Object& attr_name,
    const Function& dependent);

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
                                              LayoutId cached_layout_id,
                                              const Object& attribute_name,
                                              const Type& updated_type);

// Returns true if updating type.attribute_name will affect any caches in
// function in any form. For example, updating A.foo will affect any opcode
// that caches A's subclasses' foo attributes.
bool icIsAttrCachedInDependent(Thread* thread, const Type& type,
                               const Object& attr_name,
                               const Function& dependent);

// Evict caches for attribute_name to be shadowed by an update to
// type[attribute_name] in dependent's cache entries, and delete obsolete
// dependencies between dependent and other type attributes in caches' MRO.
void icEvictCache(Thread* thread, const Function& dependent, const Type& type,
                  const Object& attr_name, AttributeKind attribute_kind);

// Invalidate caches to be shadowed by a type attribute update made to
// type[attribute_name]. data_descriptor is set to true when the newly assigned
// value to the attribute is a data descriptor. This function is expected to be
// called after the type attribute update is already made.
//
// Refer to https://fb.quip.com/q568ASVbNIad for the details of this process.
void icInvalidateAttr(Thread* thread, const Type& type, const Object& attr_name,
                      const ValueCell& value);

// Sets a cache entry to a `left_layout_id` and `right_layout_id` key with
// the given `value` and `flags` as value.
void icUpdateBinaryOp(RawTuple caches, word index, LayoutId left_layout_id,
                      LayoutId right_layout_id, RawObject value,
                      BinaryOpFlags flags);

// Sets a cache entry for a global variable.
void icUpdateGlobalVar(Thread* thread, const Function& function, word index,
                       const ValueCell& value_cell);

// Invalidate all of the caches entries with this global variable.
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

// TODO(T54277418): Use SymbolId for binop method names.
class IcIterator {
 public:
  IcIterator(HandleScope* scope, Runtime* runtime, RawFunction function)
      : runtime_(runtime),
        bytecode_(scope, function.rewrittenBytecode()),
        caches_(scope, function.caches()),
        function_(scope, function),
        names_(scope, Code::cast(function.code()).names()),
        bytecode_index_(0),
        cache_index_(0),
        end_cache_index_(0) {
    next();
  }

  bool hasNext() { return cache_index_ >= 0; }

  void next() {
    cache_index_ = findNextFilledCacheIndex(
        caches_, cache_index_ + kIcPointersPerEntry, end_cache_index_);
    if (cache_index_ >= 0) {
      return;
    }

    // Find the next caching opcode.
    word bytecode_length = bytecode_.length();
    while (bytecode_index_ < bytecode_length) {
      BytecodeOp op = nextBytecodeOp(bytecode_, &bytecode_index_);
      if (!isByteCodeWithCache(op.bc)) continue;
      bytecode_op_ = op;
      cache_index_ = bytecode_op_.arg * kIcPointersPerCache;
      end_cache_index_ = cache_index_ + kIcPointersPerCache;
      cache_index_ =
          findNextFilledCacheIndex(caches_, cache_index_, end_cache_index_);
      if (cache_index_ >= 0) {
        return;
      }
    }
  }

  bool isAttrCache() const {
    switch (bytecode_op_.bc) {
      case LOAD_ATTR_CACHED:
      case LOAD_ATTR_INSTANCE_PROPERTY:
      case LOAD_ATTR_TYPE:
      case LOAD_METHOD_CACHED:
      case STORE_ATTR_CACHED:
      case STORE_ATTR_INSTANCE_UPDATE:
      case FOR_ITER_CACHED:
      case BINARY_SUBSCR_CACHED:
        return true;
      default:
        return false;
    }
  }

  bool isModuleAttrCache() const { return bytecode_op_.bc == LOAD_ATTR_MODULE; }

  bool isBinaryOpCache() const {
    switch (bytecode_op_.bc) {
      case BINARY_OP_CACHED:
      case COMPARE_OP_CACHED:
        return true;
      default:
        return false;
    }
  }

  bool isInplaceOpCache() const { return bytecode_op_.bc == INPLACE_OP_CACHED; }

  LayoutId layoutId() const {
    DCHECK(isAttrCache(), "should be only called for attribute caches");
    if (bytecode_op_.bc == LOAD_ATTR_TYPE) {
      RawType type = key().rawCast<RawType>();
      return Layout::cast(type.instanceLayout()).id();
    }
    return static_cast<LayoutId>(SmallInt::cast(key()).value());
  }

  bool isAttrNameEqualTo(const Object& attr_name) const;

  bool isInstanceAttr() const {
    DCHECK(isAttrCache(), "should be only called for attribute caches");
    RawObject value = caches_.at(cache_index_ + kIcEntryValueOffset);
    return value.isSmallInt();
  }

  LayoutId leftLayoutId() const {
    DCHECK(isBinaryOpCache() || isInplaceOpCache(),
           "should be only called for binop or inplace-binop caches");
    word cache_key_value = SmallInt::cast(key()).value() >> 8;
    return static_cast<LayoutId>(cache_key_value >> Header::kLayoutIdBits);
  }

  LayoutId rightLayoutId() const {
    DCHECK(isBinaryOpCache() || isInplaceOpCache(),
           "should be only called for binop or inplace-binop caches");
    word cache_key_value = SmallInt::cast(key()).value() >> 8;
    return static_cast<LayoutId>(cache_key_value &
                                 ((1 << Header::kLayoutIdBits) - 1));
  }

  RawObject leftMethodName() const;

  RawObject rightMethodName() const;

  RawObject inplaceMethodName() const;

  void evict() const {
    caches_.atPut(cache_index_ + kIcEntryKeyOffset, NoneType::object());
    caches_.atPut(cache_index_ + kIcEntryValueOffset, NoneType::object());
  }

 private:
  static word findNextFilledCacheIndex(const Tuple& caches, word cache_index,
                                       word end_cache_index) {
    for (; cache_index < end_cache_index; cache_index += kIcPointersPerEntry) {
      if (!caches.at(cache_index + kIcEntryKeyOffset).isNoneType()) {
        return cache_index;
      }
    }
    return -1;
  }

  RawObject key() const { return caches_.at(cache_index_ + kIcEntryKeyOffset); }

  Runtime* runtime_;
  MutableBytes bytecode_;
  Tuple caches_;
  Function function_;
  Tuple names_;

  word bytecode_index_;
  BytecodeOp bytecode_op_;
  word cache_index_;
  word end_cache_index_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(IcIterator);
  DISALLOW_HEAP_ALLOCATION();
};

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

inline RawObject icLookupBinaryOp(RawTuple caches, word index,
                                  LayoutId left_layout_id,
                                  LayoutId right_layout_id,
                                  BinaryOpFlags* flags_out) {
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
      *flags_out = static_cast<BinaryOpFlags>(entry_key_value & 0xff);
      return caches.at(i + kIcEntryValueOffset);
    }
  }
  return Error::notFound();
}

inline RawObject icLookupGlobalVar(RawTuple caches, word index) {
  return caches.at(index);
}

}  // namespace py
