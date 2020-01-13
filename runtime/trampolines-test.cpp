#include <memory>

#include "gtest/gtest.h"

#include "capi-handles.h"
#include "dict-builtins.h"
#include "frame.h"
#include "function-builtins.h"
#include "int-builtins.h"
#include "runtime.h"
#include "test-utils.h"

namespace py {
using namespace testing;

using CallTest = RuntimeFixture;
using TrampolinesTest = RuntimeFixture;

TEST_F(CallTest, CallBoundMethod) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def func(self):
  return self

def test(callable):
  return callable()
)")
                   .isError());

  Object function(&scope, mainModuleAt(runtime_, "func"));
  ASSERT_TRUE(function.isFunction());

  Object self(&scope, SmallInt::fromWord(1111));
  BoundMethod method(&scope, runtime_->newBoundMethod(function, self));

  Object test(&scope, mainModuleAt(runtime_, "test"));
  ASSERT_TRUE(test.isFunction());
  Function func(&scope, *test);

  Object result(&scope, Interpreter::callFunction1(
                            thread_, thread_->currentFrame(), func, method));
  EXPECT_TRUE(isIntEqualsWord(*result, 1111));
}

TEST_F(CallTest, CallBoundMethodWithArgs) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def func(self, a, b):
  return [self, a, b]

def test(callable):
  return callable(2222, 3333)
)")
                   .isError());

  Object function(&scope, mainModuleAt(runtime_, "func"));
  ASSERT_TRUE(function.isFunction());

  Object self(&scope, SmallInt::fromWord(1111));
  BoundMethod method(&scope, runtime_->newBoundMethod(function, self));

  Object test(&scope, mainModuleAt(runtime_, "test"));
  ASSERT_TRUE(test.isFunction());
  Function func(&scope, *test);

  Object result(&scope, Interpreter::callFunction1(
                            thread_, thread_->currentFrame(), func, method));
  EXPECT_PYLIST_EQ(result, {1111, 2222, 3333});
}

TEST_F(CallTest, CallBoundMethodKw) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result_self = None
result_a = None
result_b = None

def func(self, a, b):
  global result_self, result_a, result_b
  result_self = self
  result_a = a
  result_b = b

def test(callable):
  return callable(a=2222, b=3333)
)")
                   .isError());

  Object function(&scope, mainModuleAt(runtime_, "func"));
  ASSERT_TRUE(function.isFunction());

  Object self(&scope, SmallInt::fromWord(1111));
  BoundMethod method(&scope, runtime_->newBoundMethod(function, self));

  Object test(&scope, mainModuleAt(runtime_, "test"));
  ASSERT_TRUE(test.isFunction());
  Function func(&scope, *test);
  Tuple args(&scope, runtime_->newTuple(1));
  args.atPut(0, *method);
  callFunction(func, args);

  Object result_self(&scope, mainModuleAt(runtime_, "result_self"));
  EXPECT_TRUE(isIntEqualsWord(*result_self, 1111));

  Object result_a(&scope, mainModuleAt(runtime_, "result_a"));
  EXPECT_TRUE(isIntEqualsWord(*result_a, 2222));

  Object result_b(&scope, mainModuleAt(runtime_, "result_b"));
  EXPECT_TRUE(isIntEqualsWord(*result_b, 3333));
}

TEST_F(CallTest, CallBoundMethodExArgs) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result_self = None
result_a = None
result_b = None

def func(self, a, b):
  global result_self, result_a, result_b
  result_self = self
  result_a = a
  result_b = b

def test(callable):
  args = (2222, 3333)
  return callable(*args)
)")
                   .isError());

  Object function(&scope, mainModuleAt(runtime_, "func"));
  ASSERT_TRUE(function.isFunction());

  Object self(&scope, SmallInt::fromWord(1111));
  BoundMethod method(&scope, runtime_->newBoundMethod(function, self));

  Object test(&scope, mainModuleAt(runtime_, "test"));
  ASSERT_TRUE(test.isFunction());
  Function func(&scope, *test);
  Tuple args(&scope, runtime_->newTuple(1));
  args.atPut(0, *method);
  callFunction(func, args);

  Object result_self(&scope, mainModuleAt(runtime_, "result_self"));
  EXPECT_TRUE(isIntEqualsWord(*result_self, 1111));

  Object result_a(&scope, mainModuleAt(runtime_, "result_a"));
  EXPECT_TRUE(isIntEqualsWord(*result_a, 2222));

  Object result_b(&scope, mainModuleAt(runtime_, "result_b"));
  EXPECT_TRUE(isIntEqualsWord(*result_b, 3333));
}

TEST_F(CallTest, CallBoundMethodExKwargs) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result_self = None
result_a = None
result_b = None

def func(self, a, b):
  global result_self, result_a, result_b
  result_self = self
  result_a = a
  result_b = b

def test(callable):
  kwargs = {'a': 2222, 'b': 3333}
  return callable(**kwargs)
)")
                   .isError());

  Object function(&scope, mainModuleAt(runtime_, "func"));
  ASSERT_TRUE(function.isFunction());

  Object self(&scope, SmallInt::fromWord(1111));
  BoundMethod method(&scope, runtime_->newBoundMethod(function, self));

  Object test(&scope, mainModuleAt(runtime_, "test"));
  ASSERT_TRUE(test.isFunction());
  Function func(&scope, *test);
  Tuple args(&scope, runtime_->newTuple(1));
  args.atPut(0, *method);
  callFunction(func, args);

  Object result_self(&scope, mainModuleAt(runtime_, "result_self"));
  EXPECT_TRUE(isIntEqualsWord(*result_self, 1111));

  Object result_a(&scope, mainModuleAt(runtime_, "result_a"));
  EXPECT_TRUE(isIntEqualsWord(*result_a, 2222));

  Object result_b(&scope, mainModuleAt(runtime_, "result_b"));
  EXPECT_TRUE(isIntEqualsWord(*result_b, 3333));
}

TEST_F(CallTest, CallBoundMethodExArgsAndKwargs) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result_self = None
result_a = None
result_b = None

def func(self, a, b):
  global result_self, result_a, result_b
  result_self = self
  result_a = a
  result_b = b

def test(callable):
  args = (2222,)
  kwargs = {'b': 3333}
  return callable(*args, **kwargs)
)")
                   .isError());

  Object function(&scope, mainModuleAt(runtime_, "func"));
  ASSERT_TRUE(function.isFunction());

  Object self(&scope, SmallInt::fromWord(1111));
  BoundMethod method(&scope, runtime_->newBoundMethod(function, self));

  Object test(&scope, mainModuleAt(runtime_, "test"));
  ASSERT_TRUE(test.isFunction());
  Function func(&scope, *test);
  Tuple args(&scope, runtime_->newTuple(1));
  args.atPut(0, *method);
  callFunction(func, args);

  Object result_self(&scope, mainModuleAt(runtime_, "result_self"));
  EXPECT_TRUE(isIntEqualsWord(*result_self, 1111));

  Object result_a(&scope, mainModuleAt(runtime_, "result_a"));
  EXPECT_TRUE(isIntEqualsWord(*result_a, 2222));

  Object result_b(&scope, mainModuleAt(runtime_, "result_b"));
  EXPECT_TRUE(isIntEqualsWord(*result_b, 3333));
}

TEST_F(CallTest, CallDefaultArgs) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(a=1, b=2, c=3):
  return [a, b, c]

result0 = foo(33, 22, 11)
result1 = foo()
result2 = foo(1001)
result3 = foo(1001, 1002)
result4 = foo(1001, 1002, 1003)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result0(&scope, mainModuleAt(runtime_, "result0"));
  EXPECT_PYLIST_EQ(result0, {33, 22, 11});
  Object result1(&scope, mainModuleAt(runtime_, "result1"));
  EXPECT_PYLIST_EQ(result1, {1, 2, 3});
  Object result2(&scope, mainModuleAt(runtime_, "result2"));
  EXPECT_PYLIST_EQ(result2, {1001, 2, 3});
  Object result3(&scope, mainModuleAt(runtime_, "result3"));
  EXPECT_PYLIST_EQ(result3, {1001, 1002, 3});
  Object result4(&scope, mainModuleAt(runtime_, "result4"));
  EXPECT_PYLIST_EQ(result4, {1001, 1002, 1003});
}

TEST_F(CallTest, CallMethodMixPosDefaultArgs) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(a, b=2):
  return [a, b]
result = foo(1)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {1, 2});
}

TEST_F(CallTest, CallBoundMethodMixed) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class R:
  def m(self, a, b=2):
    return [a, b]
r = R()
result = r.m(9)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {9, 2});
}

TEST_F(CallTest, SingleKW) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(bar):
   return bar
result = foo(bar=2)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result"), 2));
}

TEST_F(CallTest, MixedKW) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(a, b, c):
   return [a, b, c]
result = foo(1, b = 2, c = 3)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {1, 2, 3});
}

TEST_F(CallTest, FullKW) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(a, b, c):
   return [a, b, c]
result = foo(a = 1, b = 2, c = 3)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {1, 2, 3});
}

TEST_F(CallTest, KWOutOfOrder1) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(a, b, c):
   return [a, b, c]
result = foo(c = 3, a = 1, b = 2)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {1, 2, 3});
}

TEST_F(CallTest, KWOutOfOrder2) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(a, b, c):
   return [a, b, c]
result = foo(1, c = 3, b = 2)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {1, 2, 3});
}

TEST_F(CallTest, KeywordOnly1) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(a,b, *, c):
  return [a,b,c]
result = foo(1, 2, c = 3)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {1, 2, 3});
}

TEST_F(CallTest, KeywordOnly2) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(a,b, *, c):
  return [a,b,c]
result = foo(1, b = 2, c = 3)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {1, 2, 3});
}

TEST_F(CallTest, KeyWordDefaults) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(a, b = 22, c = 33):
  return [a,b,c]
result = foo(11, c = 3)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {11, 22, 3});
}

TEST_F(CallTest, VarArgsWithExcess) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(a, b, *c):
  return [a,b,c]
result = foo(1,2,3,4,5,6)
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 2));
  Tuple tuple(&scope, result.at(2));
  ASSERT_EQ(tuple.length(), 4);
  EXPECT_TRUE(isIntEqualsWord(tuple.at(0), 3));
  EXPECT_TRUE(isIntEqualsWord(tuple.at(1), 4));
  EXPECT_TRUE(isIntEqualsWord(tuple.at(2), 5));
  EXPECT_TRUE(isIntEqualsWord(tuple.at(3), 6));
}

TEST_F(CallTest, VarArgsEmpty) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(a, b, *c):
  return [a,b,c]
result = foo(1,2)
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 2));
  Tuple tuple(&scope, result.at(2));
  EXPECT_EQ(tuple.length(), 0);
}

TEST_F(CallTest, CallWithKeywordsCalleeWithVarkeyword) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(a,b,c,**d):
    return [a,b,c,d]
result = foo(1,2,c=3,g=4,h=5,j="bar")
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, testing::mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 2));
  EXPECT_TRUE(isIntEqualsWord(result.at(2), 3));

  Dict dict(&scope, result.at(3));
  Str name_g(&scope, runtime_->newStrFromCStr("g"));
  EXPECT_TRUE(isIntEqualsWord(dictAtByStr(thread_, dict, name_g), 4));
  Str name_h(&scope, runtime_->newStrFromCStr("h"));
  EXPECT_TRUE(isIntEqualsWord(dictAtByStr(thread_, dict, name_h), 5));
  Str name_j(&scope, runtime_->newStrFromCStr("j"));
  EXPECT_TRUE(isStrEqualsCStr(dictAtByStr(thread_, dict, name_j), "bar"));
}

