#include "gtest/gtest.h"

#include <memory>

#include "bytecode.h"
#include "frame.h"
#include "handles.h"
#include "interpreter.h"
#include "objects.h"
#include "runtime.h"
#include "trampolines.h"

#include "test-utils.h"

namespace python {
using namespace testing;

TEST(InterpreterTest, IsTrueBool) {
  Runtime runtime;
  HandleScope scope;

  Thread* thread = Thread::currentThread();
  Frame* frame = thread->currentFrame();

  ASSERT_TRUE(frame->isSentinelFrame());

  Object true_value(&scope, Bool::trueObj());
  EXPECT_EQ(Interpreter::isTrue(thread, frame, true_value), Bool::trueObj());

  Object false_object(&scope, Bool::falseObj());
  EXPECT_EQ(Interpreter::isTrue(thread, frame, false_object), Bool::falseObj());
}

TEST(InterpreterTest, IsTrueInt) {
  Runtime runtime;
  HandleScope scope;

  Thread* thread = Thread::currentThread();
  Frame* frame = thread->currentFrame();

  ASSERT_TRUE(frame->isSentinelFrame());

  Object true_value(&scope, runtime.newInt(1234));
  EXPECT_EQ(Interpreter::isTrue(thread, frame, true_value), Bool::trueObj());

  Object false_value(&scope, runtime.newInt(0));
  EXPECT_EQ(Interpreter::isTrue(thread, frame, false_value), Bool::falseObj());
}

TEST(InterpreterTest, IsTrueWithDunderBoolRaisingPropagatesException) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->currentFrame();
  runFromCStr(&runtime, R"(
class Foo:
  def __bool__(self):
    raise UserWarning('')
value = Foo()
)");
  Object value(&scope, moduleAt(&runtime, "__main__", "value"));
  Object result(&scope, Interpreter::isTrue(thread, frame, value));
  EXPECT_TRUE(raised(*result, LayoutId::kUserWarning));
}

TEST(InterpreterTest, IsTrueWithDunderLenRaisingPropagatesException) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->currentFrame();
  runFromCStr(&runtime, R"(
class Foo:
  def __len__(self):
    raise UserWarning('')
value = Foo()
)");
  Object value(&scope, moduleAt(&runtime, "__main__", "value"));
  Object result(&scope, Interpreter::isTrue(thread, frame, value));
  EXPECT_TRUE(raised(*result, LayoutId::kUserWarning));
}

TEST(InterpreterTest, IsTrueDunderLen) {
  Runtime runtime;
  HandleScope scope;

  Thread* thread = Thread::currentThread();
  Frame* frame = thread->currentFrame();

  ASSERT_TRUE(frame->isSentinelFrame());

  List nonempty_list(&scope, runtime.newList());
  Object elt(&scope, NoneType::object());
  runtime.listAdd(nonempty_list, elt);

  EXPECT_EQ(Interpreter::isTrue(thread, frame, nonempty_list), Bool::trueObj());

  List empty_list(&scope, runtime.newList());
  EXPECT_EQ(Interpreter::isTrue(thread, frame, empty_list), Bool::falseObj());
}

TEST(InterpreterTest, UnaryNotWithRaisingDunderBool) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
class C:
  def __bool__(self):
    raise RuntimeError("too cool for bool")

not C()
)"),
                            LayoutId::kRuntimeError, "too cool for bool"));
}

TEST(InterpreterTest, BinaryOpInvokesSelfMethod) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
class C:
    def __sub__(self, other):
        return (C, '__sub__', self, other)

left = C()
right = C()
)");

  Thread* thread = Thread::currentThread();
  Frame* frame = thread->currentFrame();

  Module main(&scope, findModule(&runtime, "__main__"));
  Object left(&scope, moduleAt(&runtime, main, "left"));
  Object right(&scope, moduleAt(&runtime, main, "right"));
  Object c_class(&scope, moduleAt(&runtime, main, "C"));

  RawObject result = Interpreter::binaryOperation(
      thread, frame, Interpreter::BinaryOp::SUB, left, right);
  ASSERT_TRUE(result->isTuple());
  ASSERT_EQ(RawTuple::cast(result)->length(), 4);
  EXPECT_EQ(RawTuple::cast(result)->at(0), *c_class);
  ASSERT_TRUE(RawTuple::cast(result)->at(1)->isStr());
  RawStr name = RawStr::cast(RawTuple::cast(result)->at(1));
  EXPECT_TRUE(name->equalsCStr("__sub__"));
  EXPECT_EQ(RawTuple::cast(result)->at(2), *left);
  EXPECT_EQ(RawTuple::cast(result)->at(3), *right);
}

TEST(InterpreterTest, BinaryOpInvokesSelfMethodIgnoresReflectedMethod) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
class C:
    def __sub__(self, other):
        return (C, '__sub__', self, other)
    def __rsub__(self, other):
        return (C, '__rsub__', self, other)

left = C()
right = C()
)");

  Thread* thread = Thread::currentThread();
  Frame* frame = thread->currentFrame();

  Module main(&scope, findModule(&runtime, "__main__"));
  Object left(&scope, moduleAt(&runtime, main, "left"));
  Object right(&scope, moduleAt(&runtime, main, "right"));
  Object c_class(&scope, moduleAt(&runtime, main, "C"));

  RawObject result = Interpreter::binaryOperation(
      thread, frame, Interpreter::BinaryOp::SUB, left, right);
  ASSERT_TRUE(result->isTuple());
  ASSERT_EQ(RawTuple::cast(result)->length(), 4);
  EXPECT_EQ(RawTuple::cast(result)->at(0), *c_class);
  ASSERT_TRUE(RawTuple::cast(result)->at(1)->isStr());
  RawStr name = RawStr::cast(RawTuple::cast(result)->at(1));
  EXPECT_TRUE(name->equalsCStr("__sub__"));
  EXPECT_EQ(RawTuple::cast(result)->at(2), *left);
  EXPECT_EQ(RawTuple::cast(result)->at(3), *right);
}

TEST(InterpreterTest, BinaryOperationInvokesSubclassReflectedMethod) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
class C:
    def __sub__(self, other):
        return (C, '__sub__', self, other)

class D(C):
    def __rsub__(self, other):
        return (D, '__rsub__', self, other)

left = C()
right = D()
)");

  Thread* thread = Thread::currentThread();
  Frame* frame = thread->currentFrame();

  Module main(&scope, findModule(&runtime, "__main__"));
  Object left(&scope, moduleAt(&runtime, main, "left"));
  Object right(&scope, moduleAt(&runtime, main, "right"));
  Object d_class(&scope, moduleAt(&runtime, main, "D"));

  RawObject result = Interpreter::binaryOperation(
      thread, frame, Interpreter::BinaryOp::SUB, left, right);
  ASSERT_TRUE(result->isTuple());
  ASSERT_EQ(RawTuple::cast(result)->length(), 4);
  EXPECT_EQ(RawTuple::cast(result)->at(0), *d_class);
  EXPECT_TRUE(
      RawStr::cast(RawTuple::cast(result)->at(1))->equalsCStr("__rsub__"));
  EXPECT_EQ(RawTuple::cast(result)->at(2), *right);
  EXPECT_EQ(RawTuple::cast(result)->at(3), *left);
}

TEST(InterpreterTest, BinaryOperationInvokesOtherReflectedMethod) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
class C:
    pass

class D:
    def __rsub__(self, other):
        return (D, '__rsub__', self, other)

left = C()
right = D()
)");

  Thread* thread = Thread::currentThread();
  Frame* frame = thread->currentFrame();

  Module main(&scope, findModule(&runtime, "__main__"));
  Object left(&scope, moduleAt(&runtime, main, "left"));
  Object right(&scope, moduleAt(&runtime, main, "right"));
  Object d_class(&scope, moduleAt(&runtime, main, "D"));

  RawObject result = Interpreter::binaryOperation(
      thread, frame, Interpreter::BinaryOp::SUB, left, right);
  ASSERT_TRUE(result->isTuple());
  ASSERT_EQ(RawTuple::cast(result)->length(), 4);
  EXPECT_EQ(RawTuple::cast(result)->at(0), *d_class);
  EXPECT_TRUE(
      RawStr::cast(RawTuple::cast(result)->at(1))->equalsCStr("__rsub__"));
  EXPECT_EQ(RawTuple::cast(result)->at(2), *right);
  EXPECT_EQ(RawTuple::cast(result)->at(3), *left);
}

TEST(InterpreterTest, InplaceOperationCallsInplaceMethod) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
class C:
    def __isub__(self, other):
        return (C, '__isub__', self, other)

left = C()
right = C()
)");

  Thread* thread = Thread::currentThread();
  Frame* frame = thread->currentFrame();

  Module main(&scope, findModule(&runtime, "__main__"));
  Object left(&scope, moduleAt(&runtime, main, "left"));
  Object right(&scope, moduleAt(&runtime, main, "right"));
  Object c_class(&scope, moduleAt(&runtime, main, "C"));

  RawObject result = Interpreter::inplaceOperation(
      thread, frame, Interpreter::BinaryOp::SUB, left, right);
  ASSERT_TRUE(result->isTuple());
  ASSERT_EQ(RawTuple::cast(result)->length(), 4);
  EXPECT_EQ(RawTuple::cast(result)->at(0), *c_class);
  EXPECT_TRUE(
      RawStr::cast(RawTuple::cast(result)->at(1))->equalsCStr("__isub__"));
  EXPECT_EQ(RawTuple::cast(result)->at(2), *left);
  EXPECT_EQ(RawTuple::cast(result)->at(3), *right);
}

