#include "ic.h"

#include "attributedict.h"
#include "bytecode.h"
#include "dict-builtins.h"
#include "interpreter.h"
#include "runtime.h"
#include "str-builtins.h"
#include "type-builtins.h"
#include "utils.h"

namespace py {

ICState icCurrentState(RawTuple caches, word index) {
  word i = index * kIcPointersPerEntry;
  RawObject key = caches.at(i + kIcEntryKeyOffset);
  if (key.isNoneType()) {
    return ICState::kAnamorphic;
  }
  if (key.isSmallInt()) {
    return ICState::kMonomorphic;
  }
  DCHECK(key.isUnbound(),
         "unbound is the expected key for a polymorphic cache");
  return ICState::kPolymorphic;
}

// Perform the same lookup operation as typeLookupNameInMro as we're inserting
// dependent into the ValueCell in each visited type dictionary.
static void insertDependencyForTypeLookupInMro(Thread* thread,
                                               LayoutId layout_id,
                                               const Object& name,
                                               const Object& dependent) {
  HandleScope scope(thread);
  Type type(&scope, thread->runtime()->typeAt(layout_id));
  if (!type.hasMutableDict()) return;
  Tuple mro(&scope, type.mro());
  Type mro_type(&scope, *type);
  for (word i = 0; i < mro.length(); i++) {
    mro_type = mro.at(i);
    if (!mro_type.hasMutableDict()) return;
    ValueCell value_cell(&scope,
                         attributeValueCellAtPut(thread, mro_type, name));
    icInsertDependentToValueCellDependencyLink(thread, dependent, value_cell);
    if (!value_cell.isPlaceholder()) {
      // Attribute lookup terminates here. Therefore, no dependency tracking is
      // needed afterwards.
      return;
    }
  }
}

ICState icUpdateAttr(Thread* thread, const MutableTuple& caches, word index,
                     LayoutId layout_id, const Object& value,
                     const Object& name, const Function& dependent) {
  Runtime* runtime = thread->runtime();
  RawSmallInt key = SmallInt::fromWord(static_cast<word>(layout_id));
  word i = index * kIcPointersPerEntry;
  RawObject entry_key = caches.at(i + kIcEntryKeyOffset);
  if (entry_key.isNoneType() || entry_key == key) {
    caches.atPut(i + kIcEntryKeyOffset, key);
    caches.atPut(i + kIcEntryValueOffset, *value);
    insertDependencyForTypeLookupInMro(thread, layout_id, name, dependent);
    return ICState::kMonomorphic;
  }
  if (!caches.at(i + kIcEntryKeyOffset).isUnbound()) {
    // This is currently not a polymorphic cache.
    HandleScope scope(thread);
    MutableTuple polymorphic_cache(
        &scope, runtime->newMutableTuple(kIcPointersPerPolyCache));
    polymorphic_cache.atPut(kIcEntryKeyOffset,
                            caches.at(i + kIcEntryKeyOffset));
    polymorphic_cache.atPut(kIcEntryValueOffset,
                            caches.at(i + kIcEntryValueOffset));
    // Mark this entry as a polymorphic cache.
    caches.atPut(i + kIcEntryKeyOffset, Unbound::object());
    caches.atPut(i + kIcEntryValueOffset, *polymorphic_cache);
  }
  RawMutableTuple polymorphic_cache =
      MutableTuple::cast(caches.at(i + kIcEntryValueOffset));
  for (word j = 0; j < kIcPointersPerPolyCache; j += kIcPointersPerEntry) {
    entry_key = polymorphic_cache.at(j + kIcEntryKeyOffset);
    if (entry_key.isNoneType() || entry_key == key) {
      polymorphic_cache.atPut(j + kIcEntryKeyOffset, key);
      polymorphic_cache.atPut(j + kIcEntryValueOffset, *value);
      insertDependencyForTypeLookupInMro(thread, layout_id, name, dependent);
      break;
    }
  }
  return ICState::kPolymorphic;
}

bool icIsCacheEmpty(const MutableTuple& caches, word index) {
  word i = index * kIcPointersPerEntry;
  return caches.at(i + kIcEntryKeyOffset).isNoneType();
}

void icUpdateAttrModule(Thread* thread, const MutableTuple& caches, word index,
                        const Object& receiver, const ValueCell& value_cell,
                        const Function& dependent) {
  DCHECK(icIsCacheEmpty(caches, index), "cache must be empty\n");
  HandleScope scope(thread);
  word i = index * kIcPointersPerEntry;
  Module module(&scope, *receiver);
  caches.atPut(i + kIcEntryKeyOffset, SmallInt::fromWord(module.id()));
  caches.atPut(i + kIcEntryValueOffset, *value_cell);
  RawMutableBytes bytecode =
      RawMutableBytes::cast(dependent.rewrittenBytecode());
  word pc = thread->currentFrame()->virtualPC() - kCodeUnitSize;
  DCHECK(bytecode.byteAt(pc) == LOAD_ATTR_ANAMORPHIC,
         "current opcode must be LOAD_ATTR_ANAMORPHIC");
  bytecode.byteAtPut(pc, LOAD_ATTR_MODULE);
  icInsertDependentToValueCellDependencyLink(thread, dependent, value_cell);
}

void icUpdateAttrType(Thread* thread, const MutableTuple& caches, word index,
                      const Object& receiver, const Object& selector,
                      const Object& value, const Function& dependent) {
  DCHECK(icIsCacheEmpty(caches, index), "cache must be empty\n");
  word i = index * kIcPointersPerEntry;
  HandleScope scope(thread);
  Type type(&scope, *receiver);
  word id = static_cast<word>(type.instanceLayoutId());
  caches.atPut(i + kIcEntryKeyOffset, SmallInt::fromWord(id));
  caches.atPut(i + kIcEntryValueOffset, *value);
  RawMutableBytes bytecode =
      RawMutableBytes::cast(dependent.rewrittenBytecode());
  word pc = thread->currentFrame()->virtualPC() - kCodeUnitSize;
  DCHECK(bytecode.byteAt(pc) == LOAD_ATTR_ANAMORPHIC,
         "current opcode must be LOAD_ATTR_ANAMORPHIC");
  bytecode.byteAtPut(pc, LOAD_ATTR_TYPE);
  LayoutId layout_id = receiver.rawCast<RawType>().instanceLayoutId();
  insertDependencyForTypeLookupInMro(thread, layout_id, selector, dependent);
}

void icRemoveDeadWeakLinks(RawValueCell cell) {
  DCHECK(!cell.dependencyLink().isNoneType(),
         "unlink should not be called with an empty list");
  for (RawObject curr = cell.dependencyLink(); !curr.isNoneType();
       curr = WeakLink::cast(curr).next()) {
    if (!WeakLink::cast(curr).referent().isNoneType()) {
      continue;
    }
    RawObject prev = WeakLink::cast(curr).prev();
    RawObject next = WeakLink::cast(curr).next();
    if (prev.isNoneType()) {
      // Special case: unlinking the head.
      cell.setDependencyLink(next);
    } else {
      WeakLink::cast(prev).setNext(next);
    }
    if (!next.isNoneType()) {
      WeakLink::cast(next).setPrev(prev);
    }
  }
}

bool icInsertDependentToValueCellDependencyLink(Thread* thread,
                                                const Object& dependent,
                                                const ValueCell& value_cell) {
  RawObject empty_link = NoneType::object();
  bool has_dead_links = false;
  for (RawObject curr = value_cell.dependencyLink(); !curr.isNoneType();
       curr = WeakLink::cast(curr).next()) {
    RawObject referent = WeakLink::cast(curr).referent();
    if (referent == *dependent) {
      // The dependent is already in the list. Don't add it again.
      if (has_dead_links) icRemoveDeadWeakLinks(*value_cell);
      return false;
    }
    if (referent.isNoneType()) {
      if (empty_link.isNoneType()) {
        // Save the current WeakLink as a potential space for the new
        // dependent.
        empty_link = curr;
      } else {
        // We need to clean up the dead WeakLinks later.
        has_dead_links = true;
      }
    }
  }
  if (!empty_link.isNoneType()) {
    // We did not find dependent and we have a space for it, so fill the space
    WeakLink::cast(empty_link).setReferent(*dependent);
    if (has_dead_links) icRemoveDeadWeakLinks(*value_cell);
    return true;
  }
  // We did not find the dependent and we do not have space for it, so allocate
  // space and prepend it to the list.
  // Note that this implies that there were no dead WeakLinks.
  HandleScope scope(thread);
  Object old_head(&scope, value_cell.dependencyLink());
  Object none(&scope, NoneType::object());
  WeakLink new_head(&scope, thread->runtime()->newWeakLink(thread, dependent,
                                                           none, old_head));
  if (old_head.isWeakLink()) {
    WeakLink::cast(*old_head).setPrev(*new_head);
  }
  value_cell.setDependencyLink(*new_head);
  return true;
}

static void insertBinaryOpDependencies(Thread* thread,
                                       const Function& dependent,
                                       LayoutId left_layout_id,
                                       const SymbolId left_operator_id,
                                       LayoutId right_layout_id,
                                       const SymbolId right_operator_id) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object left_op_name(&scope, runtime->symbols()->at(left_operator_id));
  insertDependencyForTypeLookupInMro(thread, left_layout_id, left_op_name,
                                     dependent);
  Object right_op_name(&scope, runtime->symbols()->at(right_operator_id));
  insertDependencyForTypeLookupInMro(thread, right_layout_id, right_op_name,
                                     dependent);
}

