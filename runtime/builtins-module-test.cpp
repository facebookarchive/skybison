#include "gtest/gtest.h"

#include "builtins-module.h"
#include "runtime.h"
#include "test-utils.h"
#include "trampolines.h"

namespace python {

using namespace testing;

TEST(BuiltinsModuleTest, BuiltinCallableOnTypeReturnsTrue) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
class Foo:
  pass

a = callable(Foo)
  )");
  HandleScope scope;
  Module main(&scope, findModule(&runtime, "__main__"));
  Bool a(&scope, moduleAt(&runtime, main, "a"));
  EXPECT_TRUE(a.value());
}

TEST(BuiltinsModuleTest, BuiltinCallableOnMethodReturnsTrue) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
class Foo:
  def bar():
    return None

a = callable(Foo.bar)
b = callable(Foo().bar)
  )");
  HandleScope scope;
  Module main(&scope, findModule(&runtime, "__main__"));
  Bool a(&scope, moduleAt(&runtime, main, "a"));
  Bool b(&scope, moduleAt(&runtime, main, "b"));
  EXPECT_TRUE(a.value());
  EXPECT_TRUE(b.value());
}

TEST(BuiltinsModuleTest, BuiltinCallableOnNonCallableReturnsFalse) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
a = callable(1)
b = callable("hello")
  )");
  HandleScope scope;
  Module main(&scope, findModule(&runtime, "__main__"));
  Bool a(&scope, moduleAt(&runtime, main, "a"));
  Bool b(&scope, moduleAt(&runtime, main, "b"));
  EXPECT_FALSE(a.value());
  EXPECT_FALSE(b.value());
}

TEST(BuiltinsModuleTest, BuiltinCallableOnObjectWithCallOnTypeReturnsTrue) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
class Foo:
  def __call__(self):
    pass

f = Foo()
a = callable(f)
  )");
  HandleScope scope;
  Module main(&scope, findModule(&runtime, "__main__"));
  Bool a(&scope, moduleAt(&runtime, main, "a"));
  EXPECT_TRUE(a.value());
}

TEST(BuiltinsModuleTest,
     BuiltinCallableOnObjectWithInstanceCallButNoTypeCallReturnsFalse) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
class Foo:
  pass

def fakecall():
  pass

f = Foo()
f.__call__ = fakecall
a = callable(f)
  )");
  HandleScope scope;
  Module main(&scope, findModule(&runtime, "__main__"));
  Bool a(&scope, moduleAt(&runtime, main, "a"));
  EXPECT_FALSE(a.value());
}

TEST(BuiltinsModuleTest, BuiltinChr) {
  Runtime runtime;
  std::string result = compileAndRunToString(&runtime, "print(chr(65))");
  EXPECT_EQ(result, "A\n");
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "print(chr(1,2))"), LayoutId::kTypeError,
      "TypeError: 'chr' takes max 1 positional arguments but 2 given"));
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "print(chr('A'))"),
                            LayoutId::kTypeError,
                            "Unsupported type in builtin 'chr'"));
}

TEST(BuiltinsModuleTest, BuiltinIsinstance) {
  Runtime runtime;
  HandleScope scope;

  // Only accepts 2 arguments
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "print(isinstance(1, 1, 1))"), LayoutId::kTypeError,
      "TypeError: 'isinstance' takes max 2 positional arguments but 3 given"));
  Thread::currentThread()->clearPendingException();

  // 2nd argument must be a type
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "print(isinstance(1, 1))"), LayoutId::kTypeError,
      "isinstance() arg 2 must be a type or tuple of types"));
  Thread::currentThread()->clearPendingException();

  const char* src = R"(
class A: pass
class B(A): pass
class C(A): pass
class D(C, B): pass

