#include "codecs-module.h"

#include "gtest/gtest.h"

#include "bytearray-builtins.h"
#include "runtime.h"
#include "test-utils.h"

namespace py {

using namespace testing;

using CodecsModuleTest = RuntimeFixture;

TEST_F(CodecsModuleTest, DecodeASCIIWithWellFormedASCIIReturnsString) {
  HandleScope scope(thread_);
  byte encoded[] = {'h', 'e', 'l', 'l', 'o'};
  Object bytes(&scope, runtime_->newBytesWithAll(encoded));
  Object errors(&scope, runtime_->newStrFromCStr("strict"));
  Object index(&scope, runtime_->newInt(0));
  Object strarray(&scope, runtime_->newStrArray());
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underAsciiDecode,
                                       bytes, errors, index, strarray));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Str str(&scope, result.at(0));
  EXPECT_EQ(str.charLength(), 5);
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 5));
  EXPECT_TRUE(str.equalsCStr("hello"));
}

TEST_F(CodecsModuleTest, DecodeASCIIWithIgnoreErrorHandlerReturnsStr) {
  HandleScope scope(thread_);
  byte encoded[] = {'h', 'e', 'l', 'l', 0x80, 'o'};
  Object bytes(&scope, runtime_->newBytesWithAll(encoded));
  Object errors(&scope, runtime_->newStrFromCStr("ignore"));
  Object index(&scope, runtime_->newInt(0));
  Object strarray(&scope, runtime_->newStrArray());
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underAsciiDecode,
                                       bytes, errors, index, strarray));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Str str(&scope, result.at(0));
  EXPECT_EQ(str.charLength(), 5);
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 6));
  EXPECT_TRUE(str.equalsCStr("hello"));
}

TEST_F(CodecsModuleTest, DecodeASCIIWithReplaceErrorHandlerReturnsStr) {
  HandleScope scope(thread_);
  byte encoded[] = {'h', 'e', 'l', 'l', 0x80, 'o'};
  Object bytes(&scope, runtime_->newBytesWithAll(encoded));
  Object errors(&scope, runtime_->newStrFromCStr("replace"));
  Object index(&scope, runtime_->newInt(0));
  Object strarray(&scope, runtime_->newStrArray());
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underAsciiDecode,
                                       bytes, errors, index, strarray));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Str str(&scope, result.at(0));
  EXPECT_EQ(str.charLength(), 8);
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 6));
  word placeholder;
  EXPECT_EQ(str.codePointAt(4, &placeholder), 0xfffd);
}

TEST_F(CodecsModuleTest,
       DecodeASCIIWithSurroogateescapeErrorHandlerReturnsStr) {
  HandleScope scope(thread_);
  byte encoded[] = {'h', 'e', 'l', 'l', 0x80, 'o'};
  Object bytes(&scope, runtime_->newBytesWithAll(encoded));
  Object errors(&scope, runtime_->newStrFromCStr("surrogateescape"));
  Object index(&scope, runtime_->newInt(0));
  Object strarray(&scope, runtime_->newStrArray());
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underAsciiDecode,
                                       bytes, errors, index, strarray));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Str str(&scope, result.at(0));
  EXPECT_EQ(str.charLength(), 8);
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 6));
  word placeholder;
  EXPECT_EQ(str.codePointAt(4, &placeholder), 0xdc80);
}

TEST_F(CodecsModuleTest, DecodeASCIIWithBytesSubclassReturnsStr) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class Foo(bytes): pass
encoded = Foo(b"hello")
)")
                   .isError());
  Object bytes(&scope, mainModuleAt(runtime_, "encoded"));
  Object errors(&scope, runtime_->newStrFromCStr("strict"));
  Object index(&scope, runtime_->newInt(0));
  Object strarray(&scope, runtime_->newStrArray());
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underAsciiDecode,
                                       bytes, errors, index, strarray));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Str str(&scope, result.at(0));
  EXPECT_EQ(str.charLength(), 5);
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 5));
  EXPECT_TRUE(str.equalsCStr("hello"));
}

TEST_F(CodecsModuleTest, DecodeUTF8WithWellFormedUTF8ReturnsString) {
  HandleScope scope(thread_);
  byte encoded[] = {'h', 0xC3, 0xA9, 0xF0, 0x9D, 0x87, 0xB0,
                    'l', 'l',  'o',  0xE2, 0xB3, 0x80};
  Object bytes(&scope, runtime_->newBytesWithAll(encoded));
  Object errors(&scope, runtime_->newStrFromCStr("strict"));
  Object index(&scope, runtime_->newInt(0));
  Object strarray(&scope, runtime_->newStrArray());
  Object is_final(&scope, Bool::trueObj());
  Object result_obj(
      &scope, runBuiltin(UnderCodecsModule::underUtf8Decode, bytes, errors,
                         index, strarray, is_final));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0),
                              "h\xC3\xA9\xF0\x9D\x87\xB0llo\xE2\xB3\x80"));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 13));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), ""));
}

TEST_F(CodecsModuleTest, DecodeUTF8WithIgnoreErrorHandlerReturnsStr) {
  HandleScope scope(thread_);
  byte encoded[] = {'h', 'e', 'l', 'l', 0x80, 'o'};
  Object bytes(&scope, runtime_->newBytesWithAll(encoded));
  Object errors(&scope, runtime_->newStrFromCStr("ignore"));
  Object index(&scope, runtime_->newInt(0));
  Object strarray(&scope, runtime_->newStrArray());
  Object is_final(&scope, Bool::trueObj());
  Object result_obj(
      &scope, runBuiltin(UnderCodecsModule::underUtf8Decode, bytes, errors,
                         index, strarray, is_final));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "hello"));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 6));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), ""));
}

TEST_F(CodecsModuleTest, DecodeUTF8WithReplaceErrorHandlerReturnsStr) {
  HandleScope scope(thread_);
  byte encoded[] = {'h', 'e', 'l', 'l', 0x80, 'o'};
  Object bytes(&scope, runtime_->newBytesWithAll(encoded));
  Object errors(&scope, runtime_->newStrFromCStr("replace"));
  Object index(&scope, runtime_->newInt(0));
  Object strarray(&scope, runtime_->newStrArray());
  Object is_final(&scope, Bool::trueObj());
  Object result_obj(
      &scope, runBuiltin(UnderCodecsModule::underUtf8Decode, bytes, errors,
                         index, strarray, is_final));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Str str(&scope, result.at(0));
  EXPECT_EQ(str.charLength(), 8);
  word placeholder;
  EXPECT_EQ(str.codePointAt(4, &placeholder), 0xfffd);
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 6));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), ""));
}

