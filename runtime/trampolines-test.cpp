#include "gtest/gtest.h"

#include <memory>

#include "frame.h"
#include "function-builtins.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {
using namespace testing;

using CallTest = RuntimeFixture;
using TrampolinesTest = RuntimeFixture;
using TrampolinesDeathTest = RuntimeFixture;

TEST_F(CallTest, CallBoundMethod) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def func(self):
  return self

def test(callable):
  return callable()
)")
                   .isError());

  Object function(&scope, mainModuleAt(&runtime_, "func"));
  ASSERT_TRUE(function.isFunction());

  Object self(&scope, SmallInt::fromWord(1111));
  BoundMethod method(&scope, runtime_.newBoundMethod(function, self));

  Object test(&scope, mainModuleAt(&runtime_, "test"));
  ASSERT_TRUE(test.isFunction());
  Function func(&scope, *test);

  Object result(&scope, Interpreter::callFunction1(
                            thread_, thread_->currentFrame(), func, method));
  EXPECT_TRUE(isIntEqualsWord(*result, 1111));
}

TEST_F(CallTest, CallBoundMethodWithArgs) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def func(self, a, b):
  return [self, a, b]

def test(callable):
  return callable(2222, 3333)
)")
                   .isError());

  Object function(&scope, mainModuleAt(&runtime_, "func"));
  ASSERT_TRUE(function.isFunction());

  Object self(&scope, SmallInt::fromWord(1111));
  BoundMethod method(&scope, runtime_.newBoundMethod(function, self));

  Object test(&scope, mainModuleAt(&runtime_, "test"));
  ASSERT_TRUE(test.isFunction());
  Function func(&scope, *test);

  Object result(&scope, Interpreter::callFunction1(
                            thread_, thread_->currentFrame(), func, method));
  EXPECT_PYLIST_EQ(result, {1111, 2222, 3333});
}

TEST_F(CallTest, CallBoundMethodKw) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
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

  Object function(&scope, mainModuleAt(&runtime_, "func"));
  ASSERT_TRUE(function.isFunction());

  Object self(&scope, SmallInt::fromWord(1111));
  BoundMethod method(&scope, runtime_.newBoundMethod(function, self));

  Object test(&scope, mainModuleAt(&runtime_, "test"));
  ASSERT_TRUE(test.isFunction());
  Function func(&scope, *test);
  Tuple args(&scope, runtime_.newTuple(1));
  args.atPut(0, *method);
  callFunction(func, args);

  Object result_self(&scope, mainModuleAt(&runtime_, "result_self"));
  EXPECT_TRUE(isIntEqualsWord(*result_self, 1111));

  Object result_a(&scope, mainModuleAt(&runtime_, "result_a"));
  EXPECT_TRUE(isIntEqualsWord(*result_a, 2222));

  Object result_b(&scope, mainModuleAt(&runtime_, "result_b"));
  EXPECT_TRUE(isIntEqualsWord(*result_b, 3333));
}

TEST_F(CallTest, CallBoundMethodExArgs) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
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

  Object function(&scope, mainModuleAt(&runtime_, "func"));
  ASSERT_TRUE(function.isFunction());

  Object self(&scope, SmallInt::fromWord(1111));
  BoundMethod method(&scope, runtime_.newBoundMethod(function, self));

  Object test(&scope, mainModuleAt(&runtime_, "test"));
  ASSERT_TRUE(test.isFunction());
  Function func(&scope, *test);
  Tuple args(&scope, runtime_.newTuple(1));
  args.atPut(0, *method);
  callFunction(func, args);

  Object result_self(&scope, mainModuleAt(&runtime_, "result_self"));
  EXPECT_TRUE(isIntEqualsWord(*result_self, 1111));

  Object result_a(&scope, mainModuleAt(&runtime_, "result_a"));
  EXPECT_TRUE(isIntEqualsWord(*result_a, 2222));

  Object result_b(&scope, mainModuleAt(&runtime_, "result_b"));
  EXPECT_TRUE(isIntEqualsWord(*result_b, 3333));
}

TEST_F(CallTest, CallBoundMethodExKwargs) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
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

  Object function(&scope, mainModuleAt(&runtime_, "func"));
  ASSERT_TRUE(function.isFunction());

  Object self(&scope, SmallInt::fromWord(1111));
  BoundMethod method(&scope, runtime_.newBoundMethod(function, self));

  Object test(&scope, mainModuleAt(&runtime_, "test"));
  ASSERT_TRUE(test.isFunction());
  Function func(&scope, *test);
  Tuple args(&scope, runtime_.newTuple(1));
  args.atPut(0, *method);
  callFunction(func, args);

  Object result_self(&scope, mainModuleAt(&runtime_, "result_self"));
  EXPECT_TRUE(isIntEqualsWord(*result_self, 1111));

  Object result_a(&scope, mainModuleAt(&runtime_, "result_a"));
  EXPECT_TRUE(isIntEqualsWord(*result_a, 2222));

  Object result_b(&scope, mainModuleAt(&runtime_, "result_b"));
  EXPECT_TRUE(isIntEqualsWord(*result_b, 3333));
}

TEST_F(CallTest, CallBoundMethodExArgsAndKwargs) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
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

  Object function(&scope, mainModuleAt(&runtime_, "func"));
  ASSERT_TRUE(function.isFunction());

  Object self(&scope, SmallInt::fromWord(1111));
  BoundMethod method(&scope, runtime_.newBoundMethod(function, self));

  Object test(&scope, mainModuleAt(&runtime_, "test"));
  ASSERT_TRUE(test.isFunction());
  Function func(&scope, *test);
  Tuple args(&scope, runtime_.newTuple(1));
  args.atPut(0, *method);
  callFunction(func, args);

  Object result_self(&scope, mainModuleAt(&runtime_, "result_self"));
  EXPECT_TRUE(isIntEqualsWord(*result_self, 1111));

  Object result_a(&scope, mainModuleAt(&runtime_, "result_a"));
  EXPECT_TRUE(isIntEqualsWord(*result_a, 2222));

  Object result_b(&scope, mainModuleAt(&runtime_, "result_b"));
  EXPECT_TRUE(isIntEqualsWord(*result_b, 3333));
}

TEST_F(CallTest, CallDefaultArgs) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
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
  Object result0(&scope, mainModuleAt(&runtime_, "result0"));
  EXPECT_PYLIST_EQ(result0, {33, 22, 11});
  Object result1(&scope, mainModuleAt(&runtime_, "result1"));
  EXPECT_PYLIST_EQ(result1, {1, 2, 3});
  Object result2(&scope, mainModuleAt(&runtime_, "result2"));
  EXPECT_PYLIST_EQ(result2, {1001, 2, 3});
  Object result3(&scope, mainModuleAt(&runtime_, "result3"));
  EXPECT_PYLIST_EQ(result3, {1001, 1002, 3});
  Object result4(&scope, mainModuleAt(&runtime_, "result4"));
  EXPECT_PYLIST_EQ(result4, {1001, 1002, 1003});
}

TEST_F(CallTest, CallMethodMixPosDefaultArgs) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def foo(a, b=2):
  return [a, b]
result = foo(1)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {1, 2});
}

TEST_F(CallTest, CallBoundMethodMixed) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class R:
  def m(self, a, b=2):
    return [a, b]
r = R()
result = r.m(9)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {9, 2});
}

TEST_F(CallTest, SingleKW) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def foo(bar):
   return bar
result = foo(bar=2)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(&runtime_, "result"), 2));
}

TEST_F(CallTest, MixedKW) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def foo(a, b, c):
   return [a, b, c]
result = foo(1, b = 2, c = 3)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {1, 2, 3});
}

TEST_F(CallTest, FullKW) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def foo(a, b, c):
   return [a, b, c]
result = foo(a = 1, b = 2, c = 3)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {1, 2, 3});
}

TEST_F(CallTest, KWOutOfOrder1) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def foo(a, b, c):
   return [a, b, c]
result = foo(c = 3, a = 1, b = 2)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {1, 2, 3});
}

TEST_F(CallTest, KWOutOfOrder2) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def foo(a, b, c):
   return [a, b, c]
result = foo(1, c = 3, b = 2)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {1, 2, 3});
}

TEST_F(CallTest, KeywordOnly1) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def foo(a,b, *, c):
  return [a,b,c]
result = foo(1, 2, c = 3)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {1, 2, 3});
}

TEST_F(CallTest, KeywordOnly2) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def foo(a,b, *, c):
  return [a,b,c]
