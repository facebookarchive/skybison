#include "gtest/gtest.h"

#include <memory>

#include "builtins-module.h"
#include "bytecode.h"
#include "dict-builtins.h"
#include "frame.h"
#include "handles.h"
#include "ic.h"
#include "interpreter.h"
#include "intrinsic.h"
#include "module-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "trampolines.h"
#include "type-builtins.h"

#include "test-utils.h"

namespace py {
using namespace testing;

using InterpreterDeathTest = RuntimeFixture;
using InterpreterTest = RuntimeFixture;

TEST_F(InterpreterTest, IsTrueBool) {
  HandleScope scope(thread_);

  Frame* frame = thread_->currentFrame();

  ASSERT_TRUE(frame->isSentinel());

  Object true_value(&scope, Bool::trueObj());
  EXPECT_EQ(Interpreter::isTrue(thread_, *true_value), Bool::trueObj());

  Object false_object(&scope, Bool::falseObj());
  EXPECT_EQ(Interpreter::isTrue(thread_, *false_object), Bool::falseObj());
}

TEST_F(InterpreterTest, IsTrueInt) {
  HandleScope scope(thread_);

  Frame* frame = thread_->currentFrame();

  ASSERT_TRUE(frame->isSentinel());

  Object true_value(&scope, runtime_.newInt(1234));
  EXPECT_EQ(Interpreter::isTrue(thread_, *true_value), Bool::trueObj());

  Object false_value(&scope, runtime_.newInt(0));
  EXPECT_EQ(Interpreter::isTrue(thread_, *false_value), Bool::falseObj());
}

TEST_F(InterpreterTest, IsTrueWithDunderBoolRaisingPropagatesException) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Foo:
  def __bool__(self):
    raise UserWarning('')
value = Foo()
)")
                   .isError());
  Object value(&scope, mainModuleAt(&runtime_, "value"));
  Object result(&scope, Interpreter::isTrue(thread_, *value));
  EXPECT_TRUE(raised(*result, LayoutId::kUserWarning));
}

TEST_F(InterpreterTest, IsTrueWithDunderLenRaisingPropagatesException) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Foo:
  def __len__(self):
    raise UserWarning('')
value = Foo()
)")
                   .isError());
  Object value(&scope, mainModuleAt(&runtime_, "value"));
  Object result(&scope, Interpreter::isTrue(thread_, *value));
  EXPECT_TRUE(raised(*result, LayoutId::kUserWarning));
}

TEST_F(InterpreterTest, IsTrueWithIntSubclassDunderLenUsesBaseInt) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
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
  Object true_value(&scope, mainModuleAt(&runtime_, "true_value"));
  Object false_value(&scope, mainModuleAt(&runtime_, "false_value"));
  EXPECT_EQ(Interpreter::isTrue(thread_, *true_value), Bool::trueObj());
  EXPECT_EQ(Interpreter::isTrue(thread_, *false_value), Bool::falseObj());
}

TEST_F(InterpreterTest, IsTrueDunderLen) {
  HandleScope scope(thread_);

  Frame* frame = thread_->currentFrame();

  ASSERT_TRUE(frame->isSentinel());

  List nonempty_list(&scope, runtime_.newList());
  Object elt(&scope, NoneType::object());
  runtime_.listAdd(thread_, nonempty_list, elt);

  EXPECT_EQ(Interpreter::isTrue(thread_, *nonempty_list), Bool::trueObj());

  List empty_list(&scope, runtime_.newList());
  EXPECT_EQ(Interpreter::isTrue(thread_, *empty_list), Bool::falseObj());
}

TEST_F(InterpreterTest, UnaryOperationWithIntReturnsInt) {
  HandleScope scope(thread_);
  Object value(&scope, runtime_.newInt(23));
  Object result(&scope, Interpreter::unaryOperation(thread_, value,
                                                    SymbolId::kDunderPos));
  EXPECT_TRUE(isIntEqualsWord(*result, 23));
}

TEST_F(InterpreterTest, UnaryOperationWithBadTypeRaisesTypeError) {
  HandleScope scope(thread_);
  Object value(&scope, NoneType::object());
  Object result(&scope, Interpreter::unaryOperation(thread_, value,
                                                    SymbolId::kDunderInvert));
  EXPECT_TRUE(
      raisedWithStr(*result, LayoutId::kTypeError,
                    "bad operand type for unary '__invert__': 'NoneType'"));
}

TEST_F(InterpreterTest, UnaryOperationWithCustomDunderInvertReturnsString) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  def __invert__(self):
    return "custom invert"
c = C()
)")
                   .isError());
  Object c(&scope, mainModuleAt(&runtime_, "c"));
  Object result(
      &scope, Interpreter::unaryOperation(thread_, c, SymbolId::kDunderInvert));
  EXPECT_TRUE(isStrEqualsCStr(*result, "custom invert"));
}

TEST_F(InterpreterTest, UnaryOperationWithCustomRaisingDunderNegPropagates) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  def __neg__(self):
    raise UserWarning('')
c = C()
)")
                   .isError());
  Object c(&scope, mainModuleAt(&runtime_, "c"));
  Object result(&scope,
                Interpreter::unaryOperation(thread_, c, SymbolId::kDunderNeg));
  EXPECT_TRUE(raised(*result, LayoutId::kUserWarning));
}

TEST_F(InterpreterTest, UnaryNotWithRaisingDunderBool) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
class C:
  def __bool__(self):
    raise RuntimeError("too cool for bool")

not C()
)"),
                            LayoutId::kRuntimeError, "too cool for bool"));
}

TEST_F(InterpreterTest, BinaryOpCachedInsertsDependencyForBothOperandsTypes) {
  HandleScope scope(thread_);
  EXPECT_FALSE(runFromCStr(&runtime_, R"(
class A:
  def __add__(self, other):
    return "from class A"

class B:
  pass

def cache_binary_op(a, b):
  return a + b

a = A()
b = B()
A__add__ = A.__add__
result = cache_binary_op(a, b)
)")
                   .isError());
  ASSERT_TRUE(
      isStrEqualsCStr(mainModuleAt(&runtime_, "result"), "from class A"));

  Function cache_binary_op(&scope, mainModuleAt(&runtime_, "cache_binary_op"));
  Tuple caches(&scope, cache_binary_op.caches());
  Object a(&scope, mainModuleAt(&runtime_, "a"));
  Object b(&scope, mainModuleAt(&runtime_, "b"));
  Type type_a(&scope, mainModuleAt(&runtime_, "A"));
  Type type_b(&scope, mainModuleAt(&runtime_, "B"));
  BinaryOpFlags flag;
  ASSERT_EQ(icLookupBinaryOp(*caches, 0, a.layoutId(), b.layoutId(), &flag),
            mainModuleAt(&runtime_, "A__add__"));

  // Verify that A.__add__ has the dependent.
  Dict type_a_dict(&scope, type_a.dict());
  Str left_op_name(&scope, runtime_.symbols()->at(SymbolId::kDunderAdd));
  Object type_a_attr(&scope, dictAtByStr(thread_, type_a_dict, left_op_name));
  ASSERT_TRUE(type_a_attr.isValueCell());
  ASSERT_TRUE(ValueCell::cast(*type_a_attr).dependencyLink().isWeakLink());
  EXPECT_EQ(
      WeakLink::cast(ValueCell::cast(*type_a_attr).dependencyLink()).referent(),
      *cache_binary_op);

  // Verify that B.__radd__ has the dependent.
  Dict type_b_dict(&scope, type_b.dict());
  Str right_op_name(&scope, runtime_.symbols()->at(SymbolId::kDunderRadd));
  Object type_b_attr(&scope, dictAtByStr(thread_, type_b_dict, right_op_name));
  ASSERT_TRUE(type_b_attr.isValueCell());
  ASSERT_TRUE(ValueCell::cast(*type_b_attr).dependencyLink().isWeakLink());
  EXPECT_EQ(
      WeakLink::cast(ValueCell::cast(*type_b_attr).dependencyLink()).referent(),
      *cache_binary_op);
}

TEST_F(InterpreterTest, BinaryOpInvokesSelfMethod) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
    def __sub__(self, other):
        return (C, '__sub__', self, other)

left = C()
right = C()
)")
                   .isError());

  Frame* frame = thread_->currentFrame();

  Object left(&scope, mainModuleAt(&runtime_, "left"));
  Object right(&scope, mainModuleAt(&runtime_, "right"));
  Object c_class(&scope, mainModuleAt(&runtime_, "C"));

  Object result_obj(
      &scope, Interpreter::binaryOperation(
                  thread_, frame, Interpreter::BinaryOp::SUB, left, right));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 4);
  EXPECT_EQ(result.at(0), *c_class);
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "__sub__"));
  EXPECT_EQ(result.at(2), *left);
  EXPECT_EQ(result.at(3), *right);
}

TEST_F(InterpreterTest, BinaryOpInvokesSelfMethodIgnoresReflectedMethod) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
    def __sub__(self, other):
        return (C, '__sub__', self, other)
    def __rsub__(self, other):
        return (C, '__rsub__', self, other)

left = C()
right = C()
)")
                   .isError());

  Frame* frame = thread_->currentFrame();

  Object left(&scope, mainModuleAt(&runtime_, "left"));
  Object right(&scope, mainModuleAt(&runtime_, "right"));
  Object c_class(&scope, mainModuleAt(&runtime_, "C"));

  Object result_obj(
      &scope, Interpreter::binaryOperation(
                  thread_, frame, Interpreter::BinaryOp::SUB, left, right));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 4);
  EXPECT_EQ(result.at(0), *c_class);
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "__sub__"));
  EXPECT_EQ(result.at(2), *left);
  EXPECT_EQ(result.at(3), *right);
}

TEST_F(InterpreterTest, BinaryOperationInvokesSubclassReflectedMethod) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
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

  Frame* frame = thread_->currentFrame();

  Object left(&scope, mainModuleAt(&runtime_, "left"));
  Object right(&scope, mainModuleAt(&runtime_, "right"));
  Object d_class(&scope, mainModuleAt(&runtime_, "D"));

  Object result_obj(
      &scope, Interpreter::binaryOperation(
                  thread_, frame, Interpreter::BinaryOp::SUB, left, right));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 4);
  EXPECT_EQ(result.at(0), *d_class);
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "__rsub__"));
  EXPECT_EQ(result.at(2), *right);
  EXPECT_EQ(result.at(3), *left);
}

TEST_F(InterpreterTest, BinaryOperationInvokesOtherReflectedMethod) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
    pass

class D:
    def __rsub__(self, other):
        return (D, '__rsub__', self, other)

left = C()
right = D()
)")
                   .isError());

  Frame* frame = thread_->currentFrame();

  Object left(&scope, mainModuleAt(&runtime_, "left"));
  Object right(&scope, mainModuleAt(&runtime_, "right"));
  Object d_class(&scope, mainModuleAt(&runtime_, "D"));

  Object result_obj(
      &scope, Interpreter::binaryOperation(
                  thread_, frame, Interpreter::BinaryOp::SUB, left, right));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 4);
  EXPECT_EQ(result.at(0), *d_class);
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "__rsub__"));
  EXPECT_EQ(result.at(2), *right);
  EXPECT_EQ(result.at(3), *left);
}

TEST_F(
    InterpreterTest,
    BinaryOperationInvokesLeftMethodWhenReflectedMethodReturnsNotImplemented) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
trace = ""
class C:
    def __add__(self, other):
        global trace
        trace += "C.__add__,"
        return "C.__add__"

    def __radd__(self, other):
        raise Exception("should not be called")


class D(C):
    def __add__(self, other):
        raise Exception("should not be called")

    def __radd__(self, other):
        global trace
        trace += "D.__radd__,"
        return NotImplemented

result = C() + D()
)")
                   .isError());

  EXPECT_TRUE(isStrEqualsCStr(mainModuleAt(&runtime_, "result"), "C.__add__"));
  EXPECT_TRUE(isStrEqualsCStr(mainModuleAt(&runtime_, "trace"),
                              "D.__radd__,C.__add__,"));
}

TEST_F(InterpreterTest, BinaryOperationLookupPropagatesException) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class RaisingDescriptor:
  def __get__(self, obj, type):
    raise UserWarning()
class A:
  __mul__ = RaisingDescriptor()
a = A()
)")
                   .isError());
  Object a(&scope, mainModuleAt(&runtime_, "a"));
  Frame* frame = thread_->currentFrame();
  Object result(&scope, Interpreter::binaryOperation(
                            thread_, frame, Interpreter::BinaryOp::MUL, a, a));
  EXPECT_TRUE(raised(*result, LayoutId::kUserWarning));
}

TEST_F(InterpreterTest,
       BinaryOperationLookupReflectedMethodPropagatesException) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
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
  Object a(&scope, mainModuleAt(&runtime_, "a"));
  Object b(&scope, mainModuleAt(&runtime_, "b"));
  Frame* frame = thread_->currentFrame();
  Object result(&scope, Interpreter::binaryOperation(
                            thread_, frame, Interpreter::BinaryOp::MUL, a, b));
  EXPECT_TRUE(raised(*result, LayoutId::kUserWarning));
}

TEST_F(InterpreterTest, BinaryOperationSetMethodSetsMethod) {
  HandleScope scope(thread_);
  Object v0(&scope, runtime_.newInt(13));
  Object v1(&scope, runtime_.newInt(42));
  Object method(&scope, NoneType::object());
  BinaryOpFlags flags;
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::binaryOperationSetMethod(thread_, thread_->currentFrame(),
                                            Interpreter::BinaryOp::SUB, v0, v1,
                                            &method, &flags),
      -29));
  EXPECT_TRUE(method.isFunction());
  EXPECT_EQ(flags, kBinaryOpNotImplementedRetry);

  Object v2(&scope, runtime_.newInt(3));
  Object v3(&scope, runtime_.newInt(8));
  ASSERT_EQ(v0.layoutId(), v2.layoutId());
  ASSERT_EQ(v1.layoutId(), v3.layoutId());
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::binaryOperationWithMethod(thread_, thread_->currentFrame(),
                                             *method, flags, *v2, *v3),
      -5));
}

TEST_F(InterpreterTest,
       BinaryOperationSetMethodSetsReflectedMethodNotImplementedRetry) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
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
  Object v0(&scope, mainModuleAt(&runtime_, "v0"));
  Object v1(&scope, mainModuleAt(&runtime_, "v1"));
  Object v2(&scope, mainModuleAt(&runtime_, "v2"));
  Object v3(&scope, mainModuleAt(&runtime_, "v3"));

  Object method(&scope, NoneType::object());
  BinaryOpFlags flags;
  Object result_obj(
      &scope, Interpreter::binaryOperationSetMethod(
                  thread_, thread_->currentFrame(), Interpreter::BinaryOp::SUB,
                  v0, v1, &method, &flags));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 2);
  EXPECT_EQ(result.at(0), v1);
  EXPECT_EQ(result.at(1), v0);
  EXPECT_TRUE(method.isFunction());
  EXPECT_EQ(flags, kBinaryOpReflected | kBinaryOpNotImplementedRetry);

  ASSERT_EQ(v0.layoutId(), v2.layoutId());
  ASSERT_EQ(v1.layoutId(), v3.layoutId());
  result_obj = Interpreter::binaryOperationWithMethod(
      thread_, thread_->currentFrame(), *method, flags, *v2, *v3);
  ASSERT_TRUE(result.isTuple());
  result = *result_obj;
  ASSERT_EQ(result.length(), 2);
  EXPECT_EQ(result.at(0), v3);
  EXPECT_EQ(result.at(1), v2);
}

TEST_F(InterpreterTest, BinaryOperationSetMethodSetsReflectedMethod) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
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
  Object v0(&scope, mainModuleAt(&runtime_, "v0"));
  Object v1(&scope, mainModuleAt(&runtime_, "v1"));
  Object v2(&scope, mainModuleAt(&runtime_, "v2"));
  Object v3(&scope, mainModuleAt(&runtime_, "v3"));

  Object method(&scope, NoneType::object());
  BinaryOpFlags flags;
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::binaryOperationSetMethod(thread_, thread_->currentFrame(),
                                            Interpreter::BinaryOp::SUB, v0, v1,
                                            &method, &flags),
      -12));
  EXPECT_TRUE(method.isFunction());
  EXPECT_EQ(flags, kBinaryOpReflected);

  ASSERT_EQ(v0.layoutId(), v2.layoutId());
  ASSERT_EQ(v1.layoutId(), v3.layoutId());
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::binaryOperationWithMethod(thread_, thread_->currentFrame(),
                                             *method, flags, *v2, *v3),
      45));
}

TEST_F(InterpreterTest, BinaryOperationSetMethodSetsMethodNotImplementedRetry) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
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
  Object v0(&scope, mainModuleAt(&runtime_, "v0"));
  Object v1(&scope, mainModuleAt(&runtime_, "v1"));
  Object v2(&scope, mainModuleAt(&runtime_, "v2"));
  Object v3(&scope, mainModuleAt(&runtime_, "v3"));

  Object method(&scope, NoneType::object());
  BinaryOpFlags flags;
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::binaryOperationSetMethod(thread_, thread_->currentFrame(),
                                            Interpreter::BinaryOp::SUB, v0, v1,
                                            &method, &flags),
      2));
  EXPECT_TRUE(method.isFunction());
  EXPECT_EQ(flags, kBinaryOpNotImplementedRetry);

  ASSERT_EQ(v0.layoutId(), v2.layoutId());
  ASSERT_EQ(v1.layoutId(), v3.layoutId());
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::binaryOperationWithMethod(thread_, thread_->currentFrame(),
                                             *method, flags, *v2, *v3),
      -8));
}

TEST_F(InterpreterTest, DoBinaryOpWithCacheHitCallsCachedMethod) {
  HandleScope scope(thread_);

  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(2));
  consts.atPut(0, runtime_.newInt(7));
  consts.atPut(1, runtime_.newInt(-13));
  code.setConsts(*consts);
  const byte bytecode[] = {
      LOAD_CONST, 0, LOAD_CONST, 1, BINARY_SUBTRACT, 0, RETURN_VALUE, 0,
  };
  code.setCode(runtime_.newBytesWithAll(bytecode));

  Object qualname(&scope, Str::empty());
  Module module(&scope, runtime_.findOrCreateMainModule());
  Function function(
      &scope, runtime_.newFunctionWithCode(thread_, qualname, code, module));

  // Update inline cache.
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::callFunction0(thread_, thread_->currentFrame(), function),
      20));

  ASSERT_TRUE(function.caches().isTuple());
  Tuple caches(&scope, function.caches());
  BinaryOpFlags dummy;
  ASSERT_FALSE(icLookupBinaryOp(*caches, 0, LayoutId::kSmallInt,
                                LayoutId::kSmallInt, &dummy)
                   .isErrorNotFound());
  // Call from inline cache.
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::callFunction0(thread_, thread_->currentFrame(), function),
      20));
}

