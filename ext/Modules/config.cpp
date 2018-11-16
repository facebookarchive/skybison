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

// Order must match python::ExtensionTypes
ExtensionTypeInitializer kExtensionTypeInitializers[] = {
    {PyType_Type_Init},
    {PyBaseObject_Type_Init},
    {PyBool_Type_Init},
    {nullptr},
};

} // namespace python
