#include "type-builtins.h"

#include "bytecode.h"
#include "dict-builtins.h"
#include "frame.h"
#include "globals.h"
#include "mro.h"
#include "object-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"
#include "tuple-builtins.h"

namespace python {

bool nextTypeDictItem(RawTuple data, word* idx) {
  // Iterate through until we find a non-placeholder item.
  while (Dict::Bucket::nextItem(data, idx)) {
    if (!ValueCell::cast(Dict::Bucket::value(data, *idx)).isPlaceholder()) {
      // At this point, we have found a valid index in the buckets.
      return true;
    }
  }
  return false;
}

RawObject typeLookupNameInMro(Thread* thread, const Type& type,
                              const Object& name_str) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Tuple mro(&scope, type.mro());
  for (word i = 0; i < mro.length(); i++) {
    Type mro_type(&scope, mro.at(i));
    Dict dict(&scope, mro_type.dict());
    Object value(&scope, runtime->typeDictAt(thread, dict, name_str));
    if (!value.isError()) {
      return *value;
    }
  }
  return Error::notFound();
}

RawObject typeLookupSymbolInMro(Thread* thread, const Type& type,
                                SymbolId symbol) {
  HandleScope scope(thread);
  Object symbol_str(&scope, thread->runtime()->symbols()->at(symbol));
  return typeLookupNameInMro(thread, type, symbol_str);
}

bool typeIsDataDescriptor(Thread* thread, const Type& type) {
  // TODO(T25692962): Track "descriptorness" through a bit on the class
  HandleScope scope(thread);
  Object dunder_set_name(&scope, thread->runtime()->symbols()->DunderSet());
  return !typeLookupNameInMro(thread, type, dunder_set_name).isError();
}

bool typeIsNonDataDescriptor(Thread* thread, const Type& type) {
  // TODO(T25692962): Track "descriptorness" through a bit on the class
  HandleScope scope(thread);
  Object dunder_get_name(&scope, thread->runtime()->symbols()->DunderGet());
  return !typeLookupNameInMro(thread, type, dunder_get_name).isError();
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

RawObject typeAt(Thread* thread, const Type& type, const Object& key) {
  HandleScope scope(thread);
  Dict dict(&scope, type.dict());
  return thread->runtime()->typeDictAt(thread, dict, key);
}

RawObject typeKeys(Thread* thread, const Type& type) {
  HandleScope scope(thread);
  Dict dict(&scope, type.dict());
  Tuple data(&scope, dict.data());
  Runtime* runtime = thread->runtime();
  List keys(&scope, runtime->newList());
  Object key(&scope, NoneType::object());
  for (word i = Dict::Bucket::kFirst; nextTypeDictItem(*data, &i);) {
    key = Dict::Bucket::key(*data, i);
    runtime->listAdd(thread, keys, key);
  }
  return *keys;
}

RawObject typeLen(Thread* thread, const Type& type) {
  HandleScope scope(thread);
  Dict dict(&scope, type.dict());
  Tuple data(&scope, dict.data());
  word count = 0;
  for (word i = Dict::Bucket::kFirst; nextTypeDictItem(*data, &i);) {
    ++count;
  }
  return SmallInt::fromWord(count);
}

RawObject typeValues(Thread* thread, const Type& type) {
  HandleScope scope(thread);
  Dict dict(&scope, type.dict());
  Tuple data(&scope, dict.data());
  Runtime* runtime = thread->runtime();
  List values(&scope, runtime->newList());
  Object value(&scope, NoneType::object());
  for (word i = Dict::Bucket::kFirst; nextTypeDictItem(*data, &i);) {
    value = ValueCell::cast(Dict::Bucket::value(*data, i)).value();
    runtime->listAdd(thread, values, value);
  }
  return *values;
}

RawObject typeGetAttribute(Thread* thread, const Type& type,
                           const Object& name_str) {
  // Look for the attribute in the meta class
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Type meta_type(&scope, runtime->typeOf(*type));
  Object meta_attr(&scope, typeLookupNameInMro(thread, meta_type, name_str));
  if (!meta_attr.isError()) {
    Type meta_attr_type(&scope, runtime->typeOf(*meta_attr));
    if (typeIsDataDescriptor(thread, meta_attr_type)) {
      return Interpreter::callDescriptorGet(thread, thread->currentFrame(),
                                            meta_attr, type, meta_type);
    }
  }

  // No data descriptor found on the meta class, look in the mro of the type
  Object attr(&scope, typeLookupNameInMro(thread, type, name_str));
  if (!attr.isError()) {
    Type attr_type(&scope, runtime->typeOf(*attr));
    if (typeIsNonDataDescriptor(thread, attr_type)) {
      // Unfortunately calling `__get__` for a lookup on `type(None)` will look
      // exactly the same as calling it for a lookup on the `None` object.
      // To solve the ambiguity we add a special case for `type(None)` here.
      // Luckily it is impossible for the user to change the type so we can
      // special case the desired lookup behavior here.
      // Also see `FunctionBuiltins::dunderGet` for the related special casing
      // of lookups on the `None` object.
      if (type.builtinBase() == LayoutId::kNoneType) {
        return *attr;
      }
      Object none(&scope, NoneType::object());
      return Interpreter::callDescriptorGet(thread, thread->currentFrame(),
                                            attr, none, type);
    }
    return *attr;
  }

  // No data descriptor found on the meta class, look on the type
  Object result(&scope, instanceGetAttribute(thread, type, name_str));
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
    base.setSubclasses(runtime->newDict());
  }
  DCHECK(base.subclasses().isDict(), "direct subclasses must be dict");
  Dict subclasses(&scope, base.subclasses());
  LayoutId type_id = Layout::cast(type.instanceLayout()).id();
  Object key(&scope, SmallInt::fromWord(static_cast<word>(type_id)));
  Object none(&scope, NoneType::object());
  Object value(&scope, runtime->newWeakRef(thread, type, none));
  runtime->dictAtPut(thread, subclasses, key, value);
}

