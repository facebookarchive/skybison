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

  const char* src = R"(
def func(self):
  print(self)

def test(callable):
  return callable()
)";
  runtime.runFromCString(src);

  HandleScope scope;
  Handle<Module> module(&scope, findModule(&runtime, "__main__"));
  Handle<Object> function(&scope, findInModule(&runtime, module, "func"));
  ASSERT_TRUE(function->isFunction());

  Handle<Object> self(&scope, SmallInt::fromWord(1111));
  Handle<BoundMethod> method(&scope, runtime.newBoundMethod(function, self));

  Handle<Object> test(&scope, findInModule(&runtime, module, "test"));
  ASSERT_TRUE(test->isFunction());
  Handle<Function> func(&scope, *test);

  Handle<ObjectArray> args(&scope, runtime.newObjectArray(1));
  args->atPut(0, *method);

  std::string output = callFunctionToString(func, args);
  EXPECT_EQ(output, "1111\n");
}

TEST(CallTest, CallBoundMethodWithArgs) {
  Runtime runtime;

  const char* src = R"(
def func(self, a, b):
  print(self, a, b)

def test(callable):
  return callable(2222, 3333)
)";
  runtime.runFromCString(src);

  HandleScope scope;
  Handle<Module> module(&scope, findModule(&runtime, "__main__"));
  Handle<Object> function(&scope, findInModule(&runtime, module, "func"));
  ASSERT_TRUE(function->isFunction());

  Handle<Object> self(&scope, SmallInt::fromWord(1111));
  Handle<BoundMethod> method(&scope, runtime.newBoundMethod(function, self));

  Handle<Object> test(&scope, findInModule(&runtime, module, "test"));
  ASSERT_TRUE(test->isFunction());
  Handle<Function> func(&scope, *test);

  Handle<ObjectArray> args(&scope, runtime.newObjectArray(1));
  args->atPut(0, *method);

  std::string output = callFunctionToString(func, args);
  EXPECT_EQ(output, "1111 2222 3333\n");
}

TEST(CallTest, CallDefaultArgs) {
  Runtime runtime;
  HandleScope scope;

  const char* src = R"(
def foo(a=1, b=2, c=3):
  print(a, b, c)

print()
foo(33, 22, 11)
foo()
foo(1001)
foo(1001, 1002)
foo(1001, 1002, 1003)
)";

  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, R"(
33 22 11
1 2 3
1001 2 3
1001 1002 3
1001 1002 1003
)");
}

TEST(CallTest, CallMethodMixPosDefaultArgs) {
  const char* src = R"(
def foo(a, b=2):
  print(a, b)
foo(1)
)";

  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "1 2\n");
}

TEST(CallTest, CallBoundMethodMixed) {
  const char* src = R"(
class R:
  def __init__(self, a, b=2):
    print(a, b)
a = R(9)
)";

  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "9 2\n");
}

TEST(CallTest, SingleKW) {
  Runtime runtime;
  const char* src = R"(
def foo(bar):
   print('bar =',bar)
foo(bar=2)
)";
  const char* expected = "bar = 2\n";
  std::string result = compileAndRunToString(&runtime, src);
  EXPECT_EQ(result, expected);
}

TEST(CallTest, MixedKW) {
  Runtime runtime;
  const char* src = R"(
def foo(a, b, c):
   print(a, b, c)
foo(1, b = 2, c = 3)
)";
  const char* expected = "1 2 3\n";
  std::string result = compileAndRunToString(&runtime, src);
  EXPECT_EQ(result, expected);
}

TEST(CallTest, FullKW) {
  Runtime runtime;
  const char* src = R"(
def foo(a, b, c):
   print(a, b, c)
foo(a = 1, b = 2, c = 3)
)";
  const char* expected = "1 2 3\n";
  std::string result = compileAndRunToString(&runtime, src);
  EXPECT_EQ(result, expected);
}

TEST(CallTest, KWOutOfOrder1) {
  Runtime runtime;
  const char* src = R"(
def foo(a, b, c):
   print(a, b, c)
foo(c = 3, a = 1, b = 2)
)";
  const char* expected = "1 2 3\n";
  std::string result = compileAndRunToString(&runtime, src);
  EXPECT_EQ(result, expected);
}

