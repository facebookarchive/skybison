#include "function-builtins.h"

#include "frame.h"
#include "globals.h"
#include "mro.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

RawObject builtinTypeCall(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);

  Runtime* runtime = thread->runtime();

  // First, call __new__ to allocate a new instance.
  if (!runtime->isInstanceOfType(args.get(0))) {
    return thread->raiseTypeErrorWithCStr(
        "'__new__' requires a 'class' object");
  }
  Type type(&scope, args.get(0));
  Object dunder_new(
      &scope, runtime->lookupSymbolInMro(thread, type, SymbolId::kDunderNew));

  frame->pushValue(*dunder_new);
  for (word i = 0; i < nargs; i++) {
    frame->pushValue(args.get(i));
  }

  Object result(&scope, Interpreter::call(thread, frame, nargs));

  // Second, call __init__ to initialize the instance.

  // top of the stack should be the new instance
  Object dunder_init(
      &scope, runtime->lookupSymbolInMro(thread, type, SymbolId::kDunderInit));

  frame->pushValue(*dunder_init);
  frame->pushValue(*result);
  for (word i = 1; i < nargs; i++) {
    frame->pushValue(args.get(i));
  }

  // TODO: throw a type error if the __init__ method does not return None.
  Interpreter::call(thread, frame, nargs);

  return *result;
}

RawObject builtinTypeCallKw(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Runtime* runtime = thread->runtime();

  // First, call __new__ to allocate a new instance.
  if (!runtime->isInstanceOfType(args.get(0))) {
    return thread->raiseTypeErrorWithCStr(
        "'__new__' requires a 'class' object");
  }
  Type type(&scope, args.get(0));
  Object dunder_new(
      &scope, runtime->lookupSymbolInMro(thread, type, SymbolId::kDunderNew));
  frame->pushValue(*dunder_new);
  // Copy down the args, kwargs, and kwarg tuple that __call__ was called with
  frame->pushLocals(nargs, 0);
  Object result(&scope, Interpreter::callKw(thread, frame, nargs - 1));

  // Second, call __init__ to initialize the instance.
  Object dunder_init(
      &scope, runtime->lookupSymbolInMro(thread, type, SymbolId::kDunderInit));
  frame->pushValue(*dunder_init);
  frame->pushValue(*result);
  // Copy down everything that __call_ was called with, except for the first
  // argument (the type)
  frame->pushLocals(nargs - 1, 1);
  // TODO: throw a type error if the __init__ method does not return None.
  Interpreter::callKw(thread, frame, nargs - 1);

  return *result;
}

RawObject builtinTypeNew(Thread* thread, Frame* frame, word nargs) {
  if (nargs < 2) {
    return thread->raiseTypeErrorWithCStr("type() takes 1 or 3 arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Type metatype(&scope, args.get(0));
  LayoutId class_layout_id = RawLayout::cast(metatype->instanceLayout())->id();
  // If the first argument is exactly type, and there are no other arguments,
  // then this call acts like a "typeof" operator, and returns the type of the
  // argument.
  if (nargs == 2 && class_layout_id == LayoutId::kType) {
    Object arg(&scope, args.get(1));
    // TODO(dulinr): In the future, types that should be visible only to the
    // runtime should be shown here, and things like SmallInt should return Int
    // instead.
    return runtime->typeOf(*arg);
  }
  Object name(&scope, args.get(1));
  Type result(&scope, runtime->newTypeWithMetaclass(class_layout_id));
  result->setName(*name);

  // Compute MRO
  Tuple parents(&scope, args.get(2));
  Object maybe_mro(&scope, computeMro(thread, result, parents));
  if (maybe_mro->isError()) {
    return *maybe_mro;
  }
  result->setMro(*maybe_mro);

  Dict dict(&scope, args.get(3));
  Object class_cell_key(&scope, runtime->symbols()->DunderClassCell());
  Object class_cell(&scope, runtime->dictAt(dict, class_cell_key));
  if (!class_cell->isError()) {
    RawValueCell::cast(RawValueCell::cast(*class_cell)->value())
        ->setValue(*result);
    runtime->dictRemove(dict, class_cell_key);
  }
  Object name_key(&scope, runtime->symbols()->DunderName());
  runtime->dictAtPutInValueCell(dict, name_key, name);

  result->setDict(*dict);

  // Compute builtin base class
  LayoutId base_layout_id = runtime->computeBuiltinBaseType(result);
  // Initialize instance layout
  Layout layout(&scope,
                runtime->computeInitialLayout(thread, result, base_layout_id));
  layout->setDescribedType(*result);
  result->setInstanceLayout(*layout);

  // Copy down class flags from bases
  Tuple mro(&scope, *maybe_mro);
  word flags = 0;
  for (word i = 1; i < mro->length(); i++) {
    Type cur(&scope, mro->at(i));
    flags |= RawSmallInt::cast(cur->flags())->value();
  }
  result->setFlags(SmallInt::fromWord(flags));

  return *result;
}

RawObject builtinTypeInit(Thread*, Frame*, word) { return NoneType::object(); }

RawObject builtinTypeRepr(Thread* thread, Frame* frame, word nargs) {
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
  if (!thread->runtime()->hasSubClassFlag(*self, Type::Flag::kTypeSubclass)) {
    return thread->raiseTypeErrorWithCStr(
        "type.__repr__() requires a 'type' object");
  }

  Type type(&scope, *self);
  Str type_name(&scope, type->name());
  // Make a buffer large enough to store the formatted string.
  const char prefix[] = "<class '";
  const char suffix[] = "'>";
  const word len = sizeof(prefix) - 1 + type_name->length() + sizeof(suffix);
  char* buf = new char[len];
  char* ptr = buf;
  std::copy(prefix, prefix + sizeof(prefix), ptr);
  ptr += sizeof(prefix) - 1;
  type_name->copyTo(reinterpret_cast<byte*>(ptr), type_name->length());
  ptr += type_name->length();
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