TEST_F(InterpreterTest, DoBinaryOpWithCacheHitCallsRetry) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class MyInt(int):
  def __sub__(self, other):
    return NotImplemented
  def __rsub__(self, other):
    return NotImplemented
v0 = MyInt(3)
v1 = 7
)")
                   .isError());
  Object v0(&scope, mainModuleAt(&runtime_, "v0"));
  Object v1(&scope, mainModuleAt(&runtime_, "v1"));

  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(2));
  consts.atPut(0, *v0);
  consts.atPut(1, *v1);
  code.setConsts(*consts);
  const byte bytecode[] = {
      LOAD_CONST, 0, LOAD_CONST, 1, BINARY_SUBTRACT, 0, RETURN_VALUE, 0,
  };
  code.setCode(runtime_.newBytesWithAll(bytecode));

  Object qualname(&scope, Str::empty());
  Module module(&scope, runtime_.findOrCreateMainModule());
  Function function(
      &scope, runtime_.newFunctionWithCode(thread_, qualname, code, module));

  // Update inline cache.
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::callFunction0(thread_, thread_->currentFrame(), function),
      -4));

  ASSERT_TRUE(function.caches().isTuple());
  Tuple caches(&scope, function.caches());
  BinaryOpFlags dummy;
  ASSERT_FALSE(
      icLookupBinaryOp(*caches, 0, v0.layoutId(), v1.layoutId(), &dummy)
          .isErrorNotFound());

  // Should hit the cache for __sub__ and then call binaryOperationRetry().
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::callFunction0(thread_, thread_->currentFrame(), function),
      -4));
}

TEST_F(InterpreterTest, InplaceOpCachedInsertsDependencyForThreeAttributes) {
  HandleScope scope(thread_);
  EXPECT_FALSE(runFromCStr(&runtime_, R"(
class A:
  def __imul__(self, other):
    return "from class A"

class B:
  pass

def cache_inplace_op(a, b):
  a *= b

a = A()
b = B()
A__imul__ = A.__imul__
cache_inplace_op(a, b)
)")
                   .isError());
  Function cache_inplace_op(&scope,
                            mainModuleAt(&runtime_, "cache_inplace_op"));
  Tuple caches(&scope, cache_inplace_op.caches());
  Object a(&scope, mainModuleAt(&runtime_, "a"));
  Object b(&scope, mainModuleAt(&runtime_, "b"));
  Type type_a(&scope, mainModuleAt(&runtime_, "A"));
  Type type_b(&scope, mainModuleAt(&runtime_, "B"));
  BinaryOpFlags flag;
  ASSERT_EQ(icLookupBinaryOp(*caches, 0, a.layoutId(), b.layoutId(), &flag),
            mainModuleAt(&runtime_, "A__imul__"));

  // Verify that A.__imul__ has the dependent.
  Dict type_a_dict(&scope, type_a.dict());
  Str inplace_op_name(&scope, runtime_.symbols()->at(SymbolId::kDunderImul));
  Object inplace_attr(&scope,
                      dictAtByStr(thread_, type_a_dict, inplace_op_name));
  ASSERT_TRUE(inplace_attr.isValueCell());
  ASSERT_TRUE(ValueCell::cast(*inplace_attr).dependencyLink().isWeakLink());
  EXPECT_EQ(WeakLink::cast(ValueCell::cast(*inplace_attr).dependencyLink())
                .referent(),
            *cache_inplace_op);

  // Verify that A.__mul__ has the dependent.
  Str left_op_name(&scope, runtime_.symbols()->at(SymbolId::kDunderMul));
  Object type_a_attr(&scope, dictAtByStr(thread_, type_a_dict, left_op_name));
  ASSERT_TRUE(type_a_attr.isValueCell());
  ASSERT_TRUE(ValueCell::cast(*type_a_attr).dependencyLink().isWeakLink());
  EXPECT_EQ(
      WeakLink::cast(ValueCell::cast(*type_a_attr).dependencyLink()).referent(),
      *cache_inplace_op);

  // Verify that B.__rmul__ has the dependent.
  Dict type_b_dict(&scope, type_b.dict());
  Str right_op_name(&scope, runtime_.symbols()->at(SymbolId::kDunderRmul));
  Object type_b_attr(&scope, dictAtByStr(thread_, type_b_dict, right_op_name));
  ASSERT_TRUE(type_b_attr.isValueCell());
  ASSERT_TRUE(ValueCell::cast(*type_b_attr).dependencyLink().isWeakLink());
  EXPECT_EQ(
      WeakLink::cast(ValueCell::cast(*type_b_attr).dependencyLink()).referent(),
      *cache_inplace_op);
}

TEST_F(InterpreterTest, ImportFromWithMissingAttributeRaisesImportError) {
  HandleScope scope(thread_);
  Str name(&scope, runtime_.newStrFromCStr("foo"));
  Module module(&scope, runtime_.newModule(name));
  runtime_.addModule(module);
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, "from foo import bar"),
                            LayoutId::kImportError,
                            "cannot import name 'bar' from 'foo'"));
}

TEST_F(InterpreterTest, ImportFromCallsDunderGetattribute) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  def __getattribute__(self, name):
    return f"getattribute '{name}'"
i = C()
)")
                   .isError());
  Object i(&scope, mainModuleAt(&runtime_, "i"));

  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(1));
  consts.atPut(0, *i);
  code.setConsts(*consts);
  Tuple names(&scope, runtime_.newTuple(1));
  names.atPut(0, Runtime::internStrFromCStr(thread_, "foo"));
  code.setNames(*names);
  const byte bytecode[] = {LOAD_CONST, 0, IMPORT_FROM, 0, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));

  EXPECT_TRUE(isStrEqualsCStr(runCode(code), "getattribute 'foo'"));
}

TEST_F(InterpreterTest, ImportFromWithNonModuleRaisesImportError) {
  HandleScope scope(thread_);
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(1));
  consts.atPut(0, NoneType::object());
  code.setConsts(*consts);
  Tuple names(&scope, runtime_.newTuple(1));
  names.atPut(0, Runtime::internStrFromCStr(thread_, "foo"));
  code.setNames(*names);
  const byte bytecode[] = {LOAD_CONST, 0, IMPORT_FROM, 0, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));

  EXPECT_TRUE(raisedWithStr(runCode(code), LayoutId::kImportError,
                            "cannot import name 'foo'"));
}

TEST_F(InterpreterTest, ImportFromWithNonModulePropagatesException) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  def __getattribute__(self, name):
    raise UserWarning()
i = C()
)")
                   .isError());
  Object i(&scope, mainModuleAt(&runtime_, "i"));

  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(1));
  consts.atPut(0, *i);
  code.setConsts(*consts);
  Tuple names(&scope, runtime_.newTuple(1));
  names.atPut(0, Runtime::internStrFromCStr(thread_, "foo"));
  code.setNames(*names);
  const byte bytecode[] = {LOAD_CONST, 0, IMPORT_FROM, 0, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));

  EXPECT_TRUE(raised(runCode(code), LayoutId::kUserWarning));
}

TEST_F(InterpreterTest, InplaceOperationCallsInplaceMethod) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
    def __isub__(self, other):
        return (C, '__isub__', self, other)

left = C()
right = C()
)")
                   .isError());

  Frame* frame = thread_->currentFrame();

  Object left(&scope, mainModuleAt(&runtime_, "left"));
  Object right(&scope, mainModuleAt(&runtime_, "right"));
  Object c_class(&scope, mainModuleAt(&runtime_, "C"));

  Object result_obj(
      &scope, Interpreter::inplaceOperation(
                  thread_, frame, Interpreter::BinaryOp::SUB, left, right));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 4);
  EXPECT_EQ(result.at(0), *c_class);
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "__isub__"));
  EXPECT_EQ(result.at(2), *left);
  EXPECT_EQ(result.at(3), *right);
}

TEST_F(InterpreterTest, InplaceOperationCallsBinaryMethod) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
    def __sub__(self, other):
        return (C, '__sub__', self, other)

left = C()
right = C()
)")
                   .isError());

  Frame* frame = thread_->currentFrame();

  Object left(&scope, mainModuleAt(&runtime_, "left"));
  Object right(&scope, mainModuleAt(&runtime_, "right"));
  Object c_class(&scope, mainModuleAt(&runtime_, "C"));

  Object result_obj(
      &scope, Interpreter::inplaceOperation(
                  thread_, frame, Interpreter::BinaryOp::SUB, left, right));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 4);
  EXPECT_EQ(result.at(0), *c_class);
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "__sub__"));
  EXPECT_EQ(result.at(2), *left);
  EXPECT_EQ(result.at(3), *right);
}

TEST_F(InterpreterTest, InplaceOperationCallsBinaryMethodAfterNotImplemented) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
    def __isub__(self, other):
        return NotImplemented
    def __sub__(self, other):
        return (C, '__sub__', self, other)

left = C()
right = C()
)")
                   .isError());

  Frame* frame = thread_->currentFrame();

  Object left(&scope, mainModuleAt(&runtime_, "left"));
  Object right(&scope, mainModuleAt(&runtime_, "right"));
  Object c_class(&scope, mainModuleAt(&runtime_, "C"));

  Object result_obj(
      &scope, Interpreter::inplaceOperation(
                  thread_, frame, Interpreter::BinaryOp::SUB, left, right));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 4);
  EXPECT_EQ(result.at(0), *c_class);
  EXPECT_TRUE(isStrEqualsCStr(result.at(1), "__sub__"));
  EXPECT_EQ(result.at(2), *left);
  EXPECT_EQ(result.at(3), *right);
}

TEST_F(InterpreterTest, InplaceOperationSetMethodSetsMethodFlagsBinaryOpRetry) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class MyInt(int):
  def __isub__(self, other):
    return int(self) - other - 2
v0 = MyInt(9)
v1 = MyInt(-11)
v2 = MyInt(-3)
v3 = MyInt(7)
)")
                   .isError());
  Object v0(&scope, mainModuleAt(&runtime_, "v0"));
  Object v1(&scope, mainModuleAt(&runtime_, "v1"));
  Object v2(&scope, mainModuleAt(&runtime_, "v2"));
  Object v3(&scope, mainModuleAt(&runtime_, "v3"));
  Object method(&scope, NoneType::object());
  BinaryOpFlags flags;
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::inplaceOperationSetMethod(thread_, thread_->currentFrame(),
                                             Interpreter::BinaryOp::SUB, v0, v1,
                                             &method, &flags),
      18));
  EXPECT_EQ(flags, kInplaceBinaryOpRetry);

  ASSERT_EQ(v0.layoutId(), v2.layoutId());
  ASSERT_EQ(v1.layoutId(), v3.layoutId());
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::binaryOperationWithMethod(thread_, thread_->currentFrame(),
                                             *method, flags, *v2, *v3),
      -12));
}

TEST_F(InterpreterTest, InplaceOperationSetMethodSetsMethodFlagsReverseRetry) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
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
  Object v0(&scope, mainModuleAt(&runtime_, "v0"));
  Object v1(&scope, mainModuleAt(&runtime_, "v1"));
  Object v2(&scope, mainModuleAt(&runtime_, "v2"));
  Object v3(&scope, mainModuleAt(&runtime_, "v3"));
  Object method(&scope, NoneType::object());
  BinaryOpFlags flags;
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::inplaceOperationSetMethod(thread_, thread_->currentFrame(),
                                             Interpreter::BinaryOp::POW, v0, v1,
                                             &method, &flags),
      20));
  EXPECT_EQ(flags, kBinaryOpReflected | kBinaryOpNotImplementedRetry);

  ASSERT_EQ(v0.layoutId(), v2.layoutId());
  ASSERT_EQ(v1.layoutId(), v3.layoutId());
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::binaryOperationWithMethod(thread_, thread_->currentFrame(),
                                             *method, flags, *v2, *v3),
      249));
}

TEST_F(InterpreterDeathTest, InvalidOpcode) {
  HandleScope scope(thread_);

  Code code(&scope, newEmptyCode());
  const byte bytecode[] = {NOP, 0, NOP, 0, UNUSED_BYTECODE_202, 17, NOP, 7};
  code.setCode(runtime_.newBytesWithAll(bytecode));

  ASSERT_DEATH(static_cast<void>(runCode(code)),
               "bytecode 'UNUSED_BYTECODE_202'");
}

// To a rich comparison on two instances of the same type.  In each case, the
// method on the left side of the comparison should be used.
TEST_F(InterpreterTest, CompareOpSameType) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
    def __init__(self, value):
        self.value = value

    def __lt__(self, other):
        return self.value < other.value

c10 = C(10)
c20 = C(20)
)")
                   .isError());

  Frame* frame = thread_->currentFrame();

  ASSERT_TRUE(frame->isSentinel());

  Object left(&scope, mainModuleAt(&runtime_, "c10"));
  Object right(&scope, mainModuleAt(&runtime_, "c20"));

  Object left_lt_right(&scope, Interpreter::compareOperation(
                                   thread_, frame, CompareOp::LT, left, right));
  EXPECT_EQ(left_lt_right, Bool::trueObj());

  Object right_lt_left(&scope, Interpreter::compareOperation(
                                   thread_, frame, CompareOp::LT, right, left));
  EXPECT_EQ(right_lt_left, Bool::falseObj());
}

TEST_F(InterpreterTest, CompareOpFallback) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
    def __init__(self, value):
        self.value = value

c10 = C(10)
c20 = C(20)
)")
                   .isError());

  Frame* frame = thread_->currentFrame();

  ASSERT_TRUE(frame->isSentinel());

  Object left(&scope, mainModuleAt(&runtime_, "c10"));
  Object right(&scope, mainModuleAt(&runtime_, "c20"));

  Object left_eq_right(&scope, Interpreter::compareOperation(
                                   thread_, frame, CompareOp::EQ, left, right));
  EXPECT_EQ(left_eq_right, Bool::falseObj());
  Object left_ne_right(&scope, Interpreter::compareOperation(
                                   thread_, frame, CompareOp::NE, left, right));
  EXPECT_EQ(left_ne_right, Bool::trueObj());

  Object right_eq_left(&scope, Interpreter::compareOperation(
                                   thread_, frame, CompareOp::EQ, left, right));
  EXPECT_EQ(right_eq_left, Bool::falseObj());
  Object right_ne_left(&scope, Interpreter::compareOperation(
                                   thread_, frame, CompareOp::NE, left, right));
  EXPECT_EQ(right_ne_left, Bool::trueObj());
}

TEST_F(InterpreterTest, CompareOpSubclass) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
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

  Frame* frame = thread_->currentFrame();
  ASSERT_TRUE(frame->isSentinel());

  Object a(&scope, mainModuleAt(&runtime_, "a"));
  Object b(&scope, mainModuleAt(&runtime_, "b"));
  Object c(&scope, mainModuleAt(&runtime_, "c"));

  // Comparisons where rhs is not a subtype of lhs try lhs.__eq__(rhs) first.
  Object a_eq_b(&scope, Interpreter::compareOperation(thread_, frame,
                                                      CompareOp::EQ, a, b));
  EXPECT_EQ(a_eq_b, Bool::falseObj());
  Object called(&scope, mainModuleAt(&runtime_, "called"));
  EXPECT_TRUE(isStrEqualsCStr(*called, "A"));

  Str called_name(&scope, runtime_.newStrFromCStr("called"));
  Object none(&scope, NoneType::object());
  Module main(&scope, findMainModule(&runtime_));
  moduleAtPutByStr(thread_, main, called_name, none);
  Object b_eq_a(&scope, Interpreter::compareOperation(thread_, frame,
                                                      CompareOp::EQ, b, a));
  EXPECT_EQ(b_eq_a, Bool::trueObj());
  called = mainModuleAt(&runtime_, "called");
  EXPECT_TRUE(isStrEqualsCStr(*called, "B"));

  moduleAtPutByStr(thread_, main, called_name, none);
  Object c_eq_a(&scope, Interpreter::compareOperation(thread_, frame,
                                                      CompareOp::EQ, c, a));
  EXPECT_EQ(c_eq_a, Bool::trueObj());
  called = mainModuleAt(&runtime_, "called");
  EXPECT_TRUE(isStrEqualsCStr(*called, "C"));

  // When rhs is a subtype of lhs, only rhs.__eq__(rhs) is tried.
  moduleAtPutByStr(thread_, main, called_name, none);
  Object a_eq_c(&scope, Interpreter::compareOperation(thread_, frame,
                                                      CompareOp::EQ, a, c));
  EXPECT_EQ(a_eq_c, Bool::trueObj());
  called = mainModuleAt(&runtime_, "called");
  EXPECT_TRUE(isStrEqualsCStr(*called, "C"));
}

TEST_F(InterpreterTest, CompareOpSetMethodSetsMethod) {
  HandleScope scope(thread_);
  Object v0(&scope, runtime_.newInt(39));
  Object v1(&scope, runtime_.newInt(11));
  Object method(&scope, NoneType::object());
  BinaryOpFlags flags;
  EXPECT_EQ(Interpreter::compareOperationSetMethod(
                thread_, thread_->currentFrame(), CompareOp::LT, v0, v1,
                &method, &flags),
            Bool::falseObj());
  EXPECT_TRUE(method.isFunction());
  EXPECT_EQ(flags, kBinaryOpNotImplementedRetry);

  Object v2(&scope, runtime_.newInt(3));
  Object v3(&scope, runtime_.newInt(8));
  ASSERT_EQ(v0.layoutId(), v2.layoutId());
  ASSERT_EQ(v1.layoutId(), v3.layoutId());
  EXPECT_EQ(Interpreter::binaryOperationWithMethod(
                thread_, thread_->currentFrame(), *method, flags, *v2, *v3),
            Bool::trueObj());
}

TEST_F(InterpreterTest, CompareOpSetMethodSetsReverseMethod) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class A:
  pass

class B(A):
  def __ge__(self, other):
    return (self, other)