def test(a, b):
  print(isinstance(a, b))
)";
  runFromCStr(&runtime, src);

  // We can move these tests into the python code above once we can
  // call classes.
  RawObject object = testing::findModule(&runtime, "__main__");
  ASSERT_TRUE(object.isModule());
  Module main(&scope, object);

  // Create an instance of D
  Object type_d(&scope, moduleAt(&runtime, main, "D"));
  ASSERT_TRUE(type_d.isType());
  Layout layout(&scope, RawType::cast(*type_d).instanceLayout());
  Object instance(&scope, runtime.newInstance(layout));

  // Fetch the test function
  object = moduleAt(&runtime, main, "test");
  ASSERT_TRUE(object.isFunction());
  Function isinstance(&scope, object);

  // isinstance(1, D) should be false
  Tuple args(&scope, runtime.newTuple(2));
  args.atPut(0, SmallInt::fromWord(100));
  args.atPut(1, *type_d);
  EXPECT_EQ(callFunctionToString(isinstance, args), "False\n");

  // isinstance(D, D) should be false
  args.atPut(0, *type_d);
  args.atPut(1, *type_d);
  EXPECT_EQ(callFunctionToString(isinstance, args), "False\n");

  // isinstance(D(), D) should be true
  args.atPut(0, *instance);
  args.atPut(1, *type_d);
  EXPECT_EQ(callFunctionToString(isinstance, args), "True\n");

  // isinstance(D(), C) should be true
  Object type_c(&scope, moduleAt(&runtime, main, "C"));
  ASSERT_TRUE(type_c.isType());
  args.atPut(1, *type_c);
  EXPECT_EQ(callFunctionToString(isinstance, args), "True\n");

  // isinstance(D(), B) should be true
  Object type_b(&scope, moduleAt(&runtime, main, "B"));
  ASSERT_TRUE(type_b.isType());
  args.atPut(1, *type_b);
  EXPECT_EQ(callFunctionToString(isinstance, args), "True\n");

  // isinstance(C(), A) should be true
  Object type_a(&scope, moduleAt(&runtime, main, "A"));
  ASSERT_TRUE(type_a.isType());
  args.atPut(1, *type_a);
  EXPECT_EQ(callFunctionToString(isinstance, args), "True\n");
}

TEST(BuiltinsModuleTest, IsinstanceAcceptsTypeTuple) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);

  Tuple types(&scope, runtime.newTuple(3));
  types.atPut(0, runtime.typeAt(LayoutId::kStr));
  types.atPut(1, runtime.typeAt(LayoutId::kBool));
  types.atPut(2, runtime.typeAt(LayoutId::kException));

  Object str(&scope, runtime.newStrFromCStr("hello there!"));
  EXPECT_EQ(runBuiltin(BuiltinsModule::isinstance, str, types),
            Bool::trueObj());

  Object an_int(&scope, runtime.newInt(123));
  EXPECT_EQ(runBuiltin(BuiltinsModule::isinstance, an_int, types),
            Bool::falseObj());

  Object a_bool(&scope, Bool::falseObj());
  EXPECT_EQ(runBuiltin(BuiltinsModule::isinstance, a_bool, types),
            Bool::trueObj());

  Layout exc_layout(&scope, runtime.layoutAt(LayoutId::kException));
  Object exc(&scope, runtime.newInstance(exc_layout));
  EXPECT_EQ(runBuiltin(BuiltinsModule::isinstance, exc, types),
            Bool::trueObj());

  Object bytes(&scope, runtime.newBytes(0, 0));
  EXPECT_EQ(runBuiltin(BuiltinsModule::isinstance, bytes, types),
            Bool::falseObj());

  Tuple inner(&scope, runtime.newTuple(2));
  inner.atPut(0, runtime.typeAt(LayoutId::kInt));
  inner.atPut(1, runtime.typeAt(LayoutId::kBytes));
  types.atPut(2, *inner);

  EXPECT_EQ(runBuiltin(BuiltinsModule::isinstance, str, types),
            Bool::trueObj());
  EXPECT_EQ(runBuiltin(BuiltinsModule::isinstance, an_int, types),
            Bool::trueObj());
  EXPECT_EQ(runBuiltin(BuiltinsModule::isinstance, a_bool, types),
            Bool::trueObj());
  EXPECT_EQ(runBuiltin(BuiltinsModule::isinstance, exc, types),
            Bool::falseObj());
  EXPECT_EQ(runBuiltin(BuiltinsModule::isinstance, bytes, types),
            Bool::trueObj());

  inner.atPut(1, *an_int);
  EXPECT_TRUE(raised(runBuiltin(BuiltinsModule::isinstance, exc, types),
                     LayoutId::kTypeError));
}

TEST(BuiltinsModuleTest, BuiltinIssubclassWithSubclassReturnsTrue) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
class Foo:
  pass

class Bar(Foo):
  pass

class Baz(type):
  pass

