#include "interpreter.h"

#include <memory>

#include "gtest/gtest.h"

#include "builtins-module.h"
#include "bytecode.h"
#include "dict-builtins.h"
#include "handles.h"
#include "ic.h"
#include "list-builtins.h"
#include "module-builtins.h"
#include "modules.h"
#include "object-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "str-builtins.h"
#include "test-utils.h"
#include "trampolines.h"
#include "type-builtins.h"

namespace py {
namespace testing {

using InterpreterDeathTest = RuntimeFixture;
using InterpreterTest = RuntimeFixture;

TEST_F(InterpreterTest, IsTrueBool) {
  HandleScope scope(thread_);

  Object true_value(&scope, Bool::trueObj());
  EXPECT_EQ(Interpreter::isTrue(thread_, *true_value), Bool::trueObj());

  Object false_object(&scope, Bool::falseObj());
  EXPECT_EQ(Interpreter::isTrue(thread_, *false_object), Bool::falseObj());
}

TEST_F(InterpreterTest, IsTrueInt) {
  HandleScope scope(thread_);

  Object true_value(&scope, runtime_->newInt(1234));
  EXPECT_EQ(Interpreter::isTrue(thread_, *true_value), Bool::trueObj());

  Object false_value(&scope, runtime_->newInt(0));
  EXPECT_EQ(Interpreter::isTrue(thread_, *false_value), Bool::falseObj());
}

TEST_F(InterpreterTest, IsTrueWithDunderBoolRaisingPropagatesException) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class Foo:
  def __bool__(self):
    raise UserWarning('')
value = Foo()
)")
                   .isError());
  Object value(&scope, mainModuleAt(runtime_, "value"));
  Object result(&scope, Interpreter::isTrue(thread_, *value));
  EXPECT_TRUE(raised(*result, LayoutId::kUserWarning));
}

TEST_F(InterpreterTest, IsTrueWithDunderLenRaisingPropagatesException) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class Foo:
  def __len__(self):
    raise UserWarning('')
value = Foo()
)")
                   .isError());
  Object value(&scope, mainModuleAt(runtime_, "value"));
  Object result(&scope, Interpreter::isTrue(thread_, *value));
  EXPECT_TRUE(raised(*result, LayoutId::kUserWarning));
}

TEST_F(InterpreterTest, IsTrueWithIntSubclassDunderLenUsesBaseInt) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
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
  Object true_value(&scope, mainModuleAt(runtime_, "true_value"));
  Object false_value(&scope, mainModuleAt(runtime_, "false_value"));
  EXPECT_EQ(Interpreter::isTrue(thread_, *true_value), Bool::trueObj());
  EXPECT_EQ(Interpreter::isTrue(thread_, *false_value), Bool::falseObj());
}

TEST_F(InterpreterTest, IsTrueDunderLen) {
  HandleScope scope(thread_);

  List nonempty_list(&scope, runtime_->newList());
  Object elt(&scope, NoneType::object());
  runtime_->listAdd(thread_, nonempty_list, elt);

  EXPECT_EQ(Interpreter::isTrue(thread_, *nonempty_list), Bool::trueObj());

  List empty_list(&scope, runtime_->newList());
  EXPECT_EQ(Interpreter::isTrue(thread_, *empty_list), Bool::falseObj());
}

TEST_F(InterpreterTest, UnaryOperationWithIntReturnsInt) {
  HandleScope scope(thread_);
  Object value(&scope, runtime_->newInt(23));
  Object result(&scope,
                Interpreter::unaryOperation(thread_, value, ID(__pos__)));
  EXPECT_TRUE(isIntEqualsWord(*result, 23));
}

TEST_F(InterpreterTest, UnaryOperationWithBadTypeRaisesTypeError) {
  HandleScope scope(thread_);
  Object value(&scope, NoneType::object());
  Object result(&scope,
                Interpreter::unaryOperation(thread_, value, ID(__invert__)));
  EXPECT_TRUE(
      raisedWithStr(*result, LayoutId::kTypeError,
                    "bad operand type for unary '__invert__': 'NoneType'"));
}

TEST_F(InterpreterTest, UnaryOperationWithCustomDunderInvertReturnsString) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  def __invert__(self):
    return "custom invert"
c = C()
)")
                   .isError());
  Object c(&scope, mainModuleAt(runtime_, "c"));
  Object result(&scope,
                Interpreter::unaryOperation(thread_, c, ID(__invert__)));
  EXPECT_TRUE(isStrEqualsCStr(*result, "custom invert"));
}

TEST_F(InterpreterTest, UnaryOperationWithCustomRaisingDunderNegPropagates) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  def __neg__(self):
    raise UserWarning('')
c = C()
)")
                   .isError());
  Object c(&scope, mainModuleAt(runtime_, "c"));
  Object result(&scope, Interpreter::unaryOperation(thread_, c, ID(__neg__)));
  EXPECT_TRUE(raised(*result, LayoutId::kUserWarning));
}

TEST_F(InterpreterTest, UnaryNotWithRaisingDunderBool) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
class C:
  def __bool__(self):
    raise RuntimeError("too cool for bool")

not C()
)"),
                            LayoutId::kRuntimeError, "too cool for bool"));
}

TEST_F(InterpreterTest, BinaryOpCachedInsertsDependencyForBothOperandsTypes) {
  HandleScope scope(thread_);
  EXPECT_FALSE(runFromCStr(runtime_, R"(
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
      isStrEqualsCStr(mainModuleAt(runtime_, "result"), "from class A"));

  Function cache_binary_op(&scope, mainModuleAt(runtime_, "cache_binary_op"));
  MutableTuple caches(&scope, cache_binary_op.caches());
  Object a(&scope, mainModuleAt(runtime_, "a"));
  Object b(&scope, mainModuleAt(runtime_, "b"));
  Type type_a(&scope, mainModuleAt(runtime_, "A"));
  Type type_b(&scope, mainModuleAt(runtime_, "B"));
  BinaryOpFlags flag;
  ASSERT_EQ(icLookupBinaryOp(*caches, 0, a.layoutId(), b.layoutId(), &flag),
            mainModuleAt(runtime_, "A__add__"));

  // Verify that A.__add__ has the dependent.
  Object left_op_name(&scope, runtime_->symbols()->at(ID(__add__)));
  Object type_a_attr(&scope, typeValueCellAt(type_a, left_op_name));
  ASSERT_TRUE(type_a_attr.isValueCell());
  ASSERT_TRUE(ValueCell::cast(*type_a_attr).dependencyLink().isWeakLink());
  EXPECT_EQ(
      WeakLink::cast(ValueCell::cast(*type_a_attr).dependencyLink()).referent(),
      *cache_binary_op);

  // Verify that B.__radd__ has the dependent.
  Object right_op_name(&scope, runtime_->symbols()->at(ID(__radd__)));
  Object type_b_attr(&scope, typeValueCellAt(type_b, right_op_name));
  ASSERT_TRUE(type_b_attr.isValueCell());
  ASSERT_TRUE(ValueCell::cast(*type_b_attr).dependencyLink().isWeakLink());
  EXPECT_EQ(
      WeakLink::cast(ValueCell::cast(*type_b_attr).dependencyLink()).referent(),
      *cache_binary_op);
}

TEST_F(InterpreterTest, BinaryOpInvokesSelfMethod) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
    def __sub__(self, other):
        return (C, '__sub__', self, other)

left = C()
right = C()
)")
                   .isError());

  Object left(&scope, mainModuleAt(runtime_, "left"));
  Object right(&scope, mainModuleAt(runtime_, "right"));
  Object c_class(&scope, mainModuleAt(runtime_, "C"));

  Object result_obj(
      &scope, Interpreter::binaryOperation(thread_, Interpreter::BinaryOp::SUB,
                                           left, right));
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

  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
    def __sub__(self, other):
        return (C, '__sub__', self, other)
    def __rsub__(self, other):
        return (C, '__rsub__', self, other)

left = C()
right = C()
)")
                   .isError());

  Object left(&scope, mainModuleAt(runtime_, "left"));
  Object right(&scope, mainModuleAt(runtime_, "right"));
  Object c_class(&scope, mainModuleAt(runtime_, "C"));

  Object result_obj(
      &scope, Interpreter::binaryOperation(thread_, Interpreter::BinaryOp::SUB,
                                           left, right));
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

  ASSERT_FALSE(runFromCStr(runtime_, R"(
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

  Object left(&scope, mainModuleAt(runtime_, "left"));
  Object right(&scope, mainModuleAt(runtime_, "right"));
  Object d_class(&scope, mainModuleAt(runtime_, "D"));

  Object result_obj(
      &scope, Interpreter::binaryOperation(thread_, Interpreter::BinaryOp::SUB,
                                           left, right));
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

  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
    pass

class D:
    def __rsub__(self, other):
        return (D, '__rsub__', self, other)

left = C()
right = D()
)")
                   .isError());

  Object left(&scope, mainModuleAt(runtime_, "left"));
  Object right(&scope, mainModuleAt(runtime_, "right"));
  Object d_class(&scope, mainModuleAt(runtime_, "D"));

  Object result_obj(
      &scope, Interpreter::binaryOperation(thread_, Interpreter::BinaryOp::SUB,
                                           left, right));
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
  ASSERT_FALSE(runFromCStr(runtime_, R"(
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

  EXPECT_TRUE(isStrEqualsCStr(mainModuleAt(runtime_, "result"), "C.__add__"));
  EXPECT_TRUE(isStrEqualsCStr(mainModuleAt(runtime_, "trace"),
                              "D.__radd__,C.__add__,"));
}

TEST_F(InterpreterTest, BinaryOperationLookupPropagatesException) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class RaisingDescriptor:
  def __get__(self, obj, type):
    raise UserWarning()
class A:
  __mul__ = RaisingDescriptor()
a = A()
)")
                   .isError());
  Object a(&scope, mainModuleAt(runtime_, "a"));
  Object result(&scope, Interpreter::binaryOperation(
                            thread_, Interpreter::BinaryOp::MUL, a, a));
  EXPECT_TRUE(raised(*result, LayoutId::kUserWarning));
}

TEST_F(InterpreterTest,
       BinaryOperationLookupReflectedMethodPropagatesException) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
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
  Object a(&scope, mainModuleAt(runtime_, "a"));
  Object b(&scope, mainModuleAt(runtime_, "b"));
  Object result(&scope, Interpreter::binaryOperation(
                            thread_, Interpreter::BinaryOp::MUL, a, b));
  EXPECT_TRUE(raised(*result, LayoutId::kUserWarning));
}

TEST_F(InterpreterTest, BinaryOperationSetMethodSetsMethod) {
  HandleScope scope(thread_);
  Object v0(&scope, runtime_->newInt(13));
  Object v1(&scope, runtime_->newInt(42));
  Object method(&scope, NoneType::object());
  BinaryOpFlags flags;
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::binaryOperationSetMethod(thread_, Interpreter::BinaryOp::SUB,
                                            v0, v1, &method, &flags),
      -29));
  EXPECT_TRUE(method.isFunction());
  EXPECT_EQ(flags, kBinaryOpNotImplementedRetry);

  Object v2(&scope, runtime_->newInt(3));
  Object v3(&scope, runtime_->newInt(8));
  ASSERT_EQ(v0.layoutId(), v2.layoutId());
  ASSERT_EQ(v1.layoutId(), v3.layoutId());
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::binaryOperationWithMethod(thread_, *method, flags, *v2, *v3),
      -5));
}

TEST_F(InterpreterTest,
       BinaryOperationSetMethodSetsReflectedMethodNotImplementedRetry) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
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
  Object v0(&scope, mainModuleAt(runtime_, "v0"));
  Object v1(&scope, mainModuleAt(runtime_, "v1"));
  Object v2(&scope, mainModuleAt(runtime_, "v2"));
  Object v3(&scope, mainModuleAt(runtime_, "v3"));

  Object method(&scope, NoneType::object());
  BinaryOpFlags flags;
  Object result_obj(&scope, Interpreter::binaryOperationSetMethod(
                                thread_, Interpreter::BinaryOp::SUB, v0, v1,
                                &method, &flags));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 2);
  EXPECT_EQ(result.at(0), v1);
  EXPECT_EQ(result.at(1), v0);
  EXPECT_TRUE(method.isFunction());
  EXPECT_EQ(flags, kBinaryOpReflected | kBinaryOpNotImplementedRetry);

  ASSERT_EQ(v0.layoutId(), v2.layoutId());
  ASSERT_EQ(v1.layoutId(), v3.layoutId());
  result_obj =
      Interpreter::binaryOperationWithMethod(thread_, *method, flags, *v2, *v3);
  ASSERT_TRUE(result.isTuple());
  result = *result_obj;
  ASSERT_EQ(result.length(), 2);
  EXPECT_EQ(result.at(0), v3);
  EXPECT_EQ(result.at(1), v2);
}

TEST_F(InterpreterTest, BinaryOperationSetMethodSetsReflectedMethod) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
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
  Object v0(&scope, mainModuleAt(runtime_, "v0"));
  Object v1(&scope, mainModuleAt(runtime_, "v1"));
  Object v2(&scope, mainModuleAt(runtime_, "v2"));
  Object v3(&scope, mainModuleAt(runtime_, "v3"));

  Object method(&scope, NoneType::object());
  BinaryOpFlags flags;
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::binaryOperationSetMethod(thread_, Interpreter::BinaryOp::SUB,
                                            v0, v1, &method, &flags),
      -12));
  EXPECT_TRUE(method.isFunction());
  EXPECT_EQ(flags, kBinaryOpReflected);

  ASSERT_EQ(v0.layoutId(), v2.layoutId());
  ASSERT_EQ(v1.layoutId(), v3.layoutId());
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::binaryOperationWithMethod(thread_, *method, flags, *v2, *v3),
      45));
}

TEST_F(InterpreterTest, BinaryOperationSetMethodSetsMethodNotImplementedRetry) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
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
  Object v0(&scope, mainModuleAt(runtime_, "v0"));
  Object v1(&scope, mainModuleAt(runtime_, "v1"));
  Object v2(&scope, mainModuleAt(runtime_, "v2"));
  Object v3(&scope, mainModuleAt(runtime_, "v3"));

  Object method(&scope, NoneType::object());
  BinaryOpFlags flags;
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::binaryOperationSetMethod(thread_, Interpreter::BinaryOp::SUB,
                                            v0, v1, &method, &flags),
      2));
  EXPECT_TRUE(method.isFunction());
  EXPECT_EQ(flags, kBinaryOpNotImplementedRetry);

  ASSERT_EQ(v0.layoutId(), v2.layoutId());
  ASSERT_EQ(v1.layoutId(), v3.layoutId());
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::binaryOperationWithMethod(thread_, *method, flags, *v2, *v3),
      -8));
}

TEST_F(InterpreterTest, DoBinaryOpWithCacheHitCallsCachedMethod) {
  HandleScope scope(thread_);

  word left = SmallInt::kMaxValue + 1;
  word right = -13;
  Code code(&scope, newEmptyCode());
  Object left_obj(&scope, runtime_->newInt(left));
  Object right_obj(&scope, runtime_->newInt(right));
  Tuple consts(&scope, runtime_->newTupleWith2(left_obj, right_obj));
  code.setConsts(*consts);
  const byte bytecode[] = {
      LOAD_CONST, 0, LOAD_CONST, 1, BINARY_SUBTRACT, 0, RETURN_VALUE, 0,
  };
  code.setCode(runtime_->newBytesWithAll(bytecode));

  Object qualname(&scope, Str::empty());
  Module module(&scope, runtime_->findOrCreateMainModule());
  Function function(
      &scope, runtime_->newFunctionWithCode(thread_, qualname, code, module));

  // Update inline cache.
  EXPECT_TRUE(
      isIntEqualsWord(Interpreter::call0(thread_, function), left - right));

  ASSERT_TRUE(function.caches().isTuple());
  MutableTuple caches(&scope, function.caches());
  BinaryOpFlags dummy;
  ASSERT_FALSE(icLookupBinaryOp(*caches, 0, LayoutId::kLargeInt,
                                LayoutId::kSmallInt, &dummy)
                   .isErrorNotFound());

  // Call from inline cache.
  EXPECT_TRUE(
      isIntEqualsWord(Interpreter::call0(thread_, function), left - right));
}

TEST_F(InterpreterTest, DoBinaryOpWithCacheHitCallsRetry) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class MyInt(int):
  def __sub__(self, other):
    return NotImplemented
  def __rsub__(self, other):
    return NotImplemented
v0 = MyInt(3)
v1 = 7
)")
                   .isError());
  Object v0(&scope, mainModuleAt(runtime_, "v0"));
  Object v1(&scope, mainModuleAt(runtime_, "v1"));

  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_->newTupleWith2(v0, v1));
  code.setConsts(*consts);
  const byte bytecode[] = {
      LOAD_CONST, 0, LOAD_CONST, 1, BINARY_SUBTRACT, 0, RETURN_VALUE, 0,
  };
  code.setCode(runtime_->newBytesWithAll(bytecode));

  Object qualname(&scope, Str::empty());
  Module module(&scope, runtime_->findOrCreateMainModule());
  Function function(
      &scope, runtime_->newFunctionWithCode(thread_, qualname, code, module));

  // Update inline cache.
  EXPECT_TRUE(isIntEqualsWord(Interpreter::call0(thread_, function), -4));

  ASSERT_TRUE(function.caches().isTuple());
  MutableTuple caches(&scope, function.caches());
  BinaryOpFlags dummy;
  ASSERT_FALSE(
      icLookupBinaryOp(*caches, 0, v0.layoutId(), v1.layoutId(), &dummy)
          .isErrorNotFound());

  // Should hit the cache for __sub__ and then call binaryOperationRetry().
  EXPECT_TRUE(isIntEqualsWord(Interpreter::call0(thread_, function), -4));
}

TEST_F(InterpreterTest, DoBinaryOpWithSmallIntsRewritesOpcode) {
  HandleScope scope(thread_);

  word left = 7;
  word right = -13;
  Code code(&scope, newEmptyCode());
  Object left_obj(&scope, runtime_->newInt(left));
  Object right_obj(&scope, runtime_->newInt(right));
  Tuple consts(&scope, runtime_->newTupleWith2(left_obj, right_obj));
  code.setConsts(*consts);
  const byte bytecode[] = {
      LOAD_CONST, 0, LOAD_CONST, 1, BINARY_SUBTRACT, 0, RETURN_VALUE, 0,
  };
  code.setCode(runtime_->newBytesWithAll(bytecode));

  Object qualname(&scope, Str::empty());
  Module module(&scope, runtime_->findOrCreateMainModule());
  Function function(
      &scope, runtime_->newFunctionWithCode(thread_, qualname, code, module));

  // Update the opcode.
  ASSERT_TRUE(
      isIntEqualsWord(Interpreter::call0(thread_, function), left - right));

  MutableBytes rewritten_bytecode(&scope, function.rewrittenBytecode());
  EXPECT_EQ(rewritten_bytecode.byteAt(4), BINARY_SUB_SMALLINT);

  // Updated opcode returns the same value.
  EXPECT_TRUE(
      isIntEqualsWord(Interpreter::call0(thread_, function), left - right));
}

static bool containsBytecode(const Function& function, Bytecode bc) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  MutableBytes bytecode(&scope, function.rewrittenBytecode());
  for (word i = 0, length = bytecode.length(); i < length;) {
    BytecodeOp bco = nextBytecodeOp(bytecode, &i);
    if (bco.bc == bc) {
      return true;
    }
  }
  return false;
}

static bool functionMatchesRef2(const Function& function,
                                const Object& reference, const Object& arg0,
                                const Object& arg1) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object expected(&scope, Interpreter::call2(thread, reference, arg0, arg1));
  EXPECT_FALSE(expected.isError());
  Object actual(&scope, Interpreter::call2(thread, function, arg0, arg1));
  EXPECT_FALSE(actual.isError());
  return Runtime::objectEquals(thread, *expected, *actual) == Bool::trueObj();
}

// Test that `function(arg0, arg1) == reference(arg0, arg1)` with the assumption
// that `function` contains a `BINARY_OP_MONOMORPHIC` opcode that will be
// specialized to `opcode_specialized` when called with `arg0` and `arg1`.
// Calling the function with `arg_o` should trigger a revert to
// `BINARY_OP_MONOMORPHIC`.
static void testBinaryOpRewrite(const Function& function,
                                const Function& reference,
                                Bytecode opcode_specialized, const Object& arg0,
                                const Object& arg1, const Object& arg_o) {
  EXPECT_TRUE(containsBytecode(function, BINARY_OP_ANAMORPHIC));

  EXPECT_TRUE(functionMatchesRef2(function, reference, arg0, arg1));
  EXPECT_FALSE(containsBytecode(function, BINARY_OP_ANAMORPHIC));
  EXPECT_TRUE(containsBytecode(function, opcode_specialized));
  EXPECT_TRUE(functionMatchesRef2(function, reference, arg1, arg0));
  EXPECT_TRUE(containsBytecode(function, opcode_specialized));

  EXPECT_TRUE(functionMatchesRef2(function, reference, arg0, arg_o));
  EXPECT_TRUE(containsBytecode(function, BINARY_OP_MONOMORPHIC));
  EXPECT_FALSE(containsBytecode(function, opcode_specialized));

  EXPECT_TRUE(functionMatchesRef2(function, reference, arg0, arg1));
}

TEST_F(InterpreterTest, BinaryOpAnamorphicRewritesToBinaryAddSmallInt) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def function(a, b):
    return a + b
reference = int.__add__
)")
                   .isError());
  Function function(&scope, mainModuleAt(runtime_, "function"));
  Function reference(&scope, mainModuleAt(runtime_, "reference"));
  Object arg0(&scope, SmallInt::fromWord(34));
  Object arg1(&scope, SmallInt::fromWord(12));
  const uword digits2[] = {0x12345678, 0xabcdef};
  Object arg_l(&scope, runtime_->newIntWithDigits(digits2));
  testBinaryOpRewrite(function, reference, BINARY_ADD_SMALLINT, arg0, arg1,
                      arg_l);
}

TEST_F(InterpreterTest, BinaryOpAnamorphicRewritesToBinarySubSmallInt) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def function(a, b):
    return a - b
reference = int.__sub__
)")
                   .isError());
  Function function(&scope, mainModuleAt(runtime_, "function"));
  Function reference(&scope, mainModuleAt(runtime_, "reference"));
  Object arg0(&scope, SmallInt::fromWord(94));
  Object arg1(&scope, SmallInt::fromWord(21));
  const uword digits2[] = {0x12345678, 0xabcdef};
  Object arg_l(&scope, runtime_->newIntWithDigits(digits2));
  testBinaryOpRewrite(function, reference, BINARY_SUB_SMALLINT, arg0, arg1,
                      arg_l);
}

TEST_F(InterpreterTest, BinaryOpAnamorphicRewritesToBinaryOrSmallInt) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def function(a, b):
    return a | b
reference = int.__or__
)")
                   .isError());
  Function function(&scope, mainModuleAt(runtime_, "function"));
  Function reference(&scope, mainModuleAt(runtime_, "reference"));
  Object arg0(&scope, SmallInt::fromWord(0xa5));
  Object arg1(&scope, SmallInt::fromWord(0x42));
  const uword digits2[] = {0x12345678, 0xabcdef};
  Object arg_l(&scope, runtime_->newIntWithDigits(digits2));
  testBinaryOpRewrite(function, reference, BINARY_OR_SMALLINT, arg0, arg1,
                      arg_l);
}

TEST_F(InterpreterTest, BinaryOpAnamorphicRewritesToBinaryAndSmallInt) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def function(a, b):
    return a & b
reference = int.__and__
)")
                   .isError());
  Function function(&scope, mainModuleAt(runtime_, "function"));
  Function reference(&scope, mainModuleAt(runtime_, "reference"));
  Object arg0(&scope, SmallInt::fromWord(0xa5));
  Object arg1(&scope, SmallInt::fromWord(0x42));
  const uword digits2[] = {0x12345678, 0xabcdef};
  Object arg_l(&scope, runtime_->newIntWithDigits(digits2));
  testBinaryOpRewrite(function, reference, BINARY_AND_SMALLINT, arg0, arg1,
                      arg_l);
}

TEST_F(InterpreterTest, BinarySubscrWithListAndSmallInt) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(l, i):
    return l[i]

