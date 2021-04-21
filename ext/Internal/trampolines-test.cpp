#include <memory>

#include "cpython-data.h"
#include "gtest/gtest.h"

#include "api-handle.h"
#include "dict-builtins.h"
#include "function-utils.h"
#include "runtime.h"
#include "test-utils.h"

namespace py {
namespace testing {

using MethodTrampolinesTest = RuntimeFixture;

static PyObject* capiFunctionNoArgs(PyObject* self, PyObject* args) {
  Runtime* runtime = Thread::current()->runtime();
  runtime->collectGarbage();
  ApiHandle* handle = ApiHandle::fromPyObject(self);
  EXPECT_GT(handle->refcnt(), 0);
  EXPECT_TRUE(isStrEqualsCStr(handle->asObject(), "the self argument"));
  EXPECT_EQ(args, nullptr);
  return ApiHandle::newReference(runtime, SmallInt::fromWord(1230));
}

static RawObject newFunctionNoArgs(Thread* thread) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object name(&scope, runtime->newStrFromCStr("foo"));
  PyCFunction function_ptr = capiFunctionNoArgs;
  return newExtensionFunction(
      thread, name, reinterpret_cast<void*>(function_ptr), METH_NOARGS);
}

TEST_F(MethodTrampolinesTest, NoArgs) {
  HandleScope scope(thread_);
  Object function(&scope, newFunctionNoArgs(thread_));
  Object arg0(&scope, runtime_->newStrFromCStr("the self argument"));
  EXPECT_TRUE(
      isIntEqualsWord(Interpreter::call1(thread_, function, arg0), 1230));
}

TEST_F(MethodTrampolinesTest, NoArgsWithoutSelfRaisesTypeError) {
  HandleScope scope(thread_);
  Function function(&scope, newFunctionNoArgs(thread_));
  EXPECT_TRUE(raisedWithStr(Interpreter::call0(thread_, function),
                            LayoutId::kTypeError,
                            "'foo' must be bound to an object"));
}

TEST_F(MethodTrampolinesTest, NoArgsWithTooManyArgsRaisesTypeError) {
  HandleScope scope(thread_);
  Function function(&scope, newFunctionNoArgs(thread_));
  Object arg0(&scope, runtime_->newStrFromCStr("the self argument"));
  Object arg1(&scope, runtime_->newStrFromCStr("bar"));
  EXPECT_TRUE(raisedWithStr(Interpreter::call2(thread_, function, arg0, arg1),
                            LayoutId::kTypeError,
                            "'foo' takes no arguments (1 given)"));
}

TEST_F(MethodTrampolinesTest, NoArgsKw) {
  thread_->stackPush(newFunctionNoArgs(thread_));
  thread_->stackPush(runtime_->newStrFromCStr("the self argument"));
  thread_->stackPush(runtime_->emptyTuple());
  EXPECT_TRUE(isIntEqualsWord(Interpreter::callKw(thread_, 1), 1230));
}

TEST_F(MethodTrampolinesTest, NoArgsKwWithoutSelfRaisesTypeError) {
  thread_->stackPush(newFunctionNoArgs(thread_));
  thread_->stackPush(runtime_->emptyTuple());
  EXPECT_TRUE(raisedWithStr(Interpreter::callKw(thread_, 0),
                            LayoutId::kTypeError,
                            "'foo' must be bound to an object"));
}

TEST_F(MethodTrampolinesTest, NoArgsKwWithTooManyArgsRaisesTypeError) {
  thread_->stackPush(newFunctionNoArgs(thread_));
  thread_->stackPush(runtime_->newStrFromCStr("the self argument"));
  thread_->stackPush(runtime_->newStrFromCStr("bar"));
  thread_->stackPush(runtime_->emptyTuple());
  EXPECT_TRUE(raisedWithStr(Interpreter::callKw(thread_, 2),
                            LayoutId::kTypeError,
                            "'foo' takes no arguments (1 given)"));
}

TEST_F(MethodTrampolinesTest, NoArgsKwWithKeywordArgRaisesTypeError) {
  HandleScope scope(thread_);
  Object key(&scope, runtime_->newStrFromCStr("key"));
  Tuple kw_names(&scope, runtime_->newTupleWith1(key));
  thread_->stackPush(newFunctionNoArgs(thread_));
  thread_->stackPush(runtime_->newStrFromCStr("the self argument"));
  thread_->stackPush(runtime_->newStrFromCStr("value"));
  thread_->stackPush(*kw_names);
  EXPECT_TRUE(raisedWithStr(Interpreter::callKw(thread_, 2),
                            LayoutId::kTypeError,
                            "'foo' takes no keyword arguments"));
}

TEST_F(MethodTrampolinesTest, NoArgsExWithoutKwargs) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_->newStrFromCStr("the self argument"));
  Tuple args(&scope, runtime_->newTupleWith1(self));
  thread_->stackPush(newFunctionNoArgs(thread_));
  thread_->stackPush(*args);
  EXPECT_TRUE(isIntEqualsWord(Interpreter::callEx(thread_, 0), 1230));
}

TEST_F(MethodTrampolinesTest, NoArgsExWithKwargs) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_->newStrFromCStr("the self argument"));
  Tuple args(&scope, runtime_->newTupleWith1(self));
  thread_->stackPush(newFunctionNoArgs(thread_));
  thread_->stackPush(*args);
  thread_->stackPush(runtime_->newDict());
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::callEx(thread_, CallFunctionExFlag::VAR_KEYWORDS), 1230));
}

