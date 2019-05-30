#include "gtest/gtest.h"

#include <memory>

#include "bytecode.h"
#include "frame.h"
#include "handles.h"
#include "ic.h"
#include "interpreter.h"
#include "objects.h"
#include "runtime.h"
#include "trampolines.h"

#include "test-utils.h"

namespace python {
using namespace testing;

TEST(InterpreterTest, IsTrueBool) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  Frame* frame = thread->currentFrame();

  ASSERT_TRUE(frame->isSentinelFrame());

  Object true_value(&scope, Bool::trueObj());
  EXPECT_EQ(Interpreter::isTrue(thread, *true_value), Bool::trueObj());

  Object false_object(&scope, Bool::falseObj());
  EXPECT_EQ(Interpreter::isTrue(thread, *false_object), Bool::falseObj());
}

TEST(InterpreterTest, IsTrueInt) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  Frame* frame = thread->currentFrame();

  ASSERT_TRUE(frame->isSentinelFrame());

  Object true_value(&scope, runtime.newInt(1234));
  EXPECT_EQ(Interpreter::isTrue(thread, *true_value), Bool::trueObj());

  Object false_value(&scope, runtime.newInt(0));
  EXPECT_EQ(Interpreter::isTrue(thread, *false_value), Bool::falseObj());
}

TEST(InterpreterTest, IsTrueWithDunderBoolRaisingPropagatesException) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo:
  def __bool__(self):
    raise UserWarning('')
value = Foo()
)")
                   .isError());
  Object value(&scope, moduleAt(&runtime, "__main__", "value"));
  Object result(&scope, Interpreter::isTrue(thread, *value));
  EXPECT_TRUE(raised(*result, LayoutId::kUserWarning));
}

TEST(InterpreterTest, IsTrueWithDunderLenRaisingPropagatesException) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo:
  def __len__(self):
    raise UserWarning('')
value = Foo()
)")
                   .isError());
  Object value(&scope, moduleAt(&runtime, "__main__", "value"));
  Object result(&scope, Interpreter::isTrue(thread, *value));
  EXPECT_TRUE(raised(*result, LayoutId::kUserWarning));
}

TEST(InterpreterTest, IsTrueWithIntSubclassDunderLenUsesBaseInt) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo(int): pass
class Bar:
  def __init__(self, length):
    self.length = Foo(length)
  def __len__(self):
    return self.length
true_value = Bar(10)
false_value = Bar(0)
)")
                   .isError());
  Object true_value(&scope, moduleAt(&runtime, "__main__", "true_value"));
  Object false_value(&scope, moduleAt(&runtime, "__main__", "false_value"));
  EXPECT_EQ(Interpreter::isTrue(thread, *true_value), Bool::trueObj());
  EXPECT_EQ(Interpreter::isTrue(thread, *false_value), Bool::falseObj());
}

TEST(InterpreterTest, IsTrueDunderLen) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  Frame* frame = thread->currentFrame();

  ASSERT_TRUE(frame->isSentinelFrame());

  List nonempty_list(&scope, runtime.newList());
  Object elt(&scope, NoneType::object());
  runtime.listAdd(thread, nonempty_list, elt);

  EXPECT_EQ(Interpreter::isTrue(thread, *nonempty_list), Bool::trueObj());

  List empty_list(&scope, runtime.newList());
  EXPECT_EQ(Interpreter::isTrue(thread, *empty_list), Bool::falseObj());
}

TEST(InterpreterTest, UnaryOperationWithIntReturnsInt) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object value(&scope, runtime.newInt(23));
  Object result(
      &scope, Interpreter::unaryOperation(thread, value, SymbolId::kDunderPos));
  EXPECT_TRUE(isIntEqualsWord(*result, 23));
}

TEST(InterpreterTest, UnaryOperationWithBadTypeRaisesTypeError) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object value(&scope, NoneType::object());
  Object result(&scope, Interpreter::unaryOperation(thread, value,
                                                    SymbolId::kDunderInvert));
  EXPECT_TRUE(
      raisedWithStr(*result, LayoutId::kTypeError,
                    "bad operand type for unary '__invert__': 'NoneType'"));
}

TEST(InterpreterTest, UnaryOperationWithCustomDunderInvertReturnsString) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  def __invert__(self):
    return "custom invert"
c = C()
)")
                   .isError());
  Object c(&scope, moduleAt(&runtime, "__main__", "c"));
  Object result(
      &scope, Interpreter::unaryOperation(thread, c, SymbolId::kDunderInvert));
  EXPECT_TRUE(isStrEqualsCStr(*result, "custom invert"));
}

TEST(InterpreterTest, UnaryOperationWithCustomRaisingDunderNegPropagates) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  def __neg__(self):
    raise UserWarning('')
c = C()
)")
                   .isError());
  Object c(&scope, moduleAt(&runtime, "__main__", "c"));
  Object result(&scope,
                Interpreter::unaryOperation(thread, c, SymbolId::kDunderNeg));
  EXPECT_TRUE(raised(*result, LayoutId::kUserWarning));
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
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
    def __sub__(self, other):
        return (C, '__sub__', self, other)

left = C()
right = C()
)")
                   .isError());

  Frame* frame = thread->currentFrame();

  Module main(&scope, findModule(&runtime, "__main__"));
  Object left(&scope, moduleAt(&runtime, main, "left"));
  Object right(&scope, moduleAt(&runtime, main, "right"));
  Object c_class(&scope, moduleAt(&runtime, main, "C"));

  Object result_obj(
      &scope, Interpreter::binaryOperation(
                  thread, frame, Interpreter::BinaryOp::SUB, left, right));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 4);
  EXPECT_EQ(result.at(0), *c_class);
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "__sub__"));
  EXPECT_EQ(result.at(2), *left);
  EXPECT_EQ(result.at(3), *right);
}

TEST(InterpreterTest, BinaryOpInvokesSelfMethodIgnoresReflectedMethod) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
    def __sub__(self, other):
        return (C, '__sub__', self, other)
    def __rsub__(self, other):
        return (C, '__rsub__', self, other)

left = C()
right = C()
)")
                   .isError());

  Frame* frame = thread->currentFrame();

  Module main(&scope, findModule(&runtime, "__main__"));
  Object left(&scope, moduleAt(&runtime, main, "left"));
  Object right(&scope, moduleAt(&runtime, main, "right"));
  Object c_class(&scope, moduleAt(&runtime, main, "C"));

  Object result_obj(
      &scope, Interpreter::binaryOperation(
                  thread, frame, Interpreter::BinaryOp::SUB, left, right));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 4);
  EXPECT_EQ(result.at(0), *c_class);
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "__sub__"));
  EXPECT_EQ(result.at(2), *left);
  EXPECT_EQ(result.at(3), *right);
}

TEST(InterpreterTest, BinaryOperationInvokesSubclassReflectedMethod) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
    def __sub__(self, other):
        return (C, '__sub__', self, other)

class D(C):
    def __rsub__(self, other):
        return (D, '__rsub__', self, other)

left = C()
right = D()
)")
                   .isError());

  Frame* frame = thread->currentFrame();

  Module main(&scope, findModule(&runtime, "__main__"));
  Object left(&scope, moduleAt(&runtime, main, "left"));
  Object right(&scope, moduleAt(&runtime, main, "right"));
  Object d_class(&scope, moduleAt(&runtime, main, "D"));

  Object result_obj(
      &scope, Interpreter::binaryOperation(
                  thread, frame, Interpreter::BinaryOp::SUB, left, right));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 4);
  EXPECT_EQ(result.at(0), *d_class);
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "__rsub__"));
  EXPECT_EQ(result.at(2), *right);
  EXPECT_EQ(result.at(3), *left);
}

TEST(InterpreterTest, BinaryOperationInvokesOtherReflectedMethod) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
    pass

class D:
    def __rsub__(self, other):
        return (D, '__rsub__', self, other)

left = C()
right = D()
)")
                   .isError());

  Frame* frame = thread->currentFrame();

  Module main(&scope, findModule(&runtime, "__main__"));
  Object left(&scope, moduleAt(&runtime, main, "left"));
  Object right(&scope, moduleAt(&runtime, main, "right"));
  Object d_class(&scope, moduleAt(&runtime, main, "D"));

  Object result_obj(
      &scope, Interpreter::binaryOperation(
                  thread, frame, Interpreter::BinaryOp::SUB, left, right));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 4);
  EXPECT_EQ(result.at(0), *d_class);
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "__rsub__"));
  EXPECT_EQ(result.at(2), *right);
  EXPECT_EQ(result.at(3), *left);
}

TEST(InterpreterTest, BinaryOperationLookupPropagatesException) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class RaisingDescriptor:
  def __get__(self, obj, type):
    raise UserWarning()
class A:
  __mul__ = RaisingDescriptor()
a = A()
)")
                   .isError());
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  Frame* frame = thread->currentFrame();
  Object result(&scope, Interpreter::binaryOperation(
                            thread, frame, Interpreter::BinaryOp::MUL, a, a));
  EXPECT_TRUE(raised(*result, LayoutId::kUserWarning));
}

TEST(InterpreterTest, BinaryOperationLookupReflectedMethodPropagatesException) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class RaisingDescriptor:
  def __get__(self, obj, type):
    raise UserWarning()
class A:
  def __mul__(self, other):
    return 42
class B(A):
  __rmul__ = RaisingDescriptor()
a = A()
b = B()
)")
                   .isError());
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  Object b(&scope, moduleAt(&runtime, "__main__", "b"));
  Frame* frame = thread->currentFrame();
  Object result(&scope, Interpreter::binaryOperation(
                            thread, frame, Interpreter::BinaryOp::MUL, a, b));
  EXPECT_TRUE(raised(*result, LayoutId::kUserWarning));
}

TEST(InterpreterTest, BinaryOperationSetMethodSetsMethod) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object v0(&scope, runtime.newInt(13));
  Object v1(&scope, runtime.newInt(42));
  Object method(&scope, NoneType::object());
  IcBinopFlags flags;
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::binaryOperationSetMethod(thread, thread->currentFrame(),
                                            Interpreter::BinaryOp::SUB, v0, v1,
                                            &method, &flags),
      -29));
  EXPECT_TRUE(method.isFunction());
  EXPECT_EQ(flags, IC_BINOP_NOTIMPLEMENTED_RETRY);

  Object v2(&scope, runtime.newInt(3));
  Object v3(&scope, runtime.newInt(8));
  ASSERT_EQ(v0.layoutId(), v2.layoutId());
  ASSERT_EQ(v1.layoutId(), v3.layoutId());
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::binaryOperationWithMethod(thread, thread->currentFrame(),
                                             *method, flags, *v2, *v3),
      -5));
}

TEST(InterpreterTest,
     BinaryOperationSetMethodSetsReflectedMethodNotImplementedRetry) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class A:
  def __init__(self, x):
    self.x = x
  def __sub__(self, other):
    raise UserWarning("should not be called")
