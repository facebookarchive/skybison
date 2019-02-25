#include "type-builtins.h"

#include "frame.h"
#include "globals.h"
#include "mro.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"
#include "trampolines-inl.h"

namespace python {

RawObject typeNew(Thread* thread, LayoutId metaclass_id, const Str& name,
                  const Tuple& bases, const Dict& dict) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Type type(&scope, runtime->newTypeWithMetaclass(metaclass_id));
  type.setName(*name);

  // Compute MRO
  Object maybe_mro(&scope, computeMro(thread, type, bases));
  if (maybe_mro.isError()) {
    return *maybe_mro;
  }
  type.setMro(*maybe_mro);

  // Initialize dict
  Object class_cell_key(&scope, runtime->symbols()->DunderClassCell());
  Object class_cell(&scope, runtime->dictAt(dict, class_cell_key));
  if (!class_cell.isError()) {
    RawValueCell::cast(RawValueCell::cast(*class_cell)->value())
        ->setValue(*type);
    runtime->dictRemove(dict, class_cell_key);
  }
  type.setDict(*dict);

  // Compute builtin base class
  Object builtin_base(&scope, runtime->computeBuiltinBase(thread, type));
  if (builtin_base.isError()) {
    return *builtin_base;
  }
  Type builtin_base_type(&scope, *builtin_base);
  LayoutId base_layout_id =
      RawLayout::cast(builtin_base_type.instanceLayout())->id();

  // Initialize instance layout
  Layout layout(&scope,
                runtime->computeInitialLayout(thread, type, base_layout_id));
  layout.setDescribedType(*type);
  type.setInstanceLayout(*layout);

  // Copy down class flags from bases
  Tuple mro(&scope, *maybe_mro);
  word flags = 0;
  for (word i = 1; i < mro.length(); i++) {
    Type cur(&scope, mro.at(i));
    flags |= cur.flags();
  }
  type.setFlagsAndBuiltinBase(static_cast<RawType::Flag>(flags),
                              base_layout_id);

  return *type;
}

const BuiltinAttribute TypeBuiltins::kAttributes[] = {
    {SymbolId::kDunderMro, RawType::kMroOffset},
    {SymbolId::kDunderName, RawType::kNameOffset},
    {SymbolId::kDunderFlags, RawType::kFlagsOffset},
    {SymbolId::kDunderDict, RawType::kDictOffset},
    {SymbolId::kSentinelId, -1},
};

const NativeMethod TypeBuiltins::kNativeMethods[] = {
    {SymbolId::kDunderRepr, nativeTrampoline<dunderRepr>},
    {SymbolId::kSentinelId, nullptr},
};

const BuiltinMethod TypeBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderNew, dunderNew},
    {SymbolId::kSentinelId, nullptr},
};

void TypeBuiltins::postInitialize(Runtime* /* runtime */,
                                  const Type& new_type) {
  HandleScope scope;
  Layout layout(&scope, new_type.instanceLayout());
  layout.setOverflowAttributes(SmallInt::fromWord(RawType::kDictOffset));
}

RawObject TypeBuiltins::dunderNew(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Type metaclass(&scope, args.get(0));
  LayoutId metaclass_id = RawLayout::cast(metaclass.instanceLayout())->id();
  // If the first argument is exactly type, and there are no other arguments,
  // then this call acts like a "typeof" operator, and returns the type of the
  // argument.
  if (args.get(2).isUnboundValue() && args.get(3).isUnboundValue() &&
      metaclass_id == LayoutId::kType) {
    Object arg(&scope, args.get(1));
    // TODO(dulinr): In the future, types that should be visible only to the
    // runtime should be shown here, and things like SmallInt should return Int
    // instead.
    return runtime->typeOf(*arg);
  }
  Str name(&scope, args.get(1));
  Tuple bases(&scope, args.get(2));
  Dict dict(&scope, args.get(3));
  return typeNew(thread, metaclass_id, name, bases, dict);
}

RawObject TypeBuiltins::dunderRepr(Thread* thread, Frame* frame, word nargs) {
  if (nargs == 0) {
    return thread->raiseTypeErrorWithCStr(
        "type.__repr__(): Need a self argument");
  }
  if (nargs > 1) {
    return thread->raiseTypeError(thread->runtime()->newStrFromFormat(
        "expected 0 arguments, got %ld", nargs - 1));
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfType(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "type.__repr__() requires a 'type' object");
  }

  Type type(&scope, *self);
  Str type_name(&scope, type.name());
  // Make a buffer large enough to store the formatted string.
  const char prefix[] = "<class '";
  const char suffix[] = "'>";
  const word len = sizeof(prefix) - 1 + type_name.length() + sizeof(suffix);
  char* buf = new char[len];
  char* ptr = buf;
  std::copy(prefix, prefix + sizeof(prefix), ptr);
  ptr += sizeof(prefix) - 1;
  type_name.copyTo(reinterpret_cast<byte*>(ptr), type_name.length());
  ptr += type_name.length();
  std::copy(suffix, suffix + sizeof(suffix), ptr);
  ptr += sizeof(suffix) - 1;
  DCHECK(ptr == buf + len - 1, "Didn't write as many as expected");
  *ptr = '\0';
  // TODO(T32810595): Handle modules, qualname
  Str result(&scope, thread->runtime()->newStrFromCStr(buf));
  delete[] buf;
  return *result;
}

}  // namespace python