TEST_F(MethodTrampolinesTest, NoArgsExWithoutSelfRaisesTypeError) {
  thread_->stackPush(newFunctionNoArgs(thread_));
  thread_->stackPush(runtime_->emptyTuple());
  EXPECT_TRUE(raisedWithStr(Interpreter::callEx(thread_, 0),
                            LayoutId::kTypeError,
                            "'foo' must be bound to an object"));
}

TEST_F(MethodTrampolinesTest, NoArgsExWithArgRaisesTypeError) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_->newStrFromCStr("the self argument"));
  Object bar(&scope, runtime_->newStrFromCStr("bar"));
  Object num(&scope, runtime_->newInt(88));
  Tuple args(&scope, runtime_->newTupleWith3(self, bar, num));
  thread_->stackPush(newFunctionNoArgs(thread_));
  thread_->stackPush(*args);
  EXPECT_TRUE(raisedWithStr(Interpreter::callEx(thread_, 0),
                            LayoutId::kTypeError,
                            "'foo' takes no arguments (2 given)"));
}

TEST_F(MethodTrampolinesTest, NoArgsExWithKeywordArgRaisesTypeError) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_->newStrFromCStr("the self argument"));
  Tuple args(&scope, runtime_->newTupleWith1(self));
  Dict kwargs(&scope, runtime_->newDict());
  Object value(&scope, runtime_->newStrFromCStr("value"));
  dictAtPutById(thread_, kwargs, ID(key), value);
  thread_->stackPush(newFunctionNoArgs(thread_));
  thread_->stackPush(*args);
  thread_->stackPush(*kwargs);
  EXPECT_TRUE(raisedWithStr(
      Interpreter::callEx(thread_, CallFunctionExFlag::VAR_KEYWORDS),
      LayoutId::kTypeError, "'foo' takes no keyword arguments"));
}

static PyObject* capiFunctionOneArg(PyObject* self, PyObject* arg) {
  Runtime* runtime = Thread::current()->runtime();
  runtime->collectGarbage();
  ApiHandle* handle = ApiHandle::fromPyObject(self);
  EXPECT_GT(handle->refcnt(), 0);
  EXPECT_TRUE(isStrEqualsCStr(handle->asObject(), "the self argument"));
  ApiHandle* arg_handle = ApiHandle::fromPyObject(arg);
  EXPECT_GT(arg_handle->refcnt(), 0);
  EXPECT_TRUE(isFloatEqualsDouble(arg_handle->asObject(), 42.5));
  return ApiHandle::newReference(runtime, SmallInt::fromWord(1231));
}

static RawObject newFunctionOneArg(Thread* thread) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object name(&scope, runtime->newStrFromCStr("foo"));
  PyCFunction function_ptr = capiFunctionOneArg;
  return newExtensionFunction(thread, name,
                              reinterpret_cast<void*>(function_ptr), METH_O);
}

TEST_F(MethodTrampolinesTest, OneArg) {
  HandleScope scope(thread_);
  Function function(&scope, newFunctionOneArg(thread_));
  Object arg0(&scope, runtime_->newStrFromCStr("the self argument"));
  Object arg1(&scope, runtime_->newFloat(42.5));
  EXPECT_TRUE(
      isIntEqualsWord(Interpreter::call2(thread_, function, arg0, arg1), 1231));
}

TEST_F(MethodTrampolinesTest, OneArgWithoutSelfRaisesTypeError) {
  HandleScope scope(thread_);
  Function function(&scope, newFunctionOneArg(thread_));
  EXPECT_TRUE(raisedWithStr(Interpreter::call0(thread_, function),
                            LayoutId::kTypeError,
                            "'foo' must be bound to an object"));
}

TEST_F(MethodTrampolinesTest, OneArgWithTooManyArgsRaisesTypeError) {
  HandleScope scope(thread_);
  Function function(&scope, newFunctionOneArg(thread_));
  Object arg0(&scope, runtime_->newStrFromCStr("the self argument"));
  Object arg1(&scope, runtime_->newFloat(42.5));
  Object arg2(&scope, runtime_->newInt(5));
  Object arg3(&scope, runtime_->newInt(7));
  EXPECT_TRUE(raisedWithStr(
      Interpreter::call4(thread_, function, arg0, arg1, arg2, arg3),
      LayoutId::kTypeError, "'foo' takes exactly one argument (3 given)"));
}

TEST_F(MethodTrampolinesTest, OneArgKw) {
  thread_->stackPush(newFunctionOneArg(thread_));
  thread_->stackPush(runtime_->newStrFromCStr("the self argument"));
  thread_->stackPush(runtime_->newFloat(42.5));
  thread_->stackPush(runtime_->emptyTuple());
  EXPECT_TRUE(isIntEqualsWord(Interpreter::callKw(thread_, 2), 1231));
}

TEST_F(MethodTrampolinesTest, OneArgKwWithoutSelfRaisesTypeError) {
  thread_->stackPush(newFunctionOneArg(thread_));
  thread_->stackPush(runtime_->emptyTuple());
  EXPECT_TRUE(raisedWithStr(Interpreter::callKw(thread_, 0),
                            LayoutId::kTypeError,
                            "'foo' must be bound to an object"));
}