a = issubclass(Foo, object)
b = issubclass(Bar, Foo)
c = issubclass(Baz, type)
)");
  HandleScope scope;
  Module main(&scope, findModule(&runtime, "__main__"));
  Bool a(&scope, moduleAt(&runtime, main, "a"));
  Bool b(&scope, moduleAt(&runtime, main, "b"));
  Bool c(&scope, moduleAt(&runtime, main, "c"));
  EXPECT_TRUE(a.value());
  EXPECT_TRUE(b.value());
  EXPECT_TRUE(c.value());
}

TEST(BuiltinsModuleTest, BuiltinIssubclassWithNonSubclassReturnsFalse) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
class Foo:
  pass

class Bar(Foo):
  pass

a = issubclass(Foo, Bar)
b = issubclass(int, str)
c = issubclass(dict, list)
)");
  HandleScope scope;
  Module main(&scope, findModule(&runtime, "__main__"));
  Bool a(&scope, moduleAt(&runtime, main, "a"));
  Bool b(&scope, moduleAt(&runtime, main, "b"));
  Bool c(&scope, moduleAt(&runtime, main, "c"));
  EXPECT_FALSE(a.value());
  EXPECT_FALSE(b.value());
  EXPECT_FALSE(c.value());
}

TEST(BuiltinsModuleTest, BuiltinIssubclassWithOneSuperclassReturnsTrue) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
class Foo:
  pass

class Bar(Foo):
  pass

a = issubclass(Foo, (Bar, object))
b = issubclass(Bar, (Foo))
)");
  HandleScope scope;
  Module main(&scope, findModule(&runtime, "__main__"));
  Bool a(&scope, moduleAt(&runtime, main, "a"));
  Bool b(&scope, moduleAt(&runtime, main, "b"));
  EXPECT_TRUE(a.value());
  EXPECT_TRUE(b.value());
}

TEST(BuiltinsModuleTest, BuiltinIssubclassWithNoSuperclassReturnsFalse) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
class Foo:
  pass

a = issubclass(Foo, (str, int))
)");
  HandleScope scope;
  Module main(&scope, findModule(&runtime, "__main__"));
  Bool a(&scope, moduleAt(&runtime, main, "a"));
  EXPECT_FALSE(a.value());
}

TEST(BuiltinsModuleTest, BuiltinLen) {
  Runtime runtime;
  std::string result = compileAndRunToString(&runtime, "print(len([1,2,3]))");
  EXPECT_EQ(result, "3\n");
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "print(len(1,2))"), LayoutId::kTypeError,
      "TypeError: 'len' takes max 1 positional arguments but 2 given"));
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "print(len(1))"),
                            LayoutId::kTypeError, "object has no len()"));
}

TEST(ThreadTest, BuiltinLenGetLenFromDict) {
  Runtime runtime;

  runFromCStr(&runtime, R"(
len0 = len({})
len1 = len({'one': 1})
len5 = len({'one': 1, 'two': 2, 'three': 3, 'four': 4, 'five': 5})
)");

  HandleScope scope;
  Module main(&scope, findModule(&runtime, "__main__"));
  Object len0(&scope, moduleAt(&runtime, main, "len0"));
  EXPECT_EQ(*len0, SmallInt::fromWord(0));
  Object len1(&scope, moduleAt(&runtime, main, "len1"));
  EXPECT_EQ(*len1, SmallInt::fromWord(1));
  Object len5(&scope, moduleAt(&runtime, main, "len5"));
  EXPECT_EQ(*len5, SmallInt::fromWord(5));
}

TEST(ThreadTest, BuiltinLenGetLenFromList) {
  Runtime runtime;

  runFromCStr(&runtime, R"(
len0 = len([])
len1 = len([1])
len5 = len([1,2,3,4,5])
)");

  HandleScope scope;
  Module main(&scope, findModule(&runtime, "__main__"));
  Object len0(&scope, moduleAt(&runtime, main, "len0"));
  EXPECT_EQ(*len0, SmallInt::fromWord(0));
  Object len1(&scope, moduleAt(&runtime, main, "len1"));
  EXPECT_EQ(*len1, SmallInt::fromWord(1));
  Object len5(&scope, moduleAt(&runtime, main, "len5"));
  EXPECT_EQ(*len5, SmallInt::fromWord(5));
}

