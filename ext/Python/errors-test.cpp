#include <cerrno>

#include "gtest/gtest.h"

#include "gmock/gmock-matchers.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace py {

using namespace testing;
using ErrorsExtensionApiTest = ExtensionApi;

TEST_F(ErrorsExtensionApiTest, CompareErrorMessageOnThread) {
  ASSERT_EQ(nullptr, PyErr_Occurred());

  PyErr_SetString(PyExc_Exception, "An exception occured");
  ASSERT_EQ(PyExc_Exception, PyErr_Occurred());
}

TEST_F(ErrorsExtensionApiTest, SetObjectSetsTypeAndValue) {
  PyObject* type = nullptr;
  PyObject* value = nullptr;
  PyObject* traceback = nullptr;
  PyErr_Fetch(&type, &value, &traceback);
  EXPECT_EQ(type, nullptr);
  EXPECT_EQ(value, nullptr);
  EXPECT_EQ(traceback, nullptr);

  PyErr_SetObject(PyExc_Exception, Py_True);
  PyErr_Fetch(&type, &value, &traceback);
  EXPECT_EQ(type, PyExc_Exception);
  EXPECT_EQ(value, Py_True);
  EXPECT_EQ(traceback, nullptr);

  Py_DECREF(type);
  Py_DECREF(value);
}

TEST_F(ErrorsExtensionApiTest, SetObjectWithNonExceptionTypeRaisesSystemError) {
  PyObjectPtr bool_type(PyObject_Type(Py_True));
  PyErr_SetObject(bool_type, Py_None);
  EXPECT_EQ(PyErr_Occurred(), PyExc_SystemError);
}

TEST_F(ErrorsExtensionApiTest, SetObjectWithNonTypeRaisesSystemError) {
  PyErr_SetObject(Py_True, Py_None);
  EXPECT_EQ(PyErr_Occurred(), PyExc_SystemError);
}

TEST_F(ErrorsExtensionApiTest, ClearClearsExceptionState) {
  // Set the exception state
  Py_INCREF(PyExc_Exception);
  Py_INCREF(Py_True);
  PyErr_Restore(PyExc_Exception, Py_True, nullptr);

  // Check that an exception is pending
  EXPECT_EQ(PyErr_Occurred(), PyExc_Exception);

  // Clear the exception
  PyErr_Clear();

  // Read the exception state again and check for null
  PyObject* type = nullptr;
  PyObject* value = nullptr;
  PyObject* traceback = nullptr;
  PyErr_Fetch(&type, &value, &traceback);
  EXPECT_EQ(type, nullptr);
  EXPECT_EQ(value, nullptr);
  EXPECT_EQ(traceback, nullptr);
}

TEST_F(ErrorsExtensionApiTest, BadArgumentRaisesTypeError) {
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyErr_BadArgument(), 0);

  PyObject* type = nullptr;
  PyObject* value = nullptr;
  PyObject* traceback = nullptr;
  PyErr_Fetch(&type, &value, &traceback);
  EXPECT_EQ(type, PyExc_TypeError);

  PyObjectPtr message(
      PyUnicode_FromString("bad argument type for built-in operation"));
  ASSERT_TRUE(PyUnicode_Check(message));
  EXPECT_EQ(_PyUnicode_EQ(value, message), 1);

  // TODO(T42241510): Traceback support isn't implemented yet. Once it's ready,
  // inspect the traceback here.

  Py_DECREF(type);
  Py_DECREF(value);
}

TEST_F(ErrorsExtensionApiTest, NewExceptionWithBadNameRaisesSystemError) {
  EXPECT_EQ(PyErr_NewException("NameWithoutADot", nullptr, nullptr), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(ErrorsExtensionApiTest, NewExceptionWithoutDictOrBaseReturnsType) {
  PyObjectPtr type(PyErr_NewException("Module.Name", nullptr, nullptr));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyType_CheckExact(type));
  EXPECT_TRUE(
      PyType_IsSubtype(reinterpret_cast<PyTypeObject*>(type.get()),
                       reinterpret_cast<PyTypeObject*>(PyExc_Exception)));

  PyObjectPtr name(PyObject_GetAttrString(type, "__name__"));
  ASSERT_TRUE(PyUnicode_CheckExact(name));
  EXPECT_TRUE(isUnicodeEqualsCStr(name, "Name"));
  PyObjectPtr module_name(PyObject_GetAttrString(type, "__module__"));
  ASSERT_TRUE(PyUnicode_CheckExact(module_name));
  EXPECT_TRUE(isUnicodeEqualsCStr(module_name, "Module"));
}

TEST_F(ErrorsExtensionApiTest, NewExceptionWithSingleBaseCreatesBasesTuple) {
  PyObjectPtr type(
      PyErr_NewException("Module.Name", PyExc_ValueError, nullptr));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyType_CheckExact(type));
  EXPECT_TRUE(
      PyType_IsSubtype(reinterpret_cast<PyTypeObject*>(type.get()),
                       reinterpret_cast<PyTypeObject*>(PyExc_ValueError)));

  PyObjectPtr bases(PyObject_GetAttrString(type, "__bases__"));
  ASSERT_TRUE(PyTuple_CheckExact(bases));
  EXPECT_EQ(PyTuple_GetItem(bases, 0), PyExc_ValueError);
}

