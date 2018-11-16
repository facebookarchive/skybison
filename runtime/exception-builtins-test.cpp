#include "gtest/gtest.h"

#include "exception-builtins.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {

TEST(ExceptionBuiltinsTest, BaseExceptionNoArguments) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCString(R"(
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

  runtime.runFromCString(R"(
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

TEST(ExceptionBuiltinsTest, ExceptionManyArguments) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCString(R"(
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

TEST(ExceptionBuiltinsTest, NameErrorReturnsNameError) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCString(R"(
exc = NameError()
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

  runtime.runFromCString(R"(
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

  runtime.runFromCString(R"(
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

  runtime.runFromCString(R"(
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

TEST(ExceptionBuiltinsTest, SystemExitNoArguments) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCString(R"(
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

  runtime.runFromCString(R"(
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

  runtime.runFromCString(R"(
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

}  // namespace python