TEST_F(MethodTrampolinesTest, OneArgKwWithTooManyArgsRaisesTypeError) {
  thread_->stackPush(newFunctionOneArg(thread_));
  thread_->stackPush(runtime_->newStrFromCStr("the self argument"));
  thread_->stackPush(runtime_->newFloat(42.5));
  thread_->stackPush(runtime_->newInt(5));
  thread_->stackPush(runtime_->emptyTuple());
  EXPECT_TRUE(raisedWithStr(Interpreter::callKw(thread_, 3),
                            LayoutId::kTypeError,
                            "'foo' takes exactly one argument (2 given)"));
}

TEST_F(MethodTrampolinesTest, OneArgKwWithKeywordArgRaisesTypeError) {
  HandleScope scope(thread_);
  Object key(&scope, runtime_->newStrFromCStr("key"));
  Tuple kw_names(&scope, runtime_->newTupleWith1(key));
  thread_->stackPush(newFunctionOneArg(thread_));
  thread_->stackPush(runtime_->newFloat(42.5));
  thread_->stackPush(runtime_->newStrFromCStr("value"));
  thread_->stackPush(*kw_names);
  EXPECT_TRUE(raisedWithStr(Interpreter::callKw(thread_, 2),
                            LayoutId::kTypeError,
                            "'foo' takes no keyword arguments"));
}

TEST_F(MethodTrampolinesTest, OneArgExWithoutKwargs) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_->newStrFromCStr("the self argument"));
  Object num(&scope, runtime_->newFloat(42.5));
  Tuple args(&scope, runtime_->newTupleWith2(self, num));
  thread_->stackPush(newFunctionOneArg(thread_));
  thread_->stackPush(*args);
  EXPECT_TRUE(isIntEqualsWord(Interpreter::callEx(thread_, 0), 1231));
}

TEST_F(MethodTrampolinesTest, OneArgExWithKwargs) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_->newStrFromCStr("the self argument"));
  Object num(&scope, runtime_->newFloat(42.5));
  Tuple args(&scope, runtime_->newTupleWith2(self, num));
  thread_->stackPush(newFunctionOneArg(thread_));
  thread_->stackPush(*args);
  thread_->stackPush(runtime_->newDict());
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::callEx(thread_, CallFunctionExFlag::VAR_KEYWORDS), 1231));
}

TEST_F(MethodTrampolinesTest, OneArgExWithoutSelfRaisesTypeError) {
  thread_->stackPush(newFunctionOneArg(thread_));
  thread_->stackPush(runtime_->emptyTuple());
  EXPECT_TRUE(raisedWithStr(Interpreter::callEx(thread_, 0),
                            LayoutId::kTypeError,
                            "'foo' must be bound to an object"));
}

TEST_F(MethodTrampolinesTest, OneArgExWithTooFewArgsRaisesTypeError) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_->newStrFromCStr("the self argument"));
  Tuple args(&scope, runtime_->newTupleWith1(self));
  thread_->stackPush(newFunctionOneArg(thread_));
  thread_->stackPush(*args);
  thread_->stackPush(runtime_->newDict());
  EXPECT_TRUE(raisedWithStr(
      Interpreter::callEx(thread_, CallFunctionExFlag::VAR_KEYWORDS),
      LayoutId::kTypeError, "'foo' takes exactly one argument (0 given)"));
}

TEST_F(MethodTrampolinesTest, OneArgExWithKeywordArgRaisesTypeError) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_->newStrFromCStr("the self argument"));
  Object num(&scope, runtime_->newFloat(42.5));
  Tuple args(&scope, runtime_->newTupleWith2(self, num));
  Dict kwargs(&scope, runtime_->newDict());
  Object value(&scope, runtime_->newStrFromCStr("value"));
  dictAtPutById(thread_, kwargs, ID(key), value);
  thread_->stackPush(newFunctionOneArg(thread_));
  thread_->stackPush(*args);
  thread_->stackPush(*kwargs);
  EXPECT_TRUE(raisedWithStr(
      Interpreter::callEx(thread_, CallFunctionExFlag::VAR_KEYWORDS),
      LayoutId::kTypeError, "'foo' takes no keyword arguments"));
}

static PyObject* capiFunctionVarArgs(PyObject* self, PyObject* args) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  runtime->collectGarbage();
  ApiHandle* self_handle = ApiHandle::fromPyObject(self);
  EXPECT_GT(self_handle->refcnt(), 0);
  EXPECT_TRUE(isStrEqualsCStr(self_handle->asObject(), "the self argument"));
  ApiHandle* args_handle = ApiHandle::fromPyObject(args);
  EXPECT_GT(args_handle->refcnt(), 0);
  Object args_obj(&scope, args_handle->asObject());
  EXPECT_TRUE(args_obj.isTuple());
  Tuple args_tuple(&scope, *args_obj);
  EXPECT_EQ(args_tuple.length(), 2);
  EXPECT_TRUE(isIntEqualsWord(args_tuple.at(0), 88));
  EXPECT_TRUE(isIntEqualsWord(args_tuple.at(1), 33));
  return ApiHandle::newReference(runtime, SmallInt::fromWord(1239));
}

static RawObject newFunctionVarArgs(Thread* thread) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object name(&scope, runtime->newStrFromCStr("foo"));
  PyCFunction function_ptr = capiFunctionVarArgs;
  return newExtensionFunction(
      thread, name, reinterpret_cast<void*>(function_ptr), METH_VARARGS);
}

TEST_F(MethodTrampolinesTest, VarArgs) {
  HandleScope scope(thread_);
  Object function(&scope, newFunctionVarArgs(thread_));
  Object arg0(&scope, runtime_->newStrFromCStr("the self argument"));
  Object arg1(&scope, runtime_->newInt(88));
  Object arg2(&scope, runtime_->newInt(33));
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::call3(thread_, function, arg0, arg1, arg2), 1239));
}