void icInsertBinaryOpDependencies(Thread* thread, const Function& dependent,
                                  LayoutId left_layout_id,
                                  LayoutId right_layout_id,
                                  Interpreter::BinaryOp op) {
  SymbolId left_operator_id = Interpreter::binaryOperationSelector(op);
  SymbolId right_operator_id = Interpreter::swappedBinaryOperationSelector(op);
  insertBinaryOpDependencies(thread, dependent, left_layout_id,
                             left_operator_id, right_layout_id,
                             right_operator_id);
}

void icInsertCompareOpDependencies(Thread* thread, const Function& dependent,
                                   LayoutId left_layout_id,
                                   LayoutId right_layout_id, CompareOp op) {
  SymbolId left_operator_id = Interpreter::comparisonSelector(op);
  SymbolId right_operator_id = Interpreter::swappedComparisonSelector(op);
  insertBinaryOpDependencies(thread, dependent, left_layout_id,
                             left_operator_id, right_layout_id,
                             right_operator_id);
}

void icInsertInplaceOpDependencies(Thread* thread, const Function& dependent,
                                   LayoutId left_layout_id,
                                   LayoutId right_layout_id,
                                   Interpreter::BinaryOp op) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object inplace_op_name(
      &scope,
      runtime->symbols()->at(Interpreter::inplaceOperationSelector(op)));
  insertDependencyForTypeLookupInMro(thread, left_layout_id, inplace_op_name,
                                     dependent);
  SymbolId left_operator_id = Interpreter::binaryOperationSelector(op);
  SymbolId right_operator_id = Interpreter::swappedBinaryOperationSelector(op);
  insertBinaryOpDependencies(thread, dependent, left_layout_id,
                             left_operator_id, right_layout_id,
                             right_operator_id);
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
                                          const Object& attr_name,
                                          const Type& new_defining_type,
                                          const Object& dependent) {
  DCHECK(icIsCachedAttributeAffectedByUpdatedType(thread, cached_layout_id,
                                                  attr_name, new_defining_type),
         "icIsCachedAttributeAffectedByUpdatedType must return true");
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Type cached_type(&scope, runtime->typeAt(cached_layout_id));
  Tuple mro(&scope, cached_type.mro());
  Type mro_type(&scope, *cached_type);
  word hash = strHash(thread, *attr_name);
  for (word i = 0; i < mro.length(); ++i) {
    mro_type = mro.at(i);
    // If a mro_type's dict is not mutable, its parents must be not imutable.
    // Therefore, we can stop the MRO search here.
    if (!mro_type.hasMutableDict()) break;
    ValueCell value_cell(
        &scope, attributeValueCellAtWithHash(*mro_type, *attr_name, hash));
    icDeleteDependentInValueCell(thread, value_cell, dependent);
    if (mro_type == new_defining_type) {
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
    Thread* thread, LayoutId cached_layout_id, const Object& attr_name,
    const Function& dependent) {
  HandleScope scope(thread);
  Object supertype_obj(&scope, NoneType::object());
  Runtime* runtime = thread->runtime();
  Type type(&scope, runtime->typeAt(cached_layout_id));
  Tuple mro(&scope, type.mro());
  word hash = strHash(thread, *attr_name);
  Type mro_type(&scope, *type);
  for (word i = 0; i < mro.length(); ++i) {
    mro_type = mro.at(i);
    if (!mro_type.hasMutableDict()) {
      break;
    }
    if (attributeValueCellAtWithHash(*mro_type, *attr_name, hash)
            .isErrorNotFound() ||
        icIsAttrCachedInDependent(thread, mro_type, attr_name, dependent)) {
      break;
    }
    supertype_obj = *mro_type;
  }
  if (supertype_obj.isNoneType()) {
    return Error::notFound();
  }
  return *supertype_obj;
}

bool icIsCachedAttributeAffectedByUpdatedType(Thread* thread,
                                              LayoutId cached_layout_id,
                                              const Object& attribute_name,
                                              const Type& updated_type) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Type cached_type(&scope, runtime->typeAt(cached_layout_id));
  if (!typeIsSubclass(*cached_type, *updated_type)) {
    return false;
  }
  Tuple mro(&scope, cached_type.mro());
  Type mro_type(&scope, *cached_type);
  Object result(&scope, NoneType::object());
  word hash = strHash(thread, *attribute_name);
  for (word i = 0; i < mro.length(); ++i) {
    mro_type = mro.at(i);
    // If a mro_type's dict is not mutable, its parents must be not immutable.
    // Therefore, we can stop the MRO search here.
    if (!mro_type.hasMutableDict()) break;
    result = attributeValueCellAtWithHash(*mro_type, *attribute_name, hash);
    if (mro_type == updated_type) {
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
    if (!ValueCell::cast(*result).isPlaceholder()) {
      // A non-placeholder is found for the attribute, this is retrived as the
      // value for the attribute, so no shadowing happens.
      return false;
    }
  }
  return false;
}

bool icIsAttrCachedInDependent(Thread* thread, const Type& type,
                               const Object& attr_name,
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
      DCHECK(it.isBinaryOpCache() || it.isInplaceOpCache(),
             "a cache must be for binops or inplace-ops");
      if (attr_name == it.leftMethodName() &&
          icIsCachedAttributeAffectedByUpdatedType(thread, it.leftLayoutId(),
                                                   attr_name, type)) {
        return true;
      }
      if (attr_name == it.rightMethodName() &&
          icIsCachedAttributeAffectedByUpdatedType(thread, it.rightLayoutId(),
                                                   attr_name, type)) {
        return true;
      }
      if (it.isInplaceOpCache() && attr_name == it.inplaceMethodName() &&
          icIsCachedAttributeAffectedByUpdatedType(thread, it.leftLayoutId(),
                                                   attr_name, type)) {
        return true;
      }
    }
  }
  return false;
}

