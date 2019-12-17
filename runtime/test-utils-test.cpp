#include "test-utils.h"

#include "gtest/gtest.h"

#include "objects.h"
#include "runtime.h"

namespace py {

using namespace testing;

using TestUtils = RuntimeFixture;

TEST_F(TestUtils, IsByteArrayEquals) {
  HandleScope scope(thread_);

  const byte view[] = {'f', 'o', 'o'};
  Object bytes(&scope, runtime_.newBytesWithAll(view));
  auto const type_err = isByteArrayEqualsBytes(bytes, view);
  EXPECT_FALSE(type_err);
  const char* type_msg = "is a 'bytes'";
  EXPECT_STREQ(type_err.message(), type_msg);

  ByteArray array(&scope, runtime_.newByteArray());
  runtime_.byteArrayExtend(thread_, array, view);
  auto const ok = isByteArrayEqualsBytes(array, view);
  EXPECT_TRUE(ok);

  auto const not_equal = isByteArrayEqualsCStr(array, "bar");
  EXPECT_FALSE(not_equal);
  const char* ne_msg = "bytearray(b'foo') is not equal to bytearray(b'bar')";
  EXPECT_STREQ(not_equal.message(), ne_msg);

  Object err(&scope, Error::error());
  auto const error = isByteArrayEqualsCStr(err, "");
  EXPECT_FALSE(error);
  const char* error_msg = "is an Error";
  EXPECT_STREQ(error.message(), error_msg);

  Object result(&scope,
                thread_->raiseWithFmt(LayoutId::kValueError, "bad things"));
  auto const exc = isByteArrayEqualsBytes(result, view);
  EXPECT_FALSE(exc);
  const char* exc_msg = "pending 'ValueError' exception";
  EXPECT_STREQ(exc.message(), exc_msg);
}

TEST_F(TestUtils, IsBytesEquals) {
  HandleScope scope(thread_);

  const byte view[] = {'f', 'o', 'o'};
  Object bytes(&scope, runtime_.newBytesWithAll(view));
  auto const ok = isBytesEqualsBytes(bytes, view);
  EXPECT_TRUE(ok);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Foo(bytes): pass
foo = Foo(b"foo")
)")
                   .isError());
  Object foo(&scope, mainModuleAt(&runtime_, "foo"));
  auto const subclass_ok = isBytesEqualsBytes(foo, view);
  EXPECT_TRUE(subclass_ok);

  auto const not_equal = isBytesEqualsCStr(bytes, "bar");
  EXPECT_FALSE(not_equal);
  const char* ne_msg = "b'foo' is not equal to b'bar'";
  EXPECT_STREQ(not_equal.message(), ne_msg);

  Object str(&scope, runtime_.newStrWithAll(view));
  auto const type_err = isBytesEqualsBytes(str, view);
  EXPECT_FALSE(type_err);
  const char* type_msg = "is a 'str'";
  EXPECT_STREQ(type_err.message(), type_msg);

  Object err(&scope, Error::error());
  auto const error = isBytesEqualsCStr(err, "");
  EXPECT_FALSE(error);
  const char* error_msg = "is an Error";
  EXPECT_STREQ(error.message(), error_msg);

  Object result(&scope,
                thread_->raiseWithFmt(LayoutId::kValueError, "bad things"));
  auto const exc = isBytesEqualsBytes(result, view);
  EXPECT_FALSE(exc);
  const char* exc_msg = "pending 'ValueError' exception";
  EXPECT_STREQ(exc.message(), exc_msg);
}

TEST_F(TestUtils, IsSymbolIdEquals) {
  EXPECT_TRUE(isSymbolIdEquals(SymbolId::kBuiltins, SymbolId::kBuiltins));

  auto const exc = isSymbolIdEquals(SymbolId::kTime, SymbolId::kFunction);
  EXPECT_FALSE(exc);
  const char* exc_msg = "Expected 'function', but got 'time'";
  EXPECT_STREQ(exc.message(), exc_msg);

  auto const invalid_exc =
      isSymbolIdEquals(SymbolId::kInvalid, SymbolId::kFunction);
  EXPECT_FALSE(invalid_exc);
  const char* invalid_exc_msg = "Expected 'function', but got '<Invalid>'";
  EXPECT_STREQ(invalid_exc.message(), invalid_exc_msg);
}

