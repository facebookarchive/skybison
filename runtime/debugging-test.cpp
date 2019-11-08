#include "gtest/gtest.h"

#include "gmock/gmock-matchers.h"

#include <iostream>

#include "bytecode.h"
#include "debugging.h"
#include "dict-builtins.h"
#include "test-utils.h"

namespace py {

using namespace testing;

using DebuggingTests = RuntimeFixture;

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
  word posonlyargcount = 0;
  word kwonlyargcount = 0;
  word nlocals = 4;
  word stacksize = 1;
  word flags = Code::kNested | Code::kOptimized | Code::kNewlocals |
               Code::kVarargs | Code::kVarkeyargs;
  return runtime->newCode(argcount, posonlyargcount, kwonlyargcount, nlocals,
                          stacksize, flags, bytes, consts, names, varnames,
                          freevars, cellvars, filename, name, 0, lnotab);
}

static RawObject makeTestFunction(Thread* thread) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object qualname(&scope, runtime->newStrFromCStr("footype.baz"));
  Code code(&scope, makeTestCode(thread));
  Module module(&scope, runtime->findOrCreateMainModule());
  Function func(&scope,
                runtime->newFunctionWithCode(thread, qualname, code, module));
  Dict annotations(&scope, runtime->newDict());
  Str return_name(&scope, runtime->newStrFromCStr("return"));
  Object int_type(&scope, runtime->typeAt(LayoutId::kInt));
  dictAtPutByStr(thread, annotations, return_name, int_type);
  func.setAnnotations(*annotations);
  func.setClosure(runtime->emptyTuple());
  Dict kw_defaults(&scope, runtime->newDict());
  Str name0(&scope, runtime->newStrFromCStr("name0"));
  Object none(&scope, NoneType::object());
  dictAtPutByStr(thread, kw_defaults, name0, none);
  func.setKwDefaults(*kw_defaults);
  Tuple defaults(&scope, runtime->newTuple(1));
  defaults.atPut(0, runtime->newInt(-9));
  func.setDefaults(*defaults);
  func.setIntrinsicId(static_cast<word>(SymbolId::kList));
  func.setModule(runtime->newStrFromCStr("barmodule"));
  func.setName(runtime->newStrFromCStr("baz"));
  Dict attrs(&scope, runtime->newDict());
  Str attr_name(&scope, runtime->newStrFromCStr("funcattr0"));
  Object attr_value(&scope, runtime->newInt(4));
  dictAtPutByStr(thread, attrs, attr_name, attr_value);
  func.setDict(*attrs);
  return *func;
}

TEST_F(DebuggingTests, DumpExtendedCode) {
  HandleScope scope(thread_);
  Object code(&scope, makeTestCode(thread_));

  std::stringstream ss;
  dumpExtended(ss, *code);
  EXPECT_EQ(ss.str(),
            R"(code "name0":
  flags: optimized newlocals varargs varkeyargs nested
  argcount: 1
  posonlyargcount: 0
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

TEST_F(DebuggingTests, DumpExtendedFunction) {
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
  intrinsic_id: list
  dict: {"funcattr0": 4}
  flags: optimized newlocals varargs varkeyargs nested interpreted
  code: code "name0":
    flags: optimized newlocals varargs varkeyargs nested
    argcount: 1
    posonlyargcount: 0
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
     2 LOAD_ATTR_CACHED 1
     4 RETURN_VALUE 0
)");
}

TEST_F(DebuggingTests, DumpExtendedInstance) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  def __init__(self):
    self.foo = 5
    self.bar = "hello"
i = C()
i.baz = ()
)")
                   .isError());
  Object i(&scope, mainModuleAt(&runtime_, "i"));
  ASSERT_TRUE(i.isInstance());
  std::stringstream ss;
  dumpExtended(ss, *i);
  std::stringstream expected;
  expected << "heap object with layout " << static_cast<word>(i.layoutId())
           << R"( (<type "C">):
  (in-object) "foo" = 5
  (in-object) "bar" = "hello"
  (overflow)  "baz" = ()
)";
  EXPECT_EQ(ss.str(), expected.str());
}

