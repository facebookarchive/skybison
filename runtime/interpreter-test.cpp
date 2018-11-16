#include "gtest/gtest.h"

#include "bytecode.h"
#include "frame.h"
#include "handles.h"
#include "interpreter.h"
#include "objects.h"
#include "runtime.h"

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

} // namespace python