l = [1,2,3]
)")
                   .isError());
  Function foo(&scope, mainModuleAt(runtime_, "foo"));
  MutableBytes rewritten(&scope, foo.rewrittenBytecode());
  ASSERT_EQ(rewritten.byteAt(4), BINARY_SUBSCR_ANAMORPHIC);

  List l(&scope, mainModuleAt(runtime_, "l"));
  SmallInt zero(&scope, SmallInt::fromWord(0));
  EXPECT_TRUE(isIntEqualsWord(Interpreter::call2(thread_, foo, l, zero), 1));
  EXPECT_EQ(rewritten.byteAt(4), BINARY_SUBSCR_LIST);

  SmallInt one(&scope, SmallInt::fromWord(1));
  EXPECT_TRUE(isIntEqualsWord(Interpreter::call2(thread_, foo, l, one), 2));
  EXPECT_EQ(rewritten.byteAt(4), BINARY_SUBSCR_LIST);
}

TEST_F(
    InterpreterTest,
    BinarySubscrListRevertsBackToBinarySubscrMonomorphicWhenNonListObserved) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(l, i):
    return l[i]

l = [1,2,3]
d = {1: -1}
)")
                   .isError());
  Function foo(&scope, mainModuleAt(runtime_, "foo"));
  MutableBytes rewritten(&scope, foo.rewrittenBytecode());
  ASSERT_EQ(rewritten.byteAt(4), BINARY_SUBSCR_ANAMORPHIC);

  List l(&scope, mainModuleAt(runtime_, "l"));
  SmallInt key(&scope, SmallInt::fromWord(1));
  EXPECT_TRUE(isIntEqualsWord(Interpreter::call2(thread_, foo, l, key), 2));
  EXPECT_EQ(rewritten.byteAt(4), BINARY_SUBSCR_LIST);

  // Revert back to caching __getitem__ when a non-list is observed.
  Dict d(&scope, mainModuleAt(runtime_, "d"));
  EXPECT_TRUE(isIntEqualsWord(Interpreter::call2(thread_, foo, d, key), -1));
  EXPECT_EQ(rewritten.byteAt(4), BINARY_SUBSCR_MONOMORPHIC);
}

TEST_F(
    InterpreterTest,
    BinarySubscrListRevertsBackToBinarySubscrMonomorphicWhenNonSmallIntKeyObserved) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(l, i):
    return l[i]

l = [1,2,3]
large_int = 2**64
)")
                   .isError());
  Function foo(&scope, mainModuleAt(runtime_, "foo"));
  MutableBytes rewritten(&scope, foo.rewrittenBytecode());
  ASSERT_EQ(rewritten.byteAt(4), BINARY_SUBSCR_ANAMORPHIC);

  List l(&scope, mainModuleAt(runtime_, "l"));
  SmallInt key(&scope, SmallInt::fromWord(1));
  EXPECT_TRUE(isIntEqualsWord(Interpreter::call2(thread_, foo, l, key), 2));
  EXPECT_EQ(rewritten.byteAt(4), BINARY_SUBSCR_LIST);

  // Revert back to caching __getitem__ when the key is not SmallInt.
  LargeInt large_int(&scope, mainModuleAt(runtime_, "large_int"));
  EXPECT_TRUE(Interpreter::call2(thread_, foo, l, large_int).isError());
  EXPECT_EQ(rewritten.byteAt(4), BINARY_SUBSCR_MONOMORPHIC);
}

TEST_F(
    InterpreterTest,
    BinarySubscrListRevertsBackToBinarySubscrMonomorphicWhenNegativeKeyObserved) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(l, i):
    return l[i]

l = [1,2,3]
)")
                   .isError());
  Function foo(&scope, mainModuleAt(runtime_, "foo"));
  MutableBytes rewritten(&scope, foo.rewrittenBytecode());
  ASSERT_EQ(rewritten.byteAt(4), BINARY_SUBSCR_ANAMORPHIC);

  List l(&scope, mainModuleAt(runtime_, "l"));
  SmallInt key(&scope, SmallInt::fromWord(1));
  EXPECT_TRUE(isIntEqualsWord(Interpreter::call2(thread_, foo, l, key), 2));
  EXPECT_EQ(rewritten.byteAt(4), BINARY_SUBSCR_LIST);

  // Revert back to caching __getitem__ when the key is negative.
  SmallInt negative(&scope, SmallInt::fromWord(-1));
  EXPECT_TRUE(
      isIntEqualsWord(Interpreter::call2(thread_, foo, l, negative), 3));
  EXPECT_EQ(rewritten.byteAt(4), BINARY_SUBSCR_MONOMORPHIC);
}

TEST_F(InterpreterTest, StoreSubscrWithListAndSmallInt) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(l, i):
    l[i] = 5
    return l[i]

l = [1,2,3]
)")
                   .isError());
  Function foo(&scope, mainModuleAt(runtime_, "foo"));
  MutableBytes rewritten(&scope, foo.rewrittenBytecode());
  ASSERT_EQ(rewritten.byteAt(6), STORE_SUBSCR_ANAMORPHIC);

  List l(&scope, mainModuleAt(runtime_, "l"));
  SmallInt zero(&scope, SmallInt::fromWord(0));
  EXPECT_TRUE(isIntEqualsWord(Interpreter::call2(thread_, foo, l, zero), 5));
  EXPECT_EQ(rewritten.byteAt(6), STORE_SUBSCR_LIST);

  SmallInt one(&scope, SmallInt::fromWord(1));
  EXPECT_TRUE(isIntEqualsWord(Interpreter::call2(thread_, foo, l, one), 5));
  EXPECT_EQ(rewritten.byteAt(6), STORE_SUBSCR_LIST);
}

TEST_F(InterpreterTest,
       StoreSubscrListRevertsBackToStoreSubscrMonomorphicWhenNonListObserved) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(l, i):
    l[i] = 5
    return l[i]

l = [1,2,3]
d = {1: -1}
)")
                   .isError());
  Function foo(&scope, mainModuleAt(runtime_, "foo"));
  MutableBytes rewritten(&scope, foo.rewrittenBytecode());
  ASSERT_EQ(rewritten.byteAt(6), STORE_SUBSCR_ANAMORPHIC);

  List l(&scope, mainModuleAt(runtime_, "l"));
  SmallInt key(&scope, SmallInt::fromWord(1));
  EXPECT_TRUE(isIntEqualsWord(Interpreter::call2(thread_, foo, l, key), 5));
  EXPECT_EQ(rewritten.byteAt(6), STORE_SUBSCR_LIST);

  // Revert back to caching __getitem__ when a non-list is observed.
  Dict d(&scope, mainModuleAt(runtime_, "d"));
  EXPECT_TRUE(isIntEqualsWord(Interpreter::call2(thread_, foo, d, key), 5));
  EXPECT_EQ(rewritten.byteAt(6), STORE_SUBSCR_MONOMORPHIC);
}

TEST_F(
    InterpreterTest,
    StoreSubscrListRevertsBackToStoreSubscrMonomorphicWhenNonSmallIntKeyObserved) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(l, i):
    l[i] = 5
    return l[i]

l = [1,2,3]
large_int = 2**64
)")
                   .isError());
  Function foo(&scope, mainModuleAt(runtime_, "foo"));
  MutableBytes rewritten(&scope, foo.rewrittenBytecode());
  ASSERT_EQ(rewritten.byteAt(6), STORE_SUBSCR_ANAMORPHIC);

  List l(&scope, mainModuleAt(runtime_, "l"));
  SmallInt key(&scope, SmallInt::fromWord(1));
  EXPECT_TRUE(isIntEqualsWord(Interpreter::call2(thread_, foo, l, key), 5));
  EXPECT_EQ(rewritten.byteAt(6), STORE_SUBSCR_LIST);

  // Revert back to caching __getitem__ when the key is not SmallInt.
  LargeInt large_int(&scope, mainModuleAt(runtime_, "large_int"));
  EXPECT_TRUE(Interpreter::call2(thread_, foo, l, large_int).isError());
  EXPECT_EQ(rewritten.byteAt(6), STORE_SUBSCR_MONOMORPHIC);
}

TEST_F(
    InterpreterTest,
    StoreSubscrListRevertsBackToStoreSubscrMonomorphicWhenNegativeKeyObserved) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(l, i):
    l[i] = 5
    return l[i]

l = [1,2,3]
)")
                   .isError());
  Function foo(&scope, mainModuleAt(runtime_, "foo"));
  MutableBytes rewritten(&scope, foo.rewrittenBytecode());
  ASSERT_EQ(rewritten.byteAt(6), STORE_SUBSCR_ANAMORPHIC);

  List l(&scope, mainModuleAt(runtime_, "l"));
  SmallInt key(&scope, SmallInt::fromWord(1));
  EXPECT_TRUE(isIntEqualsWord(Interpreter::call2(thread_, foo, l, key), 5));
  EXPECT_EQ(rewritten.byteAt(6), STORE_SUBSCR_LIST);

  // Revert back to caching __getitem__ when the key is negative.
  SmallInt negative(&scope, SmallInt::fromWord(-1));
  EXPECT_TRUE(
      isIntEqualsWord(Interpreter::call2(thread_, foo, l, negative), 5));
  EXPECT_EQ(rewritten.byteAt(6), STORE_SUBSCR_MONOMORPHIC);
}

TEST_F(InterpreterTest, BinarySubscrWithTupleAndSmallInt) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(l, i):
    return l[i]

l = (1,2,3)
)")
                   .isError());
  Function foo(&scope, mainModuleAt(runtime_, "foo"));
  MutableBytes rewritten(&scope, foo.rewrittenBytecode());
  ASSERT_EQ(rewritten.byteAt(4), BINARY_SUBSCR_ANAMORPHIC);

  Tuple l(&scope, mainModuleAt(runtime_, "l"));
  SmallInt zero(&scope, SmallInt::fromWord(0));
  EXPECT_TRUE(isIntEqualsWord(Interpreter::call2(thread_, foo, l, zero), 1));
  EXPECT_EQ(rewritten.byteAt(4), BINARY_SUBSCR_TUPLE);

  SmallInt one(&scope, SmallInt::fromWord(1));
  EXPECT_TRUE(isIntEqualsWord(Interpreter::call2(thread_, foo, l, one), 2));
  EXPECT_EQ(rewritten.byteAt(4), BINARY_SUBSCR_TUPLE);
}

TEST_F(
    InterpreterTest,
    BinarySubscrTupleRevertsBackToBinarySubscrMonomorphicWhenNonTupleObserved) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(l, i):
    return l[i]

l = (1,2,3)
d = {1: -1}
)")
                   .isError());
  Function foo(&scope, mainModuleAt(runtime_, "foo"));
  MutableBytes rewritten(&scope, foo.rewrittenBytecode());
  ASSERT_EQ(rewritten.byteAt(4), BINARY_SUBSCR_ANAMORPHIC);

  Tuple l(&scope, mainModuleAt(runtime_, "l"));
  SmallInt key(&scope, SmallInt::fromWord(1));
  EXPECT_TRUE(isIntEqualsWord(Interpreter::call2(thread_, foo, l, key), 2));
  EXPECT_EQ(rewritten.byteAt(4), BINARY_SUBSCR_TUPLE);

  // Revert back to caching __getitem__ when a non-list is observed.
  Dict d(&scope, mainModuleAt(runtime_, "d"));
  EXPECT_TRUE(isIntEqualsWord(Interpreter::call2(thread_, foo, d, key), -1));
  EXPECT_EQ(rewritten.byteAt(4), BINARY_SUBSCR_MONOMORPHIC);
}

TEST_F(
    InterpreterTest,
    BinarySubscrTupleRevertsBackToBinarySubscrMonomorphicWhenNonSmallIntKeyObserved) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(l, i):
    return l[i]

l = (1,2,3)
large_int = 2**64
)")
                   .isError());
  Function foo(&scope, mainModuleAt(runtime_, "foo"));
  MutableBytes rewritten(&scope, foo.rewrittenBytecode());
  ASSERT_EQ(rewritten.byteAt(4), BINARY_SUBSCR_ANAMORPHIC);

  Tuple l(&scope, mainModuleAt(runtime_, "l"));
  SmallInt key(&scope, SmallInt::fromWord(1));
  EXPECT_TRUE(isIntEqualsWord(Interpreter::call2(thread_, foo, l, key), 2));
  EXPECT_EQ(rewritten.byteAt(4), BINARY_SUBSCR_TUPLE);

  // Revert back to caching __getitem__ when the key is not SmallInt.
  LargeInt large_int(&scope, mainModuleAt(runtime_, "large_int"));
  EXPECT_TRUE(Interpreter::call2(thread_, foo, l, large_int).isError());
  EXPECT_EQ(rewritten.byteAt(4), BINARY_SUBSCR_MONOMORPHIC);
}

TEST_F(
    InterpreterTest,
    BinarySubscrTupleRevertsBackToBinarySubscrMonomorphicWhenNegativeKeyObserved) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(l, i):
    return l[i]

l = (1,2,3)
)")
                   .isError());
  Function foo(&scope, mainModuleAt(runtime_, "foo"));
  MutableBytes rewritten(&scope, foo.rewrittenBytecode());
  ASSERT_EQ(rewritten.byteAt(4), BINARY_SUBSCR_ANAMORPHIC);

  Tuple l(&scope, mainModuleAt(runtime_, "l"));
  SmallInt key(&scope, SmallInt::fromWord(1));
  EXPECT_TRUE(isIntEqualsWord(Interpreter::call2(thread_, foo, l, key), 2));
  EXPECT_EQ(rewritten.byteAt(4), BINARY_SUBSCR_TUPLE);

  // Revert back to caching __getitem__ when the key is negative.
  SmallInt negative(&scope, SmallInt::fromWord(-1));
  EXPECT_TRUE(
      isIntEqualsWord(Interpreter::call2(thread_, foo, l, negative), 3));
  EXPECT_EQ(rewritten.byteAt(4), BINARY_SUBSCR_MONOMORPHIC);
}

TEST_F(InterpreterTest, InplaceOpCachedInsertsDependencyForThreeAttributes) {
  HandleScope scope(thread_);
  EXPECT_FALSE(runFromCStr(runtime_, R"(
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
  Function cache_inplace_op(&scope, mainModuleAt(runtime_, "cache_inplace_op"));
  MutableTuple caches(&scope, cache_inplace_op.caches());
  Object a(&scope, mainModuleAt(runtime_, "a"));
  Object b(&scope, mainModuleAt(runtime_, "b"));
  Type type_a(&scope, mainModuleAt(runtime_, "A"));
  Type type_b(&scope, mainModuleAt(runtime_, "B"));
  BinaryOpFlags flag;
  ASSERT_EQ(icLookupBinaryOp(*caches, 0, a.layoutId(), b.layoutId(), &flag),
            mainModuleAt(runtime_, "A__imul__"));

  // Verify that A.__imul__ has the dependent.
  Object inplace_op_name(&scope, runtime_->symbols()->at(ID(__imul__)));
  Object inplace_attr(&scope, typeValueCellAt(type_a, inplace_op_name));
  ASSERT_TRUE(inplace_attr.isValueCell());
  ASSERT_TRUE(ValueCell::cast(*inplace_attr).dependencyLink().isWeakLink());
  EXPECT_EQ(WeakLink::cast(ValueCell::cast(*inplace_attr).dependencyLink())
                .referent(),
            *cache_inplace_op);

  // Verify that A.__mul__ has the dependent.
  Object left_op_name(&scope, runtime_->symbols()->at(ID(__mul__)));
  Object type_a_attr(&scope, typeValueCellAt(type_a, left_op_name));
  ASSERT_TRUE(type_a_attr.isValueCell());
  ASSERT_TRUE(ValueCell::cast(*type_a_attr).dependencyLink().isWeakLink());
  EXPECT_EQ(
      WeakLink::cast(ValueCell::cast(*type_a_attr).dependencyLink()).referent(),
      *cache_inplace_op);

  // Verify that B.__rmul__ has the dependent.
  Object right_op_name(&scope, runtime_->symbols()->at(ID(__rmul__)));
  Object type_b_attr(&scope, typeValueCellAt(type_b, right_op_name));
  ASSERT_TRUE(type_b_attr.isValueCell());
  ASSERT_TRUE(ValueCell::cast(*type_b_attr).dependencyLink().isWeakLink());
  EXPECT_EQ(
      WeakLink::cast(ValueCell::cast(*type_b_attr).dependencyLink()).referent(),
      *cache_inplace_op);
}

TEST_F(InterpreterTest, ImportFromWithMissingAttributeRaisesImportError) {
  HandleScope scope(thread_);
  Str name(&scope, runtime_->newStrFromCStr("foo"));
  Module module(&scope, runtime_->newModule(name));
  Object modules(&scope, runtime_->modules());
  ASSERT_FALSE(
      objectSetItem(thread_, modules, name, module).isErrorException());
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, "from foo import bar"),
                            LayoutId::kImportError,
                            "cannot import name 'bar' from 'foo'"));
}

TEST_F(InterpreterTest, ImportFromCallsDunderGetattribute) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  def __getattribute__(self, name):
    return f"getattribute '{name}'"
i = C()
)")
                   .isError());
  Object i(&scope, mainModuleAt(runtime_, "i"));

  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_->newTupleWith1(i));
  code.setConsts(*consts);
  Object name(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  Tuple names(&scope, runtime_->newTupleWith1(name));
  code.setNames(*names);
  const byte bytecode[] = {LOAD_CONST, 0, IMPORT_FROM, 0, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));

  EXPECT_TRUE(isStrEqualsCStr(runCode(code), "getattribute 'foo'"));
}

TEST_F(InterpreterTest, ImportFromWithNonModuleRaisesImportError) {
  HandleScope scope(thread_);
  Code code(&scope, newEmptyCode());
  Object obj(&scope, NoneType::object());
  Tuple consts(&scope, runtime_->newTupleWith1(obj));
  code.setConsts(*consts);
  Object name(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  Tuple names(&scope, runtime_->newTupleWith1(name));
  code.setNames(*names);
  const byte bytecode[] = {LOAD_CONST, 0, IMPORT_FROM, 0, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));

  EXPECT_TRUE(raisedWithStr(runCode(code), LayoutId::kImportError,
                            "cannot import name 'foo'"));
}

TEST_F(InterpreterTest, ImportFromWithNonModulePropagatesException) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  def __getattribute__(self, name):
    raise UserWarning()
i = C()
)")
                   .isError());
  Object i(&scope, mainModuleAt(runtime_, "i"));

  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_->newTupleWith1(i));
  code.setConsts(*consts);
  Object name(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  Tuple names(&scope, runtime_->newTupleWith1(name));
  code.setNames(*names);
  const byte bytecode[] = {LOAD_CONST, 0, IMPORT_FROM, 0, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));

  EXPECT_TRUE(raised(runCode(code), LayoutId::kUserWarning));
}

TEST_F(InterpreterTest, InplaceOperationCallsInplaceMethod) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
    def __isub__(self, other):
        return (C, '__isub__', self, other)

left = C()
right = C()
)")
                   .isError());

  Object left(&scope, mainModuleAt(runtime_, "left"));
  Object right(&scope, mainModuleAt(runtime_, "right"));
  Object c_class(&scope, mainModuleAt(runtime_, "C"));

  Object result_obj(
      &scope, Interpreter::inplaceOperation(thread_, Interpreter::BinaryOp::SUB,
                                            left, right));
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

  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
    def __sub__(self, other):
        return (C, '__sub__', self, other)

left = C()
right = C()
)")
                   .isError());

  Object left(&scope, mainModuleAt(runtime_, "left"));
  Object right(&scope, mainModuleAt(runtime_, "right"));
  Object c_class(&scope, mainModuleAt(runtime_, "C"));

  Object result_obj(
      &scope, Interpreter::inplaceOperation(thread_, Interpreter::BinaryOp::SUB,
                                            left, right));
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

  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
    def __isub__(self, other):
        return NotImplemented
    def __sub__(self, other):
        return (C, '__sub__', self, other)

left = C()
right = C()
)")
                   .isError());

  Object left(&scope, mainModuleAt(runtime_, "left"));
  Object right(&scope, mainModuleAt(runtime_, "right"));
  Object c_class(&scope, mainModuleAt(runtime_, "C"));

  Object result_obj(
      &scope, Interpreter::inplaceOperation(thread_, Interpreter::BinaryOp::SUB,
                                            left, right));
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
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class MyInt(int):
  def __isub__(self, other):
    return int(self) - other - 2
v0 = MyInt(9)
v1 = MyInt(-11)
v2 = MyInt(-3)
v3 = MyInt(7)
)")
                   .isError());
  Object v0(&scope, mainModuleAt(runtime_, "v0"));
  Object v1(&scope, mainModuleAt(runtime_, "v1"));
  Object v2(&scope, mainModuleAt(runtime_, "v2"));
  Object v3(&scope, mainModuleAt(runtime_, "v3"));
  Object method(&scope, NoneType::object());
  BinaryOpFlags flags;
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::inplaceOperationSetMethod(
          thread_, Interpreter::BinaryOp::SUB, v0, v1, &method, &flags),
      18));
  EXPECT_EQ(flags, kInplaceBinaryOpRetry);

  ASSERT_EQ(v0.layoutId(), v2.layoutId());
  ASSERT_EQ(v1.layoutId(), v3.layoutId());
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::binaryOperationWithMethod(thread_, *method, flags, *v2, *v3),
      -12));
}

TEST_F(InterpreterTest, InplaceOperationSetMethodSetsMethodFlagsReverseRetry) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
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
  Object v0(&scope, mainModuleAt(runtime_, "v0"));
  Object v1(&scope, mainModuleAt(runtime_, "v1"));
  Object v2(&scope, mainModuleAt(runtime_, "v2"));
  Object v3(&scope, mainModuleAt(runtime_, "v3"));
  Object method(&scope, NoneType::object());
  BinaryOpFlags flags;
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::inplaceOperationSetMethod(
          thread_, Interpreter::BinaryOp::POW, v0, v1, &method, &flags),
      20));
  EXPECT_EQ(flags, kBinaryOpReflected | kBinaryOpNotImplementedRetry);

  ASSERT_EQ(v0.layoutId(), v2.layoutId());
  ASSERT_EQ(v1.layoutId(), v3.layoutId());
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::binaryOperationWithMethod(thread_, *method, flags, *v2, *v3),
      249));
}

TEST_F(InterpreterTest, InplaceAddWithSmallIntsRewritesOpcode) {
  HandleScope scope(thread_);

  word left = 7;
  word right = -13;
  Code code(&scope, newEmptyCode());
  Object left_obj(&scope, runtime_->newInt(left));
  Object right_obj(&scope, runtime_->newInt(right));
  Tuple consts(&scope, runtime_->newTupleWith2(left_obj, right_obj));
  code.setConsts(*consts);
  const byte bytecode[] = {
      LOAD_CONST, 0, LOAD_CONST, 1, INPLACE_ADD, 0, RETURN_VALUE, 0,
  };
  code.setCode(runtime_->newBytesWithAll(bytecode));

  Object qualname(&scope, Str::empty());
  Module module(&scope, runtime_->findOrCreateMainModule());
  Function function(
      &scope, runtime_->newFunctionWithCode(thread_, qualname, code, module));

  // Update the opcode.
  ASSERT_TRUE(
      isIntEqualsWord(Interpreter::call0(thread_, function), left + right));

  MutableBytes rewritten_bytecode(&scope, function.rewrittenBytecode());
  EXPECT_EQ(rewritten_bytecode.byteAt(4), INPLACE_ADD_SMALLINT);

  // Updated opcode returns the same value.
  EXPECT_TRUE(
      isIntEqualsWord(Interpreter::call0(thread_, function), left + right));
}

TEST_F(InterpreterTest, InplaceAddSmallInt) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(a, b):
    a += b
    return a
)")
                   .isError());
  Function function(&scope, mainModuleAt(runtime_, "foo"));
  MutableBytes rewritten(&scope, function.rewrittenBytecode());
  ASSERT_EQ(rewritten.byteAt(4), INPLACE_OP_ANAMORPHIC);

  SmallInt left(&scope, SmallInt::fromWord(7));
  SmallInt right(&scope, SmallInt::fromWord(-13));

  rewritten.byteAtPut(4, INPLACE_ADD_SMALLINT);
  left = SmallInt::fromWord(7);
  right = SmallInt::fromWord(-13);
  // 7 + (-13)
  EXPECT_TRUE(
      isIntEqualsWord(Interpreter::call2(thread_, function, left, right), -6));
}