TEST_F(DebuggingTests, DumpExtendedInstanceWithOverflowDict) {
  HandleScope scope(thread_);
  Function func(&scope, makeTestFunction(thread_));
  std::stringstream ss;
  dumpExtendedInstance(ss, RawInstance::cast(*func));
  std::stringstream expected;
  expected << "heap object with layout " << static_cast<word>(func.layoutId())
           << R"( (<type "function">):
  (in-object) "__code__" = <code "name0">
  (in-object) "__doc__" = "const0"
  (in-object) "__module__" = "barmodule"
  (in-object) "__module_object__" = <module "__main__">
  (in-object) "__name__" = "baz"
  (in-object) "__qualname__" = "footype.baz"
  (in-object) None = {"funcattr0": 4}
  overflow dict: {"funcattr0": 4}
)";
  EXPECT_EQ(ss.str(), expected.str());
}

TEST_F(DebuggingTests, DumpExtendedLayout) {
  HandleScope scope(thread_);
  // Create a new layout with several overflow attributes
  Object attr(&scope, runtime_.newStrFromCStr("myattr"));
  Object attr2(&scope, runtime_.newStrFromCStr("myattr2"));
  Object attr3(&scope, runtime_.newStrFromCStr("myattr3"));
  Tuple overflow(&scope, runtime_.newTuple(3));
  Object* overflow_names[] = {&attr, &attr2, &attr3};
  for (word i = 0; i < overflow.length(); i++) {
    Tuple entry(&scope, runtime_.newTuple(2));
    entry.atPut(0, **overflow_names[i]);
    entry.atPut(1, AttributeInfo(i, 0).asSmallInt());
    overflow.atPut(i, *entry);
  }
  Layout layout(&scope, layoutCreateEmpty(thread_));
  layout.setOverflowAttributes(*overflow);

  // Set some in-object attributes
  Object inobj1(&scope, runtime_.newStrFromCStr("foo"));
  Object inobj2(&scope, runtime_.newStrFromCStr("bar"));
  Tuple inobj(&scope, runtime_.newTuple(2));
  Object* inobj_names[] = {&inobj1, &inobj2};
  for (word i = 0; i < inobj.length(); i++) {
    Tuple entry(&scope, runtime_.newTuple(2));
    entry.atPut(0, **inobj_names[i]);
    entry.atPut(1, AttributeInfo(i, 0).asSmallInt());
    inobj.atPut(i, *entry);
  }
  layout.setInObjectAttributes(*inobj);
  layout.setNumInObjectAttributes(9);
  layout.setId(static_cast<LayoutId>(103));

  Type type(&scope, runtime_.typeAt(LayoutId::kObject));
  layout.setDescribedType(*type);

  std::stringstream ss;
  dumpExtended(ss, *layout);
  EXPECT_EQ(ss.str(), R"(layout 103:
  described type: <type "object">
  num in-object attributes: 9
    "foo" @ 0
    "bar" @ 1
  overflow tuple:
    "myattr" @ 0
    "myattr2" @ 1
    "myattr3" @ 2
)");
}

TEST_F(DebuggingTests, DumpExtendedLayoutWithSealedLayout) {
  HandleScope scope(thread_);
  Layout layout(&scope, layoutCreateEmpty(thread_));
  layout.setOverflowAttributes(NoneType::object());
  // Set some in-object attributes
  Object inobj1(&scope, runtime_.newStrFromCStr("foo"));
  Object inobj2(&scope, runtime_.newStrFromCStr("bar"));
  Tuple inobj(&scope, runtime_.newTuple(2));
  Object* inobj_names[] = {&inobj1, &inobj2};
  for (word i = 0; i < inobj.length(); i++) {
    Tuple entry(&scope, runtime_.newTuple(2));
    entry.atPut(0, **inobj_names[i]);
    entry.atPut(1, AttributeInfo(i, 0).asSmallInt());
    inobj.atPut(i, *entry);
  }
  layout.setInObjectAttributes(*inobj);
  layout.setId(static_cast<LayoutId>(13));
  layout.setNumInObjectAttributes(2);

  std::stringstream ss;
  dumpExtended(ss, *layout);
  EXPECT_EQ(ss.str(), R"(layout 13:
  described type: None
  num in-object attributes: 2
    "foo" @ 0
    "bar" @ 1
  sealed
)");
}

