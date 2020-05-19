#include "type-builtins.h"

#include "cpython-data.h"

#include "builtins.h"
#include "bytecode.h"
#include "capi-handles.h"
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

namespace py {

static const int kBucketNumWords = 2;
static const int kBucketKeyOffset = 0;
static const int kBucketValueOffset = 1;
static const RawObject kEmptyKey = NoneType::object();
static const RawObject kTombstoneKey = Unbound::object();
static const word kInitialCapacity = 16;

RawObject attributeName(Thread* thread, const Object& name_obj) {
  if (name_obj.isSmallStr()) {
    return *name_obj;
  }
  if (name_obj.isLargeStr()) {
    return Runtime::internLargeStr(thread, name_obj);
  }

  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfStr(*name_obj)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "attribute name must be string, not '%T'",
                                &name_obj);
  }
  HandleScope scope(thread);
  Type type(&scope, runtime->typeOf(*name_obj));
  if (typeLookupInMroById(thread, type, ID(__eq__)) != runtime->strDunderEq() ||
      typeLookupInMroById(thread, type, ID(__hash__)) !=
          runtime->strDunderHash()) {
    UNIMPLEMENTED(
        "str subclasses with __eq__ or __hash__ not supported as attribute "
        "name");
  }
  Str name_str(&scope, strUnderlying(*name_obj));
  return Runtime::internStr(thread, name_str);
}

RawObject attributeNameNoException(Thread* thread, const Object& name_obj) {
  if (name_obj.isSmallStr()) {
    return *name_obj;
  }
  if (name_obj.isLargeStr()) {
    return Runtime::internLargeStr(thread, name_obj);
  }

  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfStr(*name_obj)) {
    return Error::error();
  }
  HandleScope scope(thread);
  Type type(&scope, runtime->typeOf(*name_obj));
  if (typeLookupInMroById(thread, type, ID(__eq__)) != runtime->strDunderEq() ||
      typeLookupInMroById(thread, type, ID(__hash__)) !=
          runtime->strDunderHash()) {
    UNIMPLEMENTED(
        "str subclasses with __eq__ or __hash__ not supported as attribute "
        "name");
  }
  Str name_str(&scope, strUnderlying(*name_obj));
  return Runtime::internStr(thread, name_str);
}

static RawObject addBuiltinTypeWithLayout(Thread* thread, const Layout& layout,
                                          SymbolId name, LayoutId builtin_base,
                                          LayoutId superclass_id) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Type type(&scope, runtime->newType());
  type.setName(runtime->symbols()->at(name));
  Type superclass(&scope, runtime->typeAt(superclass_id));
  Tuple superclass_mro(&scope, superclass.mro());
  word mro_length = superclass_mro.length() + 1;
  MutableTuple mro(&scope, runtime->newMutableTuple(mro_length));
  mro.atPut(0, *type);
  mro.replaceFromWith(1, *superclass_mro, mro_length - 1);
  type.setMro(mro.becomeImmutable());
  type.setInstanceLayout(*layout);
  Type::Flag flags =
      static_cast<Type::Flag>(superclass.flags() & ~Type::Flag::kIsAbstract);
  type.setFlagsAndBuiltinBase(flags, builtin_base);
  type.setBases(runtime->newTupleWith1(superclass));
  layout.setDescribedType(*type);
  return *type;
}

RawObject addBuiltinType(Thread* thread, SymbolId name, LayoutId layout_id,
                         LayoutId superclass_id, View<BuiltinAttribute> attrs) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Layout layout(&scope, runtime->layoutCreateSubclassWithBuiltins(
                            layout_id, superclass_id, attrs));
  runtime->layoutAtPut(layout_id, *layout);
  LayoutId builtin_base = attrs.length() == 0 ? superclass_id : layout_id;
  return addBuiltinTypeWithLayout(thread, layout, name, builtin_base,
                                  superclass_id);
}

RawObject addEmptyBuiltinType(Thread* thread, SymbolId name, LayoutId layout_id,
                              LayoutId superclass_id) {
  HandleScope scope(thread);
  Type type(&scope, addBuiltinType(thread, name, layout_id, superclass_id,
                                   {nullptr, 0}));
  Layout layout(&scope, type.instanceLayout());
  thread->runtime()->layoutSetTupleOverflow(*layout);
  return *type;
}

RawObject addImmediateBuiltinType(Thread* thread, SymbolId name,
                                  LayoutId layout_id, LayoutId builtin_base,
                                  LayoutId superclass_id) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Layout layout(&scope, runtime->newLayout(layout_id));
  runtime->layoutAtPut(layout_id, *layout);
  return addBuiltinTypeWithLayout(thread, layout, name, builtin_base,
                                  superclass_id);
}

