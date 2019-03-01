#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

using namespace testing;

using StructSeqExtensionApiTest = ExtensionApi;

PyStructSequence_Field desc_fields[] = {
    {const_cast<char*>("first"), const_cast<char*>("first field")},
    {const_cast<char*>("second"), const_cast<char*>("second field")},
    {const_cast<char*>("third"), const_cast<char*>("third field")},
    {const_cast<char*>("fourth"), const_cast<char*>("fourth field")},
    {const_cast<char*>("fifth"), const_cast<char*>("fifth field")},
    {nullptr}};

PyStructSequence_Desc desc = {const_cast<char*>("Structseq"),
                              const_cast<char*>("docs"), desc_fields, 2};

TEST_F(StructSeqExtensionApiTest, NewTypeCreatesRuntimeType) {
  PyObjectPtr type(PyStructSequence_NewType(&desc));
  ASSERT_NE(type, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyType_CheckExact(type));

  PyObjectPtr seq_attr1(PyObject_GetAttrString(type, "n_sequence_fields"));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_NE(seq_attr1, nullptr);
  EXPECT_EQ(PyLong_AsLong(seq_attr1), 2);

  PyObjectPtr seq_attr2(PyObject_GetAttrString(type, "n_unnamed_fields"));
  ASSERT_NE(seq_attr2, nullptr);
  EXPECT_EQ(PyLong_AsLong(seq_attr2), 0);

  PyObjectPtr seq_attr3(PyObject_GetAttrString(type, "n_fields"));
  ASSERT_NE(seq_attr3, nullptr);
  EXPECT_EQ(PyLong_AsLong(seq_attr3), 5);
}

TEST_F(StructSeqExtensionApiTest,
       NewInstanceWithLessThanMinSizeRaisesException) {
  PyObjectPtr type(PyStructSequence_NewType(&desc));
  ASSERT_NE(type, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyType_CheckExact(type));

  ASSERT_EQ(moduleSet("__main__", "Structseq", type), 0);
  // TODO(T40700664): Use PyRun_String and test for raised exception
  EXPECT_EQ(PyRun_SimpleString(R"(
Structseq()
)"),
            -1);
}

TEST_F(StructSeqExtensionApiTest, NewInstanceWithNonSequenceRaisesException) {
  PyObjectPtr type(PyStructSequence_NewType(&desc));
  ASSERT_NE(type, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyType_CheckExact(type));

  ASSERT_EQ(moduleSet("__main__", "Structseq", type), 0);
  // TODO(T40700664): Use PyRun_String and test for raised exception
  EXPECT_EQ(PyRun_SimpleString(R"(
Structseq(1)
)"),
            -1);
}

TEST_F(StructSeqExtensionApiTest,
       NewInstanceWithMoreThanMaxSizeRaisesException) {
  PyObjectPtr type(PyStructSequence_NewType(&desc));
  ASSERT_NE(type, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyType_CheckExact(type));

  ASSERT_EQ(moduleSet("__main__", "Structseq", type), 0);
  // TODO(T40700664): Use PyRun_String and test for raised exception
  EXPECT_EQ(PyRun_SimpleString(R"(
Structseq((1,2,3,4,5,6))
)"),
            -1);
}

TEST_F(StructSeqExtensionApiTest, NewInstanceWithMinLenReturnsValue) {
  PyObjectPtr type(PyStructSequence_NewType(&desc));
  ASSERT_NE(type, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyType_CheckExact(type));

  ASSERT_EQ(moduleSet("__main__", "Structseq", type), 0);
  PyRun_SimpleString(R"(
result = Structseq((1,2))
)");
  PyObjectPtr result(moduleGet("__main__", "result"));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyTuple_Check(result));

  PyObjectPtr value(PyStructSequence_GetItem(result, 1));
  ASSERT_TRUE(PyLong_Check(value));
  EXPECT_EQ(PyLong_AsLong(value), 2);
}

TEST_F(StructSeqExtensionApiTest, NewInstanceWithLargerThanMinLenReturnsValue) {
  PyObjectPtr type(PyStructSequence_NewType(&desc));
  ASSERT_NE(type, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyType_CheckExact(type));

  ASSERT_EQ(moduleSet("__main__", "Structseq", type), 0);
  PyRun_SimpleString(R"(
result = Structseq((1,2,3))
)");
  PyObjectPtr result(moduleGet("__main__", "result"));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyTuple_Check(result));

  PyObjectPtr value(PyStructSequence_GetItem(result, 2));
  ASSERT_TRUE(PyLong_Check(value));
  EXPECT_EQ(PyLong_AsLong(value), 3);
}

