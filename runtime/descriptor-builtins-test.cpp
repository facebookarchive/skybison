#include "descriptor-builtins.h"

#include "gtest/gtest.h"

#include "builtins.h"
#include "handles.h"
#include "objects.h"
#include "runtime.h"
#include "test-utils.h"

namespace py {

using namespace testing;

using DescriptorBuiltinsTest = RuntimeFixture;

TEST_F(DescriptorBuiltinsTest, Classmethod) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class Foo():
  a = 1
  @classmethod
  def bar(cls):
    return cls.a
instance_a = Foo().bar()
Foo.a = 2
class_a = Foo.bar()
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "instance_a"), 1));
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "class_a"), 2));
}

TEST_F(DescriptorBuiltinsTest, StaticmethodObjAccess) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class E:
    @staticmethod
    def f(x):
        return x + 1

result = E().f(5)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result"), 6));
}

TEST_F(DescriptorBuiltinsTest, StaticmethodClsAccess) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class E():
    @staticmethod
    def f(x, y):
        return x + y

result = E.f(1,2)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result"), 3));
}

TEST_F(DescriptorBuiltinsTest,
       PropertyCreateEmptyGetterSetterDeleterReturnsNone) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, "x = property()").isError());
  Object x(&scope, mainModuleAt(runtime_, "x"));
  ASSERT_TRUE(x.isProperty());
  Property prop(&scope, *x);
  ASSERT_TRUE(prop.getter().isNoneType());
  ASSERT_TRUE(prop.setter().isNoneType());
  ASSERT_TRUE(prop.deleter().isNoneType());
}

TEST_F(DescriptorBuiltinsTest, PropertyCreateWithGetterSetterReturnsArgs) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def get_foo():
  pass
def set_foo():
  pass
x = property(get_foo, set_foo)
)")
                   .isError());
  Object x(&scope, mainModuleAt(runtime_, "x"));
  ASSERT_TRUE(x.isProperty());
  Property prop(&scope, *x);
  ASSERT_TRUE(prop.getter().isFunction());
  ASSERT_TRUE(prop.setter().isFunction());
  ASSERT_TRUE(prop.deleter().isNoneType());
}

TEST_F(DescriptorBuiltinsTest, PropertyModifyViaGetterReturnsGetter) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def get_foo():
  pass
def set_foo():
  pass
x = property(None, set_foo)
y = x.getter(get_foo)
)")
                   .isError());
  Object x(&scope, mainModuleAt(runtime_, "x"));
  ASSERT_TRUE(x.isProperty());
  Property x_prop(&scope, *x);
  ASSERT_TRUE(x_prop.getter().isNoneType());
  ASSERT_TRUE(x_prop.setter().isFunction());
  ASSERT_TRUE(x_prop.deleter().isNoneType());

  Object y(&scope, mainModuleAt(runtime_, "y"));
  ASSERT_TRUE(y.isProperty());
  Property y_prop(&scope, *y);
  ASSERT_TRUE(y_prop.getter().isFunction());
  ASSERT_TRUE(y_prop.setter().isFunction());
  ASSERT_TRUE(y_prop.deleter().isNoneType());
}

TEST_F(DescriptorBuiltinsTest, PropertyModifyViaSetterReturnsSetter) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def get_foo():
  pass
def set_foo():
  pass
x = property(get_foo)
y = x.setter(set_foo)
)")
                   .isError());
  Object x(&scope, mainModuleAt(runtime_, "x"));
  ASSERT_TRUE(x.isProperty());
  Property x_prop(&scope, *x);
  ASSERT_TRUE(x_prop.getter().isFunction());
  ASSERT_TRUE(x_prop.setter().isNoneType());
  ASSERT_TRUE(x_prop.deleter().isNoneType());

  Object y(&scope, mainModuleAt(runtime_, "y"));
  ASSERT_TRUE(y.isProperty());
  Property y_prop(&scope, *y);
  ASSERT_TRUE(y_prop.getter().isFunction());
  ASSERT_TRUE(y_prop.setter().isFunction());
  ASSERT_TRUE(y_prop.deleter().isNoneType());
}

TEST_F(DescriptorBuiltinsTest, PropertyAddedViaClassAccessibleViaInstance) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  def __init__(self, x):
      self.__x = x

  def getx(self):
      return self.__x

  x = property(getx)

c1 = C(24)
c2 = C(42)
result0 = c1.x
result1 = c2.x
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result0"), 24));
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result1"), 42));
}

TEST_F(DescriptorBuiltinsTest, PropertyNoDeleterRaisesAttributeError) {
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
del c1.x
)";

  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, src),
                            LayoutId::kAttributeError,
                            "can't delete attribute"));
}

TEST_F(DescriptorBuiltinsTest, PropertyNoGetterRaisesAttributeErrorUnreadable) {
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

  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, src),
                            LayoutId::kAttributeError, "unreadable attribute"));
}

TEST_F(DescriptorBuiltinsTest,
       PropertyNoSetterRaisesAttributeErrorCannotModify) {
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

  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, src),
                            LayoutId::kAttributeError, "can't set attribute"));
}

TEST_F(DescriptorBuiltinsTest, PropertyAddedViaClassAccessibleViaClass) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  def __init__(self, x):
      self.__x = x

  def getx(self):
      return self.__x

  x = property(getx)

x = C.x
)")
                   .isError());

  Object x(&scope, mainModuleAt(runtime_, "x"));
  ASSERT_TRUE(x.isProperty());
}

TEST_F(DescriptorBuiltinsTest, PropertyAddedViaClassModifiedViaSetter) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
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
)")
                   .isError());

  Object x1(&scope, mainModuleAt(runtime_, "x1"));
  EXPECT_TRUE(isIntEqualsWord(*x1, 24));
  Object x2(&scope, mainModuleAt(runtime_, "x2"));
  EXPECT_TRUE(isIntEqualsWord(*x2, 42));
}

TEST_F(DescriptorBuiltinsTest, PropertyAddedViaDecoratorSanityCheck) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
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
)")
                   .isError());

  Object x(&scope, mainModuleAt(runtime_, "x"));
  EXPECT_TRUE(isIntEqualsWord(*x, 42));
}

TEST_F(DescriptorBuiltinsTest, PropertyWithCallableDeleterDeletesValue) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
def deleter(obj):
    del obj.y

class Foo:
    x = property(None, None, deleter, doc="documentation")
    y = 123

foo = Foo()
del foo.x
foo.y
)"),
                            LayoutId::kAttributeError,
                            "'Foo' object has no attribute 'y'"));
}

TEST_F(DescriptorBuiltinsTest, PropertyWithCallableGetterReturnsValue) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class Getter:
    def __call__(self, obj):
        return 123

class Foo:
  x = property(Getter())

result = Foo().x
)")
                   .isError());
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 123));
}

TEST_F(DescriptorBuiltinsTest, PropertyWithCallableSetterSetsValue) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class Setter:
    def __call__(self, obj, value):
        obj.y = value

class Foo:
  x = property(None, Setter(), None, doc="documentation")

foo = Foo()
foo.x = 123
result = foo.y
)")
                   .isError());
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 123));
}

}  // namespace py
