#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

using namespace testing;

using TypeExtensionApiTest = ExtensionApi;
using TypeExtensionApiDeathTest = ExtensionApi;

TEST_F(TypeExtensionApiTest, PyTypeCheckOnLong) {
  PyObjectPtr pylong(PyLong_FromLong(10));
  EXPECT_FALSE(PyType_Check(pylong));
  EXPECT_FALSE(PyType_CheckExact(pylong));
}

TEST_F(TypeExtensionApiTest, PyTypeCheckOnType) {
  PyObjectPtr pylong(PyLong_FromLong(10));
  PyObjectPtr pylong_type(PyObject_Type(pylong));
  EXPECT_TRUE(PyType_Check(pylong_type));
  EXPECT_TRUE(PyType_CheckExact(pylong_type));
}

TEST_F(TypeExtensionApiDeathTest, GetFlagsFromBuiltInTypePyro) {
  PyObjectPtr pylong(PyLong_FromLong(5));
  PyObjectPtr pylong_type(PyObject_Type(pylong));
  ASSERT_TRUE(PyType_CheckExact(pylong_type));
  EXPECT_DEATH(
      PyType_GetFlags(reinterpret_cast<PyTypeObject*>(pylong_type.get())),
      "unimplemented: GetFlags from built-in types");
}

TEST_F(TypeExtensionApiDeathTest, GetFlagsFromManagedTypePyro) {
  PyRun_SimpleString(R"(class Foo: pass)");
  PyObjectPtr foo_type(testing::moduleGet("__main__", "Foo"));
  ASSERT_TRUE(PyType_CheckExact(foo_type));
  EXPECT_DEATH(
      PyType_GetFlags(reinterpret_cast<PyTypeObject*>(foo_type.get())),
      "unimplemented: GetFlags from types initialized through Python code");
}

TEST_F(TypeExtensionApiTest, GetFlagsFromExtensionTypeReturnsSetFlags) {
  PyType_Slot slots[] = {
      {0, nullptr},
  };
  static PyType_Spec spec;
  spec = {
      "foo.Bar", 0, 0, Py_TPFLAGS_DEFAULT, slots,
  };
  PyObjectPtr type(PyType_FromSpec(&spec));
  ASSERT_NE(type, nullptr);
  ASSERT_TRUE(PyType_CheckExact(type));

  EXPECT_TRUE(PyType_GetFlags(reinterpret_cast<PyTypeObject*>(type.get())) &
              Py_TPFLAGS_DEFAULT);
  EXPECT_TRUE(PyType_GetFlags(reinterpret_cast<PyTypeObject*>(type.get())) &
              Py_TPFLAGS_READY);
}

TEST_F(TypeExtensionApiTest, FromSpecCreatesRuntimeType) {
  PyType_Slot slots[] = {
      {0, nullptr},
  };
  static PyType_Spec spec;
  spec = {
      "foo.Bar", 0, 0, Py_TPFLAGS_DEFAULT, slots,
  };
  PyObjectPtr type(PyType_FromSpec(&spec));
  ASSERT_NE(type, nullptr);
  ASSERT_TRUE(PyType_CheckExact(type));

  testing::moduleSet("__main__", "Empty", type);
  PyRun_SimpleString("x = Empty");
  EXPECT_TRUE(PyType_CheckExact(testing::moduleGet("__main__", "x")));
}

TEST_F(TypeExtensionApiTest, FromSpecWithInvalidSlotThrowsError) {
  PyType_Slot slots[] = {
      {-1, nullptr},
  };
  static PyType_Spec spec;
  spec = {
      "foo.Bar", 0, 0, Py_TPFLAGS_DEFAULT, slots,
  };
  ASSERT_EQ(PyType_FromSpec(&spec), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_RuntimeError));
  // TODO(eelizondo): Check that error matches with "invalid slot offset"
}

