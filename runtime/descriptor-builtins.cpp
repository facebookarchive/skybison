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

  Object method(&scope, ClassMethod::cast(*self).function());
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

  return StaticMethod::cast(*self).function();
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
    {SymbolId::kDunderDelete, dunderDelete},
    {SymbolId::kDunderGet, dunderGet},
    {SymbolId::kDunderInit, dunderInit},
    {SymbolId::kDunderNew, dunderNew},
    {SymbolId::kDunderSet, dunderSet},
    {SymbolId::kDeleter, deleter},
    {SymbolId::kGetter, getter},
    {SymbolId::kSetter, setter},
    {SymbolId::kSentinelId, nullptr},
};
// clang-format on

RawObject PropertyBuiltins::dunderDelete(Thread* thread, Frame* frame,
                                         word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isProperty()) {
    return thread->raiseRequiresType(self_obj, SymbolId::kProperty);
  }
  Property self(&scope, *self_obj);
  Object deleter(&scope, self.deleter());
  if (deleter.isNoneType()) {
    return thread->raiseWithFmt(LayoutId::kAttributeError,
                                "can't delete attribute");
  }
  Object instance(&scope, args.get(1));
  return Interpreter::callFunction1(thread, frame, deleter, instance);
}

RawObject PropertyBuiltins::dunderGet(Thread* thread, Frame* frame,
                                      word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isProperty()) {
    return thread->raiseRequiresType(self_obj, SymbolId::kProperty);
  }
  Property self(&scope, *self_obj);
  Object getter(&scope, self.getter());
  if (getter.isNoneType()) {
    return thread->raiseWithFmt(LayoutId::kAttributeError,
                                "unreadable attribute");
  }
  Object instance(&scope, args.get(1));
  if (instance.isNoneType()) {
    return *self;
  }
  return Interpreter::callFunction1(thread, frame, getter, instance);
}

RawObject PropertyBuiltins::dunderInit(Thread* thread, Frame* frame,
                                       word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isProperty()) {
    return thread->raiseRequiresType(self_obj, SymbolId::kProperty);
  }
  Property self(&scope, *self_obj);
  self.setGetter(args.get(1));
  self.setSetter(args.get(2));
  self.setDeleter(args.get(3));
  // TODO(T42363565) Do something with the doc argument.
  return NoneType::object();
}

RawObject PropertyBuiltins::dunderNew(Thread* thread, Frame*, word) {
  HandleScope scope(thread);
  Object none(&scope, NoneType::object());
  return thread->runtime()->newProperty(none, none, none);
}

RawObject PropertyBuiltins::dunderSet(Thread* thread, Frame* frame,
                                      word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isProperty()) {
    return thread->raiseRequiresType(self_obj, SymbolId::kProperty);
  }
  Property self(&scope, *self_obj);
  Object setter(&scope, self.setter());
  if (setter.isNoneType()) {
    return thread->raiseWithFmt(LayoutId::kAttributeError,
                                "can't set attribute");
  }
  Object obj(&scope, args.get(1));
  Object value(&scope, args.get(2));
  return Interpreter::callFunction2(thread, frame, setter, obj, value);
}

RawObject PropertyBuiltins::deleter(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isProperty()) {
    return thread->raiseRequiresType(self_obj, SymbolId::kProperty);
  }
  Property self(&scope, *self_obj);
  Object getter(&scope, self.getter());
  Object setter(&scope, self.setter());
  Object deleter(&scope, args.get(1));
  return thread->runtime()->newProperty(getter, setter, deleter);
}

RawObject PropertyBuiltins::getter(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isProperty()) {
    return thread->raiseRequiresType(self_obj, SymbolId::kProperty);
  }
  Property self(&scope, *self_obj);
  Object getter(&scope, args.get(1));
  Object setter(&scope, self.setter());
  Object deleter(&scope, self.deleter());
  return thread->runtime()->newProperty(getter, setter, deleter);
}

RawObject PropertyBuiltins::setter(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isProperty()) {
    return thread->raiseRequiresType(self_obj, SymbolId::kProperty);
  }
  Property self(&scope, *self_obj);
  Object getter(&scope, self.getter());
  Object setter(&scope, args.get(1));
  Object deleter(&scope, self.deleter());
  return thread->runtime()->newProperty(getter, setter, deleter);
}

}  // namespace python
