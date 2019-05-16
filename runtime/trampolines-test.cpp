#include "gtest/gtest.h"

#include <memory>

#include "frame.h"
#include "runtime.h"
#include "test-utils.h"
#include "trampolines-inl.h"

namespace python {
using namespace testing;

TEST(CallTest, CallBoundMethod) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
def func(self):
  print(self)

def test(callable):
  return callable()
)")
                   .isError());

  Module module(&scope, findModule(&runtime, "__main__"));
  Object function(&scope, moduleAt(&runtime, module, "func"));
  ASSERT_TRUE(function.isFunction());

  Object self(&scope, SmallInt::fromWord(1111));
  BoundMethod method(&scope, runtime.newBoundMethod(function, self));

  Object test(&scope, moduleAt(&runtime, module, "test"));
  ASSERT_TRUE(test.isFunction());
  Function func(&scope, *test);

  Tuple args(&scope, runtime.newTuple(1));
  args.atPut(0, *method);

  std::string output = callFunctionToString(func, args);
  EXPECT_EQ(output, "1111\n");
}

TEST(CallTest, CallBoundMethodWithArgs) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
def func(self, a, b):
  print(self, a, b)

def test(callable):
  return callable(2222, 3333)
)")
                   .isError());

  Module module(&scope, findModule(&runtime, "__main__"));
  Object function(&scope, moduleAt(&runtime, module, "func"));
  ASSERT_TRUE(function.isFunction());

  Object self(&scope, SmallInt::fromWord(1111));
  BoundMethod method(&scope, runtime.newBoundMethod(function, self));

  Object test(&scope, moduleAt(&runtime, module, "test"));
  ASSERT_TRUE(test.isFunction());
  Function func(&scope, *test);

  Tuple args(&scope, runtime.newTuple(1));
  args.atPut(0, *method);

  std::string output = callFunctionToString(func, args);
  EXPECT_EQ(output, "1111 2222 3333\n");
}

TEST(CallTest, CallBoundMethodKw) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
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

  Module module(&scope, findModule(&runtime, "__main__"));
  Object function(&scope, moduleAt(&runtime, module, "func"));
  ASSERT_TRUE(function.isFunction());

  Object self(&scope, SmallInt::fromWord(1111));
  BoundMethod method(&scope, runtime.newBoundMethod(function, self));

  Object test(&scope, moduleAt(&runtime, module, "test"));
  ASSERT_TRUE(test.isFunction());
  Function func(&scope, *test);
  Tuple args(&scope, runtime.newTuple(1));
  args.atPut(0, *method);
  callFunction(func, args);

  Object result_self(&scope, moduleAt(&runtime, module, "result_self"));
  EXPECT_TRUE(isIntEqualsWord(*result_self, 1111));

  Object result_a(&scope, moduleAt(&runtime, module, "result_a"));
  EXPECT_TRUE(isIntEqualsWord(*result_a, 2222));

  Object result_b(&scope, moduleAt(&runtime, module, "result_b"));
  EXPECT_TRUE(isIntEqualsWord(*result_b, 3333));
}

TEST(CallTest, CallBoundMethodExArgs) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
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

  Module module(&scope, findModule(&runtime, "__main__"));
  Object function(&scope, moduleAt(&runtime, module, "func"));
  ASSERT_TRUE(function.isFunction());

  Object self(&scope, SmallInt::fromWord(1111));
  BoundMethod method(&scope, runtime.newBoundMethod(function, self));

  Object test(&scope, moduleAt(&runtime, module, "test"));
  ASSERT_TRUE(test.isFunction());
  Function func(&scope, *test);
  Tuple args(&scope, runtime.newTuple(1));
  args.atPut(0, *method);
  callFunction(func, args);

  Object result_self(&scope, moduleAt(&runtime, module, "result_self"));
  EXPECT_TRUE(isIntEqualsWord(*result_self, 1111));

  Object result_a(&scope, moduleAt(&runtime, module, "result_a"));
  EXPECT_TRUE(isIntEqualsWord(*result_a, 2222));

  Object result_b(&scope, moduleAt(&runtime, module, "result_b"));
  EXPECT_TRUE(isIntEqualsWord(*result_b, 3333));
}

TEST(CallTest, CallBoundMethodExKwargs) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
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

  Module module(&scope, findModule(&runtime, "__main__"));
  Object function(&scope, moduleAt(&runtime, module, "func"));
  ASSERT_TRUE(function.isFunction());

  Object self(&scope, SmallInt::fromWord(1111));
  BoundMethod method(&scope, runtime.newBoundMethod(function, self));

  Object test(&scope, moduleAt(&runtime, module, "test"));
  ASSERT_TRUE(test.isFunction());
  Function func(&scope, *test);
  Tuple args(&scope, runtime.newTuple(1));
  args.atPut(0, *method);
  callFunction(func, args);

  Object result_self(&scope, moduleAt(&runtime, module, "result_self"));
  EXPECT_TRUE(isIntEqualsWord(*result_self, 1111));

  Object result_a(&scope, moduleAt(&runtime, module, "result_a"));
  EXPECT_TRUE(isIntEqualsWord(*result_a, 2222));

  Object result_b(&scope, moduleAt(&runtime, module, "result_b"));
  EXPECT_TRUE(isIntEqualsWord(*result_b, 3333));
}

TEST(CallTest, CallBoundMethodExArgsAndKwargs) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
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

  Module module(&scope, findModule(&runtime, "__main__"));
  Object function(&scope, moduleAt(&runtime, module, "func"));
  ASSERT_TRUE(function.isFunction());

  Object self(&scope, SmallInt::fromWord(1111));
  BoundMethod method(&scope, runtime.newBoundMethod(function, self));

  Object test(&scope, moduleAt(&runtime, module, "test"));
  ASSERT_TRUE(test.isFunction());
  Function func(&scope, *test);
  Tuple args(&scope, runtime.newTuple(1));
  args.atPut(0, *method);
  callFunction(func, args);

  Object result_self(&scope, moduleAt(&runtime, module, "result_self"));
  EXPECT_TRUE(isIntEqualsWord(*result_self, 1111));

  Object result_a(&scope, moduleAt(&runtime, module, "result_a"));
  EXPECT_TRUE(isIntEqualsWord(*result_a, 2222));

  Object result_b(&scope, moduleAt(&runtime, module, "result_b"));
  EXPECT_TRUE(isIntEqualsWord(*result_b, 3333));
}

TEST(CallTest, CallDefaultArgs) {
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, R"(
def foo(a=1, b=2, c=3):
  print(a, b, c)

print()
foo(33, 22, 11)
foo()
foo(1001)
foo(1001, 1002)
foo(1001, 1002, 1003)
)");
  EXPECT_EQ(output, R"(
33 22 11
1 2 3
1001 2 3
1001 1002 3
1001 1002 1003
)");
}

TEST(CallTest, CallMethodMixPosDefaultArgs) {
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, R"(
def foo(a, b=2):
  print(a, b)
foo(1)
)");
  EXPECT_EQ(output, "1 2\n");
}

TEST(CallTest, CallBoundMethodMixed) {
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, R"(
class R:
  def __init__(self, a, b=2):
    print(a, b)
a = R(9)
)");
  EXPECT_EQ(output, "9 2\n");
}

TEST(CallTest, SingleKW) {
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, R"(
def foo(bar):
   print('bar =',bar)
foo(bar=2)
)");
  EXPECT_EQ(output, "bar = 2\n");
}

TEST(CallTest, MixedKW) {
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, R"(
def foo(a, b, c):
   print(a, b, c)
foo(1, b = 2, c = 3)
)");
  EXPECT_EQ(output, "1 2 3\n");
}

TEST(CallTest, FullKW) {
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, R"(
def foo(a, b, c):
   print(a, b, c)
foo(a = 1, b = 2, c = 3)
)");
  EXPECT_EQ(output, "1 2 3\n");
}

TEST(CallTest, KWOutOfOrder1) {
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, R"(
def foo(a, b, c):
   print(a, b, c)
foo(c = 3, a = 1, b = 2)
)");
  EXPECT_EQ(output, "1 2 3\n");
}

TEST(CallTest, KWOutOfOrder2) {
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, R"(
def foo(a, b, c):
   print(a, b, c)
foo(1, c = 3, b = 2)
)");
  EXPECT_EQ(output, "1 2 3\n");
}

TEST(CallTest, KeywordOnly1) {
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, R"(
def foo(a,b, *, c):
  print(a,b,c)
foo(1, 2, c = 3);
)");
  EXPECT_EQ(output, "1 2 3\n");
}

TEST(CallTest, KeywordOnly2) {
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, R"(
def foo(a,b, *, c):
  print(a,b,c)
foo(1, b = 2, c = 3);
)");
  EXPECT_EQ(output, "1 2 3\n");
}

TEST(CallTest, KeyWordDefaults) {
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, R"(
def foo(a, b = 22, c = 33):
  print(a,b,c)
foo(11, c = 3);
)");
  EXPECT_EQ(output, "11 22 3\n");
}

TEST(CallTest, VarArgsWithExcess) {
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, R"(
def foo(a, b, *c):
  print(a,b,c)
foo(1,2,3,4,5,6);
)");
  EXPECT_EQ(output, "1 2 (3, 4, 5, 6)\n");
}

TEST(CallTest, VarArgsEmpty) {
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, R"(
def foo(a, b, *c):
  print(a,b,c)
foo(1,2);
)");
  EXPECT_EQ(output, "1 2 ()\n");
}

TEST(CallTest, CallWithKeywordsCalleeWithVarkeyword) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
def foo(a,b,c,**d):
    return [a,b,c,d]
result = foo(1,2,c=3,g=4,h=5,j="bar")
)")
                   .isError());
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  List result(&scope, testing::moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 2));
  EXPECT_TRUE(isIntEqualsWord(result.at(2), 3));

  Dict dict(&scope, result.at(3));
  Object key_g(&scope, runtime.newStrFromCStr("g"));
  EXPECT_TRUE(isIntEqualsWord(runtime.dictAt(thread, dict, key_g), 4));
  Object key_h(&scope, runtime.newStrFromCStr("h"));
  EXPECT_TRUE(isIntEqualsWord(runtime.dictAt(thread, dict, key_h), 5));
  Object key_j(&scope, runtime.newStrFromCStr("j"));
  EXPECT_TRUE(isStrEqualsCStr(runtime.dictAt(thread, dict, key_j), "bar"));
}