TEST(ThreadTest, BuiltinLenGetLenFromSet) {
  Runtime runtime;

  runFromCStr(&runtime, R"(
len1 = len({1})
len5 = len({1,2,3,4,5})
)");

  HandleScope scope;
  Module main(&scope, findModule(&runtime, "__main__"));
  // TODO(cshapiro): test the empty set when we have builtins.set defined.
  Object len1(&scope, moduleAt(&runtime, main, "len1"));
  EXPECT_EQ(*len1, SmallInt::fromWord(1));
  Object len5(&scope, moduleAt(&runtime, main, "len5"));
  EXPECT_EQ(*len5, SmallInt::fromWord(5));
}

TEST(BuiltinsModuleTest, BuiltinOrd) {
  Runtime runtime;
  std::string result = compileAndRunToString(&runtime, "print(ord('A'))");
  EXPECT_EQ(result, "65\n");
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "print(ord(1,2))"), LayoutId::kTypeError,
      "TypeError: 'ord' takes max 1 positional arguments but 2 given"));
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "print(ord(1))"),
                            LayoutId::kTypeError,
                            "Unsupported type in builtin 'ord'"));
}

TEST(BuiltinsModuleTest, BuiltInPrintStdOut) {
  const char* src = R"(
import sys
print("hello", file=sys.stdout)
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "hello\n");
}

TEST(BuiltinsModuleTest, BuiltInPrintEnd) {
  const char* src = R"(
import sys
print("hi", end='ho')
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "hiho");
}

TEST(BuiltinsModuleTest, BuiltInPrintStdOutEnd) {
  const char* src = R"(
import sys
print("hi", end='ho', file=sys.stdout)
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "hiho");
}

TEST(BuiltinsModuleTest, BuiltInPrintStdErr) {  // pystone dependency
  const char* src = R"(
import sys
print("hi", file=sys.stderr, end='ya')
)";
  Runtime runtime;
  std::string output = compileAndRunToStderrString(&runtime, src);
  EXPECT_EQ(output, "hiya");
}

TEST(BuiltinsModuleTest, BuiltInPrintNone) {
  const char* src = R"(
print(None)
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "None\n");
}

TEST(BuiltinsModuleTest, BuiltInPrintStrList) {
  const char* src = R"(
print(['one', 'two'])
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "['one', 'two']\n");
}

TEST(BuiltinsModuleTest, BuiltInReprOnUserTypeWithDunderRepr) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
class Foo:
  def __repr__(self):
    return "foo"

a = repr(Foo())
)");
  HandleScope scope;
  Module main(&scope, findModule(&runtime, "__main__"));
  Object a(&scope, moduleAt(&runtime, main, "a"));
  EXPECT_TRUE(isStrEqualsCStr(*a, "foo"));
}

TEST(BuiltinsModuleTest, BuiltInReprOnClass) {
  Runtime runtime;
  runFromCStr(&runtime, "result = repr(int)");
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isStrEqualsCStr(*result, "<class 'int'>"));
}

TEST(BuiltinsModuleTest, BuiltInAsciiCallsDunderRepr) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
class Foo:
  def __repr__(self):
    return "foo"

a = ascii(Foo())
)");
  HandleScope scope;
  Module main(&scope, findModule(&runtime, "__main__"));
  Object a(&scope, moduleAt(&runtime, main, "a"));
  EXPECT_TRUE(isStrEqualsCStr(*a, "foo"));
}

TEST(BuiltinsModuleTest, GetAttrFromClassReturnsValue) {
  const char* src = R"(
class Foo:
  bar = 1
obj = getattr(Foo, 'bar')
)";
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object obj(&scope, moduleAt(&runtime, main, "obj"));
  EXPECT_EQ(*obj, SmallInt::fromWord(1));
}

TEST(BuiltinsModuleTest, GetAttrFromInstanceReturnsValue) {
  const char* src = R"(
class Foo:
  bar = 1
obj = getattr(Foo(), 'bar')
)";
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object obj(&scope, moduleAt(&runtime, main, "obj"));
  EXPECT_EQ(*obj, SmallInt::fromWord(1));
}

TEST(BuiltinsModuleTest, GetAttrFromInstanceWithMissingAttrReturnsDefault) {
  const char* src = R"(
class Foo: pass
obj = getattr(Foo(), 'bar', 2)
)";
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object obj(&scope, moduleAt(&runtime, main, "obj"));
  EXPECT_EQ(*obj, SmallInt::fromWord(2));
}

