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

  runtime.runFromCString(src);

  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Type> type(&scope, moduleAt(&runtime, main, "C"));
  ASSERT_FALSE(type->isError());
  Handle<Object> instance(&scope, moduleAt(&runtime, main, "c"));
  ASSERT_FALSE(instance->isError());
  Handle<Object> instance_type(&scope, runtime.typeOf(*instance));
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
  ASSERT_TRUE(global->isSmallInt());
  EXPECT_EQ(SmallInt::cast(*global)->value(), 2);
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
  ASSERT_TRUE(global->isSmallInt());
  EXPECT_EQ(SmallInt::cast(*global)->value(), 9);
}

TEST(TypeBuiltinsTest, BuiltinTypeCallDetectNonClsArgRaiseException) {
  Runtime runtime;
  HandleScope scope;
  Handle<Code> code(&scope, runtime.newCode());
  code->setArgcount(1);
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->pushFrame(*code);
  frame->pushValue(runtime.newStringFromCString("not_a_cls"));
  Object* result = builtinTypeCall(thread, frame, 1);
  ASSERT_TRUE(result->isError());
  // TODO(rkng): validate TypeError is thrown
  ASSERT_FALSE(thread->pendingException()->isNone());
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
  runtime.runFromCString(src);
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> c(&scope, moduleAt(&runtime, main, "c"));
  Thread* thread = Thread::currentThread();
  Handle<Object> x(&scope, runtime.newStringFromCString("x"));
  Object* attr = runtime.attributeAt(thread, c, x);
  ASSERT_FALSE(attr->isNone());
  ASSERT_TRUE(attr->isInt());
  ASSERT_EQ(SmallInt::cast(attr)->value(), 42);
}

TEST(TypeBuiltinTest, DunderReprForBuiltinReturnsString) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 1, 0);
  frame->setLocal(0, runtime.typeAt(LayoutId::kObject));
  Object* result = builtinTypeRepr(thread, frame, 1);
  ASSERT_TRUE(result->isString());
  EXPECT_PYSTRING_EQ(String::cast(result), "<class 'object'>");
}

TEST(TypeBuiltinTest, DunderReprForUserDefinedTypeReturnsString) {
  Runtime runtime;
  runtime.runFromCString(R"(
class Foo:
  pass
)");
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> type(&scope, moduleAt(&runtime, main, "Foo"));

  Frame* frame = thread->openAndLinkFrame(0, 1, 0);
  frame->setLocal(0, *type);
  Object* result = builtinTypeRepr(thread, frame, 1);
  ASSERT_TRUE(result->isString());
  // TODO(T32810595): Once module names are supported, this should become
  // "<class '__main__.Foo'>".
  EXPECT_PYSTRING_EQ(String::cast(result), "<class 'Foo'>");
}

TEST(TypeBuiltinTest, DunderNewWithOneArgReturnsTypeOfArg) {
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCString(R"(
a = type.__new__(type, 1);
b = type.__new__(type, "hello");
)");
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Type> a(&scope, moduleAt(&runtime, main, "a"));
  Handle<Type> b(&scope, moduleAt(&runtime, main, "b"));

  EXPECT_EQ(Layout::cast(a->instanceLayout())->id(), LayoutId::kSmallInt);
  EXPECT_EQ(Layout::cast(b->instanceLayout())->id(), LayoutId::kSmallStr);
}

TEST(TypeBuiltinTest, DunderNewWithOneMetaclassArgReturnsType) {
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCString(R"(
class Foo(type):
  pass
a = type.__new__(type, Foo);
)");
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Type> a(&scope, moduleAt(&runtime, main, "a"));
  EXPECT_EQ(Layout::cast(a->instanceLayout())->id(), LayoutId::kType);
}

}  // namespace python