TEST_F(ErrorsExtensionApiTest, NewExceptionWithBaseTupleStoresTuple) {
  PyObjectPtr bases(PyTuple_New(2));
  Py_INCREF(PyExc_SystemError);
  ASSERT_EQ(PyTuple_SetItem(bases, 0, PyExc_SystemError), 0);
  Py_INCREF(PyExc_ValueError);
  ASSERT_EQ(PyTuple_SetItem(bases, 1, PyExc_ValueError), 0);
  PyObjectPtr type(PyErr_NewException("Module.Name", bases, nullptr));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyType_CheckExact(type));
  EXPECT_TRUE(
      PyType_IsSubtype(reinterpret_cast<PyTypeObject*>(type.get()),
                       reinterpret_cast<PyTypeObject*>(PyExc_ValueError)));
  EXPECT_TRUE(
      PyType_IsSubtype(reinterpret_cast<PyTypeObject*>(type.get()),
                       reinterpret_cast<PyTypeObject*>(PyExc_SystemError)));

  PyObjectPtr type_bases(PyObject_GetAttrString(type, "__bases__"));
  EXPECT_EQ(type_bases, bases);
}

TEST_F(ErrorsExtensionApiTest, NewExceptionWithEmptyDictAddsModuleName) {
  PyObjectPtr dict(PyDict_New());
  PyObjectPtr type(PyErr_NewException("Module.Name", nullptr, dict));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyType_CheckExact(type));

  PyObjectPtr module_name(PyObject_GetAttrString(type, "__module__"));
  ASSERT_TRUE(PyUnicode_CheckExact(module_name));
  EXPECT_TRUE(isUnicodeEqualsCStr(module_name, "Module"));
}

TEST_F(ErrorsExtensionApiTest,
       NewExceptionWithDocWithNullDocReturnsTypeWithNoneDoc) {
  PyObjectPtr type(
      PyErr_NewExceptionWithDoc("Module.Name", nullptr, nullptr, nullptr));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyType_CheckExact(type));
  EXPECT_TRUE(
      PyType_IsSubtype(reinterpret_cast<PyTypeObject*>(type.get()),
                       reinterpret_cast<PyTypeObject*>(PyExc_Exception)));

  PyObjectPtr name(PyObject_GetAttrString(type, "__name__"));
  PyObjectPtr module_name(PyObject_GetAttrString(type, "__module__"));
  PyObjectPtr doc_string(PyObject_GetAttrString(type, "__doc__"));
  EXPECT_TRUE(isUnicodeEqualsCStr(name, "Name"));
  EXPECT_TRUE(isUnicodeEqualsCStr(module_name, "Module"));
  EXPECT_EQ(doc_string, Py_None);
}