TEST_F(CallTest, CallWithNoArgsCalleeDefaultArgsVarargsVarkeyargs) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def bar(a=1, b=2, *c, **d):
    return [a,b,c,d]
result = bar()
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, testing::mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 2));
  Tuple tuple(&scope, result.at(2));
  EXPECT_EQ(tuple.length(), 0);
  Dict dict(&scope, result.at(3));
  EXPECT_EQ(dict.numItems(), 0);
}

TEST_F(CallTest, CallPositionalCalleeVargsEmptyVarkeyargs) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def bar(a=1, b=2, *c, **d):
    return [a,b,c,d]
result = bar(1,2,3,4,5,6,7)
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, testing::mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 2));
  Tuple tuple(&scope, result.at(2));
  ASSERT_EQ(tuple.length(), 5);
  EXPECT_TRUE(isIntEqualsWord(tuple.at(0), 3));
  EXPECT_TRUE(isIntEqualsWord(tuple.at(1), 4));
  EXPECT_TRUE(isIntEqualsWord(tuple.at(2), 5));
  EXPECT_TRUE(isIntEqualsWord(tuple.at(3), 6));
  EXPECT_TRUE(isIntEqualsWord(tuple.at(4), 7));
  Dict dict(&scope, result.at(3));
  EXPECT_EQ(dict.numItems(), 0);
}

TEST_F(CallTest, CallWithKeywordsCalleeEmptyVarargsFullVarkeyargs) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def bar(a=1, b=2, *c, **d):
    return [a,b,c,d]
result = bar(a1=11, a2=12, a3=13)
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, testing::mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 2));

  Object tuple_obj(&scope, result.at(2));
  ASSERT_TRUE(tuple_obj.isTuple());
  Tuple tuple(&scope, *tuple_obj);
  EXPECT_EQ(tuple.length(), 0);

  Dict dict(&scope, result.at(3));
  Str name0(&scope, runtime_->newStrFromCStr("a3"));
  EXPECT_TRUE(isIntEqualsWord(dictAtByStr(thread_, dict, name0), 13));
  Str name1(&scope, runtime_->newStrFromCStr("a1"));
  EXPECT_TRUE(isIntEqualsWord(dictAtByStr(thread_, dict, name1), 11));
  Str name2(&scope, runtime_->newStrFromCStr("a2"));
  EXPECT_TRUE(isIntEqualsWord(dictAtByStr(thread_, dict, name2), 12));
}

TEST_F(CallTest, CallWithKeywordsCalleeFullVarargsFullVarkeyargs) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def bar(a=1, b=2, *c, **d):
    return [a,b,c,d]
result = bar(1,2,3,4,5,6,7,a9=9)
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, testing::mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 2));

  Tuple tuple(&scope, result.at(2));
  EXPECT_TRUE(isIntEqualsWord(tuple.at(0), 3));
  EXPECT_TRUE(isIntEqualsWord(tuple.at(1), 4));
  EXPECT_TRUE(isIntEqualsWord(tuple.at(2), 5));
  EXPECT_TRUE(isIntEqualsWord(tuple.at(3), 6));
  EXPECT_TRUE(isIntEqualsWord(tuple.at(4), 7));

  Dict dict(&scope, result.at(3));
  Str name_g(&scope, runtime_->newStrFromCStr("a9"));
  EXPECT_TRUE(isIntEqualsWord(dictAtByStr(thread_, dict, name_g), 9));
}

TEST_F(CallTest, CallWithOutOfOrderKeywords) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foobar(a,b,*,c):
    return [a,b,c]
result = foobar(c=3,a=1,b=2)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {1, 2, 3});
}

TEST_F(CallTest, CallWithKeywordsCalleeVarargsKeywordOnly) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foobar1(a,b,*c,d):
    return [a,b,c,d]
result = foobar1(1,2,3,4,5,d=9)
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 2));
  Tuple tuple(&scope, result.at(2));
  ASSERT_EQ(tuple.length(), 3);
  EXPECT_TRUE(isIntEqualsWord(tuple.at(0), 3));
  EXPECT_TRUE(isIntEqualsWord(tuple.at(1), 4));
  EXPECT_TRUE(isIntEqualsWord(tuple.at(2), 5));
  EXPECT_TRUE(isIntEqualsWord(result.at(3), 9));
}

TEST_F(CallTest, CallWithKeywordsCalleeVarargsVarkeyargsKeywordOnly) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foobar2(a,b,*c, e, **d):
    return [a,b,c,d,e]
result = foobar2(1,e=9,b=2,f1="a",f11=12)
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 2));
  Tuple tuple(&scope, result.at(2));
  ASSERT_EQ(tuple.length(), 0);
  Dict dict(&scope, result.at(3));
  Str f1(&scope, runtime_->newStrFromCStr("f1"));
  EXPECT_TRUE(isStrEqualsCStr(dictAtByStr(thread_, dict, f1), "a"));
  Str f11(&scope, runtime_->newStrFromCStr("f11"));
  EXPECT_TRUE(isIntEqualsWord(dictAtByStr(thread_, dict, f11), 12));
  EXPECT_TRUE(isIntEqualsWord(result.at(4), 9));
}

TEST_F(CallTest, CallEx) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(a,b,c,d):
    return [a,b,c,d]
a = (1,2,3,4)
result = foo(*a)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {1, 2, 3, 4});
}

TEST_F(CallTest, CallExBuildTupleUnpackWithCall) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(a,b,c,d):
    return [a,b,c,d]
a = (3,4)
result = foo(1,2,*a)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {1, 2, 3, 4});
}

TEST_F(CallTest, CallExKw) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(a,b,c,d):
    return [a,b,c,d]
a = {'d': 4, 'b': 2, 'a': 1, 'c': 3}
result = foo(**a)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {1, 2, 3, 4});
}

TEST_F(CallTest, KeywordOnly) {
  const char* src = R"(
def foo(a,b, *, c):
  print(a,b,c)
foo(1, 2, 3);
)";
  EXPECT_TRUE(raised(runFromCStr(runtime_, src), LayoutId::kTypeError));
}

TEST_F(CallTest, MissingKeyword) {
  const char* src = R"(
def foo(a,b, *, c):
  print(a,b,c)
foo(1, 2);
)";
  EXPECT_TRUE(raised(runFromCStr(runtime_, src), LayoutId::kTypeError));
}

TEST_F(CallTest, ArgNameMismatch) {
  const char* src = R"(
def foo(a,b, *, c):
  print(a,b,c)
foo(1, d = 2, c = 3);
)";
  EXPECT_TRUE(raised(runFromCStr(runtime_, src), LayoutId::kTypeError));
}

TEST_F(CallTest, TooManyKWArgs) {
  const char* src = R"(
def foo(a,b, *, c):
  print(a,b,c)
foo(1, 2, 4, c = 3);
)";
  EXPECT_TRUE(raised(runFromCStr(runtime_, src), LayoutId::kTypeError));
}

TEST_F(CallTest, TooManyArgs) {
  const char* src = R"(
def foo(a,b, c):
  print(a,b,c)
foo(1, 2, 3, 4);
)";
  EXPECT_TRUE(raised(runFromCStr(runtime_, src), LayoutId::kTypeError));
}

TEST_F(CallTest, TooFewArgs) {
  const char* src = R"(
def foo(a,b, c):
  print(a,b,c)
foo(3, 4);
)";
  EXPECT_TRUE(raised(runFromCStr(runtime_, src), LayoutId::kTypeError));
}

static RawObject builtinReturnSecondArg(Thread* /* thread */, Frame* frame,
                                        word nargs) {
  Arguments args(frame, nargs);
  return args.get(1);
}

static void createAndPatchBuiltinReturnSecondArg(Thread* thread,
                                                 Runtime* runtime) {
  HandleScope scope(thread);
  // Ensure we have a __main__ module.
  ASSERT_FALSE(runFromCStr(runtime, "").isError());
  Module main(&scope, findMainModule(runtime));
  runtime->moduleAddBuiltinFunction(thread, main, SymbolId::kDummy,
                                    builtinReturnSecondArg);
  ASSERT_FALSE(runFromCStr(runtime, R"(
@_patch
def dummy(first, second):
  pass
)")
                   .isError());
}

TEST_F(TrampolinesTest, BuiltinTrampolineKwPassesKwargs) {
  HandleScope scope(thread_);
  createAndPatchBuiltinReturnSecondArg(thread_, runtime_);
  ASSERT_FALSE(runFromCStr(runtime_, "result = dummy(second=12345, first=None)")
                   .isError());
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 12345));
}

TEST_F(TrampolinesTest, BuiltinTrampolineKwWithInvalidArgRaisesTypeError) {
  createAndPatchBuiltinReturnSecondArg(thread_, runtime_);
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(runtime_, "dummy(third=3, first=1)"), LayoutId::kTypeError,
      "dummy() got an unexpected keyword argument 'third'"));
}

TEST_F(TrampolinesTest, InterpreterClosureUsesArgOverCellValue) {
  HandleScope scope(thread_);

  // Create code object
  word nlocals = 1;
  Tuple varnames(&scope, runtime_->newTuple(nlocals));
  Tuple cellvars(&scope, runtime_->newTuple(1));
  Object bar(&scope, Runtime::internStrFromCStr(thread_, "bar"));
  varnames.atPut(0, *bar);
  cellvars.atPut(0, *bar);
  const byte bytecode[] = {LOAD_CLOSURE, 0, LOAD_DEREF, 0, RETURN_VALUE, 0};
  Bytes bc(&scope, runtime_->newBytesWithAll(bytecode));
  Tuple empty_tuple(&scope, runtime_->emptyTuple());
  Object empty_str(&scope, Str::empty());
  Object empty_bytes(&scope, Bytes::empty());
  word flags = Code::Flags::kOptimized | Code::Flags::kNewlocals;
  Code code(&scope,
            runtime_->newCode(/*argcount=*/1, /*posonlyargcount=*/0,
                              /*kwonlyargcount=*/0, nlocals, /*stacksize=*/0,
                              flags, /*code=*/bc, /*consts=*/empty_tuple,
                              /*names=*/empty_tuple, varnames,
                              /*freevars=*/empty_tuple, cellvars,
                              /*filename=*/empty_str, /*name=*/empty_str,
                              /*firstlineno=*/0, /*lnotab=*/empty_bytes));
  ASSERT_TRUE(!code.cell2arg().isNoneType());

  Object qualname(&scope, runtime_->newStrFromCStr("foo"));
  Module module(&scope, runtime_->findOrCreateMainModule());
  Function foo(&scope,
               runtime_->newFunctionWithCode(thread_, qualname, code, module));
  Tuple closure_tuple(&scope, runtime_->newTuple(1));
  closure_tuple.atPut(0, runtime_->newInt(99));
  foo.setClosure(*closure_tuple);

  Object argument(&scope, runtime_->newInt(3));
  EXPECT_TRUE(
      isIntEqualsWord(Interpreter::callFunction1(
                          thread_, thread_->currentFrame(), foo, argument),
                      3));
}

TEST_F(TrampolinesTest, InterpreterClosureUsesCellValue) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(arg):
  def bar():
    return arg * 3
  arg = 5
  return bar()

result = foo(-2)
)")
                   .isError());
  Function foo(&scope, mainModuleAt(runtime_, "foo"));
  ASSERT_EQ(foo.entry(), &interpreterClosureTrampoline);
  // Ensure that cellvar was populated.
  Code code(&scope, foo.code());
  ASSERT_TRUE(!code.cell2arg().isNoneType());
  Tuple cellvars(&scope, code.cellvars());
  ASSERT_EQ(cellvars.length(), 1);
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result"), 15));
}