TEST_F(StructSeqExtensionApiTest, NewInstanceWithDictReturnsValue) {
  PyObjectPtr type(PyStructSequence_NewType(&desc));
  ASSERT_NE(type, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyType_CheckExact(type));

  ASSERT_EQ(moduleSet("__main__", "Structseq", type), 0);
  PyRun_SimpleString(R"(
result = Structseq((1,2), {"third": 3})
)");
  PyObjectPtr result(moduleGet("__main__", "result"));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyTuple_Check(result));

  PyObjectPtr value(PyStructSequence_GetItem(result, 2));
  ASSERT_TRUE(PyLong_Check(value));
  EXPECT_EQ(PyLong_AsLong(value), 3);
}

TEST_F(StructSeqExtensionApiTest, NewInstanceWithOverrideIgnoresValue) {
  PyObjectPtr type(PyStructSequence_NewType(&desc));
  ASSERT_NE(type, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyType_CheckExact(type));

  ASSERT_EQ(moduleSet("__main__", "Structseq", type), 0);
  PyRun_SimpleString(R"(
result = Structseq((1,2), {"first": 5})
)");
  PyObjectPtr result(moduleGet("__main__", "result"));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyTuple_Check(result));

  PyObjectPtr value(PyStructSequence_GetItem(result, 0));
  ASSERT_TRUE(PyLong_Check(value));
  EXPECT_EQ(PyLong_AsLong(value), 1);
}

TEST_F(StructSeqExtensionApiTest, GetItem) {
  PyObjectPtr type(PyStructSequence_NewType(&desc));
  ASSERT_NE(type, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyType_CheckExact(type));

  ASSERT_EQ(moduleSet("__main__", "Structseq", type), 0);
  PyRun_SimpleString(R"(
result = Structseq((1,2))
)");
  PyObjectPtr result(moduleGet("__main__", "result"));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyTuple_Check(result));

  PyObjectPtr value(PyStructSequence_GetItem(result, 1));
  ASSERT_TRUE(PyLong_Check(value));
  EXPECT_EQ(PyLong_AsLong(value), 2);

  PyObjectPtr value2(PyObject_GetAttrString(result, "second"));
  ASSERT_TRUE(PyLong_Check(value2));
  EXPECT_EQ(PyLong_AsLong(value2), 2);

  EXPECT_EQ(value.get(), value2.get());
}

TEST_F(StructSeqExtensionApiTest, GetItemWithIndexReturnsValue) {
  PyObjectPtr type(PyStructSequence_NewType(&desc));
  ASSERT_NE(type, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyType_CheckExact(type));

  ASSERT_EQ(moduleSet("__main__", "Structseq", type), 0);
  PyRun_SimpleString(R"(
result = Structseq((1,2))[0]
)");
  PyObjectPtr result(moduleGet("__main__", "result"));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyLong_Check(result));
  EXPECT_EQ(PyLong_AsLong(result), 1);
}

TEST_F(StructSeqExtensionApiTest,
       GetItemWithIndexToHiddenValueRaisesException) {
  PyObjectPtr type(PyStructSequence_NewType(&desc));
  ASSERT_NE(type, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyType_CheckExact(type));

  ASSERT_EQ(moduleSet("__main__", "Structseq", type), 0);
  // TODO(T40700664): Use PyRun_String and test for raised exception
  EXPECT_EQ(PyRun_SimpleString(R"(
Structseq((1,2,3))[2]
)"),
            -1);
}

TEST_F(StructSeqExtensionApiTest, GetItemWithNameReturnsValue) {
  PyObjectPtr type(PyStructSequence_NewType(&desc));
  ASSERT_NE(type, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyType_CheckExact(type));

  ASSERT_EQ(moduleSet("__main__", "Structseq", type), 0);
  PyRun_SimpleString(R"(
result = Structseq((1,2)).first
)");
  PyObjectPtr result(moduleGet("__main__", "result"));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyLong_Check(result));
  EXPECT_EQ(PyLong_AsLong(result), 1);
}

TEST_F(StructSeqExtensionApiTest, GetItemWithNameToHiddenValueReturnsValue) {
  PyObjectPtr type(PyStructSequence_NewType(&desc));
  ASSERT_NE(type, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyType_CheckExact(type));

  ASSERT_EQ(moduleSet("__main__", "Structseq", type), 0);
  PyRun_SimpleString(R"(
result = Structseq((1,2,3)).third
)");
  PyObjectPtr result(moduleGet("__main__", "result"));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyLong_Check(result));
  EXPECT_EQ(PyLong_AsLong(result), 3);
}

