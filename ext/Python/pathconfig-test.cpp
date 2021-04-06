#include "Python.h"

#include "capi-fixture.h"
#include "capi-testing.h"

namespace py {
namespace testing {

using PathconfigExtensionApiTest = ExtensionApi;

// Defaults to `/usr/local` only if python is run from a build dir
// TODO(T67620993): Compare against built-in defaults
// TODO(T67625250): Make sure we test against different values depending on
// whether Pyro is being run from a build directory or not.
TEST_F(PathconfigExtensionApiTest, GetPrefixReturnsUsrLocalPyro) {
  EXPECT_STREQ(Py_GetPrefix(), L"/usr/local");
  EXPECT_STREQ(Py_GetExecPrefix(), L"/usr/local");
}

TEST_F(PathconfigExtensionApiTest, SetPathClearsPrefixAndExecPrefix) {
  Py_SetPath(L"test");
  EXPECT_STREQ(Py_GetPrefix(), L"");
  EXPECT_STREQ(Py_GetExecPrefix(), L"");
  EXPECT_STREQ(Py_GetPath(), L"test");
}

TEST(PathconfigExtensionApiTestNoFixture, PySetPathSetsSysPath) {
  // Because we can't rely on os.h (due to tests being
  // shared between CPython and Pyro, we can't link the runtime), we use
  // the default sys.path's element as the canonical location of paths.
  resetPythonEnv();
  Py_Initialize();
  std::wstring old_path(Py_GetPath());
  Py_FinalizeEx();

  int length = old_path.length() + 30;
  std::unique_ptr<wchar_t[]> sys_path(new wchar_t[length]);
  std::swprintf(sys_path.get(), length, L"%ls:/usr/local/setbyapi",
                old_path.c_str());

  resetPythonEnv();
  Py_SetPath(sys_path.get());
  Py_Initialize();

  {
    PyImport_ImportModule("sys");
    ASSERT_EQ(PyErr_Occurred(), nullptr);
    PyObjectPtr path(moduleGet("sys", "path"));
    ASSERT_NE(path, nullptr);
    PyObjectPtr path_last(PySequence_GetItem(path, -1));
    ASSERT_NE(path_last, nullptr);
    const char* cstring = PyUnicode_AsUTF8AndSize(path_last, nullptr);
    EXPECT_STREQ(cstring, "/usr/local/setbyapi");
  }

  PyErr_Clear();
  Py_FinalizeEx();
  std::setlocale(LC_CTYPE, "C");
}

}  // namespace testing
}  // namespace py
