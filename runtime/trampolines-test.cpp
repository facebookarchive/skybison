#include "gtest/gtest.h"

#include <memory>

#include "frame.h"
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
  print(self)

def test(callable):
  return callable()
)")
                   .isError());

  Module module(&scope, findModule(&runtime_, "__main__"));
  Object function(&scope, moduleAt(&runtime_, module, "func"));
  ASSERT_TRUE(function.isFunction());

  Object self(&scope, SmallInt::fromWord(1111));
  BoundMethod method(&scope, runtime_.newBoundMethod(function, self));

  Object test(&scope, moduleAt(&runtime_, module, "test"));
  ASSERT_TRUE(test.isFunction());
  Function func(&scope, *test);

  Tuple args(&scope, runtime_.newTuple(1));
  args.atPut(0, *method);

  std::string output = callFunctionToString(func, args);
  EXPECT_EQ(output, "1111\n");
}

TEST_F(CallTest, CallBoundMethodWithArgs) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def func(self, a, b):
  print(self, a, b)

def test(callable):
  return callable(2222, 3333)
)")
                   .isError());

  Module module(&scope, findModule(&runtime_, "__main__"));
  Object function(&scope, moduleAt(&runtime_, module, "func"));
  ASSERT_TRUE(function.isFunction());

  Object self(&scope, SmallInt::fromWord(1111));
  BoundMethod method(&scope, runtime_.newBoundMethod(function, self));

  Object test(&scope, moduleAt(&runtime_, module, "test"));
  ASSERT_TRUE(test.isFunction());
  Function func(&scope, *test);

  Tuple args(&scope, runtime_.newTuple(1));
  args.atPut(0, *method);

  std::string output = callFunctionToString(func, args);
  EXPECT_EQ(output, "1111 2222 3333\n");
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

  Module module(&scope, findModule(&runtime_, "__main__"));
  Object function(&scope, moduleAt(&runtime_, module, "func"));
  ASSERT_TRUE(function.isFunction());

  Object self(&scope, SmallInt::fromWord(1111));
  BoundMethod method(&scope, runtime_.newBoundMethod(function, self));

  Object test(&scope, moduleAt(&runtime_, module, "test"));
  ASSERT_TRUE(test.isFunction());
  Function func(&scope, *test);
  Tuple args(&scope, runtime_.newTuple(1));
  args.atPut(0, *method);
  callFunction(func, args);

  Object result_self(&scope, moduleAt(&runtime_, module, "result_self"));
  EXPECT_TRUE(isIntEqualsWord(*result_self, 1111));

  Object result_a(&scope, moduleAt(&runtime_, module, "result_a"));
  EXPECT_TRUE(isIntEqualsWord(*result_a, 2222));

  Object result_b(&scope, moduleAt(&runtime_, module, "result_b"));
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

  Module module(&scope, findModule(&runtime_, "__main__"));
  Object function(&scope, moduleAt(&runtime_, module, "func"));
  ASSERT_TRUE(function.isFunction());

  Object self(&scope, SmallInt::fromWord(1111));
  BoundMethod method(&scope, runtime_.newBoundMethod(function, self));

  Object test(&scope, moduleAt(&runtime_, module, "test"));
  ASSERT_TRUE(test.isFunction());
  Function func(&scope, *test);
  Tuple args(&scope, runtime_.newTuple(1));
  args.atPut(0, *method);
  callFunction(func, args);

  Object result_self(&scope, moduleAt(&runtime_, module, "result_self"));
  EXPECT_TRUE(isIntEqualsWord(*result_self, 1111));

  Object result_a(&scope, moduleAt(&runtime_, module, "result_a"));
  EXPECT_TRUE(isIntEqualsWord(*result_a, 2222));

  Object result_b(&scope, moduleAt(&runtime_, module, "result_b"));
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

  Module module(&scope, findModule(&runtime_, "__main__"));
  Object function(&scope, moduleAt(&runtime_, module, "func"));
  ASSERT_TRUE(function.isFunction());

  Object self(&scope, SmallInt::fromWord(1111));
  BoundMethod method(&scope, runtime_.newBoundMethod(function, self));

  Object test(&scope, moduleAt(&runtime_, module, "test"));
  ASSERT_TRUE(test.isFunction());
  Function func(&scope, *test);
  Tuple args(&scope, runtime_.newTuple(1));
  args.atPut(0, *method);
  callFunction(func, args);

  Object result_self(&scope, moduleAt(&runtime_, module, "result_self"));
  EXPECT_TRUE(isIntEqualsWord(*result_self, 1111));

  Object result_a(&scope, moduleAt(&runtime_, module, "result_a"));
  EXPECT_TRUE(isIntEqualsWord(*result_a, 2222));

  Object result_b(&scope, moduleAt(&runtime_, module, "result_b"));
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

  Module module(&scope, findModule(&runtime_, "__main__"));
  Object function(&scope, moduleAt(&runtime_, module, "func"));
  ASSERT_TRUE(function.isFunction());

  Object self(&scope, SmallInt::fromWord(1111));
  BoundMethod method(&scope, runtime_.newBoundMethod(function, self));

  Object test(&scope, moduleAt(&runtime_, module, "test"));
  ASSERT_TRUE(test.isFunction());
  Function func(&scope, *test);
  Tuple args(&scope, runtime_.newTuple(1));
  args.atPut(0, *method);
  callFunction(func, args);

  Object result_self(&scope, moduleAt(&runtime_, module, "result_self"));
  EXPECT_TRUE(isIntEqualsWord(*result_self, 1111));

  Object result_a(&scope, moduleAt(&runtime_, module, "result_a"));
  EXPECT_TRUE(isIntEqualsWord(*result_a, 2222));

  Object result_b(&scope, moduleAt(&runtime_, module, "result_b"));
  EXPECT_TRUE(isIntEqualsWord(*result_b, 3333));
}

