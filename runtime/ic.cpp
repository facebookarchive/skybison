#include "ic.h"

#include "bytearray-builtins.h"
#include "bytecode.h"
#include "runtime.h"
#include "utils.h"

namespace python {

void icUpdateAttr(RawTuple caches, word index, LayoutId layout_id,
                  RawObject value) {
  RawSmallInt key = SmallInt::fromWord(static_cast<word>(layout_id));
  RawObject entry_key = NoneType::object();
  for (word i = index * kIcPointersPerCache, end = i + kIcPointersPerCache;
       i < end; i += kIcPointersPerEntry) {
    entry_key = caches.at(i + kIcEntryKeyOffset);
    if (entry_key.isNoneType() || entry_key == key) {
      caches.atPut(i + kIcEntryKeyOffset, key);
      caches.atPut(i + kIcEntryValueOffset, value);
      return;
    }
  }
}

static bool dependentIncluded(RawObject dependent, RawObject link) {
  for (; !link.isNoneType(); link = WeakLink::cast(link).next()) {
    if (WeakLink::cast(link).referent() == dependent) {
      return true;
    }
  }
  return false;
}

bool icInsertDependentToValueCellDependencyLink(Thread* thread,
                                                const Object& dependent,
                                                const ValueCell& value_cell) {
  HandleScope scope(thread);
  Object link(&scope, value_cell.dependencyLink());
  if (dependentIncluded(*dependent, *link)) {
    // Already included.
    return false;
  }
  Object none(&scope, NoneType::object());
  WeakLink new_link(
      &scope, thread->runtime()->newWeakLink(thread, dependent, none, link));
  if (link.isWeakLink()) {
    WeakLink::cast(*link).setPrev(*new_link);
  }
  value_cell.setDependencyLink(*new_link);
  return true;
}

void icInsertDependencyForTypeLookupInMro(Thread* thread, const Type& type,
                                          const Object& name_str,
                                          const Object& dependent) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Tuple mro(&scope, type.mro());
  NoneType none(&scope, NoneType::object());
  for (word i = 0; i < mro.length(); i++) {
    Type mro_type(&scope, mro.at(i));
    Dict dict(&scope, mro_type.dict());
    // TODO(T46428372): Consider using a specialized dict lookup to avoid 2
    // probings.
    Object result(&scope, runtime->dictAt(thread, dict, name_str));
    DCHECK(result.isErrorNotFound() || result.isValueCell(),
           "value must be ValueCell if found");
    if (result.isErrorNotFound()) {
      result = runtime->dictAtPutInValueCell(thread, dict, name_str, none);
      ValueCell::cast(*result).makePlaceholder();
    }
    ValueCell value_cell(&scope, *result);
    icInsertDependentToValueCellDependencyLink(thread, dependent, value_cell);
    if (!value_cell.isPlaceholder()) {
      // Attribute lookup terminates here. Therefore, no dependency tracking is
      // needed afterwards.
      return;
    }
  }
}

void icDeleteDependentInValueCell(Thread* thread, const ValueCell& value_cell,
                                  const Object& dependent) {
  HandleScope scope(thread);
  Object link(&scope, value_cell.dependencyLink());
  Object prev(&scope, NoneType::object());
  while (!link.isNoneType()) {
    WeakLink weak_link(&scope, *link);
    if (weak_link.referent() == *dependent) {
      if (weak_link.next().isWeakLink()) {
        WeakLink::cast(weak_link.next()).setPrev(*prev);
      }
      if (prev.isWeakLink()) {
        WeakLink::cast(*prev).setNext(weak_link.next());
      } else {
        value_cell.setDependencyLink(weak_link.next());
      }
      break;
    }
    prev = *link;
    link = weak_link.next();
  }
}

void icDoesCacheNeedInvalidationAfterUpdate(Thread* thread,
                                            const Type& cached_type,
                                            const Str& attribute_name,
                                            const Type& updated_type,
                                            const Object& dependent) {
  DCHECK(icIsCachedAttributeAffectedByUpdatedType(thread, cached_type,
                                                  attribute_name, updated_type),
         "icIsTypeAttrFromMro must return true");
  HandleScope scope(thread);
  Tuple mro(&scope, cached_type.mro());
  Runtime* runtime = thread->runtime();
  for (word i = 0; i < mro.length(); ++i) {
    Type type(&scope, mro.at(i));
    Dict dict(&scope, type.dict());
    ValueCell value_cell(&scope, runtime->dictAt(thread, dict, attribute_name));
    icDeleteDependentInValueCell(thread, value_cell, dependent);
    if (type == updated_type) {
      DCHECK(!value_cell.isPlaceholder(),
             "value_cell for updated_type must not be Placeholder");
      return;
    }
    DCHECK(value_cell.isPlaceholder(),
           "value_cell below updated_type must be Placeholder");
  }
}