a1 = A()
b1 = B()
a2 = A()
b2 = B()
)")
                   .isError());

  Object a1(&scope, mainModuleAt(&runtime_, "a1"));
  Object b1(&scope, mainModuleAt(&runtime_, "b1"));
  Object method(&scope, NoneType::object());
  BinaryOpFlags flags;
  Object result_obj(&scope, Interpreter::compareOperationSetMethod(
                                thread_, thread_->currentFrame(), CompareOp::LE,
                                a1, b1, &method, &flags));
  EXPECT_TRUE(method.isFunction());
  EXPECT_EQ(flags, kBinaryOpReflected | kBinaryOpNotImplementedRetry);
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 2);
  EXPECT_EQ(result.at(0), b1);
  EXPECT_EQ(result.at(1), a1);

  Object a2(&scope, mainModuleAt(&runtime_, "a2"));
  Object b2(&scope, mainModuleAt(&runtime_, "b2"));
  ASSERT_EQ(a1.layoutId(), a2.layoutId());
  ASSERT_EQ(b1.layoutId(), b2.layoutId());
  result_obj = Interpreter::binaryOperationWithMethod(
      thread_, thread_->currentFrame(), *method, flags, *a2, *b2);
  ASSERT_TRUE(result_obj.isTuple());
  result = *result_obj;
  ASSERT_EQ(result.length(), 2);
  EXPECT_EQ(result.at(0), b2);
  EXPECT_EQ(result.at(1), a2);
}

TEST_F(InterpreterTest,
       CompareOpSetMethodSetsReverseMethodNotImplementedRetry) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
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
  Object v0(&scope, mainModuleAt(&runtime_, "v0"));
  Object v1(&scope, mainModuleAt(&runtime_, "v1"));
  Object v2(&scope, mainModuleAt(&runtime_, "v2"));
  Object v3(&scope, mainModuleAt(&runtime_, "v3"));
  Object method(&scope, NoneType::object());
  BinaryOpFlags flags;
  Object result_obj(&scope, Interpreter::compareOperationSetMethod(
                                thread_, thread_->currentFrame(), CompareOp::LE,
                                v0, v1, &method, &flags));
  EXPECT_TRUE(method.isFunction());
  EXPECT_EQ(flags, kBinaryOpReflected | kBinaryOpNotImplementedRetry);
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 2);
  EXPECT_EQ(result.at(0), v1);
  EXPECT_EQ(result.at(1), v0);

  ASSERT_EQ(v0.layoutId(), v2.layoutId());
  ASSERT_EQ(v1.layoutId(), v3.layoutId());
  result_obj = Interpreter::binaryOperationWithMethod(
      thread_, thread_->currentFrame(), *method, flags, *v2, *v3);
  ASSERT_TRUE(result_obj.isTuple());
  result = *result_obj;
  ASSERT_EQ(result.length(), 2);
  EXPECT_EQ(result.at(0), v3);
  EXPECT_EQ(result.at(1), v2);
}

TEST_F(InterpreterTest,
       CompareOpInvokesLeftMethodWhenReflectedMethodReturnsNotImplemented) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
trace = ""
class C:
    def __ge__(self, other):
        global trace
        trace += "C.__ge__,"
        return "C.__ge__"

    def __le__(self, other):
        raise Exception("should not be called")

class D(C):
    def __ge__(self, other):
        raise Exception("should not be called")

    def __le__(self, other):
        global trace
        trace += "D.__le__,"
        return NotImplemented

result = C() >= D()
)")
                   .isError());

  EXPECT_TRUE(isStrEqualsCStr(mainModuleAt(&runtime_, "result"), "C.__ge__"));
  EXPECT_TRUE(
      isStrEqualsCStr(mainModuleAt(&runtime_, "trace"), "D.__le__,C.__ge__,"));
}

TEST_F(
    InterpreterTest,
    CompareOpCachedInsertsDependencyForBothOperandsTypesAppropriateAttributes) {
  HandleScope scope(thread_);
  EXPECT_FALSE(runFromCStr(&runtime_, R"(
class A:
  def __ge__(self, other):
    return "from class A"

class B:
  pass

def cache_compare_op(a, b):
  return a >= b

a = A()
b = B()
A__ge__ = A.__ge__
result = cache_compare_op(a, b)
)")
                   .isError());
  ASSERT_TRUE(
      isStrEqualsCStr(mainModuleAt(&runtime_, "result"), "from class A"));

  Function cache_compare_op(&scope,
                            mainModuleAt(&runtime_, "cache_compare_op"));
  Tuple caches(&scope, cache_compare_op.caches());
  Object a_obj(&scope, mainModuleAt(&runtime_, "a"));
  Object b_obj(&scope, mainModuleAt(&runtime_, "b"));
  BinaryOpFlags flag;
  EXPECT_EQ(
      icLookupBinaryOp(*caches, 0, a_obj.layoutId(), b_obj.layoutId(), &flag),
      mainModuleAt(&runtime_, "A__ge__"));

  // Verify that A.__ge__ has the dependent.
  Type a_type(&scope, mainModuleAt(&runtime_, "A"));
  Dict a_type_dict(&scope, a_type.dict());
  Str left_op_name(&scope, runtime_.symbols()->at(SymbolId::kDunderGe));
  Object a_type_attr(&scope, dictAtByStr(thread_, a_type_dict, left_op_name));
  ASSERT_TRUE(a_type_attr.isValueCell());
  ASSERT_TRUE(ValueCell::cast(*a_type_attr).dependencyLink().isWeakLink());
  EXPECT_EQ(
      WeakLink::cast(ValueCell::cast(*a_type_attr).dependencyLink()).referent(),
      *cache_compare_op);

  // Verify that B.__le__ has the dependent.
  Type b_type(&scope, mainModuleAt(&runtime_, "B"));
  Dict b_type_dict(&scope, b_type.dict());
  Str right_op_name(&scope, runtime_.symbols()->at(SymbolId::kDunderLe));
  Object b_type_attr(&scope, dictAtByStr(thread_, b_type_dict, right_op_name));
  ASSERT_TRUE(b_type_attr.isValueCell());
  ASSERT_TRUE(ValueCell::cast(*b_type_attr).dependencyLink().isWeakLink());
  EXPECT_EQ(
      WeakLink::cast(ValueCell::cast(*b_type_attr).dependencyLink()).referent(),
      *cache_compare_op);
}

TEST_F(InterpreterTest, DoStoreFastStoresValue) {
  HandleScope scope(thread_);

  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(1));
  consts.atPut(0, SmallInt::fromWord(1111));
  code.setConsts(*consts);
  code.setNlocals(2);
  const byte bytecode[] = {LOAD_CONST, 0, STORE_FAST,   1,
                           LOAD_FAST,  1, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));

  EXPECT_TRUE(isIntEqualsWord(runCodeNoBytecodeRewriting(code), 1111));
}

TEST_F(InterpreterTest, DoLoadFastReverseLoadsValue) {
  HandleScope scope(thread_);

  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(4));
  consts.atPut(0, SmallInt::fromWord(1));
  consts.atPut(1, SmallInt::fromWord(22));
  consts.atPut(2, SmallInt::fromWord(333));
  consts.atPut(3, SmallInt::fromWord(4444));
  code.setConsts(*consts);
  code.setNlocals(4);
  const byte bytecode[] = {
      LOAD_CONST,        0, STORE_FAST,   0, LOAD_CONST,        1,
      STORE_FAST,        1, LOAD_CONST,   2, STORE_FAST,        2,
      LOAD_CONST,        3, STORE_FAST,   3, LOAD_FAST_REVERSE, 3,  // 1
      LOAD_FAST_REVERSE, 2,                                         // 22
      LOAD_FAST_REVERSE, 0,                                         // 4444
      LOAD_FAST_REVERSE, 1,                                         // 333
      BUILD_TUPLE,       4, RETURN_VALUE, 0,
  };
  code.setCode(runtime_.newBytesWithAll(bytecode));

  Object result_obj(&scope, runCodeNoBytecodeRewriting(code));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 4);
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 22));
  EXPECT_TRUE(isIntEqualsWord(result.at(2), 4444));
  EXPECT_TRUE(isIntEqualsWord(result.at(3), 333));
}

TEST_F(InterpreterTest,
       DoLoadFastReverseFromUninitializedLocalRaisesUnboundLocalError) {
  HandleScope scope(thread_);

  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(1));
  consts.atPut(0, SmallInt::fromWord(42));
  code.setConsts(*consts);
  Tuple varnames(&scope, runtime_.newTuple(3));
  varnames.atPut(0, Runtime::internStrFromCStr(thread_, "foo"));
  varnames.atPut(1, Runtime::internStrFromCStr(thread_, "bar"));
  varnames.atPut(2, Runtime::internStrFromCStr(thread_, "baz"));
  code.setVarnames(*varnames);
  code.setNlocals(3);
  const byte bytecode[] = {
      LOAD_CONST,  0, STORE_FAST,        0, LOAD_CONST,   0, STORE_FAST, 2,
      DELETE_FAST, 2, LOAD_FAST_REVERSE, 0, RETURN_VALUE, 0,
  };
  code.setCode(runtime_.newBytesWithAll(bytecode));

  EXPECT_TRUE(raisedWithStr(
      runCodeNoBytecodeRewriting(code), LayoutId::kUnboundLocalError,
      "local variable 'baz' referenced before assignment"));
}

TEST_F(InterpreterTest, DoStoreFastReverseStoresValue) {
  HandleScope scope(thread_);

  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(4));
  consts.atPut(0, SmallInt::fromWord(1));
  consts.atPut(1, SmallInt::fromWord(22));
  consts.atPut(2, SmallInt::fromWord(333));
  consts.atPut(3, SmallInt::fromWord(4444));
  code.setConsts(*consts);
  code.setNlocals(4);
  const byte bytecode[] = {
      LOAD_CONST,  0, STORE_FAST_REVERSE, 0,
      LOAD_CONST,  1, STORE_FAST_REVERSE, 1,
      LOAD_CONST,  2, STORE_FAST_REVERSE, 3,
      LOAD_CONST,  3, STORE_FAST_REVERSE, 2,
      LOAD_FAST,   0,  // 333
      LOAD_FAST,   1,  // 4444
      LOAD_FAST,   2,  // 22
      LOAD_FAST,   3,  // 1
      BUILD_TUPLE, 4, RETURN_VALUE,       0,
  };
  code.setCode(runtime_.newBytesWithAll(bytecode));

  Object result_obj(&scope, runCodeNoBytecodeRewriting(code));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 4);
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 333));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 4444));
  EXPECT_TRUE(isIntEqualsWord(result.at(2), 22));
  EXPECT_TRUE(isIntEqualsWord(result.at(3), 1));
}

TEST_F(InterpreterTest, DoStoreSubscrWithNoSetitemRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, "1[5] = 'foo'"),
                            LayoutId::kTypeError,
                            "'int' object does not support item assignment"));
}

TEST_F(InterpreterTest, DoStoreSubscrWithDescriptorPropagatesException) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
class A:
  def __get__(self, *args):
    raise RuntimeError("foo")

class B:
  __setitem__ = A()

b = B()
b[5] = 'foo'
)"),
                            LayoutId::kRuntimeError, "foo"));
}

TEST_F(InterpreterTest, DoDeleteSubscrWithNoDelitemRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, "del 1[5]"),
                            LayoutId::kTypeError,
                            "'int' object does not support item deletion"));
}

TEST_F(InterpreterTest, DoDeleteSubscrWithDescriptorPropagatesException) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
class A:
  def __get__(self, *args):
    raise RuntimeError("foo")

class B:
  __delitem__ = A()

b = B()
del b[5]
)"),
                            LayoutId::kRuntimeError, "foo"));
}

TEST_F(InterpreterTest, DoDeleteSubscrDoesntPushToStack) {
  HandleScope scope(thread_);

  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(3));
  List list(&scope, runtime_.newList());
  Int one(&scope, runtime_.newInt(1));
  runtime_.listEnsureCapacity(thread_, list, 1);
  list.setNumItems(1);
  list.atPut(0, *one);
  consts.atPut(0, SmallInt::fromWord(42));
  consts.atPut(1, *list);
  consts.atPut(2, SmallInt::fromWord(0));
  code.setConsts(*consts);

  Tuple varnames(&scope, runtime_.newTuple(0));
  code.setVarnames(*varnames);
  code.setNlocals(0);
  const byte bytecode[] = {
      LOAD_CONST,    0, LOAD_CONST,   1, LOAD_CONST, 2,
      DELETE_SUBSCR, 0, RETURN_VALUE, 0,
  };
  code.setCode(runtime_.newBytesWithAll(bytecode));

  Object result_obj(&scope, runCode(code));
  ASSERT_TRUE(result_obj.isInt());
  Int result(&scope, *result_obj);
  EXPECT_EQ(result.asWord(), 42);
  EXPECT_EQ(list.numItems(), 0);
}

TEST_F(InterpreterTest, GetIterWithSequenceReturnsIterator) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Sequence:
    def __getitem__(s, i):
        return ("foo", "bar")[i]

seq = Sequence()
)")
                   .isError());
  HandleScope scope(thread_);

  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(1));
  consts.atPut(0, mainModuleAt(&runtime_, "seq"));
  code.setConsts(*consts);

  Tuple varnames(&scope, runtime_.newTuple(0));
  code.setVarnames(*varnames);
  code.setNlocals(0);
  const byte bytecode[] = {
      LOAD_CONST, 0, GET_ITER, 0, RETURN_VALUE, 0,
  };
  code.setCode(runtime_.newBytesWithAll(bytecode));

  Object result_obj(&scope, runCode(code));
  EXPECT_TRUE(runtime_.isIterator(thread_, result_obj));
  Type result_type(&scope, runtime_.typeOf(*result_obj));
  EXPECT_TRUE(isStrEqualsCStr(result_type.name(), "iterator"));
}

TEST_F(InterpreterTest,
       GetIterWithRaisingDescriptorDunderIterPropagatesException) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
class Desc:
  def __get__(self, obj, type):
    raise UserWarning("foo")

class C:
  __iter__ = Desc()

it = C()
result = [x for x in it]
)"),
                            LayoutId::kTypeError,
                            "'C' object is not iterable"));
}

TEST_F(InterpreterTest, SequenceContains) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = {1, 2}

b = 1
c = 3
)")
                   .isError());

  Frame* frame = thread_->currentFrame();

  ASSERT_TRUE(frame->isSentinel());
  Object container(&scope, mainModuleAt(&runtime_, "a"));
  Object b(&scope, mainModuleAt(&runtime_, "b"));
  Object c(&scope, mainModuleAt(&runtime_, "c"));
  Object contains_true(
      &scope, Interpreter::sequenceContains(thread_, frame, b, container));
  Object contains_false(
      &scope, Interpreter::sequenceContains(thread_, frame, c, container));
  EXPECT_EQ(contains_true, Bool::trueObj());
  EXPECT_EQ(contains_false, Bool::falseObj());
}

TEST_F(InterpreterTest, SequenceIterSearchWithNoDunderIterRaisesTypeError) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C: pass
container = C()
)")
                   .isError());
  Frame* frame = thread_->currentFrame();
  Object container(&scope, mainModuleAt(&runtime_, "container"));
  Object val(&scope, NoneType::object());
  Object result(
      &scope, Interpreter::sequenceIterSearch(thread_, frame, val, container));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST_F(InterpreterTest,
       SequenceIterSearchWithNonCallableDunderIterRaisesTypeError) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  __iter__ = None
container = C()
)")
                   .isError());
  Frame* frame = thread_->currentFrame();
  Object container(&scope, mainModuleAt(&runtime_, "container"));
  Object val(&scope, NoneType::object());
  Object result(
      &scope, Interpreter::sequenceIterSearch(thread_, frame, val, container));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST_F(InterpreterTest, SequenceIterSearchWithNoDunderNextRaisesTypeError) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class D: pass
class C:
  def __iter__(self):
    return D()
container = C()
)")
                   .isError());
  Frame* frame = thread_->currentFrame();
  Object container(&scope, mainModuleAt(&runtime_, "container"));
  Object val(&scope, NoneType::object());
  Object result(
      &scope, Interpreter::sequenceIterSearch(thread_, frame, val, container));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST_F(InterpreterTest,
       SequenceIterSearchWithNonCallableDunderNextRaisesTypeError) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class D:
  __next__ = None
class C:
  def __iter__(self):
    return D()
container = C()
)")
                   .isError());
  Frame* frame = thread_->currentFrame();
  Object container(&scope, mainModuleAt(&runtime_, "container"));
  Object val(&scope, NoneType::object());
  Object result(
      &scope, Interpreter::sequenceIterSearch(thread_, frame, val, container));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST_F(InterpreterTest, SequenceIterSearchWithListReturnsTrue) {
  HandleScope scope(thread_);
  Frame* frame = thread_->currentFrame();
  List container(&scope, listFromRange(1, 3));
  Object val(&scope, SmallInt::fromWord(2));
  Object result(
      &scope, Interpreter::sequenceIterSearch(thread_, frame, val, container));
  ASSERT_FALSE(result.isError());
  EXPECT_EQ(result, Bool::trueObj());
}

TEST_F(InterpreterTest, SequenceIterSearchWithListReturnsFalse) {
  HandleScope scope(thread_);
  Object container(&scope, listFromRange(1, 3));
  Object val(&scope, SmallInt::fromWord(5));
  Frame* frame = thread_->currentFrame();
  Object result(
      &scope, Interpreter::sequenceIterSearch(thread_, frame, val, container));
  ASSERT_FALSE(result.isError());
  EXPECT_EQ(result, Bool::falseObj());
}

TEST_F(InterpreterTest, SequenceIterSearchWithSequenceSearchesIterator) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Seq:
    def __getitem__(s, i):
        return ("foo", "bar", 42)[i]

seq_iter = Seq()
)")
                   .isError());
  Frame* frame = thread_->currentFrame();
  ASSERT_TRUE(frame->isSentinel());
  Object seq_iter(&scope, mainModuleAt(&runtime_, "seq_iter"));
  Object obj_in_seq(&scope, SmallInt::fromWord(42));
  Object contains_true(&scope, Interpreter::sequenceIterSearch(
                                   thread_, frame, obj_in_seq, seq_iter));
  EXPECT_EQ(contains_true, Bool::trueObj());
  Object obj_not_in_seq(&scope, NoneType::object());
  Object contains_false(&scope, Interpreter::sequenceIterSearch(
                                    thread_, frame, obj_not_in_seq, seq_iter));
  EXPECT_EQ(contains_false, Bool::falseObj());
}

TEST_F(InterpreterTest,
       SequenceIterSearchWithIterThatRaisesPropagatesException) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  def __iter__(self):
    raise ZeroDivisionError("boom")
