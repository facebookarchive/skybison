#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

using namespace testing;

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

}  // namespace python