TEST_F(CodecsModuleTest, DecodeUTF8WithSurroogateescapeErrorHandlerReturnsStr) {
  HandleScope scope(thread_);
  byte encoded[] = {'h', 'e', 'l', 'l', 0x80, 'o'};
  Object bytes(&scope, runtime_->newBytesWithAll(encoded));
  Object errors(&scope, runtime_->newStrFromCStr("surrogateescape"));
  Object index(&scope, runtime_->newInt(0));
  Object strarray(&scope, runtime_->newStrArray());
  Object is_final(&scope, Bool::trueObj());
  Object result_obj(
      &scope, runBuiltin(UnderCodecsModule::underUtf8Decode, bytes, errors,
                         index, strarray, is_final));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Str str(&scope, result.at(0));
  EXPECT_EQ(str.charLength(), 8);
  word placeholder;
  EXPECT_EQ(str.codePointAt(4, &placeholder), 0xdc80);
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 6));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), ""));
}

TEST_F(CodecsModuleTest, DecodeUTF8WithInvalidStartByteReturnsIndices) {
  HandleScope scope(thread_);
  byte encoded[] = {'h', 'e', 'l', 'l', 0x80, 'o'};
  Object bytes(&scope, runtime_->newBytesWithAll(encoded));
  Object errors(&scope, runtime_->newStrFromCStr("strict"));
  Object index(&scope, runtime_->newInt(0));
  Object strarray(&scope, runtime_->newStrArray());
  Object is_final(&scope, Bool::trueObj());
  Object result_obj(
      &scope, runBuiltin(UnderCodecsModule::underUtf8Decode, bytes, errors,
                         index, strarray, is_final));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 4));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 5));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), "invalid start byte"));
}

TEST_F(CodecsModuleTest, DecodeUTF8StatefulWithInvalidStartByteReturnsIndices) {
  HandleScope scope(thread_);
  byte encoded[] = {'h', 'e', 'l', 'l', 0x80, 'o'};
  Object bytes(&scope, runtime_->newBytesWithAll(encoded));
  Object errors(&scope, runtime_->newStrFromCStr("strict"));
  Object index(&scope, runtime_->newInt(0));
  Object strarray(&scope, runtime_->newStrArray());
  Object is_final(&scope, Bool::falseObj());
  Object result_obj(
      &scope, runBuiltin(UnderCodecsModule::underUtf8Decode, bytes, errors,
                         index, strarray, is_final));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 4));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 5));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), "invalid start byte"));
}

TEST_F(CodecsModuleTest, DecodeUTF8WithUnexpectedEndReturnsIndices) {
  HandleScope scope(thread_);
  byte encoded[] = {'h', 'e', 'l', 'l', 0xC3};
  Object bytes(&scope, runtime_->newBytesWithAll(encoded));
  Object errors(&scope, runtime_->newStrFromCStr("strict"));
  Object index(&scope, runtime_->newInt(0));
  Object strarray(&scope, runtime_->newStrArray());
  Object is_final(&scope, Bool::trueObj());
  Object result_obj(
      &scope, runBuiltin(UnderCodecsModule::underUtf8Decode, bytes, errors,
                         index, strarray, is_final));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 4));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 5));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), "unexpected end of data"));
}

TEST_F(CodecsModuleTest, DecodeUTF8StatefulWithUnexpectedEndReturnsStr) {
  HandleScope scope(thread_);
  byte encoded[] = {'h', 'e', 'l', 'l', 0xC3};
  Object bytes(&scope, runtime_->newBytesWithAll(encoded));
  Object errors(&scope, runtime_->newStrFromCStr("strict"));
  Object index(&scope, runtime_->newInt(0));
  Object strarray(&scope, runtime_->newStrArray());
  Object is_final(&scope, Bool::falseObj());
  Object result_obj(
      &scope, runBuiltin(UnderCodecsModule::underUtf8Decode, bytes, errors,
                         index, strarray, is_final));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "hell"));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 4));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), ""));
}

TEST_F(CodecsModuleTest, DecodeUTF8WithInvalidFirstContReturnsIndices) {
  HandleScope scope(thread_);
  byte encoded[] = {'h', 'e', 'l', 'l', 0xE2, 0xC3, 'o'};
  Object bytes(&scope, runtime_->newBytesWithAll(encoded));
  Object errors(&scope, runtime_->newStrFromCStr("strict"));
  Object index(&scope, runtime_->newInt(0));
  Object strarray(&scope, runtime_->newStrArray());
  Object is_final(&scope, Bool::trueObj());
  Object result_obj(
      &scope, runBuiltin(UnderCodecsModule::underUtf8Decode, bytes, errors,
                         index, strarray, is_final));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 4));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 5));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), "invalid continuation byte"));
}

TEST_F(CodecsModuleTest, DecodeUTF8StatefulWithInvalidFirstContReturnsStr) {
  HandleScope scope(thread_);
  byte encoded[] = {'h', 'e', 'l', 'l', 0xE2, 0xC3, 'o'};
  Object bytes(&scope, runtime_->newBytesWithAll(encoded));
  Object errors(&scope, runtime_->newStrFromCStr("strict"));
  Object index(&scope, runtime_->newInt(0));
  Object strarray(&scope, runtime_->newStrArray());
  Object is_final(&scope, Bool::falseObj());
  Object result_obj(
      &scope, runBuiltin(UnderCodecsModule::underUtf8Decode, bytes, errors,
                         index, strarray, is_final));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "hell"));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 4));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), ""));
}

TEST_F(CodecsModuleTest, DecodeUTF8WithInvalidSecondContReturnsIndices) {
  HandleScope scope(thread_);
  byte encoded[] = {'h', 'e', 'l', 'l', 0xF0, 0x9D, 'o', 'o'};
  Object bytes(&scope, runtime_->newBytesWithAll(encoded));
  Object errors(&scope, runtime_->newStrFromCStr("strict"));
  Object index(&scope, runtime_->newInt(0));
  Object strarray(&scope, runtime_->newStrArray());
  Object is_final(&scope, Bool::trueObj());
  Object result_obj(
      &scope, runBuiltin(UnderCodecsModule::underUtf8Decode, bytes, errors,
                         index, strarray, is_final));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 4));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 6));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), "invalid continuation byte"));
}

TEST_F(CodecsModuleTest, DecodeUTF8StatefulWithInvalidSecondContReturnsStr) {
  HandleScope scope(thread_);
  byte encoded[] = {'h', 'e', 'l', 'l', 0xF0, 0x9D, 'o', 'o'};
  Object bytes(&scope, runtime_->newBytesWithAll(encoded));
  Object errors(&scope, runtime_->newStrFromCStr("strict"));
  Object index(&scope, runtime_->newInt(0));
  Object strarray(&scope, runtime_->newStrArray());
  Object is_final(&scope, Bool::falseObj());
  Object result_obj(
      &scope, runBuiltin(UnderCodecsModule::underUtf8Decode, bytes, errors,
                         index, strarray, is_final));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "hell"));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 4));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), ""));
}