TEST(CallTest, CallWithNoArgsCalleeDefaultArgsVarargsVarkeyargs) {
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, R"(
def bar(a=1, b=2, *c, **d):
    print(a,b,c,d)
bar()
)");
  EXPECT_EQ(output, "1 2 () {}\n");
}

TEST(CallTest, CallPositionalCalleeVargsEmptyVarkeyargs) {
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, R"(
def bar(a=1, b=2, *c, **d):
    print(a,b,c,d)
bar(1,2,3,4,5,6,7)
)");
  EXPECT_EQ(output, "1 2 (3, 4, 5, 6, 7) {}\n");
}

TEST(CallTest, CallWithKeywordsCalleeEmptyVarargsFullVarkeyargs) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
def bar(a=1, b=2, *c, **d):
    return [a,b,c,d]
result = bar(a1=11, a2=12, a3=13)
)")
                   .isError());
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  List result(&scope, testing::moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 2));

  Object tuple_obj(&scope, result.at(2));
  ASSERT_TRUE(tuple_obj.isTuple());
  Tuple tuple(&scope, *tuple_obj);
  EXPECT_EQ(tuple.length(), 0);

  Dict dict(&scope, result.at(3));
  Object key0(&scope, runtime.newStrFromCStr("a3"));
  EXPECT_TRUE(isIntEqualsWord(runtime.dictAt(thread, dict, key0), 13));
  Object key1(&scope, runtime.newStrFromCStr("a1"));
  EXPECT_TRUE(isIntEqualsWord(runtime.dictAt(thread, dict, key1), 11));
  Object key2(&scope, runtime.newStrFromCStr("a2"));
  EXPECT_TRUE(isIntEqualsWord(runtime.dictAt(thread, dict, key2), 12));
}

TEST(CallTest, CallWithKeywordsCalleeFullVarargsFullVarkeyargs) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
def bar(a=1, b=2, *c, **d):
    return [a,b,c,d]
result = bar(1,2,3,4,5,6,7,a9=9)
)")
                   .isError());
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  List result(&scope, testing::moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 2));

  Tuple tuple(&scope, result.at(2));
  EXPECT_TRUE(isIntEqualsWord(tuple.at(0), 3));
  EXPECT_TRUE(isIntEqualsWord(tuple.at(1), 4));
  EXPECT_TRUE(isIntEqualsWord(tuple.at(2), 5));
  EXPECT_TRUE(isIntEqualsWord(tuple.at(3), 6));
  EXPECT_TRUE(isIntEqualsWord(tuple.at(4), 7));

  Dict dict(&scope, result.at(3));
  Object key_g(&scope, runtime.newStrFromCStr("a9"));
  EXPECT_TRUE(isIntEqualsWord(runtime.dictAt(thread, dict, key_g), 9));
}

TEST(CallTest, CallWithOutOfOrderKeywords) {
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, R"(
def foobar(a,b,*,c):
    print(a,b,c)
foobar(c=3,a=1,b=2)
)");
  EXPECT_EQ(output, "1 2 3\n");
}

TEST(CallTest, CallWithKeywordsCalleeVarargsKeywordOnly) {
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, R"(
def foobar1(a,b,*c,d):
    print(a,b,c,d)
foobar1(1,2,3,4,5,d=9)
)");
  EXPECT_EQ(output, "1 2 (3, 4, 5) 9\n");
}

TEST(CallTest, CallWithKeywordsCalleeVarargsVarkeyargsKeywordOnly) {
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, R"(
def foobar2(a,b,*c, e, **d):
    print(a,b,c,d,e)
foobar2(1,e=9,b=2,f1="a",f11=12)
)");
  EXPECT_EQ(output, "1 2 () {'f1': 'a', 'f11': 12} 9\n");
}

TEST(CallTest, CallEx) {
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, R"(
def foo(a,b,c,d):
    print(a,b,c,d)
a = (1,2,3,4)
foo(*a)
)");
  EXPECT_EQ(output, "1 2 3 4\n");
}

TEST(CallTest, CallExBuildTupleUnpackWithCall) {
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, R"(
def foo(a,b,c,d):
    print(a,b,c,d)
a = (3,4)
foo(1,2,*a)
)");
  EXPECT_EQ(output, "1 2 3 4\n");
}

TEST(CallTest, CallExKw) {
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, R"(
def foo(a,b,c,d):
    print(a,b,c,d)
a = {'d': 4, 'b': 2, 'a': 1, 'c': 3}
foo(**a)
)");
  EXPECT_EQ(output, "1 2 3 4\n");
}

TEST(CallTest, KeywordOnly) {
  Runtime runtime;
  const char* src = R"(
def foo(a,b, *, c):
  print(a,b,c)
foo(1, 2, 3);
)";
  EXPECT_TRUE(raised(runFromCStr(&runtime, src), LayoutId::kTypeError));
}

TEST(CallTest, MissingKeyword) {
  Runtime runtime;
  const char* src = R"(
def foo(a,b, *, c):
  print(a,b,c)
foo(1, 2);
)";
  EXPECT_TRUE(raised(runFromCStr(&runtime, src), LayoutId::kTypeError));
}

TEST(CallTest, ArgNameMismatch) {
  Runtime runtime;
  const char* src = R"(
def foo(a,b, *, c):
  print(a,b,c)
foo(1, d = 2, c = 3);
)";
  EXPECT_TRUE(raised(runFromCStr(&runtime, src), LayoutId::kTypeError));
}

TEST(CallTest, TooManyKWArgs) {
  Runtime runtime;
  const char* src = R"(
def foo(a,b, *, c):
  print(a,b,c)
foo(1, 2, 4, c = 3);
)";
  EXPECT_TRUE(raised(runFromCStr(&runtime, src), LayoutId::kTypeError));
}

TEST(CallTest, TooManyArgs) {
  Runtime runtime;
  const char* src = R"(
def foo(a,b, c):
  print(a,b,c)
foo(1, 2, 3, 4);
)";
  EXPECT_TRUE(raised(runFromCStr(&runtime, src), LayoutId::kTypeError));
}

TEST(CallTest, TooFewArgs) {
  Runtime runtime;
  const char* src = R"(
def foo(a,b, c):
  print(a,b,c)
foo(3, 4);
)";
  EXPECT_TRUE(raised(runFromCStr(&runtime, src), LayoutId::kTypeError));
}

static RawObject firstArg(Thread*, Frame* frame, word argc) {
  if (argc == 0) {
    return NoneType::object();
  }
  Arguments args(frame, argc);
  return args.get(0);
}

TEST(TrampolinesTest, CallNativeFunctionReceivesPositionalArgument) {
  Runtime runtime;
  HandleScope scope;

  // Create the builtin function
  Function callee(&scope, runtime.newFunction());
  callee.setEntry(nativeTrampoline<firstArg>);

  // Set up a code object that calls the builtin with a single argument.
  Object name(&scope, Str::empty());
  Code code(&scope, runtime.newEmptyCode(name));
  Tuple consts(&scope, runtime.newTuple(2));
  consts.atPut(0, *callee);
  consts.atPut(1, SmallInt::fromWord(1111));
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST,    0, LOAD_CONST,   1,
                           CALL_FUNCTION, 1, RETURN_VALUE, 0};
  code.setCode(runtime.newBytesWithAll(bytecode));
  code.setStacksize(2);

  // Execute the code and make sure we get back the result we expect
  RawObject result = Thread::current()->run(code);
  EXPECT_TRUE(isIntEqualsWord(result, 1111));
}

// test "builtin-kw" func that returns a list of first position arg
// and value of kw argument 'foo'
static RawObject returnsPositionalAndKeywordArgument(Thread* thread,
                                                     Frame* frame, word argc) {
  KwArguments args(frame, argc);
  HandleScope scope(thread);
  Object foo_name(&scope, thread->runtime()->newStrFromCStr("foo"));
  Object foo_val_opt(&scope, args.getKw(*foo_name));
  Object foo_val(&scope,
                 (foo_val_opt.isError() ? NoneType::object() : *foo_val_opt));
  Tuple tuple(&scope, thread->runtime()->newTuple(2));
  tuple.atPut(0, args.get(0));
  tuple.atPut(1, *foo_val);
  return *tuple;
}

TEST(TrampolinesTest, CallNativeFunctionReceivesPositionalAndKeywordArgument) {
  Runtime runtime;
  HandleScope scope;

  // Create the builtin kw function
  Function callee(&scope, runtime.newFunction());
  callee.setEntryKw(nativeTrampolineKw<returnsPositionalAndKeywordArgument>);

  // Set up a code object that calls the builtin with (1234, foo='bar')
  Object name(&scope, Str::empty());
  Code code(&scope, runtime.newEmptyCode(name));
  Tuple consts(&scope, runtime.newTuple(4));
  consts.atPut(0, *callee);
  consts.atPut(1, SmallInt::fromWord(1234));
  consts.atPut(2, runtime.newStrFromCStr("bar"));
  Tuple kw_tuple(&scope, runtime.newTuple(1));
  kw_tuple.atPut(0, runtime.newStrFromCStr("foo"));
  consts.atPut(3, *kw_tuple);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST, 0, LOAD_CONST,       1, LOAD_CONST,   2,
                           LOAD_CONST, 3, CALL_FUNCTION_KW, 2, RETURN_VALUE, 0};
  code.setCode(runtime.newBytesWithAll(bytecode));
  code.setStacksize(4);

  // Execute the code and make sure we get back the result we expect
  RawObject result = Thread::current()->run(code);
  ASSERT_TRUE(result.isTuple());
  Tuple tuple(&scope, result);
  ASSERT_EQ(tuple.length(), 2);
  EXPECT_TRUE(isIntEqualsWord(tuple.at(0), 1234));
  EXPECT_TRUE(Str::cast(tuple.at(1)).equalsCStr("bar"));
}

