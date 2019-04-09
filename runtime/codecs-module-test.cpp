#include "gtest/gtest.h"

#include "bytearray-builtins.h"
#include "codecs-module.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(CodecsModuleTest, RegisterErrorWithNonStringFirstReturnsError) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
import _codecs
a = _codecs.register_error([], [])
)");
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_TRUE(
      raisedWithStr(*a, LayoutId::kTypeError,
                    "register_error() argument 1 must be str, not list"));
}

TEST(CodecsModuleTest, RegisterErrorWithNonCallableSecondReturnsError) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
import _codecs
a = _codecs.register_error("", [])
)");
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_TRUE(
      raisedWithStr(*a, LayoutId::kTypeError, "handler must be callable"));
}

TEST(CodecsModuleTest, DecodeASCIIWithNonBytesFirstArgumentReturnsError) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
import _codecs
a = _codecs.ascii_decode([])
)");
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_TRUE(raisedWithStr(*a, LayoutId::kTypeError,
                            "a bytes-like object is required, not 'list'"));
}

TEST(CodecsModuleTest, DecodeASCIIWithNonStringSecondArgumentReturnsError) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
import _codecs
a = _codecs.ascii_decode(b"", [])
)");
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_TRUE(raisedWithStr(
      *a, LayoutId::kTypeError,
      "ascii_decode() argument 2 must be str or None, not 'list'"));
}

TEST(CodecsModuleTest, DecodeASCIIWithZeroLengthReturnsEmptyString) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
import _codecs
a = _codecs.ascii_decode(b"")
)");
  HandleScope scope;
  Object result_obj(&scope, moduleAt(&runtime, "__main__", "a"));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Str str(&scope, result.at(0));
  EXPECT_EQ(str.length(), 0);
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 0));
  EXPECT_TRUE(str.equalsCStr(""));
}

TEST(CodecsModuleTest, DecodeASCIIWithWellFormedASCIIReturnsString) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
import _codecs
a = _codecs.ascii_decode(b"hello")
)");
  HandleScope scope;
  Object result_obj(&scope, moduleAt(&runtime, "__main__", "a"));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Str str(&scope, result.at(0));
  EXPECT_EQ(str.length(), 5);
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 5));
  EXPECT_TRUE(str.equalsCStr("hello"));
}

TEST(CodecsModuleTest, DecodeASCIIWithIgnoreErrorHandlerReturnsStr) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
import _codecs
a = _codecs.ascii_decode(b"hell\x80o", "ignore")
)");
  HandleScope scope;
  Object result_obj(&scope, moduleAt(&runtime, "__main__", "a"));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Str str(&scope, result.at(0));
  EXPECT_EQ(str.length(), 5);
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 6));
  EXPECT_TRUE(str.equalsCStr("hello"));
}

TEST(CodecsModuleTest, DecodeASCIIWithReplaceErrorHandlerReturnsStr) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
import _codecs
a = _codecs.ascii_decode(b"hell\x80o", "replace")
)");
  HandleScope scope;
  Object result_obj(&scope, moduleAt(&runtime, "__main__", "a"));
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
  runFromCStr(&runtime, R"(
import _codecs
a = _codecs.ascii_decode(b"hell\x80o", "surrogateescape")
)");
  HandleScope scope;
  Object result_obj(&scope, moduleAt(&runtime, "__main__", "a"));
  ASSERT_TRUE(result_obj.isTuple());

  Tuple result(&scope, *result_obj);
  Str str(&scope, result.at(0));
  EXPECT_EQ(str.length(), 8);
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 6));
  word placeholder;
  EXPECT_EQ(str.codePointAt(4, &placeholder), 0xdc80);
}

TEST(CodecsModuleTest, LookupErrorWithNonStringReturnsError) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
import _codecs
a = _codecs.lookup_error([])
)");
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_TRUE(raisedWithStr(*a, LayoutId::kTypeError,
                            "lookup_error() argument must be str, not list"));
}

TEST(CodecsModuleTest, LookupErrorWithUnkownNameReturnsError) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
import _codecs
a = _codecs.lookup_error('unknown')
)");
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_TRUE(raisedWithStr(*a, LayoutId::kLookupError,
                            "unknown error handler name 'unknown'"));
}

TEST(CodecsModuleTest, LookupErrorWithRegisteredErrorReturnsFunction) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
import _codecs
def test_errors():
    pass
_codecs.register_error("test", test_errors)
a = _codecs.lookup_error("test")
)");
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  ASSERT_TRUE(a.isFunction());

  Function error(&scope, *a);
  EXPECT_TRUE(isStrEqualsCStr(error.name(), "test_errors"));
}