result = foo(1, b = 2, c = 3)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {1, 2, 3});
}

TEST_F(CallTest, KeyWordDefaults) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def foo(a, b = 22, c = 33):
  return [a,b,c]
result = foo(11, c = 3)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {11, 22, 3});
}

TEST_F(CallTest, VarArgsWithExcess) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def foo(a, b, *c):
  return [a,b,c]
result = foo(1,2,3,4,5,6)
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, mainModuleAt(&runtime_, "result"));
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
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def foo(a, b, *c):
  return [a,b,c]
result = foo(1,2)
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 2));
  Tuple tuple(&scope, result.at(2));
  EXPECT_EQ(tuple.length(), 0);
}

TEST_F(CallTest, CallWithKeywordsCalleeWithVarkeyword) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def foo(a,b,c,**d):
    return [a,b,c,d]
result = foo(1,2,c=3,g=4,h=5,j="bar")
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, testing::mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 2));
  EXPECT_TRUE(isIntEqualsWord(result.at(2), 3));

  Dict dict(&scope, result.at(3));
  Object key_g(&scope, runtime_.newStrFromCStr("g"));
  EXPECT_TRUE(isIntEqualsWord(runtime_.dictAt(thread_, dict, key_g), 4));
  Object key_h(&scope, runtime_.newStrFromCStr("h"));
  EXPECT_TRUE(isIntEqualsWord(runtime_.dictAt(thread_, dict, key_h), 5));
  Object key_j(&scope, runtime_.newStrFromCStr("j"));
  EXPECT_TRUE(isStrEqualsCStr(runtime_.dictAt(thread_, dict, key_j), "bar"));
}

TEST_F(CallTest, CallWithNoArgsCalleeDefaultArgsVarargsVarkeyargs) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def bar(a=1, b=2, *c, **d):
    return [a,b,c,d]
result = bar()
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, testing::mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 2));
  Tuple tuple(&scope, result.at(2));
  EXPECT_EQ(tuple.length(), 0);
  Dict dict(&scope, result.at(3));
  EXPECT_EQ(dict.numItems(), 0);
}

TEST_F(CallTest, CallPositionalCalleeVargsEmptyVarkeyargs) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def bar(a=1, b=2, *c, **d):
    return [a,b,c,d]
result = bar(1,2,3,4,5,6,7)
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, testing::mainModuleAt(&runtime_, "result"));
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
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def bar(a=1, b=2, *c, **d):
    return [a,b,c,d]
result = bar(a1=11, a2=12, a3=13)
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, testing::mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 2));

  Object tuple_obj(&scope, result.at(2));
  ASSERT_TRUE(tuple_obj.isTuple());
  Tuple tuple(&scope, *tuple_obj);
  EXPECT_EQ(tuple.length(), 0);

  Dict dict(&scope, result.at(3));
  Object key0(&scope, runtime_.newStrFromCStr("a3"));
  EXPECT_TRUE(isIntEqualsWord(runtime_.dictAt(thread_, dict, key0), 13));
  Object key1(&scope, runtime_.newStrFromCStr("a1"));
  EXPECT_TRUE(isIntEqualsWord(runtime_.dictAt(thread_, dict, key1), 11));
  Object key2(&scope, runtime_.newStrFromCStr("a2"));
  EXPECT_TRUE(isIntEqualsWord(runtime_.dictAt(thread_, dict, key2), 12));
}

TEST_F(CallTest, CallWithKeywordsCalleeFullVarargsFullVarkeyargs) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def bar(a=1, b=2, *c, **d):
    return [a,b,c,d]
result = bar(1,2,3,4,5,6,7,a9=9)
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, testing::mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 2));

  Tuple tuple(&scope, result.at(2));
  EXPECT_TRUE(isIntEqualsWord(tuple.at(0), 3));
  EXPECT_TRUE(isIntEqualsWord(tuple.at(1), 4));
  EXPECT_TRUE(isIntEqualsWord(tuple.at(2), 5));
  EXPECT_TRUE(isIntEqualsWord(tuple.at(3), 6));
  EXPECT_TRUE(isIntEqualsWord(tuple.at(4), 7));

  Dict dict(&scope, result.at(3));
  Object key_g(&scope, runtime_.newStrFromCStr("a9"));
  EXPECT_TRUE(isIntEqualsWord(runtime_.dictAt(thread_, dict, key_g), 9));
}

TEST_F(CallTest, CallWithOutOfOrderKeywords) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def foobar(a,b,*,c):
    return [a,b,c]
result = foobar(c=3,a=1,b=2)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {1, 2, 3});
}

TEST_F(CallTest, CallWithKeywordsCalleeVarargsKeywordOnly) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def foobar1(a,b,*c,d):
    return [a,b,c,d]
result = foobar1(1,2,3,4,5,d=9)
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, mainModuleAt(&runtime_, "result"));
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
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def foobar2(a,b,*c, e, **d):
    return [a,b,c,d,e]
result = foobar2(1,e=9,b=2,f1="a",f11=12)
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 2));
  Tuple tuple(&scope, result.at(2));
  ASSERT_EQ(tuple.length(), 0);
  Dict dict(&scope, result.at(3));
  Object f1(&scope, runtime_.newStrFromCStr("f1"));
  EXPECT_TRUE(isStrEqualsCStr(runtime_.dictAt(thread_, dict, f1), "a"));
  Object f11(&scope, runtime_.newStrFromCStr("f11"));
  EXPECT_TRUE(isIntEqualsWord(runtime_.dictAt(thread_, dict, f11), 12));
  EXPECT_TRUE(isIntEqualsWord(result.at(4), 9));
}

TEST_F(CallTest, CallEx) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def foo(a,b,c,d):
    return [a,b,c,d]
a = (1,2,3,4)
result = foo(*a)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {1, 2, 3, 4});
}

TEST_F(CallTest, CallExBuildTupleUnpackWithCall) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def foo(a,b,c,d):
    return [a,b,c,d]
a = (3,4)
result = foo(1,2,*a)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {1, 2, 3, 4});
}

TEST_F(CallTest, CallExKw) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def foo(a,b,c,d):
    return [a,b,c,d]
a = {'d': 4, 'b': 2, 'a': 1, 'c': 3}
result = foo(**a)
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_PYLIST_EQ(result, {1, 2, 3, 4});
}

TEST_F(CallTest, KeywordOnly) {
  const char* src = R"(
def foo(a,b, *, c):
  print(a,b,c)
foo(1, 2, 3);
)";
  EXPECT_TRUE(raised(runFromCStr(&runtime_, src), LayoutId::kTypeError));
}

TEST_F(CallTest, MissingKeyword) {
  const char* src = R"(
def foo(a,b, *, c):
  print(a,b,c)
foo(1, 2);
)";
  EXPECT_TRUE(raised(runFromCStr(&runtime_, src), LayoutId::kTypeError));
}

TEST_F(CallTest, ArgNameMismatch) {
  const char* src = R"(
def foo(a,b, *, c):
  print(a,b,c)
foo(1, d = 2, c = 3);
)";
  EXPECT_TRUE(raised(runFromCStr(&runtime_, src), LayoutId::kTypeError));
}

TEST_F(CallTest, TooManyKWArgs) {
  const char* src = R"(
def foo(a,b, *, c):
  print(a,b,c)
foo(1, 2, 4, c = 3);
)";
  EXPECT_TRUE(raised(runFromCStr(&runtime_, src), LayoutId::kTypeError));
}

TEST_F(CallTest, TooManyArgs) {
  const char* src = R"(
def foo(a,b, c):
  print(a,b,c)
foo(1, 2, 3, 4);
)";
  EXPECT_TRUE(raised(runFromCStr(&runtime_, src), LayoutId::kTypeError));
}

TEST_F(CallTest, TooFewArgs) {
  const char* src = R"(
def foo(a,b, c):
  print(a,b,c)
foo(3, 4);
)";
  EXPECT_TRUE(raised(runFromCStr(&runtime_, src), LayoutId::kTypeError));
}

static RawObject builtinReturnSecondArg(Thread* /* thread */, Frame* frame,
                                        word nargs) {
  Arguments args(frame, nargs);
  return args.get(1);
}

static void createAndPatchBuiltinReturnSecondArg(Runtime* runtime) {
  HandleScope scope(Thread::current());
  // Ensure we have a __main__ module.
  ASSERT_FALSE(runFromCStr(runtime, "").isError());
  Module main(&scope, findMainModule(runtime));
  runtime->moduleAddBuiltinFunction(main, SymbolId::kDummy,
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
  createAndPatchBuiltinReturnSecondArg(&runtime_);
  ASSERT_FALSE(
      runFromCStr(&runtime_, "result = dummy(second=12345, first=None)")
          .isError());
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 12345));
}

