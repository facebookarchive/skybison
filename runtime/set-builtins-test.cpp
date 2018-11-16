#include "gtest/gtest.h"

#include "runtime.h"
#include "set-builtins.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(SetDeathTest, SetPopException) {
  const char* src1 = R"(
s = {1}
s.pop()
s.pop()
)";
  Runtime runtime;
  ASSERT_DEATH(
      runtime.runFromCString(src1),
      "aborting due to pending exception: "
      "pop from an empty set");
}

TEST(SetBuiltinsTest, SetPop) {
  const char* src = R"(
s = {1}
a = s.pop()
b = len(s)
)";
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCString(src);
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> a(&scope, findInModule(&runtime, main, "a"));
  Handle<Object> b(&scope, findInModule(&runtime, main, "b"));
  EXPECT_EQ(SmallInteger::cast(*a)->value(), 1);
  EXPECT_EQ(SmallInteger::cast(*b)->value(), 0);
}

TEST(SetBuiltinsTest, InitializeByTypeCall) {
  const char* src = R"(
s = set()
)";
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCString(src);
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> s(&scope, findInModule(&runtime, main, "s"));
  EXPECT_TRUE(s->isSet());
  EXPECT_EQ(Set::cast(*s)->numItems(), 0);
}

TEST(SetBuiltinTest, SetAdd) {
  const char* src = R"(
s = set()
s.add(1)
s.add("Hello, World")
)";
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCString(src);
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Set> s(&scope, findInModule(&runtime, main, "s"));
  Handle<Object> one(&scope, runtime.newInteger(1));
  Handle<Object> hello_world(
      &scope, runtime.newStringFromCString("Hello, World"));
  EXPECT_EQ(s->numItems(), 2);
  EXPECT_TRUE(runtime.setIncludes(s, one));
  EXPECT_TRUE(runtime.setIncludes(s, hello_world));
}

TEST(SetDeathTest, SetAddException) {
  const char* src1 = R"(
s = set()
s.add(1, 2)
)";
  Runtime runtime;
  ASSERT_DEATH(
      runtime.runFromCString(src1),
      "aborting due to pending exception: "
      "add\\(\\) takes exactly one argument");
}

} // namespace python