class ASub(A):
  def __rsub__(self, other):
    return (self, other)
v0 = A(3)
v1 = ASub(7)
v2 = A(8)
v3 = ASub(2)
)")
                   .isError());
  Object v0(&scope, moduleAt(&runtime, "__main__", "v0"));
  Object v1(&scope, moduleAt(&runtime, "__main__", "v1"));
  Object v2(&scope, moduleAt(&runtime, "__main__", "v2"));
  Object v3(&scope, moduleAt(&runtime, "__main__", "v3"));

  Object method(&scope, NoneType::object());
  IcBinopFlags flags;
  Object result_obj(
      &scope, Interpreter::binaryOperationSetMethod(
                  thread, thread->currentFrame(), Interpreter::BinaryOp::SUB,
                  v0, v1, &method, &flags));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 2);
  EXPECT_EQ(result.at(0), v1);
  EXPECT_EQ(result.at(1), v0);
  EXPECT_TRUE(method.isFunction());
  EXPECT_EQ(flags, IC_BINOP_REFLECTED | IC_BINOP_NOTIMPLEMENTED_RETRY);

  ASSERT_EQ(v0.layoutId(), v2.layoutId());
  ASSERT_EQ(v1.layoutId(), v3.layoutId());
  result_obj = Interpreter::binaryOperationWithMethod(
      thread, thread->currentFrame(), *method, flags, *v2, *v3);
  ASSERT_TRUE(result.isTuple());
  result = *result_obj;
  ASSERT_EQ(result.length(), 2);
  EXPECT_EQ(result.at(0), v3);
  EXPECT_EQ(result.at(1), v2);
}

TEST(InterpreterTest, BinaryOperationSetMethodSetsReflectedMethod) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class A:
  def __init__(self, x):
    self.x = x
class B:
  def __init__(self, x):
    self.x = x
  def __rsub__(self, other):
    return other.x - self.x
v0 = A(-4)
v1 = B(8)
v2 = A(33)
v3 = B(-12)
)")
                   .isError());
  Object v0(&scope, moduleAt(&runtime, "__main__", "v0"));
  Object v1(&scope, moduleAt(&runtime, "__main__", "v1"));
  Object v2(&scope, moduleAt(&runtime, "__main__", "v2"));
  Object v3(&scope, moduleAt(&runtime, "__main__", "v3"));

  Object method(&scope, NoneType::object());
  IcBinopFlags flags;
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::binaryOperationSetMethod(thread, thread->currentFrame(),
                                            Interpreter::BinaryOp::SUB, v0, v1,
                                            &method, &flags),
      -12));
  EXPECT_TRUE(method.isFunction());
  EXPECT_EQ(flags, IC_BINOP_REFLECTED);

  ASSERT_EQ(v0.layoutId(), v2.layoutId());
  ASSERT_EQ(v1.layoutId(), v3.layoutId());
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::binaryOperationWithMethod(thread, thread->currentFrame(),
                                             *method, flags, *v2, *v3),
      45));
}

TEST(InterpreterTest, BinaryOperationSetMethodSetsMethodNotImplementedRetry) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class A:
  def __init__(self, x):
    self.x = x
  def __sub__(self, other):
    return other.x - self.x
class B:
  def __init__(self, x):
    self.x = x
  def __rsub__(self, other):
    return self.x - other.x
v0 = A(4)
v1 = B(6)
v2 = A(9)
v3 = B(1)
)")
                   .isError());
  Object v0(&scope, moduleAt(&runtime, "__main__", "v0"));
  Object v1(&scope, moduleAt(&runtime, "__main__", "v1"));
  Object v2(&scope, moduleAt(&runtime, "__main__", "v2"));
  Object v3(&scope, moduleAt(&runtime, "__main__", "v3"));

  Object method(&scope, NoneType::object());
  IcBinopFlags flags;
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::binaryOperationSetMethod(thread, thread->currentFrame(),
                                            Interpreter::BinaryOp::SUB, v0, v1,
                                            &method, &flags),
      2));
  EXPECT_TRUE(method.isFunction());
  EXPECT_EQ(flags, IC_BINOP_NOTIMPLEMENTED_RETRY);

  ASSERT_EQ(v0.layoutId(), v2.layoutId());
  ASSERT_EQ(v1.layoutId(), v3.layoutId());
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::binaryOperationWithMethod(thread, thread->currentFrame(),
                                             *method, flags, *v2, *v3),
      -8));
}

TEST(InterpreterTest, DoBinaryOpWithCacheHitCallsCachedMethod) {
  Runtime runtime;
  runtime.enableCache();
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime.newTuple(2));
  consts.atPut(0, runtime.newInt(7));
  consts.atPut(1, runtime.newInt(-13));
  code.setConsts(*consts);
  const byte bytecode[] = {
      LOAD_CONST, 0, LOAD_CONST, 1, BINARY_SUBTRACT, 0, RETURN_VALUE, 0,
  };
  code.setCode(runtime.newBytesWithAll(bytecode));

  Object qualname(&scope, Str::empty());
  Object none(&scope, NoneType::object());
  Dict globals(&scope, runtime.newDict());
  Function function(
      &scope, Interpreter::makeFunction(thread, qualname, code, none, none,
                                        none, none, globals));

  // Update inline cache.
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::callFunction0(thread, thread->currentFrame(), function),
      20));

  ASSERT_TRUE(function.caches().isTuple());
  Tuple caches(&scope, function.caches());
  IcBinopFlags dummy;
  ASSERT_FALSE(
      icLookupBinop(caches, 0, LayoutId::kSmallInt, LayoutId::kSmallInt, &dummy)
          .isErrorNotFound());
  // Call from inline cache.
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::callFunction0(thread, thread->currentFrame(), function),
      20));
}

TEST(InterpreterTest, DoBinaryOpWithCacheHitCallsRetry) {
  Runtime runtime;
  runtime.enableCache();
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class MyInt(int):
  def __sub__(self, other):
    return NotImplemented
  def __rsub__(self, other):
    return NotImplemented
v0 = MyInt(3)
v1 = 7
)")
                   .isError());
  Object v0(&scope, moduleAt(&runtime, "__main__", "v0"));
  Object v1(&scope, moduleAt(&runtime, "__main__", "v1"));

  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime.newTuple(2));
  consts.atPut(0, *v0);
  consts.atPut(1, *v1);
  code.setConsts(*consts);
  const byte bytecode[] = {
      LOAD_CONST, 0, LOAD_CONST, 1, BINARY_SUBTRACT, 0, RETURN_VALUE, 0,
  };
  code.setCode(runtime.newBytesWithAll(bytecode));

  Object qualname(&scope, Str::empty());
  Object none(&scope, NoneType::object());
  Dict globals(&scope, runtime.newDict());
  Function function(
      &scope, Interpreter::makeFunction(thread, qualname, code, none, none,
                                        none, none, globals));

  // Update inline cache.
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::callFunction0(thread, thread->currentFrame(), function),
      -4));

  ASSERT_TRUE(function.caches().isTuple());
  Tuple caches(&scope, function.caches());
  IcBinopFlags dummy;
  ASSERT_FALSE(icLookupBinop(caches, 0, v0.layoutId(), v1.layoutId(), &dummy)
                   .isErrorNotFound());

  // Should hit the cache for __sub__ and then call binaryOperationRetry().
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::callFunction0(thread, thread->currentFrame(), function),
      -4));
}

TEST(InterpreterTest, ImportFromWithMissingAttributeRaisesImportError) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Str name(&scope, runtime.newStrFromCStr("foo"));
  Module module(&scope, runtime.newModule(name));
  runtime.addModule(module);
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "from foo import bar"),
                            LayoutId::kImportError,
                            "cannot import name 'bar' from 'foo'"));
}

TEST(InterpreterTest, InplaceOperationCallsInplaceMethod) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
    def __isub__(self, other):
        return (C, '__isub__', self, other)

left = C()
right = C()
)")
                   .isError());

  Frame* frame = thread->currentFrame();

  Module main(&scope, findModule(&runtime, "__main__"));
  Object left(&scope, moduleAt(&runtime, main, "left"));
  Object right(&scope, moduleAt(&runtime, main, "right"));
  Object c_class(&scope, moduleAt(&runtime, main, "C"));

  Object result_obj(
      &scope, Interpreter::inplaceOperation(
                  thread, frame, Interpreter::BinaryOp::SUB, left, right));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 4);
  EXPECT_EQ(result.at(0), *c_class);
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "__isub__"));
  EXPECT_EQ(result.at(2), *left);
  EXPECT_EQ(result.at(3), *right);
}

TEST(InterpreterTest, InplaceOperationCallsBinaryMethod) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
    def __sub__(self, other):
        return (C, '__sub__', self, other)

left = C()
right = C()
)")
                   .isError());

  Frame* frame = thread->currentFrame();

  Module main(&scope, findModule(&runtime, "__main__"));
  Object left(&scope, moduleAt(&runtime, main, "left"));
  Object right(&scope, moduleAt(&runtime, main, "right"));
  Object c_class(&scope, moduleAt(&runtime, main, "C"));

  Object result_obj(
      &scope, Interpreter::inplaceOperation(
                  thread, frame, Interpreter::BinaryOp::SUB, left, right));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 4);
  EXPECT_EQ(result.at(0), *c_class);
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "__sub__"));
  EXPECT_EQ(result.at(2), *left);
  EXPECT_EQ(result.at(3), *right);
}

TEST(InterpreterTest, InplaceOperationCallsBinaryMethodAfterNotImplemented) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
    def __isub__(self, other):
        return NotImplemented
    def __sub__(self, other):
        return (C, '__sub__', self, other)

left = C()
right = C()
)")
                   .isError());

  Frame* frame = thread->currentFrame();

  Module main(&scope, findModule(&runtime, "__main__"));
  Object left(&scope, moduleAt(&runtime, main, "left"));
  Object right(&scope, moduleAt(&runtime, main, "right"));
  Object c_class(&scope, moduleAt(&runtime, main, "C"));

  Object result_obj(
      &scope, Interpreter::inplaceOperation(
                  thread, frame, Interpreter::BinaryOp::SUB, left, right));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 4);
  EXPECT_EQ(result.at(0), *c_class);
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "__sub__"));
  EXPECT_EQ(result.at(2), *left);
  EXPECT_EQ(result.at(3), *right);
}

TEST(InterpreterTest, InplaceOperationSetMethodSetsMethodFlagsBinopRetry) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class MyInt(int):
  def __isub__(self, other):
    return int(self) - other - 2
