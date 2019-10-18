#include "Python.h"

#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

using namespace testing;

using GetArgsExtensionApiTest = ExtensionApi;

TEST_F(GetArgsExtensionApiTest, ParseStackOneObject) {
  PyObjectPtr long10(PyLong_FromLong(10));
  PyObject* args[] = {long10};
  int nargs = Py_ARRAY_LENGTH(args);

  const char* const keywords[] = {"first", nullptr};
  static _PyArg_Parser parser = {"O:ParseStackOneObject", keywords};

  PyObject* kwnames = nullptr;
  PyObject* out = nullptr;

  EXPECT_EQ(_PyArg_ParseStack(args, nargs, kwnames, &parser, &out), 1);
  EXPECT_EQ(PyLong_AsLong(out), 10);
  _PyArg_Fini();
}

TEST_F(GetArgsExtensionApiTest, ParseStackWithLongKWNamesRaisesTypeError) {
  PyObjectPtr long10(PyLong_FromLong(10));
  PyObject* args[] = {long10};
  int nargs = Py_ARRAY_LENGTH(args);

  PyObject* kwnames = PyTuple_New(1);
  PyObject* in1 = PyLong_FromLong(37);
  ASSERT_NE(-1, PyTuple_SetItem(kwnames, 0, in1));

  const char* const keywords[] = {"first", "second", nullptr};
  static _PyArg_Parser parser = {"OO:ParseStackWithLongKWNamesRaisesTypeError",
                                 keywords};
  PyObject* out1 = nullptr;

  EXPECT_EQ(_PyArg_ParseStack(args, nargs, kwnames, &parser, &out1), 0);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
  EXPECT_EQ(PyLong_AsLong(out1), 10);
  _PyArg_Fini();
}

TEST_F(GetArgsExtensionApiTest, ParseStackMultipleObjects) {
  PyObjectPtr long10(PyLong_FromLong(10));
  PyObjectPtr long33(PyLong_FromLong(33));
  PyObjectPtr test_str(PyUnicode_FromString("test_str"));
  PyObject* args[] = {long10, long33, test_str};
  int nargs = Py_ARRAY_LENGTH(args);

  const char* const keywords[] = {"first", "second", "third", nullptr};
  static _PyArg_Parser parser = {"OOU:ParseStackMultipleObjects", keywords};

  PyObject* kwnames = nullptr;
  PyObject* out1 = nullptr;
  PyObject* out2 = nullptr;
  PyObject* out3 = nullptr;

  EXPECT_EQ(
      _PyArg_ParseStack(args, nargs, kwnames, &parser, &out1, &out2, &out3), 1);
  EXPECT_EQ(PyLong_AsLong(out1), 10);
  EXPECT_EQ(PyLong_AsLong(out2), 33);
  EXPECT_EQ(out3, test_str);
  _PyArg_Fini();
}

TEST_F(GetArgsExtensionApiTest, ParseStackUnicode) {
  PyObjectPtr hello(PyUnicode_FromString("hello"));
  PyObjectPtr world(PyUnicode_FromString("world"));
  PyObject* args[] = {hello, world};
  int nargs = Py_ARRAY_LENGTH(args);

  const char* const keywords[] = {"first", "second", nullptr};
  static _PyArg_Parser parser = {"UU:ParseStackUnicode", keywords};

  PyObject* kwnames = nullptr;
  PyObject* out1 = nullptr;
  PyObject* out2 = nullptr;
  EXPECT_EQ(_PyArg_ParseStack(args, nargs, kwnames, &parser, &out1, &out2), 1);
  EXPECT_EQ(hello, out1);
  EXPECT_EQ(world, out2);
  _PyArg_Fini();
}

TEST_F(GetArgsExtensionApiTest, ParseStackWithWrongTypeRaisesTypeError) {
  PyObjectPtr long100(PyLong_FromLong(100));
  PyObject* args[] = {long100};
  int nargs = Py_ARRAY_LENGTH(args);

  const char* const keywords[] = {"first", nullptr};
  _PyArg_Parser parser = {"U:ParseStackWithWrongTypeRaisesTypeError", keywords};

  PyObject* kwnames = nullptr;
  PyObject* out1 = nullptr;
  EXPECT_EQ(_PyArg_ParseStack(args, nargs, kwnames, &parser, &out1), 0);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
  EXPECT_EQ(out1, nullptr);
  _PyArg_Fini();
}