static RawObject makeFunctionWithPosOnlyArg(Thread* thread) {
  // Create:
  //   def foo(a, /, b):
  //     return (a, b)
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object name(&scope, Runtime::internStrFromCStr(thread, "foo"));
  const byte bytecode[] = {LOAD_FAST,   0, LOAD_FAST,    1,
                           BUILD_TUPLE, 2, RETURN_VALUE, 0};
  Tuple varnames(&scope, runtime->newTuple(2));
  varnames.atPut(0, Runtime::internStrFromCStr(thread, "a"));
  varnames.atPut(1, Runtime::internStrFromCStr(thread, "b"));
  Bytes bc(&scope, runtime->newBytesWithAll(bytecode));
  Object empty_tuple(&scope, runtime->emptyTuple());
  Object empty_str(&scope, Str::empty());
  Object empty_bytes(&scope, Bytes::empty());
  Code code(&scope,
            runtime->newCode(/*argcount=*/2, /*posonlyargcount=*/1,
                             /*kwonlyargcount=*/0, /*nlocals=*/2,
                             /*stacksize=*/2,
                             Code::Flags::kNewlocals | Code::Flags::kOptimized,
                             bc, /*consts=*/empty_tuple,
                             /*names=*/empty_tuple, varnames,
                             /*freevars=*/empty_tuple,
                             /*cellvars=*/empty_tuple,
                             /*filename=*/empty_str, name,
                             /*firstlineno=*/0, /*lnotab=*/empty_bytes));
  Module module(&scope, runtime->findOrCreateMainModule());
  return runtime->newFunctionWithCode(thread, name, code, module);
}

TEST_F(TrampolinesTest, KeywordCallRejectsPositionalOnlyArgumentNames) {
  HandleScope scope(thread_);
  Function function(&scope, makeFunctionWithPosOnlyArg(thread_));

  // `foo(a=2, b=4)`
  Frame* frame = thread_->currentFrame();
  frame->pushValue(*function);
  frame->pushValue(runtime_->newInt(2));
  frame->pushValue(runtime_->newInt(4));
  Tuple keywords(&scope, runtime_->newTuple(2));
  keywords.atPut(0, Runtime::internStrFromCStr(thread_, "a"));
  keywords.atPut(1, Runtime::internStrFromCStr(thread_, "b"));
  frame->pushValue(*keywords);
  Object result_obj(&scope, Interpreter::callKw(thread_, frame, 2));
  EXPECT_TRUE(raisedWithStr(
      *result_obj, LayoutId::kTypeError,
      "keyword argument specified for positional-only argument 'a'"));
}

TEST_F(TrampolinesTest, KeywordCallAcceptsNonPositionalOnlyArgumentNames) {
  HandleScope scope(thread_);
  Function function(&scope, makeFunctionWithPosOnlyArg(thread_));

  // `foo(2, b=9)`
  Frame* frame = thread_->currentFrame();
  frame->pushValue(*function);
  frame->pushValue(runtime_->newInt(2));
  frame->pushValue(runtime_->newInt(9));
  Tuple keywords(&scope, runtime_->newTuple(1));
  keywords.atPut(0, Runtime::internStrFromCStr(thread_, "b"));
  frame->pushValue(*keywords);
  Object result_obj(&scope, Interpreter::callKw(thread_, frame, 2));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 2);
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 2));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 9));
}

TEST_F(TrampolinesTest, KeywordCallWithPositionalOnlyArgumentsAndVarKeyArgs) {
  HandleScope scope(thread_);

  // Create:
  //   def foo(a, b=7, /, c=10, **kwargs):
  //     return (a, b, c, kwargs)
  Object name(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  const byte bytecode[] = {LOAD_FAST, 0, LOAD_FAST,   1, LOAD_FAST,    2,
                           LOAD_FAST, 3, BUILD_TUPLE, 4, RETURN_VALUE, 0};
  Tuple varnames(&scope, runtime_->newTuple(4));
  varnames.atPut(0, Runtime::internStrFromCStr(thread_, "a"));
  varnames.atPut(1, Runtime::internStrFromCStr(thread_, "b"));
  varnames.atPut(2, Runtime::internStrFromCStr(thread_, "c"));
  varnames.atPut(3, Runtime::internStrFromCStr(thread_, "kwargs"));
  Bytes bc(&scope, runtime_->newBytesWithAll(bytecode));
  Object empty_tuple(&scope, runtime_->emptyTuple());
  Object empty_str(&scope, Str::empty());
  Object empty_bytes(&scope, Bytes::empty());
  Code code(&scope, runtime_->newCode(
                        /*argcount=*/3, /*posonlyargcount=*/2,
                        /*kwonlyargcount=*/0, /*nlocals=*/4,
                        /*stacksize=*/4,
                        Code::Flags::kNewlocals | Code::Flags::kOptimized |
                            Code::Flags::kVarkeyargs,
                        bc,
                        /*consts=*/empty_tuple,
                        /*names=*/empty_tuple, varnames,
                        /*freevars=*/empty_tuple,
                        /*cellvars=*/empty_tuple,
                        /*filename=*/empty_str, name,
                        /*firstlineno=*/0, /*lnotab=*/empty_bytes));
  Module module(&scope, runtime_->findOrCreateMainModule());
  Function foo(&scope,
               runtime_->newFunctionWithCode(thread_, name, code, module));
  Tuple defaults(&scope, runtime_->newTuple(2));
  defaults.atPut(0, runtime_->newInt(7));
  defaults.atPut(1, runtime_->newInt(10));
  foo.setDefaults(*defaults);
  // Call foo(1, c=13, b=5).
  Frame* frame = thread_->currentFrame();
  frame->pushValue(*foo);
  frame->pushValue(runtime_->newInt(1));
  frame->pushValue(runtime_->newInt(13));
  frame->pushValue(runtime_->newInt(5));
  Tuple keywords(&scope, runtime_->newTuple(2));
  keywords.atPut(0, Runtime::internStrFromCStr(thread_, "c"));
  keywords.atPut(1, Runtime::internStrFromCStr(thread_, "b"));
  frame->pushValue(*keywords);
  Object result_obj(&scope, Interpreter::callKw(thread_, frame, 3));

  // Expect a `(1, 7, 13, {'b': 5})` result.
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 4);
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 7));
  EXPECT_TRUE(isIntEqualsWord(result.at(2), 13));
  ASSERT_TRUE(result.at(3).isDict());
  Dict result_dict(&scope, result.at(3));
  EXPECT_EQ(result_dict.numItems(), 1);
  Object b_name(&scope, Runtime::internStrFromCStr(thread_, "b"));
  EXPECT_TRUE(isIntEqualsWord(dictAtByStr(thread_, result_dict, b_name), 5));
}

TEST_F(TrampolinesTest, ExplodeCallWithBadKeywordFails) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
def take_kwargs(a): pass

kwargs = {12: 34}
take_kwargs(**kwargs)
)"),
                            LayoutId::kTypeError, "keywords must be strings"));
}

TEST_F(TrampolinesTest, ExplodeCallWithZeroKeywords) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def foo(a=10): return a
result = foo(**{})
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_EQ(result, SmallInt::fromWord(10));
}

static PyObject* binaryReturnInt123(PyObject*, PyObject*) {
  return ApiHandle::newReference(Thread::current(), SmallInt::fromWord(123));
}

TEST_F(TrampolinesTest, ExtensionModuleNoArgReceivesNoArgsReturns) {
  HandleScope scope(thread_);
  Function callee(
      &scope, functionFromModuleMethodDef(
                  thread_, "foo", reinterpret_cast<void*>(binaryReturnInt123),
                  "", ExtensionMethodType::kMethNoArgs));

  // Set up a code object that calls the function without arguments
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_->newTuple(1));
  consts.atPut(0, *callee);
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, CALL_FUNCTION, 0, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));
  code.setStacksize(1);

  // Execute the code and make sure we get back the result we expect
  Object result(&scope, runCode(code));
  EXPECT_TRUE(isIntEqualsWord(*result, 123));
}

TEST_F(TrampolinesTest, ExtensionModuleNoArgReceivesArgsRaisesTypeError) {
  HandleScope scope(thread_);
  Function callee(
      &scope, functionFromModuleMethodDef(
                  thread_, "foo", reinterpret_cast<void*>(binaryReturnInt123),
                  "", ExtensionMethodType::kMethNoArgs));

  // Set up a code object that calls the function with a single argument.
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_->newTuple(2));
  consts.atPut(0, *callee);
  consts.atPut(1, SmallInt::fromWord(1111));
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST,    0, LOAD_CONST,   1,
                           CALL_FUNCTION, 1, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));
  code.setStacksize(2);

  // Execute the code and make sure we get back the result we expect
  EXPECT_TRUE(raisedWithStr(runCode(code), LayoutId::kTypeError,
                            "function takes no arguments"));
}

static PyObject* binaryReturnNullptrNoException(PyObject*, PyObject*) {
  return nullptr;
}

TEST_F(TrampolinesTest, ExtensionModuleNoArgReturnsNullRaisesSystemError) {
  HandleScope scope(thread_);
  Function callee(
      &scope, functionFromModuleMethodDef(
                  thread_, "foo",
                  reinterpret_cast<void*>(binaryReturnNullptrNoException), "",
                  ExtensionMethodType::kMethNoArgs));

  // Set up a code object that calls the function without arguments
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_->newTuple(1));
  consts.atPut(0, *callee);
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, CALL_FUNCTION, 0, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));
  code.setStacksize(1);

  // Execute the code and make sure we get back the result we expect
  EXPECT_TRUE(raisedWithStr(runCode(code), LayoutId::kSystemError,
                            "NULL return without exception set"));
}

TEST_F(TrampolinesTest, ExtensionModuleNoArgReceivesKwArgsRaisesTypeError) {
  HandleScope scope(thread_);
  Function callee(
      &scope, functionFromModuleMethodDef(
                  thread_, "foo", reinterpret_cast<void*>(binaryReturnInt123),
                  "", ExtensionMethodType::kMethNoArgs));

  // Set up a code object that calls the builtin with (foo='bar')
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_->newTuple(3));
  consts.atPut(0, *callee);
  consts.atPut(1, runtime_->newStrFromCStr("bar"));
  Tuple kw_tuple(&scope, runtime_->newTuple(1));
  kw_tuple.atPut(0, runtime_->newStrFromCStr("foo"));
  consts.atPut(2, *kw_tuple);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST,       0, LOAD_CONST,   1, LOAD_CONST, 2,
                           CALL_FUNCTION_KW, 1, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));
  code.setStacksize(3);

  // Execute the code and make sure we get back the result we expect
  EXPECT_TRUE(raised(runCode(code), LayoutId::kTypeError));
}

TEST_F(TrampolinesTest, ExtensionModuleNoArgReceivesZeroKwArgsReturns) {
  HandleScope scope(thread_);
  Function callee(
      &scope, functionFromModuleMethodDef(
                  thread_, "foo", reinterpret_cast<void*>(binaryReturnInt123),
                  "", ExtensionMethodType::kMethNoArgs));

  // Set up a code object that calls the builtin with (foo='bar')
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_->newTuple(2));
  consts.atPut(0, *callee);
  Tuple kw_tuple(&scope, runtime_->emptyTuple());
  consts.atPut(1, *kw_tuple);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST,       0, LOAD_CONST,   1,
                           CALL_FUNCTION_KW, 0, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));
  code.setStacksize(2);

  // Execute the code and make sure we get back the result we expect
  Object result(&scope, runCode(code));
  EXPECT_TRUE(isIntEqualsWord(*result, 123));
}

