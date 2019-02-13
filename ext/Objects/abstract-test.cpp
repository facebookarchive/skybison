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

TEST_F(AbstractExtensionApiTest, PyMappingLengthOnNullRaisesSystemError) {
  EXPECT_EQ(PyMapping_Length(nullptr), -1);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
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

TEST_F(AbstractExtensionApiTest, PyMappingSizeOnNullRaisesSystemError) {
  EXPECT_EQ(PyMapping_Size(nullptr), -1);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest, PyNumberIndexOnIntReturnsSelf) {
  PyObject* pylong = PyLong_FromLong(666);
  EXPECT_EQ(pylong, PyNumber_Index(pylong));
}

TEST_F(AbstractExtensionApiTest, PyNumberBinaryOp) {
  PyObject* v = PyLong_FromLong(0x2A);  // 0b0010'1010
  PyObject* w = PyLong_FromLong(0x38);  // 0b0011'1000
  PyObject* result = PyNumber_Xor(v, w);
  EXPECT_TRUE(PyLong_CheckExact(result));
  EXPECT_EQ(PyLong_AsLong(result), 0x12);  // 0b0001'0010
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

TEST_F(AbstractExtensionApiTest, PyNumberIndexOnNullRaisesSystemError) {
  EXPECT_EQ(PyNumber_Index(nullptr), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
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

TEST_F(AbstractExtensionApiTest, PyObjectLengthOnNullRaisesSystemError) {
  EXPECT_EQ(PyObject_Length(nullptr), -1);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
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

TEST_F(AbstractExtensionApiTest, PyObjectSizeOnNullRaisesSystemError) {
  EXPECT_EQ(PyObject_Size(nullptr), -1);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest, PyObjectTypeOnNullRaisesSystemError) {
  EXPECT_EQ(PyObject_Type(nullptr), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
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

TEST_F(AbstractExtensionApiTest, PySequenceLengthOnNull) {
  EXPECT_EQ(PySequence_Length(nullptr), -1);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
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

TEST_F(AbstractExtensionApiTest, PySequenceSizeOnNull) {
  EXPECT_EQ(PySequence_Size(nullptr), -1);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
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

TEST_F(AbstractExtensionApiTest, PyIndexCheckWithIntReturnsTrue) {
  PyObjectPtr int_num(PyLong_FromLong(1));
  EXPECT_TRUE(PyIndex_Check(int_num.get()));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, PyIndexCheckWithFloatReturnsFalse) {
  PyObjectPtr float_num(PyFloat_FromDouble(1.1));
  EXPECT_FALSE(PyIndex_Check(float_num.get()));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest,
       PyIndexCheckWithUncallableDunderIndexReturnsTrue) {
  PyRun_SimpleString(R"(
class C:
  __index__ = None
idx = C()
  )");
  PyObjectPtr idx(moduleGet("__main__", "idx"));
  EXPECT_TRUE(PyIndex_Check(idx.get()));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, PyIndexCheckWithDunderIndexReturnsTrue) {
  PyRun_SimpleString(R"(
class C:
  def __index__(self):
    return 1
idx = C()
  )");
  PyObjectPtr idx(moduleGet("__main__", "idx"));
  EXPECT_TRUE(PyIndex_Check(idx.get()));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, PyNumberCheckWithFloatReturnsTrue) {
  PyObjectPtr float_num(PyFloat_FromDouble(1.1));
  EXPECT_EQ(PyNumber_Check(float_num), 1);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, PyNumberCheckWithIntReturnsTrue) {
  PyObjectPtr int_num(PyLong_FromLong(1));
  EXPECT_EQ(PyNumber_Check(int_num), 1);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, PyNumberCheckWithFloatSubclassReturnsTrue) {
  PyRun_SimpleString(R"(
class SubFloat(float):
  pass
sub = SubFloat()
  )");
  PyObjectPtr sub(moduleGet("__main__", "sub"));
  EXPECT_EQ(PyNumber_Check(sub), 1);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, PyNumberCheckWithDunderIntClassReturnsTrue) {
  PyRun_SimpleString(R"(
class DunderIntClass():
  def __int__(self):
    return 5
i = DunderIntClass()
  )");
  PyObjectPtr i(moduleGet("__main__", "i"));
  EXPECT_EQ(PyNumber_Check(i), 1);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, PyNumberCheckWithDunderFloatClassReturnsTrue) {
  PyRun_SimpleString(R"(
class DunderFloatClass():
  def __float__(self):
    return 5.0
f = DunderFloatClass()
  )");
  PyObjectPtr f(moduleGet("__main__", "f"));
  EXPECT_EQ(PyNumber_Check(f), 1);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, PyNumberCheckWithNonNumberReturnsFalse) {
  PyObjectPtr str(PyUnicode_FromString(""));
  EXPECT_EQ(PyNumber_Check(str), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, PyNumberCheckWithNullReturnsFalse) {
  EXPECT_EQ(PyNumber_Check(nullptr), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, GetIterWithNoDunderIterRaises) {
  PyRun_SimpleString(R"(
class C:
  pass
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  ASSERT_EQ(PyObject_GetIter(c), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, GetIterWithNonCallableDunderIterRaises) {
  PyRun_SimpleString(R"(
class C:
  __iter__ = 4
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  ASSERT_EQ(PyObject_GetIter(c), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, GetIterWithDunderIterReturningNonIterRaises) {
  PyRun_SimpleString(R"(
class C:
  def __iter__(self):
    return 4
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  ASSERT_EQ(PyObject_GetIter(c), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, GetIterPropagatesException) {
  PyRun_SimpleString(R"(
class C:
  def __iter__(self):
    raise ValueError("hi")
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  ASSERT_EQ(PyObject_GetIter(c), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_ValueError));
}

TEST_F(AbstractExtensionApiTest, PyIterNextReturnsNext) {
  PyObjectPtr tuple(PyTuple_Pack(3, PyLong_FromLong(1), PyLong_FromLong(2),
                                 PyLong_FromLong(3)));
  PyObjectPtr iter(PyObject_GetIter(tuple));
  ASSERT_NE(iter, nullptr);
  PyObjectPtr next(PyIter_Next(iter));
  ASSERT_NE(next, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyLong_AsLong(next), 1);
  next = PyIter_Next(iter);
  ASSERT_NE(next, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyLong_AsLong(next), 2);
  next = PyIter_Next(iter);
  ASSERT_NE(next, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyLong_AsLong(next), 3);
  next = PyIter_Next(iter);
  ASSERT_EQ(next, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, PyIterNextOnNonIterRaises) {
  ASSERT_EQ(PyObject_GetIter(Py_None), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, PyIterNextPropagatesException) {
  PyRun_SimpleString(R"(
class C:
  def __iter__(self):
    return self
  def __next__(self):
    raise ValueError("hi")

c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr iter(PyObject_GetIter(c));
  ASSERT_NE(iter, nullptr);
  PyObjectPtr next(PyIter_Next(iter));
  ASSERT_EQ(next, nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_ValueError));
}

TEST_F(AbstractExtensionApiTest, PySequenceConcatWithNullLeftRaises) {
  PyObjectPtr tuple(PyTuple_New(0));
  PyObjectPtr result(PySequence_Concat(nullptr, tuple));
  ASSERT_EQ(result, nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest, PySequenceConcatWithNullRightRaises) {
  PyObjectPtr tuple(PyTuple_New(0));
  PyObjectPtr result(PySequence_Concat(tuple, nullptr));
  ASSERT_EQ(result, nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest, PySequenceConcatCallsDunderAdd) {
  PyObjectPtr left(PyTuple_Pack(2, PyLong_FromLong(1), PyLong_FromLong(2)));
  PyObjectPtr right(PyTuple_Pack(2, PyLong_FromLong(3), PyLong_FromLong(4)));
  PyObjectPtr result(PySequence_Concat(left, right));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(PyTuple_CheckExact(result));
  ASSERT_EQ(PyTuple_Size(result), 4);
  EXPECT_EQ(PyTuple_GetItem(result, 0), PyTuple_GetItem(left, 0));
  EXPECT_EQ(PyTuple_GetItem(result, 1), PyTuple_GetItem(left, 1));
  EXPECT_EQ(PyTuple_GetItem(result, 2), PyTuple_GetItem(right, 0));
  EXPECT_EQ(PyTuple_GetItem(result, 3), PyTuple_GetItem(right, 1));
}

TEST_F(AbstractExtensionApiTest, PySequenceRepeatWithNullSeqRaises) {
  PyObjectPtr result(PySequence_Repeat(nullptr, 5));
  ASSERT_EQ(result, nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest, PySequenceRepeatCallsDunderMul) {
  PyObjectPtr seq(PyTuple_Pack(2, PyLong_FromLong(1), PyLong_FromLong(2)));
  PyObjectPtr result(PySequence_Repeat(seq, 2));
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_EQ(PyTuple_Size(result), 4);
  EXPECT_EQ(PyTuple_GetItem(result, 0), PyTuple_GetItem(seq, 0));
  EXPECT_EQ(PyTuple_GetItem(result, 1), PyTuple_GetItem(seq, 1));
  EXPECT_EQ(PyTuple_GetItem(result, 2), PyTuple_GetItem(seq, 0));
  EXPECT_EQ(PyTuple_GetItem(result, 3), PyTuple_GetItem(seq, 1));
}

TEST_F(AbstractExtensionApiTest, PySequenceCountWithNullSeqRaises) {
  PyObjectPtr obj(PyLong_FromLong(1));
  EXPECT_EQ(PySequence_Count(nullptr, obj), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest, PySequenceCountWithNullObjRaises) {
  PyObjectPtr tuple(PyTuple_Pack(2, PyLong_FromLong(1), PyLong_FromLong(2)));
  EXPECT_EQ(PySequence_Count(tuple, nullptr), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest, PySequenceCountCountsOccurrences) {
  PyObjectPtr obj(PyLong_FromLong(2));
  PyObjectPtr tuple(PyTuple_Pack(3, PyLong_FromLong(1), PyLong_FromLong(2),
                                 PyLong_FromLong(2)));
  EXPECT_EQ(PySequence_Count(tuple, obj), 2);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, PySequenceGetItemCallsDunderGetItem) {
  PyRun_SimpleString(R"(
class C:
  def __getitem__(self, key):
    return 7
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr result(PySequence_GetItem(c, 0));
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(result, PyLong_FromLong(7));
}

TEST_F(AbstractExtensionApiTest, PySequenceSetItemWithNullValCallsDelItem) {
  PyRun_SimpleString(R"(
sideeffect = 0
class C:
  def __delitem__(self, key):
    global sideeffect
    sideeffect = 10
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  ASSERT_EQ(PySequence_SetItem(c, 0, nullptr), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  PyObjectPtr sideeffect(moduleGet("__main__", "sideeffect"));
  EXPECT_EQ(PyLong_AsLong(sideeffect), 10);
}

TEST_F(AbstractExtensionApiTest, PySequenceSetItemCallsDunderSetItem) {
  PyRun_SimpleString(R"(
sideeffect = 0
class C:
  def __setitem__(self, key, val):
    global sideeffect
    sideeffect = 10
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr val(PyLong_FromLong(4));
  ASSERT_EQ(PySequence_SetItem(c, 0, val), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  PyObjectPtr sideeffect(moduleGet("__main__", "sideeffect"));
  EXPECT_EQ(PyLong_AsLong(sideeffect), 10);
}

TEST_F(AbstractExtensionApiTest, PySequenceDelItemCallsDunderDelItem) {
  PyRun_SimpleString(R"(
sideeffect = 0
class C:
  def __delitem__(self, key):
    global sideeffect
    sideeffect = 10
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  ASSERT_EQ(PySequence_DelItem(c, 0), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  PyObjectPtr sideeffect(moduleGet("__main__", "sideeffect"));
  EXPECT_EQ(PyLong_AsLong(sideeffect), 10);
}

TEST_F(AbstractExtensionApiTest, PySequenceContainsCallsDunderContains) {
  PyRun_SimpleString(R"(
class C:
  def __getitem__(self, key):
    pass
  def __contains__(self, key):
    return True
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr key(PyLong_FromLong(3));
  ASSERT_EQ(PySequence_Contains(c, key), 1);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, PySequenceContainsPropagatesException) {
  PyRun_SimpleString(R"(
class C:
  def __getitem__(self, key):
    pass
  def __contains__(self, key):
    raise ValueError
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr key(PyLong_FromLong(3));
  ASSERT_EQ(PySequence_Contains(c, key), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_ValueError));
}

TEST_F(AbstractExtensionApiTest, PySequenceContainsFallsBackToIterSearch) {
  PyRun_SimpleString(R"(
class C:
  def __getitem__(self, key):
    pass
  def __iter__(self):
    return [1,2,3].__iter__()
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr key(PyLong_FromLong(4));
  ASSERT_EQ(PySequence_Contains(c, key), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, PySequenceIndexWithNullObjRaises) {
  PyObjectPtr tuple(PyTuple_Pack(2, PyLong_FromLong(1), PyLong_FromLong(2)));
  EXPECT_EQ(PySequence_Index(tuple, nullptr), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest, PySequenceIndexFindsFirstOccurrence) {
  PyObjectPtr obj(PyLong_FromLong(2));
  PyObjectPtr tuple(PyTuple_Pack(3, PyLong_FromLong(1), PyLong_FromLong(2),
                                 PyLong_FromLong(2)));
  EXPECT_EQ(PySequence_Index(tuple, obj), 1);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, PySequenceFastWithNullObjRaisesSystemError) {
  EXPECT_EQ(PySequence_Fast(nullptr, "msg"), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest, PySequenceFastWithTupleReturnsSameObject) {
  PyObjectPtr tuple(PyTuple_New(3));
  PyObjectPtr result(PySequence_Fast(tuple, "msg"));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(tuple, result);
}

TEST_F(AbstractExtensionApiTest, PySequenceFastWithListReturnsSameObject) {
  PyObjectPtr list(PyList_New(3));
  PyObjectPtr result(PySequence_Fast(list, "msg"));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(list, result);
}

TEST_F(AbstractExtensionApiTest, PySequenceFastWithNonIterableRaisesTypeError) {
  ASSERT_EQ(PySequence_Fast(Py_None, "msg"), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, PySequenceFastWithIterableReturnsList) {
  PyRun_SimpleString(R"(
class C:
  def __iter__(self):
    return (1, 2, 3).__iter__()
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr result(PySequence_Fast(c, "msg"));
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyList_CheckExact(result));
}

TEST_F(AbstractExtensionApiTest, PySequenceListWithNullSeqRaisesSystemError) {
  ASSERT_EQ(PySequence_List(nullptr), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest, PySequenceListWithNonIterableRaisesTypeError) {
  ASSERT_EQ(PySequence_List(Py_None), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, PySequenceListReturnsList) {
  PyRun_SimpleString(R"(
class C:
  def __iter__(self):
    return (1, 2, 3).__iter__()
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr result(PySequence_List(c));
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyList_CheckExact(result));
  EXPECT_EQ(PyList_Size(result), 3);
}

TEST_F(AbstractExtensionApiTest,
       PySequenceGetSliceWithNullSeqRaisesSystemError) {
  EXPECT_EQ(PySequence_GetSlice(nullptr, 1, 2), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest,
       PySequenceGetSliceWithNonIterableRaisesTypeError) {
  EXPECT_EQ(PySequence_GetSlice(Py_None, 1, 2), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, PySequenceGetSliceCallsDunderGetItem) {
  PyRun_SimpleString(R"(
class C:
  def __getitem__(self, key):
    return 7
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr result(PySequence_GetSlice(c, 1, 2));
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyLong_Check(result));
  EXPECT_EQ(PyLong_AsLong(result), 7);
}

TEST_F(AbstractExtensionApiTest,
       PySequenceSetSliceWithNullSeqRaisesSystemError) {
  PyObjectPtr obj(PyList_New(0));
  EXPECT_EQ(PySequence_SetSlice(nullptr, 1, 2, obj), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest,
       PySequenceSetSliceWithNonIterableRaisesTypeError) {
  PyObjectPtr obj(PyList_New(0));
  EXPECT_EQ(PySequence_SetSlice(Py_None, 1, 2, obj), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, PySequenceSetSliceCallsDunderSetItem) {
  PyRun_SimpleString(R"(
sideeffect = 0
class C:
  def __setitem__(self, key, value):
    global sideeffect
    sideeffect = 10
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr obj(PyList_New(0));
  ASSERT_EQ(PySequence_SetSlice(c, 1, 2, obj), 0);
  PyObjectPtr sideeffect(moduleGet("__main__", "sideeffect"));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyLong_AsLong(sideeffect), 10);
}

TEST_F(AbstractExtensionApiTest,
       PySequenceSetSliceWithNullObjCallsDunderDelItem) {
  PyRun_SimpleString(R"(
sideeffect = 0
class C:
  def __delitem__(self, key):
    global sideeffect
    sideeffect = 10
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  EXPECT_EQ(PySequence_SetSlice(c, 1, 2, nullptr), 0);
  PyObjectPtr sideeffect(moduleGet("__main__", "sideeffect"));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyLong_AsLong(sideeffect), 10);
}

TEST_F(AbstractExtensionApiTest,
       PySequenceDelSliceWithNullSeqRaisesSystemError) {
  PyObjectPtr obj(PyList_New(0));
  EXPECT_EQ(PySequence_DelSlice(nullptr, 1, 2), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest,
       PySequenceDelSliceWithNonIterableRaisesTypeError) {
  PyObjectPtr obj(PyList_New(0));
  EXPECT_EQ(PySequence_DelSlice(Py_None, 1, 2), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, PySequenceDelSliceCallsDunderDelItem) {
  PyRun_SimpleString(R"(
sideeffect = 0
class C:
  def __delitem__(self, key):
    global sideeffect
    sideeffect = 10
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  EXPECT_EQ(PySequence_DelSlice(c, 1, 2), 0);
  PyObjectPtr sideeffect(moduleGet("__main__", "sideeffect"));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyLong_AsLong(sideeffect), 10);
}

TEST_F(AbstractExtensionApiTest, PySequenceTupleWithNullSeqRaisesSystemError) {
  EXPECT_EQ(PySequence_Tuple(nullptr), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest,
       PySequenceTupleWithNonIterableRaisesTypeError) {
  EXPECT_EQ(PySequence_Tuple(Py_None), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, PySequenceTupleReturnsTuple) {
  PyRun_SimpleString(R"(
class C:
  def __iter__(self):
    return [1, 2, 3].__iter__()
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr result(PySequence_Tuple(c));
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyTuple_CheckExact(result));
  EXPECT_EQ(PyTuple_Size(result), 3);
}

TEST_F(AbstractExtensionApiTest, ObjectGetItemWithNullObjRaisesSystemError) {
  PyObjectPtr obj(PyLong_FromLong(1));
  EXPECT_EQ(PyObject_GetItem(nullptr, obj), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest, ObjectGetItemWithNullKeyRaisesSystemError) {
  PyObjectPtr obj(PyLong_FromLong(1));
  EXPECT_EQ(PyObject_GetItem(obj, nullptr), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest,
       ObjectGetItemWithNoDunderGetItemRaisesTypeError) {
  PyRun_SimpleString(R"(
class C:
  pass
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr key(PyLong_FromLong(7));
  EXPECT_EQ(PyObject_GetItem(c, key), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest,
       ObjectGetItemWithUncallableDunderGetItemRaisesTypeError) {
  PyRun_SimpleString(R"(
class C:
  __getitem__ = 4
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr key(PyLong_FromLong(7));
  EXPECT_EQ(PyObject_GetItem(c, key), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, ObjectGetItemCallsDunderGetItem) {
  PyRun_SimpleString(R"(
class C:
  def __getitem__(self, key):
    return key
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr key(PyLong_FromLong(7));
  EXPECT_EQ(PyObject_GetItem(c, key), key);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, ObjectGetItemPropagatesException) {
  PyRun_SimpleString(R"(
class C:
  def __getitem__(self, key):
    raise IndexError
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr key(PyLong_FromLong(7));
  EXPECT_EQ(PyObject_GetItem(c, key), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_IndexError));
}

TEST_F(AbstractExtensionApiTest,
       MappingGetItemStringWithNullObjRaisesSystemError) {
  EXPECT_EQ(PyMapping_GetItemString(nullptr, "hello"), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest,
       MappingGetItemStringWithNullKeyRaisesSystemError) {
  PyObjectPtr obj(PyLong_FromLong(1));
  EXPECT_EQ(PyMapping_GetItemString(obj, nullptr), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(AbstractExtensionApiTest,
       MappingGetItemStringWithNoDunderGetItemRaisesTypeError) {
  PyRun_SimpleString(R"(
class C:
  pass
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  EXPECT_EQ(PyMapping_GetItemString(c, "hello"), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest,
       MappingGetItemStringWithUncallableDunderGetItemRaisesTypeError) {
  PyRun_SimpleString(R"(
class C:
  __getitem__ = 4
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  EXPECT_EQ(PyMapping_GetItemString(c, "hello"), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(AbstractExtensionApiTest, MappingGetItemStringCallsDunderGetItem) {
  PyRun_SimpleString(R"(
class C:
  def __getitem__(self, key):
    return key
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  const char* key = "hello";
  PyObjectPtr result(PyMapping_GetItemString(c, key));
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(PyUnicode_Check(result));
  EXPECT_EQ(PyUnicode_CompareWithASCIIString(result, key), 0);
}

TEST_F(AbstractExtensionApiTest, MappingGetItemStringPropagatesException) {
  PyRun_SimpleString(R"(
class C:
  def __getitem__(self, key):
    raise IndexError
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  EXPECT_EQ(PyMapping_GetItemString(c, "hello"), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_IndexError));
}

TEST_F(AbstractExtensionApiTest, MappingHasKeyWithNullObjReturnsFalse) {
  PyObjectPtr obj(PyLong_FromLong(7));
  EXPECT_EQ(PyMapping_HasKey(nullptr, obj), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, MappingHasKeyWithNullKeyReturnsFalse) {
  PyObjectPtr obj(PyLong_FromLong(7));
  EXPECT_EQ(PyMapping_HasKey(obj, nullptr), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, MappingHasKeyCallsDunderGetItem) {
  PyRun_SimpleString(R"(
sideeffect = 0
class C:
  def __getitem__(self, key):
    global sideeffect
    sideeffect = 10
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr key(PyLong_FromLong(7));
  ASSERT_EQ(PyMapping_HasKey(c, key), 1);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  PyObjectPtr sideeffect(moduleGet("__main__", "sideeffect"));
  EXPECT_EQ(PyLong_AsLong(sideeffect), 10);
}

TEST_F(AbstractExtensionApiTest,
       MappingHasKeyReturnsFalseWhenExceptionIsRaised) {
  PyRun_SimpleString(R"(
class C:
  def __getitem__(self, key):
    raise IndexError
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr key(PyLong_FromLong(7));
  EXPECT_EQ(PyMapping_HasKey(c, key), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, MappingHasKeyStringWithNullObjReturnsFalse) {
  EXPECT_EQ(PyMapping_HasKeyString(nullptr, "hello"), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, MappingHasKeyStringWithNullKeyReturnsFalse) {
  PyObjectPtr obj(PyLong_FromLong(7));
  EXPECT_EQ(PyMapping_HasKeyString(obj, nullptr), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, MappingHasKeyStringCallsDunderGetItem) {
  PyRun_SimpleString(R"(
sideeffect = 0
class C:
  def __getitem__(self, key):
    global sideeffect
    sideeffect = 10
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  ASSERT_EQ(PyMapping_HasKeyString(c, "hello"), 1);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  PyObjectPtr sideeffect(moduleGet("__main__", "sideeffect"));
  EXPECT_EQ(PyLong_AsLong(sideeffect), 10);
}

TEST_F(AbstractExtensionApiTest,
       MappingHasKeyStringReturnsFalseWhenExceptionIsRaised) {
  PyRun_SimpleString(R"(
class C:
  def __getitem__(self, key):
    raise IndexError
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  EXPECT_EQ(PyMapping_HasKeyString(c, "hello"), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(AbstractExtensionApiTest, MappingKeysWithNoKeysRaisesAttributeError) {
  PyRun_SimpleString(R"(
class C:
  pass
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  EXPECT_EQ(PyMapping_Keys(c), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_AttributeError));
}

TEST_F(AbstractExtensionApiTest, MappingKeysCallsReturnsListOfKeys) {
  PyRun_SimpleString(R"(
class C:
  def keys(self):
    return ["hello", "world"]
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr result(PyMapping_Keys(c));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(PyList_Check(result));
  ASSERT_EQ(PyList_Size(result), 2);
  EXPECT_EQ(
      PyUnicode_CompareWithASCIIString(PyList_GetItem(result, 0), "hello"), 0);
  EXPECT_EQ(
      PyUnicode_CompareWithASCIIString(PyList_GetItem(result, 1), "world"), 0);
}

TEST_F(AbstractExtensionApiTest, MappingKeysCallsReturnsListOfKeysSequence) {
  PyRun_SimpleString(R"(
class C:
  def keys(self):
    return ("hello", "world").__iter__()
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr result(PyMapping_Keys(c));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(PyList_Check(result));
  ASSERT_EQ(PyList_Size(result), 2);
  EXPECT_EQ(
      PyUnicode_CompareWithASCIIString(PyList_GetItem(result, 0), "hello"), 0);
  EXPECT_EQ(
      PyUnicode_CompareWithASCIIString(PyList_GetItem(result, 1), "world"), 0);
}

TEST_F(AbstractExtensionApiTest, MappingKeysWithDictSubclassCallsKeys) {
  PyRun_SimpleString(R"(
class C(dict):
  def keys(self):
    return ("hello", "world").__iter__()
c = C()
c["a"] = 1
c["b"] = 2
c["c"] = 3
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr result(PyMapping_Keys(c));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(PyList_Check(result));
  ASSERT_EQ(PyList_Size(result), 2);
  EXPECT_EQ(
      PyUnicode_CompareWithASCIIString(PyList_GetItem(result, 0), "hello"), 0);
  EXPECT_EQ(
      PyUnicode_CompareWithASCIIString(PyList_GetItem(result, 1), "world"), 0);
}

TEST_F(AbstractExtensionApiTest, MappingItemsWithNoItemsRaisesAttributeError) {
  PyRun_SimpleString(R"(
class C:
  pass
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  EXPECT_EQ(PyMapping_Items(c), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_AttributeError));
}

TEST_F(AbstractExtensionApiTest, MappingItemsCallsReturnsListOfItems) {
  PyRun_SimpleString(R"(
class C:
  def items(self):
    return ["hello", "world"]
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr result(PyMapping_Items(c));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(PyList_Check(result));
  ASSERT_EQ(PyList_Size(result), 2);
  EXPECT_EQ(
      PyUnicode_CompareWithASCIIString(PyList_GetItem(result, 0), "hello"), 0);
  EXPECT_EQ(
      PyUnicode_CompareWithASCIIString(PyList_GetItem(result, 1), "world"), 0);
}

TEST_F(AbstractExtensionApiTest, MappingItemsCallsReturnsListOfItemsSequence) {
  PyRun_SimpleString(R"(
class C:
  def items(self):
    return ("hello", "world").__iter__()
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr result(PyMapping_Items(c));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(PyList_Check(result));
  ASSERT_EQ(PyList_Size(result), 2);
  EXPECT_EQ(
      PyUnicode_CompareWithASCIIString(PyList_GetItem(result, 0), "hello"), 0);
  EXPECT_EQ(
      PyUnicode_CompareWithASCIIString(PyList_GetItem(result, 1), "world"), 0);
}

TEST_F(AbstractExtensionApiTest, MappingItemsWithDictSubclassCallsItems) {
  PyRun_SimpleString(R"(
class C(dict):
  def items(self):
    return ("hello", "world").__iter__()
c = C()
c["a"] = 1
c["b"] = 2
c["c"] = 3
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr result(PyMapping_Items(c));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(PyList_Check(result));
  ASSERT_EQ(PyList_Size(result), 2);
  EXPECT_EQ(
      PyUnicode_CompareWithASCIIString(PyList_GetItem(result, 0), "hello"), 0);
  EXPECT_EQ(
      PyUnicode_CompareWithASCIIString(PyList_GetItem(result, 1), "world"), 0);
}

TEST_F(AbstractExtensionApiTest,
       MappingValuesWithNoValuesRaisesAttributeError) {
  PyRun_SimpleString(R"(
class C:
  pass
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  EXPECT_EQ(PyMapping_Values(c), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_AttributeError));
}

TEST_F(AbstractExtensionApiTest, MappingValuesCallsReturnsListOfValues) {
  PyRun_SimpleString(R"(
class C:
  def values(self):
    return ["hello", "world"]
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr result(PyMapping_Values(c));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(PyList_Check(result));
  ASSERT_EQ(PyList_Size(result), 2);
  EXPECT_EQ(
      PyUnicode_CompareWithASCIIString(PyList_GetItem(result, 0), "hello"), 0);
  EXPECT_EQ(
      PyUnicode_CompareWithASCIIString(PyList_GetItem(result, 1), "world"), 0);
}

TEST_F(AbstractExtensionApiTest,
       MappingValuesCallsReturnsListOfValuesSequence) {
  PyRun_SimpleString(R"(
class C:
  def values(self):
    return ("hello", "world").__iter__()
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr result(PyMapping_Values(c));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(PyList_Check(result));
  ASSERT_EQ(PyList_Size(result), 2);
  EXPECT_EQ(
      PyUnicode_CompareWithASCIIString(PyList_GetItem(result, 0), "hello"), 0);
  EXPECT_EQ(
      PyUnicode_CompareWithASCIIString(PyList_GetItem(result, 1), "world"), 0);
}

TEST_F(AbstractExtensionApiTest, MappingValuesWithDictSubclassCallsValues) {
  PyRun_SimpleString(R"(
class C(dict):
  def values(self):
    return ("hello", "world").__iter__()
c = C()
c["a"] = 1
c["b"] = 2
c["c"] = 3
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr result(PyMapping_Values(c));
  ASSERT_NE(result, nullptr);
  ASSERT_TRUE(PyList_Check(result));
  ASSERT_EQ(PyList_Size(result), 2);
  EXPECT_EQ(
      PyUnicode_CompareWithASCIIString(PyList_GetItem(result, 0), "hello"), 0);
  EXPECT_EQ(
      PyUnicode_CompareWithASCIIString(PyList_GetItem(result, 1), "world"), 0);
}

}  // namespace python