TEST_F(TrampolinesTest, BuiltinTrampolineKwWithInvalidArgRaisesTypeError) {
  createAndPatchBuiltinReturnSecondArg(&runtime_);
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, "dummy(third=3, first=1)"),
                            LayoutId::kTypeError,
                            "TypeError: invalid keyword argument supplied"));
}

TEST_F(TrampolinesTest, InterpreterClosureUsesArgOverCellValue) {
  HandleScope scope(thread_);

  // Create code object
  word nlocals = 1;
  Tuple varnames(&scope, runtime_.newTuple(nlocals));
  Tuple cellvars(&scope, runtime_.newTuple(1));
  Str bar(&scope, runtime_.internStrFromCStr(thread_, "bar"));
  varnames.atPut(0, *bar);
  cellvars.atPut(0, *bar);
  const byte bytecode[] = {LOAD_CLOSURE, 0, LOAD_DEREF, 0, RETURN_VALUE, 0};
  Bytes bc(&scope, runtime_.newBytesWithAll(bytecode));
  Tuple empty_tuple(&scope, runtime_.emptyTuple());
  Object empty_str(&scope, Str::empty());
  Object empty_bytes(&scope, Bytes::empty());
  word flags = Code::Flags::OPTIMIZED | Code::Flags::NEWLOCALS;
  Code code(&scope,
            runtime_.newCode(/*argcount=*/1, /*posonlyargcount=*/0,
                             /*kwonlyargcount=*/0, nlocals, /*stacksize=*/0,
                             flags, /*code=*/bc, /*consts=*/empty_tuple,
                             /*names=*/empty_tuple, varnames,
                             /*freevars=*/empty_tuple, cellvars,
                             /*filename=*/empty_str, /*name=*/empty_str,
                             /*firstlineno=*/0, /*lnotab=*/empty_bytes));
  ASSERT_TRUE(!code.cell2arg().isNoneType());

  Object qualname(&scope, runtime_.newStrFromCStr("foo"));
  Dict globals(&scope, runtime_.newDict());
  Function foo(&scope,
               runtime_.newFunctionWithCode(thread_, qualname, code, globals));
  Tuple closure_tuple(&scope, runtime_.newTuple(1));
  closure_tuple.atPut(0, runtime_.newInt(99));
  foo.setClosure(*closure_tuple);

  Object argument(&scope, runtime_.newInt(3));
  EXPECT_TRUE(
      isIntEqualsWord(Interpreter::callFunction1(
                          thread_, thread_->currentFrame(), foo, argument),
                      3));
}

TEST_F(TrampolinesTest, InterpreterClosureUsesCellValue) {
  HandleScope scope(thread_);

  // Create code object
  word nlocals = 2;
  Tuple consts(&scope, runtime_.newTuple(1));
  consts.atPut(0, runtime_.newInt(10));
  Tuple varnames(&scope, runtime_.newTuple(nlocals));
  Tuple cellvars(&scope, runtime_.newTuple(2));
  Str bar(&scope, runtime_.internStrFromCStr(thread_, "bar"));
  Str baz(&scope, runtime_.internStrFromCStr(thread_, "baz"));
  Str foobar(&scope, runtime_.internStrFromCStr(thread_, "foobar"));
  varnames.atPut(0, *bar);
  varnames.atPut(1, *baz);
  cellvars.atPut(0, *foobar);
  cellvars.atPut(1, *bar);
  const byte bytecode[] = {LOAD_CONST, 0, STORE_DEREF,  0,
                           LOAD_DEREF, 0, RETURN_VALUE, 0};
  Bytes bc(&scope, runtime_.newBytesWithAll(bytecode));
  Object empty_str(&scope, Str::empty());
  Object empty_bytes(&scope, Bytes::empty());
  Tuple empty_tuple(&scope, runtime_.emptyTuple());
  Code code(&scope,
            runtime_.newCode(/*argcount=*/1, /*posonlyargcount=*/0,
                             /*kwonlyargcount=*/0, nlocals, /*stacksize=*/0,
                             /*flags=*/0, /*code=*/bc, consts,
                             /*names=*/empty_tuple, varnames,
                             /*freevars=*/empty_tuple, cellvars,
                             /*filename=*/empty_str, /*name=*/empty_str,
                             /*firstlineno=*/0, /*lnotab=*/empty_bytes));
  ASSERT_TRUE(!code.cell2arg().isNoneType());

  // Create a function
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def foo(bar): pass
)")
                   .isError());
  Function foo(&scope, mainModuleAt(&runtime_, "foo"));
  foo.setEntry(interpreterTrampoline);
  foo.setCode(*code);

  // Run function
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = foo(1)
)")
                   .isError());
  Object result(&scope, testing::mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 10));
}

static RawObject makeFunctionWithPosOnlyArg(Thread* thread) {
  // Create:
  //   def foo(a, /, b):
  //     return (a, b)
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Str name(&scope, runtime->internStrFromCStr(thread, "foo"));
  const byte bytecode[] = {LOAD_FAST,   0, LOAD_FAST,    1,
                           BUILD_TUPLE, 2, RETURN_VALUE, 0};
  Tuple varnames(&scope, runtime->newTuple(2));
  varnames.atPut(0, runtime->internStrFromCStr(thread, "a"));
  varnames.atPut(1, runtime->internStrFromCStr(thread, "b"));
  Bytes bc(&scope, runtime->newBytesWithAll(bytecode));
  Object empty_tuple(&scope, runtime->emptyTuple());
  Object empty_str(&scope, Str::empty());
  Object empty_bytes(&scope, Bytes::empty());
  Code code(&scope,
            runtime->newCode(/*argcount=*/2, /*posonlyargcount=*/1,
                             /*kwonlyargcount=*/0, /*nlocals=*/2,
                             /*stacksize=*/2,
                             Code::Flags::NEWLOCALS | Code::Flags::OPTIMIZED,
                             bc, /*consts=*/empty_tuple,
                             /*names=*/empty_tuple, varnames,
                             /*freevars=*/empty_tuple,
                             /*cellvars=*/empty_tuple,
                             /*filename=*/empty_str, name,
                             /*firstlineno=*/0, /*lnotab=*/empty_bytes));
  Dict globals(&scope, runtime->newDict());
  return runtime->newFunctionWithCode(thread, name, code, globals);
}

TEST_F(TrampolinesTest, KeywordCallRejectsPositionalOnlyArgumentNames) {
  HandleScope scope(thread_);
  Function function(&scope, makeFunctionWithPosOnlyArg(thread_));

  // `foo(a=2, b=4)`
  Frame* frame = thread_->currentFrame();
  frame->pushValue(*function);
  frame->pushValue(runtime_.newInt(2));
  frame->pushValue(runtime_.newInt(4));
  Tuple keywords(&scope, runtime_.newTuple(2));
  keywords.atPut(0, runtime_.internStrFromCStr(thread_, "a"));
  keywords.atPut(1, runtime_.internStrFromCStr(thread_, "b"));
  frame->pushValue(*keywords);
  Object result_obj(&scope, Interpreter::callKw(thread_, frame, 2));
  EXPECT_TRUE(raisedWithStr(*result_obj, LayoutId::kTypeError,
                            "TypeError: keyword argument specified for "
                            "positional-only argument 'a'"));
}