static bool isTypeInMro(RawType type, RawTuple mro) {
  for (word i = 0; i < mro.length(); ++i) {
    if (type == mro.at(i)) {
      return true;
    }
  }
  return false;
}

bool icIsCachedAttributeAffectedByUpdatedType(Thread* thread,
                                              const Type& cached_type,
                                              const Str& attribute_name,
                                              const Type& updated_type) {
  HandleScope scope(thread);
  Tuple mro(&scope, cached_type.mro());
  Runtime* runtime = thread->runtime();
  if (!isTypeInMro(*cached_type, *mro)) {
    return false;
  }
  for (word i = 0; i < mro.length(); ++i) {
    Type type(&scope, mro.at(i));
    Dict dict(&scope, type.dict());
    Object result(&scope, runtime->dictAt(thread, dict, attribute_name));
    if (type == updated_type) {
      // The current type in MRO is the searched type, and the searched
      // attribute is unfound in MRO so far, so type[attribute_name] is the one
      // retrieved from this mro.
      DCHECK(result.isValueCell(), "result must be ValueCell");
      return true;
    }
    if (result.isErrorNotFound()) {
      // No ValueCell found, implying that no dependencies in this type dict and
      // above.
      return false;
    }
    ValueCell value_cell(&scope, *result);
    if (!value_cell.isPlaceholder()) {
      // A non-placeholder is found for the attribute, this is retrived as the
      // value for the attribute, so no shadowing happens.
      return false;
    }
  }
  return false;
}

void icDeleteCacheForTypeAttrInDependent(Thread* thread,
                                         const Type& updated_type,
                                         const Str& attribute_name,
                                         bool data_descriptor,
                                         const Function& dependent) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Tuple names(&scope, Code::cast(dependent.code()).names());
  Tuple caches(&scope, dependent.caches());
  // Scan through all attribute caches and delete caches whose mro matches the
  // given type & attribute_name.
  MutableBytes bytecode(&scope, dependent.rewrittenBytecode());
  word bytecode_length = bytecode.length();
  for (word i = 0; i < bytecode_length;) {
    BytecodeOp op = nextBytecodeOp(bytecode, &i);
    if (op.bc != LOAD_ATTR_CACHED && op.bc != LOAD_METHOD_CACHED &&
        op.bc != STORE_ATTR_CACHED) {
      continue;
    }
    for (word j = op.arg * kIcPointersPerCache, end = j + kIcPointersPerCache;
         j < end; j += kIcPointersPerEntry) {
      Object cache_key(&scope, caches.at(j + kIcEntryKeyOffset));
      // Unfilled cache.
      if (cache_key.isNoneType()) {
        continue;
      }
      // Attribute name doesn't match.
      if (!attribute_name.equals(names.at(originalArg(*dependent, op.arg)))) {
        continue;
      }
      // A cache offset has a higher precedence than a non-data descriptor.
      if (caches.at(j + kIcEntryValueOffset).isSmallInt() && !data_descriptor) {
        continue;
      }
      Type cached_type(&scope, runtime->typeAt(static_cast<LayoutId>(
                                   SmallInt::cast(*cache_key).value())));
      // Adding an attribute to the updated type will not affect a lookup of the
      // name name through the cached type.
      if (!icIsCachedAttributeAffectedByUpdatedType(
              thread, cached_type, attribute_name, updated_type)) {
        continue;
      }

      caches.atPut(j + kIcEntryKeyOffset, NoneType::object());
      caches.atPut(j + kIcEntryValueOffset, NoneType::object());

      // Delete all direct/indirect dependencies from the deleted cache to
      // dependent since such dependencies are gone now.
      // TODO(T47281253): Call this per (cached_type, attribute_name) after
      // cache invalidation.
      icDoesCacheNeedInvalidationAfterUpdate(
          thread, cached_type, attribute_name, updated_type, dependent);
    }
  }
}

void icInvalidateCachesForTypeAttr(Thread* thread, const Type& type,
                                   const Str& attribute_name,
                                   bool data_descriptor) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Dict dict(&scope, type.dict());
  Object value(&scope, runtime->dictAt(thread, dict, attribute_name));
  if (value.isErrorNotFound()) {
    return;
  }
  DCHECK(value.isValueCell(), "value must be ValueCell");
  ValueCell type_attr_cell(&scope, *value);
  // Delete caches for attribute_name to be shadowed by the type[attribute_name]
  // change in all dependents that depend on the attribute being updated.
  Object link(&scope, type_attr_cell.dependencyLink());
  while (!link.isNoneType()) {
    Function dependent(&scope, WeakLink::cast(*link).referent());
    // Capturing the next node in case the current node is deleted by
    // icDeleteCacheForTypeAttrInDependent
    link = WeakLink::cast(*link).next();
    icDeleteCacheForTypeAttrInDependent(thread, type, attribute_name,
                                        data_descriptor, dependent);
  }

  // In case data_descriptor is true, we shouldn't see any dependents after
  // caching invalidation.
  DCHECK(!data_descriptor || type_attr_cell.dependencyLink().isNoneType(),
         "dependencyLink must be None if data_descriptor is true");
}