container = C()
)")
                   .isError());
  Object container(&scope, mainModuleAt(&runtime_, "container"));
  Object val(&scope, SmallInt::fromWord(5));
  Frame* frame = thread_->currentFrame();
  Object result(
      &scope, Interpreter::sequenceIterSearch(thread_, frame, val, container));
  EXPECT_TRUE(raised(*result, LayoutId::kZeroDivisionError));
}

TEST_F(InterpreterTest, ContextManagerCallEnterExit) {
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
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());
  Object a(&scope, mainModuleAt(&runtime_, "a"));
  EXPECT_TRUE(isIntEqualsWord(*a, 3));
  Object b(&scope, mainModuleAt(&runtime_, "b"));
  EXPECT_TRUE(isIntEqualsWord(*b, 2));
}

TEST_F(InterpreterTest, StackCleanupAfterCallFunction) {
  // Build the following function
  //    def foo(a=1, b=2):
  //      return 42
  //
  // Then call as foo(1) and verify that the stack is cleaned up after
  // default argument expansion
  //
  HandleScope scope(thread_);

  Code code(&scope, newEmptyCode());

  Tuple consts(&scope, runtime_.newTuple(1));
  consts.atPut(0, SmallInt::fromWord(42));
  code.setConsts(*consts);

  Tuple names(&scope, runtime_.newTuple(1));
  Object key(&scope, runtime_.newStrFromCStr("foo"));
  names.atPut(0, *key);
  code.setNames(*names);
  code.setArgcount(2);
  code.setNlocals(2);
  code.setStacksize(1);

  const byte bytecode[] = {LOAD_CONST, 0, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));

  Object qualname(&scope, Str::empty());
  Module module(&scope, runtime_.findOrCreateMainModule());
  Function callee(
      &scope, runtime_.newFunctionWithCode(thread_, qualname, code, module));
  Tuple defaults(&scope, runtime_.newTuple(2));

  defaults.atPut(0, SmallInt::fromWord(1));
  defaults.atPut(1, SmallInt::fromWord(2));
  callee.setDefaults(*defaults);

  // Save starting value stack top
  Frame* frame = thread_->currentFrame();
  RawObject* value_stack_start = frame->valueStackTop();

  // Push function pointer and argument
  frame->pushValue(*callee);
  frame->pushValue(SmallInt::fromWord(1));

  // Make sure we got the right result and stack is back where it should be
  EXPECT_TRUE(isIntEqualsWord(Interpreter::call(thread_, frame, 1), 42));
  EXPECT_EQ(value_stack_start, frame->valueStackTop());
}

TEST_F(InterpreterTest, StackCleanupAfterCallExFunction) {
  // Build the following function
  //    def foo(a=1, b=2):
  //      return 42
  //
  // Then call as "f=(2,); foo(*f)" and verify that the stack is cleaned up
  // after ex and default argument expansion
  //
  HandleScope scope(thread_);

  Code code(&scope, newEmptyCode());

  Tuple consts(&scope, runtime_.newTuple(1));
  consts.atPut(0, SmallInt::fromWord(42));
  code.setConsts(*consts);

  Tuple names(&scope, runtime_.newTuple(1));
  Object key(&scope, runtime_.newStrFromCStr("foo"));
  names.atPut(0, *key);
  code.setNames(*names);
  code.setArgcount(2);
  code.setNlocals(2);
  code.setStacksize(1);

  const byte bytecode[] = {LOAD_CONST, 0, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));

  Object qualname(&scope, Str::empty());
  Module module(&scope, runtime_.findOrCreateMainModule());
  Function callee(
      &scope, runtime_.newFunctionWithCode(thread_, qualname, code, module));
  Tuple defaults(&scope, runtime_.newTuple(2));

  defaults.atPut(0, SmallInt::fromWord(1));
  defaults.atPut(1, SmallInt::fromWord(2));
  callee.setDefaults(*defaults);

  // Save starting value stack top
  Frame* frame = thread_->currentFrame();
  RawObject* value_stack_start = frame->valueStackTop();

  // Push function pointer and argument
  Tuple ex(&scope, runtime_.newTuple(1));
  ex.atPut(0, SmallInt::fromWord(2));
  frame->pushValue(*callee);
  frame->pushValue(*ex);

  // Make sure we got the right result and stack is back where it should be
  EXPECT_TRUE(isIntEqualsWord(Interpreter::callEx(thread_, frame, 0), 42));
  EXPECT_EQ(value_stack_start, frame->valueStackTop());
}

TEST_F(InterpreterTest, StackCleanupAfterCallKwFunction) {
  HandleScope scope(thread_);

  // Build the following function
  //    def foo(a=1, b=2):
  //      return 42
  //
  // Then call as "foo(b=4)" and verify that the stack is cleaned up after
  // ex and default argument expansion
  //

  Code code(&scope, newEmptyCode());

  Tuple consts(&scope, runtime_.newTuple(1));
  consts.atPut(0, SmallInt::fromWord(42));
  code.setConsts(*consts);

  Tuple names(&scope, runtime_.newTuple(1));
  Object key(&scope, runtime_.newStrFromCStr("foo"));
  names.atPut(0, *key);
  code.setNames(*names);
  code.setArgcount(2);
  code.setNlocals(2);
  code.setStacksize(1);
  Tuple var_names(&scope, runtime_.newTuple(2));
  var_names.atPut(0, runtime_.newStrFromCStr("a"));
  var_names.atPut(1, runtime_.newStrFromCStr("b"));
  code.setVarnames(*var_names);

  const byte bytecode[] = {LOAD_CONST, 0, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));

  Object qualname(&scope, Str::empty());
  Module module(&scope, runtime_.findOrCreateMainModule());
  Function callee(
      &scope, runtime_.newFunctionWithCode(thread_, qualname, code, module));
  Tuple defaults(&scope, runtime_.newTuple(2));
  defaults.atPut(0, SmallInt::fromWord(1));
  defaults.atPut(1, SmallInt::fromWord(2));
  callee.setDefaults(*defaults);

  // Save starting value stack top
  Frame* frame = thread_->currentFrame();
  RawObject* value_stack_start = frame->valueStackTop();

  // Push function pointer and argument
  Tuple arg_names(&scope, runtime_.newTuple(1));
  arg_names.atPut(0, runtime_.newStrFromCStr("b"));
  frame->pushValue(*callee);
  frame->pushValue(SmallInt::fromWord(4));
  frame->pushValue(*arg_names);

  // Make sure we got the right result and stack is back where it should be
  EXPECT_TRUE(isIntEqualsWord(Interpreter::callKw(thread_, frame, 1), 42));
  EXPECT_EQ(value_stack_start, frame->valueStackTop());
}

TEST_F(InterpreterTest, LookupMethodInvokesDescriptor) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def f(): pass

class D:
    def __get__(self, obj, owner):
        return f

class C:
    __call__ = D()

c = C()
  )")
                   .isError());
  Frame* frame = thread_->currentFrame();
  ASSERT_TRUE(frame->isSentinel());
  Object c(&scope, mainModuleAt(&runtime_, "c"));
  Object f(&scope, mainModuleAt(&runtime_, "f"));
  Object method(&scope, Interpreter::lookupMethod(thread_, frame, c,
                                                  SymbolId::kDunderCall));
  EXPECT_EQ(*f, *method);
}

TEST_F(InterpreterTest, PrepareCallableCallUnpacksBoundMethod) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  def foo():
    pass
meth = C().foo
)")
                   .isError());
  Object meth_obj(&scope, mainModuleAt(&runtime_, "meth"));
  ASSERT_TRUE(meth_obj.isBoundMethod());

  Frame* frame = thread_->currentFrame();
  frame->pushValue(*meth_obj);
  frame->pushValue(SmallInt::fromWord(1234));
  ASSERT_EQ(frame->valueStackSize(), 2);
  word nargs = 1;
  Object callable(
      &scope, Interpreter::prepareCallableCall(thread_, frame, nargs, &nargs));
  ASSERT_TRUE(callable.isFunction());
  ASSERT_EQ(nargs, 2);
  ASSERT_EQ(frame->valueStackSize(), 3);
  EXPECT_TRUE(isIntEqualsWord(frame->peek(0), 1234));
  EXPECT_TRUE(frame->peek(1).isInstance());
  EXPECT_EQ(frame->peek(2), *callable);
}

TEST_F(InterpreterTest, CallExWithListSubclassCallsDunderIter) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
class C(list):
  def __iter__(self):
    raise UserWarning('foo')

def f(a, b, c):
  return (a, b, c)

c = C([1, 2, 3])
f(*c)
)"),
                            LayoutId::kUserWarning, "foo"));
}

TEST_F(InterpreterTest, CallingUncallableRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, "(1)()"),
                            LayoutId::kTypeError,
                            "'int' object is not callable"));
}

TEST_F(InterpreterTest, CallingUncallableDunderCallRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
class C:
  __call__ = 1

c = C()
c()
  )"),
                            LayoutId::kTypeError,
                            "'int' object is not callable"));
}

TEST_F(InterpreterTest, CallingNonDescriptorDunderCallRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
class D: pass

class C:
  __call__ = D()

c = C()
c()
  )"),
                            LayoutId::kTypeError,
                            "'D' object is not callable"));
}

TEST_F(InterpreterTest, CallDescriptorReturningUncallableRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
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

TEST_F(InterpreterTest, LookupMethodLoopsOnCallBoundToDescriptor) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
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
  Frame* frame = thread_->currentFrame();
  ASSERT_TRUE(frame->isSentinel());
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_EQ(*result, SmallInt::fromWord(42));
}

TEST_F(InterpreterTest, IterateOnNonIterable) {
  const char* src = R"(
# Try to iterate on a None object which isn't iterable
a, b = None
)";
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, src), LayoutId::kTypeError,
                            "'NoneType' object is not iterable"));
}

TEST_F(InterpreterTest, DunderIterReturnsNonIterable) {
  const char* src = R"(
class Foo:
  def __iter__(self):
    return 1
a, b = Foo()
)";
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, src), LayoutId::kTypeError,
                            "iter() returned non-iterator of type 'int'"));
}

TEST_F(InterpreterTest, UnpackSequence) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
l = [1, 2, 3]
a, b, c = l
)")
                   .isError());
  Object a(&scope, mainModuleAt(&runtime_, "a"));
  Object b(&scope, mainModuleAt(&runtime_, "b"));
  Object c(&scope, mainModuleAt(&runtime_, "c"));
  EXPECT_TRUE(isIntEqualsWord(*a, 1));
  EXPECT_TRUE(isIntEqualsWord(*b, 2));
  EXPECT_TRUE(isIntEqualsWord(*c, 3));
}

TEST_F(InterpreterTest, UnpackSequenceWithSeqIterator) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Seq:
  def __getitem__(s, i):
    return ("foo", "bar", 42)[i]
a, b, c = Seq()
)")
                   .isError());
  Object a(&scope, mainModuleAt(&runtime_, "a"));
  Object b(&scope, mainModuleAt(&runtime_, "b"));
  Object c(&scope, mainModuleAt(&runtime_, "c"));
  EXPECT_TRUE(isStrEqualsCStr(*a, "foo"));
  EXPECT_TRUE(isStrEqualsCStr(*b, "bar"));
  EXPECT_TRUE(isIntEqualsWord(*c, 42));
}

TEST_F(InterpreterTest, UnpackSequenceTooFewObjects) {
  const char* src = R"(
l = [1, 2]
a, b, c = l
)";
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, src), LayoutId::kValueError,
                            "not enough values to unpack"));
}

TEST_F(InterpreterTest, UnpackSequenceTooManyObjects) {
  const char* src = R"(
l = [1, 2, 3, 4]
a, b, c = l
)";
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, src), LayoutId::kValueError,
                            "too many values to unpack"));
}

TEST_F(InterpreterTest, UnpackTuple) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
l = (1, 2, 3)
a, b, c = l
)")
                   .isError());
  Object a(&scope, mainModuleAt(&runtime_, "a"));
  Object b(&scope, mainModuleAt(&runtime_, "b"));
  Object c(&scope, mainModuleAt(&runtime_, "c"));
  EXPECT_TRUE(isIntEqualsWord(*a, 1));
  EXPECT_TRUE(isIntEqualsWord(*b, 2));
  EXPECT_TRUE(isIntEqualsWord(*c, 3));
}

TEST_F(InterpreterTest, UnpackTupleTooFewObjects) {
  const char* src = R"(
l = (1, 2)
a, b, c = l
)";
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, src), LayoutId::kValueError,
                            "not enough values to unpack"));
}

TEST_F(InterpreterTest, UnpackTupleTooManyObjects) {
  const char* src = R"(
l = (1, 2, 3, 4)
a, b, c = l
)";
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, src), LayoutId::kValueError,
                            "too many values to unpack"));
}

TEST_F(InterpreterTest, PrintExprInvokesDisplayhook) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
import sys

MY_GLOBAL = 1234

def my_displayhook(value):
  global MY_GLOBAL
  MY_GLOBAL = value

sys.displayhook = my_displayhook
  )")
                   .isError());

  Object unique(&scope, runtime_.newTuple(1));  // unique object

  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(2));
  consts.atPut(0, *unique);
  consts.atPut(1, NoneType::object());
  code.setConsts(*consts);
  code.setNlocals(0);
  const byte bytecode[] = {LOAD_CONST, 0, PRINT_EXPR,   0,
                           LOAD_CONST, 1, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));

  ASSERT_TRUE(runCode(code).isNoneType());

  Object displayhook(&scope, moduleAtByCStr(&runtime_, "sys", "displayhook"));
  Object my_displayhook(&scope, mainModuleAt(&runtime_, "my_displayhook"));
  EXPECT_EQ(*displayhook, *my_displayhook);

  Object my_global(&scope, mainModuleAt(&runtime_, "MY_GLOBAL"));
  EXPECT_EQ(*my_global, *unique);
}

TEST_F(InterpreterTest, PrintExprtDoesntPushToStack) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
import sys

def my_displayhook(value):
  pass

sys.displayhook = my_displayhook
  )")
                   .isError());

  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(2));
  consts.atPut(0, SmallInt::fromWord(42));
  consts.atPut(1, SmallInt::fromWord(0));
  code.setConsts(*consts);

  Tuple varnames(&scope, runtime_.newTuple(0));
  code.setVarnames(*varnames);
  code.setNlocals(0);
  // This bytecode loads 42 onto the stack, along with a value to print.
  // It then returns the top of the stack, which should be 42.
  const byte bytecode[] = {
      LOAD_CONST, 0, LOAD_CONST, 1, PRINT_EXPR, 0, RETURN_VALUE, 0,
  };
  code.setCode(runtime_.newBytesWithAll(bytecode));

  Object result_obj(&scope, runCode(code));
  EXPECT_TRUE(isIntEqualsWord(*result_obj, 42));
}

TEST_F(InterpreterTest, GetAiterCallsAiter) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class AsyncIterable:
  def __aiter__(self):
    return 42

a = AsyncIterable()
)")
                   .isError());

  Object a(&scope, mainModuleAt(&runtime_, "a"));

  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(1));
  consts.atPut(0, *a);
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, GET_AITER, 0, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));

  Object result(&scope, runCode(code));
  EXPECT_TRUE(isIntEqualsWord(*result, 42));
}

TEST_F(InterpreterTest, GetAiterOnNonIterable) {
  HandleScope scope(thread_);
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(1));
  consts.atPut(0, SmallInt::fromWord(123));
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, GET_AITER, 0, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));

  Object result(&scope, runCode(code));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST_F(InterpreterTest, BeforeAsyncWithCallsDunderAenter) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
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
  Object manager(&scope, mainModuleAt(&runtime_, "manager"));
  Object main_obj(&scope, findMainModule(&runtime_));
  ASSERT_TRUE(main_obj.isModule());
  Module main(&scope, *main_obj);

  Code code(&scope, newEmptyCode());
  code.setNlocals(0);
  Tuple consts(&scope, runtime_.newTuple(2));
  consts.atPut(0, SmallInt::fromWord(42));
  consts.atPut(1, *manager);
  code.setConsts(*consts);
  Tuple names(&scope, runtime_.newTuple(1));
  names.atPut(0, runtime_.newStrFromCStr("manager"));
  code.setNames(*names);
  const byte bytecode[] = {LOAD_CONST, 1, BEFORE_ASYNC_WITH, 0, POP_TOP, 0,
                           LOAD_CONST, 0, RETURN_VALUE,      0};
  code.setCode(runtime_.newBytesWithAll(bytecode));
  code.setFlags(Code::Flags::kNofree);

  Dict locals(&scope, runtime_.newDict());
  EXPECT_TRUE(isIntEqualsWord(thread_->exec(code, main, locals), 42));
  Object enter(&scope, mainModuleAt(&runtime_, "enter"));
  EXPECT_EQ(*enter, *manager);
  Object exit(&scope, mainModuleAt(&runtime_, "exit"));
  EXPECT_EQ(*exit, NoneType::object());
}

TEST_F(InterpreterTest, SetupAsyncWithPushesBlock) {
  HandleScope scope(thread_);

  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(2));
  consts.atPut(0, SmallInt::fromWord(42));
  consts.atPut(1, NoneType::object());
  code.setConsts(*consts);
  code.setNlocals(0);
  const byte bytecode[] = {
      LOAD_CONST, 0, LOAD_CONST,   1, SETUP_ASYNC_WITH, 0,
      POP_BLOCK,  0, RETURN_VALUE, 0,
  };
  code.setCode(runtime_.newBytesWithAll(bytecode));
  EXPECT_EQ(runCode(code), SmallInt::fromWord(42));
}

TEST_F(InterpreterTest, UnpackSequenceEx) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
l = [1, 2, 3, 4, 5, 6, 7]
a, b, c, *d, e, f, g  = l
)")
                   .isError());
  Object a(&scope, mainModuleAt(&runtime_, "a"));
  Object b(&scope, mainModuleAt(&runtime_, "b"));
  Object c(&scope, mainModuleAt(&runtime_, "c"));
  EXPECT_TRUE(isIntEqualsWord(*a, 1));
  EXPECT_TRUE(isIntEqualsWord(*b, 2));
  EXPECT_TRUE(isIntEqualsWord(*c, 3));

  Object d(&scope, mainModuleAt(&runtime_, "d"));
  ASSERT_TRUE(d.isList());
  List list(&scope, *d);
  EXPECT_EQ(list.numItems(), 1);
  EXPECT_TRUE(isIntEqualsWord(list.at(0), 4));

  Object e(&scope, mainModuleAt(&runtime_, "e"));
  Object f(&scope, mainModuleAt(&runtime_, "f"));
  Object g(&scope, mainModuleAt(&runtime_, "g"));
  EXPECT_TRUE(isIntEqualsWord(*e, 5));
  EXPECT_TRUE(isIntEqualsWord(*f, 6));
  EXPECT_TRUE(isIntEqualsWord(*g, 7));
}

