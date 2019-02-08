#include "gtest/gtest.h"

#include "exception-builtins.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {
using namespace testing;

TEST(ExceptionBuiltinsTest, BaseExceptionNoArguments) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
exc = BaseException()
)");

  Object exc(&scope, moduleAt(&runtime, "__main__", "exc"));
  ASSERT_TRUE(exc.isBaseException());
  BaseException base_exception(&scope, *exc);

  // No constructor arguments means args should contain an empty tuple.
  ASSERT_TRUE(base_exception.args()->isTuple());
  ASSERT_EQ(base_exception.args(), runtime.newTuple(0));
}

TEST(ExceptionBuiltinsTest, BaseExceptionManyArguments) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
exc = BaseException(1,2,3)
)");

  Object exc(&scope, moduleAt(&runtime, "__main__", "exc"));
  ASSERT_TRUE(exc.isBaseException());
  BaseException base_exception(&scope, *exc);

  // The args attribute contains a tuple of the constructor arguments.
  ASSERT_TRUE(base_exception.args()->isTuple());
  Tuple args(&scope, base_exception.args());
  EXPECT_EQ(args.at(0), SmallInt::fromWord(1));
  EXPECT_EQ(args.at(1), SmallInt::fromWord(2));
  EXPECT_EQ(args.at(2), SmallInt::fromWord(3));
}

TEST(ExceptionBuiltinsTest, StrFromBaseExceptionNoArgs) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
a = BaseException().__str__()
)");

  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_TRUE(isStrEqualsCStr(*a, ""));
}

TEST(ExceptionBuiltinsTest, StrFromBaseExceptionOneArg) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
a = BaseException("hello").__str__()
)");

  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_TRUE(isStrEqualsCStr(*a, "hello"));
}

TEST(ExceptionBuiltinsTest, StrFromBaseExceptionManyArgs) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
a = BaseException("hello", "world").__str__()
)");

  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_TRUE(isStrEqualsCStr(*a, "('hello', 'world')"));
}

TEST(ExceptionBuiltinsTest, ExceptionManyArguments) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
exc = Exception(1,2,3)
)");

  Object exc(&scope, moduleAt(&runtime, "__main__", "exc"));
  ASSERT_TRUE(exc.isException());
  Exception exception(&scope, *exc);

  // The args attribute contains a tuple of the constructor arguments.
  ASSERT_TRUE(exception.args()->isTuple());
  Tuple args(&scope, exception.args());
  EXPECT_EQ(args.at(0), SmallInt::fromWord(1));
  EXPECT_EQ(args.at(1), SmallInt::fromWord(2));
  EXPECT_EQ(args.at(2), SmallInt::fromWord(3));
}

TEST(ExceptionBuiltinsTest, SimpleExceptionTypesCanBeConstructed) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
attr_error = AttributeError()
name_error = NameError()
value_error = ValueError()
rt_error = RuntimeError()
)");

  BaseException attr_error(&scope,
                           moduleAt(&runtime, "__main__", "attr_error"));
  BaseException name_error(&scope,
                           moduleAt(&runtime, "__main__", "name_error"));
  BaseException value_error(&scope,
                            moduleAt(&runtime, "__main__", "value_error"));
  BaseException rt_error(&scope, moduleAt(&runtime, "__main__", "rt_error"));

  EXPECT_TRUE(runtime.isInstanceOfBaseException(*attr_error));
  EXPECT_EQ(attr_error.layoutId(), LayoutId::kAttributeError);
  EXPECT_TRUE(runtime.isInstanceOfBaseException(*name_error));
  EXPECT_EQ(name_error.layoutId(), LayoutId::kNameError);
  EXPECT_TRUE(runtime.isInstanceOfBaseException(*value_error));
  EXPECT_EQ(value_error.layoutId(), LayoutId::kValueError);
  EXPECT_TRUE(runtime.isInstanceOfBaseException(*rt_error));
  EXPECT_EQ(rt_error.layoutId(), LayoutId::kRuntimeError);
}

