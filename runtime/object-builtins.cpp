#include "object-builtins.h"

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

Object* builtinObjectHash(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString(
        "object.__hash__() takes no arguments");
  }
  Arguments args(frame, nargs);
  return thread->runtime()->hash(args.get(0));
}

Object* builtinObjectInit(Thread* thread, Frame* frame, word nargs) {
  // object.__init__ doesn't do anything except throw a TypeError if the wrong
  // number of arguments are given. It only throws if __new__ is not overloaded
  // or __init__ was overloaded, else it allows the excess arguments.
  if (nargs == 0) {
    return thread->throwTypeErrorFromCString("__init__ needs an argument");
  }
  if (nargs == 1) {
    return None::object();
  }
  // Too many arguments were given. Determine if the __new__ was not overwritten
  // or the __init__ was to throw a TypeError.
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Handle<Object> self(&scope, args.get(0));
  Handle<Class> type(&scope, runtime->classOf(*self));
  if (!runtime->isMethodOverloaded(thread, type, SymbolId::kDunderNew) ||
      runtime->isMethodOverloaded(thread, type, SymbolId::kDunderInit)) {
    // Throw a TypeError if extra arguments were passed, and __new__ was not
    // overwritten by self, or __init__ was overloaded by self.
    return thread->throwTypeErrorFromCString(
        "object.__init__() takes no parameters");
  }
  // Else it's alright to have extra arguments.
  return None::object();
}

Object* builtinObjectNew(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  if (nargs < 1) {
    return thread->throwTypeErrorFromCString(
        "object.__new__() takes no arguments");
  }
  HandleScope scope(thread);
  Handle<Class> klass(&scope, args.get(0));
  Handle<Layout> layout(&scope, klass->instanceLayout());
  return thread->runtime()->newInstance(layout);
}

Object* builtinObjectRepr(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString("expected 0 arguments");
  }
  Arguments args(frame, nargs);

  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));

  // TODO(T31727304): Get the module and qualified subname. For now settle for
  // the class name.
  Handle<Class> type(&scope, runtime->classOf(*self));
  Handle<String> type_name(&scope, type->name());
  char* c_string = type_name->toCString();
  Object* str = thread->runtime()->newStringFromFormat(
      "<%s object at %p>", c_string, static_cast<void*>(*self));
  free(c_string);
  return str;
}

Object* builtinObjectStr(Thread* thread, Frame* frame, word nargs) {
  if (nargs < 1) {
    return thread->throwTypeErrorFromCString(
        "object.__str__() takes a self argument");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));
  // Forward to __repr__.
  Handle<Object> repr(&scope, Interpreter::lookupMethod(thread, frame, self,
                                                        SymbolId::kDunderRepr));
  DCHECK(!repr->isError(),
         "__repr__ must be defined since it exists on object");
  return Interpreter::callMethod1(thread, frame, repr, self);
}

}  // namespace python