TEST(InterpreterTest, InplaceOperationCallsBinaryMethod) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
class C:
    def __sub__(self, other):
        return (C, '__sub__', self, other)

left = C()
right = C()
)");

  Thread* thread = Thread::currentThread();
  Frame* frame = thread->currentFrame();

  Module main(&scope, findModule(&runtime, "__main__"));
  Object left(&scope, moduleAt(&runtime, main, "left"));
  Object right(&scope, moduleAt(&runtime, main, "right"));
  Object c_class(&scope, moduleAt(&runtime, main, "C"));

  RawObject result = Interpreter::inplaceOperation(
      thread, frame, Interpreter::BinaryOp::SUB, left, right);
  ASSERT_TRUE(result->isTuple());
  ASSERT_EQ(RawTuple::cast(result)->length(), 4);
  EXPECT_EQ(RawTuple::cast(result)->at(0), *c_class);
  EXPECT_TRUE(
      RawStr::cast(RawTuple::cast(result)->at(1))->equalsCStr("__sub__"));
  EXPECT_EQ(RawTuple::cast(result)->at(2), *left);
  EXPECT_EQ(RawTuple::cast(result)->at(3), *right);
}

TEST(InterpreterTest, InplaceOperationCallsBinaryMethodAfterNotImplemented) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
class C:
    def __isub__(self, other):
        return NotImplemented
    def __sub__(self, other):
        return (C, '__sub__', self, other)

left = C()
right = C()
)");

  Thread* thread = Thread::currentThread();
  Frame* frame = thread->currentFrame();

  Module main(&scope, findModule(&runtime, "__main__"));
  Object left(&scope, moduleAt(&runtime, main, "left"));
  Object right(&scope, moduleAt(&runtime, main, "right"));
  Object c_class(&scope, moduleAt(&runtime, main, "C"));

  RawObject result = Interpreter::inplaceOperation(
      thread, frame, Interpreter::BinaryOp::SUB, left, right);
  ASSERT_TRUE(result->isTuple());
  ASSERT_EQ(RawTuple::cast(result)->length(), 4);
  EXPECT_EQ(RawTuple::cast(result)->at(0), *c_class);
  EXPECT_TRUE(
      RawStr::cast(RawTuple::cast(result)->at(1))->equalsCStr("__sub__"));
  EXPECT_EQ(RawTuple::cast(result)->at(2), *left);
  EXPECT_EQ(RawTuple::cast(result)->at(3), *right);
}

// To a rich comparison on two instances of the same type.  In each case, the
// method on the left side of the comparison should be used.
TEST(InterpreterTest, CompareOpSameType) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
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

  Module main(&scope, findModule(&runtime, "__main__"));
  Object left(&scope, moduleAt(&runtime, main, "c10"));
  Object right(&scope, moduleAt(&runtime, main, "c20"));

  RawObject left_lt_right =
      Interpreter::compareOperation(thread, frame, CompareOp::LT, left, right);
  EXPECT_EQ(left_lt_right, Bool::trueObj());

  RawObject right_lt_left =
      Interpreter::compareOperation(thread, frame, CompareOp::LT, right, left);
  EXPECT_EQ(right_lt_left, Bool::falseObj());
}

TEST(InterpreterTest, CompareOpFallback) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
class C:
    def __init__(self, value):
        self.value = value

c10 = C(10)
c20 = C(20)
)");

  Thread* thread = Thread::currentThread();
  Frame* frame = thread->currentFrame();

  ASSERT_TRUE(frame->isSentinelFrame());

  Module main(&scope, findModule(&runtime, "__main__"));
  Object left(&scope, moduleAt(&runtime, main, "c10"));
  Object right(&scope, moduleAt(&runtime, main, "c20"));

  RawObject left_eq_right =
      Interpreter::compareOperation(thread, frame, CompareOp::EQ, left, right);
  EXPECT_EQ(left_eq_right, Bool::falseObj());
  RawObject left_ne_right =
      Interpreter::compareOperation(thread, frame, CompareOp::NE, left, right);
  EXPECT_EQ(left_ne_right, Bool::trueObj());

  RawObject right_eq_left =
      Interpreter::compareOperation(thread, frame, CompareOp::EQ, left, right);
  EXPECT_EQ(right_eq_left, Bool::falseObj());
  RawObject right_ne_left =
      Interpreter::compareOperation(thread, frame, CompareOp::NE, left, right);
  EXPECT_EQ(right_ne_left, Bool::trueObj());
}

TEST(InterpreterTest, CompareOpSubclass) {
  using namespace testing;

  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
called = None
class A:
  def __eq__(self, other):
    global called
    if (called is not None):
      called = "ERROR"
    else:
      called = "A"
    return False

class B:
  def __eq__(self, other):
    global called
    if (called is not None):
      called = "ERROR"
    else:
      called = "B"
    return True

class C(A):
  def __eq__(self, other):
    global called
    if (called is not None):
      called = "ERROR"
    else:
      called = "C"
    return True

a = A()
b = B()
c = C()
)");

  Thread* thread = Thread::currentThread();
  Frame* frame = thread->currentFrame();
  ASSERT_TRUE(frame->isSentinelFrame());

  Module main(&scope, findModule(&runtime, "__main__"));
  Object a(&scope, moduleAt(&runtime, main, "a"));
  Object b(&scope, moduleAt(&runtime, main, "b"));
  Object c(&scope, moduleAt(&runtime, main, "c"));

  // Comparisons where rhs is not a subtype of lhs try lhs.__eq__(rhs) first.
  RawObject a_eq_b =
      Interpreter::compareOperation(thread, frame, CompareOp::EQ, a, b);
  EXPECT_EQ(a_eq_b, Bool::falseObj());
  Object called(&scope, moduleAt(&runtime, main, "called"));
  EXPECT_TRUE(isStrEqualsCStr(*called, "A"));

  Str called_name(&scope, runtime.newStrFromCStr("called"));
  Object none(&scope, NoneType::object());
  runtime.moduleAtPut(main, called_name, none);
  RawObject b_eq_a =
      Interpreter::compareOperation(thread, frame, CompareOp::EQ, b, a);
  EXPECT_EQ(b_eq_a, Bool::trueObj());
  called = moduleAt(&runtime, main, "called");
  EXPECT_TRUE(isStrEqualsCStr(*called, "B"));

  runtime.moduleAtPut(main, called_name, none);
  RawObject c_eq_a =
      Interpreter::compareOperation(thread, frame, CompareOp::EQ, c, a);
  EXPECT_EQ(c_eq_a, Bool::trueObj());
  called = moduleAt(&runtime, main, "called");
  EXPECT_TRUE(isStrEqualsCStr(*called, "C"));

  // When rhs is a subtype of lhs, only rhs.__eq__(rhs) is tried.
  runtime.moduleAtPut(main, called_name, none);
  RawObject a_eq_c =
      Interpreter::compareOperation(thread, frame, CompareOp::EQ, a, c);
  EXPECT_EQ(a_eq_c, Bool::trueObj());
  called = moduleAt(&runtime, main, "called");
  EXPECT_TRUE(isStrEqualsCStr(*called, "C"));
}

TEST(InterpreterTest, SequenceContains) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
a = {1, 2}

b = 1
c = 3
)");

  Thread* thread = Thread::currentThread();
  Frame* frame = thread->currentFrame();

  ASSERT_TRUE(frame->isSentinelFrame());
  Module main(&scope, findModule(&runtime, "__main__"));
  Object container(&scope, moduleAt(&runtime, main, "a"));
  Object b(&scope, moduleAt(&runtime, main, "b"));
  Object c(&scope, moduleAt(&runtime, main, "c"));
  RawObject contains_true =
      Interpreter::sequenceContains(thread, frame, b, container);
  RawObject contains_false =
      Interpreter::sequenceContains(thread, frame, c, container);
  EXPECT_EQ(contains_true, Bool::trueObj());
  EXPECT_EQ(contains_false, Bool::falseObj());
}

TEST(InterpreterTest, SequenceIterSearchWithNoDunderIterRaisesTypeError) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
class C: pass
container = C()
)");
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  Object container(&scope, moduleAt(&runtime, "__main__", "container"));
  Object val(&scope, NoneType::object());
  Object result(&scope,
                Interpreter::sequenceIterSearch(thread, frame, val, container));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(InterpreterTest,
     SequenceIterSearchWithNonCallableDunderIterRaisesTypeError) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
class C:
  __iter__ = None
container = C()
)");
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  Object container(&scope, moduleAt(&runtime, "__main__", "container"));
  Object val(&scope, NoneType::object());
  Object result(&scope,
                Interpreter::sequenceIterSearch(thread, frame, val, container));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(InterpreterTest, SequenceIterSearchWithNoDunderNextRaisesTypeError) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
class D: pass
class C:
  def __iter__(self):
    return D()
container = C()
)");
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  Object container(&scope, moduleAt(&runtime, "__main__", "container"));
  Object val(&scope, NoneType::object());
  Object result(&scope,
                Interpreter::sequenceIterSearch(thread, frame, val, container));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(InterpreterTest,
     SequenceIterSearchWithNonCallableDunderNextRaisesTypeError) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
class D:
  __next__ = None
class C:
  def __iter__(self):
    return D()
container = C()
)");
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  Object container(&scope, moduleAt(&runtime, "__main__", "container"));
  Object val(&scope, NoneType::object());
  Object result(&scope,
                Interpreter::sequenceIterSearch(thread, frame, val, container));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(InterpreterTest, SequenceIterSearchWithListReturnsTrue) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  List container(&scope, listFromRange(1, 3));
  Object val(&scope, SmallInt::fromWord(2));
  Object result(&scope,
                Interpreter::sequenceIterSearch(thread, frame, val, container));
  ASSERT_FALSE(result->isError());
  EXPECT_EQ(result, Bool::trueObj());
}