TEST_F(MethodTrampolinesTest, VarArgsWithoutSelfRaisesTypeError) {
  HandleScope scope(thread_);
  Object function(&scope, newFunctionVarArgs(thread_));
  EXPECT_TRUE(raisedWithStr(Interpreter::call0(thread_, function),
                            LayoutId::kTypeError,
                            "'foo' must be bound to an object"));
}

TEST_F(MethodTrampolinesTest, VarArgsKw) {
  thread_->stackPush(newFunctionVarArgs(thread_));
  thread_->stackPush(runtime_->newStrFromCStr("the self argument"));
  thread_->stackPush(runtime_->newInt(88));
  thread_->stackPush(runtime_->newInt(33));
  thread_->stackPush(runtime_->emptyTuple());
  EXPECT_TRUE(isIntEqualsWord(Interpreter::callKw(thread_, 3), 1239));
}

TEST_F(MethodTrampolinesTest, VarArgsKwWithoutSelfRausesTypeError) {
  thread_->stackPush(newFunctionVarArgs(thread_));
  thread_->stackPush(runtime_->emptyTuple());
  EXPECT_TRUE(raisedWithStr(Interpreter::callKw(thread_, 0),
                            LayoutId::kTypeError,
                            "'foo' must be bound to an object"));
}

TEST_F(MethodTrampolinesTest, VarArgsKwWithKeywordArgRausesTypeError) {
  HandleScope scope(thread_);
  Object key(&scope, runtime_->newStrFromCStr("key"));
  Tuple kw_names(&scope, runtime_->newTupleWith1(key));
  thread_->stackPush(newFunctionVarArgs(thread_));
  thread_->stackPush(runtime_->newStrFromCStr("the self argument"));
  thread_->stackPush(runtime_->newInt(88));
  thread_->stackPush(runtime_->newInt(33));
  thread_->stackPush(runtime_->newStrFromCStr("value"));
  thread_->stackPush(*kw_names);
  EXPECT_TRUE(raisedWithStr(Interpreter::callKw(thread_, 4),
                            LayoutId::kTypeError,
                            "'foo' takes no keyword arguments"));
}

TEST_F(MethodTrampolinesTest, VarArgsExWithoutKwargs) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_->newStrFromCStr("the self argument"));
  Object num1(&scope, runtime_->newInt(88));
  Object num2(&scope, runtime_->newInt(33));
  Tuple args(&scope, runtime_->newTupleWith3(self, num1, num2));
  thread_->stackPush(newFunctionVarArgs(thread_));
  thread_->stackPush(*args);
  EXPECT_TRUE(isIntEqualsWord(Interpreter::callEx(thread_, 0), 1239));
}

TEST_F(MethodTrampolinesTest, VarArgsExWithKwargs) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_->newStrFromCStr("the self argument"));
  Object num1(&scope, runtime_->newInt(88));
  Object num2(&scope, runtime_->newInt(33));
  Tuple args(&scope, runtime_->newTupleWith3(self, num1, num2));
  thread_->stackPush(newFunctionVarArgs(thread_));
  thread_->stackPush(*args);
  thread_->stackPush(runtime_->newDict());
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::callEx(thread_, CallFunctionExFlag::VAR_KEYWORDS), 1239));
}

TEST_F(MethodTrampolinesTest, VarArgsExWithoutSelfRaisesTypeError) {
  thread_->stackPush(newFunctionVarArgs(thread_));
  thread_->stackPush(runtime_->emptyTuple());
  EXPECT_TRUE(raisedWithStr(Interpreter::callEx(thread_, 0),
                            LayoutId::kTypeError,
                            "'foo' must be bound to an object"));
}

TEST_F(MethodTrampolinesTest, VarArgsExWithKeywordArgRaisesTypeError) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_->newStrFromCStr("the self argument"));
  Object num1(&scope, runtime_->newInt(88));
  Object num2(&scope, runtime_->newInt(33));
  Tuple args(&scope, runtime_->newTupleWith3(self, num1, num2));
  Dict kwargs(&scope, runtime_->newDict());
  Object value(&scope, runtime_->newStrFromCStr("value"));
  dictAtPutById(thread_, kwargs, ID(key), value);
  thread_->stackPush(newFunctionVarArgs(thread_));
  thread_->stackPush(*args);
  thread_->stackPush(*kwargs);
  EXPECT_TRUE(raisedWithStr(
      Interpreter::callEx(thread_, CallFunctionExFlag::VAR_KEYWORDS),
      LayoutId::kTypeError, "'foo' takes no keyword arguments"));
}

static PyObject* capiFunctionKeywordsNullKwargs(PyObject* self, PyObject* args,
                                                PyObject* kwargs) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  runtime->collectGarbage();
  ApiHandle* self_handle = ApiHandle::fromPyObject(self);
  EXPECT_GT(self_handle->refcnt(), 0);
  EXPECT_TRUE(isStrEqualsCStr(self_handle->asObject(), "the self argument"));
  ApiHandle* args_handle = ApiHandle::fromPyObject(args);
  EXPECT_GT(args_handle->refcnt(), 0);
  Object args_obj(&scope, args_handle->asObject());
  EXPECT_TRUE(args_obj.isTuple());
  Tuple args_tuple(&scope, *args_obj);
  EXPECT_EQ(args_tuple.length(), 2);
  EXPECT_TRUE(isIntEqualsWord(args_tuple.at(0), 17));
  EXPECT_TRUE(isIntEqualsWord(args_tuple.at(1), -8));
  EXPECT_EQ(kwargs, nullptr);
  return ApiHandle::newReference(runtime, SmallInt::fromWord(1237));
}