TEST(CodecsModuleTest, LookupErrorWithIgnoreReturnsFunction) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
import _codecs
a = _codecs.lookup_error('ignore')
)");
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  ASSERT_TRUE(a.isFunction());

  Function error(&scope, *a);
  EXPECT_TRUE(isStrEqualsCStr(error.name(), "ignore_errors"));
}

TEST(CodecsModuleTest, LookupErrorWithStrictReturnsFunction) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
import _codecs
a = _codecs.lookup_error('strict')
)");
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  ASSERT_TRUE(a.isFunction());

  Function error(&scope, *a);
  EXPECT_TRUE(isStrEqualsCStr(error.name(), "strict_errors"));
}

TEST(CodecsModuleTest, CodecsIsImportable) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, "import codecs").isError());
}

TEST(CodecsModuleTest, CallDecodeErrorWithStrictCallsFunction) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
import _codecs
a = _codecs._call_decode_errorhandler('strict', b'', bytearray(b''), 'testing', '', 0, 0)
)");
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  ASSERT_TRUE(a.isError());

  Object exc(&scope, Thread::current()->pendingExceptionValue());
  ASSERT_TRUE(exc.isUnicodeDecodeError());
  UnicodeDecodeError decode_exc(&scope, *exc);
  EXPECT_TRUE(isStrEqualsCStr(decode_exc.reason(), "testing"));
}

TEST(CodecsModuleTest, CallDecodeErrorWithIgnoreCallsFunction) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
import _codecs
a = _codecs._call_decode_errorhandler('ignore', b'hello', bytearray(b''), 'testing', '', 1, 2)
)");
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  ASSERT_TRUE(a.isTuple());

  Tuple result(&scope, *a);
  Object result_bytes(&scope, result.at(0));
  EXPECT_TRUE(isBytesEqualsCStr(result_bytes, "hello"));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 0));
}

TEST(CodecsModuleTest, CallDecodeErrorWithNonTupleReturnReturnsError) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
import _codecs
def test_pass(err):
  return 1
_codecs.register_error("test", test_pass)
a = _codecs._call_decode_errorhandler('test', b'hello', bytearray(b''), 'testing', '', 1, 2)
)");
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_TRUE(
      raisedWithStr(*a, LayoutId::kTypeError,
                    "decoding error handler must return (str, int) tuple"));
}

TEST(CodecsModuleTest, CallDecodeErrorWithSmallTupleReturnReturnsError) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
import _codecs
def test_pass(err):
  return (1, )
_codecs.register_error("test", test_pass)
a = _codecs._call_decode_errorhandler('test', b'hello', bytearray(b''), 'testing', '', 1, 2)
)");
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_TRUE(
      raisedWithStr(*a, LayoutId::kTypeError,
                    "decoding error handler must return (str, int) tuple"));
}

TEST(CodecsModuleTest, CallDecodeErrorWithIntFirstTupleReturnReturnsError) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
import _codecs
def test_pass(err):
  return (1, 1)
_codecs.register_error("test", test_pass)
a = _codecs._call_decode_errorhandler('test', b'hello', bytearray(b''), 'testing', '', 1, 2)
)");
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_TRUE(
      raisedWithStr(*a, LayoutId::kTypeError,
                    "decoding error handler must return (str, int) tuple"));
}

TEST(CodecsModuleTest, CallDecodeErrorWithStringSecondTupleReturnReturnsError) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
import _codecs
def test_pass(err):
  return ("", "")
_codecs.register_error("test", test_pass)
a = _codecs._call_decode_errorhandler('test', b'hello', bytearray(b''), 'testing', '', 1, 2)
)");
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_TRUE(
      raisedWithStr(*a, LayoutId::kTypeError,
                    "decoding error handler must return (str, int) tuple"));
}

TEST(CodecsModuleTest, CallDecodeErrorWithNonBytesChangedInputReturnsError) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
import _codecs
def test_pass(err):
  err.object = 1
  return("", 1)
_codecs.register_error("test", test_pass)
a = _codecs._call_decode_errorhandler('test', b'hello', bytearray(b''), 'testing', '', 1, 2)
)");
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_TRUE(raisedWithStr(*a, LayoutId::kTypeError,
                            "exception attribute object must be bytes"));
}

TEST(CodecsModuleTest, CallDecodeErrorWithImproperIndexReturnReturnsError) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
import _codecs
def test_pass(err):
  return("", 10)
_codecs.register_error("test", test_pass)
a = _codecs._call_decode_errorhandler('test', b'hello', bytearray(b''), 'testing', '', 1, 2)
)");
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_TRUE(raisedWithStr(*a, LayoutId::kIndexError,
                            "position 10 from error handler out of bounds"));
}