TEST(InterpreterTest, SequenceIterSearchWithListReturnsFalse) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Object container(&scope, listFromRange(1, 3));
  Object val(&scope, SmallInt::fromWord(5));
  Frame* frame = thread->currentFrame();
  Object result(&scope,
                Interpreter::sequenceIterSearch(thread, frame, val, container));
  ASSERT_FALSE(result->isError());
  EXPECT_EQ(result, Bool::falseObj());
}

TEST(InterpreterTest, SequenceIterSearchWithIterThatRaisesPropagatesException) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
class C:
  def __iter__(self):
    raise ZeroDivisionError("boom")
container = C()
)");
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Object container(&scope, moduleAt(&runtime, "__main__", "container"));
  Object val(&scope, SmallInt::fromWord(5));
  Frame* frame = thread->currentFrame();
  Object result(&scope,
                Interpreter::sequenceIterSearch(thread, frame, val, container));
  EXPECT_TRUE(raised(*result, LayoutId::kZeroDivisionError));
}

TEST(InterpreterTest, ContextManagerCallEnterExit) {
  const char* src = R"(
a = 1
class Foo:
  def __enter__(self):
    global a
    a = 2

  def __exit__(self, e, t, b):
    global a
    a = 3

b = 0
with Foo():
  b = a

)";
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object a(&scope, moduleAt(&runtime, main, "a"));
  EXPECT_EQ(RawSmallInt::cast(*a)->value(), 3);
  Object b(&scope, moduleAt(&runtime, main, "b"));
  EXPECT_EQ(RawSmallInt::cast(*b)->value(), 2);
}

TEST(InterpreterTest, StackCleanupAfterCallFunction) {
  // Build the following function
  //    def foo(a=1, b=2):
  //      return 42
  //
  // Then call as foo(1) and verify that the stack is cleaned up after
  // default argument expansion
  //
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();

  Code code(&scope, testing::newEmptyCode(&runtime));

  Tuple consts(&scope, runtime.newTuple(1));
  consts->atPut(0, SmallInt::fromWord(42));
  code->setConsts(*consts);

  Tuple names(&scope, runtime.newTuple(1));
  Object key(&scope, runtime.newStrFromCStr("foo"));
  names->atPut(0, *key);
  code->setNames(*names);
  code->setArgcount(2);
  code->setStacksize(1);

  const byte bytecode[] = {LOAD_CONST, 0, RETURN_VALUE, 0};
  code->setCode(runtime.newBytesWithAll(bytecode));

  Function callee(&scope, runtime.newFunction());
  callee->setCode(*code);
  callee->setEntry(interpreterTrampoline);
  Tuple defaults(&scope, runtime.newTuple(2));

  defaults->atPut(0, SmallInt::fromWord(1));
  defaults->atPut(1, SmallInt::fromWord(2));
  callee->setDefaults(*defaults);

  // Create a caller frame
  Frame* frame = thread->pushFrame(code);

  // Save starting value stack top
  RawObject* value_stack_start = frame->valueStackTop();

  // Push function pointer and argument
  frame->pushValue(*callee);
  frame->pushValue(SmallInt::fromWord(1));

  RawObject result = Interpreter::call(thread, frame, 1);

  // Make sure we got the right result and stack is back where it should be
  EXPECT_EQ(RawSmallInt::cast(result)->value(), 42);
  EXPECT_EQ(value_stack_start, frame->valueStackTop());
}

TEST(InterpreterTest, StackCleanupAfterCallExFunction) {
  // Build the following function
  //    def foo(a=1, b=2):
  //      return 42
  //
  // Then call as "f=(2,); foo(*f)" and verify that the stack is cleaned up
  // after ex and default argument expansion
  //
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();

  Code code(&scope, testing::newEmptyCode(&runtime));

  Tuple consts(&scope, runtime.newTuple(1));
  consts->atPut(0, SmallInt::fromWord(42));
  code->setConsts(*consts);

  Tuple names(&scope, runtime.newTuple(1));
  Object key(&scope, runtime.newStrFromCStr("foo"));
  names->atPut(0, *key);
  code->setNames(*names);
  code->setArgcount(2);
  code->setStacksize(1);

  const byte bytecode[] = {LOAD_CONST, 0, RETURN_VALUE, 0};
  code->setCode(runtime.newBytesWithAll(bytecode));

  Function callee(&scope, runtime.newFunction());
  callee->setCode(*code);
  callee->setEntryEx(interpreterTrampolineEx);
  Tuple defaults(&scope, runtime.newTuple(2));

  defaults->atPut(0, SmallInt::fromWord(1));
  defaults->atPut(1, SmallInt::fromWord(2));
  callee->setDefaults(*defaults);

  // Create a caller frame
  Frame* frame = thread->pushFrame(code);

  // Save starting value stack top
  RawObject* value_stack_start = frame->valueStackTop();

  // Push function pointer and argument
  Tuple ex(&scope, runtime.newTuple(1));
  ex->atPut(0, SmallInt::fromWord(2));
  frame->pushValue(*callee);
  frame->pushValue(*ex);

  RawObject result = Interpreter::callEx(thread, frame, 0);

  // Make sure we got the right result and stack is back where it should be
  EXPECT_EQ(RawSmallInt::cast(result)->value(), 42);
  EXPECT_EQ(value_stack_start, frame->valueStackTop());
}

TEST(InterpreterTest, StackCleanupAfterCallKwFunction) {
  // Build the following function
  //    def foo(a=1, b=2):
  //      return 42
  //
  // Then call as "foo(b=4)" and verify that the stack is cleaned up after
  // ex and default argument expansion
  //
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();

  Code code(&scope, testing::newEmptyCode(&runtime));

  Tuple consts(&scope, runtime.newTuple(1));
  consts->atPut(0, SmallInt::fromWord(42));
  code->setConsts(*consts);

  Tuple names(&scope, runtime.newTuple(1));
  Object key(&scope, runtime.newStrFromCStr("foo"));
  names->atPut(0, *key);
  code->setNames(*names);
  code->setArgcount(2);
  code->setStacksize(1);
  Tuple var_names(&scope, runtime.newTuple(2));
  var_names->atPut(0, runtime.newStrFromCStr("a"));
  var_names->atPut(1, runtime.newStrFromCStr("b"));
  code->setVarnames(*var_names);

  const byte bytecode[] = {LOAD_CONST, 0, RETURN_VALUE, 0};
  code->setCode(runtime.newBytesWithAll(bytecode));

  Function callee(&scope, runtime.newFunction());
  callee->setCode(*code);
  callee->setEntryKw(interpreterTrampolineKw);
  Tuple defaults(&scope, runtime.newTuple(2));

  defaults->atPut(0, SmallInt::fromWord(1));
  defaults->atPut(1, SmallInt::fromWord(2));
  callee->setDefaults(*defaults);

  // Create a caller frame
  Frame* frame = thread->pushFrame(code);

  // Save starting value stack top
  RawObject* value_stack_start = frame->valueStackTop();

  // Push function pointer and argument
  Tuple arg_names(&scope, runtime.newTuple(1));
  arg_names->atPut(0, runtime.newStrFromCStr("b"));
  frame->pushValue(*callee);
  frame->pushValue(SmallInt::fromWord(4));
  frame->pushValue(*arg_names);

  RawObject result = Interpreter::callKw(thread, frame, 1);

  // Make sure we got the right result and stack is back where it should be
  EXPECT_EQ(RawSmallInt::cast(result)->value(), 42);
  EXPECT_EQ(value_stack_start, frame->valueStackTop());
}

TEST(InterpreterTest, LookupMethodInvokesDescriptor) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
def f(): pass

class D:
    def __get__(self, obj, owner):
        return f

class C:
    __call__ = D()

c = C()
  )");
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->currentFrame();
  ASSERT_TRUE(frame->isSentinelFrame());
  Module main(&scope, findModule(&runtime, "__main__"));
  Object c(&scope, moduleAt(&runtime, main, "c"));
  Object f(&scope, moduleAt(&runtime, main, "f"));
  Object method(&scope, Interpreter::lookupMethod(thread, frame, c,
                                                  SymbolId::kDunderCall));
  EXPECT_EQ(*f, *method);
}

TEST(InterpreterTest, CallingUncallableThrowsTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "(1)()"),
                            LayoutId::kTypeError, "object is not callable"));
}

TEST(InterpreterTest, CallingUncallableDunderCallThrowsTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
class C:
  __call__ = 1

c = C()
c()
  )"),
                            LayoutId::kTypeError, "object is not callable"));
}

TEST(InterpreterTest, CallingNonDescriptorDunderCallThrowsTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
class D: pass

class C:
  __call__ = D()

c = C()
c()
  )"),
                            LayoutId::kTypeError, "object is not callable"));
}

TEST(InterpreterTest, CallDescriptorReturningUncallableThrowsTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
class D:
  def __get__(self, instance, owner):
    return 1

class C:
  __call__ = D()

c = C()
c()
  )"),
                            LayoutId::kTypeError, "object is not callable"));
}

TEST(InterpreterTest, LookupMethodLoopsOnCallBoundToDescriptor) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
def f(args):
    return args

class C0:
    def __get__(self, obj, owner):
        return f

class C1:
    __call__ = C0()

class C2:
    def __get__(self, obj, owner):
        return C1()

class C3:
    __call__ = C2()

