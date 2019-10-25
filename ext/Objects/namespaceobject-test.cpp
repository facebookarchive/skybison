#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace py {

using namespace testing;

using NamespaceExtensionApiTest = ExtensionApi;

TEST_F(NamespaceExtensionApiTest, NewReturnsNamespaceObject) {
  PyObjectPtr pynamespace(_PyNamespace_New(nullptr));
  ASSERT_EQ(PyErr_Occurred(), nullptr);

  PyObject* key0 = PyUnicode_FromString("key0");
  PyObject* value0 = PyLong_FromLong(5);
  EXPECT_EQ(PyObject_SetAttr(pynamespace, key0, value0), 0);

  PyObject* key1 = PyUnicode_FromString("key1");
  PyObject* value1 = PyUnicode_FromString("value1");
  EXPECT_EQ(PyObject_SetAttr(pynamespace, key1, value1), 0);

  PyObjectPtr repr_result(PyObject_Str(pynamespace));
  ASSERT_EQ(PyErr_Occurred(), nullptr);

  EXPECT_TRUE(
      isUnicodeEqualsCStr(repr_result, "namespace(key0=5, key1='value1')"));
}

TEST_F(NamespaceExtensionApiTest,
       NewWithDictReturnsNamespaceObjectWithAttributes) {
  PyObject* dict = PyDict_New();
  PyObject* key = PyUnicode_FromString("key");
  PyObject* value = PyLong_FromLong(5);
  ASSERT_EQ(PyDict_SetItem(dict, key, value), 0);

  PyObjectPtr pynamespace(_PyNamespace_New(dict));
  ASSERT_EQ(PyErr_Occurred(), nullptr);

  EXPECT_EQ(PyObject_GetAttr(pynamespace, key), value);
}

}  // namespace py