TEST_F(StructSeqExtensionApiTest,
       GetItemWithNameToUnsetHiddenValueReturnsValue) {
  PyObjectPtr type(PyStructSequence_NewType(&desc));
  ASSERT_NE(type, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyType_CheckExact(type));

  ASSERT_EQ(moduleSet("__main__", "Structseq", type), 0);
  PyRun_SimpleString(R"(
result = Structseq((1,2,3)).fifth
)");
  PyObjectPtr result(moduleGet("__main__", "result"));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(result, Py_None);
}

TEST_F(StructSeqExtensionApiTest, GetItemWithDictAndInvalidFieldReturnsValue) {
  PyObjectPtr type(PyStructSequence_NewType(&desc));
  ASSERT_NE(type, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyType_CheckExact(type));

  ASSERT_EQ(moduleSet("__main__", "Structseq", type), 0);
  PyRun_SimpleString(R"(
result = Structseq((1,2), {"badattr": 3}).first
)");
  PyObjectPtr result(moduleGet("__main__", "result"));
  ASSERT_TRUE(PyLong_Check(result));
  EXPECT_EQ(PyLong_AsLong(result), 1);
}

TEST_F(StructSeqExtensionApiTest,
       GetItemFromDictWithInvalidFieldRaisesException) {
  PyObjectPtr type(PyStructSequence_NewType(&desc));
  ASSERT_NE(type, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyType_CheckExact(type));

  ASSERT_EQ(moduleSet("__main__", "Structseq", type), 0);
  // TODO(T40700664): Use PyRun_String and test for raised exception
  EXPECT_EQ(PyRun_SimpleString(R"(
Structseq((1,2), {"badattr": 3}).badattr
)"),
            -1);
}

TEST_F(StructSeqExtensionApiTest, LenReturnsVisibleSize) {
  PyObjectPtr type(PyStructSequence_NewType(&desc));
  ASSERT_NE(type, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyType_CheckExact(type));

  ASSERT_EQ(moduleSet("__main__", "Structseq", type), 0);
  PyRun_SimpleString(R"(
result = len(Structseq((1,2,3)))
)");
  PyObjectPtr result(moduleGet("__main__", "result"));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyLong_Check(result));
  EXPECT_EQ(PyLong_AsLong(result), 2);
}

TEST_F(StructSeqExtensionApiTest, IterReturnsVisibleItems) {
  PyObjectPtr type(PyStructSequence_NewType(&desc));
  ASSERT_NE(type, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyType_CheckExact(type));

  ASSERT_EQ(moduleSet("__main__", "Structseq", type), 0);
  PyRun_SimpleString(R"(
structseq = Structseq((1,2,3,4,5))
result = [x for x in structseq]
)");
  PyObjectPtr result(moduleGet("__main__", "result"));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyList_Check(result));
  EXPECT_EQ(PyList_Size(result), 2);
  EXPECT_EQ(PyLong_AsLong(PyList_GetItem(result, 0)), 1);
  EXPECT_EQ(PyLong_AsLong(PyList_GetItem(result, 1)), 2);
}

TEST_F(StructSeqExtensionApiTest, ReprPyro) {
  // TODO(T40273054): Pyro only test, test the field names as well
  PyObjectPtr type(PyStructSequence_NewType(&desc));
  ASSERT_NE(type, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyType_CheckExact(type));

  ASSERT_EQ(moduleSet("__main__", "Structseq", type), 0);
  PyRun_SimpleString(R"(
result = Structseq((1,2,3)).__repr__()
)");

  PyObjectPtr result(moduleGet("__main__", "result"));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(isUnicodeEqualsCStr(result, "Structseq(1, 2)"));
}

TEST_F(StructSeqExtensionApiTest, SetItemRaisesException) {
  PyObjectPtr type(PyStructSequence_NewType(&desc));
  ASSERT_NE(type, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyType_CheckExact(type));

  ASSERT_EQ(moduleSet("__main__", "Structseq", type), 0);
  // TODO(T40700664): Use PyRun_String and test for raised exception
  EXPECT_EQ(PyRun_SimpleString(R"(
structseq = Structseq((1,2,3))
structseq.first = 4
)"),
            -1);
}

