#include "gtest/gtest.h"

#include "exception-builtins.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {
using namespace testing;

using ExceptionBuiltinsTest = RuntimeFixture;

TEST_F(ExceptionBuiltinsTest, BaseExceptionNoArguments) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
exc = BaseException()
)")
                   .isError());

  Object exc(&scope, moduleAt(&runtime_, "__main__", "exc"));
  ASSERT_TRUE(exc.isBaseException());
  BaseException base_exception(&scope, *exc);

  // No constructor arguments means args should contain an empty tuple.
  ASSERT_TRUE(base_exception.args().isTuple());
  ASSERT_EQ(base_exception.args(), runtime_.emptyTuple());
}

TEST_F(ExceptionBuiltinsTest, BaseExceptionManyArguments) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
exc = BaseException(1,2,3)
)")
                   .isError());

  Object exc(&scope, moduleAt(&runtime_, "__main__", "exc"));
  ASSERT_TRUE(exc.isBaseException());
  BaseException base_exception(&scope, *exc);

  // The args attribute contains a tuple of the constructor arguments.
  ASSERT_TRUE(base_exception.args().isTuple());
  Tuple args(&scope, base_exception.args());
  EXPECT_EQ(args.at(0), SmallInt::fromWord(1));
  EXPECT_EQ(args.at(1), SmallInt::fromWord(2));
  EXPECT_EQ(args.at(2), SmallInt::fromWord(3));
}

TEST_F(ExceptionBuiltinsTest, StrFromBaseExceptionNoArgs) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = BaseException().__str__()
)")
                   .isError());

  Object a(&scope, moduleAt(&runtime_, "__main__", "a"));
  EXPECT_TRUE(isStrEqualsCStr(*a, ""));
}

TEST_F(ExceptionBuiltinsTest, StrFromBaseExceptionOneArg) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = BaseException("hello").__str__()
)")
                   .isError());

  Object a(&scope, moduleAt(&runtime_, "__main__", "a"));
  EXPECT_TRUE(isStrEqualsCStr(*a, "hello"));
}

TEST_F(ExceptionBuiltinsTest, StrFromBaseExceptionManyArgs) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = BaseException("hello", "world").__str__()
)")
                   .isError());

  Object a(&scope, moduleAt(&runtime_, "__main__", "a"));
  EXPECT_TRUE(isStrEqualsCStr(*a, "('hello', 'world')"));
}

TEST_F(ExceptionBuiltinsTest, ExceptionManyArguments) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
exc = Exception(1,2,3)
)")
                   .isError());

  Object exc(&scope, moduleAt(&runtime_, "__main__", "exc"));
  ASSERT_TRUE(exc.isException());
  Exception exception(&scope, *exc);

  // The args attribute contains a tuple of the constructor arguments.
  ASSERT_TRUE(exception.args().isTuple());
  Tuple args(&scope, exception.args());
  EXPECT_EQ(args.at(0), SmallInt::fromWord(1));
  EXPECT_EQ(args.at(1), SmallInt::fromWord(2));
  EXPECT_EQ(args.at(2), SmallInt::fromWord(3));
}

TEST_F(ExceptionBuiltinsTest, SimpleExceptionTypesCanBeConstructed) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
attr_error = AttributeError()
name_error = NameError()
value_error = ValueError()
rt_error = RuntimeError()
)")
                   .isError());

  BaseException attr_error(&scope,
                           moduleAt(&runtime_, "__main__", "attr_error"));
  BaseException name_error(&scope,
                           moduleAt(&runtime_, "__main__", "name_error"));
  BaseException value_error(&scope,
                            moduleAt(&runtime_, "__main__", "value_error"));
  BaseException rt_error(&scope, moduleAt(&runtime_, "__main__", "rt_error"));

  EXPECT_TRUE(runtime_.isInstanceOfBaseException(*attr_error));
  EXPECT_EQ(attr_error.layoutId(), LayoutId::kAttributeError);
  EXPECT_TRUE(runtime_.isInstanceOfBaseException(*name_error));
  EXPECT_EQ(name_error.layoutId(), LayoutId::kNameError);
  EXPECT_TRUE(runtime_.isInstanceOfBaseException(*value_error));
  EXPECT_EQ(value_error.layoutId(), LayoutId::kValueError);
  EXPECT_TRUE(runtime_.isInstanceOfBaseException(*rt_error));
  EXPECT_EQ(rt_error.layoutId(), LayoutId::kRuntimeError);
}