v0 = MyInt(9)
v1 = MyInt(-11)
v2 = MyInt(-3)
v3 = MyInt(7)
)")
                   .isError());
  Object v0(&scope, moduleAt(&runtime, "__main__", "v0"));
  Object v1(&scope, moduleAt(&runtime, "__main__", "v1"));
  Object v2(&scope, moduleAt(&runtime, "__main__", "v2"));
  Object v3(&scope, moduleAt(&runtime, "__main__", "v3"));
  Object method(&scope, NoneType::object());
  IcBinopFlags flags;
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::inplaceOperationSetMethod(thread, thread->currentFrame(),
                                             Interpreter::BinaryOp::SUB, v0, v1,
                                             &method, &flags),
      18));
  EXPECT_EQ(flags, IC_INPLACE_BINOP_RETRY);

  ASSERT_EQ(v0.layoutId(), v2.layoutId());
  ASSERT_EQ(v1.layoutId(), v3.layoutId());
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::binaryOperationWithMethod(thread, thread->currentFrame(),
                                             *method, flags, *v2, *v3),
      -12));
}

TEST(InterpreterTest, InplaceOperationSetMethodSetsMethodFlagsReverseRetry) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class MyInt(int):
  pass
class MyIntSub(MyInt):
  def __rpow__(self, other):
    return int(other) ** int(self) - 7
v0 = MyInt(3)
v1 = MyIntSub(3)
v2 = MyInt(-4)
v3 = MyIntSub(4)
)")
                   .isError());
  Object v0(&scope, moduleAt(&runtime, "__main__", "v0"));
  Object v1(&scope, moduleAt(&runtime, "__main__", "v1"));
  Object v2(&scope, moduleAt(&runtime, "__main__", "v2"));
  Object v3(&scope, moduleAt(&runtime, "__main__", "v3"));
  Object method(&scope, NoneType::object());
  IcBinopFlags flags;
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::inplaceOperationSetMethod(thread, thread->currentFrame(),
                                             Interpreter::BinaryOp::POW, v0, v1,
                                             &method, &flags),
      20));
  EXPECT_EQ(flags, IC_BINOP_REFLECTED | IC_BINOP_NOTIMPLEMENTED_RETRY);

  ASSERT_EQ(v0.layoutId(), v2.layoutId());
  ASSERT_EQ(v1.layoutId(), v3.layoutId());
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::binaryOperationWithMethod(thread, thread->currentFrame(),
                                             *method, flags, *v2, *v3),
      249));
}

TEST(InterpreterDeathTest, InvalidOpcode) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  Code code(&scope, newEmptyCode());
  const byte bytecode[] = {NOP, 0, NOP, 0, UNUSED_BYTECODE_202, 17, NOP, 7};
  code.setCode(runtime.newBytesWithAll(bytecode));

  ASSERT_DEATH(thread->run(code), "bytecode 'UNUSED_BYTECODE_202'");
}

// To a rich comparison on two instances of the same type.  In each case, the
// method on the left side of the comparison should be used.
TEST(InterpreterTest, CompareOpSameType) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
    def __init__(self, value):
        self.value = value

    def __lt__(self, other):
        return self.value < other.value

c10 = C(10)
c20 = C(20)
)")
                   .isError());

  Frame* frame = thread->currentFrame();

  ASSERT_TRUE(frame->isSentinelFrame());

  Module main(&scope, findModule(&runtime, "__main__"));
  Object left(&scope, moduleAt(&runtime, main, "c10"));
  Object right(&scope, moduleAt(&runtime, main, "c20"));

  Object left_lt_right(&scope, Interpreter::compareOperation(
                                   thread, frame, CompareOp::LT, left, right));
  EXPECT_EQ(left_lt_right, Bool::trueObj());

  Object right_lt_left(&scope, Interpreter::compareOperation(
                                   thread, frame, CompareOp::LT, right, left));
  EXPECT_EQ(right_lt_left, Bool::falseObj());
}

TEST(InterpreterTest, CompareOpFallback) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
    def __init__(self, value):
        self.value = value

c10 = C(10)
c20 = C(20)
)")
                   .isError());

  Frame* frame = thread->currentFrame();

  ASSERT_TRUE(frame->isSentinelFrame());

  Module main(&scope, findModule(&runtime, "__main__"));
  Object left(&scope, moduleAt(&runtime, main, "c10"));
  Object right(&scope, moduleAt(&runtime, main, "c20"));

  Object left_eq_right(&scope, Interpreter::compareOperation(
                                   thread, frame, CompareOp::EQ, left, right));
  EXPECT_EQ(left_eq_right, Bool::falseObj());
  Object left_ne_right(&scope, Interpreter::compareOperation(
                                   thread, frame, CompareOp::NE, left, right));
  EXPECT_EQ(left_ne_right, Bool::trueObj());

  Object right_eq_left(&scope, Interpreter::compareOperation(
                                   thread, frame, CompareOp::EQ, left, right));
  EXPECT_EQ(right_eq_left, Bool::falseObj());
  Object right_ne_left(&scope, Interpreter::compareOperation(
                                   thread, frame, CompareOp::NE, left, right));
  EXPECT_EQ(right_ne_left, Bool::trueObj());
}

TEST(InterpreterTest, CompareOpSubclass) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  ASSERT_FALSE(runFromCStr(&runtime, R"(
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
)")
                   .isError());

  Frame* frame = thread->currentFrame();
  ASSERT_TRUE(frame->isSentinelFrame());

  Module main(&scope, findModule(&runtime, "__main__"));
  Object a(&scope, moduleAt(&runtime, main, "a"));
  Object b(&scope, moduleAt(&runtime, main, "b"));
  Object c(&scope, moduleAt(&runtime, main, "c"));

  // Comparisons where rhs is not a subtype of lhs try lhs.__eq__(rhs) first.
  Object a_eq_b(&scope, Interpreter::compareOperation(thread, frame,
                                                      CompareOp::EQ, a, b));
  EXPECT_EQ(a_eq_b, Bool::falseObj());
  Object called(&scope, moduleAt(&runtime, main, "called"));
  EXPECT_TRUE(isStrEqualsCStr(*called, "A"));

  Str called_name(&scope, runtime.newStrFromCStr("called"));
  Object none(&scope, NoneType::object());
  runtime.moduleAtPut(main, called_name, none);
  Object b_eq_a(&scope, Interpreter::compareOperation(thread, frame,
                                                      CompareOp::EQ, b, a));
  EXPECT_EQ(b_eq_a, Bool::trueObj());
  called = moduleAt(&runtime, main, "called");
  EXPECT_TRUE(isStrEqualsCStr(*called, "B"));

  runtime.moduleAtPut(main, called_name, none);
  Object c_eq_a(&scope, Interpreter::compareOperation(thread, frame,
                                                      CompareOp::EQ, c, a));
  EXPECT_EQ(c_eq_a, Bool::trueObj());
  called = moduleAt(&runtime, main, "called");
  EXPECT_TRUE(isStrEqualsCStr(*called, "C"));

  // When rhs is a subtype of lhs, only rhs.__eq__(rhs) is tried.
  runtime.moduleAtPut(main, called_name, none);
  Object a_eq_c(&scope, Interpreter::compareOperation(thread, frame,
                                                      CompareOp::EQ, a, c));
  EXPECT_EQ(a_eq_c, Bool::trueObj());
  called = moduleAt(&runtime, main, "called");
  EXPECT_TRUE(isStrEqualsCStr(*called, "C"));
}

TEST(InterpreterTest, CompareOpSetMethodSetsMethod) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object v0(&scope, runtime.newInt(39));
  Object v1(&scope, runtime.newInt(11));
  Object method(&scope, NoneType::object());
  IcBinopFlags flags;
  EXPECT_EQ(Interpreter::compareOperationSetMethod(
                thread, thread->currentFrame(), CompareOp::LT, v0, v1, &method,
                &flags),
            Bool::falseObj());
  EXPECT_TRUE(method.isFunction());
  EXPECT_EQ(flags, IC_BINOP_NOTIMPLEMENTED_RETRY);

  Object v2(&scope, runtime.newInt(3));
  Object v3(&scope, runtime.newInt(8));
  ASSERT_EQ(v0.layoutId(), v2.layoutId());
  ASSERT_EQ(v1.layoutId(), v3.layoutId());
  EXPECT_EQ(Interpreter::binaryOperationWithMethod(
                thread, thread->currentFrame(), *method, flags, *v2, *v3),
            Bool::trueObj());
}

TEST(InterpreterTest, CompareOpSetMethodSetsReverseMethod) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class A:
  def __init__(self, x):
    self.x = x
class ASub(A):
  def __ge__(self, other):
    return (self, other)
v0 = A(3)
v1 = ASub(7)
v2 = A(8)
v3 = ASub(2)
)")
                   .isError());
  Object v0(&scope, moduleAt(&runtime, "__main__", "v0"));
  Object v1(&scope, moduleAt(&runtime, "__main__", "v1"));
  Object v2(&scope, moduleAt(&runtime, "__main__", "v2"));
  Object v3(&scope, moduleAt(&runtime, "__main__", "v3"));
  Object method(&scope, NoneType::object());
  IcBinopFlags flags;
  Object result_obj(&scope, Interpreter::compareOperationSetMethod(
                                thread, thread->currentFrame(), CompareOp::LE,
                                v0, v1, &method, &flags));
  EXPECT_TRUE(method.isFunction());
  EXPECT_EQ(flags, IC_BINOP_REFLECTED);
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 2);
  EXPECT_EQ(result.at(0), v1);
  EXPECT_EQ(result.at(1), v0);

  ASSERT_EQ(v0.layoutId(), v2.layoutId());
  ASSERT_EQ(v1.layoutId(), v3.layoutId());
  result_obj = Interpreter::binaryOperationWithMethod(
      thread, thread->currentFrame(), *method, flags, *v2, *v3);
  ASSERT_TRUE(result_obj.isTuple());
  result = *result_obj;
  ASSERT_EQ(result.length(), 2);
  EXPECT_EQ(result.at(0), v3);
  EXPECT_EQ(result.at(1), v2);
}

TEST(InterpreterTest, CompareOpSetMethodSetsReverseMethodNotImplementedRetry) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class A:
  def __init__(self, x):
    self.x = x
  def __le__(self, other):
    raise UserWarning("should not be called")
class ASub(A):
  def __ge__(self, other):
    return (self, other)
v0 = A(3)
v1 = ASub(7)
v2 = A(8)
v3 = ASub(2)
)")
                   .isError());
  Object v0(&scope, moduleAt(&runtime, "__main__", "v0"));
  Object v1(&scope, moduleAt(&runtime, "__main__", "v1"));
  Object v2(&scope, moduleAt(&runtime, "__main__", "v2"));
  Object v3(&scope, moduleAt(&runtime, "__main__", "v3"));
  Object method(&scope, NoneType::object());
  IcBinopFlags flags;
  Object result_obj(&scope, Interpreter::compareOperationSetMethod(
                                thread, thread->currentFrame(), CompareOp::LE,
                                v0, v1, &method, &flags));
  EXPECT_TRUE(method.isFunction());
  EXPECT_EQ(flags, IC_BINOP_REFLECTED | IC_BINOP_NOTIMPLEMENTED_RETRY);
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 2);
  EXPECT_EQ(result.at(0), v1);
  EXPECT_EQ(result.at(1), v0);

  ASSERT_EQ(v0.layoutId(), v2.layoutId());
  ASSERT_EQ(v1.layoutId(), v3.layoutId());
  result_obj = Interpreter::binaryOperationWithMethod(
      thread, thread->currentFrame(), *method, flags, *v2, *v3);
  ASSERT_TRUE(result_obj.isTuple());
  result = *result_obj;
  ASSERT_EQ(result.length(), 2);
  EXPECT_EQ(result.at(0), v3);
  EXPECT_EQ(result.at(1), v2);
}