TEST_F(TrampolinesTest, KeywordCallAcceptsNonPositionalOnlyArgumentNames) {
  HandleScope scope(thread_);
  Function function(&scope, makeFunctionWithPosOnlyArg(thread_));

  // `foo(2, b=9)`
  Frame* frame = thread_->currentFrame();
  frame->pushValue(*function);
  frame->pushValue(runtime_.newInt(2));
  frame->pushValue(runtime_.newInt(9));
  Tuple keywords(&scope, runtime_.newTuple(1));
  keywords.atPut(0, runtime_.internStrFromCStr(thread_, "b"));
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
  Str name(&scope, runtime_.internStrFromCStr(thread_, "foo"));
  const byte bytecode[] = {LOAD_FAST, 0, LOAD_FAST,   1, LOAD_FAST,    2,
                           LOAD_FAST, 3, BUILD_TUPLE, 4, RETURN_VALUE, 0};
  Tuple varnames(&scope, runtime_.newTuple(4));
  varnames.atPut(0, runtime_.internStrFromCStr(thread_, "a"));
  varnames.atPut(1, runtime_.internStrFromCStr(thread_, "b"));
  varnames.atPut(2, runtime_.internStrFromCStr(thread_, "c"));
  varnames.atPut(3, runtime_.internStrFromCStr(thread_, "kwargs"));
  Bytes bc(&scope, runtime_.newBytesWithAll(bytecode));
  Object empty_tuple(&scope, runtime_.emptyTuple());
  Object empty_str(&scope, Str::empty());
  Object empty_bytes(&scope, Bytes::empty());
  Code code(&scope,
            runtime_.newCode(/*argcount=*/3, /*posonlyargcount=*/2,
                             /*kwonlyargcount=*/0, /*nlocals=*/4,
                             /*stacksize=*/4,
                             Code::Flags::NEWLOCALS | Code::Flags::OPTIMIZED |
                                 Code::Flags::VARKEYARGS,
                             bc,
                             /*consts=*/empty_tuple,
                             /*names=*/empty_tuple, varnames,
                             /*freevars=*/empty_tuple,
                             /*cellvars=*/empty_tuple,
                             /*filename=*/empty_str, name,
                             /*firstlineno=*/0, /*lnotab=*/empty_bytes));
  Dict globals(&scope, runtime_.newDict());
  Function foo(&scope,
               runtime_.newFunctionWithCode(thread_, name, code, globals));
  Tuple defaults(&scope, runtime_.newTuple(2));
  defaults.atPut(0, runtime_.newInt(7));
  defaults.atPut(1, runtime_.newInt(10));
  foo.setDefaults(*defaults);
  // Call foo(1, c=13, b=5).
  Frame* frame = thread_->currentFrame();
  frame->pushValue(*foo);
  frame->pushValue(runtime_.newInt(1));
  frame->pushValue(runtime_.newInt(13));
  frame->pushValue(runtime_.newInt(5));
  Tuple keywords(&scope, runtime_.newTuple(2));
  keywords.atPut(0, runtime_.internStrFromCStr(thread_, "c"));
  keywords.atPut(1, runtime_.internStrFromCStr(thread_, "b"));
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
  Str b_name(&scope, runtime_.internStrFromCStr(thread_, "b"));
  EXPECT_TRUE(
      isIntEqualsWord(runtime_.dictAt(thread_, result_dict, b_name), 5));
}

TEST_F(TrampolinesTest, ExplodeCallWithBadKeywordFails) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
def take_kwargs(a): pass

kwargs = {12: 34}
take_kwargs(**kwargs)
)"),
                            LayoutId::kTypeError, "keywords must be strings"));
}

TEST_F(TrampolinesTest, ExplodeCallWithZeroKeywords) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def foo(a=10): return a
result = foo(**{})
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_EQ(result, SmallInt::fromWord(10));
}

TEST_F(TrampolinesTest, ExtensionModuleNoArgReceivesNoArgsReturns) {
  binaryfunc func = [](PyObject*, PyObject*) -> PyObject* {
    return ApiHandle::newReference(Thread::current(), SmallInt::fromWord(123));
  };

  HandleScope scope(thread_);
  Function callee(&scope, functionFromModuleMethodDef(
                              thread_, "foo", bit_cast<void*>(func), "",
                              ExtensionMethodType::kMethNoArgs));

  // Set up a code object that calls the function without arguments
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(1));
  consts.atPut(0, *callee);
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, CALL_FUNCTION, 0, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));
  code.setStacksize(1);

  // Execute the code and make sure we get back the result we expect
  Object result(&scope, runCode(code));
  EXPECT_TRUE(isIntEqualsWord(*result, 123));
}

TEST_F(TrampolinesTest, ExtensionModuleNoArgReceivesArgsRaisesTypeError) {
  binaryfunc func = [](PyObject*, PyObject*) -> PyObject* {
    return ApiHandle::newReference(Thread::current(), SmallInt::fromWord(123));
  };

  HandleScope scope(thread_);
  Function callee(&scope, functionFromModuleMethodDef(
                              thread_, "foo", bit_cast<void*>(func), "",
                              ExtensionMethodType::kMethNoArgs));

  // Set up a code object that calls the function with a single argument.
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(2));
  consts.atPut(0, *callee);
  consts.atPut(1, SmallInt::fromWord(1111));
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST,    0, LOAD_CONST,   1,
                           CALL_FUNCTION, 1, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));
  code.setStacksize(2);

  // Execute the code and make sure we get back the result we expect
  EXPECT_TRUE(raisedWithStr(runCode(code), LayoutId::kTypeError,
                            "function takes no arguments"));
}

TEST_F(TrampolinesTest, ExtensionModuleNoArgReturnsNullRaisesSystemError) {
  binaryfunc func = [](PyObject*, PyObject*) -> PyObject* { return nullptr; };

  HandleScope scope(thread_);
  Function callee(&scope, functionFromModuleMethodDef(
                              thread_, "foo", bit_cast<void*>(func), "",
                              ExtensionMethodType::kMethNoArgs));

  // Set up a code object that calls the function without arguments
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(1));
  consts.atPut(0, *callee);
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, CALL_FUNCTION, 0, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));
  code.setStacksize(1);

  // Execute the code and make sure we get back the result we expect
  EXPECT_TRUE(raisedWithStr(runCode(code), LayoutId::kSystemError,
                            "NULL return without exception set"));
}

TEST_F(TrampolinesTest, ExtensionModuleNoArgReceivesKwArgsRaisesTypeError) {
  binaryfunc func = [](PyObject*, PyObject*) -> PyObject* {
    return ApiHandle::newReference(Thread::current(), SmallInt::fromWord(123));
  };

  HandleScope scope(thread_);
  Function callee(&scope, functionFromModuleMethodDef(
                              thread_, "foo", bit_cast<void*>(func), "",
                              ExtensionMethodType::kMethNoArgs));

  // Set up a code object that calls the builtin with (foo='bar')
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(3));
  consts.atPut(0, *callee);
  consts.atPut(1, runtime_.newStrFromCStr("bar"));
  Tuple kw_tuple(&scope, runtime_.newTuple(1));
  kw_tuple.atPut(0, runtime_.newStrFromCStr("foo"));
  consts.atPut(2, *kw_tuple);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST,       0, LOAD_CONST,   1, LOAD_CONST, 2,
                           CALL_FUNCTION_KW, 1, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));
  code.setStacksize(3);

  // Execute the code and make sure we get back the result we expect
  EXPECT_TRUE(raised(runCode(code), LayoutId::kTypeError));
}

TEST_F(TrampolinesTest, ExtensionModuleNoArgReceivesZeroKwArgsReturns) {
  binaryfunc func = [](PyObject*, PyObject*) -> PyObject* {
    return ApiHandle::newReference(Thread::current(), SmallInt::fromWord(123));
  };

  HandleScope scope(thread_);
  Function callee(&scope, functionFromModuleMethodDef(
                              thread_, "foo", bit_cast<void*>(func), "",
                              ExtensionMethodType::kMethNoArgs));

  // Set up a code object that calls the builtin with (foo='bar')
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(2));
  consts.atPut(0, *callee);
  Tuple kw_tuple(&scope, runtime_.emptyTuple());
  consts.atPut(1, *kw_tuple);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST,       0, LOAD_CONST,   1,
                           CALL_FUNCTION_KW, 0, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));
  code.setStacksize(2);

  // Execute the code and make sure we get back the result we expect
  Object result(&scope, runCode(code));
  EXPECT_TRUE(isIntEqualsWord(*result, 123));
}

TEST_F(TrampolinesTest,
       ExtensionModuleNoArgReceivesVariableArgsRaisesTypeError) {
  binaryfunc func = [](PyObject*, PyObject*) -> PyObject* {
    return ApiHandle::newReference(Thread::current(), SmallInt::fromWord(123));
  };

  HandleScope scope(thread_);
  Function callee(&scope, functionFromModuleMethodDef(
                              thread_, "foo", bit_cast<void*>(func), "",
                              ExtensionMethodType::kMethNoArgs));

  // Set up a code object that calls with (*(10))
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(2));
  consts.atPut(0, *callee);
  Tuple arg_tuple(&scope, runtime_.newTuple(1));
  arg_tuple.atPut(0, SmallInt::fromWord(10));
  consts.atPut(1, *arg_tuple);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST,       0, LOAD_CONST,   1,
                           CALL_FUNCTION_EX, 0, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));
  code.setStacksize(2);

  // Execute the code and make sure we get back the result we expect
  EXPECT_TRUE(raised(runCode(code), LayoutId::kTypeError));
}