TEST(BuiltinsModuleTest, GetAttrWithNonStringAttrRaisesTypeError) {
  const char* src = R"(
class Foo: pass
getattr(Foo(), 1)
)";
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, src), LayoutId::kTypeError,
                            "getattr(): attribute name must be string"));
}

TEST(BuiltinsModuleTest, GetAttrWithNonStringAttrAndDefaultRaisesTypeError) {
  const char* src = R"(
class Foo: pass
getattr(Foo(), 1, 2)
)";
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, src), LayoutId::kTypeError,
                            "getattr(): attribute name must be string"));
}

TEST(BuiltinsModuleTest,
     GetAttrFromClassMissingAttrWithoutDefaultRaisesAttributeError) {
  const char* src = R"(
class Foo:
  bar = 1
getattr(Foo, 'foo')
)";
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, src),
                            LayoutId::kAttributeError, "missing attribute"));
}

TEST(BuiltinsModuleTest, HasAttrFromClassMissingAttrReturnsFalse) {
  const char* src = R"(
class Foo: pass
obj = hasattr(Foo, 'bar')
)";
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object obj(&scope, moduleAt(&runtime, main, "obj"));
  EXPECT_EQ(*obj, Bool::falseObj());
}

TEST(BuiltinsModuleTest, HasAttrFromClassWithAttrReturnsTrue) {
  const char* src = R"(
class Foo:
  bar = 1
obj = hasattr(Foo, 'bar')
)";
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object obj(&scope, moduleAt(&runtime, main, "obj"));
  EXPECT_EQ(*obj, Bool::trueObj());
}

TEST(BuiltinsModuleTest, HasAttrWithNonStringAttrRaisesTypeError) {
  const char* src = R"(
class Foo:
  bar = 1
hasattr(Foo, 1)
)";
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, src), LayoutId::kTypeError,
                            "hasattr(): attribute name must be string"));
}

TEST(BuiltinsModuleTest, HasAttrFromInstanceMissingAttrReturnsFalse) {
  const char* src = R"(
class Foo: pass
obj = hasattr(Foo(), 'bar')
)";
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object obj(&scope, moduleAt(&runtime, main, "obj"));
  EXPECT_EQ(*obj, Bool::falseObj());
}

TEST(BuiltinsModuleTest, HasAttrFromInstanceWithAttrReturnsTrue) {
  const char* src = R"(
class Foo:
  bar = 1
obj = hasattr(Foo(), 'bar')
)";
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object obj(&scope, moduleAt(&runtime, main, "obj"));
  EXPECT_EQ(*obj, Bool::trueObj());
}

TEST(BuiltinsModuleTest, BuiltInSetAttr) {
  const char* src = R"(
class Foo:
  bar = 1
a = setattr(Foo, 'foo', 2)
b = Foo.foo
)";
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object a(&scope, moduleAt(&runtime, main, "a"));
  Object b(&scope, moduleAt(&runtime, main, "b"));
  EXPECT_EQ(*a, NoneType::object());
  EXPECT_EQ(*b, SmallInt::fromWord(2));
}

TEST(BuiltinsModuleTest, BuiltInSetAttrRaisesTypeError) {
  const char* src = R"(
class Foo:
  bar = 1
a = setattr(Foo, 2, 'foo')
)";
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, src), LayoutId::kTypeError,
                            "setattr(): attribute name must be string"));
}

TEST(BuiltinsModuleTest, ModuleAttrReturnsBuiltinsName) {
  // TODO(eelizondo): Parameterize test for all builtin types
  const char* src = R"(
a = hasattr(object, '__module__')
b = getattr(object, '__module__')
c = hasattr(list, '__module__')
d = getattr(list, '__module__')
)";
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, src);

  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_EQ(*a, Bool::trueObj());
  Object b(&scope, moduleAt(&runtime, "__main__", "b"));
  ASSERT_TRUE(b.isStr());
  EXPECT_TRUE(Str::cast(*b).equalsCStr("builtins"));

  Object c(&scope, moduleAt(&runtime, "__main__", "c"));
  EXPECT_EQ(*c, Bool::trueObj());
  Object d(&scope, moduleAt(&runtime, "__main__", "d"));
  ASSERT_TRUE(d.isStr());
  EXPECT_TRUE(Str::cast(*b).equalsCStr("builtins"));
}

