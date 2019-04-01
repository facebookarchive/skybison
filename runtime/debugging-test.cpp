#include "gtest/gtest.h"

#include "gmock/gmock-matchers.h"

#include <iostream>

#include "debugging.h"
#include "test-utils.h"

namespace python {

using namespace testing;

static RawObject makeTestCode(Thread* thread) {
  Runtime* runtime = thread->runtime();
  HandleScope scope;
  const byte bytes_array[4] = {100, 0, 83, 0};
  Bytes bytes(&scope, runtime->newBytesWithAll(bytes_array));
  Tuple consts(&scope, runtime->newTuple(1));
  consts.atPut(0, runtime->newStrFromCStr("const0"));
  Tuple names(&scope, runtime->newTuple(1));
  names.atPut(0, runtime->newStrFromCStr("name0"));
  Tuple varnames(&scope, runtime->newTuple(1));
  varnames.atPut(0, runtime->newStrFromCStr("variable0"));
  Tuple freevars(&scope, runtime->newTuple(1));
  freevars.atPut(0, runtime->newStrFromCStr("freevar0"));
  Tuple cellvars(&scope, runtime->newTuple(1));
  cellvars.atPut(0, runtime->newStrFromCStr("cellvar0"));
  Str filename(&scope, runtime->newStrFromCStr("filename0"));
  Str name(&scope, runtime->newStrFromCStr("name0"));
  Object lnotab(&scope, NoneType::object());
  return runtime->newCode(1, 0, 0, 1, 0, bytes, consts, names, varnames,
                          freevars, cellvars, filename, name, 0, lnotab);
}

TEST(DebuggingTests, DumpExtendedCode) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object code(&scope, makeTestCode(thread));

  std::stringstream ss;
  dumpExtended(ss, *code);
  EXPECT_EQ(
      ss.str(),
      R"(name: "name0" argcount: 1 kwonlyargcount: 0 nlocals: 0 stacksize: 1
filename: "filename0"
consts: ("const0",)
names: ("name0",)
cellvars: ("cellvar0",)
freevars: ("freevar0",)
varnames: ("variable0",)
   0 LOAD_CONST 0
   2 RETURN_VALUE 0
)");
}

TEST(DebuggingTests, DumpExtendedFunction) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Function func(&scope, runtime.newFunction());
  func.setAnnotations(runtime.newDict());
  func.setClosure(runtime.newTuple(0));
  func.setDefaults(runtime.newTuple(0));
  func.setKwDefaults(runtime.newDict());
  func.setModule(runtime.newStrFromCStr("barmodule"));
  func.setName(runtime.newStrFromCStr("baz"));
  func.setQualname(runtime.newStrFromCStr("footype.baz"));
  func.setCode(makeTestCode(thread));
  std::stringstream ss;
  dumpExtended(ss, *func);
  EXPECT_EQ(ss.str(), R"(name: "baz"
qualname: "footype.baz"
module: "barmodule"
annotations: {}
closure: ()
defaults: ()
kwdefaults: {}
code: name: "name0" argcount: 1 kwonlyargcount: 0 nlocals: 0 stacksize: 1
filename: "filename0"
consts: ("const0",)
names: ("name0",)
cellvars: ("cellvar0",)
freevars: ("freevar0",)
varnames: ("variable0",)
   0 LOAD_CONST 0
   2 RETURN_VALUE 0
)");
}

TEST(DebuggingTests, FormatBool) {
  Runtime runtime;
  std::stringstream ss;
  ss << Bool::trueObj() << ';' << Bool::falseObj();
  EXPECT_EQ(ss.str(), "True;False");
}

TEST(DebuggingTests, FormatCode) {
  Runtime runtime;
  HandleScope scope;
  Code code(&scope, runtime.newEmptyCode());
  code.setName(runtime.newStrFromCStr("foobar"));
  std::stringstream ss;
  ss << code;
  EXPECT_EQ(ss.str(), "<code \"foobar\">");
}