TEST(ExceptionBuiltinsTest, LookupErrorAndSubclassesHaveCorrectHierarchy) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
lookup_is_exc = issubclass(LookupError, Exception)
index_is_lookup = issubclass(IndexError, LookupError)
key_is_lookup = issubclass(KeyError, LookupError)
)");

  Bool lookup_is_exc(&scope, moduleAt(&runtime, "__main__", "lookup_is_exc"));
  Bool index_is_lookup(&scope,
                       moduleAt(&runtime, "__main__", "index_is_lookup"));
  Bool key_is_lookup(&scope, moduleAt(&runtime, "__main__", "key_is_lookup"));

  EXPECT_TRUE(lookup_is_exc.value());
  EXPECT_TRUE(index_is_lookup.value());
  EXPECT_TRUE(key_is_lookup.value());
}

TEST(ExceptionBuiltinsTest, LookupErrorAndSubclassesCanBeConstructed) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
l = LookupError()
i = IndexError()
k = KeyError()
)");

  LookupError l(&scope, moduleAt(&runtime, "__main__", "l"));
  IndexError i(&scope, moduleAt(&runtime, "__main__", "i"));
  KeyError k(&scope, moduleAt(&runtime, "__main__", "k"));

  EXPECT_TRUE(runtime.isInstanceOfBaseException(*l));
  EXPECT_TRUE(runtime.isInstanceOfBaseException(*i));
  EXPECT_TRUE(runtime.isInstanceOfBaseException(*k));
}

TEST(ExceptionBuiltinsTest, KeyErrorStrPrintsMissingKey) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
s = KeyError("key").__str__()
)");

  Object s(&scope, moduleAt(&runtime, "__main__", "s"));
  EXPECT_TRUE(isStrEqualsCStr(*s, "'key'"));
}

TEST(ExceptionBuiltinsTest,
     KeyErrorStrWithMoreThanOneArgPrintsBaseExceptionStr) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
s = KeyError("key", "key2").__str__()
b = BaseException("key", "key2").__str__()
)");

  Str s(&scope, moduleAt(&runtime, "__main__", "s"));
  Str b(&scope, moduleAt(&runtime, "__main__", "b"));
  EXPECT_TRUE(isStrEquals(s, b));
}

TEST(ExceptionBuiltinsTest, TypeErrorReturnsTypeError) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
exc = TypeError()
)");

  Object exc(&scope, moduleAt(&runtime, "__main__", "exc"));
  BaseException exception(&scope, *exc);

  // The args attribute contains a tuple of the constructor arguments.
  ASSERT_TRUE(exception.args()->isTuple());
  Tuple args(&scope, exception.args());
  EXPECT_EQ(args.length(), 0);
}

TEST(ExceptionBuiltinsTest, StopIterationNoArguments) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
exc = StopIteration()
)");

  Object exc(&scope, moduleAt(&runtime, "__main__", "exc"));
  ASSERT_TRUE(exc.isStopIteration());
  StopIteration stop_iteration(&scope, *exc);

  // No constructor arguments so value should be none.
  EXPECT_TRUE(stop_iteration.value()->isNoneType());

  // No constructor arguments means args should contain an empty tuple.
  ASSERT_TRUE(stop_iteration.args()->isTuple());
  Tuple args(&scope, stop_iteration.args());
  EXPECT_EQ(args.length(), 0);
}

TEST(ExceptionBuiltinsTest, StopIterationOneArgument) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
exc = StopIteration(1)
)");

  Object exc(&scope, moduleAt(&runtime, "__main__", "exc"));
  ASSERT_TRUE(exc.isStopIteration());
  StopIteration stop_iteration(&scope, *exc);

  // The value attribute should contain the first constructor argument.
  EXPECT_EQ(stop_iteration.value(), SmallInt::fromWord(1));

  // The args attribute contains a tuple of the constructor arguments.
  ASSERT_TRUE(stop_iteration.args()->isTuple());
  Tuple args(&scope, stop_iteration.args());
  ASSERT_EQ(args.length(), 1);
  EXPECT_EQ(args.at(0), SmallInt::fromWord(1));
}