TEST_F(InterpreterTest, UnpackSequenceExWithSeqIterator) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Seq:
  def __getitem__(s, i):
    return ("foo", "bar", 42)[i]
a, *b = Seq()
)")
                   .isError());
  EXPECT_TRUE(isStrEqualsCStr(mainModuleAt(&runtime_, "a"), "foo"));
  Object b(&scope, mainModuleAt(&runtime_, "b"));
  EXPECT_PYLIST_EQ(b, {"bar", 42});
}

TEST_F(InterpreterTest, UnpackSequenceExWithNoElementsAfter) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
l = [1, 2, 3, 4]
a, b, *c = l
)")
                   .isError());
  Object a(&scope, mainModuleAt(&runtime_, "a"));
  Object b(&scope, mainModuleAt(&runtime_, "b"));
  Object c(&scope, mainModuleAt(&runtime_, "c"));
  EXPECT_TRUE(isIntEqualsWord(*a, 1));
  EXPECT_TRUE(isIntEqualsWord(*b, 2));

  ASSERT_TRUE(c.isList());
  List list(&scope, *c);
  ASSERT_EQ(list.numItems(), 2);
  EXPECT_TRUE(isIntEqualsWord(list.at(0), 3));
  EXPECT_TRUE(isIntEqualsWord(list.at(1), 4));
}

TEST_F(InterpreterTest, UnpackSequenceExWithNoElementsBefore) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
l = [1, 2, 3, 4]
*a, b, c = l
)")
                   .isError());
  Object a(&scope, mainModuleAt(&runtime_, "a"));
  Object b(&scope, mainModuleAt(&runtime_, "b"));
  Object c(&scope, mainModuleAt(&runtime_, "c"));
  ASSERT_TRUE(a.isList());
  List list(&scope, *a);
  ASSERT_EQ(list.numItems(), 2);
  EXPECT_TRUE(isIntEqualsWord(list.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(list.at(1), 2));

  EXPECT_TRUE(isIntEqualsWord(*b, 3));
  EXPECT_TRUE(isIntEqualsWord(*c, 4));
}

TEST_F(InterpreterTest, BuildMapCallsDunderHashAndPropagatesException) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
class C:
  def __hash__(self):
    raise ValueError('foo')
d = {C(): 4}
)"),
                            LayoutId::kValueError, "foo"));
}

TEST_F(InterpreterTest, BuildMapUnpackWithDict) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
d = {**{'a': 1, 'b': 2}, 'c': 3, **{'d': 4}}
)")
                   .isError());

  Object d(&scope, mainModuleAt(&runtime_, "d"));
  ASSERT_TRUE(d.isDict());

  Dict dict(&scope, *d);
  EXPECT_EQ(dict.numItems(), 4);

  Str name(&scope, runtime_.newStrFromCStr("a"));
  Object el0(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el0, 1));

  name = runtime_.newStrFromCStr("b");
  Object el1(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el1, 2));

  name = runtime_.newStrFromCStr("c");
  Object el2(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el2, 3));

  name = runtime_.newStrFromCStr("d");
  Object el3(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el3, 4));
}

TEST_F(InterpreterTest, BuildMapUnpackWithListKeysMapping) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
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

  Object d(&scope, mainModuleAt(&runtime_, "d"));
  ASSERT_TRUE(d.isDict());

  Dict dict(&scope, *d);
  EXPECT_EQ(dict.numItems(), 4);

  Str name(&scope, runtime_.newStrFromCStr("a"));
  Object el0(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el0, 1));

  name = runtime_.newStrFromCStr("b");
  Object el1(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el1, 2));

  name = runtime_.newStrFromCStr("c");
  Object el2(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el2, 3));

  name = runtime_.newStrFromCStr("d");
  Object el3(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el3, 4));
}

TEST_F(InterpreterTest, BuildMapUnpackWithTupleKeysMapping) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
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

  Object d(&scope, mainModuleAt(&runtime_, "d"));
  ASSERT_TRUE(d.isDict());

  Dict dict(&scope, *d);
  EXPECT_EQ(dict.numItems(), 4);

  Str name(&scope, runtime_.newStrFromCStr("a"));
  Object el0(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el0, 1));

  name = runtime_.newStrFromCStr("b");
  Object el1(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el1, 2));

  name = runtime_.newStrFromCStr("c");
  Object el2(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el2, 3));

  name = runtime_.newStrFromCStr("d");
  Object el3(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el3, 4));
}

TEST_F(InterpreterTest, BuildMapUnpackWithIterableKeysMapping) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
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

  Object d(&scope, mainModuleAt(&runtime_, "d"));
  ASSERT_TRUE(d.isDict());

  Dict dict(&scope, *d);
  EXPECT_EQ(dict.numItems(), 4);

  Str name(&scope, runtime_.newStrFromCStr("a"));
  Object el0(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el0, 1));

  name = runtime_.newStrFromCStr("b");
  Object el1(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el1, 2));

  name = runtime_.newStrFromCStr("c");
  Object el2(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el2, 3));

  name = runtime_.newStrFromCStr("d");
  Object el3(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el3, 4));
}

TEST_F(InterpreterTest, BuildMapUnpackWithNonMapping) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
class Foo:
    pass

d = {**Foo(), 'd': 4}
  )"),
                            LayoutId::kTypeError, "object is not a mapping"));
}

TEST_F(InterpreterTest, BuildMapUnpackWithUnsubscriptableMapping) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
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

TEST_F(InterpreterTest, BuildMapUnpackWithNonIterableKeys) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
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

TEST_F(InterpreterTest, BuildMapUnpackWithBadIteratorKeys) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
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

TEST_F(InterpreterTest, BuildSetCallsDunderHashAndPropagatesException) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
class C:
  def __hash__(self):
    raise ValueError('foo')
s = {C()}
)"),
                            LayoutId::kValueError, "foo"));
}

TEST_F(InterpreterTest, UnpackSequenceExWithTooFewObjectsBefore) {
  const char* src = R"(
l = [1, 2]
a, b, c, *d  = l
)";
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, src), LayoutId::kValueError,
                            "not enough values to unpack"));
}

TEST_F(InterpreterTest, UnpackSequenceExWithTooFewObjectsAfter) {
  const char* src = R"(
l = [1, 2]
*a, b, c, d = l
)";
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, src), LayoutId::kValueError,
                            "not enough values to unpack"));
}

TEST_F(InterpreterTest, BuildTupleUnpackWithCall) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def foo(*args):
    return args

t = foo(*(1,2), *(3, 4))
)")
                   .isError());

  Object t(&scope, mainModuleAt(&runtime_, "t"));
  ASSERT_TRUE(t.isTuple());

  Tuple tuple(&scope, *t);
  EXPECT_TRUE(isIntEqualsWord(tuple.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(tuple.at(1), 2));
  EXPECT_TRUE(isIntEqualsWord(tuple.at(2), 3));
  EXPECT_TRUE(isIntEqualsWord(tuple.at(3), 4));
}

TEST_F(InterpreterTest, FunctionDerefsVariable) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def outer():
    var = 1
    def inner():
        return var
    del var
    return 0

v = outer()
	)")
                   .isError());

  Object v(&scope, mainModuleAt(&runtime_, "v"));
  EXPECT_TRUE(isIntEqualsWord(*v, 0));
}

TEST_F(InterpreterTest, FunctionAccessesUnboundVariable) {
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
      raisedWithStr(runFromCStr(&runtime_, src), LayoutId::kUnboundLocalError,
                    "local variable 'var' referenced before assignment"));
}

TEST_F(InterpreterTest, ImportStarImportsPublicSymbols) {
  HandleScope scope(thread_);
  Object module_src(&scope, runtime_.newStrFromCStr(R"(
def public_symbol():
    return 1
def public_symbol2():
    return 2
)"));
  Object filename(&scope, runtime_.newStrFromCStr("<test string>"));

  // Preload the module
  Object name(&scope, runtime_.newStrFromCStr("test_module"));
  Code code(&scope,
            compile(thread_, module_src, filename, SymbolId::kExec, 0, -1));
  ASSERT_FALSE(runtime_.importModuleFromCode(code, name).isError());

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
from test_module import *
a = public_symbol()
b = public_symbol2()
)")
                   .isError());

  Object a(&scope, mainModuleAt(&runtime_, "a"));
  Object b(&scope, mainModuleAt(&runtime_, "b"));
  EXPECT_TRUE(isIntEqualsWord(*a, 1));
  EXPECT_TRUE(isIntEqualsWord(*b, 2));
}

TEST_F(InterpreterTest, ImportStarDoesNotImportPrivateSymbols) {
  HandleScope scope(thread_);
  Object module_src(&scope, runtime_.newStrFromCStr(R"(
def public_symbol():
    return 1
def _private_symbol():
    return 2
)"));
  Object filename(&scope, runtime_.newStrFromCStr("<test string>"));

  // Preload the module
  Object name(&scope, runtime_.newStrFromCStr("test_module"));
  Code code(&scope,
            compile(thread_, module_src, filename, SymbolId::kExec, 0, -1));
  ASSERT_FALSE(runtime_.importModuleFromCode(code, name).isError());

  const char* main_src = R"(
from test_module import *
a = public_symbol()
b = _private_symbol()
)";

  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, main_src),
                            LayoutId::kNameError,
                            "name '_private_symbol' is not defined"));
}

TEST_F(InterpreterTest, ImportCallsBuiltinsDunderImport) {
  ASSERT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
import builtins
def import_forbidden(name, globals, locals, fromlist, level):
  raise Exception("import forbidden")
builtins.__import__ = import_forbidden
import builtins
)"),
                            LayoutId::kException, "import forbidden"));
}

TEST_F(InterpreterTest, GetAnextCallsAnextAndAwait) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
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
  Object a(&scope, mainModuleAt(&runtime_, "a"));

  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(1));
  consts.atPut(0, *a);
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, GET_ANEXT, 0, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));

  Object result(&scope, runCode(code));
  EXPECT_EQ(*a, *result);
  Object anext(&scope, mainModuleAt(&runtime_, "anext_called"));
  EXPECT_EQ(*a, *anext);
  Object await(&scope, mainModuleAt(&runtime_, "await_called"));
  EXPECT_EQ(*a, *await);
}

TEST_F(InterpreterTest, GetAnextOnNonIterable) {
  HandleScope scope(thread_);
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(1));
  consts.atPut(0, SmallInt::fromWord(123));
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, GET_ANEXT, 0, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));

  Object result(&scope, runCode(code));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST_F(InterpreterTest, GetAnextWithInvalidAnext) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class AsyncIterator:
  def __anext__(self):
    return 42

a = AsyncIterator()
)")
                   .isError());
  Object a(&scope, mainModuleAt(&runtime_, "a"));

  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(1));
  consts.atPut(0, *a);
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, GET_ANEXT, 0, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));

  Object result(&scope, runCode(code));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST_F(InterpreterTest, GetAwaitableCallsAwait) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Awaitable:
  def __await__(self):
    return 42

a = Awaitable()
)")
                   .isError());

  Object a(&scope, mainModuleAt(&runtime_, "a"));

  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(1));
  consts.atPut(0, *a);
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, GET_AWAITABLE, 0, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));

  Object result(&scope, runCode(code));
  EXPECT_TRUE(isIntEqualsWord(*result, 42));
}

TEST_F(InterpreterTest, GetAwaitableOnNonAwaitable) {
  HandleScope scope(thread_);
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(1));
  consts.atPut(0, runtime_.newStrFromCStr("foo"));
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, GET_AWAITABLE, 0, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));

  Object result(&scope, runCode(code));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST_F(InterpreterTest, BuildMapUnpackWithCallDict) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def foo(**kwargs):
    return kwargs

d = foo(**{'a': 1, 'b': 2}, **{'c': 3, 'd': 4})
)")
                   .isError());

  Object d(&scope, mainModuleAt(&runtime_, "d"));
  ASSERT_TRUE(d.isDict());

  Dict dict(&scope, *d);
  EXPECT_EQ(dict.numItems(), 4);

  Str name(&scope, runtime_.newStrFromCStr("a"));
  Object el0(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el0, 1));

  name = runtime_.newStrFromCStr("b");
  Object el1(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el1, 2));

  name = runtime_.newStrFromCStr("c");
  Object el2(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el2, 3));

  name = runtime_.newStrFromCStr("d");
  Object el3(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el3, 4));
}

TEST_F(InterpreterTest, BuildMapUnpackWithCallTupleKeys) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
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

  Object d(&scope, mainModuleAt(&runtime_, "d"));
  ASSERT_TRUE(d.isDict());

  Dict dict(&scope, *d);
  EXPECT_EQ(dict.numItems(), 4);

  Str name(&scope, runtime_.newStrFromCStr("a"));
  Object el0(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el0, 1));

  name = runtime_.newStrFromCStr("b");
  Object el1(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el1, 2));

  name = runtime_.newStrFromCStr("c");
  Object el2(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el2, 3));

  name = runtime_.newStrFromCStr("d");
  Object el3(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el3, 4));
}

TEST_F(InterpreterTest, BuildMapUnpackWithCallListKeys) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
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

  Object d(&scope, mainModuleAt(&runtime_, "d"));
  ASSERT_TRUE(d.isDict());

  Dict dict(&scope, *d);
  EXPECT_EQ(dict.numItems(), 4);

  Str name(&scope, runtime_.newStrFromCStr("a"));
  Object el0(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el0, 1));

  name = runtime_.newStrFromCStr("b");
  Object el1(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el1, 2));

  name = runtime_.newStrFromCStr("c");
  Object el2(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el2, 3));

  name = runtime_.newStrFromCStr("d");
  Object el3(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el3, 4));
}

TEST_F(InterpreterTest, BuildMapUnpackWithCallIteratorKeys) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
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

  Object d(&scope, mainModuleAt(&runtime_, "d"));
  ASSERT_TRUE(d.isDict());

  Dict dict(&scope, *d);
  EXPECT_EQ(dict.numItems(), 4);

  Str name(&scope, runtime_.newStrFromCStr("a"));
  Object el0(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el0, 1));

  name = runtime_.newStrFromCStr("b");
  Object el1(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el1, 2));

  name = runtime_.newStrFromCStr("c");
  Object el2(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el2, 3));

  name = runtime_.newStrFromCStr("d");
  Object el3(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el3, 4));
}

TEST_F(InterpreterTest, BuildMapUnpackWithCallDictNonStrKey) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
def foo(**kwargs):
    return kwargs

foo(**{'a': 1, 'b': 2}, **{'c': 3, 4: 4})
  )"),
                            LayoutId::kTypeError, "keywords must be strings"));
}

TEST_F(InterpreterTest, BuildMapUnpackWithCallDictRepeatedKeys) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
def foo(**kwargs):
    return kwargs

foo(**{'a': 1, 'b': 2}, **{'c': 3, 'a': 4})
  )"),
                            LayoutId::kTypeError,
                            "got multiple values for keyword argument"));
}

TEST_F(InterpreterTest, BuildMapUnpackWithCallNonMapping) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
class Foo:
    pass

def foo(**kwargs):
    return kwargs

foo(**{'a': 1, 'b': 2}, **Foo())
  )"),
                            LayoutId::kTypeError, "object is not a mapping"));
}

TEST_F(InterpreterTest, BuildMapUnpackWithCallNonSubscriptable) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
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

TEST_F(InterpreterTest, BuildMapUnpackWithCallListKeysNonStrKey) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
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

TEST_F(InterpreterTest, BuildMapUnpackWithCallListKeysRepeatedKeys) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
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

TEST_F(InterpreterTest, BuildMapUnpackWithCallTupleKeysNonStrKeys) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
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

TEST_F(InterpreterTest, BuildMapUnpackWithCallTupleKeysRepeatedKeys) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
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

TEST_F(InterpreterTest, BuildMapUnpackWithCallNonIterableKeys) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
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

TEST_F(InterpreterTest, BuildMapUnpackWithCallIterableWithoutNext) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
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

TEST_F(InterpreterTest, BuildMapUnpackWithCallIterableNonStrKey) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
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

TEST_F(InterpreterTest, BuildMapUnpackWithCallIterableRepeatedKeys) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
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

TEST_F(InterpreterTest, YieldFromIterReturnsIter) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class FooIterator:
    def __next__(self):
        pass

class Foo:
    def __iter__(self):
        return FooIterator()

foo = Foo()
	)")
                   .isError());

  Object foo(&scope, mainModuleAt(&runtime_, "foo"));

  // Create a code object and set the foo instance as a const
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(1));
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
  code.setCode(runtime_.newBytesWithAll(bytecode));

  // Confirm that the returned value is the iterator of Foo
  Object result(&scope, runCode(code));
  Type result_type(&scope, runtime_.typeOf(*result));
  EXPECT_TRUE(isStrEqualsCStr(result_type.name(), "FooIterator"));
}

TEST_F(InterpreterTest, YieldFromIterWithSequenceReturnsIter) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class FooSequence:
    def __getitem__(self, i):
        return ("foo", "bar")[i]

foo = FooSequence()
	)")
                   .isError());

  Object foo(&scope, mainModuleAt(&runtime_, "foo"));

  // Create a code object and set the foo instance as a const
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(1));
  consts.atPut(0, *foo);
  code.setConsts(*consts);

  // Python code:
  // foo = FooSequence()
  // def bar():
  //     yield from foo
  const byte bytecode[] = {
      LOAD_CONST,          0,  // (foo)
      GET_YIELD_FROM_ITER, 0,  // iter(foo)
      RETURN_VALUE,        0,
  };
  code.setCode(runtime_.newBytesWithAll(bytecode));

  // Confirm that the returned value is a sequence iterator
  Object result(&scope, runCode(code));
  Type result_type(&scope, runtime_.typeOf(*result));
  EXPECT_TRUE(isStrEqualsCStr(result_type.name(), "iterator"));
}

TEST_F(InterpreterTest, YieldFromIterRaisesException) {
  const char* src = R"(
def yield_from_func():
    yield from 1

for i in yield_from_func():
    pass
	)";

  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, src), LayoutId::kTypeError,
                            "'int' object is not iterable"));
}

