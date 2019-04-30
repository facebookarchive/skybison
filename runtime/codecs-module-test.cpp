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

TEST(CodecsModuleTest, EncodeASCIIWithWellFormedASCIIReturnsString) {
  Runtime runtime;
  HandleScope scope;
  Object str(&scope, runtime.newStrFromCStr("hello"));
  Object errors(&scope, runtime.newStrFromCStr("strict"));
  Object index(&scope, runtime.newInt(0));
  Object bytearray(&scope, runtime.newByteArray());
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underAsciiEncode, str,
                                       errors, index, bytearray));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Bytes bytes(&scope, result.at(0));
  EXPECT_EQ(bytes.length(), 5);
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 5));
  EXPECT_TRUE(isBytesEqualsCStr(bytes, "hello"));
}

TEST(CodecsModuleTest, EncodeASCIIWithIgnoreErrorHandlerReturnsString) {
  Runtime runtime;
  HandleScope scope;
  Object str(&scope, runtime.newStrFromCStr("hell\uac80o"));
  Object errors(&scope, runtime.newStrFromCStr("ignore"));
  Object index(&scope, runtime.newInt(0));
  Object bytearray(&scope, runtime.newByteArray());
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underAsciiEncode, str,
                                       errors, index, bytearray));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Bytes bytes(&scope, result.at(0));
  EXPECT_EQ(bytes.length(), 5);
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 6));
  EXPECT_TRUE(isBytesEqualsCStr(bytes, "hello"));
}

TEST(CodecsModuleTest, EncodeASCIIWithReplaceErrorHandlerReturnsString) {
  Runtime runtime;
  HandleScope scope;
  Object str(&scope, runtime.newStrFromCStr("hell\u0080o"));
  Object errors(&scope, runtime.newStrFromCStr("replace"));
  Object index(&scope, runtime.newInt(0));
  Object bytearray(&scope, runtime.newByteArray());
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underAsciiEncode, str,
                                       errors, index, bytearray));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Bytes bytes(&scope, result.at(0));
  EXPECT_EQ(bytes.length(), 6);
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 6));
  EXPECT_TRUE(isBytesEqualsCStr(bytes, "hell?o"));
}

TEST(CodecsModuleTest,
     EncodeASCIIWithSurrogateescapeErrorHandlerReturnsString) {
  Runtime runtime;
  HandleScope scope;
  Object str(&scope, runtime.newStrFromCStr("hell\xed\xb2\x80o"));
  Object errors(&scope, runtime.newStrFromCStr("surrogateescape"));
  Object index(&scope, runtime.newInt(0));
  Object bytearray(&scope, runtime.newByteArray());
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underAsciiEncode, str,
                                       errors, index, bytearray));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Bytes bytes(&scope, result.at(0));
  EXPECT_EQ(bytes.length(), 6);
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 6));
  EXPECT_TRUE(isBytesEqualsCStr(bytes, "hell\x80o"));
}

TEST(CodecsModuleTest, EncodeLatin1WithWellFormedLatin1ReturnsString) {
  Runtime runtime;
  HandleScope scope;
  Object str(&scope, runtime.newStrFromCStr("hell\u00e5"));
  Object errors(&scope, runtime.newStrFromCStr("strict"));
  Object index(&scope, runtime.newInt(0));
  Object bytearray(&scope, runtime.newByteArray());
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underLatin1Encode,
                                       str, errors, index, bytearray));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Bytes bytes(&scope, result.at(0));
  EXPECT_EQ(bytes.length(), 5);
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 5));
  EXPECT_TRUE(isBytesEqualsCStr(bytes, "hell\xe5"));
}

TEST(CodecsModuleTest, EncodeLatin1WithIgnoreErrorHandlerReturnsString) {
  Runtime runtime;
  HandleScope scope;
  Object str(&scope, runtime.newStrFromCStr("hell\u1c80o"));
  Object errors(&scope, runtime.newStrFromCStr("ignore"));
  Object index(&scope, runtime.newInt(0));
  Object bytearray(&scope, runtime.newByteArray());
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underLatin1Encode,
                                       str, errors, index, bytearray));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Bytes bytes(&scope, result.at(0));
  EXPECT_EQ(bytes.length(), 5);
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 6));
  EXPECT_TRUE(isBytesEqualsCStr(bytes, "hello"));
}

