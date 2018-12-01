#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

using namespace testing;

using TypeExtensionApiTest = ExtensionApi;
using TypeExtensionApiDeathTest = ExtensionApi;

TEST_F(TypeExtensionApiTest, PyTypeCheckOnInt) {
  PyObject* pylong = PyLong_FromLong(10);
  EXPECT_FALSE(PyType_Check(pylong));
  EXPECT_FALSE(PyType_CheckExact(pylong));
}

TEST_F(TypeExtensionApiTest, PyTypeCheckOnType) {
  PyObject* pylong = PyLong_FromLong(10);
  PyObject* type = reinterpret_cast<PyObject*>(Py_TYPE(pylong));
  EXPECT_TRUE(PyType_Check(type));
  EXPECT_TRUE(PyType_CheckExact(type));
}

TEST_F(TypeExtensionApiDeathTest, GetFlagsFromBuiltInTypePyro) {
  testing::PyObjectPtr long_obj(PyLong_FromLong(5));
  PyTypeObject* long_type = Py_TYPE(long_obj);
  ASSERT_TRUE(PyType_CheckExact(long_type));
  EXPECT_DEATH(PyType_GetFlags(reinterpret_cast<PyTypeObject*>(long_type)),
               "unimplemented: GetFlags from built-in types");
}

TEST_F(TypeExtensionApiDeathTest, GetFlagsFromManagedTypePyro) {
  PyRun_SimpleString(R"(class Foo: pass)");
  PyObject* foo_type = testing::moduleGet("__main__", "Foo");
  ASSERT_TRUE(PyType_CheckExact(foo_type));
  EXPECT_DEATH(
      PyType_GetFlags(reinterpret_cast<PyTypeObject*>(foo_type)),
      "unimplemented: GetFlags from types initialized through Python code");
}

TEST_F(TypeExtensionApiTest, ReadyCreatesRuntimeType) {
  // Create a simple PyTypeObject
  static PyTypeObject empty_type;
  empty_type = {PyObject_HEAD_INIT(nullptr)};
  empty_type.tp_name = "test.Empty";
  empty_type.tp_flags = Py_TPFLAGS_DEFAULT;
  ASSERT_EQ(PyType_Ready(&empty_type), 0);

  // Add to a module
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT,
      "test",
  };
  PyObject* m = PyModule_Create(&def);
  PyModule_AddObject(m, "Empty", reinterpret_cast<PyObject*>(&empty_type));

  // TODO(miro): import the module instead of injecting it into __main__
  testing::moduleSet("__main__", "test", m);
  PyRun_SimpleString("x = test.Empty");
  EXPECT_TRUE(PyType_CheckExact(testing::moduleGet("__main__", "x")));
}

// clang-format off
typedef struct {
  PyObject_HEAD
  int value;
} CustomObject;
// clang-format on

static PyObject* Custom_new(PyTypeObject* type, PyObject*, PyObject*) {
  CustomObject* self = reinterpret_cast<CustomObject*>(type->tp_alloc(type, 0));
  return reinterpret_cast<PyObject*>(self);
}

static int Custom_init(CustomObject* self, PyObject*, PyObject*) {
  self->value = 30;
  return 0;
}

static void Custom_dealloc(CustomObject* self) {
  Py_TYPE(self)->tp_free(static_cast<void*>(self));
}

TEST_F(TypeExtensionApiTest, InitializeCustomTypeInstancePyro) {
  // Instantiate Type
  static PyTypeObject custom_type;
  custom_type = {PyObject_HEAD_INIT(nullptr)};
  custom_type.tp_basicsize = sizeof(CustomObject);
  custom_type.tp_name = "custom.Custom";
  custom_type.tp_flags = Py_TPFLAGS_DEFAULT;
  custom_type.tp_alloc = PyType_GenericAlloc;
  custom_type.tp_new = reinterpret_cast<newfunc>(Custom_new);
  custom_type.tp_init = reinterpret_cast<initproc>(Custom_init);
  custom_type.tp_dealloc = reinterpret_cast<destructor>(Custom_dealloc);
  custom_type.tp_free = reinterpret_cast<freefunc>(PyObject_Del);
  PyType_Ready(&custom_type);

  // Add to a module
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT,
      "custom",
  };
  PyObject* m = PyModule_Create(&def);
  PyModule_AddObject(m, "Custom", reinterpret_cast<PyObject*>(&custom_type));

  // TODO(miro): import the module instead of injecting it into __main__
  testing::moduleSet("__main__", "custom", m);
  PyRun_SimpleString(R"(
instance1 = custom.Custom()
instance2 = custom.Custom()
)");

  // Verify the initialized value
  CustomObject* instance1 = reinterpret_cast<CustomObject*>(
      testing::moduleGet("__main__", "instance1"));
  EXPECT_EQ(instance1->value, 30);

  CustomObject* instance2 = reinterpret_cast<CustomObject*>(
      testing::moduleGet("__main__", "instance2"));
  EXPECT_EQ(Py_REFCNT(instance2), 2);
  Py_DECREF(instance2);
  Py_DECREF(instance2);
  Py_DECREF(instance1);
  Py_DECREF(instance1);
}