RawObject typeNew(Thread* thread, LayoutId metaclass_id, const Str& name,
                  const Tuple& bases, const Dict& dict) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Type type(&scope, runtime->newTypeWithMetaclass(metaclass_id));
  type.setName(*name);
  type.setBases(bases.length() > 0 ? *bases : runtime->implicitBases());

  // Compute MRO
  Object maybe_mro(&scope, Error::error());
  if (metaclass_id == LayoutId::kType) {
    // Fast path: call computeMro directly when metaclass is type
    maybe_mro = computeMro(thread, type, bases);
  } else {
    maybe_mro = thread->invokeMethodStatic1(metaclass_id, SymbolId::kMro, type);
    if (maybe_mro.isError()) {
      DCHECK(!maybe_mro.isErrorNotFound(), "all metaclasses must define mro()");
      return *maybe_mro;
    }
    maybe_mro = sequenceAsTuple(thread, maybe_mro);
  }
  if (maybe_mro.isError()) {
    return *maybe_mro;
  }
  type.setMro(*maybe_mro);

  // Copy dict to new type_dict and wrap values in ValueCells.
  Dict type_dict(&scope, runtime->newDictWithSize(dict.numItems()));
  {
    Tuple data(&scope, dict.data());
    Object value(&scope, NoneType::object());
    Object key(&scope, NoneType::object());
    for (word i = Dict::Bucket::kFirst; Dict::Bucket::nextItem(*data, &i);) {
      value = Dict::Bucket::value(*data, i);
      DCHECK(!(value.isValueCell() && ValueCell::cast(*value).isPlaceholder()),
             "value should not be a placeholder value cell");
      key = Dict::Bucket::key(*data, i);
      runtime->typeDictAtPut(thread, type_dict, key, value);
    }
  }

  Object class_cell_key(&scope, runtime->symbols()->DunderClassCell());
  Object class_cell(&scope,
                    runtime->typeDictAt(thread, type_dict, class_cell_key));
  if (!class_cell.isErrorNotFound()) {
    DCHECK(class_cell.isValueCell(), "class cell must be a value cell");
    ValueCell::cast(*class_cell).setValue(*type);
    runtime->dictRemove(thread, type_dict, class_cell_key);
  }
  type.setDict(*type_dict);

  // Compute builtin base class
  Object builtin_base(&scope, runtime->computeBuiltinBase(thread, type));
  if (builtin_base.isError()) {
    return *builtin_base;
  }
  Type builtin_base_type(&scope, *builtin_base);
  LayoutId base_layout_id =
      Layout::cast(builtin_base_type.instanceLayout()).id();

  // Initialize instance layout
  Layout layout(&scope,
                runtime->computeInitialLayout(thread, type, base_layout_id));
  layout.setDescribedType(*type);
  type.setInstanceLayout(*layout);

  // Add this type as a direct subclass of each of its bases
  Type base_type(&scope, *type);
  for (word i = 0; i < bases.length(); i++) {
    base_type = bases.at(i);
    addSubclass(thread, base_type, type);
  }

  // Copy down class flags from bases
  Tuple mro(&scope, *maybe_mro);
  word flags = 0;
  for (word i = 1; i < mro.length(); i++) {
    Type cur(&scope, mro.at(i));
    flags |= cur.flags();
  }
  type.setFlagsAndBuiltinBase(
      static_cast<Type::Flag>(flags & ~Type::Flag::kIsAbstract),
      base_layout_id);
  return *type;
}