TEST_F(TrampolinesTest,
       ExtensionModuleNoArgReceivesVariableArgsRaisesTypeError) {
  HandleScope scope(thread_);
  Function callee(
      &scope, functionFromModuleMethodDef(
                  thread_, "foo", reinterpret_cast<void*>(binaryReturnInt123),
                  "", ExtensionMethodType::kMethNoArgs));

  // Set up a code object that calls with (*(10))
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_->newTuple(2));
  consts.atPut(0, *callee);
  Tuple arg_tuple(&scope, runtime_->newTuple(1));
  arg_tuple.atPut(0, SmallInt::fromWord(10));
  consts.atPut(1, *arg_tuple);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST,       0, LOAD_CONST,   1,
                           CALL_FUNCTION_EX, 0, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));
  code.setStacksize(2);

  // Execute the code and make sure we get back the result we expect
  EXPECT_TRUE(raised(runCode(code), LayoutId::kTypeError));
}

TEST_F(TrampolinesTest, ExtensionModuleNoArgReceivesVariableArgsReturns) {
  HandleScope scope(thread_);
  Function callee(
      &scope, functionFromModuleMethodDef(
                  thread_, "foo", reinterpret_cast<void*>(binaryReturnInt123),
                  "", ExtensionMethodType::kMethNoArgs));

  // Set up a code object that calls with (*())
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_->newTuple(2));
  consts.atPut(0, *callee);
  Tuple arg_tuple(&scope, runtime_->emptyTuple());
  consts.atPut(1, *arg_tuple);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST,       0, LOAD_CONST,   1,
                           CALL_FUNCTION_EX, 0, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));
  code.setStacksize(2);

  // Execute the code and make sure we get back the result we expect
  Object result(&scope, runCode(code));
  EXPECT_TRUE(isIntEqualsWord(*result, 123));
}

TEST_F(TrampolinesTest, ExtensionModuleOneArgReceivesNoArgsRaisesTypeError) {
  HandleScope scope(thread_);
  Function callee(
      &scope, functionFromModuleMethodDef(
                  thread_, "foo", reinterpret_cast<void*>(binaryReturnInt123),
                  "", ExtensionMethodType::kMethO));

  // Set up a code object that calls the function without arguments
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_->newTuple(1));
  consts.atPut(0, *callee);
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, CALL_FUNCTION, 0, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));
  code.setStacksize(1);

  // Execute the code and make sure we get back the result we expect
  Object result(&scope, runCode(code));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

PyObject* returnSecondArgument(PyObject* /*lhs*/, PyObject* rhs) {
  return ApiHandle::newReference(Thread::current(),
                                 ApiHandle::fromPyObject(rhs)->asObject());
}

TEST_F(TrampolinesTest, ExtensionModuleOneArgReceivesOneArgReturns) {
  HandleScope scope(thread_);
  Function callee(
      &scope, functionFromModuleMethodDef(
                  thread_, "foo", reinterpret_cast<void*>(returnSecondArgument),
                  "", ExtensionMethodType::kMethO));

  // Set up a code object that calls the function with a single argument.
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_->newTuple(2));
  consts.atPut(0, *callee);
  consts.atPut(1, SmallInt::fromWord(1111));
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST,    0, LOAD_CONST,   1,
                           CALL_FUNCTION, 1, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));
  code.setStacksize(2);

  // Execute the code and make sure we get back the result we expect
  RawObject result = runCode(code);
  EXPECT_TRUE(isIntEqualsWord(result, 1111));
}

TEST_F(TrampolinesTest,
       ExtensionModuleOneArgReceivesMultipleArgsRaisesTypeError) {
  HandleScope scope(thread_);
  Function callee(
      &scope, functionFromModuleMethodDef(
                  thread_, "foo", reinterpret_cast<void*>(returnSecondArgument),
                  "", ExtensionMethodType::kMethO));

  // Set up a code object that calls the function with (123, 456)
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_->newTuple(3));
  consts.atPut(0, *callee);
  consts.atPut(1, SmallInt::fromWord(123));
  consts.atPut(2, SmallInt::fromWord(456));
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST,    0, LOAD_CONST,   1, LOAD_CONST, 2,
                           CALL_FUNCTION, 2, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));
  code.setStacksize(3);

  // Execute the code and make sure we get back the result we expect
  Object result(&scope, runCode(code));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST_F(TrampolinesTest, ExtensionModuleOneArgReturnsNullRaisesSystemError) {
  HandleScope scope(thread_);
  Function callee(
      &scope, functionFromModuleMethodDef(
                  thread_, "foo",
                  reinterpret_cast<void*>(binaryReturnNullptrNoException), "",
                  ExtensionMethodType::kMethO));

  // Set up a code object that calls the function without arguments
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_->newTuple(2));
  consts.atPut(0, *callee);
  consts.atPut(1, SmallInt::fromWord(1111));
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST,    0, LOAD_CONST,   1,
                           CALL_FUNCTION, 1, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));
  code.setStacksize(2);

  // Execute the code and make sure we get back the result we expect
  Object result(&scope, runCode(code));
  EXPECT_TRUE(raised(*result, LayoutId::kSystemError));
}

TEST_F(TrampolinesTest,
       ExtensionModuleOneArgReceivesOneArgAndZeroKwArgsReturns) {
  HandleScope scope(thread_);
  Function callee(
      &scope, functionFromModuleMethodDef(
                  thread_, "foo", reinterpret_cast<void*>(returnSecondArgument),
                  "", ExtensionMethodType::kMethO));

  // Set up a code object that calls the builtin with (1111, {})
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_->newTuple(3));
  consts.atPut(0, *callee);
  consts.atPut(1, SmallInt::fromWord(1111));
  Tuple kw_tuple(&scope, runtime_->emptyTuple());
  consts.atPut(2, *kw_tuple);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST,       0, LOAD_CONST,   1, LOAD_CONST, 2,
                           CALL_FUNCTION_KW, 1, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));
  code.setStacksize(3);

  // Execute the code and make sure we get back the result we expect
  RawObject result = runCode(code);
  EXPECT_TRUE(isIntEqualsWord(result, 1111));
}

TEST_F(TrampolinesTest, ExtensionModuleOneArgReceivesKwArgsRaisesTypeError) {
  HandleScope scope(thread_);
  Function callee(
      &scope, functionFromModuleMethodDef(
                  thread_, "foo", reinterpret_cast<void*>(binaryReturnInt123),
                  "", ExtensionMethodType::kMethO));

  // Set up a code object that calls the builtin with (1111, foo='bar')
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_->newTuple(4));
  consts.atPut(0, *callee);
  Tuple kw_tuple(&scope, runtime_->newTuple(1));
  consts.atPut(1, SmallInt::fromWord(1111));
  consts.atPut(2, runtime_->newStrFromCStr("bar"));
  kw_tuple.atPut(0, runtime_->newStrFromCStr("foo"));
  consts.atPut(3, *kw_tuple);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST, 0, LOAD_CONST,       1, LOAD_CONST,   2,
                           LOAD_CONST, 3, CALL_FUNCTION_KW, 2, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));
  code.setStacksize(4);

  // Execute the code and make sure we get back the result we expect
  Object result(&scope, runCode(code));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST_F(TrampolinesTest, ExtensionModuleOneArgReceivesOneArgExReturns) {
  HandleScope scope(thread_);
  Function callee(
      &scope, functionFromModuleMethodDef(
                  thread_, "foo", reinterpret_cast<void*>(returnSecondArgument),
                  "", ExtensionMethodType::kMethO));

  // Set up a code object that calls with (*(10))
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_->newTuple(2));
  consts.atPut(0, *callee);
  Tuple arg_tuple(&scope, runtime_->newTuple(1));
  arg_tuple.atPut(0, SmallInt::fromWord(1111));
  consts.atPut(1, *arg_tuple);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST,       0, LOAD_CONST,   1,
                           CALL_FUNCTION_EX, 0, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));
  code.setStacksize(2);

  // Execute the code and make sure we get back the result we expect
  RawObject result = runCode(code);
  EXPECT_TRUE(isIntEqualsWord(result, 1111));
}

TEST_F(TrampolinesTest, ExtensionModuleOneArgReceivesOneArgAndEmptyKwReturns) {
  HandleScope scope(thread_);
  Function callee(
      &scope, functionFromModuleMethodDef(
                  thread_, "foo", reinterpret_cast<void*>(returnSecondArgument),
                  "", ExtensionMethodType::kMethO));

  // Set up a code object that calls with (*(10), {})
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_->newTuple(3));
  consts.atPut(0, *callee);
  Tuple arg_tuple(&scope, runtime_->newTuple(1));
  arg_tuple.atPut(0, SmallInt::fromWord(1111));
  consts.atPut(1, *arg_tuple);
  Dict kw_dict(&scope, runtime_->newDict());
  consts.atPut(2, *kw_dict);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST,       0, LOAD_CONST,   1, LOAD_CONST, 2,
                           CALL_FUNCTION_EX, 1, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));
  code.setStacksize(3);

  // Execute the code and make sure we get back the result we expect
  RawObject result = runCode(code);
  EXPECT_TRUE(isIntEqualsWord(result, 1111));
}

TEST_F(TrampolinesTest,
       ExtensionModuleOneArgReceivesOneArgAndKwRaisesTypeError) {
  HandleScope scope(thread_);
  Function callee(
      &scope, functionFromModuleMethodDef(
                  thread_, "foo", reinterpret_cast<void*>(binaryReturnInt123),
                  "", ExtensionMethodType::kMethO));

  // Set up a code object that calls with (*(10), {2:3})
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_->newTuple(3));
  consts.atPut(0, *callee);
  Tuple arg_tuple(&scope, runtime_->newTuple(1));
  arg_tuple.atPut(0, SmallInt::fromWord(1111));
  consts.atPut(1, *arg_tuple);
  Dict kw_dict(&scope, runtime_->newDict());
  Object key(&scope, SmallInt::fromWord(2));
  word hash = intHash(*key);
  Object value(&scope, SmallInt::fromWord(3));
  dictAtPut(thread_, kw_dict, key, hash, value);
  consts.atPut(2, *kw_dict);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST,       0, LOAD_CONST,   1, LOAD_CONST, 2,
                           CALL_FUNCTION_EX, 1, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));
  code.setStacksize(3);

  // Execute the code and make sure we get back the result we expect
  Object result(&scope, runCode(code));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

static PyObject* pyCFunctionFastFunc(PyObject*, PyObject** args,
                                     Py_ssize_t nargs, PyObject*) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Tuple tuple(&scope, thread->runtime()->newTuple(2));
  tuple.atPut(0, ApiHandle::fromPyObject(*args)->asObject());
  tuple.atPut(1, SmallInt::fromWord(nargs));
  return ApiHandle::newReference(thread, *tuple);
}

TEST_F(TrampolinesTest, ExtensionModuleFastCallReceivesArgsReturns) {
  HandleScope scope(thread_);
  Function callee(
      &scope, functionFromModuleMethodDef(
                  thread_, "foo", reinterpret_cast<void*>(pyCFunctionFastFunc),
                  "", ExtensionMethodType::kMethFastCall));

  // Set up a code object that calls the function with a single argument.
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_->newTuple(2));
  consts.atPut(0, *callee);
  consts.atPut(1, SmallInt::fromWord(1111));
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST,    0, LOAD_CONST,   1,
                           CALL_FUNCTION, 1, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));
  code.setStacksize(2);

  // Execute the code and make sure we get back the result we expect
  RawObject output = runCode(code);
  Tuple result(&scope, output);
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 1111));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 1));
}

