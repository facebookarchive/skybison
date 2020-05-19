// Inline caches.
#pragma once

#include "bytecode.h"
#include "handles.h"
#include "interpreter.h"
#include "objects.h"

namespace py {

// Looks for a cache entry for an attribute with a `layout_id` key in the
// polymorphic cache at `index`.
// Returns the cached value. Returns `ErrorNotFound` if none was found.
RawObject icLookupPolymorphic(RawMutableTuple caches, word index,
                              LayoutId layout_id, bool* is_found);

// Returns the cached value in the monomorphic cache at `index`.
// Returns `ErrorNotFound` if none was found, and set *is_found to false.
RawObject icLookupMonomorphic(RawMutableTuple caches, word index,
                              LayoutId layout_id, bool* is_found);

// Returns the current state of the cache at caches[index].
ICState icCurrentState(RawTuple caches, word index);

// Looks for a cache entry with `left_layout_id` and `right_layout_id` as key.
// Returns the cached value comprising of an object reference and flags. Returns
// `ErrorNotFound` if none was found.
RawObject icLookupBinOpPolymorphic(RawMutableTuple caches, word index,
                                   LayoutId left_layout_id,
                                   LayoutId right_layout_id,
                                   BinaryOpFlags* flags_out);

// Same as icLookupBinaryOp, but looks at only one entry pointed by index.
RawObject icLookupBinOpMonomorphic(RawMutableTuple caches, word index,
                                   LayoutId left_layout_id,
                                   LayoutId right_layout_id,
                                   BinaryOpFlags* flags_out);

// Looks for a cache entry for a global variable.
// Returns a ValueCell in case of cache hit.
// Returns the NoneType object otherwise.
RawObject icLookupGlobalVar(RawMutableTuple caches, word index);

// Internal only: remove deallocated nodes from cell.dependencyLink.
void icRemoveDeadWeakLinks(RawValueCell cell);

RawSmallInt encodeBinaryOpKey(LayoutId left_layout_id, LayoutId right_layout_id,
                              BinaryOpFlags flags);

// Sets a cache entry for an attribute to the given `layout_id` as key and
// `value` as value.
ICState icUpdateAttr(Thread* thread, const MutableTuple& caches, word index,
                     LayoutId layout_id, const Object& value,
                     const Object& name, const Function& dependent);

bool icIsCacheEmpty(const MutableTuple& caches, word index);

void icUpdateAttrModule(Thread* thread, const MutableTuple& caches, word index,
                        const Object& receiver, const ValueCell& value_cell,
                        const Function& dependent);

void icUpdateAttrType(Thread* thread, const MutableTuple& caches, word index,
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
                      const ValueCell& value_cell);

// Sets a cache entry to a `left_layout_id` and `right_layout_id` key with
// the given `value` and `flags` as value.
ICState icUpdateBinOp(Thread* thread, const MutableTuple& caches, word index,
                      LayoutId left_layout_id, LayoutId right_layout_id,
                      const Object& value, BinaryOpFlags flags);

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
//  |   - 0: layout_id: layout id to match as SmallInt
//  |   - 1: target: cached value
//  | k: cache 1  (used by the second opcode)
//  |   - 0: Unbound
//  |   - 1: pointer to a data sturcutre for the polymorphic cache
//  |     ...
//  | k + n * kIcPointersPerEntry: cache n
//  |
//  +--------------------------------------------------------------------------
const int kIcPointersPerEntry = 2;

const int kIcEntriesPerPolyCache = 4;
const int kIcPointersPerPolyCache =
    kIcEntriesPerPolyCache * kIcPointersPerEntry;

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
        end_cache_index_(0),
        polymorphic_cache_index_(-1) {
    next();
  }

  bool hasNext() { return cache_index_ >= 0; }

