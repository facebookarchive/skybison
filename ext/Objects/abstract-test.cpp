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

TEST_F(AbstractExtensionApiTest, PyMappingLengthWithNonMappingReturnsLen) {
  PyRun_SimpleString(R"(
class Foo:
  def __len__(self):
    return 1
obj = Foo()
  )");

  PyObjectPtr obj(moduleGet("__main__", "obj"));
  EXPECT_EQ(PyMapping_Length(obj), 1);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
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

TEST_F(AbstractExtensionApiTest, PyObjectCheckBufferWithBytesReturnsTrue) {
  PyObject* obj(PyBytes_FromString("foo"));
  EXPECT_TRUE(PyObject_CheckBuffer(obj));
  Py_DECREF(obj);
}

TEST_F(AbstractExtensionApiTest, PyObjectCheckBufferWithNonBytesReturnsFalse) {
  PyObject* obj(PyLong_FromLong(2));
  EXPECT_FALSE(PyObject_CheckBuffer(obj));
  Py_DECREF(obj);
}

TEST_F(AbstractExtensionApiTest,
       PyObjectLengthWithoutDunderLenRaisesTypeError) {
  PyObjectPtr num(PyLong_FromLong(3));
  ASSERT_EQ(PyObject_Length(num), -1);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, PyObjectLengthWithNonIntLenRaisesTypeError) {
  PyRun_SimpleString(R"(
class Foo:
  def __len__(self):
    return "foo"
obj = Foo()
  )");

  PyObjectPtr obj(moduleGet("__main__", "obj"));
  ASSERT_EQ(PyObject_Length(obj), -1);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, PyObjectLengthWithoutIndexRaisesTypeError) {
  PyRun_SimpleString(R"(
class Foo: pass
class Bar:
  def __len__(self): return Foo()
obj = Bar()
  )");

  PyObjectPtr obj(moduleGet("__main__", "obj"));
  ASSERT_EQ(PyObject_Length(obj), -1);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, PyObjectLengthWithNonIntIndexRaisesTypeError) {
  PyRun_SimpleString(R"(
class Foo:
  def __index__(self): return None
class Bar:
  def __len__(self): return Foo()
obj = Bar()
  )");

  PyObjectPtr obj(moduleGet("__main__", "obj"));
  ASSERT_EQ(PyObject_Length(obj), -1);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, PyObjectLengthWithIndexReturnsValue) {
  PyRun_SimpleString(R"(
class Foo:
  def __index__(self): return 1
class Bar:
  def __len__(self): return Foo()
obj = Bar()
  )");

  PyObjectPtr obj(moduleGet("__main__", "obj"));
  EXPECT_EQ(PyObject_Length(obj), 1);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest,
       PyObjectLengthWithNegativeLenRaisesValueError) {
  PyRun_SimpleString(R"(
class Foo:
  def __len__(self):
    return -5
obj = Foo()
  )");

  PyObjectPtr obj(moduleGet("__main__", "obj"));
  ASSERT_EQ(PyObject_Length(obj), -1);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_ValueError));
}

TEST_F(AbstractExtensionApiTest,
       PyObjectLengthWithOverflowRaisesOverflowError) {
  PyRun_SimpleString(R"(
class Foo:
  def __len__(self):
    return 0x8000000000000000
obj = Foo()
  )");

  PyObjectPtr obj(moduleGet("__main__", "obj"));
  ASSERT_EQ(PyObject_Length(obj), -1);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_OverflowError));
}

TEST_F(AbstractExtensionApiTest,
       PyObjectLengthWithUnderflowRaisesOverflowError) {
  PyRun_SimpleString(R"(
class Foo:
  def __len__(self):
    return -0x8000000000000001
obj = Foo()
  )");

  PyObjectPtr obj(moduleGet("__main__", "obj"));
  ASSERT_EQ(PyObject_Length(obj), -1);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_OverflowError));
}