TEST_F(CallTest, CallDefaultArgs) {
  std::string output = compileAndRunToString(&runtime_, R"(
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

TEST_F(CallTest, CallMethodMixPosDefaultArgs) {
  std::string output = compileAndRunToString(&runtime_, R"(
def foo(a, b=2):
  print(a, b)
foo(1)
)");
  EXPECT_EQ(output, "1 2\n");
}

TEST_F(CallTest, CallBoundMethodMixed) {
  std::string output = compileAndRunToString(&runtime_, R"(
class R:
  def __init__(self, a, b=2):
    print(a, b)
a = R(9)
)");
  EXPECT_EQ(output, "9 2\n");
}

TEST_F(CallTest, SingleKW) {
  std::string output = compileAndRunToString(&runtime_, R"(
def foo(bar):
   print('bar =',bar)
foo(bar=2)
)");
  EXPECT_EQ(output, "bar = 2\n");
}

TEST_F(CallTest, MixedKW) {
  std::string output = compileAndRunToString(&runtime_, R"(
def foo(a, b, c):
   print(a, b, c)
foo(1, b = 2, c = 3)
)");
  EXPECT_EQ(output, "1 2 3\n");
}

TEST_F(CallTest, FullKW) {
  std::string output = compileAndRunToString(&runtime_, R"(
def foo(a, b, c):
   print(a, b, c)
foo(a = 1, b = 2, c = 3)
)");
  EXPECT_EQ(output, "1 2 3\n");
}

TEST_F(CallTest, KWOutOfOrder1) {
  std::string output = compileAndRunToString(&runtime_, R"(
def foo(a, b, c):
   print(a, b, c)
foo(c = 3, a = 1, b = 2)
)");
  EXPECT_EQ(output, "1 2 3\n");
}

TEST_F(CallTest, KWOutOfOrder2) {
  std::string output = compileAndRunToString(&runtime_, R"(
def foo(a, b, c):
   print(a, b, c)
foo(1, c = 3, b = 2)
)");
  EXPECT_EQ(output, "1 2 3\n");
}

TEST_F(CallTest, KeywordOnly1) {
  std::string output = compileAndRunToString(&runtime_, R"(
def foo(a,b, *, c):
  print(a,b,c)
foo(1, 2, c = 3);
)");
  EXPECT_EQ(output, "1 2 3\n");
}

TEST_F(CallTest, KeywordOnly2) {
  std::string output = compileAndRunToString(&runtime_, R"(
def foo(a,b, *, c):
  print(a,b,c)
foo(1, b = 2, c = 3);
)");
  EXPECT_EQ(output, "1 2 3\n");
}

TEST_F(CallTest, KeyWordDefaults) {
  std::string output = compileAndRunToString(&runtime_, R"(
def foo(a, b = 22, c = 33):
  print(a,b,c)
foo(11, c = 3);
)");
  EXPECT_EQ(output, "11 22 3\n");
}

TEST_F(CallTest, VarArgsWithExcess) {
  std::string output = compileAndRunToString(&runtime_, R"(
def foo(a, b, *c):
  print(a,b,c)
foo(1,2,3,4,5,6);
)");
  EXPECT_EQ(output, "1 2 (3, 4, 5, 6)\n");
}

TEST_F(CallTest, VarArgsEmpty) {
  std::string output = compileAndRunToString(&runtime_, R"(
def foo(a, b, *c):
  print(a,b,c)
foo(1,2);
)");
  EXPECT_EQ(output, "1 2 ()\n");
}

TEST_F(CallTest, CallWithKeywordsCalleeWithVarkeyword) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def foo(a,b,c,**d):
    return [a,b,c,d]
result = foo(1,2,c=3,g=4,h=5,j="bar")
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, testing::moduleAt(&runtime_, "__main__", "result"));
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
  std::string output = compileAndRunToString(&runtime_, R"(
def bar(a=1, b=2, *c, **d):
    print(a,b,c,d)
bar()
)");
  EXPECT_EQ(output, "1 2 () {}\n");
}