// NOTE: Keep the order of these type attributes same as the one from
// rewriteOperation.
static const SymbolId kUnimplementedTypeAttrUpdates[] = {
    // BINARY_*
    SymbolId::kDunderAdd, SymbolId::kDunderRadd, SymbolId::kDunderAnd,
    SymbolId::kDunderRand, SymbolId::kDunderFloordiv,
    SymbolId::kDunderRfloordiv, SymbolId::kDunderLshift,
    SymbolId::kDunderRlshift, SymbolId::kDunderMatmul, SymbolId::kDunderRmatmul,
    SymbolId::kDunderMod, SymbolId::kDunderRmod, SymbolId::kDunderOr,
    SymbolId::kDunderRor, SymbolId::kDunderPow, SymbolId::kDunderRpow,
    SymbolId::kDunderRshift, SymbolId::kDunderGetitem, SymbolId::kDunderSub,
    SymbolId::kDunderRsub, SymbolId::kDunderTruediv, SymbolId::kDunderRtruediv,
    SymbolId::kDunderXor, SymbolId::kDunderRxor,
    // COMPARE_OP
    SymbolId::kDunderLt, SymbolId::kDunderLe, SymbolId::kDunderEq,
    SymbolId::kDunderNe, SymbolId::kDunderGt, SymbolId::kDunderGe,
    // FOR_ITER
    SymbolId::kDunderNext,
    // INPLACE_*
    SymbolId::kDunderIadd, SymbolId::kDunderIand, SymbolId::kDunderIfloordiv,
    SymbolId::kDunderIlshift, SymbolId::kDunderImatmul, SymbolId::kDunderImod,
    SymbolId::kDunderImul, SymbolId::kDunderIor, SymbolId::kDunderIpow,
    SymbolId::kDunderIrshift, SymbolId::kDunderIsub, SymbolId::kDunderItruediv,
    SymbolId::kDunderIxor,
    // LOAD_ATTR, LOAD_METHOD
    SymbolId::kDunderGetattribute,
    // STORE_ATTR
    SymbolId::kDunderSetattr};

void terminateIfUnimplementedTypeAttrCacheInvalidation(Thread* thread,
                                                       const Str& attr_name) {
  Runtime* runtime = thread->runtime();
  for (uword i = 0; i < ARRAYSIZE(kUnimplementedTypeAttrUpdates); ++i) {
    if (attr_name.equals(
            runtime->symbols()->at(kUnimplementedTypeAttrUpdates[i]))) {
      UNIMPLEMENTED("unimplemented cache invalidation for type.%s update",
                    attr_name.toCStr());
    }
  }
}

