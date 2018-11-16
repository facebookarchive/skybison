#include "gtest/gtest.h"

#include "handles.h"
#include "objects.h"
#include "runtime.h"
#include "test-utils.h"
#include "type-builtins.h"

namespace python {

using namespace testing;

TEST(TypeBuiltinsTest, DunderCallClass) {
  Runtime runtime;
  HandleScope scope;

  const char* src = R"(
class C: pass
c = C()
)";

  runtime.runFromCStr(src);

  Module main(&scope, findModule(&runtime, "__main__"));
  Type type(&scope, moduleAt(&runtime, main, "C"));
  ASSERT_FALSE(type->isError());
  Object instance(&scope, moduleAt(&runtime, main, "c"));
  ASSERT_FALSE(instance->isError());
  Object instance_type(&scope, runtime.typeOf(*instance));
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

  runtime.runFromCStr(src);

  Module main(&scope, findModule(&runtime, "__main__"));
  Object global(&scope, moduleAt(&runtime, main, "g"));
  ASSERT_FALSE(global->isError());
  ASSERT_TRUE(global->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*global)->value(), 2);
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

  runtime.runFromCStr(src);

  Module main(&scope, findModule(&runtime, "__main__"));
  Object global(&scope, moduleAt(&runtime, main, "g"));
  ASSERT_FALSE(global->isError());
  ASSERT_TRUE(global->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*global)->value(), 9);
}

TEST(TypeBuiltinsTest, BuiltinTypeCallDetectNonClsArgRaiseException) {
  Runtime runtime;
  HandleScope scope;
  Code code(&scope, runtime.newCode());
  code->setArgcount(1);
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->pushFrame(*code);
  frame->pushValue(runtime.newStrFromCStr("not_a_cls"));
  RawObject result = builtinTypeCall(thread, frame, 1);
  ASSERT_TRUE(result->isError());
  ASSERT_TRUE(thread->hasPendingException());
}

TEST(TypeBuiltinTest, BuiltinTypeCallInvokeDunderInitAsCallable) {
  Runtime runtime;
  HandleScope scope;
  const char* src = R"(
class Callable:
  def __call__(self, obj):
    obj.x = 42
class C:
  __init__ = Callable()
c = C()
)";
  runtime.runFromCStr(src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object c(&scope, moduleAt(&runtime, main, "c"));
  Thread* thread = Thread::currentThread();
  Object x(&scope, runtime.newStrFromCStr("x"));
  RawObject attr = runtime.attributeAt(thread, c, x);
  ASSERT_FALSE(attr->isNoneType());
  ASSERT_TRUE(attr->isInt());
  ASSERT_EQ(RawSmallInt::cast(attr)->value(), 42);
}

TEST(TypeBuiltinTest, DunderReprForBuiltinReturnsStr) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 1, 0);
  frame->setLocal(0, runtime.typeAt(LayoutId::kObject));
  RawObject result = builtinTypeRepr(thread, frame, 1);
  ASSERT_TRUE(result->isStr());
  EXPECT_PYSTRING_EQ(RawStr::cast(result), "<class 'object'>");
}

TEST(TypeBuiltinTest, DunderReprForUserDefinedTypeReturnsStr) {
  Runtime runtime;
  runtime.runFromCStr(R"(
class Foo:
  pass
)");
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object type(&scope, moduleAt(&runtime, main, "Foo"));

  Frame* frame = thread->openAndLinkFrame(0, 1, 0);
  frame->setLocal(0, *type);
  RawObject result = builtinTypeRepr(thread, frame, 1);
  ASSERT_TRUE(result->isStr());
  // TODO(T32810595): Once module names are supported, this should become
  // "<class '__main__.Foo'>".
  EXPECT_PYSTRING_EQ(RawStr::cast(result), "<class 'Foo'>");
}

TEST(TypeBuiltinTest, DunderNewWithOneArgReturnsTypeOfArg) {
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr(R"(
a = type.__new__(type, 1);
b = type.__new__(type, "hello");
)");
  Module main(&scope, findModule(&runtime, "__main__"));
  Type a(&scope, moduleAt(&runtime, main, "a"));
  Type b(&scope, moduleAt(&runtime, main, "b"));

  EXPECT_EQ(RawLayout::cast(a->instanceLayout())->id(), LayoutId::kSmallInt);
  EXPECT_EQ(RawLayout::cast(b->instanceLayout())->id(), LayoutId::kSmallStr);
}

TEST(TypeBuiltinTest, DunderNewWithOneMetaclassArgReturnsType) {
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr(R"(
class Foo(type):
  pass
a = type.__new__(type, Foo);
)");
  Module main(&scope, findModule(&runtime, "__main__"));
  Type a(&scope, moduleAt(&runtime, main, "a"));
  EXPECT_EQ(RawLayout::cast(a->instanceLayout())->id(), LayoutId::kType);
}

}  // namespace python