// test "builtin-kw" func that returns a list of first position arg
// and value of kw arguments 'foo' and 'bar'
static RawObject returnsPositionalAndTwoKeywordArguments(Thread* thread,
                                                         Frame* frame,
                                                         word argc) {
  Runtime* runtime = thread->runtime();
  KwArguments args(frame, argc);
  HandleScope scope;
  Object foo_name(&scope, runtime->newStrFromCStr("foo"));
  Object bar_name(&scope, runtime->newStrFromCStr("bar"));
  Tuple tuple(&scope, runtime->newTuple(3));
  tuple.atPut(0, args.get(0));
  Object foo_val(&scope, args.getKw(*foo_name));
  tuple.atPut(1, (foo_val.isError() ? NoneType::object() : *foo_val));
  Object bar_val(&scope, args.getKw(*bar_name));
  tuple.atPut(2, (bar_val.isError() ? NoneType::object() : *bar_val));
  return *tuple;
}

TEST(TrampolinesTest,
     CallNativeFunctionReceivesPositionalAndTwoKeywordArguments) {
  Runtime runtime;
  HandleScope scope;

  // Create the builtin 'multi-kw' function
  Function callee(&scope, runtime.newFunction());
  callee.setEntryKw(
      nativeTrampolineKw<returnsPositionalAndTwoKeywordArguments>);

  // Code object that calls func with (1234, (foo='foo_val', bar='bar_val'))
  Object name(&scope, Str::empty());
  Code code(&scope, runtime.newEmptyCode(name));
  Tuple consts(&scope, runtime.newTuple(5));
  consts.atPut(0, *callee);
  consts.atPut(1, SmallInt::fromWord(1234));
  consts.atPut(2, runtime.newStrFromCStr("foo_val"));
  consts.atPut(3, runtime.newStrFromCStr("bar_val"));
  Tuple kw_tuple(&scope, runtime.newTuple(2));
  kw_tuple.atPut(0, runtime.newStrFromCStr("foo"));
  kw_tuple.atPut(1, runtime.newStrFromCStr("bar"));
  consts.atPut(4, *kw_tuple);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST,   0, LOAD_CONST, 1, LOAD_CONST,       2,
                           LOAD_CONST,   3, LOAD_CONST, 4, CALL_FUNCTION_KW, 3,
                           RETURN_VALUE, 0};
  code.setCode(runtime.newBytesWithAll(bytecode));
  code.setStacksize(5);

  // Execute the code and make sure we get back the result we expect
  RawObject result = Thread::current()->run(code);
  ASSERT_TRUE(result.isTuple());
  Tuple tuple(&scope, result);
  ASSERT_EQ(tuple.length(), 3);
  EXPECT_TRUE(isIntEqualsWord(tuple.at(0), 1234));
  EXPECT_TRUE(isStrEqualsCStr(tuple.at(1), "foo_val"));
  EXPECT_TRUE(isStrEqualsCStr(tuple.at(2), "bar_val"));
}

static RawObject builtinReturnSecondArg(Thread* /* thread */, Frame* frame,
                                        word nargs) {
  Arguments args(frame, nargs);
  return args.get(1);
}

static void createAndPatchBuiltinReturnSecondArg(Runtime* runtime) {
  HandleScope scope;
  // Ensure we have a __main__ module.
  ASSERT_FALSE(runFromCStr(runtime, "").isError());
  Module main(&scope, findModule(runtime, "__main__"));
  runtime->moduleAddBuiltinFunction(main, SymbolId::kDummy,
                                    builtinReturnSecondArg);
  ASSERT_FALSE(runFromCStr(runtime, R"(
@_patch
def dummy(first, second):
  pass
)")
                   .isError());
}

TEST(TrampolinesTest, BuiltinTrampolineKwPassesKwargs) {
  Runtime runtime;
  HandleScope scope;
  createAndPatchBuiltinReturnSecondArg(&runtime);
  ASSERT_FALSE(runFromCStr(&runtime, "result = dummy(second=12345, first=None)")
                   .isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 12345));
}

TEST(TrampolinesTest, BuiltinTrampolineKwWithInvalidArgRaisesTypeError) {
  Runtime runtime;
  createAndPatchBuiltinReturnSecondArg(&runtime);
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "dummy(third=3, first=1)"),
                            LayoutId::kTypeError,
                            "TypeError: invalid keyword argument supplied"));
}

TEST(TrampolinesTest, InterpreterClosureUsesArgOverCellValue) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  // Create code object
  word nlocals = 1;
  Tuple varnames(&scope, runtime.newTuple(nlocals));
  Tuple cellvars(&scope, runtime.newTuple(1));
  Str bar(&scope, runtime.internStrFromCStr(thread, "bar"));
  varnames.atPut(0, *bar);
  cellvars.atPut(0, *bar);
  const byte bytecode[] = {LOAD_CLOSURE, 0, LOAD_DEREF, 0, RETURN_VALUE, 0};
  Bytes bc(&scope, runtime.newBytesWithAll(bytecode));
  Tuple empty_tuple(&scope, runtime.newTuple(0));
  Object empty_str(&scope, Str::empty());
  Object empty_bytes(&scope, Bytes::empty());
  Code code(&scope,
            runtime.newCode(1, 0, nlocals, 0, 0, bc, empty_tuple, empty_tuple,
                            varnames, empty_tuple, cellvars, empty_str,
                            empty_str, 0, empty_bytes));
  ASSERT_TRUE(!code.cell2arg().isNoneType());

  Object qualname(&scope, runtime.newStrFromCStr("foo"));
  Tuple closure_tuple(&scope, runtime.newTuple(1));
  closure_tuple.atPut(0, runtime.newInt(99));
  Object none(&scope, NoneType::object());
  Dict globals(&scope, runtime.newDict());
  Dict builtins(&scope, runtime.newDict());
  Function foo(&scope,
               Interpreter::makeFunction(thread, qualname, code, closure_tuple,
                                         none, none, none, globals, builtins));

  Object argument(&scope, runtime.newInt(3));
  EXPECT_TRUE(isIntEqualsWord(
      Interpreter::callFunction1(thread, thread->currentFrame(), foo, argument),
      3));
}

TEST(TrampolinesTest, InterpreterClosureUsesCellValue) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  // Create code object
  word nlocals = 2;
  Tuple consts(&scope, runtime.newTuple(1));
  consts.atPut(0, runtime.newInt(10));
  Tuple varnames(&scope, runtime.newTuple(nlocals));
  Tuple cellvars(&scope, runtime.newTuple(2));
  Str bar(&scope, runtime.internStrFromCStr(thread, "bar"));
  Str baz(&scope, runtime.internStrFromCStr(thread, "baz"));
  Str foobar(&scope, runtime.internStrFromCStr(thread, "foobar"));
  varnames.atPut(0, *bar);
  varnames.atPut(1, *baz);
  cellvars.atPut(0, *foobar);
  cellvars.atPut(1, *bar);
  const byte bytecode[] = {LOAD_CONST, 0, STORE_DEREF,  0,
                           LOAD_DEREF, 0, RETURN_VALUE, 0};
  Bytes bc(&scope, runtime.newBytesWithAll(bytecode));
  Object empty_str(&scope, Str::empty());
  Object empty_bytes(&scope, Bytes::empty());
  Tuple empty_tuple(&scope, runtime.newTuple(0));
  Code code(&scope,
            runtime.newCode(1, 0, nlocals, 0, 0, bc, consts, empty_tuple,
                            varnames, empty_tuple, cellvars, empty_str,
                            empty_str, 0, empty_bytes));
  ASSERT_TRUE(!code.cell2arg().isNoneType());

  // Create a function
  ASSERT_FALSE(runFromCStr(&runtime, R"(
def foo(bar): pass
)")
                   .isError());
  Function foo(&scope, moduleAt(&runtime, "__main__", "foo"));
  foo.setEntry(interpreterTrampoline);
  foo.setCode(*code);

  // Run function
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = foo(1)
)")
                   .isError());
  Object result(&scope, testing::moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 10));
}

TEST(TrampolinesTest, ExplodeCallWithBadKeywordFails) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
def take_kwargs(a): pass

kwargs = {12: 34}
take_kwargs(**kwargs)
)"),
                            LayoutId::kTypeError, "keywords must be strings"));
}

TEST(TrampolinesTest, ExplodeCallWithZeroKeywords) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
def foo(a=10): return a
result = foo(**{})
)")
                   .isError());
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_EQ(result, SmallInt::fromWord(10));
}

TEST(TrampolinesTest, ExtensionModuleNoArgReceivesNoArgsReturns) {
  binaryfunc func = [](PyObject*, PyObject*) -> PyObject* {
    return ApiHandle::newReference(Thread::current(), SmallInt::fromWord(123));
  };

  Runtime runtime;
  HandleScope scope;
  Str mod_name(&scope, runtime.newStrFromCStr("foobar"));
  Function callee(&scope, runtime.newFunction());
  callee.setModule(runtime.newModule(mod_name));
  callee.setCode(runtime.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntry(moduleTrampolineNoArgs);

  // Set up a code object that calls the function without arguments
  Object name(&scope, Str::empty());
  Code code(&scope, runtime.newEmptyCode(name));
  Tuple consts(&scope, runtime.newTuple(1));
  consts.atPut(0, *callee);
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, CALL_FUNCTION, 0, RETURN_VALUE, 0};
  code.setCode(runtime.newBytesWithAll(bytecode));
  code.setStacksize(1);

  // Execute the code and make sure we get back the result we expect
  Object result(&scope, Thread::current()->run(code));
  EXPECT_TRUE(isIntEqualsWord(*result, 123));
}

