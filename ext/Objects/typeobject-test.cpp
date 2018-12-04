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

TEST_F(TypeExtensionApiTest, GetFlagsFromExtensionTypeReturnsSetFlags) {
  PyType_Slot slot[] = {
      {0, nullptr},
  };
  PyType_Spec spec;
  spec.name = "test.Empty";
  spec.flags = Py_TPFLAGS_DEFAULT;
  spec.slots = slot;
  PyObjectPtr type(PyType_FromSpec(&spec));
  ASSERT_NE(type.get(), nullptr);
  ASSERT_TRUE(PyType_CheckExact(type.get()));

  EXPECT_TRUE(PyType_GetFlags(reinterpret_cast<PyTypeObject*>(type.get())) &
              Py_TPFLAGS_DEFAULT);
  EXPECT_TRUE(PyType_GetFlags(reinterpret_cast<PyTypeObject*>(type.get())) &
              Py_TPFLAGS_READY);
}

TEST_F(TypeExtensionApiTest, FromSpecCreatesRuntimeType) {
  PyType_Slot slots[] = {
      {0, nullptr},
  };
  PyType_Spec spec;
  spec.name = "test.Empty";
  spec.flags = Py_TPFLAGS_DEFAULT;
  spec.slots = slots;
  PyObjectPtr type(PyType_FromSpec(&spec));
  ASSERT_NE(type.get(), nullptr);
  ASSERT_TRUE(PyType_CheckExact(type.get()));

  testing::moduleSet("__main__", "Empty", type.get());
  PyRun_SimpleString("x = Empty");
  EXPECT_TRUE(PyType_CheckExact(testing::moduleGet("__main__", "x")));
}

TEST_F(TypeExtensionApiTest, FromSpecWithInvalidSlotThrowsError) {
  PyType_Slot slots[] = {
      {-1, nullptr},
  };
  PyType_Spec spec;
  spec.name = "test.Empty";
  spec.flags = Py_TPFLAGS_DEFAULT;
  spec.slots = slots;
  ASSERT_EQ(PyType_FromSpec(&spec), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_RuntimeError));
  // TODO(eelizondo): Check that error matches with "invalid slot offset"
}

// clang-format off
typedef struct {
  PyObject_HEAD
  int value;
} CustomObject;
// clang-format on

TEST_F(TypeExtensionApiTest, CallExtensionTypeReturnsExtensionInstancePyro) {
  newfunc custom_new = [](PyTypeObject* type, PyObject*, PyObject*) {
    void* slot = PyType_GetSlot(type, Py_tp_alloc);
    return reinterpret_cast<allocfunc>(slot)(type, 0);
  };
  initproc custom_init = [](PyObject* self, PyObject*, PyObject*) {
    reinterpret_cast<CustomObject*>(self)->value = 30;
    return 0;
  };
  destructor custom_dealloc = [](PyObject* self) {
    void* slot = PyType_GetSlot(Py_TYPE(self), Py_tp_free);
    return reinterpret_cast<freefunc>(slot)(self);
  };
  PyType_Slot custom_slots[] = {
      {Py_tp_alloc, reinterpret_cast<void*>(PyType_GenericAlloc)},
      {Py_tp_new, reinterpret_cast<void*>(custom_new)},
      {Py_tp_init, reinterpret_cast<void*>(custom_init)},
      {Py_tp_dealloc, reinterpret_cast<void*>(custom_dealloc)},
      {Py_tp_free, reinterpret_cast<void*>(PyObject_Del)},
      {0, nullptr},
  };
  PyType_Spec custom_spec = {
      "custom.Custom",    sizeof(CustomObject), 0,
      Py_TPFLAGS_DEFAULT, custom_slots,
  };
  PyObjectPtr custom_type(PyType_FromSpec(&custom_spec));
  ASSERT_NE(custom_type.get(), nullptr);
  ASSERT_TRUE(PyType_CheckExact(custom_type.get()));

  testing::moduleSet("__main__", "Custom", custom_type.get());
  PyRun_SimpleString(R"(
instance = Custom()
)");

  PyObjectPtr instance(testing::moduleGet("__main__", "instance"));
  ASSERT_NE(instance.get(), nullptr);
  CustomObject* custom_instance =
      reinterpret_cast<CustomObject*>(instance.get());
  EXPECT_EQ(custom_instance->value, 30);
  EXPECT_EQ(Py_REFCNT(custom_instance), 2);
  Py_DECREF(custom_instance);
}

TEST_F(TypeExtensionApiTest, GenericAllocationReturnsMallocMemory) {
  int basic_size = sizeof(CustomObject);
  int item_size = 16;
  PyType_Slot custom_slots[] = {
      {0, nullptr},
  };
  PyType_Spec custom_spec = {
      "custom.Custom", basic_size, item_size, Py_TPFLAGS_DEFAULT, custom_slots,
  };
  PyObjectPtr custom_type(PyType_FromSpec(&custom_spec));
  ASSERT_NE(custom_type.get(), nullptr);
  ASSERT_TRUE(PyType_CheckExact(custom_type.get()));

  PyObjectPtr result(PyType_GenericAlloc(
      reinterpret_cast<PyTypeObject*>(custom_type.get()), item_size));
  ASSERT_NE(result.get(), nullptr);
  EXPECT_EQ(Py_REFCNT(result.get()), 1);
  EXPECT_EQ(Py_SIZE(result.get()), item_size);
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

TEST_F(TypeExtensionApiTest, GetSlotFromExtensionType) {
  newfunc custom_new = [](PyTypeObject* type, PyObject*, PyObject*) {
    void* slot = PyType_GetSlot(type, Py_tp_alloc);
    return reinterpret_cast<allocfunc>(slot)(type, 0);
  };
  initproc custom_init = [](PyObject*, PyObject*, PyObject*) { return 0; };
  PyType_Slot custom_slots[] = {
      {Py_tp_alloc, reinterpret_cast<void*>(PyType_GenericAlloc)},
      {Py_tp_new, reinterpret_cast<void*>(custom_new)},
      {Py_tp_init, reinterpret_cast<void*>(custom_init)},
      {0, nullptr},
  };
  PyType_Spec custom_spec = {
      "custom.Custom",    sizeof(CustomObject), 0,
      Py_TPFLAGS_DEFAULT, custom_slots,
  };
  PyObjectPtr custom_type(PyType_FromSpec(&custom_spec));
  ASSERT_NE(custom_type.get(), nullptr);
  ASSERT_TRUE(PyType_CheckExact(custom_type.get()));

  PyTypeObject* custom_type_obj =
      reinterpret_cast<PyTypeObject*>(custom_type.get());
  EXPECT_EQ(PyType_GetSlot(custom_type_obj, Py_tp_alloc),
            reinterpret_cast<void*>(PyType_GenericAlloc));
  EXPECT_EQ(PyType_GetSlot(custom_type_obj, Py_tp_new),
            reinterpret_cast<void*>(custom_new));
  EXPECT_EQ(PyType_GetSlot(custom_type_obj, Py_tp_init),
            reinterpret_cast<void*>(custom_init));
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

}  // namespace python