TEST_F(CallTest, CallPositionalCalleeVargsEmptyVarkeyargs) {
  std::string output = compileAndRunToString(&runtime_, R"(
def bar(a=1, b=2, *c, **d):
    print(a,b,c,d)
bar(1,2,3,4,5,6,7)
)");
  EXPECT_EQ(output, "1 2 (3, 4, 5, 6, 7) {}\n");
}

TEST_F(CallTest, CallWithKeywordsCalleeEmptyVarargsFullVarkeyargs) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def bar(a=1, b=2, *c, **d):
    return [a,b,c,d]
result = bar(a1=11, a2=12, a3=13)
)")
                   .isError());
  HandleScope scope(thread_);
  List result(&scope, testing::moduleAt(&runtime_, "__main__", "result"));
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
  List result(&scope, testing::moduleAt(&runtime_, "__main__", "result"));
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
  std::string output = compileAndRunToString(&runtime_, R"(
def foobar(a,b,*,c):
    print(a,b,c)
foobar(c=3,a=1,b=2)
)");
  EXPECT_EQ(output, "1 2 3\n");
}

TEST_F(CallTest, CallWithKeywordsCalleeVarargsKeywordOnly) {
  std::string output = compileAndRunToString(&runtime_, R"(
def foobar1(a,b,*c,d):
    print(a,b,c,d)
foobar1(1,2,3,4,5,d=9)
)");
  EXPECT_EQ(output, "1 2 (3, 4, 5) 9\n");
}

TEST_F(CallTest, CallWithKeywordsCalleeVarargsVarkeyargsKeywordOnly) {
  std::string output = compileAndRunToString(&runtime_, R"(
def foobar2(a,b,*c, e, **d):
    print(a,b,c,d,e)
foobar2(1,e=9,b=2,f1="a",f11=12)
)");
  EXPECT_EQ(output, "1 2 () {'f1': 'a', 'f11': 12} 9\n");
}

