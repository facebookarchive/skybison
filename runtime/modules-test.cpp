#include "gtest/gtest.h"

#include "runtime.h"

namespace python {

TEST(ModulesTest, TestCreate) {
  Runtime runtime;
  Object* name = runtime.createString("mymodule");
  ASSERT_NE(name, nullptr);
  Object* obj = runtime.createModule(name);
  ASSERT_NE(obj, nullptr);
  ASSERT_TRUE(obj->isModule());
  Module* mod = Module::cast(obj);
  EXPECT_EQ(mod->name(), name);
  obj = mod->dictionary();
  EXPECT_TRUE(obj->isDictionary());
}

} // namespace python
