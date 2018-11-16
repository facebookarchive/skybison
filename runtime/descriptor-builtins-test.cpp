#include "gtest/gtest.h"

#include "descriptor-builtins.h"
#include "handles.h"
#include "objects.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(DescriptorBuiltinsTest, Classmethod) {
  const char* src = R"(
class Foo():
  a = 1
  @classmethod
  def bar(cls):
    print(cls.a)
a = Foo()
a.bar()
Foo.a = 2
Foo.bar()
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "1\n2\n");
}

TEST(DescriptorBuiltinsTest, StaticmethodObjAccess) {
  const char* src = R"(
class E:
    @staticmethod
    def f(x):
        return x + 1

e = E()
print(e.f(5))
)";

  Runtime runtime;
  const std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "6\n");
}

TEST(DescriptorBuiltinsTest, StaticmethodClsAccess) {
  const char* src = R"(
class E():
    @staticmethod
    def f(x, y):
        return x + y

print(E.f(1,2))
)";

  Runtime runtime;
  const std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "3\n");
}

TEST(DescriptorBuiltinsTest, PropertyAddedViaClassAccessibleViaInstance) {
  const char* src = R"(
class C:
  def __init__(self, x):
      self.__x = x

  def getx(self):
      return self.__x

  x = property(getx)

c1 = C(24)
c2 = C(42)
print(c1.x, c2.x)
)";

  Runtime runtime;
  const std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "24 42\n");
}

TEST(DescriptorBuiltinsTest, PropertyAddedViaClassAccessibleViaClass) {
  const char* src = R"(
class C:
  def __init__(self, x):
      self.__x = x

  def getx(self):
      return self.__x

  x = property(getx)

x = C.x
)";

  Runtime runtime;
  HandleScope scope;
  runtime.runFromCString(src);
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> x(&scope, moduleAt(&runtime, main, "x"));
  ASSERT_TRUE(x->isProperty());
}

TEST(DescriptorBuiltinsTest, PropertyAddedViaDecoratorAccessibleViaInstance) {
  const char* src = R"(
class C:
  def __init__(self, x = None):
      self.__x = x

  @property
  def x(self):
      return self.__x

x = C(24).x
)";

  Runtime runtime;
  HandleScope scope;
  runtime.runFromCString(src);
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> x(&scope, moduleAt(&runtime, main, "x"));
  ASSERT_TRUE(x->isInteger());
  EXPECT_EQ(SmallInteger::cast(*x)->value(), 24);
}

} // namespace python