  void next() {
    if (polymorphic_cache_index_ >= 0) {
      // We're currently iterating through a polymorphic cache.
      RawMutableTuple polymorphic_cache =
          MutableTuple::cast(caches_.at(cache_index_ + kIcEntryValueOffset));
      polymorphic_cache_index_ += kIcPointersPerEntry;
      for (; polymorphic_cache_index_ < kIcPointersPerPolyCache;
           polymorphic_cache_index_ += kIcPointersPerEntry) {
        if (!polymorphic_cache.at(polymorphic_cache_index_ + kIcEntryKeyOffset)
                 .isNoneType()) {
          return;
        }
      }
      polymorphic_cache_index_ = -1;
    }

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
      cache_index_ = bytecode_op_.arg * kIcPointersPerEntry;
      end_cache_index_ = cache_index_ + kIcPointersPerEntry;
      cache_index_ =
          findNextFilledCacheIndex(caches_, cache_index_, end_cache_index_);
      if (cache_index_ >= 0) {
        if (caches_.at(cache_index_ + kIcEntryKeyOffset).isUnbound()) {
          polymorphic_cache_index_ = 0;
        }
        return;
      }
    }
  }

  bool isAttrCache() const {
    switch (bytecode_op_.bc) {
      case BINARY_SUBSCR_ANAMORPHIC:
      case BINARY_SUBSCR_MONOMORPHIC:
      case BINARY_SUBSCR_POLYMORPHIC:
      case FOR_ITER_MONOMORPHIC:
      case FOR_ITER_POLYMORPHIC:
      case FOR_ITER_ANAMORPHIC:
      case LOAD_ATTR_INSTANCE:
      case LOAD_ATTR_INSTANCE_PROPERTY:
      case LOAD_ATTR_INSTANCE_SLOT_DESCR:
      case LOAD_ATTR_INSTANCE_TYPE:
      case LOAD_ATTR_INSTANCE_TYPE_BOUND_METHOD:
      case LOAD_ATTR_INSTANCE_TYPE_DESCR:
      case LOAD_ATTR_TYPE:
      case LOAD_ATTR_ANAMORPHIC:
      case LOAD_METHOD_ANAMORPHIC:
      case LOAD_METHOD_INSTANCE_FUNCTION:
      case LOAD_METHOD_POLYMORPHIC:
      case STORE_ATTR_INSTANCE:
      case STORE_ATTR_INSTANCE_OVERFLOW:
      case STORE_ATTR_INSTANCE_OVERFLOW_UPDATE:
      case STORE_ATTR_INSTANCE_UPDATE:
      case STORE_ATTR_POLYMORPHIC:
      case STORE_ATTR_ANAMORPHIC:
      case STORE_SUBSCR_ANAMORPHIC:
        return true;
      default:
        return false;
    }
  }

  bool isModuleAttrCache() const { return bytecode_op_.bc == LOAD_ATTR_MODULE; }

  bool isBinaryOpCache() const {
    switch (bytecode_op_.bc) {
      case BINARY_OP_MONOMORPHIC:
      case BINARY_OP_POLYMORPHIC:
      case BINARY_OP_ANAMORPHIC:
      case COMPARE_OP_MONOMORPHIC:
      case COMPARE_OP_POLYMORPHIC:
      case COMPARE_OP_ANAMORPHIC:
        return true;
      default:
        return false;
    }
  }

  bool isInplaceOpCache() const {
    switch (bytecode_op_.bc) {
      case INPLACE_OP_MONOMORPHIC:
      case INPLACE_OP_POLYMORPHIC:
      case INPLACE_OP_ANAMORPHIC:
        return true;
      default:
        return false;
    }
  }

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
    return value().isSmallInt();
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
    if (polymorphic_cache_index_ < 0) {
      caches_.atPut(cache_index_ + kIcEntryKeyOffset, NoneType::object());
      caches_.atPut(cache_index_ + kIcEntryValueOffset, NoneType::object());
      return;
    }
    RawMutableTuple polymorphic_cache =
        MutableTuple::cast(caches_.at(cache_index_ + kIcEntryValueOffset));
    polymorphic_cache.atPut(polymorphic_cache_index_ + kIcEntryKeyOffset,
                            NoneType::object());
    polymorphic_cache.atPut(polymorphic_cache_index_ + kIcEntryValueOffset,
                            NoneType::object());
  }

 private:
  static word findNextFilledCacheIndex(const MutableTuple& caches,
                                       word cache_index, word end_cache_index) {
    for (; cache_index < end_cache_index; cache_index += kIcPointersPerEntry) {
      if (!caches.at(cache_index + kIcEntryKeyOffset).isNoneType()) {
        return cache_index;
      }
    }
    return -1;
  }

  RawObject key() const {
    if (polymorphic_cache_index_ < 0) {
      return caches_.at(cache_index_ + kIcEntryKeyOffset);
    }
    RawMutableTuple polymorphic_cache =
        MutableTuple::cast(caches_.at(cache_index_ + kIcEntryValueOffset));
    return polymorphic_cache.at(polymorphic_cache_index_ + kIcEntryKeyOffset);
  }

  RawObject value() const {
    if (polymorphic_cache_index_ < 0) {
      return caches_.at(cache_index_ + kIcEntryValueOffset);
    }
    RawMutableTuple polymorphic_cache =
        MutableTuple::cast(caches_.at(cache_index_ + kIcEntryValueOffset));
    return polymorphic_cache.at(polymorphic_cache_index_ + kIcEntryValueOffset);
  }

  Runtime* runtime_;
  MutableBytes bytecode_;
  MutableTuple caches_;
  Function function_;
  Tuple names_;

  word bytecode_index_;
  BytecodeOp bytecode_op_;
  word cache_index_;
  word end_cache_index_;
  word polymorphic_cache_index_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(IcIterator);
  DISALLOW_HEAP_ALLOCATION();
};

