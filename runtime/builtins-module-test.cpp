#include "gtest/gtest.h"

#include "builtins-module.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(BuiltinsModuleDeathTest, BuiltinCallableOnTypeReturnsTrue) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
class Foo:
  pass

a = callable(Foo)
  )");
  HandleScope scope;
  Module main(&scope, findModule(&runtime, "__main__"));
  Bool a(&scope, moduleAt(&runtime, main, "a"));
  EXPECT_TRUE(a->value());
}

TEST(BuiltinsModuleDeathTest, BuiltinCallableOnMethodReturnsTrue) {
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
  EXPECT_TRUE(a->value());
  EXPECT_TRUE(b->value());
}

TEST(BuiltinsModuleDeathTest, BuiltinCallableOnNonCallableReturnsFalse) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
a = callable(1)
b = callable("hello")
  )");
  HandleScope scope;
  Module main(&scope, findModule(&runtime, "__main__"));
  Bool a(&scope, moduleAt(&runtime, main, "a"));
  Bool b(&scope, moduleAt(&runtime, main, "b"));
  EXPECT_FALSE(a->value());
  EXPECT_FALSE(b->value());
}

TEST(BuiltinsModuleDeathTest,
     BuiltinCallableOnObjectWithCallOnTypeReturnsTrue) {
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
  EXPECT_TRUE(a->value());
}

TEST(BuiltinsModuleDeathTest,
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
  EXPECT_FALSE(a->value());
}

TEST(BuiltinsModuleDeathTest, BuiltinChr) {
  Runtime runtime;
  std::string result = compileAndRunToString(&runtime, "print(chr(65))");
  EXPECT_EQ(result, "A\n");
  ASSERT_DEATH(
      runFromCStr(&runtime, "print(chr(1,2))"),
      "aborting due to pending exception: Unexpected 1 argumment in 'chr'");
  ASSERT_DEATH(
      runFromCStr(&runtime, "print(chr('A'))"),
      "aborting due to pending exception: Unsupported type in builtin 'chr'");
}

TEST(BuiltinsModuleDeathTest, BuiltinIsinstance) {
  Runtime runtime;
  HandleScope scope;

  // Only accepts 2 arguments
  EXPECT_DEATH(
      runFromCStr(&runtime, "print(isinstance(1, 1, 1))"),
      "aborting due to pending exception: isinstance expected 2 arguments");

  // 2nd argument must be a type
  EXPECT_DEATH(
      runFromCStr(&runtime, "print(isinstance(1, 1))"),
      "aborting due to pending exception: isinstance arg 2 must be a type");

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
  ASSERT_TRUE(object->isModule());
  Module main(&scope, object);

  // Create an instance of D
  Object type_d(&scope, moduleAt(&runtime, main, "D"));
  ASSERT_TRUE(type_d->isType());
  Layout layout(&scope, RawType::cast(*type_d)->instanceLayout());
  Object instance(&scope, runtime.newInstance(layout));

  // Fetch the test function
  object = moduleAt(&runtime, main, "test");
  ASSERT_TRUE(object->isFunction());
  Function isinstance(&scope, object);

  // isinstance(1, D) should be false
  Tuple args(&scope, runtime.newTuple(2));
  args->atPut(0, SmallInt::fromWord(100));
  args->atPut(1, *type_d);
  EXPECT_EQ(callFunctionToString(isinstance, args), "False\n");

  // isinstance(D, D) should be false
  args->atPut(0, *type_d);
  args->atPut(1, *type_d);
  EXPECT_EQ(callFunctionToString(isinstance, args), "False\n");

  // isinstance(D(), D) should be true
  args->atPut(0, *instance);
  args->atPut(1, *type_d);
  EXPECT_EQ(callFunctionToString(isinstance, args), "True\n");

  // isinstance(D(), C) should be true
  Object type_c(&scope, moduleAt(&runtime, main, "C"));
  ASSERT_TRUE(type_c->isType());
  args->atPut(1, *type_c);
  EXPECT_EQ(callFunctionToString(isinstance, args), "True\n");

  // isinstance(D(), B) should be true
  Object type_b(&scope, moduleAt(&runtime, main, "B"));
  ASSERT_TRUE(type_b->isType());
  args->atPut(1, *type_b);
  EXPECT_EQ(callFunctionToString(isinstance, args), "True\n");

  // isinstance(C(), A) should be true
  Object type_a(&scope, moduleAt(&runtime, main, "A"));
  ASSERT_TRUE(type_a->isType());
  args->atPut(1, *type_a);
  EXPECT_EQ(callFunctionToString(isinstance, args), "True\n");
}

