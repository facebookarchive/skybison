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
  ASSERT_FALSE(type->isError());
  Object instance(&scope, moduleAt(&runtime, "__main__", "c"));
  ASSERT_FALSE(instance->isError());
  Object instance_type(&scope, runtime.typeOf(*instance));
  ASSERT_FALSE(instance_type->isError());

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
  ASSERT_FALSE(global->isError());
  ASSERT_TRUE(global->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*global)->value(), 2);
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
  ASSERT_FALSE(global->isError());
  ASSERT_TRUE(global->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*global)->value(), 9);
}

TEST(TypeBuiltinsTest, BuiltinTypeCallDetectNonClsArgRaiseException) {
  Runtime runtime;
  HandleScope scope;
  Code code(&scope, testing::newEmptyCode(&runtime));
  code->setArgcount(1);
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->pushFrame(code);
  frame->pushValue(runtime.newStrFromCStr("not_a_cls"));
  RawObject result = TypeBuiltins::dunderCall(thread, frame, 1);
  ASSERT_TRUE(result->isError());
  ASSERT_TRUE(hasPendingExceptionWithLayout(LayoutId::kTypeError));
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
  ASSERT_FALSE(attr->isNoneType());
  ASSERT_TRUE(attr->isInt());
  ASSERT_EQ(RawSmallInt::cast(attr)->value(), 42);
}

TEST(TypeBuiltinTest, DunderReprForBuiltinReturnsStr) {
  Runtime runtime;
  HandleScope scope;
  Type type(&scope, runtime.typeAt(LayoutId::kObject));
  Object result(&scope, runBuiltin(TypeBuiltins::dunderRepr, type));
  ASSERT_TRUE(result->isStr());
  EXPECT_PYSTRING_EQ(RawStr::cast(*result), "<class 'object'>");
}

TEST(TypeBuiltinTest, DunderReprForUserDefinedTypeReturnsStr) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
class Foo:
  pass
)");
  HandleScope scope;
  Object type(&scope, moduleAt(&runtime, "__main__", "Foo"));

  Object result(&scope, runBuiltin(TypeBuiltins::dunderRepr, type));
  ASSERT_TRUE(result->isStr());
  // TODO(T32810595): Once module names are supported, this should become
  // "<class '__main__.Foo'>".
  EXPECT_PYSTRING_EQ(RawStr::cast(*result), "<class 'Foo'>");
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

  EXPECT_EQ(RawLayout::cast(a->instanceLayout())->id(), LayoutId::kSmallInt);
  EXPECT_EQ(RawLayout::cast(b->instanceLayout())->id(), LayoutId::kSmallStr);
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
  EXPECT_EQ(RawLayout::cast(a->instanceLayout())->id(), LayoutId::kType);
}

}  // namespace python
