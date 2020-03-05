#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace py {

class ClassMethodBuiltins
    : public Builtins<ClassMethodBuiltins, ID(classmethod),
                      LayoutId::kClassMethod> {
 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ClassMethodBuiltins);
};

class StaticMethodBuiltins
    : public Builtins<StaticMethodBuiltins, ID(staticmethod),
                      LayoutId::kStaticMethod> {
 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(StaticMethodBuiltins);
};

class PropertyBuiltins
    : public Builtins<PropertyBuiltins, ID(property), LayoutId::kProperty> {
 public:
  static const BuiltinAttribute kAttributes[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(PropertyBuiltins);
};

}  // namespace py
