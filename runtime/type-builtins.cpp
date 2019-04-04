#include "type-builtins.h"

#include "bytecode.h"
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
    RawValueCell::cast(RawValueCell::cast(*class_cell).value()).setValue(*type);
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
      RawLayout::cast(builtin_base_type.instanceLayout()).id();

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
  type.setBases(*bases);
  return *type;
}

const BuiltinAttribute TypeBuiltins::kAttributes[] = {
    {SymbolId::kDunderBases, RawType::kBasesOffset, AttributeFlags::kReadOnly},
    {SymbolId::kDunderDict, RawType::kDictOffset, AttributeFlags::kReadOnly},
    {SymbolId::kDunderFlags, RawType::kFlagsOffset, AttributeFlags::kReadOnly},
    {SymbolId::kDunderMro, RawType::kMroOffset, AttributeFlags::kReadOnly},
    {SymbolId::kDunderName, RawType::kNameOffset},
    {SymbolId::kSentinelId, -1},
};

const BuiltinMethod TypeBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderCall, dunderCall},
    {SymbolId::kDunderNew, dunderNew},
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
  Object metaclass_obj(&scope, args.get(0));
  Tuple pargs(&scope, args.get(1));
  Dict kwargs(&scope, args.get(2));
  // Shortcut for type(x) calls.
  if (pargs.length() == 1 && kwargs.numItems() == 0 &&
      metaclass_obj == runtime->typeAt(LayoutId::kType)) {
    return runtime->typeOf(pargs.at(0));
  }

  if (!runtime->isInstanceOfType(*metaclass_obj)) {
    return thread->raiseTypeErrorWithCStr("self must be a type instance");
  }
  Type metaclass(&scope, *metaclass_obj);

  Object dunder_new(
      &scope, runtime->attributeAtId(thread, metaclass, SymbolId::kDunderNew));
  CHECK(!dunder_new.isError(), "metaclass must have __new__");
  frame->pushValue(*dunder_new);
  Tuple call_args(&scope, runtime->newTuple(pargs.length() + 1));
  call_args.atPut(0, *metaclass);
  for (word i = 0, length = pargs.length(); i < length; i++) {
    call_args.atPut(i + 1, pargs.at(i));
  }
  frame->pushValue(*call_args);
  frame->pushValue(*kwargs);
  Object instance(&scope, Interpreter::callEx(
                              thread, frame, CallFunctionExFlag::VAR_KEYWORDS));
  if (instance.isError()) return *instance;
  if (!runtime->isInstance(instance, metaclass)) {
    return *instance;
  }

  Object dunder_init(
      &scope, runtime->attributeAtId(thread, metaclass, SymbolId::kDunderInit));
  CHECK(!dunder_init.isError(), "metaclass must have __init__");
  frame->pushValue(*dunder_init);
  call_args.atPut(0, *instance);
  frame->pushValue(*call_args);
  frame->pushValue(*kwargs);
  Object result(&scope, Interpreter::callEx(thread, frame,
                                            CallFunctionExFlag::VAR_KEYWORDS));
  if (result.isError()) return *result;
  if (!result.isNoneType()) {
    Object type_name(&scope, metaclass.name());
    return thread->raiseTypeError(
        runtime->newStrFromFmt("%S.__init__ returned non None", &type_name));
  }
  return *instance;
}

RawObject TypeBuiltins::dunderNew(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Type metaclass(&scope, args.get(0));
  LayoutId metaclass_id = RawLayout::cast(metaclass.instanceLayout()).id();
  // If the first argument is exactly type, and there are no other arguments,
  // then this call acts like a "typeof" operator, and returns the type of the
  // argument.
  if (args.get(2).isUnbound() && args.get(3).isUnbound() &&
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

}  // namespace python
