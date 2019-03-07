#include "gtest/gtest.h"

#include "handles.h"
#include "objects.h"
#include "runtime.h"
#include "test-utils.h"
#include "type-builtins.h"

namespace python {

using namespace testing;

TEST(TypeBuiltinsTest, DunderCallType) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
class C: pass
c = C()
)");

  Type type(&scope, moduleAt(&runtime, "__main__", "C"));
  ASSERT_FALSE(type.isError());
  Object instance(&scope, moduleAt(&runtime, "__main__", "c"));
  ASSERT_FALSE(instance.isError());
  Object instance_type(&scope, runtime.typeOf(*instance));
  ASSERT_FALSE(instance_type.isError());

  EXPECT_EQ(*type, *instance_type);
}

TEST(TypeBuiltinsTest, DunderCallTypeWithInit) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
class C:
  def __init__(self):
    global g
    g = 2

g = 1
C()
)");

  Object global(&scope, moduleAt(&runtime, "__main__", "g"));
  ASSERT_FALSE(global.isError());
  EXPECT_TRUE(isIntEqualsWord(*global, 2));
}

TEST(TypeBuiltinsTest, DunderCallTypeWithInitAndArgs) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
class C:
  def __init__(self, x):
    global g
    g = x

g = 1
C(9)
)");

  Object global(&scope, moduleAt(&runtime, "__main__", "g"));
  ASSERT_FALSE(global.isError());
  EXPECT_TRUE(isIntEqualsWord(*global, 9));
}

TEST(TypeBuiltinsTest, BuiltinTypeCallDetectNonClsArgRaiseException) {
  Runtime runtime;
  ASSERT_TRUE(raisedWithStr(runFromCStr(&runtime, "type.__call__(5)"),
                            LayoutId::kTypeError,
                            "self must be a type instance"));
}

TEST(TypeBuiltinTest, BuiltinTypeCallInvokeDunderInitAsCallable) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
class Callable:
  def __call__(self, obj):
    obj.x = 42
class C:
  __init__ = Callable()
c = C()
)");
  Object c(&scope, moduleAt(&runtime, "__main__", "c"));
  Thread* thread = Thread::currentThread();
  Object x(&scope, runtime.newStrFromCStr("x"));
  RawObject attr = runtime.attributeAt(thread, c, x);
  EXPECT_TRUE(isIntEqualsWord(attr, 42));
}

TEST(TypeBuiltinTest, DunderReprForBuiltinReturnsStr) {
  Runtime runtime;
  runFromCStr(&runtime, "result = type.__repr__(object)");
  EXPECT_TRUE(isStrEqualsCStr(moduleAt(&runtime, "__main__", "result"),
                              "<class 'object'>"));
}

TEST(TypeBuiltinTest, DunderReprForUserDefinedTypeReturnsStr) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
class Foo:
  pass
result = type.__repr__(Foo)
)");
  // TODO(T32810595): Once module names are supported, this should become
  // "<class '__main__.Foo'>".
  EXPECT_TRUE(isStrEqualsCStr(moduleAt(&runtime, "__main__", "result"),
                              "<class 'Foo'>"));
}

TEST(TypeBuiltinTest, DunderNewWithOneArgReturnsTypeOfArg) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
a = type.__new__(type, 1);
b = type.__new__(type, "hello");
)");
  Type a(&scope, moduleAt(&runtime, "__main__", "a"));
  Type b(&scope, moduleAt(&runtime, "__main__", "b"));

  EXPECT_EQ(RawLayout::cast(a.instanceLayout()).id(), LayoutId::kSmallInt);
  EXPECT_EQ(RawLayout::cast(b.instanceLayout()).id(), LayoutId::kSmallStr);
}

TEST(TypeBuiltinTest, DunderNewWithOneMetaclassArgReturnsType) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
class Foo(type):
  pass
a = type.__new__(type, Foo);
)");
  Type a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_EQ(RawLayout::cast(a.instanceLayout()).id(), LayoutId::kType);
}

TEST(TypeBuiltinsTest, TypeHasDunderMroAttribute) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, "result = str.__class__.__mro__");
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result.isTuple());
}

TEST(TypeBuiltinsTest, TypeHasDunderNameAttribute) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, "result = str.__class__.__name__");
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result.isStr());
  EXPECT_TRUE(isStrEqualsCStr(RawStr::cast(*result), "type"));
}

TEST(TypeBuiltinsTest, TypeHasDunderFlagsAttribute) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, "result = str.__class__.__flags__");
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result.isInt());
}

TEST(TypeBuiltinsTest, TypeHasDunderDictAttribute) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, "result = str.__class__.__dict__");
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result.isDict());
}

TEST(TypeBuiltinTest, DunderCallReceivesExArgs) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
class C:
  def __init__(self, *args):
    self.args = args

  def num_args(self):
    return len(self.args)

result = C(*(1,2,3)).num_args()
)");
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_EQ(*result, RawSmallInt::fromWord(3));
}

TEST(TypeBuiltinTest, ClassMethodDunderCallReceivesExArgs) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
class Foo:
  @classmethod
  def foo(cls, *args):
    return len(args)

result = Foo.foo(*(1,2,3))
)");
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_EQ(*result, RawSmallInt::fromWord(3));
}

TEST(TypeBuiltinTest, TypeNewReceivesExArgs) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
ty = type.__new__(type, *("foo", (object,), {'a': 1}))
name = ty.__name__
)");
  HandleScope scope;
  Object name(&scope, moduleAt(&runtime, "__main__", "name"));
  EXPECT_TRUE(isStrEqualsCStr(*name, "foo"));
}

TEST(TypeBuiltinTest, TypeCallWithInitReturningNonNoneRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
class C:
  def __init__(self, *args, **kwargs):
    return 5
C()
)"),
                            LayoutId::kTypeError,
                            "C.__init__ returned non None"));
}

}  // namespace python