TEST(CallTest, KWOutOfOrder2) {
  Runtime runtime;
  const char* src = R"(
def foo(a, b, c):
   print(a, b, c)
foo(1, c = 3, b = 2)
)";
  const char* expected = "1 2 3\n";
  std::string result = compileAndRunToString(&runtime, src);
  EXPECT_EQ(result, expected);
}

TEST(CallTest, KeywordOnly1) {
  Runtime runtime;
  const char* src = R"(
def foo(a,b, *, c):
  print(a,b,c)
foo(1, 2, c = 3);
)";
  const char* expected = "1 2 3\n";
  std::string result = compileAndRunToString(&runtime, src);
  EXPECT_EQ(result, expected);
}

TEST(CallTest, KeywordOnly2) {
  Runtime runtime;
  const char* src = R"(
def foo(a,b, *, c):
  print(a,b,c)
foo(1, b = 2, c = 3);
)";
  const char* expected = "1 2 3\n";
  std::string result = compileAndRunToString(&runtime, src);
  EXPECT_EQ(result, expected);
}

TEST(CallTest, KeyWordDefaults) {
  Runtime runtime;
  const char* src = R"(
def foo(a, b = 22, c = 33):
  print(a,b,c)
foo(11, c = 3);
)";
  const char* expected = "11 22 3\n";
  std::string result = compileAndRunToString(&runtime, src);
  EXPECT_EQ(result, expected);
}

TEST(CallTest, VarArgsWithExcess) {
  Runtime runtime;
  const char* src = R"(
def foo(a, b, *c):
  print(a,b,c)
foo(1,2,3,4,5,6);
)";
  const char* expected = "1 2 (3, 4, 5, 6)\n";
  std::string result = compileAndRunToString(&runtime, src);
  EXPECT_EQ(result, expected);
}

TEST(CallTest, VarArgsEmpty) {
  Runtime runtime;
  const char* src = R"(
def foo(a, b, *c):
  print(a,b,c)
foo(1,2);
)";
  const char* expected = "1 2 ()\n";
  std::string result = compileAndRunToString(&runtime, src);
  EXPECT_EQ(result, expected);
}

TEST(CallTest, CallWithKeywordsCalleeWithVarkeyword) {
  Runtime runtime;
  const char* src = R"(
def foo(a,b,c,**d):
    print(a,b,c,d)
foo(1,2,c=3,g=4,h=5,i=6,j="bar")
)";
  const char* expected = "1 2 3 {'g': 4, 'h': 5, 'i': 6, 'j': 'bar'}\n";
  std::string result = compileAndRunToString(&runtime, src);
  EXPECT_EQ(result, expected);
}

TEST(CallTest, CallWithNoArgsCalleeDefaultArgsVarargsVarkeyargs) {
  Runtime runtime;
  const char* src = R"(
def bar(a=1, b=2, *c, **d):
    print(a,b,c,d)
bar()
)";
  const char* expected = "1 2 () {}\n";
  std::string result = compileAndRunToString(&runtime, src);
  EXPECT_EQ(result, expected);
}

TEST(CallTest, CallPositionalCalleeVargsEmptyVarkeyargs) {
  Runtime runtime;
  const char* src = R"(
def bar(a=1, b=2, *c, **d):
    print(a,b,c,d)
bar(1,2,3,4,5,6,7)
)";
  const char* expected = "1 2 (3, 4, 5, 6, 7) {}\n";
  std::string result = compileAndRunToString(&runtime, src);
  EXPECT_EQ(result, expected);
}

TEST(CallTest, CallWithKeywordsCalleeEmptyVarargsFullVarkeyargs) {
  Runtime runtime;
  const char* src = R"(
def bar(a=1, b=2, *c, **d):
    print(a,b,c,d)
bar(a1=11, a2=12, a3=13)
)";
  const char* expected = "1 2 () {'a1': 11, 'a2': 12, 'a3': 13}\n";
  std::string result = compileAndRunToString(&runtime, src);
  EXPECT_EQ(result, expected);
}

