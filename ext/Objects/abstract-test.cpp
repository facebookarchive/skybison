#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

using namespace testing;

using AbstractExtensionApiTest = ExtensionApi;

TEST_F(AbstractExtensionApiTest, PyMappingCheckWithoutGetItemReturnsFalse) {
  PyRun_SimpleString(R"(
class ClassWithoutDunderGetItem: pass

obj = ClassWithoutDunderGetItem()
  )");

  PyObjectPtr obj(moduleGet("__main__", "obj"));
  EXPECT_FALSE(PyMapping_Check(obj));
}

TEST_F(AbstractExtensionApiTest,
       PyMappingCheckWithoutGetItemOnClassReturnsFalse) {
  PyRun_SimpleString(R"(
class ClassWithoutDunderGetItem: pass

obj = ClassWithoutDunderGetItem()
obj.__getitem__ = lambda self, key: 1
  )");

  PyObjectPtr obj(moduleGet("__main__", "obj"));
  EXPECT_FALSE(PyMapping_Check(obj));
}

TEST_F(AbstractExtensionApiTest, PyMappingCheckWithNumericReturnsFalse) {
  PyObjectPtr num(PyLong_FromLong(4));
  EXPECT_FALSE(PyMapping_Check(num));
}

TEST_F(AbstractExtensionApiTest, PyMappingCheckWithSetReturnsFalse) {
  PyObjectPtr set(PySet_New(nullptr));
  EXPECT_FALSE(PyMapping_Check(set));
}

TEST_F(AbstractExtensionApiTest, PyMappingCheckWithBooleanReturnsFalse) {
  PyObjectPtr py_false(Py_False);
  EXPECT_FALSE(PyMapping_Check(py_false));
}

TEST_F(AbstractExtensionApiTest, PyMappingCheckWithGetItemMethodReturnsTrue) {
  PyRun_SimpleString(R"(
class ClassWithDunderGetItemMethod:
  def __getitem__(self, key):
    return None

obj = ClassWithDunderGetItemMethod()
  )");

  PyObjectPtr obj(moduleGet("__main__", "obj"));
  EXPECT_TRUE(PyMapping_Check(obj));
}

TEST_F(AbstractExtensionApiTest, PyMappingCheckWithGetItemAttrReturnsTrue) {
  PyRun_SimpleString(R"(
class ClassWithDunderGetItemAttr:
  __getitem__ = 42

obj = ClassWithDunderGetItemAttr()
  )");

  PyObjectPtr obj(moduleGet("__main__", "obj"));
  EXPECT_TRUE(PyMapping_Check(obj));
}

TEST_F(AbstractExtensionApiTest, PyMappingCheckWithStringReturnsTrue) {
  PyObjectPtr str(PyUnicode_FromString("foo"));
  EXPECT_TRUE(PyMapping_Check(str));
}

TEST_F(AbstractExtensionApiTest, PyMappingCheckWithListReturnsTrue) {
  PyObjectPtr list(PyList_New(3));
  EXPECT_TRUE(PyMapping_Check(list));
}

TEST_F(AbstractExtensionApiTest, PyMappingCheckWithDictReturnsTrue) {
  PyObjectPtr dict(PyDict_New());
  EXPECT_TRUE(PyMapping_Check(dict));
}

TEST_F(AbstractExtensionApiTest, PyNumberIndexOnIntReturnsSelf) {
  PyObject* pylong = PyLong_FromLong(666);
  EXPECT_EQ(pylong, PyNumber_Index(pylong));
}

TEST_F(AbstractExtensionApiTest, PyNumberBinaryOp) {
  PyObject* v = PyLong_FromLong(0x2A);  // 0b101010
  PyObject* w = PyLong_FromLong(0x38);  // 0b111000
  PyObject* result = PyNumber_Xor(v, w);
  EXPECT_TRUE(PyLong_CheckExact(result));
  EXPECT_EQ(PyLong_AsLong(result), 0x12);  // 0b010010
  Py_DECREF(v);
  Py_DECREF(w);
  Py_DECREF(result);
}

TEST_F(AbstractExtensionApiTest, PyNumberBinaryOpCallsDunderMethod) {
  PyRun_SimpleString(R"(
class ClassWithDunderAdd:
  def __add__(self, other):
    return "hello";

a = ClassWithDunderAdd()
  )");
  PyObject* a = testing::moduleGet("__main__", "a");
  PyObject* i = PyLong_FromLong(7);
  PyObject* result = PyNumber_Add(a, i);
  EXPECT_TRUE(PyUnicode_Check(result));
  EXPECT_STREQ(PyUnicode_AsUTF8(result), "hello");
  Py_DECREF(a);
  Py_DECREF(i);
  Py_DECREF(result);
}