RawObject typeSetAttr(Thread* thread, const Type& type,
                      const Object& name_interned_str, const Object& value) {
  Runtime* runtime = thread->runtime();
  DCHECK(runtime->isInternedStr(thread, name_interned_str),
         "name must be an interned string");
  HandleScope scope(thread);
  if (type.isBuiltin()) {
    Object type_name(&scope, type.name());
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "can't set attributes of built-in/extension type '%S'", &type_name);
  }

  // Check for a data descriptor
  Type metatype(&scope, runtime->typeOf(*type));
  Object meta_attr(&scope,
                   typeLookupNameInMro(thread, metatype, name_interned_str));
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
  Dict type_dict(&scope, type.dict());
  runtime->typeDictAtPut(thread, type_dict, name_interned_str, value);
  return NoneType::object();
}

const BuiltinAttribute TypeBuiltins::kAttributes[] = {
    {SymbolId::kDunderDoc, RawType::kDocOffset},
    {SymbolId::kDunderFlags, RawType::kFlagsOffset, AttributeFlags::kReadOnly},
    {SymbolId::kDunderMro, RawType::kMroOffset, AttributeFlags::kReadOnly},
    {SymbolId::kDunderName, RawType::kNameOffset},
    {SymbolId::kInvalid, RawType::kDictOffset},
    {SymbolId::kInvalid, RawType::kProxyOffset},
    {SymbolId::kSentinelId, -1},
};

const BuiltinMethod TypeBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderCall, dunderCall},
    {SymbolId::kDunderGetattribute, dunderGetattribute},
    {SymbolId::kDunderNew, dunderNew},
    {SymbolId::kDunderSetattr, dunderSetattr},
    {SymbolId::kDunderSubclasses, dunderSubclasses},
    {SymbolId::kMro, mro},
    {SymbolId::kSentinelId, nullptr},
};

void TypeBuiltins::postInitialize(Runtime* /* runtime */,
                                  const Type& new_type) {
  HandleScope scope;
  Layout layout(&scope, new_type.instanceLayout());
  layout.setOverflowAttributes(SmallInt::fromWord(RawType::kDictOffset));
}

RawObject TypeBuiltins::dunderCall(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object self_obj(&scope, args.get(0));
  Tuple pargs(&scope, args.get(1));
  Dict kwargs(&scope, args.get(2));
  // Shortcut for type(x) calls.
  if (pargs.length() == 1 && kwargs.numItems() == 0 &&
      self_obj == runtime->typeAt(LayoutId::kType)) {
    return runtime->typeOf(pargs.at(0));
  }

  if (!runtime->isInstanceOfType(*self_obj)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "self must be a type instance");
  }
  Type self(&scope, *self_obj);

  Object dunder_new_name(&scope, runtime->symbols()->DunderNew());
  Object dunder_new(&scope, typeGetAttribute(thread, self, dunder_new_name));
  CHECK(!dunder_new.isError(), "self must have __new__");
  frame->pushValue(*dunder_new);
  Tuple call_args(&scope, runtime->newTuple(pargs.length() + 1));
  call_args.atPut(0, *self);
  call_args.replaceFromWith(1, *pargs, pargs.length());
  frame->pushValue(*call_args);
  frame->pushValue(*kwargs);
  Object instance(&scope, Interpreter::callEx(
                              thread, frame, CallFunctionExFlag::VAR_KEYWORDS));
  if (instance.isError()) return *instance;
  Type type(&scope, runtime->typeOf(*instance));
  if (!runtime->isSubclass(type, self)) {
    return *instance;
  }

  Object dunder_init_name(&scope, runtime->symbols()->DunderInit());
  Object dunder_init(&scope, typeGetAttribute(thread, self, dunder_init_name));
  CHECK(!dunder_init.isError(), "self must have __init__");
  frame->pushValue(*dunder_init);
  call_args.atPut(0, *instance);
  frame->pushValue(*call_args);
  frame->pushValue(*kwargs);
  Object result(&scope, Interpreter::callEx(thread, frame,
                                            CallFunctionExFlag::VAR_KEYWORDS));
  if (result.isError()) return *result;
  if (!result.isNoneType()) {
    Object type_name(&scope, self.name());
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "%S.__init__ returned non None", &type_name);
  }
  return *instance;
}