TEST_F(ExceptionBuiltinsTest, LookupErrorAndSubclassesHaveCorrectHierarchy) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
lookup_is_exc = issubclass(LookupError, Exception)
index_is_lookup = issubclass(IndexError, LookupError)
key_is_lookup = issubclass(KeyError, LookupError)
)")
                   .isError());

  Bool lookup_is_exc(&scope, moduleAt(&runtime_, "__main__", "lookup_is_exc"));
  Bool index_is_lookup(&scope,
                       moduleAt(&runtime_, "__main__", "index_is_lookup"));
  Bool key_is_lookup(&scope, moduleAt(&runtime_, "__main__", "key_is_lookup"));

  EXPECT_TRUE(lookup_is_exc.value());
  EXPECT_TRUE(index_is_lookup.value());
  EXPECT_TRUE(key_is_lookup.value());
}

TEST_F(ExceptionBuiltinsTest, LookupErrorAndSubclassesCanBeConstructed) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
l = LookupError()
i = IndexError()
k = KeyError()
)")
                   .isError());

  LookupError l(&scope, moduleAt(&runtime_, "__main__", "l"));
  IndexError i(&scope, moduleAt(&runtime_, "__main__", "i"));
  KeyError k(&scope, moduleAt(&runtime_, "__main__", "k"));

  EXPECT_TRUE(runtime_.isInstanceOfBaseException(*l));
  EXPECT_TRUE(runtime_.isInstanceOfBaseException(*i));
  EXPECT_TRUE(runtime_.isInstanceOfBaseException(*k));
}

TEST_F(ExceptionBuiltinsTest, KeyErrorStrPrintsMissingKey) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
s = KeyError("key").__str__()
)")
                   .isError());

  Object s(&scope, moduleAt(&runtime_, "__main__", "s"));
  EXPECT_TRUE(isStrEqualsCStr(*s, "'key'"));
}

TEST_F(ExceptionBuiltinsTest,
       KeyErrorStrWithMoreThanOneArgPrintsBaseExceptionStr) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
s = KeyError("key", "key2").__str__()
b = BaseException("key", "key2").__str__()
)")
                   .isError());

  Str s(&scope, moduleAt(&runtime_, "__main__", "s"));
  Str b(&scope, moduleAt(&runtime_, "__main__", "b"));
  EXPECT_TRUE(isStrEquals(s, b));
}

TEST_F(ExceptionBuiltinsTest, TypeErrorReturnsTypeError) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
exc = TypeError()
)")
                   .isError());

  Object exc(&scope, moduleAt(&runtime_, "__main__", "exc"));
  BaseException exception(&scope, *exc);

  // The args attribute contains a tuple of the constructor arguments.
  ASSERT_TRUE(exception.args().isTuple());
  Tuple args(&scope, exception.args());
  EXPECT_EQ(args.length(), 0);
}

TEST_F(ExceptionBuiltinsTest, StopIterationNoArguments) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
exc = StopIteration()
)")
                   .isError());

  Object exc(&scope, moduleAt(&runtime_, "__main__", "exc"));
  ASSERT_TRUE(exc.isStopIteration());
  StopIteration stop_iteration(&scope, *exc);

  // No constructor arguments so value should be none.
  EXPECT_TRUE(stop_iteration.value().isNoneType());

  // No constructor arguments means args should contain an empty tuple.
  ASSERT_TRUE(stop_iteration.args().isTuple());
  Tuple args(&scope, stop_iteration.args());
  EXPECT_EQ(args.length(), 0);
}

TEST_F(ExceptionBuiltinsTest, StopIterationOneArgument) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
exc = StopIteration(1)
)")
                   .isError());

  Object exc(&scope, moduleAt(&runtime_, "__main__", "exc"));
  ASSERT_TRUE(exc.isStopIteration());
  StopIteration stop_iteration(&scope, *exc);

  // The value attribute should contain the first constructor argument.
  EXPECT_EQ(stop_iteration.value(), SmallInt::fromWord(1));

  // The args attribute contains a tuple of the constructor arguments.
  ASSERT_TRUE(stop_iteration.args().isTuple());
  Tuple args(&scope, stop_iteration.args());
  ASSERT_EQ(args.length(), 1);
  EXPECT_EQ(args.at(0), SmallInt::fromWord(1));
}