TEST_F(ErrorsExtensionApiTest,
       NewExceptionWithDocWithNonDictRaisesSystemError) {
  PyObjectPtr not_dict(PyList_New(0));
  EXPECT_EQ(PyErr_NewExceptionWithDoc("Module.Name", "DOC", nullptr, not_dict),
            nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(ErrorsExtensionApiTest, NewExceptionWithDocWithStrReturnsType) {
  PyObjectPtr type(
      PyErr_NewExceptionWithDoc("Module.Name", "DOC", nullptr, nullptr));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyType_CheckExact(type));
  EXPECT_TRUE(
      PyType_IsSubtype(reinterpret_cast<PyTypeObject*>(type.get()),
                       reinterpret_cast<PyTypeObject*>(PyExc_Exception)));

  PyObjectPtr name(PyObject_GetAttrString(type, "__name__"));
  PyObjectPtr module_name(PyObject_GetAttrString(type, "__module__"));
  PyObjectPtr doc_string(PyObject_GetAttrString(type, "__doc__"));
  EXPECT_TRUE(isUnicodeEqualsCStr(name, "Name"));
  EXPECT_TRUE(isUnicodeEqualsCStr(module_name, "Module"));
  EXPECT_TRUE(isUnicodeEqualsCStr(doc_string, "DOC"));
}

TEST_F(ErrorsExtensionApiTest, NoMemoryRaisesMemoryError) {
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyErr_NoMemory(), nullptr);

  PyObject* type = nullptr;
  PyObject* value = nullptr;
  PyObject* traceback = nullptr;
  PyErr_Fetch(&type, &value, &traceback);
  EXPECT_EQ(type, PyExc_MemoryError);
  EXPECT_EQ(value, nullptr);
  // TODO(T42241510): Traceback support isn't implemented yet. Once it's ready,
  // inspect the traceback here.

  Py_DECREF(type);
}

#pragma push_macro("PyErr_BadInternalCall")
#undef PyErr_BadInternalCall
// PyErr_BadInternalCall() has an assert(0) in CPython.
TEST_F(ErrorsExtensionApiTest, BadInternalCallRaisesSystemErrorPyro) {
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  PyErr_BadInternalCall();

  PyObject* type = nullptr;
  PyObject* value = nullptr;
  PyObject* traceback = nullptr;
  PyErr_Fetch(&type, &value, &traceback);
  EXPECT_EQ(type, PyExc_SystemError);

  PyObjectPtr message(
      PyUnicode_FromString("bad argument to internal function"));
  ASSERT_TRUE(PyUnicode_Check(message));
  EXPECT_EQ(_PyUnicode_EQ(value, message), 1);

  // TODO(T42241510): Traceback support isn't implemented yet. Once it's ready,
  // inspect the traceback here.

  Py_DECREF(type);
  Py_DECREF(value);
}
#pragma pop_macro("PyErr_BadInternalCall")

TEST_F(ErrorsExtensionApiTest, UnderBadInternalCallRaisesSystemError) {
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  _PyErr_BadInternalCall("abc", 123);

  PyObject* type = nullptr;
  PyObject* value = nullptr;
  PyObject* traceback = nullptr;
  PyErr_Fetch(&type, &value, &traceback);
  EXPECT_EQ(type, PyExc_SystemError);

  PyObjectPtr message(
      PyUnicode_FromString("abc:123: bad argument to internal function"));
  ASSERT_TRUE(PyUnicode_Check(message));
  EXPECT_EQ(_PyUnicode_EQ(value, message), 1);

  // TODO(T42241510): Traceback support isn't implemented yet. Once it's ready,
  // inspect the traceback here.

  Py_DECREF(type);
  Py_DECREF(value);
}

TEST_F(ErrorsExtensionApiTest, ExceptionMatches) {
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  PyErr_NoMemory();
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_MemoryError));
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_Exception));
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_BaseException));
}

TEST_F(ErrorsExtensionApiTest, Fetch) {
  PyErr_SetObject(PyExc_Exception, Py_True);

  PyObject* type = nullptr;
  PyObject* value = nullptr;
  PyObject* traceback = nullptr;
  PyErr_Fetch(&type, &value, &traceback);
  EXPECT_EQ(type, PyExc_Exception);
  EXPECT_EQ(value, Py_True);
  EXPECT_EQ(traceback, nullptr);

  Py_DECREF(type);
  Py_DECREF(value);
}

TEST_F(ErrorsExtensionApiTest, GetExcInfoWhenNoCaughtException) {
  PyObject* p_type;
  PyObject* p_value;
  PyObject* p_traceback;
  PyErr_SetExcInfo(nullptr, nullptr, nullptr);
  PyErr_GetExcInfo(&p_type, &p_value, &p_traceback);
  EXPECT_EQ(p_type, nullptr);
  EXPECT_EQ(p_value, nullptr);
  EXPECT_EQ(p_traceback, nullptr);
}

