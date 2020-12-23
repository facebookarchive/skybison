#include "type-builtins.h"

#include "cpython-data.h"

#include "attributedict.h"
#include "builtins.h"
#include "bytecode.h"
#include "capi.h"
#include "dict-builtins.h"
#include "frame.h"
#include "globals.h"
#include "ic.h"
#include "list-builtins.h"
#include "module-builtins.h"
#include "mro.h"
#include "object-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "str-builtins.h"
#include "thread.h"
#include "typeslots.h"

namespace py {

static RawObject addBuiltinTypeWithLayout(Thread* thread, const Layout& layout,
                                          SymbolId name, LayoutId builtin_base,
                                          LayoutId superclass_id, word flags) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Type type(&scope, runtime->newType());
  type.setName(runtime->symbols()->at(name));
  Type superclass(&scope, runtime->typeAt(superclass_id));
  type.setInstanceLayout(*layout);
  type.setInstanceLayoutId(layout.id());
  flags |= superclass.flags() & Type::kInheritableFlags;
  type.setFlagsAndBuiltinBase(static_cast<Type::Flag>(flags), builtin_base);
  type.setBases(runtime->newTupleWith1(superclass));
  layout.setDescribedType(*type);
  return *type;
}

RawObject addBuiltinType(Thread* thread, SymbolId name, LayoutId layout_id,
                         LayoutId superclass_id, View<BuiltinAttribute> attrs,
                         word size, bool basetype) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Layout layout(&scope, runtime->layoutCreateSubclassWithBuiltins(
                            thread, layout_id, superclass_id, attrs, size));
  runtime->layoutAtPut(layout_id, *layout);
  LayoutId builtin_base = attrs.length() == 0 ? superclass_id : layout_id;
  word flags = basetype ? Type::Flag::kIsBasetype : Type::Flag::kNone;
  return addBuiltinTypeWithLayout(thread, layout, name, builtin_base,
                                  superclass_id, flags);
}

RawObject addImmediateBuiltinType(Thread* thread, SymbolId name,
                                  LayoutId layout_id, LayoutId builtin_base,
                                  LayoutId superclass_id, bool basetype) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Layout layout(&scope, runtime->newLayout(layout_id));
  runtime->layoutAtPut(layout_id, *layout);
  word flags = basetype ? Type::Flag::kIsBasetype : Type::Flag::kNone;
  return addBuiltinTypeWithLayout(thread, layout, name, builtin_base,
                                  superclass_id, flags);
}

RawObject findBuiltinTypeWithName(Thread* thread, const Object& name) {
  DCHECK(Runtime::isInternedStr(thread, name), "must be interned str");
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object layout(&scope, NoneType::object());
  Object type_obj(&scope, NoneType::object());
  for (int i = 0; i <= static_cast<int>(LayoutId::kLastBuiltinId); i++) {
    layout = runtime->layoutAtSafe(static_cast<LayoutId>(i));
    if (layout.isErrorNotFound()) continue;
    type_obj = Layout::cast(*layout).describedType();
    if (!type_obj.isType()) continue;
    if (Type::cast(*type_obj).name() == name) {
      return *type_obj;
    }
  }
  return Error::notFound();
}

RawObject raiseTypeErrorCannotSetImmutable(Thread* thread, const Type& type) {
  HandleScope scope(thread);
  Object type_name(&scope, type.name());
  return thread->raiseWithFmt(
      LayoutId::kTypeError,
      "can't set attributes of built-in/extension type '%S'", &type_name);
}

RawObject typeAt(const Type& type, const Object& name) {
  word hash = internedStrHash(*name);
  return attributeAtWithHash(*type, *name, hash);
}

RawObject typeAtById(Thread* thread, const Type& type, SymbolId id) {
  RawObject name = thread->runtime()->symbols()->at(id);
  return attributeAt(*type, name);
}

// Returns type flags updated with the given attribute.
static word computeAttributeTypeFlags(Thread* thread, const Type& type,
                                      SymbolId name, word flags) {
  Runtime* runtime = thread->runtime();
  // Custom MROs can contain types that are not part of the subclass hierarchy
  // which does not fit our cache invalidation strategy for the flags. Be safe
  // and do not set any!
  if (flags & Type::Flag::kHasCustomMro) {
    return flags & ~(Type::Flag::kHasObjectDunderGetattribute |
                     Type::Flag::kHasTypeDunderGetattribute |
                     Type::Flag::kHasModuleDunderGetattribute |
                     Type::Flag::kHasObjectDunderNew);
  }
  if (name == ID(__getattribute__)) {
    RawObject value = typeLookupInMroById(thread, *type, name);
    if (value == runtime->objectDunderGetattribute()) {
      flags |= Type::Flag::kHasObjectDunderGetattribute;
    } else {
      flags &= ~Type::Flag::kHasObjectDunderGetattribute;
    }
    if (value == runtime->typeDunderGetattribute()) {
      flags |= Type::Flag::kHasTypeDunderGetattribute;
    } else {
      flags &= ~Type::Flag::kHasTypeDunderGetattribute;
    }
    if (value == runtime->moduleDunderGetattribute()) {
      flags |= Type::Flag::kHasModuleDunderGetattribute;
    } else {
      flags &= ~Type::Flag::kHasModuleDunderGetattribute;
    }
    return flags;
  }
  if (name == ID(__new__)) {
    RawObject value = typeLookupInMroById(thread, *type, name);
    if (value == runtime->objectDunderNew()) {
      flags |= Type::Flag::kHasObjectDunderNew;
    } else {
      flags &= ~Type::Flag::kHasObjectDunderNew;
    }
    return flags;
  }
  return flags;
}