TEST_F(ExceptionBuiltinsTest, StopIterationManyArguments) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
exc = StopIteration(4, 5, 6)
)")
                   .isError());

  Object exc(&scope, moduleAt(&runtime_, "__main__", "exc"));
  ASSERT_TRUE(exc.isStopIteration());
  StopIteration stop_iteration(&scope, *exc);

  // The value attribute should contain the first constructor argument.
  EXPECT_EQ(stop_iteration.value(), SmallInt::fromWord(4));

  // The args attribute contains a tuple of the constructor arguments.
  ASSERT_TRUE(stop_iteration.args().isTuple());
  Tuple args(&scope, stop_iteration.args());
  ASSERT_EQ(args.length(), 3);
  EXPECT_EQ(args.at(0), SmallInt::fromWord(4));
  EXPECT_EQ(args.at(1), SmallInt::fromWord(5));
  EXPECT_EQ(args.at(2), SmallInt::fromWord(6));
}

TEST_F(ExceptionBuiltinsTest, NotImplementedErrorNoArguments) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
exc = NotImplementedError()
exc_is_rt_error = issubclass(NotImplementedError, RuntimeError)
)")
                   .isError());

  NotImplementedError exc(&scope, moduleAt(&runtime_, "__main__", "exc"));
  Bool exc_is_rt_error(&scope,
                       moduleAt(&runtime_, "__main__", "exc_is_rt_error"));

  EXPECT_TRUE(runtime_.isInstanceOfBaseException(*exc));

  EXPECT_TRUE(exc_is_rt_error.value());
}

TEST_F(ExceptionBuiltinsTest, SystemExitNoArguments) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
exc = SystemExit()
)")
                   .isError());

  Object exc(&scope, moduleAt(&runtime_, "__main__", "exc"));
  ASSERT_TRUE(exc.isSystemExit());
  SystemExit system_exit(&scope, *exc);
  ASSERT_TRUE(system_exit.args().isTuple());

  // No constructor arguments so code should be none.
  EXPECT_TRUE(system_exit.code().isNoneType());

  // No constructor arguments means args should contain an empty tuple.
  Tuple args(&scope, system_exit.args());
  EXPECT_EQ(args.length(), 0);
}

TEST_F(ExceptionBuiltinsTest, SystemExitOneArgument) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
exc = SystemExit(1)
)")
                   .isError());

  Object exc(&scope, moduleAt(&runtime_, "__main__", "exc"));
  ASSERT_TRUE(exc.isSystemExit());
  SystemExit system_exit(&scope, *exc);
  ASSERT_TRUE(system_exit.args().isTuple());

  // The code attribute should contain the first constructor argument.
  EXPECT_EQ(system_exit.code(), SmallInt::fromWord(1));

  // The args attribute contains a tuple of the constructor arguments.
  Tuple args(&scope, system_exit.args());
  ASSERT_EQ(args.length(), 1);
  EXPECT_EQ(args.at(0), SmallInt::fromWord(1));
}

TEST_F(ExceptionBuiltinsTest, SystemExitManyArguments) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
exc = SystemExit(4, 5, 6)
)")
                   .isError());

  Object exc(&scope, moduleAt(&runtime_, "__main__", "exc"));
  ASSERT_TRUE(exc.isSystemExit());
  SystemExit system_exit(&scope, *exc);

  // The code attribute should contain the first constructor argument.
  EXPECT_EQ(system_exit.code(), SmallInt::fromWord(4));

  // The args attribute contains a tuple of the constructor arguments.
  ASSERT_TRUE(system_exit.args().isTuple());
  Tuple args(&scope, system_exit.args());
  ASSERT_EQ(args.length(), 3);
  EXPECT_EQ(args.at(0), SmallInt::fromWord(4));
  EXPECT_EQ(args.at(1), SmallInt::fromWord(5));
  EXPECT_EQ(args.at(2), SmallInt::fromWord(6));
}