TEST_F(GetArgsExtensionApiTest, ParseStackString) {
  PyObjectPtr hello(PyUnicode_FromString("hello"));
  PyObjectPtr world(PyUnicode_FromString("world"));
  PyObject* args[] = {hello, world};
  int nargs = Py_ARRAY_LENGTH(args);

  const char* const keywords[] = {"first", "second", nullptr};
  _PyArg_Parser parser = {"sz:ParseStackString", keywords};

  PyObject* kwnames = nullptr;
  char* out1 = nullptr;
  char* out2 = nullptr;
  EXPECT_EQ(_PyArg_ParseStack(args, nargs, kwnames, &parser, &out1, &out2), 1);
  EXPECT_STREQ("hello", out1);
  EXPECT_STREQ("world", out2);
  _PyArg_Fini();
}

TEST_F(GetArgsExtensionApiTest, ParseTupleOneObject) {
  PyObjectPtr pytuple(PyTuple_New(1));
  PyObject* in = PyLong_FromLong(42);
  ASSERT_NE(-1, PyTuple_SetItem(pytuple, 0, in));

  long refcnt = Py_REFCNT(in);
  PyObject* out;
  EXPECT_TRUE(PyArg_ParseTuple(pytuple, "O:xyz", &out));
  // This returns a borrowed reference, verify ref count did not change
  EXPECT_EQ(Py_REFCNT(out), refcnt);
  EXPECT_EQ(42, PyLong_AsLong(out));
}

TEST_F(GetArgsExtensionApiTest, ParseTupleMultipleObjects) {
  PyObject* pytuple = PyTuple_New(3);
  PyObject* in1 = PyLong_FromLong(111);
  PyObject* in2 = Py_None;
  Py_INCREF(in2);
  PyObject* in3 = PyLong_FromLong(333);
  ASSERT_NE(-1, PyTuple_SetItem(pytuple, 0, in1));
  ASSERT_NE(-1, PyTuple_SetItem(pytuple, 1, in2));
  ASSERT_NE(-1, PyTuple_SetItem(pytuple, 2, in3));

  PyObject* out1;
  PyObject* out2;
  PyObject* out3;
  EXPECT_TRUE(PyArg_ParseTuple(pytuple, "OOO:xyz", &out1, &out2, &out3));
  EXPECT_EQ(111, PyLong_AsLong(out1));
  EXPECT_EQ(Py_None, out2);
  EXPECT_EQ(333, PyLong_AsLong(out3));
}

TEST_F(GetArgsExtensionApiTest, ParseTupleUnicodeObject) {
  PyObject* pytuple = PyTuple_New(1);
  PyObject* in1 = PyUnicode_FromString("pyro");
  ASSERT_NE(-1, PyTuple_SetItem(pytuple, 0, in1));

  PyObject* out1;
  EXPECT_TRUE(PyArg_ParseTuple(pytuple, "U:is_frozen", &out1));
  EXPECT_EQ(in1, out1);
}

TEST_F(GetArgsExtensionApiTest, ParseTupleWithWrongType) {
  PyObjectPtr pytuple(PyTuple_New(1));
  PyObject* in = PyLong_FromLong(42);
  ASSERT_NE(in, nullptr);
  ASSERT_NE(-1, PyTuple_SetItem(pytuple, 0, in));

  PyObject* out1 = nullptr;
  EXPECT_FALSE(PyArg_ParseTuple(pytuple, "U:is_frozen", &out1));
  EXPECT_NE(PyErr_Occurred(), nullptr);
  EXPECT_EQ(out1, nullptr);
}

TEST_F(GetArgsExtensionApiTest, ParseTupleString) {
  PyObject* pytuple = PyTuple_New(2);
  PyObject* in1 = PyUnicode_FromString("hello");
  PyObject* in2 = PyUnicode_FromString("world");
  ASSERT_NE(-1, PyTuple_SetItem(pytuple, 0, in1));
  ASSERT_NE(-1, PyTuple_SetItem(pytuple, 1, in2));

  char *out1, *out2;
  EXPECT_TRUE(PyArg_ParseTuple(pytuple, "sz", &out1, &out2));
  EXPECT_STREQ("hello", out1);
  EXPECT_STREQ("world", out2);
}