TEST(TrampolinesTest, ExtensionModuleNoArgReceivesArgsRaisesTypeError) {
  binaryfunc func = [](PyObject*, PyObject*) -> PyObject* {
    return ApiHandle::newReference(Thread::current(), SmallInt::fromWord(123));
  };

  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Str mod_name(&scope, runtime.newStrFromCStr("foobar"));
  Function callee(&scope, runtime.newFunction());
  callee.setModule(runtime.newModule(mod_name));
  callee.setCode(runtime.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntry(moduleTrampolineNoArgs);

  // Set up a code object that calls the function with a single argument.
  Object name(&scope, Str::empty());
  Code code(&scope, runtime.newEmptyCode(name));
  Tuple consts(&scope, runtime.newTuple(2));
  consts.atPut(0, *callee);
  consts.atPut(1, SmallInt::fromWord(1111));
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST,    0, LOAD_CONST,   1,
                           CALL_FUNCTION, 1, RETURN_VALUE, 0};
  code.setCode(runtime.newBytesWithAll(bytecode));
  code.setStacksize(2);

  // Execute the code and make sure we get back the result we expect
  EXPECT_TRUE(raisedWithStr(thread->run(code), LayoutId::kTypeError,
                            "function takes no arguments"));
}

TEST(TrampolinesTest, ExtensionModuleNoArgReturnsNullRaisesSystemError) {
  binaryfunc func = [](PyObject*, PyObject*) -> PyObject* { return nullptr; };

  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope;
  Str mod_name(&scope, runtime.newStrFromCStr("foobar"));
  Function callee(&scope, runtime.newFunction());
  callee.setModule(runtime.newModule(mod_name));
  callee.setCode(runtime.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntry(moduleTrampolineNoArgs);

  // Set up a code object that calls the function without arguments
  Object name(&scope, Str::empty());
  Code code(&scope, runtime.newEmptyCode(name));
  Tuple consts(&scope, runtime.newTuple(1));
  consts.atPut(0, *callee);
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, CALL_FUNCTION, 0, RETURN_VALUE, 0};
  code.setCode(runtime.newBytesWithAll(bytecode));
  code.setStacksize(1);

  // Execute the code and make sure we get back the result we expect
  EXPECT_TRUE(raisedWithStr(thread->run(code), LayoutId::kSystemError,
                            "NULL return without exception set"));
}

TEST(TrampolinesTest, ExtensionModuleNoArgReceivesKwArgsRaisesTypeError) {
  binaryfunc func = [](PyObject*, PyObject*) -> PyObject* {
    return ApiHandle::newReference(Thread::current(), SmallInt::fromWord(123));
  };

  Runtime runtime;
  HandleScope scope;
  Str mod_name(&scope, runtime.newStrFromCStr("foobar"));
  Function callee(&scope, runtime.newFunction());
  callee.setModule(runtime.newModule(mod_name));
  callee.setCode(runtime.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntryKw(moduleTrampolineNoArgsKw);

  // Set up a code object that calls the builtin with (foo='bar')
  Object name(&scope, Str::empty());
  Code code(&scope, runtime.newEmptyCode(name));
  Tuple consts(&scope, runtime.newTuple(3));
  consts.atPut(0, *callee);
  consts.atPut(1, runtime.newStrFromCStr("bar"));
  Tuple kw_tuple(&scope, runtime.newTuple(1));
  kw_tuple.atPut(0, runtime.newStrFromCStr("foo"));
  consts.atPut(2, *kw_tuple);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST,       0, LOAD_CONST,   1, LOAD_CONST, 2,
                           CALL_FUNCTION_KW, 1, RETURN_VALUE, 0};
  code.setCode(runtime.newBytesWithAll(bytecode));
  code.setStacksize(3);

  // Execute the code and make sure we get back the result we expect
  EXPECT_TRUE(raised(Thread::current()->run(code), LayoutId::kTypeError));
}

TEST(TrampolinesTest, ExtensionModuleNoArgReceivesZeroKwArgsReturns) {
  binaryfunc func = [](PyObject*, PyObject*) -> PyObject* {
    return ApiHandle::newReference(Thread::current(), SmallInt::fromWord(123));
  };

  Runtime runtime;
  HandleScope scope;
  Str mod_name(&scope, runtime.newStrFromCStr("foobar"));
  Function callee(&scope, runtime.newFunction());
  callee.setModule(runtime.newModule(mod_name));
  callee.setCode(runtime.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntryKw(moduleTrampolineNoArgsKw);

  // Set up a code object that calls the builtin with (foo='bar')
  Object name(&scope, Str::empty());
  Code code(&scope, runtime.newEmptyCode(name));
  Tuple consts(&scope, runtime.newTuple(2));
  consts.atPut(0, *callee);
  Tuple kw_tuple(&scope, runtime.newTuple(0));
  consts.atPut(1, *kw_tuple);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST,       0, LOAD_CONST,   1,
                           CALL_FUNCTION_KW, 0, RETURN_VALUE, 0};
  code.setCode(runtime.newBytesWithAll(bytecode));
  code.setStacksize(2);

  // Execute the code and make sure we get back the result we expect
  Object result(&scope, Thread::current()->run(code));
  EXPECT_TRUE(isIntEqualsWord(*result, 123));
}

TEST(TrampolinesTest, ExtensionModuleNoArgReceivesVariableArgsRaisesTypeError) {
  binaryfunc func = [](PyObject*, PyObject*) -> PyObject* {
    return ApiHandle::newReference(Thread::current(), SmallInt::fromWord(123));
  };

  Runtime runtime;
  HandleScope scope;
  Str mod_name(&scope, runtime.newStrFromCStr("foobar"));
  Function callee(&scope, runtime.newFunction());
  callee.setModule(runtime.newModule(mod_name));
  callee.setCode(runtime.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntryEx(moduleTrampolineNoArgsEx);

  // Set up a code object that calls with (*(10))
  Object name(&scope, Str::empty());
  Code code(&scope, runtime.newEmptyCode(name));
  Tuple consts(&scope, runtime.newTuple(2));
  consts.atPut(0, *callee);
  Tuple arg_tuple(&scope, runtime.newTuple(1));
  arg_tuple.atPut(0, SmallInt::fromWord(10));
  consts.atPut(1, *arg_tuple);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST,       0, LOAD_CONST,   1,
                           CALL_FUNCTION_EX, 0, RETURN_VALUE, 0};
  code.setCode(runtime.newBytesWithAll(bytecode));
  code.setStacksize(2);

  // Execute the code and make sure we get back the result we expect
  EXPECT_TRUE(raised(Thread::current()->run(code), LayoutId::kTypeError));
}

TEST(TrampolinesTest, ExtensionModuleNoArgReceivesVariableArgsReturns) {
  binaryfunc func = [](PyObject*, PyObject*) -> PyObject* {
    return ApiHandle::newReference(Thread::current(), SmallInt::fromWord(123));
  };

  Runtime runtime;
  HandleScope scope;
  Str mod_name(&scope, runtime.newStrFromCStr("foobar"));
  Function callee(&scope, runtime.newFunction());
  callee.setModule(runtime.newModule(mod_name));
  callee.setCode(runtime.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntryEx(moduleTrampolineNoArgsEx);

  // Set up a code object that calls with (*())
  Object name(&scope, Str::empty());
  Code code(&scope, runtime.newEmptyCode(name));
  Tuple consts(&scope, runtime.newTuple(2));
  consts.atPut(0, *callee);
  Tuple arg_tuple(&scope, runtime.newTuple(0));
  consts.atPut(1, *arg_tuple);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST,       0, LOAD_CONST,   1,
                           CALL_FUNCTION_EX, 0, RETURN_VALUE, 0};
  code.setCode(runtime.newBytesWithAll(bytecode));
  code.setStacksize(2);

  // Execute the code and make sure we get back the result we expect
  Object result(&scope, Thread::current()->run(code));
  EXPECT_TRUE(isIntEqualsWord(*result, 123));
}