TEST_F(StructSeqExtensionApiTest, TupleSizeReturnsVisibleSize) {
  PyObjectPtr type(PyStructSequence_NewType(&desc));
  ASSERT_NE(type, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyType_CheckExact(type));

  PyObjectPtr instance(PyStructSequence_New(type.asTypeObject()));
  ASSERT_TRUE(PyTuple_Check(instance));
  EXPECT_EQ(PyTuple_Size(instance), 2);
}

TEST_F(StructSeqExtensionApiTest, GetItemReturnsValue) {
  PyObjectPtr type(PyStructSequence_NewType(&desc));
  ASSERT_NE(type, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyType_CheckExact(type));

  PyObjectPtr instance(PyStructSequence_New(type.asTypeObject()));
  ASSERT_TRUE(PyTuple_Check(instance));

  PyObject* value = PyLong_FromLong(123);  // reference will be stolen
  EXPECT_EQ(PyStructSequence_SET_ITEM(instance.get(), 0, value), value);
  ASSERT_EQ(PyErr_Occurred(), nullptr);

  PyObjectPtr result(PyStructSequence_GET_ITEM(instance.get(), 0));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyLong_AsLong(result), 123);
}

TEST_F(StructSeqExtensionApiTest,
       GetItemFromUninitializedFieldReturnsNonePyro) {
  // Pyro only test as CPython initializes these to nullptr
  PyObjectPtr type(PyStructSequence_NewType(&desc));
  ASSERT_NE(type, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyType_CheckExact(type));

  PyObjectPtr instance(PyStructSequence_New(type.asTypeObject()));
  ASSERT_TRUE(PyTuple_Check(instance));

  PyObjectPtr result(PyStructSequence_GET_ITEM(instance.get(), 0));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(result, Py_None);
}

TEST_F(StructSeqExtensionApiTest, GetItemHiddenFieldReturnsValue) {
  PyObjectPtr type(PyStructSequence_NewType(&desc));
  ASSERT_NE(type, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyType_CheckExact(type));

  PyObjectPtr instance(PyStructSequence_New(type.asTypeObject()));
  ASSERT_TRUE(PyTuple_Check(instance));

  PyStructSequence_SetItem(instance, 4, PyLong_FromLong(123));
  ASSERT_EQ(PyErr_Occurred(), nullptr);

  PyObjectPtr result(PyStructSequence_GetItem(instance.get(), 4));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyLong_AsLong(result), 123);
}

TEST_F(StructSeqExtensionApiTest, GetNamedItemReturnsValue) {
  PyObjectPtr type(PyStructSequence_NewType(&desc));
  ASSERT_NE(type, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyType_CheckExact(type));

  PyObjectPtr instance(PyStructSequence_New(type.asTypeObject()));
  ASSERT_TRUE(PyTuple_Check(instance));

  PyStructSequence_SetItem(instance, 0, PyLong_FromLong(123));
  ASSERT_EQ(PyErr_Occurred(), nullptr);

  PyObjectPtr result(PyObject_GetAttrString(instance, "first"));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyLong_AsLong(result), 123);
}

TEST_F(StructSeqExtensionApiTest,
       GetNamedItemFromUninitializedFieldReturnsNone) {
  PyObjectPtr type(PyStructSequence_NewType(&desc));
  ASSERT_NE(type, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyType_CheckExact(type));

  PyObjectPtr instance(PyStructSequence_New(type.asTypeObject()));
  ASSERT_TRUE(PyTuple_Check(instance));

  PyObjectPtr result(PyObject_GetAttrString(instance, "first"));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(result, Py_None);
}

TEST_F(StructSeqExtensionApiTest, GetItemWithInvalidIndexRaisesIndexErrorPyro) {
  // This is a Pyro only test as CPython does an unchecked tuple access
  PyObjectPtr type(PyStructSequence_NewType(&desc));
  ASSERT_NE(type, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyType_CheckExact(type));

  PyObjectPtr instance(PyStructSequence_New(type.asTypeObject()));
  ASSERT_TRUE(PyTuple_Check(instance));

  ASSERT_EQ(PyStructSequence_GetItem(instance, 15), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_IndexError));
}

TEST_F(StructSeqExtensionApiTest, SetItemWithInvalidIndexRaisesIndexPyro) {
  // This is a Pyro only test as CPython does an unchecked tuple access
  PyObjectPtr type(PyStructSequence_NewType(&desc));
  ASSERT_NE(type, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyType_CheckExact(type));

  PyObjectPtr instance(PyStructSequence_New(type.asTypeObject()));
  ASSERT_TRUE(PyTuple_Check(instance));

  PyStructSequence_SetItem(instance, 15, PyLong_FromLong(123));
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_IndexError));
}

}  // namespace python