TEST_F(DebuggingTests, DumpExtendedLayoutWithDictOverflow) {
  HandleScope scope(thread_);
  Layout layout(&scope, layoutCreateEmpty(thread_));
  layout.setOverflowAttributes(SmallInt::fromWord(654321));
  layout.setInObjectAttributes(runtime_.emptyTuple());
  layout.setNumInObjectAttributes(0);
  layout.setId(static_cast<LayoutId>(1234));

  std::stringstream ss;
  dumpExtended(ss, *layout);
  EXPECT_EQ(ss.str(), R"(layout 1234:
  described type: None
  num in-object attributes: 0
  overflow dict @ 654321
)");
}

TEST_F(DebuggingTests, DumpExtendedType) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class A:
  pass
class B(bytes):
  pass
class C(A, B):
  def __init__(self):
    self.x = 0
    self.y = 1
)")
                   .isError());
  Object c(&scope, mainModuleAt(&runtime_, "C"));
  ASSERT_TRUE(c.isType());

  std::stringstream ss;
  dumpExtended(ss, *c);
  std::stringstream expected;
  expected << R"(type "C":
  bases: (<type "A">, <type "B">)
  mro: (<type "C">, <type "A">, <type "B">, <type "bytes">, <type "object">)
  flags: has_dunder_dict
  builtin base: <layout )"
           << static_cast<word>(LayoutId::kBytes) << R"( ("bytes")>
  layout )"
           << static_cast<word>(
                  Layout::cast(Type::cast(*c).instanceLayout()).id())
           << R"(:
    described type: <type "C">
    num in-object attributes: 3
      None @ 0
    overflow tuple:
)";
  EXPECT_EQ(ss.str(), expected.str());
}

TEST_F(DebuggingTests, FormatBool) {
  std::stringstream ss;
  ss << Bool::trueObj() << ';' << Bool::falseObj();
  EXPECT_EQ(ss.str(), "True;False");
}

TEST_F(DebuggingTests, FormatBoundMethod) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  def foo():
    pass
bound_method = C().foo
)")
                   .isError());
  Object bound_method(&scope, mainModuleAt(&runtime_, "bound_method"));
  ASSERT_TRUE(bound_method.isBoundMethod());
  std::stringstream ss;
  ss << bound_method;
  EXPECT_EQ(ss.str(), "<bound_method \"C.foo\", <\"C\" object>>");
}

TEST_F(DebuggingTests, FormatByteArray) {
  ASSERT_FALSE(runFromCStr(&runtime_, "ba = bytearray(b\"foo'\")").isError());
  HandleScope scope(thread_);
  Object bytearray(&scope, mainModuleAt(&runtime_, "ba"));
  ASSERT_TRUE(bytearray.isByteArray());
  std::stringstream ss;
  ss << bytearray;
  EXPECT_EQ(ss.str(), R"(bytearray(b"foo\'"))");
}

TEST_F(DebuggingTests, FormatCode) {
  HandleScope scope(thread_);
  Code code(&scope, newEmptyCode());
  code.setName(runtime_.newStrFromCStr("foobar"));
  std::stringstream ss;
  ss << code;
  EXPECT_EQ(ss.str(), "<code \"foobar\">");
}

TEST_F(DebuggingTests, FormatDict) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_.newDict());
  Str key0(&scope, runtime_.newStrFromCStr("hello"));
  Object key1(&scope, NoneType::object());
  Object hash_obj(&scope, Interpreter::hash(thread_, key1));
  ASSERT_FALSE(hash_obj.isErrorException());
  word hash = SmallInt::cast(*hash_obj).value();
  Object value0(&scope, runtime_.newInt(88));
  Object value1(&scope, runtime_.emptyTuple());
  dictAtPutByStr(thread_, dict, key0, value0);
  dictAtPut(thread_, dict, key1, hash, value1);
  std::stringstream ss;
  ss << dict;
  EXPECT_TRUE(ss.str() == R"({"hello": 88, None: ()})" ||
              ss.str() == R"({None: (), "hello": 88}")");
}