// Propagate attribute type flags through `type`'s descendents.
static void typePropagateAttributeTypeFlag(Thread* thread, const Type& type,
                                           SymbolId attr_name) {
  Type::Flag new_flags = Type::Flag::kNone;
  new_flags = static_cast<Type::Flag>(
      computeAttributeTypeFlags(thread, type, attr_name, type.flags()));
  if (new_flags == type.flags()) {
    // Stop tree traversal since flags doesn't change for this type and its
    // subclases.
    return;
  }
  type.setFlags(new_flags);
  if (type.subclasses().isNoneType()) {
    return;
  }
  HandleScope scope(thread);
  List subclasses(&scope, type.subclasses());
  Type subclass(&scope, thread->runtime()->typeAt(LayoutId::kObject));
  for (word i = 0, length = subclasses.numItems(); i < length; ++i) {
    RawObject referent = WeakRef::cast(subclasses.at(i)).referent();
    if (referent.isNoneType()) continue;
    subclass = referent.rawCast<RawType>();
    if (typeAtById(thread, subclass, attr_name).isErrorNotFound()) {
      // subclass inherits the attribute for `attr_name`.
      typePropagateAttributeTypeFlag(thread, subclass, attr_name);
    }
  }
}

static const SymbolId kAttributesForTypeFlags[] = {
    ID(__getattribute__),
    ID(__new__),
};

// Returns `SymbolId` for `attr_name` if given `attr_name` is marked in
// Type::Flags. Returns `SymbolId::kInvalid` otherwise. See Type::Flags.
static SymbolId attributeForTypeFlag(Thread* thread, const Object& attr_name) {
  Symbols* symbols = thread->runtime()->symbols();
  for (SymbolId id : kAttributesForTypeFlags) {
    if (attr_name == symbols->at(id)) return id;
  }
  return SymbolId::kInvalid;
}

RawObject typeAtPut(Thread* thread, const Type& type, const Object& name,
                    const Object& value) {
  DCHECK(thread->runtime()->isInternedStr(thread, name),
         "name should be an interned str");
  RawValueCell value_cell =
      ValueCell::cast(attributeValueCellAtPut(thread, type, name));
  value_cell.setValue(*value);
  if (!value_cell.dependencyLink().isNoneType()) {
    HandleScope scope(thread);
    ValueCell value_cell_obj(&scope, value_cell);
    icInvalidateAttr(thread, type, name, value_cell_obj);
    return *value_cell_obj;
  }
  SymbolId attr_name_for_type_flag = attributeForTypeFlag(thread, name);
  if (attributeForTypeFlag(thread, name) != SymbolId::kInvalid) {
    typePropagateAttributeTypeFlag(thread, type, attr_name_for_type_flag);
  }
  return value_cell;
}

RawObject typeAtPutById(Thread* thread, const Type& type, SymbolId id,
                        const Object& value) {
  HandleScope scope(thread);
  Object name(&scope, thread->runtime()->symbols()->at(id));
  return typeAtPut(thread, type, name, value);
}

RawObject typeAtSetLocation(RawType type, RawObject name, word hash,
                            Object* location) {
  RawObject result = attributeValueCellAtWithHash(type, name, hash);
  if (result.isErrorNotFound()) return result;
  if (ValueCell::cast(result).isPlaceholder()) return Error::notFound();
  if (location != nullptr) {
    *location = result;
  }
  return ValueCell::cast(result).value();
}

bool typeIsDataDescriptor(Thread* thread, RawType type) {
  if (type.isBuiltin()) {
    LayoutId layout_id = type.instanceLayoutId();
    return layout_id == LayoutId::kProperty ||
           layout_id == LayoutId::kSlotDescriptor;
  }
  // TODO(T25692962): Track "descriptorness" through a bit on the class
  return !typeLookupInMroById(thread, type, ID(__set__)).isError();
}

bool typeIsNonDataDescriptor(Thread* thread, RawType type) {
  if (type.isBuiltin()) {
    switch (type.instanceLayoutId()) {
      case LayoutId::kClassMethod:
      case LayoutId::kFunction:
      case LayoutId::kProperty:
      case LayoutId::kStaticMethod:
        return true;
      default:
        return false;
    }
  }
  // TODO(T25692962): Track "descriptorness" through a bit on the class
  return !typeLookupInMroById(thread, type, ID(__get__)).isError();
}

RawObject resolveDescriptorGet(Thread* thread, const Object& descr,
                               const Object& instance,
                               const Object& instance_type) {
  if (!typeIsNonDataDescriptor(
          thread, thread->runtime()->typeOf(*descr).rawCast<RawType>())) {
    return *descr;
  }
  return Interpreter::callDescriptorGet(thread, descr, instance, instance_type);
}

RawObject typeAssignFromDict(Thread* thread, const Type& type,
                             const Dict& dict) {
  HandleScope scope(thread);
  Object key(&scope, NoneType::object());
  Object value(&scope, NoneType::object());
  for (word i = 0; dictNextItem(dict, &i, &key, &value);) {
    DCHECK(!(value.isValueCell() && ValueCell::cast(*value).isPlaceholder()),
           "value should not be a placeholder value cell");
    key = attributeName(thread, key);
    if (key.isErrorException()) return *key;
    typeAtPut(thread, type, key, value);
  }
  return NoneType::object();
}

RawObject typeLookupInMroSetLocation(Thread* thread, RawType type,
                                     RawObject name, Object* location) {
  RawTuple mro = Tuple::cast(type.mro());
  word hash = internedStrHash(name);
  for (word i = 0, length = mro.length(); i < length; i++) {
    DCHECK(thread->runtime()->isInstanceOfType(mro.at(i)), "non-type in MRO");
    RawType mro_type = mro.at(i).rawCast<RawType>();
    RawObject result = typeAtSetLocation(mro_type, name, hash, location);
    if (!result.isErrorNotFound()) {
      return result;
    }
  }
  return Error::notFound();
}

RawObject typeLookupInMro(Thread* thread, RawType type, RawObject name) {
  RawTuple mro = Tuple::cast(type.mro());
  word hash = internedStrHash(name);
  for (word i = 0, length = mro.length(); i < length; i++) {
    DCHECK(thread->runtime()->isInstanceOfType(mro.at(i)), "non-type in MRO");
    RawType mro_type = mro.at(i).rawCast<RawType>();
    RawObject result = attributeAtWithHash(mro_type, name, hash);
    if (!result.isErrorNotFound()) {
      return result;
    }
  }
  return Error::notFound();
}