TEST(BuiltinsModuleDeathTest, BuiltinIssubclassWithSubclassReturnsTrue) {
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
  EXPECT_TRUE(a->value());
  EXPECT_TRUE(b->value());
  EXPECT_TRUE(c->value());
}

TEST(BuiltinsModuleDeathTest, BuiltinIssubclassWithNonSubclassReturnsFalse) {
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
  EXPECT_FALSE(a->value());
  EXPECT_FALSE(b->value());
  EXPECT_FALSE(c->value());
}

TEST(BuiltinsModuleDeathTest, BuiltinIssubclassWithOneSuperclassReturnsTrue) {
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
  EXPECT_TRUE(a->value());
  EXPECT_TRUE(b->value());
}

TEST(BuiltinsModuleDeathTest, BuiltinIssubclassWithNoSuperclassReturnsFalse) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
class Foo:
  pass

a = issubclass(Foo, (str, int))
)");
  HandleScope scope;
  Module main(&scope, findModule(&runtime, "__main__"));
  Bool a(&scope, moduleAt(&runtime, main, "a"));
  EXPECT_FALSE(a->value());
}

TEST(BuiltinsModuleDeathTest, BuiltinLen) {
  Runtime runtime;
  std::string result = compileAndRunToString(&runtime, "print(len([1,2,3]))");
  EXPECT_EQ(result, "3\n");
  ASSERT_DEATH(runFromCStr(&runtime, "print(len(1,2))"),
               "aborting due to pending exception: "
               "len\\(\\) takes exactly one argument");
  ASSERT_DEATH(runFromCStr(&runtime, "print(len(1))"),
               "aborting due to pending exception: "
               "object has no len()");
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

TEST(BuiltinsModuleDeathTest, BuiltinOrd) {
  Runtime runtime;
  std::string result = compileAndRunToString(&runtime, "print(ord('A'))");
  EXPECT_EQ(result, "65\n");
  ASSERT_DEATH(
      runFromCStr(&runtime, "print(ord(1,2))"),
      "aborting due to pending exception: Unexpected 1 argumment in 'ord'");
  ASSERT_DEATH(
      runFromCStr(&runtime, "print(ord(1))"),
      "aborting due to pending exception: Unsupported type in builtin 'ord'");
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

  EXPECT_TRUE(RawStr::cast(*a)->equalsCStr("foo"));
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

TEST(BuiltinsModuleDeathTest, GetAttrWithNonStringAttrThrows) {
  const char* src = R"(
class Foo: pass
getattr(Foo(), 1)
)";
  Runtime runtime;
  EXPECT_DEATH(runFromCStr(&runtime, src), "attribute name must be string");
}

TEST(BuiltinsModuleDeathTest, GetAttrWithNonStringAttrAndDefaultThrows) {
  const char* src = R"(
class Foo: pass
getattr(Foo(), 1, 2)
)";
  Runtime runtime;
  EXPECT_DEATH(runFromCStr(&runtime, src), "attribute name must be string");
}

TEST(BuiltinsModuleDeathTest, GetAttrFromClassMissingAttrWithoutDefaultThrows) {
  const char* src = R"(
class Foo:
  bar = 1
getattr(Foo, 'foo')
)";
  Runtime runtime;
  EXPECT_DEATH(runFromCStr(&runtime, src), "missing attribute");
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

TEST(BuiltinsModuleDeathTest, HasAttrWithNonStringAttrThrows) {
  const char* src = R"(
class Foo:
  bar = 1
hasattr(Foo, 1)
)";
  Runtime runtime;
  EXPECT_DEATH(runFromCStr(&runtime, src), "attribute name must be string");
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