TEST(InterpreterTest, SequenceContains) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = {1, 2}

b = 1
c = 3
)")
                   .isError());

  Frame* frame = thread->currentFrame();

  ASSERT_TRUE(frame->isSentinelFrame());
  Module main(&scope, findModule(&runtime, "__main__"));
  Object container(&scope, moduleAt(&runtime, main, "a"));
  Object b(&scope, moduleAt(&runtime, main, "b"));
  Object c(&scope, moduleAt(&runtime, main, "c"));
  Object contains_true(
      &scope, Interpreter::sequenceContains(thread, frame, b, container));
  Object contains_false(
      &scope, Interpreter::sequenceContains(thread, frame, c, container));
  EXPECT_EQ(contains_true, Bool::trueObj());
  EXPECT_EQ(contains_false, Bool::falseObj());
}

TEST(InterpreterTest, SequenceIterSearchWithNoDunderIterRaisesTypeError) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C: pass
container = C()
)")
                   .isError());
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
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  __iter__ = None
container = C()
)")
                   .isError());
  Frame* frame = thread->currentFrame();
  Object container(&scope, moduleAt(&runtime, "__main__", "container"));
  Object val(&scope, NoneType::object());
  Object result(&scope,
                Interpreter::sequenceIterSearch(thread, frame, val, container));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(InterpreterTest, SequenceIterSearchWithNoDunderNextRaisesTypeError) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class D: pass
class C:
  def __iter__(self):
    return D()
container = C()
)")
                   .isError());
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
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class D:
  __next__ = None
class C:
  def __iter__(self):
    return D()
container = C()
)")
                   .isError());
  Frame* frame = thread->currentFrame();
  Object container(&scope, moduleAt(&runtime, "__main__", "container"));
  Object val(&scope, NoneType::object());
  Object result(&scope,
                Interpreter::sequenceIterSearch(thread, frame, val, container));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(InterpreterTest, SequenceIterSearchWithListReturnsTrue) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  List container(&scope, listFromRange(1, 3));
  Object val(&scope, SmallInt::fromWord(2));
  Object result(&scope,
                Interpreter::sequenceIterSearch(thread, frame, val, container));
  ASSERT_FALSE(result.isError());
  EXPECT_EQ(result, Bool::trueObj());
}

TEST(InterpreterTest, SequenceIterSearchWithListReturnsFalse) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object container(&scope, listFromRange(1, 3));
  Object val(&scope, SmallInt::fromWord(5));
  Frame* frame = thread->currentFrame();
  Object result(&scope,
                Interpreter::sequenceIterSearch(thread, frame, val, container));
  ASSERT_FALSE(result.isError());
  EXPECT_EQ(result, Bool::falseObj());
}

TEST(InterpreterTest, SequenceIterSearchWithIterThatRaisesPropagatesException) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  def __iter__(self):
    raise ZeroDivisionError("boom")
container = C()
)")
                   .isError());
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
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, src).isError());
  Module main(&scope, findModule(&runtime, "__main__"));
  Object a(&scope, moduleAt(&runtime, main, "a"));
  EXPECT_TRUE(isIntEqualsWord(*a, 3));
  Object b(&scope, moduleAt(&runtime, main, "b"));
  EXPECT_TRUE(isIntEqualsWord(*b, 2));
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
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  Code code(&scope, newEmptyCode());

  Tuple consts(&scope, runtime.newTuple(1));
  consts.atPut(0, SmallInt::fromWord(42));
  code.setConsts(*consts);

  Tuple names(&scope, runtime.newTuple(1));
  Object key(&scope, runtime.newStrFromCStr("foo"));
  names.atPut(0, *key);
  code.setNames(*names);
  code.setArgcount(2);
  code.setStacksize(1);

  const byte bytecode[] = {LOAD_CONST, 0, RETURN_VALUE, 0};
  code.setCode(runtime.newBytesWithAll(bytecode));

  Object qualname(&scope, Str::empty());
  Object none(&scope, NoneType::object());
  Dict globals(&scope, runtime.newDict());
  Function callee(&scope,
                  Interpreter::makeFunction(thread, qualname, code, none, none,
                                            none, none, globals));
  Tuple defaults(&scope, runtime.newTuple(2));

  defaults.atPut(0, SmallInt::fromWord(1));
  defaults.atPut(1, SmallInt::fromWord(2));
  callee.setDefaults(*defaults);

  // Create a caller frame
  Frame* frame = thread->pushCallFrame(*callee);

  // Save starting value stack top
  RawObject* value_stack_start = frame->valueStackTop();

  // Push function pointer and argument
  frame->pushValue(*callee);
  frame->pushValue(SmallInt::fromWord(1));

  // Make sure we got the right result and stack is back where it should be
  EXPECT_TRUE(isIntEqualsWord(Interpreter::call(thread, frame, 1), 42));
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
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  Code code(&scope, newEmptyCode());

  Tuple consts(&scope, runtime.newTuple(1));
  consts.atPut(0, SmallInt::fromWord(42));
  code.setConsts(*consts);

  Tuple names(&scope, runtime.newTuple(1));
  Object key(&scope, runtime.newStrFromCStr("foo"));
  names.atPut(0, *key);
  code.setNames(*names);
  code.setArgcount(2);
  code.setStacksize(1);

  const byte bytecode[] = {LOAD_CONST, 0, RETURN_VALUE, 0};
  code.setCode(runtime.newBytesWithAll(bytecode));

  Object qualname(&scope, Str::empty());
  Object none(&scope, NoneType::object());
  Dict globals(&scope, runtime.newDict());
  Function callee(&scope,
                  Interpreter::makeFunction(thread, qualname, code, none, none,
                                            none, none, globals));
  Tuple defaults(&scope, runtime.newTuple(2));

  defaults.atPut(0, SmallInt::fromWord(1));
  defaults.atPut(1, SmallInt::fromWord(2));
  callee.setDefaults(*defaults);

  // Create a caller frame
  Frame* frame = thread->pushCallFrame(*callee);

  // Save starting value stack top
  RawObject* value_stack_start = frame->valueStackTop();

  // Push function pointer and argument
  Tuple ex(&scope, runtime.newTuple(1));
  ex.atPut(0, SmallInt::fromWord(2));
  frame->pushValue(*callee);
  frame->pushValue(*ex);

  // Make sure we got the right result and stack is back where it should be
  EXPECT_TRUE(isIntEqualsWord(Interpreter::callEx(thread, frame, 0), 42));
  EXPECT_EQ(value_stack_start, frame->valueStackTop());
}

TEST(InterpreterTest, StackCleanupAfterCallKwFunction) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  // Build the following function
  //    def foo(a=1, b=2):
  //      return 42
  //
  // Then call as "foo(b=4)" and verify that the stack is cleaned up after
  // ex and default argument expansion
  //

  Code code(&scope, newEmptyCode());

  Tuple consts(&scope, runtime.newTuple(1));
  consts.atPut(0, SmallInt::fromWord(42));
  code.setConsts(*consts);

  Tuple names(&scope, runtime.newTuple(1));
  Object key(&scope, runtime.newStrFromCStr("foo"));
  names.atPut(0, *key);
  code.setNames(*names);
  code.setArgcount(2);
  code.setStacksize(1);
  Tuple var_names(&scope, runtime.newTuple(2));
  var_names.atPut(0, runtime.newStrFromCStr("a"));
  var_names.atPut(1, runtime.newStrFromCStr("b"));
  code.setVarnames(*var_names);

  const byte bytecode[] = {LOAD_CONST, 0, RETURN_VALUE, 0};
  code.setCode(runtime.newBytesWithAll(bytecode));

  Object qualname(&scope, Str::empty());
  Object none(&scope, NoneType::object());
  Dict globals(&scope, runtime.newDict());
  Function callee(&scope,
                  Interpreter::makeFunction(thread, qualname, code, none, none,
                                            none, none, globals));
  Tuple defaults(&scope, runtime.newTuple(2));

  defaults.atPut(0, SmallInt::fromWord(1));
  defaults.atPut(1, SmallInt::fromWord(2));
  callee.setDefaults(*defaults);

  // Create a caller frame
  Frame* frame = thread->pushCallFrame(*callee);

  // Save starting value stack top
  RawObject* value_stack_start = frame->valueStackTop();

  // Push function pointer and argument
  Tuple arg_names(&scope, runtime.newTuple(1));
  arg_names.atPut(0, runtime.newStrFromCStr("b"));
  frame->pushValue(*callee);
  frame->pushValue(SmallInt::fromWord(4));
  frame->pushValue(*arg_names);

  // Make sure we got the right result and stack is back where it should be
  EXPECT_TRUE(isIntEqualsWord(Interpreter::callKw(thread, frame, 1), 42));
  EXPECT_EQ(value_stack_start, frame->valueStackTop());
}

TEST(InterpreterTest, LookupMethodInvokesDescriptor) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
def f(): pass

class D:
    def __get__(self, obj, owner):
        return f

class C:
    __call__ = D()

c = C()
  )")
                   .isError());
  Frame* frame = thread->currentFrame();
  ASSERT_TRUE(frame->isSentinelFrame());
  Module main(&scope, findModule(&runtime, "__main__"));
  Object c(&scope, moduleAt(&runtime, main, "c"));
  Object f(&scope, moduleAt(&runtime, main, "f"));
  Object method(&scope, Interpreter::lookupMethod(thread, frame, c,
                                                  SymbolId::kDunderCall));
  EXPECT_EQ(*f, *method);
}

TEST(InterpreterTest, PrepareCallableCallUnpacksBoundMethod) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  def foo():
    pass
meth = C().foo
)")
                   .isError());
  Object meth_obj(&scope, moduleAt(&runtime, "__main__", "meth"));
  ASSERT_TRUE(meth_obj.isBoundMethod());

  Frame* frame = thread->currentFrame();
  frame->pushValue(*meth_obj);
  frame->pushValue(SmallInt::fromWord(1234));
  ASSERT_EQ(frame->valueStackSize(), 2);
  word nargs = 1;
  Object callable(
      &scope, Interpreter::prepareCallableCall(thread, frame, nargs, &nargs));
  ASSERT_TRUE(callable.isFunction());
  ASSERT_EQ(nargs, 2);
  ASSERT_EQ(frame->valueStackSize(), 3);
  EXPECT_TRUE(isIntEqualsWord(frame->peek(0), 1234));
  EXPECT_TRUE(frame->peek(1).isInstance());
  EXPECT_EQ(frame->peek(2), *callable);
}

