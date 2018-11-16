void initialize_PyBaseObject_Type();
void initialize_PyType_Type();

namespace python {

extern "C" void* PyInit_errno();

struct ModuleInitializer {
  const char* name;
  void* (*initfunc)();
};

struct ExtensionTypeInitializer {
  void (*initfunc)();
};

ModuleInitializer kModuleInitializers[] = {
    {"errno", PyInit_errno},
    {nullptr, nullptr},
};

ExtensionTypeInitializer kExtensionTypeInitializers[] = {
    {initialize_PyBaseObject_Type},
    {initialize_PyType_Type},
    {nullptr},
};

} // namespace python
