#include "cpython-func.h"
#include "cpython-types.h"

extern "C" PyObject* PyInit__capsule();
extern "C" PyObject* PyInit__myreadline();
extern "C" PyObject* PyInit__stat();
extern "C" PyObject* PyInit__stentry();
extern "C" PyObject* PyInit_atexit();
extern "C" PyObject* PyInit_errno();
extern "C" PyObject* PyInit_posix();

// _empty module to test loading from init tab
PyObject* PyInit__empty() {
  static PyModuleDef def;
  def = {};
  def.m_name = "_empty";
  return PyModule_Create(&def);
}

// clang-format off
struct _inittab _PyImport_Inittab[] = {
    {"_capsule", PyInit__capsule},
    {"_empty", PyInit__empty},
    {"_myreadline", PyInit__myreadline},
    {"_stat", PyInit__stat},
    {"_stentry", PyInit__stentry},
    {"atexit", PyInit_atexit},
    {"errno", PyInit_errno},
    {"posix", PyInit_posix},
    {nullptr, nullptr},
};
// clang-format on