TEST(InterpreterTest, CallingUncallableRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "(1)()"),
                            LayoutId::kTypeError,
                            "'int' object is not callable"));
}

TEST(InterpreterTest, CallingUncallableDunderCallRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
class C:
  __call__ = 1

c = C()
c()
  )"),
                            LayoutId::kTypeError,
                            "'int' object is not callable"));
}

TEST(InterpreterTest, CallingNonDescriptorDunderCallRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
class D: pass

class C:
  __call__ = D()

c = C()
c()
  )"),
                            LayoutId::kTypeError,
                            "'D' object is not callable"));
}

TEST(InterpreterTest, CallDescriptorReturningUncallableRaisesTypeError) {
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
                            LayoutId::kTypeError,
                            "'int' object is not callable"));
}

TEST(InterpreterTest, LookupMethodLoopsOnCallBoundToDescriptor) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
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
  )")
                   .isError());
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
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
l = [1, 2, 3]
a, b, c = l
)")
                   .isError());
  Module main(&scope, findModule(&runtime, "__main__"));
  Object a(&scope, moduleAt(&runtime, main, "a"));
  Object b(&scope, moduleAt(&runtime, main, "b"));
  Object c(&scope, moduleAt(&runtime, main, "c"));
  EXPECT_TRUE(isIntEqualsWord(*a, 1));
  EXPECT_TRUE(isIntEqualsWord(*b, 2));
  EXPECT_TRUE(isIntEqualsWord(*c, 3));
}

TEST(InterpreterTest, UnpackSequenceTooFewObjects) {
  Runtime runtime;
  const char* src = R"(
l = [1, 2]
a, b, c = l
)";
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, src), LayoutId::kValueError,
                            "not enough values to unpack"));
}

TEST(InterpreterTest, UnpackSequenceTooManyObjects) {
  Runtime runtime;
  const char* src = R"(
l = [1, 2, 3, 4]
a, b, c = l
)";
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, src), LayoutId::kValueError,
                            "too many values to unpack"));
}

TEST(InterpreterTest, PrintExprInvokesDisplayhook) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
import sys

MY_GLOBAL = 1234

def my_displayhook(value):
  global MY_GLOBAL
  MY_GLOBAL = value

sys.displayhook = my_displayhook
  )")
                   .isError());

  Object unique(&scope, runtime.newTuple(1));  // unique object

  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime.newTuple(2));
  consts.atPut(0, *unique);
  consts.atPut(1, NoneType::object());
  code.setConsts(*consts);
  code.setNlocals(0);
  const byte bytecode[] = {LOAD_CONST, 0, PRINT_EXPR,   0,
                           LOAD_CONST, 1, RETURN_VALUE, 0};
  code.setCode(runtime.newBytesWithAll(bytecode));

  ASSERT_TRUE(thread->run(code).isNoneType());

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
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class AsyncIterable:
  def __aiter__(self):
    return 42

a = AsyncIterable()
)")
                   .isError());

  Module main(&scope, findModule(&runtime, "__main__"));
  Object a(&scope, moduleAt(&runtime, main, "a"));

  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime.newTuple(1));
  consts.atPut(0, *a);
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, GET_AITER, 0, RETURN_VALUE, 0};
  code.setCode(runtime.newBytesWithAll(bytecode));

  Object result(&scope, Thread::current()->run(code));
  EXPECT_TRUE(isIntEqualsWord(*result, 42));
}

TEST(InterpreterTest, GetAiterOnNonIterable) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime.newTuple(1));
  consts.atPut(0, SmallInt::fromWord(123));
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, GET_AITER, 0, RETURN_VALUE, 0};
  code.setCode(runtime.newBytesWithAll(bytecode));

  Object result(&scope, Thread::current()->run(code));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(InterpreterTest, BeforeAsyncWithCallsDunderAenter) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
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
  )")
                   .isError());
  Object manager(&scope, moduleAt(&runtime, "__main__", "manager"));
  Object main_obj(&scope, findModule(&runtime, "__main__"));
  ASSERT_TRUE(main_obj.isModule());
  Module main(&scope, *main_obj);

  Code code(&scope, newEmptyCode());
  code.setNlocals(0);
  Tuple consts(&scope, runtime.newTuple(2));
  consts.atPut(0, SmallInt::fromWord(42));
  consts.atPut(1, *manager);
  code.setConsts(*consts);
  Tuple names(&scope, runtime.newTuple(1));
  names.atPut(0, runtime.newStrFromCStr("manager"));
  code.setNames(*names);
  const byte bytecode[] = {LOAD_CONST, 1, BEFORE_ASYNC_WITH, 0, POP_TOP, 0,
                           LOAD_CONST, 0, RETURN_VALUE,      0};
  code.setCode(runtime.newBytesWithAll(bytecode));

  Dict globals(&scope, main.dict());
  Dict locals(&scope, runtime.newDict());
  EXPECT_TRUE(isIntEqualsWord(thread->exec(code, globals, locals), 42));
  Object enter(&scope, moduleAt(&runtime, "__main__", "enter"));
  EXPECT_EQ(*enter, *manager);
  Object exit(&scope, moduleAt(&runtime, "__main__", "exit"));
  EXPECT_EQ(*exit, NoneType::object());
}

TEST(InterpreterTest, SetupAsyncWithPushesBlock) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime.newTuple(2));
  consts.atPut(0, SmallInt::fromWord(42));
  consts.atPut(1, NoneType::object());
  code.setConsts(*consts);
  code.setNlocals(0);
  const byte bytecode[] = {
      LOAD_CONST, 0, LOAD_CONST,   1, SETUP_ASYNC_WITH, 0,
      POP_BLOCK,  0, RETURN_VALUE, 0,
  };
  code.setCode(runtime.newBytesWithAll(bytecode));
  EXPECT_EQ(thread->run(code), SmallInt::fromWord(42));
}

TEST(InterpreterTest, UnpackSequenceEx) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
l = [1, 2, 3, 4, 5, 6, 7]
a, b, c, *d, e, f, g  = l
)")
                   .isError());
  Module main(&scope, findModule(&runtime, "__main__"));
  Object a(&scope, moduleAt(&runtime, main, "a"));
  Object b(&scope, moduleAt(&runtime, main, "b"));
  Object c(&scope, moduleAt(&runtime, main, "c"));
  EXPECT_TRUE(isIntEqualsWord(*a, 1));
  EXPECT_TRUE(isIntEqualsWord(*b, 2));
  EXPECT_TRUE(isIntEqualsWord(*c, 3));

  Object d(&scope, moduleAt(&runtime, main, "d"));
  ASSERT_TRUE(d.isList());
  List list(&scope, *d);
  EXPECT_EQ(list.numItems(), 1);
  EXPECT_TRUE(isIntEqualsWord(list.at(0), 4));

  Object e(&scope, moduleAt(&runtime, main, "e"));
  Object f(&scope, moduleAt(&runtime, main, "f"));
  Object g(&scope, moduleAt(&runtime, main, "g"));
  EXPECT_TRUE(isIntEqualsWord(*e, 5));
  EXPECT_TRUE(isIntEqualsWord(*f, 6));
  EXPECT_TRUE(isIntEqualsWord(*g, 7));
}

TEST(InterpreterTest, UnpackSequenceExWithNoElementsAfter) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
l = [1, 2, 3, 4]
a, b, *c = l
)")
                   .isError());
  Module main(&scope, findModule(&runtime, "__main__"));
  Object a(&scope, moduleAt(&runtime, main, "a"));
  Object b(&scope, moduleAt(&runtime, main, "b"));
  Object c(&scope, moduleAt(&runtime, main, "c"));
  EXPECT_TRUE(isIntEqualsWord(*a, 1));
  EXPECT_TRUE(isIntEqualsWord(*b, 2));

  ASSERT_TRUE(c.isList());
  List list(&scope, *c);
  ASSERT_EQ(list.numItems(), 2);
  EXPECT_TRUE(isIntEqualsWord(list.at(0), 3));
  EXPECT_TRUE(isIntEqualsWord(list.at(1), 4));
}

TEST(InterpreterTest, UnpackSequenceExWithNoElementsBefore) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
l = [1, 2, 3, 4]
*a, b, c = l
)")
                   .isError());
  Module main(&scope, findModule(&runtime, "__main__"));
  Object a(&scope, moduleAt(&runtime, main, "a"));
  Object b(&scope, moduleAt(&runtime, main, "b"));
  Object c(&scope, moduleAt(&runtime, main, "c"));
  ASSERT_TRUE(a.isList());
  List list(&scope, *a);
  ASSERT_EQ(list.numItems(), 2);
  EXPECT_TRUE(isIntEqualsWord(list.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(list.at(1), 2));

  EXPECT_TRUE(isIntEqualsWord(*b, 3));
  EXPECT_TRUE(isIntEqualsWord(*c, 4));
}

TEST(InterpreterTest, BuildMapCallsDunderHashAndPropagatesException) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
class C:
  def __hash__(self):
    raise ValueError('foo')
d = {C(): 4}
)"),
                            LayoutId::kValueError, "foo"));
}

TEST(InterpreterTest, BuildMapUnpackWithDict) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
d = {**{'a': 1, 'b': 2}, 'c': 3, **{'d': 4}}
)")
                   .isError());

  Module main(&scope, findModule(&runtime, "__main__"));
  Object d(&scope, moduleAt(&runtime, main, "d"));
  ASSERT_TRUE(d.isDict());

  Dict dict(&scope, *d);
  EXPECT_EQ(dict.numItems(), 4);

  Object key(&scope, SmallStr::fromCStr("a"));
  Object el0(&scope, runtime.dictAt(thread, dict, key));
  EXPECT_TRUE(isIntEqualsWord(*el0, 1));

  key = SmallStr::fromCStr("b");
  Object el1(&scope, runtime.dictAt(thread, dict, key));
  EXPECT_TRUE(isIntEqualsWord(*el1, 2));

  key = SmallStr::fromCStr("c");
  Object el2(&scope, runtime.dictAt(thread, dict, key));
  EXPECT_TRUE(isIntEqualsWord(*el2, 3));

  key = SmallStr::fromCStr("d");
  Object el3(&scope, runtime.dictAt(thread, dict, key));
  EXPECT_TRUE(isIntEqualsWord(*el3, 4));
}