TEST_F(TrampolinesTest, ExtensionModuleFastCallReturnsNullRaisesSystemError) {
  _PyCFunctionFast func = [](PyObject*, PyObject**, Py_ssize_t,
                             PyObject*) -> PyObject* { return nullptr; };

  HandleScope scope(thread_);
  Function callee(&scope, functionFromModuleMethodDef(
                              thread_, "foo", bit_cast<void*>(func), "",
                              ExtensionMethodType::kMethFastCall));

  // Set up a code object that calls the function without arguments
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_->newTuple(1));
  consts.atPut(0, *callee);
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, CALL_FUNCTION, 0, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));
  code.setStacksize(1);

  // Execute the code and make sure we get back the result we expect
  EXPECT_TRUE(raisedWithStr(runCode(code), LayoutId::kSystemError,
                            "NULL return without exception set"));
}

TEST_F(TrampolinesTest, ExtensionModuleFastcallReceivesKwArgsReturns) {
  HandleScope scope(thread_);
  Function callee(
      &scope, functionFromModuleMethodDef(
                  thread_, "foo", reinterpret_cast<void*>(pyCFunctionFastFunc),
                  "", ExtensionMethodType::kMethFastCall));

  // Set up a code object that calls the builtin with ("bar", foo=1111)
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_->newTuple(4));
  consts.atPut(0, *callee);
  consts.atPut(1, runtime_->newStrFromCStr("bar"));
  consts.atPut(2, SmallInt::fromWord(1111));
  Tuple kw_tuple(&scope, runtime_->newTuple(1));
  kw_tuple.atPut(0, runtime_->newStrFromCStr("foo"));
  consts.atPut(3, *kw_tuple);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST, 0, LOAD_CONST,       1, LOAD_CONST,   2,
                           LOAD_CONST, 3, CALL_FUNCTION_KW, 2, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));
  code.setStacksize(4);

  // Execute the code and make sure we get back the result we expect
  RawObject output = runCode(code);
  Tuple result(&scope, output);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "bar"));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 1));
}

TEST_F(TrampolinesTest, ExtensionModuleFastCallReceivesVariableArgsReturns) {
  HandleScope scope(thread_);
  Function callee(
      &scope, functionFromModuleMethodDef(
                  thread_, "foo", reinterpret_cast<void*>(pyCFunctionFastFunc),
                  "", ExtensionMethodType::kMethFastCall));

  // Set up a code object that calls with (*(10))
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_->newTuple(2));
  consts.atPut(0, *callee);
  Tuple arg_tuple(&scope, runtime_->newTuple(1));
  arg_tuple.atPut(0, SmallInt::fromWord(10));
  consts.atPut(1, *arg_tuple);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST,       0, LOAD_CONST,   1,
                           CALL_FUNCTION_EX, 0, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));
  code.setStacksize(2);

  // Execute the code and make sure we get back the result we expect
  RawObject output = runCode(code);
  Tuple result(&scope, output);
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 10));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 1));
}

TEST_F(TrampolinesTest, ExtensionModuleFastCallReceivesVariableKwArgsReturns) {
  HandleScope scope(thread_);
  Function callee(
      &scope, functionFromModuleMethodDef(
                  thread_, "foo", reinterpret_cast<void*>(pyCFunctionFastFunc),
                  "", ExtensionMethodType::kMethFastCall));

  // Set up a code object that calls with (*(10), **{"foo":1111})
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_->newTuple(3));
  consts.atPut(0, *callee);
  Tuple arg_tuple(&scope, runtime_->newTuple(1));
  arg_tuple.atPut(0, SmallInt::fromWord(10));
  consts.atPut(1, *arg_tuple);
  Dict kw_dict(&scope, runtime_->newDict());
  Str name(&scope, runtime_->newStrFromCStr("foo"));
  Object value(&scope, SmallInt::fromWord(1111));
  dictAtPutByStr(thread_, kw_dict, name, value);
  consts.atPut(2, *kw_dict);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST,       0, LOAD_CONST,   1, LOAD_CONST, 2,
                           CALL_FUNCTION_EX, 1, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));
  code.setStacksize(3);

  // Execute the code and make sure we get back the result we expect
  RawObject output = runCode(code);
  Tuple result(&scope, output);
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 10));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 1));
}

TEST_F(TrampolinesTest, ExtensionModuleVarArgReceivesNoArgsReturns) {
  HandleScope scope(thread_);
  Function callee(
      &scope, functionFromModuleMethodDef(
                  thread_, "foo", reinterpret_cast<void*>(binaryReturnInt123),
                  "", ExtensionMethodType::kMethVarArgs));

  // Set up a code object that calls the function without arguments
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_->newTuple(1));
  consts.atPut(0, *callee);
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, CALL_FUNCTION, 0, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));
  code.setStacksize(1);

  // Execute the code and make sure we get back the result we expect
  RawObject result = runCode(code);
  EXPECT_TRUE(isIntEqualsWord(result, 123));
}

static PyObject* returnFirstTupleArg(PyObject*, PyObject* args) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Tuple arg_tuple(&scope, ApiHandle::fromPyObject(args)->asObject());
  return ApiHandle::newReference(thread, arg_tuple.at(0));
}

TEST_F(TrampolinesTest, ExtensionModuleVarArgReceivesArgsReturns) {
  HandleScope scope(thread_);
  Function callee(
      &scope, functionFromModuleMethodDef(
                  thread_, "foo", reinterpret_cast<void*>(returnFirstTupleArg),
                  "", ExtensionMethodType::kMethVarArgs));

  // Set up a code object that calls the function with a single argument.
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_->newTuple(2));
  consts.atPut(0, *callee);
  consts.atPut(1, SmallInt::fromWord(1111));
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST,    0, LOAD_CONST,   1,
                           CALL_FUNCTION, 1, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));
  code.setStacksize(2);

  // Execute the code and make sure we get back the result we expect
  RawObject result = runCode(code);
  EXPECT_TRUE(isIntEqualsWord(result, 1111));
}

TEST_F(TrampolinesTest, ExtensionModuleVarArgReturnsNullRaisesSystemError) {
  HandleScope scope(thread_);
  Function callee(
      &scope, functionFromModuleMethodDef(
                  thread_, "foo",
                  reinterpret_cast<void*>(binaryReturnNullptrNoException), "",
                  ExtensionMethodType::kMethVarArgs));

  // Set up a code object that calls the function without arguments
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_->newTuple(1));
  consts.atPut(0, *callee);
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, CALL_FUNCTION, 0, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));
  code.setStacksize(1);

  // Execute the code and make sure we get back the result we expect
  EXPECT_TRUE(raisedWithStr(runCode(code), LayoutId::kSystemError,
                            "NULL return without exception set"));
}

TEST_F(TrampolinesTest, ExtensionModuleVarArgReceivesZeroKwArgsReturns) {
  HandleScope scope(thread_);
  Function callee(
      &scope, functionFromModuleMethodDef(
                  thread_, "foo", reinterpret_cast<void*>(returnFirstTupleArg),
                  "", ExtensionMethodType::kMethVarArgs));

  // Set up a code object that calls the builtin with (1111, {})
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_->newTuple(3));
  consts.atPut(0, *callee);
  consts.atPut(1, SmallInt::fromWord(1111));
  Tuple kw_tuple(&scope, runtime_->emptyTuple());
  consts.atPut(2, *kw_tuple);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST,       0, LOAD_CONST,   1, LOAD_CONST, 2,
                           CALL_FUNCTION_KW, 1, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));
  code.setStacksize(3);

  // Execute the code and make sure we get back the result we expect
  RawObject result = runCode(code);
  EXPECT_TRUE(isIntEqualsWord(result, 1111));
}

TEST_F(TrampolinesTest, ExtensionModuleVarArgReceivesKwArgsRaisesTypeError) {
  HandleScope scope(thread_);
  Function callee(
      &scope, functionFromModuleMethodDef(
                  thread_, "foo", reinterpret_cast<void*>(binaryReturnInt123),
                  "", ExtensionMethodType::kMethVarArgs));

  // Set up a code object that calls the builtin with (1111, foo='bar')
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_->newTuple(4));
  consts.atPut(0, *callee);
  Tuple kw_tuple(&scope, runtime_->newTuple(1));
  consts.atPut(1, SmallInt::fromWord(1111));
  consts.atPut(2, runtime_->newStrFromCStr("bar"));
  kw_tuple.atPut(0, runtime_->newStrFromCStr("foo"));
  consts.atPut(3, *kw_tuple);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST, 0, LOAD_CONST,       1, LOAD_CONST,   2,
                           LOAD_CONST, 3, CALL_FUNCTION_KW, 2, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));
  code.setStacksize(4);

  // Execute the code and make sure we get back the result we expect
  Object result(&scope, runCode(code));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST_F(TrampolinesTest, ExtensionModuleVarArgReceivesVarArgsReturns) {
  HandleScope scope(thread_);
  Function callee(
      &scope, functionFromModuleMethodDef(
                  thread_, "foo", reinterpret_cast<void*>(returnFirstTupleArg),
                  "", ExtensionMethodType::kMethVarArgs));

  // Set up a code object that calls with (*(10))
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_->newTuple(2));
  consts.atPut(0, *callee);
  Tuple arg_tuple(&scope, runtime_->newTuple(1));
  arg_tuple.atPut(0, SmallInt::fromWord(1111));
  consts.atPut(1, *arg_tuple);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST,       0, LOAD_CONST,   1,
                           CALL_FUNCTION_EX, 0, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));
  code.setStacksize(2);

  // Execute the code and make sure we get back the result we expect
  RawObject result = runCode(code);
  EXPECT_TRUE(isIntEqualsWord(result, 1111));
}

TEST_F(TrampolinesTest, ExtensionModuleVarArgReceivesVarArgsAndEmptyKwReturns) {
  HandleScope scope(thread_);
  Function callee(
      &scope, functionFromModuleMethodDef(
                  thread_, "foo", reinterpret_cast<void*>(returnFirstTupleArg),
                  "", ExtensionMethodType::kMethVarArgs));

  // Set up a code object that calls with (*(10), {})
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_->newTuple(3));
  consts.atPut(0, *callee);
  Tuple arg_tuple(&scope, runtime_->newTuple(1));
  arg_tuple.atPut(0, SmallInt::fromWord(1111));
  consts.atPut(1, *arg_tuple);
  Dict kw_dict(&scope, runtime_->newDict());
  consts.atPut(2, *kw_dict);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST,       0, LOAD_CONST,   1, LOAD_CONST, 2,
                           CALL_FUNCTION_EX, 1, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));
  code.setStacksize(3);

  // Execute the code and make sure we get back the result we expect
  RawObject result = runCode(code);
  EXPECT_TRUE(isIntEqualsWord(result, 1111));
}