inline RawObject icLookupPolymorphic(RawMutableTuple caches, word index,
                                     LayoutId layout_id, bool* is_found) {
  word i = index * kIcPointersPerEntry;
  DCHECK(caches.at(i + kIcEntryKeyOffset).isUnbound(),
         "cache.at(index) is expected to be polymorphic");
  RawSmallInt key = SmallInt::fromWord(static_cast<word>(layout_id));
  caches = MutableTuple::cast(caches.at(i + kIcEntryValueOffset));
  for (word j = 0; j < kIcPointersPerPolyCache; j += kIcPointersPerEntry) {
    if (caches.at(j + kIcEntryKeyOffset) == key) {
      *is_found = true;
      return caches.at(j + kIcEntryValueOffset);
    }
  }
  *is_found = false;
  return Error::notFound();
}

inline RawObject icLookupMonomorphic(RawMutableTuple caches, word index,
                                     LayoutId layout_id, bool* is_found) {
  word i = index * kIcPointersPerEntry;
  DCHECK(!caches.at(i + kIcEntryKeyOffset).isUnbound(),
         "cache.at(index) is expected to be monomorphic");
  RawSmallInt key = SmallInt::fromWord(static_cast<word>(layout_id));
  if (caches.at(i + kIcEntryKeyOffset) == key) {
    *is_found = true;
    return caches.at(i + kIcEntryValueOffset);
  }
  *is_found = false;
  return Error::notFound();
}

inline RawObject icLookupBinOpPolymorphic(RawMutableTuple caches, word index,
                                          LayoutId left_layout_id,
                                          LayoutId right_layout_id,
                                          BinaryOpFlags* flags_out) {
  static_assert(Header::kLayoutIdBits * 2 + kBitsPerByte <= SmallInt::kBits,
                "Two layout ids and flags overflow a SmallInt");
  word i = index * kIcPointersPerEntry;
  DCHECK(caches.at(i + kIcEntryKeyOffset).isUnbound(),
         "cache.at(index) is expected to be polymorphic");
  word key_high_bits = static_cast<word>(left_layout_id)
                           << Header::kLayoutIdBits |
                       static_cast<word>(right_layout_id);
  caches = MutableTuple::cast(caches.at(i + kIcEntryValueOffset));
  for (word j = 0; j < kIcPointersPerPolyCache; j += kIcPointersPerEntry) {
    RawObject entry_key = caches.at(j + kIcEntryKeyOffset);
    // Stop the search if we found an empty entry.
    if (entry_key.isNoneType()) {
      break;
    }
    word entry_key_value = SmallInt::cast(entry_key).value();
    if (entry_key_value >> kBitsPerByte == key_high_bits) {
      *flags_out = static_cast<BinaryOpFlags>(entry_key_value & 0xff);
      return caches.at(j + kIcEntryValueOffset);
    }
  }
  return Error::notFound();
}

inline RawObject icLookupBinOpMonomorphic(RawMutableTuple caches, word index,
                                          LayoutId left_layout_id,
                                          LayoutId right_layout_id,
                                          BinaryOpFlags* flags_out) {
  static_assert(Header::kLayoutIdBits * 2 + kBitsPerByte <= SmallInt::kBits,
                "Two layout ids and flags overflow a SmallInt");
  word key_high_bits = static_cast<word>(left_layout_id)
                           << Header::kLayoutIdBits |
                       static_cast<word>(right_layout_id);
  word i = index * kIcPointersPerEntry;
  DCHECK(!caches.at(i + kIcEntryKeyOffset).isUnbound(),
         "cache.at(index) is expected to be monomorphic");
  RawObject entry_key = caches.at(i + kIcEntryKeyOffset);
  // Stop the search if we found an empty entry.
  if (entry_key.isNoneType()) {
    return Error::notFound();
  }
  word entry_key_value = SmallInt::cast(entry_key).value();
  if (entry_key_value >> kBitsPerByte == key_high_bits) {
    *flags_out = static_cast<BinaryOpFlags>(entry_key_value & 0xff);
    return caches.at(i + kIcEntryValueOffset);
  }
  return Error::notFound();
}

inline RawObject icLookupGlobalVar(RawMutableTuple caches, word index) {
  return caches.at(index);
}

}  // namespace py