c = C3()
result = c(42)
  )");
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->currentFrame();
  ASSERT_TRUE(frame->isSentinelFrame());
  Module main(&scope, findModule(&runtime, "__main__"));
  Object result(&scope, moduleAt(&runtime, main, "result"));
  EXPECT_EQ(*result, SmallInt::fromWord(42));
}

TEST(InterpreterTest, IterateOnNonIterable) {
  const char* src = R"(
# Try to iterate on a None object which isn't iterable
a, b = None
)";
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, src), LayoutId::kTypeError,
                            "object is not iterable"));
}

TEST(InterpreterTest, DunderIterReturnsNonIterable) {
  const char* src = R"(
class Foo:
  def __iter__(self):
    return 1
a, b = Foo()
)";
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, src), LayoutId::kTypeError,
                            "iter() returned non-iterator"));
}

TEST(InterpreterTest, UnpackSequence) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
l = [1, 2, 3]
a, b, c = l
)");
  Module main(&scope, findModule(&runtime, "__main__"));
  Object a(&scope, moduleAt(&runtime, main, "a"));
  Object b(&scope, moduleAt(&runtime, main, "b"));
  Object c(&scope, moduleAt(&runtime, main, "c"));
  ASSERT_TRUE(a->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*a)->value(), 1);
  ASSERT_TRUE(b->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*b)->value(), 2);
  ASSERT_TRUE(c->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*c)->value(), 3);
}

TEST(InterpreterTest, UnpackSequenceTooFewObjects) {
  Runtime runtime;
  HandleScope scope;
  const char* src = R"(
l = [1, 2]
a, b, c = l
)";
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, src), LayoutId::kValueError,
                            "not enough values to unpack"));
}

TEST(InterpreterTest, UnpackSequenceTooManyObjects) {
  Runtime runtime;
  HandleScope scope;
  const char* src = R"(
l = [1, 2, 3, 4]
a, b, c = l
)";
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, src), LayoutId::kValueError,
                            "too many values to unpack"));
}

TEST(InterpreterTest, PrintExprInvokesDisplayhook) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
import sys

MY_GLOBAL = 1234

def my_displayhook(value):
  global MY_GLOBAL
  MY_GLOBAL = value

sys.displayhook = my_displayhook
  )");

  Object unique(&scope, runtime.newTuple(1));  // unique object

  Code code(&scope, testing::newEmptyCode(&runtime));
  Tuple consts(&scope, runtime.newTuple(2));
  consts->atPut(0, *unique);
  consts->atPut(1, NoneType::object());
  code->setConsts(*consts);
  code->setNlocals(0);
  const byte bytecode[] = {LOAD_CONST, 0, PRINT_EXPR,   0,
                           LOAD_CONST, 1, RETURN_VALUE, 0};
  code->setCode(runtime.newBytesWithAll(bytecode));

  RawObject result = Thread::currentThread()->run(code);
  ASSERT_TRUE(result->isNoneType());

  Module sys(&scope, findModule(&runtime, "sys"));
  Object displayhook(&scope, moduleAt(&runtime, sys, "displayhook"));
  Module main(&scope, findModule(&runtime, "__main__"));
  Object my_displayhook(&scope, moduleAt(&runtime, main, "my_displayhook"));
  EXPECT_EQ(*displayhook, *my_displayhook);

  Object my_global(&scope, moduleAt(&runtime, main, "MY_GLOBAL"));
  EXPECT_EQ(*my_global, *unique);
}

TEST(InterpreterTest, GetAiterCallsAiter) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
class AsyncIterable:
  def __aiter__(self):
    return 42

a = AsyncIterable()
)");

  Module main(&scope, findModule(&runtime, "__main__"));
  Object a(&scope, moduleAt(&runtime, main, "a"));

  Code code(&scope, testing::newEmptyCode(&runtime));
  Tuple consts(&scope, runtime.newTuple(1));
  consts->atPut(0, *a);
  code->setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, GET_AITER, 0, RETURN_VALUE, 0};
  code->setCode(runtime.newBytesWithAll(bytecode));

  Object result(&scope, Thread::currentThread()->run(code));
  ASSERT_TRUE(result->isSmallInt());
  EXPECT_EQ(42, RawSmallInt::cast(*result)->value());
}

TEST(InterpreterTest, GetAiterOnNonIterable) {
  Runtime runtime;
  HandleScope scope;
  Code code(&scope, testing::newEmptyCode(&runtime));
  Tuple consts(&scope, runtime.newTuple(1));
  consts->atPut(0, SmallInt::fromWord(123));
  code->setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, GET_AITER, 0, RETURN_VALUE, 0};
  code->setCode(runtime.newBytesWithAll(bytecode));

  Object result(&scope, Thread::currentThread()->run(code));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(InterpreterTest, BeforeAsyncWithCallsDunderAenter) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
enter = None
exit = None

class M:
  def __aenter__(self):
    global enter
    enter = self

  def __aexit__(self, exc_type, exc_value, traceback):
    global exit
    exit = self

manager = M()
  )");

  Module main(&scope, findModule(&runtime, "__main__"));

  Code code(&scope, testing::newEmptyCode(&runtime));
  code->setNlocals(0);

  Tuple consts(&scope, runtime.newTuple(1));
  consts->atPut(0, SmallInt::fromWord(42));
  code->setConsts(*consts);

  Tuple names(&scope, runtime.newTuple(1));
  names->atPut(0, runtime.newStrFromCStr("manager"));
  code->setNames(*names);

  const byte bytecode[] = {LOAD_GLOBAL, 0, BEFORE_ASYNC_WITH, 0, POP_TOP, 0,
                           LOAD_CONST,  0, RETURN_VALUE,      0};
  code->setCode(runtime.newBytesWithAll(bytecode));

  Dict globals(&scope, main->dict());
  Dict builtins(&scope, runtime.newDict());
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->pushFrame(code);
  frame->setGlobals(*globals);
  frame->setFastGlobals(runtime.computeFastGlobals(code, globals, builtins));

  Object result(&scope, Interpreter::execute(thread, frame));
  ASSERT_EQ(*result, SmallInt::fromWord(42));

  Object manager(&scope, moduleAt(&runtime, main, "manager"));
  Object enter(&scope, moduleAt(&runtime, main, "enter"));
  EXPECT_EQ(*enter, *manager);

  Object exit(&scope, moduleAt(&runtime, main, "exit"));
  EXPECT_EQ(*exit, NoneType::object());
}

TEST(InterpreterTest, SetupAsyncWithPushesBlock) {
  Runtime runtime;
  HandleScope scope;

  Code code(&scope, testing::newEmptyCode(&runtime));
  Tuple consts(&scope, runtime.newTuple(2));
  consts->atPut(0, SmallInt::fromWord(42));
  consts->atPut(1, NoneType::object());
  code->setConsts(*consts);
  code->setNlocals(0);
  const byte bc[] = {
      LOAD_CONST, 0, LOAD_CONST,   1, SETUP_ASYNC_WITH, 0,
      POP_BLOCK,  0, RETURN_VALUE, 0,
  };
  code->setCode(runtime.newBytesWithAll(bc));
  RawObject result = Thread::currentThread()->run(code);
  EXPECT_EQ(result, SmallInt::fromWord(42));
}

TEST(InterpreterTest, UnpackSequenceEx) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
l = [1, 2, 3, 4, 5, 6, 7]
a, b, c, *d, e, f, g  = l
)");
  Module main(&scope, findModule(&runtime, "__main__"));
  Object a(&scope, moduleAt(&runtime, main, "a"));
  Object b(&scope, moduleAt(&runtime, main, "b"));
  Object c(&scope, moduleAt(&runtime, main, "c"));
  ASSERT_TRUE(a->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*a)->value(), 1);
  ASSERT_TRUE(b->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*b)->value(), 2);
  ASSERT_TRUE(c->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*c)->value(), 3);

  Object d(&scope, moduleAt(&runtime, main, "d"));
  ASSERT_TRUE(d->isList());
  List list(&scope, *d);
  EXPECT_EQ(list->numItems(), 1);
  EXPECT_EQ(RawSmallInt::cast(list->at(0))->value(), 4);

  Object e(&scope, moduleAt(&runtime, main, "e"));
  Object f(&scope, moduleAt(&runtime, main, "f"));
  Object g(&scope, moduleAt(&runtime, main, "g"));
  ASSERT_TRUE(e->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*e)->value(), 5);
  ASSERT_TRUE(f->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*f)->value(), 6);
  ASSERT_TRUE(g->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*g)->value(), 7);
}

TEST(InterpreterTest, UnpackSequenceExWithNoElementsAfter) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
l = [1, 2, 3, 4]
a, b, *c = l
)");
  Module main(&scope, findModule(&runtime, "__main__"));
  Object a(&scope, moduleAt(&runtime, main, "a"));
  Object b(&scope, moduleAt(&runtime, main, "b"));
  Object c(&scope, moduleAt(&runtime, main, "c"));
  ASSERT_TRUE(a->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*a)->value(), 1);
  ASSERT_TRUE(b->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*b)->value(), 2);

  ASSERT_TRUE(c->isList());
  List list(&scope, *c);
  ASSERT_EQ(list->numItems(), 2);
  EXPECT_EQ(RawSmallInt::cast(list->at(0))->value(), 3);
  EXPECT_EQ(RawSmallInt::cast(list->at(1))->value(), 4);
}

