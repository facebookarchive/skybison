#include "gtest/gtest.h"

#include "compile.h"
#include "test-utils.h"

namespace py {

using namespace testing;

using CompileTest = RuntimeFixture;

TEST_F(CompileTest, CompileFromCStrReturnsCodeObject) {
  HandleScope scope(thread_);
  Code code(&scope, compileFromCStr("a = 123", "<test>"));
  EXPECT_EQ(code.argcount(), 0);
  EXPECT_TRUE(Str::cast(code.filename()).equalsCStr("<test>"));
  Tuple names(&scope, code.names());
  ASSERT_EQ(names.length(), 1);
  EXPECT_TRUE(Str::cast(names.at(0)).equalsCStr("a"));
  Tuple consts(&scope, code.consts());
  ASSERT_EQ(consts.length(), 2);
  EXPECT_EQ(consts.at(0), SmallInt::fromWord(123));
  EXPECT_EQ(consts.at(1), NoneType::object());
  byte bytecode[] = {
      LOAD_CONST, 0, STORE_NAME, 0, LOAD_CONST, 1, RETURN_VALUE, 0,
  };
  Bytes expected_bytecode(&scope, runtime_.newBytesWithAll(bytecode));
  EXPECT_EQ(Bytes::cast(code.code()).compare(*expected_bytecode), 0);
}

TEST_F(CompileTest, CompileFromCStrWithSyntaxErrorRaisesSyntaxErrorException) {
  EXPECT_TRUE(raised(compileFromCStr(",,,", "<test>"), LayoutId::kSyntaxError));
}

}  // namespace py