TEST_F(ExceptionBuiltinsTest, SystemExitGetValue) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
exc = SystemExit(1111)
result = exc.value
)")
                   .isError());

  // The value attribute should contain the first constructor argument.
  Object result(&scope, moduleAt(&runtime_, "__main__", "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 1111));
}

TEST_F(ExceptionBuiltinsTest, ImportErrorConstructEmpty) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, "x = ImportError()").isError());
  Object data(&scope, moduleAt(&runtime_, "__main__", "x"));
  ASSERT_TRUE(data.isImportError());

  ImportError err(&scope, *data);
  EXPECT_EQ(err.msg(), NoneType::object());
  EXPECT_EQ(err.path(), NoneType::object());
  EXPECT_EQ(err.name(), NoneType::object());

  err.setMsg(SmallInt::fromWord(1111));
  EXPECT_TRUE(isIntEqualsWord(err.msg(), 1111));

  err.setPath(SmallInt::fromWord(2222));
  EXPECT_TRUE(isIntEqualsWord(err.path(), 2222));

  err.setName(SmallInt::fromWord(3333));
  EXPECT_TRUE(isIntEqualsWord(err.name(), 3333));
}

TEST_F(ExceptionBuiltinsTest, ImportErrorConstructWithMsg) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, "x = ImportError(1111)").isError());
  Object data(&scope, moduleAt(&runtime_, "__main__", "x"));
  ASSERT_TRUE(data.isImportError());

  ImportError err(&scope, *data);
  EXPECT_TRUE(isIntEqualsWord(err.msg(), 1111));
  EXPECT_EQ(err.path(), NoneType::object());
  EXPECT_EQ(err.name(), NoneType::object());
}

TEST_F(ExceptionBuiltinsTest, ImportErrorConstructWithMsgNameAndPath) {
  HandleScope scope(thread_);
  ASSERT_FALSE(
      runFromCStr(&runtime_, "x = ImportError(1111, name=2222, path=3333)")
          .isError());
  Object data(&scope, moduleAt(&runtime_, "__main__", "x"));
  ASSERT_TRUE(data.isImportError());

  ImportError err(&scope, *data);
  EXPECT_TRUE(isIntEqualsWord(err.msg(), 1111));
  EXPECT_TRUE(isIntEqualsWord(err.name(), 2222));
  EXPECT_TRUE(isIntEqualsWord(err.path(), 3333));
}

TEST_F(ExceptionBuiltinsTest, ImportErrorConstructWithInvalidKwargs) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, "x = ImportError(foo=123)"),
                            LayoutId::kTypeError,
                            "TypeError: invalid keyword argument supplied"));
}

TEST_F(ExceptionBuiltinsTest, ModuleNotFoundErrorManyArguments) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
exc = ModuleNotFoundError(1111, name=2222, path=3333)
)")
                   .isError());

  Object data(&scope, moduleAt(&runtime_, "__main__", "exc"));
  ASSERT_TRUE(data.isModuleNotFoundError());

  ModuleNotFoundError err(&scope, *data);
  EXPECT_TRUE(isIntEqualsWord(err.msg(), 1111));
  EXPECT_TRUE(isIntEqualsWord(err.name(), 2222));
  EXPECT_TRUE(isIntEqualsWord(err.path(), 3333));
}

TEST_F(ExceptionBuiltinsTest, DunderReprWithNoArgsHasEmptyParens) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = NameError().__repr__()
)")
                   .isError());

  EXPECT_TRUE(isStrEqualsCStr(moduleAt(&runtime_, "__main__", "result"),
                              "NameError()"));
}

TEST_F(ExceptionBuiltinsTest, DunderReprCallsTupleRepr) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
n = NameError().__class__.__name__
result = NameError(1, 2).__repr__()
)")
                   .isError());

  EXPECT_TRUE(
      isStrEqualsCStr(moduleAt(&runtime_, "__main__", "n"), "NameError"));
  EXPECT_TRUE(isStrEqualsCStr(moduleAt(&runtime_, "__main__", "result"),
                              "NameError(1, 2)"));
}

TEST_F(ExceptionBuiltinsTest,
       UnicodeDecodeErrorWithImproperFirstArgumentsRaisesTypeError) {
  const char* bad_arg = "exc = UnicodeDecodeError([], b'', 1, 1, '1')";
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, bad_arg),
                            LayoutId::kTypeError,
                            "argument 1 must be str, not list"));
}

TEST_F(ExceptionBuiltinsTest,
       UnicodeDecodeErrorWithImproperSecondArgumentsRaisesTypeError) {
  const char* bad_arg = "exc = UnicodeDecodeError('1', [], 1, 1, '1')";
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, bad_arg),
                            LayoutId::kTypeError,
                            "a bytes-like object is required, not 'list'"));
}

TEST_F(ExceptionBuiltinsTest,
       UnicodeDecodeErrorWithImproperThirdArgumentsRaisesTypeError) {
  const char* bad_arg = "exc = UnicodeDecodeError('1', b'', [], 1, '1')";
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(&runtime_, bad_arg), LayoutId::kTypeError,
                    "'list' object cannot be interpreted as an integer"));
}