TEST_F(ErrorsExtensionApiTest, GetExcInfoWhenCaughtException) {
  binaryfunc func = [](PyObject*, PyObject*) {
    PyObject* p_type;
    PyObject* p_value;
    PyObject* p_traceback;
    PyErr_GetExcInfo(&p_type, &p_value, &p_traceback);
    EXPECT_EQ(p_type, PyExc_Exception);
    PyObjectPtr args(PyObject_GetAttrString(p_value, "args"));
    PyObject* first_arg = PyTuple_GetItem(args, 0);
    EXPECT_TRUE(isUnicodeEqualsCStr(first_arg, "some str"));
    Py_INCREF(Py_None);
    Py_XDECREF(p_type);
    Py_XDECREF(p_value);
    Py_XDECREF(p_traceback);
    return Py_None;
  };
  PyMethodDef foo_methods[] = {{"noargs", func, METH_NOARGS}, {nullptr}};
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT, "foo", nullptr, 0, foo_methods,
  };
  PyObjectPtr module(PyModule_Create(&def));
  moduleSet("__main__", "foo", module);
  ASSERT_TRUE(PyModule_CheckExact(module));
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  PyRun_SimpleString(R"(
try:
  raise Exception('some str')
except:
  foo.noargs()
)");
}

TEST_F(ErrorsExtensionApiTest, GivenExceptionMatches) {
  // An exception matches itself and all of its super types up to and including
  // BaseException.
  EXPECT_EQ(PyErr_GivenExceptionMatches(PyExc_MemoryError, PyExc_MemoryError),
            1);
  EXPECT_EQ(PyErr_GivenExceptionMatches(PyExc_MemoryError, PyExc_Exception), 1);
  EXPECT_EQ(PyErr_GivenExceptionMatches(PyExc_MemoryError, PyExc_BaseException),
            1);

  // An exception should not match a disjoint exception type.
  EXPECT_EQ(PyErr_GivenExceptionMatches(PyExc_MemoryError, PyExc_IOError), 0);

  // If the objects are not exceptions or exception classes, the matching falls
  // back to an identity comparison.
  EXPECT_TRUE(PyErr_GivenExceptionMatches(Py_True, Py_True));
}

TEST_F(ErrorsExtensionApiTest, GivenExceptionMatchesWithNullptr) {
  // If any argument is a null pointer zero is returned.
  EXPECT_EQ(PyErr_GivenExceptionMatches(nullptr, nullptr), 0);
  EXPECT_EQ(PyErr_GivenExceptionMatches(PyExc_SystemError, nullptr), 0);
  EXPECT_EQ(PyErr_GivenExceptionMatches(nullptr, PyExc_SystemError), 0);
}

TEST_F(ErrorsExtensionApiTest, GivenExceptionMatchesWithTuple) {
  PyObject* exc1 = PyTuple_Pack(1, PyExc_Exception);
  ASSERT_NE(exc1, nullptr);
  EXPECT_EQ(PyErr_GivenExceptionMatches(PyExc_MemoryError, exc1), 1);
  EXPECT_EQ(PyErr_GivenExceptionMatches(PyExc_SystemExit, exc1), 0);
  Py_DECREF(exc1);

  // Linear search
  PyObject* exc2 = PyTuple_Pack(2, PyExc_Warning, PyExc_Exception);
  ASSERT_NE(exc2, nullptr);
  EXPECT_EQ(PyErr_GivenExceptionMatches(PyExc_MemoryError, exc2), 1);
  EXPECT_EQ(PyErr_GivenExceptionMatches(PyExc_SystemExit, exc2), 0);
  Py_DECREF(exc2);

  // Recursion
  PyObjectPtr inner(PyTuple_Pack(1, PyExc_Exception));
  PyObject* exc3 = PyTuple_Pack(2, inner.get(), PyExc_Warning);
  ASSERT_NE(exc3, nullptr);
  EXPECT_EQ(PyErr_GivenExceptionMatches(PyExc_MemoryError, exc3), 1);
  EXPECT_EQ(PyErr_GivenExceptionMatches(PyExc_SystemExit, exc3), 0);
  Py_DECREF(exc3);
}

TEST_F(ErrorsExtensionApiTest, Restore) {
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  Py_INCREF(PyExc_Exception);
  Py_INCREF(Py_True);
  PyErr_Restore(PyExc_Exception, Py_True, nullptr);

  PyObject* type = nullptr;
  PyObject* value = nullptr;
  PyObject* traceback = nullptr;
  PyErr_Fetch(&type, &value, &traceback);
  EXPECT_EQ(type, PyExc_Exception);
  EXPECT_EQ(value, Py_True);
  EXPECT_EQ(traceback, nullptr);

  Py_DECREF(type);
  Py_DECREF(value);
}