Type::Slot slotToTypeSlot(int slot) {
  switch (slot) {
    case Py_mp_ass_subscript:
      return Type::Slot::kMapAssSubscript;
    case Py_mp_length:
      return Type::Slot::kMapLength;
    case Py_mp_subscript:
      return Type::Slot::kMapSubscript;
    case Py_nb_absolute:
      return Type::Slot::kNumberAbsolute;
    case Py_nb_add:
      return Type::Slot::kNumberAdd;
    case Py_nb_and:
      return Type::Slot::kNumberAnd;
    case Py_nb_bool:
      return Type::Slot::kNumberBool;
    case Py_nb_divmod:
      return Type::Slot::kNumberDivmod;
    case Py_nb_float:
      return Type::Slot::kNumberFloat;
    case Py_nb_floor_divide:
      return Type::Slot::kNumberFloorDivide;
    case Py_nb_index:
      return Type::Slot::kNumberIndex;
    case Py_nb_inplace_add:
      return Type::Slot::kNumberInplaceAdd;
    case Py_nb_inplace_and:
      return Type::Slot::kNumberInplaceAnd;
    case Py_nb_inplace_floor_divide:
      return Type::Slot::kNumberInplaceFloorDivide;
    case Py_nb_inplace_lshift:
      return Type::Slot::kNumberInplaceLshift;
    case Py_nb_inplace_multiply:
      return Type::Slot::kNumberInplaceMultiply;
    case Py_nb_inplace_or:
      return Type::Slot::kNumberInplaceOr;
    case Py_nb_inplace_power:
      return Type::Slot::kNumberInplacePower;
    case Py_nb_inplace_remainder:
      return Type::Slot::kNumberInplaceRemainder;
    case Py_nb_inplace_rshift:
      return Type::Slot::kNumberInplaceRshift;
    case Py_nb_inplace_subtract:
      return Type::Slot::kNumberInplaceSubtract;
    case Py_nb_inplace_true_divide:
      return Type::Slot::kNumberInplaceTrueDivide;
    case Py_nb_inplace_xor:
      return Type::Slot::kNumberInplaceXor;
    case Py_nb_int:
      return Type::Slot::kNumberInt;
    case Py_nb_invert:
      return Type::Slot::kNumberInvert;
    case Py_nb_lshift:
      return Type::Slot::kNumberLshift;
    case Py_nb_multiply:
      return Type::Slot::kNumberMultiply;
    case Py_nb_negative:
      return Type::Slot::kNumberNegative;
    case Py_nb_or:
      return Type::Slot::kNumberOr;
    case Py_nb_positive:
      return Type::Slot::kNumberPositive;
    case Py_nb_power:
      return Type::Slot::kNumberPower;
    case Py_nb_remainder:
      return Type::Slot::kNumberRemainder;
    case Py_nb_rshift:
      return Type::Slot::kNumberRshift;
    case Py_nb_subtract:
      return Type::Slot::kNumberSubtract;
    case Py_nb_true_divide:
      return Type::Slot::kNumberTrueDivide;
    case Py_nb_xor:
      return Type::Slot::kNumberXor;
    case Py_sq_ass_item:
      return Type::Slot::kSequenceAssItem;
    case Py_sq_concat:
      return Type::Slot::kSequenceConcat;
    case Py_sq_contains:
      return Type::Slot::kSequenceContains;
    case Py_sq_inplace_concat:
      return Type::Slot::kSequenceInplaceConcat;
    case Py_sq_inplace_repeat:
      return Type::Slot::kSequenceInplaceRepeat;
    case Py_sq_item:
      return Type::Slot::kSequenceItem;
    case Py_sq_length:
      return Type::Slot::kSequenceLength;
    case Py_sq_repeat:
      return Type::Slot::kSequenceRepeat;
    case Py_tp_alloc:
      return Type::Slot::kAlloc;
    case Py_tp_base:
      return Type::Slot::kBase;
    case Py_tp_bases:
      return Type::Slot::kBases;
    case Py_tp_call:
      return Type::Slot::kCall;
    case Py_tp_clear:
      return Type::Slot::kClear;
    case Py_tp_dealloc:
      return Type::Slot::kDealloc;
    case Py_tp_del:
      return Type::Slot::kDel;
    case Py_tp_descr_get:
      return Type::Slot::kDescrGet;
    case Py_tp_descr_set:
      return Type::Slot::kDescrSet;
    case Py_tp_doc:
      return Type::Slot::kDoc;
    case Py_tp_getattr:
      return Type::Slot::kGetattr;
    case Py_tp_getattro:
      return Type::Slot::kGetattro;
    case Py_tp_hash:
      return Type::Slot::kHash;
    case Py_tp_init:
      return Type::Slot::kInit;
    case Py_tp_is_gc:
      return Type::Slot::kIsGc;
    case Py_tp_iter:
      return Type::Slot::kIter;
    case Py_tp_iternext:
      return Type::Slot::kIternext;
    case Py_tp_methods:
      return Type::Slot::kMethods;
    case Py_tp_new:
      return Type::Slot::kNew;
    case Py_tp_repr:
      return Type::Slot::kRepr;
    case Py_tp_richcompare:
      return Type::Slot::kRichcompare;
    case Py_tp_setattr:
      return Type::Slot::kSetattr;
    case Py_tp_setattro:
      return Type::Slot::kSetattro;
    case Py_tp_str:
      return Type::Slot::kStr;
    case Py_tp_traverse:
      return Type::Slot::kTraverse;
    case Py_tp_members:
      return Type::Slot::kMembers;
    case Py_tp_getset:
      return Type::Slot::kGetset;
    case Py_tp_free:
      return Type::Slot::kFree;
    case Py_nb_matrix_multiply:
      return Type::Slot::kNumberMatrixMultiply;
    case Py_nb_inplace_matrix_multiply:
      return Type::Slot::kNumberInplaceMatrixMultiply;
    case Py_am_await:
      return Type::Slot::kAsyncAwait;
    case Py_am_aiter:
      return Type::Slot::kAsyncAiter;
    case Py_am_anext:
      return Type::Slot::kAsyncAnext;
    case Py_tp_finalize:
      return Type::Slot::kFinalize;
    default:
      return Type::Slot::kEnd;
  }
}

