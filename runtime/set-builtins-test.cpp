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

} // namespace python