RawObject typeLookupInMroById(Thread* thread, RawType type, SymbolId id) {
  return typeLookupInMro(thread, type, thread->runtime()->symbols()->at(id));
}

RawObject typeRemove(Thread* thread, const Type& type, const Object& name) {
  DCHECK(Runtime::isInternedStr(thread, name), "expected interned str");
  HandleScope scope(thread);
  word index;
  Object value_cell_obj(&scope, NoneType::object());
  if (!attributeFindForRemoval(type, name, &value_cell_obj, &index)) {
    return Error::notFound();
  }
  ValueCell value_cell(&scope, *value_cell_obj);
  icInvalidateAttr(thread, type, name, value_cell);
  attributeRemove(type, index);
  if (value_cell.isPlaceholder()) return Error::notFound();
  SymbolId attr_name_for_type_flag = attributeForTypeFlag(thread, name);
  if (attributeForTypeFlag(thread, name) != SymbolId::kInvalid) {
    typePropagateAttributeTypeFlag(thread, type, attr_name_for_type_flag);
  }
  return value_cell.value();
}

RawObject typeRemoveById(Thread* thread, const Type& type, SymbolId id) {
  HandleScope scope(thread);
  Object name(&scope, thread->runtime()->symbols()->at(id));
  return typeRemove(thread, type, name);
}

RawObject typeKeys(Thread* thread, const Type& type) {
  return attributeKeys(thread, type);
}

word typeLen(Thread* thread, const Type& type) {
  return attributeLen(thread, type);
}

RawObject typeValues(Thread* thread, const Type& type) {
  return attributeValues(thread, type);
}

RawObject typeGetAttribute(Thread* thread, const Type& type,
                           const Object& name) {
  return typeGetAttributeSetLocation(thread, type, name, nullptr);
}

RawObject typeGetAttributeSetLocation(Thread* thread, const Type& type,
                                      const Object& name,
                                      Object* location_out) {
  // Look for the attribute in the meta class
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Type meta_type(&scope, runtime->typeOf(*type));
  Object meta_attr(&scope, typeLookupInMro(thread, *meta_type, *name));
  if (!meta_attr.isError()) {
    // TODO(T56002494): Remove this once type.__getattribute__ gets cached.
    if (meta_attr.isProperty()) {
      Object getter(&scope, Property::cast(*meta_attr).getter());
      if (!getter.isNoneType()) {
        return Interpreter::call1(thread, getter, type);
      }
    }
    Type meta_attr_type(&scope, runtime->typeOf(*meta_attr));
    if (typeIsDataDescriptor(thread, *meta_attr_type)) {
      return Interpreter::callDescriptorGet(thread, meta_attr, type, meta_type);
    }
  }

  // No data descriptor found on the meta class, look in the mro of the type
  Object attr(&scope,
              typeLookupInMroSetLocation(thread, *type, *name, location_out));
  if (!attr.isError()) {
    // TODO(T56002494): Remove this once type.__getattribute__ gets cached.
    if (attr.isFunction()) {
      // We always return the function object itself instead of a BoundMethod
      // due to the exception made below and another exception for NoneType in
      // function.__get__.
      return *attr;
    }
    Type attr_type(&scope, runtime->typeOf(*attr));
    if (typeIsNonDataDescriptor(thread, *attr_type)) {
      // Unfortunately calling `__get__` for a lookup on `type(None)` will look
      // exactly the same as calling it for a lookup on the `None` object.
      // To solve the ambiguity we add a special case for `type(None)` here.
      // Luckily it is impossible for the user to change the type so we can
      // special case the desired lookup behavior here.
      // Also see `METH(function, __get__)` for the related special casing
      // of lookups on the `None` object.
      if (type.builtinBase() == LayoutId::kNoneType) {
        return *attr;
      }
      if (location_out != nullptr) {
        *location_out = NoneType::object();
      }
      Object none(&scope, NoneType::object());
      return Interpreter::callDescriptorGet(thread, attr, none, type);
    }
    return *attr;
  }

  // No data descriptor found on the meta class, look on the type
  Object result(&scope, instanceGetAttribute(thread, type, name));
  if (!result.isError()) {
    return *result;
  }

  // No attr found in type or its mro, use the non-data descriptor found in
  // the metaclass (if any).
  if (!meta_attr.isError()) {
    return resolveDescriptorGet(thread, meta_attr, type, meta_type);
  }

  return Error::notFound();
}

static void addSubclass(Thread* thread, const Type& base, const Type& type) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  if (base.subclasses().isNoneType()) {
    base.setSubclasses(runtime->newList());
  }
  List subclasses(&scope, base.subclasses());
  Object value(&scope, runtime->newWeakRef(thread, type));
  runtime->listAdd(thread, subclasses, value);
}

void typeAddDocstring(Thread* thread, const Type& type) {
  // If the type dictionary doesn't contain a __doc__, set it from the doc
  // slot
  if (typeAtById(thread, type, ID(__doc__)).isErrorNotFound()) {
    HandleScope scope(thread);
    Object doc(&scope, type.doc());
    typeAtPutById(thread, type, ID(__doc__), doc);
  }
}

static RawObject fixedAttributeBaseOfType(Thread* thread, const Type& type);