TEST_F(InterpreterTest, InplaceAddSmallIntRevertsBackToInplaceOp) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(a, b):
    a += b
    return a
)")
                   .isError());
  Function function(&scope, mainModuleAt(runtime_, "foo"));
  MutableBytes rewritten(&scope, function.rewrittenBytecode());
  ASSERT_EQ(rewritten.byteAt(4), INPLACE_OP_ANAMORPHIC);

  LargeInt left(&scope, runtime_->newInt(SmallInt::kMaxValue + 1));
  SmallInt right(&scope, SmallInt::fromWord(13));

  rewritten.byteAtPut(4, INPLACE_ADD_SMALLINT);
  // LARGE_SMALL_INT += SMALL_INT
  EXPECT_TRUE(
      isIntEqualsWord(Interpreter::call2(thread_, function, left, right),
                      SmallInt::kMaxValue + 1 + 13));
  EXPECT_EQ(rewritten.byteAt(4), INPLACE_OP_MONOMORPHIC);
}

TEST_F(InterpreterTest, InplaceSubtractWithSmallIntsRewritesOpcode) {
  HandleScope scope(thread_);

  word left = 7;
  word right = -13;
  Code code(&scope, newEmptyCode());
  Object left_obj(&scope, runtime_->newInt(left));
  Object right_obj(&scope, runtime_->newInt(right));
  Tuple consts(&scope, runtime_->newTupleWith2(left_obj, right_obj));
  code.setConsts(*consts);
  const byte bytecode[] = {
      LOAD_CONST, 0, LOAD_CONST, 1, INPLACE_SUBTRACT, 0, RETURN_VALUE, 0,
  };
  code.setCode(runtime_->newBytesWithAll(bytecode));

  Object qualname(&scope, Str::empty());
  Module module(&scope, runtime_->findOrCreateMainModule());
  Function function(
      &scope, runtime_->newFunctionWithCode(thread_, qualname, code, module));

  // Update the opcode.
  ASSERT_TRUE(
      isIntEqualsWord(Interpreter::call0(thread_, function), left - right));

  MutableBytes rewritten_bytecode(&scope, function.rewrittenBytecode());
  EXPECT_EQ(rewritten_bytecode.byteAt(4), INPLACE_SUB_SMALLINT);

  // Updated opcode returns the same value.
  EXPECT_TRUE(
      isIntEqualsWord(Interpreter::call0(thread_, function), left - right));
}

TEST_F(InterpreterTest, InplaceSubtractSmallInt) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(a, b):
    a -= b
    return a
)")
                   .isError());
  Function function(&scope, mainModuleAt(runtime_, "foo"));
  MutableBytes rewritten(&scope, function.rewrittenBytecode());
  ASSERT_EQ(rewritten.byteAt(4), INPLACE_OP_ANAMORPHIC);

  SmallInt left(&scope, SmallInt::fromWord(7));
  SmallInt right(&scope, SmallInt::fromWord(-13));

  rewritten.byteAtPut(4, INPLACE_SUB_SMALLINT);
  left = SmallInt::fromWord(7);
  right = SmallInt::fromWord(-13);
  // 7 - (-13)
  EXPECT_TRUE(
      isIntEqualsWord(Interpreter::call2(thread_, function, left, right), 20));
}

TEST_F(InterpreterTest, InplaceSubSmallIntRevertsBackToInplaceOp) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(a, b):
    a -= b
    return a
)")
                   .isError());
  Function function(&scope, mainModuleAt(runtime_, "foo"));
  MutableBytes rewritten(&scope, function.rewrittenBytecode());
  ASSERT_EQ(rewritten.byteAt(4), INPLACE_OP_ANAMORPHIC);

  LargeInt left(&scope, runtime_->newInt(SmallInt::kMaxValue + 1));
  SmallInt right(&scope, SmallInt::fromWord(13));

  rewritten.byteAtPut(4, INPLACE_SUB_SMALLINT);
  // LARGE_SMALL_INT -= SMALL_INT
  EXPECT_TRUE(
      isIntEqualsWord(Interpreter::call2(thread_, function, left, right),
                      SmallInt::kMaxValue + 1 - 13));
  EXPECT_EQ(rewritten.byteAt(4), INPLACE_OP_MONOMORPHIC);
}

TEST_F(InterpreterDeathTest, InvalidOpcode) {
  HandleScope scope(thread_);

  Code code(&scope, newEmptyCode());
  const byte bytecode[] = {NOP, 0, NOP, 0, UNUSED_BYTECODE_6, 17, NOP, 7};
  code.setCode(runtime_->newBytesWithAll(bytecode));

  ASSERT_DEATH(static_cast<void>(runCode(code)),
               "bytecode 'UNUSED_BYTECODE_6'");
}

TEST_F(InterpreterTest, CallDescriptorGetWithBuiltinTypeDescriptors) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(

def class_method_func(self): pass

def static_method_func(cls): pass

class C:
    class_method = classmethod(class_method_func)

    static_method = staticmethod(static_method_func)

    @property
    def property_field(self): return "property"

    def function_field(self): pass

i = C()
)")
                   .isError());
  HandleScope scope(thread_);
  Type c(&scope, mainModuleAt(runtime_, "C"));
  Type type(&scope, runtime_->typeOf(*c));
  Object i(&scope, mainModuleAt(runtime_, "i"));

  Object class_method_name(&scope,
                           Runtime::internStrFromCStr(thread_, "class_method"));
  Object class_method(&scope, typeAt(c, class_method_name));
  BoundMethod class_method_result(
      &scope, Interpreter::callDescriptorGet(thread_, class_method, i, c));
  EXPECT_EQ(class_method_result.self(), *c);
  EXPECT_EQ(class_method_result.function(),
            mainModuleAt(runtime_, "class_method_func"));

  Object static_method_name(
      &scope, Runtime::internStrFromCStr(thread_, "static_method"));
  Object static_method(&scope, typeAt(c, static_method_name));
  Function static_method_result(
      &scope, Interpreter::callDescriptorGet(thread_, static_method, c, type));
  EXPECT_EQ(*static_method_result,
            mainModuleAt(runtime_, "static_method_func"));

  Object property_field_name(
      &scope, Runtime::internStrFromCStr(thread_, "property_field"));
  Object property_field(&scope, typeAt(c, property_field_name));
  Object property_field_result(
      &scope, Interpreter::callDescriptorGet(thread_, property_field, i, c));
  EXPECT_TRUE(isStrEqualsCStr(*property_field_result, "property"));

  Object function_field_name(
      &scope, Runtime::internStrFromCStr(thread_, "function_field"));
  Object function_field(&scope, typeAt(c, function_field_name));
  BoundMethod function_field_result(
      &scope, Interpreter::callDescriptorGet(thread_, function_field, i, c));
  EXPECT_EQ(function_field_result.self(), *i);
  EXPECT_EQ(function_field_result.function(), *function_field);

  Object none(&scope, NoneType::object());
  Function function_field_result_from_none_instance(
      &scope, Interpreter::callDescriptorGet(thread_, function_field, none, c));
  EXPECT_EQ(function_field_result_from_none_instance, *function_field);

  Type none_type(&scope, runtime_->typeAt(LayoutId::kNoneType));
  BoundMethod function_field_result_from_none_instance_of_none_type(
      &scope,
      Interpreter::callDescriptorGet(thread_, function_field, none, none_type));
  EXPECT_EQ(function_field_result_from_none_instance_of_none_type.self(),
            *none);
  EXPECT_EQ(function_field_result_from_none_instance_of_none_type.function(),
            *function_field);
}

TEST_F(InterpreterTest, CompareInAnamorphicWithStrRewritesOpcode) {
  HandleScope scope(thread_);
  Code code(&scope, newEmptyCode());
  Object obj1(&scope, runtime_->newStrFromCStr("test"));
  Object obj2(&scope, runtime_->newStrFromCStr("test string"));
  Tuple consts(&scope, runtime_->newTupleWith2(obj1, obj2));
  code.setConsts(*consts);
  const byte bytecode[] = {
      LOAD_CONST, 0, LOAD_CONST, 1, COMPARE_IN_ANAMORPHIC, 0, RETURN_VALUE, 0,
  };
  code.setCode(runtime_->newBytesWithAll(bytecode));

  Object qualname(&scope, Str::empty());
  Module module(&scope, runtime_->findOrCreateMainModule());
  Function function(
      &scope, runtime_->newFunctionWithCode(thread_, qualname, code, module));

  // Update the opcode.
  ASSERT_EQ(Interpreter::call0(thread_, function), Bool::trueObj());

  MutableBytes rewritten_bytecode(&scope, function.rewrittenBytecode());
  EXPECT_EQ(rewritten_bytecode.byteAt(4), COMPARE_IN_STR);

  // Updated opcode returns the same value.
  ASSERT_EQ(Interpreter::call0(thread_, function), Bool::trueObj());
}

TEST_F(InterpreterTest, CompareInAnamorphicWithDictRewritesOpcode) {
  HandleScope scope(thread_);
  Code code(&scope, newEmptyCode());
  Dict dict(&scope, runtime_->newDict());
  Str key(&scope, runtime_->newStrFromCStr("test"));
  word key_hash = strHash(thread_, *key);
  dictAtPut(thread_, dict, key, key_hash, key);
  Tuple consts(&scope, runtime_->newTupleWith2(key, dict));
  code.setConsts(*consts);
  const byte bytecode[] = {
      LOAD_CONST, 0, LOAD_CONST, 1, COMPARE_IN_ANAMORPHIC, 0, RETURN_VALUE, 0,
  };
  code.setCode(runtime_->newBytesWithAll(bytecode));

  Object qualname(&scope, Str::empty());
  Module module(&scope, runtime_->findOrCreateMainModule());
  Function function(
      &scope, runtime_->newFunctionWithCode(thread_, qualname, code, module));

  // Update the opcode.
  ASSERT_EQ(Interpreter::call0(thread_, function), Bool::trueObj());

  MutableBytes rewritten_bytecode(&scope, function.rewrittenBytecode());
  EXPECT_EQ(rewritten_bytecode.byteAt(4), COMPARE_IN_DICT);

  // Updated opcode returns the same value.
  ASSERT_EQ(Interpreter::call0(thread_, function), Bool::trueObj());
}

TEST_F(InterpreterTest, CompareInAnamorphicWithTupleRewritesOpcode) {
  HandleScope scope(thread_);
  Code code(&scope, newEmptyCode());
  Object obj(&scope, runtime_->newStrFromCStr("test"));
  Tuple tuple(&scope, runtime_->newTupleWith1(obj));
  Tuple consts(&scope, runtime_->newTupleWith2(obj, tuple));
  code.setConsts(*consts);
  const byte bytecode[] = {
      LOAD_CONST, 0, LOAD_CONST, 1, COMPARE_IN_ANAMORPHIC, 0, RETURN_VALUE, 0,
  };
  code.setCode(runtime_->newBytesWithAll(bytecode));

  Object qualname(&scope, Str::empty());
  Module module(&scope, runtime_->findOrCreateMainModule());
  Function function(
      &scope, runtime_->newFunctionWithCode(thread_, qualname, code, module));

  // Update the opcode.
  ASSERT_EQ(Interpreter::call0(thread_, function), Bool::trueObj());

  MutableBytes rewritten_bytecode(&scope, function.rewrittenBytecode());
  EXPECT_EQ(rewritten_bytecode.byteAt(4), COMPARE_IN_TUPLE);

  // Updated opcode returns the same value.
  ASSERT_EQ(Interpreter::call0(thread_, function), Bool::trueObj());
}

TEST_F(InterpreterTest, CompareInAnamorphicWithListRewritesOpcode) {
  HandleScope scope(thread_);
  Code code(&scope, newEmptyCode());
  List list(&scope, runtime_->newList());
  Object value0(&scope, runtime_->newStrFromCStr("value0"));
  Object value1(&scope, runtime_->newStrFromCStr("test"));
  listInsert(thread_, list, value0, 0);
  listInsert(thread_, list, value1, 1);
  Tuple consts(&scope, runtime_->newTupleWith2(value1, list));
  code.setConsts(*consts);
  const byte bytecode[] = {
      LOAD_CONST, 0, LOAD_CONST, 1, COMPARE_IN_ANAMORPHIC, 0, RETURN_VALUE, 0,
  };
  code.setCode(runtime_->newBytesWithAll(bytecode));

  Object qualname(&scope, Str::empty());
  Module module(&scope, runtime_->findOrCreateMainModule());
  Function function(
      &scope, runtime_->newFunctionWithCode(thread_, qualname, code, module));

  // Update the opcode.
  ASSERT_EQ(Interpreter::call0(thread_, function), Bool::trueObj());

  MutableBytes rewritten_bytecode(&scope, function.rewrittenBytecode());
  EXPECT_EQ(rewritten_bytecode.byteAt(4), COMPARE_IN_LIST);

  // Updated opcode returns the same value.
  ASSERT_EQ(Interpreter::call0(thread_, function), Bool::trueObj());
}

// To a rich comparison on two instances of the same type.  In each case, the
// method on the left side of the comparison should be used.
TEST_F(InterpreterTest, CompareOpSameType) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
    def __init__(self, value):
        self.value = value

    def __lt__(self, other):
        return self.value < other.value

c10 = C(10)
c20 = C(20)
)")
                   .isError());

  Object left(&scope, mainModuleAt(runtime_, "c10"));
  Object right(&scope, mainModuleAt(runtime_, "c20"));

  Object left_lt_right(&scope, Interpreter::compareOperation(
                                   thread_, CompareOp::LT, left, right));
  EXPECT_EQ(left_lt_right, Bool::trueObj());

  Object right_lt_left(&scope, Interpreter::compareOperation(
                                   thread_, CompareOp::LT, right, left));
  EXPECT_EQ(right_lt_left, Bool::falseObj());
}

TEST_F(InterpreterTest, CompareOpFallback) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
    def __init__(self, value):
        self.value = value

c10 = C(10)
c20 = C(20)
)")
                   .isError());

  Object left(&scope, mainModuleAt(runtime_, "c10"));
  Object right(&scope, mainModuleAt(runtime_, "c20"));

  Object left_eq_right(&scope, Interpreter::compareOperation(
                                   thread_, CompareOp::EQ, left, right));
  EXPECT_EQ(left_eq_right, Bool::falseObj());
  Object left_ne_right(&scope, Interpreter::compareOperation(
                                   thread_, CompareOp::NE, left, right));
  EXPECT_EQ(left_ne_right, Bool::trueObj());

  Object right_eq_left(&scope, Interpreter::compareOperation(
                                   thread_, CompareOp::EQ, left, right));
  EXPECT_EQ(right_eq_left, Bool::falseObj());
  Object right_ne_left(&scope, Interpreter::compareOperation(
                                   thread_, CompareOp::NE, left, right));
  EXPECT_EQ(right_ne_left, Bool::trueObj());
}

TEST_F(InterpreterTest, CompareOpSubclass) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(runtime_, R"(
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

  Object a(&scope, mainModuleAt(runtime_, "a"));
  Object b(&scope, mainModuleAt(runtime_, "b"));
  Object c(&scope, mainModuleAt(runtime_, "c"));

  // Comparisons where rhs is not a subtype of lhs try lhs.__eq__(rhs) first.
  Object a_eq_b(&scope,
                Interpreter::compareOperation(thread_, CompareOp::EQ, a, b));
  EXPECT_EQ(a_eq_b, Bool::falseObj());
  Object called(&scope, mainModuleAt(runtime_, "called"));
  EXPECT_TRUE(isStrEqualsCStr(*called, "A"));

  Object called_name(&scope, Runtime::internStrFromCStr(thread_, "called"));
  Object none(&scope, NoneType::object());
  Module main(&scope, findMainModule(runtime_));
  moduleAtPut(thread_, main, called_name, none);
  Object b_eq_a(&scope,
                Interpreter::compareOperation(thread_, CompareOp::EQ, b, a));
  EXPECT_EQ(b_eq_a, Bool::trueObj());
  called = mainModuleAt(runtime_, "called");
  EXPECT_TRUE(isStrEqualsCStr(*called, "B"));

  moduleAtPut(thread_, main, called_name, none);
  Object c_eq_a(&scope,
                Interpreter::compareOperation(thread_, CompareOp::EQ, c, a));
  EXPECT_EQ(c_eq_a, Bool::trueObj());
  called = mainModuleAt(runtime_, "called");
  EXPECT_TRUE(isStrEqualsCStr(*called, "C"));

  // When rhs is a subtype of lhs, only rhs.__eq__(rhs) is tried.
  moduleAtPut(thread_, main, called_name, none);
  Object a_eq_c(&scope,
                Interpreter::compareOperation(thread_, CompareOp::EQ, a, c));
  EXPECT_EQ(a_eq_c, Bool::trueObj());
  called = mainModuleAt(runtime_, "called");
  EXPECT_TRUE(isStrEqualsCStr(*called, "C"));
}

TEST_F(InterpreterTest, CompareOpWithStrsRewritesOpcode) {
  HandleScope scope(thread_);

  Code code(&scope, newEmptyCode());
  Object obj1(&scope, runtime_->newStrFromCStr("abc"));
  Object obj2(&scope, runtime_->newStrFromCStr("def"));
  Tuple consts(&scope, runtime_->newTupleWith2(obj1, obj2));
  code.setConsts(*consts);
  const byte bytecode[] = {
      LOAD_CONST,   0,
      LOAD_CONST,   1,
      COMPARE_OP,   static_cast<byte>(CompareOp::EQ),
      RETURN_VALUE, 0,
  };
  code.setCode(runtime_->newBytesWithAll(bytecode));

  Object qualname(&scope, Str::empty());
  Module module(&scope, runtime_->findOrCreateMainModule());
  Function function(
      &scope, runtime_->newFunctionWithCode(thread_, qualname, code, module));

  // Update the opcode.
  ASSERT_EQ(Interpreter::call0(thread_, function), Bool::falseObj());

  MutableBytes rewritten_bytecode(&scope, function.rewrittenBytecode());
  EXPECT_EQ(rewritten_bytecode.byteAt(4), COMPARE_EQ_STR);

  // Updated opcode returns the same value.
  ASSERT_EQ(Interpreter::call0(thread_, function), Bool::falseObj());
}

TEST_F(InterpreterTest, CompareOpSmallIntsRewritesOpcode) {
  HandleScope scope(thread_);

  word left = 7;
  word right = -13;
  Code code(&scope, newEmptyCode());
  Object obj1(&scope, runtime_->newInt(left));
  Object obj2(&scope, runtime_->newInt(right));
  Tuple consts(&scope, runtime_->newTupleWith2(obj1, obj2));
  code.setConsts(*consts);
  const byte bytecode[] = {
      LOAD_CONST,   0,
      LOAD_CONST,   1,
      COMPARE_OP,   static_cast<byte>(CompareOp::LT),
      RETURN_VALUE, 0,
  };
  code.setCode(runtime_->newBytesWithAll(bytecode));

  Object qualname(&scope, Str::empty());
  Module module(&scope, runtime_->findOrCreateMainModule());
  Function function(
      &scope, runtime_->newFunctionWithCode(thread_, qualname, code, module));

  // Update the opcode.
  ASSERT_EQ(Interpreter::call0(thread_, function), Bool::falseObj());

  MutableBytes rewritten_bytecode(&scope, function.rewrittenBytecode());
  EXPECT_EQ(rewritten_bytecode.byteAt(4), COMPARE_LT_SMALLINT);

  // Updated opcode returns the same value.
  ASSERT_EQ(Interpreter::call0(thread_, function), Bool::falseObj());
}

TEST_F(InterpreterTest, CompareOpWithSmallInts) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(a, b):
    return a == b
)")
                   .isError());
  Function function(&scope, mainModuleAt(runtime_, "foo"));
  MutableBytes rewritten(&scope, function.rewrittenBytecode());
  ASSERT_EQ(rewritten.byteAt(4), COMPARE_OP_ANAMORPHIC);

  SmallInt left(&scope, SmallInt::fromWord(7));
  SmallInt right(&scope, SmallInt::fromWord(-13));

  rewritten.byteAtPut(4, COMPARE_EQ_SMALLINT);
  left = SmallInt::fromWord(7);
  right = SmallInt::fromWord(-13);
  // 7 == -13
  EXPECT_EQ(Interpreter::call2(thread_, function, left, right),
            Bool::falseObj());
  // 7 == 7
  left = SmallInt::fromWord(7);
  right = SmallInt::fromWord(7);
  EXPECT_EQ(Interpreter::call2(thread_, function, left, right),
            Bool::trueObj());
  EXPECT_EQ(rewritten.byteAt(4), COMPARE_EQ_SMALLINT);

  rewritten.byteAtPut(4, COMPARE_NE_SMALLINT);
  left = SmallInt::fromWord(7);
  right = SmallInt::fromWord(7);
  // 7 != 7
  EXPECT_EQ(Interpreter::call2(thread_, function, left, right),
            Bool::falseObj());
  left = SmallInt::fromWord(7);
  right = SmallInt::fromWord(-13);
  // 7 != -13
  EXPECT_EQ(Interpreter::call2(thread_, function, left, right),
            Bool::trueObj());
  EXPECT_EQ(rewritten.byteAt(4), COMPARE_NE_SMALLINT);

  rewritten.byteAtPut(4, COMPARE_GT_SMALLINT);
  left = SmallInt::fromWord(10);
  right = SmallInt::fromWord(10);
  // 10 > 10
  EXPECT_EQ(Interpreter::call2(thread_, function, left, right),
            Bool::falseObj());
  left = SmallInt::fromWord(10);
  right = SmallInt::fromWord(-10);
  // 10 > -10
  EXPECT_EQ(Interpreter::call2(thread_, function, left, right),
            Bool::trueObj());
  EXPECT_EQ(rewritten.byteAt(4), COMPARE_GT_SMALLINT);

  rewritten.byteAtPut(4, COMPARE_GE_SMALLINT);
  left = SmallInt::fromWord(-10);
  right = SmallInt::fromWord(10);
  // -10 >= 10
  EXPECT_EQ(Interpreter::call2(thread_, function, left, right),
            Bool::falseObj());
  left = SmallInt::fromWord(10);
  right = SmallInt::fromWord(10);
  // 10 >= 10
  EXPECT_EQ(Interpreter::call2(thread_, function, left, right),
            Bool::trueObj());
  left = SmallInt::fromWord(11);
  right = SmallInt::fromWord(10);
  // 11 > = 10
  EXPECT_EQ(Interpreter::call2(thread_, function, left, right),
            Bool::trueObj());
  EXPECT_EQ(rewritten.byteAt(4), COMPARE_GE_SMALLINT);

  rewritten.byteAtPut(4, COMPARE_LT_SMALLINT);
  left = SmallInt::fromWord(10);
  right = SmallInt::fromWord(-10);
  // 10 < -10
  EXPECT_EQ(Interpreter::call2(thread_, function, left, right),
            Bool::falseObj());
  left = SmallInt::fromWord(-10);
  right = SmallInt::fromWord(10);
  // -10 < 10
  EXPECT_EQ(Interpreter::call2(thread_, function, left, right),
            Bool::trueObj());
  EXPECT_EQ(rewritten.byteAt(4), COMPARE_LT_SMALLINT);

  rewritten.byteAtPut(4, COMPARE_LE_SMALLINT);
  left = SmallInt::fromWord(10);
  right = SmallInt::fromWord(-10);
  // 10 <= -10
  EXPECT_EQ(Interpreter::call2(thread_, function, left, right),
            Bool::falseObj());
  left = SmallInt::fromWord(10);
  right = SmallInt::fromWord(10);
  // 10 <= 10
  EXPECT_EQ(Interpreter::call2(thread_, function, left, right),
            Bool::trueObj());
  left = SmallInt::fromWord(9);
  right = SmallInt::fromWord(10);
  // 9 <= 10
  EXPECT_EQ(Interpreter::call2(thread_, function, left, right),
            Bool::trueObj());
  EXPECT_EQ(rewritten.byteAt(4), COMPARE_LE_SMALLINT);
}

