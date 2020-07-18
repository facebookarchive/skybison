#include "capi-fixture.h"

#include <clocale>

#include "Python.h"

namespace py {
namespace testing {

const char* argv0;

void resetPythonEnv() {
  // TODO(T70174461): Replace with config API when we upgraded to 3.8
  Py_BytesWarningFlag = 0;
  Py_DebugFlag = 0;
  Py_DontWriteBytecodeFlag = 0;
  Py_FrozenFlag = 0;
  Py_HashRandomizationFlag = 0;
  Py_IgnoreEnvironmentFlag = 0;
  Py_InspectFlag = 0;
  Py_InteractiveFlag = 0;
  Py_IsolatedFlag = 0;
  Py_NoSiteFlag = 1;
  Py_NoUserSiteDirectory = 0;
  Py_OptimizeFlag = 0;
  Py_QuietFlag = 0;
  Py_UTF8Mode = 1;
  Py_UnbufferedStdioFlag = 0;
  Py_VerboseFlag = 0;
  Py_SetPath(nullptr);
  std::setlocale(LC_CTYPE, "");
  wchar_t* argv0_w = Py_DecodeLocale(argv0, nullptr);
  Py_SetProgramName(argv0_w);
  PyMem_RawFree(argv0_w);
  std::setlocale(LC_CTYPE, "en_US.UTF-8");
}

void ExtensionApi::SetUp() {
  resetPythonEnv();
  Py_Initialize();
}

void ExtensionApi::TearDown() {
  Py_FinalizeEx();
  std::setlocale(LC_CTYPE, "C");
}

}  // namespace testing
}  // namespace py