TEST(InterpreterTest, BuildMapUnpackWithListKeysMapping) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
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
)")
                   .isError());

  Module main(&scope, findModule(&runtime, "__main__"));
  Object d(&scope, moduleAt(&runtime, main, "d"));
  ASSERT_TRUE(d.isDict());

  Dict dict(&scope, *d);
  EXPECT_EQ(dict.numItems(), 4);

  Object key(&scope, SmallStr::fromCStr("a"));
  Object el0(&scope, runtime.dictAt(thread, dict, key));
  EXPECT_TRUE(isIntEqualsWord(*el0, 1));

  key = SmallStr::fromCStr("b");
  Object el1(&scope, runtime.dictAt(thread, dict, key));
  EXPECT_TRUE(isIntEqualsWord(*el1, 2));

  key = SmallStr::fromCStr("c");
  Object el2(&scope, runtime.dictAt(thread, dict, key));
  EXPECT_TRUE(isIntEqualsWord(*el2, 3));

  key = SmallStr::fromCStr("d");
  Object el3(&scope, runtime.dictAt(thread, dict, key));
  EXPECT_TRUE(isIntEqualsWord(*el3, 4));
}

TEST(InterpreterTest, BuildMapUnpackWithTupleKeysMapping) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
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
)")
                   .isError());

  Module main(&scope, findModule(&runtime, "__main__"));
  Object d(&scope, moduleAt(&runtime, main, "d"));
  ASSERT_TRUE(d.isDict());

  Dict dict(&scope, *d);
  EXPECT_EQ(dict.numItems(), 4);

  Object key(&scope, SmallStr::fromCStr("a"));
  Object el0(&scope, runtime.dictAt(thread, dict, key));
  EXPECT_TRUE(isIntEqualsWord(*el0, 1));

  key = SmallStr::fromCStr("b");
  Object el1(&scope, runtime.dictAt(thread, dict, key));
  EXPECT_TRUE(isIntEqualsWord(*el1, 2));

  key = SmallStr::fromCStr("c");
  Object el2(&scope, runtime.dictAt(thread, dict, key));
  EXPECT_TRUE(isIntEqualsWord(*el2, 3));

  key = SmallStr::fromCStr("d");
  Object el3(&scope, runtime.dictAt(thread, dict, key));
  EXPECT_TRUE(isIntEqualsWord(*el3, 4));
}

TEST(InterpreterTest, BuildMapUnpackWithIterableKeysMapping) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
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
)")
                   .isError());

  Module main(&scope, findModule(&runtime, "__main__"));
  Object d(&scope, moduleAt(&runtime, main, "d"));
  ASSERT_TRUE(d.isDict());

  Dict dict(&scope, *d);
  EXPECT_EQ(dict.numItems(), 4);

  Object key(&scope, SmallStr::fromCStr("a"));
  Object el0(&scope, runtime.dictAt(thread, dict, key));
  EXPECT_TRUE(isIntEqualsWord(*el0, 1));

  key = SmallStr::fromCStr("b");
  Object el1(&scope, runtime.dictAt(thread, dict, key));
  EXPECT_TRUE(isIntEqualsWord(*el1, 2));

  key = SmallStr::fromCStr("c");
  Object el2(&scope, runtime.dictAt(thread, dict, key));
  EXPECT_TRUE(isIntEqualsWord(*el2, 3));

  key = SmallStr::fromCStr("d");
  Object el3(&scope, runtime.dictAt(thread, dict, key));
  EXPECT_TRUE(isIntEqualsWord(*el3, 4));
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

TEST(InterpreterTest, BuildSetCallsDunderHashAndPropagatesException) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
class C:
  def __hash__(self):
    raise ValueError('foo')
s = {C()}
)"),
                            LayoutId::kValueError, "foo"));
}

TEST(InterpreterTest, UnpackSequenceExWithTooFewObjectsBefore) {
  Runtime runtime;
  const char* src = R"(
l = [1, 2]
a, b, c, *d  = l
)";
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, src), LayoutId::kValueError,
                            "not enough values to unpack"));
}

TEST(InterpreterTest, UnpackSequenceExWithTooFewObjectsAfter) {
  Runtime runtime;
  const char* src = R"(
l = [1, 2]
*a, b, c, d = l
)";
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, src), LayoutId::kValueError,
                            "not enough values to unpack"));
}

TEST(InterpreterTest, BuildTupleUnpackWithCall) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
def foo(*args):
    return args

t = foo(*(1,2), *(3, 4))
)")
                   .isError());

  Module main(&scope, findModule(&runtime, "__main__"));
  Object t(&scope, moduleAt(&runtime, main, "t"));
  ASSERT_TRUE(t.isTuple());

  Tuple tuple(&scope, *t);
  EXPECT_TRUE(isIntEqualsWord(tuple.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(tuple.at(1), 2));
  EXPECT_TRUE(isIntEqualsWord(tuple.at(2), 3));
  EXPECT_TRUE(isIntEqualsWord(tuple.at(3), 4));
}

TEST(InterpreterTest, FunctionDerefsVariable) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
def outer():
    var = 1
    def inner():
        return var
    del var
    return 0

v = outer()
	)")
                   .isError());

  Module main(&scope, findModule(&runtime, "__main__"));
  Object v(&scope, moduleAt(&runtime, main, "v"));
  EXPECT_TRUE(isIntEqualsWord(*v, 0));
}

TEST(InterpreterTest, FunctionAccessesUnboundVariable) {
  Runtime runtime;
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
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  const char* module_src = R"(
def public_symbol():
    return 1
def public_symbol2():
    return 2
)";

  // Preload the module
  Object name(&scope, runtime.newStrFromCStr("test_module"));
  std::unique_ptr<char[]> buffer(
      Runtime::compileFromCStr(module_src, "<test string>"));
  ASSERT_FALSE(runtime.importModuleFromBuffer(buffer.get(), name).isError());

  ASSERT_FALSE(runFromCStr(&runtime, R"(
from test_module import *
a = public_symbol()
b = public_symbol2()
)")
                   .isError());

  Module main(&scope, findModule(&runtime, "__main__"));
  Object a(&scope, moduleAt(&runtime, main, "a"));
  Object b(&scope, moduleAt(&runtime, main, "b"));
  EXPECT_TRUE(isIntEqualsWord(*a, 1));
  EXPECT_TRUE(isIntEqualsWord(*b, 2));
}

TEST(InterpreterTest, ImportStarDoesNotImportPrivateSymbols) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  const char* module_src = R"(
def public_symbol():
    return 1
def _private_symbol():
    return 2
)";

  // Preload the module
  Object name(&scope, runtime.newStrFromCStr("test_module"));
  std::unique_ptr<char[]> buffer(
      Runtime::compileFromCStr(module_src, "<test string>"));
  ASSERT_FALSE(runtime.importModuleFromBuffer(buffer.get(), name).isError());

  const char* main_src = R"(
from test_module import *
a = public_symbol()
b = _private_symbol()
)";

  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, main_src),
                            LayoutId::kNameError,
                            "name '_private_symbol' is not defined"));
}

TEST(InterpreterTest, ImportCallsBuiltinsDunderImport) {
  Runtime runtime;
  ASSERT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
import builtins
def import_forbidden(name, globals, locals, fromlist, level):
  raise Exception("import forbidden")
builtins.__import__ = import_forbidden
import builtins
)"),
                            LayoutId::kException, "import forbidden"));
}

TEST(InterpreterTest, GetAnextCallsAnextAndAwait) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
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
)")
                   .isError());
  Module main(&scope, findModule(&runtime, "__main__"));
  Object a(&scope, moduleAt(&runtime, main, "a"));

  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime.newTuple(1));
  consts.atPut(0, *a);
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, GET_ANEXT, 0, RETURN_VALUE, 0};
  code.setCode(runtime.newBytesWithAll(bytecode));

  Object result(&scope, Thread::current()->run(code));
  EXPECT_EQ(*a, *result);
  Object anext(&scope, moduleAt(&runtime, main, "anext_called"));
  EXPECT_EQ(*a, *anext);
  Object await(&scope, moduleAt(&runtime, main, "await_called"));
  EXPECT_EQ(*a, *await);
}

TEST(InterpreterTest, GetAnextOnNonIterable) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime.newTuple(1));
  consts.atPut(0, SmallInt::fromWord(123));
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, GET_ANEXT, 0, RETURN_VALUE, 0};
  code.setCode(runtime.newBytesWithAll(bytecode));

  Object result(&scope, Thread::current()->run(code));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(InterpreterTest, GetAnextWithInvalidAnext) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class AsyncIterator:
  def __anext__(self):
    return 42

a = AsyncIterator()
)")
                   .isError());
  Module main(&scope, findModule(&runtime, "__main__"));
  Object a(&scope, moduleAt(&runtime, main, "a"));

  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime.newTuple(1));
  consts.atPut(0, *a);
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, GET_ANEXT, 0, RETURN_VALUE, 0};
  code.setCode(runtime.newBytesWithAll(bytecode));

  Object result(&scope, Thread::current()->run(code));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(InterpreterTest, GetAwaitableCallsAwait) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Awaitable:
  def __await__(self):
    return 42

a = Awaitable()
)")
                   .isError());

  Module main(&scope, findModule(&runtime, "__main__"));
  Object a(&scope, moduleAt(&runtime, main, "a"));

  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime.newTuple(1));
  consts.atPut(0, *a);
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, GET_AWAITABLE, 0, RETURN_VALUE, 0};
  code.setCode(runtime.newBytesWithAll(bytecode));

  Object result(&scope, Thread::current()->run(code));
  EXPECT_TRUE(isIntEqualsWord(*result, 42));
}

TEST(InterpreterTest, GetAwaitableOnNonAwaitable) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime.newTuple(1));
  consts.atPut(0, runtime.newStrFromCStr("foo"));
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, GET_AWAITABLE, 0, RETURN_VALUE, 0};
  code.setCode(runtime.newBytesWithAll(bytecode));

  Object result(&scope, Thread::current()->run(code));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(InterpreterTest, BuildMapUnpackWithCallDict) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
def foo(**kwargs):
    return kwargs

d = foo(**{'a': 1, 'b': 2}, **{'c': 3, 'd': 4})
)")
                   .isError());

  Module main(&scope, findModule(&runtime, "__main__"));
  Object d(&scope, moduleAt(&runtime, main, "d"));
  ASSERT_TRUE(d.isDict());

  Dict dict(&scope, *d);
  EXPECT_EQ(dict.numItems(), 4);

  Object key(&scope, SmallStr::fromCStr("a"));
  Object el0(&scope, runtime.dictAt(thread, dict, key));
  EXPECT_TRUE(isIntEqualsWord(*el0, 1));

  key = SmallStr::fromCStr("b");
  Object el1(&scope, runtime.dictAt(thread, dict, key));
  EXPECT_TRUE(isIntEqualsWord(*el1, 2));

  key = SmallStr::fromCStr("c");
  Object el2(&scope, runtime.dictAt(thread, dict, key));
  EXPECT_TRUE(isIntEqualsWord(*el2, 3));

  key = SmallStr::fromCStr("d");
  Object el3(&scope, runtime.dictAt(thread, dict, key));
  EXPECT_TRUE(isIntEqualsWord(*el3, 4));
}

