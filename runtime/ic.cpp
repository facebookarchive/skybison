#include "ic.h"

#include "bytecode.h"
#include "runtime.h"
#include "type-builtins.h"
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
                                          const Str& name,
                                          const Object& dependent) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Tuple mro(&scope, type.mro());
  NoneType none(&scope, NoneType::object());
  for (word i = 0; i < mro.length(); i++) {
    Type mro_type(&scope, mro.at(i));
    if (mro_type.isSealed()) break;
    Dict dict(&scope, mro_type.dict());
    // TODO(T46428372): Consider using a specialized dict lookup to avoid 2
    // probings.
    Object result(&scope, runtime->dictAtByStr(thread, dict, name));
    DCHECK(result.isErrorNotFound() || result.isValueCell(),
           "value must be ValueCell if found");
    if (result.isErrorNotFound()) {
      result = runtime->dictAtPutInValueCellByStr(thread, dict, name, none);
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

void icDeleteDependentFromInheritingTypes(Thread* thread,
                                          LayoutId cached_layout_id,
                                          const Str& attr_name,
                                          const Type& new_defining_type,
                                          const Object& dependent) {
  DCHECK(icIsCachedAttributeAffectedByUpdatedType(thread, cached_layout_id,
                                                  attr_name, new_defining_type),
         "icIsCachedAttributeAffectedByUpdatedType must return true");
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Tuple mro(&scope, Type::cast(runtime->typeAt(cached_layout_id)).mro());
  for (word i = 0; i < mro.length(); ++i) {
    Type type(&scope, mro.at(i));
    Dict dict(&scope, type.dict());
    ValueCell value_cell(&scope, runtime->dictAtByStr(thread, dict, attr_name));
    icDeleteDependentInValueCell(thread, value_cell, dependent);
    if (type == new_defining_type) {
      // This can be a placeholder for some caching opcodes that depend on not
      // found attributes. For example, a >= b depends on type(b).__le__  even
      // when it is not found in case it's defined afterwards.
      return;
    }
    DCHECK(value_cell.isPlaceholder(),
           "value_cell below updated_type must be Placeholder");
  }
}

RawObject icHighestSuperTypeNotInMroOfOtherCachedTypes(
    Thread* thread, LayoutId cached_layout_id, const Str& attr_name,
    const Function& dependent) {
  HandleScope scope(thread);
  Object supertype_obj(&scope, NoneType::object());
  Runtime* runtime = thread->runtime();
  Tuple mro(&scope, Type::cast(runtime->typeAt(cached_layout_id)).mro());
  for (word i = 0; i < mro.length(); ++i) {
    Type type(&scope, mro.at(i));
    if (type.isSealed()) {
      break;
    }
    Dict type_dict(&scope, type.dict());
    if (!runtime->dictIncludesByStr(thread, type_dict, attr_name) ||
        icIsAttrCachedInDependent(thread, type, attr_name, dependent)) {
      break;
    }
    supertype_obj = *type;
  }
  if (supertype_obj.isNoneType()) {
    return Error::notFound();
  }
  return *supertype_obj;
}

bool icIsCachedAttributeAffectedByUpdatedType(Thread* thread,
                                              LayoutId cached_layout_id,
                                              const Str& attribute_name,
                                              const Type& updated_type) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Type cached_type(&scope, runtime->typeAt(cached_layout_id));
  if (!runtime->isSubclass(cached_type, updated_type)) {
    return false;
  }
  Tuple mro(&scope, cached_type.mro());
  for (word i = 0; i < mro.length(); ++i) {
    Type type(&scope, mro.at(i));
    Dict dict(&scope, type.dict());
    Object result(&scope, runtime->dictAtByStr(thread, dict, attribute_name));
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

bool icIsAttrCachedInDependent(Thread* thread, const Type& type,
                               const Str& attr_name,
                               const Function& dependent) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  for (IcIterator it(&scope, runtime, *dependent); it.hasNext(); it.next()) {
    if (it.isAttrCache()) {
      if (!it.isAttrNameEqualTo(attr_name)) {
        continue;
      }
      if (icIsCachedAttributeAffectedByUpdatedType(thread, it.layoutId(),
                                                   attr_name, type)) {
        return true;
      }
    } else {
      DCHECK(it.isBinopCache(),
             "a cache must be either for attributes or binops");
      if (attr_name.equals(it.leftMethodName()) &&
          icIsCachedAttributeAffectedByUpdatedType(thread, it.leftLayoutId(),
                                                   attr_name, type)) {
        return true;
      }
      if (attr_name.equals(it.rightMethodName()) &&
          icIsCachedAttributeAffectedByUpdatedType(thread, it.rightLayoutId(),
                                                   attr_name, type)) {
        return true;
      }
    }
  }
  return false;
}

void icEvictAttrCache(Thread* thread, const IcIterator& it,
                      const Type& updated_type, const Str& updated_attr,
                      AttributeKind attribute_kind, const Function& dependent) {
  DCHECK(it.isAttrCache(), "ic should point to an attribute cache");
  if (!it.isAttrNameEqualTo(updated_attr)) {
    return;
  }
  // We don't invalidate instance offset caches when non-data descriptor is
  // assigned to the cached type.
  if (it.isInstanceAttr() &&
      attribute_kind == AttributeKind::kNotADataDescriptor) {
    return;
  }
  // The updated type doesn't shadow the cached type.
  if (!icIsCachedAttributeAffectedByUpdatedType(thread, it.layoutId(),
                                                updated_attr, updated_type)) {
    return;
  }

  // Now that we know that the updated type attribute shadows the cached type
  // attribute, clear the cache.
  LayoutId cached_layout_id = it.layoutId();
  it.evict();

  // Delete all direct/indirect dependencies from the deleted cache to
  // dependent since such dependencies are gone now.
  // TODO(T54202245): Remove dependency links in parent classes of updated_type.
  icDeleteDependentFromInheritingTypes(thread, cached_layout_id, updated_attr,
                                       updated_type, dependent);
}

// TODO(T54277418): Pass SymbolId for updated_attr.
void icEvictBinopCache(Thread* thread, const IcIterator& it,
                       const Type& updated_type, const Str& updated_attr,
                       const Function& dependent) {
  if (it.leftMethodName() != updated_attr &&
      it.rightMethodName() != updated_attr) {
    // This cache cannot be affected since it references a different attribute
    // than the one we are looking for.
    return;
  }
  bool evict_lhs = false;
  if (it.leftMethodName() == updated_attr) {
    evict_lhs = icIsCachedAttributeAffectedByUpdatedType(
        thread, it.leftLayoutId(), updated_attr, updated_type);
  }
  bool evict_rhs = false;
  if (!evict_lhs && it.rightMethodName() == updated_attr) {
    evict_rhs = icIsCachedAttributeAffectedByUpdatedType(
        thread, it.rightLayoutId(), updated_attr, updated_type);
  }

  if (!evict_lhs && !evict_rhs) {
    // This cache does not reference attributes that are implemented by the
    // affected type.
    return;
  }

  LayoutId cached_layout_id = it.leftLayoutId();
  LayoutId other_cached_layout_id = it.rightLayoutId();
  HandleScope scope(thread);
  Str other_method_name(&scope, it.rightMethodName());
  if (evict_rhs) {
    // The RHS type is the one that is being affected.  This is either because
    // the RHS type is a supertype of the LHS type or because the LHS type did
    // not implement the binary operation.
    cached_layout_id = it.rightLayoutId();
    other_cached_layout_id = it.leftLayoutId();
    other_method_name = it.leftMethodName();
  }
  it.evict();

  // Remove this function from the dependency links in the dictionaries of
  // subtypes, starting at cached type, of the updated type that looked up the
  // attribute through the updated type.
  icDeleteDependentFromInheritingTypes(thread, cached_layout_id, updated_attr,
                                       updated_type, dependent);

  // TODO(T54202245): Remove dependency links in parent classes of update_type.

  // Walk up the MRO from the updated class looking for a super type that is not
  // referenced by another cache in this same function.
  Object supertype_obj(&scope, icHighestSuperTypeNotInMroOfOtherCachedTypes(
                                   thread, other_cached_layout_id,
                                   other_method_name, dependent));
  if (supertype_obj.isErrorNotFound()) {
    // typeAt(other_cached_layout_id).other_method_name_id is still cached so no
    // more dependencies need to be deleted.
    return;
  }
  Type supertype(&scope, *supertype_obj);
  // Remove this function from all of the dependency links in the dictionaries
  // of supertypes from the updated type up to the last supertype that is
  // exclusively referenced by the type in this cache (and no other caches in
  // this function.)
  icDeleteDependentFromInheritingTypes(thread, other_cached_layout_id,
                                       other_method_name, supertype, dependent);

  // TODO(T54202245): Remove depdency links in the parent classes of
  // other_cached_layout_id.
}

void icEvictCache(Thread* thread, const Function& dependent, const Type& type,
                  const Str& attr, AttributeKind attribute_kind) {
  HandleScope scope(thread);
  // Scan through all caches and delete caches shadowed by type.attr.
  for (IcIterator it(&scope, thread->runtime(), *dependent); it.hasNext();
       it.next()) {
    if (it.isAttrCache()) {
      icEvictAttrCache(thread, it, type, attr, attribute_kind, dependent);
    } else {
      DCHECK(it.isBinopCache(),
             "a cache must be either for attributes or binops");
      icEvictBinopCache(thread, it, type, attr, dependent);
    }
  }
}

void icInvalidateAttr(Thread* thread, const Type& type, const Str& attr_name,
                      const ValueCell& value) {
  HandleScope scope(thread);
  // Delete caches for attr_name to be shadowed by the type[attr_name]
  // change in all dependents that depend on the attribute being updated.
  Type value_type(&scope, thread->runtime()->typeOf(value.value()));
  AttributeKind attribute_kind = typeIsDataDescriptor(thread, value_type)
                                     ? AttributeKind::kDataDescriptor
                                     : AttributeKind::kNotADataDescriptor;
  Object link(&scope, value.dependencyLink());
  while (!link.isNoneType()) {
    Function dependent(&scope, WeakLink::cast(*link).referent());
    // Capturing the next node in case the current node is deleted by
    // icEvictCacheForTypeAttrInDependent
    link = WeakLink::cast(*link).next();
    icEvictCache(thread, dependent, type, attr_name, attribute_kind);
  }
  // In case is_data_descriptor is true, we shouldn't see any dependents after
  // caching invalidation.
  DCHECK(attribute_kind == AttributeKind::kNotADataDescriptor ||
             value.dependencyLink().isNoneType(),
         "dependencyLink must be None if is_data_descriptor is true");
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
      return;
    }
  }
}

