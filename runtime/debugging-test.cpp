#include "gtest/gtest.h"

#include "gmock/gmock-matchers.h"

#include <iostream>

#include "bytecode.h"
#include "debugging.h"
#include "test-utils.h"

namespace python {

using namespace testing;

static RawObject makeTestCode(Thread* thread) {
  Runtime* runtime = thread->runtime();
  HandleScope scope;
  const byte bytes_array[] = {LOAD_CONST, 0, LOAD_ATTR, 0, RETURN_VALUE, 0};
  Bytes bytes(&scope, runtime->newBytesWithAll(bytes_array));
  Tuple consts(&scope, runtime->newTuple(1));
  consts.atPut(0, runtime->newStrFromCStr("const0"));
  Tuple names(&scope, runtime->newTuple(1));
  names.atPut(0, runtime->newStrFromCStr("name0"));
  Tuple varnames(&scope, runtime->newTuple(4));
  varnames.atPut(0, runtime->newStrFromCStr("argument0"));
  varnames.atPut(1, runtime->newStrFromCStr("varargs"));
  varnames.atPut(2, runtime->newStrFromCStr("varkeyargs"));
  varnames.atPut(3, runtime->newStrFromCStr("variable0"));
  Tuple freevars(&scope, runtime->newTuple(1));
  freevars.atPut(0, runtime->newStrFromCStr("freevar0"));
  Tuple cellvars(&scope, runtime->newTuple(1));
  cellvars.atPut(0, runtime->newStrFromCStr("cellvar0"));
  Str filename(&scope, runtime->newStrFromCStr("filename0"));
  Str name(&scope, runtime->newStrFromCStr("name0"));
  Object lnotab(&scope, Bytes::empty());
  word argcount = 1;
  word kwonlyargcount = 0;
  word nlocals = 4;
  word stacksize = 1;
  word flags = Code::NESTED | Code::OPTIMIZED | Code::NEWLOCALS |
               Code::VARARGS | Code::VARKEYARGS;
  return runtime->newCode(argcount, kwonlyargcount, nlocals, stacksize, flags,
                          bytes, consts, names, varnames, freevars, cellvars,
                          filename, name, 0, lnotab);
}

static RawObject makeTestFunction(Thread* thread) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object qualname(&scope, runtime->newStrFromCStr("footype.baz"));
  Code code(&scope, makeTestCode(thread));
  Object closure(&scope, runtime->newTuple(0));
  Dict annotations(&scope, runtime->newDict());
  Object return_name(&scope, runtime->newStrFromCStr("return"));
  Object int_type(&scope, runtime->typeAt(LayoutId::kInt));
  runtime->dictAtPut(thread, annotations, return_name, int_type);
  Dict kw_defaults(&scope, runtime->newDict());
  Object name0(&scope, runtime->newStrFromCStr("name0"));
  Object none(&scope, NoneType::object());
  runtime->dictAtPut(thread, kw_defaults, name0, none);
  Tuple defaults(&scope, runtime->newTuple(1));
  defaults.atPut(0, runtime->newInt(-9));
  Dict globals(&scope, runtime->newDict());
  Function func(&scope, Interpreter::makeFunction(
                            thread, qualname, code, closure, annotations,
                            kw_defaults, defaults, globals));
  func.setModule(runtime->newStrFromCStr("barmodule"));
  func.setName(runtime->newStrFromCStr("baz"));
  Dict attrs(&scope, runtime->newDict());
  Object attr_name(&scope, runtime->newStrFromCStr("funcattr0"));
  Object attr_value(&scope, runtime->newInt(4));
  runtime->dictAtPut(thread, attrs, attr_name, attr_value);
  func.setDict(*attrs);
  return *func;
}

TEST(DebuggingTests, DumpExtendedCode) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object code(&scope, makeTestCode(thread));

  std::stringstream ss;
  dumpExtended(ss, *code);
  EXPECT_EQ(ss.str(),
            R"(code "name0":
  flags: optimized newlocals varargs varkeyargs nested
  argcount: 1
  kwonlyargcount: 0
  nlocals: 4
  stacksize: 1
  filename: "filename0"
  consts: ("const0",)
  names: ("name0",)
  cellvars: ("cellvar0",)
  freevars: ("freevar0",)
  varnames: ("argument0", "varargs", "varkeyargs", "variable0")
     0 LOAD_CONST 0
     2 LOAD_ATTR 0
     4 RETURN_VALUE 0
)");
}

