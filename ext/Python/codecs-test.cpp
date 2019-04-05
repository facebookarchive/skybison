#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

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
  PyObject* inc_dec(_PyCodecInfo_GetIncrementalDecoder(test, "ignore"));
  ASSERT_EQ(moduleSet("__main__", "inc_dec", inc_dec), 0);

  PyRun_SimpleString(R"(
result = inc_dec.decode(b'hel\x80lo')
  )");

  PyObjectPtr result(moduleGet("__main__", "result"));
  EXPECT_TRUE(isUnicodeEqualsCStr(result, "hello"));
}

}  // namespace python
