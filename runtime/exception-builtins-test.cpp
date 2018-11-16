#include "gtest/gtest.h"

#include "exception-builtins.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {
using namespace testing;

TEST(ExceptionBuiltinsTest, BaseExceptionNoArguments) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
exc = BaseException()
)");

  Handle<Module> main(&scope, testing::findModule(&runtime, "__main__"));
  Handle<Object> exc(&scope, testing::moduleAt(&runtime, main, "exc"));
  ASSERT_TRUE(exc->isBaseException());
  Handle<BaseException> base_exception(&scope, *exc);

  // No constructor arguments means args should contain an empty tuple.
  ASSERT_TRUE(base_exception->args()->isObjectArray());
  ASSERT_EQ(base_exception->args(), runtime.newObjectArray(0));
}

TEST(ExceptionBuiltinsTest, BaseExceptionManyArguments) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
exc = BaseException(1,2,3)
)");

  Handle<Module> main(&scope, testing::findModule(&runtime, "__main__"));
  Handle<Object> exc(&scope, testing::moduleAt(&runtime, main, "exc"));
  ASSERT_TRUE(exc->isBaseException());
  Handle<BaseException> base_exception(&scope, *exc);

  // The args attribute contains a tuple of the constructor arguments.
  ASSERT_TRUE(base_exception->args()->isObjectArray());
  Handle<ObjectArray> args(&scope, base_exception->args());
  EXPECT_EQ(args->at(0), SmallInt::fromWord(1));
  EXPECT_EQ(args->at(1), SmallInt::fromWord(2));
  EXPECT_EQ(args->at(2), SmallInt::fromWord(3));
}

TEST(ExceptionBuiltinsTest, StrFromBaseExceptionNoArgs) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
a = BaseException().__str__()
)");

  Handle<Module> main(&scope, testing::findModule(&runtime, "__main__"));
  Handle<Str> a(&scope, testing::moduleAt(&runtime, main, "a"));
  EXPECT_PYSTRING_EQ(*a, "");
}

TEST(ExceptionBuiltinsTest, StrFromBaseExceptionOneArg) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
a = BaseException("hello").__str__()
)");

  Handle<Module> main(&scope, testing::findModule(&runtime, "__main__"));
  Handle<Str> a(&scope, testing::moduleAt(&runtime, main, "a"));
  EXPECT_PYSTRING_EQ(*a, "hello");
}

TEST(ExceptionBuiltinsTest, StrFromBaseExceptionManyArgs) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
a = BaseException("hello", "world").__str__()
)");

  Handle<Module> main(&scope, testing::findModule(&runtime, "__main__"));
  Handle<Str> a(&scope, testing::moduleAt(&runtime, main, "a"));
  EXPECT_PYSTRING_EQ(*a, "('hello', 'world')");
}

TEST(ExceptionBuiltinsTest, ExceptionManyArguments) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
exc = Exception(1,2,3)
)");

  Handle<Module> main(&scope, testing::findModule(&runtime, "__main__"));
  Handle<Object> exc(&scope, testing::moduleAt(&runtime, main, "exc"));
  ASSERT_TRUE(exc->isException());
  Handle<Exception> exception(&scope, *exc);

  // The args attribute contains a tuple of the constructor arguments.
  ASSERT_TRUE(exception->args()->isObjectArray());
  Handle<ObjectArray> args(&scope, exception->args());
  EXPECT_EQ(args->at(0), SmallInt::fromWord(1));
  EXPECT_EQ(args->at(1), SmallInt::fromWord(2));
  EXPECT_EQ(args->at(2), SmallInt::fromWord(3));
}

TEST(ExceptionBuiltinsTest, SimpleExceptionTypesCanBeConstructed) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
attr_error = AttributeError()
name_error = NameError()
value_error = ValueError()
rt_error = RuntimeError()
)");

  Handle<Module> main(&scope, testing::findModule(&runtime, "__main__"));
  UncheckedHandle<Exception> attr_error(
      &scope, testing::moduleAt(&runtime, main, "attr_error"));
  UncheckedHandle<Exception> name_error(
      &scope, testing::moduleAt(&runtime, main, "name_error"));
  UncheckedHandle<Exception> value_error(
      &scope, testing::moduleAt(&runtime, main, "value_error"));
  UncheckedHandle<Exception> rt_error(
      &scope, testing::moduleAt(&runtime, main, "rt_error"));

  EXPECT_TRUE(
      runtime.hasSubClassFlag(*attr_error, Type::Flag::kBaseExceptionSubclass));
  EXPECT_EQ(attr_error->layoutId(), LayoutId::kAttributeError);
  EXPECT_TRUE(
      runtime.hasSubClassFlag(*name_error, Type::Flag::kBaseExceptionSubclass));
  EXPECT_EQ(name_error->layoutId(), LayoutId::kNameError);
  EXPECT_TRUE(runtime.hasSubClassFlag(*value_error,
                                      Type::Flag::kBaseExceptionSubclass));
  EXPECT_EQ(value_error->layoutId(), LayoutId::kValueError);
  EXPECT_TRUE(
      runtime.hasSubClassFlag(*rt_error, Type::Flag::kBaseExceptionSubclass));
  EXPECT_EQ(rt_error->layoutId(), LayoutId::kRuntimeError);
}