TEST_F(TypeExtensionApiTest, CallExtensionTypeReturnsExtensionInstancePyro) {
  // clang-format off
  struct BarObject {
    PyObject_HEAD
    int value;
  };
  // clang-format on
  newfunc new_func = [](PyTypeObject* type, PyObject*, PyObject*) {
    void* slot = PyType_GetSlot(type, Py_tp_alloc);
    return reinterpret_cast<allocfunc>(slot)(type, 0);
  };
  initproc init_func = [](PyObject* self, PyObject*, PyObject*) {
    reinterpret_cast<BarObject*>(self)->value = 30;
    return 0;
  };
  destructor dealloc_func = [](PyObject* self) {
    PyObjectPtr type(PyObject_Type(self));
    void* slot =
        PyType_GetSlot(reinterpret_cast<PyTypeObject*>(type.get()), Py_tp_free);
    return reinterpret_cast<freefunc>(slot)(self);
  };
  PyType_Slot slots[] = {
      {Py_tp_alloc, reinterpret_cast<void*>(PyType_GenericAlloc)},
      {Py_tp_new, reinterpret_cast<void*>(new_func)},
      {Py_tp_init, reinterpret_cast<void*>(init_func)},
      {Py_tp_dealloc, reinterpret_cast<void*>(dealloc_func)},
      {Py_tp_free, reinterpret_cast<void*>(PyObject_Del)},
      {0, nullptr},
  };
  static PyType_Spec spec;
  spec = {
      "foo.Bar", sizeof(BarObject), 0, Py_TPFLAGS_DEFAULT, slots,
  };
  PyObjectPtr type(PyType_FromSpec(&spec));
  ASSERT_NE(type, nullptr);
  ASSERT_TRUE(PyType_CheckExact(type));

  testing::moduleSet("__main__", "Bar", type);
  PyRun_SimpleString(R"(
bar = Bar()
)");

  PyObjectPtr bar(testing::moduleGet("__main__", "bar"));
  ASSERT_NE(bar, nullptr);
  BarObject* barobj = reinterpret_cast<BarObject*>(bar.get());
  EXPECT_EQ(barobj->value, 30);
  EXPECT_EQ(Py_REFCNT(barobj), 2);
  Py_DECREF(barobj);
}

TEST_F(TypeExtensionApiTest, GenericAllocationReturnsMallocMemory) {
  // These numbers determine the allocated size of the PyObject
  // The values in this test are abitrary and are usally set with `sizeof(Foo)`
  int basic_size = 10;
  int item_size = 5;
  PyType_Slot slots[] = {
      {0, nullptr},
  };
  static PyType_Spec spec;
  spec = {
      "foo.Bar", basic_size, item_size, Py_TPFLAGS_DEFAULT, slots,
  };
  PyObjectPtr type(PyType_FromSpec(&spec));
  ASSERT_NE(type, nullptr);
  ASSERT_TRUE(PyType_CheckExact(type));

  PyObjectPtr result(PyType_GenericAlloc(
      reinterpret_cast<PyTypeObject*>(type.get()), item_size));
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(Py_REFCNT(result), 1);
  EXPECT_EQ(Py_SIZE(result.get()), item_size);
}

TEST_F(TypeExtensionApiTest, IsSubtypeWithSameTypeReturnsTrue) {
  PyObjectPtr pylong(PyLong_FromLong(10));
  PyObjectPtr pylong_type(PyObject_Type(pylong));
  EXPECT_TRUE(
      PyType_IsSubtype(reinterpret_cast<PyTypeObject*>(pylong_type.get()),
                       reinterpret_cast<PyTypeObject*>(pylong_type.get())));
}

TEST_F(TypeExtensionApiTest, IsSubtypeWithSubtypeReturnsTrue) {
  EXPECT_EQ(PyRun_SimpleString("class MyFloat(float): pass"), 0);
  PyObjectPtr pyfloat(PyFloat_FromDouble(1.23));
  PyObjectPtr pyfloat_type(PyObject_Type(pyfloat));
  PyObjectPtr myfloat_type(moduleGet("__main__", "MyFloat"));
  EXPECT_TRUE(
      PyType_IsSubtype(reinterpret_cast<PyTypeObject*>(myfloat_type.get()),
                       reinterpret_cast<PyTypeObject*>(pyfloat_type.get())));
}

TEST_F(TypeExtensionApiTest, IsSubtypeWithDifferentTypesReturnsFalse) {
  PyObjectPtr pylong(PyLong_FromLong(10));
  PyObjectPtr pylong_type(PyObject_Type(pylong));
  PyObjectPtr pyuni(PyUnicode_FromString("string"));
  PyObjectPtr pyuni_type(PyObject_Type(pyuni));
  EXPECT_FALSE(
      PyType_IsSubtype(reinterpret_cast<PyTypeObject*>(pylong_type.get()),
                       reinterpret_cast<PyTypeObject*>(pyuni_type.get())));
}

TEST_F(TypeExtensionApiTest, GetSlotFromBuiltinTypeThrowsSystemError) {
  PyObjectPtr pylong(PyLong_FromLong(5));
  PyObjectPtr pylong_type(PyObject_Type(pylong));
  ASSERT_TRUE(
      PyType_CheckExact(reinterpret_cast<PyTypeObject*>(pylong_type.get())));

  EXPECT_EQ(PyType_GetSlot(reinterpret_cast<PyTypeObject*>(pylong_type.get()),
                           Py_tp_new),
            nullptr);
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

  PyObjectPtr foo_type(testing::moduleGet("__main__", "Foo"));
  ASSERT_TRUE(PyType_CheckExact(foo_type));
  EXPECT_DEATH(PyType_GetSlot(reinterpret_cast<PyTypeObject*>(foo_type.get()),
                              Py_tp_init),
               "Get slots from types initialized through Python code");
}