static RawObject newFunctionKeywordsNullKwargs(Thread* thread) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object name(&scope, runtime->newStrFromCStr("foo"));
  PyCFunctionWithKeywords function_ptr = capiFunctionKeywordsNullKwargs;
  return newExtensionFunction(thread, name,
                              reinterpret_cast<void*>(function_ptr),
                              METH_VARARGS | METH_KEYWORDS);
}

TEST_F(MethodTrampolinesTest, Keywords) {
  HandleScope scope(thread_);
  Object function(&scope, newFunctionKeywordsNullKwargs(thread_));
  Object arg0(&scope, runtime_->newStrFromCStr("the self argument"));
  Object arg1(&scope, runtime_->newInt(17));
  Object arg2(&scope, runtime_->newInt(-8));
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::call3(thread_, function, arg0, arg1, arg2), 1237));
}

TEST_F(MethodTrampolinesTest, KeywordsWithoutSelfRaisesTypeError) {
  HandleScope scope(thread_);
  Object function(&scope, newFunctionKeywordsNullKwargs(thread_));
  EXPECT_TRUE(raisedWithStr(Interpreter::call0(thread_, function),
                            LayoutId::kTypeError,
                            "'foo' must be bound to an object"));
}

static PyObject* capiFunctionKeywords(PyObject* self, PyObject* args,
                                      PyObject* kwargs) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  runtime->collectGarbage();
  ApiHandle* self_handle = ApiHandle::fromPyObject(self);
  EXPECT_GT(self_handle->refcnt(), 0);
  EXPECT_TRUE(isStrEqualsCStr(self_handle->asObject(), "the self argument"));
  ApiHandle* args_handle = ApiHandle::fromPyObject(args);
  EXPECT_GT(args_handle->refcnt(), 0);
  Object args_obj(&scope, args_handle->asObject());
  EXPECT_TRUE(args_obj.isTuple());
  Tuple args_tuple(&scope, *args_obj);
  EXPECT_EQ(args_tuple.length(), 2);
  EXPECT_TRUE(isIntEqualsWord(args_tuple.at(0), 17));
  EXPECT_TRUE(isIntEqualsWord(args_tuple.at(1), -8));

  ApiHandle* kwargs_handle = ApiHandle::fromPyObject(kwargs);
  EXPECT_GT(kwargs_handle->refcnt(), 0);
  Object kwargs_obj(&scope, kwargs_handle->asObject());
  EXPECT_TRUE(kwargs_obj.isDict());
  Dict kwargs_dict(&scope, *kwargs_obj);
  EXPECT_EQ(kwargs_dict.numItems(), 1);
  EXPECT_TRUE(
      isStrEqualsCStr(dictAtById(thread, kwargs_dict, ID(key)), "value"));
  return ApiHandle::newReference(runtime, SmallInt::fromWord(1237));
}

static RawObject newFunctionKeywords(Thread* thread) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object name(&scope, runtime->newStrFromCStr("foo"));
  PyCFunctionWithKeywords function_ptr = capiFunctionKeywords;
  return newExtensionFunction(thread, name,
                              reinterpret_cast<void*>(function_ptr),
                              METH_VARARGS | METH_KEYWORDS);
}

TEST_F(MethodTrampolinesTest, KeywordsKw) {
  HandleScope scope(thread_);
  Object key(&scope, runtime_->newStrFromCStr("key"));
  Tuple kw_names(&scope, runtime_->newTupleWith1(key));
  thread_->stackPush(newFunctionKeywords(thread_));
  thread_->stackPush(runtime_->newStrFromCStr("the self argument"));
  thread_->stackPush(runtime_->newInt(17));
  thread_->stackPush(runtime_->newInt(-8));
  thread_->stackPush(runtime_->newStrFromCStr("value"));
  thread_->stackPush(*kw_names);
  EXPECT_TRUE(isIntEqualsWord(Interpreter::callKw(thread_, 4), 1237));
}

TEST_F(MethodTrampolinesTest, KeywordsKwWithoutSelfRaisesTypeError) {
  HandleScope scope(thread_);
  Object key(&scope, runtime_->newStrFromCStr("key"));
  Tuple kw_names(&scope, runtime_->newTupleWith1(key));
  thread_->stackPush(newFunctionKeywords(thread_));
  thread_->stackPush(runtime_->newStrFromCStr("value"));
  thread_->stackPush(*kw_names);
  EXPECT_TRUE(raisedWithStr(Interpreter::callKw(thread_, 1),
                            LayoutId::kTypeError,
                            "'foo' must be bound to an object"));
}

TEST_F(MethodTrampolinesTest, KeywordsExWithoutKwargs) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_->newStrFromCStr("the self argument"));
  Object num1(&scope, runtime_->newInt(17));
  Object num2(&scope, runtime_->newInt(-8));
  Tuple args(&scope, runtime_->newTupleWith3(self, num1, num2));
  thread_->stackPush(newFunctionKeywordsNullKwargs(thread_));
  thread_->stackPush(*args);
  EXPECT_TRUE(isIntEqualsWord(Interpreter::callEx(thread_, 0), 1237));
}

