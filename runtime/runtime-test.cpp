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

TEST(RuntimeTest, NewString) {
  Runtime runtime;
  HandleScope scope;

  Handle<String> empty0(&scope, runtime.newString(0));
  EXPECT_EQ(empty0->length(), 0);

  Handle<String> empty1(&scope, runtime.newString(0));
  EXPECT_EQ(*empty0, *empty1);

  Handle<String> empty2(&scope, runtime.newStringFromCString("\0"));
  EXPECT_EQ(*empty0, *empty2);
}

} // namespace python