TEST(ExceptionBuiltinsTest, StopIterationManyArguments) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
exc = StopIteration(4, 5, 6)
)");

  Object exc(&scope, moduleAt(&runtime, "__main__", "exc"));
  ASSERT_TRUE(exc.isStopIteration());
  StopIteration stop_iteration(&scope, *exc);

  // The value attribute should contain the first constructor argument.
  EXPECT_EQ(stop_iteration.value(), SmallInt::fromWord(4));

  // The args attribute contains a tuple of the constructor arguments.
  ASSERT_TRUE(stop_iteration.args()->isTuple());
  Tuple args(&scope, stop_iteration.args());
  ASSERT_EQ(args.length(), 3);
  EXPECT_EQ(args.at(0), SmallInt::fromWord(4));
  EXPECT_EQ(args.at(1), SmallInt::fromWord(5));
  EXPECT_EQ(args.at(2), SmallInt::fromWord(6));
}

TEST(ExceptionBuiltinsTest, NotImplementedErrorNoArguments) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
exc = NotImplementedError()
exc_is_rt_error = issubclass(NotImplementedError, RuntimeError)
)");

  NotImplementedError exc(&scope, moduleAt(&runtime, "__main__", "exc"));
  Bool exc_is_rt_error(&scope,
                       moduleAt(&runtime, "__main__", "exc_is_rt_error"));

  EXPECT_TRUE(runtime.isInstanceOfBaseException(*exc));

  EXPECT_TRUE(exc_is_rt_error.value());
}

TEST(ExceptionBuiltinsTest, SystemExitNoArguments) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
exc = SystemExit()
)");

  Object exc(&scope, moduleAt(&runtime, "__main__", "exc"));
  ASSERT_TRUE(exc.isSystemExit());
  SystemExit system_exit(&scope, *exc);
  ASSERT_TRUE(system_exit.args()->isTuple());

  // No constructor arguments so code should be none.
  EXPECT_TRUE(system_exit.code()->isNoneType());

  // No constructor arguments means args should contain an empty tuple.
  Tuple args(&scope, system_exit.args());
  EXPECT_EQ(args.length(), 0);
}

TEST(ExceptionBuiltinsTest, SystemExitOneArgument) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
exc = SystemExit(1)
)");

  Object exc(&scope, moduleAt(&runtime, "__main__", "exc"));
  ASSERT_TRUE(exc.isSystemExit());
  SystemExit system_exit(&scope, *exc);
  ASSERT_TRUE(system_exit.args()->isTuple());

  // The code attribute should contain the first constructor argument.
  EXPECT_EQ(system_exit.code(), SmallInt::fromWord(1));

  // The args attribute contains a tuple of the constructor arguments.
  Tuple args(&scope, system_exit.args());
  ASSERT_EQ(args.length(), 1);
  EXPECT_EQ(args.at(0), SmallInt::fromWord(1));
}

TEST(ExceptionBuiltinsTest, SystemExitManyArguments) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
exc = SystemExit(4, 5, 6)
)");

  Object exc(&scope, moduleAt(&runtime, "__main__", "exc"));
  ASSERT_TRUE(exc.isSystemExit());
  SystemExit system_exit(&scope, *exc);

  // The code attribute should contain the first constructor argument.
  EXPECT_EQ(system_exit.code(), SmallInt::fromWord(4));

  // The args attribute contains a tuple of the constructor arguments.
  ASSERT_TRUE(system_exit.args()->isTuple());
  Tuple args(&scope, system_exit.args());
  ASSERT_EQ(args.length(), 3);
  EXPECT_EQ(args.at(0), SmallInt::fromWord(4));
  EXPECT_EQ(args.at(1), SmallInt::fromWord(5));
  EXPECT_EQ(args.at(2), SmallInt::fromWord(6));
}