TEST(BuiltinsModuleTest, QualnameAttrReturnsTypeName) {
  // TODO(eelizondo): Parameterize test for all builtin types
  const char* src = R"(
a = hasattr(object, '__qualname__')
b = getattr(object, '__qualname__')
c = hasattr(list, '__qualname__')
d = getattr(list, '__qualname__')
)";
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, src);

  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_EQ(*a, Bool::trueObj());
  Object b(&scope, moduleAt(&runtime, "__main__", "b"));
  ASSERT_TRUE(b.isStr());
  EXPECT_TRUE(Str::cast(*b).equalsCStr("object"));

  Object c(&scope, moduleAt(&runtime, "__main__", "c"));
  EXPECT_EQ(*c, Bool::trueObj());
  Object d(&scope, moduleAt(&runtime, "__main__", "d"));
  ASSERT_TRUE(d.isStr());
  EXPECT_TRUE(Str::cast(*d).equalsCStr("list"));
}

TEST(BuiltinsModuleTest, BuiltinCompile) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(
      &runtime,
      R"(code = compile("a = 1\nb = 2", "<string>", "eval", dont_inherit=True))");
  Str filename(&scope, runtime.newStrFromCStr("<string>"));
  Code code(&scope, moduleAt(&runtime, "__main__", "code"));
  ASSERT_TRUE(code.filename().isStr());
  EXPECT_TRUE(Str::cast(code.filename()).equals(*filename));

  ASSERT_TRUE(code.names().isTuple());
  Tuple names(&scope, code.names());
  ASSERT_EQ(names.length(), 2);
  ASSERT_TRUE(names.contains(runtime.newStrFromCStr("a")));
  ASSERT_TRUE(names.contains(runtime.newStrFromCStr("b")));
}

TEST(BuiltinsModuleTest, BuiltinCompileBytes) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
data = b'a = 1; b = 2'
code = compile(data, "<string>", "eval", dont_inherit=True)
)");
  Code code(&scope, moduleAt(&runtime, "__main__", "code"));
  Object filename(&scope, code.filename());
  EXPECT_TRUE(isStrEqualsCStr(*filename, "<string>"));

  ASSERT_TRUE(code.names().isTuple());
  Tuple names(&scope, code.names());
  ASSERT_EQ(names.length(), 2);
  ASSERT_TRUE(names.contains(runtime.newStrFromCStr("a")));
  ASSERT_TRUE(names.contains(runtime.newStrFromCStr("b")));
}

TEST(BuiltinsModuleDeathTest, BuiltinCompileRaisesTypeErrorGivenTooFewArgs) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "compile(1)"), LayoutId::kTypeError,
      "TypeError: 'compile' takes min 3 positional arguments but 1 given"));
}

TEST(BuiltinsModuleDeathTest, BuiltinCompileRaisesTypeErrorGivenTooManyArgs) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "compile(1, 2, 3, 4, 5, 6, 7, 8, 9)"),
      LayoutId::kTypeError,
      "TypeError: 'compile' takes max 6 positional arguments but 9 given"));
}

TEST(BuiltinsModuleTest, BuiltinCompileRaisesTypeErrorGivenBadMode) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime,
                  "compile('hello', 'hello', 'hello', dont_inherit=True)"),
      LayoutId::kValueError,
      "Expected mode to be 'exec', 'eval', or 'single' in 'compile'"));
}

TEST(BuiltinsModuleTest, BuiltinExecSetsGlobal) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
a = 1337
exec("a = 1338")
  )");
  // We can't use runBuiltin here because it does not set up the frame properly
  // for functions that need globals, implicitGlobals, etc
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_TRUE(isIntEqualsWord(*a, 1338));
}

TEST(BuiltinsModuleTest, BuiltinExecSetsGlobalGivenGlobals) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, "");
  Module main(&scope, findModule(&runtime, "__main__"));
  Dict globals(&scope, main.dict());
  Str globals_name(&scope, runtime.newStrFromCStr("gl"));
  runtime.moduleDictAtPut(globals, globals_name, globals);
  runFromCStr(&runtime, R"(
a = 1337
result = exec("a = 1338", gl)
  )");
  Object result(&scope, moduleAt(&runtime, main, "result"));
  ASSERT_TRUE(result.isNoneType());
  Object a(&scope, moduleAt(&runtime, main, "a"));
  EXPECT_TRUE(isIntEqualsWord(*a, 1338));
}