TEST_F(DebuggingTests, FormatError) {
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

TEST_F(DebuggingTests, FormatFloat) {
  std::stringstream ss;
  ss << runtime_.newFloat(42.42);
  EXPECT_EQ(ss.str(), "0x1.535c28f5c28f6p+5");
}

TEST_F(DebuggingTests, FormatFunction) {
  HandleScope scope(thread_);
  std::stringstream ss;
  Object function(&scope, moduleAtByCStr(&runtime_, "builtins", "callable"));
  ASSERT_TRUE(function.isFunction());
  ss << function;
  EXPECT_EQ(ss.str(), R"(<function "callable">)");
}

TEST_F(DebuggingTests, FormatLargeInt) {
  std::stringstream ss;
  const uword digits[] = {0x12345, kMaxUword};
  ss << runtime_.newIntWithDigits(digits);
  EXPECT_EQ(ss.str(), "largeint([0x0000000000012345, 0xffffffffffffffff])");
}

TEST_F(DebuggingTests, FormatLargeStr) {
  HandleScope scope(thread_);
  std::stringstream ss;
  Object str(&scope, runtime_.newStrFromCStr("hello world"));
  EXPECT_TRUE(str.isLargeStr());
  ss << str;
  EXPECT_EQ(ss.str(), "\"hello world\"");
}

TEST_F(DebuggingTests, FormatLayout) {
  HandleScope scope(thread_);
  Layout layout(&scope, layoutCreateEmpty(thread_));
  layout.setId(static_cast<LayoutId>(101));
  Type type(&scope, runtime_.typeAt(LayoutId::kFloat));
  layout.setDescribedType(*type);

  std::stringstream ss;
  ss << layout;
  EXPECT_EQ(ss.str(), "<layout 101 (\"float\")>");
}

TEST_F(DebuggingTests, FormatList) {
  HandleScope scope(thread_);
  List list(&scope, runtime_.newList());
  Object o0(&scope, NoneType::object());
  Object o1(&scope, runtime_.newInt(17));
  runtime_.listAdd(thread_, list, o0);
  runtime_.listAdd(thread_, list, o1);
  std::stringstream ss;
  ss << list;
  EXPECT_EQ(ss.str(), "[None, 17]");
}

TEST_F(DebuggingTests, FormatModule) {
  HandleScope scope(thread_);
  Object name(&scope, runtime_.newStrFromCStr("foomodule"));
  Object module(&scope, runtime_.newModule(name));
  std::stringstream ss;
  ss << module;
  EXPECT_EQ(ss.str(), R"(<module "foomodule">)");
}

TEST_F(DebuggingTests, FormatNone) {
  std::stringstream ss;
  ss << NoneType::object();
  EXPECT_EQ(ss.str(), "None");
}

TEST_F(DebuggingTests, FormatObjectWithBuiltinClass) {
  std::stringstream ss;
  ss << NotImplementedType::object();
  EXPECT_EQ(ss.str(), R"(<"NotImplementedType" object>)");
}

TEST_F(DebuggingTests, FormatObjectWithUserDefinedClass) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Foo:
  pass
foo = Foo()
)")
                   .isError());
  Object foo(&scope, mainModuleAt(&runtime_, "foo"));
  std::stringstream ss;
  ss << foo;
  EXPECT_EQ(ss.str(), R"(<"Foo" object>)");
}

TEST_F(DebuggingTests, FormatObjectWithUnknownType) {
  HandleScope scope(thread_);
  Object obj(&scope, NotImplementedType::object());
  // Phabricate a nameless type...
  Type::cast(runtime_.typeOf(*obj)).setName(NoneType::object());

  std::stringstream ss;
  std::stringstream expected;
  ss << obj;
  expected << "<object with LayoutId " << static_cast<word>(obj.layoutId())
           << ">";
  EXPECT_EQ(ss.str(), expected.str());
}

TEST_F(DebuggingTests, FormatSmallInt) {
  std::stringstream ss;
  ss << SmallInt::fromWord(-42) << ';'
     << SmallInt::fromWord(SmallInt::kMinValue) << ';'
     << SmallInt::fromWord(SmallInt::kMaxValue);
  std::stringstream expected;
  expected << "-42;" << SmallInt::kMinValue << ";" << SmallInt::kMaxValue;
  EXPECT_EQ(ss.str(), expected.str());
}