TEST_F(TrampolinesTest, ExtensionModuleNoArgReceivesVariableArgsReturns) {
  binaryfunc func = [](PyObject*, PyObject*) -> PyObject* {
    return ApiHandle::newReference(Thread::current(), SmallInt::fromWord(123));
  };

  HandleScope scope(thread_);
  Function callee(&scope, functionFromModuleMethodDef(
                              thread_, "foo", bit_cast<void*>(func), "",
                              ExtensionMethodType::kMethNoArgs));

  // Set up a code object that calls with (*())
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(2));
  consts.atPut(0, *callee);
  Tuple arg_tuple(&scope, runtime_.emptyTuple());
  consts.atPut(1, *arg_tuple);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST,       0, LOAD_CONST,   1,
                           CALL_FUNCTION_EX, 0, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));
  code.setStacksize(2);

  // Execute the code and make sure we get back the result we expect
  Object result(&scope, runCode(code));
  EXPECT_TRUE(isIntEqualsWord(*result, 123));
}

TEST_F(TrampolinesTest, ExtensionModuleOneArgReceivesNoArgsRaisesTypeError) {
  binaryfunc func = [](PyObject*, PyObject*) -> PyObject* {
    return ApiHandle::newReference(Thread::current(), SmallInt::fromWord(123));
  };

  HandleScope scope(thread_);
  Function callee(
      &scope, functionFromModuleMethodDef(thread_, "foo", bit_cast<void*>(func),
                                          "", ExtensionMethodType::kMethO));

  // Set up a code object that calls the function without arguments
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(1));
  consts.atPut(0, *callee);
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, CALL_FUNCTION, 0, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));
  code.setStacksize(1);

  // Execute the code and make sure we get back the result we expect
  Object result(&scope, runCode(code));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST_F(TrampolinesTest, ExtensionModuleOneArgReceivesOneArgReturns) {
  binaryfunc func = [](PyObject*, PyObject* arg) -> PyObject* { return arg; };

  HandleScope scope(thread_);
  Function callee(
      &scope, functionFromModuleMethodDef(thread_, "foo", bit_cast<void*>(func),
                                          "", ExtensionMethodType::kMethO));

  // Set up a code object that calls the function with a single argument.
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(2));
  consts.atPut(0, *callee);
  consts.atPut(1, SmallInt::fromWord(1111));
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST,    0, LOAD_CONST,   1,
                           CALL_FUNCTION, 1, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));
  code.setStacksize(2);

  // Execute the code and make sure we get back the result we expect
  RawObject result = runCode(code);
  EXPECT_TRUE(isIntEqualsWord(result, 1111));
}

TEST_F(TrampolinesTest,
       ExtensionModuleOneArgReceivesMultipleArgsRaisesTypeError) {
  binaryfunc func = [](PyObject*, PyObject* arg) -> PyObject* { return arg; };

  HandleScope scope(thread_);
  Function callee(
      &scope, functionFromModuleMethodDef(thread_, "foo", bit_cast<void*>(func),
                                          "", ExtensionMethodType::kMethO));

  // Set up a code object that calls the function with (123, 456)
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(3));
  consts.atPut(0, *callee);
  consts.atPut(1, SmallInt::fromWord(123));
  consts.atPut(2, SmallInt::fromWord(456));
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST,    0, LOAD_CONST,   1, LOAD_CONST, 2,
                           CALL_FUNCTION, 2, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));
  code.setStacksize(3);

  // Execute the code and make sure we get back the result we expect
  Object result(&scope, runCode(code));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST_F(TrampolinesTest, ExtensionModuleOneArgReturnsNullRaisesSystemError) {
  binaryfunc func = [](PyObject*, PyObject*) -> PyObject* { return nullptr; };

  HandleScope scope(thread_);
  Function callee(
      &scope, functionFromModuleMethodDef(thread_, "foo", bit_cast<void*>(func),
                                          "", ExtensionMethodType::kMethO));

  // Set up a code object that calls the function without arguments
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(2));
  consts.atPut(0, *callee);
  consts.atPut(1, SmallInt::fromWord(1111));
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST,    0, LOAD_CONST,   1,
                           CALL_FUNCTION, 1, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));
  code.setStacksize(2);

  // Execute the code and make sure we get back the result we expect
  Object result(&scope, runCode(code));
  EXPECT_TRUE(raised(*result, LayoutId::kSystemError));
}

TEST_F(TrampolinesTest,
       ExtensionModuleOneArgReceivesOneArgAndZeroKwArgsReturns) {
  binaryfunc func = [](PyObject*, PyObject* arg) -> PyObject* { return arg; };

  HandleScope scope(thread_);
  Function callee(
      &scope, functionFromModuleMethodDef(thread_, "foo", bit_cast<void*>(func),
                                          "", ExtensionMethodType::kMethO));

  // Set up a code object that calls the builtin with (1111, {})
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(3));
  consts.atPut(0, *callee);
  consts.atPut(1, SmallInt::fromWord(1111));
  Tuple kw_tuple(&scope, runtime_.emptyTuple());
  consts.atPut(2, *kw_tuple);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST,       0, LOAD_CONST,   1, LOAD_CONST, 2,
                           CALL_FUNCTION_KW, 1, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));
  code.setStacksize(3);

  // Execute the code and make sure we get back the result we expect
  RawObject result = runCode(code);
  EXPECT_TRUE(isIntEqualsWord(result, 1111));
}

TEST_F(TrampolinesTest, ExtensionModuleOneArgReceivesKwArgsRaisesTypeError) {
  binaryfunc func = [](PyObject*, PyObject*) -> PyObject* {
    return ApiHandle::newReference(Thread::current(), SmallInt::fromWord(123));
  };

  HandleScope scope(thread_);
  Function callee(
      &scope, functionFromModuleMethodDef(thread_, "foo", bit_cast<void*>(func),
                                          "", ExtensionMethodType::kMethO));

  // Set up a code object that calls the builtin with (1111, foo='bar')
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(4));
  consts.atPut(0, *callee);
  Tuple kw_tuple(&scope, runtime_.newTuple(1));
  consts.atPut(1, SmallInt::fromWord(1111));
  consts.atPut(2, runtime_.newStrFromCStr("bar"));
  kw_tuple.atPut(0, runtime_.newStrFromCStr("foo"));
  consts.atPut(3, *kw_tuple);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST, 0, LOAD_CONST,       1, LOAD_CONST,   2,
                           LOAD_CONST, 3, CALL_FUNCTION_KW, 2, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));
  code.setStacksize(4);

  // Execute the code and make sure we get back the result we expect
  Object result(&scope, runCode(code));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST_F(TrampolinesTest, ExtensionModuleOneArgReceivesOneArgExReturns) {
  binaryfunc func = [](PyObject*, PyObject* arg) -> PyObject* { return arg; };

  HandleScope scope(thread_);
  Function callee(
      &scope, functionFromModuleMethodDef(thread_, "foo", bit_cast<void*>(func),
                                          "", ExtensionMethodType::kMethO));

  // Set up a code object that calls with (*(10))
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(2));
  consts.atPut(0, *callee);
  Tuple arg_tuple(&scope, runtime_.newTuple(1));
  arg_tuple.atPut(0, SmallInt::fromWord(1111));
  consts.atPut(1, *arg_tuple);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST,       0, LOAD_CONST,   1,
                           CALL_FUNCTION_EX, 0, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));
  code.setStacksize(2);

  // Execute the code and make sure we get back the result we expect
  RawObject result = runCode(code);
  EXPECT_TRUE(isIntEqualsWord(result, 1111));
}

TEST_F(TrampolinesTest, ExtensionModuleOneArgReceivesOneArgAndEmptyKwReturns) {
  binaryfunc func = [](PyObject*, PyObject* arg) -> PyObject* { return arg; };

  HandleScope scope(thread_);
  Function callee(
      &scope, functionFromModuleMethodDef(thread_, "foo", bit_cast<void*>(func),
                                          "", ExtensionMethodType::kMethO));

  // Set up a code object that calls with (*(10), {})
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(3));
  consts.atPut(0, *callee);
  Tuple arg_tuple(&scope, runtime_.newTuple(1));
  arg_tuple.atPut(0, SmallInt::fromWord(1111));
  consts.atPut(1, *arg_tuple);
  Dict kw_dict(&scope, runtime_.newDict());
  consts.atPut(2, *kw_dict);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST,       0, LOAD_CONST,   1, LOAD_CONST, 2,
                           CALL_FUNCTION_EX, 1, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));
  code.setStacksize(3);

  // Execute the code and make sure we get back the result we expect
  RawObject result = runCode(code);
  EXPECT_TRUE(isIntEqualsWord(result, 1111));
}