void icEvictAttr(Thread* thread, const IcIterator& it, const Type& updated_type,
                 const Object& updated_attr, AttributeKind attribute_kind,
                 const Function& dependent) {
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

void icDeleteDependentToDefiningType(Thread* thread, const Function& dependent,
                                     LayoutId cached_layout_id,
                                     const Object& attr_name) {
  HandleScope scope(thread);
  // Walk up the MRO from the updated class looking for a super type that is not
  // referenced by another cache in this same function.
  Object supertype_obj(&scope,
                       icHighestSuperTypeNotInMroOfOtherCachedTypes(
                           thread, cached_layout_id, attr_name, dependent));
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
  icDeleteDependentFromInheritingTypes(thread, cached_layout_id, attr_name,
                                       supertype, dependent);

  // TODO(T54202245): Remove depdency links in the parent classes of
  // other_cached_layout_id.
}

// TODO(T54277418): Pass SymbolId for updated_attr.
void icEvictBinaryOp(Thread* thread, const IcIterator& it,
                     const Type& updated_type, const Object& updated_attr,
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
  Object other_method_name(&scope, it.rightMethodName());
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

  icDeleteDependentToDefiningType(thread, dependent, other_cached_layout_id,
                                  other_method_name);
}

void icEvictInplaceOp(Thread* thread, const IcIterator& it,
                      const Type& updated_type, const Object& updated_attr,
                      const Function& dependent) {
  if (it.inplaceMethodName() != updated_attr &&
      it.leftMethodName() != updated_attr &&
      it.rightMethodName() != updated_attr) {
    // This cache cannot be affected since it references a different attribute
    // than the one we are looking for.
    return;
  }
  bool evict_inplace = false;
  if (it.inplaceMethodName() == updated_attr) {
    evict_inplace = icIsCachedAttributeAffectedByUpdatedType(
        thread, it.leftLayoutId(), updated_attr, updated_type);
  }
  bool evict_lhs = false;
  if (!evict_inplace && it.leftMethodName() == updated_attr) {
    evict_lhs = icIsCachedAttributeAffectedByUpdatedType(
        thread, it.leftLayoutId(), updated_attr, updated_type);
  }
  bool evict_rhs = false;
  if (!evict_inplace && !evict_lhs && it.rightMethodName() == updated_attr) {
    evict_rhs = icIsCachedAttributeAffectedByUpdatedType(
        thread, it.rightLayoutId(), updated_attr, updated_type);
  }

  if (!evict_inplace && !evict_lhs && !evict_rhs) {
    // This cache does not reference attributes that are implemented by the
    // affected type.
    return;
  }

  LayoutId left_layout_id = it.leftLayoutId();
  LayoutId right_layout_id = it.rightLayoutId();
  HandleScope scope(thread);
  Object inplace_method_name(&scope, it.inplaceMethodName());
  Object left_method_name(&scope, it.leftMethodName());
  Object right_method_name(&scope, it.rightMethodName());
  it.evict();

  // Remove this function from the dependency links in the dictionaries of
  // subtypes of the directly affected type.
  // There are two other types that this function may not depend on anymore due
  // to this eviction. We remove this function from the dependency links of
  // these types up to their defining types to get rid of the dependencies being
  // tracked for the evicted cache.
  // TODO(T54202245): Remove dependency links in parent classes of the directly
  // affected type.
  if (evict_inplace) {
    icDeleteDependentFromInheritingTypes(
        thread, left_layout_id, inplace_method_name, updated_type, dependent);
    icDeleteDependentToDefiningType(thread, dependent, left_layout_id,
                                    left_method_name);
    icDeleteDependentToDefiningType(thread, dependent, right_layout_id,
                                    right_method_name);
    return;
  }
  if (evict_lhs) {
    icDeleteDependentFromInheritingTypes(
        thread, left_layout_id, left_method_name, updated_type, dependent);
    icDeleteDependentToDefiningType(thread, dependent, left_layout_id,
                                    inplace_method_name);
    icDeleteDependentToDefiningType(thread, dependent, right_layout_id,
                                    right_method_name);
    return;
  }
  DCHECK(evict_rhs, "evict_rhs must be true");
  icDeleteDependentFromInheritingTypes(
      thread, right_layout_id, right_method_name, updated_type, dependent);
  icDeleteDependentToDefiningType(thread, dependent, left_layout_id,
                                  inplace_method_name);
  icDeleteDependentToDefiningType(thread, dependent, left_layout_id,
                                  left_method_name);
}

void icEvictCache(Thread* thread, const Function& dependent, const Type& type,
                  const Object& attr_name, AttributeKind attribute_kind) {
  HandleScope scope(thread);
  // Scan through all caches and delete caches shadowed by type.attr_name.
  // TODO(T54277418): Filter out attr that cannot be converted to SymbolId.
  for (IcIterator it(&scope, thread->runtime(), *dependent); it.hasNext();
       it.next()) {
    if (it.isAttrCache()) {
      icEvictAttr(thread, it, type, attr_name, attribute_kind, dependent);
    } else if (it.isBinaryOpCache()) {
      icEvictBinaryOp(thread, it, type, attr_name, dependent);
    } else if (it.isInplaceOpCache()) {
      icEvictInplaceOp(thread, it, type, attr_name, dependent);
    } else {
      CHECK(it.isModuleAttrCache(),
            "a cache must be for attributes, binops, or inplace-ops");
    }
  }
}

void icInvalidateAttr(Thread* thread, const Type& type, const Object& attr_name,
                      const ValueCell& value_cell) {
  HandleScope scope(thread);
  // Delete caches for attr_name to be shadowed by the type[attr_name]
  // change in all dependents that depend on the attribute being updated.
  Type value_type(&scope, thread->runtime()->typeOf(value_cell.value()));
  AttributeKind attribute_kind = typeIsDataDescriptor(*value_type)
                                     ? AttributeKind::kDataDescriptor
                                     : AttributeKind::kNotADataDescriptor;
  Object link(&scope, value_cell.dependencyLink());
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
             value_cell.dependencyLink().isNoneType(),
         "dependencyLink must be None if is_data_descriptor is true");
}