TEST(ExceptionBuiltinsTest, LookupErrorAndSubclassesHaveCorrectHierarchy) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
lookup_is_exc = issubclass(LookupError, Exception)
index_is_lookup = issubclass(IndexError, LookupError)
key_is_lookup = issubclass(KeyError, LookupError)
)");

  Handle<Module> main(&scope, testing::findModule(&runtime, "__main__"));
  Handle<Bool> lookup_is_exc(
      &scope, testing::moduleAt(&runtime, main, "lookup_is_exc"));
  Handle<Bool> index_is_lookup(
      &scope, testing::moduleAt(&runtime, main, "index_is_lookup"));
  Handle<Bool> key_is_lookup(
      &scope, testing::moduleAt(&runtime, main, "key_is_lookup"));

  EXPECT_TRUE(lookup_is_exc->value());
  EXPECT_TRUE(index_is_lookup->value());
  EXPECT_TRUE(key_is_lookup->value());
}

TEST(ExceptionBuiltinsTest, LookupErrorAndSubclassesCanBeConstructed) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
l = LookupError()
i = IndexError()
k = KeyError()
)");

  Handle<Module> main(&scope, testing::findModule(&runtime, "__main__"));
  Handle<LookupError> l(&scope, testing::moduleAt(&runtime, main, "l"));
  Handle<IndexError> i(&scope, testing::moduleAt(&runtime, main, "i"));
  Handle<KeyError> k(&scope, testing::moduleAt(&runtime, main, "k"));

  EXPECT_TRUE(runtime.hasSubClassFlag(*l, Type::Flag::kBaseExceptionSubclass));
  EXPECT_TRUE(runtime.hasSubClassFlag(*i, Type::Flag::kBaseExceptionSubclass));
  EXPECT_TRUE(runtime.hasSubClassFlag(*k, Type::Flag::kBaseExceptionSubclass));
}

TEST(ExceptionBuiltinsTest, KeyErrorStrPrintsMissingKey) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
s = KeyError("key").__str__()
)");

  Handle<Module> main(&scope, testing::findModule(&runtime, "__main__"));
  Handle<Str> s(&scope, testing::moduleAt(&runtime, main, "s"));
  EXPECT_PYSTRING_EQ(*s, "'key'");
}

TEST(ExceptionBuiltinsTest,
     KeyErrorStrWithMoreThanOneArgPrintsBaseExceptionStr) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
s = KeyError("key", "key2").__str__()
b = BaseException("key", "key2").__str__()
)");

  Handle<Module> main(&scope, testing::findModule(&runtime, "__main__"));
  Handle<Str> s(&scope, testing::moduleAt(&runtime, main, "s"));
  Handle<Str> b(&scope, testing::moduleAt(&runtime, main, "b"));
  EXPECT_PYSTRING_EQ(*s, *b);
}

TEST(ExceptionBuiltinsTest, TypeErrorReturnsTypeError) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
exc = TypeError()
)");

  Handle<Module> main(&scope, testing::findModule(&runtime, "__main__"));
  Handle<Object> exc(&scope, testing::moduleAt(&runtime, main, "exc"));
  UncheckedHandle<Exception> exception(&scope, *exc);

  // The args attribute contains a tuple of the constructor arguments.
  ASSERT_TRUE(exception->args()->isObjectArray());
  Handle<ObjectArray> args(&scope, exception->args());
  EXPECT_EQ(args->length(), 0);
}

TEST(ExceptionBuiltinsTest, StopIterationNoArguments) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
exc = StopIteration()
)");

  Handle<Module> main(&scope, testing::findModule(&runtime, "__main__"));
  Handle<Object> exc(&scope, testing::moduleAt(&runtime, main, "exc"));
  ASSERT_TRUE(exc->isStopIteration());
  Handle<StopIteration> stop_iteration(&scope, *exc);

  // No constructor arguments so value should be none.
  EXPECT_TRUE(stop_iteration->value()->isNone());

  // No constructor arguments means args should contain an empty tuple.
  ASSERT_TRUE(stop_iteration->args()->isObjectArray());
  Handle<ObjectArray> args(&scope, stop_iteration->args());
  EXPECT_EQ(args->length(), 0);
}