TEST_F(TrampolinesTest,
       ExtensionModuleVarArgReceivesVarArgsAndKwRaisesTypeError) {
  HandleScope scope(thread_);
  Function callee(
      &scope, functionFromModuleMethodDef(
                  thread_, "foo", reinterpret_cast<void*>(binaryReturnInt123),
                  "", ExtensionMethodType::kMethVarArgs));

  // Set up a code object that calls with (*(10), {})
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_->newTuple(3));
  consts.atPut(0, *callee);
  Tuple arg_tuple(&scope, runtime_->newTuple(1));
  arg_tuple.atPut(0, SmallInt::fromWord(1111));
  consts.atPut(1, *arg_tuple);
  Dict kw_dict(&scope, runtime_->newDict());
  Object key(&scope, SmallInt::fromWord(2));
  word hash = intHash(*key);
  Object value(&scope, SmallInt::fromWord(3));
  dictAtPut(thread_, kw_dict, key, hash, value);
  consts.atPut(2, *kw_dict);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST,       0, LOAD_CONST,   1, LOAD_CONST, 2,
                           CALL_FUNCTION_EX, 1, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));
  code.setStacksize(3);

  // Execute the code and make sure we get back the result we expect
  Object result(&scope, runCode(code));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

static PyObject* ternaryReturnInt123(PyObject*, PyObject*, PyObject*) {
  return ApiHandle::newReference(Thread::current(), SmallInt::fromWord(123));
}

TEST_F(TrampolinesTest, ExtensionModuleKeywordArgReceivesNoArgsReturns) {
  HandleScope scope(thread_);
  Function callee(
      &scope, functionFromModuleMethodDef(
                  thread_, "foo", reinterpret_cast<void*>(ternaryReturnInt123),
                  "", ExtensionMethodType::kMethVarArgsAndKeywords));

  // Set up a code object that calls the function without arguments
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_->newTuple(1));
  consts.atPut(0, *callee);
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, CALL_FUNCTION, 0, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));
  code.setStacksize(1);

  // Execute the code and make sure we get back the result we expect
  RawObject result = runCode(code);
  EXPECT_TRUE(isIntEqualsWord(result, 123));
}

static PyObject* ternaryReturnFirstTupleArg(PyObject*, PyObject* args,
                                            PyObject*) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Tuple arg_tuple(&scope, ApiHandle::fromPyObject(args)->asObject());
  return ApiHandle::newReference(thread, arg_tuple.at(0));
}

TEST_F(TrampolinesTest, ExtensionModuleKeywordArgReceivesArgsReturns) {
  HandleScope scope(thread_);
  Function callee(
      &scope,
      functionFromModuleMethodDef(
          thread_, "foo", reinterpret_cast<void*>(ternaryReturnFirstTupleArg),
          "", ExtensionMethodType::kMethVarArgsAndKeywords));

  // Set up a code object that calls the function with a single argument.
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_->newTuple(2));
  consts.atPut(0, *callee);
  consts.atPut(1, SmallInt::fromWord(1111));
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST,    0, LOAD_CONST,   1,
                           CALL_FUNCTION, 1, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));
  code.setStacksize(2);

  // Execute the code and make sure we get back the result we expect
  RawObject result = runCode(code);
  EXPECT_TRUE(isIntEqualsWord(result, 1111));
}

static PyObject* ternaryReturnNullptrNoException(PyObject*, PyObject*,
                                                 PyObject*) {
  return nullptr;
}

TEST_F(TrampolinesTest, ExtensionModuleKeywordArgReturnsNullRaisesSystemError) {
  HandleScope scope(thread_);
  Function callee(
      &scope, functionFromModuleMethodDef(
                  thread_, "foo",
                  reinterpret_cast<void*>(ternaryReturnNullptrNoException), "",
                  ExtensionMethodType::kMethVarArgsAndKeywords));

  // Set up a code object that calls the function without arguments
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_->newTuple(1));
  consts.atPut(0, *callee);
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, CALL_FUNCTION, 0, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));
  code.setStacksize(1);

  // Execute the code and make sure we get back the result we expect
  Object result(&scope, runCode(code));
  EXPECT_TRUE(raisedWithStr(*result, LayoutId::kSystemError,
                            "NULL return without exception set"));
}

static PyObject* returnsKwargCalledFoo(PyObject*, PyObject*, PyObject* kwargs) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Str foo_str(&scope, runtime->newStrFromCStr("foo"));
  Dict keyword_dict(&scope, ApiHandle::fromPyObject(kwargs)->asObject());
  return ApiHandle::newReference(thread,
                                 dictAtByStr(thread, keyword_dict, foo_str));
}

TEST_F(TrampolinesTest, ExtensionModuleKeywordArgReceivesKwArgsReturns) {
  HandleScope scope(thread_);
  Function callee(
      &scope,
      functionFromModuleMethodDef(
          thread_, "foo", reinterpret_cast<void*>(returnsKwargCalledFoo), "",
          ExtensionMethodType::kMethVarArgsAndKeywords));

  // Set up a code object that calls the builtin with ("bar", foo=1111)
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_->newTuple(4));
  consts.atPut(0, *callee);
  consts.atPut(1, runtime_->newStrFromCStr("bar"));
  consts.atPut(2, SmallInt::fromWord(1111));
  Tuple kw_tuple(&scope, runtime_->newTuple(1));
  kw_tuple.atPut(0, runtime_->newStrFromCStr("foo"));
  consts.atPut(3, *kw_tuple);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST, 0, LOAD_CONST,       1, LOAD_CONST,   2,
                           LOAD_CONST, 3, CALL_FUNCTION_KW, 2, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));
  code.setStacksize(4);

  // Execute the code and make sure we get back the result we expect
  RawObject result = runCode(code);
  EXPECT_TRUE(isIntEqualsWord(result, 1111));
}

TEST_F(TrampolinesTest, ExtensionModuleKeywordArgReceivesMultipleArgsReturns) {
  HandleScope scope(thread_);
  Function callee(
      &scope,
      functionFromModuleMethodDef(
          thread_, "foo", reinterpret_cast<void*>(ternaryReturnFirstTupleArg),
          "", ExtensionMethodType::kMethVarArgsAndKeywords));

  // Set up a code object that calls the builtin with (123, 456, foo=789)
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_->newTuple(5));
  consts.atPut(0, *callee);
  consts.atPut(1, SmallInt::fromWord(456));
  consts.atPut(2, SmallInt::fromWord(123));
  consts.atPut(3, SmallInt::fromWord(789));
  Tuple kw_tuple(&scope, runtime_->newTuple(1));
  kw_tuple.atPut(0, runtime_->newStrFromCStr("foo"));
  consts.atPut(4, *kw_tuple);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST,   0, LOAD_CONST, 1, LOAD_CONST,       2,
                           LOAD_CONST,   3, LOAD_CONST, 4, CALL_FUNCTION_KW, 3,
                           RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));
  code.setStacksize(5);

  // Execute the code and make sure we get back the result we expect
  RawObject result = runCode(code);
  EXPECT_TRUE(isIntEqualsWord(result, 456));
}

TEST_F(TrampolinesTest,
       ExtensionModuleKeywordArgReceivesMultipleKwArgsReturns) {
  HandleScope scope(thread_);
  Function callee(
      &scope,
      functionFromModuleMethodDef(
          thread_, "foo", reinterpret_cast<void*>(returnsKwargCalledFoo), "",
          ExtensionMethodType::kMethVarArgsAndKeywords));

  // Set up a code object that calls the builtin with ("foo"=1234, "bar"=5678)
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_->newTuple(4));
  consts.atPut(0, *callee);
  consts.atPut(1, SmallInt::fromWord(1234));
  consts.atPut(2, SmallInt::fromWord(5678));
  Tuple kw_tuple(&scope, runtime_->newTuple(2));
  kw_tuple.atPut(0, runtime_->newStrFromCStr("foo"));
  kw_tuple.atPut(1, runtime_->newStrFromCStr("bar"));
  consts.atPut(3, *kw_tuple);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST, 0, LOAD_CONST,       1, LOAD_CONST,   2,
                           LOAD_CONST, 3, CALL_FUNCTION_KW, 2, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));
  code.setStacksize(4);

  // Execute the code and make sure we get back the result we expect
  RawObject result = runCode(code);
  EXPECT_TRUE(isIntEqualsWord(result, 1234));
}

TEST_F(TrampolinesTest, ExtensionModuleKeywordArgReceivesVariableArgsReturns) {
  HandleScope scope(thread_);
  Function callee(
      &scope,
      functionFromModuleMethodDef(
          thread_, "foo", reinterpret_cast<void*>(ternaryReturnFirstTupleArg),
          "", ExtensionMethodType::kMethVarArgsAndKeywords));

  // Set up a code object that calls with (*(10))
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_->newTuple(2));
  consts.atPut(0, *callee);
  Tuple arg_tuple(&scope, runtime_->newTuple(1));
  arg_tuple.atPut(0, SmallInt::fromWord(10));
  consts.atPut(1, *arg_tuple);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST,       0, LOAD_CONST,   1,
                           CALL_FUNCTION_EX, 0, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));
  code.setStacksize(2);

  // Execute the code and make sure we get back the result we expect
  RawObject result = runCode(code);
  EXPECT_TRUE(isIntEqualsWord(result, 10));
}

TEST_F(TrampolinesTest,
       ExtensionModuleKeywordArgReceivesVariableKwArgsReturns) {
  HandleScope scope(thread_);
  Function callee(
      &scope,
      functionFromModuleMethodDef(
          thread_, "foo", reinterpret_cast<void*>(returnsKwargCalledFoo), "",
          ExtensionMethodType::kMethVarArgsAndKeywords));

  // Set up a code object that calls with (*(10), **{"foo":1111})
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_->newTuple(3));
  consts.atPut(0, *callee);
  Tuple arg_tuple(&scope, runtime_->newTuple(1));
  arg_tuple.atPut(0, SmallInt::fromWord(10));
  consts.atPut(1, *arg_tuple);
  Dict kw_dict(&scope, runtime_->newDict());
  Str name(&scope, runtime_->newStrFromCStr("foo"));
  Object value(&scope, SmallInt::fromWord(1111));
  dictAtPutByStr(thread_, kw_dict, name, value);
  consts.atPut(2, *kw_dict);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST,       0, LOAD_CONST,   1, LOAD_CONST, 2,
                           CALL_FUNCTION_EX, 1, RETURN_VALUE, 0};
  code.setCode(runtime_->newBytesWithAll(bytecode));
  code.setStacksize(3);

  // Execute the code and make sure we get back the result we expect
  RawObject result = runCode(code);
  EXPECT_TRUE(isIntEqualsWord(result, 1111));
}

static RawObject numArgs(Thread*, Frame*, word nargs) {
  return SmallInt::fromWord(nargs);
}

static void createAndPatchBuiltinNumArgs(Thread* thread, Runtime* runtime) {
  // Ensure we have a __main__ module.
  ASSERT_FALSE(runFromCStr(runtime, "").isError());
  HandleScope scope;
  Module main(&scope, findMainModule(runtime));
  runtime->moduleAddBuiltinFunction(thread, main, SymbolId::kDummy, numArgs);
  ASSERT_FALSE(runFromCStr(runtime, R"(
@_patch
def dummy(first, second):
  pass
)")
                   .isError());
}

TEST_F(TrampolinesTest, BuiltinTrampolineExReceivesExArgs) {
  createAndPatchBuiltinNumArgs(thread_, runtime_);
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, "result = dummy(*(1,2))").isError());
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 2));
}

TEST_F(TrampolinesTest, BuiltinTrampolineExReceivesMixOfPositionalAndExArgs1) {
  createAndPatchBuiltinNumArgs(thread_, runtime_);
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, "result = dummy(1, *(2,))").isError());
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 2));
}

