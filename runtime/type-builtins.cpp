#include "function-builtins.h"

#include "frame.h"
#include "globals.h"
#include "mro.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

Object* builtinTypeCall(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);

  Runtime* runtime = thread->runtime();

  // First, call __new__ to allocate a new instance.
  if (!runtime->isInstanceOfClass(args.get(0))) {
    return thread->throwTypeErrorFromCString(
        "'__new__' requires a 'class' object");
  }
  Handle<Type> type(&scope, args.get(0));
  Handle<Object> dunder_new(
      &scope, runtime->lookupSymbolInMro(thread, type, SymbolId::kDunderNew));

  frame->pushValue(*dunder_new);
  for (word i = 0; i < nargs; i++) {
    frame->pushValue(args.get(i));
  }

  Handle<Object> result(&scope, Interpreter::call(thread, frame, nargs));

  // Second, call __init__ to initialize the instance.

  // top of the stack should be the new instance
  Handle<Object> dunder_init(
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

Object* builtinTypeNew(Thread* thread, Frame* frame, word nargs) {
  if (nargs < 2) {
    return thread->throwTypeErrorFromCString("type() takes 1 or 3 arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Handle<Type> metatype(&scope, args.get(0));
  LayoutId class_layout_id = Layout::cast(metatype->instanceLayout())->id();
  // If the first argument is exactly type, and there are no other arguments,
  // then this call acts like a "typeof" operator, and returns the type of the
  // argument.
  if (nargs == 2 && class_layout_id == LayoutId::kType) {
    Handle<Object> arg(&scope, args.get(1));
    // TODO(dulinr): In the future, types that should be visible only to the
    // runtime should be shown here, and things like SmallInt should return Int
    // instead.
    return runtime->typeOf(*arg);
  }
  Handle<Object> name(&scope, args.get(1));
  Handle<Type> result(&scope, runtime->newClassWithMetaclass(class_layout_id));
  result->setName(*name);

  // Compute MRO
  Handle<ObjectArray> parents(&scope, args.get(2));
  Handle<Object> maybe_mro(&scope, computeMro(thread, result, parents));
  if (maybe_mro->isError()) {
    return *maybe_mro;
  }
  result->setMro(*maybe_mro);

  Handle<Dict> dict(&scope, args.get(3));
  Handle<Object> class_cell_key(&scope, runtime->symbols()->DunderClassCell());
  Handle<Object> class_cell(&scope, runtime->dictAt(dict, class_cell_key));
  if (!class_cell->isError()) {
    ValueCell::cast(ValueCell::cast(*class_cell)->value())->setValue(*result);
    Object* tmp;
    runtime->dictRemove(dict, class_cell_key, &tmp);
  }
  Handle<Object> name_key(&scope, runtime->symbols()->DunderName());
  runtime->dictAtPutInValueCell(dict, name_key, name);

  result->setDict(*dict);

  // Compute builtin base class
  LayoutId base_layout_id = runtime->computeBuiltinBaseClass(result);
  // Initialize instance layout
  Handle<Layout> layout(
      &scope, runtime->computeInitialLayout(thread, result, base_layout_id));
  layout->setDescribedClass(*result);
  result->setInstanceLayout(*layout);

  // Copy down class flags from bases
  Handle<ObjectArray> mro(&scope, *maybe_mro);
  word flags = 0;
  for (word i = 1; i < mro->length(); i++) {
    Handle<Type> cur(&scope, mro->at(i));
    flags |= SmallInt::cast(cur->flags())->value();
  }
  result->setFlags(SmallInt::fromWord(flags));

  return *result;
}

Object* builtinTypeInit(Thread*, Frame*, word) { return None::object(); }

Object* builtinTypeRepr(Thread* thread, Frame* frame, word nargs) {
  if (nargs == 0) {
    return thread->throwTypeErrorFromCString(
        "type.__repr__(): Need a self argument");
  }
  if (nargs > 1) {
    return thread->throwTypeError(thread->runtime()->newStringFromFormat(
        "expected 0 arguments, got %ld", nargs - 1));
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));
  if (!thread->runtime()->hasSubClassFlag(*self, Type::Flag::kTypeSubclass)) {
    return thread->throwTypeErrorFromCString(
        "type.__repr__() requires a 'type' object");
  }

  Handle<Type> type(&scope, *self);
  Handle<String> type_name(&scope, type->name());
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
  Handle<String> result(&scope, thread->runtime()->newStringFromCString(buf));
  delete[] buf;
  return *result;
}

}  // namespace python