TEST(CallTest, CallWithKeywordsCalleeFullVarargsFullVarkeyargs) {
  Runtime runtime;
  const char* src = R"(
def bar(a=1, b=2, *c, **d):
    print(a,b,c,d)
bar(1,2,3,4,5,6,7,a9=9)
)";
  const char* expected = "1 2 (3, 4, 5, 6, 7) {'a9': 9}\n";
  std::string result = compileAndRunToString(&runtime, src);
  EXPECT_EQ(result, expected);
}

TEST(CallTest, CallWithOutOfOrderKeywords) {
  Runtime runtime;
  const char* src = R"(
def foobar(a,b,*,c):
    print(a,b,c)
foobar(c=3,a=1,b=2)
)";
  const char* expected = "1 2 3\n";
  std::string result = compileAndRunToString(&runtime, src);
  EXPECT_EQ(result, expected);
}

TEST(CallTest, CallWithKeywordsCalleeVarargsKeywordOnly) {
  Runtime runtime;
  const char* src = R"(
def foobar1(a,b,*c,d):
    print(a,b,c,d)
foobar1(1,2,3,4,5,d=9)
)";
  const char* expected = "1 2 (3, 4, 5) 9\n";
  std::string result = compileAndRunToString(&runtime, src);
  EXPECT_EQ(result, expected);
}

TEST(CallTest, CallWithKeywordsCalleeVarargsVarkeyargsKeywordOnly) {
  Runtime runtime;
  const char* src = R"(
def foobar2(a,b,*c, e, **d):
    print(a,b,c,d,e)
foobar2(1,e=9,b=2,f1="a",f11=12)
)";
  const char* expected = "1 2 () {'f1': 'a', 'f11': 12} 9\n";
  std::string result = compileAndRunToString(&runtime, src);
  EXPECT_EQ(result, expected);
}

TEST(CallTest, CallEx) {
  Runtime runtime;
  const char* src = R"(
def foo(a,b,c,d):
    print(a,b,c,d)
a = (1,2,3,4)
foo(*a)
)";
  const char* expected = "1 2 3 4\n";
  std::string result = compileAndRunToString(&runtime, src);
  EXPECT_EQ(result, expected);
}

TEST(CallTest, CallExBuildTupleUnpackWithCall) {
  Runtime runtime;
  const char* src = R"(
def foo(a,b,c,d):
    print(a,b,c,d)
a = (3,4)
foo(1,2,*a)
)";
  const char* expected = "1 2 3 4\n";
  std::string result = compileAndRunToString(&runtime, src);
  EXPECT_EQ(result, expected);
}

TEST(CallTest, CallExKw) {
  Runtime runtime;
  const char* src = R"(
def foo(a,b,c,d):
    print(a,b,c,d)
a = {'d': 4, 'b': 2, 'a': 1, 'c': 3}
foo(**a)
)";
  const char* expected = "1 2 3 4\n";
  std::string result = compileAndRunToString(&runtime, src);
  EXPECT_EQ(result, expected);
}

TEST(CallDeathTest, KeywordOnly) {
  Runtime runtime;
  const char* src = R"(
def foo(a,b, *, c):
  print(a,b,c)
foo(1, 2, 3);
)";
  std::unique_ptr<char[]> buffer(Runtime::compile(src));
  EXPECT_DEATH(runtime.run(buffer.get()), "TypeError");
}

TEST(CallDeathTest, MissingKeyword) {
  Runtime runtime;
  const char* src = R"(
def foo(a,b, *, c):
  print(a,b,c)
foo(1, 2);
)";
  std::unique_ptr<char[]> buffer(Runtime::compile(src));
  EXPECT_DEATH(runtime.run(buffer.get()), "TypeError");
}

TEST(CallDeathTest, ArgNameMismatch) {
  Runtime runtime;
  const char* src = R"(
def foo(a,b, *, c):
  print(a,b,c)
foo(1, d = 2, c = 3);
)";
  std::unique_ptr<char[]> buffer(Runtime::compile(src));
  EXPECT_DEATH(runtime.run(buffer.get()), "TypeError");
}

