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
  Handle<Object> type_class_instance(&scope, runtime.newInstance(layout));
  EXPECT_TRUE(type_class_instance->isInstance());
}

} // namespace python
