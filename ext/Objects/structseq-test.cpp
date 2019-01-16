#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

using namespace testing;

using StructSeqExtensionApiTest = ExtensionApi;

TEST_F(StructSeqExtensionApiTest, NewTypeCreatesRuntimeType) {
  PyStructSequence_Field desc_fields[] = {
      {const_cast<char*>("foo"), const_cast<char*>("foo field")},
      {PyStructSequence_UnnamedField, const_cast<char*>("unnamed field 1")},
      {PyStructSequence_UnnamedField, const_cast<char*>("unnamed field 2")},
      {nullptr}};
  PyStructSequence_Desc desc = {const_cast<char*>("Foo"),
                                const_cast<char*>("docs"), desc_fields, 1};
  PyObjectPtr type(
      reinterpret_cast<PyObject*>(PyStructSequence_NewType(&desc)));
  ASSERT_NE(type, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyType_CheckExact(type));

  PyObjectPtr seq_attr1(PyObject_GetAttrString(type, "n_sequence_fields"));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_NE(seq_attr1, nullptr);
  EXPECT_EQ(PyLong_AsLong(seq_attr1), 1);

  PyObjectPtr seq_attr2(PyObject_GetAttrString(type, "n_unnamed_fields"));
  ASSERT_NE(seq_attr2, nullptr);
  EXPECT_EQ(PyLong_AsLong(seq_attr2), 2);

  PyObjectPtr seq_attr3(PyObject_GetAttrString(type, "n_fields"));
  ASSERT_NE(seq_attr3, nullptr);
  EXPECT_EQ(PyLong_AsLong(seq_attr3), 3);
}

}  // namespace python
