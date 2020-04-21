#include "Python.h"
#include "gtest/gtest.h"

#include "capi-fixture.h"
#include "capi-testing.h"

namespace py {
namespace testing {

using DictExtensionApiTest = ExtensionApi;

TEST_F(DictExtensionApiTest, GetItemFromNonDictReturnsNull) {
  // Pass a non dictionary
  PyObject* result = PyDict_GetItem(Py_None, Py_None);
  EXPECT_EQ(result, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(DictExtensionApiTest, GetItemNonExistingKeyReturnsNull) {
  PyObjectPtr dict(PyDict_New());
  PyObjectPtr nonkey(PyLong_FromLong(10));

  // Pass a non existing key
  PyObject* result = PyDict_GetItem(dict, nonkey);
  EXPECT_EQ(result, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(DictExtensionApiTest, GetItemReturnsBorrowedValue) {
  PyObject* dict = PyDict_New();
  PyObject* key = PyLong_FromLong(10);
  PyObject* value = PyLong_FromLong(0);

  // Insert the value into the dictionary
  ASSERT_EQ(PyDict_SetItem(dict, key, value), 0);

  // Record the reference count of the value
  long refcnt = Py_REFCNT(value);

  // Get a new reference to the value from the dictionary
  PyObject* value2 = PyDict_GetItem(dict, key);

  // The new reference should be equal to the original reference
  EXPECT_EQ(value2, value);

  // The reference count should not be affected
  EXPECT_EQ(Py_REFCNT(value), refcnt);

  Py_DECREF(value);
  Py_DECREF(key);
  Py_DECREF(dict);
}

TEST_F(DictExtensionApiTest, GetItemWithDictSubclassReturnsValue) {
  PyRun_SimpleString(R"(
class Foo(dict): pass
obj = Foo()
  )");

  PyObjectPtr obj(mainModuleGet("obj"));
  PyObjectPtr key(PyLong_FromLong(1));
  PyObjectPtr val(PyLong_FromLong(2));
  ASSERT_EQ(PyDict_SetItem(obj, key, val), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);

  PyObject* result = PyDict_GetItem(obj, key);
  EXPECT_EQ(result, val);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(DictExtensionApiTest, GetItemWithBigHashTruncatesHash) {
  PyRun_SimpleString(R"(
class C:
    def __init__(self, v):
        self.v = v
    def __hash__(self):
        return 1180591620717411303424
    def __eq__(self, other):
        return self.v == other.v
c1 = C(4)
c2 = C(5)
  )");

  PyObjectPtr c1(mainModuleGet("c1"));
  PyObjectPtr c2(mainModuleGet("c2"));
  PyObjectPtr v1(PyLong_FromLong(1));
  PyObjectPtr v2(PyLong_FromLong(2));
  PyObjectPtr dict(PyDict_New());
  ASSERT_EQ(PyDict_SetItem(dict, c1, v1), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);

  ASSERT_EQ(PyDict_SetItem(dict, c2, v2), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);

  PyObject* result = PyDict_GetItem(dict, c1);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(result, v1);
}

TEST_F(DictExtensionApiTest, GetItemWithIntSubclassHashUsesInt) {
  PyRun_SimpleString(R"(
class H(int):
  pass
class C:
  def __init__(self, v):
    self.v = v
  def __hash__(self):
    return H(42)
  def __eq__(self, other):
    return self.v == other.v
c = C(4)
)");

  PyObjectPtr c(mainModuleGet("c"));
  PyObjectPtr v(PyLong_FromLong(1));
  PyObjectPtr dict(PyDict_New());
  ASSERT_EQ(PyDict_SetItem(dict, c, v), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);

  PyObject* result = PyDict_GetItem(dict, c);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(result, v);
}

TEST_F(DictExtensionApiTest, GetItemWithSameIdentityReturnsObject) {
  ASSERT_EQ(PyRun_SimpleString(R"(
called_dunder_eq = False
class C:
  def __eq__(self, other):
    global called_dunder_eq
    called_dunder_eq = True
  def __hash__(self):
    return 5
c = C()
d = {}
d[c] = 4
)"),
            0);
  PyObjectPtr c(mainModuleGet("c"));
  PyObjectPtr dict(mainModuleGet("d"));
  PyObject* result = PyDict_GetItem(dict, c);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  PyObjectPtr called_dunder_eq(mainModuleGet("called_dunder_eq"));
  ASSERT_EQ(called_dunder_eq, Py_False);
  EXPECT_EQ(PyLong_AsLong(result), 4);
}

TEST_F(DictExtensionApiTest, GetItemWithDifferentHashReturnsNull) {
  ASSERT_EQ(PyRun_SimpleString(R"(
called_dunder_eq = False
class C:
  def __init__(self, h):
      self.h = h
  def __eq__(self, other):
    global called_dunder_eq
    called_dunder_eq = True
    return True
  def __hash__(self):
    return self.h

c = C(1)
d = {}
d[C(2)] = 2
)"),
            0);
  PyObjectPtr c(mainModuleGet("c"));
  PyObjectPtr dict(mainModuleGet("d"));
  PyObject* result = PyDict_GetItem(dict, c);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  PyObjectPtr called_dunder_eq(mainModuleGet("called_dunder_eq"));
  ASSERT_EQ(called_dunder_eq, Py_False);
  EXPECT_EQ(result, nullptr);
}

TEST_F(DictExtensionApiTest, GetItemWithDunderEqReturnsObject) {
  ASSERT_EQ(PyRun_SimpleString(R"(
called_dunder_eq = False
class C:
  def __eq__(self, other):
    global called_dunder_eq
    called_dunder_eq = True
    return True
  def __hash__(self):
    return 5

d = {}
c = C()
d[C()] = 4
)"),
            0);
  PyObjectPtr c(mainModuleGet("c"));
  PyObjectPtr dict(mainModuleGet("d"));
  PyObject* result = PyDict_GetItem(dict, c);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  PyObjectPtr called_dunder_eq(mainModuleGet("called_dunder_eq"));
  ASSERT_EQ(called_dunder_eq, Py_True);
  EXPECT_EQ(PyLong_AsLong(result), 4);
}

TEST_F(DictExtensionApiTest, GetItemWithFalseDunderEqReturnsNull) {
  ASSERT_EQ(PyRun_SimpleString(R"(
called_dunder_eq = False
class C:
  def __eq__(self, other):
    global called_dunder_eq
    called_dunder_eq = True
    return False
  def __hash__(self):
    return 5

c = C()
d = {}
d[C()] = 4
)"),
            0);
  PyObjectPtr c(mainModuleGet("c"));
  PyObjectPtr dict(mainModuleGet("d"));
  PyObject* result = PyDict_GetItem(dict, c);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  PyObjectPtr called_dunder_eq(mainModuleGet("called_dunder_eq"));
  ASSERT_EQ(called_dunder_eq, Py_True);
  EXPECT_EQ(result, nullptr);
}

TEST_F(DictExtensionApiTest,
       GetItemWithExceptionDunderEqSwallowsAndReturnsNull) {
  ASSERT_EQ(PyRun_SimpleString(R"(
called_dunder_eq = False
class C:
  def __eq__(self, other):
    global called_dunder_eq
    called_dunder_eq = True
    raise ValueError('foo')
  def __hash__(self):
    return 5

c = C()
d = {}
d[C()] = 4
)"),
            0);
  PyObjectPtr c(mainModuleGet("c"));
  PyObjectPtr dict(mainModuleGet("d"));
  PyObject* result = PyDict_GetItem(dict, c);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  PyObjectPtr called_dunder_eq(mainModuleGet("called_dunder_eq"));
  ASSERT_EQ(called_dunder_eq, Py_True);
  EXPECT_EQ(result, nullptr);
}

TEST_F(DictExtensionApiTest, GetItemWithNotImplementedDunderEqReturnsNull) {
  ASSERT_EQ(PyRun_SimpleString(R"(
called_dunder_eq = False
class C:
  def __eq__(self, other):
    global called_dunder_eq
    called_dunder_eq = True
    return NotImplemented
  def __hash__(self):
    return 5

c = C()
d = {}
d[C()] = 4
)"),
            0);
  PyObjectPtr c(mainModuleGet("c"));
  PyObjectPtr dict(mainModuleGet("d"));
  PyObject* result = PyDict_GetItem(dict, c);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  PyObjectPtr called_dunder_eq(mainModuleGet("called_dunder_eq"));
  ASSERT_EQ(called_dunder_eq, Py_True);
  EXPECT_EQ(result, nullptr);
}

TEST_F(DictExtensionApiTest,
       GetItemCallsExistingKeyDunderEqAndThenLookedKeyDunderEq) {
  ASSERT_EQ(PyRun_SimpleString(R"(
seq_num = 0

def new_seq_num():
  global seq_num
  seq_num += 1
  return seq_num

c_eq = 0
c_hash = 0

class C:
  def __eq__(self, other):
    global c_eq
    c_eq = new_seq_num()
    return NotImplemented

  def __hash__(self):
    global c_hash
    c_hash = new_seq_num()
    return 5

c = C()

d_eq = 0
d_hash = 0

class D:
  def __eq__(self, other):
    global d_eq
    d_eq = new_seq_num()
    return True

  def __hash__(self):
    global d_hash
    d_hash = new_seq_num()
    return 5

d = D()
)"),
            0);
  PyObjectPtr dict(PyDict_New());
  PyObjectPtr c(mainModuleGet("c"));
  PyObjectPtr value(PyLong_FromLong(500));

  ASSERT_EQ(PyDict_SetItem(dict, c, value), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_EQ(PyDict_Size(dict), 1);
  PyObjectPtr c_eq(mainModuleGet("c_eq"));
  ASSERT_EQ(PyLong_AsLong(c_eq), 0);
  PyObjectPtr c_hash(mainModuleGet("c_hash"));
  ASSERT_EQ(PyLong_AsLong(c_hash), 1);

  PyObjectPtr d(mainModuleGet("d"));
  PyObject* result = PyDict_GetItem(dict, d);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_EQ(PyLong_AsLong(result), 500);
  c_hash = mainModuleGet("c_hash");
  EXPECT_EQ(PyLong_AsLong(c_hash), 1);
  PyObjectPtr d_hash(mainModuleGet("d_hash"));
  EXPECT_EQ(PyLong_AsLong(d_hash), 2);
  c_eq = mainModuleGet("c_eq");
  EXPECT_EQ(PyLong_AsLong(c_eq), 3);
  PyObjectPtr d_eq(mainModuleGet("d_eq"));
  EXPECT_EQ(PyLong_AsLong(d_eq), 4);
}

TEST_F(DictExtensionApiTest, GetItemComparesHashValueFirst) {
  ASSERT_EQ(PyRun_SimpleString(R"(
class C:
  def __init__(self, hash_code):
    self.hash_code = hash_code

  def __eq__(self, other):
    raise UserWarning("unexpected")

  def __hash__(self):
    return self.hash_code

c = C(4)
d = C(5)
)"),
            0);
  PyObjectPtr dict(PyDict_New());
  PyObjectPtr c(mainModuleGet("c"));
  PyObjectPtr value(PyLong_FromLong(500));

  ASSERT_EQ(PyDict_SetItem(dict, c, value), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_EQ(PyDict_Size(dict), 1);

  PyObjectPtr d(mainModuleGet("d"));
  ASSERT_EQ(PyDict_GetItem(dict, d), nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(DictExtensionApiTest, GetItemKnownHashFromNonDictRaisesSystemError) {
  // Pass a non dictionary
  PyObject* result = _PyDict_GetItem_KnownHash(Py_None, Py_None, 0);
  EXPECT_EQ(result, nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(DictExtensionApiTest, GetItemKnownHashNonExistingKeyReturnsNull) {
  PyObjectPtr dict(PyDict_New());
  PyObjectPtr nonkey(PyLong_FromLong(11));

  // Pass a non existing key
  PyObject* result = _PyDict_GetItem_KnownHash(dict, nonkey, 0);
  EXPECT_EQ(result, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(DictExtensionApiTest, GetItemKnownHashReturnsBorrowedValue) {
  PyObject* dict = PyDict_New();
  PyObject* key = PyLong_FromLong(10);
  PyObject* value = PyLong_FromLong(0);

  // Insert the value into the dictionary
  Py_hash_t hash = Py_hash_t{1} << ((sizeof(Py_hash_t) * CHAR_BIT) - 1);
  ASSERT_EQ(_PyDict_SetItem_KnownHash(dict, key, value, hash), 0);

  // Record the reference count of the value
  long refcnt = Py_REFCNT(value);

  // Get a new reference to the value from the dictionary
  PyObject* value2 = _PyDict_GetItem_KnownHash(dict, key, hash);

  // The new reference should be equal to the original reference
  EXPECT_EQ(value2, value);

  // The reference count should not be affected
  EXPECT_EQ(Py_REFCNT(value), refcnt);

  Py_DECREF(value);
  Py_DECREF(key);
  Py_DECREF(dict);
}

TEST_F(DictExtensionApiTest, GetItemStringReturnsValue) {
  PyObjectPtr dict(PyDict_New());
  const char* key_cstr = "key";
  PyObjectPtr key(PyUnicode_FromString(key_cstr));
  PyObjectPtr value(PyLong_FromLong(0));
  ASSERT_EQ(PyDict_SetItem(dict, key, value), 0);

  PyObject* item = PyDict_GetItemString(dict, key_cstr);
  EXPECT_EQ(item, value);
}

TEST_F(DictExtensionApiTest, SetItemWithNonDictRaisesSystemError) {
  PyObjectPtr set(PySet_New(nullptr));
  PyObjectPtr key(PyLong_FromLong(0));
  PyObjectPtr val(PyLong_FromLong(0));

  ASSERT_EQ(PyDict_SetItem(set, key, val), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(DictExtensionApiTest, SetItemWithNewDictReturnsZero) {
  PyObjectPtr dict(PyDict_New());
  PyObjectPtr key(PyLong_FromLong(0));
  PyObjectPtr val(PyLong_FromLong(0));

  EXPECT_EQ(PyDict_SetItem(dict, key, val), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(DictExtensionApiTest, SetItemWithNewDictSubclassReturnsZero) {
  PyRun_SimpleString(R"(
class Foo(dict): pass
obj = Foo()
  )");

  PyObjectPtr obj(mainModuleGet("obj"));
  PyObjectPtr key(PyLong_FromLong(0));
  PyObjectPtr val(PyLong_FromLong(0));

  EXPECT_EQ(PyDict_SetItem(obj, key, val), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(DictExtensionApiTest,
       SetItemWithDunderHashReturningNonIntRaisesTypeError) {
  PyRun_SimpleString(R"(
class C:
    def __hash__(self):
        return "foo"
    def __eq__(self, other):
        return self == other
c = C()
)");
  PyObjectPtr dict(PyDict_New());
  PyObjectPtr key(mainModuleGet("c"));
  PyObjectPtr val(PyLong_FromLong(0));

  ASSERT_EQ(PyDict_SetItem(dict, key, val), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(DictExtensionApiTest, SetItemWithIntSubclassHashReturnsZero) {
  PyRun_SimpleString(R"(
class H(int):
  pass
class C:
  def __init__(self, v):
    self.v = v
  def __hash__(self):
    return H(42)
  def __eq__(self, other):
    return self.v == other.v
c = C(4)
)");

  PyObjectPtr c(mainModuleGet("c"));
  PyObjectPtr v(PyLong_FromLong(1));
  PyObjectPtr dict(PyDict_New());
  EXPECT_EQ(PyDict_SetItem(dict, c, v), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(DictExtensionApiTest, SetItemWithSameIdentitySupersedesValue) {
  ASSERT_EQ(PyRun_SimpleString(R"(
called_dunder_eq = False
class C:
  def __eq__(self, other):
    global called_dunder_eq
    called_dunder_eq = True
  def __hash__(self):
    return 5

c = C()
d = {}
d[c] = 0
)"),
            0);
  PyObjectPtr c(mainModuleGet("c"));
  PyObjectPtr dict(mainModuleGet("d"));
  PyObjectPtr value(PyLong_FromLong(1));
  ASSERT_EQ(PyDict_SetItem(dict, c, value), 0);
  ASSERT_EQ(PyDict_Size(dict), 1);
  PyObject* result = PyDict_GetItem(dict, c);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  PyObjectPtr called_dunder_eq(mainModuleGet("called_dunder_eq"));
  ASSERT_EQ(called_dunder_eq, Py_False);
  EXPECT_EQ(value, result);
}

TEST_F(DictExtensionApiTest, SetItemWithDifferentHashInsertsValue) {
  ASSERT_EQ(PyRun_SimpleString(R"(
called_dunder_eq = False
class C:
  def __init__(self, h):
      self.h = h
  def __eq__(self, other):
    global called_dunder_eq
    called_dunder_eq = True
    return True
  def __hash__(self):
    return self.h

c = C(1)
d = {}
d[C(2)] = 2
)"),
            0);
  PyObjectPtr c(mainModuleGet("c"));
  PyObjectPtr dict(mainModuleGet("d"));
  PyObjectPtr value(PyLong_FromLong(1));
  ASSERT_EQ(PyDict_SetItem(dict, c, value), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  PyObjectPtr called_dunder_eq(mainModuleGet("called_dunder_eq"));
  ASSERT_EQ(called_dunder_eq, Py_False);
  EXPECT_EQ(PyDict_Size(dict), 2);
}

TEST_F(DictExtensionApiTest, SetItemWithDunderEqSupersedesValue) {
  ASSERT_EQ(PyRun_SimpleString(R"(
called_dunder_eq = False
class C:
  def __eq__(self, other):
    global called_dunder_eq
    called_dunder_eq = True
    return True
  def __hash__(self):
    return 5

c = C()
d = {}
d[C()] = 0
)"),
            0);
  PyObjectPtr c(mainModuleGet("c"));
  PyObjectPtr dict(mainModuleGet("d"));
  PyObjectPtr value(PyLong_FromLong(1));
  ASSERT_EQ(PyDict_SetItem(dict, c, value), 0);
  ASSERT_EQ(PyDict_Size(dict), 1);
  PyObject* result = PyDict_GetItem(dict, c);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  PyObjectPtr called_dunder_eq(mainModuleGet("called_dunder_eq"));
  ASSERT_EQ(called_dunder_eq, Py_True);
  EXPECT_EQ(value, result);
}

TEST_F(DictExtensionApiTest, SetItemWithFalseDunderEqInsertsValue) {
  ASSERT_EQ(PyRun_SimpleString(R"(
called_dunder_eq = False
class C:
  def __eq__(self, other):
    global called_dunder_eq
    called_dunder_eq = True
    return False
  def __hash__(self):
    return 5

c = C()
d = {}
d[C()] = 0
)"),
            0);
  PyObjectPtr c(mainModuleGet("c"));
  PyObjectPtr dict(mainModuleGet("d"));
  PyObjectPtr value(PyLong_FromLong(1));
  ASSERT_EQ(PyDict_SetItem(dict, c, value), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  PyObjectPtr called_dunder_eq(mainModuleGet("called_dunder_eq"));
  ASSERT_EQ(called_dunder_eq, Py_True);
  EXPECT_EQ(PyDict_Size(dict), 2);
}

TEST_F(DictExtensionApiTest, SetItemWithExceptionDunderEqRaisesException) {
  ASSERT_EQ(PyRun_SimpleString(R"(
called_dunder_eq = False
class C:
  def __eq__(self, other):
    global called_dunder_eq
    called_dunder_eq = True
    raise ValueError('foo')
  def __hash__(self):
    return 5

c = C()
d = {}
d[C()] = 0
)"),
            0);
  PyObjectPtr c(mainModuleGet("c"));
  PyObjectPtr dict(mainModuleGet("d"));
  PyObjectPtr value(PyLong_FromLong(1));
  EXPECT_EQ(PyDict_SetItem(dict, c, value), -1);
  EXPECT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_ValueError));
  PyErr_Clear();
  PyObjectPtr called_dunder_eq(mainModuleGet("called_dunder_eq"));
  ASSERT_EQ(called_dunder_eq, Py_True);
  EXPECT_EQ(PyDict_Size(dict), 1);
}

TEST_F(DictExtensionApiTest, SetItemWithNotImplementedDunderEqInsertsValue) {
  ASSERT_EQ(PyRun_SimpleString(R"(
called_dunder_eq = False
class C:
  def __eq__(self, other):
    global called_dunder_eq
    called_dunder_eq = True
    return NotImplemented
  def __hash__(self):
    return 5

c = C()
d = {}
d[C()] = 0
)"),
            0);
  PyObjectPtr c(mainModuleGet("c"));
  PyObjectPtr dict(mainModuleGet("d"));
  PyObjectPtr value(PyLong_FromLong(1));
  ASSERT_EQ(PyDict_SetItem(dict, c, value), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  PyObjectPtr called_dunder_eq(mainModuleGet("called_dunder_eq"));
  ASSERT_EQ(called_dunder_eq, Py_True);
  EXPECT_EQ(PyDict_Size(dict), 2);
}

TEST_F(DictExtensionApiTest,
       SetItemCallsExistingKeyDunderEqAndThenLookedKeyDunderEq) {
  ASSERT_EQ(PyRun_SimpleString(R"(
seq_num = 0

def new_seq_num():
  global seq_num
  seq_num += 1
  return seq_num

c_eq = 0
c_hash = 0

class C:
  def __eq__(self, other):
    global c_eq
    c_eq = new_seq_num()
    return NotImplemented

  def __hash__(self):
    global c_hash
    c_hash = new_seq_num()
    return 5

c = C()

d_eq = 0
d_hash = 0

class D:
  def __eq__(self, other):
    global d_eq
    d_eq = new_seq_num()
    return True

  def __hash__(self):
    global d_hash
    d_hash = new_seq_num()
    return 5

d = D()
)"),
            0);
  PyObjectPtr dict(PyDict_New());
  PyObjectPtr c(mainModuleGet("c"));
  PyObjectPtr value(PyLong_FromLong(1));

  ASSERT_EQ(PyDict_SetItem(dict, c, value), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_EQ(PyDict_Size(dict), 1);
  PyObjectPtr c_eq(mainModuleGet("c_eq"));
  ASSERT_EQ(PyLong_AsLong(c_eq), 0);
  PyObjectPtr c_hash(mainModuleGet("c_hash"));
  ASSERT_EQ(PyLong_AsLong(c_hash), 1);

  PyObjectPtr d(mainModuleGet("d"));
  ASSERT_EQ(PyDict_SetItem(dict, d, value), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_EQ(PyDict_Size(dict), 1);
  c_hash = mainModuleGet("c_hash");
  EXPECT_EQ(PyLong_AsLong(c_hash), 1);
  PyObjectPtr d_hash(mainModuleGet("d_hash"));
  EXPECT_EQ(PyLong_AsLong(d_hash), 2);
  c_eq = mainModuleGet("c_eq");
  EXPECT_EQ(PyLong_AsLong(c_eq), 3);
  PyObjectPtr d_eq(mainModuleGet("d_eq"));
  EXPECT_EQ(PyLong_AsLong(d_eq), 4);
}

TEST_F(DictExtensionApiTest, SetItemRetainsExistingKeyObject) {
  ASSERT_EQ(PyRun_SimpleString(R"(
class C:
  def __eq__(self, other):
    return True

  def __hash__(self):
    return 5

c = C()
d = C()
)"),
            0);
  PyObjectPtr dict(PyDict_New());
  PyObjectPtr c(mainModuleGet("c"));
  PyObjectPtr d(mainModuleGet("d"));
  PyObjectPtr c_value(PyLong_FromLong(1));
  PyObjectPtr d_value(PyLong_FromLong(2));

  ASSERT_EQ(PyDict_SetItem(dict, c, c_value), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_EQ(PyDict_Size(dict), 1);

  ASSERT_EQ(PyDict_SetItem(dict, d, d_value), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_EQ(PyDict_Size(dict), 1);

  PyObjectPtr result(PyDict_Items(dict));
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyList_CheckExact(result));
  EXPECT_EQ(PyList_Size(result), 1);

  PyObject* kv = PyList_GetItem(result, 0);
  ASSERT_TRUE(PyTuple_CheckExact(kv));
  ASSERT_EQ(PyTuple_Size(kv), 2);
  EXPECT_EQ(PyTuple_GetItem(kv, 0), c);
  EXPECT_EQ(PyTuple_GetItem(kv, 1), d_value);
}

TEST_F(DictExtensionApiTest, SetItemComparesHashValueFirst) {
  ASSERT_EQ(PyRun_SimpleString(R"(
class C:
  def __init__(self, hash_code):
    self.hash_code = hash_code

  def __eq__(self, other):
    raise UserWarning("unexpected")

  def __hash__(self):
    return self.hash_code

c = C(4)
d = C(5)
)"),
            0);
  PyObjectPtr dict(PyDict_New());
  PyObjectPtr c(mainModuleGet("c"));
  PyObjectPtr value(PyLong_FromLong(500));

  ASSERT_EQ(PyDict_SetItem(dict, c, value), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_EQ(PyDict_Size(dict), 1);

  PyObjectPtr d(mainModuleGet("d"));
  ASSERT_EQ(PyDict_SetItem(dict, d, value), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_EQ(PyDict_Size(dict), 2);
}

TEST_F(DictExtensionApiTest, SizeWithNonDictReturnsNegative) {
  PyObject* list = PyList_New(0);
  EXPECT_EQ(PyDict_Size(list), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));

  Py_DECREF(list);
}

TEST_F(DictExtensionApiTest, SizeWithEmptyDictReturnsZero) {
  PyObjectPtr dict(PyDict_New());
  EXPECT_EQ(PyDict_Size(dict), 0);
}

TEST_F(DictExtensionApiTest, SizeWithNonEmptyDict) {
  PyObjectPtr dict(PyDict_New());
  PyObjectPtr key1(PyLong_FromLong(1));
  PyObjectPtr key2(PyLong_FromLong(2));
  PyObjectPtr value1(PyLong_FromLong(0));
  PyObjectPtr value2(PyLong_FromLong(0));
  PyObjectPtr value3(PyLong_FromLong(0));

  // Dict starts out empty
  EXPECT_EQ(PyDict_Size(dict), 0);

  // Inserting items for two different keys
  ASSERT_EQ(PyDict_SetItem(dict, key1, value1), 0);
  ASSERT_EQ(PyDict_SetItem(dict, key2, value2), 0);
  EXPECT_EQ(PyDict_Size(dict), 2);

  // Replace value for existing key
  ASSERT_EQ(PyDict_SetItem(dict, key1, value3), 0);
  EXPECT_EQ(PyDict_Size(dict), 2);
}

TEST_F(DictExtensionApiTest, ContainsWithKeyInDictReturnsOne) {
  PyObjectPtr dict(PyDict_New());
  PyObjectPtr key(PyLong_FromLong(10));
  PyObjectPtr value(PyLong_FromLong(11));
  ASSERT_EQ(PyDict_SetItem(dict, key, value), 0);
  EXPECT_EQ(PyDict_Contains(dict, key), 1);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(DictExtensionApiTest, ContainsWithKeyNotInDictReturnsZero) {
  PyObjectPtr dict(PyDict_New());
  PyObjectPtr key(PyLong_FromLong(10));
  PyObjectPtr value(PyLong_FromLong(11));
  ASSERT_EQ(PyDict_SetItem(dict, key, value), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  PyObjectPtr key2(PyLong_FromLong(666));
  EXPECT_EQ(PyDict_Contains(dict, key2), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(DictExtensionApiTest, ItemsWithNonDictRaisesSystemError) {
  ASSERT_EQ(PyDict_Items(Py_None), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(DictExtensionApiTest, ItemsWithDictReturnsList) {
  PyObjectPtr dict(PyDict_New());
  PyObjectPtr key(PyLong_FromLong(10));
  PyObjectPtr value(PyLong_FromLong(11));
  ASSERT_EQ(PyDict_SetItem(dict, key, value), 0);

  PyObjectPtr result(PyDict_Items(dict));
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyList_CheckExact(result));
  EXPECT_EQ(PyList_Size(result), 1);

  PyObject* kv = PyList_GetItem(result, 0);
  ASSERT_TRUE(PyTuple_CheckExact(kv));
  ASSERT_EQ(PyTuple_Size(kv), 2);
  EXPECT_EQ(PyTuple_GetItem(kv, 0), key);
  EXPECT_EQ(PyTuple_GetItem(kv, 1), value);
}

TEST_F(DictExtensionApiTest, KeysWithNonDictRaisesSystemError) {
  EXPECT_EQ(PyDict_Keys(Py_None), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(DictExtensionApiTest, KeysWithDictReturnsList) {
  PyObjectPtr dict(PyDict_New());

  PyObjectPtr key(PyLong_FromLong(10));
  PyObjectPtr value(PyLong_FromLong(11));
  ASSERT_EQ(PyDict_SetItem(dict, key, value), 0);

  PyObjectPtr result(PyDict_Keys(dict));
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyList_CheckExact(result));
  EXPECT_EQ(PyList_Size(result), 1);
  EXPECT_EQ(PyList_GetItem(result, 0), key);
}

TEST_F(DictExtensionApiTest, ValuesWithNonDictReturnsNull) {
  EXPECT_EQ(PyDict_Values(Py_None), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(DictExtensionApiTest, ValuesWithEmptyDictReturnsEmptyList) {
  PyObjectPtr dict(PyDict_New());
  PyObjectPtr result(PyDict_Values(dict));
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyList_CheckExact(result));
  EXPECT_EQ(PyList_Size(result), 0);
}

TEST_F(DictExtensionApiTest, ValuesWithNonEmptyDictReturnsNonEmptyList) {
  PyObjectPtr dict(PyDict_New());

  PyObjectPtr key(PyLong_FromLong(10));
  PyObjectPtr value(PyLong_FromLong(11));
  ASSERT_EQ(PyDict_SetItem(dict, key, value), 0);

  PyObjectPtr result(PyDict_Values(dict));
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyList_CheckExact(result));
  EXPECT_EQ(PyList_Size(result), 1);
  EXPECT_EQ(PyList_GetItem(result, 0), value);
}

TEST_F(DictExtensionApiTest, ClearWithNonDictDoesNotRaise) {
  PyDict_Clear(Py_None);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(DictExtensionApiTest, ClearRemovesAllItems) {
  PyObjectPtr dict(PyDict_New());
  PyObjectPtr one(PyLong_FromLong(1));
  PyObjectPtr two(PyLong_FromLong(1));
  PyDict_SetItem(dict, one, two);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  PyObjectPtr three(PyLong_FromLong(1));
  PyObjectPtr four(PyLong_FromLong(1));
  PyDict_SetItem(dict, three, four);
  ASSERT_EQ(PyErr_Occurred(), nullptr);

  PyDict_Clear(dict);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyDict_Size(dict), 0);
}

TEST_F(DictExtensionApiTest, GETSIZEWithEmptyDictReturnsZero) {
  PyObjectPtr dict(PyDict_New());
  EXPECT_EQ(PyDict_GET_SIZE(dict.get()), 0);
}

TEST_F(DictExtensionApiTest, GETSIZEWithNonEmptyDict) {
  PyObjectPtr dict(PyDict_New());
  PyObjectPtr key1(PyLong_FromLong(1));
  PyObjectPtr key2(PyLong_FromLong(2));
  PyObjectPtr value1(PyLong_FromLong(0));
  PyObjectPtr value2(PyLong_FromLong(0));
  PyObjectPtr value3(PyLong_FromLong(0));

  // Dict starts out empty
  EXPECT_EQ(PyDict_GET_SIZE(dict.get()), 0);

  // Inserting items for two different keys
  ASSERT_EQ(PyDict_SetItem(dict, key1, value1), 0);
  ASSERT_EQ(PyDict_SetItem(dict, key2, value2), 0);
  EXPECT_EQ(PyDict_GET_SIZE(dict.get()), 2);

  // Replace value for existing key
  ASSERT_EQ(PyDict_SetItem(dict, key1, value3), 0);
  EXPECT_EQ(PyDict_GET_SIZE(dict.get()), 2);
}

TEST_F(DictExtensionApiTest, GetItemWithErrorNonExistingKeyReturnsNull) {
  PyObjectPtr dict(PyDict_New());
  PyObjectPtr key(PyLong_FromLong(666));
  PyObjectPtr result(PyDict_GetItemWithError(dict, key));
  EXPECT_EQ(result, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(DictExtensionApiTest, GetItemWithErrorReturnsBorrowedValue) {
  PyObjectPtr dict(PyDict_New());
  PyObjectPtr key(PyLong_FromLong(10));
  PyObjectPtr value(PyLong_FromLong(666));

  // Insert the value into the dictionary
  ASSERT_EQ(PyDict_SetItem(dict, key, value), 0);

  // Record the reference count of the value
  long refcnt = Py_REFCNT(value);

  // Get a new reference to the value from the dictionary
  PyObject* value2 = PyDict_GetItemWithError(dict, key);
  ASSERT_EQ(PyErr_Occurred(), nullptr);

  // The new reference should be equal to the original reference
  EXPECT_EQ(value2, value);

  // The reference count should not be affected
  EXPECT_EQ(Py_REFCNT(value), refcnt);
}

TEST_F(DictExtensionApiTest, GetItemWithErrorWithDictSubclassReturnsValue) {
  PyRun_SimpleString(R"(
class Foo(dict): pass
obj = Foo()
  )");

  PyObjectPtr obj(mainModuleGet("obj"));
  PyObjectPtr key(PyLong_FromLong(1));
  PyObjectPtr val(PyLong_FromLong(2));
  ASSERT_EQ(PyDict_SetItem(obj, key, val), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);

  PyObject* result = PyDict_GetItemWithError(obj, key);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(result, val);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(DictExtensionApiTest,
       GetItemWithErrorWithUnhashableObjectRaisesTypeError) {
  PyRun_SimpleString(R"(
class C:
  __hash__ = None
obj = C()
)");
  PyObjectPtr dict(PyDict_New());
  PyObjectPtr key(mainModuleGet("obj"));
  EXPECT_EQ(PyDict_GetItemWithError(dict, key), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(DictExtensionApiTest, DelItemWithNonDictReturnsNegativeOne) {
  EXPECT_EQ(PyDict_DelItem(Py_None, Py_None), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(DictExtensionApiTest, DelItemWithKeyInDictReturnsZero) {
  PyObjectPtr dict(PyDict_New());
  PyObjectPtr key(PyLong_FromLong(10));
  PyObjectPtr value(PyLong_FromLong(11));
  ASSERT_EQ(PyDict_SetItem(dict, key, value), 0);
  EXPECT_EQ(PyDict_DelItem(dict, key), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(DictExtensionApiTest, DelItemWithKeyNotInDictRaisesKeyError) {
  PyObjectPtr dict(PyDict_New());
  PyObjectPtr key(PyLong_FromLong(10));
  EXPECT_EQ(PyDict_DelItem(dict, key), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_KeyError));
}

TEST_F(DictExtensionApiTest, DelItemWithUnhashableObjectRaisesTypeError) {
  PyRun_SimpleString(R"(
class C:
  __hash__ = None
c = C()
)");
  PyObjectPtr dict(PyDict_New());
  PyObject* main = PyImport_AddModule("__main__");
  PyObjectPtr key(PyObject_GetAttrString(main, "c"));
  EXPECT_EQ(PyDict_DelItem(dict, key), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(DictExtensionApiTest, DelItemStringWithNonDictReturnsNegativeOne) {
  EXPECT_EQ(PyDict_DelItemString(Py_None, "hello, there"), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(DictExtensionApiTest, DelItemStringWithKeyInDictReturnsZero) {
  PyObjectPtr dict(PyDict_New());
  const char* strkey = "hello, there";
  PyObjectPtr key(PyUnicode_FromString(strkey));
  PyObjectPtr value(PyLong_FromLong(666));
  ASSERT_EQ(PyDict_SetItem(dict, key, value), 0);
  EXPECT_EQ(PyDict_DelItemString(dict, strkey), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(DictExtensionApiTest, DelItemStringWithKeyNotInDictReturnsNegativeOne) {
  PyObjectPtr dict(PyDict_New());
  EXPECT_EQ(PyDict_DelItemString(dict, "hello, there"), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_KeyError));
}

TEST_F(DictExtensionApiTest, NextWithEmptyDictReturnsFalse) {
  PyObject* key = nullptr;
  PyObject* value = nullptr;
  Py_ssize_t pos = 0;
  PyObjectPtr dict(PyDict_New());
  EXPECT_EQ(PyDict_Next(dict, &pos, &key, &value), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(DictExtensionApiTest, NextWithNonEmptyDictReturnsKeysAndValues) {
  PyObjectPtr dict(PyDict_New());
  PyObjectPtr one(PyLong_FromLong(1));
  PyObjectPtr two(PyLong_FromLong(2));
  PyDict_SetItem(dict, one, two);
  PyObjectPtr three(PyLong_FromLong(3));
  PyObjectPtr four(PyLong_FromLong(4));
  PyDict_SetItem(dict, three, four);

  Py_ssize_t pos = 0;
  PyObject* key = nullptr;
  PyObject* value = nullptr;
  ASSERT_EQ(PyDict_Next(dict, &pos, &key, &value), 1);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(key, one);
  EXPECT_EQ(value, two);

  ASSERT_EQ(PyDict_Next(dict, &pos, &key, &value), 1);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(key, three);
  EXPECT_EQ(value, four);

  ASSERT_EQ(PyDict_Next(dict, &pos, &key, &value), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(DictExtensionApiTest, NextAcceptsNullKeyPointer) {
  PyObjectPtr dict(PyDict_New());
  PyObjectPtr one(PyLong_FromLong(1));
  PyObjectPtr two(PyLong_FromLong(2));
  PyDict_SetItem(dict, one, two);
  PyObjectPtr three(PyLong_FromLong(3));
  PyObjectPtr four(PyLong_FromLong(4));
  PyDict_SetItem(dict, three, four);

  Py_ssize_t pos = 0;
  PyObject* value = nullptr;
  ASSERT_EQ(PyDict_Next(dict, &pos, nullptr, &value), 1);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(DictExtensionApiTest, NextAcceptsNullValuePointer) {
  PyObjectPtr dict(PyDict_New());
  PyObjectPtr one(PyLong_FromLong(1));
  PyObjectPtr two(PyLong_FromLong(2));
  PyDict_SetItem(dict, one, two);
  PyObjectPtr three(PyLong_FromLong(3));
  PyObjectPtr four(PyLong_FromLong(4));
  PyDict_SetItem(dict, three, four);

  Py_ssize_t pos = 0;
  PyObject* key = nullptr;
  ASSERT_EQ(PyDict_Next(dict, &pos, &key, nullptr), 1);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(DictExtensionApiTest, UnderNextWithEmptyDictReturnsFalse) {
  PyObject* key = nullptr;
  PyObject* value = nullptr;
  Py_hash_t hash = 0;
  Py_ssize_t pos = 0;
  PyObjectPtr dict(PyDict_New());
  EXPECT_EQ(_PyDict_Next(dict, &pos, &key, &value, &hash), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(DictExtensionApiTest, UnderNextWithNonEmptyDictReturnsKeysAndValues) {
  PyObjectPtr dict(PyDict_New());
  PyObjectPtr one(PyLong_FromLong(1));
  PyObjectPtr two(PyLong_FromLong(2));
  PyDict_SetItem(dict, one, two);
  PyObjectPtr three(PyLong_FromLong(3));
  PyObjectPtr four(PyLong_FromLong(4));
  PyDict_SetItem(dict, three, four);

  Py_ssize_t pos = 0;
  PyObject* key = nullptr;
  PyObject* value = nullptr;
  Py_hash_t hash = 0;
  ASSERT_EQ(_PyDict_Next(dict, &pos, &key, &value, &hash), 1);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(key, one);
  EXPECT_EQ(value, two);
  EXPECT_EQ(hash, 1);

  ASSERT_EQ(_PyDict_Next(dict, &pos, &key, &value, &hash), 1);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(key, three);
  EXPECT_EQ(value, four);
  EXPECT_EQ(hash, 3);

  ASSERT_EQ(_PyDict_Next(dict, &pos, &key, &value, &hash), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(DictExtensionApiTest, UnderNextAcceptsNullKeyPointer) {
  PyObjectPtr dict(PyDict_New());
  PyObjectPtr one(PyLong_FromLong(1));
  PyObjectPtr two(PyLong_FromLong(2));
  PyDict_SetItem(dict, one, two);
  PyObjectPtr three(PyLong_FromLong(3));
  PyObjectPtr four(PyLong_FromLong(4));
  PyDict_SetItem(dict, three, four);

  Py_ssize_t pos = 0;
  PyObject* value = nullptr;
  Py_hash_t hash = 0;
  ASSERT_EQ(_PyDict_Next(dict, &pos, nullptr, &value, &hash), 1);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(DictExtensionApiTest, UnderNextAcceptsNullValuePointer) {
  PyObjectPtr dict(PyDict_New());
  PyObjectPtr one(PyLong_FromLong(1));
  PyObjectPtr two(PyLong_FromLong(2));
  PyDict_SetItem(dict, one, two);
  PyObjectPtr three(PyLong_FromLong(3));
  PyObjectPtr four(PyLong_FromLong(4));
  PyDict_SetItem(dict, three, four);

  Py_ssize_t pos = 0;
  PyObject* key = nullptr;
  Py_hash_t hash = 0;
  ASSERT_EQ(_PyDict_Next(dict, &pos, &key, nullptr, &hash), 1);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(DictExtensionApiTest, UnderNextAcceptsNullHashPointer) {
  PyObjectPtr dict(PyDict_New());
  PyObjectPtr one(PyLong_FromLong(1));
  PyObjectPtr two(PyLong_FromLong(2));
  PyDict_SetItem(dict, one, two);
  PyObjectPtr three(PyLong_FromLong(3));
  PyObjectPtr four(PyLong_FromLong(4));
  PyDict_SetItem(dict, three, four);

  Py_ssize_t pos = 0;
  PyObject* key = nullptr;
  PyObject* value = nullptr;
  ASSERT_EQ(_PyDict_Next(dict, &pos, &key, &value, nullptr), 1);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(DictExtensionApiTest, CopyWithNullRaisesSystemError) {
  EXPECT_EQ(PyDict_Copy(nullptr), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(DictExtensionApiTest, CopyWithNonDictInstanceRaisesSystemError) {
  EXPECT_EQ(PyDict_Copy(Py_None), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(DictExtensionApiTest, CopyMakesShallowCopyOfDictElements) {
  PyObjectPtr dict(PyDict_New());
  PyObjectPtr one(PyLong_FromLong(1));
  PyObjectPtr val1(PyTuple_New(0));
  PyDict_SetItem(dict, one, val1);
  PyObjectPtr three(PyLong_FromLong(3));
  PyObjectPtr val2(PyTuple_New(0));
  PyDict_SetItem(dict, three, val2);

  PyObjectPtr copy(PyDict_Copy(dict));
  ASSERT_NE(copy, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyDict_CheckExact(copy));
  EXPECT_EQ(PyDict_Size(copy), 2);
  EXPECT_EQ(PyDict_GetItem(copy, one), val1);
  EXPECT_EQ(PyDict_GetItem(copy, three), val2);
}

TEST_F(DictExtensionApiTest, MergeWithNullLhsRaisesSystemError) {
  PyObjectPtr rhs(PyDict_New());
  ASSERT_EQ(PyDict_Merge(nullptr, rhs, 0), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(DictExtensionApiTest, MergeWithNonDictLhsRaisesSystemError) {
  PyObjectPtr rhs(PyDict_New());
  ASSERT_EQ(PyDict_Merge(Py_None, rhs, 0), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(DictExtensionApiTest, MergeWithNullRhsRaisesSystemError) {
  PyObjectPtr lhs(PyDict_New());
  ASSERT_EQ(PyDict_Merge(lhs, nullptr, 0), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(DictExtensionApiTest, MergeAddsKeysToLhs) {
  PyObjectPtr rhs(PyDict_New());
  PyObjectPtr one(PyLong_FromLong(1));
  PyObjectPtr two(PyLong_FromLong(2));
  PyDict_SetItem(rhs, one, two);
  PyObjectPtr three(PyLong_FromLong(3));
  PyObjectPtr four(PyLong_FromLong(4));
  PyDict_SetItem(rhs, three, four);

  PyObjectPtr lhs(PyDict_New());
  ASSERT_EQ(PyDict_Merge(lhs, rhs, 0), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyDict_Size(lhs), 2);

  EXPECT_TRUE(PyDict_Contains(lhs, one));
  EXPECT_EQ(PyDict_GetItem(lhs, one), two);

  EXPECT_TRUE(PyDict_Contains(lhs, three));
  EXPECT_EQ(PyDict_GetItem(lhs, three), four);
}

TEST_F(DictExtensionApiTest, MergeWithoutOverrideIgnoresKeys) {
  PyObjectPtr lhs(PyDict_New());
  PyObjectPtr rhs(PyDict_New());
  PyObjectPtr one(PyLong_FromLong(1));
  PyObjectPtr two(PyLong_FromLong(2));
  PyDict_SetItem(lhs, one, two);
  PyDict_SetItem(rhs, one, two);
  PyObjectPtr three(PyLong_FromLong(3));
  PyObjectPtr four(PyLong_FromLong(4));
  PyDict_SetItem(rhs, three, four);
  PyObjectPtr not_in_rhs(PyLong_FromLong(666));
  PyDict_SetItem(lhs, three, not_in_rhs);

  ASSERT_EQ(PyDict_Merge(lhs, rhs, 0), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyDict_Size(lhs), 2);
  EXPECT_EQ(PyDict_GetItem(lhs, one), two);
  EXPECT_EQ(PyDict_GetItem(lhs, three), not_in_rhs);
}

TEST_F(DictExtensionApiTest, MergeWithOverrideReplacesKeys) {
  PyObjectPtr lhs(PyDict_New());
  PyObjectPtr rhs(PyDict_New());
  PyObjectPtr one(PyLong_FromLong(1));
  PyObjectPtr two(PyLong_FromLong(2));
  PyDict_SetItem(lhs, one, two);
  PyDict_SetItem(rhs, one, two);
  PyObjectPtr three(PyLong_FromLong(3));
  PyObjectPtr four(PyLong_FromLong(4));
  PyDict_SetItem(rhs, three, four);
  PyObjectPtr not_in_rhs(PyLong_FromLong(666));
  PyDict_SetItem(lhs, three, not_in_rhs);

  ASSERT_EQ(PyDict_Merge(lhs, rhs, 1), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyDict_Size(lhs), 2);

  EXPECT_TRUE(PyDict_Contains(lhs, one));
  EXPECT_EQ(PyDict_GetItem(lhs, one), two);

  EXPECT_TRUE(PyDict_Contains(lhs, three));
  EXPECT_EQ(PyDict_GetItem(lhs, three), four);
}

TEST_F(DictExtensionApiTest, MergeWithNonMappingRaisesAttributeError) {
  PyRun_SimpleString(R"(
class Mapping:
  pass
m = Mapping()
)");
  PyObjectPtr rhs(mainModuleGet("m"));
  PyObjectPtr lhs(PyDict_New());
  ASSERT_EQ(PyDict_Merge(lhs, rhs, 0), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_AttributeError));
}

TEST_F(DictExtensionApiTest, MergeWithMappingRhsAddsKeysToLhs) {
  PyRun_SimpleString(R"(
class Mapping:
    def __init__(self):
        self.d = {1:2, 3:4}
    def keys(self):
        return self.d.keys()
    def __getitem__(self, i):
        return self.d[i]
m = Mapping()
)");
  PyObjectPtr rhs(mainModuleGet("m"));
  PyObjectPtr lhs(PyDict_New());
  ASSERT_EQ(PyDict_Merge(lhs, rhs, 0), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyDict_Size(lhs), 2);

  PyObjectPtr one(PyLong_FromLong(1));
  EXPECT_TRUE(PyDict_Contains(lhs, one));
  PyObject* two = PyDict_GetItem(lhs, one);
  EXPECT_EQ(PyLong_AsLong(two), 2);

  PyObjectPtr three(PyLong_FromLong(3));
  EXPECT_TRUE(PyDict_Contains(lhs, three));
  PyObject* four = PyDict_GetItem(lhs, three);
  EXPECT_EQ(PyLong_AsLong(four), 4);
}

TEST_F(DictExtensionApiTest, UpdateWithNullLhsRaisesSystemError) {
  PyObjectPtr rhs(PyDict_New());
  ASSERT_EQ(PyDict_Update(nullptr, rhs), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(DictExtensionApiTest, UpdateWithNonListLhsRaisesSystemError) {
  PyObjectPtr rhs(PyDict_New());
  ASSERT_EQ(PyDict_Update(Py_None, rhs), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(DictExtensionApiTest, UpdateWithNullRhsRaisesSystemError) {
  PyObjectPtr lhs(PyDict_New());
  ASSERT_EQ(PyDict_Update(lhs, nullptr), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(DictExtensionApiTest, UpdateWithLhsEqualRhsDoesNothing) {
  PyObjectPtr lhs(PyDict_New());
  PyObject* rhs = lhs;
  ASSERT_EQ(PyDict_Update(lhs, rhs), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(lhs, rhs);
}

TEST_F(DictExtensionApiTest, UpdateWithEmptyRhsDoesNothing) {
  PyObjectPtr lhs(PyDict_New());

  PyObjectPtr one(PyLong_FromLong(1));
  PyObjectPtr two(PyLong_FromLong(2));
  PyDict_SetItem(lhs, one, two);
  PyObjectPtr three(PyLong_FromLong(3));
  PyObjectPtr four(PyLong_FromLong(4));
  PyDict_SetItem(lhs, three, four);
  ASSERT_EQ(PyDict_Size(lhs), 2);

  PyObjectPtr rhs(PyDict_New());
  ASSERT_EQ(PyDict_Update(lhs, rhs), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyDict_Size(lhs), 2);
  EXPECT_EQ(PyDict_GetItem(lhs, one), two);
  EXPECT_EQ(PyDict_GetItem(lhs, three), four);
}

TEST_F(DictExtensionApiTest, UpdateWithEmptyLhsAddsKeysToLhs) {
  PyObjectPtr rhs(PyDict_New());
  PyObjectPtr one(PyLong_FromLong(1));
  PyObjectPtr two(PyLong_FromLong(2));
  PyDict_SetItem(rhs, one, two);
  PyObjectPtr three(PyLong_FromLong(3));
  PyObjectPtr four(PyLong_FromLong(4));
  PyDict_SetItem(rhs, three, four);
  ASSERT_EQ(PyDict_Size(rhs), 2);

  PyObjectPtr lhs(PyDict_New());
  ASSERT_EQ(PyDict_Update(lhs, rhs), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyDict_Size(lhs), 2);

  EXPECT_TRUE(PyDict_Contains(lhs, one));
  EXPECT_EQ(PyDict_GetItem(lhs, one), two);
  EXPECT_TRUE(PyDict_Contains(lhs, three));
  EXPECT_EQ(PyDict_GetItem(lhs, three), four);
}

TEST_F(DictExtensionApiTest, UpdateOverwritesKeys) {
  PyObjectPtr rhs(PyDict_New());
  PyObjectPtr one(PyLong_FromLong(1));
  PyObjectPtr two(PyLong_FromLong(2));
  PyDict_SetItem(rhs, one, two);
  PyObjectPtr three(PyLong_FromLong(3));
  PyObjectPtr four(PyLong_FromLong(4));
  PyDict_SetItem(rhs, three, four);
  ASSERT_EQ(PyDict_Size(rhs), 2);

  PyObjectPtr lhs(PyDict_New());
  PyObjectPtr not_in_rhs(PyLong_FromLong(666));
  ASSERT_EQ(PyDict_SetItem(lhs, one, not_in_rhs), 0);
  ASSERT_EQ(PyDict_GetItem(lhs, one), not_in_rhs);

  ASSERT_EQ(PyDict_Update(lhs, rhs), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyDict_Size(lhs), 2);

  EXPECT_TRUE(PyDict_Contains(lhs, one));
  EXPECT_EQ(PyDict_GetItem(lhs, one), two);

  EXPECT_TRUE(PyDict_Contains(lhs, three));
  EXPECT_EQ(PyDict_GetItem(lhs, three), four);
}

}  // namespace testing
}  // namespace py