RawSmallInt encodeBinaryOpKey(LayoutId left_layout_id, LayoutId right_layout_id,
                              BinaryOpFlags flags) {
  word key_high_bits = static_cast<word>(left_layout_id)
                           << Header::kLayoutIdBits |
                       static_cast<word>(right_layout_id);
  return SmallInt::fromWord(key_high_bits << kBitsPerByte |
                            static_cast<word>(flags));
}

ICState icUpdateBinOp(Thread* thread, const MutableTuple& caches, word index,
                      LayoutId left_layout_id, LayoutId right_layout_id,
                      const Object& value, BinaryOpFlags flags) {
  static_assert(Header::kLayoutIdBits * 2 + kBitsPerByte <= SmallInt::kBits,
                "Two layout ids and flags overflow a SmallInt");
  word key_high_bits = static_cast<word>(left_layout_id)
                           << Header::kLayoutIdBits |
                       static_cast<word>(right_layout_id);
  word i = index * kIcPointersPerEntry;
  RawObject new_key = encodeBinaryOpKey(left_layout_id, right_layout_id, flags);
  RawObject entry_key = caches.at(i + kIcEntryKeyOffset);
  if (entry_key.isNoneType() ||
      (entry_key.isSmallInt() &&
       SmallInt::cast(entry_key).value() >> kBitsPerByte == key_high_bits)) {
    caches.atPut(i + kIcEntryKeyOffset, new_key);
    caches.atPut(i + kIcEntryValueOffset, *value);
    return ICState::kMonomorphic;
  }

  if (!caches.at(i + kIcEntryKeyOffset).isUnbound()) {
    // Upgrade this cache to a polymorphic cache.
    HandleScope scope(thread);
    MutableTuple polymorphic_cache(
        &scope, thread->runtime()->newMutableTuple(kIcPointersPerPolyCache));
    polymorphic_cache.atPut(kIcEntryKeyOffset,
                            caches.at(i + kIcEntryKeyOffset));
    polymorphic_cache.atPut(kIcEntryValueOffset,
                            caches.at(i + kIcEntryValueOffset));
    // Mark this entry as a polymorphic cache.
    caches.atPut(i + kIcEntryKeyOffset, Unbound::object());
    caches.atPut(i + kIcEntryValueOffset, *polymorphic_cache);
  }
  RawMutableTuple polymorphic_cache =
      MutableTuple::cast(caches.at(i + kIcEntryValueOffset));
  for (word j = 0; j < kIcPointersPerPolyCache; j += kIcPointersPerEntry) {
    entry_key = polymorphic_cache.at(j + kIcEntryKeyOffset);
    if (entry_key.isNoneType() ||
        SmallInt::cast(entry_key).value() >> kBitsPerByte == key_high_bits) {
      polymorphic_cache.atPut(j + kIcEntryKeyOffset, new_key);
      polymorphic_cache.atPut(j + kIcEntryValueOffset, *value);
      break;
    }
  }
  return ICState::kPolymorphic;
}

