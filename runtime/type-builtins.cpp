#include "function-builtins.h"

#include "frame.h"
#include "globals.h"
#include "mro.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

Object* builtinTypeCall(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread->handles());
  Arguments args(frame, nargs);

  Runtime* runtime = thread->runtime();
  Handle<Object> name(&scope, runtime->symbols()->DunderNew());

  // First, call __new__ to allocate a new instance.

  if (!args.get(0)->isClass()) {
    return thread->throwTypeErrorFromCString(
        "'__new__' requires a 'class' object");
  }
  Handle<Class> type(&scope, args.get(0));
  Handle<Function> dunder_new(&scope,
                              runtime->lookupNameInMro(thread, type, name));

  frame->pushValue(*dunder_new);
  for (word i = 0; i < nargs; i++) {
    frame->pushValue(args.get(i));
  }

  Handle<Object> result(&scope, Interpreter::call(thread, frame, nargs));

  // Second, call __init__ to initialize the instance.

  // top of the stack should be the new instance
  Handle<Object> init(&scope, runtime->symbols()->DunderInit());
  Handle<Function> dunder_init(&scope,
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
  Handle<Class> metatype(&scope, args.get(0));
  Handle<Object> name(&scope, args.get(1));
  Handle<Class> result(&scope, runtime->newClass());
  result->setName(*name);

  // Compute MRO
  Handle<ObjectArray> parents(&scope, args.get(2));
  Handle<Object> mro(&scope, computeMro(thread, result, parents));
  if (mro->isError()) {
    return *mro;
  }
  result->setMro(*mro);

  Handle<Dictionary> dictionary(&scope, args.get(3));
  Handle<Object> class_cell_key(&scope, runtime->symbols()->DunderClassCell());
  Handle<Object> class_cell(&scope,
                            runtime->dictionaryAt(dictionary, class_cell_key));
  if (!class_cell->isError()) {
    ValueCell::cast(ValueCell::cast(*class_cell)->value())->setValue(*result);
    Object* tmp;
    runtime->dictionaryRemove(dictionary, class_cell_key, &tmp);
  }
  result->setDictionary(*dictionary);

  // Initialize instance layout
  Handle<Layout> layout(&scope, runtime->computeInitialLayout(thread, result));
  layout->setDescribedClass(*result);
  result->setInstanceLayout(*layout);

  // Initialize builtin base class
  result->setBuiltinBaseClass(runtime->computeBuiltinBaseClass(result));
  Handle<Class> base(&scope, result->builtinBaseClass());
  Handle<Class> list(&scope, thread->runtime()->classAt(LayoutId::kList));
  if (Boolean::cast(thread->runtime()->isSubClass(base, list))->value()) {
    result->setFlag(Class::Flag::kListSubclass);
    layout->addDelegateSlot();
  }
  return *result;
}

Object* builtinTypeInit(Thread*, Frame*, word) { return None::object(); }

}  // namespace python