TEST(TrampolinesTest, ExtensionModuleOneArgReceivesNoArgsRaisesTypeError) {
  binaryfunc func = [](PyObject*, PyObject*) -> PyObject* {
    return ApiHandle::newReference(Thread::current(), SmallInt::fromWord(123));
  };

  Runtime runtime;
  HandleScope scope;
  Str mod_name(&scope, runtime.newStrFromCStr("foobar"));
  Function callee(&scope, runtime.newFunction());
  callee.setModule(runtime.newModule(mod_name));
  callee.setCode(runtime.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntry(moduleTrampolineOneArg);

  // Set up a code object that calls the function without arguments
  Object name(&scope, Str::empty());
  Code code(&scope, runtime.newEmptyCode(name));
  Tuple consts(&scope, runtime.newTuple(1));
  consts.atPut(0, *callee);
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, CALL_FUNCTION, 0, RETURN_VALUE, 0};
  code.setCode(runtime.newBytesWithAll(bytecode));
  code.setStacksize(1);

  // Execute the code and make sure we get back the result we expect
  Object result(&scope, Thread::current()->run(code));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(TrampolinesTest, ExtensionModuleOneArgReceivesOneArgReturns) {
  binaryfunc func = [](PyObject*, PyObject* arg) -> PyObject* { return arg; };

  Runtime runtime;
  HandleScope scope;
  Str mod_name(&scope, runtime.newStrFromCStr("foobar"));
  Function callee(&scope, runtime.newFunction());
  callee.setModule(runtime.newModule(mod_name));
  callee.setCode(runtime.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntry(moduleTrampolineOneArg);

  // Set up a code object that calls the function with a single argument.
  Object name(&scope, Str::empty());
  Code code(&scope, runtime.newEmptyCode(name));
  Tuple consts(&scope, runtime.newTuple(2));
  consts.atPut(0, *callee);
  consts.atPut(1, SmallInt::fromWord(1111));
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST,    0, LOAD_CONST,   1,
                           CALL_FUNCTION, 1, RETURN_VALUE, 0};
  code.setCode(runtime.newBytesWithAll(bytecode));
  code.setStacksize(2);

  // Execute the code and make sure we get back the result we expect
  RawObject result = Thread::current()->run(code);
  EXPECT_TRUE(isIntEqualsWord(result, 1111));
}

TEST(TrampolinesTest,
     ExtensionModuleOneArgReceivesMultipleArgsRaisesTypeError) {
  binaryfunc func = [](PyObject*, PyObject* arg) -> PyObject* { return arg; };

  Runtime runtime;
  HandleScope scope;
  Str mod_name(&scope, runtime.newStrFromCStr("foobar"));
  Function callee(&scope, runtime.newFunction());
  callee.setModule(runtime.newModule(mod_name));
  callee.setCode(runtime.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntry(moduleTrampolineOneArg);

  // Set up a code object that calls the function with (123, 456)
  Object name(&scope, Str::empty());
  Code code(&scope, runtime.newEmptyCode(name));
  Tuple consts(&scope, runtime.newTuple(3));
  consts.atPut(0, *callee);
  consts.atPut(1, SmallInt::fromWord(123));
  consts.atPut(2, SmallInt::fromWord(456));
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST,    0, LOAD_CONST,   1, LOAD_CONST, 2,
                           CALL_FUNCTION, 2, RETURN_VALUE, 0};
  code.setCode(runtime.newBytesWithAll(bytecode));
  code.setStacksize(3);

  // Execute the code and make sure we get back the result we expect
  Object result(&scope, Thread::current()->run(code));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(TrampolinesTest, ExtensionModuleOneArgReturnsNullRaisesSystemError) {
  binaryfunc func = [](PyObject*, PyObject*) -> PyObject* { return nullptr; };

  Runtime runtime;
  HandleScope scope;
  Str mod_name(&scope, runtime.newStrFromCStr("foobar"));
  Function callee(&scope, runtime.newFunction());
  callee.setModule(runtime.newModule(mod_name));
  callee.setCode(runtime.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntry(moduleTrampolineOneArg);

  // Set up a code object that calls the function without arguments
  Object name(&scope, Str::empty());
  Code code(&scope, runtime.newEmptyCode(name));
  Tuple consts(&scope, runtime.newTuple(2));
  consts.atPut(0, *callee);
  consts.atPut(1, SmallInt::fromWord(1111));
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST,    0, LOAD_CONST,   1,
                           CALL_FUNCTION, 1, RETURN_VALUE, 0};
  code.setCode(runtime.newBytesWithAll(bytecode));
  code.setStacksize(2);

  // Execute the code and make sure we get back the result we expect
  Object result(&scope, Thread::current()->run(code));
  EXPECT_TRUE(raised(*result, LayoutId::kSystemError));
}

TEST(TrampolinesTest, ExtensionModuleOneArgReceivesOneArgAndZeroKwArgsReturns) {
  binaryfunc func = [](PyObject*, PyObject* arg) -> PyObject* { return arg; };

  Runtime runtime;
  HandleScope scope;
  Str mod_name(&scope, runtime.newStrFromCStr("foobar"));
  Function callee(&scope, runtime.newFunction());
  callee.setModule(runtime.newModule(mod_name));
  callee.setCode(runtime.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntryKw(moduleTrampolineOneArgKw);

  // Set up a code object that calls the builtin with (1111, {})
  Object name(&scope, Str::empty());
  Code code(&scope, runtime.newEmptyCode(name));
  Tuple consts(&scope, runtime.newTuple(3));
  consts.atPut(0, *callee);
  consts.atPut(1, SmallInt::fromWord(1111));
  Tuple kw_tuple(&scope, runtime.newTuple(0));
  consts.atPut(2, *kw_tuple);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST,       0, LOAD_CONST,   1, LOAD_CONST, 2,
                           CALL_FUNCTION_KW, 1, RETURN_VALUE, 0};
  code.setCode(runtime.newBytesWithAll(bytecode));
  code.setStacksize(3);

  // Execute the code and make sure we get back the result we expect
  RawObject result = Thread::current()->run(code);
  EXPECT_TRUE(isIntEqualsWord(result, 1111));
}

TEST(TrampolinesTest, ExtensionModuleOneArgReceivesKwArgsRaisesTypeError) {
  binaryfunc func = [](PyObject*, PyObject*) -> PyObject* {
    return ApiHandle::newReference(Thread::current(), SmallInt::fromWord(123));
  };

  Runtime runtime;
  HandleScope scope;
  Str mod_name(&scope, runtime.newStrFromCStr("foobar"));
  Function callee(&scope, runtime.newFunction());
  callee.setModule(runtime.newModule(mod_name));
  callee.setCode(runtime.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntryKw(moduleTrampolineOneArgKw);

  // Set up a code object that calls the builtin with (1111, foo='bar')
  Object name(&scope, Str::empty());
  Code code(&scope, runtime.newEmptyCode(name));
  Tuple consts(&scope, runtime.newTuple(4));
  consts.atPut(0, *callee);
  Tuple kw_tuple(&scope, runtime.newTuple(1));
  consts.atPut(1, SmallInt::fromWord(1111));
  consts.atPut(2, runtime.newStrFromCStr("bar"));
  kw_tuple.atPut(0, runtime.newStrFromCStr("foo"));
  consts.atPut(3, *kw_tuple);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST, 0, LOAD_CONST,       1, LOAD_CONST,   2,
                           LOAD_CONST, 3, CALL_FUNCTION_KW, 2, RETURN_VALUE, 0};
  code.setCode(runtime.newBytesWithAll(bytecode));
  code.setStacksize(4);

  // Execute the code and make sure we get back the result we expect
  Object result(&scope, Thread::current()->run(code));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(TrampolinesTest, ExtensionModuleOneArgReceivesOneArgExReturns) {
  binaryfunc func = [](PyObject*, PyObject* arg) -> PyObject* { return arg; };

  Runtime runtime;
  HandleScope scope;
  Str mod_name(&scope, runtime.newStrFromCStr("foobar"));
  Function callee(&scope, runtime.newFunction());
  callee.setModule(runtime.newModule(mod_name));
  callee.setCode(runtime.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntryEx(moduleTrampolineOneArgEx);

  // Set up a code object that calls with (*(10))
  Object name(&scope, Str::empty());
  Code code(&scope, runtime.newEmptyCode(name));
  Tuple consts(&scope, runtime.newTuple(2));
  consts.atPut(0, *callee);
  Tuple arg_tuple(&scope, runtime.newTuple(1));
  arg_tuple.atPut(0, SmallInt::fromWord(1111));
  consts.atPut(1, *arg_tuple);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST,       0, LOAD_CONST,   1,
                           CALL_FUNCTION_EX, 0, RETURN_VALUE, 0};
  code.setCode(runtime.newBytesWithAll(bytecode));
  code.setStacksize(2);

  // Execute the code and make sure we get back the result we expect
  RawObject result = Thread::current()->run(code);
  EXPECT_TRUE(isIntEqualsWord(result, 1111));
}

TEST(TrampolinesTest, ExtensionModuleOneArgReceivesOneArgAndEmptyKwReturns) {
  binaryfunc func = [](PyObject*, PyObject* arg) -> PyObject* { return arg; };

  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope;
  Str mod_name(&scope, runtime.newStrFromCStr("foobar"));
  Function callee(&scope, runtime.newFunction());
  callee.setModule(runtime.newModule(mod_name));
  callee.setCode(runtime.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntryEx(moduleTrampolineOneArgEx);

  // Set up a code object that calls with (*(10), {})
  Object name(&scope, Str::empty());
  Code code(&scope, runtime.newEmptyCode(name));
  Tuple consts(&scope, runtime.newTuple(3));
  consts.atPut(0, *callee);
  Tuple arg_tuple(&scope, runtime.newTuple(1));
  arg_tuple.atPut(0, SmallInt::fromWord(1111));
  consts.atPut(1, *arg_tuple);
  Dict kw_dict(&scope, runtime.newDict());
  consts.atPut(2, *kw_dict);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST,       0, LOAD_CONST,   1, LOAD_CONST, 2,
                           CALL_FUNCTION_EX, 1, RETURN_VALUE, 0};
  code.setCode(runtime.newBytesWithAll(bytecode));
  code.setStacksize(3);

  // Execute the code and make sure we get back the result we expect
  RawObject result = thread->run(code);
  EXPECT_TRUE(isIntEqualsWord(result, 1111));
}

TEST(TrampolinesTest, ExtensionModuleOneArgReceivesOneArgAndKwRaisesTypeError) {
  binaryfunc func = [](PyObject*, PyObject*) -> PyObject* {
    return ApiHandle::newReference(Thread::current(), SmallInt::fromWord(123));
  };

  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Str mod_name(&scope, runtime.newStrFromCStr("foobar"));
  Function callee(&scope, runtime.newFunction());
  callee.setModule(runtime.newModule(mod_name));
  callee.setCode(runtime.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntryEx(moduleTrampolineOneArgEx);

  // Set up a code object that calls with (*(10), {2:3})
  Object name(&scope, Str::empty());
  Code code(&scope, runtime.newEmptyCode(name));
  Tuple consts(&scope, runtime.newTuple(3));
  consts.atPut(0, *callee);
  Tuple arg_tuple(&scope, runtime.newTuple(1));
  arg_tuple.atPut(0, SmallInt::fromWord(1111));
  consts.atPut(1, *arg_tuple);
  Dict kw_dict(&scope, runtime.newDict());
  Object key(&scope, SmallInt::fromWord(2));
  Object value(&scope, SmallInt::fromWord(3));
  runtime.dictAtPut(thread, kw_dict, key, value);
  consts.atPut(2, *kw_dict);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST,       0, LOAD_CONST,   1, LOAD_CONST, 2,
                           CALL_FUNCTION_EX, 1, RETURN_VALUE, 0};
  code.setCode(runtime.newBytesWithAll(bytecode));
  code.setStacksize(3);

  // Execute the code and make sure we get back the result we expect
  Object result(&scope, Thread::current()->run(code));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(TrampolinesTest, ExtensionModuleVarArgReceivesNoArgsReturns) {
  binaryfunc func = [](PyObject*, PyObject*) -> PyObject* {
    return ApiHandle::newReference(Thread::current(), SmallInt::fromWord(123));
  };

  Runtime runtime;
  HandleScope scope;
  Str mod_name(&scope, runtime.newStrFromCStr("foobar"));
  Function callee(&scope, runtime.newFunction());
  callee.setModule(runtime.newModule(mod_name));
  callee.setCode(runtime.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntry(moduleTrampolineVarArgs);

  // Set up a code object that calls the function without arguments
  Object name(&scope, Str::empty());
  Code code(&scope, runtime.newEmptyCode(name));
  Tuple consts(&scope, runtime.newTuple(1));
  consts.atPut(0, *callee);
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, CALL_FUNCTION, 0, RETURN_VALUE, 0};
  code.setCode(runtime.newBytesWithAll(bytecode));
  code.setStacksize(1);

  // Execute the code and make sure we get back the result we expect
  RawObject result = Thread::current()->run(code);
  EXPECT_TRUE(isIntEqualsWord(result, 123));
}

TEST(TrampolinesTest, ExtensionModuleVarArgReceivesArgsReturns) {
  binaryfunc func = [](PyObject*, PyObject* args) -> PyObject* {
    HandleScope scope;
    Tuple arg_tuple(&scope, ApiHandle::fromPyObject(args)->asObject());
    return ApiHandle::newReference(Thread::current(), arg_tuple.at(0));
  };

  Runtime runtime;
  HandleScope scope;
  Str mod_name(&scope, runtime.newStrFromCStr("foobar"));
  Function callee(&scope, runtime.newFunction());
  callee.setModule(runtime.newModule(mod_name));
  callee.setCode(runtime.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntry(moduleTrampolineVarArgs);

  // Set up a code object that calls the function with a single argument.
  Object name(&scope, Str::empty());
  Code code(&scope, runtime.newEmptyCode(name));
  Tuple consts(&scope, runtime.newTuple(2));
  consts.atPut(0, *callee);
  consts.atPut(1, SmallInt::fromWord(1111));
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST,    0, LOAD_CONST,   1,
                           CALL_FUNCTION, 1, RETURN_VALUE, 0};
  code.setCode(runtime.newBytesWithAll(bytecode));
  code.setStacksize(2);

  // Execute the code and make sure we get back the result we expect
  RawObject result = Thread::current()->run(code);
  EXPECT_TRUE(isIntEqualsWord(result, 1111));
}

TEST(TrampolinesTest, ExtensionModuleVarArgReturnsNullRaisesSystemError) {
  binaryfunc func = [](PyObject*, PyObject*) -> PyObject* { return nullptr; };

  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope;
  Str mod_name(&scope, runtime.newStrFromCStr("foobar"));
  Function callee(&scope, runtime.newFunction());
  callee.setModule(runtime.newModule(mod_name));
  callee.setCode(runtime.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntry(moduleTrampolineVarArgs);

  // Set up a code object that calls the function without arguments
  Object name(&scope, Str::empty());
  Code code(&scope, runtime.newEmptyCode(name));
  Tuple consts(&scope, runtime.newTuple(1));
  consts.atPut(0, *callee);
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, CALL_FUNCTION, 0, RETURN_VALUE, 0};
  code.setCode(runtime.newBytesWithAll(bytecode));
  code.setStacksize(1);

  // Execute the code and make sure we get back the result we expect
  EXPECT_TRUE(raisedWithStr(thread->run(code), LayoutId::kSystemError,
                            "NULL return without exception set"));
}

TEST(TrampolinesTest, ExtensionModuleVarArgReceivesZeroKwArgsReturns) {
  binaryfunc func = [](PyObject*, PyObject* args) -> PyObject* {
    HandleScope scope;
    Tuple arg_tuple(&scope, ApiHandle::fromPyObject(args)->asObject());
    return ApiHandle::newReference(Thread::current(), arg_tuple.at(0));
  };

  Runtime runtime;
  HandleScope scope;
  Str mod_name(&scope, runtime.newStrFromCStr("foobar"));
  Function callee(&scope, runtime.newFunction());
  callee.setModule(runtime.newModule(mod_name));
  callee.setCode(runtime.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntryKw(moduleTrampolineVarArgsKw);

  // Set up a code object that calls the builtin with (1111, {})
  Object name(&scope, Str::empty());
  Code code(&scope, runtime.newEmptyCode(name));
  Tuple consts(&scope, runtime.newTuple(3));
  consts.atPut(0, *callee);
  consts.atPut(1, SmallInt::fromWord(1111));
  Tuple kw_tuple(&scope, runtime.newTuple(0));
  consts.atPut(2, *kw_tuple);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST,       0, LOAD_CONST,   1, LOAD_CONST, 2,
                           CALL_FUNCTION_KW, 1, RETURN_VALUE, 0};
  code.setCode(runtime.newBytesWithAll(bytecode));
  code.setStacksize(3);

  // Execute the code and make sure we get back the result we expect
  RawObject result = Thread::current()->run(code);
  EXPECT_TRUE(isIntEqualsWord(result, 1111));
}

TEST(TrampolinesTest, ExtensionModuleVarArgReceivesKwArgsRaisesTypeError) {
  binaryfunc func = [](PyObject*, PyObject*) -> PyObject* {
    return ApiHandle::newReference(Thread::current(), SmallInt::fromWord(123));
  };

  Runtime runtime;
  HandleScope scope;
  Str mod_name(&scope, runtime.newStrFromCStr("foobar"));
  Function callee(&scope, runtime.newFunction());
  callee.setModule(runtime.newModule(mod_name));
  callee.setCode(runtime.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntryKw(moduleTrampolineVarArgsKw);

  // Set up a code object that calls the builtin with (1111, foo='bar')
  Object name(&scope, Str::empty());
  Code code(&scope, runtime.newEmptyCode(name));
  Tuple consts(&scope, runtime.newTuple(4));
  consts.atPut(0, *callee);
  Tuple kw_tuple(&scope, runtime.newTuple(1));
  consts.atPut(1, SmallInt::fromWord(1111));
  consts.atPut(2, runtime.newStrFromCStr("bar"));
  kw_tuple.atPut(0, runtime.newStrFromCStr("foo"));
  consts.atPut(3, *kw_tuple);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST, 0, LOAD_CONST,       1, LOAD_CONST,   2,
                           LOAD_CONST, 3, CALL_FUNCTION_KW, 2, RETURN_VALUE, 0};
  code.setCode(runtime.newBytesWithAll(bytecode));
  code.setStacksize(4);

  // Execute the code and make sure we get back the result we expect
  Object result(&scope, Thread::current()->run(code));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(TrampolinesTest, ExtensionModuleVarArgReceivesVarArgsReturns) {
  binaryfunc func = [](PyObject*, PyObject* args) -> PyObject* {
    HandleScope scope;
    Tuple arg_tuple(&scope, ApiHandle::fromPyObject(args)->asObject());
    return ApiHandle::newReference(Thread::current(), arg_tuple.at(0));
  };

  Runtime runtime;
  HandleScope scope;
  Str mod_name(&scope, runtime.newStrFromCStr("foobar"));
  Function callee(&scope, runtime.newFunction());
  callee.setModule(runtime.newModule(mod_name));
  callee.setCode(runtime.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntryEx(moduleTrampolineVarArgsEx);

  // Set up a code object that calls with (*(10))
  Object name(&scope, Str::empty());
  Code code(&scope, runtime.newEmptyCode(name));
  Tuple consts(&scope, runtime.newTuple(2));
  consts.atPut(0, *callee);
  Tuple arg_tuple(&scope, runtime.newTuple(1));
  arg_tuple.atPut(0, SmallInt::fromWord(1111));
  consts.atPut(1, *arg_tuple);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST,       0, LOAD_CONST,   1,
                           CALL_FUNCTION_EX, 0, RETURN_VALUE, 0};
  code.setCode(runtime.newBytesWithAll(bytecode));
  code.setStacksize(2);

  // Execute the code and make sure we get back the result we expect
  RawObject result = Thread::current()->run(code);
  EXPECT_TRUE(isIntEqualsWord(result, 1111));
}

TEST(TrampolinesTest, ExtensionModuleVarArgReceivesVarArgsAndEmptyKwReturns) {
  binaryfunc func = [](PyObject*, PyObject* args) -> PyObject* {
    HandleScope scope;
    Tuple arg_tuple(&scope, ApiHandle::fromPyObject(args)->asObject());
    return ApiHandle::newReference(Thread::current(), arg_tuple.at(0));
  };

  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Str mod_name(&scope, runtime.newStrFromCStr("foobar"));
  Function callee(&scope, runtime.newFunction());
  callee.setModule(runtime.newModule(mod_name));
  callee.setCode(runtime.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntryEx(moduleTrampolineVarArgsEx);

  // Set up a code object that calls with (*(10), {})
  Object name(&scope, Str::empty());
  Code code(&scope, runtime.newEmptyCode(name));
  Tuple consts(&scope, runtime.newTuple(3));
  consts.atPut(0, *callee);
  Tuple arg_tuple(&scope, runtime.newTuple(1));
  arg_tuple.atPut(0, SmallInt::fromWord(1111));
  consts.atPut(1, *arg_tuple);
  Dict kw_dict(&scope, runtime.newDict());
  consts.atPut(2, *kw_dict);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST,       0, LOAD_CONST,   1, LOAD_CONST, 2,
                           CALL_FUNCTION_EX, 1, RETURN_VALUE, 0};
  code.setCode(runtime.newBytesWithAll(bytecode));
  code.setStacksize(3);

  // Execute the code and make sure we get back the result we expect
  RawObject result = Thread::current()->run(code);
  EXPECT_TRUE(isIntEqualsWord(result, 1111));
}

TEST(TrampolinesTest,
     ExtensionModuleVarArgReceivesVarArgsAndKwRaisesTypeError) {
  binaryfunc func = [](PyObject*, PyObject*) -> PyObject* {
    return ApiHandle::newReference(Thread::current(), SmallInt::fromWord(123));
  };

  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Str mod_name(&scope, runtime.newStrFromCStr("foobar"));
  Function callee(&scope, runtime.newFunction());
  callee.setModule(runtime.newModule(mod_name));
  callee.setCode(runtime.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntryEx(moduleTrampolineVarArgsEx);

  // Set up a code object that calls with (*(10), {})
  Object name(&scope, Str::empty());
  Code code(&scope, runtime.newEmptyCode(name));
  Tuple consts(&scope, runtime.newTuple(3));
  consts.atPut(0, *callee);
  Tuple arg_tuple(&scope, runtime.newTuple(1));
  arg_tuple.atPut(0, SmallInt::fromWord(1111));
  consts.atPut(1, *arg_tuple);
  Dict kw_dict(&scope, runtime.newDict());
  Object key(&scope, SmallInt::fromWord(2));
  Object value(&scope, SmallInt::fromWord(3));
  runtime.dictAtPut(thread, kw_dict, key, value);
  consts.atPut(2, *kw_dict);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST,       0, LOAD_CONST,   1, LOAD_CONST, 2,
                           CALL_FUNCTION_EX, 1, RETURN_VALUE, 0};
  code.setCode(runtime.newBytesWithAll(bytecode));
  code.setStacksize(3);

  // Execute the code and make sure we get back the result we expect
  Object result(&scope, Thread::current()->run(code));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(TrampolinesTest, ExtensionModuleKeywordArgReceivesNoArgsReturns) {
  ternaryfunc func = [](PyObject*, PyObject*, PyObject*) -> PyObject* {
    return ApiHandle::newReference(Thread::current(), SmallInt::fromWord(123));
  };

  Runtime runtime;
  HandleScope scope;
  Str mod_name(&scope, runtime.newStrFromCStr("foobar"));
  Function callee(&scope, runtime.newFunction());
  callee.setModule(runtime.newModule(mod_name));
  callee.setCode(runtime.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntry(moduleTrampolineKeywords);

  // Set up a code object that calls the function without arguments
  Object name(&scope, Str::empty());
  Code code(&scope, runtime.newEmptyCode(name));
  Tuple consts(&scope, runtime.newTuple(1));
  consts.atPut(0, *callee);
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, CALL_FUNCTION, 0, RETURN_VALUE, 0};
  code.setCode(runtime.newBytesWithAll(bytecode));
  code.setStacksize(1);

  // Execute the code and make sure we get back the result we expect
  RawObject result = Thread::current()->run(code);
  EXPECT_TRUE(isIntEqualsWord(result, 123));
}

TEST(TrampolinesTest, ExtensionModuleKeywordArgReceivesArgsReturns) {
  ternaryfunc func = [](PyObject*, PyObject* args, PyObject*) -> PyObject* {
    HandleScope scope;
    Tuple arg_tuple(&scope, ApiHandle::fromPyObject(args)->asObject());
    return ApiHandle::newReference(Thread::current(), arg_tuple.at(0));
  };

  Runtime runtime;
  HandleScope scope;
  Str mod_name(&scope, runtime.newStrFromCStr("foobar"));
  Function callee(&scope, runtime.newFunction());
  callee.setModule(runtime.newModule(mod_name));
  callee.setCode(runtime.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntry(moduleTrampolineKeywords);

  // Set up a code object that calls the function with a single argument.
  Object name(&scope, Str::empty());
  Code code(&scope, runtime.newEmptyCode(name));
  Tuple consts(&scope, runtime.newTuple(2));
  consts.atPut(0, *callee);
  consts.atPut(1, SmallInt::fromWord(1111));
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST,    0, LOAD_CONST,   1,
                           CALL_FUNCTION, 1, RETURN_VALUE, 0};
  code.setCode(runtime.newBytesWithAll(bytecode));
  code.setStacksize(2);

  // Execute the code and make sure we get back the result we expect
  RawObject result = Thread::current()->run(code);
  EXPECT_TRUE(isIntEqualsWord(result, 1111));
}

TEST(TrampolinesTest, ExtensionModuleKeywordArgReturnsNullRaisesSystemError) {
  ternaryfunc func = [](PyObject*, PyObject*, PyObject*) -> PyObject* {
    return nullptr;
  };

  Runtime runtime;
  HandleScope scope;
  Str mod_name(&scope, runtime.newStrFromCStr("foobar"));
  Function callee(&scope, runtime.newFunction());
  callee.setModule(runtime.newModule(mod_name));
  callee.setCode(runtime.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntry(moduleTrampolineKeywords);

  // Set up a code object that calls the function without arguments
  Object name(&scope, Str::empty());
  Code code(&scope, runtime.newEmptyCode(name));
  Tuple consts(&scope, runtime.newTuple(1));
  consts.atPut(0, *callee);
  code.setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST, 0, CALL_FUNCTION, 0, RETURN_VALUE, 0};
  code.setCode(runtime.newBytesWithAll(bytecode));
  code.setStacksize(1);

  // Execute the code and make sure we get back the result we expect
  Object result(&scope, Thread::current()->run(code));
  EXPECT_TRUE(raisedWithStr(*result, LayoutId::kSystemError,
                            "NULL return without exception set"));
}

TEST(TrampolinesTest, ExtensionModuleKeywordArgReceivesKwArgsReturns) {
  ternaryfunc func = [](PyObject*, PyObject*, PyObject* kwargs) -> PyObject* {
    Thread* thread = Thread::current();
    HandleScope scope(thread);
    Runtime* runtime = thread->runtime();
    Str foo_str(&scope, runtime->newStrFromCStr("foo"));
    Dict keyword_dict(&scope, ApiHandle::fromPyObject(kwargs)->asObject());
    return ApiHandle::newReference(
        thread, runtime->dictAt(thread, keyword_dict, foo_str));
  };

  Runtime runtime;
  HandleScope scope;
  Str mod_name(&scope, runtime.newStrFromCStr("foobar"));
  Function callee(&scope, runtime.newFunction());
  callee.setModule(runtime.newModule(mod_name));
  callee.setCode(runtime.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntryKw(moduleTrampolineKeywordsKw);

  // Set up a code object that calls the builtin with ("bar", foo=1111)
  Object name(&scope, Str::empty());
  Code code(&scope, runtime.newEmptyCode(name));
  Tuple consts(&scope, runtime.newTuple(4));
  consts.atPut(0, *callee);
  consts.atPut(1, runtime.newStrFromCStr("bar"));
  consts.atPut(2, SmallInt::fromWord(1111));
  Tuple kw_tuple(&scope, runtime.newTuple(1));
  kw_tuple.atPut(0, runtime.newStrFromCStr("foo"));
  consts.atPut(3, *kw_tuple);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST, 0, LOAD_CONST,       1, LOAD_CONST,   2,
                           LOAD_CONST, 3, CALL_FUNCTION_KW, 2, RETURN_VALUE, 0};
  code.setCode(runtime.newBytesWithAll(bytecode));
  code.setStacksize(4);

  // Execute the code and make sure we get back the result we expect
  RawObject result = Thread::current()->run(code);
  EXPECT_TRUE(isIntEqualsWord(result, 1111));
}

TEST(TrampolinesTest, ExtensionModuleKeywordArgReceivesMultipleArgsReturns) {
  ternaryfunc func = [](PyObject*, PyObject* args, PyObject*) -> PyObject* {
    HandleScope scope;
    Tuple args_tuple(&scope, ApiHandle::fromPyObject(args)->asObject());
    return ApiHandle::newReference(Thread::current(), args_tuple.at(1));
  };

  Runtime runtime;
  HandleScope scope;
  Str mod_name(&scope, runtime.newStrFromCStr("foobar"));
  Function callee(&scope, runtime.newFunction());
  callee.setModule(runtime.newModule(mod_name));
  callee.setCode(runtime.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntryKw(moduleTrampolineKeywordsKw);

  // Set up a code object that calls the builtin with (123, 456, foo=789)
  Object name(&scope, Str::empty());
  Code code(&scope, runtime.newEmptyCode(name));
  Tuple consts(&scope, runtime.newTuple(5));
  consts.atPut(0, *callee);
  consts.atPut(1, SmallInt::fromWord(123));
  consts.atPut(2, SmallInt::fromWord(456));
  consts.atPut(3, SmallInt::fromWord(789));
  Tuple kw_tuple(&scope, runtime.newTuple(1));
  kw_tuple.atPut(0, runtime.newStrFromCStr("foo"));
  consts.atPut(4, *kw_tuple);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST,   0, LOAD_CONST, 1, LOAD_CONST,       2,
                           LOAD_CONST,   3, LOAD_CONST, 4, CALL_FUNCTION_KW, 3,
                           RETURN_VALUE, 0};
  code.setCode(runtime.newBytesWithAll(bytecode));
  code.setStacksize(5);

  // Execute the code and make sure we get back the result we expect
  RawObject result = Thread::current()->run(code);
  EXPECT_TRUE(isIntEqualsWord(result, 456));
}

TEST(TrampolinesTest, ExtensionModuleKeywordArgReceivesMultipleKwArgsReturns) {
  ternaryfunc func = [](PyObject*, PyObject*, PyObject* kwargs) -> PyObject* {
    Thread* thread = Thread::current();
    HandleScope scope(thread);
    Runtime* runtime = thread->runtime();
    Str foo_str(&scope, runtime->newStrFromCStr("bar"));
    Dict keyword_dict(&scope, ApiHandle::fromPyObject(kwargs)->asObject());
    return ApiHandle::newReference(
        thread, runtime->dictAt(thread, keyword_dict, foo_str));
  };

  Runtime runtime;
  HandleScope scope;
  Str mod_name(&scope, runtime.newStrFromCStr("foobar"));
  Function callee(&scope, runtime.newFunction());
  callee.setModule(runtime.newModule(mod_name));
  callee.setCode(runtime.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntryKw(moduleTrampolineKeywordsKw);

  // Set up a code object that calls the builtin with ("foo"=1234, "bar"=5678)
  Object name(&scope, Str::empty());
  Code code(&scope, runtime.newEmptyCode(name));
  Tuple consts(&scope, runtime.newTuple(4));
  consts.atPut(0, *callee);
  consts.atPut(1, SmallInt::fromWord(1234));
  consts.atPut(2, SmallInt::fromWord(5678));
  Tuple kw_tuple(&scope, runtime.newTuple(2));
  kw_tuple.atPut(0, runtime.newStrFromCStr("foo"));
  kw_tuple.atPut(1, runtime.newStrFromCStr("bar"));
  consts.atPut(3, *kw_tuple);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST, 0, LOAD_CONST,       1, LOAD_CONST,   2,
                           LOAD_CONST, 3, CALL_FUNCTION_KW, 2, RETURN_VALUE, 0};
  code.setCode(runtime.newBytesWithAll(bytecode));
  code.setStacksize(4);

  // Execute the code and make sure we get back the result we expect
  RawObject result = Thread::current()->run(code);
  EXPECT_TRUE(isIntEqualsWord(result, 5678));
}

TEST(TrampolinesTest, ExtensionModuleKeywordArgReceivesVariableArgsReturns) {
  ternaryfunc func = [](PyObject*, PyObject* args, PyObject*) -> PyObject* {
    HandleScope scope;
    Tuple arg_tuple(&scope, ApiHandle::fromPyObject(args)->asObject());
    return ApiHandle::newReference(Thread::current(), arg_tuple.at(0));
  };

  Runtime runtime;
  HandleScope scope;
  Str mod_name(&scope, runtime.newStrFromCStr("foobar"));
  Function callee(&scope, runtime.newFunction());
  callee.setModule(runtime.newModule(mod_name));
  callee.setCode(runtime.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntryEx(moduleTrampolineKeywordsEx);

  // Set up a code object that calls with (*(10))
  Object name(&scope, Str::empty());
  Code code(&scope, runtime.newEmptyCode(name));
  Tuple consts(&scope, runtime.newTuple(2));
  consts.atPut(0, *callee);
  Tuple arg_tuple(&scope, runtime.newTuple(1));
  arg_tuple.atPut(0, SmallInt::fromWord(10));
  consts.atPut(1, *arg_tuple);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST,       0, LOAD_CONST,   1,
                           CALL_FUNCTION_EX, 0, RETURN_VALUE, 0};
  code.setCode(runtime.newBytesWithAll(bytecode));
  code.setStacksize(2);

  // Execute the code and make sure we get back the result we expect
  RawObject result = Thread::current()->run(code);
  EXPECT_TRUE(isIntEqualsWord(result, 10));
}

TEST(TrampolinesTest, ExtensionModuleKeywordArgReceivesVariableKwArgsReturns) {
  ternaryfunc func = [](PyObject*, PyObject*, PyObject* kwargs) -> PyObject* {
    Thread* thread = Thread::current();
    HandleScope scope(thread);
    Runtime* runtime = thread->runtime();
    Str foo_str(&scope, runtime->newStrFromCStr("foo"));
    Dict keyword_dict(&scope, ApiHandle::fromPyObject(kwargs)->asObject());
    return ApiHandle::newReference(
        thread, runtime->dictAt(thread, keyword_dict, foo_str));
  };

  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope;
  Str mod_name(&scope, runtime.newStrFromCStr("foobar"));
  Function callee(&scope, runtime.newFunction());
  callee.setModule(runtime.newModule(mod_name));
  callee.setCode(runtime.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntryEx(moduleTrampolineKeywordsEx);

  // Set up a code object that calls with (*(10), **{"foo":1111})
  Object name(&scope, Str::empty());
  Code code(&scope, runtime.newEmptyCode(name));
  Tuple consts(&scope, runtime.newTuple(3));
  consts.atPut(0, *callee);
  Tuple arg_tuple(&scope, runtime.newTuple(1));
  arg_tuple.atPut(0, SmallInt::fromWord(10));
  consts.atPut(1, *arg_tuple);
  Dict kw_dict(&scope, runtime.newDict());
  Object key(&scope, runtime.newStrFromCStr("foo"));
  Object value(&scope, SmallInt::fromWord(1111));
  runtime.dictAtPut(thread, kw_dict, key, value);
  consts.atPut(2, *kw_dict);
  code.setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST,       0, LOAD_CONST,   1, LOAD_CONST, 2,
                           CALL_FUNCTION_EX, 1, RETURN_VALUE, 0};
  code.setCode(runtime.newBytesWithAll(bytecode));
  code.setStacksize(3);

  // Execute the code and make sure we get back the result we expect
  RawObject result = Thread::current()->run(code);
  EXPECT_TRUE(isIntEqualsWord(result, 1111));
}

static RawObject numArgs(Thread*, Frame*, word nargs) {
  return SmallInt::fromWord(nargs);
}

static void createAndPatchBuiltinNumArgs(Runtime* runtime) {
  // Ensure we have a __main__ module.
  ASSERT_FALSE(runFromCStr(runtime, "").isError());
  HandleScope scope;
  Module main(&scope, findModule(runtime, "__main__"));
  runtime->moduleAddBuiltinFunction(main, SymbolId::kDummy, numArgs);
  ASSERT_FALSE(runFromCStr(runtime, R"(
@_patch
def dummy(first, second):
  pass
)")
                   .isError());
}

TEST(TrampolinesTest, BuiltinTrampolineExReceivesExArgs) {
  Runtime runtime;
  createAndPatchBuiltinNumArgs(&runtime);
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, "result = dummy(*(1,2))").isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 2));
}

TEST(TrampolinesTest, BuiltinTrampolineExReceivesMixOfPositionalAndExArgs1) {
  Runtime runtime;
  createAndPatchBuiltinNumArgs(&runtime);
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, "result = dummy(1, *(2,))").isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 2));
}

