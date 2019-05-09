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

TEST(CodecsModuleTest, EncodeUTF16WithWellFormedASCIIReturnsBytes) {
  Runtime runtime;
  HandleScope scope;
  Object str(&scope, runtime.newStrFromCStr("hi"));
  Object errors(&scope, runtime.newStrFromCStr("unknown"));
  Object index(&scope, runtime.newInt(0));
  Object bytearray(&scope, runtime.newByteArray());
  Object byteorder(&scope, runtime.newInt(0));
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underUtf16Encode, str,
                                       errors, index, bytearray, byteorder));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Bytes bytes(&scope, result.at(0));
  EXPECT_EQ(bytes.length(), 4);
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 2));
  byte expected[] = {'h', 0x00, 'i', 0x00};
  EXPECT_TRUE(isBytesEqualsBytes(bytes, expected));
}

TEST(CodecsModuleTest, EncodeUTF16WithLargeIntByteorderRaisesOverflowError) {
  Runtime runtime;
  HandleScope scope;
  Object str(&scope, runtime.newStrFromCStr("hi"));
  Object errors(&scope, runtime.newStrFromCStr("unknown"));
  Object index(&scope, runtime.newInt(0));
  Object bytearray(&scope, runtime.newByteArray());
  Object byteorder(&scope, runtime.newInt(kMaxWord));
  EXPECT_TRUE(raisedWithStr(runBuiltin(UnderCodecsModule::underUtf16Encode, str,
                                       errors, index, bytearray, byteorder),
                            LayoutId::kOverflowError,
                            "Python int too large to convert to C int"));
}

TEST(CodecsModuleTest, EncodeUTF16WithIgnoreErrorHandlerReturnsStr) {
  Runtime runtime;
  HandleScope scope;
  Object str(&scope, runtime.newStrFromCStr("h\xed\xb2\x80i"));
  Object errors(&scope, runtime.newStrFromCStr("ignore"));
  Object index(&scope, runtime.newInt(0));
  Object bytearray(&scope, runtime.newByteArray());
  Object byteorder(&scope, runtime.newInt(0));
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underUtf16Encode, str,
                                       errors, index, bytearray, byteorder));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Bytes bytes(&scope, result.at(0));
  EXPECT_EQ(bytes.length(), 4);
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 3));
  byte expected[] = {'h', 0x00, 'i', 0x00};
  EXPECT_TRUE(isBytesEqualsBytes(bytes, expected));
}

TEST(CodecsModuleTest, EncodeUTF16WithReplaceErrorHandlerReturnsStr) {
  Runtime runtime;
  HandleScope scope;
  Object str(&scope, runtime.newStrFromCStr("hi\xed\xb2\x80"));
  Object errors(&scope, runtime.newStrFromCStr("replace"));
  Object index(&scope, runtime.newInt(0));
  Object bytearray(&scope, runtime.newByteArray());
  Object byteorder(&scope, runtime.newInt(0));
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underUtf16Encode, str,
                                       errors, index, bytearray, byteorder));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Bytes bytes(&scope, result.at(0));
  EXPECT_EQ(bytes.length(), 6);
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 3));
  byte expected[] = {'h', 0x00, 'i', 0x00, '?', 0x00};
  EXPECT_TRUE(isBytesEqualsBytes(bytes, expected));
}

TEST(CodecsModuleTest, EncodeUTF16WithSurroogateescapeErrorHandlerReturnsStr) {
  Runtime runtime;
  HandleScope scope;
  Object str(&scope, runtime.newStrFromCStr("h\xed\xb2\x80i"));
  Object errors(&scope, runtime.newStrFromCStr("surrogateescape"));
  Object index(&scope, runtime.newInt(0));
  Object bytearray(&scope, runtime.newByteArray());
  Object byteorder(&scope, runtime.newInt(0));
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underUtf16Encode, str,
                                       errors, index, bytearray, byteorder));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Bytes bytes(&scope, result.at(0));
  EXPECT_EQ(bytes.length(), 6);
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 3));
  byte expected[] = {'h', 0x00, 0x80, 0x00, 'i', 0x00};
  EXPECT_TRUE(isBytesEqualsBytes(bytes, expected));
}

TEST(CodecsModuleTest, EncodeUTF16WithSupplementaryStringReturnsUTF16Bytes) {
  Runtime runtime;
  HandleScope scope;
  Object str(&scope, runtime.newStrFromCStr("h\U0001d1f0i"));
  Object errors(&scope, runtime.newStrFromCStr("strict"));
  Object index(&scope, runtime.newInt(0));
  Object bytearray(&scope, runtime.newByteArray());
  Object byteorder(&scope, runtime.newInt(0));
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underUtf16Encode, str,
                                       errors, index, bytearray, byteorder));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Bytes bytes(&scope, result.at(0));
  EXPECT_EQ(bytes.length(), 8);
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 3));
  byte expected[] = {'h', 0x00, '4', 0xd8, 0xf0, 0xdd, 'i', 0x00};
  EXPECT_TRUE(isBytesEqualsBytes(bytes, expected));
}