TEST(BuiltinsModuleTest, BuiltinExecWithEmptyGlobalsFailsToSetGlobal) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
a = 1337
result = exec("a = 1338", {})
  )");
  Module main(&scope, findModule(&runtime, "__main__"));
  Object result(&scope, moduleAt(&runtime, main, "result"));
  ASSERT_TRUE(result.isNoneType());
  Object a(&scope, moduleAt(&runtime, main, "a"));
  EXPECT_TRUE(isIntEqualsWord(*a, 1337));
}

TEST(BuiltinsModuleDeathTest, BuiltinExecWithNonDictGlobalsRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
a = 1337
result = exec("a = 1338", 7)
  )"),
                            LayoutId::kTypeError,
                            "Expected 'globals' to be dict in 'exec'"));
}

TEST(BuiltinsModuleTest, BuiltinExecExWithTupleCallsExec) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
exec(*("a = 1338",))
  )");
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_TRUE(isIntEqualsWord(*a, 1338));
}

TEST(BuiltinsModuleTest, BuiltinExecExWithListCallsExec) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
exec(*["a = 1338"])
  )");
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_TRUE(isIntEqualsWord(*a, 1338));
}

TEST(BuiltinsModuleTest, CopyFunctionEntriesCopies) {
  Runtime runtime;
  HandleScope scope;
  Function::Entry entry = BuiltinsModule::chr;
  Str qualname(&scope, runtime.symbols()->Chr());
  Function func(&scope,
                runtime.newBuiltinFunction(SymbolId::kChr, qualname, entry));
  runFromCStr(&runtime, R"(
def chr(self):
  "docstring"
  pass
)");
  Function python_func(&scope, moduleAt(&runtime, "__main__", "chr"));
  copyFunctionEntries(Thread::currentThread(), func, python_func);
  Code base_code(&scope, func.code());
  Code patch_code(&scope, python_func.code());
  EXPECT_EQ(patch_code.code(), base_code.code());
  EXPECT_EQ(python_func.entry(), &builtinTrampoline);
  EXPECT_EQ(python_func.entryKw(), &builtinTrampolineKw);
  EXPECT_EQ(python_func.entryEx(), &builtinTrampolineEx);
}

TEST(BuiltinsModuleDeathTest, CopyFunctionEntriesRedefinitionDies) {
  Runtime runtime;
  HandleScope scope;
  Function::Entry entry = BuiltinsModule::chr;
  Str qualname(&scope, runtime.symbols()->Chr());
  Function func(&scope,
                runtime.newBuiltinFunction(SymbolId::kChr, qualname, entry));
  runFromCStr(&runtime, R"(
def chr(self):
  return 42
)");
  Function python_func(&scope, moduleAt(&runtime, "__main__", "chr"));
  ASSERT_DEATH(copyFunctionEntries(Thread::currentThread(), func, python_func),
               "Redefinition of native code method 'chr' in managed code");
}

TEST(BuiltinsModuleTest, UnderPatchWithBadPatchFuncRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object not_func(&scope, runtime.newInt(12));
  EXPECT_TRUE(raisedWithStr(runBuiltin(BuiltinsModule::underPatch, not_func),
                            LayoutId::kTypeError,
                            "_patch expects function argument"));
}

TEST(BuiltinsModuleTest, UnderPatchWithBadBaseFuncRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
not_a_function = 1234

@_patch
def not_a_function():
  pass
)"),
                            LayoutId::kTypeError,
                            "_patch can only patch functions"));
}

TEST(BuiltinsModuleTest, AllOnListWithOnlyTrueReturnsTrue) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
result = all([True, True])
  )");
  HandleScope scope;
  Bool result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(result.value());
}

TEST(BuiltinsModuleTest, AllOnListWithFalseReturnsFalse) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
result = all([True, False, True])
  )");
  HandleScope scope;
  Bool result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_FALSE(result.value());
}

TEST(BuiltinsModuleTest, AnyOnListWithOnlyFalseReturnsFalse) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
result = any([False, False])
  )");
  HandleScope scope;
  Bool result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_FALSE(result.value());
}