TEST_F(GetArgsExtensionApiTest, ParseTupleStringFromNone) {
  PyObjectPtr pytuple(PyTuple_New(2));
  Py_INCREF(Py_None);
  ASSERT_NE(-1, PyTuple_SetItem(pytuple, 0, Py_None));
  Py_INCREF(Py_None);
  ASSERT_NE(-1, PyTuple_SetItem(pytuple, 1, Py_None));

  char *out1, *out2;
  int size = 123;
  EXPECT_TRUE(PyArg_ParseTuple(pytuple, "zz#", &out1, &out2, &size));
  EXPECT_EQ(nullptr, out1);
  EXPECT_EQ(nullptr, out2);
  EXPECT_EQ(0, size);
}

TEST_F(GetArgsExtensionApiTest, ParseTupleStringWithSize) {
  PyObjectPtr pytuple(PyTuple_New(2));
  ASSERT_NE(-1, PyTuple_SetItem(pytuple, 0, PyUnicode_FromString("hello")));
  ASSERT_NE(-1, PyTuple_SetItem(pytuple, 1, PyUnicode_FromString("cpython")));

  char *out1, *out2;
  int size1, size2;
  EXPECT_TRUE(PyArg_ParseTuple(pytuple, "s#z#", &out1, &size1, &out2, &size2));
  EXPECT_STREQ("hello", out1);
  EXPECT_EQ(5, size1);
  EXPECT_STREQ("cpython", out2);
  EXPECT_EQ(7, size2);
}

TEST_F(GetArgsExtensionApiTest, ParseTupleNumbers) {
  const int k_ints = 11;
  PyObject* pytuple = PyTuple_New(k_ints);
  for (int i = 0; i < k_ints; i++) {
    ASSERT_EQ(0, PyTuple_SetItem(pytuple, i, PyLong_FromLong(123 + i)));
  }

  unsigned char nb, n_b;
  short int nh;
  unsigned short int n_h;
  int ni;
  unsigned int n_i;
  long int nl;
  unsigned long nk;
  long long n_l;
  unsigned long long n_k;
  Py_ssize_t nn;

  EXPECT_TRUE(PyArg_ParseTuple(pytuple, "bBhHiIlkLKn", &nb, &n_b, &nh, &n_h,
                               &ni, &n_i, &nl, &nk, &n_l, &n_k, &nn));
  EXPECT_EQ(123, nb);
  EXPECT_EQ(124, n_b);
  EXPECT_EQ(125, nh);
  EXPECT_EQ(126, n_h);
  EXPECT_EQ(127, ni);
  EXPECT_EQ(128U, n_i);
  EXPECT_EQ(129, nl);
  EXPECT_EQ(130UL, nk);
  EXPECT_EQ(131, n_l);
  EXPECT_EQ(132ULL, n_k);
  EXPECT_EQ(133, nn);
}

TEST_F(GetArgsExtensionApiTest, ParseTupleOptionalPresent) {
  PyObjectPtr pytuple(PyTuple_New(1));
  ASSERT_NE(-1, PyTuple_SetItem(pytuple, 0, PyLong_FromLong(111)));

  PyObject* out = nullptr;
  EXPECT_TRUE(PyArg_ParseTuple(pytuple, "|O", &out));
  ASSERT_NE(out, nullptr);
  EXPECT_EQ(111, PyLong_AsLong(out));
}

TEST_F(GetArgsExtensionApiTest, ParseTupleOptionalNotPresent) {
  PyObjectPtr pytuple(PyTuple_New(0));

  PyObject* out = nullptr;
  EXPECT_TRUE(PyArg_ParseTuple(pytuple, "|O", &out));
  ASSERT_EQ(out, nullptr);
}

