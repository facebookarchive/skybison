#include "gtest/gtest.h"

#include "bool-builtins.h"
#include "builtins-module.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(BoolBuiltinsTest, NewFromNonZeroIntegerReturnsTrue) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);
  frame->setLocal(0, runtime.typeAt(LayoutId::kBool));
  frame->setLocal(1, SmallInt::fromWord(2));
  Object* result = builtinBoolNew(thread, frame, 2);
  EXPECT_TRUE(Bool::cast(result)->value());
}

TEST(BoolBuiltinsTest, NewFromZerorReturnsFalse) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);
  frame->setLocal(0, runtime.typeAt(LayoutId::kBool));
  frame->setLocal(1, SmallInt::fromWord(0));
  Object* result = builtinBoolNew(thread, frame, 2);
  EXPECT_FALSE(Bool::cast(result)->value());
}

TEST(BoolBuiltinsTest, NewFromTrueReturnsTrue) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);
  frame->setLocal(0, runtime.typeAt(LayoutId::kBool));
  frame->setLocal(1, Bool::trueObj());
  Object* result = builtinBoolNew(thread, frame, 2);
  EXPECT_TRUE(Bool::cast(result)->value());
  thread->popFrame();
}

TEST(BoolBuiltinsTest, NewFromFalseReturnsTrue) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);
  frame->setLocal(0, runtime.typeAt(LayoutId::kBool));
  frame->setLocal(1, Bool::falseObj());
  Object* result = builtinBoolNew(thread, frame, 2);
  EXPECT_FALSE(Bool::cast(result)->value());
  thread->popFrame();
}

TEST(BoolBuiltinsTest, NewFromNoneIsFalse) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();

  Frame* frame = thread->openAndLinkFrame(0, 2, 0);
  frame->setLocal(0, runtime.typeAt(LayoutId::kBool));
  frame->setLocal(1, None::object());
  Object* result = builtinBoolNew(thread, frame, 2);
  EXPECT_FALSE(Bool::cast(result)->value());
  thread->popFrame();
}

TEST(BoolBuiltinsTest, NewFromUserDefinedType) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  runtime.runFromCString(R"(
class Foo:
  def __bool__(self):
    return True

class Bar:
  def __bool__(self):
    return False

foo = Foo()
bar = Bar()
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> foo(&scope, moduleAt(&runtime, main, "foo"));
  Handle<Object> bar(&scope, moduleAt(&runtime, main, "bar"));

  {
    Frame* frame = thread->openAndLinkFrame(0, 2, 0);
    frame->setLocal(0, runtime.typeAt(LayoutId::kBool));
    frame->setLocal(1, *foo);
    Object* result = builtinBoolNew(thread, frame, 2);
    EXPECT_TRUE(Bool::cast(result)->value());
    thread->popFrame();
  }
  {
    Frame* frame = thread->openAndLinkFrame(0, 2, 0);
    frame->setLocal(0, runtime.typeAt(LayoutId::kBool));
    frame->setLocal(1, *bar);
    Object* result = builtinBoolNew(thread, frame, 2);
    EXPECT_FALSE(Bool::cast(result)->value());
    thread->popFrame();
  }
}

}  // namespace python