static void createAndPatchBuiltinNumArgsVariadic(Runtime* runtime) {
  // Ensure we have a __main__ module.
  ASSERT_FALSE(runFromCStr(runtime, "").isError());
  HandleScope scope;
  Module main(&scope, findModule(runtime, "__main__"));
  runtime->moduleAddBuiltinFunction(main, SymbolId::kDummy, numArgs);
  ASSERT_FALSE(runFromCStr(runtime, R"(
@_patch
def dummy(*args):
  pass
)")
                   .isError());
}

TEST(TrampolinesTest,
     BuiltinTrampolineExReceivesOnePositionalArgAndTwoVariableArgs) {
  Runtime runtime;
  createAndPatchBuiltinNumArgsVariadic(&runtime);
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, "result = dummy(1, *(2, 3))").isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 1));
}

static void createAndPatchBuiltinNumArgsArgsKwargs(Runtime* runtime) {
  // Ensure we have a __main__ module.
  ASSERT_FALSE(runFromCStr(runtime, "").isError());
  HandleScope scope;
  Module main(&scope, findModule(runtime, "__main__"));
  runtime->moduleAddBuiltinFunction(main, SymbolId::kDummy, numArgs);
  ASSERT_FALSE(runFromCStr(runtime, R"(
@_patch
def dummy(*args, **kwargs):
  pass
)")
                   .isError());
}

