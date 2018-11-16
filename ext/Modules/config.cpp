#include "cpython-types.h"

extern "C" PyObject* PyInit__stat();
extern "C" PyObject* PyInit_errno();

struct _inittab _PyImport_Inittab[] = {
    {"_stat", PyInit__stat},
    {"errno", PyInit_errno},
    {nullptr, nullptr},
};