TEST(DebuggingTests, DumpExtendedFunction) {
  Runtime runtime(/*cache_enabled=*/true);
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object func(&scope, makeTestFunction(thread));
  std::stringstream ss;
  dumpExtended(ss, *func);
  EXPECT_EQ(ss.str(), R"(function "baz":
  qualname: "footype.baz"
  module: "barmodule"
  annotations: {"return": <type "int">}
  closure: ()
  defaults: (-9,)
  kwdefaults: {"name0": None}
  dict: {"funcattr0": 4}
  code: code "name0":
    flags: optimized newlocals varargs varkeyargs nested
    argcount: 1
    kwonlyargcount: 0
    nlocals: 4
    stacksize: 1
    filename: "filename0"
    consts: ("const0",)
    names: ("name0",)
    cellvars: ("cellvar0",)
    freevars: ("freevar0",)
    varnames: ("argument0", "varargs", "varkeyargs", "variable0")
       0 LOAD_CONST 0
       2 LOAD_ATTR 0
       4 RETURN_VALUE 0
  Rewritten bytecode:
     0 LOAD_CONST 0
     2 LOAD_ATTR_CACHED 0
     4 RETURN_VALUE 0
)");
}

TEST(DebuggingTests, DumpExtendedHeapObject) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  def __init__(self):
    self.foo = 5
    self.bar = "hello"
i = C()
i.baz = ()
)")
                   .isError());
  Object i(&scope, moduleAt(&runtime, "__main__", "i"));
  ASSERT_TRUE(i.isHeapObject());
  std::stringstream ss;
  dumpExtended(ss, *i);
  EXPECT_EQ(ss.str(), R"(heap object <type "C">:
  (in-object) "foo" = 5
  (in-object) "bar" = "hello"
  (overflow)  "baz" = ()
)");
}

TEST(DebuggingTests, DumpExtendedHeapObjectWithOverflowDict) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object func(&scope, makeTestFunction(thread));
  std::stringstream ss;
  dumpExtendedHeapObject(ss, RawHeapObject::cast(*func));
  EXPECT_EQ(ss.str(), R"(heap object <type "function">:
  (in-object) "__code__" = <code "name0">
  (in-object) "__doc__" = "const0"
  (in-object) "__globals__" = {}
  (in-object) "__module__" = "barmodule"
  (in-object) "__name__" = "baz"
  (in-object) "__qualname__" = "footype.baz"
  (in-object) "__dict__" = {"funcattr0": 4}
  overflow dict: {"funcattr0": 4}
)");
}

TEST(DebuggingTests, FormatBool) {
  Runtime runtime;
  std::stringstream ss;
  ss << Bool::trueObj() << ';' << Bool::falseObj();
  EXPECT_EQ(ss.str(), "True;False");
}

TEST(DebuggingTests, FormatBoundMethod) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  def foo():
    pass
bound_method = C().foo
)")
                   .isError());
  Object bound_method(&scope, moduleAt(&runtime, "__main__", "bound_method"));
  ASSERT_TRUE(bound_method.isBoundMethod());
  std::stringstream ss;
  ss << bound_method;
  EXPECT_EQ(ss.str(), "<bound_method \"C.foo\", <\"C\" object>>");
}

TEST(DebuggingTests, FormatCode) {
  Runtime runtime;
  HandleScope scope;
  Code code(&scope, newEmptyCode());
  code.setName(runtime.newStrFromCStr("foobar"));
  std::stringstream ss;
  ss << code;
  EXPECT_EQ(ss.str(), "<code \"foobar\">");
}

TEST(DebuggingTests, FormatDict) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Dict dict(&scope, runtime.newDict());
  Object key0(&scope, runtime.newStrFromCStr("hello"));
  Object key1(&scope, NoneType::object());
  Object value0(&scope, runtime.newInt(88));
  Object value1(&scope, runtime.newTuple(0));
  runtime.dictAtPut(thread, dict, key0, value0);
  runtime.dictAtPut(thread, dict, key1, value1);
  std::stringstream ss;
  ss << dict;
  EXPECT_TRUE(ss.str() == R"({"hello": 88, None: ()})" ||
              ss.str() == R"({None: (), "hello": 88}")");
}