TEST(BuiltinsModuleTest, AnyOnListWithTrueReturnsTrue) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
result = any([False, True, False])
  )");
  HandleScope scope;
  Bool result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(result.value());
}

TEST(BuiltinsModuleTest, RangeOnNonIntegerRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, R"(range("foo", "bar", "baz"))"),
      LayoutId::kTypeError, "Arguments to range() must be integers"));
}

TEST(BuiltinsModuleTest, RangeWithOnlyStopDefaultsOtherArguments) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
result = range(5)
  )");
  HandleScope scope;
  Range result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_EQ(result.start(), 0);
  EXPECT_EQ(result.stop(), 5);
  EXPECT_EQ(result.step(), 1);
}

TEST(BuiltinsModuleTest, RangeWithStartAndStopDefaultsStep) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
result = range(1, 5)
  )");
  HandleScope scope;
  Range result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_EQ(result.start(), 1);
  EXPECT_EQ(result.stop(), 5);
  EXPECT_EQ(result.step(), 1);
}

TEST(BuiltinsModuleTest, RangeWithAllArgsSetsAllArgs) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
result = range(1, 5, 7)
  )");
  HandleScope scope;
  Range result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_EQ(result.start(), 1);
  EXPECT_EQ(result.stop(), 5);
  EXPECT_EQ(result.step(), 7);
}

TEST(BuiltinsModuleTest, FormatWithNonStrFmtSpecRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(
      raised(runFromCStr(&runtime, "format('hi', 1)"), LayoutId::kTypeError));
}

TEST(BuiltinsModuleTest, FormatCallsDunderFormat) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
class C:
  def __format__(self, fmt_spec):
    return "foobar"
result = format(C(), 'hi')
)");
  EXPECT_TRUE(
      isStrEqualsCStr(moduleAt(&runtime, "__main__", "result"), "foobar"));
}

TEST(BuiltinsModuleTest, FormatRaisesWhenDunderFormatReturnsNonStr) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
class C:
  def __format__(self, fmt_spec):
    return 1
)");
  EXPECT_TRUE(
      raised(runFromCStr(&runtime, "format(C(), 'hi')"), LayoutId::kTypeError));
}

TEST(BuiltinsModuleTest, IterWithIterableCallsDunderIter) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
l = list(iter([1, 2, 3]))
)");
  HandleScope scope;
  Object l(&scope, moduleAt(&runtime, "__main__", "l"));
  EXPECT_PYLIST_EQ(l, {1, 2, 3});
}

TEST(BuiltinsModuleTest, IterWithNonIterableRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
iter(None)
)"),
                            LayoutId::kTypeError,
                            "'NoneType' object is not iterable"));
}

TEST(BuiltinsModuleTest, IterWithRaisingDunderIterPropagatesException) {
  Runtime runtime;
  EXPECT_TRUE(raised(runFromCStr(&runtime, R"(
class C:
  def __iter__(self):
    raise UserWarning()
iter(C())
)"),
                     LayoutId::kUserWarning));
}

TEST(BuiltinsModuleTest, IterWithCallableReturnsIterator) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
class C:
  def __init__(self):
    self.x = 0
  def __call__(self):
    self.x += 1
    return self.x
c = C()
callable_iter = iter(c, 3)
reduced = callable_iter.__reduce__()
l = list(callable_iter)
)");
  HandleScope scope;
  Object l(&scope, moduleAt(&runtime, "__main__", "l"));
  EXPECT_PYLIST_EQ(l, {1, 2});

  Object iter(&scope, moduleAt(&runtime, "builtins", "iter"));
  Object c(&scope, moduleAt(&runtime, "__main__", "c"));
  Object reduced_obj(&scope, moduleAt(&runtime, "__main__", "reduced"));
  ASSERT_TRUE(reduced_obj.isTuple());
  Tuple reduced(&scope, *reduced_obj);
  ASSERT_EQ(reduced.length(), 2);
  EXPECT_EQ(reduced.at(0), iter);
  ASSERT_TRUE(reduced.at(1).isTuple());
  Tuple inner(&scope, reduced.at(1));
  ASSERT_EQ(inner.length(), 2);
  EXPECT_EQ(inner.at(0), c);
  EXPECT_TRUE(isIntEqualsWord(inner.at(1), 3));
}

}  // namespace python