TEST(ExceptionBuiltinsTest, StopIterationOneArgument) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
exc = StopIteration(1)
)");

  Handle<Module> main(&scope, testing::findModule(&runtime, "__main__"));
  Handle<Object> exc(&scope, testing::moduleAt(&runtime, main, "exc"));
  ASSERT_TRUE(exc->isStopIteration());
  Handle<StopIteration> stop_iteration(&scope, *exc);

  // The value attribute should contain the first constructor argument.
  EXPECT_EQ(stop_iteration->value(), SmallInt::fromWord(1));

  // The args attribute contains a tuple of the constructor arguments.
  ASSERT_TRUE(stop_iteration->args()->isObjectArray());
  Handle<ObjectArray> args(&scope, stop_iteration->args());
  ASSERT_EQ(args->length(), 1);
  EXPECT_EQ(args->at(0), SmallInt::fromWord(1));
}

TEST(ExceptionBuiltinsTest, StopIterationManyArguments) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
exc = StopIteration(4, 5, 6)
)");

  Handle<Module> main(&scope, testing::findModule(&runtime, "__main__"));
  Handle<Object> exc(&scope, testing::moduleAt(&runtime, main, "exc"));
  ASSERT_TRUE(exc->isStopIteration());
  Handle<StopIteration> stop_iteration(&scope, *exc);

  // The value attribute should contain the first constructor argument.
  EXPECT_EQ(stop_iteration->value(), SmallInt::fromWord(4));

  // The args attribute contains a tuple of the constructor arguments.
  ASSERT_TRUE(stop_iteration->args()->isObjectArray());
  Handle<ObjectArray> args(&scope, stop_iteration->args());
  ASSERT_EQ(args->length(), 3);
  EXPECT_EQ(args->at(0), SmallInt::fromWord(4));
  EXPECT_EQ(args->at(1), SmallInt::fromWord(5));
  EXPECT_EQ(args->at(2), SmallInt::fromWord(6));
}

TEST(ExceptionBuiltinsTest, NotImplementedErrorNoArguments) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
exc = NotImplementedError()
exc_is_rt_error = issubclass(NotImplementedError, RuntimeError)
)");

  Handle<Module> main(&scope, testing::findModule(&runtime, "__main__"));
  Handle<NotImplementedError> exc(&scope,
                                  testing::moduleAt(&runtime, main, "exc"));
  Handle<Bool> exc_is_rt_error(
      &scope, testing::moduleAt(&runtime, main, "exc_is_rt_error"));

  EXPECT_TRUE(
      runtime.hasSubClassFlag(*exc, Type::Flag::kBaseExceptionSubclass));

  EXPECT_TRUE(exc_is_rt_error->value());
}

TEST(ExceptionBuiltinsTest, SystemExitNoArguments) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
exc = SystemExit()
)");

  Handle<Module> main(&scope, testing::findModule(&runtime, "__main__"));
  Handle<Object> exc(&scope, testing::moduleAt(&runtime, main, "exc"));
  ASSERT_TRUE(exc->isSystemExit());
  Handle<SystemExit> system_exit(&scope, *exc);
  ASSERT_TRUE(system_exit->args()->isObjectArray());

  // No constructor arguments so code should be none.
  EXPECT_TRUE(system_exit->code()->isNone());

  // No constructor arguments means args should contain an empty tuple.
  Handle<ObjectArray> args(&scope, system_exit->args());
  EXPECT_EQ(args->length(), 0);
}

TEST(ExceptionBuiltinsTest, SystemExitOneArgument) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
exc = SystemExit(1)
)");

  Handle<Module> main(&scope, testing::findModule(&runtime, "__main__"));
  Handle<Object> exc(&scope, testing::moduleAt(&runtime, main, "exc"));
  ASSERT_TRUE(exc->isSystemExit());
  Handle<SystemExit> system_exit(&scope, *exc);
  ASSERT_TRUE(system_exit->args()->isObjectArray());

  // The code attribute should contain the first constructor argument.
  EXPECT_EQ(system_exit->code(), SmallInt::fromWord(1));

  // The args attribute contains a tuple of the constructor arguments.
  Handle<ObjectArray> args(&scope, system_exit->args());
  ASSERT_EQ(args->length(), 1);
  EXPECT_EQ(args->at(0), SmallInt::fromWord(1));
}

TEST(ExceptionBuiltinsTest, SystemExitManyArguments) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
exc = SystemExit(4, 5, 6)
)");

  Handle<Module> main(&scope, testing::findModule(&runtime, "__main__"));
  Handle<Object> exc(&scope, testing::moduleAt(&runtime, main, "exc"));
  ASSERT_TRUE(exc->isSystemExit());
  Handle<SystemExit> system_exit(&scope, *exc);

  // The code attribute should contain the first constructor argument.
  EXPECT_EQ(system_exit->code(), SmallInt::fromWord(4));

  // The args attribute contains a tuple of the constructor arguments.
  ASSERT_TRUE(system_exit->args()->isObjectArray());
  Handle<ObjectArray> args(&scope, system_exit->args());
  ASSERT_EQ(args->length(), 3);
  EXPECT_EQ(args->at(0), SmallInt::fromWord(4));
  EXPECT_EQ(args->at(1), SmallInt::fromWord(5));
  EXPECT_EQ(args->at(2), SmallInt::fromWord(6));
}