TEST(DebuggingTests, FormatError) {
  Runtime runtime;
  std::stringstream ss;
  ss << Error::error();
  EXPECT_EQ(ss.str(), "Error");

  ss.str("");
  ss << Error::exception();
  EXPECT_EQ(ss.str(), "Error<Exception>");

  ss.str("");
  ss << Error::notFound();
  EXPECT_EQ(ss.str(), "Error<NotFound>");

  ss.str("");
  ss << Error::noMoreItems();
  EXPECT_EQ(ss.str(), "Error<NoMoreItems>");

  ss.str("");
  ss << Error::outOfMemory();
  EXPECT_EQ(ss.str(), "Error<OutOfMemory>");

  ss.str("");
  ss << Error::outOfBounds();
  EXPECT_EQ(ss.str(), "Error<OutOfBounds>");
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
  const uword digits[] = {0x12345, kMaxUword};
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
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  List list(&scope, runtime.newList());
  Object o0(&scope, NoneType::object());
  Object o1(&scope, runtime.newInt(17));
  runtime.listAdd(thread, list, o0);
  runtime.listAdd(thread, list, o1);
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
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo:
  pass
foo = Foo()
)")
                   .isError());
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
  Type::cast(runtime.typeOf(*obj)).setName(NoneType::object());

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
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class MyClass:
  pass
)")
                   .isError());
  Object my_class(&scope, moduleAt(&runtime, "__main__", "MyClass"));
  std::stringstream ss;
  ss << my_class;
  EXPECT_EQ(ss.str(), "<type \"MyClass\">");
}

TEST(DebuggingTests, FormatFrame) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
def func(arg0, arg1):
  hello = "world"
  return arg0 + arg1
)")
                   .isError());
  Function func(&scope, moduleAt(&runtime, "__main__", "func"));

  Thread* thread = Thread::current();
  Frame* root = thread->currentFrame();
  root->setVirtualPC(8);
  root->pushValue(NoneType::object());
  Function function(&scope, makeTestFunction(thread));
  root->pushValue(*function);
  root->pushValue(runtime.newStrFromCStr("foo bar"));
  root->pushValue(runtime.newTuple(0));
  root->pushValue(runtime.newDict());
  ASSERT_EQ(root->previousFrame(), nullptr);

  Frame* frame0 = thread->pushCallFrame(*function);
  frame0->setVirtualPC(42);
  frame0->setLocal(3, runtime.newStrFromCStr("bar foo"));
  frame0->pushValue(*func);
  frame0->pushValue(runtime.newInt(-9));
  frame0->pushValue(runtime.newInt(17));
  Frame* frame1 = thread->pushCallFrame(*func);
  frame1->setVirtualPC(4);
  frame1->setLocal(2, runtime.newStrFromCStr("world"));

  std::stringstream ss;
  ss << thread->currentFrame();
  EXPECT_EQ(ss.str(), R"(- initial frame
  pc: 8
  stack:
    4: None
    3: <function "footype.baz">
    2: "foo bar"
    1: ()
    0: {}
- function: <function "footype.baz">
  code: "name0"
  pc: 42 ("filename0":0)
  locals:
    0 "argument0": "foo bar"
    1 "varargs": ()
    2 "varkeyargs": {}
    3 "variable0": "bar foo"
    4 "freevar0": 0
    5 "cellvar0": 0
  stack:
    2: <function "func">
    1: -9
    0: 17
- function: <function "func">
  code: "func"
  pc: 4 ("<test string>":4)
  locals:
    0 "arg0": -9
    1 "arg1": 17
    2 "hello": "world"
)");
}

TEST(DebuggingTests, FormatFrameNullptr) {
  Runtime runtime;
  std::stringstream ss;
  ss << static_cast<Frame*>(nullptr);
  EXPECT_EQ(ss.str(), "<nullptr>");
}

}  // namespace python