TEST_F(TypeExtensionApiTest, GenericAllocationReturnsMallocMemory) {
  // Instantiate Type
  static PyTypeObject custom_type;
  custom_type = {PyObject_HEAD_INIT(nullptr)};
  custom_type.tp_basicsize = sizeof(CustomObject);
  custom_type.tp_name = "custom.Custom";
  custom_type.tp_flags = Py_TPFLAGS_DEFAULT;
  PyType_Ready(&custom_type);

  PyObject* result = PyType_GenericAlloc(&custom_type, 0);
  EXPECT_EQ(1, Py_REFCNT(result));
  Py_DECREF(result);
}

TEST_F(TypeExtensionApiTest, IsSubtypeWithSameTypeReturnsTrue) {
  PyObjectPtr pylong(PyLong_FromLong(10));
  EXPECT_TRUE(PyType_IsSubtype(Py_TYPE(pylong), Py_TYPE(pylong)));
}

TEST_F(TypeExtensionApiTest, IsSubtypeWithSubtypeReturnsTrue) {
  EXPECT_EQ(PyRun_SimpleString("class MyFloat(float): pass"), 0);
  PyObjectPtr pyfloat(PyFloat_FromDouble(1.23));
  PyTypeObject* myfloat_type =
      reinterpret_cast<PyTypeObject*>(moduleGet("__main__", "MyFloat"));
  EXPECT_TRUE(PyType_IsSubtype(myfloat_type, Py_TYPE(pyfloat)));
  Py_DECREF(myfloat_type);
}

TEST_F(TypeExtensionApiTest, IsSubtypeWithDifferentTypesReturnsFalse) {
  PyObjectPtr pylong(PyLong_FromLong(10));
  PyObjectPtr pyuni(PyUnicode_FromString("string"));
  EXPECT_FALSE(PyType_IsSubtype(Py_TYPE(pylong), Py_TYPE(pyuni)));
}