static void createAndPatchBuiltinNumArgsVariadic(Thread* thread,
                                                 Runtime* runtime) {
  // Ensure we have a __main__ module.
  ASSERT_FALSE(runFromCStr(runtime, "").isError());
  HandleScope scope;
  Module main(&scope, findMainModule(runtime));
  runtime->moduleAddBuiltinFunction(thread, main, SymbolId::kDummy, numArgs);
  ASSERT_FALSE(runFromCStr(runtime, R"(
@_patch
def dummy(*args):
  pass
)")
                   .isError());
}

TEST_F(TrampolinesTest,
       BuiltinTrampolineExReceivesOnePositionalArgAndTwoVariableArgs) {
  createAndPatchBuiltinNumArgsVariadic(thread_, runtime_);
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, "result = dummy(1, *(2, 3))").isError());
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 1));
}

static void createAndPatchBuiltinNumArgsArgsKwargs(Thread* thread,
                                                   Runtime* runtime) {
  // Ensure we have a __main__ module.
  ASSERT_FALSE(runFromCStr(runtime, "").isError());
  HandleScope scope;
  Module main(&scope, findMainModule(runtime));
  runtime->moduleAddBuiltinFunction(thread, main, SymbolId::kDummy, numArgs);
  ASSERT_FALSE(runFromCStr(runtime, R"(
@_patch
def dummy(*args, **kwargs):
  pass
)")
                   .isError());
}

TEST_F(TrampolinesTest,
       BuiltinTrampolineExReceivesTwoPositionalOneVariableAndTwoKwArgs) {
  createAndPatchBuiltinNumArgsArgsKwargs(thread_, runtime_);
  HandleScope scope(thread_);
  ASSERT_FALSE(
      runFromCStr(runtime_,
                  "result = dummy(1, 2, *(3,), **{'foo': 1, 'bar': 2})")
          .isError());
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 2));
}

TEST_F(TrampolinesTest, BuiltinTrampolineExReceivesVarArgs) {
  createAndPatchBuiltinNumArgs(thread_, runtime_);
  HandleScope scope(thread_);
  ASSERT_FALSE(
      runFromCStr(runtime_, "result = dummy(*(1,), second=5)").isError());
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 2));
}

TEST_F(TrampolinesTest, BuiltinTrampolineExWithTooFewArgsRaisesTypeError) {
  createAndPatchBuiltinNumArgs(thread_, runtime_);
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(runtime_, "dummy(*(1,))"), LayoutId::kTypeError,
                    "'dummy' takes min 2 positional arguments but 1 given"));
}

TEST_F(TrampolinesTest, BuiltinTrampolineExWithTooManyArgsRaisesTypeError) {
  createAndPatchBuiltinNumArgs(thread_, runtime_);
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(runtime_, "dummy(*(1,2,3,4,5))"), LayoutId::kTypeError,
      "'dummy' takes max 2 positional arguments but 5 given"));
}

TEST_F(TrampolinesTest, CallFunctionExWithNamedArgAndExplodeKwargs) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def f(description, conflict_handler):
    return [description, conflict_handler]

result = f(description="foo", **{"conflict_handler": "conflict_handler value"})
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {"foo", "conflict_handler value"});
}

TEST_F(TrampolinesTest, CallFunctionExWithExplodeKwargsStrSubclassAlwaysEq) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C(str):
    def __eq__(self, other):
        return True
    __hash__ = str.__hash__

def f(param):
    return param

actual = C("foo")
result = f(**{actual: 5})
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result"), 5));
}

TEST_F(TrampolinesTest,
       CallFunctionExWithExplodeKwargsStrSubclassReturnNonBool) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
class D:
    def __bool__(self):
        raise UserWarning('foo')

class C(str):
    def __eq__(self, other):
        return D()
    __hash__ = str.__hash__

def f(param):
    return param

actual = C("foo")
result = f(**{actual: 5})
)"),
                            LayoutId::kUserWarning, "foo"));
}

TEST_F(TrampolinesTest,
       CallFunctionExWithExplodeKwargsStrSubclassRaiseException) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
class C(str):
    def __eq__(self, other):
        raise UserWarning('foo')
    __hash__ = str.__hash__

def f(param):
    return param

actual = C("foo")
result = f(**{actual: 5})
)"),
                            LayoutId::kUserWarning, "foo"));
}

TEST_F(TrampolinesTest, CallFunctionExWithExplodeKwargsStrSubclassNotEq) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
class C(str):
    __hash__ = str.__hash__

def f(param):
    return param

actual = C("foo")
result = f(**{actual: 5})
)"),
                            LayoutId::kTypeError,
                            "f() got an unexpected keyword argument 'foo'"));
}

TEST_F(TrampolinesTest, CallFunctionExWithExplodeKwargsStrSubclassNeverEq) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
class C(str):
    def __eq__(self, other):
        return False
    __hash__ = str.__hash__

def f(param):
    return param

actual = C("param")
result = f(**{actual: 5})
)"),
                            LayoutId::kTypeError,
                            "f() got an unexpected keyword argument 'param'"));
}

TEST_F(TrampolinesTest, CallFunctionWithParameterInVarnames) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
def add_argument(*args, **kwargs):
    action = action_class(**kwargs)

def init():
    add_argument(action='help')

init()
)"),
                            LayoutId::kNameError,
                            "name 'action_class' is not defined"));
}

TEST_F(TrampolinesTest, CallFunctionWithParameterInVarargname) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def test(*args, **kwargs):
    return kwargs['args']

result = test(args=5)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result"), 5));
}

TEST_F(TrampolinesTest, CallFunctionWithPositionalArgAndParameterInVarargname) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
def test(pos, *args, **kwargs):
    return kwargs['args']

result = test(1, args=5)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(runtime_, "result"), 5));
}

static PyObject* capiFunctionNoArgs(PyObject* self, PyObject* args) {
  Thread* thread = Thread::current();
  thread->runtime()->collectGarbage();
  EXPECT_TRUE(ApiHandle::hasExtensionReference(self));
  EXPECT_EQ(args, nullptr);
  return ApiHandle::newReference(thread, SmallInt::fromWord(1234));
}

TEST_F(TrampolinesTest, MethodTrampolineNoArgs) {
  HandleScope scope(thread_);
  Frame* frame = thread_->currentFrame();
  Function function(&scope, newEmptyFunction());
  function.setCode(
      runtime_->newIntFromCPtr(reinterpret_cast<void*>(capiFunctionNoArgs)));
  frame->pushValue(*function);
  Object self(&scope, runtime_->newTuple(1));
  frame->pushValue(*self);
  Object result(&scope, methodTrampolineNoArgs(thread_, frame, 1));
  EXPECT_TRUE(isIntEqualsWord(*result, 1234));
}

TEST_F(TrampolinesTest, MethodTrampolineNoArgsKw) {
  HandleScope scope(thread_);
  Frame* frame = thread_->currentFrame();
  Function function(&scope, newEmptyFunction());
  function.setCode(
      runtime_->newIntFromCPtr(reinterpret_cast<void*>(capiFunctionNoArgs)));
  frame->pushValue(*function);
  Object self(&scope, runtime_->newTuple(1));
  frame->pushValue(*self);
  Object result(&scope, methodTrampolineNoArgsKw(thread_, frame, 0));
  EXPECT_TRUE(isIntEqualsWord(*result, 1234));
}

TEST_F(TrampolinesTest, MethodTrampolineNoArgsEx) {
  HandleScope scope(thread_);
  Frame* frame = thread_->currentFrame();
  Function function(&scope, newEmptyFunction());
  function.setCode(
      runtime_->newIntFromCPtr(reinterpret_cast<void*>(capiFunctionNoArgs)));
  frame->pushValue(*function);
  Tuple varargs(&scope, runtime_->newTuple(1));
  varargs.atPut(0, runtime_->newTuple(1));  // self
  frame->pushValue(*varargs);
  Object result(&scope, methodTrampolineNoArgsEx(thread_, frame, 0));
  EXPECT_TRUE(isIntEqualsWord(*result, 1234));
}

static PyObject* capiFunctionOneArg(PyObject* self, PyObject* args) {
  Thread* thread = Thread::current();
  thread->runtime()->collectGarbage();
  EXPECT_TRUE(ApiHandle::hasExtensionReference(self));
  EXPECT_TRUE(ApiHandle::hasExtensionReference(args));
  return ApiHandle::newReference(thread, SmallInt::fromWord(1234));
}

TEST_F(TrampolinesTest, MethodTrampolineOneArg) {
  HandleScope scope(thread_);
  Frame* frame = thread_->currentFrame();
  Function function(&scope, newEmptyFunction());
  function.setCode(
      runtime_->newIntFromCPtr(reinterpret_cast<void*>(capiFunctionOneArg)));
  frame->pushValue(*function);
  Object self(&scope, runtime_->newTuple(1));
  frame->pushValue(*self);
  Object args(&scope, runtime_->newTuple(1));
  frame->pushValue(*args);
  Object result(&scope, methodTrampolineOneArg(thread_, frame, 2));
  EXPECT_TRUE(isIntEqualsWord(*result, 1234));
}

TEST_F(TrampolinesTest, MethodTrampolineOneArgKw) {
  HandleScope scope(thread_);
  Frame* frame = thread_->currentFrame();
  Function function(&scope, newEmptyFunction());
  function.setCode(
      runtime_->newIntFromCPtr(reinterpret_cast<void*>(capiFunctionOneArg)));
  frame->pushValue(*function);
  Object arg(&scope, runtime_->newTuple(1));
  frame->pushValue(*arg);
  Object self(&scope, runtime_->newTuple(1));
  frame->pushValue(*self);
  Tuple kwargs(&scope, runtime_->newTuple(0));
  frame->pushValue(*kwargs);
  Object result(&scope, methodTrampolineOneArgKw(thread_, frame, 2));
  EXPECT_TRUE(isIntEqualsWord(*result, 1234));
}

TEST_F(TrampolinesTest, MethodTrampolineOneArgEx) {
  HandleScope scope(thread_);
  Frame* frame = thread_->currentFrame();
  Function function(&scope, newEmptyFunction());
  function.setCode(
      runtime_->newIntFromCPtr(reinterpret_cast<void*>(capiFunctionOneArg)));
  frame->pushValue(*function);
  Tuple varargs(&scope, runtime_->newTuple(2));
  varargs.atPut(0, runtime_->newTuple(1));  // self
  varargs.atPut(1, runtime_->newTuple(1));  // arg
  frame->pushValue(*varargs);
  Object result(&scope, methodTrampolineOneArgEx(thread_, frame, 0));
  EXPECT_TRUE(isIntEqualsWord(*result, 1234));
}

static PyObject* capiFunctionVarArgs(PyObject* self, PyObject* args) {
  Thread* thread = Thread::current();
  thread->runtime()->collectGarbage();
  EXPECT_TRUE(ApiHandle::hasExtensionReference(self));
  EXPECT_TRUE(ApiHandle::hasExtensionReference(args));
  return ApiHandle::newReference(thread, SmallInt::fromWord(1234));
}

TEST_F(TrampolinesTest, MethodTrampolineVarArgs) {
  HandleScope scope(thread_);
  Frame* frame = thread_->currentFrame();
  Function function(&scope, newEmptyFunction());
  function.setCode(
      runtime_->newIntFromCPtr(reinterpret_cast<void*>(capiFunctionVarArgs)));
  frame->pushValue(*function);
  Object self(&scope, runtime_->newTuple(1));
  frame->pushValue(*self);
  Object arg(&scope, runtime_->newTuple(1));
  frame->pushValue(*arg);
  Object result(&scope, methodTrampolineVarArgs(thread_, frame, 2));
  EXPECT_TRUE(isIntEqualsWord(*result, 1234));
}

