#include "builtins.h"

#include "bytecode.h"
#include "frame.h"
#include "globals.h"
#include "interpreter.h"
#include "mro.h"
#include "objects.h"
#include "os.h"
#include "runtime.h"
#include "thread.h"
#include "trampolines-inl.h"

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

  Object** sp = frame->valueStackTop();
  *--sp = *dunder_new;
  for (word i = 0; i < nargs; i++) {
    *--sp = args.get(i);
  }
  frame->setValueStackTop(sp);

  Handle<Object> result(&scope, dunder_new->entry()(thread, frame, nargs));

  // Pop all of the arguments we pushed for the __new__ call.  While we will
  // push the same number of arguments again for the __init__ call below,
  // starting over from scratch keeps the addressing arithmetic simple.
  frame->setValueStackTop(sp + nargs + 1);

  // Second, call __init__ to initialize the instance.

  // top of the stack should be the new instance
  Handle<Object> init(&scope, runtime->symbols()->DunderInit());
  Handle<Function> dunder_init(
      &scope, runtime->lookupNameInMro(thread, type, init));

  sp = frame->valueStackTop();
  *--sp = *dunder_init;
  *--sp = *result;
  for (word i = 1; i < nargs; i++) {
    *--sp = args.get(i);
  }
  frame->setValueStackTop(sp);

  dunder_init->entry()(thread, frame, nargs);

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

Object* builtinObjectInit(Thread* thread, Frame*, word nargs) {
  if (nargs != 1) {
    thread->throwTypeErrorFromCString("object.__init__() takes no arguments");
    return Error::object();
  }
  return None::object();
}

Object* builtinObjectNew(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  if (nargs < 1) {
    thread->throwTypeErrorFromCString("object.__new__() takes no arguments");
    return Error::object();
  }
  HandleScope scope(thread->handles());
  Handle<Class> klass(&scope, args.get(0));
  Handle<Layout> layout(&scope, klass->instanceLayout());
  return thread->runtime()->newInstance(layout);
}

// Descriptor
Object* functionDescriptorGet(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 3) {
    return thread->throwTypeErrorFromCString("__get__ needs 3 arguments");
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Handle<Object> self(&scope, args.get(0));
  Handle<Object> instance(&scope, args.get(1));
  if (instance->isNone()) {
    return *self;
  }
  return thread->runtime()->newBoundMethod(self, instance);
}

Object* classmethodDescriptorGet(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 3) {
    return thread->throwTypeErrorFromCString("__get__ needs 3 arguments");
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Handle<Object> self(&scope, args.get(0));
  Handle<Object> owner(&scope, args.get(2));

  Handle<Object> method(&scope, ClassMethod::cast(*self)->function());
  return thread->runtime()->newBoundMethod(method, owner);
}

Object* staticmethodDescriptorGet(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 3) {
    return thread->throwTypeErrorFromCString("__get__ needs 3 arguments");
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Handle<Object> self(&scope, args.get(0));

  return StaticMethod::cast(*self)->function();
}

// ClassMethod
Object* builtinClassMethodNew(Thread* thread, Frame*, word) {
  return thread->runtime()->newClassMethod();
}

Object* builtinClassMethodInit(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString(
        "classmethod expected 1 arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<ClassMethod> classmethod(&scope, args.get(0));
  Handle<Object> arg(&scope, args.get(1));
  classmethod->setFunction(*arg);
  return *classmethod;
}

// StaticMethod
Object* builtinStaticMethodNew(Thread* thread, Frame*, word) {
  return thread->runtime()->newStaticMethod();
}

Object* builtinStaticMethodInit(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString(
        "staticmethod expected 1 arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<StaticMethod> staticmethod(&scope, args.get(0));
  Handle<Object> arg(&scope, args.get(1));
  staticmethod->setFunction(*arg);
  return *staticmethod;
}

} // namespace python