TEST(InterpreterTest, UnpackSequenceExWithNoElementsBefore) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
l = [1, 2, 3, 4]
*a, b, c = l
)");
  Module main(&scope, findModule(&runtime, "__main__"));
  Object a(&scope, moduleAt(&runtime, main, "a"));
  Object b(&scope, moduleAt(&runtime, main, "b"));
  Object c(&scope, moduleAt(&runtime, main, "c"));
  ASSERT_TRUE(a->isList());
  List list(&scope, *a);
  ASSERT_EQ(list->numItems(), 2);
  EXPECT_EQ(RawSmallInt::cast(list->at(0))->value(), 1);
  EXPECT_EQ(RawSmallInt::cast(list->at(1))->value(), 2);

  ASSERT_TRUE(b->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*b)->value(), 3);
  ASSERT_TRUE(c->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*c)->value(), 4);
}

TEST(InterpreterTest, BuildMapUnpackWithDict) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
d = {**{'a': 1, 'b': 2}, 'c': 3, **{'d': 4}}
)");

  Module main(&scope, findModule(&runtime, "__main__"));
  Object d(&scope, moduleAt(&runtime, main, "d"));
  ASSERT_TRUE(d->isDict());

  Dict dict(&scope, *d);
  EXPECT_EQ(dict->numItems(), 4);

  Object key(&scope, SmallStr::fromCStr("a"));
  Object el0(&scope, runtime.dictAt(dict, key));
  ASSERT_TRUE(el0->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*el0)->value(), 1);

  key = SmallStr::fromCStr("b");
  Object el1(&scope, runtime.dictAt(dict, key));
  ASSERT_TRUE(el1->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*el1)->value(), 2);

  key = SmallStr::fromCStr("c");
  Object el2(&scope, runtime.dictAt(dict, key));
  ASSERT_TRUE(el2->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*el2)->value(), 3);

  key = SmallStr::fromCStr("d");
  Object el3(&scope, runtime.dictAt(dict, key));
  ASSERT_TRUE(el3->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*el3)->value(), 4);
}

TEST(InterpreterTest, BuildMapUnpackWithListKeysMapping) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
class Foo:
    def __init__(self):
        self.idx = 0
        self._items = [('a', 1), ('b', 2), ('c', 3)]

    def keys(self):
        return [x[0] for x in self._items]

    def __getitem__(self, key):
        for k, v in self._items:
            if key == k:
                return v
        raise KeyError()

d = {**Foo(), 'd': 4}
)");

  Module main(&scope, findModule(&runtime, "__main__"));
  Object d(&scope, moduleAt(&runtime, main, "d"));
  ASSERT_TRUE(d->isDict());

  Dict dict(&scope, *d);
  EXPECT_EQ(dict->numItems(), 4);

  Object key(&scope, SmallStr::fromCStr("a"));
  Object el0(&scope, runtime.dictAt(dict, key));
  ASSERT_TRUE(el0->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*el0)->value(), 1);

  key = SmallStr::fromCStr("b");
  Object el1(&scope, runtime.dictAt(dict, key));
  ASSERT_TRUE(el1->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*el1)->value(), 2);

  key = SmallStr::fromCStr("c");
  Object el2(&scope, runtime.dictAt(dict, key));
  ASSERT_TRUE(el2->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*el2)->value(), 3);

  key = SmallStr::fromCStr("d");
  Object el3(&scope, runtime.dictAt(dict, key));
  ASSERT_TRUE(el3->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*el3)->value(), 4);
}

TEST(InterpreterTest, BuildMapUnpackWithTupleKeysMapping) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
class Foo:
    def __init__(self):
        self.idx = 0
        self._items = [('a', 1), ('b', 2), ('c', 3)]

    def keys(self):
        return ('a', 'b', 'c')

    def __getitem__(self, key):
        for k, v in self._items:
            if key == k:
                return v
        raise KeyError()

d = {**Foo(), 'd': 4}
)");

  Module main(&scope, findModule(&runtime, "__main__"));
  Object d(&scope, moduleAt(&runtime, main, "d"));
  ASSERT_TRUE(d->isDict());

  Dict dict(&scope, *d);
  EXPECT_EQ(dict->numItems(), 4);

  Object key(&scope, SmallStr::fromCStr("a"));
  Object el0(&scope, runtime.dictAt(dict, key));
  ASSERT_TRUE(el0->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*el0)->value(), 1);

  key = SmallStr::fromCStr("b");
  Object el1(&scope, runtime.dictAt(dict, key));
  ASSERT_TRUE(el1->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*el1)->value(), 2);

  key = SmallStr::fromCStr("c");
  Object el2(&scope, runtime.dictAt(dict, key));
  ASSERT_TRUE(el2->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*el2)->value(), 3);

  key = SmallStr::fromCStr("d");
  Object el3(&scope, runtime.dictAt(dict, key));
  ASSERT_TRUE(el3->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*el3)->value(), 4);
}

TEST(InterpreterTest, BuildMapUnpackWithIterableKeysMapping) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
class KeysIter:
    def __init__(self, keys):
        self.idx = 0
        self.keys = keys

    def __iter__(self):
        return self

    def __next__(self):
        if self.idx == len(self.keys):
            raise StopIteration
        r = self.keys[self.idx]
        self.idx += 1
        return r

class Foo:
    def __init__(self):
        self.idx = 0
        self._items = [('a', 1), ('b', 2), ('c', 3)]

    def keys(self):
        return KeysIter([x[0] for x in self._items])

    def __getitem__(self, key):
        for k, v in self._items:
            if key == k:
                return v
        raise KeyError()

d = {**Foo(), 'd': 4}
)");

  Module main(&scope, findModule(&runtime, "__main__"));
  Object d(&scope, moduleAt(&runtime, main, "d"));
  ASSERT_TRUE(d->isDict());

  Dict dict(&scope, *d);
  EXPECT_EQ(dict->numItems(), 4);

  Object key(&scope, SmallStr::fromCStr("a"));
  Object el0(&scope, runtime.dictAt(dict, key));
  ASSERT_TRUE(el0->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*el0)->value(), 1);

  key = SmallStr::fromCStr("b");
  Object el1(&scope, runtime.dictAt(dict, key));
  ASSERT_TRUE(el1->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*el1)->value(), 2);

  key = SmallStr::fromCStr("c");
  Object el2(&scope, runtime.dictAt(dict, key));
  ASSERT_TRUE(el2->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*el2)->value(), 3);

  key = SmallStr::fromCStr("d");
  Object el3(&scope, runtime.dictAt(dict, key));
  ASSERT_TRUE(el3->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*el3)->value(), 4);
}

TEST(InterpreterTest, BuildMapUnpackWithNonMapping) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
class Foo:
    pass

d = {**Foo(), 'd': 4}
  )"),
                            LayoutId::kTypeError, "object is not a mapping"));
}

TEST(InterpreterTest, BuildMapUnpackWithUnsubscriptableMapping) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
class Foo:
    def __init__(self):
        self.idx = 0
        self._items = [('a', 1), ('b', 2), ('c', 3)]

    def keys(self):
        return ('a', 'b', 'c')

d = {**Foo(), 'd': 4}
  )"),
                            LayoutId::kTypeError,
                            "object is not subscriptable"));
}

TEST(InterpreterTest, BuildMapUnpackWithNonIterableKeys) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
class Foo:
    def __init__(self):
        self.idx = 0
        self._items = [('a', 1), ('b', 2), ('c', 3)]

    def keys(self):
        return None

    def __getitem__(self, key):
        pass

d = {**Foo(), 'd': 4}
  )"),
                            LayoutId::kTypeError, "keys() is not iterable"));
}

TEST(InterpreterTest, BuildMapUnpackWithBadIteratorKeys) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
class KeysIter:
    def __iter__(self):
        return self

class Foo:
    def __init__(self):
        pass

    def keys(self):
        return KeysIter()

    def __getitem__(self, key):
        pass

d = {**Foo(), 'd': 4}
  )"),
                            LayoutId::kTypeError, "keys() is not iterable"));
}

TEST(InterpreterTest, UnpackSequenceExWithTooFewObjectsBefore) {
  Runtime runtime;
  HandleScope scope;
  const char* src = R"(
l = [1, 2]
a, b, c, *d  = l
)";
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, src), LayoutId::kValueError,
                            "not enough values to unpack"));
}

TEST(InterpreterTest, UnpackSequenceExWithTooFewObjectsAfter) {
  Runtime runtime;
  HandleScope scope;
  const char* src = R"(
l = [1, 2]
*a, b, c, d = l
)";
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, src), LayoutId::kValueError,
                            "not enough values to unpack"));
}

TEST(InterpreterTest, BuildTupleUnpackWithCall) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
def foo(*args):
    return args

t = foo(*(1,2), *(3, 4))
)");

  Module main(&scope, findModule(&runtime, "__main__"));
  Object t(&scope, moduleAt(&runtime, main, "t"));
  ASSERT_TRUE(t->isTuple());

  Tuple tuple(&scope, *t);
  EXPECT_EQ(RawSmallInt::cast(tuple->at(0))->value(), 1);
  EXPECT_EQ(RawSmallInt::cast(tuple->at(1))->value(), 2);
  EXPECT_EQ(RawSmallInt::cast(tuple->at(2))->value(), 3);
  EXPECT_EQ(RawSmallInt::cast(tuple->at(3))->value(), 4);
}

TEST(InterpreterTest, FunctionDerefsVariable) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
def outer():
    var = 1
    def inner():
        return var
    del var
    return 0

v = outer()
	)");

  Module main(&scope, findModule(&runtime, "__main__"));
  Object v(&scope, moduleAt(&runtime, main, "v"));
  ASSERT_TRUE(v->isInt());
  Int result(&scope, *v);
  EXPECT_EQ(result->asWord(), 0);
}