TEST_F(TypeExtensionApiTest, GetSlotFromBuiltinTypeThrowsSystemError) {
  testing::PyObjectPtr long_obj(PyLong_FromLong(5));
  PyTypeObject* long_type = Py_TYPE(long_obj);
  ASSERT_TRUE(PyType_CheckExact(long_type));

  EXPECT_EQ(PyType_GetSlot(long_type, Py_tp_new), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(TypeExtensionApiTest, GetSlotFromStaticExtensionTypeThrowsSystemError) {
  static PyTypeObject type_obj;
  type_obj = {PyObject_HEAD_INIT(nullptr)};
  type_obj.tp_name = "Foo";

  EXPECT_EQ(PyType_GetSlot(&type_obj, Py_tp_new), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(TypeExtensionApiDeathTest,
       GetSlotFromManagedTypeReturnsFunctionPointerPyro) {
  PyRun_SimpleString(R"(
class Foo:
    def __init__(self):
        pass
  )");

  PyObject* foo_type = testing::moduleGet("__main__", "Foo");
  ASSERT_TRUE(PyType_CheckExact(foo_type));
  EXPECT_DEATH(
      PyType_GetSlot(reinterpret_cast<PyTypeObject*>(foo_type), Py_tp_init),
      "Get slots from types initialized through Python code");
}

TEST_F(TypeExtensionApiDeathTest, InitSlotWrapperReturnsInstancePyro) {
  PyRun_SimpleString(R"(
class Foo(object):
    def __init__(self):
        self.bar = 3
  )");

  PyObject* foo_type = testing::moduleGet("__main__", "Foo");
  ASSERT_TRUE(PyType_CheckExact(foo_type));
  auto foo_type_obj = reinterpret_cast<PyTypeObject*>(foo_type);
  EXPECT_DEATH(PyType_GetSlot(foo_type_obj, Py_tp_new),
               "Get slots from types initialized through Python code");
}

TEST_F(TypeExtensionApiDeathTest, CallSlotsWithDescriptorsReturnsInstancePyro) {
  PyRun_SimpleString(R"(
def custom_get(self, instance, value):
    return self

def custom_new(type):
    type.baz = 5
    return object.__new__(type)

def custom_init(self):
    self.bar = 3
# custom_init.__get__ = custom_get

class Foo(object): pass
Foo.__new__ = custom_new
Foo.__init__ = custom_init
  )");

  PyObject* foo_type = testing::moduleGet("__main__", "Foo");
  ASSERT_TRUE(PyType_CheckExact(foo_type));
  auto foo_type_obj = reinterpret_cast<PyTypeObject*>(foo_type);
  EXPECT_DEATH(PyType_GetSlot(foo_type_obj, Py_tp_new),
               "Get slots from types initialized through Python code");
}

TEST_F(TypeExtensionApiDeathTest, SlotWrapperWithArgumentsAbortsPyro) {
  PyRun_SimpleString(R"(
class Foo:
    def __new__(self, value):
        self.bar = value
  )");

  PyObject* foo_type = testing::moduleGet("__main__", "Foo");
  ASSERT_TRUE(PyType_CheckExact(foo_type));
  auto foo_type_obj = reinterpret_cast<PyTypeObject*>(foo_type);
  EXPECT_DEATH(PyType_GetSlot(foo_type_obj, Py_tp_new),
               "Get slots from types initialized through Python code");
}

TEST_F(TypeExtensionApiTest, GetNonExistentSlotFromManagedTypeReturnsNull) {
  PyRun_SimpleString(R"(
class Foo: pass
  )");

  PyObject* foo_type = testing::moduleGet("__main__", "Foo");
  ASSERT_TRUE(PyType_CheckExact(foo_type));
  EXPECT_EQ(PyType_GetSlot(reinterpret_cast<PyTypeObject*>(foo_type), Py_nb_or),
            nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  Py_DECREF(foo_type);
}

TEST_F(TypeExtensionApiTest, GetSlotFromNegativeSlotThrowsSystemError) {
  PyRun_SimpleString(R"(
class Foo: pass
  )");

  PyObject* foo_type = testing::moduleGet("__main__", "Foo");
  ASSERT_TRUE(PyType_CheckExact(foo_type));

  EXPECT_EQ(PyType_GetSlot(reinterpret_cast<PyTypeObject*>(foo_type), -1),
            nullptr);

  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(TypeExtensionApiTest, GetSlotFromLargerThanMaxSlotReturnsNull) {
  PyRun_SimpleString(R"(
class Foo: pass
  )");

  PyObject* foo_type = testing::moduleGet("__main__", "Foo");
  ASSERT_TRUE(PyType_CheckExact(foo_type));

  EXPECT_EQ(PyType_GetSlot(reinterpret_cast<PyTypeObject*>(foo_type), 1000),
            nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(TypeExtensionApiDeathTest, VarSizeFromBuiltInTypePyro) {
  testing::PyObjectPtr long_obj(PyLong_FromLong(5));
  PyTypeObject* long_type = Py_TYPE(long_obj);
  ASSERT_TRUE(PyType_CheckExact(long_type));
  EXPECT_DEATH(
      _PyObject_VAR_SIZE(reinterpret_cast<PyTypeObject*>(long_type), 1),
      "unimplemented: VAR_SIZE from built-in types");
}

TEST_F(TypeExtensionApiDeathTest, VarSizeFromManagedTypePyro) {
  PyRun_SimpleString(R"(class Foo: pass)");
  PyObject* foo_type = testing::moduleGet("__main__", "Foo");
  ASSERT_TRUE(PyType_CheckExact(foo_type));
  EXPECT_DEATH(
      _PyObject_VAR_SIZE(reinterpret_cast<PyTypeObject*>(foo_type), 1),
      "unimplemented: VAR_SIZE from types initialized through Python code");
}

}  // namespace python