TEST_F(CodecsModuleTest, DecodeUTF8WithInvalidThirdContReturnsIndices) {
  HandleScope scope(thread_);
  byte encoded[] = {'h', 'e', 'l', 'l', 0xF0, 0x9D, 0x87, 'o'};
  Object bytes(&scope, runtime_->newBytesWithAll(encoded));
  Object errors(&scope, runtime_->newStrFromCStr("strict"));
  Object index(&scope, runtime_->newInt(0));
  Object strarray(&scope, runtime_->newStrArray());
  Object is_final(&scope, Bool::trueObj());
  Object result_obj(
      &scope, runBuiltin(UnderCodecsModule::underUtf8Decode, bytes, errors,
                         index, strarray, is_final));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 4));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 7));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), "invalid continuation byte"));
}

TEST_F(CodecsModuleTest, DecodeUTF8StatefulWithInvalidThirdContReturnsStr) {
  HandleScope scope(thread_);
  byte encoded[] = {'h', 'e', 'l', 'l', 0xF0, 0x9D, 0x87, 'o'};
  Object bytes(&scope, runtime_->newBytesWithAll(encoded));
  Object errors(&scope, runtime_->newStrFromCStr("strict"));
  Object index(&scope, runtime_->newInt(0));
  Object strarray(&scope, runtime_->newStrArray());
  Object is_final(&scope, Bool::falseObj());
  Object result_obj(
      &scope, runBuiltin(UnderCodecsModule::underUtf8Decode, bytes, errors,
                         index, strarray, is_final));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "hell"));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 4));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), ""));
}

TEST_F(CodecsModuleTest, DecodeUTF8WithBytesSubclassReturnsStr) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class Foo(bytes): pass
encoded = Foo(b"hello")
)")
                   .isError());
  Object bytes(&scope, mainModuleAt(runtime_, "encoded"));
  Object errors(&scope, runtime_->newStrFromCStr("strict"));
  Object index(&scope, runtime_->newInt(0));
  Object strarray(&scope, runtime_->newStrArray());
  Object is_final(&scope, Bool::trueObj());
  Object result_obj(
      &scope, runBuiltin(UnderCodecsModule::underUtf8Decode, bytes, errors,
                         index, strarray, is_final));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "hello"));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 5));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), ""));
}

TEST_F(CodecsModuleTest, DecodeEscapeWithWellFormedLatin1ReturnsString) {
  HandleScope scope(thread_);
  byte encoded[] = {'h', 'e', 'l', 'l', 0xE9, 'o'};
  Object bytes(&scope, runtime_->newBytesWithAll(encoded));
  Object errors(&scope, runtime_->newStrFromCStr("strict"));
  Object encoding(&scope, runtime_->newStrFromCStr(""));
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underEscapeDecode,
                                       bytes, errors, encoding));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Object decoded(&scope, result.at(0));
  EXPECT_TRUE(isBytesEqualsCStr(decoded, "hell\xC3\xA9o"));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 6));
  EXPECT_TRUE(isIntEqualsWord(result.at(2), -1));
}

TEST_F(CodecsModuleTest, DecodeEscapeWithIgnoreAndTrailingSlashReturnsStr) {
  HandleScope scope(thread_);
  byte encoded[] = {'h', 'e', 'l', 'l', 'o', '\\'};
  Object bytes(&scope, runtime_->newBytesWithAll(encoded));
  Object errors(&scope, runtime_->newStrFromCStr("ignore"));
  Object encoding(&scope, runtime_->newStrFromCStr(""));
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underEscapeDecode,
                                       bytes, errors, encoding));
  ASSERT_TRUE(result_obj.isStr());
  EXPECT_TRUE(isStrEqualsCStr(*result_obj, "Trailing \\ in string"));
}

TEST_F(CodecsModuleTest, DecodeEscapeWithIgnoreAndTruncatedHexIterates) {
  HandleScope scope(thread_);
  byte encoded[] = {'h', 'e', 'l', 'l', '\\', 'x', '1', 'o'};
  Object bytes(&scope, runtime_->newBytesWithAll(encoded));
  Object errors(&scope, runtime_->newStrFromCStr("ignore"));
  Object encoding(&scope, runtime_->newStrFromCStr(""));
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underEscapeDecode,
                                       bytes, errors, encoding));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Object decoded(&scope, result.at(0));
  EXPECT_TRUE(isBytesEqualsCStr(decoded, "hello"));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 8));
  EXPECT_TRUE(isIntEqualsWord(result.at(2), -1));
}

TEST_F(CodecsModuleTest, DecodeEscapeWithReplaceAndTruncatedHexIterates) {
  HandleScope scope(thread_);
  byte encoded[] = {'h', 'e', 'l', 'l', '\\', 'x', 'o'};
  Object bytes(&scope, runtime_->newBytesWithAll(encoded));
  Object errors(&scope, runtime_->newStrFromCStr("replace"));
  Object encoding(&scope, runtime_->newStrFromCStr(""));
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underEscapeDecode,
                                       bytes, errors, encoding));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Object decoded(&scope, result.at(0));
  EXPECT_TRUE(isBytesEqualsCStr(decoded, "hell?o"));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 7));
  EXPECT_TRUE(isIntEqualsWord(result.at(2), -1));
}

TEST_F(CodecsModuleTest, DecodeEscapeWithStrictAndTruncatedHexReturnsMessage) {
  HandleScope scope(thread_);
  byte encoded[] = {'h', 'e', 'l', 'l', 'o', '\\', 'x', '1'};
  Object bytes(&scope, runtime_->newBytesWithAll(encoded));
  Object errors(&scope, runtime_->newStrFromCStr("strict"));
  Object encoding(&scope, runtime_->newStrFromCStr(""));
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underEscapeDecode,
                                       bytes, errors, encoding));
  ASSERT_TRUE(result_obj.isStr());
  EXPECT_TRUE(isStrEqualsCStr(*result_obj, "invalid \\x escape at position 5"));
}

TEST_F(CodecsModuleTest,
       DecodeEscapeWithUnknownHandlerAndTruncatedHexReturnsMessage) {
  HandleScope scope(thread_);
  byte encoded[] = {'h', 'e', 'l', 'l', 'o', '\\', 'x', '1'};
  Object bytes(&scope, runtime_->newBytesWithAll(encoded));
  Object errors(&scope, runtime_->newStrFromCStr("surrogateescape"));
  Object encoding(&scope, runtime_->newStrFromCStr(""));
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underEscapeDecode,
                                       bytes, errors, encoding));
  ASSERT_TRUE(result_obj.isStr());
  EXPECT_TRUE(isStrEqualsCStr(
      *result_obj,
      "decoding error; unknown error handling code: surrogateescape"));
}

