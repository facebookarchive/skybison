namespace python {

extern "C" void* PyInit_errno();

struct ModuleInitializer {
  const char* name;
  void* (*initfunc)();
};

ModuleInitializer kModuleInitializers[] = {
    {"errno", PyInit_errno},
    {nullptr, nullptr}
};

} // namespace python