// This searches recursively through `bases` for classes with the
// `kIsFixedAttributeBase` flag set. The algorithm picks the entry in bases
// which leads to a fixed attribute base class that is equal or a superclass of
// the fixed attribute bases found by the other bases entries.
// If `get_fixed_attr_base` is false, then the fixed attribute base is returned.
// If it is true, then the first entry in `bases` that is superclass of the
// fixed attribute base is returned.
static RawObject computeFixedAttributeBaseImpl(Thread* thread,
                                               const Tuple& bases,
                                               bool get_fixed_attr_base) {
  HandleScope scope(thread);
  Type result(&scope, bases.at(0));
  if (!result.isBasetype()) {
    Object type_name(&scope, result.name());
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "type '%S' is not an acceptable base type",
                                &type_name);
  }
  word bases_length = bases.length();
  if (!get_fixed_attr_base && bases_length == 1) {
    return *result;
  }

  Type result_fixed_attr_base(&scope, fixedAttributeBaseOfType(thread, result));
  Type base(&scope, *result);
  Type fixed_attr_base(&scope, *result);
  for (word i = 1, length = bases.length(); i < length; i++) {
    base = bases.at(i);
    if (!base.isBasetype()) {
      Object type_name(&scope, base.name());
      return thread->raiseWithFmt(LayoutId::kTypeError,
                                  "type '%S' is not an acceptable base type",
                                  &type_name);
    }

    fixed_attr_base = fixedAttributeBaseOfType(thread, base);
    if (typeIsSubclass(*result_fixed_attr_base, *fixed_attr_base)) {
      continue;
    }
    if (typeIsSubclass(*fixed_attr_base, *result_fixed_attr_base)) {
      result = *base;
      result_fixed_attr_base = *fixed_attr_base;
    } else {
      return thread->raiseWithFmt(
          LayoutId::kTypeError,
          "multiple bases have instance lay-out conflict");
    }
  }
  return get_fixed_attr_base ? *result_fixed_attr_base : *result;
}

// Returns the most generic base of `type` on `type's type hierarchy that
// contains all in-object attributes of `type`. Note that this is designed to
// simulate `solid_base` from CPython's typeobject.c.
static RawObject fixedAttributeBaseOfType(Thread* thread, const Type& type) {
  if (type.hasFlag(Type::Flag::kIsFixedAttributeBase)) {
    return *type;
  }
  if (!type.hasFlag(Type::Flag::kHasSlots)) {
    return thread->runtime()->typeAt(type.builtinBase());
  }
  HandleScope scope(thread);
  Tuple bases(&scope, type.bases());
  return computeFixedAttributeBaseImpl(thread, bases, true);
}

// Returns the most generic base among `bases` that captures inherited
// attributes with a fixed offset (either from __slots__ or builtin types)
// Note that this simulates `best_base` from CPython's typeobject.c.
static RawObject computeFixedAttributeBase(Thread* thread, const Tuple& bases) {
  return computeFixedAttributeBaseImpl(thread, bases, false);
}

static RawObject validateSlots(Thread* thread, const Type& type,
                               const Tuple& slots, bool base_has_instance_dict,
                               bool* add_instance_dict) {
  HandleScope scope(thread);
  word slots_len = slots.length();
  Runtime* runtime = thread->runtime();
  Str dunder_dict(&scope, runtime->symbols()->at(ID(__dict__)));
  *add_instance_dict = false;
  List result(&scope, runtime->newList());
  Object slot_obj(&scope, NoneType::object());
  Str slot_str(&scope, Str::empty());
  for (word i = 0; i < slots_len; i++) {
    slot_obj = slots.at(i);
    if (!runtime->isInstanceOfStr(*slot_obj)) {
      return thread->raiseWithFmt(LayoutId::kTypeError,
                                  "__slots__ items must be strings, not '%T'",
                                  &slot_obj);
    }
    slot_str = *slot_obj;
    if (!strIsIdentifier(slot_str)) {
      return thread->raiseWithFmt(LayoutId::kTypeError,
                                  "__slots__ must be identifiers");
    }
    slot_str = attributeName(thread, slot_str);
    if (slot_str == dunder_dict) {
      if (base_has_instance_dict || *add_instance_dict) {
        return thread->raiseWithFmt(
            LayoutId::kTypeError,
            "__dict__ slot disallowed: we already got one");
      }
      *add_instance_dict = true;
      continue;
    }
    if (!typeAt(type, slot_str).isErrorNotFound()) {
      return thread->raiseWithFmt(
          LayoutId::kValueError,
          "'%S' in __slots__ conflicts with class variable", &slot_str);
    }
    runtime->listAdd(thread, result, slot_str);
  }
  if (result.numItems() == 0) return NoneType::object();
  listSort(thread, result);
  return *result;
}

static word estimateNumAttributes(Thread* thread, const Type& type) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  // Collect set of in-object attributes by scanning the __init__ method of
  // each class in the MRO. This is used to determine the number of slots
  // allocated for in-object attributes when instances are created.
  Tuple mro(&scope, type.mro());
  Dict attr_names(&scope, runtime->newDict());
  for (word i = 0; i < mro.length(); i++) {
    Type mro_type(&scope, mro.at(i));
    Object maybe_init(&scope, runtime->classConstructor(mro_type));
    if (!maybe_init.isFunction()) {
      continue;
    }
    Function init(&scope, *maybe_init);
    RawObject maybe_code = init.code();
    if (!maybe_code.isCode()) {
      continue;  // native trampoline
    }
    Code code(&scope, maybe_code);
    if (code.code().isSmallInt()) {
      continue;  // builtin trampoline
    }
    runtime->collectAttributes(code, attr_names);
  }
  return attr_names.numItems();
}

static void setSlotAttributes(Thread* thread, const MutableTuple& dst,
                              word start_index, const List& slots) {
  HandleScope scope(thread);
  Object descriptor(&scope, NoneType::object());
  Object name(&scope, NoneType::object());
  Runtime* runtime = thread->runtime();
  for (word i = 0, offset = start_index * kPointerSize; i < slots.numItems();
       i++, offset += kPointerSize) {
    descriptor = slots.at(i);
    AttributeInfo info(offset, AttributeFlags::kInObject |
                                   AttributeFlags::kFixedOffset |
                                   AttributeFlags::kInitWithUnbound);
    name = SlotDescriptor::cast(*descriptor).name();
    dst.atPut(start_index + i, runtime->layoutNewAttribute(name, info));
    SlotDescriptor::cast(*descriptor).setOffset(offset);
  }
}