TEST(TrampolinesTest,
     BuiltinTrampolineExReceivesTwoPositionalOneVariableAndTwoKwArgs) {
  Runtime runtime;
  createAndPatchBuiltinNumArgsArgsKwargs(&runtime);
  HandleScope scope;
  ASSERT_FALSE(
      runFromCStr(&runtime,
                  "result = dummy(1, 2, *(3,), **{'foo': 1, 'bar': 2})")
          .isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 2));
}

TEST(TrampolinesTest, BuiltinTrampolineExReceivesVarArgs) {
  Runtime runtime;
  createAndPatchBuiltinNumArgs(&runtime);
  HandleScope scope;
  ASSERT_FALSE(
      runFromCStr(&runtime, "result = dummy(*(1,), second=5)").isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 2));
}

TEST(TrampolinesDeathTest, BuiltinTrampolineExWithTooFewArgsRaisesTypeError) {
  Runtime runtime;
  createAndPatchBuiltinNumArgs(&runtime);
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "dummy(*(1,))"), LayoutId::kTypeError,
      "TypeError: 'dummy' takes 2 positional arguments but 1 given"));
}

TEST(TrampolinesDeathTest, BuiltinTrampolineExWithTooManyArgsRaisesTypeError) {
  Runtime runtime;
  createAndPatchBuiltinNumArgs(&runtime);
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "dummy(*(1,2,3,4,5))"), LayoutId::kTypeError,
      "TypeError: 'dummy' takes max 2 positional arguments but 5 given"));
}