TEST_F(ErrorsExtensionApiTest, ChainExceptionsSetsContext) {
  // First, set an exception.
  PyErr_SetString(PyExc_RuntimeError, "whoops");

  // Next, attempt to restore a different exception. It should be chained to the
  // existing RuntimeError.
  PyObject* exc = PyExc_TypeError;
  Py_INCREF(exc);
  PyObject* val = Py_None;
  Py_INCREF(val);
  PyObject* tb = Py_None;
  Py_INCREF(tb);
  _PyErr_ChainExceptions(exc, val, tb);
  ASSERT_NE(PyErr_Occurred(), nullptr);

  // Make sure the RuntimeError has the new TypeError as its context attribute.
  PyErr_Fetch(&exc, &val, &tb);
  ASSERT_EQ(PyErr_GivenExceptionMatches(exc, PyExc_RuntimeError), 1);
  ASSERT_EQ(PyErr_GivenExceptionMatches(val, PyExc_RuntimeError), 1);

  PyObjectPtr ctx(PyException_GetContext(val));
  EXPECT_EQ(PyErr_GivenExceptionMatches(ctx, PyExc_TypeError), 1);

  EXPECT_EQ(tb, nullptr);

  Py_DECREF(exc);
  Py_DECREF(val);
}

TEST_F(ErrorsExtensionApiTest, NormalizeCreatesException) {
  PyObject* exc = PyExc_RuntimeError;
  PyObject* val = PyUnicode_FromString("something went wrong!");
  PyObjectPtr val_orig(val);
  Py_INCREF(val_orig);
  PyObject* tb = nullptr;
  PyErr_NormalizeException(&exc, &val, &tb);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_GivenExceptionMatches(exc, PyExc_RuntimeError));
  ASSERT_TRUE(PyErr_GivenExceptionMatches(val, PyExc_RuntimeError));
  PyObjectPtr args(PyObject_GetAttrString(val, "args"));
  ASSERT_TRUE(PyTuple_CheckExact(args));
  ASSERT_EQ(PyTuple_Size(args), 1);
  EXPECT_EQ(PyTuple_GetItem(args, 0), val_orig);

  Py_DECREF(val);
}

TEST_F(ErrorsExtensionApiTest, NormalizeWithNullTypeDoesNothing) {
  PyObject* exc = nullptr;
  PyObject* val = nullptr;
  PyObject* tb = nullptr;
  PyErr_NormalizeException(&exc, &val, &tb);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(exc, nullptr);
  EXPECT_EQ(val, nullptr);
  EXPECT_EQ(tb, nullptr);
}

TEST_F(ErrorsExtensionApiTest, NormalizeWithNullValueUsesNone) {
  PyObject* exc = PyExc_TypeError;
  PyObject* val = Py_None;
  Py_INCREF(val);
  PyObject* tb = nullptr;
  PyErr_NormalizeException(&exc, &val, &tb);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_GivenExceptionMatches(exc, PyExc_TypeError));
  ASSERT_TRUE(PyErr_GivenExceptionMatches(val, PyExc_TypeError));
  PyObjectPtr args(PyObject_GetAttrString(val, "args"));
  ASSERT_TRUE(PyTuple_CheckExact(args));
  EXPECT_EQ(PyTuple_Size(args), 0);

  Py_DECREF(val);
}

TEST_F(ErrorsExtensionApiTest, NormalizeWithTupleUsesArgs) {
  PyObject* exc = PyExc_Exception;
  PyObject* val = PyTuple_New(2);
  PyObjectPtr t0(PyLong_FromLong(111));
  PyObjectPtr t1(PyUnicode_FromString("hello"));
  Py_INCREF(t0);
  PyTuple_SET_ITEM(val, 0, t0);
  Py_INCREF(t1);
  PyTuple_SET_ITEM(val, 1, t1);
  PyObject* tb = nullptr;
  PyErr_NormalizeException(&exc, &val, &tb);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_GivenExceptionMatches(exc, PyExc_Exception));
  ASSERT_TRUE(PyErr_GivenExceptionMatches(val, PyExc_Exception));
  PyObjectPtr args(PyObject_GetAttrString(val, "args"));
  ASSERT_TRUE(PyTuple_CheckExact(args));
  ASSERT_EQ(PyTuple_Size(args), 2);
  EXPECT_EQ(PyTuple_GetItem(args, 0), t0);
  EXPECT_EQ(PyTuple_GetItem(args, 1), t1);

  Py_DECREF(val);
}