TEST_F(TrampolinesTest, MethodTrampolineVarArgsKw) {
  HandleScope scope(thread_);
  Frame* frame = thread_->currentFrame();
  Function function(&scope, newEmptyFunction());
  function.setCode(
      runtime_->newIntFromCPtr(reinterpret_cast<void*>(capiFunctionVarArgs)));
  frame->pushValue(*function);
  Object self(&scope, runtime_->newTuple(1));
  frame->pushValue(*self);
  Object arg0(&scope, runtime_->newTuple(1));
  frame->pushValue(*arg0);
  Object arg1(&scope, runtime_->newTuple(1));
  frame->pushValue(*arg1);
  Object kwargs(&scope, runtime_->newTuple(0));
  frame->pushValue(*kwargs);
  Object result(&scope, methodTrampolineVarArgsKw(thread_, frame, 3));
  EXPECT_TRUE(isIntEqualsWord(*result, 1234));
}

TEST_F(TrampolinesTest, MethodTrampolineVarArgsEx) {
  HandleScope scope(thread_);
  Frame* frame = thread_->currentFrame();
  Function function(&scope, newEmptyFunction());
  function.setCode(
      runtime_->newIntFromCPtr(reinterpret_cast<void*>(capiFunctionVarArgs)));
  frame->pushValue(*function);
  Tuple varargs(&scope, runtime_->newTuple(1));
  Object self(&scope, runtime_->newTuple(1));
  varargs.atPut(0, *self);
  frame->pushValue(*varargs);
  Object result(&scope, methodTrampolineVarArgsEx(thread_, frame, 0));
  EXPECT_TRUE(isIntEqualsWord(*result, 1234));
}

static PyObject* capiFunctionKeywordsNullKwargs(PyObject* self, PyObject* args,
                                                PyObject* kwargs) {
  Thread* thread = Thread::current();
  thread->runtime()->collectGarbage();
  EXPECT_TRUE(ApiHandle::hasExtensionReference(self));
  EXPECT_TRUE(ApiHandle::hasExtensionReference(args));
  EXPECT_EQ(kwargs, nullptr);
  return ApiHandle::newReference(thread, SmallInt::fromWord(1234));
}

TEST_F(TrampolinesTest, MethodTrampolineKeywords) {
  HandleScope scope(thread_);
  Frame* frame = thread_->currentFrame();
  Function function(&scope, newEmptyFunction());
  function.setCode(runtime_->newIntFromCPtr(
      reinterpret_cast<void*>(capiFunctionKeywordsNullKwargs)));
  frame->pushValue(*function);
  Object self(&scope, runtime_->newTuple(1));
  frame->pushValue(*self);
  Object result(&scope, methodTrampolineKeywords(thread_, frame, 1));
  EXPECT_TRUE(isIntEqualsWord(*result, 1234));
}

static PyObject* capiFunctionKeywords(PyObject* self, PyObject* args,
                                      PyObject* kwargs) {
  Thread* thread = Thread::current();
  thread->runtime()->collectGarbage();
  EXPECT_TRUE(ApiHandle::hasExtensionReference(self));
  EXPECT_TRUE(ApiHandle::hasExtensionReference(args));
  EXPECT_TRUE(ApiHandle::hasExtensionReference(kwargs));
  return ApiHandle::newReference(thread, SmallInt::fromWord(1234));
}

TEST_F(TrampolinesTest, MethodTrampolineKeywordsKw) {
  HandleScope scope(thread_);
  Frame* frame = thread_->currentFrame();
  Function function(&scope, newEmptyFunction());
  function.setCode(
      runtime_->newIntFromCPtr(reinterpret_cast<void*>(capiFunctionKeywords)));
  frame->pushValue(*function);
  Object self(&scope, runtime_->newTuple(1));
  frame->pushValue(*self);
  frame->pushValue(SmallStr::fromCStr("bar"));
  Tuple kwnames(&scope, runtime_->newTuple(1));
  kwnames.atPut(0, SmallStr::fromCStr("foo"));
  frame->pushValue(*kwnames);
  Object result(&scope, methodTrampolineKeywordsKw(thread_, frame, 2));
  EXPECT_TRUE(isIntEqualsWord(*result, 1234));
}

TEST_F(TrampolinesTest, MethodTrampolineKeywordsEx) {
  HandleScope scope(thread_);
  Frame* frame = thread_->currentFrame();
  Function function(&scope, newEmptyFunction());
  function.setCode(runtime_->newIntFromCPtr(
      reinterpret_cast<void*>(capiFunctionKeywordsNullKwargs)));
  frame->pushValue(*function);
  Tuple varargs(&scope, runtime_->newTuple(1));
  Object self(&scope, runtime_->newTuple(1));
  varargs.atPut(0, *self);
  frame->pushValue(*varargs);
  Object result(&scope, methodTrampolineKeywordsEx(thread_, frame, 0));
  EXPECT_TRUE(isIntEqualsWord(*result, 1234));
}

static PyObject* capiFunctionFastCallNullKwnames(PyObject* self,
                                                 PyObject** args, word nargs,
                                                 PyObject* kwnames) {
  Thread* thread = Thread::current();
  thread->runtime()->collectGarbage();
  EXPECT_TRUE(ApiHandle::hasExtensionReference(self));
  for (word i = 0; i < nargs; i++) {
    EXPECT_TRUE(ApiHandle::hasExtensionReference(args[i]))
        << "Expected fastcall arg #" << i << " to be owned by the trampoline";
  }
  EXPECT_EQ(kwnames, nullptr);
  return ApiHandle::newReference(thread, SmallInt::fromWord(1234));
}

TEST_F(TrampolinesTest, MethodTrampolineFastCall) {
  HandleScope scope(thread_);
  Frame* frame = thread_->currentFrame();
  Function function(&scope, newEmptyFunction());
  function.setCode(runtime_->newIntFromCPtr(
      reinterpret_cast<void*>(capiFunctionFastCallNullKwnames)));
  frame->pushValue(*function);
  Object self(&scope, runtime_->newTuple(1));
  frame->pushValue(*self);
  Object arg0(&scope, runtime_->newTuple(1));
  frame->pushValue(*arg0);
  Object result(&scope, methodTrampolineFastCall(thread_, frame, 2));
  EXPECT_TRUE(isIntEqualsWord(*result, 1234));
}

static PyObject* capiFunctionFastCall(PyObject* self, PyObject** args,
                                      word nargs, PyObject* kwnames) {
  Thread* thread = Thread::current();
  thread->runtime()->collectGarbage();
  EXPECT_TRUE(ApiHandle::hasExtensionReference(self));
  word num_keywords =
      Tuple::cast(ApiHandle::fromPyObject(kwnames)->asObject()).length();
  for (word i = 0; i < nargs + num_keywords; i++) {
    EXPECT_TRUE(ApiHandle::hasExtensionReference(args[i]))
        << "Expected fastcall arg #" << i << " to be owned by the trampoline";
  }
  EXPECT_TRUE(ApiHandle::hasExtensionReference(kwnames));
  return ApiHandle::newReference(thread, SmallInt::fromWord(1234));
}

TEST_F(TrampolinesTest, MethodTrampolineFastCallKw) {
  HandleScope scope(thread_);
  Frame* frame = thread_->currentFrame();
  Function function(&scope, newEmptyFunction());
  function.setCode(
      runtime_->newIntFromCPtr(reinterpret_cast<void*>(capiFunctionFastCall)));
  frame->pushValue(*function);
  Object self(&scope, runtime_->newTuple(1));
  frame->pushValue(*self);
  Object kwarg0(&scope, runtime_->newTuple(1));
  frame->pushValue(*kwarg0);
  Tuple kwnames(&scope, runtime_->newTuple(1));
  kwnames.atPut(0, SmallStr::fromCStr("foo"));
  frame->pushValue(*kwnames);
  Object result(&scope, methodTrampolineFastCallKw(thread_, frame, 2));
  EXPECT_TRUE(isIntEqualsWord(*result, 1234));
}

TEST_F(TrampolinesTest, MethodTrampolineFastCallEx) {
  HandleScope scope(thread_);
  Frame* frame = thread_->currentFrame();
  Function function(&scope, newEmptyFunction());
  function.setCode(
      runtime_->newIntFromCPtr(reinterpret_cast<void*>(capiFunctionFastCall)));
  frame->pushValue(*function);
  Tuple varargs(&scope, runtime_->newTuple(2));
  Object self(&scope, runtime_->newTuple(1));
  varargs.atPut(0, *self);
  varargs.atPut(1, SmallStr::fromCStr("bar"));
  frame->pushValue(*varargs);
  Dict varkeywords(&scope, runtime_->newDict());
  Str key(&scope, SmallStr::fromCStr("baz"));
  Object value(&scope, runtime_->newTuple(1));
  dictAtPutByStr(thread_, varkeywords, key, value);
  frame->pushValue(*varkeywords);
  Object result(&scope, methodTrampolineFastCallEx(
                            thread_, frame, CallFunctionExFlag::VAR_KEYWORDS));
  EXPECT_TRUE(isIntEqualsWord(*result, 1234));
}

TEST_F(TrampolinesTest, ModuleTrampolineFastCall) {
  HandleScope scope(thread_);
  Frame* frame = thread_->currentFrame();
  Function function(&scope, newEmptyFunction());
  function.setCode(runtime_->newIntFromCPtr(
      reinterpret_cast<void*>(capiFunctionFastCallNullKwnames)));
  frame->pushValue(*function);
  Object arg0(&scope, runtime_->newTuple(1));
  frame->pushValue(*arg0);
  Object result(&scope, moduleTrampolineFastCall(thread_, frame, 1));
  EXPECT_TRUE(isIntEqualsWord(*result, 1234));
}

TEST_F(TrampolinesTest, ModuleTrampolineFastCallKw) {
  HandleScope scope(thread_);
  Frame* frame = thread_->currentFrame();
  Function function(&scope, newEmptyFunction());
  function.setCode(
      runtime_->newIntFromCPtr(reinterpret_cast<void*>(capiFunctionFastCall)));
  frame->pushValue(*function);
  Object arg0(&scope, runtime_->newTuple(1));
  frame->pushValue(*arg0);
  Tuple kwnames(&scope, runtime_->newTuple(1));
  kwnames.atPut(0, SmallStr::fromCStr("foo"));
  frame->pushValue(*kwnames);
  Object result(&scope, moduleTrampolineFastCallKw(thread_, frame, 1));
  EXPECT_TRUE(isIntEqualsWord(*result, 1234));
}

TEST_F(TrampolinesTest, ModuleTrampolineFastCallEx) {
  HandleScope scope(thread_);
  Frame* frame = thread_->currentFrame();
  Function function(&scope, newEmptyFunction());
  function.setCode(
      runtime_->newIntFromCPtr(reinterpret_cast<void*>(capiFunctionFastCall)));
  frame->pushValue(*function);
  Tuple varargs(&scope, runtime_->newTuple(1));
  varargs.atPut(0, SmallStr::fromCStr("bar"));
  frame->pushValue(*varargs);
  Dict varkeywords(&scope, runtime_->newDict());
  Str key(&scope, SmallStr::fromCStr("baz"));
  Object value(&scope, runtime_->newTuple(1));
  dictAtPutByStr(thread_, varkeywords, key, value);
  frame->pushValue(*varkeywords);
  Object result(&scope, moduleTrampolineFastCallEx(
                            thread_, frame, CallFunctionExFlag::VAR_KEYWORDS));
  EXPECT_TRUE(isIntEqualsWord(*result, 1234));
}

}  // namespace py
