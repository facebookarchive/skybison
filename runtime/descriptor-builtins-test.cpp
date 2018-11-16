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

TEST(DescriptorBuiltinsTest,
     PropertyCreateEmptyGetterSetterDeleterReturnsNone) {
  const char* src = R"(
x = property()
)";
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr(src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object x(&scope, moduleAt(&runtime, main, "x"));
  ASSERT_TRUE(x->isProperty());
  Property prop(&scope, *x);
  ASSERT_TRUE(prop->getter()->isNoneType());
  ASSERT_TRUE(prop->setter()->isNoneType());
  ASSERT_TRUE(prop->deleter()->isNoneType());
}

TEST(DescriptorBuiltinsTest, PropertyCreateWithGetterSetterReturnsArgs) {
  const char* src = R"(
def get_foo():
  pass
def set_foo():
  pass
x = property(get_foo, set_foo)
)";
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr(src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object x(&scope, moduleAt(&runtime, main, "x"));
  ASSERT_TRUE(x->isProperty());
  Property prop(&scope, *x);
  ASSERT_TRUE(prop->getter()->isFunction());
  ASSERT_TRUE(prop->setter()->isFunction());
  ASSERT_TRUE(prop->deleter()->isNoneType());
}

TEST(DescriptorBuiltinsTest, PropertyModifyViaGetterReturnsGetter) {
  const char* src = R"(
def get_foo():
  pass
def set_foo():
  pass
x = property(None, set_foo)
y = x.getter(get_foo)
)";
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr(src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object x(&scope, moduleAt(&runtime, main, "x"));
  ASSERT_TRUE(x->isProperty());
  Property x_prop(&scope, *x);
  ASSERT_TRUE(x_prop->getter()->isNoneType());
  ASSERT_TRUE(x_prop->setter()->isFunction());
  ASSERT_TRUE(x_prop->deleter()->isNoneType());

  Object y(&scope, moduleAt(&runtime, main, "y"));
  ASSERT_TRUE(y->isProperty());
  Property y_prop(&scope, *y);
  ASSERT_TRUE(y_prop->getter()->isFunction());
  ASSERT_TRUE(y_prop->setter()->isFunction());
  ASSERT_TRUE(y_prop->deleter()->isNoneType());
}

TEST(DescriptorBuiltinsTest, PropertyModifyViaSetterReturnsSetter) {
  const char* src = R"(
def get_foo():
  pass
def set_foo():
  pass
x = property(get_foo)
y = x.setter(set_foo)
)";
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr(src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object x(&scope, moduleAt(&runtime, main, "x"));
  ASSERT_TRUE(x->isProperty());
  Property x_prop(&scope, *x);
  ASSERT_TRUE(x_prop->getter()->isFunction());
  ASSERT_TRUE(x_prop->setter()->isNoneType());
  ASSERT_TRUE(x_prop->deleter()->isNoneType());

  Object y(&scope, moduleAt(&runtime, main, "y"));
  ASSERT_TRUE(y->isProperty());
  Property y_prop(&scope, *y);
  ASSERT_TRUE(y_prop->getter()->isFunction());
  ASSERT_TRUE(y_prop->setter()->isFunction());
  ASSERT_TRUE(y_prop->deleter()->isNoneType());
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

TEST(DescriptorBuiltinsDeathTest,
     PropertyNoGetterThrowsAttributeErrorUnreadable) {
  const char* src = R"(
class C:
  def __init__(self, x):
      self.__x = x

  def setx(self, value):
      self.__x = value

  x = property(None, setx)

c1 = C(24)
c1.x
)";

  Runtime runtime;
  EXPECT_DEATH(runtime.runFromCStr(src), "unreadable attribute");
}

TEST(DescriptorBuiltinsDeathTest,
     PropertyNoSetterThrowsAttributeErrorCannotModify) {
  const char* src = R"(
class C:
  def __init__(self, x):
      self.__x = x

  def getx(self):
      return self.__x

  x = property(getx)

c1 = C(24)
c1.x = 42
)";

  Runtime runtime;
  EXPECT_DEATH(runtime.runFromCStr(src), "can't set attribute");
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
  runtime.runFromCStr(src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object x(&scope, moduleAt(&runtime, main, "x"));
  ASSERT_TRUE(x->isProperty());
}

TEST(DescriptorBuiltinsTest, PropertyAddedViaClassModifiedViaSetter) {
  const char* src = R"(
class C:
  def __init__(self, x):
      self.__x = x

  def getx(self):
      return self.__x

  def setx(self, value):
      self.__x = value

  x = property(getx, setx)

c1 = C(24)
x1 = c1.x
c1.x = 42
x2 = c1.x
)";

  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr(src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object x1(&scope, moduleAt(&runtime, main, "x1"));
  ASSERT_TRUE(x1->isInt());
  EXPECT_EQ(RawSmallInt::cast(*x1)->value(), 24);
  Object x2(&scope, moduleAt(&runtime, main, "x2"));
  ASSERT_TRUE(x2->isInt());
  EXPECT_EQ(RawSmallInt::cast(*x2)->value(), 42);
}

TEST(DescriptorBuiltinsTest, PropertyAddedViaDecoratorSanityCheck) {
  const char* src = R"(
class C:
  def __init__(self, x):
      self.__x = x

  @property
  def x(self):
      return self.__x

  @x.setter
  def x(self, value):
      self.__x = value

c1 = C(24)
c1.x = 42
x = c1.x
)";

  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr(src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object x(&scope, moduleAt(&runtime, main, "x"));
  ASSERT_TRUE(x->isInt());
  EXPECT_EQ(RawSmallInt::cast(*x)->value(), 42);
}

}  // namespace python