TEST_F(TypeExtensionApiDeathTest, InitSlotWrapperReturnsInstancePyro) {
  PyRun_SimpleString(R"(
class Foo(object):
    def __init__(self):
        self.bar = 3
  )");

  PyObjectPtr foo_type(testing::moduleGet("__main__", "Foo"));
  ASSERT_TRUE(PyType_CheckExact(foo_type));
  EXPECT_DEATH(PyType_GetSlot(reinterpret_cast<PyTypeObject*>(foo_type.get()),
                              Py_tp_new),
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

class Foo(object): pass
Foo.__new__ = custom_new
Foo.__init__ = custom_init
  )");

  PyObjectPtr foo_type(testing::moduleGet("__main__", "Foo"));
  ASSERT_TRUE(PyType_CheckExact(foo_type));
  EXPECT_DEATH(PyType_GetSlot(reinterpret_cast<PyTypeObject*>(foo_type.get()),
                              Py_tp_new),
               "Get slots from types initialized through Python code");
}

TEST_F(TypeExtensionApiDeathTest, SlotWrapperWithArgumentsAbortsPyro) {
  PyRun_SimpleString(R"(
class Foo:
    def __new__(self, value):
        self.bar = value
  )");

  PyObjectPtr foo_type(testing::moduleGet("__main__", "Foo"));
  ASSERT_TRUE(PyType_CheckExact(foo_type));
  EXPECT_DEATH(PyType_GetSlot(reinterpret_cast<PyTypeObject*>(foo_type.get()),
                              Py_tp_new),
               "Get slots from types initialized through Python code");
}

TEST_F(TypeExtensionApiTest, GetNonExistentSlotFromManagedTypeAbortsPyro) {
  PyRun_SimpleString(R"(
class Foo: pass
  )");

  PyObjectPtr foo_type(testing::moduleGet("__main__", "Foo"));
  ASSERT_TRUE(PyType_CheckExact(foo_type));
  EXPECT_DEATH(
      PyType_GetSlot(reinterpret_cast<PyTypeObject*>(foo_type.get()), Py_nb_or),
      "Get slots from types initialized through Python code");
}

TEST_F(TypeExtensionApiTest, GetSlotFromNegativeSlotThrowsSystemError) {
  PyRun_SimpleString(R"(
class Foo: pass
  )");

  PyObjectPtr foo_type(testing::moduleGet("__main__", "Foo"));
  ASSERT_TRUE(PyType_CheckExact(foo_type));

  EXPECT_EQ(PyType_GetSlot(reinterpret_cast<PyTypeObject*>(foo_type.get()), -1),
            nullptr);

  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(TypeExtensionApiTest, GetSlotFromLargerThanMaxSlotReturnsNull) {
  PyRun_SimpleString(R"(
class Foo: pass
  )");

  PyObjectPtr foo_type(testing::moduleGet("__main__", "Foo"));
  ASSERT_TRUE(PyType_CheckExact(foo_type));

  EXPECT_EQ(
      PyType_GetSlot(reinterpret_cast<PyTypeObject*>(foo_type.get()), 1000),
      nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(TypeExtensionApiTest, GetSlotFromExtensionType) {
  newfunc new_func = [](PyTypeObject* type, PyObject*, PyObject*) {
    void* slot = PyType_GetSlot(type, Py_tp_alloc);
    return reinterpret_cast<allocfunc>(slot)(type, 0);
  };
  initproc init_func = [](PyObject*, PyObject*, PyObject*) { return 0; };
  binaryfunc add_func = [](PyObject*, PyObject*) { return PyLong_FromLong(7); };
  PyType_Slot slots[] = {
      {Py_tp_alloc, reinterpret_cast<void*>(PyType_GenericAlloc)},
      {Py_tp_new, reinterpret_cast<void*>(new_func)},
      {Py_tp_init, reinterpret_cast<void*>(init_func)},
      {Py_nb_add, reinterpret_cast<void*>(add_func)},
      {0, nullptr},
  };
  static PyType_Spec spec;
  spec = {
      "foo.Bar", 0, 0, Py_TPFLAGS_DEFAULT, slots,
  };
  PyObjectPtr type(PyType_FromSpec(&spec));
  ASSERT_NE(type, nullptr);
  ASSERT_TRUE(PyType_CheckExact(type));

  PyTypeObject* typeobj = reinterpret_cast<PyTypeObject*>(type.get());
  EXPECT_EQ(PyType_GetSlot(typeobj, Py_tp_alloc),
            reinterpret_cast<void*>(PyType_GenericAlloc));
  EXPECT_EQ(PyType_GetSlot(typeobj, Py_tp_new),
            reinterpret_cast<void*>(new_func));
  EXPECT_EQ(PyType_GetSlot(typeobj, Py_tp_init),
            reinterpret_cast<void*>(init_func));
  EXPECT_EQ(PyType_GetSlot(typeobj, Py_nb_add),
            reinterpret_cast<void*>(add_func));
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

}  // namespace python
