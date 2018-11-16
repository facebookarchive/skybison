namespace python {

// Keep this in sync with ModuleInitializer in runtime/runtime.cpp
struct ModuleInitializer {
  const char* name;
  void* (*initfunc)();
};

extern "C" void* PyInit__stat();
extern "C" void* PyInit_errno();

ModuleInitializer kModuleInitializers[] = {
    {"_stat", PyInit__stat},
    {"errno", PyInit_errno},
    {nullptr, nullptr},
};

}  // namespace python
