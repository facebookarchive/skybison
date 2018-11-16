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

TEST(RuntimeTest, NewCode) {
  Runtime runtime;
  HandleScope scope;

  Handle<Code> code(&scope, runtime.newCode());
  EXPECT_EQ(code->argcount(), 0);
  EXPECT_EQ(code->cell2arg(), 0);
  ASSERT_TRUE(code->cellvars()->isObjectArray());
  EXPECT_EQ(ObjectArray::cast(code->cellvars())->length(), 0);
  EXPECT_TRUE(code->code()->isNone());
  EXPECT_TRUE(code->consts()->isNone());
  EXPECT_TRUE(code->filename()->isNone());
  EXPECT_EQ(code->firstlineno(), 0);
  EXPECT_EQ(code->flags(), 0);
  ASSERT_TRUE(code->freevars()->isObjectArray());
  EXPECT_EQ(ObjectArray::cast(code->freevars())->length(), 0);
  EXPECT_EQ(code->kwonlyargcount(), 0);
  EXPECT_TRUE(code->lnotab()->isNone());
  EXPECT_TRUE(code->name()->isNone());
  EXPECT_EQ(code->nlocals(), 0);
  EXPECT_EQ(code->stacksize(), 0);
  EXPECT_TRUE(code->varnames()->isNone());
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