TEST_F(DebuggingTests, FormatSmallStr) {
  HandleScope scope(thread_);
  std::stringstream ss;
  Object str(&scope, runtime_.newStrFromCStr("aa"));
  EXPECT_TRUE(str.isSmallStr());
  ss << str;
  EXPECT_EQ(ss.str(), "\"aa\"");
}

TEST_F(DebuggingTests, FormatTuple) {
  HandleScope scope(thread_);
  Tuple tuple(&scope, runtime_.newTuple(2));
  tuple.atPut(0, Bool::trueObj());
  tuple.atPut(1, runtime_.newStrFromCStr("hey"));
  std::stringstream ss;
  ss << tuple;
  EXPECT_EQ(ss.str(), R"((True, "hey"))");
}

TEST_F(DebuggingTests, FormatTupleWithoutElements) {
  std::stringstream ss;
  ss << runtime_.emptyTuple();
  EXPECT_EQ(ss.str(), "()");
}

TEST_F(DebuggingTests, FormatTupleWithOneElement) {
  HandleScope scope(thread_);
  Tuple tuple(&scope, runtime_.newTuple(1));
  tuple.atPut(0, runtime_.newInt(77));
  std::stringstream ss;
  ss << tuple;
  EXPECT_EQ(ss.str(), "(77,)");
}

TEST_F(DebuggingTests, FormatType) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class MyClass:
  pass
)")
                   .isError());
  Object my_class(&scope, mainModuleAt(&runtime_, "MyClass"));
  std::stringstream ss;
  ss << my_class;
  EXPECT_EQ(ss.str(), "<type \"MyClass\">");
}

TEST_F(DebuggingTests, FormatFrame) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def func(arg0, arg1):
  hello = "world"
  return arg0 + arg1
)")
                   .isError());
  Function func(&scope, mainModuleAt(&runtime_, "func"));

  Object empty_tuple(&scope, runtime_.emptyTuple());
  Str name(&scope, runtime_.newStrFromCStr("_bytearray_check"));
  Code code(&scope,
            runtime_.newBuiltinCode(/*argcount=*/0, /*posonlyargcount=*/0,
                                    /*kwonlyargcount=*/0,
                                    /*flags=*/0, /*entry=*/nullptr,
                                    /*parameter_names=*/empty_tuple, name));
  Str qualname(&scope, runtime_.newStrFromCStr("test._bytearray_check"));
  Module module(&scope, runtime_.findOrCreateMainModule());
  Function builtin(
      &scope, runtime_.newFunctionWithCode(thread_, qualname, code, module));

  Frame* root = thread_->currentFrame();
  ASSERT_TRUE(root->isSentinel());
  root->setVirtualPC(8);
  root->pushValue(NoneType::object());
  root->pushValue(*builtin);
  Frame* frame0 = thread_->pushNativeFrame(0);

  Function function(&scope, makeTestFunction(thread_));
  frame0->pushValue(*function);
  frame0->pushValue(runtime_.newStrFromCStr("foo bar"));
  frame0->pushValue(runtime_.emptyTuple());
  frame0->pushValue(runtime_.newDict());

  Frame* frame1 = thread_->pushCallFrame(*function);
  frame1->setVirtualPC(42);
  frame1->setLocal(3, runtime_.newStrFromCStr("bar foo"));
  frame1->setLocal(4, runtime_.newInt(88));
  frame1->setLocal(5, runtime_.newInt(-99));
  frame1->pushValue(*func);
  frame1->pushValue(runtime_.newInt(-9));
  frame1->pushValue(runtime_.newInt(17));
  Frame* frame2 = thread_->pushCallFrame(*func);
  frame2->setVirtualPC(4);
  frame2->setLocal(2, runtime_.newStrFromCStr("world"));

  std::stringstream ss;
  ss << thread_->currentFrame();
  EXPECT_EQ(ss.str(), R"(- initial frame
  pc: 8
  stack:
    1: None
    0: <function "test._bytearray_check">
- function: <function "test._bytearray_check">
  code: "_bytearray_check"
  pc: n/a (native)
  stack:
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
    4 "freevar0": 88
    5 "cellvar0": -99
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

TEST_F(DebuggingTests, FormatFrameNullptr) {
  std::stringstream ss;
  ss << static_cast<Frame*>(nullptr);
  EXPECT_EQ(ss.str(), "<nullptr>");
}

}  // namespace py