TEST_F(GetArgsExtensionApiTest, ParseTupleObjectWithCorrectType) {
  PyObjectPtr pytuple(PyTuple_New(1));
  PyObject* in = PyLong_FromLong(111);
  PyTypeObject* type = Py_TYPE(in);
  ASSERT_NE(-1, PyTuple_SetItem(pytuple, 0, in));

  PyObject* out = nullptr;
  EXPECT_TRUE(PyArg_ParseTuple(pytuple, "O!", type, &out));

  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(111, PyLong_AsLong(out));
}

TEST_F(GetArgsExtensionApiTest, ParseTupleObjectWithIncorrectType) {
  PyObjectPtr pytuple(PyTuple_New(1));
  PyObject* in = PyLong_FromLong(111);
  PyTypeObject* type = Py_TYPE(pytuple);
  ASSERT_NE(-1, PyTuple_SetItem(pytuple, 0, in));

  PyObject* out = nullptr;
  EXPECT_FALSE(PyArg_ParseTuple(pytuple, "O!", type, &out));

  EXPECT_NE(PyErr_Occurred(), nullptr);
  EXPECT_EQ(out, nullptr);
}

TEST_F(GetArgsExtensionApiTest, ParseTupleObjectWithConverter) {
  using converter_func = void (*)(PyObject*, void*);
  converter_func converter = [](PyObject* ptr, void* out) {
    *static_cast<int*>(out) = 1 + PyLong_AsLong(ptr);
  };

  PyObjectPtr pytuple(PyTuple_New(1));
  ASSERT_NE(-1, PyTuple_SetItem(pytuple, 0, PyLong_FromLong(111)));

  int out = 0;
  EXPECT_TRUE(
      PyArg_ParseTuple(pytuple, "O&", converter, static_cast<void*>(&out)));
  EXPECT_EQ(out, 112);
}

TEST_F(GetArgsExtensionApiTest, OldStyleParseWithInt) {
  PyObjectPtr pylong(PyLong_FromLong(666));
  int n = 0;
  EXPECT_TRUE(PyArg_Parse(pylong, "i", &n));
  EXPECT_EQ(n, 666);
}

TEST_F(GetArgsExtensionApiTest, ParseTupleAndKeywordsParseFromDict) {
  PyObjectPtr args(PyTuple_New(0));
  PyObjectPtr kwargs(PyDict_New());
  PyObjectPtr key(PyUnicode_FromString("first"));
  PyObjectPtr value(PyLong_FromLong(42));
  ASSERT_EQ(PyDict_SetItem(kwargs, key, value), 0);

  const char* kwnames[] = {"first", nullptr};
  int out = -1;
  EXPECT_TRUE(PyArg_ParseTupleAndKeywords(args, kwargs, "i",
                                          const_cast<char**>(kwnames), &out));
  EXPECT_EQ(out, 42);
}

TEST_F(GetArgsExtensionApiTest, ParseTupleAndKeywordsParseFromTuple) {
  PyObjectPtr args(PyTuple_New(1));
  ASSERT_NE(-1, PyTuple_SetItem(args, 0, PyLong_FromLong(43)));
  PyObjectPtr kwargs(PyDict_New());

  const char* kwnames[] = {"first", nullptr};
  int out = -1;
  EXPECT_TRUE(PyArg_ParseTupleAndKeywords(args, kwargs, "i",
                                          const_cast<char**>(kwnames), &out));
  EXPECT_EQ(out, 43);
}

TEST_F(GetArgsExtensionApiTest, ParseTupleAndKeywordsParseFromTupleAndDict) {
  PyObjectPtr args(PyTuple_New(1));
  ASSERT_NE(-1, PyTuple_SetItem(args, 0, PyLong_FromLong(44)));
  PyObjectPtr kwargs(PyDict_New());
  PyObjectPtr key(PyUnicode_FromString("second"));
  PyObjectPtr value(PyLong_FromLong(45));
  ASSERT_EQ(PyDict_SetItem(kwargs, key, value), 0);

  const char* kwnames[] = {"first", "second", nullptr};
  int out1 = -1, out2 = -1;
  EXPECT_TRUE(PyArg_ParseTupleAndKeywords(
      args, kwargs, "ii", const_cast<char**>(kwnames), &out1, &out2));
  EXPECT_EQ(out1, 44);
  EXPECT_EQ(out2, 45);
}