TEST(InterpreterTest, FunctionAccessesUnboundVariable) {
  Runtime runtime;
  HandleScope scope;
  const char* src = R"(
def outer():
    var = 1
    def inner():
        return var
    del var
    return var

v = outer()
  )";

  EXPECT_TRUE(
      raisedWithStr(runFromCStr(&runtime, src), LayoutId::kUnboundLocalError,
                    "local variable 'var' referenced before assignment"));
}

TEST(InterpreterTest, ImportStarImportsPublicSymbols) {
  Runtime runtime;
  HandleScope scope;

  const char* module_src = R"(
def public_symbol():
    return 1
def public_symbol2():
    return 2
)";

  // Preload the module
  Object name(&scope, runtime.newStrFromCStr("test_module"));
  std::unique_ptr<char[]> buffer(Runtime::compile(module_src));
  runtime.importModuleFromBuffer(buffer.get(), name);

  runFromCStr(&runtime, R"(
from test_module import *
a = public_symbol()
b = public_symbol2()
)");

  Module main(&scope, findModule(&runtime, "__main__"));
  Object a(&scope, moduleAt(&runtime, main, "a"));
  Object b(&scope, moduleAt(&runtime, main, "b"));
  ASSERT_TRUE(a->isInt());
  ASSERT_TRUE(b->isInt());

  Int result1(&scope, *a);
  Int result2(&scope, *b);
  EXPECT_EQ(result1->asWord(), 1);
  EXPECT_EQ(result2->asWord(), 2);
}

TEST(InterpreterTest, ImportStarDoesNotImportPrivateSymbols) {
  Runtime runtime;
  HandleScope scope;

  const char* module_src = R"(
def public_symbol():
    return 1
def _private_symbol():
    return 2
)";

  // Preload the module
  Object name(&scope, runtime.newStrFromCStr("test_module"));
  std::unique_ptr<char[]> buffer(Runtime::compile(module_src));
  runtime.importModuleFromBuffer(buffer.get(), name);

  const char* main_src = R"(
from test_module import *
a = public_symbol()
b = _private_symbol()
)";

  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, main_src),
                            LayoutId::kNameError,
                            "name '_private_symbol' is not defined"));
}

TEST(InterpreterTest, GetAnextCallsAnextAndAwait) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
anext_called = None
await_called = None

class AsyncIterator:
  def __anext__(self):
    global anext_called
    anext_called = self
    return self

  def __await__(self):
    global await_called
    await_called = self
    return self

a = AsyncIterator()
)");
  Module main(&scope, findModule(&runtime, "__main__"));
  Object a(&scope, moduleAt(&runtime, main, "a"));

  Code code(&scope, testing::newEmptyCode(&runtime));
  Tuple consts(&scope, runtime.newTuple(1));
  consts->atPut(0, *a);
  code->setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, GET_ANEXT, 0, RETURN_VALUE, 0};
  code->setCode(runtime.newBytesWithAll(bytecode));

  Object result(&scope, Thread::currentThread()->run(code));
  EXPECT_EQ(*a, *result);
  Object anext(&scope, moduleAt(&runtime, main, "anext_called"));
  EXPECT_EQ(*a, *anext);
  Object await(&scope, moduleAt(&runtime, main, "await_called"));
  EXPECT_EQ(*a, *await);
}

TEST(InterpreterTest, GetAnextOnNonIterable) {
  Runtime runtime;
  HandleScope scope;
  Code code(&scope, testing::newEmptyCode(&runtime));
  Tuple consts(&scope, runtime.newTuple(1));
  consts->atPut(0, SmallInt::fromWord(123));
  code->setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, GET_ANEXT, 0, RETURN_VALUE, 0};
  code->setCode(runtime.newBytesWithAll(bytecode));

  Object result(&scope, Thread::currentThread()->run(code));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(InterpreterTest, GetAnextWithInvalidAnext) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
class AsyncIterator:
  def __anext__(self):
    return 42

a = AsyncIterator()
)");
  Module main(&scope, findModule(&runtime, "__main__"));
  Object a(&scope, moduleAt(&runtime, main, "a"));

  Code code(&scope, testing::newEmptyCode(&runtime));
  Tuple consts(&scope, runtime.newTuple(1));
  consts->atPut(0, *a);
  code->setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, GET_ANEXT, 0, RETURN_VALUE, 0};
  code->setCode(runtime.newBytesWithAll(bytecode));

  Object result(&scope, Thread::currentThread()->run(code));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(InterpreterTest, GetAwaitableCallsAwait) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
class Awaitable:
  def __await__(self):
    return 42

a = Awaitable()
)");

  Module main(&scope, findModule(&runtime, "__main__"));
  Object a(&scope, moduleAt(&runtime, main, "a"));

  Code code(&scope, testing::newEmptyCode(&runtime));
  Tuple consts(&scope, runtime.newTuple(1));
  consts->atPut(0, *a);
  code->setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, GET_AWAITABLE, 0, RETURN_VALUE, 0};
  code->setCode(runtime.newBytesWithAll(bytecode));

  Object result(&scope, Thread::currentThread()->run(code));
  ASSERT_TRUE(result->isSmallInt());
  EXPECT_EQ(42, RawSmallInt::cast(*result)->value());
}

TEST(InterpreterTest, GetAwaitableOnNonAwaitable) {
  Runtime runtime;
  HandleScope scope;
  Code code(&scope, testing::newEmptyCode(&runtime));
  Tuple consts(&scope, runtime.newTuple(1));
  consts->atPut(0, runtime.newStrFromCStr("foo"));
  code->setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, GET_AWAITABLE, 0, RETURN_VALUE, 0};
  code->setCode(runtime.newBytesWithAll(bytecode));

  Object result(&scope, Thread::currentThread()->run(code));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(InterpreterTest, BuildMapUnpackWithCallDict) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
def foo(**kwargs):
    return kwargs

d = foo(**{'a': 1, 'b': 2}, **{'c': 3, 'd': 4})
)");

  Module main(&scope, findModule(&runtime, "__main__"));
  Object d(&scope, moduleAt(&runtime, main, "d"));
  ASSERT_TRUE(d->isDict());

  Dict dict(&scope, *d);
  EXPECT_EQ(dict->numItems(), 4);

  Object key(&scope, SmallStr::fromCStr("a"));
  Object el0(&scope, runtime.dictAt(dict, key));
  ASSERT_TRUE(el0->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*el0)->value(), 1);

  key = SmallStr::fromCStr("b");
  Object el1(&scope, runtime.dictAt(dict, key));
  ASSERT_TRUE(el1->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*el1)->value(), 2);

  key = SmallStr::fromCStr("c");
  Object el2(&scope, runtime.dictAt(dict, key));
  ASSERT_TRUE(el2->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*el2)->value(), 3);

  key = SmallStr::fromCStr("d");
  Object el3(&scope, runtime.dictAt(dict, key));
  ASSERT_TRUE(el3->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*el3)->value(), 4);
}

TEST(InterpreterTest, BuildMapUnpackWithCallTupleKeys) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
class Foo:
    def __init__(self, d):
        self.d = d

    def keys(self):
        return ('c', 'd')

    def __getitem__(self, key):
        return self.d[key]

def foo(**kwargs):
    return kwargs

d = foo(**{'a': 1, 'b': 2}, **Foo({'c': 3, 'd': 4}))
)");

  Module main(&scope, findModule(&runtime, "__main__"));
  Object d(&scope, moduleAt(&runtime, main, "d"));
  ASSERT_TRUE(d->isDict());

  Dict dict(&scope, *d);
  EXPECT_EQ(dict->numItems(), 4);

  Object key(&scope, SmallStr::fromCStr("a"));
  Object el0(&scope, runtime.dictAt(dict, key));
  ASSERT_TRUE(el0->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*el0)->value(), 1);

  key = SmallStr::fromCStr("b");
  Object el1(&scope, runtime.dictAt(dict, key));
  ASSERT_TRUE(el1->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*el1)->value(), 2);

  key = SmallStr::fromCStr("c");
  Object el2(&scope, runtime.dictAt(dict, key));
  ASSERT_TRUE(el2->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*el2)->value(), 3);

  key = SmallStr::fromCStr("d");
  Object el3(&scope, runtime.dictAt(dict, key));
  ASSERT_TRUE(el3->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*el3)->value(), 4);
}

TEST(InterpreterTest, BuildMapUnpackWithCallListKeys) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
class Foo:
    def __init__(self, d):
        self.d = d

    def keys(self):
        return ['c', 'd']

    def __getitem__(self, key):
        return self.d[key]

def foo(**kwargs):
    return kwargs

d = foo(**{'a': 1, 'b': 2}, **Foo({'c': 3, 'd': 4}))
)");

  Module main(&scope, findModule(&runtime, "__main__"));
  Object d(&scope, moduleAt(&runtime, main, "d"));
  ASSERT_TRUE(d->isDict());

  Dict dict(&scope, *d);
  EXPECT_EQ(dict->numItems(), 4);

  Object key(&scope, SmallStr::fromCStr("a"));
  Object el0(&scope, runtime.dictAt(dict, key));
  ASSERT_TRUE(el0->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*el0)->value(), 1);

  key = SmallStr::fromCStr("b");
  Object el1(&scope, runtime.dictAt(dict, key));
  ASSERT_TRUE(el1->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*el1)->value(), 2);

  key = SmallStr::fromCStr("c");
  Object el2(&scope, runtime.dictAt(dict, key));
  ASSERT_TRUE(el2->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*el2)->value(), 3);

  key = SmallStr::fromCStr("d");
  Object el3(&scope, runtime.dictAt(dict, key));
  ASSERT_TRUE(el3->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*el3)->value(), 4);
}