TEST_F(CodecsModuleTest, DecodeEscapeEscapesSingleOctals) {
  HandleScope scope(thread_);
  byte encoded[] = {'h', 'e', 'l', 'l', 'o', '\\', '0', 'w'};
  Object bytes(&scope, runtime_->newBytesWithAll(encoded));
  Object errors(&scope, runtime_->newStrFromCStr("strict"));
  Object encoding(&scope, runtime_->newStrFromCStr(""));
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underEscapeDecode,
                                       bytes, errors, encoding));

  Tuple result(&scope, *result_obj);
  Object decoded(&scope, result.at(0));
  byte escaped[] = {'h', 'e', 'l', 'l', 'o', 0x00, 'w'};
  EXPECT_TRUE(isBytesEqualsBytes(decoded, View<byte>{escaped}));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 8));
  EXPECT_TRUE(isIntEqualsWord(result.at(2), -1));
}

TEST_F(CodecsModuleTest, DecodeEscapeEscapesMidStringDoubleOctals) {
  HandleScope scope(thread_);
  byte encoded[] = {'h', 'e', 'l', 'l', 'o', '\\', '4', '0', 'w'};
  Object bytes(&scope, runtime_->newBytesWithAll(encoded));
  Object errors(&scope, runtime_->newStrFromCStr("strict"));
  Object encoding(&scope, runtime_->newStrFromCStr(""));
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underEscapeDecode,
                                       bytes, errors, encoding));

  Tuple result(&scope, *result_obj);
  Object decoded(&scope, result.at(0));
  EXPECT_TRUE(isBytesEqualsCStr(decoded, "hello w"));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 9));
  EXPECT_TRUE(isIntEqualsWord(result.at(2), -1));
}

TEST_F(CodecsModuleTest, DecodeEscapeEscapesEndStringDoubleOctals) {
  HandleScope scope(thread_);
  byte encoded[] = {'h', 'e', 'l', 'l', 'o', '\\', '4', '0'};
  Object bytes(&scope, runtime_->newBytesWithAll(encoded));
  Object errors(&scope, runtime_->newStrFromCStr("strict"));
  Object encoding(&scope, runtime_->newStrFromCStr(""));
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underEscapeDecode,
                                       bytes, errors, encoding));

  Tuple result(&scope, *result_obj);
  Object decoded(&scope, result.at(0));
  EXPECT_TRUE(isBytesEqualsCStr(decoded, "hello "));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 8));
  EXPECT_TRUE(isIntEqualsWord(result.at(2), -1));
}

TEST_F(CodecsModuleTest, DecodeEscapeEscapesTripleOctals) {
  HandleScope scope(thread_);
  byte encoded[] = {'h', 'e', 'l', 'l', 'o', '\\', '7', '7', '7', 'w'};
  Object bytes(&scope, runtime_->newBytesWithAll(encoded));
  Object errors(&scope, runtime_->newStrFromCStr("strict"));
  Object encoding(&scope, runtime_->newStrFromCStr(""));
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underEscapeDecode,
                                       bytes, errors, encoding));

  Tuple result(&scope, *result_obj);
  Object decoded(&scope, result.at(0));
  EXPECT_TRUE(isBytesEqualsCStr(decoded, "hello\xFFw"));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 10));
  EXPECT_TRUE(isIntEqualsWord(result.at(2), -1));
}

TEST_F(CodecsModuleTest, DecodeEscapeEscapesHex) {
  HandleScope scope(thread_);
  byte encoded[] = {'h', 'e', 'l', 'l', 'o', '\\', 'x', 'e', 'E', 'w'};
  Object bytes(&scope, runtime_->newBytesWithAll(encoded));
  Object errors(&scope, runtime_->newStrFromCStr("strict"));
  Object encoding(&scope, runtime_->newStrFromCStr(""));
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underEscapeDecode,
                                       bytes, errors, encoding));

  Tuple result(&scope, *result_obj);
  Object decoded(&scope, result.at(0));
  EXPECT_TRUE(isBytesEqualsCStr(decoded, "hello\xEEw"));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 10));
  EXPECT_TRUE(isIntEqualsWord(result.at(2), -1));
}

TEST_F(CodecsModuleTest, DecodeEscapeSetsFirstInvalidEscape) {
  HandleScope scope(thread_);
  byte encoded[] = {'h', 'e', 'l', 'l', '\\', 'y', 'o'};
  Object bytes(&scope, runtime_->newBytesWithAll(encoded));
  Object errors(&scope, runtime_->newStrFromCStr("strict"));
  Object encoding(&scope, runtime_->newStrFromCStr(""));
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underEscapeDecode,
                                       bytes, errors, encoding));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Object decoded(&scope, result.at(0));
  EXPECT_TRUE(isBytesEqualsCStr(decoded, "hell\\yo"));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 7));
  EXPECT_TRUE(isIntEqualsWord(result.at(2), 5));
}

TEST_F(CodecsModuleTest, DecodeEscapeWithBytesSubclassReturnsStr) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class Foo(bytes): pass
encoded = Foo(b"hello")
)")
                   .isError());
  Object bytes(&scope, mainModuleAt(runtime_, "encoded"));
  Object errors(&scope, runtime_->newStrFromCStr("strict"));
  Object encoding(&scope, runtime_->newStrFromCStr(""));
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underEscapeDecode,
                                       bytes, errors, encoding));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Object decoded(&scope, result.at(0));
  EXPECT_TRUE(isBytesEqualsCStr(decoded, "hello"));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 5));
  EXPECT_TRUE(isIntEqualsWord(result.at(2), -1));
}

TEST_F(CodecsModuleTest, DecodeUnicodeEscapeWithWellFormedLatin1ReturnsString) {
  HandleScope scope(thread_);
  byte encoded[] = {'h', 'e', 'l', 'l', 0xE9, 'o'};
  Object bytes(&scope, runtime_->newBytesWithAll(encoded));
  Object errors(&scope, runtime_->newStrFromCStr("strict"));
  Object index(&scope, runtime_->newInt(0));
  Object strarray(&scope, runtime_->newStrArray());
  Object result_obj(
      &scope, runBuiltin(UnderCodecsModule::underUnicodeEscapeDecode, bytes,
                         errors, index, strarray));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Str str(&scope, result.at(0));
  EXPECT_EQ(str.charLength(), 7);
  EXPECT_TRUE(str.equalsCStr("hell\xC3\xA9o"));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 6));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), ""));
  EXPECT_TRUE(isIntEqualsWord(result.at(3), -1));
}

TEST_F(CodecsModuleTest, DecodeUnicodeEscapeWithIgnoreErrorHandlerReturnsStr) {
  HandleScope scope(thread_);
  byte encoded[] = {'h', 'e', 'l', 'l', 'o', '\\'};
  Object bytes(&scope, runtime_->newBytesWithAll(encoded));
  Object errors(&scope, runtime_->newStrFromCStr("ignore"));
  Object index(&scope, runtime_->newInt(0));
  Object strarray(&scope, runtime_->newStrArray());
  Object result_obj(
      &scope, runBuiltin(UnderCodecsModule::underUnicodeEscapeDecode, bytes,
                         errors, index, strarray));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Str str(&scope, result.at(0));
  EXPECT_EQ(str.charLength(), 5);
  EXPECT_TRUE(str.equalsCStr("hello"));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 6));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), ""));
  EXPECT_TRUE(isIntEqualsWord(result.at(3), -1));
}