TEST_F(ExceptionBuiltinsTest,
       UnicodeDecodeErrorWithImproperFourthArgumentsRaisesTypeError) {
  const char* bad_arg = "exc = UnicodeDecodeError('1', b'', 1, [], '1')";
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(&runtime_, bad_arg), LayoutId::kTypeError,
                    "'list' object cannot be interpreted as an integer"));
}

TEST_F(ExceptionBuiltinsTest,
       UnicodeDecodeErrorWithImproperFifthArgumentsRaisesTypeError) {
  const char* bad_arg = "exc = UnicodeDecodeError('1', b'', 1, 1, [])";
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, bad_arg),
                            LayoutId::kTypeError,
                            "argument 5 must be str, not list"));
}

TEST_F(ExceptionBuiltinsTest, UnicodeDecodeErrorReturnsObjectWithFieldsSet) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_,
                           "exc = UnicodeDecodeError('en', b'ob', 1, 2, 're')")
                   .isError());

  Object data(&scope, moduleAt(&runtime_, "__main__", "exc"));
  ASSERT_TRUE(data.isUnicodeDecodeError());

  UnicodeDecodeError err(&scope, *data);
  EXPECT_TRUE(isStrEqualsCStr(err.encoding(), "en"));
  Object bytes(&scope, err.object());
  EXPECT_TRUE(isBytesEqualsCStr(bytes, "ob"));
  EXPECT_TRUE(isIntEqualsWord(err.start(), 1));
  EXPECT_TRUE(isIntEqualsWord(err.end(), 2));
  EXPECT_TRUE(isStrEqualsCStr(err.reason(), "re"));
}

TEST_F(ExceptionBuiltinsTest,
       UnicodeDecodeErrorWithIndexSubclassReturnsObject) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Ind():
    def __index__(self):
        return 1
i = Ind()
exc = UnicodeDecodeError('en', b'ob', i, i, 're')
)")
                   .isError());

  Object data(&scope, moduleAt(&runtime_, "__main__", "exc"));
  ASSERT_TRUE(data.isUnicodeDecodeError());

  UnicodeDecodeError err(&scope, *data);
  EXPECT_TRUE(isIntEqualsWord(err.start(), 1));
  EXPECT_TRUE(isIntEqualsWord(err.end(), 1));
}

TEST_F(ExceptionBuiltinsTest,
       UnicodeEncodeErrorWithImproperFirstArgumentsRaisesTypeError) {
  const char* bad_arg = "exc = UnicodeEncodeError([], '', 1, 1, '1')";
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, bad_arg),
                            LayoutId::kTypeError,
                            "argument 1 must be str, not list"));
}

TEST_F(ExceptionBuiltinsTest,
       UnicodeEncodeErrorWithImproperSecondArgumentsRaisesTypeError) {
  const char* bad_arg = "exc = UnicodeEncodeError('1', [], 1, 1, '1')";
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, bad_arg),
                            LayoutId::kTypeError,
                            "argument 2 must be str, not 'list'"));
}

TEST_F(ExceptionBuiltinsTest,
       UnicodeEncodeErrorWithImproperThirdArgumentsRaisesTypeError) {
  const char* bad_arg = "exc = UnicodeEncodeError('1', '', [], 1, '1')";
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(&runtime_, bad_arg), LayoutId::kTypeError,
                    "'list' object cannot be interpreted as an integer"));
}

TEST_F(ExceptionBuiltinsTest,
       UnicodeEncodeErrorWithImproperFourthArgumentsRaisesTypeError) {
  const char* bad_arg = "exc = UnicodeEncodeError('1', '', 1, [], '1')";
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(&runtime_, bad_arg), LayoutId::kTypeError,
                    "'list' object cannot be interpreted as an integer"));
}

TEST_F(ExceptionBuiltinsTest,
       UnicodeEncodeErrorWithImproperFifthArgumentsRaisesTypeError) {
  const char* bad_arg = "exc = UnicodeEncodeError('1', '', 1, 1, [])";
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, bad_arg),
                            LayoutId::kTypeError,
                            "argument 5 must be str, not list"));
}

