#include "gtest/gtest.h"

#include "objects.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(TestUtils, PyListEqual) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);

  runtime.runFromCStr(R"(
l = [None, False, 100, 200.5, 'hello']
i = 123456
)");
  Object list(&scope, moduleAt(&runtime, "__main__", "l"));
  Object not_list(&scope, moduleAt(&runtime, "__main__", "i"));

  auto const ok = AssertPyListEqual(
      "", "", list, {Value::none(), false, 100, 200.5, "hello"});
  EXPECT_TRUE(ok);

  auto const bad_type = AssertPyListEqual("not_list", "", not_list, {});
  ASSERT_FALSE(bad_type);
  const char* type_msg = R"( Type of: not_list
  Actual: smallint
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
  Actual: hello
Expected: four)";
  EXPECT_STREQ(bad_str.message(), str_msg);
}

TEST(TestUtils, NewEmptyCode) {
  Runtime runtime;
  HandleScope scope;

  Code code(&scope, testing::newEmptyCode(&runtime));
  EXPECT_EQ(code->argcount(), 0);
  EXPECT_TRUE(code->cell2arg()->isNoneType());
  ASSERT_TRUE(code->cellvars()->isTuple());
  EXPECT_EQ(RawTuple::cast(code->cellvars())->length(), 0);
  EXPECT_TRUE(code->code()->isNoneType());
  EXPECT_TRUE(code->consts()->isNoneType());
  EXPECT_TRUE(code->filename()->isNoneType());
  EXPECT_EQ(code->firstlineno(), 0);
  EXPECT_EQ(code->flags(), 0);
  ASSERT_TRUE(code->freevars()->isTuple());
  EXPECT_EQ(RawTuple::cast(code->freevars())->length(), 0);
  EXPECT_EQ(code->kwonlyargcount(), 0);
  EXPECT_TRUE(code->lnotab()->isNoneType());
  EXPECT_TRUE(code->name()->isNoneType());
  EXPECT_EQ(code->nlocals(), 0);
  EXPECT_EQ(code->stacksize(), 0);
  EXPECT_TRUE(code->varnames()->isTuple());
}

}  // namespace python