TEST(BuiltinsModuleDeathTest, BuiltInSetAttrThrow) {
  const char* src = R"(
class Foo:
  bar = 1
a = setattr(Foo, 2, 'foo')
)";
  Runtime runtime;
  HandleScope scope;
  EXPECT_DEATH(runFromCStr(&runtime, src), "attribute name must be string");
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
  ASSERT_TRUE(b->isStr());
  EXPECT_TRUE(Str::cast(b)->equalsCStr("builtins"));

  Object c(&scope, moduleAt(&runtime, "__main__", "c"));
  EXPECT_EQ(*a, Bool::trueObj());
  Object d(&scope, moduleAt(&runtime, "__main__", "d"));
  ASSERT_TRUE(b->isStr());
  EXPECT_TRUE(Str::cast(b)->equalsCStr("builtins"));
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
  ASSERT_TRUE(b->isStr());
  EXPECT_TRUE(Str::cast(b)->equalsCStr("object"));

  Object c(&scope, moduleAt(&runtime, "__main__", "c"));
  EXPECT_EQ(*c, Bool::trueObj());
  Object d(&scope, moduleAt(&runtime, "__main__", "d"));
  ASSERT_TRUE(d->isStr());
  EXPECT_TRUE(Str::cast(d)->equalsCStr("list"));
}

TEST(BuiltinsModuleTest, BuiltinCompile) {
  Runtime runtime;
  HandleScope scope;
  const char* program = R"(
a = 1
b = 2
  )";
  Str code_str(&scope, runtime.newStrFromCStr(program));
  Str filename(&scope, runtime.newStrFromCStr("<string>"));
  Str mode(&scope, runtime.newStrFromCStr("eval"));
  Code code(&scope, runBuiltin(Builtins::compile, code_str, filename, mode));
  ASSERT_TRUE(code->filename().isStr());
  EXPECT_TRUE(Str::cast(code->filename()).equals(filename));

  ASSERT_TRUE(code->names().isTuple());
  Tuple names(&scope, code->names());
  ASSERT_EQ(names->length(), 2);
  ASSERT_TRUE(names->contains(runtime.newStrFromCStr("a")));
  ASSERT_TRUE(names->contains(runtime.newStrFromCStr("b")));
}

TEST(BuiltinsModuleTest, BuiltinCompileThrowsTypeErrorGivenTooFewArgs) {
  Runtime runtime;
  HandleScope scope;
  SmallInt one(&scope, RawSmallInt::fromWord(1));
  Object result(&scope, runBuiltin(Builtins::compile, one));
  ASSERT_TRUE(result->isError());
  Thread* thread = Thread::currentThread();
  EXPECT_EQ(thread->pendingExceptionType(),
            runtime.typeAt(LayoutId::kTypeError));
  EXPECT_TRUE(thread->pendingExceptionValue()->isStr());
}

TEST(BuiltinsModuleTest, BuiltinCompileThrowsTypeErrorGivenTooManyArgs) {
  Runtime runtime;
  HandleScope scope;
  SmallInt one(&scope, RawSmallInt::fromWord(1));
  Object result(
      &scope, runBuiltin(Builtins::compile, one, one, one, one, one, one, one));
  ASSERT_TRUE(result->isError());
  Thread* thread = Thread::currentThread();
  EXPECT_EQ(thread->pendingExceptionType(),
            runtime.typeAt(LayoutId::kTypeError));
  EXPECT_TRUE(thread->pendingExceptionValue()->isStr());
}

TEST(BuiltinsModuleTest, BuiltinCompileThrowsTypeErrorGivenBadMode) {
  Runtime runtime;
  HandleScope scope;
  Str hello(&scope, RawSmallStr::fromCStr("hello"));
  Object result(&scope, runBuiltin(Builtins::compile, hello, hello, hello));
  ASSERT_TRUE(result->isError());
  Thread* thread = Thread::currentThread();
  EXPECT_EQ(thread->pendingExceptionType(),
            runtime.typeAt(LayoutId::kValueError));
  EXPECT_TRUE(thread->pendingExceptionValue()->isStr());
}