TEST(InterpreterTest, BuildMapUnpackWithCallIteratorKeys) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
class Iter:
    def __init__(self, keys):
        self.idx = 0
        self.keys = keys

    def __iter__(self):
        return self

    def __next__(self):
        if self.idx >= len(self.keys):
            raise StopIteration()
        r = self.keys[self.idx]
        self.idx += 1
        return r

    def __length_hint__(self):
        return len(self.keys) - self.idx

class Foo:
    def __init__(self, d):
        self.d = d

    def keys(self):
        return Iter(['c', 'd'])

    def __getitem__(self, key):
        return self.d[key]

def foo(**kwargs):
    return kwargs

d = foo(**{'a': 1, 'b': 2}, **Foo({'c': 3, 'd': 4}))
)");

  Module main(&scope, findModule(&runtime, "__main__"));
  Object d(&scope, moduleAt(&runtime, main, "d"));
  ASSERT_TRUE(d->isDict());

  Dict dict(&scope, *d);
  EXPECT_EQ(dict->numItems(), 4);

  Object key(&scope, SmallStr::fromCStr("a"));
  Object el0(&scope, runtime.dictAt(dict, key));
  ASSERT_TRUE(el0->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*el0)->value(), 1);

  key = SmallStr::fromCStr("b");
  Object el1(&scope, runtime.dictAt(dict, key));
  ASSERT_TRUE(el1->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*el1)->value(), 2);

  key = SmallStr::fromCStr("c");
  Object el2(&scope, runtime.dictAt(dict, key));
  ASSERT_TRUE(el2->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*el2)->value(), 3);

  key = SmallStr::fromCStr("d");
  Object el3(&scope, runtime.dictAt(dict, key));
  ASSERT_TRUE(el3->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*el3)->value(), 4);
}

TEST(InterpreterTest, BuildMapUnpackWithCallDictNonStrKey) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
def foo(**kwargs):
    return kwargs

foo(**{'a': 1, 'b': 2}, **{'c': 3, 4: 4})
  )"),
                            LayoutId::kTypeError, "keywords must be strings"));
}

TEST(InterpreterTest, BuildMapUnpackWithCallDictRepeatedKeys) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
def foo(**kwargs):
    return kwargs

foo(**{'a': 1, 'b': 2}, **{'c': 3, 'a': 4})
  )"),
                            LayoutId::kTypeError,
                            "got multiple values for keyword argument"));
}

TEST(InterpreterTest, BuildMapUnpackWithCallNonMapping) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
class Foo:
    pass

def foo(**kwargs):
    return kwargs

foo(**{'a': 1, 'b': 2}, **Foo())
  )"),
                            LayoutId::kTypeError, "object is not a mapping"));
}

TEST(InterpreterTest, BuildMapUnpackWithCallNonSubscriptable) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
class Foo:
    def keys(self):
        pass

def foo(**kwargs):
    return kwargs

foo(**{'a': 1, 'b': 2}, **Foo())
  )"),
                            LayoutId::kTypeError,
                            "object is not subscriptable"));
}

TEST(InterpreterTest, BuildMapUnpackWithCallListKeysNonStrKey) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
class Foo:
    def keys(self):
        return [1]

    def __getitem__(self, key):
        pass

def foo(**kwargs):
    return kwargs

foo(**{'a': 1, 'b': 2}, **Foo())
  )"),
                            LayoutId::kTypeError, "keywords must be strings"));
}

TEST(InterpreterTest, BuildMapUnpackWithCallListKeysRepeatedKeys) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
class Foo:
    def keys(self):
        return ['a']

    def __getitem__(self, key):
        pass

def foo(**kwargs):
    return kwargs

foo(**{'a': 1, 'b': 2}, **Foo())
  )"),
                            LayoutId::kTypeError,
                            "got multiple values for keyword argument"));
}

TEST(InterpreterTest, BuildMapUnpackWithCallTupleKeysNonStrKeys) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
class Foo:
    def keys(self):
        return (1,)

    def __getitem__(self, key):
        pass

def foo(**kwargs):
    return kwargs

foo(**{'a': 1, 'b': 2}, **Foo())
  )"),
                            LayoutId::kTypeError, "keywords must be strings"));
}

TEST(InterpreterTest, BuildMapUnpackWithCallTupleKeysRepeatedKeys) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
class Foo:
    def keys(self):
        return ('a',)

    def __getitem__(self, key):
        pass

def foo(**kwargs):
    return kwargs

foo(**{'a': 1, 'b': 2}, **Foo())
  )"),
                            LayoutId::kTypeError,
                            "got multiple values for keyword argument"));
}

TEST(InterpreterTest, BuildMapUnpackWithCallNonIterableKeys) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
class Foo:
    def keys(self):
        return None

    def __getitem__(self, key):
        pass

def foo(**kwargs):
    return kwargs

foo(**{'a': 1, 'b': 2}, **Foo())
  )"),
                            LayoutId::kTypeError, "keys() is not iterable"));
}

TEST(InterpreterTest, BuildMapUnpackWithCallIterableWithoutNext) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
class Iter:
    def __iter__(self):
        return self

class Foo:
    def keys(self):
        return Iter()

    def __getitem__(self, key):
        pass

def foo(**kwargs):
    return kwargs

foo(**{'a': 1, 'b': 2}, **Foo())
  )"),
                            LayoutId::kTypeError, "keys() is not iterable"));
}

TEST(InterpreterTest, BuildMapUnpackWithCallIterableNonStrKey) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
class Iter:
    def __init__(self, keys):
        self.idx = 0
        self.keys = keys

    def __iter__(self):
        return self

    def __next__(self):
        if self.idx >= len(self.keys):
            raise StopIteration()
        r = self.keys[self.idx]
        self.idx += 1
        return r

    def __length_hint__(self):
        return len(self.keys) - self.idx

class Foo:
    def keys(self):
        return Iter((1, 2, 3))

    def __getitem__(self, key):
        return 0

def foo(**kwargs):
    return kwargs

foo(**{'a': 1, 'b': 2}, **Foo())
  )"),
                            LayoutId::kTypeError, "keywords must be strings"));
}

TEST(InterpreterTest, BuildMapUnpackWithCallIterableRepeatedKeys) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
class Iter:
    def __init__(self, keys):
        self.idx = 0
        self.keys = keys

    def __iter__(self):
        return self

    def __next__(self):
        if self.idx >= len(self.keys):
            raise StopIteration()
        r = self.keys[self.idx]
        self.idx += 1
        return r

    def __length_hint__(self):
        return len(self.keys) - self.idx

class Foo:
    def keys(self):
        return Iter(('a', 'a'))

    def __getitem__(self, key):
        return 0

def foo(**kwargs):
    return kwargs

foo(**{'a': 1, 'b': 2}, **Foo())
  )"),
                            LayoutId::kTypeError,
                            "got multiple values for keyword argument"));
}

TEST(InterpreterTest, YieldFromIterReturnsIter) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
class FooIterator:
    pass

class Foo:
    def __iter__(self):
        return FooIterator()

foo = Foo()
	)");

  Module main(&scope, findModule(&runtime, "__main__"));
  Object foo(&scope, moduleAt(&runtime, main, "foo"));

  // Create a code object and set the foo instance as a const
  Code code(&scope, testing::newEmptyCode(&runtime));
  Tuple consts(&scope, runtime.newTuple(1));
  consts->atPut(0, *foo);
  code->setConsts(*consts);

  // Python code:
  // foo = Foo()
  // def bar():
  //     yield from foo
  const byte bc[] = {
      LOAD_CONST,          0,  // (foo)
      GET_YIELD_FROM_ITER, 0,  // iter(foo)
      RETURN_VALUE,        0,
  };
  code->setCode(runtime.newBytesWithAll(bc));

  // Confirm that the returned value is the iterator of Foo
  Object result(&scope, Thread::currentThread()->run(code));
  Type result_type(&scope, runtime.typeOf(*result));
  EXPECT_TRUE(isStrEqualsCStr(result_type->name(), "FooIterator"));
}

TEST(InterpreterTest, YieldFromIterThrowsException) {
  Runtime runtime;
  HandleScope scope;

  const char* src = R"(
def yield_from_func():
    yield from 1

for i in yield_from_func():
    pass
	)";

  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, src), LayoutId::kTypeError,
                            "object is not iterable"));
}

TEST(InterpreterTest, MakeFunctionSetsDunderModule) {
  Runtime runtime;
  HandleScope scope;
  Object module_name(&scope, runtime.newStrFromCStr("foo"));
  std::unique_ptr<char[]> buffer(Runtime::compile(R"(
def bar(): pass
)"));
  runtime.importModuleFromBuffer(buffer.get(), module_name);
  runFromCStr(&runtime, R"(
import foo
def baz(): pass
a = getattr(foo.bar, '__module__')
b = getattr(baz, '__module__')
)");
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  ASSERT_TRUE(a->isStr());
  EXPECT_TRUE(Str::cast(*a)->equalsCStr("foo"));
  Object b(&scope, moduleAt(&runtime, "__main__", "b"));
  ASSERT_TRUE(b->isStr());
  EXPECT_TRUE(Str::cast(*b)->equalsCStr("__main__"));
}

TEST(InterpreterTest, MakeFunctionSetsDunderQualname) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
class Foo():
    def bar(): pass
def baz(): pass
a = getattr(Foo.bar, '__qualname__')
b = getattr(baz, '__qualname__')
)");
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  ASSERT_TRUE(a->isStr());
  EXPECT_TRUE(Str::cast(*a)->equalsCStr("Foo.bar"));
  Object b(&scope, moduleAt(&runtime, "__main__", "b"));
  ASSERT_TRUE(b->isStr());
  EXPECT_TRUE(Str::cast(*b)->equalsCStr("baz"));
}

