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

  Handle<Class> type(&scope, args.get(0));
  Handle<Function> dunder_new(
      &scope, runtime->lookupNameInMro(thread, type, name));

  frame->pushValue(*dunder_new);
  for (word i = 0; i < nargs; i++) {
    frame->pushValue(args.get(i));
  }

  Handle<Object> result(&scope, dunder_new->entry()(thread, frame, nargs));

  // Pop all of the arguments we pushed for the __new__ call.  While we will
  // push the same number of arguments again for the __init__ call below,
  // starting over from scratch keeps the addressing arithmetic simple.
  frame->dropValues(nargs + 1);

  // Second, call __init__ to initialize the instance.

  // top of the stack should be the new instance
  Handle<Object> init(&scope, runtime->symbols()->DunderInit());
  Handle<Function> dunder_init(
      &scope, runtime->lookupNameInMro(thread, type, init));

  frame->pushValue(*dunder_init);
  frame->pushValue(*result);
  for (word i = 1; i < nargs; i++) {
    frame->pushValue(args.get(i));
  }

  dunder_init->entry()(thread, frame, nargs);
  frame->dropValues(nargs + 1);

  // TODO: throw a type error if the __init__ method does not return None.
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

Object* builtinTypeInit(Thread*, Frame*, word) {
  return None::object();
}

} // namespace python