TEST_F(AbstractExtensionApiTest, PyObjectLengthWithEmptyDictReturnsZero) {
  PyObjectPtr dict(PyDict_New());
  EXPECT_EQ(PyObject_Length(dict), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, PyObjectLengthWithEmptyListReturnsZero) {
  PyObjectPtr list(PyList_New(0));
  EXPECT_EQ(PyObject_Length(list), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, PyObjectLengthWithEmptyStringReturnsZero) {
  PyObjectPtr str(PyUnicode_FromString(""));
  EXPECT_EQ(PyObject_Length(str), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, PyObjectLengthWithNonEmptyDictReturnsValue) {
  PyObjectPtr dict(PyDict_New());

  {
    PyObjectPtr value(PyLong_FromLong(3));

    PyObjectPtr key1(PyLong_FromLong(1));
    PyDict_SetItem(dict, key1, value);

    PyObjectPtr key2(PyLong_FromLong(2));
    PyDict_SetItem(dict, key2, value);
  }

  EXPECT_EQ(PyObject_Length(dict), 2);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, PyObjectLengthWithNonEmptyListReturnsValue) {
  PyObjectPtr list(PyList_New(3));
  EXPECT_EQ(PyObject_Length(list), 3);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, PyObjectLengthWithNonEmptyStringReturnsValue) {
  PyObjectPtr str(PyUnicode_FromString("foo"));
  EXPECT_EQ(PyObject_Length(str), 3);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, PyObjectTypeReturnsType) {
  PyObjectPtr num(PyLong_FromLong(4));
  PyObjectPtr type(PyObject_Type(num));
  EXPECT_TRUE(PyType_Check(type));
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

TEST_F(AbstractExtensionApiTest, PySequenceLengthWithNonSequenceReturnsValue) {
  PyRun_SimpleString(R"(
class Foo:
  def __len__(self):
    return 1
obj = Foo()
  )");

  PyObjectPtr obj(moduleGet("__main__", "obj"));
  EXPECT_EQ(PySequence_Length(obj), 1);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

// PySequence_Length fails on `dict` in CPython, but succeeds on subclasses
TEST_F(AbstractExtensionApiTest,
       PySequenceLengthWithEmptyDictSubclassReturnsZero) {
  PyRun_SimpleString(R"(
class Foo(dict):
  pass
obj = Foo()
  )");

  PyObjectPtr obj(moduleGet("__main__", "obj"));
  EXPECT_EQ(PySequence_Length(obj), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest,
       PySequenceLengthWithNonEmptyDictSubclassReturnsValue) {
  PyRun_SimpleString(R"(
class Foo(dict):
  pass
obj = Foo()
  )");

  PyObjectPtr obj(moduleGet("__main__", "obj"));
  PyObjectPtr one(PyLong_FromLong(1));
  PyObjectPtr two(PyLong_FromLong(2));

  ASSERT_EQ(PyDict_SetItem(obj, one, two), 0);
  ASSERT_EQ(PyDict_SetItem(obj, two, one), 0);

  EXPECT_EQ(PySequence_Length(obj), 2);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, PyObjCallFunctionObjArgsWithNullReturnsNull) {
  testing::PyObjectPtr test(PyObject_CallFunctionObjArgs(nullptr, nullptr));
  EXPECT_EQ(nullptr, test);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest,
       PyObjCallFunctionObjArgsWithNonFunctionReturnsNull) {
  testing::PyObjectPtr non_func(PyTuple_New(0));
  testing::PyObjectPtr test(PyObject_CallFunctionObjArgs(non_func, nullptr));
  EXPECT_EQ(test, nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, CallFunctionObjArgsWithNoArgsReturnsValue) {
  PyRun_SimpleString(R"(
def func():
  return 5
  )");

  testing::PyObjectPtr func(testing::moduleGet("__main__", "func"));
  testing::PyObjectPtr func_result(PyObject_CallFunctionObjArgs(func, nullptr));
  EXPECT_EQ(PyLong_AsLong(func_result), 5);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest,
       CallFunctionObjArgsWithCallableClassReturnsValue) {
  PyRun_SimpleString(R"(
class Foo():
  def __call__(self):
    return 5
f = Foo()
  )");

  testing::PyObjectPtr f(testing::moduleGet("__main__", "f"));
  testing::PyObjectPtr f_result(PyObject_CallFunctionObjArgs(f, nullptr));
  EXPECT_EQ(PyLong_AsLong(f_result), 5);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest,
       CallFunctionObjArgsWithManyArgumentsReturnsValue) {
  PyRun_SimpleString(R"(
def func(a, b, c, d, e, f):
  return a + b + c + d + e + f
  )");

  testing::PyObjectPtr func(testing::moduleGet("__main__", "func"));
  PyObject* one = PyLong_FromLong(1);
  PyObject* two = PyLong_FromLong(2);
  testing::PyObjectPtr func_result(PyObject_CallFunctionObjArgs(
      func, one, one, two, two, one, two, nullptr));
  Py_DECREF(one);
  Py_DECREF(two);
  EXPECT_EQ(PyLong_AsLong(func_result), 9);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

}  // namespace python