TEST_F(TrampolinesTest,
       ExtensionModuleOneArgReceivesOneArgAndKwRaisesTypeError) {
  binaryfunc func = [](PyObject*, PyObject*) -> PyObject* {
    return ApiHandle::newReference(Thread::current(), SmallInt::fromWord(123));
  };

  HandleScope scope(thread_);
  Function callee(
      &scope, functionFromModuleMethodDef(thread_, "foo", bit_cast<void*>(func),
                                          "", ExtensionMethodType::kMethO));

  // Set up a code object that calls with (*(10), {2:3})
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(3));
  consts.atPut(0, *callee);
  Tuple arg_tuple(&scope, runtime_.newTuple(1));
  arg_tuple.atPut(0, SmallInt::fromWord(1111));
  consts.atPut(1, *arg_tuple);
  Dict kw_dict(&scope, runtime_.newDict());
  Object key(&scope, SmallInt::fromWord(2));
  Object value(&scope, SmallInt::fromWord(3));
  runtime_.dictAtPut(thread_, kw_dict, key, value);
  consts.atPut(2, *kw_dict);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST,       0, LOAD_CONST,   1, LOAD_CONST, 2,
                           CALL_FUNCTION_EX, 1, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));
  code.setStacksize(3);

  // Execute the code and make sure we get back the result we expect
  Object result(&scope, runCode(code));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST_F(TrampolinesTest, ExtensionModuleVarArgReceivesNoArgsReturns) {
  binaryfunc func = [](PyObject*, PyObject*) -> PyObject* {
    return ApiHandle::newReference(Thread::current(), SmallInt::fromWord(123));
  };

  HandleScope scope(thread_);
  Function callee(&scope, functionFromModuleMethodDef(
                              thread_, "foo", bit_cast<void*>(func), "",
                              ExtensionMethodType::kMethVarArgs));

  // Set up a code object that calls the function without arguments
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(1));
  consts.atPut(0, *callee);
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, CALL_FUNCTION, 0, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));
  code.setStacksize(1);

  // Execute the code and make sure we get back the result we expect
  RawObject result = runCode(code);
  EXPECT_TRUE(isIntEqualsWord(result, 123));
}

TEST_F(TrampolinesTest, ExtensionModuleVarArgReceivesArgsReturns) {
  binaryfunc func = [](PyObject*, PyObject* args) -> PyObject* {
    Thread* thread = Thread::current();
    HandleScope scope(thread);
    Tuple arg_tuple(&scope, ApiHandle::fromPyObject(args)->asObject());
    return ApiHandle::newReference(thread, arg_tuple.at(0));
  };

  HandleScope scope(thread_);
  Function callee(&scope, functionFromModuleMethodDef(
                              thread_, "foo", bit_cast<void*>(func), "",
                              ExtensionMethodType::kMethVarArgs));

  // Set up a code object that calls the function with a single argument.
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(2));
  consts.atPut(0, *callee);
  consts.atPut(1, SmallInt::fromWord(1111));
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST,    0, LOAD_CONST,   1,
                           CALL_FUNCTION, 1, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));
  code.setStacksize(2);

  // Execute the code and make sure we get back the result we expect
  RawObject result = runCode(code);
  EXPECT_TRUE(isIntEqualsWord(result, 1111));
}

TEST_F(TrampolinesTest, ExtensionModuleVarArgReturnsNullRaisesSystemError) {
  binaryfunc func = [](PyObject*, PyObject*) -> PyObject* { return nullptr; };

  HandleScope scope(thread_);
  Function callee(&scope, functionFromModuleMethodDef(
                              thread_, "foo", bit_cast<void*>(func), "",
                              ExtensionMethodType::kMethVarArgs));

  // Set up a code object that calls the function without arguments
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(1));
  consts.atPut(0, *callee);
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, CALL_FUNCTION, 0, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));
  code.setStacksize(1);

  // Execute the code and make sure we get back the result we expect
  EXPECT_TRUE(raisedWithStr(runCode(code), LayoutId::kSystemError,
                            "NULL return without exception set"));
}

TEST_F(TrampolinesTest, ExtensionModuleVarArgReceivesZeroKwArgsReturns) {
  binaryfunc func = [](PyObject*, PyObject* args) -> PyObject* {
    Thread* thread = Thread::current();
    HandleScope scope(thread);
    Tuple arg_tuple(&scope, ApiHandle::fromPyObject(args)->asObject());
    return ApiHandle::newReference(thread, arg_tuple.at(0));
  };

  HandleScope scope(thread_);
  Function callee(&scope, functionFromModuleMethodDef(
                              thread_, "foo", bit_cast<void*>(func), "",
                              ExtensionMethodType::kMethVarArgs));

  // Set up a code object that calls the builtin with (1111, {})
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(3));
  consts.atPut(0, *callee);
  consts.atPut(1, SmallInt::fromWord(1111));
  Tuple kw_tuple(&scope, runtime_.emptyTuple());
  consts.atPut(2, *kw_tuple);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST,       0, LOAD_CONST,   1, LOAD_CONST, 2,
                           CALL_FUNCTION_KW, 1, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));
  code.setStacksize(3);

  // Execute the code and make sure we get back the result we expect
  RawObject result = runCode(code);
  EXPECT_TRUE(isIntEqualsWord(result, 1111));
}

TEST_F(TrampolinesTest, ExtensionModuleVarArgReceivesKwArgsRaisesTypeError) {
  binaryfunc func = [](PyObject*, PyObject*) -> PyObject* {
    return ApiHandle::newReference(Thread::current(), SmallInt::fromWord(123));
  };

  HandleScope scope(thread_);
  Function callee(&scope, functionFromModuleMethodDef(
                              thread_, "foo", bit_cast<void*>(func), "",
                              ExtensionMethodType::kMethVarArgs));

  // Set up a code object that calls the builtin with (1111, foo='bar')
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(4));
  consts.atPut(0, *callee);
  Tuple kw_tuple(&scope, runtime_.newTuple(1));
  consts.atPut(1, SmallInt::fromWord(1111));
  consts.atPut(2, runtime_.newStrFromCStr("bar"));
  kw_tuple.atPut(0, runtime_.newStrFromCStr("foo"));
  consts.atPut(3, *kw_tuple);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST, 0, LOAD_CONST,       1, LOAD_CONST,   2,
                           LOAD_CONST, 3, CALL_FUNCTION_KW, 2, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));
  code.setStacksize(4);

  // Execute the code and make sure we get back the result we expect
  Object result(&scope, runCode(code));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST_F(TrampolinesTest, ExtensionModuleVarArgReceivesVarArgsReturns) {
  binaryfunc func = [](PyObject*, PyObject* args) -> PyObject* {
    Thread* thread = Thread::current();
    HandleScope scope(thread);
    Tuple arg_tuple(&scope, ApiHandle::fromPyObject(args)->asObject());
    return ApiHandle::newReference(thread, arg_tuple.at(0));
  };

  HandleScope scope(thread_);
  Function callee(&scope, functionFromModuleMethodDef(
                              thread_, "foo", bit_cast<void*>(func), "",
                              ExtensionMethodType::kMethVarArgs));

  // Set up a code object that calls with (*(10))
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(2));
  consts.atPut(0, *callee);
  Tuple arg_tuple(&scope, runtime_.newTuple(1));
  arg_tuple.atPut(0, SmallInt::fromWord(1111));
  consts.atPut(1, *arg_tuple);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST,       0, LOAD_CONST,   1,
                           CALL_FUNCTION_EX, 0, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));
  code.setStacksize(2);

  // Execute the code and make sure we get back the result we expect
  RawObject result = runCode(code);
  EXPECT_TRUE(isIntEqualsWord(result, 1111));
}

