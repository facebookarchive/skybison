#include "gtest/gtest.h"

#include "Python.h"
#include "runtime/runtime.h"
#include "runtime/test-utils.h"

namespace python {

TEST(PyTypeObject, EmptyType) {
  Runtime runtime;
  HandleScope scope;
  Handle<Dictionary> extensions_dict(&scope, runtime.extensionTypes());

  // Create a simple PyTypeObject
  PyTypeObject EmptyType{PyObject_HEAD_INIT(nullptr)};
  EmptyType.tp_name = "Empty";
  EmptyType.tp_flags = Py_TPFLAGS_DEFAULT;

  // EmptyType is not initialized yet
  EXPECT_FALSE(PyType_GetFlags(&EmptyType) & Py_TPFLAGS_READY);

  // Run PyType_Ready
  EXPECT_EQ(0, extensions_dict->numItems());
  EXPECT_EQ(0, PyType_Ready(&EmptyType));
  EXPECT_EQ(1, extensions_dict->numItems());

  // Expect PyTypeObject to contain the correct flags
  EXPECT_TRUE(PyType_GetFlags(&EmptyType) & Py_TPFLAGS_DEFAULT);
  EXPECT_TRUE(PyType_GetFlags(&EmptyType) & Py_TPFLAGS_READY);
}

TEST(PyTypeObject, EmptyTypeGetRuntimeClass) {
  Runtime runtime;
  HandleScope scope;
  Handle<Dictionary> extensions_dict(&scope, runtime.extensionTypes());

  // Create a simple PyTypeObject
  PyTypeObject EmptyType{PyObject_HEAD_INIT(nullptr)};
  EmptyType.tp_name = "Empty";
  EmptyType.tp_flags = Py_TPFLAGS_DEFAULT;
  PyType_Ready(&EmptyType);

  // Obtain EmptyType Class object from the runtime
  Handle<Object> tp_name(
      &scope, runtime.newStringFromCString(EmptyType.tp_name));
  Handle<Object> type_class_obj(
      &scope, runtime.dictionaryAt(extensions_dict, tp_name));
  EXPECT_TRUE(type_class_obj->isClass());

  // Confirm the class name
  Handle<Class> type_class(&scope, *type_class_obj);
  Handle<String> type_class_name(&scope, type_class->name());
  EXPECT_TRUE(type_class_name->equalsCString(EmptyType.tp_name));

  // Confirm the PyTypeObject pointer
  Handle<Integer> pytype_addr_obj(&scope, type_class->extensionType());
  EXPECT_EQ(&EmptyType, pytype_addr_obj->asCPointer());
}

TEST(PyTypeObject, EmptyTypeInstantiateObject) {
  Runtime runtime;
  HandleScope scope;
  Handle<Dictionary> extensions_dict(&scope, runtime.extensionTypes());

  // Create a simple PyTypeObject
  PyTypeObject EmptyType{PyObject_HEAD_INIT(nullptr)};
  EmptyType.tp_name = "Empty";
  EmptyType.tp_flags = Py_TPFLAGS_DEFAULT;
  PyType_Ready(&EmptyType);
  Handle<Object> tp_name(
      &scope, runtime.newStringFromCString(EmptyType.tp_name));
  Handle<Class> type_class(
      &scope, runtime.dictionaryAt(extensions_dict, tp_name));

  // Instantiate a class object
  Handle<Layout> layout(&scope, type_class->instanceLayout());
  Handle<Object> type_instance(&scope, runtime.newInstance(layout));
  EXPECT_TRUE(type_instance->isInstance());
}

typedef struct {
  PyObject_HEAD;
  int value;
} CustomObject;

static PyObject*
Custom_new(PyTypeObject* type, PyObject* args, PyObject* kwds) {
  // TODO(T29877036): malloc should be handled by tp_alloc
  CustomObject* self = (CustomObject*)std::malloc(sizeof(CustomObject));
  return (PyObject*)self;
}

static int Custom_init(CustomObject* self, PyObject* args, PyObject* kwds) {
  self->value = 30;
  return 0;
}

TEST(PyTypeObject, InitializeCustomTypeypeInstance) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();

  // Instantiate module
  Handle<Object> module_name(&scope, runtime.newStringFromCString("custom"));
  Handle<Module> module(&scope, runtime.newModule(module_name));
  Handle<Dictionary> module_dict(&scope, module->dictionary());
  runtime.addModule(module);

  // Instantiate Class
  PyTypeObject custom_type{PyObject_HEAD_INIT(nullptr)};
  custom_type.tp_name = "custom.Custom";
  custom_type.tp_flags = Py_TPFLAGS_DEFAULT;
  custom_type.tp_new = Custom_new;
  custom_type.tp_init = (initproc)Custom_init;
  PyType_Ready(&custom_type);

  // Add class to the runtime
  Handle<Object> tp_name(
      &scope, runtime.newStringFromCString(custom_type.tp_name));
  Handle<Dictionary> ext_dict(&scope, runtime.extensionTypes());
  Handle<Object> type_class(&scope, runtime.dictionaryAt(ext_dict, tp_name));
  Handle<Object> ob_name(&scope, runtime.newStringFromCString("Custom"));
  runtime.dictionaryAtPutInValueCell(module_dict, ob_name, type_class);

  runtime.runFromCString(R"(
import custom
instance = custom.Custom()
)");

  // Verify the initialized value
  Handle<Module> main(&scope, testing::findModule(&runtime, "__main__"));
  Handle<Object> instance_obj(
      &scope, testing::findInModule(&runtime, main, "instance"));
  ASSERT_TRUE(instance_obj->isInstance());

  // Get instance value
  Handle<HeapObject> instance(&scope, *instance_obj);
  Handle<Object> attr_name(&scope, runtime.symbols()->ExtensionPtr());
  Handle<Integer> extension_obj(
      &scope, runtime.instanceAt(thread, instance, attr_name));
  CustomObject* custom_obj =
      static_cast<CustomObject*>(extension_obj->asCPointer());
  EXPECT_EQ(custom_obj->value, 30);

  // TODO(T29877145): free should be handled by tp_dealloc
  std::free(custom_obj);
}

} // namespace python