TEST_F(InterpreterTest, MakeFunctionSetsDunderModule) {
  HandleScope scope(thread_);
  Object module_name(&scope, runtime_.newStrFromCStr("foo"));
  Object module_src(&scope, runtime_.newStrFromCStr(R"(
def bar(): pass
)"));
  Object filename(&scope, runtime_.newStrFromCStr("<test string>"));
  Code code(&scope,
            compile(thread_, module_src, filename, SymbolId::kExec, 0, -1));
  ASSERT_FALSE(runtime_.importModuleFromCode(code, module_name).isError());
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
import foo
def baz(): pass
a = getattr(foo.bar, '__module__')
b = getattr(baz, '__module__')
)")
                   .isError());
  Object a(&scope, mainModuleAt(&runtime_, "a"));
  ASSERT_TRUE(a.isStr());
  EXPECT_TRUE(Str::cast(*a).equalsCStr("foo"));
  Object b(&scope, mainModuleAt(&runtime_, "b"));
  ASSERT_TRUE(b.isStr());
  EXPECT_TRUE(Str::cast(*b).equalsCStr("__main__"));
}

TEST_F(InterpreterTest, MakeFunctionSetsDunderQualname) {
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Foo():
    def bar(): pass
def baz(): pass
a = getattr(Foo.bar, '__qualname__')
b = getattr(baz, '__qualname__')
)")
                   .isError());
  Object a(&scope, mainModuleAt(&runtime_, "a"));
  ASSERT_TRUE(a.isStr());
  EXPECT_TRUE(Str::cast(*a).equalsCStr("Foo.bar"));
  Object b(&scope, mainModuleAt(&runtime_, "b"));
  ASSERT_TRUE(b.isStr());
  EXPECT_TRUE(Str::cast(*b).equalsCStr("baz"));
}

TEST_F(InterpreterTest, MakeFunctionSetsDunderDoc) {
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def foo():
    """This is a docstring"""
    pass
def bar(): pass
)")
                   .isError());
  Object foo(&scope, testing::mainModuleAt(&runtime_, "foo"));
  ASSERT_TRUE(foo.isFunction());
  Object foo_docstring(&scope, Function::cast(*foo).doc());
  ASSERT_TRUE(foo_docstring.isStr());
  EXPECT_TRUE(Str::cast(*foo_docstring).equalsCStr("This is a docstring"));

  Object bar(&scope, testing::mainModuleAt(&runtime_, "bar"));
  ASSERT_TRUE(bar.isFunction());
  Object bar_docstring(&scope, Function::cast(*bar).doc());
  EXPECT_TRUE(bar_docstring.isNoneType());
}

TEST_F(InterpreterTest, FunctionCallWithNonFunctionRaisesTypeError) {
  HandleScope scope(thread_);
  Frame* frame = thread_->currentFrame();
  Str not_a_func(&scope, Str::empty());
  frame->pushValue(*not_a_func);
  EXPECT_TRUE(
      raised(Interpreter::call(thread_, frame, 0), LayoutId::kTypeError));
}

TEST_F(InterpreterTest, FunctionCallExWithNonFunctionRaisesTypeError) {
  HandleScope scope(thread_);
  Frame* frame = thread_->currentFrame();
  Str not_a_func(&scope, Str::empty());
  frame->pushValue(*not_a_func);
  Tuple empty_args(&scope, runtime_.emptyTuple());
  frame->pushValue(*empty_args);
  EXPECT_TRUE(raisedWithStr(Interpreter::callEx(thread_, frame, 0),
                            LayoutId::kTypeError,
                            "'str' object is not callable"));
}

TEST_F(InterpreterTest, CallExWithDescriptorDunderCall) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
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
  EXPECT_TRUE(isStrEqualsCStr(mainModuleAt(&runtime_, "result"), "hello!"));
}

TEST_F(InterpreterTest, DoDeleteNameOnDictSubclass) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
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

TEST_F(InterpreterTest, DoStoreNameOnDictSubclass) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
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

TEST_F(InterpreterTest, StoreSubscr) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
l = [0]
for i in range(5):
    l[0] += i
)")
                   .isError());

  HandleScope scope;
  Object l_obj(&scope, testing::mainModuleAt(&runtime_, "l"));
  ASSERT_TRUE(l_obj.isList());
  List l(&scope, *l_obj);
  ASSERT_EQ(l.numItems(), 1);
  EXPECT_EQ(l.at(0), SmallInt::fromWord(10));
}

// TODO(bsimmers) Rewrite these exception tests to ensure that the specific
// bytecodes we care about are being exercised, so we're not be at the mercy of
// compiler optimizations or changes.
TEST_F(InterpreterTest, ExceptCatchesException) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
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
  Object n(&scope, testing::mainModuleAt(&runtime_, "n"));
  EXPECT_TRUE(isIntEqualsWord(*n, 2));
}

TEST_F(InterpreterTest, RaiseCrossesFunctions) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
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
  Object n(&scope, testing::mainModuleAt(&runtime_, "n"));
  EXPECT_TRUE(isIntEqualsWord(*n, 2));
}

TEST_F(InterpreterTest, RaiseFromSetsCause) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
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
  Object exc_obj(&scope, testing::mainModuleAt(&runtime_, "exc"));
  ASSERT_EQ(exc_obj.layoutId(), LayoutId::kTypeError);
  BaseException exc(&scope, *exc_obj);
  EXPECT_EQ(exc.cause().layoutId(), LayoutId::kRuntimeError);
}

TEST_F(InterpreterTest, ExceptWithRightTypeCatches) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
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
  Object n(&scope, testing::mainModuleAt(&runtime_, "n"));
  EXPECT_TRUE(isIntEqualsWord(*n, 2));
}

TEST_F(InterpreterTest, ExceptWithRightTupleTypeCatches) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
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
  Object n(&scope, testing::mainModuleAt(&runtime_, "n"));
  EXPECT_TRUE(isIntEqualsWord(*n, 2));
}

TEST_F(InterpreterTest, ExceptWithWrongTypePasses) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
try:
    raise RuntimeError("something went wrong")
except StopIteration:
    pass
)"),
                            LayoutId::kRuntimeError, "something went wrong"));
}

TEST_F(InterpreterTest, ExceptWithWrongTupleTypePasses) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
try:
    raise RuntimeError("something went wrong")
except (StopIteration, ImportError):
    pass
)"),
                            LayoutId::kRuntimeError, "something went wrong"));
}

TEST_F(InterpreterTest, RaiseTypeCreatesException) {
  EXPECT_TRUE(raised(runFromCStr(&runtime_, "raise StopIteration"),
                     LayoutId::kStopIteration));
}

TEST_F(InterpreterTest, BareRaiseReraises) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
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
  Object my_error(&scope, testing::mainModuleAt(&runtime_, "MyError"));
  EXPECT_EQ(runtime_.typeOf(*my_error), runtime_.typeAt(LayoutId::kType));
  Object inner(&scope, testing::mainModuleAt(&runtime_, "inner"));
  EXPECT_EQ(runtime_.typeOf(*inner), *my_error);
  Object outer(&scope, testing::mainModuleAt(&runtime_, "outer"));
  EXPECT_EQ(*inner, *outer);
}

TEST_F(InterpreterTest, ExceptWithNonExceptionTypeRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
try:
  raise RuntimeError
except str:
  pass
)"),
                            LayoutId::kTypeError,
                            "catching classes that do not inherit from "
                            "BaseException is not allowed"));
}

TEST_F(InterpreterTest, ExceptWithNonExceptionTypeInTupleRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
try:
  raise RuntimeError
except (StopIteration, int, RuntimeError):
  pass
)"),
                            LayoutId::kTypeError,
                            "catching classes that do not inherit from "
                            "BaseException is not allowed"));
}

TEST_F(InterpreterTest, RaiseWithNoActiveExceptionRaisesRuntimeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, "raise\n"),
                            LayoutId::kRuntimeError,
                            "No active exception to reraise"));
}

TEST_F(InterpreterTest, LoadAttrSetLocationSetsLocation) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  def __init__(self):
    self.foo = 42
i = C()
)")
                   .isError());
  Object i(&scope, mainModuleAt(&runtime_, "i"));

  Str name(&scope, runtime_.newStrFromCStr("foo"));
  Object to_cache(&scope, NoneType::object());
  Interpreter::LoadAttrKind kind;
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::loadAttrSetLocation(thread_, i, name, &kind, &to_cache),
      42));
  EXPECT_EQ(kind, Interpreter::LoadAttrKind::kInstance);
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::loadAttrWithLocation(thread_, *i, *to_cache), 42));
}

TEST_F(InterpreterTest, LoadAttrSetLocationSetsLocationToProprty) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
    foo = property (lambda self: "data descriptor")

c = C()
)")
                   .isError());
  Type type_c(&scope, mainModuleAt(&runtime_, "C"));
  Object c(&scope, mainModuleAt(&runtime_, "c"));

  Str name(&scope, runtime_.newStrFromCStr("foo"));
  Object to_cache(&scope, NoneType::object());
  Interpreter::LoadAttrKind kind = Interpreter::LoadAttrKind::kUnknown;
  EXPECT_TRUE(isStrEqualsCStr(
      Interpreter::loadAttrSetLocation(thread_, c, name, &kind, &to_cache),
      "data descriptor"));
  EXPECT_EQ(kind, Interpreter::LoadAttrKind::kInstance);
  EXPECT_TRUE(isStrEqualsCStr(
      resolveDescriptorGet(thread_, to_cache, c, type_c), "data descriptor"));
}

TEST_F(InterpreterTest,
       LoadAttrSetLocationDoesNotSetLocationToProprtyWithNoneGetter) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
C_foo = property (fget=None, fset=lambda self,v: None, fdel=lambda self: None)
class C:
    foo = C_foo

c = C()
)")
                   .isError());
  Object c(&scope, mainModuleAt(&runtime_, "c"));
  Str name(&scope, runtime_.newStrFromCStr("foo"));
  Object to_cache(&scope, NoneType::object());
  Interpreter::LoadAttrKind kind = Interpreter::LoadAttrKind::kUnknown;
  EXPECT_TRUE(
      Interpreter::loadAttrSetLocation(thread_, c, name, &kind, &to_cache)
          .isError());
  EXPECT_TRUE(to_cache.isNoneType());
}

TEST_F(InterpreterTest, LoadAttrWithModuleSetLocationSetsLocation) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a_global = 1234
)")
                   .isError());
  Object mod(&scope, findMainModule(&runtime_));
  Str name(&scope, runtime_.newStrFromCStr("a_global"));

  Object to_cache(&scope, NoneType::object());
  Interpreter::LoadAttrKind kind;
  ASSERT_TRUE(isIntEqualsWord(
      Interpreter::loadAttrSetLocation(thread_, mod, name, &kind, &to_cache),
      1234));
  EXPECT_EQ(kind, Interpreter::LoadAttrKind::kModule);
  EXPECT_EQ(to_cache.layoutId(), LayoutId::kValueCell);
}

TEST_F(InterpreterTest, LoadAttrWithTypeSetLocationSetsLocation) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  an_attribute = 1234
)")
                   .isError());
  Object type(&scope, mainModuleAt(&runtime_, "C"));

  Str name(&scope, runtime_.newStrFromCStr("an_attribute"));

  Object to_cache(&scope, NoneType::object());
  Interpreter::LoadAttrKind kind;
  ASSERT_TRUE(isIntEqualsWord(
      Interpreter::loadAttrSetLocation(thread_, type, name, &kind, &to_cache),
      1234));
  EXPECT_EQ(kind, Interpreter::LoadAttrKind::kType);
  EXPECT_EQ(to_cache.layoutId(), LayoutId::kValueCell);
}

TEST_F(InterpreterTest,
       LoadAttrSetLocationWithCustomGetAttributeSetsNoLocation) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  def __getattribute__(self, name):
    return 11
i = C()
)")
                   .isError());
  Object i(&scope, mainModuleAt(&runtime_, "i"));

  Str name(&scope, runtime_.newStrFromCStr("foo"));
  Object to_cache(&scope, NoneType::object());
  Interpreter::LoadAttrKind kind;
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::loadAttrSetLocation(thread_, i, name, &kind, &to_cache),
      11));
  EXPECT_EQ(kind, Interpreter::LoadAttrKind::kUnknown);
  EXPECT_TRUE(to_cache.isNoneType());
}

TEST_F(InterpreterTest, LoadAttrSetLocationCallsDunderGetattr) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  def __init__(self):
    self.foo = 42
  def __getattr__(self, name):
    return 5
i = C()
)")
                   .isError());
  Object i(&scope, mainModuleAt(&runtime_, "i"));

  Str name(&scope, runtime_.newStrFromCStr("bar"));
  Object to_cache(&scope, NoneType::object());
  Interpreter::LoadAttrKind kind;

  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::loadAttrSetLocation(thread_, i, name, &kind, &to_cache), 5));
  EXPECT_EQ(kind, Interpreter::LoadAttrKind::kInstance);
  EXPECT_TRUE(to_cache.isNoneType());
}

TEST_F(InterpreterTest,
       LoadAttrSetLocationWithNoAttributeRaisesAttributeError) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
obj = object()
)")
                   .isError());

  Object obj(&scope, mainModuleAt(&runtime_, "obj"));
  Str name(&scope, runtime_.newStrFromCStr("nonexistent_attr"));
  Interpreter::LoadAttrKind kind;
  EXPECT_TRUE(raisedWithStr(
      Interpreter::loadAttrSetLocation(thread_, obj, name, &kind, nullptr),
      LayoutId::kAttributeError,
      "'object' object has no attribute 'nonexistent_attr'"));
}

TEST_F(InterpreterTest, LoadAttrWithoutAttrUnwindsAttributeException) {
  HandleScope scope(thread_);

  // Set up a code object that runs: {}.foo
  Code code(&scope, newEmptyCode());
  Tuple names(&scope, runtime_.newTuple(1));
  Str foo(&scope, runtime_.newStrFromCStr("foo"));
  names.atPut(0, *foo);
  code.setNames(*names);

  // load arguments and execute the code
  const byte bytecode[] = {BUILD_MAP, 0, LOAD_ATTR, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));
  code.setStacksize(1);

  // Execute the code and make sure to get the unwinded Error
  EXPECT_TRUE(runCode(code).isError());
}

TEST_F(InterpreterTest, ExplodeCallAcceptsList) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def f(a, b):
  return [b, a]

args = ['a', 'b']
result = f(*args)
)")
                   .isError());

  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {"b", "a"});
}

TEST_F(InterpreterTest, ExplodeWithIterableCalls) {
  HandleScope scope(thread_);
  EXPECT_FALSE(runFromCStr(&runtime_, R"(
def f(a, b):
  return (b, a)
def gen():
  yield 1
  yield 2
result = f(*gen())
)")
                   .isError());

  Object result_obj(&scope, mainModuleAt(&runtime_, "result"));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 2));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 1));
}

TEST_F(InterpreterTest, FormatValueCallsDunderStr) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  def __str__(self):
    return "foobar"
result = f"{C()!s}"
)")
                   .isError());
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(isStrEqualsCStr(*result, "foobar"));
}

TEST_F(InterpreterTest, FormatValueFallsBackToDunderRepr) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  def __repr__(self):
    return "foobar"
result = f"{C()!s}"
)")
                   .isError());
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(isStrEqualsCStr(*result, "foobar"));
}

TEST_F(InterpreterTest, FormatValueCallsDunderRepr) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  def __repr__(self):
    return "foobar"
result = f"{C()!r}"
)")
                   .isError());
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(isStrEqualsCStr(*result, "foobar"));
}

TEST_F(InterpreterTest, FormatValueAsciiCallsDunderRepr) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  def __repr__(self):
    return "foobar"
result = f"{C()!a}"
)")
                   .isError());
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(isStrEqualsCStr(*result, "foobar"));
}

TEST_F(InterpreterTest, BreakInTryBreaks) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = 0
for i in range(5):
  try:
    break
  except:
    pass
result = 10
)")
                   .isError());
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 10));
}

TEST_F(InterpreterTest, ContinueInExceptContinues) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
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
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, -4));
}

TEST_F(InterpreterTest, RaiseInLoopRaisesRuntimeError) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
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
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 1003));
}

TEST_F(InterpreterTest, ReturnInsideTryRunsFinally) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
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
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 56789));

  Object ran_finally(&scope, mainModuleAt(&runtime_, "ran_finally"));
  EXPECT_EQ(*ran_finally, Bool::trueObj());
}

TEST_F(InterpreterTest, ReturnInsideFinallyOverridesEarlierReturn) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def f():
  try:
    return 123
  finally:
    return 456

result = f()
)")
                   .isError());
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 456));
}

TEST_F(InterpreterTest, ReturnInsideWithRunsDunderExit) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
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
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 1234));

  Object sequence(&scope, mainModuleAt(&runtime_, "sequence"));
  EXPECT_TRUE(isStrEqualsCStr(*sequence, "enter in foo exit"));
}

TEST_F(InterpreterTest,
       WithStatementWithManagerWithoutEnterRaisesAttributeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
with None:
  pass
)"),
                            LayoutId::kAttributeError, "__enter__"));
}

TEST_F(InterpreterTest,
       WithStatementWithManagerWithoutExitRaisesAttributeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
class C:
  def __enter__(self):
    pass
with C():
  pass
)"),
                            LayoutId::kAttributeError, "__exit__"));
}