static RawObject typeComputeLayout(Thread* thread, const Type& type,
                                   bool enable_overflow, const Object& slots) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Tuple bases(&scope, type.bases());
  Object fixed_attr_base_obj(&scope, computeFixedAttributeBase(thread, bases));
  if (fixed_attr_base_obj.isErrorException()) return *fixed_attr_base_obj;
  Type fixed_attr_base(&scope, *fixed_attr_base_obj);

  // Create new layout.
  DCHECK(type.instanceLayout().isNoneType(),
         "must not set layout multiple times");
  LayoutId layout_id = runtime->reserveLayoutId(thread);
  Layout layout(&scope, runtime->newLayout(layout_id));

  // Compute in-object attributes.
  Layout base_layout(&scope, fixed_attr_base.instanceLayout());
  Tuple base_attributes(&scope, base_layout.inObjectAttributes());
  word num_base_attributes = base_attributes.length();
  word num_slots = slots.isList() ? List::cast(*slots).numItems() : 0;
  word num_initial_attributes = num_base_attributes + num_slots;
  if (num_initial_attributes == 0) {
    layout.setInObjectAttributes(runtime->emptyTuple());
  } else {
    MutableTuple attributes(&scope,
                            runtime->newMutableTuple(num_initial_attributes));
    attributes.replaceFromWith(0, *base_attributes, num_base_attributes);
    if (num_slots > 0) {
      List slots_list(&scope, *slots);
      setSlotAttributes(thread, attributes, /*start_index=*/num_base_attributes,
                        slots_list);
    }
    layout.setInObjectAttributes(attributes.becomeImmutable());
  }

  word num_extra_attributes = estimateNumAttributes(thread, type);
  layout.setNumInObjectAttributes(num_initial_attributes +
                                  num_extra_attributes);

  // Determine overflow mode.
  Type base(&scope, *type);
  Object overflow(&scope, base_layout.overflowAttributes());
  Object base_overflow(&scope, NoneType::object());
  bool bases_have_custom_dict = false;
  for (word i = 0, length = bases.length(); i < length; i++) {
    base = bases.at(i);
    if (base.hasCustomDict()) bases_have_custom_dict = true;
    base_overflow = Layout::cast(base.instanceLayout()).overflowAttributes();
    if (overflow == base_overflow || base_overflow.isNoneType()) continue;
    if (overflow.isNoneType() && base_overflow.isTuple()) {
      overflow = *base_overflow;
      continue;
    }
    UNIMPLEMENTED("Mixing dict and tuple overflow");
  }
  DCHECK(!overflow.isTuple() || Tuple::cast(*overflow).length() == 0,
         "base layout must not have tuple overflow attributes assigned");
  if (enable_overflow && !bases_have_custom_dict && overflow.isNoneType()) {
    overflow = runtime->emptyTuple();
  }
  layout.setOverflowAttributes(*overflow);

  runtime->layoutAtPut(layout_id, *layout);
  layout.setDescribedType(*type);
  type.setInstanceLayout(*layout);
  type.setInstanceLayoutId(layout.id());
  type.setBuiltinBase(fixed_attr_base.builtinBase());
  return NoneType::object();
}

static RawObject getModuleNameAtFrame(Thread* thread, int depth) {
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  for (int i = 0; i < depth && !frame->isSentinel(); i++) {
    frame = frame->previousFrame();
  }
  if (frame->isSentinel()) return Error::notFound();
  Object module_obj(&scope, frame->function().moduleObject());
  // Some functions (e.g C-API extension functions) have no associated module.
  if (module_obj.isNoneType()) return Error::notFound();
  Module module(&scope, *module_obj);
  return moduleAtById(thread, module, ID(__name__));
}

