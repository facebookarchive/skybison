#include "Python.h"
#include "gtest/gtest.h"

#include "capi-fixture.h"
#include "capi-testing.h"

namespace py {
namespace testing {

using ConfigExtensionApiTest = ExtensionApi;

TEST_F(ConfigExtensionApiTest, ImportUnderAstReturnsModule) {
  PyObjectPtr module(PyImport_ImportModule("_ast"));
  ASSERT_NE(module, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyModule_Check(module));
}

TEST_F(ConfigExtensionApiTest, ImportUnderAstModuleMethods) {
  PyRun_SimpleString(R"(
import ast
c = "print('hello') "
b = ast.parse(c)
a = ast.dump(b)
)");
  PyObjectPtr a(mainModuleGet("a"));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(isUnicodeEqualsCStr(
      a,
      "Module(body=[Expr(value=Call(func=Name(id='print', ctx=Load()), "
      "args=[Str(s='hello')], keywords=[]))])"));
}

TEST_F(ConfigExtensionApiTest, ImportUnderBisectReturnsModule) {
  PyObjectPtr module(PyImport_ImportModule("_bisect"));
  ASSERT_NE(module, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyModule_Check(module));
}

TEST_F(ConfigExtensionApiTest, ImportUnderBlake2ReturnsModule) {
  PyObjectPtr module(PyImport_ImportModule("_blake2"));
  ASSERT_NE(module, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyModule_Check(module));
}

TEST_F(ConfigExtensionApiTest, ImportUnderBz2ReturnsModule) {
  PyObjectPtr module(PyImport_ImportModule("_bz2"));
  ASSERT_NE(module, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyModule_Check(module));
}

TEST_F(ConfigExtensionApiTest, ImportUnderCsvReturnsModule) {
  PyObjectPtr module(PyImport_ImportModule("_csv"));
  ASSERT_NE(module, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyModule_Check(module));
}

TEST_F(ConfigExtensionApiTest, ImportUnderHashlibReturnsModule) {
  PyObjectPtr module(PyImport_ImportModule("_hashlib"));
  ASSERT_NE(module, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyModule_Check(module));
}

TEST_F(ConfigExtensionApiTest, ImportUnderLocaleReturnsModule) {
  PyObjectPtr module(PyImport_ImportModule("_locale"));
  ASSERT_NE(module, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyModule_Check(module));
}

TEST_F(ConfigExtensionApiTest, ImportUnderLzmaReturnsModule) {
  PyObjectPtr module(PyImport_ImportModule("_lzma"));
  ASSERT_NE(module, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyModule_Check(module));
}

TEST_F(ConfigExtensionApiTest,
       ImportUnderMultiprocessingReadlineReturnsModule) {
  PyObjectPtr module(PyImport_ImportModule("_multiprocessing"));
  ASSERT_NE(module, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyModule_Check(module));
}

TEST_F(ConfigExtensionApiTest, ImportUnderPosixSubprocessReturnsModule) {
  PyObjectPtr module(PyImport_ImportModule("_posixsubprocess"));
  ASSERT_NE(module, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyModule_Check(module));
}

TEST_F(ConfigExtensionApiTest, ImportUnderRandomReturnsModule) {
  PyObjectPtr module(PyImport_ImportModule("_random"));
  ASSERT_NE(module, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyModule_Check(module));
}

TEST_F(ConfigExtensionApiTest, ImportUnderSocketReturnsModule) {
  PyObjectPtr module(PyImport_ImportModule("_socket"));
  ASSERT_NE(module, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyModule_Check(module));
}

TEST_F(ConfigExtensionApiTest, ImportUnderSha3ReturnsModule) {
  PyObjectPtr module(PyImport_ImportModule("_sha3"));
  ASSERT_NE(module, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyModule_Check(module));
}

TEST_F(ConfigExtensionApiTest, ImportUnderSqlite3ReturnsModule) {
  PyObjectPtr module(PyImport_ImportModule("_sqlite3"));
  ASSERT_NE(module, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyModule_Check(module));
}

TEST_F(ConfigExtensionApiTest, ImportUnderSreReturnsModule) {
  PyObjectPtr module(PyImport_ImportModule("_sre"));
  ASSERT_NE(module, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyModule_Check(module));
}

TEST_F(ConfigExtensionApiTest, ImportUnderSslReturnsModule) {
  PyObjectPtr module(PyImport_ImportModule("_ssl"));
  ASSERT_NE(module, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyModule_Check(module));
}

TEST_F(ConfigExtensionApiTest, ImportUnderSslModuleMethods) {
  PyRun_SimpleString(R"(
import ssl
ssl.create_default_context()
)");
  ASSERT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ConfigExtensionApiTest, ImportUnderStatReturnsModule) {
  PyObjectPtr module(PyImport_ImportModule("_stat"));
  ASSERT_NE(module, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyModule_Check(module));
}

TEST_F(ConfigExtensionApiTest, ImportUnderStructReturnsModule) {
  PyObjectPtr module(PyImport_ImportModule("_struct"));
  ASSERT_NE(module, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyModule_Check(module));
}

TEST_F(ConfigExtensionApiTest, ImportUnderSymtableReturnsModule) {
  PyObjectPtr module(PyImport_ImportModule("_symtable"));
  ASSERT_NE(module, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyModule_Check(module));
}

TEST_F(ConfigExtensionApiTest, ImportAtexitReturnsModule) {
  PyObjectPtr module(PyImport_ImportModule("atexit"));
  ASSERT_NE(module, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyModule_Check(module));
}

TEST_F(ConfigExtensionApiTest, ImportAudioopReturnsModule) {
  PyObjectPtr module(PyImport_ImportModule("audioop"));
  ASSERT_NE(module, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyModule_Check(module));
}

TEST_F(ConfigExtensionApiTest, ImportBinasciiReturnsModule) {
  PyObjectPtr module(PyImport_ImportModule("binascii"));
  ASSERT_NE(module, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyModule_Check(module));
}

TEST_F(ConfigExtensionApiTest, ImportErrnoReturnsModule) {
  PyObjectPtr module(PyImport_ImportModule("errno"));
  ASSERT_NE(module, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyModule_Check(module));
}

TEST_F(ConfigExtensionApiTest, ImportFcntlReturnsModule) {
  PyObjectPtr module(PyImport_ImportModule("fcntl"));
  ASSERT_NE(module, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyModule_Check(module));
}

TEST_F(ConfigExtensionApiTest, ImportGrpReturnsModule) {
  PyObjectPtr module(PyImport_ImportModule("grp"));
  ASSERT_NE(module, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyModule_Check(module));
}

TEST_F(ConfigExtensionApiTest, ImportMathReturnsModule) {
  PyObjectPtr module(PyImport_ImportModule("math"));
  ASSERT_NE(module, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyModule_Check(module));
}

TEST_F(ConfigExtensionApiTest, ImportMathModuleMethods) {
  PyRun_SimpleString(R"(
import math
a = math.sin(1)
b = math.floor(12.34)
)");
  PyObjectPtr a(mainModuleGet("a"));
  PyObjectPtr b(mainModuleGet("b"));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyFloat_Check(a), 1);
  EXPECT_EQ(PyLong_Check(b), 1);
}

TEST_F(ConfigExtensionApiTest, ImportPosixReturnsModule) {
  PyObjectPtr module(PyImport_ImportModule("posix"));
  ASSERT_NE(module, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyModule_Check(module));
}

TEST_F(ConfigExtensionApiTest, ImportPwdReturnsModule) {
  PyObjectPtr module(PyImport_ImportModule("pwd"));
  ASSERT_NE(module, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyModule_Check(module));
}

TEST_F(ConfigExtensionApiTest, ImportPyexpatReturnsModule) {
  PyObjectPtr module(PyImport_ImportModule("pyexpat"));
  ASSERT_NE(module, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyModule_Check(module));
}

TEST_F(ConfigExtensionApiTest, ImportReadlineReturnsModule) {
  PyObjectPtr module(PyImport_ImportModule("readline"));
  ASSERT_NE(module, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyModule_Check(module));
}

TEST_F(ConfigExtensionApiTest, ImportResourceReturnsModule) {
  PyObjectPtr module(PyImport_ImportModule("resource"));
  ASSERT_NE(module, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyModule_Check(module));
}

TEST_F(ConfigExtensionApiTest, ImportSelectReturnsModule) {
  PyObjectPtr module(PyImport_ImportModule("select"));
  ASSERT_NE(module, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyModule_Check(module));
}

TEST_F(ConfigExtensionApiTest, ImportSyslogReturnsModule) {
  PyObjectPtr module(PyImport_ImportModule("syslog"));
  ASSERT_NE(module, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyModule_Check(module));
}

TEST_F(ConfigExtensionApiTest, ImportTermiosReturnsModule) {
  PyObjectPtr module(PyImport_ImportModule("termios"));
  ASSERT_NE(module, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyModule_Check(module));
}

TEST_F(ConfigExtensionApiTest, ImportTimeReturnsModule) {
  PyObjectPtr module(PyImport_ImportModule("time"));
  ASSERT_NE(module, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyModule_Check(module));
}

TEST_F(ConfigExtensionApiTest, TimeModuleMethods) {
  PyRun_SimpleString(R"(
import time
a = time.time()
b = time.perf_counter()
)");
  PyObjectPtr a(mainModuleGet("a"));
  PyObjectPtr b(mainModuleGet("b"));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyFloat_Check(a), 1);
  EXPECT_EQ(PyFloat_Check(b), 1);
}

TEST_F(ConfigExtensionApiTest, ImportZlibReturnsModule) {
  PyObjectPtr module(PyImport_ImportModule("zlib"));
  ASSERT_NE(module, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyModule_Check(module));
}

TEST_F(ConfigExtensionApiTest, ZlibModuleMethods) {
  PyRun_SimpleString(R"(
import zlib
input = b'Hello world'
z = zlib.compress(input)
result = zlib.decompress(z)
)");
  PyObjectPtr result(mainModuleGet("result"));
  PyObjectPtr input(mainModuleGet("input"));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_NE(result, input);
  EXPECT_STREQ(PyBytes_AsString(result), PyBytes_AsString(input));
}

}  // namespace testing
}  // namespace py
