#include "gtest/gtest.h"

#include "handles.h"
#include "objects.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(TypeBuiltinsTest, DunderCallClass) {
  Runtime runtime;
  HandleScope scope;

  const char* src = R"(
class C: pass
c = C()
)";

  runtime.runFromCString(src);

  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Class> type(&scope, moduleAt(&runtime, main, "C"));
  ASSERT_FALSE(type->isError());
  Handle<Object> instance(&scope, moduleAt(&runtime, main, "c"));
  ASSERT_FALSE(instance->isError());
  Handle<Object> instance_type(&scope, runtime.classOf(*instance));
  ASSERT_FALSE(instance_type->isError());

  EXPECT_EQ(*type, *instance_type);
}

TEST(TypeBuiltinsTest, DunderCallClassWithInit) {
  Runtime runtime;
  HandleScope scope;

  const char* src = R"(
class C:
  def __init__(self):
    global g
    g = 2

g = 1
C()
)";

  runtime.runFromCString(src);

  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> global(&scope, moduleAt(&runtime, main, "g"));
  ASSERT_FALSE(global->isError());
  ASSERT_TRUE(global->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(*global)->value(), 2);
}

TEST(TypeBuiltinsTest, DunderCallClassWithInitAndArgs) {
  Runtime runtime;
  HandleScope scope;

  const char* src = R"(
class C:
  def __init__(self, x):
    global g
    g = x

g = 1
C(9)
)";

  runtime.runFromCString(src);

  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> global(&scope, moduleAt(&runtime, main, "g"));
  ASSERT_FALSE(global->isError());
  ASSERT_TRUE(global->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(*global)->value(), 9);
}

}  // namespace python