TEST_F(ErrorsExtensionApiTest, NormalizeWithNonExceptionDoesNothing) {
  PyObject *exc = PyLong_FromLong(123), *exc_orig = exc;
  PyObject *val = PyLong_FromLong(456), *val_orig = val;
  PyObject* tb = nullptr;
  PyErr_NormalizeException(&exc, &val, &tb);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(exc, exc_orig);
  EXPECT_EQ(val, val_orig);
  EXPECT_EQ(tb, nullptr);

  Py_DECREF(val);
  Py_DECREF(exc);
}

TEST_F(ErrorsExtensionApiTest, NormalizeWithFailingConstructorReturnsNewError) {
  // TODO(bsimmers): Once we have PyType_FromSpec() (or PyType_Ready() can
  // handle base classes), add a similar test to ensure that
  // PyErr_NormalizeException() doesn't loop infinintely when normalization
  // keeps failing.

  ASSERT_EQ(PyRun_SimpleString(R"(
class BadException(Exception):
  def __init__(self, arg):
    raise RuntimeError(arg)
)"),
            0);
  PyObject* exc = moduleGet("__main__", "BadException");
  ASSERT_TRUE(PyType_Check(exc));

  const char* msg = "couldn't construct BadException";
  PyObject* val = PyUnicode_FromString(msg);
  PyObject* tb = nullptr;
  PyErr_NormalizeException(&exc, &val, &tb);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_GivenExceptionMatches(exc, PyExc_RuntimeError));
  ASSERT_TRUE(PyErr_GivenExceptionMatches(val, PyExc_RuntimeError));
  PyObjectPtr args(PyObject_GetAttrString(val, "args"));
  ASSERT_TRUE(PyTuple_CheckExact(args));
  ASSERT_EQ(PyTuple_Size(args), 1);
  PyObject* str = PyTuple_GetItem(args, 0);
  EXPECT_TRUE(isUnicodeEqualsCStr(str, msg));

  Py_XDECREF(val);
  Py_XDECREF(exc);
  Py_XDECREF(tb);
}