TEST_F(CallTest, CallEx) {
  std::string output = compileAndRunToString(&runtime_, R"(
def foo(a,b,c,d):
    print(a,b,c,d)
a = (1,2,3,4)
foo(*a)
)");
  EXPECT_EQ(output, "1 2 3 4\n");
}

TEST_F(CallTest, CallExBuildTupleUnpackWithCall) {
  std::string output = compileAndRunToString(&runtime_, R"(
def foo(a,b,c,d):
    print(a,b,c,d)
a = (3,4)
foo(1,2,*a)
)");
  EXPECT_EQ(output, "1 2 3 4\n");
}

TEST_F(CallTest, CallExKw) {
  std::string output = compileAndRunToString(&runtime_, R"(
def foo(a,b,c,d):
    print(a,b,c,d)
a = {'d': 4, 'b': 2, 'a': 1, 'c': 3}
foo(**a)
)");
  EXPECT_EQ(output, "1 2 3 4\n");
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

TEST_F(TrampolinesTest, BuiltinTrampolineKwPassesKwargs) {
  HandleScope scope(thread_);
  createAndPatchBuiltinReturnSecondArg(&runtime_);
  ASSERT_FALSE(
      runFromCStr(&runtime_, "result = dummy(second=12345, first=None)")
          .isError());
  Object result(&scope, moduleAt(&runtime_, "__main__", "result"));
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
            runtime_.newCode(1, 0, nlocals, 0, flags, bc, empty_tuple,
                             empty_tuple, varnames, empty_tuple, cellvars,
                             empty_str, empty_str, 0, empty_bytes));
  ASSERT_TRUE(!code.cell2arg().isNoneType());

  Object qualname(&scope, runtime_.newStrFromCStr("foo"));
  Tuple closure_tuple(&scope, runtime_.newTuple(1));
  closure_tuple.atPut(0, runtime_.newInt(99));
  Object none(&scope, NoneType::object());
  Dict globals(&scope, runtime_.newDict());
  Function foo(&scope,
               Interpreter::makeFunction(thread_, qualname, code, closure_tuple,
                                         none, none, none, globals));

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
            runtime_.newCode(1, 0, nlocals, 0, 0, bc, consts, empty_tuple,
                             varnames, empty_tuple, cellvars, empty_str,
                             empty_str, 0, empty_bytes));
  ASSERT_TRUE(!code.cell2arg().isNoneType());

  // Create a function
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def foo(bar): pass
)")
                   .isError());
  Function foo(&scope, moduleAt(&runtime_, "__main__", "foo"));
  foo.setEntry(interpreterTrampoline);
  foo.setCode(*code);

  // Run function
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = foo(1)
)")
                   .isError());
  Object result(&scope, testing::moduleAt(&runtime_, "__main__", "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 10));
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
  Object result(&scope, moduleAt(&runtime_, "__main__", "result"));
  EXPECT_EQ(result, SmallInt::fromWord(10));
}

TEST_F(TrampolinesTest, ExtensionModuleNoArgReceivesNoArgsReturns) {
  binaryfunc func = [](PyObject*, PyObject*) -> PyObject* {
    return ApiHandle::newReference(Thread::current(), SmallInt::fromWord(123));
  };

  HandleScope scope(thread_);
  Str mod_name(&scope, runtime_.newStrFromCStr("foobar"));
  Function callee(&scope, runtime_.newFunction());
  callee.setModule(runtime_.newModule(mod_name));
  callee.setCode(runtime_.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntry(moduleTrampolineNoArgs);

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
  Str mod_name(&scope, runtime_.newStrFromCStr("foobar"));
  Function callee(&scope, runtime_.newFunction());
  callee.setModule(runtime_.newModule(mod_name));
  callee.setCode(runtime_.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntry(moduleTrampolineNoArgs);

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
  Str mod_name(&scope, runtime_.newStrFromCStr("foobar"));
  Function callee(&scope, runtime_.newFunction());
  callee.setModule(runtime_.newModule(mod_name));
  callee.setCode(runtime_.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntry(moduleTrampolineNoArgs);

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
  Str mod_name(&scope, runtime_.newStrFromCStr("foobar"));
  Function callee(&scope, runtime_.newFunction());
  callee.setModule(runtime_.newModule(mod_name));
  callee.setCode(runtime_.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntryKw(moduleTrampolineNoArgsKw);

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
  Str mod_name(&scope, runtime_.newStrFromCStr("foobar"));
  Function callee(&scope, runtime_.newFunction());
  callee.setModule(runtime_.newModule(mod_name));
  callee.setCode(runtime_.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntryKw(moduleTrampolineNoArgsKw);

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
  Str mod_name(&scope, runtime_.newStrFromCStr("foobar"));
  Function callee(&scope, runtime_.newFunction());
  callee.setModule(runtime_.newModule(mod_name));
  callee.setCode(runtime_.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntryEx(moduleTrampolineNoArgsEx);

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
  Str mod_name(&scope, runtime_.newStrFromCStr("foobar"));
  Function callee(&scope, runtime_.newFunction());
  callee.setModule(runtime_.newModule(mod_name));
  callee.setCode(runtime_.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntryEx(moduleTrampolineNoArgsEx);

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
  Str mod_name(&scope, runtime_.newStrFromCStr("foobar"));
  Function callee(&scope, runtime_.newFunction());
  callee.setModule(runtime_.newModule(mod_name));
  callee.setCode(runtime_.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntry(moduleTrampolineOneArg);

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
  Str mod_name(&scope, runtime_.newStrFromCStr("foobar"));
  Function callee(&scope, runtime_.newFunction());
  callee.setModule(runtime_.newModule(mod_name));
  callee.setCode(runtime_.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntry(moduleTrampolineOneArg);

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
  Str mod_name(&scope, runtime_.newStrFromCStr("foobar"));
  Function callee(&scope, runtime_.newFunction());
  callee.setModule(runtime_.newModule(mod_name));
  callee.setCode(runtime_.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntry(moduleTrampolineOneArg);

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
  Str mod_name(&scope, runtime_.newStrFromCStr("foobar"));
  Function callee(&scope, runtime_.newFunction());
  callee.setModule(runtime_.newModule(mod_name));
  callee.setCode(runtime_.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntry(moduleTrampolineOneArg);

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
  Str mod_name(&scope, runtime_.newStrFromCStr("foobar"));
  Function callee(&scope, runtime_.newFunction());
  callee.setModule(runtime_.newModule(mod_name));
  callee.setCode(runtime_.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntryKw(moduleTrampolineOneArgKw);

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
  Str mod_name(&scope, runtime_.newStrFromCStr("foobar"));
  Function callee(&scope, runtime_.newFunction());
  callee.setModule(runtime_.newModule(mod_name));
  callee.setCode(runtime_.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntryKw(moduleTrampolineOneArgKw);

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
  Str mod_name(&scope, runtime_.newStrFromCStr("foobar"));
  Function callee(&scope, runtime_.newFunction());
  callee.setModule(runtime_.newModule(mod_name));
  callee.setCode(runtime_.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntryEx(moduleTrampolineOneArgEx);

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
  Str mod_name(&scope, runtime_.newStrFromCStr("foobar"));
  Function callee(&scope, runtime_.newFunction());
  callee.setModule(runtime_.newModule(mod_name));
  callee.setCode(runtime_.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntryEx(moduleTrampolineOneArgEx);

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
  Str mod_name(&scope, runtime_.newStrFromCStr("foobar"));
  Function callee(&scope, runtime_.newFunction());
  callee.setModule(runtime_.newModule(mod_name));
  callee.setCode(runtime_.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntryEx(moduleTrampolineOneArgEx);

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
  Str mod_name(&scope, runtime_.newStrFromCStr("foobar"));
  Function callee(&scope, runtime_.newFunction());
  callee.setModule(runtime_.newModule(mod_name));
  callee.setCode(runtime_.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntry(moduleTrampolineVarArgs);

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
  Str mod_name(&scope, runtime_.newStrFromCStr("foobar"));
  Function callee(&scope, runtime_.newFunction());
  callee.setModule(runtime_.newModule(mod_name));
  callee.setCode(runtime_.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntry(moduleTrampolineVarArgs);

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
  Str mod_name(&scope, runtime_.newStrFromCStr("foobar"));
  Function callee(&scope, runtime_.newFunction());
  callee.setModule(runtime_.newModule(mod_name));
  callee.setCode(runtime_.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntry(moduleTrampolineVarArgs);

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
  Str mod_name(&scope, runtime_.newStrFromCStr("foobar"));
  Function callee(&scope, runtime_.newFunction());
  callee.setModule(runtime_.newModule(mod_name));
  callee.setCode(runtime_.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntryKw(moduleTrampolineVarArgsKw);

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
  Str mod_name(&scope, runtime_.newStrFromCStr("foobar"));
  Function callee(&scope, runtime_.newFunction());
  callee.setModule(runtime_.newModule(mod_name));
  callee.setCode(runtime_.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntryKw(moduleTrampolineVarArgsKw);

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
  Str mod_name(&scope, runtime_.newStrFromCStr("foobar"));
  Function callee(&scope, runtime_.newFunction());
  callee.setModule(runtime_.newModule(mod_name));
  callee.setCode(runtime_.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntryEx(moduleTrampolineVarArgsEx);

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
  Str mod_name(&scope, runtime_.newStrFromCStr("foobar"));
  Function callee(&scope, runtime_.newFunction());
  callee.setModule(runtime_.newModule(mod_name));
  callee.setCode(runtime_.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntryEx(moduleTrampolineVarArgsEx);

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
  Str mod_name(&scope, runtime_.newStrFromCStr("foobar"));
  Function callee(&scope, runtime_.newFunction());
  callee.setModule(runtime_.newModule(mod_name));
  callee.setCode(runtime_.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntryEx(moduleTrampolineVarArgsEx);

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
  Str mod_name(&scope, runtime_.newStrFromCStr("foobar"));
  Function callee(&scope, runtime_.newFunction());
  callee.setModule(runtime_.newModule(mod_name));
  callee.setCode(runtime_.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntry(moduleTrampolineKeywords);

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
  Str mod_name(&scope, runtime_.newStrFromCStr("foobar"));
  Function callee(&scope, runtime_.newFunction());
  callee.setModule(runtime_.newModule(mod_name));
  callee.setCode(runtime_.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntry(moduleTrampolineKeywords);

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
  Str mod_name(&scope, runtime_.newStrFromCStr("foobar"));
  Function callee(&scope, runtime_.newFunction());
  callee.setModule(runtime_.newModule(mod_name));
  callee.setCode(runtime_.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntry(moduleTrampolineKeywords);

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
  Str mod_name(&scope, runtime_.newStrFromCStr("foobar"));
  Function callee(&scope, runtime_.newFunction());
  callee.setModule(runtime_.newModule(mod_name));
  callee.setCode(runtime_.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntryKw(moduleTrampolineKeywordsKw);

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
  Str mod_name(&scope, runtime_.newStrFromCStr("foobar"));
  Function callee(&scope, runtime_.newFunction());
  callee.setModule(runtime_.newModule(mod_name));
  callee.setCode(runtime_.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntryKw(moduleTrampolineKeywordsKw);

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
  Str mod_name(&scope, runtime_.newStrFromCStr("foobar"));
  Function callee(&scope, runtime_.newFunction());
  callee.setModule(runtime_.newModule(mod_name));
  callee.setCode(runtime_.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntryKw(moduleTrampolineKeywordsKw);

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
  Str mod_name(&scope, runtime_.newStrFromCStr("foobar"));
  Function callee(&scope, runtime_.newFunction());
  callee.setModule(runtime_.newModule(mod_name));
  callee.setCode(runtime_.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntryEx(moduleTrampolineKeywordsEx);

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
  Str mod_name(&scope, runtime_.newStrFromCStr("foobar"));
  Function callee(&scope, runtime_.newFunction());
  callee.setModule(runtime_.newModule(mod_name));
  callee.setCode(runtime_.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setEntryEx(moduleTrampolineKeywordsEx);

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
  Module main(&scope, findModule(runtime, "__main__"));
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
  Object result(&scope, moduleAt(&runtime_, "__main__", "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 2));
}

TEST_F(TrampolinesTest, BuiltinTrampolineExReceivesMixOfPositionalAndExArgs1) {
  createAndPatchBuiltinNumArgs(&runtime_);
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, "result = dummy(1, *(2,))").isError());
  Object result(&scope, moduleAt(&runtime_, "__main__", "result"));
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

TEST_F(TrampolinesTest,
       BuiltinTrampolineExReceivesOnePositionalArgAndTwoVariableArgs) {
  createAndPatchBuiltinNumArgsVariadic(&runtime_);
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, "result = dummy(1, *(2, 3))").isError());
  Object result(&scope, moduleAt(&runtime_, "__main__", "result"));
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

TEST_F(TrampolinesTest,
       BuiltinTrampolineExReceivesTwoPositionalOneVariableAndTwoKwArgs) {
  createAndPatchBuiltinNumArgsArgsKwargs(&runtime_);
  HandleScope scope(thread_);
  ASSERT_FALSE(
      runFromCStr(&runtime_,
                  "result = dummy(1, 2, *(3,), **{'foo': 1, 'bar': 2})")
          .isError());
  Object result(&scope, moduleAt(&runtime_, "__main__", "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 2));
}

TEST_F(TrampolinesTest, BuiltinTrampolineExReceivesVarArgs) {
  createAndPatchBuiltinNumArgs(&runtime_);
  HandleScope scope(thread_);
  ASSERT_FALSE(
      runFromCStr(&runtime_, "result = dummy(*(1,), second=5)").isError());
  Object result(&scope, moduleAt(&runtime_, "__main__", "result"));
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

TEST_F(TrampolinesTest, SlotTrampolineKwAllowsCallWithNoKwargs) {
  Function::Entry func = [](Thread*, Frame* frame, word argc) -> RawObject {
    EXPECT_EQ(argc, 1);
    Arguments args(frame, argc);
    return args.get(0);
  };

  HandleScope scope(thread_);
  Function callee(&scope, runtime_.newFunction());
  callee.setEntryKw(slotTrampolineKw);
  Code callee_code(&scope, newEmptyCode());
  callee_code.setCode(runtime_.newIntFromCPtr(bit_cast<void*>(func)));
  callee.setCode(*callee_code);

  // Set up a code object that calls the function with one positional arg and an
  // empty keyword names tuple.
  Code code(&scope, newEmptyCode());
  Tuple consts(&scope, runtime_.newTuple(3));
  consts.atPut(0, *callee);
  consts.atPut(1, runtime_.newInt(1234));
  Tuple kw_tuple(&scope, runtime_.emptyTuple());
  consts.atPut(2, *kw_tuple);
  code.setConsts(*consts);

  const byte bytecode[] = {LOAD_CONST,       0, LOAD_CONST,   1, LOAD_CONST, 2,
                           CALL_FUNCTION_KW, 1, RETURN_VALUE, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));
  code.setStacksize(3);

  // Execute the code and make sure we get back the result we expect.
  Object result(&scope, runCode(code));
  EXPECT_TRUE(isIntEqualsWord(*result, 1234));
}

}  // namespace python