TEST_F(CodecsModuleTest, DecodeUnicodeEscapeWithReplaceErrorHandlerReturnsStr) {
  HandleScope scope(thread_);
  byte encoded[] = {'h', 'e', 'l', 'l', 'o', '\\'};
  Object bytes(&scope, runtime_->newBytesWithAll(encoded));
  Object errors(&scope, runtime_->newStrFromCStr("replace"));
  Object index(&scope, runtime_->newInt(0));
  Object strarray(&scope, runtime_->newStrArray());
  Object result_obj(
      &scope, runBuiltin(UnderCodecsModule::underUnicodeEscapeDecode, bytes,
                         errors, index, strarray));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Str str(&scope, result.at(0));
  EXPECT_EQ(str.charLength(), 8);
  word placeholder;
  EXPECT_EQ(str.codePointAt(5, &placeholder), 0xfffd);
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 6));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), ""));
  EXPECT_TRUE(isIntEqualsWord(result.at(3), -1));
}

TEST_F(CodecsModuleTest,
       DecodeUnicodeEscapeReturnsMessageWhenEscapeAtEndOfString) {
  HandleScope scope(thread_);
  byte encoded[] = {'h', 'e', 'l', 'l', 'o', '\\'};
  Object bytes(&scope, runtime_->newBytesWithAll(encoded));
  Object errors(&scope, runtime_->newStrFromCStr("not-a-handler"));
  Object index(&scope, runtime_->newInt(0));
  Object strarray(&scope, runtime_->newStrArray());
  Object result_obj(
      &scope, runBuiltin(UnderCodecsModule::underUnicodeEscapeDecode, bytes,
                         errors, index, strarray));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 5));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 6));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), "\\ at end of string"));
  EXPECT_TRUE(isIntEqualsWord(result.at(3), -1));
}

TEST_F(CodecsModuleTest, DecodeUnicodeEscapeReturnsMessageOnTruncatedHex) {
  HandleScope scope(thread_);
  byte encoded[] = {'h', 'e', 'l', 'l', 'o', '\\', 'x', '1'};
  Object bytes(&scope, runtime_->newBytesWithAll(encoded));
  Object errors(&scope, runtime_->newStrFromCStr("not-a-handler"));
  Object index(&scope, runtime_->newInt(0));
  Object strarray(&scope, runtime_->newStrArray());
  Object result_obj(
      &scope, runBuiltin(UnderCodecsModule::underUnicodeEscapeDecode, bytes,
                         errors, index, strarray));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 5));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 8));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), "truncated \\xXX escape"));
  EXPECT_TRUE(isIntEqualsWord(result.at(3), -1));
}

TEST_F(CodecsModuleTest,
       DecodeUnicodeEscapeReturnsMessageOnTruncatedSmallUnicode) {
  HandleScope scope(thread_);
  byte encoded[] = {'h', 'e', 'l', 'l', 'o', '\\', 'u', '0'};
  Object bytes(&scope, runtime_->newBytesWithAll(encoded));
  Object errors(&scope, runtime_->newStrFromCStr("not-a-handler"));
  Object index(&scope, runtime_->newInt(0));
  Object strarray(&scope, runtime_->newStrArray());
  Object result_obj(
      &scope, runBuiltin(UnderCodecsModule::underUnicodeEscapeDecode, bytes,
                         errors, index, strarray));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 5));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 8));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), "truncated \\uXXXX escape"));
  EXPECT_TRUE(isIntEqualsWord(result.at(3), -1));
}

TEST_F(CodecsModuleTest,
       DecodeUnicodeEscapeReturnsMessageOnTruncatedLargeUnicode) {
  HandleScope scope(thread_);
  byte encoded[] = {'h', 'e', 'l', 'l', 'o', '\\', 'U', '0'};
  Object bytes(&scope, runtime_->newBytesWithAll(encoded));
  Object errors(&scope, runtime_->newStrFromCStr("not-a-handler"));
  Object index(&scope, runtime_->newInt(0));
  Object strarray(&scope, runtime_->newStrArray());
  Object result_obj(
      &scope, runBuiltin(UnderCodecsModule::underUnicodeEscapeDecode, bytes,
                         errors, index, strarray));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 5));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 8));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), "truncated \\uXXXXXXXX escape"));
  EXPECT_TRUE(isIntEqualsWord(result.at(3), -1));
}

TEST_F(CodecsModuleTest, DecodeUnicodeEscapeReturnsMessageOnOversizedUnicode) {
  HandleScope scope(thread_);
  byte encoded[] = {'h', 'e', 'l', 'l', 'o', '\\', 'U', '0',
                    '1', '1', '0', '0', '0', '0',  '0'};
  Object bytes(&scope, runtime_->newBytesWithAll(encoded));
  Object errors(&scope, runtime_->newStrFromCStr("not-a-handler"));
  Object index(&scope, runtime_->newInt(0));
  Object strarray(&scope, runtime_->newStrArray());
  Object result_obj(
      &scope, runBuiltin(UnderCodecsModule::underUnicodeEscapeDecode, bytes,
                         errors, index, strarray));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 5));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 15));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), "illegal Unicode character"));
  EXPECT_TRUE(isIntEqualsWord(result.at(3), -1));
}

TEST_F(CodecsModuleTest, DecodeUnicodeEscapeWithTruncatedHexProperlyIterates) {
  HandleScope scope(thread_);
  byte encoded[] = {'h', 'e', 'l', 'l', '\\', 'U', '1',
                    '1', '0', '0', '0', '0',  'o'};
  Object bytes(&scope, runtime_->newBytesWithAll(encoded));
  Object errors(&scope, runtime_->newStrFromCStr("ignore"));
  Object index(&scope, runtime_->newInt(0));
  Object strarray(&scope, runtime_->newStrArray());
  Object result_obj(
      &scope, runBuiltin(UnderCodecsModule::underUnicodeEscapeDecode, bytes,
                         errors, index, strarray));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "hello"));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 13));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), ""));
  EXPECT_TRUE(isIntEqualsWord(result.at(3), -1));
}