TEST_F(AbstractExtensionApiTest, PyNumberBinaryOpWithInvalidArgsReturnsNull) {
  PyObject* v = PyLong_FromLong(1);
  PyObject* w = PyUnicode_FromString("pyro");
  PyObject* result = PyNumber_Subtract(v, w);
  ASSERT_EQ(result, nullptr);
  // TODO(T34841408): check the error message
  EXPECT_NE(PyErr_Occurred(), nullptr);
  Py_DECREF(v);
  Py_DECREF(w);
}

TEST_F(AbstractExtensionApiTest, PyNumberIndexCallsIndex) {
  PyRun_SimpleString(R"(
class IntLikeClass:
  def __index__(self):
    return 42;

i = IntLikeClass();
  )");
  PyObject* i = testing::moduleGet("__main__", "i");
  PyObject* index = PyNumber_Index(i);
  EXPECT_NE(index, nullptr);
  EXPECT_TRUE(PyLong_CheckExact(index));
  EXPECT_EQ(42, PyLong_AsLong(index));
}

TEST_F(AbstractExtensionApiTest, PyNumberIndexOnNonInt) {
  PyObject* str = PyUnicode_FromString("not an int");
  EXPECT_EQ(PyNumber_Index(str), nullptr);
  // TODO(T34841408): check the error message
  EXPECT_NE(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, PyNumberIndexWithIndexReturningNonInt) {
  PyRun_SimpleString(R"(
class IntLikeClass:
  def __index__(self):
    return "not an int";

i = IntLikeClass();
  )");
  PyObject* i = testing::moduleGet("__main__", "i");
  EXPECT_EQ(PyNumber_Index(i), nullptr);
  // TODO(T34841408): check the error message
  EXPECT_NE(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, PySequenceCheckWithoutGetItemReturnsFalse) {
  PyRun_SimpleString(R"(
class ClassWithoutDunderGetItem: pass

obj = ClassWithoutDunderGetItem()
  )");

  PyObjectPtr obj(moduleGet("__main__", "obj"));
  EXPECT_FALSE(PySequence_Check(obj));
}

TEST_F(AbstractExtensionApiTest,
       PySequenceCheckWithoutGetItemOnClassReturnsFalse) {
  PyRun_SimpleString(R"(
class ClassWithoutDunderGetItem: pass

obj = ClassWithoutDunderGetItem()
obj.__getitem__ = lambda self, key : 1
  )");

  PyObjectPtr obj(moduleGet("__main__", "obj"));
  EXPECT_FALSE(PySequence_Check(obj));
}

TEST_F(AbstractExtensionApiTest, PySequenceCheckWithNumericReturnsFalse) {
  PyObjectPtr num(PyLong_FromLong(3));
  EXPECT_FALSE(PySequence_Check(num));
}

TEST_F(AbstractExtensionApiTest, PySequenceCheckWithSetReturnsFalse) {
  PyObjectPtr set(PySet_New(nullptr));
  EXPECT_FALSE(PySequence_Check(set));
}

TEST_F(AbstractExtensionApiTest, PySequenceCheckWithDictReturnsFalse) {
  PyObjectPtr dict(PyDict_New());
  EXPECT_FALSE(PySequence_Check(dict));
}

TEST_F(AbstractExtensionApiTest, PySequenceCheckWithDictSubclassReturnsFalse) {
  PyRun_SimpleString(R"(
class Subclass(dict): pass

obj = Subclass()
)");

  PyObjectPtr obj(moduleGet("__main__", "obj"));
  EXPECT_FALSE(PySequence_Check(obj));
}

TEST_F(AbstractExtensionApiTest, PySequenceCheckWithNoneReturnsFalse) {
  PyObjectPtr py_none(Py_None);
  EXPECT_FALSE(PySequence_Check(py_none));
}

TEST_F(AbstractExtensionApiTest, PySequenceCheckWithGetItemMethodReturnsTrue) {
  PyRun_SimpleString(R"(
class ClassWithDunderGetItemMethod:
  def __getitem__(self, key):
    return None

obj = ClassWithDunderGetItemMethod()
  )");

  PyObjectPtr obj(moduleGet("__main__", "obj"));
  EXPECT_TRUE(PySequence_Check(obj));
}

TEST_F(AbstractExtensionApiTest, PySequenceCheckWithGetItemAttrReturnsTrue) {
  PyRun_SimpleString(R"(
class ClassWithDunderGetItemAttr:
  __getitem__ = 42

obj = ClassWithDunderGetItemAttr()
  )");

  PyObjectPtr obj(moduleGet("__main__", "obj"));
  EXPECT_TRUE(PySequence_Check(obj));
}

TEST_F(AbstractExtensionApiTest, PySequenceCheckWithStringReturnsTrue) {
  PyObjectPtr str(PyUnicode_FromString("foo"));
  EXPECT_TRUE(PySequence_Check(str));
}

TEST_F(AbstractExtensionApiTest, PySequenceCheckWithListReturnsTrue) {
  PyObjectPtr list(PyList_New(3));
  EXPECT_TRUE(PySequence_Check(list));
}

}  // namespace python
