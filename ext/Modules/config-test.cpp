#include "Python.h"
#include "gtest/gtest.h"

#include "capi-fixture.h"
#include "capi-testing.h"

namespace py {

using namespace testing;

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
  PyObjectPtr a(moduleGet("__main__", "a"));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(isUnicodeEqualsCStr(
      a,
      "Module(body=[Expr(value=Call(func=Name(id='print', ctx=Load()), "
      "args=[Str(s='hello')], keywords=[]))])"));
}

TEST_F(ConfigExtensionApiTest, ImportUnderBlake2ReturnsModule) {
  PyObjectPtr module(PyImport_ImportModule("_blake2"));
  ASSERT_NE(module, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyModule_Check(module));
}

TEST_F(ConfigExtensionApiTest, ImportUnderCapsuleReturnsModule) {
  PyObjectPtr module(PyImport_ImportModule("_capsule"));
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

TEST_F(ConfigExtensionApiTest, ImportUnderMyReadlineReturnsModule) {
  PyObjectPtr module(PyImport_ImportModule("_myreadline"));
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

TEST_F(ConfigExtensionApiTest, ImportUnderStentryReturnsModule) {
  PyObjectPtr module(PyImport_ImportModule("_stentry"));
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

TEST_F(ConfigExtensionApiTest, ImportAtexitReturnsModule) {
  PyObjectPtr module(PyImport_ImportModule("atexit"));
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
  PyObjectPtr a(moduleGet("__main__", "a"));
  PyObjectPtr b(moduleGet("__main__", "b"));
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

TEST_F(ConfigExtensionApiTest, ImportUnicodedataReturnsModule) {
  PyObjectPtr module(PyImport_ImportModule("unicodedata"));
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
  PyObjectPtr a(moduleGet("__main__", "a"));
  PyObjectPtr b(moduleGet("__main__", "b"));
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

}  // namespace py