TEST_F(CodecsModuleTest, DecodeUnicodeEscapeProperlyEscapesSingleOctals) {
  HandleScope scope(thread_);
  byte encoded[] = {'h', 'e', 'l', 'l', 'o', '\\', '0', 'w'};
  Object bytes(&scope, runtime_->newBytesWithAll(encoded));
  Object errors(&scope, runtime_->newStrFromCStr("strict"));
  Object index(&scope, runtime_->newInt(0));
  Object strarray(&scope, runtime_->newStrArray());
  Object result_obj(
      &scope, runBuiltin(UnderCodecsModule::underUnicodeEscapeDecode, bytes,
                         errors, index, strarray));

  Tuple result(&scope, *result_obj);
  byte escaped[] = {'h', 'e', 'l', 'l', 'o', 0x00, 'w'};
  Str expected(&scope, runtime_->newStrWithAll(View<byte>{escaped}));
  Object decoded(&scope, result.at(0));
  EXPECT_TRUE(isStrEquals(decoded, expected));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 8));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), ""));
  EXPECT_TRUE(isIntEqualsWord(result.at(3), -1));
}

TEST_F(CodecsModuleTest,
       DecodeUnicodeEscapeProperlyEscapesMidStringDoubleOctals) {
  HandleScope scope(thread_);
  byte encoded[] = {'h', 'e', 'l', 'l', 'o', '\\', '4', '0', 'w'};
  Object bytes(&scope, runtime_->newBytesWithAll(encoded));
  Object errors(&scope, runtime_->newStrFromCStr("strict"));
  Object index(&scope, runtime_->newInt(0));
  Object strarray(&scope, runtime_->newStrArray());
  Object result_obj(
      &scope, runBuiltin(UnderCodecsModule::underUnicodeEscapeDecode, bytes,
                         errors, index, strarray));

  Tuple result(&scope, *result_obj);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "hello w"));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 9));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), ""));
  EXPECT_TRUE(isIntEqualsWord(result.at(3), -1));
}

TEST_F(CodecsModuleTest,
       DecodeUnicodeEscapeProperlyEscapesEndStringDoubleOctals) {
  HandleScope scope(thread_);
  byte encoded[] = {'h', 'e', 'l', 'l', 'o', '\\', '4', '0'};
  Object bytes(&scope, runtime_->newBytesWithAll(encoded));
  Object errors(&scope, runtime_->newStrFromCStr("strict"));
  Object index(&scope, runtime_->newInt(0));
  Object strarray(&scope, runtime_->newStrArray());
  Object result_obj(
      &scope, runBuiltin(UnderCodecsModule::underUnicodeEscapeDecode, bytes,
                         errors, index, strarray));

  Tuple result(&scope, *result_obj);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "hello "));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 8));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), ""));
  EXPECT_TRUE(isIntEqualsWord(result.at(3), -1));
}

TEST_F(CodecsModuleTest, DecodeUnicodeEscapeProperlyEscapesTripleOctals) {
  HandleScope scope(thread_);
  byte encoded[] = {'h', 'e', 'l', 'l', 'o', '\\', '7', '7', '7', 'w'};
  Object bytes(&scope, runtime_->newBytesWithAll(encoded));
  Object errors(&scope, runtime_->newStrFromCStr("strict"));
  Object index(&scope, runtime_->newInt(0));
  Object strarray(&scope, runtime_->newStrArray());
  Object result_obj(
      &scope, runBuiltin(UnderCodecsModule::underUnicodeEscapeDecode, bytes,
                         errors, index, strarray));

  Tuple result(&scope, *result_obj);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "hello\xC7\xBFw"));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 10));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), ""));
  EXPECT_TRUE(isIntEqualsWord(result.at(3), -1));
}

TEST_F(CodecsModuleTest, DecodeUnicodeEscapeSetsFirstInvalidEscape) {
  HandleScope scope(thread_);
  byte encoded[] = {'h', 'e', 'l', 'l', '\\', 'y', 'o'};
  Object bytes(&scope, runtime_->newBytesWithAll(encoded));
  Object errors(&scope, runtime_->newStrFromCStr("strict"));
  Object index(&scope, runtime_->newInt(0));
  Object strarray(&scope, runtime_->newStrArray());
  Object result_obj(
      &scope, runBuiltin(UnderCodecsModule::underUnicodeEscapeDecode, bytes,
                         errors, index, strarray));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  EXPECT_TRUE(isStrEqualsCStr(result.at(0), "hell\\yo"));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 7));
  EXPECT_TRUE(isStrEqualsCStr(result.at(2), ""));
  EXPECT_TRUE(isIntEqualsWord(result.at(3), 5));
}

TEST_F(CodecsModuleTest, DecodeUnicodeEscapeWithBytesSubclassReturnsStr) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class Foo(bytes): pass
encoded = Foo(b"hello")
)")
                   .isError());
  Object bytes(&scope, mainModuleAt(runtime_, "encoded"));
  Object errors(&scope, runtime_->newStrFromCStr("strict"));
  Object index(&scope, runtime_->newInt(0));
  Object strarray(&scope, runtime_->newStrArray());
  Object result_obj(
      &scope, runBuiltin(UnderCodecsModule::underUnicodeEscapeDecode, bytes,
                         errors, index, strarray));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Str str(&scope, result.at(0));
  EXPECT_EQ(str.charLength(), 5);
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 5));
  EXPECT_TRUE(str.equalsCStr("hello"));
}

TEST_F(CodecsModuleTest, EncodeASCIIWithWellFormedASCIIReturnsString) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_->newStrFromCStr("hello"));
  Object errors(&scope, runtime_->newStrFromCStr("strict"));
  Object index(&scope, runtime_->newInt(0));
  Object bytearray(&scope, runtime_->newByteArray());
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underAsciiEncode, str,
                                       errors, index, bytearray));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Bytes bytes(&scope, result.at(0));
  EXPECT_EQ(bytes.length(), 5);
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 5));
  EXPECT_TRUE(isBytesEqualsCStr(bytes, "hello"));
}

TEST_F(CodecsModuleTest, EncodeASCIIWithIgnoreErrorHandlerReturnsString) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_->newStrFromCStr("hell\uac80o"));
  Object errors(&scope, runtime_->newStrFromCStr("ignore"));
  Object index(&scope, runtime_->newInt(0));
  Object bytearray(&scope, runtime_->newByteArray());
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underAsciiEncode, str,
                                       errors, index, bytearray));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Bytes bytes(&scope, result.at(0));
  EXPECT_EQ(bytes.length(), 5);
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 6));
  EXPECT_TRUE(isBytesEqualsCStr(bytes, "hello"));
}

TEST_F(CodecsModuleTest, EncodeASCIIWithReplaceErrorHandlerReturnsString) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_->newStrFromCStr("hell\u0080o"));
  Object errors(&scope, runtime_->newStrFromCStr("replace"));
  Object index(&scope, runtime_->newInt(0));
  Object bytearray(&scope, runtime_->newByteArray());
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underAsciiEncode, str,
                                       errors, index, bytearray));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Bytes bytes(&scope, result.at(0));
  EXPECT_EQ(bytes.length(), 6);
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 6));
  EXPECT_TRUE(isBytesEqualsCStr(bytes, "hell?o"));
}