TEST(ExceptionBuiltinsTest, SystemExitGetValue) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
exc = SystemExit(1111)
result = exc.value
)");

  // The value attribute should contain the first constructor argument.
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result.isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*result)->value(), 1111);
}

TEST(ExceptionBuiltinsTest, ImportErrorConstructEmpty) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, "x = ImportError()");
  Object data(&scope, moduleAt(&runtime, "__main__", "x"));
  ASSERT_TRUE(data.isImportError());

  ImportError err(&scope, *data);
  EXPECT_EQ(err.msg(), NoneType::object());
  EXPECT_EQ(err.path(), NoneType::object());
  EXPECT_EQ(err.name(), NoneType::object());

  err.setMsg(SmallInt::fromWord(1111));
  ASSERT_TRUE(err.msg()->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(err.msg())->value(), 1111);

  err.setPath(SmallInt::fromWord(2222));
  ASSERT_TRUE(err.path()->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(err.path())->value(), 2222);

  err.setName(SmallInt::fromWord(3333));
  ASSERT_TRUE(err.name()->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(err.name())->value(), 3333);
}

TEST(ExceptionBuiltinsTest, ImportErrorConstructWithMsg) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, "x = ImportError(1111)");
  Object data(&scope, moduleAt(&runtime, "__main__", "x"));
  ASSERT_TRUE(data.isImportError());

  ImportError err(&scope, *data);
  ASSERT_TRUE(err.msg()->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(err.msg())->value(), 1111);
  EXPECT_EQ(err.path(), NoneType::object());
  EXPECT_EQ(err.name(), NoneType::object());
}

TEST(ExceptionBuiltinsTest, ImportErrorConstructWithMsgNameAndPath) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, "x = ImportError(1111, name=2222, path=3333)");
  Object data(&scope, moduleAt(&runtime, "__main__", "x"));
  ASSERT_TRUE(data.isImportError());

  ImportError err(&scope, *data);
  ASSERT_TRUE(err.msg()->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(err.msg())->value(), 1111);
  ASSERT_TRUE(err.name()->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(err.name())->value(), 2222);
  ASSERT_TRUE(err.path()->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(err.path())->value(), 3333);
}

TEST(ExceptionBuiltinsTest, ImportErrorConstructWithInvalidKwargs) {
  Runtime runtime;
  HandleScope scope;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "x = ImportError(foo=123)"),
                            LayoutId::kTypeError,
                            "invalid keyword arguments supplied"));
}

TEST(ExceptionBuiltinsTest, ModuleNotFoundErrorManyArguments) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
exc = ModuleNotFoundError(1111, name=2222, path=3333)
)");

  Object data(&scope, moduleAt(&runtime, "__main__", "exc"));
  ASSERT_TRUE(data.isModuleNotFoundError());

  ModuleNotFoundError err(&scope, *data);
  ASSERT_TRUE(err.msg()->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(err.msg())->value(), 1111);
  ASSERT_TRUE(err.name()->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(err.name())->value(), 2222);
  ASSERT_TRUE(err.path()->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(err.path())->value(), 3333);
}

TEST(ExceptionBuiltinsTest, DunderReprWithNoArgsHasEmptyParens) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
result = NameError().__repr__()
)");

  EXPECT_TRUE(
      isStrEqualsCStr(moduleAt(&runtime, "__main__", "result"), "NameError()"));
}

TEST(ExceptionBuiltinsTest, DunderReprCallsTupleRepr) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
n = NameError().__class__.__name__
result = NameError(1, 2).__repr__()
)");

  EXPECT_TRUE(
      isStrEqualsCStr(moduleAt(&runtime, "__main__", "n"), "NameError"));
  EXPECT_TRUE(isStrEqualsCStr(moduleAt(&runtime, "__main__", "result"),
                              "NameError(1, 2)"));
}

}  // namespace python