TEST(DebuggingTests, FormatDict) {
  Runtime runtime;
  HandleScope scope;
  Dict dict(&scope, runtime.newDict());
  Object key0(&scope, runtime.newStrFromCStr("hello"));
  Object key1(&scope, NoneType::object());
  Object value0(&scope, runtime.newInt(88));
  Object value1(&scope, runtime.newTuple(0));
  runtime.dictAtPut(dict, key0, value0);
  runtime.dictAtPut(dict, key1, value1);
  std::stringstream ss;
  ss << dict;
  EXPECT_TRUE(ss.str() == R"({"hello": 88, None: ()})" ||
              ss.str() == R"({None: (), "hello": 88}")");
}

TEST(DebuggingTests, FormatError) {
  Runtime runtime;
  std::stringstream ss;
  ss << Error::object();
  EXPECT_EQ(ss.str(), "Error");
}

TEST(DebuggingTests, FormatFloat) {
  Runtime runtime;
  std::stringstream ss;
  ss << runtime.newFloat(42.42);
  EXPECT_EQ(ss.str(), "0x1.535c28f5c28f6p+5");
}

TEST(DebuggingTests, FormatFunction) {
  Runtime runtime;
  HandleScope scope;
  std::stringstream ss;
  Object function(&scope, moduleAt(&runtime, "builtins", "callable"));
  ASSERT_TRUE(function.isFunction());
  ss << function;
  EXPECT_EQ(ss.str(), R"(<function "callable">)");
}

TEST(DebuggingTests, FormatLargeInt) {
  Runtime runtime;
  std::stringstream ss;
  const uword digits[2] = {0x12345, kMaxUword};
  ss << runtime.newIntWithDigits(digits);
  EXPECT_EQ(ss.str(), "largeint([0x0000000000012345, 0xffffffffffffffff])");
}

TEST(DebuggingTests, FormatLargeStr) {
  Runtime runtime;
  HandleScope scope;
  std::stringstream ss;
  Object str(&scope, runtime.newStrFromCStr("hello world"));
  EXPECT_TRUE(str.isLargeStr());
  ss << str;
  EXPECT_EQ(ss.str(), "\"hello world\"");
}

TEST(DebuggingTests, FormatList) {
  Runtime runtime;
  HandleScope scope;
  List list(&scope, runtime.newList());
  Object o0(&scope, NoneType::object());
  Object o1(&scope, runtime.newInt(17));
  runtime.listAdd(list, o0);
  runtime.listAdd(list, o1);
  std::stringstream ss;
  ss << list;
  EXPECT_EQ(ss.str(), "[None, 17]");
}

TEST(DebuggingTests, FormatModule) {
  Runtime runtime;
  HandleScope scope;
  Object name(&scope, runtime.newStrFromCStr("foomodule"));
  Object module(&scope, runtime.newModule(name));
  std::stringstream ss;
  ss << module;
  EXPECT_EQ(ss.str(), R"(<module "foomodule">)");
}

TEST(DebuggingTests, FormatNone) {
  Runtime runtime;
  std::stringstream ss;
  ss << NoneType::object();
  EXPECT_EQ(ss.str(), "None");
}

TEST(DebuggingTests, FormatObjectWithBuiltinClass) {
  Runtime runtime;
  std::stringstream ss;
  ss << NotImplementedType::object();
  EXPECT_EQ(ss.str(), R"(<"NotImplementedType" object>)");
}

TEST(DebuggingTests, FormatObjectWithUserDefinedClass) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
class Foo:
  pass
foo = Foo()
)");
  Object foo(&scope, moduleAt(&runtime, "__main__", "foo"));
  std::stringstream ss;
  ss << foo;
  EXPECT_EQ(ss.str(), R"(<"Foo" object>)");
}

TEST(DebuggingTests, FormatObjectWithUnknownType) {
  Runtime runtime;
  HandleScope scope;
  Object obj(&scope, NotImplementedType::object());
  // Phabricate a nameless type...
  RawType::cast(runtime.typeOf(*obj)).setName(NoneType::object());

  std::stringstream ss;
  std::stringstream expected;
  ss << obj;
  expected << "<object with LayoutId " << static_cast<word>(obj.layoutId())
           << ">";
  EXPECT_EQ(ss.str(), expected.str());
}