void icUpdateBinop(RawTuple caches, word index, LayoutId left_layout_id,
                   LayoutId right_layout_id, RawObject value,
                   IcBinopFlags flags) {
  word key_high_bits = static_cast<word>(left_layout_id)
                           << Header::kLayoutIdBits |
                       static_cast<word>(right_layout_id);
  RawObject entry_key = NoneType::object();
  for (word i = index * kIcPointersPerCache, end = i + kIcPointersPerCache;
       i < end; i += kIcPointersPerEntry) {
    entry_key = caches.at(i + kIcEntryKeyOffset);
    if (entry_key.isNoneType() ||
        SmallInt::cast(entry_key).value() >> kBitsPerByte == key_high_bits) {
      caches.atPut(i + kIcEntryKeyOffset,
                   SmallInt::fromWord(key_high_bits << kBitsPerByte |
                                      static_cast<word>(flags)));
      caches.atPut(i + kIcEntryValueOffset, value);
    }
  }
}

void icUpdateGlobalVar(Thread* thread, const Function& function, word index,
                       const ValueCell& value_cell) {
  HandleScope scope(thread);
  Tuple caches(&scope, function.caches());
  DCHECK(caches.at(index).isNoneType(),
         "cache entry must be empty one before update");
  icInsertDependentToValueCellDependencyLink(thread, function, value_cell);
  caches.atPut(index, *value_cell);

  // Update all global variable access to the cached value in the function.
  MutableBytes bytecode(&scope, function.rewrittenBytecode());
  word bytecode_length = bytecode.length();
  byte target_arg = static_cast<byte>(index);
  for (word i = 0; i < bytecode_length;) {
    BytecodeOp op = nextBytecodeOp(bytecode, &i);
    if (op.arg != target_arg) {
      continue;
    }
    if (op.bc == LOAD_GLOBAL) {
      bytecode.byteAtPut(i - 2, LOAD_GLOBAL_CACHED);
    } else if (op.bc == STORE_GLOBAL) {
      bytecode.byteAtPut(i - 2, STORE_GLOBAL_CACHED);
    }
  }
}

void icInvalidateGlobalVar(Thread* thread, const ValueCell& value_cell) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Tuple caches(&scope, runtime->emptyTuple());
  Object referent(&scope, NoneType::object());
  Object function(&scope, NoneType::object());
  Object link(&scope, value_cell.dependencyLink());
  MutableBytes bytecode(&scope, runtime->newMutableBytesUninitialized(0));
  while (!link.isNoneType()) {
    DCHECK(link.isWeakLink(), "ValuCell.dependenyLink must be a WeakLink");
    referent = WeakLink::cast(*link).referent();
    if (referent.isNoneType()) {
      // The function got deallocated.
      link = WeakLink::cast(*link).next();
      continue;
    }
    DCHECK(referent.isFunction(),
           "dependencyLink's payload must be a function");
    function = *referent;
    word names_length =
        Tuple::cast(Code::cast(Function::cast(*function).code()).names())
            .length();
    DCHECK(names_length < 256,
           "more than 256 global names may require bytecode stretching");

    // Empty the cache.
    caches = Function::cast(*function).caches();
    word name_index_found = -1;
    for (word i = 0; i < names_length; i++) {
      if (caches.at(i) == *value_cell) {
        caches.atPut(i, NoneType::object());
        name_index_found = i;
        break;
      }
    }
    DCHECK(name_index_found >= 0,
           "a dependent function must have cached the value");

    bytecode = Function::cast(*function).rewrittenBytecode();
    word bytecode_length = bytecode.length();
    for (word i = 0; i < bytecode_length;) {
      BytecodeOp op = nextBytecodeOp(bytecode, &i);
      Bytecode original_bc = op.bc;
      switch (op.bc) {
        case LOAD_GLOBAL_CACHED:
          original_bc = LOAD_GLOBAL;
          break;
        case STORE_GLOBAL_CACHED:
          original_bc = STORE_GLOBAL;
          break;
        default:
          break;
      }
      if (op.bc != original_bc && op.arg == name_index_found) {
        bytecode.byteAtPut(i - 2, original_bc);
      }
    }
    link = WeakLink::cast(*link).next();
  }
  value_cell.setDependencyLink(NoneType::object());
}

}  // namespace python