TEST_F(MethodTrampolinesTest, KeywordsExWithKwargs) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_->newStrFromCStr("the self argument"));
  Object num1(&scope, runtime_->newInt(17));
  Object num2(&scope, runtime_->newInt(-8));
  Tuple args(&scope, runtime_->newTupleWith3(self, num1, num2));
  Dict kwargs(&scope, runtime_->newDict());
  Object value(&scope, runtime_->newStrFromCStr("value"));
  dictAtPutById(thread_, kwargs, ID(key), value);
  thread_->stackPush(newFunctionKeywords(thread_));
  thread_->stackPush(*args);
  thread_->stackPush(*kwargs);
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::callEx(thread_, CallFunctionExFlag::VAR_KEYWORDS), 1237));
}

TEST_F(MethodTrampolinesTest, KeywordsExWithoutSelfRaisesTypeError) {
  thread_->stackPush(newFunctionKeywordsNullKwargs(thread_));
  thread_->stackPush(runtime_->emptyTuple());
  EXPECT_TRUE(raisedWithStr(Interpreter::callEx(thread_, 0),
                            LayoutId::kTypeError,
                            "'foo' must be bound to an object"));
}

static PyObject* capiFunctionFast(PyObject* self, PyObject* const* args,
                                  Py_ssize_t nargs) {
  Runtime* runtime = Thread::current()->runtime();
  runtime->collectGarbage();
  ApiHandle* self_handle = ApiHandle::fromPyObject(self);
  EXPECT_GT(self_handle->refcnt(), 0);
  EXPECT_TRUE(isStrEqualsCStr(ApiHandle::fromPyObject(self)->asObject(),
                              "the self argument"));
  EXPECT_EQ(nargs, 2);
  ApiHandle* arg0_handle = ApiHandle::fromPyObject(args[0]);
  EXPECT_GT(arg0_handle->refcnt(), 0);
  ApiHandle* arg1_handle = ApiHandle::fromPyObject(args[1]);
  EXPECT_GT(arg1_handle->refcnt(), 0);
  EXPECT_TRUE(isFloatEqualsDouble(arg0_handle->asObject(), -13.));
  EXPECT_TRUE(isFloatEqualsDouble(arg1_handle->asObject(), 0.125));
  return ApiHandle::newReference(runtime, SmallInt::fromWord(1236));
}

static RawObject newExtensionFunctionFast(Thread* thread) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object name(&scope, runtime->newStrFromCStr("foo"));
  _PyCFunctionFast function_ptr = capiFunctionFast;
  return newExtensionFunction(
      thread, name, reinterpret_cast<void*>(function_ptr), METH_FASTCALL);
}

TEST_F(MethodTrampolinesTest, Fast) {
  HandleScope scope(thread_);
  Object function(&scope, newExtensionFunctionFast(thread_));
  Object arg0(&scope, runtime_->newStrFromCStr("the self argument"));
  Object arg1(&scope, runtime_->newFloat(-13.));
  Object arg2(&scope, runtime_->newFloat(0.125));
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::call3(thread_, function, arg0, arg1, arg2), 1236));
}

TEST_F(MethodTrampolinesTest, FastWithoutSelfRaisesTypeError) {
  HandleScope scope(thread_);
  Object function(&scope, newExtensionFunctionFast(thread_));
  EXPECT_TRUE(raisedWithStr(Interpreter::call0(thread_, function),
                            LayoutId::kTypeError,
                            "'foo' must be bound to an object"));
}

TEST_F(MethodTrampolinesTest, FastKw) {
  thread_->stackPush(newExtensionFunctionFast(thread_));
  thread_->stackPush(runtime_->newStrFromCStr("the self argument"));
  thread_->stackPush(runtime_->newFloat(-13.));
  thread_->stackPush(runtime_->newFloat(0.125));
  thread_->stackPush(runtime_->emptyTuple());
  EXPECT_TRUE(isIntEqualsWord(Interpreter::callKw(thread_, 3), 1236));
}

TEST_F(MethodTrampolinesTest, FastKwWithoutSelfRaisesTypeError) {
  thread_->stackPush(newExtensionFunctionFast(thread_));
  thread_->stackPush(runtime_->emptyTuple());
  EXPECT_TRUE(raisedWithStr(Interpreter::callKw(thread_, 0),
                            LayoutId::kTypeError,
                            "'foo' must be bound to an object"));
}

TEST_F(MethodTrampolinesTest, FastKwWithKeywordRaisesTypeError) {
  HandleScope scope(thread_);
  Object key(&scope, runtime_->newStrFromCStr("key"));
  Tuple kw_names(&scope, runtime_->newTupleWith1(key));
  thread_->stackPush(newExtensionFunctionFast(thread_));
  thread_->stackPush(runtime_->newStrFromCStr("the self argument"));
  thread_->stackPush(runtime_->newFloat(-13.));
  thread_->stackPush(runtime_->newFloat(0.125));
  thread_->stackPush(runtime_->newStrFromCStr("value"));
  thread_->stackPush(*kw_names);
  EXPECT_TRUE(raisedWithStr(Interpreter::callKw(thread_, 4),
                            LayoutId::kTypeError,
                            "'foo' takes no keyword arguments"));
}

TEST_F(MethodTrampolinesTest, FastExWithoutKwargs) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_->newStrFromCStr("the self argument"));
  Object num1(&scope, runtime_->newFloat(-13.));
  Object num2(&scope, runtime_->newFloat(0.125));
  Tuple args(&scope, runtime_->newTupleWith3(self, num1, num2));
  thread_->stackPush(newExtensionFunctionFast(thread_));
  thread_->stackPush(*args);
  EXPECT_TRUE(isIntEqualsWord(Interpreter::callEx(thread_, 0), 1236));
}

