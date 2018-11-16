namespace python {

struct ModuleInitializer {
  const char* name;
  void* (*initfunc)();
};

struct ExtensionTypeInitializer {
  void (*initfunc)();
};

extern "C" void* PyInit_errno();

ModuleInitializer kModuleInitializers[] = {
    {"errno", PyInit_errno},
    {nullptr, nullptr},
};

void PyType_Type_Init();
void PyBaseObject_Type_Init();
void PyBool_Type_Init();
void PyDict_Type_Init();
void PyFloat_Type_Init();
void PyList_Type_Init();
void PyLong_Type_Init();
void PyMemberDescr_Type_Init();
void PyModule_Type_Init();
void PyNone_Type_Init();
void PyTuple_Type_Init();
void PyUnicode_Type_Init();

// Order must match python::ExtensionTypes
ExtensionTypeInitializer kExtensionTypeInitializers[] = {
    {PyType_Type_Init},
    {PyBaseObject_Type_Init},
    {PyBool_Type_Init},
    {PyDict_Type_Init},
    {PyFloat_Type_Init},
    {PyList_Type_Init},
    {PyLong_Type_Init},
    {PyMemberDescr_Type_Init},
    {PyModule_Type_Init},
    {PyNone_Type_Init},
    {PyTuple_Type_Init},
    {PyUnicode_Type_Init},
    {nullptr},
};

} // namespace python
