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
    return thread->throwTypeErrorFromCStr("classmethod expected 1 arguments");
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
    return thread->throwTypeErrorFromCStr("__get__ needs 3 arguments");
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
    return thread->throwTypeErrorFromCStr("__get__ needs 3 arguments");
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
    return thread->throwTypeErrorFromCStr("staticmethod expected 1 arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<StaticMethod> staticmethod(&scope, args.get(0));
  Handle<Object> arg(&scope, args.get(1));
  staticmethod->setFunction(*arg);
  return *staticmethod;
}

// property

Object* builtinPropertyDeleter(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCStr(
        "property.deleter expects 1 arguments");
  }
  Arguments args(frame, nargs);
  if (!args.get(0)->isProperty()) {
    return thread->throwTypeErrorFromCStr(
        "'deleter' requires a 'property' object");
  }

  HandleScope scope(thread);
  Handle<Property> property(&scope, args.get(0));
  Handle<Object> getter(&scope, property->getter());
  Handle<Object> setter(&scope, property->setter());
  Handle<Object> deleter(&scope, args.get(1));
  return thread->runtime()->newProperty(getter, setter, deleter);
}

Object* builtinPropertyDunderGet(Thread* thread, Frame* frame, word nargs) {
  if (nargs < 3 || nargs > 4) {
    return thread->throwTypeErrorFromCStr(
        "property.__get__ expects 2-3 arguments");
  }

  HandleScope scope(thread);
  Arguments args(frame, nargs);
  if (!args.get(0)->isProperty()) {
    return thread->throwTypeErrorFromCStr(
        "'__get__' requires a 'property' object");
  }
  Handle<Property> property(&scope, args.get(0));
  Handle<Object> obj(&scope, args.get(1));

  if (property->getter() == None::object()) {
    return thread->throwAttributeErrorFromCStr("unreadable attribute");
  }

  if (obj->isNone()) {
    return *property;
  }

  Handle<Object> getter(&scope, property->getter());
  return Interpreter::callMethod1(thread, frame, getter, obj);
}

Object* builtinPropertyDunderSet(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 3) {
    return thread->throwTypeErrorFromCStr(
        "property.__set__ expects 2 arguments");
  }

  HandleScope scope(thread);
  Arguments args(frame, nargs);
  if (!args.get(0)->isProperty()) {
    return thread->throwTypeErrorFromCStr(
        "'__set__' requires a 'property' object");
  }
  Handle<Property> property(&scope, args.get(0));
  Handle<Object> obj(&scope, args.get(1));
  Handle<Object> value(&scope, args.get(2));

  if (property->setter()->isNone()) {
    return thread->throwAttributeErrorFromCStr("can't set attribute");
  }

  Handle<Object> setter(&scope, property->setter());
  return Interpreter::callMethod2(thread, frame, setter, obj, value);
}

Object* builtinPropertyGetter(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCStr(
        "property.getter expects 1 arguments");
  }
  Arguments args(frame, nargs);
  if (!args.get(0)->isProperty()) {
    return thread->throwTypeErrorFromCStr(
        "'getter' requires a 'property' object");
  }
  HandleScope scope(thread);
  Handle<Property> property(&scope, args.get(0));
  Handle<Object> getter(&scope, args.get(1));
  Handle<Object> setter(&scope, property->setter());
  Handle<Object> deleter(&scope, property->deleter());
  return thread->runtime()->newProperty(getter, setter, deleter);
}

Object* builtinPropertyInit(Thread* thread, Frame* frame, word nargs) {
  if (nargs < 1 || nargs > 4) {
    return thread->throwTypeErrorFromCStr("property expects up to 3 arguments");
  }
  Arguments args(frame, nargs);
  if (!args.get(0)->isProperty()) {
    return thread->throwTypeErrorFromCStr(
        "'__init__' requires a 'property' object");
  }
  HandleScope scope(thread);
  Handle<Property> property(&scope, args.get(0));
  if (nargs > 1) {
    property->setGetter(args.get(1));
  }
  if (nargs > 2) {
    property->setSetter(args.get(2));
  }
  if (nargs > 3) {
    property->setDeleter(args.get(3));
  }
  return *property;
}

Object* builtinPropertyNew(Thread* thread, Frame*, word) {
  HandleScope scope(thread);
  Handle<Object> none(&scope, None::object());
  return thread->runtime()->newProperty(none, none, none);
}

Object* builtinPropertySetter(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCStr(
        "property.setter expects 1 arguments");
  }
  Arguments args(frame, nargs);
  if (!args.get(0)->isProperty()) {
    return thread->throwTypeErrorFromCStr(
        "'setter' requires a 'property' object");
  }
  HandleScope scope(thread);
  Handle<Property> property(&scope, args.get(0));
  Handle<Object> getter(&scope, property->getter());
  Handle<Object> setter(&scope, args.get(1));
  Handle<Object> deleter(&scope, property->deleter());
  return thread->runtime()->newProperty(getter, setter, deleter);
}

}  // namespace python
