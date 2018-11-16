#include "gtest/gtest.h"

#include "bytecode.h"
#include "frame.h"
#include "handles.h"
#include "interpreter.h"
#include "objects.h"
#include "runtime.h"

#include "test-utils.h"

namespace python {

TEST(InterpreterTest, IsTrueBoolean) {
  Runtime runtime;
  HandleScope scope;

  Thread* thread = Thread::currentThread();
  Frame* frame = thread->currentFrame();

  ASSERT_TRUE(frame->isSentinelFrame());

  Object** sp = frame->valueStackTop();
  sp--;

  Handle<Object> true_value(&scope, Boolean::trueObj());
  *sp = *true_value;
  EXPECT_EQ(Interpreter::isTrue(thread, frame, sp), Boolean::trueObj());

  Handle<Object> false_object(&scope, Boolean::falseObj());
  *sp = *false_object;
  EXPECT_EQ(Interpreter::isTrue(thread, frame, sp), Boolean::falseObj());
}

TEST(InterpreterTest, IsTrueInteger) {
  Runtime runtime;
  HandleScope scope;

  Thread* thread = Thread::currentThread();
  Frame* frame = thread->currentFrame();

  ASSERT_TRUE(frame->isSentinelFrame());

  Object** sp = frame->valueStackTop();

  Handle<Object> true_value(&scope, runtime.newInteger(1234));
  *sp = *true_value;
  EXPECT_EQ(Interpreter::isTrue(thread, frame, sp), Boolean::trueObj());

  Handle<Object> false_value(&scope, runtime.newInteger(0));
  *sp = *false_value;
  EXPECT_EQ(Interpreter::isTrue(thread, frame, sp), Boolean::falseObj());
}

TEST(InterpreterTest, IsTrueDunderLen) {
  Runtime runtime;
  HandleScope scope;

  Thread* thread = Thread::currentThread();
  Frame* frame = thread->currentFrame();

  ASSERT_TRUE(frame->isSentinelFrame());

  Object** sp = frame->valueStackTop();

  Handle<List> nonempty_list(&scope, runtime.newList());
  Handle<Object> elt(&scope, None::object());
  runtime.listAdd(nonempty_list, elt);

  Handle<Object> true_value(&scope, *nonempty_list);
  *sp = *true_value;
  EXPECT_EQ(Interpreter::isTrue(thread, frame, sp), Boolean::trueObj());

  Handle<List> empty_list(&scope, runtime.newList());
  Handle<Object> false_value(&scope, *empty_list);
  *sp = *false_value;
  EXPECT_EQ(Interpreter::isTrue(thread, frame, sp), Boolean::falseObj());
}

// To a rich comparison on two instances of the same type.  In each case, the
// method on the left side of the comparison should be used.
TEST(InterpreterTest, CompareOpSameClass) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCString(R"(
class C:
    def __init__(self, value):
        self.value = value

    def __lt__(self, other):
        return self.value < other.value

c10 = C(10)
c20 = C(20)
)");

  Thread* thread = Thread::currentThread();
  Frame* frame = thread->currentFrame();

  ASSERT_TRUE(frame->isSentinelFrame());

  Handle<Module> main(&scope, testing::findModule(&runtime, "__main__"));
  Handle<Object> left(&scope, testing::moduleAt(&runtime, main, "c10"));
  Handle<Object> right(&scope, testing::moduleAt(&runtime, main, "c20"));

  Object* left_lt_right = Interpreter::compareOperation(
      thread, frame, frame->valueStackTop(), CompareOp::LT, left, right);
  EXPECT_EQ(left_lt_right, Boolean::trueObj());

  Object* right_lt_left = Interpreter::compareOperation(
      thread, frame, frame->valueStackTop(), CompareOp::LT, right, left);
  EXPECT_EQ(right_lt_left, Boolean::falseObj());
}

TEST(InterpreterTest, CompareOpFallback) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCString(R"(
class C:
    def __init__(self, value):
        self.value = value

c10 = C(10)
c20 = C(20)
)");

  Thread* thread = Thread::currentThread();
  Frame* frame = thread->currentFrame();

  ASSERT_TRUE(frame->isSentinelFrame());

  Handle<Module> main(&scope, testing::findModule(&runtime, "__main__"));
  Handle<Object> left(&scope, testing::moduleAt(&runtime, main, "c10"));
  Handle<Object> right(&scope, testing::moduleAt(&runtime, main, "c20"));

  Object* left_eq_right = Interpreter::compareOperation(
      thread, frame, frame->valueStackTop(), CompareOp::EQ, left, right);
  EXPECT_EQ(left_eq_right, Boolean::falseObj());
  Object* left_ne_right = Interpreter::compareOperation(
      thread, frame, frame->valueStackTop(), CompareOp::NE, left, right);
  EXPECT_EQ(left_ne_right, Boolean::trueObj());

  Object* right_eq_left = Interpreter::compareOperation(
      thread, frame, frame->valueStackTop(), CompareOp::EQ, left, right);
  EXPECT_EQ(right_eq_left, Boolean::falseObj());
  Object* right_ne_left = Interpreter::compareOperation(
      thread, frame, frame->valueStackTop(), CompareOp::NE, left, right);
  EXPECT_EQ(right_ne_left, Boolean::trueObj());
}

} // namespace python