RawObject typeNew(Thread* thread, const Type& metaclass, const Str& name,
                  const Tuple& bases, const Dict& dict, word flags,
                  bool inherit_slots, bool add_instance_dict) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();

  LayoutId metaclass_id = Layout::cast(metaclass.instanceLayout()).id();
  Type type(&scope, runtime->newTypeWithMetaclass(metaclass_id));
  type.setName(*name);
  type.setBases(*bases);

  // Determine metaclass.mro method. Set `mro_method` to `None` if it is the
  // default `type.mro`.
  Object mro_method(&scope, Unbound::object());
  if (metaclass_id != LayoutId::kType) {
    mro_method = Interpreter::lookupMethod(thread, type, ID(mro));
    if (mro_method.isErrorException()) return *mro_method;
    if (mro_method.isErrorNotFound()) {
      Object mro_name(&scope, runtime->symbols()->at(ID(mro)));
      return objectRaiseAttributeError(thread, metaclass, mro_name);
    }
    Type builtin_type(&scope, runtime->typeAt(LayoutId::kType));
    if (mro_method == typeAtById(thread, builtin_type, ID(mro))) {
      mro_method = Unbound::object();
    }
  }
  Object mro_obj(&scope, NoneType::object());
  if (mro_method.isUnbound()) {
    mro_obj = computeMro(thread, type);
    if (mro_obj.isErrorException()) return *mro_obj;
  } else {
    flags |= Type::Flag::kHasCustomMro;
    mro_obj = Interpreter::callMethod1(thread, mro_method, type);
    if (mro_obj.isErrorException()) return *mro_obj;
    if (!mro_obj.isTuple()) {
      mro_obj = thread->invokeFunction1(ID(builtins), ID(tuple), mro_obj);
      if (mro_obj.isErrorException()) return *mro_obj;
      CHECK(mro_obj.isTuple(), "Result of builtins.tuple should be tuple");
    }
  }

  Tuple mro(&scope, *mro_obj);
  type.setMro(*mro);

  Object result(&scope, typeAssignFromDict(thread, type, dict));
  if (result.isErrorException()) return *result;

  if (flags & Type::Flag::kIsCPythonHeaptype) {
    if (typeAtById(thread, type, ID(__module__)).isErrorNotFound()) {
      // Use depth=3 to skip over frame of `_type_init`, `type.__new__`
      // and `type.__call__`.
      Object module_name(&scope, getModuleNameAtFrame(thread, /*depth=*/3));
      if (!module_name.isErrorNotFound()) {
        typeAtPutById(thread, type, ID(__module__), module_name);
      }
    }
  } else {
    // Non-heap-types in CPython have no `__module__` unless there is a
    // "." in `tp_name`. Remove the attribute when it equals "builtins".
    Object module_name(&scope, typeAtById(thread, type, ID(__module__)));
    if (module_name.isStr() &&
        Str::cast(*module_name).equals(runtime->symbols()->at(ID(builtins)))) {
      typeRemoveById(thread, type, ID(__module__));
    }
  }

  Object qualname(&scope, typeRemoveById(thread, type, ID(__qualname__)));
  if (qualname.isErrorNotFound()) {
    qualname = *name;
  } else if (!runtime->isInstanceOfStr(*qualname)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "type __qualname__ must be a str, not %T",
                                &qualname);
  }
  type.setQualname(*qualname);

  // TODO(T53997177): Centralize type initialization
  typeAddDocstring(thread, type);

  // Special-case __new__ to be a staticmethod
  Object dunder_new(&scope, typeAtById(thread, type, ID(__new__)));
  if (dunder_new.isFunction()) {
    StaticMethod dunder_new_method(&scope, runtime->newStaticMethod());
    dunder_new_method.setFunction(*dunder_new);
    typeAtPutById(thread, type, ID(__new__), dunder_new_method);
  }

  // Special-case __init_subclass__ to be a classmethod
  Object init_subclass(&scope, typeAtById(thread, type, ID(__init_subclass__)));
  if (init_subclass.isFunction()) {
    ClassMethod init_subclass_method(&scope, runtime->newClassMethod());
    init_subclass_method.setFunction(*init_subclass);
    typeAtPutById(thread, type, ID(__init_subclass__), init_subclass_method);
  }

  // Special-case __class_getitem__ to be a classmethod
  Object class_getitem(&scope, typeAtById(thread, type, ID(__class_getitem__)));
  if (class_getitem.isFunction()) {
    ClassMethod class_getitem_method(&scope, runtime->newClassMethod());
    class_getitem_method.setFunction(*class_getitem);
    typeAtPutById(thread, type, ID(__class_getitem__), class_getitem_method);
  }

  Object class_cell(&scope, typeAtById(thread, type, ID(__classcell__)));
  if (!class_cell.isErrorNotFound()) {
    DCHECK(class_cell.isCell(), "class cell must be a cell");
    Cell::cast(*class_cell).setValue(*type);
    Object class_cell_name(&scope, runtime->symbols()->at(ID(__classcell__)));
    typeRemove(thread, type, class_cell_name);
  }

  // Analyze bases: Merge flags; add to subclasses lists; check for attribute
  // dictionaries.
  Type base_type(&scope, *type);
  bool bases_have_instance_dict = false;
  bool bases_have_type_slots = false;
  word bases_flags = 0;
  for (word i = 0; i < bases.length(); i++) {
    base_type = bases.at(i);
    bases_flags |= base_type.flags();
    addSubclass(thread, base_type, type);
    bases_have_type_slots |= typeHasSlots(base_type);
    if (base_type.hasCustomDict()) bases_have_instance_dict = true;
    if (!Layout::cast(base_type.instanceLayout()).isSealed()) {
      bases_have_instance_dict = true;
    }
  }
  if (bases_have_instance_dict) add_instance_dict = false;
  flags |= (bases_flags & Type::kInheritableFlags);
  // Attribute flags are set explicitly here since `typeAssignFromDict` cannot
  // compute it properly with a partially created type object:
  // Computing this properly depends if Type::Flag::kHasCustomMro is set or not.
  for (SymbolId id : kAttributesForTypeFlags) {
    flags = computeAttributeTypeFlags(thread, type, id, flags);
  }
  // TODO(T66646764): This is a hack to make `type` look finalized. Remove this.
  type.setFlags(static_cast<Type::Flag>(flags));

  if (bases_have_type_slots) {
    if (inherit_slots) {
      result = typeInheritSlots(thread, type);
      if (result.isErrorException()) return *result;
    }
  }

  Object dunder_slots_obj(&scope, typeAtById(thread, type, ID(__slots__)));
  Object slots_obj(&scope, NoneType::object());
  if (!dunder_slots_obj.isErrorNotFound()) {
    // NOTE: CPython raises an exception when slots are given to a subtype of a
    // type with type.tp_itemsize != 0, which means having a variable length.
    // For example, __slots__ in int's subtype or str's type is disallowed.
    // This behavior is ignored in Pyro since all objects' size in RawObject is
    // fixed in Pyro.
    if (runtime->isInstanceOfStr(*dunder_slots_obj)) {
      dunder_slots_obj = runtime->newTupleWith1(dunder_slots_obj);
    } else if (!runtime->isInstanceOfTuple(*dunder_slots_obj)) {
      Type tuple_type(&scope, runtime->typeAt(LayoutId::kTuple));
      dunder_slots_obj =
          Interpreter::call1(thread, tuple_type, dunder_slots_obj);
      if (dunder_slots_obj.isErrorException()) {
        return *dunder_slots_obj;
      }
      CHECK(dunder_slots_obj.isTuple(), "Converting __slots__ to tuple failed");
    }
    Tuple slots_tuple(&scope, *dunder_slots_obj);
    slots_obj = validateSlots(thread, type, slots_tuple,
                              bases_have_instance_dict, &add_instance_dict);
    if (slots_obj.isErrorException()) {
      return *slots_obj;
    }
    if (!slots_obj.isNoneType()) {
      List slots(&scope, *slots_obj);
      // Add descriptors that mediate access to __slots__ attributes.
      Object slot_descriptor(&scope, NoneType::object());
      Object slot_name(&scope, NoneType::object());
      for (word i = 0, num_items = slots.numItems(); i < num_items; i++) {
        slot_name = slots.at(i);
        slot_descriptor = runtime->newSlotDescriptor(type, slot_name);
        typeAtPut(thread, type, slot_name, slot_descriptor);
        slots.atPut(i, *slot_descriptor);
      }
    }
  }

  if (!slots_obj.isNoneType()) {
    flags |= Type::Flag::kHasSlots;
    flags |= Type::Flag::kIsFixedAttributeBase;
  }

  type.setFlags(static_cast<Type::Flag>(flags));

  // Add a `__dict__` descriptor when we added an instance dict.
  if (add_instance_dict &&
      typeAtById(thread, type, ID(__dict__)).isErrorNotFound()) {
    Object instance_proxy(&scope, runtime->typeAt(LayoutId::kInstanceProxy));
    CHECK(instance_proxy.isType(), "instance_proxy not found");
    Module under_builtins(&scope, runtime->findModuleById(ID(_builtins)));
    Function under_instance_dunder_dict_set(
        &scope,
        moduleAtById(thread, under_builtins, ID(_instance_dunder_dict_set)));
    Object none(&scope, NoneType::object());
    Object property(&scope,
                    runtime->newProperty(instance_proxy,
                                         under_instance_dunder_dict_set, none));
    typeAtPutById(thread, type, ID(__dict__), property);
  }

  Function type_dunder_call(&scope,
                            runtime->lookupNameInModule(thread, ID(_builtins),
                                                        ID(_type_dunder_call)));
  type.setCtor(*type_dunder_call);

  result = typeComputeLayout(thread, type, add_instance_dict, slots_obj);
  if (result.isErrorException()) return *result;

  return *type;
}