TEST(TrampolinesTest, SlotTrampolineKwAllowsCallWithNoKwargs) {
  Function::Entry func = [](Thread*, Frame* frame, word argc) -> RawObject {
    EXPECT_EQ(argc, 1);
    Arguments args(frame, argc);
    return args.get(0);
  };

  Runtime runtime;
  HandleScope scope;
  Object name(&scope, runtime.newStrFromCStr("callee"));
  Function callee(&scope, runtime.newFunction());
  callee.setEntryKw(slotTrampolineKw);
  Code callee_code(&scope, runtime.newEmptyCode(name));
  callee_code.setCode(runtime.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setCode(*callee_code);

  // Set up a code object that calls the function with one positional arg and an
  // empty keyword names tuple.
  Object code_name(&scope, Str::empty());
  Code code(&scope, runtime.newEmptyCode(code_name));
  Tuple consts(&scope, runtime.newTuple(3));
  consts.atPut(0, *callee);
  consts.atPut(1, runtime.newInt(1234));
  Tuple kw_tuple(&scope, runtime.newTuple(0));
  consts.atPut(2, *kw_tuple);
  code.setConsts(*consts);

  const byte bytecode[] = {LOAD_CONST,       0, LOAD_CONST,   1, LOAD_CONST, 2,
                           CALL_FUNCTION_KW, 1, RETURN_VALUE, 0};
  code.setCode(runtime.newBytesWithAll(bytecode));
  code.setStacksize(3);

  // Execute the code and make sure we get back the result we expect.
  Object result(&scope, Thread::current()->run(code));
  EXPECT_TRUE(isIntEqualsWord(*result, 1234));
}

}  // namespace python