TEST(InterpreterTest, BuildMapUnpackWithCallTupleKeys) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
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
)")
                   .isError());

  Module main(&scope, findModule(&runtime, "__main__"));
  Object d(&scope, moduleAt(&runtime, main, "d"));
  ASSERT_TRUE(d.isDict());

  Dict dict(&scope, *d);
  EXPECT_EQ(dict.numItems(), 4);

  Object key(&scope, SmallStr::fromCStr("a"));
  Object el0(&scope, runtime.dictAt(thread, dict, key));
  EXPECT_TRUE(isIntEqualsWord(*el0, 1));

  key = SmallStr::fromCStr("b");
  Object el1(&scope, runtime.dictAt(thread, dict, key));
  EXPECT_TRUE(isIntEqualsWord(*el1, 2));

  key = SmallStr::fromCStr("c");
  Object el2(&scope, runtime.dictAt(thread, dict, key));
  EXPECT_TRUE(isIntEqualsWord(*el2, 3));

  key = SmallStr::fromCStr("d");
  Object el3(&scope, runtime.dictAt(thread, dict, key));
  EXPECT_TRUE(isIntEqualsWord(*el3, 4));
}

TEST(InterpreterTest, BuildMapUnpackWithCallListKeys) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
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
)")
                   .isError());

  Module main(&scope, findModule(&runtime, "__main__"));
  Object d(&scope, moduleAt(&runtime, main, "d"));
  ASSERT_TRUE(d.isDict());

  Dict dict(&scope, *d);
  EXPECT_EQ(dict.numItems(), 4);

  Object key(&scope, SmallStr::fromCStr("a"));
  Object el0(&scope, runtime.dictAt(thread, dict, key));
  EXPECT_TRUE(isIntEqualsWord(*el0, 1));

  key = SmallStr::fromCStr("b");
  Object el1(&scope, runtime.dictAt(thread, dict, key));
  EXPECT_TRUE(isIntEqualsWord(*el1, 2));

  key = SmallStr::fromCStr("c");
  Object el2(&scope, runtime.dictAt(thread, dict, key));
  EXPECT_TRUE(isIntEqualsWord(*el2, 3));

  key = SmallStr::fromCStr("d");
  Object el3(&scope, runtime.dictAt(thread, dict, key));
  EXPECT_TRUE(isIntEqualsWord(*el3, 4));
}

TEST(InterpreterTest, BuildMapUnpackWithCallIteratorKeys) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
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
)")
                   .isError());

  Module main(&scope, findModule(&runtime, "__main__"));
  Object d(&scope, moduleAt(&runtime, main, "d"));
  ASSERT_TRUE(d.isDict());

  Dict dict(&scope, *d);
  EXPECT_EQ(dict.numItems(), 4);

  Object key(&scope, SmallStr::fromCStr("a"));
  Object el0(&scope, runtime.dictAt(thread, dict, key));
  EXPECT_TRUE(isIntEqualsWord(*el0, 1));

  key = SmallStr::fromCStr("b");
  Object el1(&scope, runtime.dictAt(thread, dict, key));
  EXPECT_TRUE(isIntEqualsWord(*el1, 2));

  key = SmallStr::fromCStr("c");
  Object el2(&scope, runtime.dictAt(thread, dict, key));
  EXPECT_TRUE(isIntEqualsWord(*el2, 3));

  key = SmallStr::fromCStr("d");
  Object el3(&scope, runtime.dictAt(thread, dict, key));
  EXPECT_TRUE(isIntEqualsWord(*el3, 4));
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
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  ASSERT_FALSE(runFromCStr(&runtime, R"(
class FooIterator:
    pass

class Foo:
    def __iter__(self):
        return FooIterator()

foo = Foo()
	)")
                   .isError());

  Module main(&scope, findModule(&runtime, "__main__"));
  Object foo(&scope, moduleAt(&runtime, main, "foo"));

  // Create a code object and set the foo instance as a const
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime.newTuple(1));
  consts.atPut(0, *foo);
  code.setConsts(*consts);

  // Python code:
  // foo = Foo()
  // def bar():
  //     yield from foo
  const byte bytecode[] = {
      LOAD_CONST,          0,  // (foo)
      GET_YIELD_FROM_ITER, 0,  // iter(foo)
      RETURN_VALUE,        0,
  };
  code.setCode(runtime.newBytesWithAll(bytecode));

  // Confirm that the returned value is the iterator of Foo
  Object result(&scope, Thread::current()->run(code));
  Type result_type(&scope, runtime.typeOf(*result));
  EXPECT_TRUE(isStrEqualsCStr(result_type.name(), "FooIterator"));
}

TEST(InterpreterTest, YieldFromIterRaisesException) {
  Runtime runtime;

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
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object module_name(&scope, runtime.newStrFromCStr("foo"));
  std::unique_ptr<char[]> buffer(Runtime::compileFromCStr(R"(
def bar(): pass
)",
                                                          "<test string>"));
  ASSERT_FALSE(
      runtime.importModuleFromBuffer(buffer.get(), module_name).isError());
  ASSERT_FALSE(runFromCStr(&runtime, R"(
import foo
def baz(): pass
a = getattr(foo.bar, '__module__')
b = getattr(baz, '__module__')
)")
                   .isError());
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  ASSERT_TRUE(a.isStr());
  EXPECT_TRUE(Str::cast(*a).equalsCStr("foo"));
  Object b(&scope, moduleAt(&runtime, "__main__", "b"));
  ASSERT_TRUE(b.isStr());
  EXPECT_TRUE(Str::cast(*b).equalsCStr("__main__"));
}

TEST(InterpreterTest, MakeFunctionSetsDunderQualname) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo():
    def bar(): pass
def baz(): pass
a = getattr(Foo.bar, '__qualname__')
b = getattr(baz, '__qualname__')
)")
                   .isError());
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  ASSERT_TRUE(a.isStr());
  EXPECT_TRUE(Str::cast(*a).equalsCStr("Foo.bar"));
  Object b(&scope, moduleAt(&runtime, "__main__", "b"));
  ASSERT_TRUE(b.isStr());
  EXPECT_TRUE(Str::cast(*b).equalsCStr("baz"));
}

TEST(InterpreterTest, MakeFunctionSetsDunderDoc) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
def foo():
    """This is a docstring"""
    pass
def bar(): pass
)")
                   .isError());
  Object foo(&scope, testing::moduleAt(&runtime, "__main__", "foo"));
  ASSERT_TRUE(foo.isFunction());
  Object foo_docstring(&scope, Function::cast(*foo).doc());
  ASSERT_TRUE(foo_docstring.isStr());
  EXPECT_TRUE(Str::cast(*foo_docstring).equalsCStr("This is a docstring"));

  Object bar(&scope, testing::moduleAt(&runtime, "__main__", "bar"));
  ASSERT_TRUE(bar.isFunction());
  Object bar_docstring(&scope, Function::cast(*bar).doc());
  EXPECT_TRUE(bar_docstring.isNoneType());
}

TEST(InterpreterTest, FunctionCallWithNonFunctionRaisesTypeError) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  Str not_a_func(&scope, Str::empty());
  frame->pushValue(*not_a_func);
  EXPECT_TRUE(
      raised(Interpreter::call(thread, frame, 0), LayoutId::kTypeError));
}

TEST(InterpreterTest, FunctionCallExWithNonFunctionRaisesTypeError) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  Str not_a_func(&scope, Str::empty());
  frame->pushValue(*not_a_func);
  Tuple empty_args(&scope, runtime.newTuple(0));
  frame->pushValue(*empty_args);
  EXPECT_TRUE(raisedWithStr(Interpreter::callEx(thread, frame, 0),
                            LayoutId::kTypeError,
                            "'str' object is not callable"));
}

TEST(InterpreterTest, CallExWithDescriptorDunderCall) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class FakeFunc:
    def __get__(self, obj, owner):
        return self
    def __call__(self, arg):
        return arg

class C:
    __call__ = FakeFunc()

args = ["hello!"]
result = C()(*args)
)")
                   .isError());
  EXPECT_TRUE(
      isStrEqualsCStr(moduleAt(&runtime, "__main__", "result"), "hello!"));
}

TEST(InterpreterTest, DoDeleteNameOnDictSubclass) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class MyDict(dict): pass
class Meta(type):
  @classmethod
  def __prepare__(cls, *args, **kwargs):
    d = MyDict()
    d['x'] = 42
    return d
class C(metaclass=Meta):
  del x
)")
                   .isError());
}

TEST(InterpreterTest, DoStoreNameOnDictSubclass) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class MyDict(dict): pass
class Meta(type):
  @classmethod
  def __prepare__(cls, *args, **kwargs):
    return MyDict()
class C(metaclass=Meta):
  x = 42
)")
                   .isError());
}

TEST(InterpreterTest, StoreSubscr) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
l = [0]
for i in range(5):
    l[0] += i
)")
                   .isError());

  HandleScope scope;
  Object l_obj(&scope, testing::moduleAt(&runtime, "__main__", "l"));
  ASSERT_TRUE(l_obj.isList());
  List l(&scope, *l_obj);
  ASSERT_EQ(l.numItems(), 1);
  EXPECT_EQ(l.at(0), SmallInt::fromWord(10));
}

// TODO(bsimmers) Rewrite these exception tests to ensure that the specific
// bytecodes we care about are being exercised, so we're not be at the mercy of
// compiler optimizations or changes.
TEST(InterpreterTest, ExceptCatchesException) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
n = 0
try:
    raise RuntimeError("something went wrong")
    n = 1
except:
    if n == 0:
        n = 2
)")
                   .isError());

  HandleScope scope;
  Object n(&scope, testing::moduleAt(&runtime, "__main__", "n"));
  EXPECT_TRUE(isIntEqualsWord(*n, 2));
}

TEST(InterpreterTest, RaiseCrossesFunctions) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
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
)")
                   .isError());

  HandleScope scope;
  Object n(&scope, testing::moduleAt(&runtime, "__main__", "n"));
  EXPECT_TRUE(isIntEqualsWord(*n, 2));
}

TEST(InterpreterTest, RaiseFromSetsCause) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
try:
  try:
    raise RuntimeError
  except Exception as e:
    raise TypeError from e
except Exception as e:
  exc = e
)")
                   .isError());

  HandleScope scope;
  Object exc_obj(&scope, testing::moduleAt(&runtime, "__main__", "exc"));
  ASSERT_EQ(exc_obj.layoutId(), LayoutId::kTypeError);
  BaseException exc(&scope, *exc_obj);
  EXPECT_EQ(exc.cause().layoutId(), LayoutId::kRuntimeError);
}

TEST(InterpreterTest, ExceptWithRightTypeCatches) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
n = 0
try:
    raise RuntimeError("whoops")
    n = 1
except RuntimeError:
    if n == 0:
        n = 2
)")
                   .isError());

  HandleScope scope;
  Object n(&scope, testing::moduleAt(&runtime, "__main__", "n"));
  EXPECT_TRUE(isIntEqualsWord(*n, 2));
}