TEST(CallDeathTest, TooManyKWArgs) {
  Runtime runtime;
  const char* src = R"(
def foo(a,b, *, c):
  print(a,b,c)
foo(1, 2, 4, c = 3);
)";
  std::unique_ptr<char[]> buffer(Runtime::compile(src));
  EXPECT_DEATH(runtime.run(buffer.get()), "TypeError");
}

TEST(CallDeathTest, TooManyArgs) {
  Runtime runtime;
  const char* src = R"(
def foo(a,b, c):
  print(a,b,c)
foo(1, 2, 3, 4);
)";
  std::unique_ptr<char[]> buffer(Runtime::compile(src));
  EXPECT_DEATH(runtime.run(buffer.get()), "TypeError");
}

TEST(CallDeathTest, TooFewArgs) {
  Runtime runtime;
  const char* src = R"(
def foo(a,b, c):
  print(a,b,c)
foo(3, 4);
)";
  std::unique_ptr<char[]> buffer(Runtime::compile(src));
  EXPECT_DEATH(runtime.run(buffer.get()), "TypeError");
}

static Object* firstArg(Thread*, Frame* frame, word argc) {
  if (argc == 0) {
    return None::object();
  }
  Arguments args(frame, argc);
  return args.get(0);
}

TEST(TrampolineTest, CallNativeFunctionReceivesPositionalArgument) {
  Runtime runtime;
  HandleScope scope;

  // Create the builtin function
  Handle<Function> callee(&scope, runtime.newFunction());
  callee->setEntry(nativeTrampoline<firstArg>);

  // Set up a code object that calls the builtin with a single argument.
  Handle<Code> code(&scope, runtime.newCode());
  Handle<ObjectArray> consts(&scope, runtime.newObjectArray(2));
  consts->atPut(0, *callee);
  consts->atPut(1, SmallInt::fromWord(1111));
  code->setConsts(*consts);
  const byte bytecode[] = {LOAD_CONST,    0, LOAD_CONST,   1,
                           CALL_FUNCTION, 1, RETURN_VALUE, 0};
  code->setCode(runtime.newByteArrayWithAll(bytecode));
  code->setStacksize(2);

  // Execute the code and make sure we get back the result we expect
  Object* result = Thread::currentThread()->run(*code);
  ASSERT_TRUE(result->isSmallInt());
  ASSERT_EQ(SmallInt::cast(result)->value(), 1111);
}

// test "builtin-kw" func that returns a list of first position arg
// and value of kw argument 'foo'
static Object* returnsPositionalAndKeywordArgument(Thread* thread, Frame* frame,
                                                   word argc) {
  KwArguments args(frame, argc);
  HandleScope scope(thread);
  Handle<Object> foo_name(&scope,
                          thread->runtime()->newStringFromCString("foo"));
  Handle<Object> foo_val_opt(&scope, args.getKw(*foo_name));
  Handle<Object> foo_val(
      &scope, (foo_val_opt->isError() ? None::object() : *foo_val_opt));
  Handle<ObjectArray> tuple(&scope, thread->runtime()->newObjectArray(2));
  tuple->atPut(0, args.get(0));
  tuple->atPut(1, *foo_val);
  return *tuple;
}

TEST(TrampolineTest, CallNativeFunctionReceivesPositionalAndKeywordArgument) {
  Runtime runtime;
  HandleScope scope;

  // Create the builtin kw function
  Handle<Function> callee(&scope, runtime.newFunction());
  callee->setEntryKw(nativeTrampolineKw<returnsPositionalAndKeywordArgument>);

  // Set up a code object that calls the builtin with (1234, foo='bar')
  Handle<Code> code(&scope, runtime.newCode());
  Handle<ObjectArray> consts(&scope, runtime.newObjectArray(4));
  consts->atPut(0, *callee);
  consts->atPut(1, SmallInt::fromWord(1234));
  consts->atPut(2, runtime.newStringFromCString("bar"));
  Handle<ObjectArray> kw_tuple(&scope, runtime.newObjectArray(1));
  kw_tuple->atPut(0, runtime.newStringFromCString("foo"));
  consts->atPut(3, *kw_tuple);
  code->setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST, 0, LOAD_CONST,       1, LOAD_CONST,   2,
                           LOAD_CONST, 3, CALL_FUNCTION_KW, 2, RETURN_VALUE, 0};
  code->setCode(runtime.newByteArrayWithAll(bytecode));
  code->setStacksize(4);

  // Execute the code and make sure we get back the result we expect
  Object* result = Thread::currentThread()->run(*code);
  ASSERT_TRUE(result->isObjectArray());
  Handle<ObjectArray> tuple(&scope, result);
  ASSERT_EQ(tuple->length(), 2);
  EXPECT_EQ(SmallInt::cast(tuple->at(0))->value(), 1234);
  EXPECT_TRUE(String::cast(tuple->at(1))->equalsCString("bar"));
}

