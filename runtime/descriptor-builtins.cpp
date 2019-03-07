#include "descriptor-builtins.h"

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

// classmethod

const BuiltinMethod ClassMethodBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderGet, dunderGet},
    {SymbolId::kDunderInit, dunderInit},
    {SymbolId::kDunderNew, dunderNew},
    {SymbolId::kSentinelId, nullptr},
};

RawObject ClassMethodBuiltins::dunderNew(Thread* thread, Frame*, word) {
  return thread->runtime()->newClassMethod();
}

RawObject ClassMethodBuiltins::dunderInit(Thread* thread, Frame* frame,
                                          word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  ClassMethod classmethod(&scope, args.get(0));
  Object arg(&scope, args.get(1));
  classmethod.setFunction(*arg);
  return NoneType::object();
}

RawObject ClassMethodBuiltins::dunderGet(Thread* thread, Frame* frame,
                                         word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Object owner(&scope, args.get(2));

  Object method(&scope, RawClassMethod::cast(*self).function());
  return thread->runtime()->newBoundMethod(method, owner);
}

// staticmethod

const BuiltinMethod StaticMethodBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderGet, dunderGet},
    {SymbolId::kDunderInit, dunderInit},
    {SymbolId::kDunderNew, dunderNew},
    {SymbolId::kSentinelId, nullptr},
};

RawObject StaticMethodBuiltins::dunderGet(Thread* thread, Frame* frame,
                                          word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));

  return RawStaticMethod::cast(*self).function();
}

RawObject StaticMethodBuiltins::dunderNew(Thread* thread, Frame*, word) {
  return thread->runtime()->newStaticMethod();
}

RawObject StaticMethodBuiltins::dunderInit(Thread* thread, Frame* frame,
                                           word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  StaticMethod staticmethod(&scope, args.get(0));
  Object arg(&scope, args.get(1));
  staticmethod.setFunction(*arg);
  return NoneType::object();
}

// property

// clang-format off
const BuiltinMethod PropertyBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDeleter, deleter},
    {SymbolId::kDunderGet, dunderGet},
    {SymbolId::kDunderInit, dunderInit},
    {SymbolId::kDunderNew, dunderNew},
    {SymbolId::kDunderSet, dunderSet},
    {SymbolId::kGetter, getter},
    {SymbolId::kSetter, setter},
    {SymbolId::kSentinelId, nullptr},
};
// clang-format on

RawObject PropertyBuiltins::deleter(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  if (!args.get(0).isProperty()) {
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

RawObject PropertyBuiltins::dunderGet(Thread* thread, Frame* frame,
                                      word nargs) {
  if (nargs < 3 || nargs > 4) {
    return thread->raiseTypeErrorWithCStr(
        "property.__get__ expects 2-3 arguments");
  }

  HandleScope scope(thread);
  Arguments args(frame, nargs);
  if (!args.get(0).isProperty()) {
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

RawObject PropertyBuiltins::dunderSet(Thread* thread, Frame* frame,
                                      word nargs) {
  if (nargs != 3) {
    return thread->raiseTypeErrorWithCStr(
        "property.__set__ expects 2 arguments");
  }

  HandleScope scope(thread);
  Arguments args(frame, nargs);
  if (!args.get(0).isProperty()) {
    return thread->raiseTypeErrorWithCStr(
        "'__set__' requires a 'property' object");
  }
  Property property(&scope, args.get(0));
  Object obj(&scope, args.get(1));
  Object value(&scope, args.get(2));

  if (property.setter().isNoneType()) {
    return thread->raiseAttributeErrorWithCStr("can't set attribute");
  }

  Object setter(&scope, property.setter());
  return Interpreter::callFunction2(thread, frame, setter, obj, value);
}

RawObject PropertyBuiltins::getter(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  if (!args.get(0).isProperty()) {
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

RawObject PropertyBuiltins::dunderInit(Thread* thread, Frame* frame,
                                       word nargs) {
  Arguments args(frame, nargs);
  if (!args.get(0).isProperty()) {
    return thread->raiseTypeErrorWithCStr(
        "'__init__' requires a 'property' object");
  }
  HandleScope scope(thread);
  Property property(&scope, args.get(0));
  property.setGetter(args.get(1));
  property.setSetter(args.get(2));
  property.setDeleter(args.get(3));
  return NoneType::object();
}

RawObject PropertyBuiltins::dunderNew(Thread* thread, Frame*, word) {
  HandleScope scope(thread);
  Object none(&scope, NoneType::object());
  return thread->runtime()->newProperty(none, none, none);
}

RawObject PropertyBuiltins::setter(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  if (!args.get(0).isProperty()) {
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