TEST_F(TestUtils, PyListEqual) {
  HandleScope scope(thread_);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
l = [None, False, 100, 200.5, 'hello']
i = 123456
)")
                   .isError());
  Object list(&scope, mainModuleAt(&runtime_, "l"));
  Object not_list(&scope, mainModuleAt(&runtime_, "i"));

  auto const ok = AssertPyListEqual(
      "", "", list, {Value::none(), false, 100, 200.5, "hello"});
  EXPECT_TRUE(ok);

  auto const bad_type = AssertPyListEqual("not_list", "", not_list, {});
  ASSERT_FALSE(bad_type);
  const char* type_msg = R"( Type of: not_list
  Actual: int
Expected: list)";
  EXPECT_STREQ(bad_type.message(), type_msg);

  auto const bad_length = AssertPyListEqual("list", "", list, {1, 2, 3});
  ASSERT_FALSE(bad_length);
  const char* length_msg = R"(Length of: list
   Actual: 5
 Expected: 3)";
  EXPECT_STREQ(bad_length.message(), length_msg);

  auto const bad_elem_type =
      AssertPyListEqual("list", "", list, {0, 1, 2, 3, 4});
  ASSERT_FALSE(bad_elem_type);
  const char* elem_type_msg = R"( Type of: list[0]
  Actual: NoneType
Expected: int)";
  EXPECT_STREQ(bad_elem_type.message(), elem_type_msg);

  auto const bad_bool =
      AssertPyListEqual("list", "", list, {Value::none(), true, 2, 3, 4});
  ASSERT_FALSE(bad_bool);
  const char* bool_msg = R"(Value of: list[1]
  Actual: False
Expected: True)";
  EXPECT_STREQ(bad_bool.message(), bool_msg);

  auto const bad_int =
      AssertPyListEqual("list", "", list, {Value::none(), false, 2, 3, 4});
  ASSERT_FALSE(bad_int);
  const char* int_msg = R"(Value of: list[2]
  Actual: 100
Expected: 2)";
  EXPECT_STREQ(bad_int.message(), int_msg);

  auto const bad_float = AssertPyListEqual(
      "list", "", list, {Value::none(), false, 100, 200.25, 4});
  ASSERT_FALSE(bad_float);
  const char* float_msg = R"(Value of: list[3]
  Actual: 200.5
Expected: 200.25)";
  EXPECT_STREQ(bad_float.message(), float_msg);

  auto const bad_str = AssertPyListEqual(
      "list", "", list, {Value::none(), false, 100, 200.5, "four"});
  ASSERT_FALSE(bad_str);
  const char* str_msg = R"(Value of: list[4]
  Actual: "hello"
Expected: four)";
  EXPECT_STREQ(bad_str.message(), str_msg);
}

TEST_F(TestUtils, NewEmptyCode) {
  HandleScope scope(thread_);

  Code code(&scope, newEmptyCode());
  EXPECT_EQ(code.argcount(), 0);
  EXPECT_TRUE(code.cell2arg().isNoneType());
  ASSERT_TRUE(code.cellvars().isTuple());
  EXPECT_EQ(Tuple::cast(code.cellvars()).length(), 0);
  EXPECT_TRUE(code.code().isBytes());
  EXPECT_TRUE(code.consts().isTuple());
  EXPECT_TRUE(code.filename().isStr());
  EXPECT_EQ(code.firstlineno(), 0);
  EXPECT_EQ(code.flags(), Code::Flags::kNofree | Code::Flags::kOptimized |
                              Code::Flags::kNewlocals);
  ASSERT_TRUE(code.freevars().isTuple());
  EXPECT_EQ(Tuple::cast(code.freevars()).length(), 0);
  EXPECT_EQ(code.kwonlyargcount(), 0);
  EXPECT_TRUE(code.lnotab().isBytes());
  EXPECT_TRUE(code.name().isStr());
  EXPECT_EQ(code.nlocals(), 0);
  EXPECT_EQ(code.stacksize(), 0);
  EXPECT_TRUE(code.varnames().isTuple());
}

}  // namespace py