void icUpdateGlobalVar(Thread* thread, const Function& function, word index,
                       const ValueCell& value_cell) {
  HandleScope scope(thread);
  MutableTuple caches(&scope, function.caches());
  // TODO(T46426927): Remove this once an invariant of updating cache only once
  // holds.
  if (!caches.at(index).isNoneType()) {
    // An attempt to update the same cache entry with the same value can happen
    // by LOAD_NAME and STORE_NAME which don't get modified to a cached opcode.
    DCHECK(caches.at(index) == *value_cell, "caches.at(index) == *value_cell");
    return;
  }
  // Insert function as the first dependent on value_cell.
  Object old_head(&scope, value_cell.dependencyLink());
  Object none(&scope, NoneType::object());
  WeakLink new_head(
      &scope, thread->runtime()->newWeakLink(thread, function, none, old_head));
  if (old_head.isWeakLink()) {
    WeakLink::cast(*old_head).setPrev(*new_head);
  }
  value_cell.setDependencyLink(*new_head);

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
    MutableTuple caches(&scope, Function::cast(*function).caches());
    DCHECK_BOUND(names_length, caches.length());
    word name_index_found = -1;
    for (word i = 0; i < names_length; i++) {
      if (caches.at(i) == *value_cell) {
        caches.atPut(i, NoneType::object());
        name_index_found = i;
        break;
      }
    }
    bytecode = Function::cast(*function).rewrittenBytecode();
    word bytecode_length = bytecode.length();
    for (word i = 0; i < bytecode_length;) {
      BytecodeOp op = nextBytecodeOp(bytecode, &i);
      Bytecode original_bc = op.bc;
      switch (op.bc) {
        case LOAD_ATTR_MODULE: {
          original_bc = LOAD_ATTR_ANAMORPHIC;
          word index = op.arg * kIcPointersPerEntry;
          if (caches.at(index + kIcEntryValueOffset) == *value_cell) {
            caches.atPut(index + kIcEntryKeyOffset, NoneType::object());
            caches.atPut(index + kIcEntryValueOffset, NoneType::object());
          }
          break;
        }
        case LOAD_GLOBAL_CACHED:
          original_bc = LOAD_GLOBAL;
          if (op.bc != original_bc && op.arg == name_index_found) {
            bytecode.byteAtPut(i - 2, original_bc);
          }
          break;
        case STORE_GLOBAL_CACHED:
          original_bc = STORE_GLOBAL;
          if (op.bc != original_bc && op.arg == name_index_found) {
            bytecode.byteAtPut(i - 2, original_bc);
          }
          break;
        default:
          break;
      }
    }
    link = WeakLink::cast(*link).next();
  }
  value_cell.setDependencyLink(NoneType::object());
}