TEST_F(InterpreterTest, CompareOpWithSmallIntsRevertsBackToCompareOp) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(a, b):
    return a == b
)")
                   .isError());
  Function function(&scope, mainModuleAt(runtime_, "foo"));
  MutableBytes rewritten(&scope, function.rewrittenBytecode());
  ASSERT_EQ(rewritten.byteAt(4), COMPARE_OP_ANAMORPHIC);

  LargeInt left(&scope, runtime_->newInt(SmallInt::kMaxValue + 1));
  LargeInt right(&scope, runtime_->newInt(SmallInt::kMaxValue + 1));

  rewritten.byteAtPut(4, COMPARE_EQ_SMALLINT);
  // LARGE_SMALL_INT == SMALL_INT
  EXPECT_EQ(Interpreter::call2(thread_, function, left, right),
            Bool::trueObj());
  EXPECT_EQ(rewritten.byteAt(4), COMPARE_OP_MONOMORPHIC);
}

TEST_F(InterpreterTest, CompareOpSetMethodSetsMethod) {
  HandleScope scope(thread_);
  Object v0(&scope, runtime_->newInt(39));
  Object v1(&scope, runtime_->newInt(11));
  Object method(&scope, NoneType::object());
  BinaryOpFlags flags;
  EXPECT_EQ(Interpreter::compareOperationSetMethod(thread_, CompareOp::LT, v0,
                                                   v1, &method, &flags),
            Bool::falseObj());
  EXPECT_TRUE(method.isFunction());
  EXPECT_EQ(flags, kBinaryOpNotImplementedRetry);

  Object v2(&scope, runtime_->newInt(3));
  Object v3(&scope, runtime_->newInt(8));
  ASSERT_EQ(v0.layoutId(), v2.layoutId());
  ASSERT_EQ(v1.layoutId(), v3.layoutId());
  EXPECT_EQ(
      Interpreter::binaryOperationWithMethod(thread_, *method, flags, *v2, *v3),
      Bool::trueObj());
}

TEST_F(InterpreterTest, CompareOpSetMethodSetsReverseMethod) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
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

  Object a1(&scope, mainModuleAt(runtime_, "a1"));
  Object b1(&scope, mainModuleAt(runtime_, "b1"));
  Object method(&scope, NoneType::object());
  BinaryOpFlags flags;
  Object result_obj(
      &scope, Interpreter::compareOperationSetMethod(thread_, CompareOp::LE, a1,
                                                     b1, &method, &flags));
  EXPECT_TRUE(method.isFunction());
  EXPECT_EQ(flags, kBinaryOpReflected | kBinaryOpNotImplementedRetry);
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 2);
  EXPECT_EQ(result.at(0), b1);
  EXPECT_EQ(result.at(1), a1);

  Object a2(&scope, mainModuleAt(runtime_, "a2"));
  Object b2(&scope, mainModuleAt(runtime_, "b2"));
  ASSERT_EQ(a1.layoutId(), a2.layoutId());
  ASSERT_EQ(b1.layoutId(), b2.layoutId());
  result_obj =
      Interpreter::binaryOperationWithMethod(thread_, *method, flags, *a2, *b2);
  ASSERT_TRUE(result_obj.isTuple());
  result = *result_obj;
  ASSERT_EQ(result.length(), 2);
  EXPECT_EQ(result.at(0), b2);
  EXPECT_EQ(result.at(1), a2);
}

TEST_F(InterpreterTest,
       CompareOpSetMethodSetsReverseMethodNotImplementedRetry) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
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
  Object v0(&scope, mainModuleAt(runtime_, "v0"));
  Object v1(&scope, mainModuleAt(runtime_, "v1"));
  Object v2(&scope, mainModuleAt(runtime_, "v2"));
  Object v3(&scope, mainModuleAt(runtime_, "v3"));
  Object method(&scope, NoneType::object());
  BinaryOpFlags flags;
  Object result_obj(
      &scope, Interpreter::compareOperationSetMethod(thread_, CompareOp::LE, v0,
                                                     v1, &method, &flags));
  EXPECT_TRUE(method.isFunction());
  EXPECT_EQ(flags, kBinaryOpReflected | kBinaryOpNotImplementedRetry);
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 2);
  EXPECT_EQ(result.at(0), v1);
  EXPECT_EQ(result.at(1), v0);

  ASSERT_EQ(v0.layoutId(), v2.layoutId());
  ASSERT_EQ(v1.layoutId(), v3.layoutId());
  result_obj =
      Interpreter::binaryOperationWithMethod(thread_, *method, flags, *v2, *v3);
  ASSERT_TRUE(result_obj.isTuple());
  result = *result_obj;
  ASSERT_EQ(result.length(), 2);
  EXPECT_EQ(result.at(0), v3);
  EXPECT_EQ(result.at(1), v2);
}

TEST_F(InterpreterTest,
       CompareOpInvokesLeftMethodWhenReflectedMethodReturnsNotImplemented) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
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

  EXPECT_TRUE(isStrEqualsCStr(mainModuleAt(runtime_, "result"), "C.__ge__"));
  EXPECT_TRUE(
      isStrEqualsCStr(mainModuleAt(runtime_, "trace"), "D.__le__,C.__ge__,"));
}

TEST_F(
    InterpreterTest,
    CompareOpCachedInsertsDependencyForBothOperandsTypesAppropriateAttributes) {
  HandleScope scope(thread_);
  EXPECT_FALSE(runFromCStr(runtime_, R"(
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
      isStrEqualsCStr(mainModuleAt(runtime_, "result"), "from class A"));

  Function cache_compare_op(&scope, mainModuleAt(runtime_, "cache_compare_op"));
  MutableTuple caches(&scope, cache_compare_op.caches());
  Object a_obj(&scope, mainModuleAt(runtime_, "a"));
  Object b_obj(&scope, mainModuleAt(runtime_, "b"));
  BinaryOpFlags flag;
  EXPECT_EQ(
      icLookupBinaryOp(*caches, 0, a_obj.layoutId(), b_obj.layoutId(), &flag),
      mainModuleAt(runtime_, "A__ge__"));

  // Verify that A.__ge__ has the dependent.
  Type a_type(&scope, mainModuleAt(runtime_, "A"));
  Object left_op_name(&scope, runtime_->symbols()->at(ID(__ge__)));
  Object a_type_attr(&scope, typeValueCellAt(a_type, left_op_name));
  ASSERT_TRUE(a_type_attr.isValueCell());
  ASSERT_TRUE(ValueCell::cast(*a_type_attr).dependencyLink().isWeakLink());
  EXPECT_EQ(
      WeakLink::cast(ValueCell::cast(*a_type_attr).dependencyLink()).referent(),
      *cache_compare_op);

  // Verify that B.__le__ has the dependent.
  Type b_type(&scope, mainModuleAt(runtime_, "B"));
  Object right_op_name(&scope, runtime_->symbols()->at(ID(__le__)));
  Object b_type_attr(&scope, typeValueCellAt(b_type, right_op_name));
  ASSERT_TRUE(b_type_attr.isValueCell());
  ASSERT_TRUE(ValueCell::cast(*b_type_attr).dependencyLink().isWeakLink());
  EXPECT_EQ(
      WeakLink::cast(ValueCell::cast(*b_type_attr).dependencyLink()).referent(),
      *cache_compare_op);
}

TEST_F(InterpreterTest, DoStoreFastStoresValue) {
  HandleScope scope(thread_);

  Code code(&scope, newEmptyCode());
  Object obj(&scope, SmallInt::fromWord(1111));
  Tuple consts(&scope, runtime_->newTupleWith1(obj));
  code.setConsts(*consts);
  code.setNlocals(2);
  const byte bytecode[] = {LOAD_CONST, 0, STORE_FAST,   1,
                           LOAD_FAST,  1, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));

  EXPECT_TRUE(isIntEqualsWord(runCodeNoBytecodeRewriting(code), 1111));
}

TEST_F(InterpreterTest, DoLoadFastReverseLoadsValue) {
  HandleScope scope(thread_);

  Code code(&scope, newEmptyCode());
  Object obj1(&scope, SmallInt::fromWord(1));
  Object obj2(&scope, SmallInt::fromWord(22));
  Object obj3(&scope, SmallInt::fromWord(333));
  Object obj4(&scope, SmallInt::fromWord(4444));
  Tuple consts(&scope, runtime_->newTupleWith4(obj1, obj2, obj3, obj4));
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
  code.setCode(runtime_->newBytesWithAll(bytecode));

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
  Object obj(&scope, SmallInt::fromWord(42));
  Tuple consts(&scope, runtime_->newTupleWith1(obj));
  code.setConsts(*consts);
  Object obj1(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  Object obj2(&scope, Runtime::internStrFromCStr(thread_, "bar"));
  Object obj3(&scope, Runtime::internStrFromCStr(thread_, "baz"));
  Tuple varnames(&scope, runtime_->newTupleWith3(obj1, obj2, obj3));
  code.setVarnames(*varnames);
  code.setNlocals(3);
  const byte bytecode[] = {
      LOAD_CONST,  0, STORE_FAST,        0, LOAD_CONST,   0, STORE_FAST, 2,
      DELETE_FAST, 2, LOAD_FAST_REVERSE, 0, RETURN_VALUE, 0,
  };
  code.setCode(runtime_->newBytesWithAll(bytecode));

  EXPECT_TRUE(raisedWithStr(
      runCodeNoBytecodeRewriting(code), LayoutId::kUnboundLocalError,
      "local variable 'baz' referenced before assignment"));
}

TEST_F(InterpreterTest, DoStoreFastReverseStoresValue) {
  HandleScope scope(thread_);

  Code code(&scope, newEmptyCode());
  Object obj1(&scope, SmallInt::fromWord(1));
  Object obj2(&scope, SmallInt::fromWord(22));
  Object obj3(&scope, SmallInt::fromWord(333));
  Object obj4(&scope, SmallInt::fromWord(4444));
  Tuple consts(&scope, runtime_->newTupleWith4(obj1, obj2, obj3, obj4));
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
  code.setCode(runtime_->newBytesWithAll(bytecode));

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
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, "1[5] = 'foo'"),
                            LayoutId::kTypeError,
                            "'int' object does not support item assignment"));
}

TEST_F(InterpreterTest, DoStoreSubscrWithDescriptorPropagatesException) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
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
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, "del 1[5]"),
                            LayoutId::kTypeError,
                            "'int' object does not support item deletion"));
}

TEST_F(InterpreterTest, DoDeleteSubscrWithDescriptorPropagatesException) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
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
  List list(&scope, runtime_->newList());
  Int one(&scope, runtime_->newInt(1));
  runtime_->listEnsureCapacity(thread_, list, 1);
  list.setNumItems(1);
  list.atPut(0, *one);
  Object obj1(&scope, SmallInt::fromWord(42));
  Object obj3(&scope, SmallInt::fromWord(0));
  Tuple consts(&scope, runtime_->newTupleWith3(obj1, list, obj3));
  code.setConsts(*consts);

  Tuple varnames(&scope, runtime_->newTuple(0));
  code.setVarnames(*varnames);
  code.setNlocals(0);
  const byte bytecode[] = {
      LOAD_CONST,    0, LOAD_CONST,   1, LOAD_CONST, 2,
      DELETE_SUBSCR, 0, RETURN_VALUE, 0,
  };
  code.setCode(runtime_->newBytesWithAll(bytecode));

  Object result_obj(&scope, runCode(code));
  ASSERT_TRUE(result_obj.isInt());
  Int result(&scope, *result_obj);
  EXPECT_EQ(result.asWord(), 42);
  EXPECT_EQ(list.numItems(), 0);
}

TEST_F(InterpreterTest, GetIterWithSequenceReturnsIterator) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class Sequence:
    def __getitem__(s, i):
        return ("foo", "bar")[i]

seq = Sequence()
)")
                   .isError());
  HandleScope scope(thread_);

  Code code(&scope, newEmptyCode());
  Object obj(&scope, mainModuleAt(runtime_, "seq"));
  Tuple consts(&scope, runtime_->newTupleWith1(obj));
  code.setConsts(*consts);

  Tuple varnames(&scope, runtime_->newTuple(0));
  code.setVarnames(*varnames);
  code.setNlocals(0);
  const byte bytecode[] = {
      LOAD_CONST, 0, GET_ITER, 0, RETURN_VALUE, 0,
  };
  code.setCode(runtime_->newBytesWithAll(bytecode));

  Object result_obj(&scope, runCode(code));
  EXPECT_TRUE(runtime_->isIterator(thread_, result_obj));
  Type result_type(&scope, runtime_->typeOf(*result_obj));
  EXPECT_TRUE(isStrEqualsCStr(result_type.name(), "iterator"));
}

TEST_F(InterpreterTest,
       GetIterWithRaisingDescriptorDunderIterPropagatesException) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
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

  ASSERT_FALSE(runFromCStr(runtime_, R"(
a = {1, 2}

b = 1
c = 3
)")
                   .isError());

  Object container(&scope, mainModuleAt(runtime_, "a"));
  Object b(&scope, mainModuleAt(runtime_, "b"));
  Object c(&scope, mainModuleAt(runtime_, "c"));
  Object contains_true(&scope,
                       Interpreter::sequenceContains(thread_, b, container));
  Object contains_false(&scope,
                        Interpreter::sequenceContains(thread_, c, container));
  EXPECT_EQ(contains_true, Bool::trueObj());
  EXPECT_EQ(contains_false, Bool::falseObj());
}

TEST_F(InterpreterTest, SequenceIterSearchWithNoDunderIterRaisesTypeError) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C: pass
container = C()
)")
                   .isError());
  Object container(&scope, mainModuleAt(runtime_, "container"));
  Object val(&scope, NoneType::object());
  Object result(&scope,
                Interpreter::sequenceIterSearch(thread_, val, container));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST_F(InterpreterTest,
       SequenceIterSearchWithNonCallableDunderIterRaisesTypeError) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  __iter__ = None
container = C()
)")
                   .isError());
  Object container(&scope, mainModuleAt(runtime_, "container"));
  Object val(&scope, NoneType::object());
  Object result(&scope,
                Interpreter::sequenceIterSearch(thread_, val, container));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST_F(InterpreterTest, SequenceIterSearchWithNoDunderNextRaisesTypeError) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class D: pass
class C:
  def __iter__(self):
    return D()
container = C()
)")
                   .isError());
  Object container(&scope, mainModuleAt(runtime_, "container"));
  Object val(&scope, NoneType::object());
  Object result(&scope,
                Interpreter::sequenceIterSearch(thread_, val, container));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST_F(InterpreterTest,
       SequenceIterSearchWithNonCallableDunderNextRaisesTypeError) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class D:
  __next__ = None
class C:
  def __iter__(self):
    return D()
container = C()
)")
                   .isError());
  Object container(&scope, mainModuleAt(runtime_, "container"));
  Object val(&scope, NoneType::object());
  Object result(&scope,
                Interpreter::sequenceIterSearch(thread_, val, container));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST_F(InterpreterTest, SequenceIterSearchWithListReturnsTrue) {
  HandleScope scope(thread_);
  List container(&scope, listFromRange(1, 3));
  Object val(&scope, SmallInt::fromWord(2));
  Object result(&scope,
                Interpreter::sequenceIterSearch(thread_, val, container));
  ASSERT_FALSE(result.isError());
  EXPECT_EQ(result, Bool::trueObj());
}

TEST_F(InterpreterTest, SequenceIterSearchWithListReturnsFalse) {
  HandleScope scope(thread_);
  Object container(&scope, listFromRange(1, 3));
  Object val(&scope, SmallInt::fromWord(5));
  Object result(&scope,
                Interpreter::sequenceIterSearch(thread_, val, container));
  ASSERT_FALSE(result.isError());
  EXPECT_EQ(result, Bool::falseObj());
}

TEST_F(InterpreterTest, SequenceIterSearchWithSequenceSearchesIterator) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class Seq:
    def __getitem__(s, i):
        return ("foo", "bar", 42)[i]

seq_iter = Seq()
)")
                   .isError());
  Object seq_iter(&scope, mainModuleAt(runtime_, "seq_iter"));
  Object obj_in_seq(&scope, SmallInt::fromWord(42));
  Object contains_true(
      &scope, Interpreter::sequenceIterSearch(thread_, obj_in_seq, seq_iter));
  EXPECT_EQ(contains_true, Bool::trueObj());
  Object obj_not_in_seq(&scope, NoneType::object());
  Object contains_false(&scope, Interpreter::sequenceIterSearch(
                                    thread_, obj_not_in_seq, seq_iter));
  EXPECT_EQ(contains_false, Bool::falseObj());
}

TEST_F(InterpreterTest,
       SequenceIterSearchWithIterThatRaisesPropagatesException) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  def __iter__(self):
    raise ZeroDivisionError("boom")
container = C()
)")
                   .isError());
  Object container(&scope, mainModuleAt(runtime_, "container"));
  Object val(&scope, SmallInt::fromWord(5));
  Object result(&scope,
                Interpreter::sequenceIterSearch(thread_, val, container));
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
  ASSERT_FALSE(runFromCStr(runtime_, src).isError());
  Object a(&scope, mainModuleAt(runtime_, "a"));
  EXPECT_TRUE(isIntEqualsWord(*a, 3));
  Object b(&scope, mainModuleAt(runtime_, "b"));
  EXPECT_TRUE(isIntEqualsWord(*b, 2));
}

TEST_F(InterpreterTest, ContextManagerCallEnterExitOfNotFunctionType) {
  const char* src = R"(
class MyFunction:
  def __init__(self, fn):
    self.fn = fn

  def __get__(self, instance, instance_type):
    return self.fn

a = 1

def my_enter():
  global a
  a = 2

def my_exit(e, t, b):
  global a
  a = 3

class Foo:
  __enter__ = MyFunction(my_enter)
  __exit__ = MyFunction(my_exit)

b = 0
with Foo():
  b = a
)";
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, src).isError());
  Object a(&scope, mainModuleAt(runtime_, "a"));
  EXPECT_TRUE(isIntEqualsWord(*a, 3));
  Object b(&scope, mainModuleAt(runtime_, "b"));
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

  Object obj(&scope, SmallInt::fromWord(42));
  Tuple consts(&scope, runtime_->newTupleWith1(obj));
  code.setConsts(*consts);

  Object name(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  Tuple names(&scope, runtime_->newTupleWith1(name));
  code.setNames(*names);
  code.setArgcount(2);
  code.setNlocals(2);
  code.setStacksize(1);

  const byte bytecode[] = {LOAD_CONST, 0, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));

  Object qualname(&scope, Str::empty());
  Module module(&scope, runtime_->findOrCreateMainModule());
  Function callee(
      &scope, runtime_->newFunctionWithCode(thread_, qualname, code, module));

  Object obj1(&scope, SmallInt::fromWord(1));
  Object obj2(&scope, SmallInt::fromWord(2));
  Tuple defaults(&scope, runtime_->newTupleWith2(obj1, obj2));
  callee.setDefaults(*defaults);

  // Save starting value stack top
  RawObject* value_stack_start = thread_->stackPointer();

  // Push function pointer and argument
  thread_->stackPush(*callee);
  thread_->stackPush(SmallInt::fromWord(1));

  // Make sure we got the right result and stack is back where it should be
  EXPECT_TRUE(isIntEqualsWord(Interpreter::call(thread_, 1), 42));
  EXPECT_EQ(value_stack_start, thread_->stackPointer());
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

  Object obj(&scope, SmallInt::fromWord(42));
  Tuple consts(&scope, runtime_->newTupleWith1(obj));
  code.setConsts(*consts);

  Object name(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  Tuple names(&scope, runtime_->newTupleWith1(name));
  code.setNames(*names);
  code.setArgcount(2);
  code.setNlocals(2);
  code.setStacksize(1);

  const byte bytecode[] = {LOAD_CONST, 0, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));

  Object qualname(&scope, Str::empty());
  Module module(&scope, runtime_->findOrCreateMainModule());
  Function callee(
      &scope, runtime_->newFunctionWithCode(thread_, qualname, code, module));

  Object obj1(&scope, SmallInt::fromWord(1));
  Object obj2(&scope, SmallInt::fromWord(2));
  Tuple defaults(&scope, runtime_->newTupleWith2(obj1, obj2));
  callee.setDefaults(*defaults);

  // Save starting value stack top
  RawObject* value_stack_start = thread_->stackPointer();

  // Push function pointer and argument
  Object arg(&scope, SmallInt::fromWord(2));
  Tuple ex(&scope, runtime_->newTupleWith1(arg));
  thread_->stackPush(*callee);
  thread_->stackPush(*ex);

  // Make sure we got the right result and stack is back where it should be
  EXPECT_TRUE(isIntEqualsWord(Interpreter::callEx(thread_, 0), 42));
  EXPECT_EQ(value_stack_start, thread_->stackPointer());
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

  Object obj(&scope, SmallInt::fromWord(42));
  Tuple consts(&scope, runtime_->newTupleWith1(obj));
  code.setConsts(*consts);

  Object name(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  Tuple names(&scope, runtime_->newTupleWith1(name));
  code.setNames(*names);
  code.setArgcount(2);
  code.setNlocals(2);
  code.setStacksize(1);
  Object name1(&scope, Runtime::internStrFromCStr(thread_, "a"));
  Object name2(&scope, Runtime::internStrFromCStr(thread_, "b"));
  Tuple varnames(&scope, runtime_->newTupleWith2(name1, name2));
  code.setVarnames(*varnames);

  const byte bytecode[] = {LOAD_CONST, 0, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));

  Object qualname(&scope, Str::empty());
  Module module(&scope, runtime_->findOrCreateMainModule());
  Function callee(
      &scope, runtime_->newFunctionWithCode(thread_, qualname, code, module));
  Object default1(&scope, SmallInt::fromWord(1));
  Object default2(&scope, SmallInt::fromWord(2));
  Tuple defaults(&scope, runtime_->newTupleWith2(default1, default2));
  callee.setDefaults(*defaults);

  // Save starting value stack top
  RawObject* value_stack_start = thread_->stackPointer();

  // Push function pointer and argument
  Object arg(&scope, Runtime::internStrFromCStr(thread_, "b"));
  Tuple arg_names(&scope, runtime_->newTupleWith1(arg));
  thread_->stackPush(*callee);
  thread_->stackPush(SmallInt::fromWord(4));
  thread_->stackPush(*arg_names);

  // Make sure we got the right result and stack is back where it should be
  EXPECT_TRUE(isIntEqualsWord(Interpreter::callKw(thread_, 1), 42));
  EXPECT_EQ(value_stack_start, thread_->stackPointer());
}

TEST_F(InterpreterTest, LookupMethodInvokesDescriptor) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def f(): pass

class D:
    def __get__(self, obj, owner):
        return f

class C:
    __call__ = D()

c = C()
  )")
                   .isError());
  Object c(&scope, mainModuleAt(runtime_, "c"));
  Object f(&scope, mainModuleAt(runtime_, "f"));
  Object method(&scope, Interpreter::lookupMethod(thread_, c, ID(__call__)));
  EXPECT_EQ(*f, *method);
}

TEST_F(InterpreterTest, PrepareCallableCallUnpacksBoundMethod) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  def foo():
    pass
meth = C().foo
)")
                   .isError());
  Object meth_obj(&scope, mainModuleAt(runtime_, "meth"));
  ASSERT_TRUE(meth_obj.isBoundMethod());

  thread_->stackPush(*meth_obj);
  thread_->stackPush(SmallInt::fromWord(1234));
  ASSERT_EQ(thread_->valueStackSize(), 2);
  word nargs = 1;
  Interpreter::PrepareCallableResult result =
      Interpreter::prepareCallableCall(thread_, nargs, nargs);
  ASSERT_TRUE(result.function.isFunction());
  ASSERT_EQ(result.nargs, 2);
  ASSERT_EQ(thread_->valueStackSize(), 3);
  EXPECT_TRUE(isIntEqualsWord(thread_->stackPeek(0), 1234));
  EXPECT_TRUE(thread_->stackPeek(1).isInstance());
  EXPECT_EQ(thread_->stackPeek(2), result.function);
}

TEST_F(InterpreterTest, CallExWithListSubclassCallsDunderIter) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
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