TEST(CodecsModuleTest, EncodeLatin1WithReplaceErrorHandlerReturnsString) {
  Runtime runtime;
  HandleScope scope;
  Object str(&scope, runtime.newStrFromCStr("hell\u0180o"));
  Object errors(&scope, runtime.newStrFromCStr("replace"));
  Object index(&scope, runtime.newInt(0));
  Object bytearray(&scope, runtime.newByteArray());
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underLatin1Encode,
                                       str, errors, index, bytearray));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Bytes bytes(&scope, result.at(0));
  EXPECT_EQ(bytes.length(), 6);
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 6));
  EXPECT_TRUE(isBytesEqualsCStr(bytes, "hell?o"));
}

TEST(CodecsModuleTest,
     EncodeLatin1WithSurrogateescapeErrorHandlerReturnsString) {
  Runtime runtime;
  HandleScope scope;
  Object str(&scope, runtime.newStrFromCStr("hell\xed\xb2\x80o"));
  Object errors(&scope, runtime.newStrFromCStr("surrogateescape"));
  Object index(&scope, runtime.newInt(0));
  Object bytearray(&scope, runtime.newByteArray());
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underLatin1Encode,
                                       str, errors, index, bytearray));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Bytes bytes(&scope, result.at(0));
  EXPECT_EQ(bytes.length(), 6);
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 6));
  EXPECT_TRUE(isBytesEqualsCStr(bytes, "hell\x80o"));
}

TEST(CodecsModuleTest, EncodeUTF8WithWellFormedASCIIReturnsString) {
  Runtime runtime;
  HandleScope scope;
  Object str(&scope, runtime.newStrFromCStr("hello"));
  Object errors(&scope, runtime.newStrFromCStr("strict"));
  Object index(&scope, runtime.newInt(0));
  Object bytearray(&scope, runtime.newByteArray());
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underUtf8Encode, str,
                                       errors, index, bytearray));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Bytes bytes(&scope, result.at(0));
  EXPECT_EQ(bytes.length(), 5);
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 5));
  EXPECT_TRUE(isBytesEqualsCStr(bytes, "hello"));
}

TEST(CodecsModuleTest, EncodeUTF8WithIgnoreErrorHandlerReturnsStr) {
  Runtime runtime;
  HandleScope scope;
  Object str(&scope, runtime.newStrFromCStr("hell\xed\xb2\x80o"));
  Object errors(&scope, runtime.newStrFromCStr("ignore"));
  Object index(&scope, runtime.newInt(0));
  Object bytearray(&scope, runtime.newByteArray());
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underUtf8Encode, str,
                                       errors, index, bytearray));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Bytes bytes(&scope, result.at(0));
  EXPECT_EQ(bytes.length(), 5);
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 6));
  EXPECT_TRUE(isBytesEqualsCStr(bytes, "hello"));
}

TEST(CodecsModuleTest, EncodeUTF8WithReplaceErrorHandlerReturnsStr) {
  Runtime runtime;
  HandleScope scope;
  Object str(&scope, runtime.newStrFromCStr("hell\xed\xb2\x80o"));
  Object errors(&scope, runtime.newStrFromCStr("replace"));
  Object index(&scope, runtime.newInt(0));
  Object bytearray(&scope, runtime.newByteArray());
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underUtf8Encode, str,
                                       errors, index, bytearray));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Bytes bytes(&scope, result.at(0));
  EXPECT_EQ(bytes.length(), 6);
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 6));
  EXPECT_TRUE(isBytesEqualsCStr(bytes, "hell?o"));
}

TEST(CodecsModuleTest, EncodeUTF8WithSurroogateescapeErrorHandlerReturnsStr) {
  Runtime runtime;
  HandleScope scope;
  Object str(&scope, runtime.newStrFromCStr("hell\xed\xb2\x80o"));
  Object errors(&scope, runtime.newStrFromCStr("surrogateescape"));
  Object index(&scope, runtime.newInt(0));
  Object bytearray(&scope, runtime.newByteArray());
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underUtf8Encode, str,
                                       errors, index, bytearray));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Bytes bytes(&scope, result.at(0));
  EXPECT_EQ(bytes.length(), 6);
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 6));
  EXPECT_TRUE(isBytesEqualsCStr(bytes, "hell\x80o"));
}

TEST(CodecsModuleTest, EncodeUTF8WithUnknownErrorHandlerReturnsSurrogateRange) {
  Runtime runtime;
  HandleScope scope;
  Object str(&scope, runtime.newStrFromCStr("hell\xed\xb2\x80\xed\xb2\x80o"));
  Object errors(&scope, runtime.newStrFromCStr("unknown"));
  Object index(&scope, runtime.newInt(0));
  Object bytearray(&scope, runtime.newByteArray());
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underUtf8Encode, str,
                                       errors, index, bytearray));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 4));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 6));
}

}  // namespace python