TEST_F(InterpreterTest,
       WithStatementWithManagerEnterRaisingPropagatesException) {
  EXPECT_TRUE(raised(runFromCStr(&runtime_, R"(
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

TEST_F(InterpreterTest, WithStatementPropagatesException) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
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

TEST_F(InterpreterTest, WithStatementPassesCorrectExceptionToExit) {
  HandleScope scope(thread_);
  EXPECT_TRUE(raised(runFromCStr(&runtime_, R"(
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
  Object exit_info(&scope, mainModuleAt(&runtime_, "exit_info"));
  ASSERT_TRUE(exit_info.isTuple());
  Tuple tuple(&scope, *exit_info);
  ASSERT_EQ(tuple.length(), 3);
  EXPECT_EQ(tuple.at(0), runtime_.typeAt(LayoutId::kStopIteration));

  Object raised_exc(&scope, mainModuleAt(&runtime_, "raised_exc"));
  EXPECT_EQ(tuple.at(1), *raised_exc);

  // TODO(bsimmers): Check traceback once we record them.
}

TEST_F(InterpreterTest, WithStatementSwallowsException) {
  HandleScope scope(thread_);
  EXPECT_FALSE(runFromCStr(&runtime_, R"(
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

  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 1234));
}

TEST_F(InterpreterTest, WithStatementWithRaisingExitRaises) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
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

TEST_F(InterpreterTest, LoadNameReturnsSameResultAsCahedValueFromLoadGlobal) {
  EXPECT_FALSE(runFromCStr(&runtime_, R"(
t = 400

def update_t():
  global t
  t = 500

def get_t():
  global t
  return t

update_t()
load_name_t = t
load_global_t = get_t()
)")
                   .isError());
  EXPECT_EQ(mainModuleAt(&runtime_, "load_name_t"),
            mainModuleAt(&runtime_, "load_global_t"));
}

TEST_F(InterpreterTest, LoadGlobalCachedReturnsModuleDictValue) {
  HandleScope scope(thread_);
  EXPECT_FALSE(runFromCStr(&runtime_, R"(
a = 400

def foo():
  return a + a

result = foo()
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(&runtime_, "result"), 800));
  Function function(&scope, mainModuleAt(&runtime_, "foo"));
  ASSERT_TRUE(isStrEqualsCStr(
      Tuple::cast(Code::cast(function.code()).names()).at(0), "a"));
  Tuple caches(&scope, function.caches());
  EXPECT_TRUE(
      isIntEqualsWord(valueCellValue(icLookupGlobalVar(*caches, 0)), 400));
}

TEST_F(InterpreterTest,
       LoadGlobalCachedReturnsBuiltinDictValueAndSetsPlaceholder) {
  HandleScope scope(thread_);
  EXPECT_FALSE(runFromCStr(&runtime_, R"(
__builtins__.a = 400

def foo():
  return a + a

result = foo()
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(&runtime_, "result"), 800));
  Function function(&scope, mainModuleAt(&runtime_, "foo"));
  ASSERT_TRUE(isStrEqualsCStr(
      Tuple::cast(Code::cast(function.code()).names()).at(0), "a"));
  Tuple caches(&scope, function.caches());
  EXPECT_TRUE(
      isIntEqualsWord(valueCellValue(icLookupGlobalVar(*caches, 0)), 400));

  Module module(&scope, function.moduleObject());
  Str name(&scope, runtime_.newStrFromCStr("a"));
  Dict module_dict(&scope, module.dict());
  Object module_entry(&scope, dictAtByStr(thread_, module_dict, name));
  ASSERT_TRUE(module_entry.isValueCell());
  EXPECT_TRUE(ValueCell::cast(*module_entry).isPlaceholder());
}

TEST_F(InterpreterTest, StoreGlobalCachedInvalidatesCachedBuiltinToBeShadowed) {
  HandleScope scope(thread_);
  EXPECT_FALSE(runFromCStr(&runtime_, R"(
__builtins__.a = 400

def foo():
  return a + a

def bar():
  # Shadowing __builtins__.a.
  global a
  a = 123

foo()
bar()
)")
                   .isError());
  Function function(&scope, mainModuleAt(&runtime_, "foo"));
  ASSERT_TRUE(isStrEqualsCStr(
      Tuple::cast(Code::cast(function.code()).names()).at(0), "a"));
  Tuple caches(&scope, function.caches());
  EXPECT_TRUE(icLookupGlobalVar(*caches, 0).isNoneType());
}

TEST_F(InterpreterTest, DeleteGlobalInvalidatesCachedValue) {
  HandleScope scope(thread_);
  EXPECT_FALSE(runFromCStr(&runtime_, R"(
a = 400
def foo():
  return a + a

def bar():
  global a
  del a

foo()
bar()
)")
                   .isError());
  Function function(&scope, mainModuleAt(&runtime_, "foo"));
  ASSERT_TRUE(isStrEqualsCStr(
      Tuple::cast(Code::cast(function.code()).names()).at(0), "a"));
  Tuple caches(&scope, function.caches());
  EXPECT_TRUE(icLookupGlobalVar(*caches, 0).isNoneType());
}

TEST_F(InterpreterTest, StoreNameInvalidatesCachedBuiltinToBeShadowed) {
  HandleScope scope(thread_);
  EXPECT_FALSE(runFromCStr(&runtime_, R"(
__builtins__.a = 400

def foo():
  return a + a

foo()
a = 800
)")
                   .isError());
  Function function(&scope, mainModuleAt(&runtime_, "foo"));
  ASSERT_TRUE(isStrEqualsCStr(
      Tuple::cast(Code::cast(function.code()).names()).at(0), "a"));
  Tuple caches(&scope, function.caches());
  EXPECT_TRUE(icLookupGlobalVar(*caches, 0).isNoneType());
}

TEST_F(InterpreterTest, DeleteNameInvalidatesCachedGlobalVar) {
  HandleScope scope(thread_);
  EXPECT_FALSE(runFromCStr(&runtime_, R"(
a = 400
def foo():
  return a + a

foo()
del a
)")
                   .isError());
  Function function(&scope, mainModuleAt(&runtime_, "foo"));
  ASSERT_TRUE(isStrEqualsCStr(
      Tuple::cast(Code::cast(function.code()).names()).at(0), "a"));
  Tuple caches(&scope, function.caches());
  EXPECT_TRUE(icLookupGlobalVar(*caches, 0).isNoneType());
}

TEST_F(
    InterpreterTest,
    StoreAttrCachedInvalidatesInstanceOffsetCachesByAssigningTypeDescriptor) {
  HandleScope scope(thread_);
  EXPECT_FALSE(runFromCStr(&runtime_, R"(
class C:
  def __init__(self):
    self.foo = 400

def get_foo(c):
  return c.foo

def do_not_invalidate0():
  C.bar = property (lambda self: "data descriptor in a different attr")

def do_not_invalidate1():
  C.foo = 9999

def invalidate():
  C.foo = property (lambda self: "data descriptor")

c = C()
)")
                   .isError());
  Object c(&scope, mainModuleAt(&runtime_, "c"));
  Function get_foo(&scope, mainModuleAt(&runtime_, "get_foo"));
  Function do_not_invalidate0(&scope,
                              mainModuleAt(&runtime_, "do_not_invalidate0"));
  Function do_not_invalidate1(&scope,
                              mainModuleAt(&runtime_, "do_not_invalidate1"));
  Function invalidate(&scope, mainModuleAt(&runtime_, "invalidate"));
  Tuple caches(&scope, get_foo.caches());
  // Load the cache
  ASSERT_TRUE(icLookupAttr(*caches, 1, c.layoutId()).isErrorNotFound());
  ASSERT_TRUE(isIntEqualsWord(
      Interpreter::callFunction1(thread_, thread_->currentFrame(), get_foo, c),
      400));
  ASSERT_TRUE(icLookupAttr(*caches, 1, c.layoutId()).isSmallInt());

  // Assign a data descriptor to a different attribute name.
  ASSERT_TRUE(Interpreter::callFunction0(thread_, thread_->currentFrame(),
                                         do_not_invalidate0)
                  .isNoneType());
  EXPECT_TRUE(icLookupAttr(*caches, 1, c.layoutId()).isSmallInt());

  // Assign a non-data descriptor to the cache's attribute name.
  ASSERT_TRUE(Interpreter::callFunction0(thread_, thread_->currentFrame(),
                                         do_not_invalidate1)
                  .isNoneType());
  EXPECT_TRUE(icLookupAttr(*caches, 1, c.layoutId()).isSmallInt());

  // Assign a data descriptor the cache's attribute name that actually causes
  // invalidation.
  ASSERT_TRUE(
      Interpreter::callFunction0(thread_, thread_->currentFrame(), invalidate)
          .isNoneType());
  // Verify that the cache is empty and calling get_foo() returns a fresh value.
  EXPECT_TRUE(icLookupAttr(*caches, 1, c.layoutId()).isErrorNotFound());
  EXPECT_TRUE(isStrEqualsCStr(
      Interpreter::callFunction1(thread_, thread_->currentFrame(), get_foo, c),
      "data descriptor"));
}

TEST_F(InterpreterTest,
       StoreAttrCachedInvalidatesTypeAttrCachesByUpdatingTypeAttribute) {
  HandleScope scope(thread_);
  EXPECT_FALSE(runFromCStr(&runtime_, R"(
class C:
  def foo(self):
    return 400;

def call_foo(c):
  return c.foo()

def do_not_invalidate():
  C.bar = lambda c: "new type attr"

def invalidate():
  C.foo = lambda c: "new type attr"

old_foo = C.foo
c = C()
)")
                   .isError());
  Object c(&scope, mainModuleAt(&runtime_, "c"));
  Function old_foo(&scope, mainModuleAt(&runtime_, "old_foo"));
  Function call_foo(&scope, mainModuleAt(&runtime_, "call_foo"));
  Function do_not_invalidate(&scope,
                             mainModuleAt(&runtime_, "do_not_invalidate"));
  Function invalidate(&scope, mainModuleAt(&runtime_, "invalidate"));
  Tuple caches(&scope, call_foo.caches());
  // Load the cache
  ASSERT_TRUE(icLookupAttr(*caches, 1, c.layoutId()).isErrorNotFound());
  ASSERT_TRUE(isIntEqualsWord(
      Interpreter::callFunction1(thread_, thread_->currentFrame(), call_foo, c),
      400));
  ASSERT_EQ(icLookupAttr(*caches, 1, c.layoutId()), *old_foo);

  // Assign a non-data descriptor to different attribute name.
  ASSERT_TRUE(Interpreter::callFunction0(thread_, thread_->currentFrame(),
                                         do_not_invalidate)
                  .isNoneType());
  ASSERT_EQ(icLookupAttr(*caches, 1, c.layoutId()), *old_foo);

  // Invalidate the cache.
  ASSERT_TRUE(
      Interpreter::callFunction0(thread_, thread_->currentFrame(), invalidate)
          .isNoneType());
  // Verify that the cache is empty and calling get_foo() returns a fresh value.
  EXPECT_TRUE(icLookupAttr(*caches, 1, c.layoutId()).isErrorNotFound());
  EXPECT_TRUE(isStrEqualsCStr(
      Interpreter::callFunction1(thread_, thread_->currentFrame(), call_foo, c),
      "new type attr"));
}

TEST_F(
    InterpreterTest,
    StoreAttrCachedInvalidatesAttributeCachesByUpdatingMatchingTypeAttributesOfSuperclass) {
  HandleScope scope(thread_);
  EXPECT_FALSE(runFromCStr(&runtime_, R"(
class B:
  pass

class C(B):
  def __init__(self):
    self.foo = 400

class D(C):
  pass

def get_foo(c):
  return c.foo

def do_not_invalidate():
  D.foo = property (lambda self: "data descriptor")

def invalidate():
  B.foo = property (lambda self: "data descriptor")

c = C()
)")
                   .isError());
  Type type_b(&scope, mainModuleAt(&runtime_, "B"));
  Type type_c(&scope, mainModuleAt(&runtime_, "C"));
  Object c(&scope, mainModuleAt(&runtime_, "c"));
  Function get_foo(&scope, mainModuleAt(&runtime_, "get_foo"));
  Function do_not_invalidate(&scope,
                             mainModuleAt(&runtime_, "do_not_invalidate"));
  Function invalidate(&scope, mainModuleAt(&runtime_, "invalidate"));
  Tuple caches(&scope, get_foo.caches());
  // Load the cache.
  ASSERT_TRUE(icLookupAttr(*caches, 1, c.layoutId()).isErrorNotFound());
  ASSERT_TRUE(isIntEqualsWord(
      Interpreter::callFunction1(thread_, thread_->currentFrame(), get_foo, c),
      400));
  ASSERT_TRUE(icLookupAttr(*caches, 1, c.layoutId()).isSmallInt());

  // Updating a subclass' type attribute doesn't invalidate the cache.
  ASSERT_TRUE(Interpreter::callFunction0(thread_, thread_->currentFrame(),
                                         do_not_invalidate)
                  .isNoneType());
  ASSERT_TRUE(icLookupAttr(*caches, 1, c.layoutId()).isSmallInt());

  // Verify that all type dictionaries in C's mro have dependentices to get_foo.
  Dict type_b_dict(&scope, type_b.dict());
  Dict type_c_dict(&scope, type_c.dict());
  Str foo_name(&scope, runtime_.newStrFromCStr("foo"));
  Object result(&scope, dictAtByStr(thread_, type_b_dict, foo_name));
  ASSERT_TRUE(result.isValueCell());
  ASSERT_TRUE(ValueCell::cast(*result).dependencyLink().isWeakLink());
  EXPECT_EQ(
      WeakLink::cast(ValueCell::cast(*result).dependencyLink()).referent(),
      *get_foo);

  result = dictAtByStr(thread_, type_c_dict, foo_name);
  ASSERT_TRUE(result.isValueCell());
  ASSERT_TRUE(ValueCell::cast(*result).dependencyLink().isWeakLink());
  EXPECT_EQ(
      WeakLink::cast(ValueCell::cast(*result).dependencyLink()).referent(),
      *get_foo);

  // Invalidate the cache.
  ASSERT_TRUE(
      Interpreter::callFunction0(thread_, thread_->currentFrame(), invalidate)
          .isNoneType());
  // Verify that the cache is empty and calling get_foo() returns a fresh value.
  EXPECT_TRUE(icLookupAttr(*caches, 1, c.layoutId()).isErrorNotFound());
  EXPECT_TRUE(isStrEqualsCStr(
      Interpreter::callFunction1(thread_, thread_->currentFrame(), get_foo, c),
      "data descriptor"));
}

TEST_F(InterpreterTest, StoreAttrCachedInvalidatesBinaryOpCaches) {
  HandleScope scope(thread_);
  EXPECT_FALSE(runFromCStr(&runtime_, R"(
def cache_A_add(a, b):
  return a + b

class A:
  def __add__(self, other): return "A.__add__"

class B:
  pass

def update_A_add():
  A.__add__ = lambda self, other: "new A.__add__"

a = A()
b = B()

A_add = A.__add__

cache_A_add(a, b)
)")
                   .isError());
  Object a(&scope, mainModuleAt(&runtime_, "a"));
  Object b(&scope, mainModuleAt(&runtime_, "b"));
  Object a_add(&scope, mainModuleAt(&runtime_, "A_add"));

  Function cache_a_add(&scope, mainModuleAt(&runtime_, "cache_A_add"));
  BinaryOpFlags flags_out;
  // Ensure that A.__add__ is cached in cache_A_add.
  Object cached_in_cache_a_add(
      &scope, icLookupBinaryOp(Tuple::cast(cache_a_add.caches()), 0,
                               a.layoutId(), b.layoutId(), &flags_out));
  ASSERT_EQ(cached_in_cache_a_add, *a_add);

  // Ensure that cache_a_add is being tracked as a dependent from A.__add__.
  Dict type_a_dict(&scope, Type::cast(mainModuleAt(&runtime_, "A")).dict());
  ValueCell a_add_value_cell(
      &scope, dictAtById(thread_, type_a_dict, SymbolId::kDunderAdd));
  ASSERT_FALSE(a_add_value_cell.isPlaceholder());
  EXPECT_EQ(WeakLink::cast(a_add_value_cell.dependencyLink()).referent(),
            *cache_a_add);

  // Ensure that cache_a_add is being tracked as a dependent from B.__radd__.
  Dict type_b_dict(&scope, Type::cast(mainModuleAt(&runtime_, "B")).dict());
  ValueCell b_radd_value_cell(
      &scope, dictAtById(thread_, type_b_dict, SymbolId::kDunderRadd));
  ASSERT_TRUE(b_radd_value_cell.isPlaceholder());
  EXPECT_EQ(WeakLink::cast(b_radd_value_cell.dependencyLink()).referent(),
            *cache_a_add);

  // Updating A.__add__ invalidates the cache.
  Function invalidate(&scope, mainModuleAt(&runtime_, "update_A_add"));
  ASSERT_TRUE(
      Interpreter::callFunction0(thread_, thread_->currentFrame(), invalidate)
          .isNoneType());
  // Verify that the cache is evicted.
  EXPECT_TRUE(icLookupBinaryOp(Tuple::cast(cache_a_add.caches()), 0,
                               a.layoutId(), b.layoutId(), &flags_out)
                  .isErrorNotFound());
  // Verify that the dependencies are deleted.
  EXPECT_TRUE(a_add_value_cell.dependencyLink().isNoneType());
  EXPECT_TRUE(b_radd_value_cell.dependencyLink().isNoneType());
}

TEST_F(InterpreterTest, StoreAttrCachedInvalidatesCompareOpTypeAttrCaches) {
  HandleScope scope(thread_);
  EXPECT_FALSE(runFromCStr(&runtime_, R"(
def cache_compare_op(a, b):
  return a >= b

class A:
  def __le__(self, other): return True

  def __ge__(self, other): return True

class B:
  def __le__(self, other): return True

  def __ge__(self, other): return True

def do_not_invalidate():
  A.__le__ = lambda self, other: False
  B.__ge__ = lambda self, other: False

def invalidate():
  A.__ge__ = lambda self, other: False

a = A()
b = B()
A__ge__ = A.__ge__
c = cache_compare_op(a, b)
)")
                   .isError());
  Object a(&scope, mainModuleAt(&runtime_, "a"));
  Object b(&scope, mainModuleAt(&runtime_, "b"));
  Object type_a__dunder_ge(&scope, mainModuleAt(&runtime_, "A__ge__"));

  // Ensure that A.__ge__ is cached.
  Function cache_compare_op(&scope,
                            mainModuleAt(&runtime_, "cache_compare_op"));
  Tuple caches(&scope, cache_compare_op.caches());
  BinaryOpFlags flags_out;
  Object cached(&scope, icLookupBinaryOp(*caches, 0, a.layoutId(), b.layoutId(),
                                         &flags_out));
  ASSERT_EQ(*cached, *type_a__dunder_ge);

  // Updating irrelevant compare op dunder functions doesn't trigger
  // invalidation.
  Function do_not_invalidate(&scope,
                             mainModuleAt(&runtime_, "do_not_invalidate"));
  ASSERT_TRUE(Interpreter::callFunction0(thread_, thread_->currentFrame(),
                                         do_not_invalidate)
                  .isNoneType());
  cached = icLookupBinaryOp(*caches, 0, a.layoutId(), b.layoutId(), &flags_out);
  EXPECT_EQ(*cached, *type_a__dunder_ge);

  // Updating relevant compare op dunder functions triggers invalidation.
  Function invalidate(&scope, mainModuleAt(&runtime_, "invalidate"));
  ASSERT_TRUE(
      Interpreter::callFunction0(thread_, thread_->currentFrame(), invalidate)
          .isNoneType());
  ASSERT_TRUE(
      icLookupBinaryOp(*caches, 0, a.layoutId(), b.layoutId(), &flags_out)
          .isErrorNotFound());
}

TEST_F(InterpreterTest, StoreAttrCachedInvalidatesInplaceOpCaches) {
  HandleScope scope(thread_);
  EXPECT_FALSE(runFromCStr(&runtime_, R"(
def cache_A_iadd(a, b):
  a += b

class A:
  def __iadd__(self, other): return "A.__iadd__"

class B:
  pass

def update_A_iadd():
  A.__iadd__ = lambda self, other: "new A.__add__"

a = A()
b = B()

A_iadd = A.__iadd__

cache_A_iadd(a, b)
)")
                   .isError());
  Object a(&scope, mainModuleAt(&runtime_, "a"));
  Object b(&scope, mainModuleAt(&runtime_, "b"));
  Object a_iadd(&scope, mainModuleAt(&runtime_, "A_iadd"));

  Function cache_a_iadd(&scope, mainModuleAt(&runtime_, "cache_A_iadd"));
  BinaryOpFlags flags_out;
  // Ensure that A.__iadd__ is cached in cache_A_iadd.
  Object cached_in_cache_a_iadd(
      &scope, icLookupBinaryOp(Tuple::cast(cache_a_iadd.caches()), 0,
                               a.layoutId(), b.layoutId(), &flags_out));
  ASSERT_EQ(cached_in_cache_a_iadd, *a_iadd);

  // Ensure that cache_a_iadd is being tracked as a dependent from A.__iadd__.
  Dict type_a_dict(&scope, Type::cast(mainModuleAt(&runtime_, "A")).dict());
  ValueCell a_iadd_value_cell(
      &scope, dictAtById(thread_, type_a_dict, SymbolId::kDunderIadd));
  ASSERT_FALSE(a_iadd_value_cell.isPlaceholder());
  EXPECT_EQ(WeakLink::cast(a_iadd_value_cell.dependencyLink()).referent(),
            *cache_a_iadd);

  ValueCell a_add_value_cell(
      &scope, dictAtById(thread_, type_a_dict, SymbolId::kDunderAdd));
  ASSERT_TRUE(a_add_value_cell.isPlaceholder());
  EXPECT_EQ(WeakLink::cast(a_add_value_cell.dependencyLink()).referent(),
            *cache_a_iadd);

  // Ensure that cache_a_iadd is being tracked as a dependent from B.__riadd__.
  Dict type_b_dict(&scope, Type::cast(mainModuleAt(&runtime_, "B")).dict());
  ValueCell b_radd_value_cell(
      &scope, dictAtById(thread_, type_b_dict, SymbolId::kDunderRadd));
  ASSERT_TRUE(b_radd_value_cell.isPlaceholder());
  EXPECT_EQ(WeakLink::cast(b_radd_value_cell.dependencyLink()).referent(),
            *cache_a_iadd);

  // Updating A.__iadd__ invalidates the cache.
  Function invalidate(&scope, mainModuleAt(&runtime_, "update_A_iadd"));
  ASSERT_TRUE(
      Interpreter::callFunction0(thread_, thread_->currentFrame(), invalidate)
          .isNoneType());
  // Verify that the cache is evicted.
  EXPECT_TRUE(icLookupBinaryOp(Tuple::cast(cache_a_iadd.caches()), 0,
                               a.layoutId(), b.layoutId(), &flags_out)
                  .isErrorNotFound());
  // Verify that the dependencies are deleted.
  EXPECT_TRUE(a_iadd_value_cell.dependencyLink().isNoneType());
  EXPECT_TRUE(a_add_value_cell.dependencyLink().isNoneType());
  EXPECT_TRUE(b_radd_value_cell.dependencyLink().isNoneType());
}

TEST_F(InterpreterTest, LoadMethodLoadingMethodFollowedByCallMethod) {
  HandleScope scope(thread_);
  EXPECT_FALSE(runFromCStr(&runtime_, R"(
class C:
  def __init__(self):
    self.val = 40

  def compute(self, arg0, arg1):
    return self.val + arg0 + arg1

def test():
  return c.compute(10, 20)

c = C()
)")
                   .isError());
  Function test_function(&scope, mainModuleAt(&runtime_, "test"));
  MutableBytes bytecode(&scope, test_function.rewrittenBytecode());
  ASSERT_EQ(bytecode.byteAt(2), LOAD_ATTR_CACHED);
  ASSERT_EQ(bytecode.byteAt(8), CALL_FUNCTION);
  bytecode.byteAtPut(2, LOAD_METHOD);
  bytecode.byteAtPut(8, CALL_METHOD);

  EXPECT_TRUE(
      isIntEqualsWord(Interpreter::callFunction0(
                          thread_, thread_->currentFrame(), test_function),
                      70));
}

TEST_F(InterpreterTest,
       LoadMethodCachedCachingNonFunctionFollowedByCallMethod) {
  HandleScope scope(thread_);
  EXPECT_FALSE(runFromCStr(&runtime_, R"(
class C:
  def __init__(self):
    self.val = 40

def foo(a, b): return a + b
c = C()
c.compute = foo
def test():
  return c.compute(10, 20)
)")
                   .isError());
  Function test_function(&scope, mainModuleAt(&runtime_, "test"));
  MutableBytes bytecode(&scope, test_function.rewrittenBytecode());
  ASSERT_EQ(bytecode.byteAt(2), LOAD_ATTR_CACHED);
  ASSERT_EQ(bytecode.byteAt(8), CALL_FUNCTION);
  bytecode.byteAtPut(2, LOAD_METHOD_CACHED);
  bytecode.byteAtPut(8, CALL_METHOD);

  Object c(&scope, mainModuleAt(&runtime_, "c"));
  LayoutId layout_id = c.layoutId();
  Tuple caches(&scope, test_function.caches());
  // Cache miss.
  ASSERT_TRUE(
      icLookupAttr(*caches, bytecode.byteAt(3), layout_id).isErrorNotFound());
  EXPECT_TRUE(
      isIntEqualsWord(Interpreter::callFunction0(
                          thread_, thread_->currentFrame(), test_function),
                      30));

  // Cache hit.
  ASSERT_TRUE(
      icLookupAttr(*caches, bytecode.byteAt(3), layout_id).isSmallInt());
  EXPECT_TRUE(
      isIntEqualsWord(Interpreter::callFunction0(
                          thread_, thread_->currentFrame(), test_function),
                      30));
}

TEST_F(InterpreterTest, LoadMethodCachedCachingFunctionFollowedByCallMethod) {
  HandleScope scope(thread_);
  EXPECT_FALSE(runFromCStr(&runtime_, R"(
class C:
  def __init__(self):
    self.val = 40

  def compute(self, arg0, arg1):
    return self.val + arg0 + arg1

def test():
  return c.compute(10, 20)

c = C()
)")
                   .isError());
  Function test_function(&scope, mainModuleAt(&runtime_, "test"));
  MutableBytes bytecode(&scope, test_function.rewrittenBytecode());
  ASSERT_EQ(bytecode.byteAt(2), LOAD_ATTR_CACHED);
  ASSERT_EQ(bytecode.byteAt(8), CALL_FUNCTION);
  bytecode.byteAtPut(2, LOAD_METHOD_CACHED);
  bytecode.byteAtPut(8, CALL_METHOD);

  // Cache miss.
  Object c(&scope, mainModuleAt(&runtime_, "c"));
  LayoutId layout_id = c.layoutId();
  Tuple caches(&scope, test_function.caches());
  ASSERT_TRUE(
      icLookupAttr(*caches, bytecode.byteAt(3), layout_id).isErrorNotFound());
  EXPECT_TRUE(
      isIntEqualsWord(Interpreter::callFunction0(
                          thread_, thread_->currentFrame(), test_function),
                      70));

  // Cache hit.
  ASSERT_TRUE(
      icLookupAttr(*caches, bytecode.byteAt(3), layout_id).isFunction());
  EXPECT_TRUE(
      isIntEqualsWord(Interpreter::callFunction0(
                          thread_, thread_->currentFrame(), test_function),
                      70));
}

TEST_F(InterpreterTest, DoLoadImmediate) {
  HandleScope scope(thread_);
  EXPECT_FALSE(runFromCStr(&runtime_, R"(
def test():
  return None

result = test()
)")
                   .isError());
  Function test_function(&scope, mainModuleAt(&runtime_, "test"));
  MutableBytes bytecode(&scope, test_function.rewrittenBytecode());
  // Verify that rewriting replaces LOAD_CONST for LOAD_IMMEDIATE.
  EXPECT_EQ(bytecode.byteAt(0), LOAD_IMMEDIATE);
  EXPECT_EQ(bytecode.byteAt(1), static_cast<byte>(NoneType::object().raw()));
  EXPECT_TRUE(mainModuleAt(&runtime_, "result").isNoneType());
}

TEST_F(InterpreterTest, LoadAttrCachedInsertsExecutingFunctionAsDependent) {
  EXPECT_FALSE(runFromCStr(&runtime_, R"(
class C:
  def __init__(self):
    self.foo = 400

def cache_attribute(c):
  return c.foo

c = C()
)")
                   .isError());
  HandleScope scope(thread_);
  Type type_c(&scope, mainModuleAt(&runtime_, "C"));
  Object c(&scope, mainModuleAt(&runtime_, "c"));
  Function cache_attribute(&scope, mainModuleAt(&runtime_, "cache_attribute"));
  Tuple caches(&scope, cache_attribute.caches());
  ASSERT_EQ(caches.length(), 2 * kIcPointersPerCache);

  // Load the cache.
  ASSERT_TRUE(icLookupAttr(*caches, 1, c.layoutId()).isErrorNotFound());
  ASSERT_TRUE(
      isIntEqualsWord(Interpreter::callFunction1(
                          thread_, thread_->currentFrame(), cache_attribute, c),
                      400));
  ASSERT_TRUE(icLookupAttr(*caches, 1, c.layoutId()).isSmallInt());

  // Verify that cache_attribute function is added as a dependent.
  Dict type_c_dict(&scope, type_c.dict());
  Str foo_name(&scope, runtime_.newStrFromCStr("foo"));
  ValueCell value_cell(&scope, dictAtByStr(thread_, type_c_dict, foo_name));
  ASSERT_TRUE(value_cell.dependencyLink().isWeakLink());
  EXPECT_EQ(WeakLink::cast(value_cell.dependencyLink()).referent(),
            *cache_attribute);
}

TEST_F(InterpreterTest, StoreAttrCachedInsertsExecutingFunctionAsDependent) {
  EXPECT_FALSE(runFromCStr(&runtime_, R"(
class C:
  def __init__(self):
    self.foo = 400

def cache_attribute(c):
  c.foo = 500

c = C()
)")
                   .isError());
  HandleScope scope(thread_);
  Type type_c(&scope, mainModuleAt(&runtime_, "C"));
  Object c(&scope, mainModuleAt(&runtime_, "c"));
  Function cache_attribute(&scope, mainModuleAt(&runtime_, "cache_attribute"));
  Tuple caches(&scope, cache_attribute.caches());
  ASSERT_EQ(caches.length(), 2 * kIcPointersPerCache);

  // Load the cache.
  ASSERT_TRUE(icLookupAttr(*caches, 1, c.layoutId()).isErrorNotFound());
  ASSERT_TRUE(Interpreter::callFunction1(thread_, thread_->currentFrame(),
                                         cache_attribute, c)
                  .isNoneType());
  ASSERT_TRUE(icLookupAttr(*caches, 1, c.layoutId()).isSmallInt());

  // Verify that cache_attribute function is added as a dependent.
  Dict type_c_dict(&scope, type_c.dict());
  Str foo_name(&scope, runtime_.newStrFromCStr("foo"));
  ValueCell value_cell(&scope, dictAtByStr(thread_, type_c_dict, foo_name));
  ASSERT_TRUE(value_cell.dependencyLink().isWeakLink());
  EXPECT_EQ(WeakLink::cast(value_cell.dependencyLink()).referent(),
            *cache_attribute);
}

TEST_F(InterpreterTest, StoreAttrsCausingShadowingInvalidatesCache) {
  EXPECT_FALSE(runFromCStr(&runtime_, R"(
class A:
  def foo(self): return 40

class B(A):
  def foo(self): return 50

class C(B):
  pass

def function_that_caches_attr_lookup(a, b, c):
  return a.foo() + b.foo() + c.foo()

def func_that_causes_shadowing_of_attr_a():
  A.foo = lambda self: 300

def func_that_causes_shadowing_of_attr_b():
  B.foo = lambda self: 200


# Caching A.foo and B.foo in cache_attribute.
a = A()
b = B()
c = C()
a_foo = A.foo
b_foo = B.foo
function_that_caches_attr_lookup(a, b, c)
)")
                   .isError());
  HandleScope scope(thread_);
  Type type_a(&scope, mainModuleAt(&runtime_, "A"));
  Type type_b(&scope, mainModuleAt(&runtime_, "B"));
  Object a(&scope, mainModuleAt(&runtime_, "a"));
  Object b(&scope, mainModuleAt(&runtime_, "b"));
  Object c(&scope, mainModuleAt(&runtime_, "c"));
  Function function_that_caches_attr_lookup(
      &scope, mainModuleAt(&runtime_, "function_that_caches_attr_lookup"));
  Tuple caches(&scope, function_that_caches_attr_lookup.caches());
  // 0: global variable
  // 1: a.foo
  // 2: b.foo
  // 3: binary op cache
  // 4: c.foo
  // 5, binary op cache
  Function a_foo(&scope, mainModuleAt(&runtime_, "a_foo"));
  Function b_foo(&scope, mainModuleAt(&runtime_, "b_foo"));
  ASSERT_EQ(caches.length(), 6 * kIcPointersPerCache);
  ASSERT_EQ(icLookupAttr(*caches, 1, a.layoutId()), *a_foo);
  ASSERT_EQ(icLookupAttr(*caches, 2, b.layoutId()), *b_foo);
  ASSERT_EQ(icLookupAttr(*caches, 4, c.layoutId()), *b_foo);

  // Verify that function_that_caches_attr_lookup cached the attribute lookup
  // and appears on the dependency list of A.foo.
  Dict type_a_dict(&scope, type_a.dict());
  Str foo_name(&scope, runtime_.newStrFromCStr("foo"));
  ValueCell foo_in_a(&scope, dictAtByStr(thread_, type_a_dict, foo_name));
  ASSERT_TRUE(foo_in_a.dependencyLink().isWeakLink());
  ASSERT_EQ(WeakLink::cast(foo_in_a.dependencyLink()).referent(),
            *function_that_caches_attr_lookup);

  // Verify that function_that_caches_attr_lookup cached the attribute lookup
  // and appears on the dependency list of B.foo.
  Dict type_b_dict(&scope, type_b.dict());
  ValueCell foo_in_b(&scope, dictAtByStr(thread_, type_b_dict, foo_name));
  ASSERT_TRUE(foo_in_b.dependencyLink().isWeakLink());
  ASSERT_EQ(WeakLink::cast(foo_in_b.dependencyLink()).referent(),
            *function_that_caches_attr_lookup);

  // Verify that function_that_caches_attr_lookup cached the attribute lookup
  // and appears on the dependency list of C.foo.
  Dict type_c_dict(&scope, type_b.dict());
  ValueCell foo_in_c(&scope, dictAtByStr(thread_, type_c_dict, foo_name));
  ASSERT_TRUE(foo_in_c.dependencyLink().isWeakLink());
  ASSERT_EQ(WeakLink::cast(foo_in_c.dependencyLink()).referent(),
            *function_that_caches_attr_lookup);

  // Change the class A so that any caches that reference A.foo are invalidated.
  Function func_that_causes_shadowing_of_attr_a(
      &scope, mainModuleAt(&runtime_, "func_that_causes_shadowing_of_attr_a"));
  ASSERT_TRUE(Interpreter::callFunction0(thread_, thread_->currentFrame(),
                                         func_that_causes_shadowing_of_attr_a)
                  .isNoneType());
  // Verify that the cache for A.foo is cleared out, and dependent does not
  // depend on A.foo anymore.
  EXPECT_TRUE(icLookupAttr(*caches, 1, a.layoutId()).isErrorNotFound());
  EXPECT_TRUE(foo_in_a.dependencyLink().isNoneType());
  // Check that any lookups of B have not been invalidated.
  EXPECT_EQ(icLookupAttr(*caches, 2, b.layoutId()), *b_foo);
  EXPECT_EQ(WeakLink::cast(foo_in_b.dependencyLink()).referent(),
            *function_that_caches_attr_lookup);
  // Check that any lookups of C have not been invalidated.
  EXPECT_EQ(icLookupAttr(*caches, 4, c.layoutId()), *b_foo);
  EXPECT_EQ(WeakLink::cast(foo_in_c.dependencyLink()).referent(),
            *function_that_caches_attr_lookup);

  // Invalidate the cache for B.foo.
  Function func_that_causes_shadowing_of_attr_b(
      &scope, mainModuleAt(&runtime_, "func_that_causes_shadowing_of_attr_b"));
  ASSERT_TRUE(Interpreter::callFunction0(thread_, thread_->currentFrame(),
                                         func_that_causes_shadowing_of_attr_b)
                  .isNoneType());
  // Check that caches for A are still invalidated.
  EXPECT_TRUE(icLookupAttr(*caches, 1, a.layoutId()).isErrorNotFound());
  EXPECT_TRUE(foo_in_a.dependencyLink().isNoneType());
  // Check that caches for B and C got just invalidated since they refer to
  // B.foo.
  EXPECT_TRUE(icLookupAttr(*caches, 2, b.layoutId()).isErrorNotFound());
  EXPECT_TRUE(foo_in_b.dependencyLink().isNoneType());
  EXPECT_TRUE(icLookupAttr(*caches, 4, c.layoutId()).isErrorNotFound());
  EXPECT_TRUE(foo_in_c.dependencyLink().isNoneType());
}

TEST_F(InterpreterTest, DoIntrinsicWithSlowPathDoesNotAlterStack) {
  HandleScope scope(thread_);
  Object obj(&scope, runtime_.newList());
  Frame* frame = thread_->currentFrame();
  frame->pushValue(*obj);
  ASSERT_FALSE(doIntrinsic(thread_, frame, SymbolId::kUnderTupleLen));
  EXPECT_EQ(frame->peek(0), *obj);
}

}  // namespace py