// test "builtin-kw" func that returns a list of first position arg
// and value of kw arguments 'foo' and 'bar'
static Object* returnsPositionalAndTwoKeywordArguments(Thread* thread,
                                                       Frame* frame,
                                                       word argc) {
  Runtime* runtime = thread->runtime();
  KwArguments args(frame, argc);
  HandleScope scope;
  Handle<Object> foo_name(&scope, runtime->newStringFromCString("foo"));
  Handle<Object> bar_name(&scope, runtime->newStringFromCString("bar"));
  Handle<ObjectArray> tuple(&scope, runtime->newObjectArray(3));
  tuple->atPut(0, args.get(0));
  Handle<Object> foo_val(&scope, args.getKw(*foo_name));
  tuple->atPut(1, (foo_val->isError() ? None::object() : *foo_val));
  Handle<Object> bar_val(&scope, args.getKw(*bar_name));
  tuple->atPut(2, (bar_val->isError() ? None::object() : *bar_val));
  return *tuple;
}

TEST(TrampolineTest,
     CallNativeFunctionReceivesPositionalAndTwoKeywordArguments) {
  Runtime runtime;
  HandleScope scope;

  // Create the builtin 'multi-kw' function
  Handle<Function> callee(&scope, runtime.newFunction());
  callee->setEntryKw(
      nativeTrampolineKw<returnsPositionalAndTwoKeywordArguments>);

  // Code object that calls func with (1234, (foo='foo_val', bar='bar_val'))
  Handle<Code> code(&scope, runtime.newCode());
  Handle<ObjectArray> consts(&scope, runtime.newObjectArray(5));
  consts->atPut(0, *callee);
  consts->atPut(1, SmallInt::fromWord(1234));
  consts->atPut(2, runtime.newStringFromCString("foo_val"));
  consts->atPut(3, runtime.newStringFromCString("bar_val"));
  Handle<ObjectArray> kw_tuple(&scope, runtime.newObjectArray(2));
  kw_tuple->atPut(0, runtime.newStringFromCString("foo"));
  kw_tuple->atPut(1, runtime.newStringFromCString("bar"));
  consts->atPut(4, *kw_tuple);
  code->setConsts(*consts);

  // load arguments and call builtin kw function
  const byte bytecode[] = {LOAD_CONST,   0, LOAD_CONST, 1, LOAD_CONST,       2,
                           LOAD_CONST,   3, LOAD_CONST, 4, CALL_FUNCTION_KW, 3,
                           RETURN_VALUE, 0};
  code->setCode(runtime.newByteArrayWithAll(bytecode));
  code->setStacksize(5);

  // Execute the code and make sure we get back the result we expect
  Object* result = Thread::currentThread()->run(*code);
  ASSERT_TRUE(result->isObjectArray());
  Handle<ObjectArray> tuple(&scope, result);
  ASSERT_EQ(tuple->length(), 3);
  ASSERT_TRUE(tuple->at(0)->isInt());
  EXPECT_EQ(SmallInt::cast(tuple->at(0))->value(), 1234);
  ASSERT_TRUE(tuple->at(1)->isString());
  EXPECT_TRUE(String::cast(tuple->at(1))->equalsCString("foo_val"));
  ASSERT_TRUE(tuple->at(2)->isString());
  EXPECT_TRUE(String::cast(tuple->at(2))->equalsCString("bar_val"));
}

}  // namespace python