TEST_F(GetArgsExtensionApiTest, ParseTupleAndKeywordsWithOptionals) {
  PyObjectPtr args(PyTuple_New(0));
  PyObjectPtr kwargs(PyDict_New());
  PyObjectPtr key(PyUnicode_FromString("second"));
  PyObjectPtr value(PyLong_FromLong(42));
  ASSERT_EQ(PyDict_SetItem(kwargs, key, value), 0);

  const char* kwnames[] = {"first", "second", nullptr};
  int out1 = -1, out2 = -1;
  EXPECT_TRUE(PyArg_ParseTupleAndKeywords(
      args, kwargs, "|ii", const_cast<char**>(kwnames), &out1, &out2));
  EXPECT_EQ(out1, -1);
  EXPECT_EQ(out2, 42);
}

#pragma push_macro("_PyArg_NoKeywords")
#undef _PyArg_NoKeywords
TEST_F(GetArgsExtensionApiTest, NoKeywordsWithNullptrReturnsTrue) {
  EXPECT_EQ(_PyArg_NoKeywords("", nullptr), 1);
}
#pragma pop_macro("_PyArg_NoKeywords")

#pragma push_macro("_PyArg_NoKeywords")
#undef _PyArg_NoKeywords
TEST_F(GetArgsExtensionApiTest, NoKeywordsWithEmptyDictReturnsTrue) {
  PyObjectPtr empty_dict(PyDict_New());
  EXPECT_EQ(_PyArg_NoKeywords("", empty_dict), 1);
}
#pragma pop_macro("_PyArg_NoKeywords")

#pragma push_macro("_PyArg_NoKeywords")
#undef _PyArg_NoKeywords
TEST_F(GetArgsExtensionApiTest, NoKeywordsWithNonDictRaisesSystemError) {
  PyObjectPtr not_a_dict(PyTuple_New(10));
  EXPECT_EQ(_PyArg_NoKeywords("", not_a_dict), 0);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}
#pragma pop_macro("_PyArg_NoKeywords")

#pragma push_macro("_PyArg_NoKeywords")
#undef _PyArg_NoKeywords
TEST_F(GetArgsExtensionApiTest, NoKeywordsWithNonEmptyDictRaisesTypeError) {
  PyObjectPtr non_empty_dict(PyDict_New());
  PyDict_SetItemString(non_empty_dict, "my key", PyTuple_New(0));
  EXPECT_EQ(_PyArg_NoKeywords("", non_empty_dict.get()), 0);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}
#pragma pop_macro("_PyArg_NoKeywords")

#pragma push_macro("_PyArg_NoPositional")
#undef _PyArg_NoPositional
TEST_F(GetArgsExtensionApiTest, NoPositionalWithNullptrReturnsTrue) {
  EXPECT_EQ(_PyArg_NoPositional("", nullptr), 1);
}
#pragma pop_macro("_PyArg_NoPositional")

#pragma push_macro("_PyArg_NoPositional")
#undef _PyArg_NoPositional
TEST_F(GetArgsExtensionApiTest, NoPositionalWitEmptyTupleReturnsTrue) {
  PyObjectPtr empty_tuple(PyTuple_New(0));
  EXPECT_EQ(_PyArg_NoPositional("", empty_tuple), 1);
}
#pragma pop_macro("_PyArg_NoPositional")

#pragma push_macro("_PyArg_NoPositional")
#undef _PyArg_NoPositional
TEST_F(GetArgsExtensionApiTest, NoPositionalWithNonTupleRaisesSystemError) {
  PyObjectPtr not_a_tuple(PyLong_FromLong(1));
  EXPECT_EQ(_PyArg_NoPositional("", not_a_tuple), 0);
  EXPECT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}
#pragma pop_macro("_PyArg_NoPositional")

#pragma push_macro("_PyArg_NoPositional")
#undef _PyArg_NoPositional
TEST_F(GetArgsExtensionApiTest, NoPositionalWithNonEmptyTupleRaisesTypeError) {
  PyObjectPtr non_empty_tuple(PyTuple_New(10));
  EXPECT_EQ(_PyArg_NoPositional("", non_empty_tuple), 0);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}
#pragma pop_macro("_PyArg_NoPositional")

}  // namespace python