TEST_F(ErrorsExtensionApiTest, ProgramTextObjectWithNullFilenameReturnsNull) {
  EXPECT_EQ(PyErr_ProgramTextObject(nullptr, 5), nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ErrorsExtensionApiTest,
       ProgramTextObjectWithNonPositiveLinenoReturnsNull) {
  PyObjectPtr filename(PyUnicode_FromString("filename"));
  EXPECT_EQ(PyErr_ProgramTextObject(filename, -5), nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ErrorsExtensionApiTest,
       ProgramTextObjectWithNonExistentFileReturnsNull) {
  PyObjectPtr filename(PyUnicode_FromString("foobarbazquux"));
  EXPECT_EQ(PyErr_ProgramTextObject(filename, 5), nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ErrorsExtensionApiTest, SetExcInfoValuesRetrievedByGetExcInfo) {
  PyObjectPtr type(PyExc_TypeError);
  Py_INCREF(type);
  PyObjectPtr val(PyUnicode_FromString("some str"));
  PyObject* traceback = nullptr;
  PyErr_SetExcInfo(type, val, traceback);

  PyObject* p_type;
  PyObject* p_value;
  PyObject* p_traceback;
  PyErr_GetExcInfo(&p_type, &p_value, &p_traceback);
  EXPECT_EQ(p_type, type);
  EXPECT_EQ(p_value, val);
  EXPECT_EQ(p_traceback, traceback);
}

TEST_F(ErrorsExtensionApiTest, SetFromErrnoWithZeroSetsError) {
  errno = 0;
  ASSERT_EQ(PyErr_SetFromErrno(PyExc_TypeError), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(ErrorsExtensionApiTest, SetFromErrnoWithNonZeroSetsError) {
  errno = 1;
  ASSERT_EQ(PyErr_SetFromErrno(PyExc_SystemError), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(ErrorsExtensionApiTest, SetFromErrnoWithFilenameSetsError) {
  errno = 1;
  ASSERT_EQ(PyErr_SetFromErrnoWithFilename(PyExc_NameError, "foo"), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_NameError));
}

TEST_F(ErrorsExtensionApiTest, SetFromErrnoWithFilenameObjectSetsError) {
  errno = 1;
  PyObjectPtr foo(PyUnicode_FromString("foo"));
  ASSERT_EQ(PyErr_SetFromErrnoWithFilenameObject(PyExc_KeyError, foo), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_KeyError));
}

TEST_F(ErrorsExtensionApiTest, SetFromErrnoWithFilenameObjectsSetsError) {
  errno = 1;
  PyObjectPtr foo(PyUnicode_FromString("foo"));
  PyObjectPtr bar(PyUnicode_FromString("bar"));
  ASSERT_EQ(
      PyErr_SetFromErrnoWithFilenameObjects(PyExc_ChildProcessError, foo, bar),
      nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_ChildProcessError));
}

TEST_F(ErrorsExtensionApiTest, SetStringSetsValue) {
  PyErr_SetString(PyExc_Exception, "An exception occured");
  PyObject* type = nullptr;
  PyObject* value = nullptr;
  PyObject* traceback = nullptr;
  PyErr_Fetch(&type, &value, &traceback);
  EXPECT_EQ(traceback, nullptr);
  EXPECT_EQ(type, PyExc_Exception);
  EXPECT_TRUE(isUnicodeEqualsCStr(value, "An exception occured"));

  Py_DECREF(type);
  Py_DECREF(value);
}

TEST_F(ErrorsExtensionApiTest, FormatWithNoArgsSetsAppropriateFields) {
  ASSERT_EQ(PyErr_Format(PyExc_TypeError, "hello error"), nullptr);
  PyObject* type = nullptr;
  PyObject* value = nullptr;
  PyObject* traceback = nullptr;
  PyErr_Fetch(&type, &value, &traceback);
  EXPECT_EQ(type, PyExc_TypeError);
  EXPECT_TRUE(isUnicodeEqualsCStr(value, "hello error"));
  EXPECT_EQ(traceback, nullptr);

  Py_DECREF(type);
  Py_DECREF(value);
}

TEST_F(ErrorsExtensionApiTest, FormatWithManyArgsSetsAppropriateFields) {
  ASSERT_EQ(PyErr_Format(PyExc_MemoryError, "h%c%s", 'e', "llo world"),
            nullptr);
  PyObject* type = nullptr;
  PyObject* value = nullptr;
  PyObject* traceback = nullptr;
  PyErr_Fetch(&type, &value, &traceback);
  EXPECT_EQ(type, PyExc_MemoryError);
  EXPECT_TRUE(isUnicodeEqualsCStr(value, "hello world"));
  EXPECT_EQ(traceback, nullptr);

  Py_DECREF(type);
  Py_DECREF(value);
}

TEST_F(ErrorsExtensionApiTest, FormatFromCauseWithoutExceptionFailsDeathTest) {
  EXPECT_DEATH(_PyErr_FormatFromCause(PyExc_TypeError, ""), "");
}

TEST_F(ErrorsExtensionApiTest, FormatFromCauseSetsCauseAndContext) {
  ASSERT_EQ(PyErr_Format(PyExc_MemoryError, "%s", "original cause"), nullptr);
  ASSERT_EQ(_PyErr_FormatFromCause(PyExc_TypeError, "%s", "new error"),
            nullptr);
  PyObject* type = nullptr;
  PyObject* value = nullptr;
  PyObject* traceback = nullptr;
  PyErr_Fetch(&type, &value, &traceback);
  EXPECT_EQ(type, PyExc_TypeError);
  Py_XDECREF(type);
  EXPECT_EQ(traceback, nullptr);
  Py_XDECREF(traceback);
  PyObjectPtr cause(PyException_GetCause(value));
  PyObjectPtr context(PyException_GetContext(value));
  EXPECT_TRUE(PyErr_GivenExceptionMatches(cause, PyExc_MemoryError));
  EXPECT_TRUE(PyErr_GivenExceptionMatches(context, PyExc_MemoryError));
  Py_XDECREF(value);
}

TEST_F(ErrorsExtensionApiTest, WriteUnraisableClearsException) {
  PyErr_SetString(PyExc_MemoryError, "original cause");
  PyErr_WriteUnraisable(Py_None);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ErrorsExtensionApiTest, WriteUnraisableCallsDunderRepr) {
  PyRun_SimpleString(R"(
class C:
  def __repr__(self):
    return "foo"
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyErr_SetString(PyExc_MemoryError, "original cause");
  CaptureStdStreams streams;
  PyErr_WriteUnraisable(c);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_THAT(streams.err(),
              ::testing::StartsWith("Exception ignored in: foo"));
  EXPECT_EQ(streams.out(), "");
}

TEST_F(ErrorsExtensionApiTest,
       WriteUnraisableDoesNotFailWithNonCallableDunderRepr) {
  PyRun_SimpleString(R"(
class C:
  __repr__ = 5
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyErr_SetString(PyExc_MemoryError, "original cause");
  CaptureStdStreams streams;
  PyErr_WriteUnraisable(c);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_THAT(
      streams.err(),
      ::testing::StartsWith("Exception ignored in: <object repr() failed>"));
  EXPECT_EQ(streams.out(), "");
}

TEST_F(ErrorsExtensionApiTest,
       WriteUnraisableWithNonStrDunderModuleWritesUnknown) {
  PyRun_SimpleString(R"(
class C(BaseException):
  pass
C.__module__ = 5
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr ctype(moduleGet("__main__", "C"));
  PyErr_SetString(ctype, "original cause");
  CaptureStdStreams streams;
  PyErr_WriteUnraisable(c);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_THAT(streams.err(),
              ::testing::EndsWith("<unknown>C: original cause\n"));
  EXPECT_EQ(streams.out(), "");
}

TEST_F(ErrorsExtensionApiTest, WriteUnraisableWritesModuleName) {
  PyRun_SimpleString(R"(
class C(BaseException):
  pass
C.__module__ = "foo"
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyObjectPtr ctype(moduleGet("__main__", "C"));
  PyErr_SetString(ctype, "original cause");
  CaptureStdStreams streams;
  PyErr_WriteUnraisable(c);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_THAT(streams.err(), ::testing::EndsWith("foo.C: original cause\n"));
  EXPECT_EQ(streams.out(), "");
}

TEST_F(ErrorsExtensionApiTest, WriteUnraisableCallsDunderStrOnVal) {
  PyRun_SimpleString(R"(
class C:
  def __str__(self):
    return "bar"
C.__module__ = "foo"
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyErr_SetObject(PyExc_MemoryError, c);
  CaptureStdStreams streams;
  PyErr_WriteUnraisable(Py_None);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_THAT(streams.err(), ::testing::EndsWith("MemoryError: bar\n"));
  EXPECT_EQ(streams.out(), "");
}

TEST_F(ErrorsExtensionApiTest,
       WriteUnraisableDoesNotFailWithNonCallableDunderStr) {
  PyRun_SimpleString(R"(
class C:
  __str__ = 5
C.__module__ = "foo"
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  PyErr_SetObject(PyExc_MemoryError, c);
  CaptureStdStreams streams;
  PyErr_WriteUnraisable(Py_None);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_THAT(streams.err(),
              ::testing::EndsWith("MemoryError: <exception str() failed>\n"));
  EXPECT_EQ(streams.out(), "");
}

TEST_F(ErrorsExtensionApiTest, SetObjectWithCaughtExceptionSetsContext) {
  PyCFunction test_set_object = [](PyObject*, PyObject*) -> PyObject* {
    PyErr_SetString(PyExc_ValueError, "something went wrong");
    return nullptr;
  };
  static PyMethodDef methods[2];
  methods[0] = {
      "test_set_object",
      test_set_object,
      METH_NOARGS,
      "doc",
  };
  methods[1] = {nullptr};
  static PyModuleDef mod_def;
  mod_def = {
      PyModuleDef_HEAD_INIT,
      "errors_test",  // m_name
      "doc",          // m_doc
      0,              // m_size
      methods,        // m_methods
      nullptr,        // m_slots
      nullptr,        // m_traverse
      nullptr,        // m_clear
      nullptr,        // m_free
  };

  PyObjectPtr module(PyModule_Create(&mod_def));
  ASSERT_NE(module, nullptr);
  ASSERT_EQ(moduleSet("__main__", "errors_test", module), 0);
  ASSERT_EQ(PyRun_SimpleString(R"(
try:
  try:
    raise RuntimeError("blorp")
  except RuntimeError as exc:
    inner_exc = exc
    errors_test.test_set_object()
except ValueError as exc:
  outer_exc = exc
)"),
            0);

  PyObjectPtr inner_exc(moduleGet("__main__", "inner_exc"));
  ASSERT_NE(inner_exc, nullptr);
  PyObjectPtr outer_exc(moduleGet("__main__", "outer_exc"));
  ASSERT_NE(outer_exc, nullptr);
  PyObjectPtr outer_ctx(PyException_GetContext(outer_exc));
  EXPECT_EQ(outer_ctx, inner_exc);
  EXPECT_EQ(PyException_GetContext(inner_exc), nullptr);
}

}  // namespace py