TEST_F(InterpreterTest, CallFunctionWithInterruptSetReturnsErrorException) {
  auto set_pending_signal = [](Thread* thread, Arguments) -> RawObject {
    thread->runtime()->setPendingSignal(thread, SIGINT);
    return NoneType::object();
  };
  addBuiltin("set_pending_signal", set_pending_signal, {nullptr, 0}, 0);
  EXPECT_FALSE(runFromCStr(runtime_, R"(
executed = False
def foo():
  global executed
  executed = True

def bar():
  set_pending_signal()
  foo()
)")
                   .isError());
  HandleScope scope(thread_);
  Object bar(&scope, mainModuleAt(runtime_, "bar"));
  thread_->stackPush(*bar);
  EXPECT_TRUE(
      raised(Interpreter::call0(thread_, bar), LayoutId::kKeyboardInterrupt));
  Object executed(&scope, mainModuleAt(runtime_, "executed"));
  EXPECT_EQ(executed, Bool::falseObj());
}

TEST_F(InterpreterTest, CallFunctionWithBuiltinRaisesRecursionError) {
  auto abort_builtin = [](Thread*, Arguments) -> RawObject {
    // Should never get here.
    abort();
  };
  addBuiltin("abort", abort_builtin, {nullptr, 0}, 0);
  EXPECT_FALSE(runFromCStr(runtime_, R"(
x = None
def foo():
  global x
  x = 1
  abort()
  x = 2
)")
                   .isError());

  HandleScope scope(thread_);
  Object foo(&scope, mainModuleAt(runtime_, "foo"));

  // Fill stack until we can fit exactly 1 function call.
  RawObject* saved_sp = thread_->stackPointer();
  while (!thread_->wouldStackOverflow(Frame::kSize * 2)) {
    thread_->stackPush(NoneType::object());
  }
  EXPECT_TRUE(
      raised(Interpreter::call0(thread_, foo), LayoutId::kRecursionError));
  Object x(&scope, mainModuleAt(runtime_, "x"));
  EXPECT_TRUE(isIntEqualsWord(*x, 1));
  thread_->setStackPointer(saved_sp);
}

TEST_F(InterpreterTest, CallingUncallableRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, "(1)()"),
                            LayoutId::kTypeError,
                            "'int' object is not callable"));
}

TEST_F(InterpreterTest, CallingUncallableDunderCallRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
class C:
  __call__ = 1

c = C()
c()
  )"),
                            LayoutId::kTypeError,
                            "'int' object is not callable"));
}

TEST_F(InterpreterTest,
       CallingBoundMethodWithNonFunctionDunderFuncCallsDunderFunc) {
  EXPECT_FALSE(runFromCStr(runtime_, R"(
# from types import MethodType
MethodType = method

class C:
  def __call__(self, arg):
    return self, arg

func = C()
instance = object()
bound_method = MethodType(func, instance)
result = bound_method()
  )")
                   .isError());
  CHECK(!thread_->hasPendingException(), "no errors pls");
  HandleScope scope(thread_);
  Object result_obj(&scope, mainModuleAt(runtime_, "result"));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 2);
  Object func(&scope, mainModuleAt(runtime_, "func"));
  EXPECT_EQ(result.at(0), *func);
  Object instance(&scope, mainModuleAt(runtime_, "instance"));
  EXPECT_EQ(result.at(1), *instance);
}

TEST_F(InterpreterTest, CallingNonDescriptorDunderCallRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
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
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
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
  ASSERT_FALSE(runFromCStr(runtime_, R"(
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
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_EQ(*result, SmallInt::fromWord(42));
}

TEST_F(InterpreterTest, IterateOnNonIterable) {
  const char* src = R"(
# Try to iterate on a None object which isn't iterable
a, b = None
)";
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, src), LayoutId::kTypeError,
                            "'NoneType' object is not iterable"));
}

TEST_F(InterpreterTest, DunderIterReturnsNonIterable) {
  const char* src = R"(
class Foo:
  def __iter__(self):
    return 1
a, b = Foo()
)";
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, src), LayoutId::kTypeError,
                            "iter() returned non-iterator of type 'int'"));
}

TEST_F(InterpreterTest, UnpackSequence) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
l = [1, 2, 3]
a, b, c = l
)")
                   .isError());
  Object a(&scope, mainModuleAt(runtime_, "a"));
  Object b(&scope, mainModuleAt(runtime_, "b"));
  Object c(&scope, mainModuleAt(runtime_, "c"));
  EXPECT_TRUE(isIntEqualsWord(*a, 1));
  EXPECT_TRUE(isIntEqualsWord(*b, 2));
  EXPECT_TRUE(isIntEqualsWord(*c, 3));
}

TEST_F(InterpreterTest, UnpackSequenceWithSeqIterator) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class Seq:
  def __getitem__(s, i):
    return ("foo", "bar", 42)[i]
a, b, c = Seq()
)")
                   .isError());
  Object a(&scope, mainModuleAt(runtime_, "a"));
  Object b(&scope, mainModuleAt(runtime_, "b"));
  Object c(&scope, mainModuleAt(runtime_, "c"));
  EXPECT_TRUE(isStrEqualsCStr(*a, "foo"));
  EXPECT_TRUE(isStrEqualsCStr(*b, "bar"));
  EXPECT_TRUE(isIntEqualsWord(*c, 42));
}

TEST_F(InterpreterTest, UnpackSequenceTooFewObjects) {
  const char* src = R"(
l = [1, 2]
a, b, c = l
)";
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, src), LayoutId::kValueError,
                            "not enough values to unpack"));
}

TEST_F(InterpreterTest, UnpackSequenceTooManyObjects) {
  const char* src = R"(
l = [1, 2, 3, 4]
a, b, c = l
)";
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, src), LayoutId::kValueError,
                            "too many values to unpack"));
}

TEST_F(InterpreterTest, UnpackTuple) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
l = (1, 2, 3)
a, b, c = l
)")
                   .isError());
  Object a(&scope, mainModuleAt(runtime_, "a"));
  Object b(&scope, mainModuleAt(runtime_, "b"));
  Object c(&scope, mainModuleAt(runtime_, "c"));
  EXPECT_TRUE(isIntEqualsWord(*a, 1));
  EXPECT_TRUE(isIntEqualsWord(*b, 2));
  EXPECT_TRUE(isIntEqualsWord(*c, 3));
}

TEST_F(InterpreterTest, UnpackTupleTooFewObjects) {
  const char* src = R"(
l = (1, 2)
a, b, c = l
)";
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, src), LayoutId::kValueError,
                            "not enough values to unpack"));
}

TEST_F(InterpreterTest, UnpackTupleTooManyObjects) {
  const char* src = R"(
l = (1, 2, 3, 4)
a, b, c = l
)";
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, src), LayoutId::kValueError,
                            "too many values to unpack"));
}

TEST_F(InterpreterTest, PrintExprInvokesDisplayhook) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
import sys

MY_GLOBAL = 1234

def my_displayhook(value):
  global MY_GLOBAL
  MY_GLOBAL = value

sys.displayhook = my_displayhook
  )")
                   .isError());

  Object unique(&scope, runtime_->newTuple(1));  // unique object

  Code code(&scope, newEmptyCode());
  Object none(&scope, NoneType::object());
  Tuple consts(&scope, runtime_->newTupleWith2(unique, none));
  code.setConsts(*consts);
  code.setNlocals(0);
  const byte bytecode[] = {LOAD_CONST, 0, PRINT_EXPR,   0,
                           LOAD_CONST, 1, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));

  ASSERT_TRUE(runCode(code).isNoneType());

  Object displayhook(&scope, moduleAtByCStr(runtime_, "sys", "displayhook"));
  Object my_displayhook(&scope, mainModuleAt(runtime_, "my_displayhook"));
  EXPECT_EQ(*displayhook, *my_displayhook);

  Object my_global(&scope, mainModuleAt(runtime_, "MY_GLOBAL"));
  EXPECT_EQ(*my_global, *unique);
}

TEST_F(InterpreterTest, PrintExprtDoesntPushToStack) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
import sys

def my_displayhook(value):
  pass

sys.displayhook = my_displayhook
  )")
                   .isError());

  Code code(&scope, newEmptyCode());
  Object obj1(&scope, SmallInt::fromWord(42));
  Object obj2(&scope, SmallInt::fromWord(0));
  Tuple consts(&scope, runtime_->newTupleWith2(obj1, obj2));
  code.setConsts(*consts);

  Tuple varnames(&scope, runtime_->newTuple(0));
  code.setVarnames(*varnames);
  code.setNlocals(0);
  // This bytecode loads 42 onto the stack, along with a value to print.
  // It then returns the top of the stack, which should be 42.
  const byte bytecode[] = {
      LOAD_CONST, 0, LOAD_CONST, 1, PRINT_EXPR, 0, RETURN_VALUE, 0,
  };
  code.setCode(runtime_->newBytesWithAll(bytecode));

  Object result_obj(&scope, runCode(code));
  EXPECT_TRUE(isIntEqualsWord(*result_obj, 42));
}

TEST_F(InterpreterTest, GetAiterCallsAiter) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class AsyncIterable:
  def __aiter__(self):
    return 42

a = AsyncIterable()
)")
                   .isError());

  Object a(&scope, mainModuleAt(runtime_, "a"));

  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_->newTupleWith1(a));
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, GET_AITER, 0, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));

  Object result(&scope, runCode(code));
  EXPECT_TRUE(isIntEqualsWord(*result, 42));
}

TEST_F(InterpreterTest, GetAiterOnNonIterable) {
  HandleScope scope(thread_);
  Code code(&scope, newEmptyCode());
  Object obj(&scope, SmallInt::fromWord(123));
  Tuple consts(&scope, runtime_->newTupleWith1(obj));
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, GET_AITER, 0, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));

  Object result(&scope, runCode(code));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST_F(InterpreterTest, BeforeAsyncWithCallsDunderAenter) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
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
  Object manager(&scope, mainModuleAt(runtime_, "manager"));
  Object main_obj(&scope, findMainModule(runtime_));
  ASSERT_TRUE(main_obj.isModule());
  Module main(&scope, *main_obj);

  Code code(&scope, newEmptyCode());
  code.setNlocals(0);
  Object obj(&scope, SmallInt::fromWord(42));
  Tuple consts(&scope, runtime_->newTupleWith2(obj, manager));
  code.setConsts(*consts);
  Object name(&scope, Runtime::internStrFromCStr(thread_, "manager"));
  Tuple names(&scope, runtime_->newTupleWith1(name));
  code.setNames(*names);
  const byte bytecode[] = {LOAD_CONST, 1, BEFORE_ASYNC_WITH, 0, POP_TOP, 0,
                           LOAD_CONST, 0, RETURN_VALUE,      0};
  code.setCode(runtime_->newBytesWithAll(bytecode));
  code.setFlags(Code::Flags::kNofree);

  Dict locals(&scope, runtime_->newDict());
  EXPECT_TRUE(isIntEqualsWord(thread_->exec(code, main, locals), 42));
  Object enter(&scope, mainModuleAt(runtime_, "enter"));
  EXPECT_EQ(*enter, *manager);
  Object exit(&scope, mainModuleAt(runtime_, "exit"));
  EXPECT_EQ(*exit, NoneType::object());
}

TEST_F(InterpreterTest, BeforeAsyncWithRaisesAttributeErrorIfAexitNotDefined) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class M:
  pass

manager = M()
  )")
                   .isError());

  Code code(&scope, newEmptyCode());
  code.setNlocals(0);
  Tuple consts(&scope, runtime_->newTuple(1));
  consts.atPut(0, mainModuleAt(runtime_, "manager"));
  code.setConsts(*consts);
  Tuple names(&scope, runtime_->newTuple(1));
  names.atPut(0, Runtime::internStrFromCStr(thread_, "manager"));
  code.setNames(*names);
  const byte bytecode[] = {LOAD_CONST, 0, BEFORE_ASYNC_WITH, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));

  EXPECT_TRUE(raisedWithStr(runCode(code), LayoutId::kAttributeError,
                            "'M' object has no attribute '__aexit__'"));
}

TEST_F(InterpreterTest, BeforeAsyncWithRaisesAttributeErrorIfAenterNotDefined) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class M:
  def __aexit__(self):
    pass

manager = M()
  )")
                   .isError());

  Code code(&scope, newEmptyCode());
  code.setNlocals(0);
  Tuple consts(&scope, runtime_->newTuple(1));
  consts.atPut(0, mainModuleAt(runtime_, "manager"));
  code.setConsts(*consts);
  Tuple names(&scope, runtime_->newTuple(1));
  names.atPut(0, Runtime::internStrFromCStr(thread_, "manager"));
  code.setNames(*names);
  const byte bytecode[] = {LOAD_CONST, 0, BEFORE_ASYNC_WITH, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));

  EXPECT_TRUE(raisedWithStr(runCode(code), LayoutId::kAttributeError,
                            "'M' object has no attribute '__aenter__'"));
}

TEST_F(InterpreterTest,
       BeforeAsyncWithPropagatesExceptionIfResolvingAexitDynamicallyRaises) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class A:
  def __get__(self, obj, type=None):
    raise RuntimeError("foo")

class M:
  __aexit__ = A()

  async def __aenter__(self):
    pass

manager = M()
  )")
                   .isError());

  Code code(&scope, newEmptyCode());
  code.setNlocals(0);
  Tuple consts(&scope, runtime_->newTuple(1));
  consts.atPut(0, mainModuleAt(runtime_, "manager"));
  code.setConsts(*consts);
  Tuple names(&scope, runtime_->newTuple(1));
  names.atPut(0, Runtime::internStrFromCStr(thread_, "manager"));
  code.setNames(*names);
  const byte bytecode[] = {LOAD_CONST, 0, BEFORE_ASYNC_WITH, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));

  EXPECT_TRUE(raisedWithStr(runCode(code), LayoutId::kRuntimeError, "foo"));
}

TEST_F(InterpreterTest,
       BeforeAsyncWithPropagatesExceptionIfResolvingAenterDynamicallyRaises) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class A:
  def __get__(self, obj, type=None):
    raise RuntimeError("foo")

class M:
  __aenter__ = A()

  async def __aexit__(self, a, b, c):
    pass

manager = M()
  )")
                   .isError());

  Code code(&scope, newEmptyCode());
  code.setNlocals(0);
  Tuple consts(&scope, runtime_->newTuple(1));
  consts.atPut(0, mainModuleAt(runtime_, "manager"));
  code.setConsts(*consts);
  Tuple names(&scope, runtime_->newTuple(1));
  names.atPut(0, Runtime::internStrFromCStr(thread_, "manager"));
  code.setNames(*names);
  const byte bytecode[] = {LOAD_CONST, 0, BEFORE_ASYNC_WITH, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));

  EXPECT_TRUE(raisedWithStr(runCode(code), LayoutId::kRuntimeError, "foo"));
}

TEST_F(InterpreterTest, SetupAsyncWithPushesBlock) {
  HandleScope scope(thread_);

  Code code(&scope, newEmptyCode());
  Object obj(&scope, SmallInt::fromWord(42));
  Object none(&scope, NoneType::object());
  Tuple consts(&scope, runtime_->newTupleWith2(obj, none));
  code.setConsts(*consts);
  code.setNlocals(0);
  const byte bytecode[] = {
      LOAD_CONST, 0, LOAD_CONST,   1, SETUP_ASYNC_WITH, 0,
      POP_BLOCK,  0, RETURN_VALUE, 0,
  };
  code.setCode(runtime_->newBytesWithAll(bytecode));
  EXPECT_EQ(runCode(code), SmallInt::fromWord(42));
}

TEST_F(InterpreterTest, UnpackSequenceEx) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
l = [1, 2, 3, 4, 5, 6, 7]
a, b, c, *d, e, f, g  = l
)")
                   .isError());
  Object a(&scope, mainModuleAt(runtime_, "a"));
  Object b(&scope, mainModuleAt(runtime_, "b"));
  Object c(&scope, mainModuleAt(runtime_, "c"));
  EXPECT_TRUE(isIntEqualsWord(*a, 1));
  EXPECT_TRUE(isIntEqualsWord(*b, 2));
  EXPECT_TRUE(isIntEqualsWord(*c, 3));

  Object d(&scope, mainModuleAt(runtime_, "d"));
  ASSERT_TRUE(d.isList());
  List list(&scope, *d);
  EXPECT_EQ(list.numItems(), 1);
  EXPECT_TRUE(isIntEqualsWord(list.at(0), 4));

  Object e(&scope, mainModuleAt(runtime_, "e"));
  Object f(&scope, mainModuleAt(runtime_, "f"));
  Object g(&scope, mainModuleAt(runtime_, "g"));
  EXPECT_TRUE(isIntEqualsWord(*e, 5));
  EXPECT_TRUE(isIntEqualsWord(*f, 6));
  EXPECT_TRUE(isIntEqualsWord(*g, 7));
}

TEST_F(InterpreterTest, UnpackSequenceExWithSeqIterator) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class Seq:
  def __getitem__(s, i):
    return ("foo", "bar", 42)[i]
a, *b = Seq()
)")
                   .isError());
  EXPECT_TRUE(isStrEqualsCStr(mainModuleAt(runtime_, "a"), "foo"));
  Object b(&scope, mainModuleAt(runtime_, "b"));
  EXPECT_PYLIST_EQ(b, {"bar", 42});
}

TEST_F(InterpreterTest, UnpackSequenceExWithNoElementsAfter) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
l = [1, 2, 3, 4]
a, b, *c = l
)")
                   .isError());
  Object a(&scope, mainModuleAt(runtime_, "a"));
  Object b(&scope, mainModuleAt(runtime_, "b"));
  Object c(&scope, mainModuleAt(runtime_, "c"));
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
  ASSERT_FALSE(runFromCStr(runtime_, R"(
l = [1, 2, 3, 4]
*a, b, c = l
)")
                   .isError());
  Object a(&scope, mainModuleAt(runtime_, "a"));
  Object b(&scope, mainModuleAt(runtime_, "b"));
  Object c(&scope, mainModuleAt(runtime_, "c"));
  ASSERT_TRUE(a.isList());
  List list(&scope, *a);
  ASSERT_EQ(list.numItems(), 2);
  EXPECT_TRUE(isIntEqualsWord(list.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(list.at(1), 2));

  EXPECT_TRUE(isIntEqualsWord(*b, 3));
  EXPECT_TRUE(isIntEqualsWord(*c, 4));
}

TEST_F(InterpreterTest, BuildMapCallsDunderHashAndPropagatesException) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
class C:
  def __hash__(self):
    raise ValueError('foo')
d = {C(): 4}
)"),
                            LayoutId::kValueError, "foo"));
}

TEST_F(InterpreterTest, BuildMapUnpackWithDict) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
d = {**{'a': 1, 'b': 2}, 'c': 3, **{'d': 4}}
)")
                   .isError());

  Object d(&scope, mainModuleAt(runtime_, "d"));
  ASSERT_TRUE(d.isDict());

  Dict dict(&scope, *d);
  EXPECT_EQ(dict.numItems(), 4);

  Str name(&scope, runtime_->newStrFromCStr("a"));
  Object el0(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el0, 1));

  name = runtime_->newStrFromCStr("b");
  Object el1(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el1, 2));

  name = runtime_->newStrFromCStr("c");
  Object el2(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el2, 3));

  name = runtime_->newStrFromCStr("d");
  Object el3(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el3, 4));
}

TEST_F(InterpreterTest, BuildMapUnpackWithListKeysMapping) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
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

  Object d(&scope, mainModuleAt(runtime_, "d"));
  ASSERT_TRUE(d.isDict());

  Dict dict(&scope, *d);
  EXPECT_EQ(dict.numItems(), 4);

  Str name(&scope, runtime_->newStrFromCStr("a"));
  Object el0(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el0, 1));

  name = runtime_->newStrFromCStr("b");
  Object el1(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el1, 2));

  name = runtime_->newStrFromCStr("c");
  Object el2(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el2, 3));

  name = runtime_->newStrFromCStr("d");
  Object el3(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el3, 4));
}

TEST_F(InterpreterTest, BuildMapUnpackWithTupleKeysMapping) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
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

  Object d(&scope, mainModuleAt(runtime_, "d"));
  ASSERT_TRUE(d.isDict());

  Dict dict(&scope, *d);
  EXPECT_EQ(dict.numItems(), 4);

  Str name(&scope, runtime_->newStrFromCStr("a"));
  Object el0(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el0, 1));

  name = runtime_->newStrFromCStr("b");
  Object el1(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el1, 2));

  name = runtime_->newStrFromCStr("c");
  Object el2(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el2, 3));

  name = runtime_->newStrFromCStr("d");
  Object el3(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el3, 4));
}

TEST_F(InterpreterTest, BuildMapUnpackWithIterableKeysMapping) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
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

  Object d(&scope, mainModuleAt(runtime_, "d"));
  ASSERT_TRUE(d.isDict());

  Dict dict(&scope, *d);
  EXPECT_EQ(dict.numItems(), 4);

  Str name(&scope, runtime_->newStrFromCStr("a"));
  Object el0(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el0, 1));

  name = runtime_->newStrFromCStr("b");
  Object el1(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el1, 2));

  name = runtime_->newStrFromCStr("c");
  Object el2(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el2, 3));

  name = runtime_->newStrFromCStr("d");
  Object el3(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el3, 4));
}

TEST_F(InterpreterTest, BuildMapUnpackWithNonMapping) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
class Foo:
    pass

d = {**Foo(), 'd': 4}
  )"),
                            LayoutId::kTypeError,
                            "'Foo' object is not a mapping"));
}

TEST_F(InterpreterTest, BuildMapUnpackWithUnsubscriptableMapping) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
class Foo:
    def __init__(self):
        self.idx = 0
        self._items = [('a', 1), ('b', 2), ('c', 3)]

    def keys(self):
        return ('a', 'b', 'c')

d = {**Foo(), 'd': 4}
  )"),
                            LayoutId::kTypeError,
                            "'Foo' object is not a mapping"));
}

TEST_F(InterpreterTest, BuildMapUnpackWithNonIterableKeys) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
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
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
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
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
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
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, src), LayoutId::kValueError,
                            "not enough values to unpack"));
}

TEST_F(InterpreterTest, UnpackSequenceExWithTooFewObjectsAfter) {
  const char* src = R"(
l = [1, 2]
*a, b, c, d = l
)";
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, src), LayoutId::kValueError,
                            "not enough values to unpack"));
}

TEST_F(InterpreterTest, BuildTupleUnpackWithCall) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(*args):
    return args

t = foo(*(1,2), *(3, 4))
)")
                   .isError());

  Object t(&scope, mainModuleAt(runtime_, "t"));
  ASSERT_TRUE(t.isTuple());

  Tuple tuple(&scope, *t);
  EXPECT_TRUE(isIntEqualsWord(tuple.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(tuple.at(1), 2));
  EXPECT_TRUE(isIntEqualsWord(tuple.at(2), 3));
  EXPECT_TRUE(isIntEqualsWord(tuple.at(3), 4));
}

TEST_F(InterpreterTest, FunctionDerefsVariable) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def outer():
    var = 1
    def inner():
        return var
    del var
    return 0

v = outer()
	)")
                   .isError());

  Object v(&scope, mainModuleAt(runtime_, "v"));
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
      raisedWithStr(runFromCStr(runtime_, src), LayoutId::kUnboundLocalError,
                    "local variable 'var' referenced before assignment"));
}

TEST_F(InterpreterTest, ImportStarImportsPublicSymbols) {
  HandleScope scope(thread_);
  Object module_src(&scope, runtime_->newStrFromCStr(R"(
def public_symbol():
    return 1
def public_symbol2():
    return 2
)"));
  Object filename(&scope, runtime_->newStrFromCStr("<test string>"));

  // Preload the module
  Object name(&scope, runtime_->newStrFromCStr("test_module"));
  Code code(&scope, compile(thread_, module_src, filename, ID(exec),
                            /*flags=*/0, /*optimize=*/0));
  ASSERT_FALSE(executeModuleFromCode(thread_, code, name).isError());

  ASSERT_FALSE(runFromCStr(runtime_, R"(
from test_module import *
a = public_symbol()
b = public_symbol2()
)")
                   .isError());

  Object a(&scope, mainModuleAt(runtime_, "a"));
  Object b(&scope, mainModuleAt(runtime_, "b"));
  EXPECT_TRUE(isIntEqualsWord(*a, 1));
  EXPECT_TRUE(isIntEqualsWord(*b, 2));
}

TEST_F(InterpreterTest, ImportStarDoesNotImportPrivateSymbols) {
  HandleScope scope(thread_);
  Object module_src(&scope, runtime_->newStrFromCStr(R"(
def public_symbol():
    return 1
def _private_symbol():
    return 2
)"));
  Object filename(&scope, runtime_->newStrFromCStr("<test string>"));

  // Preload the module
  Object name(&scope, runtime_->newStrFromCStr("test_module"));
  Code code(&scope, compile(thread_, module_src, filename, ID(exec),
                            /*flags=*/0, /*optimize=*/0));
  ASSERT_FALSE(executeModuleFromCode(thread_, code, name).isError());

  const char* main_src = R"(
from test_module import *
a = public_symbol()
b = _private_symbol()
)";

  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, main_src),
                            LayoutId::kNameError,
                            "name '_private_symbol' is not defined"));
}