TEST_F(TrampolinesTest, ExtensionModuleVarArgReceivesVarArgsAndEmptyKwReturns) {
  binaryfunc func = [](PyObject*, PyObject* args) -> PyObject* {
    Thread* thread = Thread::current();
    HandleScope scope(thread);
    Tuple arg_tuple(&scope, ApiHandle::fromPyObject(args)->asObject());
    return ApiHandle::newReference(thread, arg_tuple.at(0));
  };

  HandleScope scope(thread_);
  Function callee(&scope, functionFromModuleMethodDef(
                              thread_, "foo", bit_cast<void*>(func), "",
                              ExtensionMethodType::kMethVarArgs));

  // Set up a code object that calls with (*(10), {})
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(3));
  consts.atPut(0, *callee);
  Tuple arg_tuple(&scope, runtime_.newTuple(1));
  arg_tuple.atPut(0, SmallInt::fromWord(1111));
  consts.atPut(1, *arg_tuple);
  Dict kw_dict(&scope, runtime_.newDict());
  consts.atPut(2, *kw_dict);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST,       0, LOAD_CONST,   1, LOAD_CONST, 2,
                           CALL_FUNCTION_EX, 1, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));
  code.setStacksize(3);

  // Execute the code and make sure we get back the result we expect
  RawObject result = runCode(code);
  EXPECT_TRUE(isIntEqualsWord(result, 1111));
}

TEST_F(TrampolinesTest,
       ExtensionModuleVarArgReceivesVarArgsAndKwRaisesTypeError) {
  binaryfunc func = [](PyObject*, PyObject*) -> PyObject* {
    return ApiHandle::newReference(Thread::current(), SmallInt::fromWord(123));
  };

  HandleScope scope(thread_);
  Function callee(&scope, functionFromModuleMethodDef(
                              thread_, "foo", bit_cast<void*>(func), "",
                              ExtensionMethodType::kMethVarArgs));

  // Set up a code object that calls with (*(10), {})
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(3));
  consts.atPut(0, *callee);
  Tuple arg_tuple(&scope, runtime_.newTuple(1));
  arg_tuple.atPut(0, SmallInt::fromWord(1111));
  consts.atPut(1, *arg_tuple);
  Dict kw_dict(&scope, runtime_.newDict());
  Object key(&scope, SmallInt::fromWord(2));
  Object value(&scope, SmallInt::fromWord(3));
  runtime_.dictAtPut(thread_, kw_dict, key, value);
  consts.atPut(2, *kw_dict);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST,       0, LOAD_CONST,   1, LOAD_CONST, 2,
                           CALL_FUNCTION_EX, 1, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));
  code.setStacksize(3);

  // Execute the code and make sure we get back the result we expect
  Object result(&scope, runCode(code));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST_F(TrampolinesTest, ExtensionModuleKeywordArgReceivesNoArgsReturns) {
  ternaryfunc func = [](PyObject*, PyObject*, PyObject*) -> PyObject* {
    return ApiHandle::newReference(Thread::current(), SmallInt::fromWord(123));
  };

  HandleScope scope(thread_);
  Function callee(&scope, functionFromModuleMethodDef(
                              thread_, "foo", bit_cast<void*>(func), "",
                              ExtensionMethodType::kMethVarArgsAndKeywords));

  // Set up a code object that calls the function without arguments
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(1));
  consts.atPut(0, *callee);
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, CALL_FUNCTION, 0, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));
  code.setStacksize(1);

  // Execute the code and make sure we get back the result we expect
  RawObject result = runCode(code);
  EXPECT_TRUE(isIntEqualsWord(result, 123));
}

TEST_F(TrampolinesTest, ExtensionModuleKeywordArgReceivesArgsReturns) {
  ternaryfunc func = [](PyObject*, PyObject* args, PyObject*) -> PyObject* {
    Thread* thread = Thread::current();
    HandleScope scope(thread);
    Tuple arg_tuple(&scope, ApiHandle::fromPyObject(args)->asObject());
    return ApiHandle::newReference(thread, arg_tuple.at(0));
  };

  HandleScope scope(thread_);
  Function callee(&scope, functionFromModuleMethodDef(
                              thread_, "foo", bit_cast<void*>(func), "",
                              ExtensionMethodType::kMethVarArgsAndKeywords));

  // Set up a code object that calls the function with a single argument.
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(2));
  consts.atPut(0, *callee);
  consts.atPut(1, SmallInt::fromWord(1111));
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST,    0, LOAD_CONST,   1,
                           CALL_FUNCTION, 1, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));
  code.setStacksize(2);

  // Execute the code and make sure we get back the result we expect
  RawObject result = runCode(code);
  EXPECT_TRUE(isIntEqualsWord(result, 1111));
}

TEST_F(TrampolinesTest, ExtensionModuleKeywordArgReturnsNullRaisesSystemError) {
  ternaryfunc func = [](PyObject*, PyObject*, PyObject*) -> PyObject* {
    return nullptr;
  };

  HandleScope scope(thread_);
  Function callee(&scope, functionFromModuleMethodDef(
                              thread_, "foo", bit_cast<void*>(func), "",
                              ExtensionMethodType::kMethVarArgsAndKeywords));

  // Set up a code object that calls the function without arguments
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(1));
  consts.atPut(0, *callee);
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, CALL_FUNCTION, 0, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));
  code.setStacksize(1);

  // Execute the code and make sure we get back the result we expect
  Object result(&scope, runCode(code));
  EXPECT_TRUE(raisedWithStr(*result, LayoutId::kSystemError,
                            "NULL return without exception set"));
}

TEST_F(TrampolinesTest, ExtensionModuleKeywordArgReceivesKwArgsReturns) {
  ternaryfunc func = [](PyObject*, PyObject*, PyObject* kwargs) -> PyObject* {
    Thread* thread = Thread::current();
    HandleScope scope(thread);
    Runtime* runtime = thread->runtime();
    Str foo_str(&scope, runtime->newStrFromCStr("foo"));
    Dict keyword_dict(&scope, ApiHandle::fromPyObject(kwargs)->asObject());
    return ApiHandle::newReference(
        thread, runtime->dictAt(thread, keyword_dict, foo_str));
  };

  HandleScope scope(thread_);
  Function callee(&scope, functionFromModuleMethodDef(
                              thread_, "foo", bit_cast<void*>(func), "",
                              ExtensionMethodType::kMethVarArgsAndKeywords));

  // Set up a code object that calls the builtin with ("bar", foo=1111)
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(4));
  consts.atPut(0, *callee);
  consts.atPut(1, runtime_.newStrFromCStr("bar"));
  consts.atPut(2, SmallInt::fromWord(1111));
  Tuple kw_tuple(&scope, runtime_.newTuple(1));
  kw_tuple.atPut(0, runtime_.newStrFromCStr("foo"));
  consts.atPut(3, *kw_tuple);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST, 0, LOAD_CONST,       1, LOAD_CONST,   2,
                           LOAD_CONST, 3, CALL_FUNCTION_KW, 2, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));
  code.setStacksize(4);

  // Execute the code and make sure we get back the result we expect
  RawObject result = runCode(code);
  EXPECT_TRUE(isIntEqualsWord(result, 1111));
}

TEST_F(TrampolinesTest, ExtensionModuleKeywordArgReceivesMultipleArgsReturns) {
  ternaryfunc func = [](PyObject*, PyObject* args, PyObject*) -> PyObject* {
    Thread* thread = Thread::current();
    HandleScope scope(thread);
    Tuple args_tuple(&scope, ApiHandle::fromPyObject(args)->asObject());
    return ApiHandle::newReference(thread, args_tuple.at(1));
  };

  HandleScope scope(thread_);
  Function callee(&scope, functionFromModuleMethodDef(
                              thread_, "foo", bit_cast<void*>(func), "",
                              ExtensionMethodType::kMethVarArgsAndKeywords));

  // Set up a code object that calls the builtin with (123, 456, foo=789)
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(5));
  consts.atPut(0, *callee);
  consts.atPut(1, SmallInt::fromWord(123));
  consts.atPut(2, SmallInt::fromWord(456));
  consts.atPut(3, SmallInt::fromWord(789));
  Tuple kw_tuple(&scope, runtime_.newTuple(1));
  kw_tuple.atPut(0, runtime_.newStrFromCStr("foo"));
  consts.atPut(4, *kw_tuple);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST,   0, LOAD_CONST, 1, LOAD_CONST,       2,
                           LOAD_CONST,   3, LOAD_CONST, 4, CALL_FUNCTION_KW, 3,
                           RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));
  code.setStacksize(5);

  // Execute the code and make sure we get back the result we expect
  RawObject result = runCode(code);
  EXPECT_TRUE(isIntEqualsWord(result, 456));
}