TEST(InterpreterTest, ExceptWithRightTupleTypeCatches) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
n = 0
try:
    raise RuntimeError()
    n = 1
except (StopIteration, RuntimeError, ImportError):
    if n == 0:
        n = 2
)")
                   .isError());

  HandleScope scope;
  Object n(&scope, testing::moduleAt(&runtime, "__main__", "n"));
  EXPECT_TRUE(isIntEqualsWord(*n, 2));
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
  ASSERT_FALSE(runFromCStr(&runtime, R"(
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
)")
                   .isError());

  HandleScope scope;
  Object my_error(&scope, testing::moduleAt(&runtime, "__main__", "MyError"));
  EXPECT_EQ(runtime.typeOf(*my_error), runtime.typeAt(LayoutId::kType));
  Object inner(&scope, testing::moduleAt(&runtime, "__main__", "inner"));
  EXPECT_EQ(runtime.typeOf(*inner), *my_error);
  Object outer(&scope, testing::moduleAt(&runtime, "__main__", "outer"));
  EXPECT_EQ(*inner, *outer);
}

TEST(InterpreterTest, ExceptWithNonExceptionTypeRaisesTypeError) {
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

TEST(InterpreterTest, ExceptWithNonExceptionTypeInTupleRaisesTypeError) {
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

TEST(InterpreterTest, RaiseWithNoActiveExceptionRaisesRuntimeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "raise\n"),
                            LayoutId::kRuntimeError,
                            "No active exception to reraise"));
}

TEST(InterpreterTest, LoadAttrSetLocationSetsLocation) {
  Runtime runtime;
  runtime.enableCache();
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  def __init__(self):
    self.foo = 42
i = C()
)")
                   .isError());
  Object i(&scope, moduleAt(&runtime, "__main__", "i"));

  Object name(&scope, runtime.newStrFromCStr("foo"));
  Object to_cache(&scope, NoneType::object());
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::loadAttrSetLocation(thread, i, name, &to_cache), 42));
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::loadAttrWithLocation(thread, *i, *to_cache), 42));
}

TEST(InterpreterTest, LoadAttrSetLocationWithCustomGetAttributeSetsNoLocation) {
  Runtime runtime;
  runtime.enableCache();
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  def __getattribute__(self, name):
    return 11
i = C()
)")
                   .isError());
  Object i(&scope, moduleAt(&runtime, "__main__", "i"));

  Object name(&scope, runtime.newStrFromCStr("foo"));
  Object to_cache(&scope, NoneType::object());
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::loadAttrSetLocation(thread, i, name, &to_cache), 11));
  EXPECT_TRUE(to_cache.isNoneType());
}

TEST(InterpreterTest, LoadAttrSetLocationCallsDunderGetattr) {
  Runtime runtime;
  runtime.enableCache();
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  def __init__(self):
    self.foo = 42
  def __getattr__(self, name):
    return 5
i = C()
)")
                   .isError());
  Object i(&scope, moduleAt(&runtime, "__main__", "i"));

  Object name(&scope, runtime.newStrFromCStr("bar"));
  Object to_cache(&scope, NoneType::object());
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::loadAttrSetLocation(thread, i, name, &to_cache), 5));
  EXPECT_TRUE(to_cache.isNoneType());
}

TEST(InterpreterTest, LoadAttrSetLocationWithNoAttributeRaisesAttributeError) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
obj = object()
)")
                   .isError());

  Object obj(&scope, moduleAt(&runtime, "__main__", "obj"));
  Str name(&scope, runtime.newStrFromCStr("nonexistent_attr"));
  EXPECT_TRUE(raisedWithStr(
      Interpreter::loadAttrSetLocation(thread, obj, name, nullptr),
      LayoutId::kAttributeError,
      "'object' object has no attribute 'nonexistent_attr'"));
}

TEST(InterpreterTest, LoadAttrWithoutAttrUnwindsAttributeException) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  // Set up a code object that runs: {}.foo
  Code code(&scope, newEmptyCode());
  Tuple names(&scope, runtime.newTuple(1));
  Str foo(&scope, runtime.newStrFromCStr("foo"));
  names.atPut(0, *foo);
  code.setNames(*names);

  // load arguments and execute the code
  const byte bytecode[] = {BUILD_MAP, 0, LOAD_ATTR, 0};
  code.setCode(runtime.newBytesWithAll(bytecode));
  code.setStacksize(1);

  // Execute the code and make sure to get the unwinded Error
  EXPECT_TRUE(thread->run(code).isError());
}

TEST(InterpreterTest, ExplodeCallAcceptsList) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
def f(a, b):
  return [b, a]

args = ['a', 'b']
result = f(*args)
)")
                   .isError());

  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_PYLIST_EQ(result, {"b", "a"});
}

TEST(InterpreterDeathTest, ExplodeWithIterableCalls) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  EXPECT_FALSE(runFromCStr(&runtime, R"(
def f(a, b):
  return (b, a)
def gen():
  yield 1
  yield 2
result = f(*gen())
)")
                   .isError());

  Object result_obj(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 2));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 1));
}

TEST(InterpreterTest, FormatValueCallsDunderStr) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  def __str__(self):
    return "foobar"
result = f"{C()!s}"
)")
                   .isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isStrEqualsCStr(*result, "foobar"));
}

TEST(InterpreterTest, FormatValueFallsBackToDunderRepr) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  def __repr__(self):
    return "foobar"
result = f"{C()!s}"
)")
                   .isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isStrEqualsCStr(*result, "foobar"));
}

TEST(InterpreterTest, FormatValueCallsDunderRepr) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  def __repr__(self):
    return "foobar"
result = f"{C()!r}"
)")
                   .isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isStrEqualsCStr(*result, "foobar"));
}

TEST(InterpreterTest, FormatValueAsciiCallsDunderRepr) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  def __repr__(self):
    return "foobar"
result = f"{C()!a}"
)")
                   .isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isStrEqualsCStr(*result, "foobar"));
}

TEST(InterpreterTest, BreakInTryBreaks) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = 0
for i in range(5):
  try:
    break
  except:
    pass
result = 10
)")
                   .isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 10));
}

TEST(InterpreterTest, ContinueInExceptContinues) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = 0
for i in range(5):
  try:
    if i == 3:
      raise RuntimeError()
  except:
    result += i
    continue
  result -= i
)")
                   .isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, -4));
}

TEST(InterpreterTest, RaiseInLoopRaisesRuntimeError) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = 0
try:
  for i in range(5):
    result += i
    if i == 2:
      raise RuntimeError()
  result += 100
except:
  result += 1000
)")
                   .isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 1003));
}

TEST(InterpreterTest, ReturnInsideTryRunsFinally) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
ran_finally = False

def f():
  try:
    return 56789
  finally:
    global ran_finally
    ran_finally = True

result = f()
)")
                   .isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 56789));

  Object ran_finally(&scope, moduleAt(&runtime, "__main__", "ran_finally"));
  EXPECT_EQ(*ran_finally, Bool::trueObj());
}

TEST(InterpreterTest, ReturnInsideFinallyOverridesEarlierReturn) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
def f():
  try:
    return 123
  finally:
    return 456

result = f()
)")
                   .isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 456));
}

TEST(InterpreterTest, ReturnInsideWithRunsDunderExit) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
sequence = ""

class Mgr:
    def __enter__(self):
        global sequence
        sequence += "enter "
    def __exit__(self, exc, value, tb):
        global sequence
        sequence += "exit"

def foo():
    with Mgr():
        global sequence
        sequence += "in foo "
        return 1234

result = foo()
)")
                   .isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 1234));

  Object sequence(&scope, moduleAt(&runtime, "__main__", "sequence"));
  EXPECT_TRUE(isStrEqualsCStr(*sequence, "enter in foo exit"));
}

TEST(InterpreterTest,
     WithStatementWithManagerWithoutEnterRaisesAttributeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
with None:
  pass
)"),
                            LayoutId::kAttributeError, "__enter__"));
}

TEST(InterpreterTest, WithStatementWithManagerWithoutExitRaisesAttributeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
class C:
  def __enter__(self):
    pass
with C():
  pass
)"),
                            LayoutId::kAttributeError, "__exit__"));
}

TEST(InterpreterTest, WithStatementWithManagerEnterRaisingPropagatesException) {
  Runtime runtime;
  EXPECT_TRUE(raised(runFromCStr(&runtime, R"(
class C:
  def __enter__(self):
    raise UserWarning('')
  def __exit__(self, *args):
    pass
with C():
  pass
)"),
                     LayoutId::kUserWarning));
}

TEST(InterpreterTest, WithStatementPropagatesException) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
class Mgr:
    def __enter__(self):
        pass
    def __exit__(self, exc, value, tb):
        return ()

def raises():
  raise RuntimeError("It's dead, Jim")

with Mgr():
  raises()
)"),
                            LayoutId::kRuntimeError, "It's dead, Jim"));
}

TEST(InterpreterTest, WithStatementPassesCorrectExceptionToExit) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  EXPECT_TRUE(raised(runFromCStr(&runtime, R"(
raised_exc = None
exit_info = None

class Mgr:
  def __enter__(self):
    pass
  def __exit__(self, exc, value, tb):
    global exit_info
    exit_info = (exc, value, tb)

def raises():
  global raised_exc
  raised_exc = StopIteration("nope")
  raise raised_exc

with Mgr():
  raises()
)"),
                     LayoutId::kStopIteration));
  Object exit_info(&scope, moduleAt(&runtime, "__main__", "exit_info"));
  ASSERT_TRUE(exit_info.isTuple());
  Tuple tuple(&scope, *exit_info);
  ASSERT_EQ(tuple.length(), 3);
  EXPECT_EQ(tuple.at(0), runtime.typeAt(LayoutId::kStopIteration));

  Object raised_exc(&scope, moduleAt(&runtime, "__main__", "raised_exc"));
  EXPECT_EQ(tuple.at(1), *raised_exc);

  // TODO(bsimmers): Check traceback once we record them.
}

TEST(InterpreterTest, WithStatementSwallowsException) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  EXPECT_FALSE(runFromCStr(&runtime, R"(
class Mgr:
  def __enter__(self):
    pass
  def __exit__(self, exc, value, tb):
    return 1

def raises():
  raise RuntimeError()

with Mgr():
  raises()
result = 1234
)")
                   .isError());

  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 1234));
}

TEST(InterpreterTest, WithStatementWithRaisingExitRaises) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
class Mgr:
  def __enter__(self):
    pass
  def __exit__(self, exc, value, tb):
    raise RuntimeError("from exit")

def raises():
  raise RuntimeError("from raises")

with Mgr():
  raises()
)"),
                            LayoutId::kRuntimeError, "from exit"));

  // TODO(T40269344): Inspect __context__ from the raised exception.
}

}  // namespace python