TEST(CodecsModuleTest, EncodeUTF16LeWithSupplementaryStringReturnsUTF16Bytes) {
  Runtime runtime;
  HandleScope scope;
  Object str(&scope, runtime.newStrFromCStr("h\U0001d1f0i"));
  Object errors(&scope, runtime.newStrFromCStr("strict"));
  Object index(&scope, runtime.newInt(0));
  Object bytearray(&scope, runtime.newByteArray());
  Object byteorder(&scope, runtime.newInt(-1));
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underUtf16Encode, str,
                                       errors, index, bytearray, byteorder));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Bytes bytes(&scope, result.at(0));
  EXPECT_EQ(bytes.length(), 8);
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 3));
  byte expected[] = {'h', 0x00, '4', 0xd8, 0xf0, 0xdd, 'i', 0x00};
  EXPECT_TRUE(isBytesEqualsBytes(bytes, expected));
}

TEST(CodecsModuleTest, EncodeUTF16BeWithSupplementaryStringReturnsUTF16Bytes) {
  Runtime runtime;
  HandleScope scope;
  Object str(&scope, runtime.newStrFromCStr("h\U0001d1f0i"));
  Object errors(&scope, runtime.newStrFromCStr("strict"));
  Object index(&scope, runtime.newInt(0));
  Object bytearray(&scope, runtime.newByteArray());
  Object byteorder(&scope, runtime.newInt(1));
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underUtf16Encode, str,
                                       errors, index, bytearray, byteorder));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Bytes bytes(&scope, result.at(0));
  EXPECT_EQ(bytes.length(), 8);
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 3));
  byte expected[] = {0x00, 'h', 0xd8, '4', 0xdd, 0xf0, 0x00, 'i'};
  EXPECT_TRUE(isBytesEqualsBytes(bytes, expected));
}

TEST(CodecsModuleTest, EncodeUTF32WithWellFormedASCIIReturnsBytes) {
  Runtime runtime;
  HandleScope scope;
  Object str(&scope, runtime.newStrFromCStr("hi"));
  Object errors(&scope, runtime.newStrFromCStr("unknown"));
  Object index(&scope, runtime.newInt(0));
  Object bytearray(&scope, runtime.newByteArray());
  Object byteorder(&scope, runtime.newInt(0));
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underUtf32Encode, str,
                                       errors, index, bytearray, byteorder));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Bytes bytes(&scope, result.at(0));
  EXPECT_EQ(bytes.length(), 8);
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 2));
  byte expected[] = {'h', 0x00, 0x00, 0x00, 'i', 0x00, 0x00, 0x00};
  EXPECT_TRUE(isBytesEqualsBytes(bytes, expected));
}

TEST(CodecsModuleTest, EncodeUTF32WithLargeIntByteorderRaisesOverflowError) {
  Runtime runtime;
  HandleScope scope;
  Object str(&scope, runtime.newStrFromCStr("hi"));
  Object errors(&scope, runtime.newStrFromCStr("unknown"));
  Object index(&scope, runtime.newInt(0));
  Object bytearray(&scope, runtime.newByteArray());
  Object byteorder(&scope, runtime.newInt(kMaxWord));
  EXPECT_TRUE(raisedWithStr(runBuiltin(UnderCodecsModule::underUtf32Encode, str,
                                       errors, index, bytearray, byteorder),
                            LayoutId::kOverflowError,
                            "Python int too large to convert to C int"));
}

TEST(CodecsModuleTest, EncodeUTF32WithIgnoreErrorHandlerReturnsStr) {
  Runtime runtime;
  HandleScope scope;
  Object str(&scope, runtime.newStrFromCStr("h\xed\xb2\x80i"));
  Object errors(&scope, runtime.newStrFromCStr("ignore"));
  Object index(&scope, runtime.newInt(0));
  Object bytearray(&scope, runtime.newByteArray());
  Object byteorder(&scope, runtime.newInt(0));
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underUtf32Encode, str,
                                       errors, index, bytearray, byteorder));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Bytes bytes(&scope, result.at(0));
  EXPECT_EQ(bytes.length(), 8);
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 3));
  byte expected[] = {'h', 0x00, 0x00, 0x00, 'i', 0x00, 0x00, 0x00};
  EXPECT_TRUE(isBytesEqualsBytes(bytes, expected));
}