bool typeIsDataDescriptor(Thread* thread, const Type& type) {
  if (type.isBuiltin()) {
    LayoutId layout_id = Layout::cast(type.instanceLayout()).id();
    return layout_id == LayoutId::kProperty ||
           layout_id == LayoutId::kSlotDescriptor;
  }
  // TODO(T25692962): Track "descriptorness" through a bit on the class
  return !typeLookupInMroById(thread, type, ID(__set__)).isError();
}

bool typeIsNonDataDescriptor(Thread* thread, const Type& type) {
  if (type.isBuiltin()) {
    switch (Layout::cast(type.instanceLayout()).id()) {
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
  HandleScope scope(thread);
  Type type(&scope, thread->runtime()->typeOf(*descr));
  if (!typeIsNonDataDescriptor(thread, type)) return *descr;
  return Interpreter::callDescriptorGet(thread, thread->currentFrame(), descr,
                                        instance, instance_type);
}

static inline RawObject lookupCell(RawMutableTuple data, RawObject name,
                                   word hash, bool return_placeholder) {
  word mask = (data.length() - 1) >> 1;
  for (word bucket = hash & mask, num_probes = 0;;
       bucket = (bucket + ++num_probes) & mask) {
    word idx = bucket * kBucketNumWords;
    RawObject key = data.at(idx + kBucketKeyOffset);
    if (key == name) {
      RawValueCell cell = ValueCell::cast(data.at(idx + kBucketValueOffset));
      if (!return_placeholder && cell.isPlaceholder()) {
        return Error::notFound();
      }
      return cell;
    }
    if (key == kEmptyKey) {
      return Error::notFound();
    }
    // Remaining cases are either a key that does not match or tombstone.
  }
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

static RawObject typeAtWithHash(RawType type, RawObject name, word hash) {
  RawObject result =
      lookupCell(MutableTuple::cast(type.attributes()), name, hash, false);
  if (result.isErrorNotFound()) return result;
  return ValueCell::cast(result).value();
}

static word internedStrHash(RawObject name) {
  if (name.isSmallStr()) {
    return SmallStr::cast(name).hash();
  }
  word hash = LargeStr::cast(name).header().hashCode();
  DCHECK(hash != Header::kUninitializedHash,
         "hash has not been computed (string not interned?)");
  return hash;
}

RawObject typeAt(const Type& type, const Object& name) {
  word hash = internedStrHash(*name);
  return typeAtWithHash(*type, *name, hash);
}

RawObject typeValueCellAt(const Type& type, const Object& name) {
  word hash = internedStrHash(*name);
  return typeValueCellAtWithHash(type, name, hash);
}

RawObject typeValueCellAtWithHash(const Type& type, const Object& name,
                                  word hash) {
  return lookupCell(MutableTuple::cast(type.attributes()), *name, hash, true);
}

static RawObject typeAtSetLocation(RawType type, RawObject name, word hash,
                                   Object* location) {
  RawObject result =
      lookupCell(MutableTuple::cast(type.attributes()), name, hash, false);
  if (result.isErrorNotFound()) return result;
  if (location != nullptr) {
    *location = result;
  }
  return ValueCell::cast(result).value();
}

RawObject typeAtById(Thread* thread, const Type& type, SymbolId id) {
  RawObject str = thread->runtime()->symbols()->at(id);
  word hash = internedStrHash(str);
  return typeAtWithHash(*type, str, hash);
}

RawObject typeAtPut(Thread* thread, const Type& type, const Object& name,
                    const Object& value) {
  DCHECK(thread->runtime()->isInternedStr(thread, name),
         "name should be an interned str");
  RawValueCell value_cell =
      ValueCell::cast(typeValueCellAtPut(thread, type, name));
  value_cell.setValue(*value);
  if (!value_cell.dependencyLink().isNoneType()) {
    HandleScope scope(thread);
    ValueCell value_cell_obj(&scope, value_cell);
    icInvalidateAttr(thread, type, name, value_cell_obj);
  }
  return value_cell;
}

RawObject typeAtPutById(Thread* thread, const Type& type, SymbolId id,
                        const Object& value) {
  HandleScope scope(thread);
  Object name(&scope, thread->runtime()->symbols()->at(id));
  return typeAtPut(thread, type, name, value);
}

static NEVER_INLINE void typeGrowAttributes(Thread* thread, const Type& type) {
  HandleScope scope(thread);
  Tuple old_data(&scope, type.attributes());

  // Count the number of filled buckets that are not tombstones.
  word old_capacity = old_data.length();
  word num_items = 0;
  for (word idx = 0; idx < old_capacity; idx += kBucketNumWords) {
    RawObject key = old_data.at(idx + kBucketKeyOffset);
    if (key != kEmptyKey && key != kTombstoneKey) {
      num_items++;
    }
  }

  // Grow if more than half of the buckets are filled, otherwise maintain size
  // and just clean out the tombstones.
  word old_num_buckets = old_capacity >> 1;
  word new_capacity = old_capacity;
  if (num_items > (old_num_buckets >> 1)) {
    // Grow if more than half of the buckets are filled, otherwise just clean
    // out the tombstones.
    new_capacity *= 2;
  }

  // Allocate new tuple and re-hash.
  MutableTuple new_data(&scope,
                        thread->runtime()->newMutableTuple(new_capacity));
  word num_buckets = new_capacity >> 1;
  DCHECK(Utils::isPowerOfTwo(num_buckets), "must be power of two");
  word new_remaining = (num_buckets * 2) / 3;
  word mask = num_buckets - 1;
  Object key(&scope, NoneType::object());
  for (word old_idx = 0; old_idx < old_capacity; old_idx += kBucketNumWords) {
    key = old_data.at(old_idx + kBucketKeyOffset);
    if (key == kEmptyKey || key == kTombstoneKey) {
      continue;
    }
    DCHECK(key.isStr(), "key must be None, _Unbound or str");
    word hash = internedStrHash(*key);
    word bucket = hash & mask;
    word num_probes = 0;
    while (new_data.at(bucket * kBucketNumWords + kBucketKeyOffset) !=
           kEmptyKey) {
      num_probes++;
      bucket = (bucket + num_probes) & mask;
    }
    new_data.atPut(bucket * kBucketNumWords + kBucketKeyOffset, *key);
    new_data.atPut(bucket * kBucketNumWords + kBucketValueOffset,
                   old_data.at(old_idx + kBucketValueOffset));
    new_remaining--;
  }
  DCHECK(new_remaining > 0, "must have remaining buckets");
  type.setAttributes(*new_data);
  type.setAttributesRemaining(new_remaining);
}

RawObject inline USED typeValueCellAtPut(Thread* thread, const Type& type,
                                         const Object& name) {
  HandleScope scope(thread);
  MutableTuple data_obj(&scope, type.attributes());
  RawMutableTuple data = *data_obj;
  word hash = internedStrHash(*name);
  word mask = (data.length() - 1) >> 1;
  for (word bucket = hash & mask, num_probes = 0, last_tombstone = -1;;
       bucket = (bucket + ++num_probes) & mask) {
    word idx = bucket * kBucketNumWords;
    RawObject key = data.at(idx + kBucketKeyOffset);
    if (key == *name) {
      return RawValueCell::cast(data.at(idx + kBucketValueOffset));
    }
    if (key == kEmptyKey) {
      DCHECK(Runtime::isInternedStr(thread, name), "expected interned str");
      RawValueCell cell = ValueCell::cast(thread->runtime()->newValueCell());
      cell.makePlaceholder();
      // newValueCell() may have triggered a GC; restore raw references.
      data = *data_obj;
      if (last_tombstone >= 0) {
        // Overwrite an existing tombstone entry.
        word last_tombstone_idx = last_tombstone * kBucketNumWords;
        data.atPut(last_tombstone_idx + kBucketKeyOffset, *name);
        data.atPut(last_tombstone_idx + kBucketValueOffset, cell);
      } else {
        // Use new bucket.
        data.atPut(idx + kBucketKeyOffset, *name);
        data.atPut(idx + kBucketValueOffset, cell);
        word remaining = type.attributesRemaining() - 1;
        type.setAttributesRemaining(remaining);
        if (remaining == 0) {
          ValueCell cell_obj(&scope, cell);
          typeGrowAttributes(thread, type);
          return *cell_obj;
        }
      }
      return cell;
    }
    if (key == kTombstoneKey) {
      last_tombstone = bucket;
    }
  }
}

RawObject typeLookupInMroSetLocation(Thread* thread, const Type& type,
                                     const Object& name, Object* location) {
  RawTuple mro = Tuple::cast(type.mro());
  RawObject name_raw = *name;
  word hash = internedStrHash(name_raw);
  for (word i = 0; i < mro.length(); i++) {
    DCHECK(thread->runtime()->isInstanceOfType(mro.at(i)), "non-type in MRO");
    RawType mro_type = mro.at(i).rawCast<RawType>();
    RawObject result = typeAtSetLocation(mro_type, name_raw, hash, location);
    if (!result.isErrorNotFound()) {
      return result;
    }
  }
  return Error::notFound();
}

RawObject typeLookupInMro(Thread* thread, const Type& type,
                          const Object& name) {
  RawTuple mro = Tuple::cast(type.mro());
  RawObject name_raw = *name;
  word hash = internedStrHash(name_raw);
  for (word i = 0; i < mro.length(); i++) {
    DCHECK(thread->runtime()->isInstanceOfType(mro.at(i)), "non-type in MRO");
    RawType mro_type = mro.at(i).rawCast<RawType>();
    RawObject result = typeAtWithHash(mro_type, name_raw, hash);
    if (!result.isErrorNotFound()) {
      return result;
    }
  }
  return Error::notFound();
}

RawObject typeLookupInMroById(Thread* thread, const Type& type, SymbolId id) {
  HandleScope scope(thread);
  Object name(&scope, thread->runtime()->symbols()->at(id));
  return typeLookupInMro(thread, type, name);
}

RawObject typeRemove(Thread* thread, const Type& type, const Object& name) {
  HandleScope scope(thread);
  MutableTuple data(&scope, type.attributes());
  word hash = internedStrHash(*name);
  word mask = (data.length() - 1) >> 1;
  Object key(&scope, NoneType::object());
  for (word bucket = hash & mask, num_probes = 0;;
       bucket = (bucket + ++num_probes) & mask) {
    word idx = bucket * kBucketNumWords;
    key = data.at(idx + kBucketKeyOffset);
    if (key == name) {
      // Set to tombstone and invalidate caches.
      ValueCell value_cell(&scope, data.at(idx + kBucketValueOffset));
      icInvalidateAttr(thread, type, name, value_cell);
      DCHECK(
          data == type.attributes() && data.at(idx + kBucketKeyOffset) == name,
          "attributes changed?");
      data.atPut(idx + kBucketKeyOffset, kTombstoneKey);
      data.atPut(idx + kBucketValueOffset, NoneType::object());
      return *value_cell;
    }
    if (key.isNoneType()) {
      return Error::notFound();
    }
    // Remaining cases are either a key that does not match or tombstone.
  }
}

RawObject typeKeys(Thread* thread, const Type& type) {
  HandleScope scope(thread);
  MutableTuple data(&scope, type.attributes());
  Runtime* runtime = thread->runtime();
  List keys(&scope, runtime->newList());
  Object key(&scope, NoneType::object());
  Object cell(&scope, NoneType::object());
  for (word i = 0, length = data.length(); i < length; i += kBucketNumWords) {
    key = data.at(i + kBucketKeyOffset);
    if (key == kEmptyKey || key == kTombstoneKey) {
      continue;
    }
    DCHECK(key.isStr(), "key must be a str");
    cell = data.at(i + kBucketValueOffset);
    if (ValueCell::cast(*cell).isPlaceholder()) {
      continue;
    }
    runtime->listAdd(thread, keys, key);
  }
  return *keys;
}

RawObject typeLen(Thread* thread, const Type& type) {
  HandleScope scope(thread);
  MutableTuple data(&scope, type.attributes());
  Object key(&scope, NoneType::object());
  Object cell(&scope, NoneType::object());
  word count = 0;
  for (word i = 0, length = data.length(); i < length; i += kBucketNumWords) {
    key = data.at(i + kBucketKeyOffset);
    if (key == kEmptyKey || key == kTombstoneKey) {
      continue;
    }
    DCHECK(key.isStr(), "key must be a str");
    cell = data.at(i + kBucketValueOffset);
    if (ValueCell::cast(*cell).isPlaceholder()) {
      continue;
    }
    count++;
  }
  return SmallInt::fromWord(count);
}

RawObject typeValues(Thread* thread, const Type& type) {
  HandleScope scope(thread);
  MutableTuple data(&scope, type.attributes());
  Runtime* runtime = thread->runtime();
  List values(&scope, runtime->newList());
  Object key(&scope, NoneType::object());
  Object value(&scope, NoneType::object());
  for (word i = 0, length = data.length(); i < length; i += kBucketNumWords) {
    key = data.at(i + kBucketKeyOffset);
    if (key == kEmptyKey || key == kTombstoneKey) {
      continue;
    }
    DCHECK(key.isStr(), "key must be a str");
    value = data.at(i + kBucketValueOffset);
    if (ValueCell::cast(*value).isPlaceholder()) {
      continue;
    }
    value = ValueCell::cast(*value).value();
    runtime->listAdd(thread, values, value);
  }
  return *values;
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
  Object meta_attr(&scope, typeLookupInMro(thread, meta_type, name));
  if (!meta_attr.isError()) {
    // TODO(T56002494): Remove this once type.__getattribute__ gets cached.
    if (meta_attr.isProperty()) {
      Object getter(&scope, Property::cast(*meta_attr).getter());
      if (!getter.isNoneType()) {
        return Interpreter::callFunction1(thread, thread->currentFrame(),
                                          getter, type);
      }
    }
    Type meta_attr_type(&scope, runtime->typeOf(*meta_attr));
    if (typeIsDataDescriptor(thread, meta_attr_type)) {
      return Interpreter::callDescriptorGet(thread, thread->currentFrame(),
                                            meta_attr, type, meta_type);
    }
  }

  // No data descriptor found on the meta class, look in the mro of the type
  Object attr(&scope,
              typeLookupInMroSetLocation(thread, type, name, location_out));
  if (!attr.isError()) {
    // TODO(T56002494): Remove this once type.__getattribute__ gets cached.
    if (attr.isFunction()) {
      // We always return the function object itself instead of a BoundMethod
      // due to the exception made below and another exception for NoneType in
      // function.__get__.
      return *attr;
    }
    Type attr_type(&scope, runtime->typeOf(*attr));
    if (typeIsNonDataDescriptor(thread, attr_type)) {
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
      return Interpreter::callDescriptorGet(thread, thread->currentFrame(),
                                            attr, none, type);
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
  Object none(&scope, NoneType::object());
  Object value(&scope, runtime->newWeakRef(thread, type, none));
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

static RawObject computeFixedAttributeBase(Thread* thread, const Tuple& bases);

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
  return computeFixedAttributeBase(thread, bases);
}

// Returns the most generic base among `bases` that captures inherited
// attributes with a fixed offset (either from __slots__ or builtin types)
// Note that this simulates `best_base` from CPython's typeobject.c.
static RawObject computeFixedAttributeBase(Thread* thread, const Tuple& bases) {
  HandleScope scope(thread);
  Type base0(&scope, bases.at(0));
  word length = bases.length();
  Type result(&scope, fixedAttributeBaseOfType(thread, base0));
  if (length == 1) return *result;

  for (word i = 1; i < length; i++) {
    Type base(&scope, bases.at(i));
    Type fixed_attr_base(&scope, fixedAttributeBaseOfType(thread, base));
    if (result == fixed_attr_base || typeIsSubclass(result, fixed_attr_base)) {
      continue;
    }
    if (typeIsSubclass(fixed_attr_base, result)) {
      result = *fixed_attr_base;
    } else {
      return thread->raiseWithFmt(
          LayoutId::kTypeError,
          "multiple bases have instance lay-out conflict");
    }
  }
  return *result;
}

static RawObject validateSlots(Thread* thread, const Type& type,
                               const Tuple& slots,
                               LayoutId fixed_attr_base_layout_id,
                               bool* should_add_dunder_dict) {
  HandleScope scope(thread);
  word slots_len = slots.length();
  Runtime* runtime = thread->runtime();
  Str dunder_dict(&scope, runtime->symbols()->at(ID(__dict__)));
  *should_add_dunder_dict = false;
  List result(&scope, runtime->newList());
  Object slot_obj(&scope, NoneType::object());
  Str slot_str(&scope, Str::empty());
  Layout base_layout(&scope, runtime->layoutAt(fixed_attr_base_layout_id));
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
      if (*should_add_dunder_dict) {
        return thread->raiseWithFmt(
            LayoutId::kTypeError,
            "__dict__ slot disallowed: we already got one");
      }
      *should_add_dunder_dict = true;
      continue;
    }
    if (!typeAt(type, slot_str).isErrorNotFound()) {
      return thread->raiseWithFmt(
          LayoutId::kValueError,
          "'%S' in __slots__ conflicts with class variable", &slot_str);
    }
    AttributeInfo ignored;
    // Filter out attribute already defined by `fixed_attr_base`.
    if (!Runtime::layoutFindAttribute(*base_layout, slot_str, &ignored)) {
      runtime->listAdd(thread, result, slot_str);
    }
  }
  listSort(thread, result);
  return *result;
}

RawObject typeInit(Thread* thread, const Type& type, const Str& name,
                   const Dict& dict, const Tuple& mro) {
  type.setName(*name);
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  if (mro.isTuple()) {
    type.setMro(*mro);
  } else {
    Tuple mro_copy(&scope, runtime->tupleSubseq(thread, mro, 0, mro.length()));
    type.setMro(*mro_copy);
  }

  Object result(&scope, typeAssignFromDict(thread, type, dict));
  if (result.isErrorException()) return *result;

  Object class_cell(&scope, typeAtById(thread, type, ID(__classcell__)));
  if (!class_cell.isErrorNotFound()) {
    DCHECK(class_cell.isCell(), "class cell must be a cell");
    Cell::cast(*class_cell).setValue(*type);
    Object class_cell_name(&scope, runtime->symbols()->at(ID(__classcell__)));
    typeRemove(thread, type, class_cell_name);
  }
  // TODO(T53997177): Centralize type initialization
  typeAddDocstring(thread, type);

  Tuple bases(&scope, type.bases());
  Object fixed_attr_base_obj(&scope, computeFixedAttributeBase(thread, bases));
  if (fixed_attr_base_obj.isErrorException()) {
    return *fixed_attr_base_obj;
  }
  Type fixed_attr_base_type(&scope, *fixed_attr_base_obj);
  LayoutId fixed_attr_base =
      Layout::cast(fixed_attr_base_type.instanceLayout()).id();

  // Analyze bases: Merge flags; add to subclasses list.
  word flags = static_cast<word>(type.flags());
  Type base_type(&scope, *type);
  for (word i = 0; i < bases.length(); i++) {
    base_type = bases.at(i);
    flags |= base_type.flags();
    addSubclass(thread, base_type, type);
  }
  flags &= ~Type::Flag::kIsAbstract;
  // TODO(T66646764): This is a hack to make `type` look finalized. Remove this.
  type.setFlags(static_cast<Type::Flag>(flags));

  // Add NativeProxy's attributes only if fixed_attr_base is not added with
  // NativeProxy's attributes.
  bool should_add_native_proxy_attributes = false;
  if (flags & Type::Flag::kIsNativeProxy) {
    if (addInheritedSlots(type).isError()) {
      return Error::exception();
    }
    if (!fixed_attr_base_type.hasFlag(Type::Flag::kIsNativeProxy)) {
      should_add_native_proxy_attributes = true;
      DCHECK(fixed_attr_base == LayoutId::kObject,
             "A NativeProxy is not compatible with builtin type layouts");
    }
  }
  // TODO(T53800222): We may need a better signal than is/is not a builtin
  // class.
  bool should_add_dunder_dict = !(flags & Type::Flag::kIsNativeProxy);

  Layout layout(&scope, runtime->layoutAt(LayoutId::kNoneType));
  Object dunder_slots_obj(&scope, typeAtById(thread, type, ID(__slots__)));
  bool has_non_empty_dunder_slots = false;
  if (dunder_slots_obj.isErrorNotFound()) {
    layout = runtime->computeInitialLayout(thread, type, fixed_attr_base);
    if (should_add_native_proxy_attributes) {
      layout = runtime->createNativeProxyLayout(thread, layout);
    }
  } else {
    // NOTE: CPython raises an exception when slots are given to a subtype of a
    // type with type.tp_itemsize != 0, which means having a variable length.
    // For example, __slots__ in int's subtype or str's type is disallowed.
    // This behavior is ignored in Pyro since all objects' size in RawObject is
    // fixed in Pyro.
    if (runtime->isInstanceOfStr(*dunder_slots_obj)) {
      Tuple slots_tuple(&scope, runtime->newTuple(1));
      slots_tuple.atPut(0, *dunder_slots_obj);
      dunder_slots_obj = *slots_tuple;
    } else if (!runtime->isInstanceOfTuple(*dunder_slots_obj)) {
      Type tuple_type(&scope, runtime->typeAt(LayoutId::kTuple));
      dunder_slots_obj = Interpreter::callFunction1(
          thread, thread->currentFrame(), tuple_type, dunder_slots_obj);
      if (dunder_slots_obj.isErrorException()) {
        return *dunder_slots_obj;
      }
      DCHECK(dunder_slots_obj.isTuple(), "tuple is expected");
    }
    Tuple slots_tuple(&scope, *dunder_slots_obj);
    Object sorted_dunder_slots_obj(
        &scope, validateSlots(thread, type, slots_tuple, fixed_attr_base,
                              &should_add_dunder_dict));
    if (sorted_dunder_slots_obj.isErrorException()) {
      return *sorted_dunder_slots_obj;
    }
    List slots(&scope, *sorted_dunder_slots_obj);
    if (slots.numItems() > 0) {
      if (should_add_native_proxy_attributes) {
        // If NativeProxy is used with __slots__, we should start the layout
        // with NativeProxy first.
        Layout initial_fixed_attr_based_on_object(
            &scope,
            runtime->computeInitialLayout(thread, type, LayoutId::kObject));
        Layout native_proxy_layout(
            &scope, runtime->createNativeProxyLayout(
                        thread, initial_fixed_attr_based_on_object));
        // This layout contains NativeProxy's attributes at the beginning, so
        // that we can add __slots__ attributes after them.
        fixed_attr_base = native_proxy_layout.id();
      }
      // Create a new layout with in-object attributes for __slots__ based off
      // fixed_attr_base.
      layout = runtime->computeInitialLayoutWithSlotAttributes(
          thread, type, fixed_attr_base, slots);
      // Add descriptors that mediate access to __slots__ attributes.
      Object slot_descriptor(&scope, NoneType::object());
      Object slot_name(&scope, NoneType::object());
      for (word i = 0; i < slots.numItems(); i++) {
        slot_name = slots.at(i);
        AttributeInfo info;
        CHECK(Runtime::layoutFindAttribute(*layout, slot_name, &info),
              "expected to find the slot attribute");
        DCHECK(
            info.isInObject() && info.isFixedOffset(),
            "slot attributes are expected to be in object with a fixed offset");
        slot_descriptor =
            runtime->newSlotDescriptor(type, slot_name, info.offset());
        typeAtPut(thread, type, slot_name, slot_descriptor);
      }
    } else {
      layout = runtime->computeInitialLayout(thread, type, fixed_attr_base);
      if (should_add_native_proxy_attributes) {
        layout = runtime->createNativeProxyLayout(thread, layout);
      }
    }
    if (!should_add_dunder_dict && !(flags & Type::Flag::kHasDunderDict)) {
      // Seal the type when bases do not have __dict__, and __dict_ does not
      // appear in __slots__.
      layout.setOverflowAttributes(NoneType::object());
      DCHECK(layout.isSealed(), "layout.isSealed() is expected");
    }
    has_non_empty_dunder_slots = slots.numItems() > 0;
  }

  // Initialize instance layout.
  layout.setDescribedType(*type);
  type.setInstanceLayout(*layout);

  if (should_add_dunder_dict) {
    flags |= Type::Flag::kHasDunderDict;
  }

  if (has_non_empty_dunder_slots) {
    flags |= Type::Flag::kHasSlots;
    flags |= Type::Flag::kIsFixedAttributeBase;
  }

  if (!has_non_empty_dunder_slots) {
    flags &= ~Type::Flag::kIsFixedAttributeBase;
  }

  LayoutId builtin_base = fixed_attr_base_type.builtinBase();
  if (builtin_base == LayoutId::kError) {
    return Error::exception();
  }
  type.setFlagsAndBuiltinBase(static_cast<Type::Flag>(flags), builtin_base);

  if (type.hasFlag(Type::Flag::kHasDunderDict) &&
      typeLookupInMroById(thread, type, ID(__dict__)).isErrorNotFound()) {
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
  if (!should_add_dunder_dict) {
    Str dunder_dict(&scope, runtime->symbols()->at(ID(__dict__)));
    typeRemove(thread, type, dunder_dict);
  }

  // Special-case __init_subclass__ to be a classmethod
  Object init_subclass(&scope, typeAtById(thread, type, ID(__init_subclass__)));
  if (init_subclass.isFunction()) {
    ClassMethod init_subclass_method(&scope, runtime->newClassMethod());
    init_subclass_method.setFunction(*init_subclass);
    typeAtPutById(thread, type, ID(__init_subclass__), init_subclass_method);
  }

  // Ensure that __class_getitem__ is a classmethod.  For convenience, the user
  // is allowed to define __class_getitem__ as a function.  When that happens,
  // wrap the function in a classmethod.
  Object class_getitem(&scope, typeAtById(thread, type, ID(__class_getitem__)));
  if (class_getitem.isFunction()) {
    ClassMethod class_getitem_method(&scope, runtime->newClassMethod());
    class_getitem_method.setFunction(*class_getitem);
    typeAtPutById(thread, type, ID(__class_getitem__), class_getitem_method);
  }

  Function type_dunder_call(&scope,
                            runtime->lookupNameInModule(thread, ID(_builtins),
                                                        ID(_type_dunder_call)));
  type.setCtor(*type_dunder_call);
  return *type;
}

void typeInitAttributes(Thread* thread, const Type& type) {
  type.setAttributes(thread->runtime()->newMutableTuple(kInitialCapacity));
  word num_buckets = kInitialCapacity >> 1;
  type.setAttributesRemaining((num_buckets * 2) / 3);
}

RawObject typeNew(Thread* thread, LayoutId metaclass_id, const Str& name,
                  const Tuple& bases, const Dict& dict, Type::Flag flags) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Type type(&scope, runtime->newTypeWithMetaclass(metaclass_id));
  type.setName(*name);
  type.setBases(*bases);
  Object mro_obj(&scope, computeMro(thread, type));
  if (mro_obj.isError()) return *mro_obj;
  Tuple mro(&scope, *mro_obj);
  type.setFlags(flags);
  return typeInit(thread, type, name, dict, mro);
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
  word hash = internedStrHash(*attr_name);
  RawObject existing_attr =
      lookupCell(MutableTuple::cast(type.attributes()), *attr_name, hash,
                 /*return_placeholder=*/true);
  if (!existing_attr.isValueCell()) {
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
  if (type.isBuiltin()) {
    Object type_name(&scope, type.name());
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "can't set attributes of built-in/extension type '%S'", &type_name);
  }

  // Check for a data descriptor
  Type metatype(&scope, runtime->typeOf(*type));
  Object meta_attr(&scope, typeLookupInMro(thread, metatype, name));
  if (!meta_attr.isError()) {
    Type meta_attr_type(&scope, runtime->typeOf(*meta_attr));
    if (typeIsDataDescriptor(thread, meta_attr_type)) {
      Object set_result(
          &scope, Interpreter::callDescriptorSet(thread, thread->currentFrame(),
                                                 meta_attr, type, value));
      if (set_result.isError()) return *set_result;
      return NoneType::object();
    }
  }

  // No data descriptor found, store the attribute in the type dict
  typeAtPut(thread, type, name, value);
  return NoneType::object();
}

static const BuiltinAttribute kTypeAttributes[] = {
    {ID(__doc__), RawType::kDocOffset},
    {ID(_type__flags), RawType::kFlagsOffset, AttributeFlags::kHidden},
    {ID(__mro__), RawType::kMroOffset, AttributeFlags::kReadOnly},
    {ID(__name__), RawType::kNameOffset},
    {ID(_type__abstract_methods), RawType::kAbstractMethodsOffset,
     AttributeFlags::kHidden},
    {ID(_type__attributes), RawType::kAttributesOffset,
     AttributeFlags::kHidden},
    {ID(_type__attributes_remaining), RawType::kAttributesRemainingOffset,
     AttributeFlags::kHidden},
    {ID(_type__bases), RawType::kBasesOffset, AttributeFlags::kHidden},
    {ID(_type__instance_layout), RawType::kInstanceLayoutOffset,
     AttributeFlags::kHidden},
    {ID(_type__proxy), RawType::kProxyOffset, AttributeFlags::kHidden},
    {ID(_type__slots), RawType::kSlotsOffset, AttributeFlags::kHidden},
    {ID(_type__subclasses), RawType::kSubclassesOffset,
     AttributeFlags::kHidden},
};

void initializeTypeTypes(Thread* thread) {
  HandleScope scope(thread);
  Type type(&scope, addBuiltinType(thread, ID(type), LayoutId::kType,
                                   /*superclass_id=*/LayoutId::kObject,
                                   kTypeAttributes));
  word flags = static_cast<word>(type.flags());
  flags |= RawType::Flag::kSealSubtypeLayouts;
  type.setFlags(static_cast<Type::Flag>(flags));

  addBuiltinType(thread, ID(type_proxy), LayoutId::kTypeProxy,
                 /*superclass_id=*/LayoutId::kObject, {nullptr, 0});
}

RawObject METH(type, __base__)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Runtime* runtime = thread->runtime();
  Type self(&scope, args.get(0));
  if (self == runtime->typeAt(LayoutId::kObject)) {
    return NoneType::object();
  }
  Tuple bases(&scope, self.bases());
  word num_bases = bases.length();
  if (num_bases == 0) {
    // self is the 'object' type
    return NoneType::object();
  }
  Type current_base(&scope, bases.at(0));
  Type candidate(&scope, runtime->typeAt(current_base.builtinBase()));
  Type winner(&scope, *candidate);
  Type best_base(&scope, *current_base);
  for (word i = 1; i < num_bases; i++) {
    current_base = bases.at(i);
    candidate = runtime->typeAt(current_base.builtinBase());
    if (typeIsSubclass(winner, candidate)) {
      continue;
    }
    if (typeIsSubclass(candidate, winner)) {
      winner = *candidate;
      best_base = *current_base;
    } else {
      return thread->raiseWithFmt(
          LayoutId::kTypeError,
          "multiple bases have instance lay-out conflict");
    }
  }
  return *best_base;
}

RawObject METH(type, __getattribute__)(Thread* thread, Frame* frame,
                                       word nargs) {
  Arguments args(frame, nargs);
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

RawObject METH(type, __setattr__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfType(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(type));
  }
  Type self(&scope, *self_obj);
  if (self.isBuiltin()) {
    Str type_name(&scope, self.name());
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "can't set attributes of built-in/extension type '%S'", &type_name);
  }
  Object name(&scope, args.get(1));
  name = attributeName(thread, name);
  if (name.isErrorException()) return *name;
  Object value(&scope, args.get(2));
  return typeSetAttr(thread, self, name, value);
}

RawObject METH(type, __subclasses__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
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

RawObject METH(type, mro)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfType(*self)) {
    return thread->raiseRequiresType(self, ID(type));
  }
  Type type(&scope, *self);
  Object maybe_mro(&scope, computeMro(thread, type));
  if (maybe_mro.isError()) {
    return *maybe_mro;
  }
  List result(&scope, runtime->newList());
  Tuple mro(&scope, *maybe_mro);
  listExtend(thread, result, mro, mro.length());
  return *result;
}

}  // namespace py