TEST_F(InterpreterTest, ImportStarWorksWithDictImplicitGlobals) {
  HandleScope scope(thread_);
  Object module_src(&scope, runtime_->newStrFromCStr(R"(
def foo():
    return "bar"
def baz():
    return "quux"
)"));
  Object filename(&scope, runtime_->newStrFromCStr("<test string>"));

  // Preload the module
  Object name(&scope, runtime_->newStrFromCStr("test_module"));
  Code module_code(&scope, compile(thread_, module_src, filename, ID(exec),
                                   /*flags=*/0, /*optimize=*/0));
  ASSERT_FALSE(executeModuleFromCode(thread_, module_code, name).isError());

  const char* main_src = R"(
from test_module import *
a = foo()
b = baz()
)";

  Object str(&scope, runtime_->newStrFromCStr(main_src));
  Code main_code(&scope, compile(thread_, str, filename, ID(exec),
                                 /*flags=*/0, /*optimize=*/0));
  Module main_module(&scope, runtime_->findOrCreateMainModule());
  Dict implicit_globals(&scope, runtime_->newDict());
  Object result(&scope,
                thread_->exec(main_code, main_module, implicit_globals));
  EXPECT_FALSE(result.isError());
  EXPECT_EQ(implicit_globals.numItems(), 4);
}

TEST_F(InterpreterTest, ImportStarWorksWithUserDefinedImplicitGlobals) {
  HandleScope scope(thread_);
  Object module_src(&scope, runtime_->newStrFromCStr(R"(
def foo():
    return "bar"
def baz():
    return "quux"
)"));
  Object filename(&scope, runtime_->newStrFromCStr("<test string>"));

  // Preload the module
  Object name(&scope, runtime_->newStrFromCStr("test_module"));
  Code module_code(&scope, compile(thread_, module_src, filename, ID(exec),
                                   /*flags=*/0, /*optimize=*/0));
  ASSERT_FALSE(executeModuleFromCode(thread_, module_code, name).isError());

  EXPECT_FALSE(runFromCStr(runtime_, R"(
class C:
   def __init__(self):
      self.mydict = {}
   def __setitem__(self, key, value):
      self.mydict[key] = value
   def __getitem__(self, key):
      return self.mydict[key]
)")
                   .isError());

  const char* main_src = R"(
from test_module import *
a = foo()
b = baz()
)";

  Object str(&scope, runtime_->newStrFromCStr(main_src));
  Code main_code(&scope, compile(thread_, str, filename, ID(exec),
                                 /*flags=*/0, /*optimize=*/0));
  Module main_module(&scope, runtime_->findOrCreateMainModule());
  Type implicit_globals_type(&scope, mainModuleAt(runtime_, "C"));
  Object implicit_globals(
      &scope, thread_->invokeMethod1(implicit_globals_type, ID(__call__)));
  Object result(&scope,
                thread_->exec(main_code, main_module, implicit_globals));
  EXPECT_FALSE(result.isError());
}

TEST_F(InterpreterTest, ImportCallsBuiltinsDunderImport) {
  ASSERT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
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
  ASSERT_FALSE(runFromCStr(runtime_, R"(
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

  # Return from __await__ must be an "iterable" type
  def __next__(self):
    pass

a = AsyncIterator()
)")
                   .isError());
  Object a(&scope, mainModuleAt(runtime_, "a"));

  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_->newTupleWith1(a));
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST,  0, GET_ANEXT,    0,
                           BUILD_TUPLE, 2, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));

  Tuple result(&scope, runCode(code));
  EXPECT_EQ(*a, result.at(0));
  EXPECT_EQ(*a, result.at(1));
  Object anext(&scope, mainModuleAt(runtime_, "anext_called"));
  EXPECT_EQ(*a, *anext);
  Object await(&scope, mainModuleAt(runtime_, "await_called"));
  EXPECT_EQ(*a, *await);
}

TEST_F(InterpreterTest, GetAnextCallsAnextButNotAwaitOnAsyncGenerator) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
async def f():
  yield

async_gen = f()

class AsyncIterator:
  def __anext__(self):
    return async_gen

async_it = AsyncIterator()
)")
                   .isError());
  Object async_gen(&scope, mainModuleAt(runtime_, "async_gen"));
  Object async_it(&scope, mainModuleAt(runtime_, "async_it"));
  // The async generator object instance should not have an __await__() method.
  ASSERT_TRUE(Interpreter::lookupMethod(thread_, async_gen, ID(__await__))
                  .isErrorNotFound());
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_->newTupleWith1(async_it));
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST,  0, GET_ANEXT,    0,
                           BUILD_TUPLE, 2, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));
  Tuple result(&scope, runCode(code));
  EXPECT_EQ(*async_it, result.at(0));
  EXPECT_EQ(runtime_->typeOf(result.at(1)),
            runtime_->typeAt(LayoutId::kAsyncGenerator));
}

TEST_F(InterpreterTest, GetAnextOnNonIterable) {
  HandleScope scope(thread_);
  Code code(&scope, newEmptyCode());
  Object obj(&scope, SmallInt::fromWord(123));
  Tuple consts(&scope, runtime_->newTupleWith1(obj));
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, GET_ANEXT, 0, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));

  Object result(&scope, runCode(code));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST_F(InterpreterTest, GetAnextWithInvalidAnext) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class AsyncIterator:
  def __anext__(self):
    return 42

a = AsyncIterator()
)")
                   .isError());
  Object a(&scope, mainModuleAt(runtime_, "a"));

  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_->newTupleWith1(a));
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, GET_ANEXT, 0, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));

  Object result(&scope, runCode(code));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

static RawObject runCodeCallingGetAwaitableOnObject(Thread* thread,
                                                    const Object& obj) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime->newTupleWith1(obj));
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, GET_AWAITABLE, 0, RETURN_VALUE, 0};
  code.setCode(runtime->newBytesWithAll(bytecode));
  return runCode(code);
}

TEST_F(InterpreterTest, GetAwaitableCallsAwait) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
# Return from __await__ must be an "iterable" type
iterable = iter([])

class Awaitable:
  def __await__(self):
    return iterable

a = Awaitable()
)")
                   .isError());

  Object iterable(&scope, mainModuleAt(runtime_, "iterable"));
  Object a(&scope, mainModuleAt(runtime_, "a"));
  Object result(&scope, runCodeCallingGetAwaitableOnObject(thread_, a));
  EXPECT_EQ(result, iterable);
}

TEST_F(InterpreterTest, GetAwaitableIsNoOpOnCoroutine) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
async def f(): pass

coro = f()
)")
                   .isError());

  Object coro(&scope, mainModuleAt(runtime_, "coro"));
  Object result(&scope, runCodeCallingGetAwaitableOnObject(thread_, coro));
  EXPECT_TRUE(*result == *coro);
}

TEST_F(InterpreterTest, GetAwaitableIsNoOpOnAsyncGenerator) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
async def f(): yield

async_gen = f()
)")
                   .isError());

  Object async_gen(&scope, mainModuleAt(runtime_, "async_gen"));
  Object result(&scope, runCodeCallingGetAwaitableOnObject(thread_, async_gen));
  EXPECT_TRUE(*result == *async_gen);
}

TEST_F(InterpreterTest, GetAwaitableRaisesOnUnflaggedGenerator) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def f(): yield

generator = f()
)")
                   .isError());

  Object generator(&scope, mainModuleAt(runtime_, "generator"));
  Object result(&scope, runCodeCallingGetAwaitableOnObject(thread_, generator));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST_F(InterpreterTest, GetAwaitableIsNoOpOnFlaggedGenerator) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def f(): yield
)")
                   .isError());
  Function generator_function(&scope, mainModuleAt(runtime_, "f"));
  generator_function.setFlags(generator_function.flags() |
                              RawFunction::Flags::kIterableCoroutine);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
generator = f()
)")
                   .isError());
  Object generator(&scope, mainModuleAt(runtime_, "generator"));
  Object result(&scope, runCodeCallingGetAwaitableOnObject(thread_, generator));
  EXPECT_TRUE(*result == *generator);
}

TEST_F(InterpreterTest, GetAwaitableOnNonAwaitable) {
  HandleScope scope(thread_);
  Object str(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  Object result(&scope, runCodeCallingGetAwaitableOnObject(thread_, str));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST_F(InterpreterTest, BuildMapUnpackWithCallDict) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(**kwargs):
    return kwargs

d = foo(**{'a': 1, 'b': 2}, **{'c': 3, 'd': 4})
)")
                   .isError());

  Object d(&scope, mainModuleAt(runtime_, "d"));
  ASSERT_TRUE(d.isDict());

  Dict dict(&scope, *d);
  EXPECT_EQ(dict.numItems(), 4);

  Str name(&scope, runtime_->newStrFromCStr("a"));
  Object el0(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el0, 1));

  name = runtime_->newStrFromCStr("b");
  Object el1(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el1, 2));

  name = runtime_->newStrFromCStr("c");
  Object el2(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el2, 3));

  name = runtime_->newStrFromCStr("d");
  Object el3(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el3, 4));
}

TEST_F(InterpreterTest, BuildMapUnpackWithCallTupleKeys) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
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

  Object d(&scope, mainModuleAt(runtime_, "d"));
  ASSERT_TRUE(d.isDict());

  Dict dict(&scope, *d);
  EXPECT_EQ(dict.numItems(), 4);

  Str name(&scope, runtime_->newStrFromCStr("a"));
  Object el0(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el0, 1));

  name = runtime_->newStrFromCStr("b");
  Object el1(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el1, 2));

  name = runtime_->newStrFromCStr("c");
  Object el2(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el2, 3));

  name = runtime_->newStrFromCStr("d");
  Object el3(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el3, 4));
}

TEST_F(InterpreterTest, BuildMapUnpackWithCallListKeys) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
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

  Object d(&scope, mainModuleAt(runtime_, "d"));
  ASSERT_TRUE(d.isDict());

  Dict dict(&scope, *d);
  EXPECT_EQ(dict.numItems(), 4);

  Str name(&scope, runtime_->newStrFromCStr("a"));
  Object el0(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el0, 1));

  name = runtime_->newStrFromCStr("b");
  Object el1(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el1, 2));

  name = runtime_->newStrFromCStr("c");
  Object el2(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el2, 3));

  name = runtime_->newStrFromCStr("d");
  Object el3(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el3, 4));
}

TEST_F(InterpreterTest, BuildMapUnpackWithCallIteratorKeys) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
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

  Object d(&scope, mainModuleAt(runtime_, "d"));
  ASSERT_TRUE(d.isDict());

  Dict dict(&scope, *d);
  EXPECT_EQ(dict.numItems(), 4);

  Str name(&scope, runtime_->newStrFromCStr("a"));
  Object el0(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el0, 1));

  name = runtime_->newStrFromCStr("b");
  Object el1(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el1, 2));

  name = runtime_->newStrFromCStr("c");
  Object el2(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el2, 3));

  name = runtime_->newStrFromCStr("d");
  Object el3(&scope, dictAtByStr(thread_, dict, name));
  EXPECT_TRUE(isIntEqualsWord(*el3, 4));
}

TEST_F(InterpreterTest, BuildMapUnpackWithCallDictNonStrKey) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
def foo(**kwargs):
    return kwargs

foo(**{'a': 1, 'b': 2}, **{'c': 3, 4: 4})
  )"),
                            LayoutId::kTypeError, "keywords must be strings"));
}

TEST_F(InterpreterTest, BuildMapUnpackWithCallDictRepeatedKeys) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
def foo(**kwargs):
    return kwargs

foo(**{'a': 1, 'b': 2}, **{'c': 3, 'a': 4})
  )"),
                            LayoutId::kTypeError,
                            "got multiple values for keyword argument 'a'"));
}

TEST_F(InterpreterTest, BuildMapUnpackWithCallNonMapping) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
class Foo:
    pass

def foo(**kwargs):
    return kwargs

foo(**{'a': 1, 'b': 2}, **Foo())
  )"),
                            LayoutId::kTypeError,
                            "'Foo' object is not a mapping"));
}

TEST_F(InterpreterTest, BuildMapUnpackWithCallNonSubscriptable) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
class Foo:
    def keys(self):
        pass

def foo(**kwargs):
    return kwargs

foo(**{'a': 1, 'b': 2}, **Foo())
  )"),
                            LayoutId::kTypeError,
                            "'Foo' object is not a mapping"));
}

TEST_F(InterpreterTest, BuildMapUnpackWithCallListKeysNonStrKey) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
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
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
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
                            "got multiple values for keyword argument 'a'"));
}

TEST_F(InterpreterTest, BuildMapUnpackWithCallTupleKeysNonStrKeys) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
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
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
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
                            "got multiple values for keyword argument 'a'"));
}

TEST_F(InterpreterTest, BuildMapUnpackWithCallNonIterableKeys) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
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
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
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
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
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
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
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
                            "got multiple values for keyword argument 'a'"));
}

TEST_F(InterpreterTest, YieldFromIterReturnsIter) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(runtime_, R"(
class FooIterator:
    def __next__(self):
        pass

class Foo:
    def __iter__(self):
        return FooIterator()

foo = Foo()
	)")
                   .isError());

  Object foo(&scope, mainModuleAt(runtime_, "foo"));

  // Create a code object and set the foo instance as a const
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_->newTupleWith1(foo));
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
  code.setCode(runtime_->newBytesWithAll(bytecode));

  // Confirm that the returned value is the iterator of Foo
  Object result(&scope, runCode(code));
  Type result_type(&scope, runtime_->typeOf(*result));
  EXPECT_TRUE(isStrEqualsCStr(result_type.name(), "FooIterator"));
}

TEST_F(InterpreterTest, YieldFromIterWithSequenceReturnsIter) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(runtime_, R"(
class FooSequence:
    def __getitem__(self, i):
        return ("foo", "bar")[i]

foo = FooSequence()
	)")
                   .isError());

  Object foo(&scope, mainModuleAt(runtime_, "foo"));

  // Create a code object and set the foo instance as a const
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_->newTupleWith1(foo));
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
  code.setCode(runtime_->newBytesWithAll(bytecode));

  // Confirm that the returned value is a sequence iterator
  Object result(&scope, runCode(code));
  Type result_type(&scope, runtime_->typeOf(*result));
  EXPECT_TRUE(isStrEqualsCStr(result_type.name(), "iterator"));
}

TEST_F(InterpreterTest, YieldFromIterRaisesException) {
  const char* src = R"(
def yield_from_func():
    yield from 1

for i in yield_from_func():
    pass
	)";

  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, src), LayoutId::kTypeError,
                            "'int' object is not iterable"));
}

TEST_F(InterpreterTest, YieldFromCoroutineInNonCoroutineIterRaisesException) {
  const char* src = R"(
async def coro():
  pass

def f():
    yield from coro()

f().send(None)
	)";

  EXPECT_TRUE(
      raisedWithStr(runFromCStr(runtime_, src), LayoutId::kTypeError,
                    "cannot 'yield from' a coroutine object in a non-coroutine "
                    "generator"));
}

TEST_F(InterpreterTest, MakeFunctionSetsDunderModule) {
  HandleScope scope(thread_);
  Object module_name(&scope, runtime_->newStrFromCStr("foo"));
  Object module_src(&scope, runtime_->newStrFromCStr(R"(
def bar(): pass
)"));
  Object filename(&scope, runtime_->newStrFromCStr("<test string>"));
  Code code(&scope, compile(thread_, module_src, filename, ID(exec),
                            /*flags=*/0, /*optimize=*/0));
  ASSERT_FALSE(executeModuleFromCode(thread_, code, module_name).isError());
  ASSERT_FALSE(runFromCStr(runtime_, R"(
import foo
def baz(): pass
a = getattr(foo.bar, '__module__')
b = getattr(baz, '__module__')
)")
                   .isError());
  Object a(&scope, mainModuleAt(runtime_, "a"));
  ASSERT_TRUE(a.isStr());
  EXPECT_TRUE(Str::cast(*a).equalsCStr("foo"));
  Object b(&scope, mainModuleAt(runtime_, "b"));
  ASSERT_TRUE(b.isStr());
  EXPECT_TRUE(Str::cast(*b).equalsCStr("__main__"));
}

TEST_F(InterpreterTest, MakeFunctionSetsDunderQualname) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class Foo():
    def bar(): pass
def baz(): pass
a = getattr(Foo.bar, '__qualname__')
b = getattr(baz, '__qualname__')
)")
                   .isError());
  Object a(&scope, mainModuleAt(runtime_, "a"));
  ASSERT_TRUE(a.isStr());
  EXPECT_TRUE(Str::cast(*a).equalsCStr("Foo.bar"));
  Object b(&scope, mainModuleAt(runtime_, "b"));
  ASSERT_TRUE(b.isStr());
  EXPECT_TRUE(Str::cast(*b).equalsCStr("baz"));
}

TEST_F(InterpreterTest, MakeFunctionSetsDunderDoc) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo():
    """This is a docstring"""
    pass
def bar(): pass
)")
                   .isError());
  Object foo(&scope, testing::mainModuleAt(runtime_, "foo"));
  ASSERT_TRUE(foo.isFunction());
  Object foo_docstring(&scope, Function::cast(*foo).doc());
  ASSERT_TRUE(foo_docstring.isStr());
  EXPECT_TRUE(Str::cast(*foo_docstring).equalsCStr("This is a docstring"));

  Object bar(&scope, testing::mainModuleAt(runtime_, "bar"));
  ASSERT_TRUE(bar.isFunction());
  Object bar_docstring(&scope, Function::cast(*bar).doc());
  EXPECT_TRUE(bar_docstring.isNoneType());
}

TEST_F(InterpreterTest, OpcodesAreCounted) {
  if (useCppInterpreter()) {
    GTEST_SKIP();
  }

  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def a(a, b):
  return a + b
def func():
  return a(7, 88)
)")
                   .isError());
  Object func(&scope, mainModuleAt(runtime_, "func"));
  EXPECT_EQ(thread_->opcodeCount(), 0);
  ASSERT_FALSE(Interpreter::call0(thread_, func).isError());
  EXPECT_EQ(thread_->opcodeCount(), 0);

  runtime_->interpreter()->setOpcodeCounting(true);
  runtime_->reinitInterpreter();

  word count_before = thread_->opcodeCount();
  ASSERT_FALSE(Interpreter::call0(thread_, func).isError());
  EXPECT_EQ(thread_->opcodeCount() - count_before, 9);

  runtime_->interpreter()->setOpcodeCounting(false);
  runtime_->reinitInterpreter();

  count_before = thread_->opcodeCount();
  ASSERT_FALSE(Interpreter::call0(thread_, func).isError());
  EXPECT_EQ(thread_->opcodeCount() - count_before, 0);
}

static RawObject startCounting(Thread* thread, Arguments) {
  thread->runtime()->interpreter()->setOpcodeCounting(true);
  thread->runtime()->reinitInterpreter();
  return NoneType::object();
}

static RawObject stopCounting(Thread* thread, Arguments) {
  thread->runtime()->interpreter()->setOpcodeCounting(false);
  thread->runtime()->reinitInterpreter();
  return NoneType::object();
}

TEST_F(InterpreterTest, ReinitInterpreterEnablesOpcodeCounting) {
  if (useCppInterpreter()) {
    GTEST_SKIP();
  }

  addBuiltin("start_counting", startCounting, {nullptr, 0}, 0);
  addBuiltin("stop_counting", stopCounting, {nullptr, 0}, 0);

  EXPECT_EQ(thread_->opcodeCount(), 0);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def bar():
  start_counting()
def func():
  x = 5
  x = 5
  x = 5
  x = 5
  x = 5
  x = 5
  x = 5
  x = 5
  x = 5
  x = 5
  return 5
func()
bar()
func()
stop_counting()
func()
)")
                   .isError());
  // I do not want to hardcode opcode counts for the calls here (since that
  // may change in the future). So this just checks that we have at least
  // 10*2 = 20 opcodes for a `func()` call, but no more than double that amount
  // to make sure we did not consider the `foo()` call before and after
  // counting was enabled.
  word count = thread_->opcodeCount();
  EXPECT_TRUE(20 < count && count < 40);
}

TEST_F(InterpreterTest, FunctionCallWithNonFunctionRaisesTypeError) {
  HandleScope scope(thread_);
  Str not_a_func(&scope, Str::empty());
  thread_->stackPush(*not_a_func);
  EXPECT_TRUE(raised(Interpreter::call(thread_, 0), LayoutId::kTypeError));
}

TEST_F(InterpreterTest, FunctionCallExWithNonFunctionRaisesTypeError) {
  HandleScope scope(thread_);
  Str not_a_func(&scope, Str::empty());
  thread_->stackPush(*not_a_func);
  Tuple empty_args(&scope, runtime_->emptyTuple());
  thread_->stackPush(*empty_args);
  EXPECT_TRUE(raisedWithStr(Interpreter::callEx(thread_, 0),
                            LayoutId::kTypeError,
                            "'str' object is not callable"));
}

TEST_F(InterpreterTest, CallExWithDescriptorDunderCall) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
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
  EXPECT_TRUE(isStrEqualsCStr(mainModuleAt(runtime_, "result"), "hello!"));
}

TEST_F(InterpreterTest, DoDeleteNameOnDictSubclass) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
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
  ASSERT_FALSE(runFromCStr(runtime_, R"(
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
  ASSERT_FALSE(runFromCStr(runtime_, R"(
l = [0]
for i in range(5):
    l[0] += i
)")
                   .isError());

  HandleScope scope(thread_);
  Object l_obj(&scope, testing::mainModuleAt(runtime_, "l"));
  ASSERT_TRUE(l_obj.isList());
  List l(&scope, *l_obj);
  ASSERT_EQ(l.numItems(), 1);
  EXPECT_EQ(l.at(0), SmallInt::fromWord(10));
}

TEST_F(InterpreterTest, StoreSubscrWithListRewritesToStoreSubscrList) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(l, i):
    l[i] = 4
    return 100

l = [1,2,3]
d = {1: -1}
)")
                   .isError());
  Function foo(&scope, mainModuleAt(runtime_, "foo"));
  MutableBytes rewritten(&scope, foo.rewrittenBytecode());
  ASSERT_EQ(rewritten.byteAt(6), STORE_SUBSCR_ANAMORPHIC);

  List l(&scope, mainModuleAt(runtime_, "l"));
  SmallInt key(&scope, SmallInt::fromWord(1));
  EXPECT_TRUE(isIntEqualsWord(Interpreter::call2(thread_, foo, l, key), 100));
  EXPECT_EQ(rewritten.byteAt(6), STORE_SUBSCR_LIST);

  // Revert back to caching __getitem__ when a non-list is observed.
  Dict d(&scope, mainModuleAt(runtime_, "d"));
  EXPECT_TRUE(isIntEqualsWord(Interpreter::call2(thread_, foo, d, key), 100));
  EXPECT_EQ(rewritten.byteAt(6), STORE_SUBSCR_MONOMORPHIC);
}

// TODO(bsimmers) Rewrite these exception tests to ensure that the specific
// bytecodes we care about are being exercised, so we're not be at the mercy of
// compiler optimizations or changes.
TEST_F(InterpreterTest, ExceptCatchesException) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
n = 0
try:
    raise RuntimeError("something went wrong")
    n = 1
except:
    if n == 0:
        n = 2
)")
                   .isError());

  HandleScope scope(thread_);
  Object n(&scope, testing::mainModuleAt(runtime_, "n"));
  EXPECT_TRUE(isIntEqualsWord(*n, 2));
}

TEST_F(InterpreterTest, RaiseCrossesFunctions) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
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

  HandleScope scope(thread_);
  Object n(&scope, testing::mainModuleAt(runtime_, "n"));
  EXPECT_TRUE(isIntEqualsWord(*n, 2));
}

TEST_F(InterpreterTest, RaiseFromSetsCause) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
try:
  try:
    raise RuntimeError
  except Exception as e:
    raise TypeError from e