TEST(CodecsModuleTest,
     CallDecodeErrorWithNegativeIndexReturnReturnsProperIndex) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
import _codecs
def test_pass(err):
  return("", -1)
_codecs.register_error("test", test_pass)
a = _codecs._call_decode_errorhandler('test', b'hello', bytearray(b''), 'testing', '', 1, 2)
)");
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  ASSERT_TRUE(a.isTuple());

  Tuple result(&scope, *a);
  Object result_bytes(&scope, result.at(0));
  EXPECT_TRUE(isBytesEqualsCStr(result_bytes, "hello"));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 4));
}

TEST(CodecsModuleTest, CallDecodeAppendsStringToOutput) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
import _codecs
def test_pass(err):
  return("test", 1)
_codecs.register_error("test", test_pass)
a = bytearray(b'')
_codecs._call_decode_errorhandler('test', b'hello', a, 'testing', '', 1, 2)
)");
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  ASSERT_TRUE(a.isByteArray());

  ByteArray result(&scope, *a);
  Object result_bytes(&scope,
                      byteArrayAsBytes(Thread::current(), &runtime, result));
  EXPECT_TRUE(isBytesEqualsCStr(result_bytes, "test"));
}

TEST(CodecsModuleTest, RegisterWithNonCallableFails) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
import _codecs
_codecs.register("test")
)"),
                            LayoutId::kTypeError, "argument must be callable"));
}

TEST(CodecsModuleTest, LookupCodecThatDoesntReturnTupleRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
import _codecs
_codecs.register(lambda x: 1 if x == "test" else None)
test = _codecs.lookup("test")
)"),
                            LayoutId::kTypeError,
                            "codec search functions must return 4-tuples"));
}

TEST(CodecsModuleTest, LookupCodecThatDoesntReturnAFourTupleRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
import _codecs
_codecs.register(lambda x: (1, 2) if x == "test" else None)
test = _codecs.lookup("test")
)"),
                            LayoutId::kTypeError,
                            "codec search functions must return 4-tuples"));
}

TEST(CodecsModuleTest, LookupUnknownCodecRaisesLookupError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
import _codecs
test = _codecs.lookup("gibberish")
)"),
                            LayoutId::kLookupError,
                            "unknown encoding: gibberish"));
}

TEST(CodecsModuleTest, LookupReturnsFirstRegisteredCodec) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
import _codecs
_codecs.register(lambda x: (1, 2, 3, 4) if x == "test" else None)
_codecs.register(lambda x: (5, 6, 7, 8) if x == "test" else None)
test = _codecs.lookup("test")
)");
  HandleScope scope;
  Object test(&scope, moduleAt(&runtime, "__main__", "test"));
  ASSERT_TRUE(test.isTuple());

  Tuple result(&scope, *test);
  ASSERT_EQ(result.length(), 4);
  EXPECT_TRUE(isIntEqualsWord(result.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(result.at(1), 2));
  EXPECT_TRUE(isIntEqualsWord(result.at(2), 3));
  EXPECT_TRUE(isIntEqualsWord(result.at(3), 4));
}

TEST(CodecsModuleTest, LookupProperlyCachesEncodings) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
import _codecs
def wrapper():
  a = []
  def test(exc):
    a.append(1)
    return (len(a), 0, 0, 0)
  return test
_codecs.register(wrapper())
first_abc = _codecs.lookup("abc")
first_bcd = _codecs.lookup("bcd")
second_abc = _codecs.lookup("abc")
)");
  HandleScope scope;
  Object first_abc(&scope, moduleAt(&runtime, "__main__", "first_abc"));
  Object first_bcd(&scope, moduleAt(&runtime, "__main__", "first_bcd"));
  Object second_abc(&scope, moduleAt(&runtime, "__main__", "second_abc"));
  EXPECT_EQ(*first_abc, *second_abc);
  EXPECT_NE(*first_abc, *first_bcd);
}

TEST(CodecsModuleTest, LookupProperlyNormalizedEncodingString) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
import _codecs
_codecs.register(lambda x: (0, 0, 0, 0) if x == "te-st" else None)
space = _codecs.lookup("te st")
uppercase = _codecs.lookup("TE-ST")
)");
  HandleScope scope;
  Object space(&scope, moduleAt(&runtime, "__main__", "space"));
  Object uppercase(&scope, moduleAt(&runtime, "__main__", "uppercase"));
  EXPECT_TRUE(space.isTuple());
  EXPECT_TRUE(uppercase.isTuple());
}

}  // namespace python
