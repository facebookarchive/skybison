#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace py {

RawObject METH(classmethod, __new__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(classmethod, __init__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(classmethod, __get__)(Thread* thread, Frame* frame, word nargs);

class ClassMethodBuiltins
    : public Builtins<ClassMethodBuiltins, ID(classmethod),
                      LayoutId::kClassMethod> {
 public:
  static const BuiltinMethod kBuiltinMethods[];
};

RawObject METH(staticmethod, __new__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(staticmethod, __init__)(Thread* thread, Frame* frame,
                                       word nargs);
RawObject METH(staticmethod, __get__)(Thread* thread, Frame* frame, word nargs);

class StaticMethodBuiltins
    : public Builtins<StaticMethodBuiltins, ID(staticmethod),
                      LayoutId::kStaticMethod> {
 public:
  static const BuiltinMethod kBuiltinMethods[];
};

RawObject METH(property, __delete__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(property, __get__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(property, __init__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(property, __new__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(property, __set__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(property, deleter)(Thread* thread, Frame* frame, word nargs);
RawObject METH(property, getter)(Thread* thread, Frame* frame, word nargs);
RawObject METH(property, setter)(Thread* thread, Frame* frame, word nargs);

class PropertyBuiltins
    : public Builtins<PropertyBuiltins, ID(property), LayoutId::kProperty> {
 public:
  static const BuiltinMethod kBuiltinMethods[];
};

}  // namespace py
