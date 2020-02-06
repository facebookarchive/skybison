#include "Python.h"
#include "gtest/gtest.h"

#include "capi-fixture.h"
#include "capi-testing.h"

namespace py {
namespace testing {

using GcModuleExtensionApiTest = ExtensionApi;

TEST_F(GcModuleExtensionApiTest, GCMallocReturnsNotNullPtr) {
  PyObject* py_obj = _PyObject_GC_Malloc(12);
  EXPECT_NE(py_obj, nullptr);
  PyObject_GC_Del(py_obj);
}

TEST_F(GcModuleExtensionApiTest, GCDelWithTrackedObjectSucceeds) {
  PyObject* py_obj = _PyObject_GC_Malloc(12);
  ASSERT_NE(py_obj, nullptr);
  PyObject_GC_Track(py_obj);
  PyObject_GC_Del(py_obj);
}

TEST_F(GcModuleExtensionApiTest, GCDelWithUnTrackedObjectSucceeds) {
  PyObject* py_obj = _PyObject_GC_Malloc(12);
  ASSERT_NE(py_obj, nullptr);
  PyObject_GC_UnTrack(py_obj);
  PyObject_GC_Del(py_obj);
}

TEST_F(GcModuleExtensionApiTest, GCTrackWithUnTrackedObjectSucceeds) {
  PyObject* py_obj = _PyObject_GC_Malloc(12);
  ASSERT_NE(py_obj, nullptr);
  PyObject_GC_Track(py_obj);
  PyObject_GC_UnTrack(py_obj);
  PyObject_GC_Track(py_obj);
  PyObject_GC_Del(py_obj);
}

TEST_F(GcModuleExtensionApiTest, NewReturnsAllocatedObject) {
  struct BarObject {
    PyObject_HEAD int value;
  };
  destructor dealloc = [](PyObject* self) {
    PyTypeObject* type = Py_TYPE(self);
    PyObject_GC_Del(self);
    Py_DECREF(type);
  };
  PyType_Slot slots[] = {
      {Py_tp_dealloc, reinterpret_cast<void*>(dealloc)},
      {0, nullptr},
  };
  static PyType_Spec spec;
  spec = {
      "foo.Bar", sizeof(BarObject), 0, Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
      slots,
  };
  PyObjectPtr type(PyType_FromSpec(&spec));
  Py_ssize_t refcnt = Py_REFCNT(type.get());
  PyObjectPtr instance(reinterpret_cast<PyObject*>(
      PyObject_GC_New(BarObject, type.asTypeObject())));
  PyObject_GC_Track(instance);
  ASSERT_NE(instance, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_GE(Py_REFCNT(instance), 1);  // CPython
  ASSERT_LE(Py_REFCNT(instance), 2);  // Pyro
  EXPECT_EQ(Py_REFCNT(type), refcnt + 1);
}

TEST_F(GcModuleExtensionApiTest, NewVarReturnsAllocatedObject) {
  struct BarObject {
    PyObject_HEAD int value;
  };
  struct BarContainer {
    PyObject_VAR_HEAD BarObject* items[1];
  };
  destructor dealloc = [](PyObject* self) {
    PyTypeObject* type = Py_TYPE(self);
    PyObject_GC_Del(self);
    Py_DECREF(type);
  };
  PyType_Slot slots[] = {
      {Py_tp_dealloc, reinterpret_cast<void*>(dealloc)},
      {0, nullptr},
  };
  static PyType_Spec spec;
  spec = {
      "foo.Bar",
      sizeof(BarContainer),
      sizeof(BarObject),
      Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
      slots,
  };
  PyObjectPtr type(PyType_FromSpec(&spec));
  PyObjectPtr instance(reinterpret_cast<PyObject*>(
      PyObject_GC_NewVar(BarContainer, type.asTypeObject(), 5)));
  PyObject_GC_Track(instance);
  ASSERT_NE(instance, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_GE(Py_REFCNT(instance), 1);  // CPython
  ASSERT_LE(Py_REFCNT(instance), 2);  // Pyro
  EXPECT_EQ(Py_SIZE(instance.get()), 5);
}

}  // namespace testing
}  // namespace py