TEST_F(MethodTrampolinesTest, FastExWithKwargs) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_->newStrFromCStr("the self argument"));
  Object num1(&scope, runtime_->newFloat(-13.));
  Object num2(&scope, runtime_->newFloat(0.125));
  Tuple args(&scope, runtime_->newTupleWith3(self, num1, num2));
  thread_->stackPush(newExtensionFunctionFast(thread_));
  thread_->stackPush(*args);
  thread_->stackPush(runtime_->newDict());
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::callEx(thread_, CallFunctionExFlag::VAR_KEYWORDS), 1236));
}

TEST_F(MethodTrampolinesTest, FastExWithoutSelfRaisesTypeError) {
  thread_->stackPush(newExtensionFunctionFast(thread_));
  thread_->stackPush(runtime_->emptyTuple());
  EXPECT_TRUE(raisedWithStr(Interpreter::callEx(thread_, 0),
                            LayoutId::kTypeError,
                            "'foo' must be bound to an object"));
}

TEST_F(MethodTrampolinesTest, FastExWithKeywordArgRaisesTypeError) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_->newStrFromCStr("the self argument"));
  Object num1(&scope, runtime_->newFloat(-13.));
  Object num2(&scope, runtime_->newFloat(0.125));
  Tuple args(&scope, runtime_->newTupleWith3(self, num1, num2));
  Dict kwargs(&scope, runtime_->newDict());
  Object value(&scope, runtime_->newStrFromCStr("value"));
  dictAtPutById(thread_, kwargs, ID(key), value);
  thread_->stackPush(newExtensionFunctionFast(thread_));
  thread_->stackPush(*args);
  thread_->stackPush(*kwargs);
  EXPECT_TRUE(raisedWithStr(
      Interpreter::callEx(thread_, CallFunctionExFlag::VAR_KEYWORDS),
      LayoutId::kTypeError, "'foo' takes no keyword arguments"));
}

static PyObject* capiFunctionFastWithKeywordsNullKwnames(PyObject* self,
                                                         PyObject* const* args,
                                                         Py_ssize_t nargs,
                                                         PyObject* kwnames) {
  Runtime* runtime = Thread::current()->runtime();
  runtime->collectGarbage();
  ApiHandle* self_handle = ApiHandle::fromPyObject(self);
  EXPECT_GT(self_handle->refcnt(), 0);
  EXPECT_TRUE(isStrEqualsCStr(ApiHandle::fromPyObject(self)->asObject(),
                              "the self argument"));
  EXPECT_EQ(nargs, 2);
  ApiHandle* arg0_handle = ApiHandle::fromPyObject(args[0]);
  EXPECT_GT(arg0_handle->refcnt(), 0);
  ApiHandle* arg1_handle = ApiHandle::fromPyObject(args[1]);
  EXPECT_GT(arg1_handle->refcnt(), 0);
  EXPECT_TRUE(isFloatEqualsDouble(arg0_handle->asObject(), 42.5));
  EXPECT_TRUE(isFloatEqualsDouble(arg1_handle->asObject(), -8.8));
  EXPECT_EQ(kwnames, nullptr);
  return ApiHandle::newReference(runtime, SmallInt::fromWord(1238));
}

static RawObject newExtensionFunctionFastWithKeywordsNullKwnames(
    Thread* thread) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object name(&scope, runtime->newStrFromCStr("foo"));
  _PyCFunctionFastWithKeywords function_ptr =
      capiFunctionFastWithKeywordsNullKwnames;
  return newExtensionFunction(thread, name,
                              reinterpret_cast<void*>(function_ptr),
                              METH_FASTCALL | METH_KEYWORDS);
}

TEST_F(MethodTrampolinesTest, FastWithKeywords) {
  HandleScope scope(thread_);
  Object function(&scope,
                  newExtensionFunctionFastWithKeywordsNullKwnames(thread_));
  Object arg0(&scope, runtime_->newStrFromCStr("the self argument"));
  Object arg1(&scope, runtime_->newFloat(42.5));
  Object arg2(&scope, runtime_->newFloat(-8.8));
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::call3(thread_, function, arg0, arg1, arg2), 1238));
}

TEST_F(MethodTrampolinesTest, FastWithKeywordsWithoutSelfRaisesTypeError) {
  HandleScope scope(thread_);
  Object function(&scope,
                  newExtensionFunctionFastWithKeywordsNullKwnames(thread_));
  EXPECT_TRUE(raisedWithStr(Interpreter::call0(thread_, function),
                            LayoutId::kTypeError,
                            "'foo' must be bound to an object"));
}