TEST(DebuggingTests, FormatSmallInt) {
  Runtime runtime;
  std::stringstream ss;
  ss << SmallInt::fromWord(-42) << ';'
     << SmallInt::fromWord(SmallInt::kMinValue) << ';'
     << SmallInt::fromWord(SmallInt::kMaxValue);
  std::stringstream expected;
  expected << "-42;" << SmallInt::kMinValue << ";" << SmallInt::kMaxValue;
  EXPECT_EQ(ss.str(), expected.str());
}

TEST(DebuggingTests, FormatSmallStr) {
  Runtime runtime;
  HandleScope scope;
  std::stringstream ss;
  Object str(&scope, runtime.newStrFromCStr("aa"));
  EXPECT_TRUE(str.isSmallStr());
  ss << str;
  EXPECT_EQ(ss.str(), "\"aa\"");
}

TEST(DebuggingTests, FormatTuple) {
  Runtime runtime;
  HandleScope scope;
  Tuple tuple(&scope, runtime.newTuple(2));
  tuple.atPut(0, Bool::trueObj());
  tuple.atPut(1, runtime.newStrFromCStr("hey"));
  std::stringstream ss;
  ss << tuple;
  EXPECT_EQ(ss.str(), R"((True, "hey"))");
}

TEST(DebuggingTests, FormatTupleWithoutElements) {
  Runtime runtime;
  std::stringstream ss;
  ss << runtime.newTuple(0);
  EXPECT_EQ(ss.str(), "()");
}

TEST(DebuggingTests, FormatTupleWithOneElement) {
  Runtime runtime;
  HandleScope scope;
  Tuple tuple(&scope, runtime.newTuple(1));
  tuple.atPut(0, runtime.newInt(77));
  std::stringstream ss;
  ss << tuple;
  EXPECT_EQ(ss.str(), "(77,)");
}

TEST(DebuggingTests, FormatType) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
class MyClass:
  pass
)");
  Object my_class(&scope, moduleAt(&runtime, "__main__", "MyClass"));
  std::stringstream ss;
  ss << my_class;
  EXPECT_EQ(ss.str(), "<type \"MyClass\">");
}

TEST(DebuggingTests, FormatFrame) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
def func(arg0, arg1):
  hello = "world"
  return arg0 + arg1
)");
  Function func(&scope, moduleAt(&runtime, "__main__", "func"));

  Thread* thread = Thread::current();
  Frame* root = thread->currentFrame();
  root->setVirtualPC(8);
  root->pushValue(NoneType::object());
  ASSERT_EQ(root->previousFrame(), nullptr);

  Frame* frame0 = thread->openAndLinkFrame(0, 1, 1);
  frame0->setLocal(0, runtime.newStrFromCStr("foo bar"));
  frame0->pushValue(*func);
  frame0->setVirtualPC(42);
  Frame* frame1 = thread->openAndLinkFrame(0, 3, 2);
  frame1->setVirtualPC(4);
  frame1->setCode(func.code());
  frame1->setLocal(0, runtime.newInt(-9));
  frame1->setLocal(1, runtime.newInt(17));
  frame1->setLocal(2, runtime.newStrFromCStr("world"));

  std::stringstream ss;
  ss << thread->currentFrame();
  EXPECT_THAT(ss.str(), ::testing::MatchesRegex(R"(- pc: 8
- pc: 42
  function: None
  0: "foo bar"
- pc: 4 (.+)
  function: <function "func">
  0 "arg0": -9
  1 "arg1": 17
  2 "hello": "world"
)"));
}

TEST(DebuggingTests, FormatFrameNullptr) {
  Runtime runtime;
  std::stringstream ss;
  ss << static_cast<Frame*>(nullptr);
  EXPECT_EQ(ss.str(), "<nullptr>");
}

}  // namespace python