TEST_F(CodecsModuleTest,
       EncodeASCIIWithSurrogateescapeErrorHandlerReturnsString) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_->newStrFromCStr("hell\xed\xb2\x80o"));
  Object errors(&scope, runtime_->newStrFromCStr("surrogateescape"));
  Object index(&scope, runtime_->newInt(0));
  Object bytearray(&scope, runtime_->newByteArray());
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underAsciiEncode, str,
                                       errors, index, bytearray));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Bytes bytes(&scope, result.at(0));
  EXPECT_EQ(bytes.length(), 6);
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 6));
  EXPECT_TRUE(isBytesEqualsCStr(bytes, "hell\x80o"));
}

TEST_F(CodecsModuleTest, EncodeLatin1WithWellFormedLatin1ReturnsString) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_->newStrFromCStr("hell\u00e5"));
  Object errors(&scope, runtime_->newStrFromCStr("strict"));
  Object index(&scope, runtime_->newInt(0));
  Object bytearray(&scope, runtime_->newByteArray());
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underLatin1Encode,
                                       str, errors, index, bytearray));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Bytes bytes(&scope, result.at(0));
  EXPECT_EQ(bytes.length(), 5);
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 5));
  EXPECT_TRUE(isBytesEqualsCStr(bytes, "hell\xe5"));
}

TEST_F(CodecsModuleTest, EncodeLatin1WithIgnoreErrorHandlerReturnsString) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_->newStrFromCStr("hell\u1c80o"));
  Object errors(&scope, runtime_->newStrFromCStr("ignore"));
  Object index(&scope, runtime_->newInt(0));
  Object bytearray(&scope, runtime_->newByteArray());
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underLatin1Encode,
                                       str, errors, index, bytearray));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Bytes bytes(&scope, result.at(0));
  EXPECT_EQ(bytes.length(), 5);
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 6));
  EXPECT_TRUE(isBytesEqualsCStr(bytes, "hello"));
}

TEST_F(CodecsModuleTest, EncodeLatin1WithReplaceErrorHandlerReturnsString) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_->newStrFromCStr("hell\u0180o"));
  Object errors(&scope, runtime_->newStrFromCStr("replace"));
  Object index(&scope, runtime_->newInt(0));
  Object bytearray(&scope, runtime_->newByteArray());
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underLatin1Encode,
                                       str, errors, index, bytearray));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Bytes bytes(&scope, result.at(0));
  EXPECT_EQ(bytes.length(), 6);
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 6));
  EXPECT_TRUE(isBytesEqualsCStr(bytes, "hell?o"));
}

TEST_F(CodecsModuleTest,
       EncodeLatin1WithSurrogateescapeErrorHandlerReturnsString) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_->newStrFromCStr("hell\xed\xb2\x80o"));
  Object errors(&scope, runtime_->newStrFromCStr("surrogateescape"));
  Object index(&scope, runtime_->newInt(0));
  Object bytearray(&scope, runtime_->newByteArray());
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underLatin1Encode,
                                       str, errors, index, bytearray));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Bytes bytes(&scope, result.at(0));
  EXPECT_EQ(bytes.length(), 6);
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 6));
  EXPECT_TRUE(isBytesEqualsCStr(bytes, "hell\x80o"));
}

TEST_F(CodecsModuleTest, EncodeUTF8WithWellFormedASCIIReturnsString) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_->newStrFromCStr("hello"));
  Object errors(&scope, runtime_->newStrFromCStr("strict"));
  Object index(&scope, runtime_->newInt(0));
  Object bytearray(&scope, runtime_->newByteArray());
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underUtf8Encode, str,
                                       errors, index, bytearray));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Bytes bytes(&scope, result.at(0));
  EXPECT_EQ(bytes.length(), 5);
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 5));
  EXPECT_TRUE(isBytesEqualsCStr(bytes, "hello"));
}

TEST_F(CodecsModuleTest, EncodeUTF8WithIgnoreErrorHandlerReturnsStr) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_->newStrFromCStr("hell\xed\xb2\x80o"));
  Object errors(&scope, runtime_->newStrFromCStr("ignore"));
  Object index(&scope, runtime_->newInt(0));
  Object bytearray(&scope, runtime_->newByteArray());
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underUtf8Encode, str,
                                       errors, index, bytearray));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Bytes bytes(&scope, result.at(0));
  EXPECT_EQ(bytes.length(), 5);
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 6));
  EXPECT_TRUE(isBytesEqualsCStr(bytes, "hello"));
}

TEST_F(CodecsModuleTest, EncodeUTF8WithReplaceErrorHandlerReturnsStr) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_->newStrFromCStr("hell\xed\xb2\x80o"));
  Object errors(&scope, runtime_->newStrFromCStr("replace"));
  Object index(&scope, runtime_->newInt(0));
  Object bytearray(&scope, runtime_->newByteArray());
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underUtf8Encode, str,
                                       errors, index, bytearray));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Bytes bytes(&scope, result.at(0));
  EXPECT_EQ(bytes.length(), 6);
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 6));
  EXPECT_TRUE(isBytesEqualsCStr(bytes, "hell?o"));
}

TEST_F(CodecsModuleTest, EncodeUTF8WithSurroogateescapeErrorHandlerReturnsStr) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_->newStrFromCStr("hell\xed\xb2\x80o"));
  Object errors(&scope, runtime_->newStrFromCStr("surrogateescape"));
  Object index(&scope, runtime_->newInt(0));
  Object bytearray(&scope, runtime_->newByteArray());
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underUtf8Encode, str,
                                       errors, index, bytearray));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Bytes bytes(&scope, result.at(0));
  EXPECT_EQ(bytes.length(), 6);
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 6));
  EXPECT_TRUE(isBytesEqualsCStr(bytes, "hell\x80o"));
}

TEST_F(CodecsModuleTest,
       EncodeUTF8WithUnknownErrorHandlerReturnsSurrogateRange) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_->newStrFromCStr("hell\xed\xb2\x80\xed\xb2\x80o"));
  Object errors(&scope, runtime_->newStrFromCStr("unknown"));
  Object index(&scope, runtime_->newInt(0));
  Object bytearray(&scope, runtime_->newByteArray());
  Object result_obj(&scope, runBuiltin(UnderCodecsModule::underUtf8Encode, str,
                                       errors, index, bytearray));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 4));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 6));
}

TEST_F(CodecsModuleTest, EncodeUTF16WithWellFormedASCIIReturnsBytes) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_->newStrFromCStr("hi"));
  Object errors(&scope, runtime_->newStrFromCStr("unknown"));
  Object index(&scope, runtime_->newInt(0));
  Object bytearray(&scope, runtime_->newByteArray());
  Object byteorder(&scope, runtime_->newInt(0));
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