TEST(BuiltinsModuleTest, BuiltinExecSetsGlobal) {
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr(R"(
a = 1337
exec("a = 1338")
  )");
  // We can't use runBuiltin here because it does not set up the frame properly
  // for functions that need globals, implicitGlobals, etc
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  ASSERT_TRUE(a->isSmallInt());
  EXPECT_EQ(SmallInt::cast(a)->value(), 1338);
}

TEST(BuiltinsModuleTest, BuiltinExecSetsGlobalGivenGlobals) {
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr(R"(
a = 1337
  )");
  Module main(&scope, findModule(&runtime, "__main__"));
  Str code(&scope, runtime.newStrFromCStr("a = 1338"));
  Dict globals(&scope, main->dict());
  Object result(&scope, runBuiltin(Builtins::exec, code, globals));
  ASSERT_TRUE(result->isNoneType());
  Object a(&scope, moduleAt(&runtime, main, "a"));
  ASSERT_TRUE(a->isSmallInt());
  EXPECT_EQ(SmallInt::cast(a)->value(), 1338);
}

TEST(BuiltinsModuleTest, BuiltinExecWithEmptyGlobalsFailsToSetGlobal) {
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr(R"(
a = 1337
  )");
  Str code(&scope, runtime.newStrFromCStr("a = 1338"));
  Dict globals(&scope, runtime.newDict());
  Object result(&scope, runBuiltin(Builtins::exec, code, globals));
  ASSERT_TRUE(result->isNoneType());
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  ASSERT_TRUE(a->isSmallInt());
  EXPECT_EQ(SmallInt::cast(a)->value(), 1337);
}

TEST(BuiltinsModuleTest, BuiltinExecWithNonDictGlobalsRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Str code(&scope, runtime.newStrFromCStr("a = 1338"));
  Object globals_not_a_dict(&scope, SmallInt::fromWord(5));
  Object result(&scope, runBuiltin(Builtins::exec, code, globals_not_a_dict));
  ASSERT_TRUE(result->isError());
  Thread* thread = Thread::currentThread();
  EXPECT_EQ(thread->pendingExceptionType(),
            runtime.typeAt(LayoutId::kTypeError));
  EXPECT_TRUE(thread->pendingExceptionValue()->isStr());
}

TEST(BuiltinsModuleTest, PythonBuiltinAnnotationSetsFunctionSignature) {
  Runtime runtime;
  HandleScope scope;
  // Since runtime.createMainModule is private, we have to create and add
  // __main__ ourselves
  Str main_sym(&scope, runtime.symbols()->DunderMain());
  Module main(&scope, runtime.newModule(main_sym));
  runtime.addModule(main);

  // A little dance to bring the builtins.str into the globals so that
  // buildClassKw can find it when bootstrapping.
  Module builtins(&scope, findModule(&runtime, "builtins"));
  Str str_sym(&scope, runtime.symbols()->Str());
  Type str_type(&scope, runtime.moduleAt(builtins, str_sym));
  runtime.moduleAtPut(main, str_sym, str_type);

  // We can't run buildClassKw on str because there's already a bunch of junk
  // defined on str that we don't want to try and re-patch. So we'll take a
  // method from a new class and patch str with that. Gross.
  runtime.runFromCStr(R"(
class bumble():
  def __lt__(self, other: str) -> bool:
    """O, Canada"""
    pass
)");

  Type bumble_type(&scope, moduleAt(&runtime, main, "bumble"));
  Str dunder_lt_sym(&scope, runtime.symbols()->DunderLt());
  Dict str_dict(&scope, str_type->dict());

  // Clear the annotations so that we can meaningfully test the patch
  Function str_lt(&scope, runtime.typeDictAt(str_dict, dunder_lt_sym));
  str_lt->setAnnotations(RawNoneType::object());
  ASSERT_TRUE(RawFunction::cast(runtime.typeDictAt(str_dict, dunder_lt_sym))
                  .annotations()
                  .isNoneType());

  Dict bumble_dict(&scope, bumble_type->dict());
  Function bumble_lt(&scope, runtime.typeDictAt(bumble_dict, dunder_lt_sym));
  patchFunctionAttrs(Thread::currentThread(), str_dict, bumble_lt);

  Dict annotations(&scope, str_lt->annotations());
  EXPECT_EQ(annotations->numItems(), 2);
  Str other_name(&scope, runtime.newStrFromCStr("other"));
  Object other_ann(&scope, runtime.dictAt(annotations, other_name));
  ASSERT_TRUE(other_ann->isType());
  EXPECT_EQ(RawType::cast(*other_ann), *str_type);
  Str return_name(&scope, runtime.newStrFromCStr("return"));
  Object return_ann(&scope, runtime.dictAt(annotations, return_name));
  ASSERT_TRUE(return_ann->isType());
  EXPECT_EQ(RawType::cast(*return_ann), moduleAt(&runtime, builtins, "bool"));
}

