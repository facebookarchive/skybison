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
  Handle<Object> name(&scope, runtime->symbols()->DunderNew());

  // First, call __new__ to allocate a new instance.
  if (!runtime->isInstanceOfClass(args.get(0))) {
    return thread->throwTypeErrorFromCString(
        "'__new__' requires a 'class' object");
  }
  Handle<Type> type(&scope, args.get(0));
  Handle<Object> dunder_new(&scope,
                            runtime->lookupNameInMro(thread, type, name));

  frame->pushValue(*dunder_new);
  for (word i = 0; i < nargs; i++) {
    frame->pushValue(args.get(i));
  }

  Handle<Object> result(&scope, Interpreter::call(thread, frame, nargs));

  // Second, call __init__ to initialize the instance.

  // top of the stack should be the new instance
  Handle<Object> init(&scope, runtime->symbols()->DunderInit());
  Handle<Object> dunder_init(&scope,
                             runtime->lookupNameInMro(thread, type, init));

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
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Handle<Type> metatype(&scope, args.get(0));
  LayoutId class_layout_id = Layout::cast(metatype->instanceLayout())->id();
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

}  // namespace python
