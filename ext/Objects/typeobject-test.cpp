#include "gtest/gtest.h"

#include "Python.h"
#include "runtime/frame.h"
#include "runtime/runtime.h"
#include "runtime/test-utils.h"

namespace python {

TEST(TypeObject, ReadyInitializesType) {
  Runtime runtime;
  HandleScope scope;
  Handle<Dictionary> extensions_dict(&scope, runtime.extensionTypes());

  // Create a simple PyTypeObject
  PyTypeObject empty_type{PyObject_HEAD_INIT(nullptr)};
  empty_type.tp_name = "Empty";
  empty_type.tp_flags = Py_TPFLAGS_DEFAULT;

  // EmptyType is not initialized yet
  EXPECT_FALSE(PyType_GetFlags(&empty_type) & Py_TPFLAGS_READY);

  // Run PyType_Ready
  EXPECT_EQ(0, extensions_dict->numItems());
  EXPECT_EQ(0, PyType_Ready(&empty_type));
  EXPECT_EQ(1, extensions_dict->numItems());

  // Expect PyTypeObject to contain the correct flags
  EXPECT_TRUE(PyType_GetFlags(&empty_type) & Py_TPFLAGS_DEFAULT);
  EXPECT_TRUE(PyType_GetFlags(&empty_type) & Py_TPFLAGS_READY);
}

TEST(TypeObject, ReadyCreatesRuntimeClass) {
  Runtime runtime;
  HandleScope scope;
  Handle<Dictionary> extensions_dict(&scope, runtime.extensionTypes());

  // Create a simple PyTypeObject
  PyTypeObject empty_type{PyObject_HEAD_INIT(nullptr)};
  empty_type.tp_name = "Empty";
  empty_type.tp_flags = Py_TPFLAGS_DEFAULT;
  PyType_Ready(&empty_type);

  // Obtain EmptyType Class object from the runtime
  Handle<Object> type_id(
      &scope, runtime.newIntegerFromCPointer(static_cast<void*>(&empty_type)));
  Handle<Object> type_class_obj(&scope,
                                runtime.dictionaryAt(extensions_dict, type_id));
  EXPECT_TRUE(type_class_obj->isClass());

  // Confirm the class name
  Handle<Class> type_class(&scope, *type_class_obj);
  Handle<String> type_class_name(&scope, type_class->name());
  EXPECT_TRUE(type_class_name->equalsCString(empty_type.tp_name));

  // Confirm the PyTypeObject pointer
  Handle<Integer> pytype_addr_obj(&scope, type_class->extensionType());
  EXPECT_EQ(&empty_type, pytype_addr_obj->asCPointer());
}

TEST(TypeObject, ReadiedTypeCreatesRuntimeInstance) {
  Runtime runtime;
  HandleScope scope;
  Handle<Dictionary> extensions_dict(&scope, runtime.extensionTypes());

  // Create a simple PyTypeObject
  PyTypeObject empty_type{PyObject_HEAD_INIT(nullptr)};
  empty_type.tp_name = "Empty";
  empty_type.tp_flags = Py_TPFLAGS_DEFAULT;
  PyType_Ready(&empty_type);
  Handle<Object> type_id(
      &scope, runtime.newIntegerFromCPointer(static_cast<void*>(&empty_type)));
  Handle<Class> type_class(&scope,
                           runtime.dictionaryAt(extensions_dict, type_id));

  // Instantiate a class object
  Handle<Layout> layout(&scope, type_class->instanceLayout());
  Handle<Object> type_instance(&scope, runtime.newInstance(layout));
  EXPECT_TRUE(type_instance->isInstance());
}

typedef struct {
  PyObject_HEAD;
  int value;
} CustomObject;

static PyObject* Custom_new(PyTypeObject* type, PyObject* args,
                            PyObject* kwds) {
  CustomObject* self = reinterpret_cast<CustomObject*>(type->tp_alloc(type, 0));
  return reinterpret_cast<PyObject*>(self);
}

static int Custom_init(CustomObject* self, PyObject* args, PyObject* kwds) {
  self->value = 30;
  return 0;
}

static void Custom_dealloc(CustomObject* self) {
  Py_TYPE(self)->tp_free(static_cast<void*>(self));
}

TEST(TypeObject, InitializeCustomTypeInstance) {
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
  custom_type.tp_basicsize = sizeof(CustomObject);
  custom_type.tp_name = "custom.Custom";
  custom_type.tp_flags = Py_TPFLAGS_DEFAULT;
  custom_type.tp_alloc = PyType_GenericAlloc;
  custom_type.tp_new = Custom_new;
  custom_type.tp_init = reinterpret_cast<initproc>(Custom_init);
  custom_type.tp_dealloc = reinterpret_cast<destructor>(Custom_dealloc);
  custom_type.tp_free = PyObject_Del;
  PyType_Ready(&custom_type);

  // Add class to the runtime
  Handle<Object> type_id(
      &scope, runtime.newIntegerFromCPointer(static_cast<void*>(&custom_type)));
  Handle<Dictionary> ext_dict(&scope, runtime.extensionTypes());
  Handle<Object> type_class(&scope, runtime.dictionaryAt(ext_dict, type_id));
  Handle<Object> ob_name(&scope, runtime.newStringFromCString("Custom"));
  runtime.dictionaryAtPutInValueCell(module_dict, ob_name, type_class);

  runtime.runFromCString(R"(
import custom
instance = custom.Custom()
instance2 = custom.Custom()
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

  // Decref and dealloc custom instance
  Handle<HeapObject> instance2(
      &scope, testing::findInModule(&runtime, main, "instance2"));
  Handle<Integer> object_ptr2(&scope,
                              runtime.instanceAt(thread, instance2, attr_name));
  CustomObject* custom_obj2 =
      static_cast<CustomObject*>(object_ptr2->asCPointer());
  EXPECT_EQ(1, Py_REFCNT(custom_obj2));
  Py_DECREF(custom_obj2);
}

TEST(TypeObject, GenericAllocationReturnsMallocMemory) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();

  // Instantiate Class
  PyTypeObject custom_type{PyObject_HEAD_INIT(nullptr)};
  custom_type.tp_basicsize = sizeof(CustomObject);
  custom_type.tp_name = "custom.Custom";
  custom_type.tp_flags = Py_TPFLAGS_DEFAULT;

  PyType_Ready(&custom_type);

  PyObject* result = PyType_GenericAlloc(&custom_type, 0);
  Handle<Object> instance_obj(&scope,
                              ApiHandle::fromPyObject(result)->asObject());
  ASSERT_TRUE(instance_obj->isInstance());
  EXPECT_EQ(1, Py_REFCNT(result));
}

TEST(TypeObject, RuntimeInitializesStaticPyTypObjects) {
  Runtime runtime;
  Runtime runtime2;

  PyTypeObject* pytype1 = static_cast<PyTypeObject*>(
      runtime.builtinExtensionTypes(static_cast<word>(ExtensionTypes::kType)));
  PyTypeObject* pytype2 = static_cast<PyTypeObject*>(
      runtime2.builtinExtensionTypes(static_cast<word>(ExtensionTypes::kType)));
  EXPECT_NE(pytype1, pytype2);
  EXPECT_STREQ(pytype1->tp_name, pytype2->tp_name);

  // Both PyTypeObject will have a different memory from the PyVarObject header.
  // This offsets past the header, and compares the rest of memory
  word length = sizeof(*pytype1) - OFFSET_OF(PyTypeObject, tp_name);
  EXPECT_EQ(0, memcmp(&pytype1->tp_name, &pytype2->tp_name, length));
}

}  // namespace python