except Exception as e:
  exc = e
)")
                   .isError());

  HandleScope scope(thread_);
  Object exc_obj(&scope, testing::mainModuleAt(runtime_, "exc"));
  ASSERT_EQ(exc_obj.layoutId(), LayoutId::kTypeError);
  BaseException exc(&scope, *exc_obj);
  EXPECT_EQ(exc.cause().layoutId(), LayoutId::kRuntimeError);
}

TEST_F(InterpreterTest, ExceptWithRightTypeCatches) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
n = 0
try:
    raise RuntimeError("whoops")
    n = 1
except RuntimeError:
    if n == 0:
        n = 2
)")
                   .isError());

  HandleScope scope(thread_);
  Object n(&scope, testing::mainModuleAt(runtime_, "n"));
  EXPECT_TRUE(isIntEqualsWord(*n, 2));
}

TEST_F(InterpreterTest, ExceptWithRightTupleTypeCatches) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
n = 0
try:
    raise RuntimeError()
    n = 1
except (StopIteration, RuntimeError, ImportError):
    if n == 0:
        n = 2
)")
                   .isError());

  HandleScope scope(thread_);
  Object n(&scope, testing::mainModuleAt(runtime_, "n"));
  EXPECT_TRUE(isIntEqualsWord(*n, 2));
}

TEST_F(InterpreterTest, ExceptWithWrongTypePasses) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
try:
    raise RuntimeError("something went wrong")
except StopIteration:
    pass
)"),
                            LayoutId::kRuntimeError, "something went wrong"));
}

TEST_F(InterpreterTest, ExceptWithWrongTupleTypePasses) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
try:
    raise RuntimeError("something went wrong")
except (StopIteration, ImportError):
    pass
)"),
                            LayoutId::kRuntimeError, "something went wrong"));
}

TEST_F(InterpreterTest, RaiseTypeCreatesException) {
  EXPECT_TRUE(raised(runFromCStr(runtime_, "raise StopIteration"),
                     LayoutId::kStopIteration));
}

TEST_F(InterpreterTest, BareRaiseReraises) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
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

  HandleScope scope(thread_);
  Object my_error(&scope, testing::mainModuleAt(runtime_, "MyError"));
  EXPECT_EQ(runtime_->typeOf(*my_error), runtime_->typeAt(LayoutId::kType));
  Object inner(&scope, testing::mainModuleAt(runtime_, "inner"));
  EXPECT_EQ(runtime_->typeOf(*inner), *my_error);
  Object outer(&scope, testing::mainModuleAt(runtime_, "outer"));
  EXPECT_EQ(*inner, *outer);
}

TEST_F(InterpreterTest, ExceptWithNonExceptionTypeRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
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
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
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
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, "raise\n"),
                            LayoutId::kRuntimeError,
                            "No active exception to reraise"));
}

TEST_F(InterpreterTest, LoadAttrSetLocationSetsLocation) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  def __init__(self):
    self.foo = 42
i = C()
)")
                   .isError());
  Object i(&scope, mainModuleAt(runtime_, "i"));

  Object name(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  Object to_cache(&scope, NoneType::object());
  LoadAttrKind kind = LoadAttrKind::kInstanceOffset;
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::loadAttrSetLocation(thread_, i, name, &kind, &to_cache),
      42));
  EXPECT_EQ(kind, LoadAttrKind::kInstanceOffset);
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::loadAttrWithLocation(thread_, *i, *to_cache), 42));
}

TEST_F(InterpreterTest, LoadAttrSetLocationSetsLocationToProprty) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(

def foo(self): return "data descriptor"

class C:
    foo = property (foo)

c = C()
)")
                   .isError());
  Object c(&scope, mainModuleAt(runtime_, "c"));
  Str name(&scope, runtime_->newStrFromCStr("foo"));
  Object to_cache(&scope, NoneType::object());
  LoadAttrKind kind = LoadAttrKind::kUnknown;
  EXPECT_TRUE(isStrEqualsCStr(
      Interpreter::loadAttrSetLocation(thread_, c, name, &kind, &to_cache),
      "data descriptor"));
  EXPECT_EQ(kind, LoadAttrKind::kInstanceProperty);
  EXPECT_EQ(to_cache, mainModuleAt(runtime_, "foo"));
}

TEST_F(InterpreterTest,
       LoadAttrSetLocationSetLocationToPropertyAsDataDescriptorWithNoneGetter) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
C_foo = property (fget=None, fset=lambda self,v: None, fdel=lambda self: None)
class C:
    foo = C_foo

c = C()
)")
                   .isError());
  Object c(&scope, mainModuleAt(runtime_, "c"));
  Str name(&scope, runtime_->newStrFromCStr("foo"));
  Object to_cache(&scope, NoneType::object());
  LoadAttrKind kind = LoadAttrKind::kUnknown;
  EXPECT_TRUE(
      Interpreter::loadAttrSetLocation(thread_, c, name, &kind, &to_cache)
          .isError());
  EXPECT_EQ(to_cache, mainModuleAt(runtime_, "C_foo"));
  EXPECT_EQ(kind, LoadAttrKind::kInstanceTypeDescr);
}

TEST_F(InterpreterTest, LoadAttrWithModuleSetLocationSetsLocation) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
a_global = 1234
)")
                   .isError());
  Object mod(&scope, findMainModule(runtime_));
  Object name(&scope, Runtime::internStrFromCStr(thread_, "a_global"));

  Object to_cache(&scope, NoneType::object());
  LoadAttrKind kind;
  ASSERT_TRUE(isIntEqualsWord(
      Interpreter::loadAttrSetLocation(thread_, mod, name, &kind, &to_cache),
      1234));
  EXPECT_EQ(kind, LoadAttrKind::kModule);
  EXPECT_EQ(to_cache.layoutId(), LayoutId::kValueCell);
}

TEST_F(InterpreterTest, LoadAttrWithTypeSetLocationSetsLocation) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  an_attribute = 1234
)")
                   .isError());
  Object type(&scope, mainModuleAt(runtime_, "C"));

  Object name(&scope, Runtime::internStrFromCStr(thread_, "an_attribute"));

  Object to_cache(&scope, NoneType::object());
  LoadAttrKind kind;
  ASSERT_TRUE(isIntEqualsWord(
      Interpreter::loadAttrSetLocation(thread_, type, name, &kind, &to_cache),
      1234));
  EXPECT_EQ(kind, LoadAttrKind::kType);
  EXPECT_EQ(to_cache.layoutId(), LayoutId::kValueCell);
}

TEST_F(InterpreterTest,
       LoadAttrSetLocationWithCustomGetAttributeSetsNoLocation) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  def __getattribute__(self, name):
    return 11
i = C()
)")
                   .isError());
  Object i(&scope, mainModuleAt(runtime_, "i"));

  Object name(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  Object to_cache(&scope, NoneType::object());
  LoadAttrKind kind;
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::loadAttrSetLocation(thread_, i, name, &kind, &to_cache),
      11));
  EXPECT_EQ(kind, LoadAttrKind::kUnknown);
  EXPECT_TRUE(to_cache.isNoneType());
}

TEST_F(InterpreterTest, LoadAttrSetLocationCallsDunderGetattr) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  def __init__(self):
    self.foo = 42
  def __getattr__(self, name):
    return 5
i = C()
)")
                   .isError());
  Object i(&scope, mainModuleAt(runtime_, "i"));

  Object name(&scope, Runtime::internStrFromCStr(thread_, "bar"));
  Object to_cache(&scope, NoneType::object());
  LoadAttrKind kind;

  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::loadAttrSetLocation(thread_, i, name, &kind, &to_cache), 5));
  EXPECT_EQ(kind, LoadAttrKind::kUnknown);
  EXPECT_TRUE(to_cache.isNoneType());
}

TEST_F(InterpreterTest,
       LoadAttrSetLocationWithNoAttributeRaisesAttributeError) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
obj = object()
)")
                   .isError());

  Object obj(&scope, mainModuleAt(runtime_, "obj"));
  Object name(&scope, Runtime::internStrFromCStr(thread_, "nonexistent_attr"));
  LoadAttrKind kind;
  EXPECT_TRUE(raisedWithStr(
      Interpreter::loadAttrSetLocation(thread_, obj, name, &kind, nullptr),
      LayoutId::kAttributeError,
      "'object' object has no attribute 'nonexistent_attr'"));
}

TEST_F(InterpreterTest, LoadAttrWithoutAttrUnwindsAttributeException) {
  HandleScope scope(thread_);

  // Set up a code object that runs: {}.foo
  Code code(&scope, newEmptyCode());
  Object foo(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  Tuple names(&scope, runtime_->newTupleWith1(foo));
  code.setNames(*names);

  // load arguments and execute the code
  const byte bytecode[] = {BUILD_MAP, 0, LOAD_ATTR, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));
  code.setStacksize(1);

  // Execute the code and make sure to get the unwinded Error
  EXPECT_TRUE(runCode(code).isError());
}

TEST_F(InterpreterTest, ExplodeCallAcceptsList) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def f(a, b):
  return [b, a]

args = ['a', 'b']
result = f(*args)
)")
                   .isError());

  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {"b", "a"});
}

TEST_F(InterpreterTest, ExplodeWithIterableCalls) {
  HandleScope scope(thread_);
  EXPECT_FALSE(runFromCStr(runtime_, R"(
def f(a, b):
  return (b, a)
def gen():
  yield 1
  yield 2
result = f(*gen())
)")
                   .isError());

  Object result_obj(&scope, mainModuleAt(runtime_, "result"));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 2));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 1));
}

TEST_F(InterpreterTest, ForIterAnamorphicWithBuiltinIterRewritesOpcode) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(i, s=0):
  for a in i:
    s += a
  return s

list_obj = [4,5]
dict_obj = {4: "a", 5: "b"}
tuple_obj = (4,5)
range_obj = range(4,6)
str_obj = "45"

class C:
  def __iter__(self):
    return D()

class D:
  def __init__(self):
    self.used = False

  def __next__(self):
    if self.used:
      raise StopIteration
    self.used = True
    return 400

user_obj = C()
)")
                   .isError());
  Function foo(&scope, mainModuleAt(runtime_, "foo"));
  MutableBytes bytecode(&scope, foo.rewrittenBytecode());
  ASSERT_EQ(bytecode.byteAt(6), FOR_ITER_ANAMORPHIC);

  Object arg(&scope, mainModuleAt(runtime_, "list_obj"));
  EXPECT_TRUE(isIntEqualsWord(Interpreter::call1(thread_, foo, arg), 9));
  EXPECT_EQ(bytecode.byteAt(6), FOR_ITER_LIST);

  arg = mainModuleAt(runtime_, "dict_obj");
  EXPECT_TRUE(isIntEqualsWord(Interpreter::call1(thread_, foo, arg), 9));
  EXPECT_EQ(bytecode.byteAt(6), FOR_ITER_DICT);

  arg = mainModuleAt(runtime_, "tuple_obj");
  EXPECT_TRUE(isIntEqualsWord(Interpreter::call1(thread_, foo, arg), 9));
  EXPECT_EQ(bytecode.byteAt(6), FOR_ITER_TUPLE);

  arg = mainModuleAt(runtime_, "range_obj");
  EXPECT_TRUE(isIntEqualsWord(Interpreter::call1(thread_, foo, arg), 9));
  EXPECT_EQ(bytecode.byteAt(6), FOR_ITER_RANGE);

  arg = mainModuleAt(runtime_, "str_obj");
  Str s(&scope, runtime_->newStrFromCStr(""));
  EXPECT_TRUE(isStrEqualsCStr(Interpreter::call2(thread_, foo, arg, s), "45"));
  EXPECT_EQ(bytecode.byteAt(6), FOR_ITER_STR);

  // Resetting the opcode.
  arg = mainModuleAt(runtime_, "user_obj");
  EXPECT_TRUE(isIntEqualsWord(Interpreter::call1(thread_, foo, arg), 400));
  EXPECT_EQ(bytecode.byteAt(6), FOR_ITER_MONOMORPHIC);
}

TEST_F(InterpreterTest, FormatValueCallsDunderStr) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  def __str__(self):
    return "foobar"
result = f"{C()!s}"
)")
                   .isError());
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(isStrEqualsCStr(*result, "foobar"));
}

TEST_F(InterpreterTest, FormatValueFallsBackToDunderRepr) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  def __repr__(self):
    return "foobar"
result = f"{C()!s}"
)")
                   .isError());
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(isStrEqualsCStr(*result, "foobar"));
}

TEST_F(InterpreterTest, FormatValueCallsDunderRepr) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  def __repr__(self):
    return "foobar"
result = f"{C()!r}"
)")
                   .isError());
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(isStrEqualsCStr(*result, "foobar"));
}

TEST_F(InterpreterTest, FormatValueAsciiCallsDunderRepr) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  def __repr__(self):
    return "foobar"
result = f"{C()!a}"
)")
                   .isError());
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(isStrEqualsCStr(*result, "foobar"));
}

TEST_F(InterpreterTest, BreakInTryBreaks) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result = 0
for i in range(5):
  try:
    break
  except:
    pass
result = 10
)")
                   .isError());
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 10));
}

TEST_F(InterpreterTest, ContinueInExceptContinues) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
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
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, -4));
}

TEST_F(InterpreterTest, RaiseInLoopRaisesRuntimeError) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
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
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 1003));
}

TEST_F(InterpreterTest, ReturnInsideTryRunsFinally) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
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
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 56789));

  Object ran_finally(&scope, mainModuleAt(runtime_, "ran_finally"));
  EXPECT_EQ(*ran_finally, Bool::trueObj());
}

TEST_F(InterpreterTest, ReturnInsideFinallyOverridesEarlierReturn) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def f():
  try:
    return 123
  finally:
    return 456

result = f()
)")
                   .isError());
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 456));
}

TEST_F(InterpreterTest, ReturnInsideWithRunsDunderExit) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
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
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 1234));

  Object sequence(&scope, mainModuleAt(runtime_, "sequence"));
  EXPECT_TRUE(isStrEqualsCStr(*sequence, "enter in foo exit"));
}

TEST_F(InterpreterTest,
       WithStatementWithManagerWithoutEnterRaisesAttributeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
with None:
  pass
)"),
                            LayoutId::kAttributeError, "__enter__"));
}