TEST_F(TrampolinesTest,
       ExtensionModuleKeywordArgReceivesMultipleKwArgsReturns) {
  ternaryfunc func = [](PyObject*, PyObject*, PyObject* kwargs) -> PyObject* {
    Thread* thread = Thread::current();
    HandleScope scope(thread);
    Runtime* runtime = thread->runtime();
    Str foo_str(&scope, runtime->newStrFromCStr("bar"));
    Dict keyword_dict(&scope, ApiHandle::fromPyObject(kwargs)->asObject());
    return ApiHandle::newReference(
        thread, runtime->dictAt(thread, keyword_dict, foo_str));
  };

  HandleScope scope(thread_);
  Function callee(&scope, functionFromModuleMethodDef(
                              thread_, "foo", bit_cast<void*>(func), "",
                              ExtensionMethodType::kMethVarArgsAndKeywords));

  // Set up a code object that calls the builtin with ("foo"=1234, "bar"=5678)
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(4));
  consts.atPut(0, *callee);
  consts.atPut(1, SmallInt::fromWord(1234));
  consts.atPut(2, SmallInt::fromWord(5678));
  Tuple kw_tuple(&scope, runtime_.newTuple(2));
  kw_tuple.atPut(0, runtime_.newStrFromCStr("foo"));
  kw_tuple.atPut(1, runtime_.newStrFromCStr("bar"));
  consts.atPut(3, *kw_tuple);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST, 0, LOAD_CONST,       1, LOAD_CONST,   2,
                           LOAD_CONST, 3, CALL_FUNCTION_KW, 2, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));
  code.setStacksize(4);

  // Execute the code and make sure we get back the result we expect
  RawObject result = runCode(code);
  EXPECT_TRUE(isIntEqualsWord(result, 5678));
}

TEST_F(TrampolinesTest, ExtensionModuleKeywordArgReceivesVariableArgsReturns) {
  ternaryfunc func = [](PyObject*, PyObject* args, PyObject*) -> PyObject* {
    Thread* thread = Thread::current();
    HandleScope scope(thread);
    Tuple arg_tuple(&scope, ApiHandle::fromPyObject(args)->asObject());
    return ApiHandle::newReference(thread, arg_tuple.at(0));
  };

  HandleScope scope(thread_);
  Function callee(&scope, functionFromModuleMethodDef(
                              thread_, "foo", bit_cast<void*>(func), "",
                              ExtensionMethodType::kMethVarArgsAndKeywords));

  // Set up a code object that calls with (*(10))
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(2));
  consts.atPut(0, *callee);
  Tuple arg_tuple(&scope, runtime_.newTuple(1));
  arg_tuple.atPut(0, SmallInt::fromWord(10));
  consts.atPut(1, *arg_tuple);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST,       0, LOAD_CONST,   1,
                           CALL_FUNCTION_EX, 0, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));
  code.setStacksize(2);

  // Execute the code and make sure we get back the result we expect
  RawObject result = runCode(code);
  EXPECT_TRUE(isIntEqualsWord(result, 10));
}

TEST_F(TrampolinesTest,
       ExtensionModuleKeywordArgReceivesVariableKwArgsReturns) {
  ternaryfunc func = [](PyObject*, PyObject*, PyObject* kwargs) -> PyObject* {
    Thread* thread = Thread::current();
    HandleScope scope(thread);
    Runtime* runtime = thread->runtime();
    Str foo_str(&scope, runtime->newStrFromCStr("foo"));
    Dict keyword_dict(&scope, ApiHandle::fromPyObject(kwargs)->asObject());
    return ApiHandle::newReference(
        thread, runtime->dictAt(thread, keyword_dict, foo_str));
  };

  HandleScope scope(thread_);
  Function callee(&scope, functionFromModuleMethodDef(
                              thread_, "foo", bit_cast<void*>(func), "",
                              ExtensionMethodType::kMethVarArgsAndKeywords));

  // Set up a code object that calls with (*(10), **{"foo":1111})
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(3));
  consts.atPut(0, *callee);
  Tuple arg_tuple(&scope, runtime_.newTuple(1));
  arg_tuple.atPut(0, SmallInt::fromWord(10));
  consts.atPut(1, *arg_tuple);
  Dict kw_dict(&scope, runtime_.newDict());
  Object key(&scope, runtime_.newStrFromCStr("foo"));
  Object value(&scope, SmallInt::fromWord(1111));
  runtime_.dictAtPut(thread_, kw_dict, key, value);
  consts.atPut(2, *kw_dict);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST,       0, LOAD_CONST,   1, LOAD_CONST, 2,
                           CALL_FUNCTION_EX, 1, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));
  code.setStacksize(3);

  // Execute the code and make sure we get back the result we expect
  RawObject result = runCode(code);
  EXPECT_TRUE(isIntEqualsWord(result, 1111));
}

static RawObject numArgs(Thread*, Frame*, word nargs) {
  return SmallInt::fromWord(nargs);
}

static void createAndPatchBuiltinNumArgs(Runtime* runtime) {
  // Ensure we have a __main__ module.
  ASSERT_FALSE(runFromCStr(runtime, "").isError());
  HandleScope scope;
  Module main(&scope, findMainModule(runtime));
  runtime->moduleAddBuiltinFunction(main, SymbolId::kDummy, numArgs);
  ASSERT_FALSE(runFromCStr(runtime, R"(
@_patch
def dummy(first, second):
  pass
)")
                   .isError());
}

TEST_F(TrampolinesTest, BuiltinTrampolineExReceivesExArgs) {
  createAndPatchBuiltinNumArgs(&runtime_);
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, "result = dummy(*(1,2))").isError());
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 2));
}

TEST_F(TrampolinesTest, BuiltinTrampolineExReceivesMixOfPositionalAndExArgs1) {
  createAndPatchBuiltinNumArgs(&runtime_);
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, "result = dummy(1, *(2,))").isError());
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 2));
}

static void createAndPatchBuiltinNumArgsVariadic(Runtime* runtime) {
  // Ensure we have a __main__ module.
  ASSERT_FALSE(runFromCStr(runtime, "").isError());
  HandleScope scope;
  Module main(&scope, findMainModule(runtime));
  runtime->moduleAddBuiltinFunction(main, SymbolId::kDummy, numArgs);
  ASSERT_FALSE(runFromCStr(runtime, R"(
@_patch
def dummy(*args):
  pass
)")
                   .isError());
}

TEST_F(TrampolinesTest,
       BuiltinTrampolineExReceivesOnePositionalArgAndTwoVariableArgs) {
  createAndPatchBuiltinNumArgsVariadic(&runtime_);
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, "result = dummy(1, *(2, 3))").isError());
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 1));
}

static void createAndPatchBuiltinNumArgsArgsKwargs(Runtime* runtime) {
  // Ensure we have a __main__ module.
  ASSERT_FALSE(runFromCStr(runtime, "").isError());
  HandleScope scope;
  Module main(&scope, findMainModule(runtime));
  runtime->moduleAddBuiltinFunction(main, SymbolId::kDummy, numArgs);
  ASSERT_FALSE(runFromCStr(runtime, R"(
@_patch
def dummy(*args, **kwargs):
  pass
)")
                   .isError());
}

TEST_F(TrampolinesTest,
       BuiltinTrampolineExReceivesTwoPositionalOneVariableAndTwoKwArgs) {
  createAndPatchBuiltinNumArgsArgsKwargs(&runtime_);
  HandleScope scope(thread_);
  ASSERT_FALSE(
      runFromCStr(&runtime_,
                  "result = dummy(1, 2, *(3,), **{'foo': 1, 'bar': 2})")
          .isError());
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 2));
}

TEST_F(TrampolinesTest, BuiltinTrampolineExReceivesVarArgs) {
  createAndPatchBuiltinNumArgs(&runtime_);
  HandleScope scope(thread_);
  ASSERT_FALSE(
      runFromCStr(&runtime_, "result = dummy(*(1,), second=5)").isError());
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 2));
}

TEST_F(TrampolinesDeathTest, BuiltinTrampolineExWithTooFewArgsRaisesTypeError) {
  createAndPatchBuiltinNumArgs(&runtime_);
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_, "dummy(*(1,))"), LayoutId::kTypeError,
      "TypeError: 'dummy' takes 2 positional arguments but 1 given"));
}

TEST_F(TrampolinesDeathTest,
       BuiltinTrampolineExWithTooManyArgsRaisesTypeError) {
  createAndPatchBuiltinNumArgs(&runtime_);
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_, "dummy(*(1,2,3,4,5))"), LayoutId::kTypeError,
      "TypeError: 'dummy' takes max 2 positional arguments but 5 given"));
}

}  // namespace python