TEST(CodecsModuleTest, EncodeUTF32WithReplaceErrorHandlerReturnsStr) {
  Runtime runtime;
  HandleScope scope;
  Object str(&scope, runtime.newStrFromCStr("hi\xed\xb2\x80"));
  Object errors(&scope, runtime.newStrFromCStr("replace"));
  Object index(&scope, runtime.newInt(0));
  Object bytearray(&scope, runtime.newByteArray());
  Object byteorder(&scope, runtime.newInt(0));
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underUtf32Encode, str,
                                       errors, index, bytearray, byteorder));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Bytes bytes(&scope, result.at(0));
  EXPECT_EQ(bytes.length(), 12);
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 3));
  byte expected[] = {'h',  0x00, 0x00, 0x00, 'i',  0x00,
                     0x00, 0x00, '?',  0x00, 0x00, 0x00};
  EXPECT_TRUE(isBytesEqualsBytes(bytes, expected));
}

TEST(CodecsModuleTest, EncodeUTF32WithSurroogateescapeErrorHandlerReturnsStr) {
  Runtime runtime;
  HandleScope scope;
  Object str(&scope, runtime.newStrFromCStr("h\xed\xb2\x80i"));
  Object errors(&scope, runtime.newStrFromCStr("surrogateescape"));
  Object index(&scope, runtime.newInt(0));
  Object bytearray(&scope, runtime.newByteArray());
  Object byteorder(&scope, runtime.newInt(0));
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underUtf32Encode, str,
                                       errors, index, bytearray, byteorder));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Bytes bytes(&scope, result.at(0));
  EXPECT_EQ(bytes.length(), 12);
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 3));
  byte expected[] = {'h',  0x00, 0x00, 0x00, 0x80, 0x00,
                     0x00, 0x00, 'i',  0x00, 0x00, 0x00};
  EXPECT_TRUE(isBytesEqualsBytes(bytes, expected));
}

TEST(CodecsModuleTest, EncodeUTF32WithSupplementaryStringReturnsUTF32Bytes) {
  Runtime runtime;
  HandleScope scope;
  Object str(&scope, runtime.newStrFromCStr("h\U0001d1f0i"));
  Object errors(&scope, runtime.newStrFromCStr("strict"));
  Object index(&scope, runtime.newInt(0));
  Object bytearray(&scope, runtime.newByteArray());
  Object byteorder(&scope, runtime.newInt(0));
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underUtf32Encode, str,
                                       errors, index, bytearray, byteorder));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Bytes bytes(&scope, result.at(0));
  EXPECT_EQ(bytes.length(), 12);
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 3));
  byte expected[] = {'h',  0x00, 0x00, 0x00, 0xf0, 0xd1,
                     0x01, 0x00, 'i',  0x00, 0x00, 0x00};
  EXPECT_TRUE(isBytesEqualsBytes(bytes, expected));
}

TEST(CodecsModuleTest, EncodeUTF32LeWithSupplementaryStringReturnsUTF32Bytes) {
  Runtime runtime;
  HandleScope scope;
  Object str(&scope, runtime.newStrFromCStr("h\U0001d1f0i"));
  Object errors(&scope, runtime.newStrFromCStr("strict"));
  Object index(&scope, runtime.newInt(0));
  Object bytearray(&scope, runtime.newByteArray());
  Object byteorder(&scope, runtime.newInt(-1));
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underUtf32Encode, str,
                                       errors, index, bytearray, byteorder));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Bytes bytes(&scope, result.at(0));
  EXPECT_EQ(bytes.length(), 12);
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 3));
  byte expected[] = {'h',  0x00, 0x00, 0x00, 0xf0, 0xd1,
                     0x01, 0x00, 'i',  0x00, 0x00, 0x00};
  EXPECT_TRUE(isBytesEqualsBytes(bytes, expected));
}

TEST(CodecsModuleTest, EncodeUTF32BeWithSupplementaryStringReturnsUTF32Bytes) {
  Runtime runtime;
  HandleScope scope;
  Object str(&scope, runtime.newStrFromCStr("h\U0001d1f0i"));
  Object errors(&scope, runtime.newStrFromCStr("strict"));
  Object index(&scope, runtime.newInt(0));
  Object bytearray(&scope, runtime.newByteArray());
  Object byteorder(&scope, runtime.newInt(1));
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underUtf32Encode, str,
                                       errors, index, bytearray, byteorder));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Bytes bytes(&scope, result.at(0));
  EXPECT_EQ(bytes.length(), 12);
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 3));
  byte expected[] = {0x00, 0x00, 0x00, 'h',  0x00, 0x01,
                     0xd1, 0xf0, 0x00, 0x00, 0x00, 'i'};
  EXPECT_TRUE(isBytesEqualsBytes(bytes, expected));
}

}  // namespace python
