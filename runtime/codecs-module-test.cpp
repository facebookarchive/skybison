#include "gtest/gtest.h"

#include "bytearray-builtins.h"
#include "codecs-module.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(CodecsModuleTest, DecodeASCIIWithWellFormedASCIIReturnsString) {
  Runtime runtime;
  HandleScope scope;
  byte encoded[] = {'h', 'e', 'l', 'l', 'o'};
  Bytes bytes(&scope, runtime.newBytesWithAll(encoded));
  Str errors(&scope, runtime.newStrFromCStr("strict"));
  Int index(&scope, runtime.newInt(0));
  ByteArray bytearray(&scope, runtime.newByteArray());
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underAsciiDecode,
                                       bytes, errors, index, bytearray));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Str str(&scope, result.at(0));
  EXPECT_EQ(str.length(), 5);
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 5));
  EXPECT_TRUE(str.equalsCStr("hello"));
}

TEST(CodecsModuleTest, DecodeASCIIWithIgnoreErrorHandlerReturnsStr) {
  Runtime runtime;
  HandleScope scope;
  byte encoded[] = {'h', 'e', 'l', 'l', 0x80, 'o'};
  Bytes bytes(&scope, runtime.newBytesWithAll(encoded));
  Str errors(&scope, runtime.newStrFromCStr("ignore"));
  Int index(&scope, runtime.newInt(0));
  ByteArray bytearray(&scope, runtime.newByteArray());
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underAsciiDecode,
                                       bytes, errors, index, bytearray));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Str str(&scope, result.at(0));
  EXPECT_EQ(str.length(), 5);
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 6));
  EXPECT_TRUE(str.equalsCStr("hello"));
}

TEST(CodecsModuleTest, DecodeASCIIWithReplaceErrorHandlerReturnsStr) {
  Runtime runtime;
  HandleScope scope;
  byte encoded[] = {'h', 'e', 'l', 'l', 0x80, 'o'};
  Bytes bytes(&scope, runtime.newBytesWithAll(encoded));
  Str errors(&scope, runtime.newStrFromCStr("replace"));
  Int index(&scope, runtime.newInt(0));
  ByteArray bytearray(&scope, runtime.newByteArray());
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underAsciiDecode,
                                       bytes, errors, index, bytearray));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Str str(&scope, result.at(0));
  EXPECT_EQ(str.length(), 8);
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 6));
  word placeholder;
  EXPECT_EQ(str.codePointAt(4, &placeholder), 0xfffd);
}

TEST(CodecsModuleTest, DecodeASCIIWithSurroogateescapeErrorHandlerReturnsStr) {
  Runtime runtime;
  HandleScope scope;
  byte encoded[] = {'h', 'e', 'l', 'l', 0x80, 'o'};
  Bytes bytes(&scope, runtime.newBytesWithAll(encoded));
  Str errors(&scope, runtime.newStrFromCStr("surrogateescape"));
  Int index(&scope, runtime.newInt(0));
  ByteArray bytearray(&scope, runtime.newByteArray());
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underAsciiDecode,
                                       bytes, errors, index, bytearray));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Str str(&scope, result.at(0));
  EXPECT_EQ(str.length(), 8);
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 6));
  word placeholder;
  EXPECT_EQ(str.codePointAt(4, &placeholder), 0xdc80);
}

}  // namespace python
