#include "Python.h"
#include "gtest/gtest.h"

#include "capi-fixture.h"
#include "capi-testing.h"

namespace py {

using namespace testing;

using CodecsExtensionApiTest = ExtensionApi;

TEST_F(CodecsExtensionApiTest, GetIncrementalDecoderInstantiatesDecoder) {
  PyRun_SimpleString(R"(
import codecs
class Inc(codecs.IncrementalDecoder):
    def decode(self, input, final=False):
        return codecs.ascii_decode(input, self.errors)[0]

test = codecs.CodecInfo('test', None, None, incrementaldecoder=Inc)
  )");

  PyObjectPtr test(moduleGet("__main__", "test"));
  PyObjectPtr inc_dec(_PyCodecInfo_GetIncrementalDecoder(test, "ignore"));
  ASSERT_EQ(moduleSet("__main__", "inc_dec", inc_dec), 0);

  PyRun_SimpleString(R"(
result = inc_dec.decode(b'hel\x80lo')
  )");

  PyObjectPtr result(moduleGet("__main__", "result"));
  EXPECT_TRUE(isUnicodeEqualsCStr(result, "hello"));
}

TEST_F(CodecsExtensionApiTest,
       LookupTextEncodingWithUnknownEncodingRaisesLookupError) {
  PyObjectPtr error(_PyCodec_LookupTextEncoding("gibberish", "alt"));
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_LookupError));
  EXPECT_EQ(error, nullptr);
}

TEST_F(CodecsExtensionApiTest, LookupTextEncodingWithTupleReturnsTuple) {
  PyRun_SimpleString(R"(
import _codecs
_codecs.register(lambda x: (1, 2, 3, 4))
)");
  PyObjectPtr codec(_PyCodec_LookupTextEncoding("any", "alt"));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyTuple_CheckExact(codec));
  EXPECT_EQ(PyLong_AsLong(PyTuple_GetItem(codec, 0)), 1);
  EXPECT_EQ(PyLong_AsLong(PyTuple_GetItem(codec, 1)), 2);
  EXPECT_EQ(PyLong_AsLong(PyTuple_GetItem(codec, 2)), 3);
  EXPECT_EQ(PyLong_AsLong(PyTuple_GetItem(codec, 3)), 4);
}

TEST_F(CodecsExtensionApiTest,
       LookupTextEncodingWithTupleSubclassReturnsTuple) {
  PyRun_SimpleString(R"(
import _codecs
class TupSub(tuple): pass
_codecs.register(lambda x: TupSub((1, 2, 3, 4)))
)");
  PyObjectPtr codec(_PyCodec_LookupTextEncoding("any", "alt"));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_FALSE(PyTuple_CheckExact(codec));
  ASSERT_TRUE(PyTuple_Check(codec));
  EXPECT_EQ(PyLong_AsLong(PyTuple_GetItem(codec, 0)), 1);
  EXPECT_EQ(PyLong_AsLong(PyTuple_GetItem(codec, 1)), 2);
  EXPECT_EQ(PyLong_AsLong(PyTuple_GetItem(codec, 2)), 3);
  EXPECT_EQ(PyLong_AsLong(PyTuple_GetItem(codec, 3)), 4);
}

TEST_F(CodecsExtensionApiTest,
       LookupTextEncodingWithOverwrittenEncodingFieldRaisesLookupError) {
  PyRun_SimpleString(R"(
import _codecs
class TupSub(tuple):
  _is_text_encoding = False
_codecs.register(lambda x: TupSub((1, 2, 3, 4)))
)");
  PyObjectPtr error(_PyCodec_LookupTextEncoding("any", "alt"));
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_LookupError));
  EXPECT_EQ(error, nullptr);
}

TEST_F(CodecsExtensionApiTest, StrictErrorsWithNonExceptionRaisesTypeError) {
  PyObjectPtr non_exc(PyLong_FromLong(0));
  EXPECT_EQ(PyCodec_StrictErrors(non_exc), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(CodecsExtensionApiTest, StrictErrorsWithExceptionRaisesIt) {
  PyObjectPtr exc(PyUnicodeDecodeError_Create("enc", "obj", 3, 1, 2, "rea"));
  EXPECT_EQ(PyCodec_StrictErrors(exc), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_UnicodeDecodeError));
}

}  // namespace py