TEST(ExceptionBuiltinsTest, SystemExitGetValue) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
exc = SystemExit(1111)
result = exc.value
)");

  Handle<Module> main(&scope, testing::findModule(&runtime, "__main__"));
  // The value attribute should contain the first constructor argument.
  Handle<Object> result(&scope, testing::moduleAt(&runtime, main, "result"));
  ASSERT_TRUE(result->isSmallInt());
  EXPECT_EQ(SmallInt::cast(*result)->value(), 1111);
}

TEST(ExceptionBuiltinsTest, ImportErrorConstructEmpty) {
  const char* src = R"(
x = ImportError()
)";
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr(src);
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> data(&scope, moduleAt(&runtime, main, "x"));
  ASSERT_TRUE(data->isImportError());

  Handle<ImportError> err(&scope, *data);
  EXPECT_EQ(err->msg(), None::object());
  EXPECT_EQ(err->path(), None::object());
  EXPECT_EQ(err->name(), None::object());

  err->setMsg(SmallInt::fromWord(1111));
  ASSERT_TRUE(err->msg()->isSmallInt());
  EXPECT_EQ(SmallInt::cast(err->msg())->value(), 1111);

  err->setPath(SmallInt::fromWord(2222));
  ASSERT_TRUE(err->path()->isSmallInt());
  EXPECT_EQ(SmallInt::cast(err->path())->value(), 2222);

  err->setName(SmallInt::fromWord(3333));
  ASSERT_TRUE(err->name()->isSmallInt());
  EXPECT_EQ(SmallInt::cast(err->name())->value(), 3333);
}

TEST(ExceptionBuiltinsTest, ImportErrorConstructWithMsg) {
  const char* src = R"(
x = ImportError(1111)
)";
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr(src);
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> data(&scope, moduleAt(&runtime, main, "x"));
  ASSERT_TRUE(data->isImportError());

  Handle<ImportError> err(&scope, *data);
  ASSERT_TRUE(err->msg()->isSmallInt());
  EXPECT_EQ(SmallInt::cast(err->msg())->value(), 1111);
  EXPECT_EQ(err->path(), None::object());
  EXPECT_EQ(err->name(), None::object());
}

TEST(ExceptionBuiltinsTest, ImportErrorConstructWithMsgNameAndPath) {
  const char* src = R"(
x = ImportError(1111, name=2222, path=3333)
)";
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr(src);
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> data(&scope, moduleAt(&runtime, main, "x"));
  ASSERT_TRUE(data->isImportError());

  Handle<ImportError> err(&scope, *data);
  ASSERT_TRUE(err->msg()->isSmallInt());
  EXPECT_EQ(SmallInt::cast(err->msg())->value(), 1111);
  ASSERT_TRUE(err->name()->isSmallInt());
  EXPECT_EQ(SmallInt::cast(err->name())->value(), 2222);
  ASSERT_TRUE(err->path()->isSmallInt());
  EXPECT_EQ(SmallInt::cast(err->path())->value(), 3333);
}

TEST(ExceptionBuiltinsDeathTest, ImportErrorConstructWithInvalidKwargs) {
  const char* src = R"(
x = ImportError(foo=123)
)";
  Runtime runtime;
  HandleScope scope;
  EXPECT_DEATH(runtime.runFromCStr(src), "RAISE_VARARGS");
}

TEST(ExceptionBuiltinsTest, ModuleNotFoundErrorManyArguments) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
exc = ModuleNotFoundError(1111, name=2222, path=3333)
)");

  Handle<Module> main(&scope, testing::findModule(&runtime, "__main__"));
  Handle<Object> data(&scope, testing::moduleAt(&runtime, main, "exc"));
  ASSERT_TRUE(data->isModuleNotFoundError());

  Handle<ModuleNotFoundError> err(&scope, *data);
  ASSERT_TRUE(err->msg()->isSmallInt());
  EXPECT_EQ(SmallInt::cast(err->msg())->value(), 1111);
  ASSERT_TRUE(err->name()->isSmallInt());
  EXPECT_EQ(SmallInt::cast(err->name())->value(), 2222);
  ASSERT_TRUE(err->path()->isSmallInt());
  EXPECT_EQ(SmallInt::cast(err->path())->value(), 3333);
}

}  // namespace python