// NOTE: Keep the order of these type attributes same as the one from
// rewriteOperation.
static const SymbolId kUnimplementedTypeAttrUpdates[] = {
    // LOAD_ATTR, LOAD_METHOD
    ID(__getattribute__),
    // STORE_ATTR
    ID(__setattr__)};

void terminateIfUnimplementedTypeAttrCacheInvalidation(
    Thread* thread, const Type& type, const Object& attr_name) {
  if (attributeValueCellAt(*type, *attr_name).isErrorNotFound()) {
    // No need for cache invalidation due to the absence of the attribute.
    return;
  }
  Runtime* runtime = thread->runtime();
  DCHECK(Runtime::isInternedStr(thread, attr_name), "expected interned str");
  for (uword i = 0; i < ARRAYSIZE(kUnimplementedTypeAttrUpdates); ++i) {
    if (attr_name == runtime->symbols()->at(kUnimplementedTypeAttrUpdates[i])) {
      UNIMPLEMENTED("unimplemented cache invalidation for type.%s update",
                    Str::cast(*attr_name).toCStr());
    }
  }
}

RawObject typeSetAttr(Thread* thread, const Type& type, const Object& name,
                      const Object& value) {
  Runtime* runtime = thread->runtime();
  DCHECK(runtime->isInternedStr(thread, name),
         "name must be an interned string");
  // Make sure cache invalidation is correctly done for this.
  terminateIfUnimplementedTypeAttrCacheInvalidation(thread, type, name);
  HandleScope scope(thread);
  if (!type.hasMutableDict()) {
    return raiseTypeErrorCannotSetImmutable(thread, type);
  }

  // Check for a data descriptor
  Type metatype(&scope, runtime->typeOf(*type));
  Object meta_attr(&scope, typeLookupInMro(thread, *metatype, *name));
  if (!meta_attr.isError()) {
    Type meta_attr_type(&scope, runtime->typeOf(*meta_attr));
    if (typeIsDataDescriptor(thread, *meta_attr_type)) {
      Object set_result(&scope, Interpreter::callDescriptorSet(
                                    thread, meta_attr, type, value));
      if (set_result.isError()) return *set_result;
      return NoneType::object();
    }
  }

  // No data descriptor found, store the attribute in the type dict
  typeAtPut(thread, type, name, value);
  return NoneType::object();
}

RawObject typeSetDunderClass(Thread* thread, const Object& self,
                             const Type& new_type) {
  Runtime* runtime = thread->runtime();
  // TODO(T60761420): A module can't change its type since its attributes are
  // cached based on object identity (and not layout id). This needs extra
  // cache invalidation code here to support it.
  if (runtime->isInstanceOfModule(*self)) {
    UNIMPLEMENTED("Cannot change type of modules");
  }

  HandleScope scope(thread);
  Type instance_type(&scope, runtime->typeOf(*self));
  // Builtin base type must match
  if (instance_type.builtinBase() != new_type.builtinBase()) {
    Str type_name(&scope, new_type.name());
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "__class__ assignment '%T' object layout differs from '%S'", &self,
        &type_name);
  }

  // Handle C Extension types
  if (instance_type.hasFlag(RawType::Flag::kHasNativeData) &&
      new_type.hasFlag(RawType::Flag::kHasNativeData)) {
    // TODO(T60752528): Handle __class__ setter for C Extension Types
    UNIMPLEMENTED("Check if native memory is compatible");
  } else if (instance_type.hasFlag(RawType::Flag::kHasNativeData) !=
             new_type.hasFlag(RawType::Flag::kHasNativeData)) {
    Str type_name(&scope, new_type.name());
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "__class__ assignment '%T' object layout differs from '%S'", &self,
        &type_name);
  }

  // LOAD_ATTR_TYPE caches attributes based on the instance layout id of the
  // type. Create a copy of the layout with a new id, to avoid incorrectly
  // hitting the cache.
  if (runtime->isInstanceOfType(*self)) {
    Type type(&scope, *self);
    Layout instance_layout(&scope, type.instanceLayout());
    Layout new_layout(&scope,
                      runtime->layoutCreateCopy(thread, instance_layout));
    type.setInstanceLayout(*new_layout);
    type.setInstanceLayoutId(new_layout.id());
  }

  // Transition the layout
  Instance instance(&scope, *self);
  Layout from_layout(&scope, runtime->layoutOf(*instance));
  Layout new_layout(
      &scope, runtime->layoutSetDescribedType(thread, from_layout, new_type));
  instance.setLayoutId(new_layout.id());
  return NoneType::object();
}