void icUpdateGlobalVar(Thread* thread, const Function& function, word index,
                       const ValueCell& value_cell) {
  HandleScope scope(thread);
  Tuple caches(&scope, function.caches());
  // TODO(T46426927): Remove this once an invariant of updating cache only once
  // holds.
  if (!caches.at(index).isNoneType()) {
    // An attempt to update the same cache entry with the same value can happen
    // by LOAD_NAME and STORE_NAME which don't get modified to a cached opcode.
    DCHECK(caches.at(index) == *value_cell, "caches.at(index) == *value_cell");
    return;
  }
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
    // Empty the cache.
    caches = Function::cast(*function).caches();
    DCHECK_BOUND(names_length, caches.length());
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

RawObject IcIterator::leftMethodName() const {
  DCHECK(isBinopCache(), "should be only called for attribute caches");
  CompareOp compare_op =
      static_cast<CompareOp>(originalArg(*function_, bytecode_op_.arg));
  return runtime_->symbols()->at(runtime_->comparisonSelector(compare_op));
}

RawObject IcIterator::rightMethodName() const {
  DCHECK(isBinopCache(), "should be only called for attribute caches");
  CompareOp compare_op =
      static_cast<CompareOp>(originalArg(*function_, bytecode_op_.arg));
  return runtime_->symbols()->at(
      runtime_->swappedComparisonSelector(compare_op));
}

}  // namespace python