bool IcIterator::isAttrNameEqualTo(const Object& attr_name) const {
  DCHECK(isAttrCache(), "should be only called for attribute caches");
  switch (bytecode_op_.bc) {
    case FOR_ITER_MONOMORPHIC:
    case FOR_ITER_POLYMORPHIC:
    case FOR_ITER_ANAMORPHIC:
      return attr_name == runtime_->symbols()->at(ID(__next__));
    case BINARY_SUBSCR_ANAMORPHIC:
    case BINARY_SUBSCR_MONOMORPHIC:
    case BINARY_SUBSCR_POLYMORPHIC:
      return attr_name == runtime_->symbols()->at(ID(__getitem__));
    case STORE_SUBSCR_ANAMORPHIC:
      return attr_name == runtime_->symbols()->at(ID(__setitem__));
    default:
      return attr_name == names_.at(originalArg(*function_, bytecode_op_.arg));
  }
}

static bool isBinaryOpOrInplaceOp(Bytecode bc) {
  switch (bc) {
    case BINARY_OP_MONOMORPHIC:
    case BINARY_OP_POLYMORPHIC:
    case BINARY_OP_ANAMORPHIC:
    case INPLACE_OP_MONOMORPHIC:
    case INPLACE_OP_POLYMORPHIC:
    case INPLACE_OP_ANAMORPHIC:
      return true;
    default:
      return false;
  }
}