TEST_F(ExceptionBuiltinsTest, UnicodeEncodeErrorReturnsObjectWithFieldsSet) {
  HandleScope scope(thread_);
  ASSERT_FALSE(
      runFromCStr(&runtime_, "exc = UnicodeEncodeError('en', 'ob', 1, 2, 're')")
          .isError());

  Object data(&scope, moduleAt(&runtime_, "__main__", "exc"));
  ASSERT_TRUE(data.isUnicodeEncodeError());

  UnicodeEncodeError err(&scope, *data);
  EXPECT_TRUE(isStrEqualsCStr(err.encoding(), "en"));
  EXPECT_TRUE(isStrEqualsCStr(err.object(), "ob"));
  EXPECT_TRUE(isIntEqualsWord(err.start(), 1));
  EXPECT_TRUE(isIntEqualsWord(err.end(), 2));
  EXPECT_TRUE(isStrEqualsCStr(err.reason(), "re"));
}

TEST_F(ExceptionBuiltinsTest,
       UnicodeEncodeErrorWithIndexSubclassReturnsObject) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Ind():
    def __index__(self):
        return 1
i = Ind()
exc = UnicodeEncodeError('en', 'ob', i, i, 're')
)")
                   .isError());

  Object data(&scope, moduleAt(&runtime_, "__main__", "exc"));
  ASSERT_TRUE(data.isUnicodeEncodeError());

  UnicodeEncodeError err(&scope, *data);
  EXPECT_TRUE(isIntEqualsWord(err.start(), 1));
  EXPECT_TRUE(isIntEqualsWord(err.end(), 1));
}

TEST_F(ExceptionBuiltinsTest,
       UnicodeTranslateErrorWithImproperFirstArgumentsRaisesTypeError) {
  const char* bad_arg = "exc = UnicodeTranslateError([], 1, 1, '1')";
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, bad_arg),
                            LayoutId::kTypeError,
                            "argument 1 must be str, not list"));
}

TEST_F(ExceptionBuiltinsTest,
       UnicodeTranslateErrorWithImproperSecondArgumentsRaisesTypeError) {
  const char* bad_arg = "exc = UnicodeTranslateError('1', [], 1, '1')";
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(&runtime_, bad_arg), LayoutId::kTypeError,
                    "'list' object cannot be interpreted as an integer"));
}

TEST_F(ExceptionBuiltinsTest,
       UnicodeTranslateErrorWithImproperThirdArgumentsRaisesTypeError) {
  const char* bad_arg = "exc = UnicodeTranslateError('1', 1, [], '1')";
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(&runtime_, bad_arg), LayoutId::kTypeError,
                    "'list' object cannot be interpreted as an integer"));
}

TEST_F(ExceptionBuiltinsTest,
       UnicodeTranslateErrorWithImproperFourthArgumentsRaisesTypeError) {
  const char* bad_arg = "exc = UnicodeTranslateError('1', 1, 1, [])";
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, bad_arg),
                            LayoutId::kTypeError,
                            "argument 4 must be str, not list"));
}

TEST_F(ExceptionBuiltinsTest, UnicodeTranslateErrorReturnsObjectWithFieldsSet) {
  HandleScope scope(thread_);
  ASSERT_FALSE(
      runFromCStr(&runtime_, "exc = UnicodeTranslateError('obj', 1, 2, 're')")
          .isError());

  Object data(&scope, moduleAt(&runtime_, "__main__", "exc"));
  ASSERT_TRUE(data.isUnicodeTranslateError());

  UnicodeTranslateError err(&scope, *data);
  EXPECT_TRUE(isStrEqualsCStr(err.object(), "obj"));
  EXPECT_TRUE(isIntEqualsWord(err.start(), 1));
  EXPECT_TRUE(isIntEqualsWord(err.end(), 2));
  EXPECT_TRUE(isStrEqualsCStr(err.reason(), "re"));
}

TEST_F(ExceptionBuiltinsTest,
       UnicodeTranslateErrorWithIndexSubclassReturnsObject) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Ind():
    def __index__(self):
        return 1
i = Ind()
exc = UnicodeTranslateError('en', i, i, 're')
)")
                   .isError());

  Object data(&scope, moduleAt(&runtime_, "__main__", "exc"));
  ASSERT_TRUE(data.isUnicodeTranslateError());

  UnicodeTranslateError err(&scope, *data);
  EXPECT_TRUE(isIntEqualsWord(err.start(), 1));
  EXPECT_TRUE(isIntEqualsWord(err.end(), 1));
}

}  // namespace python