static const BuiltinAttribute kTypeAttributes[] = {
    {ID(_type__attributes), RawType::kAttributesOffset,
     AttributeFlags::kHidden},
    {ID(_type__attributes_remaining), RawType::kAttributesRemainingOffset,
     AttributeFlags::kHidden},
    {ID(__mro__), RawType::kMroOffset, AttributeFlags::kReadOnly},
    {ID(_type__bases), RawType::kBasesOffset, AttributeFlags::kHidden},
    {ID(_type__instance_layout), RawType::kInstanceLayoutOffset,
     AttributeFlags::kHidden},
    {ID(_type__instance_layout_id), RawType::kInstanceLayoutIdOffset,
     AttributeFlags::kHidden},
    {ID(_type__name), RawType::kNameOffset, AttributeFlags::kHidden},
    {ID(__doc__), RawType::kDocOffset},
    {ID(_type__flags), RawType::kFlagsOffset, AttributeFlags::kHidden},
    {ID(_type__slots), RawType::kSlotsOffset, AttributeFlags::kHidden},
    {ID(_type__abstract_methods), RawType::kAbstractMethodsOffset,
     AttributeFlags::kHidden},
    {ID(_type__subclasses), RawType::kSubclassesOffset,
     AttributeFlags::kHidden},
    {ID(_type__proxy), RawType::kProxyOffset, AttributeFlags::kHidden},
    {ID(_type__ctor), RawType::kCtorOffset, AttributeFlags::kHidden},
    {ID(_type__qualname), RawType::kQualnameOffset, AttributeFlags::kHidden},
};

static const BuiltinAttribute kTypeProxyAttributes[] = {
    {ID(_type_proxy__type), TypeProxy::kTypeOffset, AttributeFlags::kHidden},
};

void initializeTypeTypes(Thread* thread) {
  HandleScope scope(thread);
  Type type(&scope, addBuiltinType(thread, ID(type), LayoutId::kType,
                                   /*superclass_id=*/LayoutId::kObject,
                                   kTypeAttributes, Type::kSize,
                                   /*basetype=*/true));
  word flags = static_cast<word>(type.flags());
  flags |= RawType::Flag::kHasCustomDict;
  type.setFlags(static_cast<Type::Flag>(flags));

  addBuiltinType(thread, ID(type_proxy), LayoutId::kTypeProxy,
                 /*superclass_id=*/LayoutId::kObject, kTypeProxyAttributes,
                 TypeProxy::kSize, /*basetype=*/false);
}

RawObject METH(type, __base__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfType(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(type));
  }
  Type self(&scope, *self_obj);
  Tuple bases(&scope, self.bases());
  if (bases.length() == 0) {
    return NoneType::object();
  }
  return computeFixedAttributeBase(thread, bases);
}

RawObject METH(type, __basicsize__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfType(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(type));
  }
  Type self(&scope, *self_obj);
  if (!self.hasNativeData()) {
    if (self.isCPythonHeaptype()) {
      // If self is a heap type here, there are two possibiliities:
      // It either had a __basicsize__ of 0, which means we've already set a
      // default __basicsize__ of sizeof(PyObject)
      // Or someone created a heap type with just enough space for a
      // PyObject_HEAD and no additional data.
      // It should be safe to return sizeof(PyObject) in both cases.
      return runtime->newInt(sizeof(PyObject));
    }
    Str name(&scope, strUnderlying(self.name()));
    UNIMPLEMENTED("'__basicsize__' for type '%s'", name.toCStr());
  }
  uword basicsize = typeGetBasicSize(self);
  return runtime->newIntFromUnsigned(basicsize);
}

RawObject METH(type, __flags__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfType(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(type));
  }
  Type self(&scope, *self_obj);
  uword cpython_flags = typeGetFlags(self);
  return runtime->newIntFromUnsigned(cpython_flags);
}

RawObject METH(type, __getattribute__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfType(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(type));
  }
  Type self(&scope, *self_obj);
  Object name(&scope, args.get(1));
  name = attributeName(thread, name);
  if (name.isErrorException()) return *name;
  Object result(&scope, typeGetAttribute(thread, self, name));
  if (result.isErrorNotFound()) {
    Object type_name(&scope, self.name());
    return thread->raiseWithFmt(LayoutId::kAttributeError,
                                "type object '%S' has no attribute '%S'",
                                &type_name, &name);
  }
  return *result;
}

RawObject METH(type, __setattr__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfType(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(type));
  }
  Type self(&scope, *self_obj);
  Object name(&scope, args.get(1));
  name = attributeName(thread, name);
  if (name.isErrorException()) return *name;
  Object value(&scope, args.get(2));
  return typeSetAttr(thread, self, name, value);
}

RawObject METH(type, __subclasses__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfType(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(type));
  }
  Type self(&scope, *self_obj);
  Object subclasses_obj(&scope, self.subclasses());
  if (subclasses_obj.isNoneType()) {
    return runtime->newList();
  }

  // Check list for `None` and compact it.
  List subclasses(&scope, *subclasses_obj);
  word num_items = subclasses.numItems();
  Object ref(&scope, NoneType::object());
  Object value(&scope, NoneType::object());
  word compact_shift = 0;
  for (word i = 0; i < num_items; i++) {
    ref = subclasses.at(i);
    value = WeakRef::cast(*ref).referent();
    if (value.isNoneType()) {
      compact_shift++;
      continue;
    }
    if (compact_shift > 0) {
      subclasses.atPut(i - compact_shift, *ref);
    }
  }
  if (compact_shift > 0) {
    num_items -= compact_shift;
    subclasses.setNumItems(num_items);
  }

  List result(&scope, runtime->newList());
  runtime->listEnsureCapacity(thread, result, num_items);
  for (word i = 0; i < num_items; i++) {
    ref = subclasses.at(i);
    value = WeakRef::cast(*ref).referent();
    runtime->listAdd(thread, result, value);
  }
  return *result;
}

}  // namespace py