TEST(InterpreterTest, MakeFunctionSetsDunderDoc) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
def foo():
    """This is a docstring"""
    pass
def bar(): pass
)");
  HandleScope scope;
  Object foo(&scope, testing::moduleAt(&runtime, "__main__", "foo"));
  ASSERT_TRUE(foo->isFunction());
  Object foo_docstring(&scope, RawFunction::cast(*foo)->doc());
  ASSERT_TRUE(foo_docstring->isStr());
  EXPECT_TRUE(RawStr::cast(*foo_docstring)->equalsCStr("This is a docstring"));

  Object bar(&scope, testing::moduleAt(&runtime, "__main__", "bar"));
  ASSERT_TRUE(bar->isFunction());
  Object bar_docstring(&scope, RawFunction::cast(*bar)->doc());
  EXPECT_TRUE(bar_docstring->isNoneType());
}

TEST(InterpreterTest, FunctionCallWithNonFunctionRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->currentFrame();
  Str not_a_func(&scope, runtime.newStrFromCStr(""));
  frame->pushValue(*not_a_func);
  RawObject result = Interpreter::call(thread, frame, 0);
  EXPECT_TRUE(result->isError());
  EXPECT_TRUE(thread->hasPendingException());
}

TEST(InterpreterTest, StoreSubscr) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
l = [0]
for i in range(5):
    l[0] += i
)");

  HandleScope scope;
  Object l_obj(&scope, testing::moduleAt(&runtime, "__main__", "l"));
  ASSERT_TRUE(l_obj->isList());
  List l(&scope, *l_obj);
  ASSERT_EQ(l->numItems(), 1);
  EXPECT_EQ(l->at(0), SmallInt::fromWord(10));
}

// TODO(bsimmers) Rewrite these exception tests to ensure that the specific
// bytecodes we care about are being exercised, so we're not be at the mercy of
// compiler optimizations or changes.
TEST(InterpreterTest, ExceptCatchesException) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
n = 0
try:
    raise RuntimeError("something went wrong")
    n = 1
except:
    if n == 0:
        n = 2
)");

  HandleScope scope;
  Object n(&scope, testing::moduleAt(&runtime, "__main__", "n"));
  ASSERT_TRUE(n->isInt());
  Int n_int(&scope, *n);
  EXPECT_EQ(n_int->asWord(), 2);
}

TEST(InterpreterTest, RaiseCrossesFunctions) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
def sub():
  raise RuntimeError("from sub")

def main():
  sub()

n = 0
try:
  main()
  n = 1
except:
  if n == 0:
    n = 2
)");

  HandleScope scope;
  Object n(&scope, testing::moduleAt(&runtime, "__main__", "n"));
  ASSERT_TRUE(n->isInt());
  Int n_int(&scope, *n);
  EXPECT_EQ(n_int->asWord(), 2);
}

TEST(InterpreterTest, RaiseFromSetsCause) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
try:
  try:
    raise RuntimeError
  except Exception as e:
    raise TypeError from e
except Exception as e:
  exc = e
)");

  HandleScope scope;
  Object exc_obj(&scope, testing::moduleAt(&runtime, "__main__", "exc"));
  ASSERT_EQ(exc_obj->layoutId(), LayoutId::kTypeError);
  BaseException exc(&scope, *exc_obj);
  EXPECT_EQ(exc->cause()->layoutId(), LayoutId::kRuntimeError);
}

TEST(InterpreterTest, ExceptWithRightTypeCatches) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
n = 0
try:
    raise RuntimeError("whoops")
    n = 1
except RuntimeError:
    if n == 0:
        n = 2
)");

  HandleScope scope;
  Object n(&scope, testing::moduleAt(&runtime, "__main__", "n"));
  ASSERT_TRUE(n->isInt());
  Int n_int(&scope, *n);
  EXPECT_EQ(n_int->asWord(), 2);
}

TEST(InterpreterTest, ExceptWithRightTupleTypeCatches) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
n = 0
try:
    raise RuntimeError()
    n = 1
except (StopIteration, RuntimeError, ImportError):
    if n == 0:
        n = 2
)");

  HandleScope scope;
  Object n(&scope, testing::moduleAt(&runtime, "__main__", "n"));
  ASSERT_TRUE(n->isInt());
  Int n_int(&scope, *n);
  EXPECT_EQ(n_int->asWord(), 2);
}

TEST(InterpreterTest, ExceptWithWrongTypePasses) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
try:
    raise RuntimeError("something went wrong")
except StopIteration:
    pass
)"),
                            LayoutId::kRuntimeError, "something went wrong"));
}

TEST(InterpreterTest, ExceptWithWrongTupleTypePasses) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
try:
    raise RuntimeError("something went wrong")
except (StopIteration, ImportError):
    pass
)"),
                            LayoutId::kRuntimeError, "something went wrong"));
}

TEST(InterpreterTest, RaiseTypeCreatesException) {
  Runtime runtime;
  EXPECT_TRUE(raised(runFromCStr(&runtime, "raise StopIteration"),
                     LayoutId::kStopIteration));
}

TEST(InterpreterTest, BareRaiseReraises) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
class MyError(Exception):
  pass

inner = None
outer = None
try:
  try:
    raise MyError()
  except Exception as exc:
    inner = exc
    raise
except Exception as exc:
  outer = exc
)");

  HandleScope scope;
  Object my_error(&scope, testing::moduleAt(&runtime, "__main__", "MyError"));
  EXPECT_EQ(runtime.typeOf(*my_error), runtime.typeAt(LayoutId::kType));
  Object inner(&scope, testing::moduleAt(&runtime, "__main__", "inner"));
  EXPECT_EQ(runtime.typeOf(*inner), *my_error);
  Object outer(&scope, testing::moduleAt(&runtime, "__main__", "outer"));
  EXPECT_EQ(*inner, *outer);
}

TEST(InterpreterTest, ExceptWithNonExceptionTypeRaises) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
try:
  raise RuntimeError
except str:
  pass
)"),
                            LayoutId::kTypeError,
                            "catching classes that do not inherit from "
                            "BaseException is not allowed"));
}

TEST(InterpreterTest, ExceptWithNonExceptionTypeInTupleRaises) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
try:
  raise RuntimeError
except (StopIteration, int, RuntimeError):
  pass
)"),
                            LayoutId::kTypeError,
                            "catching classes that do not inherit from "
                            "BaseException is not allowed"));
}

TEST(InterpreterTest, RaiseWithNoActiveExceptionRaises) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "raise\n"),
                            LayoutId::kRuntimeError,
                            "No active exception to reraise"));
}

TEST(InterpreterTest, LoadAttrWithoutAttrUnwindsAttributeException) {
  Runtime runtime;
  HandleScope scope;

  // Set up a code object that runs: {}.foo
  Code code(&scope, testing::newEmptyCode(&runtime));
  Tuple names(&scope, runtime.newTuple(1));
  Str foo(&scope, runtime.newStrFromCStr("foo"));
  names->atPut(0, *foo);
  code->setNames(*names);

  // load arguments and execute the code
  const byte bytecode[] = {BUILD_MAP, 0, LOAD_ATTR, 0};
  code->setCode(runtime.newBytesWithAll(bytecode));
  code->setStacksize(1);

  // Execute the code and make sure to get the unwinded Error
  RawObject result = Thread::currentThread()->run(code);
  ASSERT_TRUE(result->isError());
}

TEST(InterpreterTest, ExplodeCallAcceptsList) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
def f(a, b):
  return [b, a]

args = ['a', 'b']
result = f(*args)
)");

  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_PYLIST_EQ(result, {"b", "a"});
}

TEST(InterpreterDeathTest, ExplodeWithIterableRaises) {
  Runtime runtime;
  HandleScope scope;
  // TODO(bsimmers): Change this to inspect result once sequenceAsTuple() is
  // fixed.
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(&runtime, R"(
def f():
  pass
def gen():
  yield 1
  yield 2
result = f(*gen())
)"),
                    LayoutId::kNotImplementedError,
                    "Iterables not yet supported in sequenceAsTuple()"));
}

TEST(InterpreterTest, FormatValueCallsDunderStr) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  runFromCStr(&runtime, R"(
class C:
  def __str__(self):
    return "foobar"
result = f"{C()!s}"
)");
  HandleScope scope(thread);
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isStrEqualsCStr(*result, "foobar"));
}

TEST(InterpreterTest, FormatValueFallsBackToDunderRepr) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  runFromCStr(&runtime, R"(
class C:
  def __repr__(self):
    return "foobar"
result = f"{C()!s}"
)");
  HandleScope scope(thread);
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isStrEqualsCStr(*result, "foobar"));
}

TEST(InterpreterTest, FormatValueCallsDunderRepr) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  runFromCStr(&runtime, R"(
class C:
  def __repr__(self):
    return "foobar"
result = f"{C()!r}"
)");
  HandleScope scope(thread);
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isStrEqualsCStr(*result, "foobar"));
}

TEST(InterpreterTest, FormatValueAsciiCallsDunderRepr) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  runFromCStr(&runtime, R"(
class C:
  def __repr__(self):
    return "foobar"
result = f"{C()!a}"
)");
  HandleScope scope(thread);
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isStrEqualsCStr(*result, "foobar"));
}

}  // namespace python