RawObject TypeBuiltins::dunderGetattribute(Thread* thread, Frame* frame,
                                           word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfType(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kType);
  }
  Type self(&scope, *self_obj);
  Object name(&scope, args.get(1));
  if (!runtime->isInstanceOfStr(*name)) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError, "attribute name must be string, not '%T'", &name);
  }
  Object result(&scope, typeGetAttribute(thread, self, name));
  if (result.isErrorNotFound()) {
    Object type_name(&scope, self.name());
    return thread->raiseWithFmt(LayoutId::kAttributeError,
                                "type object '%S' has no attribute '%S'",
                                &type_name, &name);
  }
  return *result;
}

RawObject TypeBuiltins::dunderNew(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Type metaclass(&scope, args.get(0));
  LayoutId metaclass_id = Layout::cast(metaclass.instanceLayout()).id();
  // If the first argument is exactly type, and there are no other arguments,
  // then this call acts like a "typeof" operator, and returns the type of the
  // argument.
  if (args.get(2).isUnbound() && args.get(3).isUnbound() &&
      metaclass_id == LayoutId::kType) {
    return thread->runtime()->typeOf(args.get(1));
  }
  Str name(&scope, args.get(1));
  Tuple bases(&scope, args.get(2));
  Dict dict(&scope, args.get(3));
  return typeNew(thread, metaclass_id, name, bases, dict);
}

RawObject TypeBuiltins::dunderSetattr(Thread* thread, Frame* frame,
                                      word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfType(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kType);
  }
  Type self(&scope, *self_obj);
  Object name(&scope, args.get(1));
  if (!runtime->isInstanceOfStr(*name)) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError, "attribute name must be string, not '%T'", &name);
  }
  if (!name.isStr()) {
    UNIMPLEMENTED("Strict subclass of string");
  }
  Str interned_name(&scope, runtime->internStr(thread, name));
  // Make sure cache invalidation is correctly done for this.
  if (runtime->isCacheEnabled()) {
    terminateIfUnimplementedTypeAttrCacheInvalidation(thread, interned_name);
  }
  Object value(&scope, args.get(2));
  return typeSetAttr(thread, self, interned_name, value);
}

RawObject TypeBuiltins::dunderSubclasses(Thread* thread, Frame* frame,
                                         word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfType(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kType);
  }
  Type self(&scope, *self_obj);
  Object subclasses_obj(&scope, self.subclasses());
  if (subclasses_obj.isNoneType()) {
    return runtime->newList();
  }
  Dict subclasses(&scope, *subclasses_obj);
  DictValueIterator iter(&scope,
                         runtime->newDictValueIterator(thread, subclasses));
  List result(&scope, runtime->newList());
  Object value(&scope, Unbound::object());
  while (!(value = dictValueIteratorNext(thread, iter)).isErrorNoMoreItems()) {
    DCHECK(value.isWeakRef(), "__subclasses__ element is not a WeakRef");
    WeakRef ref(&scope, *value);
    value = ref.referent();
    if (!value.isNoneType()) {
      runtime->listAdd(thread, result, value);
    }
  }
  return *result;
}

RawObject TypeBuiltins::mro(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfType(*self)) {
    return thread->raiseRequiresType(self, SymbolId::kType);
  }
  Type type(&scope, *self);
  Tuple parents(&scope, type.bases());
  Object mro(&scope, computeMro(thread, type, parents));
  if (mro.isError()) {
    return *mro;
  }
  List result(&scope, runtime->newList());
  result.setItems(*mro);
  result.setNumItems(Tuple::cast(*mro).length());
  return *result;
}

}  // namespace python