static PyObject* capiFunctionFastWithKeywords(PyObject* self,
                                              PyObject* const* args,
                                              Py_ssize_t nargs,
                                              PyObject* kwnames) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  runtime->collectGarbage();
  ApiHandle* self_handle = ApiHandle::fromPyObject(self);
  EXPECT_GT(self_handle->refcnt(), 0);
  EXPECT_TRUE(isStrEqualsCStr(ApiHandle::fromPyObject(self)->asObject(),
                              "the self argument"));
  EXPECT_EQ(nargs, 2);
  ApiHandle* arg0_handle = ApiHandle::fromPyObject(args[0]);
  EXPECT_GT(arg0_handle->refcnt(), 0);
  ApiHandle* arg1_handle = ApiHandle::fromPyObject(args[1]);
  EXPECT_GT(arg1_handle->refcnt(), 0);
  EXPECT_TRUE(isFloatEqualsDouble(arg0_handle->asObject(), 42.5));
  EXPECT_TRUE(isFloatEqualsDouble(arg1_handle->asObject(), -8.8));

  ApiHandle* kwnames_handle = ApiHandle::fromPyObject(kwnames);
  EXPECT_GT(kwnames_handle->refcnt(), 0);
  Object kwnames_obj(&scope, kwnames_handle->asObject());
  EXPECT_TRUE(kwnames_obj.isTuple());
  Tuple kwnames_tuple(&scope, *kwnames_obj);
  EXPECT_EQ(kwnames_tuple.length(), 2);
  EXPECT_TRUE(isStrEqualsCStr(kwnames_tuple.at(0), "foo"));
  EXPECT_TRUE(isStrEqualsCStr(kwnames_tuple.at(1), "bar"));
  EXPECT_TRUE(isStrEqualsCStr(ApiHandle::fromPyObject(args[2])->asObject(),
                              "foo_value"));
  EXPECT_TRUE(isStrEqualsCStr(ApiHandle::fromPyObject(args[3])->asObject(),
                              "bar_value"));
  return ApiHandle::newReference(runtime, SmallInt::fromWord(1238));
}

static RawObject newExtensionFunctionFastWithKeywords(Thread* thread) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object name(&scope, runtime->newStrFromCStr("foo"));
  _PyCFunctionFastWithKeywords function_ptr = capiFunctionFastWithKeywords;
  return newExtensionFunction(thread, name,
                              reinterpret_cast<void*>(function_ptr),
                              METH_FASTCALL | METH_KEYWORDS);
}

TEST_F(MethodTrampolinesTest, FastWithKeywordsKw) {
  HandleScope scope(thread_);
  Object foo(&scope, runtime_->newStrFromCStr("foo"));
  Object bar(&scope, runtime_->newStrFromCStr("bar"));
  Tuple kw_names(&scope, runtime_->newTupleWith2(foo, bar));
  thread_->stackPush(newExtensionFunctionFastWithKeywords(thread_));
  thread_->stackPush(runtime_->newStrFromCStr("the self argument"));
  thread_->stackPush(runtime_->newFloat(42.5));
  thread_->stackPush(runtime_->newFloat(-8.8));
  thread_->stackPush(runtime_->newStrFromCStr("foo_value"));
  thread_->stackPush(runtime_->newStrFromCStr("bar_value"));
  thread_->stackPush(*kw_names);
  EXPECT_TRUE(isIntEqualsWord(Interpreter::callKw(thread_, 5), 1238));
}

TEST_F(MethodTrampolinesTest, FastWithKeywordsKwWihoutSelfRaisesTypeError) {
  thread_->stackPush(newExtensionFunctionFastWithKeywords(thread_));
  thread_->stackPush(runtime_->emptyTuple());
  EXPECT_TRUE(raisedWithStr(Interpreter::callKw(thread_, 0),
                            LayoutId::kTypeError,
                            "'foo' must be bound to an object"));
}

TEST_F(MethodTrampolinesTest, FastWithKeywordsExWithoutKwargs) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_->newStrFromCStr("the self argument"));
  Object num1(&scope, runtime_->newFloat(42.5));
  Object num2(&scope, runtime_->newFloat(-8.8));
  Tuple args(&scope, runtime_->newTupleWith3(self, num1, num2));
  thread_->stackPush(newExtensionFunctionFastWithKeywordsNullKwnames(thread_));
  thread_->stackPush(*args);
  EXPECT_TRUE(isIntEqualsWord(Interpreter::callEx(thread_, 0), 1238));
}

TEST_F(MethodTrampolinesTest, FastWithKeywordsExWithKwargs) {
  HandleScope scope(thread_);
  Object self(&scope, runtime_->newStrFromCStr("the self argument"));
  Object num1(&scope, runtime_->newFloat(42.5));
  Object num2(&scope, runtime_->newFloat(-8.8));
  Tuple args(&scope, runtime_->newTupleWith3(self, num1, num2));
  Dict kwargs(&scope, runtime_->newDict());
  Object foo(&scope, runtime_->newStrFromCStr("foo"));
  Object foo_value(&scope, runtime_->newStrFromCStr("foo_value"));
  dictAtPutByStr(thread_, kwargs, foo, foo_value);
  Object bar(&scope, runtime_->newStrFromCStr("bar"));
  Object bar_value(&scope, runtime_->newStrFromCStr("bar_value"));
  dictAtPutByStr(thread_, kwargs, bar, bar_value);
  thread_->stackPush(newExtensionFunctionFastWithKeywords(thread_));
  thread_->stackPush(*args);
  thread_->stackPush(*kwargs);
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::callEx(thread_, CallFunctionExFlag::VAR_KEYWORDS), 1238));
}

TEST_F(MethodTrampolinesTest, FastWithKeywordsExWithoutSelfRaisesTypeError) {
  thread_->stackPush(newExtensionFunctionFastWithKeywords(thread_));
  thread_->stackPush(runtime_->emptyTuple());
  EXPECT_TRUE(raisedWithStr(Interpreter::callEx(thread_, 0),
                            LayoutId::kTypeError,
                            "'foo' must be bound to an object"));
}

}  // namespace testing
}  // namespace py