TEST_F(InterpreterTest,
       WithStatementWithManagerWithoutExitRaisesAttributeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
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
  EXPECT_TRUE(raised(runFromCStr(runtime_, R"(
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
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
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
  EXPECT_TRUE(raised(runFromCStr(runtime_, R"(
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
  Object exit_info(&scope, mainModuleAt(runtime_, "exit_info"));
  ASSERT_TRUE(exit_info.isTuple());
  Tuple tuple(&scope, *exit_info);
  ASSERT_EQ(tuple.length(), 3);
  EXPECT_EQ(tuple.at(0), runtime_->typeAt(LayoutId::kStopIteration));

  Object raised_exc(&scope, mainModuleAt(runtime_, "raised_exc"));
  EXPECT_EQ(tuple.at(1), *raised_exc);

  // TODO(bsimmers): Check traceback once we record them.
}

TEST_F(InterpreterTest, WithStatementSwallowsException) {
  HandleScope scope(thread_);
  EXPECT_FALSE(runFromCStr(runtime_, R"(
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

  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 1234));
}

TEST_F(InterpreterTest, WithStatementWithRaisingExitRaises) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
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
  EXPECT_FALSE(runFromCStr(runtime_, R"(
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
  EXPECT_EQ(mainModuleAt(runtime_, "load_name_t"),
            mainModuleAt(runtime_, "load_global_t"));
}

TEST_F(InterpreterTest, LoadGlobalCachedReturnsModuleDictValue) {
  HandleScope scope(thread_);
  EXPECT_FALSE(runFromCStr(runtime_, R"(
a = 400

def foo():
  return a + a

result = foo()
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result"), 800));
  Function function(&scope, mainModuleAt(runtime_, "foo"));
  ASSERT_TRUE(isStrEqualsCStr(
      Tuple::cast(Code::cast(function.code()).names()).at(0), "a"));
  MutableTuple caches(&scope, function.caches());
  EXPECT_TRUE(
      isIntEqualsWord(valueCellValue(icLookupGlobalVar(*caches, 0)), 400));
}

TEST_F(InterpreterTest,
       LoadGlobalCachedReturnsBuiltinDictValueAndSetsPlaceholder) {
  HandleScope scope(thread_);
  EXPECT_FALSE(runFromCStr(runtime_, R"(
__builtins__.a = 400

def foo():
  return a + a

result = foo()
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result"), 800));
  Function function(&scope, mainModuleAt(runtime_, "foo"));
  ASSERT_TRUE(isStrEqualsCStr(
      Tuple::cast(Code::cast(function.code()).names()).at(0), "a"));
  MutableTuple caches(&scope, function.caches());
  EXPECT_TRUE(
      isIntEqualsWord(valueCellValue(icLookupGlobalVar(*caches, 0)), 400));

  Module module(&scope, function.moduleObject());
  Object name(&scope, Runtime::internStrFromCStr(thread_, "a"));
  Dict module_dict(&scope, module.dict());
  Object module_entry(&scope, dictAtByStr(thread_, module_dict, name));
  ASSERT_TRUE(module_entry.isValueCell());
  EXPECT_TRUE(ValueCell::cast(*module_entry).isPlaceholder());
}

TEST_F(InterpreterTest, StoreGlobalCachedInvalidatesCachedBuiltinToBeShadowed) {
  HandleScope scope(thread_);
  EXPECT_FALSE(runFromCStr(runtime_, R"(
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
  Function function(&scope, mainModuleAt(runtime_, "foo"));
  ASSERT_TRUE(isStrEqualsCStr(
      Tuple::cast(Code::cast(function.code()).names()).at(0), "a"));
  MutableTuple caches(&scope, function.caches());
  EXPECT_TRUE(icLookupGlobalVar(*caches, 0).isNoneType());
}

TEST_F(InterpreterTest, DeleteGlobalInvalidatesCachedValue) {
  HandleScope scope(thread_);
  EXPECT_FALSE(runFromCStr(runtime_, R"(
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
  Function function(&scope, mainModuleAt(runtime_, "foo"));
  ASSERT_TRUE(isStrEqualsCStr(
      Tuple::cast(Code::cast(function.code()).names()).at(0), "a"));
  MutableTuple caches(&scope, function.caches());
  EXPECT_TRUE(icLookupGlobalVar(*caches, 0).isNoneType());
}

TEST_F(InterpreterTest, StoreNameInvalidatesCachedBuiltinToBeShadowed) {
  HandleScope scope(thread_);
  EXPECT_FALSE(runFromCStr(runtime_, R"(
__builtins__.a = 400

def foo():
  return a + a

foo()
a = 800
)")
                   .isError());
  Function function(&scope, mainModuleAt(runtime_, "foo"));
  ASSERT_TRUE(isStrEqualsCStr(
      Tuple::cast(Code::cast(function.code()).names()).at(0), "a"));
  MutableTuple caches(&scope, function.caches());
  EXPECT_TRUE(icLookupGlobalVar(*caches, 0).isNoneType());
}

TEST_F(InterpreterTest, DeleteNameInvalidatesCachedGlobalVar) {
  HandleScope scope(thread_);
  EXPECT_FALSE(runFromCStr(runtime_, R"(
a = 400
def foo():
  return a + a

foo()
del a
)")
                   .isError());
  Function function(&scope, mainModuleAt(runtime_, "foo"));
  ASSERT_TRUE(isStrEqualsCStr(
      Tuple::cast(Code::cast(function.code()).names()).at(0), "a"));
  MutableTuple caches(&scope, function.caches());
  EXPECT_TRUE(icLookupGlobalVar(*caches, 0).isNoneType());
}

TEST_F(
    InterpreterTest,
    StoreAttrCachedInvalidatesInstanceOffsetCachesByAssigningTypeDescriptor) {
  HandleScope scope(thread_);
  EXPECT_FALSE(runFromCStr(runtime_, R"(
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
  Object c(&scope, mainModuleAt(runtime_, "c"));
  Function get_foo(&scope, mainModuleAt(runtime_, "get_foo"));
  Function do_not_invalidate0(&scope,
                              mainModuleAt(runtime_, "do_not_invalidate0"));
  Function do_not_invalidate1(&scope,
                              mainModuleAt(runtime_, "do_not_invalidate1"));
  Function invalidate(&scope, mainModuleAt(runtime_, "invalidate"));
  MutableTuple caches(&scope, get_foo.caches());
  // Load the cache
  ASSERT_TRUE(icLookupAttr(*caches, 1, c.layoutId()).isErrorNotFound());
  ASSERT_TRUE(isIntEqualsWord(Interpreter::call1(thread_, get_foo, c), 400));
  ASSERT_TRUE(icLookupAttr(*caches, 1, c.layoutId()).isSmallInt());

  // Assign a data descriptor to a different attribute name.
  ASSERT_TRUE(Interpreter::call0(thread_, do_not_invalidate0).isNoneType());
  EXPECT_TRUE(icLookupAttr(*caches, 1, c.layoutId()).isSmallInt());

  // Assign a non-data descriptor to the cache's attribute name.
  ASSERT_TRUE(Interpreter::call0(thread_, do_not_invalidate1).isNoneType());
  EXPECT_TRUE(icLookupAttr(*caches, 1, c.layoutId()).isSmallInt());

  // Assign a data descriptor the cache's attribute name that actually causes
  // invalidation.
  ASSERT_TRUE(Interpreter::call0(thread_, invalidate).isNoneType());
  // Verify that the cache is empty and calling get_foo() returns a fresh value.
  EXPECT_TRUE(icLookupAttr(*caches, 1, c.layoutId()).isErrorNotFound());
  EXPECT_TRUE(isStrEqualsCStr(Interpreter::call1(thread_, get_foo, c),
                              "data descriptor"));
}

TEST_F(InterpreterTest,
       StoreAttrCachedInvalidatesTypeAttrCachesByUpdatingTypeAttribute) {
  HandleScope scope(thread_);
  EXPECT_FALSE(runFromCStr(runtime_, R"(
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
  Object c(&scope, mainModuleAt(runtime_, "c"));
  Function old_foo(&scope, mainModuleAt(runtime_, "old_foo"));
  Function call_foo(&scope, mainModuleAt(runtime_, "call_foo"));
  Function do_not_invalidate(&scope,
                             mainModuleAt(runtime_, "do_not_invalidate"));
  Function invalidate(&scope, mainModuleAt(runtime_, "invalidate"));
  MutableTuple caches(&scope, call_foo.caches());
  // Load the cache
  ASSERT_TRUE(icLookupAttr(*caches, 1, c.layoutId()).isErrorNotFound());
  ASSERT_TRUE(isIntEqualsWord(Interpreter::call1(thread_, call_foo, c), 400));
  ASSERT_EQ(icLookupAttr(*caches, 1, c.layoutId()), *old_foo);

  // Assign a non-data descriptor to different attribute name.
  ASSERT_TRUE(Interpreter::call0(thread_, do_not_invalidate).isNoneType());
  ASSERT_EQ(icLookupAttr(*caches, 1, c.layoutId()), *old_foo);

  // Invalidate the cache.
  ASSERT_TRUE(Interpreter::call0(thread_, invalidate).isNoneType());
  // Verify that the cache is empty and calling get_foo() returns a fresh value.
  EXPECT_TRUE(icLookupAttr(*caches, 1, c.layoutId()).isErrorNotFound());
  EXPECT_TRUE(isStrEqualsCStr(Interpreter::call1(thread_, call_foo, c),
                              "new type attr"));
}

TEST_F(
    InterpreterTest,
    StoreAttrCachedInvalidatesAttributeCachesByUpdatingMatchingTypeAttributesOfSuperclass) {
  HandleScope scope(thread_);
  EXPECT_FALSE(runFromCStr(runtime_, R"(
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
  Type type_b(&scope, mainModuleAt(runtime_, "B"));
  Type type_c(&scope, mainModuleAt(runtime_, "C"));
  Object c(&scope, mainModuleAt(runtime_, "c"));
  Function get_foo(&scope, mainModuleAt(runtime_, "get_foo"));
  Function do_not_invalidate(&scope,
                             mainModuleAt(runtime_, "do_not_invalidate"));
  Function invalidate(&scope, mainModuleAt(runtime_, "invalidate"));
  MutableTuple caches(&scope, get_foo.caches());
  // Load the cache.
  ASSERT_TRUE(icLookupAttr(*caches, 1, c.layoutId()).isErrorNotFound());
  ASSERT_TRUE(isIntEqualsWord(Interpreter::call1(thread_, get_foo, c), 400));
  ASSERT_TRUE(icLookupAttr(*caches, 1, c.layoutId()).isSmallInt());

  // Updating a subclass' type attribute doesn't invalidate the cache.
  ASSERT_TRUE(Interpreter::call0(thread_, do_not_invalidate).isNoneType());
  ASSERT_TRUE(icLookupAttr(*caches, 1, c.layoutId()).isSmallInt());

  // Verify that all type dictionaries in C's mro have dependentices to get_foo.
  Object foo_name(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  Object result(&scope, typeValueCellAt(type_b, foo_name));
  ASSERT_TRUE(result.isValueCell());
  ASSERT_TRUE(ValueCell::cast(*result).dependencyLink().isWeakLink());
  EXPECT_EQ(
      WeakLink::cast(ValueCell::cast(*result).dependencyLink()).referent(),
      *get_foo);

  result = typeValueCellAt(type_c, foo_name);
  ASSERT_TRUE(result.isValueCell());
  ASSERT_TRUE(ValueCell::cast(*result).dependencyLink().isWeakLink());
  EXPECT_EQ(
      WeakLink::cast(ValueCell::cast(*result).dependencyLink()).referent(),
      *get_foo);

  // Invalidate the cache.
  ASSERT_TRUE(Interpreter::call0(thread_, invalidate).isNoneType());
  // Verify that the cache is empty and calling get_foo() returns a fresh value.
  EXPECT_TRUE(icLookupAttr(*caches, 1, c.layoutId()).isErrorNotFound());
  EXPECT_TRUE(isStrEqualsCStr(Interpreter::call1(thread_, get_foo, c),
                              "data descriptor"));
}

TEST_F(InterpreterTest, StoreAttrCachedInvalidatesBinaryOpCaches) {
  HandleScope scope(thread_);
  EXPECT_FALSE(runFromCStr(runtime_, R"(
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
  Object a(&scope, mainModuleAt(runtime_, "a"));
  Object b(&scope, mainModuleAt(runtime_, "b"));
  Object a_add(&scope, mainModuleAt(runtime_, "A_add"));

  Function cache_a_add(&scope, mainModuleAt(runtime_, "cache_A_add"));
  BinaryOpFlags flags_out;
  // Ensure that A.__add__ is cached in cache_A_add.
  Object cached_in_cache_a_add(
      &scope, icLookupBinaryOp(MutableTuple::cast(cache_a_add.caches()), 0,
                               a.layoutId(), b.layoutId(), &flags_out));
  ASSERT_EQ(cached_in_cache_a_add, *a_add);

  // Ensure that cache_a_add is being tracked as a dependent from A.__add__.
  Type type_a(&scope, mainModuleAt(runtime_, "A"));
  Str dunder_add(&scope, runtime_->symbols()->at(ID(__add__)));
  ValueCell a_add_value_cell(&scope, typeValueCellAt(type_a, dunder_add));
  ASSERT_FALSE(a_add_value_cell.isPlaceholder());
  EXPECT_EQ(WeakLink::cast(a_add_value_cell.dependencyLink()).referent(),
            *cache_a_add);

  // Ensure that cache_a_add is being tracked as a dependent from B.__radd__.
  Type type_b(&scope, mainModuleAt(runtime_, "B"));
  Str dunder_radd(&scope, runtime_->symbols()->at(ID(__radd__)));
  ValueCell b_radd_value_cell(&scope, typeValueCellAt(type_b, dunder_radd));
  ASSERT_TRUE(b_radd_value_cell.isPlaceholder());
  EXPECT_EQ(WeakLink::cast(b_radd_value_cell.dependencyLink()).referent(),
            *cache_a_add);

  // Updating A.__add__ invalidates the cache.
  Function invalidate(&scope, mainModuleAt(runtime_, "update_A_add"));
  ASSERT_TRUE(Interpreter::call0(thread_, invalidate).isNoneType());
  // Verify that the cache is evicted.
  EXPECT_TRUE(icLookupBinaryOp(MutableTuple::cast(cache_a_add.caches()), 0,
                               a.layoutId(), b.layoutId(), &flags_out)
                  .isErrorNotFound());
  // Verify that the dependencies are deleted.
  EXPECT_TRUE(a_add_value_cell.dependencyLink().isNoneType());
  EXPECT_TRUE(b_radd_value_cell.dependencyLink().isNoneType());
}

TEST_F(InterpreterTest, StoreAttrCachedInvalidatesCompareOpTypeAttrCaches) {
  HandleScope scope(thread_);
  EXPECT_FALSE(runFromCStr(runtime_, R"(
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
  Object a(&scope, mainModuleAt(runtime_, "a"));
  Object b(&scope, mainModuleAt(runtime_, "b"));
  Object type_a__dunder_ge(&scope, mainModuleAt(runtime_, "A__ge__"));

  // Ensure that A.__ge__ is cached.
  Function cache_compare_op(&scope, mainModuleAt(runtime_, "cache_compare_op"));
  MutableTuple caches(&scope, cache_compare_op.caches());
  BinaryOpFlags flags_out;
  Object cached(&scope, icLookupBinaryOp(*caches, 0, a.layoutId(), b.layoutId(),
                                         &flags_out));
  ASSERT_EQ(*cached, *type_a__dunder_ge);

  // Updating irrelevant compare op dunder functions doesn't trigger
  // invalidation.
  Function do_not_invalidate(&scope,
                             mainModuleAt(runtime_, "do_not_invalidate"));
  ASSERT_TRUE(Interpreter::call0(thread_, do_not_invalidate).isNoneType());
  cached = icLookupBinaryOp(*caches, 0, a.layoutId(), b.layoutId(), &flags_out);
  EXPECT_EQ(*cached, *type_a__dunder_ge);

  // Updating relevant compare op dunder functions triggers invalidation.
  Function invalidate(&scope, mainModuleAt(runtime_, "invalidate"));
  ASSERT_TRUE(Interpreter::call0(thread_, invalidate).isNoneType());
  ASSERT_TRUE(
      icLookupBinaryOp(*caches, 0, a.layoutId(), b.layoutId(), &flags_out)
          .isErrorNotFound());
}

TEST_F(InterpreterTest, StoreAttrCachedInvalidatesInplaceOpCaches) {
  HandleScope scope(thread_);
  EXPECT_FALSE(runFromCStr(runtime_, R"(
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
  Object a(&scope, mainModuleAt(runtime_, "a"));
  Object b(&scope, mainModuleAt(runtime_, "b"));
  Object a_iadd(&scope, mainModuleAt(runtime_, "A_iadd"));

  Function cache_a_iadd(&scope, mainModuleAt(runtime_, "cache_A_iadd"));
  BinaryOpFlags flags_out;
  // Ensure that A.__iadd__ is cached in cache_A_iadd.
  Object cached_in_cache_a_iadd(
      &scope, icLookupBinaryOp(MutableTuple::cast(cache_a_iadd.caches()), 0,
                               a.layoutId(), b.layoutId(), &flags_out));
  ASSERT_EQ(cached_in_cache_a_iadd, *a_iadd);

  // Ensure that cache_a_iadd is being tracked as a dependent from A.__iadd__.
  Type type_a(&scope, mainModuleAt(runtime_, "A"));
  Str dunder_iadd(&scope, runtime_->symbols()->at(ID(__iadd__)));
  ValueCell a_iadd_value_cell(&scope, typeValueCellAt(type_a, dunder_iadd));
  ASSERT_FALSE(a_iadd_value_cell.isPlaceholder());
  EXPECT_EQ(WeakLink::cast(a_iadd_value_cell.dependencyLink()).referent(),
            *cache_a_iadd);

  Str dunder_add(&scope, runtime_->symbols()->at(ID(__add__)));
  ValueCell a_add_value_cell(&scope, typeValueCellAt(type_a, dunder_add));
  ASSERT_TRUE(a_add_value_cell.isPlaceholder());
  EXPECT_EQ(WeakLink::cast(a_add_value_cell.dependencyLink()).referent(),
            *cache_a_iadd);

  // Ensure that cache_a_iadd is being tracked as a dependent from B.__riadd__.
  Type type_b(&scope, mainModuleAt(runtime_, "B"));
  Str dunder_radd(&scope, runtime_->symbols()->at(ID(__radd__)));
  ValueCell b_radd_value_cell(&scope, typeValueCellAt(type_b, dunder_radd));
  ASSERT_TRUE(b_radd_value_cell.isPlaceholder());
  EXPECT_EQ(WeakLink::cast(b_radd_value_cell.dependencyLink()).referent(),
            *cache_a_iadd);

  // Updating A.__iadd__ invalidates the cache.
  Function invalidate(&scope, mainModuleAt(runtime_, "update_A_iadd"));
  ASSERT_TRUE(Interpreter::call0(thread_, invalidate).isNoneType());
  // Verify that the cache is evicted.
  EXPECT_TRUE(icLookupBinaryOp(MutableTuple::cast(cache_a_iadd.caches()), 0,
                               a.layoutId(), b.layoutId(), &flags_out)
                  .isErrorNotFound());
  // Verify that the dependencies are deleted.
  EXPECT_TRUE(a_iadd_value_cell.dependencyLink().isNoneType());
  EXPECT_TRUE(a_add_value_cell.dependencyLink().isNoneType());
  EXPECT_TRUE(b_radd_value_cell.dependencyLink().isNoneType());
}

TEST_F(InterpreterTest, LoadMethodLoadingMethodFollowedByCallMethod) {
  HandleScope scope(thread_);
  EXPECT_FALSE(runFromCStr(runtime_, R"(
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
  Function test_function(&scope, mainModuleAt(runtime_, "test"));
  MutableBytes bytecode(&scope, test_function.rewrittenBytecode());
  ASSERT_EQ(bytecode.byteAt(2), LOAD_METHOD_ANAMORPHIC);
  ASSERT_EQ(bytecode.byteAt(8), CALL_METHOD);

  EXPECT_TRUE(isIntEqualsWord(Interpreter::call0(thread_, test_function), 70));
}

TEST_F(InterpreterTest, LoadMethodInitDoesNotCacheInstanceAttributes) {
  HandleScope scope(thread_);
  EXPECT_FALSE(runFromCStr(runtime_, R"(
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
  Function test_function(&scope, mainModuleAt(runtime_, "test"));
  MutableBytes bytecode(&scope, test_function.rewrittenBytecode());
  ASSERT_EQ(bytecode.byteAt(2), LOAD_METHOD_ANAMORPHIC);
  ASSERT_EQ(bytecode.byteAt(8), CALL_METHOD);

  Object c(&scope, mainModuleAt(runtime_, "c"));
  LayoutId layout_id = c.layoutId();
  MutableTuple caches(&scope, test_function.caches());
  // Cache miss.
  ASSERT_TRUE(
      icLookupAttr(*caches, bytecode.byteAt(3), layout_id).isErrorNotFound());
  EXPECT_TRUE(isIntEqualsWord(Interpreter::call0(thread_, test_function), 30));

  // Still cache miss.
  ASSERT_TRUE(
      icLookupAttr(*caches, bytecode.byteAt(3), layout_id).isErrorNotFound());
}

TEST_F(InterpreterTest, LoadMethodCachedCachingFunctionFollowedByCallMethod) {
  HandleScope scope(thread_);
  EXPECT_FALSE(runFromCStr(runtime_, R"(
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
  Function test_function(&scope, mainModuleAt(runtime_, "test"));
  MutableBytes bytecode(&scope, test_function.rewrittenBytecode());
  ASSERT_EQ(bytecode.byteAt(2), LOAD_METHOD_ANAMORPHIC);
  ASSERT_EQ(bytecode.byteAt(8), CALL_METHOD);

  // Cache miss.
  Object c(&scope, mainModuleAt(runtime_, "c"));
  LayoutId layout_id = c.layoutId();
  MutableTuple caches(&scope, test_function.caches());
  ASSERT_TRUE(
      icLookupAttr(*caches, bytecode.byteAt(3), layout_id).isErrorNotFound());
  EXPECT_TRUE(isIntEqualsWord(Interpreter::call0(thread_, test_function), 70));

  // Cache hit.
  ASSERT_TRUE(
      icLookupAttr(*caches, bytecode.byteAt(3), layout_id).isFunction());
  EXPECT_TRUE(isIntEqualsWord(Interpreter::call0(thread_, test_function), 70));
}

TEST_F(InterpreterTest, LoadMethodCachedDoesNotCacheProperty) {
  HandleScope scope(thread_);
  EXPECT_FALSE(runFromCStr(runtime_, R"(
class C:
  @property
  def foo(self): return lambda: 1234

def call_foo(c):
  return c.foo()

c = C()
call_foo(c)
)")
                   .isError());
  Function call_foo(&scope, mainModuleAt(runtime_, "call_foo"));
  MutableBytes bytecode(&scope, call_foo.rewrittenBytecode());
  ASSERT_EQ(bytecode.byteAt(2), LOAD_METHOD_ANAMORPHIC);
  ASSERT_EQ(bytecode.byteAt(4), CALL_METHOD);

  MutableTuple caches(&scope, call_foo.caches());
  EXPECT_TRUE(icIsCacheEmpty(caches, bytecode.byteAt(3)));
}

TEST_F(InterpreterTest, LoadMethodUpdatesOpcodeWithCaching) {
  HandleScope scope(thread_);
  EXPECT_FALSE(runFromCStr(runtime_, R"(
class C:
  def foo(self):
    return 4

class D:
  def foo(self):
    return -4

def test(c):
  return c.foo()

c = C()
d = D()
)")
                   .isError());
  Function test_function(&scope, mainModuleAt(runtime_, "test"));
  Object c(&scope, mainModuleAt(runtime_, "c"));
  Object d(&scope, mainModuleAt(runtime_, "d"));
  MutableBytes bytecode(&scope, test_function.rewrittenBytecode());
  ASSERT_EQ(bytecode.byteAt(2), LOAD_METHOD_ANAMORPHIC);
  ASSERT_TRUE(
      isIntEqualsWord(Interpreter::call1(thread_, test_function, c), 4));
  EXPECT_EQ(bytecode.byteAt(2), LOAD_METHOD_INSTANCE_FUNCTION);

  ASSERT_TRUE(
      isIntEqualsWord(Interpreter::call1(thread_, test_function, d), -4));
  EXPECT_EQ(bytecode.byteAt(2), LOAD_METHOD_POLYMORPHIC);
}

TEST_F(InterpreterTest, DoLoadImmediate) {
  HandleScope scope(thread_);
  EXPECT_FALSE(runFromCStr(runtime_, R"(
def test():
  return None

result = test()
)")
                   .isError());
  Function test_function(&scope, mainModuleAt(runtime_, "test"));
  MutableBytes bytecode(&scope, test_function.rewrittenBytecode());
  // Verify that rewriting replaces LOAD_CONST for LOAD_IMMEDIATE.
  EXPECT_EQ(bytecode.byteAt(0), LOAD_IMMEDIATE);
  EXPECT_EQ(bytecode.byteAt(1), static_cast<byte>(NoneType::object().raw()));
  EXPECT_TRUE(mainModuleAt(runtime_, "result").isNoneType());
}

TEST_F(InterpreterTest, LoadAttrCachedInsertsExecutingFunctionAsDependent) {
  EXPECT_FALSE(runFromCStr(runtime_, R"(
class C:
  def __init__(self):
    self.foo = 400

def cache_attribute(c):
  return c.foo

c = C()
)")
                   .isError());
  HandleScope scope(thread_);
  Type type_c(&scope, mainModuleAt(runtime_, "C"));
  Object c(&scope, mainModuleAt(runtime_, "c"));
  Function cache_attribute(&scope, mainModuleAt(runtime_, "cache_attribute"));
  MutableTuple caches(&scope, cache_attribute.caches());
  ASSERT_EQ(caches.length(), 2 * kIcPointersPerEntry);

  // Load the cache.
  ASSERT_TRUE(icLookupAttr(*caches, 1, c.layoutId()).isErrorNotFound());
  ASSERT_TRUE(
      isIntEqualsWord(Interpreter::call1(thread_, cache_attribute, c), 400));
  ASSERT_TRUE(icLookupAttr(*caches, 1, c.layoutId()).isSmallInt());

  // Verify that cache_attribute function is added as a dependent.
  Object foo_name(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  ValueCell value_cell(&scope, typeValueCellAt(type_c, foo_name));
  ASSERT_TRUE(value_cell.dependencyLink().isWeakLink());
  EXPECT_EQ(WeakLink::cast(value_cell.dependencyLink()).referent(),
            *cache_attribute);
}

TEST_F(InterpreterTest,
       LoadAttrInstanceOnInvalidatedCacheUpdatesCacheCorrectly) {
  EXPECT_FALSE(runFromCStr(runtime_, R"(
class C:
  def __init__(self):
    self.foo = "instance attribute"

def cache_attribute(c):
  return c.foo

def invalidate_attribute(c):
  C.foo = property(lambda e: "descriptor attribute")

c = C()
)")
                   .isError());
  HandleScope scope(thread_);
  Object c(&scope, mainModuleAt(runtime_, "c"));
  Function cache_attribute(&scope, mainModuleAt(runtime_, "cache_attribute"));
  MutableBytes bytecode(&scope, cache_attribute.rewrittenBytecode());
  ASSERT_EQ(bytecode.byteAt(2), LOAD_ATTR_ANAMORPHIC);
  Tuple caches(&scope, cache_attribute.caches());
  ASSERT_EQ(caches.length(), 2 * kIcPointersPerEntry);

  // Load the cache.
  ASSERT_EQ(icCurrentState(*caches, 1), ICState::kAnamorphic);
  ASSERT_TRUE(isStrEqualsCStr(Interpreter::call1(thread_, cache_attribute, c),
                              "instance attribute"));
  ASSERT_EQ(icCurrentState(*caches, 1), ICState::kMonomorphic);
  ASSERT_EQ(bytecode.byteAt(2), LOAD_ATTR_INSTANCE);

  // Invalidate the cache.
  Function invalidate_attribute(&scope,
                                mainModuleAt(runtime_, "invalidate_attribute"));
  ASSERT_TRUE(
      Interpreter::call1(thread_, invalidate_attribute, c).isNoneType());
  ASSERT_EQ(icCurrentState(*caches, 1), ICState::kAnamorphic);
  ASSERT_EQ(bytecode.byteAt(2), LOAD_ATTR_INSTANCE);

  // Load the cache again.
  EXPECT_TRUE(isStrEqualsCStr(Interpreter::call1(thread_, cache_attribute, c),
                              "descriptor attribute"));
  EXPECT_EQ(icCurrentState(*caches, 1), ICState::kMonomorphic);
  EXPECT_EQ(bytecode.byteAt(2), LOAD_ATTR_INSTANCE_PROPERTY);
}

TEST_F(InterpreterTest, StoreAttrCachedInsertsExecutingFunctionAsDependent) {
  EXPECT_FALSE(runFromCStr(runtime_, R"(
class C:
  def __init__(self):
    self.foo = 400

def cache_attribute(c):
  c.foo = 500

c = C()
)")
                   .isError());
  HandleScope scope(thread_);
  Type type_c(&scope, mainModuleAt(runtime_, "C"));
  Object c(&scope, mainModuleAt(runtime_, "c"));
  Function cache_attribute(&scope, mainModuleAt(runtime_, "cache_attribute"));
  MutableTuple caches(&scope, cache_attribute.caches());
  ASSERT_EQ(caches.length(), 2 * kIcPointersPerEntry);

  // Load the cache.
  ASSERT_TRUE(icLookupAttr(*caches, 1, c.layoutId()).isErrorNotFound());
  ASSERT_TRUE(Interpreter::call1(thread_, cache_attribute, c).isNoneType());
  ASSERT_TRUE(icLookupAttr(*caches, 1, c.layoutId()).isSmallInt());

  // Verify that cache_attribute function is added as a dependent.
  Object foo_name(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  ValueCell value_cell(&scope, typeValueCellAt(type_c, foo_name));
  ASSERT_TRUE(value_cell.dependencyLink().isWeakLink());
  EXPECT_EQ(WeakLink::cast(value_cell.dependencyLink()).referent(),
            *cache_attribute);
}

TEST_F(InterpreterTest, StoreAttrsCausingShadowingInvalidatesCache) {
  EXPECT_FALSE(runFromCStr(runtime_, R"(
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
  Type type_a(&scope, mainModuleAt(runtime_, "A"));
  Type type_b(&scope, mainModuleAt(runtime_, "B"));
  Type type_c(&scope, mainModuleAt(runtime_, "C"));
  Object a(&scope, mainModuleAt(runtime_, "a"));
  Object b(&scope, mainModuleAt(runtime_, "b"));
  Object c(&scope, mainModuleAt(runtime_, "c"));
  Function function_that_caches_attr_lookup(
      &scope, mainModuleAt(runtime_, "function_that_caches_attr_lookup"));
  MutableTuple caches(&scope, function_that_caches_attr_lookup.caches());
  // 0: global variable
  // 1: a.foo
  // 2: b.foo
  // 3: binary op cache
  // 4: c.foo
  // 5, binary op cache
  Function a_foo(&scope, mainModuleAt(runtime_, "a_foo"));
  Function b_foo(&scope, mainModuleAt(runtime_, "b_foo"));
  ASSERT_EQ(caches.length(), 6 * kIcPointersPerEntry);
  ASSERT_EQ(icLookupAttr(*caches, 1, a.layoutId()), *a_foo);
  ASSERT_EQ(icLookupAttr(*caches, 2, b.layoutId()), *b_foo);
  ASSERT_EQ(icLookupAttr(*caches, 4, c.layoutId()), *b_foo);

  // Verify that function_that_caches_attr_lookup cached the attribute lookup
  // and appears on the dependency list of A.foo.
  Object foo_name(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  ValueCell foo_in_a(&scope, typeValueCellAt(type_a, foo_name));
  ASSERT_TRUE(foo_in_a.dependencyLink().isWeakLink());
  ASSERT_EQ(WeakLink::cast(foo_in_a.dependencyLink()).referent(),
            *function_that_caches_attr_lookup);

  // Verify that function_that_caches_attr_lookup cached the attribute lookup
  // and appears on the dependency list of B.foo.
  ValueCell foo_in_b(&scope, typeValueCellAt(type_b, foo_name));
  ASSERT_TRUE(foo_in_b.dependencyLink().isWeakLink());
  ASSERT_EQ(WeakLink::cast(foo_in_b.dependencyLink()).referent(),
            *function_that_caches_attr_lookup);

  // Verify that function_that_caches_attr_lookup cached the attribute lookup
  // and appears on the dependency list of C.foo.
  ValueCell foo_in_c(&scope, typeValueCellAt(type_c, foo_name));
  ASSERT_TRUE(foo_in_c.dependencyLink().isWeakLink());
  ASSERT_EQ(WeakLink::cast(foo_in_c.dependencyLink()).referent(),
            *function_that_caches_attr_lookup);

  // Change the class A so that any caches that reference A.foo are invalidated.
  Function func_that_causes_shadowing_of_attr_a(
      &scope, mainModuleAt(runtime_, "func_that_causes_shadowing_of_attr_a"));
  ASSERT_TRUE(Interpreter::call0(thread_, func_that_causes_shadowing_of_attr_a)
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
      &scope, mainModuleAt(runtime_, "func_that_causes_shadowing_of_attr_b"));
  ASSERT_TRUE(Interpreter::call0(thread_, func_that_causes_shadowing_of_attr_b)
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

TEST_F(InterpreterTest, IntrinsicWithSlowPathDoesNotAlterStack) {
  HandleScope scope(thread_);
  Object obj(&scope, runtime_->newList());
  thread_->stackPush(*obj);
  Module module(&scope, runtime_->findModuleById(ID(_builtins)));
  Function tuple_len_func(&scope,
                          moduleAtById(thread_, module, ID(_tuple_len)));
  IntrinsicFunction function =
      reinterpret_cast<IntrinsicFunction>(tuple_len_func.intrinsic());
  ASSERT_NE(function, nullptr);
  ASSERT_FALSE(function(thread_));
  EXPECT_EQ(thread_->stackPeek(0), *obj);
}

}  // namespace testing
}  // namespace py