TEST(BuiltinsModuleTest, PythonBuiltinAnnotationSetsFunctionDocString) {
  Runtime runtime;
  HandleScope scope;
  // Since runtime.createMainModule is private, we have to create and add
  // __main__ ourselves
  Str main_sym(&scope, runtime.symbols()->DunderMain());
  Module main(&scope, runtime.newModule(main_sym));
  runtime.addModule(main);

  // A little dance to bring the builtins.str into the globals so that
  // buildClassKw can find it when bootstrapping.
  Module builtins(&scope, findModule(&runtime, "builtins"));
  Str str_sym(&scope, runtime.symbols()->Str());
  Type str_type(&scope, runtime.moduleAt(builtins, str_sym));
  runtime.moduleAtPut(main, str_sym, str_type);

  // We can't run buildClassKw on str because there's already a bunch of junk
  // defined on str that we don't want to try and re-patch. So we'll take a
  // method from a new class and patch str with that. Gross.
  runtime.runFromCStr(R"(
class bumble():
  def __lt__(self, other):
    """O, Canada"""
    pass
)");

  Type bumble_type(&scope, moduleAt(&runtime, main, "bumble"));
  Str dunder_lt_sym(&scope, runtime.symbols()->DunderLt());
  Dict bumble_dict(&scope, bumble_type->dict());
  Function bumble_lt(&scope, runtime.typeDictAt(bumble_dict, dunder_lt_sym));
  Dict str_dict(&scope, str_type->dict());
  patchFunctionAttrs(Thread::currentThread(), str_dict, bumble_lt);

  Function dunder_lt(&scope, runtime.typeDictAt(str_dict, dunder_lt_sym));
  ASSERT_TRUE(dunder_lt->isFunction());
  Object doc_object(&scope, dunder_lt->doc());
  ASSERT_TRUE(doc_object->isStr());
  Str doc(&scope, *doc_object);
  EXPECT_PYSTRING_EQ(doc, "O, Canada");
}

TEST(BuiltinsModuleDeathTest,
     PythonBuiltinAnnotationCannotOverrideNativeDefinition) {
  Runtime runtime;
  HandleScope scope;
  // Since runtime.createMainModule is private, we have to create and add
  // __main__ ourselves
  Str main_sym(&scope, runtime.symbols()->DunderMain());
  Module main(&scope, runtime.newModule(main_sym));
  runtime.addModule(main);

  // A little dance to bring the builtins.str into the globals so that
  // buildClassKw can find it when bootstrapping.
  Module builtins(&scope, findModule(&runtime, "builtins"));
  Str str_sym(&scope, runtime.symbols()->Str());
  Type str_type(&scope, runtime.moduleAt(builtins, str_sym));
  runtime.moduleAtPut(main, str_sym, str_type);

  // We can't run buildClassKw on str because there's already a bunch of junk
  // defined on str that we don't want to try and re-patch. So we'll take a
  // method from a new class and patch str with that. Gross.
  runtime.runFromCStr(R"(
class bumble():
  def __lt__(self, other):
    return "moose"
)");

  Type bumble_type(&scope, moduleAt(&runtime, main, "bumble"));
  Str dunder_lt_sym(&scope, runtime.symbols()->DunderLt());
  Dict bumble_dict(&scope, bumble_type->dict());
  Function bumble_lt(&scope, runtime.typeDictAt(bumble_dict, dunder_lt_sym));
  Dict str_dict(&scope, str_type->dict());
  ASSERT_DEATH(patchFunctionAttrs(Thread::currentThread(), str_dict, bumble_lt),
               "Redefinition of native code method __lt__ in managed code");
}

}  // namespace python
