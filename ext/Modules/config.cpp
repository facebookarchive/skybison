
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

void initialize_PyBaseObject_Type();
void initialize_PyType_Type();

ExtensionTypeInitializer kExtensionTypeInitializers[] = {
    {initialize_PyBaseObject_Type},
    {initialize_PyType_Type},
    {nullptr},
};

} // namespace python