RawObject IcIterator::leftMethodName() const {
  DCHECK(isBinaryOpCache() || isInplaceOpCache(),
         "should be only called for binop or inplace-ops");
  int32_t arg = originalArg(*function_, bytecode_op_.arg);
  SymbolId method;
  if (isBinaryOpOrInplaceOp(bytecode_op_.bc)) {
    Interpreter::BinaryOp binary_op = static_cast<Interpreter::BinaryOp>(arg);
    method = Interpreter::binaryOperationSelector(binary_op);
  } else {
    DCHECK(bytecode_op_.bc == COMPARE_OP_MONOMORPHIC ||
               bytecode_op_.bc == COMPARE_OP_POLYMORPHIC ||
               bytecode_op_.bc == COMPARE_OP_ANAMORPHIC,
           "binop cache must be for BINARY_OP_*, INPLACE_OP_*, or "
           "COMPARE_OP_*");
    CompareOp compare_op = static_cast<CompareOp>(arg);
    method = Interpreter::comparisonSelector(compare_op);
  }
  return runtime_->symbols()->at(method);
}

RawObject IcIterator::rightMethodName() const {
  DCHECK(isBinaryOpCache() || isInplaceOpCache(),
         "should be only called for binop or inplace-ops");
  int32_t arg = originalArg(*function_, bytecode_op_.arg);
  SymbolId method;
  if (isBinaryOpOrInplaceOp(bytecode_op_.bc)) {
    Interpreter::BinaryOp binary_op = static_cast<Interpreter::BinaryOp>(arg);
    method = Interpreter::swappedBinaryOperationSelector(binary_op);
  } else {
    DCHECK(bytecode_op_.bc == COMPARE_OP_MONOMORPHIC ||
               bytecode_op_.bc == COMPARE_OP_POLYMORPHIC ||
               bytecode_op_.bc == COMPARE_OP_ANAMORPHIC,
           "binop cache must be for BINARY_OP_*, INPLACE_OP_*, or "
           "COMPARE_OP_*");
    CompareOp compare_op = static_cast<CompareOp>(arg);
    method = Interpreter::swappedComparisonSelector(compare_op);
  }
  return runtime_->symbols()->at(method);
}

RawObject IcIterator::inplaceMethodName() const {
  DCHECK(bytecode_op_.bc == INPLACE_OP_MONOMORPHIC ||
             bytecode_op_.bc == INPLACE_OP_POLYMORPHIC ||
             bytecode_op_.bc == INPLACE_OP_ANAMORPHIC,
         "should only be called for INPLACE_OP_*");
  int32_t arg = originalArg(*function_, bytecode_op_.arg);
  Interpreter::BinaryOp binary_op = static_cast<Interpreter::BinaryOp>(arg);
  SymbolId method = Interpreter::inplaceOperationSelector(binary_op);
  return runtime_->symbols()->at(method);
}

}  // namespace py
