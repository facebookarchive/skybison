#include "descriptor-builtins.h"

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

// classmethod

RawObject builtinClassMethodNew(Thread* thread, Frame*, word) {
  return thread->runtime()->newClassMethod();
}

RawObject builtinClassMethodInit(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("classmethod expected 1 arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  ClassMethod classmethod(&scope, args.get(0));
  Object arg(&scope, args.get(1));
  classmethod.setFunction(*arg);
  return *classmethod;
}

RawObject builtinClassMethodGet(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 3) {
    return thread->raiseTypeErrorWithCStr("__get__ needs 3 arguments");
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Object owner(&scope, args.get(2));

  Object method(&scope, RawClassMethod::cast(*self)->function());
  return thread->runtime()->newBoundMethod(method, owner);
}

// staticmethod

RawObject builtinStaticMethodGet(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 3) {
    return thread->raiseTypeErrorWithCStr("__get__ needs 3 arguments");
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));

  return RawStaticMethod::cast(*self)->function();
}

RawObject builtinStaticMethodNew(Thread* thread, Frame*, word) {
  return thread->runtime()->newStaticMethod();
}

RawObject builtinStaticMethodInit(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("staticmethod expected 1 arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  StaticMethod staticmethod(&scope, args.get(0));
  Object arg(&scope, args.get(1));
  staticmethod.setFunction(*arg);
  return *staticmethod;
}

// property

RawObject builtinPropertyDeleter(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr(
        "property.deleter expects 1 arguments");
  }
  Arguments args(frame, nargs);
  if (!args.get(0)->isProperty()) {
    return thread->raiseTypeErrorWithCStr(
        "'deleter' requires a 'property' object");
  }

  HandleScope scope(thread);
  Property property(&scope, args.get(0));
  Object getter(&scope, property.getter());
  Object setter(&scope, property.setter());
  Object deleter(&scope, args.get(1));
  return thread->runtime()->newProperty(getter, setter, deleter);
}

RawObject builtinPropertyDunderGet(Thread* thread, Frame* frame, word nargs) {
  if (nargs < 3 || nargs > 4) {
    return thread->raiseTypeErrorWithCStr(
        "property.__get__ expects 2-3 arguments");
  }

  HandleScope scope(thread);
  Arguments args(frame, nargs);
  if (!args.get(0)->isProperty()) {
    return thread->raiseTypeErrorWithCStr(
        "'__get__' requires a 'property' object");
  }
  Property property(&scope, args.get(0));
  Object obj(&scope, args.get(1));

  if (property.getter() == NoneType::object()) {
    return thread->raiseAttributeErrorWithCStr("unreadable attribute");
  }

  if (obj.isNoneType()) {
    return *property;
  }

  Object getter(&scope, property.getter());
  return Interpreter::callFunction1(thread, frame, getter, obj);
}

RawObject builtinPropertyDunderSet(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 3) {
    return thread->raiseTypeErrorWithCStr(
        "property.__set__ expects 2 arguments");
  }

  HandleScope scope(thread);
  Arguments args(frame, nargs);
  if (!args.get(0)->isProperty()) {
    return thread->raiseTypeErrorWithCStr(
        "'__set__' requires a 'property' object");
  }
  Property property(&scope, args.get(0));
  Object obj(&scope, args.get(1));
  Object value(&scope, args.get(2));

  if (property.setter()->isNoneType()) {
    return thread->raiseAttributeErrorWithCStr("can't set attribute");
  }

  Object setter(&scope, property.setter());
  return Interpreter::callFunction2(thread, frame, setter, obj, value);
}

RawObject builtinPropertyGetter(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr(
        "property.getter expects 1 arguments");
  }
  Arguments args(frame, nargs);
  if (!args.get(0)->isProperty()) {
    return thread->raiseTypeErrorWithCStr(
        "'getter' requires a 'property' object");
  }
  HandleScope scope(thread);
  Property property(&scope, args.get(0));
  Object getter(&scope, args.get(1));
  Object setter(&scope, property.setter());
  Object deleter(&scope, property.deleter());
  return thread->runtime()->newProperty(getter, setter, deleter);
}

RawObject builtinPropertyInit(Thread* thread, Frame* frame, word nargs) {
  if (nargs < 1 || nargs > 4) {
    return thread->raiseTypeErrorWithCStr("property expects up to 3 arguments");
  }
  Arguments args(frame, nargs);
  if (!args.get(0)->isProperty()) {
    return thread->raiseTypeErrorWithCStr(
        "'__init__' requires a 'property' object");
  }
  HandleScope scope(thread);
  Property property(&scope, args.get(0));
  if (nargs > 1) {
    property.setGetter(args.get(1));
  }
  if (nargs > 2) {
    property.setSetter(args.get(2));
  }
  if (nargs > 3) {
    property.setDeleter(args.get(3));
  }
  return *property;
}

RawObject builtinPropertyNew(Thread* thread, Frame*, word) {
  HandleScope scope(thread);
  Object none(&scope, NoneType::object());
  return thread->runtime()->newProperty(none, none, none);
}

RawObject builtinPropertySetter(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr(
        "property.setter expects 1 arguments");
  }
  Arguments args(frame, nargs);
  if (!args.get(0)->isProperty()) {
    return thread->raiseTypeErrorWithCStr(
        "'setter' requires a 'property' object");
  }
  HandleScope scope(thread);
  Property property(&scope, args.get(0));
  Object getter(&scope, property.getter());
  Object setter(&scope, args.get(1));
  Object deleter(&scope, property.deleter());
  return thread->runtime()->newProperty(getter, setter, deleter);
}

}  // namespace python
