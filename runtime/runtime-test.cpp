#include "gtest/gtest.h"

#include "runtime.h"

namespace python {

TEST(RuntimeTest, CollectGarbage) {
  Runtime runtime;
  ASSERT_TRUE(runtime.heap()->verify());
  runtime.collectGarbage();
  ASSERT_TRUE(runtime.heap()->verify());
}

TEST(RuntimeTest, BuiltinsModuleExists) {
  Runtime runtime;
  Object* name = runtime.newStringFromCString("builtins");
  ASSERT_NE(name, nullptr);
  Object* modules = runtime.modules();
  ASSERT_TRUE(modules->isDictionary());
  Object* builtins;
  bool found = Dictionary::at(modules, name, Object::hash(name), &builtins);
  ASSERT_TRUE(found);
  ASSERT_TRUE(builtins->isModule());
}

} // namespace python
