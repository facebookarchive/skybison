#include "descriptor-builtins.h"

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

// classmethod

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

Object* builtinClassMethodGet(Thread* thread, Frame* frame, word nargs) {
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

// staticmethod

Object* builtinStaticMethodGet(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 3) {
    return thread->throwTypeErrorFromCString("__get__ needs 3 arguments");
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Handle<Object> self(&scope, args.get(0));

  return StaticMethod::cast(*self)->function();
}

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