TEST_F(CodecsModuleTest, EncodeUTF16WithLargeIntByteorderRaisesOverflowError) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_->newStrFromCStr("hi"));
  Object errors(&scope, runtime_->newStrFromCStr("unknown"));
  Object index(&scope, runtime_->newInt(0));
  Object bytearray(&scope, runtime_->newByteArray());
  Object byteorder(&scope, runtime_->newInt(kMaxWord));
  EXPECT_TRUE(raisedWithStr(runBuiltin(UnderCodecsModule::underUtf16Encode, str,
                                       errors, index, bytearray, byteorder),
                            LayoutId::kOverflowError,
                            "Python int too large to convert to C int"));
}

TEST_F(CodecsModuleTest, EncodeUTF16WithIgnoreErrorHandlerReturnsStr) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_->newStrFromCStr("h\xed\xb2\x80i"));
  Object errors(&scope, runtime_->newStrFromCStr("ignore"));
  Object index(&scope, runtime_->newInt(0));
  Object bytearray(&scope, runtime_->newByteArray());
  Object byteorder(&scope, runtime_->newInt(0));
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

TEST_F(CodecsModuleTest, EncodeUTF16WithReplaceErrorHandlerReturnsStr) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_->newStrFromCStr("hi\xed\xb2\x80"));
  Object errors(&scope, runtime_->newStrFromCStr("replace"));
  Object index(&scope, runtime_->newInt(0));
  Object bytearray(&scope, runtime_->newByteArray());
  Object byteorder(&scope, runtime_->newInt(0));
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

TEST_F(CodecsModuleTest,
       EncodeUTF16WithSurroogateescapeErrorHandlerReturnsStr) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_->newStrFromCStr("h\xed\xb2\x80i"));
  Object errors(&scope, runtime_->newStrFromCStr("surrogateescape"));
  Object index(&scope, runtime_->newInt(0));
  Object bytearray(&scope, runtime_->newByteArray());
  Object byteorder(&scope, runtime_->newInt(0));
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

TEST_F(CodecsModuleTest, EncodeUTF16WithSupplementaryStringReturnsUTF16Bytes) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_->newStrFromCStr("h\U0001d1f0i"));
  Object errors(&scope, runtime_->newStrFromCStr("strict"));
  Object index(&scope, runtime_->newInt(0));
  Object bytearray(&scope, runtime_->newByteArray());
  Object byteorder(&scope, runtime_->newInt(0));
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

TEST_F(CodecsModuleTest,
       EncodeUTF16LeWithSupplementaryStringReturnsUTF16Bytes) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_->newStrFromCStr("h\U0001d1f0i"));
  Object errors(&scope, runtime_->newStrFromCStr("strict"));
  Object index(&scope, runtime_->newInt(0));
  Object bytearray(&scope, runtime_->newByteArray());
  Object byteorder(&scope, runtime_->newInt(-1));
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

TEST_F(CodecsModuleTest,
       EncodeUTF16BeWithSupplementaryStringReturnsUTF16Bytes) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_->newStrFromCStr("h\U0001d1f0i"));
  Object errors(&scope, runtime_->newStrFromCStr("strict"));
  Object index(&scope, runtime_->newInt(0));
  Object bytearray(&scope, runtime_->newByteArray());
  Object byteorder(&scope, runtime_->newInt(1));
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

TEST_F(CodecsModuleTest, EncodeUTF32WithWellFormedASCIIReturnsBytes) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_->newStrFromCStr("hi"));
  Object errors(&scope, runtime_->newStrFromCStr("unknown"));
  Object index(&scope, runtime_->newInt(0));
  Object bytearray(&scope, runtime_->newByteArray());
  Object byteorder(&scope, runtime_->newInt(0));
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

TEST_F(CodecsModuleTest, EncodeUTF32WithLargeIntByteorderRaisesOverflowError) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_->newStrFromCStr("hi"));
  Object errors(&scope, runtime_->newStrFromCStr("unknown"));
  Object index(&scope, runtime_->newInt(0));
  Object bytearray(&scope, runtime_->newByteArray());
  Object byteorder(&scope, runtime_->newInt(kMaxWord));
  EXPECT_TRUE(raisedWithStr(runBuiltin(UnderCodecsModule::underUtf32Encode, str,
                                       errors, index, bytearray, byteorder),
                            LayoutId::kOverflowError,
                            "Python int too large to convert to C int"));
}

TEST_F(CodecsModuleTest, EncodeUTF32WithIgnoreErrorHandlerReturnsStr) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_->newStrFromCStr("h\xed\xb2\x80i"));
  Object errors(&scope, runtime_->newStrFromCStr("ignore"));
  Object index(&scope, runtime_->newInt(0));
  Object bytearray(&scope, runtime_->newByteArray());
  Object byteorder(&scope, runtime_->newInt(0));
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

TEST_F(CodecsModuleTest, EncodeUTF32WithReplaceErrorHandlerReturnsStr) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_->newStrFromCStr("hi\xed\xb2\x80"));
  Object errors(&scope, runtime_->newStrFromCStr("replace"));
  Object index(&scope, runtime_->newInt(0));
  Object bytearray(&scope, runtime_->newByteArray());
  Object byteorder(&scope, runtime_->newInt(0));
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

TEST_F(CodecsModuleTest,
       EncodeUTF32WithSurroogateescapeErrorHandlerReturnsStr) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_->newStrFromCStr("h\xed\xb2\x80i"));
  Object errors(&scope, runtime_->newStrFromCStr("surrogateescape"));
  Object index(&scope, runtime_->newInt(0));
  Object bytearray(&scope, runtime_->newByteArray());
  Object byteorder(&scope, runtime_->newInt(0));
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

TEST_F(CodecsModuleTest, EncodeUTF32WithSupplementaryStringReturnsUTF32Bytes) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_->newStrFromCStr("h\U0001d1f0i"));
  Object errors(&scope, runtime_->newStrFromCStr("strict"));
  Object index(&scope, runtime_->newInt(0));
  Object bytearray(&scope, runtime_->newByteArray());
  Object byteorder(&scope, runtime_->newInt(0));
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

TEST_F(CodecsModuleTest,
       EncodeUTF32LeWithSupplementaryStringReturnsUTF32Bytes) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_->newStrFromCStr("h\U0001d1f0i"));
  Object errors(&scope, runtime_->newStrFromCStr("strict"));
  Object index(&scope, runtime_->newInt(0));
  Object bytearray(&scope, runtime_->newByteArray());
  Object byteorder(&scope, runtime_->newInt(-1));
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

TEST_F(CodecsModuleTest,
       EncodeUTF32BeWithSupplementaryStringReturnsUTF32Bytes) {
  HandleScope scope(thread_);
  Object str(&scope, runtime_->newStrFromCStr("h\U0001d1f0i"));
  Object errors(&scope, runtime_->newStrFromCStr("strict"));
  Object index(&scope, runtime_->newInt(0));
  Object bytearray(&scope, runtime_->newByteArray());
  Object byteorder(&scope, runtime_->newInt(1));
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

}  // namespace py
