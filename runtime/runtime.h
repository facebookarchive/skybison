#pragma once

#include "heap.h"
#include "objects.h"

namespace python {

class Runtime {
public:
  static void initialize();

  static Class* byteArrayClass() { return Class::cast(byteArrayClass_); }
  static Class* objectArrayClass() { return Class::cast(objectArrayClass_); }
  static Class* codeClass() { return Class::cast(codeClass_); }
  static Class* classClass() { return Class::cast(classClass_); }
  static Class* stringClass() { return Class::cast(stringClass_); }
  static Class* functionClass() { return Class::cast(functionClass_); }

 private:
  // the equivalent of sys.modules in python
  static Object* modules_;

  static Object* byteArrayClass_;
  static Object* objectArrayClass_;
  static Object* classClass_;
  static Object* codeClass_;
  static Object* stringClass_;
  static Object* functionClass_;
  static Object* moduleClass_;
  static Object* dictionaryClass_;

  static void allocateClasses();
};

}  // namespace python
