#include "cpython-func.h"
#include "cpython-types.h"

extern "C" PyObject* PyInit__stat();
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
    {"_empty", PyInit__empty},
    {"_stat", PyInit__stat},
    {"errno", PyInit_errno},
    {"posix", PyInit_posix},
    {nullptr, nullptr},
};
// clang-format on
