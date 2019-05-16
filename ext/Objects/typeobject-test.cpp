#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"
#include "structmember.h"

namespace python {

using namespace testing;

using TypeExtensionApiTest = ExtensionApi;
using TypeExtensionApiDeathTest = ExtensionApi;

TEST_F(TypeExtensionApiTest, PyTypeCheckOnLong) {
  PyObjectPtr pylong(PyLong_FromLong(10));
  EXPECT_FALSE(PyType_Check(pylong));
  EXPECT_FALSE(PyType_CheckExact(pylong));
}

TEST_F(TypeExtensionApiTest, PyTypeCheckOnType) {
  PyObjectPtr pylong(PyLong_FromLong(10));
  PyObjectPtr pylong_type(PyObject_Type(pylong));
  EXPECT_TRUE(PyType_Check(pylong_type));
  EXPECT_TRUE(PyType_CheckExact(pylong_type));
}

TEST_F(TypeExtensionApiDeathTest, GetFlagsFromBuiltInTypePyro) {
  PyObjectPtr pylong(PyLong_FromLong(5));
  PyObjectPtr pylong_type(PyObject_Type(pylong));
  ASSERT_TRUE(PyType_CheckExact(pylong_type));
  EXPECT_DEATH(
      PyType_GetFlags(reinterpret_cast<PyTypeObject*>(pylong_type.get())),
      "unimplemented: GetFlags from built-in types");
}

TEST_F(TypeExtensionApiDeathTest, GetFlagsFromManagedTypePyro) {
  PyRun_SimpleString(R"(class Foo: pass)");
  PyObjectPtr foo_type(testing::moduleGet("__main__", "Foo"));
  ASSERT_TRUE(PyType_CheckExact(foo_type));
  EXPECT_DEATH(
      PyType_GetFlags(reinterpret_cast<PyTypeObject*>(foo_type.get())),
      "unimplemented: GetFlags from types initialized through Python code");
}

TEST_F(TypeExtensionApiTest, GetFlagsFromExtensionTypeReturnsSetFlags) {
  PyType_Slot slots[] = {
      {0, nullptr},
  };
  static PyType_Spec spec;
  spec = {
      "foo.Bar", 0, 0, Py_TPFLAGS_DEFAULT, slots,
  };
  PyObjectPtr type(PyType_FromSpec(&spec));
  ASSERT_NE(type, nullptr);
  ASSERT_TRUE(PyType_CheckExact(type));

  EXPECT_TRUE(PyType_GetFlags(reinterpret_cast<PyTypeObject*>(type.get())) &
              Py_TPFLAGS_DEFAULT);
  EXPECT_TRUE(PyType_GetFlags(reinterpret_cast<PyTypeObject*>(type.get())) &
              Py_TPFLAGS_READY);
}

TEST_F(TypeExtensionApiTest, FromSpecCreatesRuntimeType) {
  PyType_Slot slots[] = {
      {0, nullptr},
  };
  static PyType_Spec spec;
  spec = {
      "foo.Bar", 0, 0, Py_TPFLAGS_DEFAULT, slots,
  };
  PyObjectPtr type(PyType_FromSpec(&spec));
  ASSERT_NE(type, nullptr);
  ASSERT_TRUE(PyType_CheckExact(type));

  testing::moduleSet("__main__", "Empty", type);
  PyRun_SimpleString("x = Empty");
  EXPECT_TRUE(PyType_CheckExact(testing::moduleGet("__main__", "x")));
}

TEST_F(TypeExtensionApiTest, FromSpecWithInvalidSlotRaisesError) {
  PyType_Slot slots[] = {
      {-1, nullptr},
  };
  static PyType_Spec spec;
  spec = {
      "foo.Bar", 0, 0, Py_TPFLAGS_DEFAULT, slots,
  };
  ASSERT_EQ(PyType_FromSpec(&spec), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_RuntimeError));
  // TODO(eelizondo): Check that error matches with "invalid slot offset"
}

TEST_F(TypeExtensionApiTest, CallExtensionTypeReturnsExtensionInstancePyro) {
  // clang-format off
  struct BarObject {
    PyObject_HEAD
    int value;
  };
  // clang-format on
  newfunc new_func = [](PyTypeObject* type, PyObject*, PyObject*) {
    void* slot = PyType_GetSlot(type, Py_tp_alloc);
    return reinterpret_cast<allocfunc>(slot)(type, 0);
  };
  initproc init_func = [](PyObject* self, PyObject*, PyObject*) {
    reinterpret_cast<BarObject*>(self)->value = 30;
    return 0;
  };
  destructor dealloc_func = [](PyObject* self) {
    PyObjectPtr type(PyObject_Type(self));
    void* slot =
        PyType_GetSlot(reinterpret_cast<PyTypeObject*>(type.get()), Py_tp_free);
    return reinterpret_cast<freefunc>(slot)(self);
  };
  PyType_Slot slots[] = {
      {Py_tp_alloc, reinterpret_cast<void*>(PyType_GenericAlloc)},
      {Py_tp_new, reinterpret_cast<void*>(new_func)},
      {Py_tp_init, reinterpret_cast<void*>(init_func)},
      {Py_tp_dealloc, reinterpret_cast<void*>(dealloc_func)},
      {Py_tp_free, reinterpret_cast<void*>(PyObject_Del)},
      {0, nullptr},
  };
  static PyType_Spec spec;
  spec = {
      "foo.Bar", sizeof(BarObject), 0, Py_TPFLAGS_DEFAULT, slots,
  };
  PyObjectPtr type(PyType_FromSpec(&spec));
  ASSERT_NE(type, nullptr);
  ASSERT_TRUE(PyType_CheckExact(type));

  testing::moduleSet("__main__", "Bar", type);
  PyRun_SimpleString(R"(
bar = Bar()
)");

  PyObjectPtr bar(testing::moduleGet("__main__", "bar"));
  ASSERT_NE(bar, nullptr);
  BarObject* barobj = reinterpret_cast<BarObject*>(bar.get());
  EXPECT_EQ(barobj->value, 30);
  EXPECT_EQ(Py_REFCNT(barobj), 2);
  // TODO(T42827325): This DECREF is here to make up for a reference counting
  // bug in our handle code.
  Py_DECREF(barobj);
}

TEST_F(TypeExtensionApiTest, GenericAllocationReturnsMallocMemory) {
  // These numbers determine the allocated size of the PyObject
  // The values in this test are abitrary and are usally set with `sizeof(Foo)`
  int basic_size = 10;
  int item_size = 5;
  destructor dealloc_func = [](PyObject* self) {
    PyObjectPtr type(PyObject_Type(self));
    auto free_function = reinterpret_cast<freefunc>(PyType_GetSlot(
        reinterpret_cast<PyTypeObject*>(type.get()), Py_tp_free));
    return free_function(self);
  };
  PyType_Slot slots[] = {
      {Py_tp_dealloc, reinterpret_cast<void*>(dealloc_func)},
      {Py_tp_free, reinterpret_cast<void*>(PyObject_Del)},
      {0, nullptr},
  };
  static PyType_Spec spec;
  spec = {
      "foo.Bar", basic_size, item_size, Py_TPFLAGS_DEFAULT, slots,
  };
  PyObjectPtr type(PyType_FromSpec(&spec));
  ASSERT_NE(type, nullptr);
  ASSERT_TRUE(PyType_CheckExact(type));

  PyObjectPtr result(PyType_GenericAlloc(
      reinterpret_cast<PyTypeObject*>(type.get()), item_size));
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(Py_REFCNT(result), 1);
  EXPECT_EQ(Py_SIZE(result.get()), item_size);
}

TEST_F(TypeExtensionApiTest, IsSubtypeWithSameTypeReturnsTrue) {
  PyObjectPtr pylong(PyLong_FromLong(10));
  PyObjectPtr pylong_type(PyObject_Type(pylong));
  EXPECT_TRUE(
      PyType_IsSubtype(reinterpret_cast<PyTypeObject*>(pylong_type.get()),
                       reinterpret_cast<PyTypeObject*>(pylong_type.get())));
}

TEST_F(TypeExtensionApiTest, IsSubtypeWithSubtypeReturnsTrue) {
  EXPECT_EQ(PyRun_SimpleString("class MyFloat(float): pass"), 0);
  PyObjectPtr pyfloat(PyFloat_FromDouble(1.23));
  PyObjectPtr pyfloat_type(PyObject_Type(pyfloat));
  PyObjectPtr myfloat_type(moduleGet("__main__", "MyFloat"));
  EXPECT_TRUE(
      PyType_IsSubtype(reinterpret_cast<PyTypeObject*>(myfloat_type.get()),
                       reinterpret_cast<PyTypeObject*>(pyfloat_type.get())));
}

TEST_F(TypeExtensionApiTest, IsSubtypeWithDifferentTypesReturnsFalse) {
  PyObjectPtr pylong(PyLong_FromLong(10));
  PyObjectPtr pylong_type(PyObject_Type(pylong));
  PyObjectPtr pyuni(PyUnicode_FromString("string"));
  PyObjectPtr pyuni_type(PyObject_Type(pyuni));
  EXPECT_FALSE(
      PyType_IsSubtype(reinterpret_cast<PyTypeObject*>(pylong_type.get()),
                       reinterpret_cast<PyTypeObject*>(pyuni_type.get())));
}

TEST_F(TypeExtensionApiTest, GetSlotFromBuiltinTypeRaisesSystemError) {
  PyObjectPtr pylong(PyLong_FromLong(5));
  PyObjectPtr pylong_type(PyObject_Type(pylong));
  ASSERT_TRUE(
      PyType_CheckExact(reinterpret_cast<PyTypeObject*>(pylong_type.get())));

  EXPECT_EQ(PyType_GetSlot(reinterpret_cast<PyTypeObject*>(pylong_type.get()),
                           Py_tp_new),
            nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(TypeExtensionApiDeathTest,
       GetSlotFromManagedTypeReturnsFunctionPointerPyro) {
  PyRun_SimpleString(R"(
class Foo:
    def __init__(self):
        pass
  )");

  PyObjectPtr foo_type(testing::moduleGet("__main__", "Foo"));
  ASSERT_TRUE(PyType_CheckExact(foo_type));
  EXPECT_DEATH(PyType_GetSlot(reinterpret_cast<PyTypeObject*>(foo_type.get()),
                              Py_tp_init),
               "Get slots from types initialized through Python code");
}

TEST_F(TypeExtensionApiDeathTest, InitSlotWrapperReturnsInstancePyro) {
  PyRun_SimpleString(R"(
class Foo(object):
    def __init__(self):
        self.bar = 3
  )");

  PyObjectPtr foo_type(testing::moduleGet("__main__", "Foo"));
  ASSERT_TRUE(PyType_CheckExact(foo_type));
  EXPECT_DEATH(PyType_GetSlot(reinterpret_cast<PyTypeObject*>(foo_type.get()),
                              Py_tp_new),
               "Get slots from types initialized through Python code");
}

TEST_F(TypeExtensionApiDeathTest, CallSlotsWithDescriptorsReturnsInstancePyro) {
  PyRun_SimpleString(R"(
def custom_get(self, instance, value):
    return self

def custom_new(type):
    type.baz = 5
    return object.__new__(type)

def custom_init(self):
    self.bar = 3

class Foo(object): pass
Foo.__new__ = custom_new
Foo.__init__ = custom_init
  )");

  PyObjectPtr foo_type(testing::moduleGet("__main__", "Foo"));
  ASSERT_TRUE(PyType_CheckExact(foo_type));
  EXPECT_DEATH(PyType_GetSlot(reinterpret_cast<PyTypeObject*>(foo_type.get()),
                              Py_tp_new),
               "Get slots from types initialized through Python code");
}

TEST_F(TypeExtensionApiDeathTest, SlotWrapperWithArgumentsAbortsPyro) {
  PyRun_SimpleString(R"(
class Foo:
    def __new__(self, value):
        self.bar = value
  )");

  PyObjectPtr foo_type(testing::moduleGet("__main__", "Foo"));
  ASSERT_TRUE(PyType_CheckExact(foo_type));
  EXPECT_DEATH(PyType_GetSlot(reinterpret_cast<PyTypeObject*>(foo_type.get()),
                              Py_tp_new),
               "Get slots from types initialized through Python code");
}

TEST_F(TypeExtensionApiTest, GetNonExistentSlotFromManagedTypeAbortsPyro) {
  PyRun_SimpleString(R"(
class Foo: pass
  )");

  PyObjectPtr foo_type(testing::moduleGet("__main__", "Foo"));
  ASSERT_TRUE(PyType_CheckExact(foo_type));
  EXPECT_DEATH(
      PyType_GetSlot(reinterpret_cast<PyTypeObject*>(foo_type.get()), Py_nb_or),
      "Get slots from types initialized through Python code");
}

TEST_F(TypeExtensionApiTest, GetSlotFromNegativeSlotRaisesSystemError) {
  PyRun_SimpleString(R"(
class Foo: pass
  )");

  PyObjectPtr foo_type(testing::moduleGet("__main__", "Foo"));
  ASSERT_TRUE(PyType_CheckExact(foo_type));

  EXPECT_EQ(PyType_GetSlot(reinterpret_cast<PyTypeObject*>(foo_type.get()), -1),
            nullptr);

  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(TypeExtensionApiTest, GetSlotFromLargerThanMaxSlotReturnsNull) {
  PyRun_SimpleString(R"(
class Foo: pass
  )");

  PyObjectPtr foo_type(testing::moduleGet("__main__", "Foo"));
  ASSERT_TRUE(PyType_CheckExact(foo_type));

  EXPECT_EQ(
      PyType_GetSlot(reinterpret_cast<PyTypeObject*>(foo_type.get()), 1000),
      nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(TypeExtensionApiTest, GetSlotFromExtensionType) {
  newfunc new_func = [](PyTypeObject* type, PyObject*, PyObject*) {
    void* slot = PyType_GetSlot(type, Py_tp_alloc);
    return reinterpret_cast<allocfunc>(slot)(type, 0);
  };
  initproc init_func = [](PyObject*, PyObject*, PyObject*) { return 0; };
  binaryfunc add_func = [](PyObject*, PyObject*) { return PyLong_FromLong(7); };
  PyType_Slot slots[] = {
      {Py_tp_alloc, reinterpret_cast<void*>(PyType_GenericAlloc)},
      {Py_tp_new, reinterpret_cast<void*>(new_func)},
      {Py_tp_init, reinterpret_cast<void*>(init_func)},
      {Py_nb_add, reinterpret_cast<void*>(add_func)},
      {0, nullptr},
  };
  static PyType_Spec spec;
  spec = {
      "foo.Bar", 0, 0, Py_TPFLAGS_DEFAULT, slots,
  };
  PyObjectPtr type(PyType_FromSpec(&spec));
  ASSERT_NE(type, nullptr);
  ASSERT_TRUE(PyType_CheckExact(type));

  PyTypeObject* typeobj = reinterpret_cast<PyTypeObject*>(type.get());
  EXPECT_EQ(PyType_GetSlot(typeobj, Py_tp_alloc),
            reinterpret_cast<void*>(PyType_GenericAlloc));
  EXPECT_EQ(PyType_GetSlot(typeobj, Py_tp_new),
            reinterpret_cast<void*>(new_func));
  EXPECT_EQ(PyType_GetSlot(typeobj, Py_tp_init),
            reinterpret_cast<void*>(init_func));
  EXPECT_EQ(PyType_GetSlot(typeobj, Py_nb_add),
            reinterpret_cast<void*>(add_func));
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

// METH_NOARGS and CALL_FUNCTION

TEST_F(TypeExtensionApiTest, MethodsMethNoargsPosCall) {
  binaryfunc meth = [](PyObject*, PyObject*) { return PyLong_FromLong(1234); };
  static PyMethodDef methods[] = {{"noargs", meth, METH_NOARGS}, {nullptr}};
  PyType_Slot slots[] = {
      {Py_tp_methods, methods},
      {0, nullptr},
  };
  static PyType_Spec spec;
  spec = {
      "__main__.C", 0, 0, Py_TPFLAGS_DEFAULT, slots,
  };
  PyObjectPtr type(PyType_FromSpec(&spec));
  ASSERT_NE(type, nullptr);
  testing::moduleSet("__main__", "C", type);
  PyRun_SimpleString(R"(
result = C().noargs()
)");
  PyObjectPtr result(testing::moduleGet("__main__", "result"));
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(PyLong_AsLong(result), 1234);
}

// METH_NOARGS and CALL_FUNCTION_EX

TEST_F(TypeExtensionApiTest, MethodsMethNoargsExCall) {
  binaryfunc meth = [](PyObject*, PyObject*) { return PyLong_FromLong(1234); };
  static PyMethodDef methods[] = {{"noargs", meth, METH_NOARGS}, {nullptr}};
  PyType_Slot slots[] = {
      {Py_tp_methods, methods},
      {0, nullptr},
  };
  static PyType_Spec spec;
  spec = {
      "__main__.C", 0, 0, Py_TPFLAGS_DEFAULT, slots,
  };
  PyObjectPtr type(PyType_FromSpec(&spec));
  ASSERT_NE(type, nullptr);
  testing::moduleSet("__main__", "C", type);
  PyRun_SimpleString(R"(
result = C().noargs(*[])
)");
  PyObjectPtr result(testing::moduleGet("__main__", "result"));
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(PyLong_AsLong(result), 1234);
}

// METH_NOARGS and CALL_FUNCTION_EX with VARKEYWORDS

TEST_F(TypeExtensionApiTest, MethodsMethNoargsExNoKwargsCall) {
  binaryfunc meth = [](PyObject*, PyObject*) { return PyLong_FromLong(1234); };
  static PyMethodDef methods[] = {{"noargs", meth, METH_NOARGS}, {nullptr}};
  PyType_Slot slots[] = {
      {Py_tp_methods, methods},
      {0, nullptr},
  };
  static PyType_Spec spec;
  spec = {
      "__main__.C", 0, 0, Py_TPFLAGS_DEFAULT, slots,
  };
  PyObjectPtr type(PyType_FromSpec(&spec));
  ASSERT_NE(type, nullptr);
  testing::moduleSet("__main__", "C", type);
  PyRun_SimpleString(R"(
result = C().noargs(*[],**{})
)");
  PyObjectPtr result(testing::moduleGet("__main__", "result"));
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(PyLong_AsLong(result), 1234);
}

TEST_F(TypeExtensionApiTest, MethodsMethNoargsExHasKwargsRaisesTypeError) {
  binaryfunc meth = [](PyObject*, PyObject*) { return PyLong_FromLong(1234); };
  static PyMethodDef methods[] = {{"noargs", meth, METH_NOARGS}, {nullptr}};
  PyType_Slot slots[] = {
      {Py_tp_methods, methods},
      {0, nullptr},
  };
  static PyType_Spec spec;
  spec = {
      "__main__.C", 0, 0, Py_TPFLAGS_DEFAULT, slots,
  };
  PyObjectPtr type(PyType_FromSpec(&spec));
  ASSERT_NE(type, nullptr);
  testing::moduleSet("__main__", "C", type);
  PyRun_SimpleString(R"(
self = C()
result = False
try:
  self.noargs(*[],**{'foo': 'bar'})
except:
  result = True
)");
  PyObjectPtr result(testing::moduleGet("__main__", "result"));
  ASSERT_EQ(result, Py_True);
}

// METH_O and CALL_FUNCTION

TEST_F(TypeExtensionApiTest, MethodsMethOneArgPosCall) {
  binaryfunc meth = [](PyObject* self, PyObject* arg) {
    return PyTuple_Pack(2, self, arg);
  };
  static PyMethodDef methods[] = {{"onearg", meth, METH_O}, {nullptr}};
  PyType_Slot slots[] = {
      {Py_tp_methods, methods},
      {0, nullptr},
  };
  static PyType_Spec spec;
  spec = {
      "__main__.C", 0, 0, Py_TPFLAGS_DEFAULT, slots,
  };
  PyObjectPtr type(PyType_FromSpec(&spec));
  ASSERT_NE(type, nullptr);
  testing::moduleSet("__main__", "C", type);
  PyRun_SimpleString(R"(
self = C()
result = self.onearg(1234)
)");
  PyObjectPtr self(testing::moduleGet("__main__", "self"));
  PyObjectPtr result(testing::moduleGet("__main__", "result"));
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(PyTuple_CheckExact(result), 1);
  ASSERT_EQ(PyTuple_Size(result), 2);
  EXPECT_EQ(PyTuple_GetItem(result, 0), self);
  EXPECT_EQ(PyLong_AsLong(PyTuple_GetItem(result, 1)), 1234);
}

TEST_F(TypeExtensionApiTest, MethodsMethOneArgNoArgsRaisesTypeError) {
  binaryfunc meth = [](PyObject*, PyObject*) {
    ADD_FAILURE();  // unreachable
    return Py_None;
  };
  static PyMethodDef methods[] = {{"onearg", meth, METH_O}, {nullptr}};
  PyType_Slot slots[] = {
      {Py_tp_methods, methods},
      {0, nullptr},
  };
  static PyType_Spec spec;
  spec = {
      "__main__.C", 0, 0, Py_TPFLAGS_DEFAULT, slots,
  };
  PyObjectPtr type(PyType_FromSpec(&spec));
  ASSERT_NE(type, nullptr);
  testing::moduleSet("__main__", "C", type);
  PyRun_SimpleString(R"(
result = False
self = C()
try:
  self.onearg()
except TypeError:
  result = True
)");
  PyObjectPtr result(testing::moduleGet("__main__", "result"));
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result, Py_True);
}

// METH_O and CALL_FUNCTION_KW

TEST_F(TypeExtensionApiTest, MethodsMethOneArgKwCall) {
  binaryfunc meth = [](PyObject*, PyObject*) {
    ADD_FAILURE();  // unreachable
    return Py_None;
  };
  static PyMethodDef methods[] = {{"onearg", meth, METH_O}, {nullptr}};
  PyType_Slot slots[] = {
      {Py_tp_methods, methods},
      {0, nullptr},
  };
  static PyType_Spec spec;
  spec = {
      "__main__.C", 0, 0, Py_TPFLAGS_DEFAULT, slots,
  };
  PyObjectPtr type(PyType_FromSpec(&spec));
  ASSERT_NE(type, nullptr);
  testing::moduleSet("__main__", "C", type);
  ASSERT_NO_FATAL_FAILURE(PyRun_SimpleString(R"(
try:
  obj = C().onearg(foo=1234)
  result = False
except TypeError:
  result = True
)"));
  PyObjectPtr result(testing::moduleGet("__main__", "result"));
  EXPECT_EQ(result, Py_True);
}

// METH_O and CALL_FUNCTION_EX

TEST_F(TypeExtensionApiTest, MethodsMethOneArgExCall) {
  binaryfunc meth = [](PyObject* self, PyObject* arg) {
    return PyTuple_Pack(2, self, arg);
  };
  static PyMethodDef methods[] = {{"onearg", meth, METH_O}, {nullptr}};
  PyType_Slot slots[] = {
      {Py_tp_methods, methods},
      {0, nullptr},
  };
  static PyType_Spec spec;
  spec = {
      "__main__.C", 0, 0, Py_TPFLAGS_DEFAULT, slots,
  };
  PyObjectPtr type(PyType_FromSpec(&spec));
  ASSERT_NE(type, nullptr);
  testing::moduleSet("__main__", "C", type);
  PyRun_SimpleString(R"(
obj = C()
result = obj.onearg(*[1234])
)");
  PyObjectPtr obj(testing::moduleGet("__main__", "obj"));
  PyObjectPtr result(testing::moduleGet("__main__", "result"));
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(PyTuple_CheckExact(result), 1);
  ASSERT_EQ(PyTuple_Size(result), 2);
  EXPECT_EQ(PyTuple_GetItem(result, 0), obj);
  EXPECT_EQ(PyLong_AsLong(PyTuple_GetItem(result, 1)), 1234);
}

// METH_VARARGS and CALL_FUNCTION

TEST_F(TypeExtensionApiTest, MethodsVarargsArgPosCall) {
  binaryfunc meth = [](PyObject* self, PyObject* arg) {
    return PyTuple_Pack(2, self, arg);
  };
  static PyMethodDef methods[] = {{"varargs", meth, METH_VARARGS}, {nullptr}};
  PyType_Slot slots[] = {
      {Py_tp_methods, methods},
      {0, nullptr},
  };
  static PyType_Spec spec;
  spec = {
      "__main__.C", 0, 0, Py_TPFLAGS_DEFAULT, slots,
  };
  PyObjectPtr type(PyType_FromSpec(&spec));
  ASSERT_NE(type, nullptr);
  testing::moduleSet("__main__", "C", type);
  PyRun_SimpleString(R"(
self = C()
result = self.varargs(1234)
)");
  PyObjectPtr self(testing::moduleGet("__main__", "self"));
  PyObjectPtr result(testing::moduleGet("__main__", "result"));
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(PyTuple_CheckExact(result), 1);
  ASSERT_EQ(PyTuple_Size(result), 2);
  EXPECT_EQ(PyTuple_GetItem(result, 0), self);
  PyObject* args = PyTuple_GetItem(result, 1);
  EXPECT_TRUE(PyTuple_CheckExact(args));
  EXPECT_EQ(PyLong_AsLong(PyTuple_GetItem(args, 0)), 1234);
}

TEST_F(TypeExtensionApiTest, MethodsVarargsArgPosNoArgsCall) {
  binaryfunc meth = [](PyObject* self, PyObject* arg) {
    return PyTuple_Pack(2, self, arg);
  };
  static PyMethodDef methods[] = {{"varargs", meth, METH_VARARGS}, {nullptr}};
  PyType_Slot slots[] = {
      {Py_tp_methods, methods},
      {0, nullptr},
  };
  static PyType_Spec spec;
  spec = {
      "__main__.C", 0, 0, Py_TPFLAGS_DEFAULT, slots,
  };
  PyObjectPtr type(PyType_FromSpec(&spec));
  ASSERT_NE(type, nullptr);
  testing::moduleSet("__main__", "C", type);
  PyRun_SimpleString(R"(
self = C()
result = self.varargs()
)");
  PyObjectPtr self(testing::moduleGet("__main__", "self"));
  PyObjectPtr result(testing::moduleGet("__main__", "result"));
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(PyTuple_CheckExact(result), 1);
  ASSERT_EQ(PyTuple_Size(result), 2);
  EXPECT_EQ(PyTuple_GetItem(result, 0), self);
  PyObject* args = PyTuple_GetItem(result, 1);
  EXPECT_TRUE(PyTuple_CheckExact(args));
  EXPECT_EQ(PyTuple_Size(args), 0);
}

// METH_VARARGS and CALL_FUNCTION_KW

TEST_F(TypeExtensionApiTest, MethodsVarargsArgKwCall) {
  binaryfunc meth = [](PyObject* self, PyObject* arg) {
    return PyTuple_Pack(2, self, arg);
  };
  static PyMethodDef methods[] = {{"varargs", meth, METH_VARARGS}, {nullptr}};
  PyType_Slot slots[] = {
      {Py_tp_methods, methods},
      {0, nullptr},
  };
  static PyType_Spec spec;
  spec = {
      "__main__.C", 0, 0, Py_TPFLAGS_DEFAULT, slots,
  };
  PyObjectPtr type(PyType_FromSpec(&spec));
  ASSERT_NE(type, nullptr);
  testing::moduleSet("__main__", "C", type);
  PyRun_SimpleString(R"(
try:
  obj = C().varargs(foo=1234)
  result = False
except TypeError:
  result = True
)");
  PyObjectPtr result(testing::moduleGet("__main__", "result"));
  EXPECT_EQ(result, Py_True);
}

// METH_VARARGS and CALL_FUNCTION_EX

TEST_F(TypeExtensionApiTest, MethodsVarargsArgExCall) {
  binaryfunc meth = [](PyObject* self, PyObject* arg) {
    return PyTuple_Pack(2, self, arg);
  };
  static PyMethodDef methods[] = {{"varargs", meth, METH_VARARGS}, {nullptr}};
  PyType_Slot slots[] = {
      {Py_tp_methods, methods},
      {0, nullptr},
  };
  static PyType_Spec spec;
  spec = {
      "__main__.C", 0, 0, Py_TPFLAGS_DEFAULT, slots,
  };
  PyObjectPtr type(PyType_FromSpec(&spec));
  ASSERT_NE(type, nullptr);
  testing::moduleSet("__main__", "C", type);
  PyRun_SimpleString(R"(
self = C()
result = self.varargs(*[1234])
)");
  PyObjectPtr self(testing::moduleGet("__main__", "self"));
  PyObjectPtr result(testing::moduleGet("__main__", "result"));
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(PyTuple_CheckExact(result), 1);
  ASSERT_EQ(PyTuple_Size(result), 2);
  EXPECT_EQ(PyTuple_GetItem(result, 0), self);
  PyObject* args = PyTuple_GetItem(result, 1);
  EXPECT_TRUE(PyTuple_CheckExact(args));
  EXPECT_EQ(PyLong_AsLong(PyTuple_GetItem(args, 0)), 1234);
}

TEST_F(TypeExtensionApiTest, MethodsVarargsArgExHasEmptyKwargsCall) {
  binaryfunc meth = [](PyObject* self, PyObject* arg) {
    return PyTuple_Pack(2, self, arg);
  };
  static PyMethodDef methods[] = {{"varargs", meth, METH_VARARGS}, {nullptr}};
  PyType_Slot slots[] = {
      {Py_tp_methods, methods},
      {0, nullptr},
  };
  static PyType_Spec spec;
  spec = {
      "__main__.C", 0, 0, Py_TPFLAGS_DEFAULT, slots,
  };
  PyObjectPtr type(PyType_FromSpec(&spec));
  ASSERT_NE(type, nullptr);
  testing::moduleSet("__main__", "C", type);
  PyRun_SimpleString(R"(
self = C()
result = self.varargs(*[1234], **{})
)");
  PyObjectPtr self(testing::moduleGet("__main__", "self"));
  PyObjectPtr result(testing::moduleGet("__main__", "result"));
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(PyTuple_CheckExact(result), 1);
  ASSERT_EQ(PyTuple_Size(result), 2);
  EXPECT_EQ(PyTuple_GetItem(result, 0), self);
  PyObject* args = PyTuple_GetItem(result, 1);
  EXPECT_TRUE(PyTuple_CheckExact(args));
  EXPECT_EQ(PyLong_AsLong(PyTuple_GetItem(args, 0)), 1234);
}

// METH_KEYWORDS and CALL_FUNCTION

TEST_F(TypeExtensionApiTest, MethodsMethKeywordsPosCall) {
  ternaryfunc meth = [](PyObject* self, PyObject* args, PyObject* kwargs) {
    if (kwargs == nullptr) return PyTuple_Pack(2, self, args);
    return PyTuple_Pack(3, self, args, kwargs);
  };
  static PyMethodDef methods[] = {
      {"keywords", reinterpret_cast<PyCFunction>(reinterpret_cast<void*>(meth)),
       METH_VARARGS | METH_KEYWORDS},
      {nullptr}};
  PyType_Slot slots[] = {
      {Py_tp_methods, methods},
      {0, nullptr},
  };

  static PyType_Spec spec;
  spec = {
      "__main__.C", 0, 0, Py_TPFLAGS_DEFAULT, slots,
  };

  PyObjectPtr type(PyType_FromSpec(&spec));
  ASSERT_NE(type, nullptr);
  testing::moduleSet("__main__", "C", type);
  PyRun_SimpleString(R"(
self = C()
result = self.keywords(1234)
)");

  PyObjectPtr self(testing::moduleGet("__main__", "self"));
  PyObjectPtr result(testing::moduleGet("__main__", "result"));
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(PyTuple_CheckExact(result), 1);
  ASSERT_EQ(PyTuple_Size(result), 2);
  EXPECT_EQ(PyTuple_GetItem(result, 0), self);

  PyObject* args = PyTuple_GetItem(result, 1);
  EXPECT_TRUE(PyTuple_CheckExact(args));
  EXPECT_EQ(PyTuple_Size(args), 1);
  EXPECT_EQ(PyLong_AsLong(PyTuple_GetItem(args, 0)), 1234);
}

// METH_KEYWORDS and CALL_FUNCTION_KW

TEST_F(TypeExtensionApiTest, MethodsMethKeywordsKwCall) {
  ternaryfunc meth = [](PyObject* self, PyObject* args, PyObject* kwargs) {
    if (kwargs == nullptr) return PyTuple_Pack(2, self, args);
    return PyTuple_Pack(3, self, args, kwargs);
  };
  static PyMethodDef methods[] = {
      {"keywords", reinterpret_cast<PyCFunction>(reinterpret_cast<void*>(meth)),
       METH_VARARGS | METH_KEYWORDS},
      {nullptr}};
  PyType_Slot slots[] = {
      {Py_tp_methods, methods},
      {0, nullptr},
  };

  static PyType_Spec spec;
  spec = {
      "__main__.C", 0, 0, Py_TPFLAGS_DEFAULT, slots,
  };

  PyObjectPtr type(PyType_FromSpec(&spec));
  ASSERT_NE(type, nullptr);
  testing::moduleSet("__main__", "C", type);
  PyRun_SimpleString(R"(
self = C()
result = self.keywords(1234, kwarg=5678)
)");

  PyObjectPtr self(testing::moduleGet("__main__", "self"));
  PyObjectPtr result(testing::moduleGet("__main__", "result"));
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(PyTuple_CheckExact(result), 1);
  ASSERT_EQ(PyTuple_Size(result), 3);
  EXPECT_EQ(PyTuple_GetItem(result, 0), self);

  PyObject* args = PyTuple_GetItem(result, 1);
  EXPECT_TRUE(PyTuple_CheckExact(args));
  EXPECT_EQ(PyTuple_Size(args), 1);
  EXPECT_EQ(PyLong_AsLong(PyTuple_GetItem(args, 0)), 1234);

  PyObject* kwargs = PyTuple_GetItem(result, 2);
  EXPECT_TRUE(PyDict_CheckExact(kwargs));
  ASSERT_EQ(PyDict_Size(kwargs), 1);
  PyObject* item = PyDict_GetItemString(kwargs, "kwarg");
  EXPECT_TRUE(isLongEqualsLong(item, 5678));
}

// METH_KEYWORDS and CALL_FUNCTION_EX

TEST_F(TypeExtensionApiTest, MethodsMethKeywordsExCall) {
  ternaryfunc meth = [](PyObject* self, PyObject* args, PyObject* kwargs) {
    return PyTuple_Pack(3, self, args, kwargs);
  };
  static PyMethodDef methods[] = {
      {"keywords", reinterpret_cast<PyCFunction>(reinterpret_cast<void*>(meth)),
       METH_VARARGS | METH_KEYWORDS},
      {nullptr}};
  PyType_Slot slots[] = {
      {Py_tp_methods, methods},
      {0, nullptr},
  };

  static PyType_Spec spec;
  spec = {
      "__main__.C", 0, 0, Py_TPFLAGS_DEFAULT, slots,
  };

  PyObjectPtr type(PyType_FromSpec(&spec));
  ASSERT_NE(type, nullptr);
  testing::moduleSet("__main__", "C", type);
  PyRun_SimpleString(R"(
self = C()
result = self.keywords(*[1234], kwarg=5678)
)");

  PyObjectPtr self(testing::moduleGet("__main__", "self"));
  PyObjectPtr result(testing::moduleGet("__main__", "result"));
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(PyTuple_CheckExact(result), 1);
  ASSERT_EQ(PyTuple_Size(result), 3);
  EXPECT_EQ(PyTuple_GetItem(result, 0), self);

  PyObject* args = PyTuple_GetItem(result, 1);
  EXPECT_TRUE(PyTuple_CheckExact(args));
  EXPECT_EQ(PyTuple_Size(args), 1);
  EXPECT_EQ(PyLong_AsLong(PyTuple_GetItem(args, 0)), 1234);

  PyObject* kwargs = PyTuple_GetItem(result, 2);
  EXPECT_TRUE(PyDict_CheckExact(kwargs));
  ASSERT_EQ(PyDict_Size(kwargs), 1);
  PyObject* item = PyDict_GetItemString(kwargs, "kwarg");
  EXPECT_TRUE(isLongEqualsLong(item, 5678));
}

TEST_F(TypeExtensionApiTest, MethodsMethKeywordsExEmptyKwargsCall) {
  ternaryfunc meth = [](PyObject* self, PyObject* args, PyObject* kwargs) {
    if (kwargs == nullptr) return PyTuple_Pack(2, self, args);
    return PyTuple_Pack(3, self, args, kwargs);
  };
  static PyMethodDef methods[] = {
      {"keywords", reinterpret_cast<PyCFunction>(reinterpret_cast<void*>(meth)),
       METH_VARARGS | METH_KEYWORDS},
      {nullptr}};
  PyType_Slot slots[] = {
      {Py_tp_methods, methods},
      {0, nullptr},
  };

  static PyType_Spec spec;
  spec = {
      "__main__.C", 0, 0, Py_TPFLAGS_DEFAULT, slots,
  };

  PyObjectPtr type(PyType_FromSpec(&spec));
  ASSERT_NE(type, nullptr);
  testing::moduleSet("__main__", "C", type);
  PyRun_SimpleString(R"(
self = C()
result = self.keywords(*[1234], *{})
)");

  PyObjectPtr self(testing::moduleGet("__main__", "self"));
  PyObjectPtr result(testing::moduleGet("__main__", "result"));
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(PyTuple_CheckExact(result), 1);
  ASSERT_EQ(PyTuple_Size(result), 2);
  EXPECT_EQ(PyTuple_GetItem(result, 0), self);

  PyObject* args = PyTuple_GetItem(result, 1);
  EXPECT_TRUE(PyTuple_CheckExact(args));
  EXPECT_EQ(PyTuple_Size(args), 1);
  EXPECT_EQ(PyLong_AsLong(PyTuple_GetItem(args, 0)), 1234);
}

TEST_F(TypeExtensionApiTest, GetObjectCreatedInManagedCode) {
  static PyType_Slot slots[1];
  slots[0] = {0, nullptr};
  static PyType_Spec spec;
  spec = {
      "__main__.Foo", 0, 0, Py_TPFLAGS_DEFAULT, slots,
  };
  PyObjectPtr type(PyType_FromSpec(&spec));
  ASSERT_NE(type, nullptr);
  ASSERT_EQ(PyType_CheckExact(type), 1);
  ASSERT_EQ(moduleSet("__main__", "Foo", type), 0);

  // This is similar to CallExtensionTypeReturnsExtensionInstancePyro, but it
  // tests the RawObject -> PyObject* path for objects that were created on the
  // managed heap and had no corresponding PyObject* before the call to
  // moduleGet().
  ASSERT_EQ(PyRun_SimpleString("f = Foo()"), 0);
  PyObjectPtr foo(moduleGet("__main__", "f"));
  EXPECT_NE(foo, nullptr);
}

TEST_F(TypeExtensionApiTest, GenericNewReturnsExtensionInstance) {
  // clang-format off
  struct BarObject {
    PyObject_HEAD
  };
  // clang-format on
  destructor dealloc_func = [](PyObject* self) {
    PyObjectPtr type(PyObject_Type(self));
    auto free_func = reinterpret_cast<freefunc>(PyType_GetSlot(
        reinterpret_cast<PyTypeObject*>(type.get()), Py_tp_free));
    return free_func(self);
  };
  PyType_Slot slots[] = {
      {Py_tp_alloc, reinterpret_cast<void*>(PyType_GenericAlloc)},
      {Py_tp_new, reinterpret_cast<void*>(PyType_GenericNew)},
      {Py_tp_dealloc, reinterpret_cast<void*>(dealloc_func)},
      {Py_tp_free, reinterpret_cast<void*>(PyObject_Del)},
      {0, nullptr},
  };
  static PyType_Spec spec;
  spec = {
      "foo.Bar", sizeof(BarObject), 0, Py_TPFLAGS_DEFAULT, slots,
  };
  PyObjectPtr type(PyType_FromSpec(&spec));
  ASSERT_NE(type, nullptr);
  ASSERT_TRUE(PyType_CheckExact(type));

  auto new_func = reinterpret_cast<newfunc>(
      PyType_GetSlot(reinterpret_cast<PyTypeObject*>(type.get()), Py_tp_new));
  PyObjectPtr bar(
      new_func(reinterpret_cast<PyTypeObject*>(type.get()), nullptr, nullptr));
  EXPECT_NE(bar, nullptr);
}

// Given one slot id and a function pointer to go with it, create a Bar type
// containing that slot.
template <typename T>
static void createBarTypeWithSlot(int slot, T pfunc) {
  static PyType_Slot slots[2];
  slots[0] = {slot, reinterpret_cast<void*>(pfunc)};
  slots[1] = {0, nullptr};
  static PyType_Spec spec;
  spec = {
      "__main__.Bar", 0, 0, Py_TPFLAGS_DEFAULT, slots,
  };
  PyObjectPtr type(PyType_FromSpec(&spec));
  ASSERT_NE(type, nullptr);
  ASSERT_EQ(PyType_CheckExact(type), 1);
  ASSERT_EQ(moduleSet("__main__", "Bar", type), 0);
}

TEST_F(TypeExtensionApiTest, CallBinarySlotFromManagedCode) {
  binaryfunc add_func = [](PyObject* a, PyObject* b) {
    PyObjectPtr num(PyLong_FromLong(24));
    return PyLong_Check(a) ? PyNumber_Add(a, num) : PyNumber_Add(num, b);
  };
  ASSERT_NO_FATAL_FAILURE(createBarTypeWithSlot(Py_nb_add, add_func));

  ASSERT_EQ(PyRun_SimpleString(R"(
b = Bar()
r1 = b.__add__(12)
r2 = Bar.__add__(b, 24)
r3 = 1000 + b
args = (b, 42)
r4 = Bar.__add__(*args)
kwargs = {}
r5 = b.__add__(100, **kwargs)
b += -12
)"),
            0);

  PyObjectPtr r1(moduleGet("__main__", "r1"));
  EXPECT_TRUE(isLongEqualsLong(r1, 36));

  PyObjectPtr r2(moduleGet("__main__", "r2"));
  EXPECT_TRUE(isLongEqualsLong(r2, 48));

  PyObjectPtr r3(moduleGet("__main__", "r3"));
  EXPECT_TRUE(isLongEqualsLong(r3, 1024));

  PyObjectPtr r4(moduleGet("__main__", "r4"));
  EXPECT_TRUE(isLongEqualsLong(r4, 66));

  PyObjectPtr r5(moduleGet("__main__", "r5"));
  EXPECT_TRUE(isLongEqualsLong(r5, 124));

  PyObjectPtr b(moduleGet("__main__", "b"));
  EXPECT_TRUE(isLongEqualsLong(b, 12));
}

TEST_F(TypeExtensionApiTest, CallBinarySlotWithKwargsRaisesTypeError) {
  binaryfunc dummy_add = [](PyObject*, PyObject*) -> PyObject* {
    EXPECT_TRUE(false) << "Shouldn't be called";
    Py_RETURN_NONE;
  };
  ASSERT_NO_FATAL_FAILURE(createBarTypeWithSlot(Py_nb_add, dummy_add));

  // TODO(T40700664): Use PyRun_String() so we can directly inspect the thrown
  // exception(s).
  ASSERT_EQ(PyRun_SimpleString(R"(
b = Bar()
try:
  b.__add__(a=2)
  raise RuntimeError("call didn't throw")
except TypeError:
  pass

try:
  kwargs = {'a': 2}
  b.__add__(**kwargs)
  raise RuntimeError("call didn't throw")
except TypeError:
  pass
)"),
            0);
}

TEST_F(TypeExtensionApiTest, CallHashSlotFromManagedCode) {
  hashfunc hash_func = [](PyObject*) -> Py_hash_t { return 0xba5eba11; };
  ASSERT_NO_FATAL_FAILURE(createBarTypeWithSlot(Py_tp_hash, hash_func));

  ASSERT_EQ(PyRun_SimpleString(R"(
b = Bar()
h1 = b.__hash__()
h2 = Bar.__hash__(b)
)"),
            0);

  PyObjectPtr h1(moduleGet("__main__", "h1"));
  EXPECT_TRUE(isLongEqualsLong(h1, 0xba5eba11));

  PyObjectPtr h2(moduleGet("__main__", "h2"));
  EXPECT_TRUE(isLongEqualsLong(h2, 0xba5eba11));
}

TEST_F(TypeExtensionApiTest, CallCallSlotFromManagedCode) {
  ternaryfunc call_func = [](PyObject* self, PyObject* args, PyObject* kwargs) {
    return PyTuple_Pack(3, self, args, kwargs ? kwargs : Py_None);
  };
  ASSERT_NO_FATAL_FAILURE(createBarTypeWithSlot(Py_tp_call, call_func));

  ASSERT_EQ(PyRun_SimpleString(R"(
b = Bar()
r1 = b.__call__()
r2 = b.__call__('a', 'b', c='see')
r3 = b('hello!')
args=(b,"an argument")
r4 = Bar.__call__(*args)
)"),
            0);

  PyObjectPtr b(moduleGet("__main__", "b"));
  PyObjectPtr r1(moduleGet("__main__", "r1"));
  ASSERT_EQ(PyTuple_Check(r1), 1);
  ASSERT_EQ(PyTuple_Size(r1), 3);
  EXPECT_EQ(PyTuple_GetItem(r1, 0), b);
  PyObject* tmp = PyTuple_GetItem(r1, 1);
  ASSERT_EQ(PyTuple_Check(tmp), 1);
  EXPECT_EQ(PyTuple_Size(tmp), 0);
  EXPECT_EQ(PyTuple_GetItem(r1, 2), Py_None);

  PyObjectPtr r2(moduleGet("__main__", "r2"));
  ASSERT_EQ(PyTuple_Check(r2), 1);
  ASSERT_EQ(PyTuple_Size(r2), 3);
  EXPECT_EQ(PyTuple_GetItem(r2, 0), b);
  tmp = PyTuple_GetItem(r2, 1);
  ASSERT_EQ(PyTuple_Check(tmp), 1);
  ASSERT_EQ(PyTuple_Size(tmp), 2);
  EXPECT_TRUE(isUnicodeEqualsCStr(PyTuple_GetItem(tmp, 0), "a"));
  EXPECT_TRUE(isUnicodeEqualsCStr(PyTuple_GetItem(tmp, 1), "b"));
  tmp = PyTuple_GetItem(r2, 2);
  ASSERT_EQ(PyDict_Check(tmp), 1);
  PyObjectPtr key(PyUnicode_FromString("c"));
  EXPECT_TRUE(isUnicodeEqualsCStr(PyDict_GetItem(tmp, key), "see"));

  PyObjectPtr r3(moduleGet("__main__", "r3"));
  ASSERT_EQ(PyTuple_Check(r3), 1);
  ASSERT_EQ(PyTuple_Size(r3), 3);
  EXPECT_EQ(PyTuple_GetItem(r3, 0), b);
  tmp = PyTuple_GetItem(r3, 1);
  ASSERT_EQ(PyTuple_Check(tmp), 1);
  ASSERT_EQ(PyTuple_Size(tmp), 1);
  EXPECT_TRUE(isUnicodeEqualsCStr(PyTuple_GetItem(tmp, 0), "hello!"));
  EXPECT_EQ(PyTuple_GetItem(r3, 2), Py_None);

  PyObjectPtr r4(moduleGet("__main__", "r4"));
  ASSERT_EQ(PyTuple_Check(r4), 1);
  ASSERT_EQ(PyTuple_Size(r4), 3);
  EXPECT_EQ(PyTuple_GetItem(r4, 0), b);
  tmp = PyTuple_GetItem(r4, 1);
  ASSERT_EQ(PyTuple_Check(tmp), 1);
  ASSERT_EQ(PyTuple_Size(tmp), 1);
  EXPECT_TRUE(isUnicodeEqualsCStr(PyTuple_GetItem(tmp, 0), "an argument"));
  EXPECT_EQ(PyTuple_GetItem(r4, 2), Py_None);
}

TEST_F(TypeExtensionApiTest, CallGetattroSlotFromManagedCode) {
  getattrofunc getattr_func = [](PyObject* self, PyObject* name) {
    return PyTuple_Pack(2, name, self);
  };
  ASSERT_NO_FATAL_FAILURE(createBarTypeWithSlot(Py_tp_getattro, getattr_func));

  ASSERT_EQ(PyRun_SimpleString(R"(
b = Bar()
r = b.foo_bar
)"),
            0);

  PyObjectPtr b(moduleGet("__main__", "b"));
  ASSERT_NE(b, nullptr);
  PyObjectPtr r(moduleGet("__main__", "r"));
  ASSERT_NE(r, nullptr);
  ASSERT_EQ(PyTuple_Check(r), 1);
  EXPECT_TRUE(isUnicodeEqualsCStr(PyTuple_GetItem(r, 0), "foo_bar"));
  EXPECT_EQ(PyTuple_GetItem(r, 1), b);
}

// Pyro-only due to
// https://github.com/python/cpython/commit/4dcdb78c6ffd203c9d72ef41638cc4a0e3857adf
TEST_F(TypeExtensionApiTest, CallSetattroSlotFromManagedCodePyro) {
  setattrofunc setattr_func = [](PyObject* self, PyObject* name,
                                 PyObject* value) {
    PyObjectPtr tuple(value ? PyTuple_Pack(3, self, name, value)
                            : PyTuple_Pack(2, self, name));
    const char* var = value ? "set_attr" : "del_attr";
    moduleSet("__main__", var, tuple);
    return 0;
  };
  ASSERT_NO_FATAL_FAILURE(createBarTypeWithSlot(Py_tp_setattro, setattr_func));

  ASSERT_EQ(PyRun_SimpleString(R"(
b = Bar()
r1 = b.__setattr__("attr", 1234)
)"),
            0);

  PyObjectPtr b(moduleGet("__main__", "b"));
  PyObjectPtr r1(moduleGet("__main__", "r1"));
  EXPECT_EQ(r1, Py_None);
  PyObjectPtr set_attr(moduleGet("__main__", "set_attr"));
  ASSERT_EQ(PyTuple_Check(set_attr), 1);
  ASSERT_EQ(PyTuple_Size(set_attr), 3);
  EXPECT_EQ(PyTuple_GetItem(set_attr, 0), b);
  EXPECT_TRUE(isUnicodeEqualsCStr(PyTuple_GetItem(set_attr, 1), "attr"));
  EXPECT_TRUE(isLongEqualsLong(PyTuple_GetItem(set_attr, 2), 1234));

  ASSERT_EQ(PyRun_SimpleString(R"(r2 = b.__delattr__("other attr"))"), 0);
  PyObjectPtr r2(moduleGet("__main__", "r2"));
  EXPECT_EQ(r2, Py_None);
  PyObjectPtr del_attr(moduleGet("__main__", "del_attr"));
  ASSERT_EQ(PyTuple_Check(del_attr), 1);
  ASSERT_EQ(PyTuple_Size(del_attr), 2);
  EXPECT_EQ(PyTuple_GetItem(del_attr, 0), b);
  EXPECT_TRUE(isUnicodeEqualsCStr(PyTuple_GetItem(del_attr, 1), "other attr"));
}

TEST_F(TypeExtensionApiTest, SetattrSlotIsIgnored) {
  setattrfunc func = [](PyObject*, char*, PyObject*) {
    EXPECT_TRUE(false) << "Shouldn't be called";
    return 0;
  };
  ASSERT_NO_FATAL_FAILURE(createBarTypeWithSlot(Py_tp_setattr, func));

  // TODO(T40700664): Use PyRun_String() to inspect the exception more directly.
  ASSERT_EQ(PyRun_SimpleString(R"(
b = Bar()
try:
  # This should complain that there's no such attribute instead of calling
  # our slot.
  b.__setattr__("attr", 123)
except AttributeError:
  pass
  )"),
            0);
}

TEST_F(TypeExtensionApiTest, CallRichcompareSlotFromManagedCode) {
  richcmpfunc cmp_func = [](PyObject* self, PyObject* other, int op) {
    PyObjectPtr op_obj(PyLong_FromLong(op));
    return PyTuple_Pack(3, self, other, op_obj.get());
  };
  ASSERT_NO_FATAL_FAILURE(createBarTypeWithSlot(Py_tp_richcompare, cmp_func));

  ASSERT_EQ(PyRun_SimpleString(R"(
b = Bar()
r1 = b.__eq__("equal")
r2 = b.__gt__(0xcafe)
)"),
            0);

  PyObjectPtr b(moduleGet("__main__", "b"));
  PyObjectPtr r1(moduleGet("__main__", "r1"));
  ASSERT_EQ(PyTuple_Check(r1), 1);
  ASSERT_EQ(PyTuple_Size(r1), 3);
  EXPECT_EQ(PyTuple_GetItem(r1, 0), b);
  EXPECT_TRUE(isUnicodeEqualsCStr(PyTuple_GetItem(r1, 1), "equal"));
  EXPECT_TRUE(isLongEqualsLong(PyTuple_GetItem(r1, 2), Py_EQ));

  PyObjectPtr r2(moduleGet("__main__", "r2"));
  ASSERT_EQ(PyTuple_Check(r2), 1);
  ASSERT_EQ(PyTuple_Size(r2), 3);
  EXPECT_EQ(PyTuple_GetItem(r2, 0), b);
  EXPECT_TRUE(isLongEqualsLong(PyTuple_GetItem(r2, 1), 0xcafe));
  EXPECT_TRUE(isLongEqualsLong(PyTuple_GetItem(r2, 2), Py_GT));
}

TEST_F(TypeExtensionApiTest, CallNextSlotFromManagedCode) {
  unaryfunc next_func = [](PyObject* self) {
    Py_INCREF(self);
    return self;
  };
  ASSERT_NO_FATAL_FAILURE(createBarTypeWithSlot(Py_tp_iternext, next_func));

  ASSERT_EQ(PyRun_SimpleString(R"(
b = Bar()
r = b.__next__()
)"),
            0);

  PyObjectPtr b(moduleGet("__main__", "b"));
  PyObjectPtr r(moduleGet("__main__", "r"));
  EXPECT_EQ(r, b);
}

TEST_F(TypeExtensionApiTest, NextSlotReturningNullRaisesStopIteration) {
  unaryfunc next_func = [](PyObject*) -> PyObject* { return nullptr; };
  ASSERT_NO_FATAL_FAILURE(createBarTypeWithSlot(Py_tp_iternext, next_func));

  ASSERT_EQ(PyRun_SimpleString(R"(
caught = False
try:
  Bar().__next__()
except StopIteration:
  caught = True
)"),
            0);

  PyObjectPtr caught(moduleGet("__main__", "caught"));
  EXPECT_EQ(caught, Py_True);
}

TEST_F(TypeExtensionApiTest, CallDescrGetSlotFromManagedCode) {
  descrgetfunc get_func = [](PyObject* self, PyObject* instance,
                             PyObject* owner) {
    return PyTuple_Pack(3, self, instance, owner);
  };
  ASSERT_NO_FATAL_FAILURE(createBarTypeWithSlot(Py_tp_descr_get, get_func));

  ASSERT_EQ(PyRun_SimpleString(R"(
b = Bar()
b2 = Bar()
r = b.__get__(b2, Bar)
)"),
            0);

  PyObjectPtr bar(moduleGet("__main__", "Bar"));
  PyObjectPtr b(moduleGet("__main__", "b"));
  PyObjectPtr b2(moduleGet("__main__", "b2"));
  PyObjectPtr r(moduleGet("__main__", "r"));
  ASSERT_EQ(PyTuple_Check(r), 1);
  ASSERT_EQ(PyTuple_Size(r), 3);
  EXPECT_EQ(PyTuple_GetItem(r, 0), b);
  EXPECT_EQ(PyTuple_GetItem(r, 1), b2);
  EXPECT_EQ(PyTuple_GetItem(r, 2), bar);
}

TEST_F(TypeExtensionApiTest, DescrGetSlotWithNonesRaisesTypeError) {
  descrgetfunc get_func = [](PyObject*, PyObject*, PyObject*) -> PyObject* {
    EXPECT_TRUE(false) << "Shouldn't be called";
    return nullptr;
  };
  ASSERT_NO_FATAL_FAILURE(createBarTypeWithSlot(Py_tp_descr_get, get_func));

  // TODO(T40700664): Use PyRun_String() so we can inspect the exception more
  // directly.
  ASSERT_EQ(PyRun_SimpleString(R"(
b = Bar()
exc = None
try:
  b.__get__(None, None)
except TypeError as e:
  exc = e
)"),
            0);
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  EXPECT_EQ(PyErr_GivenExceptionMatches(exc, PyExc_TypeError), 1);
}

TEST_F(TypeExtensionApiTest, CallDescrSetSlotFromManagedCode) {
  descrsetfunc set_func = [](PyObject* /* self */, PyObject* obj,
                             PyObject* value) -> int {
    EXPECT_TRUE(isLongEqualsLong(obj, 123));
    EXPECT_TRUE(isLongEqualsLong(value, 456));
    return 0;
  };
  ASSERT_NO_FATAL_FAILURE(createBarTypeWithSlot(Py_tp_descr_set, set_func));

  ASSERT_EQ(PyRun_SimpleString(R"(
b = Bar()
b.__set__(123, 456)
)"),
            0);
}

TEST_F(TypeExtensionApiTest, CallDescrDeleteSlotFromManagedCode) {
  descrsetfunc set_func = [](PyObject* /* self */, PyObject* obj,
                             PyObject* value) -> int {
    EXPECT_TRUE(isLongEqualsLong(obj, 24));
    EXPECT_EQ(value, nullptr);
    return 0;
  };
  ASSERT_NO_FATAL_FAILURE(createBarTypeWithSlot(Py_tp_descr_set, set_func));

  ASSERT_EQ(PyRun_SimpleString(R"(
b = Bar()
b.__delete__(24)
)"),
            0);
}

TEST_F(TypeExtensionApiTest, CallInitSlotFromManagedCode) {
  initproc init_func = [](PyObject* /* self */, PyObject* args,
                          PyObject* kwargs) -> int {
    moduleSet("__main__", "args", args);
    moduleSet("__main__", "kwargs", kwargs);
    return 0;
  };
  ASSERT_NO_FATAL_FAILURE(createBarTypeWithSlot(Py_tp_init, init_func));

  ASSERT_EQ(PyRun_SimpleString(R"(
b = Bar.__new__(Bar)
b.__init__(123, four=4)
)"),
            0);

  PyObjectPtr args(moduleGet("__main__", "args"));
  ASSERT_NE(args, nullptr);
  ASSERT_EQ(PyTuple_Check(args), 1);
  ASSERT_EQ(PyTuple_Size(args), 1);
  EXPECT_TRUE(isLongEqualsLong(PyTuple_GetItem(args, 0), 123));

  PyObjectPtr kwargs(moduleGet("__main__", "kwargs"));
  ASSERT_NE(kwargs, nullptr);
  ASSERT_EQ(PyDict_Check(kwargs), 1);
  ASSERT_EQ(PyDict_Size(kwargs), 1);
  EXPECT_TRUE(isLongEqualsLong(PyDict_GetItemString(kwargs, "four"), 4));
}

TEST_F(TypeExtensionApiTest, CallDelSlotFromManagedCode) {
  destructor del_func = [](PyObject* /* self */) {
    moduleSet("__main__", "called", Py_True);
  };
  ASSERT_NO_FATAL_FAILURE(createBarTypeWithSlot(Py_tp_finalize, del_func));

  ASSERT_EQ(PyRun_SimpleString(R"(
b = Bar()
b.__del__()
)"),
            0);
  PyObjectPtr called(moduleGet("__main__", "called"));
  EXPECT_EQ(called, Py_True);
}

TEST_F(TypeExtensionApiTest, CallTernarySlotFromManagedCode) {
  ternaryfunc pow_func = [](PyObject* self, PyObject* value, PyObject* mod) {
    return PyTuple_Pack(3, self, value, mod);
  };
  ASSERT_NO_FATAL_FAILURE(createBarTypeWithSlot(Py_nb_power, pow_func));

  ASSERT_EQ(PyRun_SimpleString(R"(
b = Bar()
r1 = b.__pow__(123, 456)
r2 = b.__pow__(789)
)"),
            0);
  PyObjectPtr b(moduleGet("__main__", "b"));
  PyObjectPtr r1(moduleGet("__main__", "r1"));
  ASSERT_EQ(PyTuple_Check(r1), 1);
  ASSERT_EQ(PyTuple_Size(r1), 3);
  EXPECT_EQ(PyTuple_GetItem(r1, 0), b);
  EXPECT_TRUE(isLongEqualsLong(PyTuple_GetItem(r1, 1), 123));
  EXPECT_TRUE(isLongEqualsLong(PyTuple_GetItem(r1, 2), 456));

  PyObjectPtr r2(moduleGet("__main__", "r2"));
  ASSERT_EQ(PyTuple_Check(r2), 1);
  ASSERT_EQ(PyTuple_Size(r1), 3);
  EXPECT_EQ(PyTuple_GetItem(r2, 0), b);
  EXPECT_TRUE(isLongEqualsLong(PyTuple_GetItem(r2, 1), 789));
  EXPECT_EQ(PyTuple_GetItem(r2, 2), Py_None);
}

TEST_F(TypeExtensionApiTest, CallInquirySlotFromManagedCode) {
  inquiry bool_func = [](PyObject* self) {
    PyObjectPtr b(moduleGet("__main__", "b"));
    EXPECT_EQ(self, b);
    return 1;
  };
  ASSERT_NO_FATAL_FAILURE(createBarTypeWithSlot(Py_nb_bool, bool_func));

  ASSERT_EQ(PyRun_SimpleString(R"(
b = Bar()
r = b.__bool__()
  )"),
            0);
  PyObjectPtr r(moduleGet("__main__", "r"));
  EXPECT_EQ(r, Py_True);
}

TEST_F(TypeExtensionApiTest, CallObjobjargSlotFromManagedCode) {
  objobjargproc set_func = [](PyObject* self, PyObject* key, PyObject* value) {
    PyObjectPtr b(moduleGet("__main__", "b"));
    EXPECT_EQ(self, b);
    moduleSet("__main__", "key", key);
    moduleSet("__main__", "value", value);
    return 0;
  };
  ASSERT_NO_FATAL_FAILURE(createBarTypeWithSlot(Py_mp_ass_subscript, set_func));

  ASSERT_EQ(PyRun_SimpleString(R"(
b = Bar()
r = b.__setitem__("some key", "a value")
)"),
            0);
  PyObjectPtr r(moduleGet("__main__", "r"));
  EXPECT_EQ(r, Py_None);

  PyObjectPtr key(moduleGet("__main__", "key"));
  EXPECT_TRUE(isUnicodeEqualsCStr(key, "some key"));

  PyObjectPtr value(moduleGet("__main__", "value"));
  EXPECT_TRUE(isUnicodeEqualsCStr(value, "a value"));
}

TEST_F(TypeExtensionApiTest, CallObjobjSlotFromManagedCode) {
  objobjproc contains_func = [](PyObject* self, PyObject* value) {
    PyObjectPtr b(moduleGet("__main__", "b"));
    EXPECT_EQ(self, b);
    moduleSet("__main__", "value", value);
    return 123456;
  };
  ASSERT_NO_FATAL_FAILURE(createBarTypeWithSlot(Py_sq_contains, contains_func));

  ASSERT_EQ(PyRun_SimpleString(R"(
b = Bar()
r = b.__contains__("a key")
)"),
            0);
  PyObjectPtr r(moduleGet("__main__", "r"));
  EXPECT_EQ(r, Py_True);

  PyObjectPtr value(moduleGet("__main__", "value"));
  EXPECT_TRUE(isUnicodeEqualsCStr(value, "a key"));
}

TEST_F(TypeExtensionApiTest, CallDelitemSlotFromManagedCode) {
  objobjargproc del_func = [](PyObject* self, PyObject* key, PyObject* value) {
    PyObjectPtr b(moduleGet("__main__", "b"));
    EXPECT_EQ(self, b);
    EXPECT_EQ(value, nullptr);
    moduleSet("__main__", "key", key);
    return 0;
  };
  ASSERT_NO_FATAL_FAILURE(createBarTypeWithSlot(Py_mp_ass_subscript, del_func));

  ASSERT_EQ(PyRun_SimpleString(R"(
b = Bar()
r = b.__delitem__("another key")
)"),
            0);
  PyObjectPtr r(moduleGet("__main__", "r"));
  EXPECT_EQ(r, Py_None);

  PyObjectPtr key(moduleGet("__main__", "key"));
  EXPECT_TRUE(isUnicodeEqualsCStr(key, "another key"));
}

TEST_F(TypeExtensionApiTest, CallLenSlotFromManagedCode) {
  lenfunc len_func = [](PyObject* self) -> Py_ssize_t {
    PyObjectPtr b(moduleGet("__main__", "b"));
    EXPECT_EQ(self, b);
    return 0xdeadbeef;
  };
  ASSERT_NO_FATAL_FAILURE(createBarTypeWithSlot(Py_sq_length, len_func));

  ASSERT_EQ(PyRun_SimpleString(R"(
b = Bar()
r = b.__len__()
)"),
            0);
  PyObjectPtr r(moduleGet("__main__", "r"));
  EXPECT_TRUE(isLongEqualsLong(r, 0xdeadbeef));
}

TEST_F(TypeExtensionApiTest, CallIndexargSlotFromManagedCode) {
  ssizeargfunc mul_func = [](PyObject* self, Py_ssize_t i) {
    PyObjectPtr b(moduleGet("__main__", "b"));
    EXPECT_EQ(self, b);
    return PyLong_FromLong(i * 456);
  };
  ASSERT_NO_FATAL_FAILURE(createBarTypeWithSlot(Py_sq_repeat, mul_func));

  ASSERT_EQ(PyRun_SimpleString(R"(
b = Bar()
r = b.__mul__(123)
)"),
            0);
  PyObjectPtr r(moduleGet("__main__", "r"));
  EXPECT_TRUE(isLongEqualsLong(r, 123 * 456));
}

TEST_F(TypeExtensionApiTest, CallSqItemSlotFromManagedCode) {
  ssizeargfunc item_func = [](PyObject* self, Py_ssize_t i) {
    PyObjectPtr b(moduleGet("__main__", "b"));
    EXPECT_EQ(self, b);
    return PyLong_FromLong(i + 100);
  };
  ASSERT_NO_FATAL_FAILURE(createBarTypeWithSlot(Py_sq_item, item_func));

  ASSERT_EQ(PyRun_SimpleString(R"(
b = Bar()
r = b.__getitem__(1337)
)"),
            0);
  PyObjectPtr r(moduleGet("__main__", "r"));
  EXPECT_TRUE(isLongEqualsLong(r, 1337 + 100));
}

TEST_F(TypeExtensionApiTest, CallSqSetitemSlotFromManagedCode) {
  ssizeobjargproc set_func = [](PyObject* self, Py_ssize_t i, PyObject* value) {
    PyObjectPtr b(moduleGet("__main__", "b"));
    EXPECT_EQ(self, b);
    PyObjectPtr key(PyLong_FromLong(i));
    moduleSet("__main__", "key", key);
    moduleSet("__main__", "value", value);
    return 0;
  };
  ASSERT_NO_FATAL_FAILURE(createBarTypeWithSlot(Py_sq_ass_item, set_func));

  ASSERT_EQ(PyRun_SimpleString(R"(
b = Bar()
r = b.__setitem__(123, 456)
)"),
            0);
  PyObjectPtr r(moduleGet("__main__", "r"));
  EXPECT_EQ(r, Py_None);

  PyObjectPtr key(moduleGet("__main__", "key"));
  EXPECT_TRUE(isLongEqualsLong(key, 123));

  PyObjectPtr value(moduleGet("__main__", "value"));
  EXPECT_TRUE(isLongEqualsLong(value, 456));
}

TEST_F(TypeExtensionApiTest, CallSqDelitemSlotFromManagedCode) {
  ssizeobjargproc del_func = [](PyObject* self, Py_ssize_t i, PyObject* value) {
    PyObjectPtr b(moduleGet("__main__", "b"));
    EXPECT_EQ(self, b);
    PyObjectPtr key(PyLong_FromLong(i));
    moduleSet("__main__", "key", key);
    EXPECT_EQ(value, nullptr);
    return 0;
  };
  ASSERT_NO_FATAL_FAILURE(createBarTypeWithSlot(Py_sq_ass_item, del_func));

  ASSERT_EQ(PyRun_SimpleString(R"(
b = Bar()
r = b.__delitem__(7890)
)"),
            0);
  PyObjectPtr r(moduleGet("__main__", "r"));
  EXPECT_EQ(r, Py_None);

  PyObjectPtr key(moduleGet("__main__", "key"));
  EXPECT_TRUE(isLongEqualsLong(key, 7890));
}

TEST_F(TypeExtensionApiTest, HashNotImplementedSlotSetsNoneDunderHash) {
  ASSERT_NO_FATAL_FAILURE(
      createBarTypeWithSlot(Py_tp_hash, PyObject_HashNotImplemented));
  PyObjectPtr bar(moduleGet("__main__", "Bar"));
  PyObjectPtr hash(PyObject_GetAttrString(bar, "__hash__"));
  EXPECT_EQ(hash, Py_None);
}

TEST_F(TypeExtensionApiTest, CallNewSlotFromManagedCode) {
  ternaryfunc new_func = [](PyObject* type, PyObject* args, PyObject* kwargs) {
    PyObjectPtr name(PyObject_GetAttrString(type, "__name__"));
    EXPECT_TRUE(isUnicodeEqualsCStr(name, "Bar"));
    EXPECT_EQ(PyTuple_Check(args), 1);
    EXPECT_EQ(kwargs, nullptr);
    Py_INCREF(args);
    return args;
  };
  ASSERT_NO_FATAL_FAILURE(createBarTypeWithSlot(Py_tp_new, new_func));

  ASSERT_EQ(PyRun_SimpleString(R"(
r = Bar.__new__(Bar, 1, 2, 3)
)"),
            0);
  PyObjectPtr r(moduleGet("__main__", "r"));
  ASSERT_EQ(PyTuple_Check(r), 1);
  ASSERT_EQ(PyTuple_Size(r), 3);
  EXPECT_TRUE(isLongEqualsLong(PyTuple_GetItem(r, 0), 1));
  EXPECT_TRUE(isLongEqualsLong(PyTuple_GetItem(r, 1), 2));
  EXPECT_TRUE(isLongEqualsLong(PyTuple_GetItem(r, 2), 3));
}

TEST_F(TypeExtensionApiTest, NbAddSlotTakesPrecedenceOverSqConcatSlot) {
  binaryfunc add_func = [](PyObject* /* self */, PyObject* obj) {
    EXPECT_TRUE(isUnicodeEqualsCStr(obj, "foo"));
    return PyLong_FromLong(0xf00);
  };
  binaryfunc concat_func = [](PyObject*, PyObject*) -> PyObject* {
    std::abort();
  };
  static PyType_Slot slots[3];
  // Both of these slots map to __add__. nb_add appears in slotdefs first, so it
  // wins.
  slots[0] = {Py_nb_add, reinterpret_cast<void*>(add_func)};
  slots[1] = {Py_sq_concat, reinterpret_cast<void*>(concat_func)};
  slots[2] = {0, nullptr};
  static PyType_Spec spec;
  spec = {
      "__main__.Bar", 0, 0, Py_TPFLAGS_DEFAULT, slots,
  };
  PyObjectPtr type(PyType_FromSpec(&spec));
  ASSERT_NE(type, nullptr);
  ASSERT_EQ(PyType_CheckExact(type), 1);
  ASSERT_EQ(moduleSet("__main__", "Bar", type), 0);

  ASSERT_EQ(PyRun_SimpleString(R"(
b = Bar()
r = b.__add__("foo")
)"),
            0);
  PyObjectPtr r(moduleGet("__main__", "r"));
  EXPECT_TRUE(isLongEqualsLong(r, 0xf00));
}

TEST_F(TypeExtensionApiTest, TypeSlotPropagatesException) {
  binaryfunc add_func = [](PyObject*, PyObject*) -> PyObject* {
    PyErr_SetString(PyExc_RuntimeError, "hello, there!");
    return nullptr;
  };
  ASSERT_NO_FATAL_FAILURE(createBarTypeWithSlot(Py_nb_add, add_func));

  // TODO(T40700664): Use PyRun_String() so we can inspect the exception more
  // directly.
  EXPECT_EQ(PyRun_SimpleString(R"(
exc = None
try:
  Bar().__add__(1)
except RuntimeError as e:
  exc = e
)"),
            0);
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  EXPECT_EQ(PyErr_GivenExceptionMatches(exc, PyExc_RuntimeError), 1);
}

// TODO(T42827325): This destroyBar rountine is here to make up for a reference
// counting bug in our handle code. Therefore, all tests using this function
// will only work with Pyro until the bug is fixed.
static void destroyBar() {
  PyObjectPtr type(moduleGet("__main__", "Bar"));
  PyObject* b = moduleGet("__main__", "b");
  auto des = reinterpret_cast<destructor>(PyType_GetSlot(
      reinterpret_cast<PyTypeObject*>(type.get()), Py_tp_dealloc));
  ASSERT_NE(des, nullptr);
  des(b);
}

static void createBarTypeWithMembers() {
  struct BarObject {
    PyObject_HEAD int t_bool;
    char t_byte;
    unsigned char t_ubyte;
    short t_short;
    unsigned short t_ushort;
    int t_int;
    unsigned int t_uint;
    long t_long;
    unsigned long t_ulong;
    Py_ssize_t t_pyssizet;
    float t_float;
    double t_double;
    const char* t_string;
    char t_char;
    PyObject* t_object;
    PyObject* t_object_null;
    long long t_longlong;
    unsigned long long t_ulonglong;
  };

  static PyMemberDef members[22];
  members[0] = {const_cast<char*>("t_bool"), T_BOOL,
                offsetof(BarObject, t_bool)};
  members[1] = {const_cast<char*>("t_byte"), T_BYTE,
                offsetof(BarObject, t_byte)};
  members[2] = {const_cast<char*>("t_ubyte"), T_UBYTE,
                offsetof(BarObject, t_ubyte)};
  members[3] = {const_cast<char*>("t_short"), T_SHORT,
                offsetof(BarObject, t_short)};
  members[4] = {const_cast<char*>("t_ushort"), T_USHORT,
                offsetof(BarObject, t_ushort)};
  members[5] = {const_cast<char*>("t_int"), T_INT, offsetof(BarObject, t_int)};
  members[6] = {const_cast<char*>("t_uint"), T_UINT,
                offsetof(BarObject, t_uint)};
  members[7] = {const_cast<char*>("t_long"), T_LONG,
                offsetof(BarObject, t_long)};
  members[8] = {const_cast<char*>("t_ulong"), T_ULONG,
                offsetof(BarObject, t_ulong)};
  members[9] = {const_cast<char*>("t_pyssize"), T_PYSSIZET,
                offsetof(BarObject, t_pyssizet)};
  members[10] = {const_cast<char*>("t_float"), T_FLOAT,
                 offsetof(BarObject, t_float)};
  members[11] = {const_cast<char*>("t_double"), T_DOUBLE,
                 offsetof(BarObject, t_double)};
  members[12] = {const_cast<char*>("t_string"), T_STRING,
                 offsetof(BarObject, t_string)};
  members[13] = {const_cast<char*>("t_char"), T_CHAR,
                 offsetof(BarObject, t_char)};
  members[14] = {const_cast<char*>("t_object"), T_OBJECT,
                 offsetof(BarObject, t_object)};
  members[15] = {const_cast<char*>("t_object_null"), T_OBJECT,
                 offsetof(BarObject, t_object_null)};
  members[16] = {const_cast<char*>("t_objectex"), T_OBJECT_EX,
                 offsetof(BarObject, t_object)};
  members[17] = {const_cast<char*>("t_objectex_null"), T_OBJECT_EX,
                 offsetof(BarObject, t_object_null)};
  members[18] = {const_cast<char*>("t_longlong"), T_LONGLONG,
                 offsetof(BarObject, t_longlong)};
  members[19] = {const_cast<char*>("t_ulonglong"), T_ULONGLONG,
                 offsetof(BarObject, t_ulonglong)};
  members[20] = {const_cast<char*>("t_int_readonly"), T_INT,
                 offsetof(BarObject, t_int), READONLY};
  members[21] = {nullptr};

  newfunc new_func = [](PyTypeObject* type, PyObject*, PyObject*) {
    void* slot = PyType_GetSlot(type, Py_tp_alloc);
    return reinterpret_cast<allocfunc>(slot)(type, 0);
  };
  initproc init_func = [](PyObject* self, PyObject*, PyObject*) {
    reinterpret_cast<BarObject*>(self)->t_bool = 1;
    reinterpret_cast<BarObject*>(self)->t_byte = -12;
    reinterpret_cast<BarObject*>(self)->t_ubyte = -1;
    reinterpret_cast<BarObject*>(self)->t_short = -12;
    reinterpret_cast<BarObject*>(self)->t_ushort = -1;
    reinterpret_cast<BarObject*>(self)->t_int = -1234;
    reinterpret_cast<BarObject*>(self)->t_uint = -1;
    reinterpret_cast<BarObject*>(self)->t_long = -1234;
    reinterpret_cast<BarObject*>(self)->t_ulong = -1;
    reinterpret_cast<BarObject*>(self)->t_pyssizet = 1234;
    reinterpret_cast<BarObject*>(self)->t_float = 1.0;
    reinterpret_cast<BarObject*>(self)->t_double = 1.0;
    reinterpret_cast<BarObject*>(self)->t_string = "foo";
    reinterpret_cast<BarObject*>(self)->t_char = 'a';
    reinterpret_cast<BarObject*>(self)->t_object = PyList_New(0);
    reinterpret_cast<BarObject*>(self)->t_object_null = nullptr;
    reinterpret_cast<BarObject*>(self)->t_longlong =
        std::numeric_limits<long long>::max();
    reinterpret_cast<BarObject*>(self)->t_ulonglong = -1;
    return 0;
  };
  destructor dealloc_func = [](PyObject* self) {
    PyObjectPtr type(PyObject_Type(self));
    void* slot =
        PyType_GetSlot(reinterpret_cast<PyTypeObject*>(type.get()), Py_tp_free);
    return reinterpret_cast<freefunc>(slot)(self);
  };
  static PyType_Slot slots[7];
  // TODO(T40540469): Most of functions should be inherited from object.
  // However, inheritance is not supported yet. For now, just set them manually.
  slots[0] = {Py_tp_new, reinterpret_cast<void*>(new_func)};
  slots[1] = {Py_tp_init, reinterpret_cast<void*>(init_func)};
  slots[2] = {Py_tp_alloc, reinterpret_cast<void*>(PyType_GenericAlloc)},
  slots[3] = {Py_tp_dealloc, reinterpret_cast<void*>(dealloc_func)};
  slots[4] = {Py_tp_free, reinterpret_cast<void*>(PyObject_Del)};
  slots[5] = {Py_tp_members, reinterpret_cast<void*>(members)};
  slots[6] = {0, nullptr};
  static PyType_Spec spec;
  spec = {
      "__main__.Bar", sizeof(BarObject), 0, Py_TPFLAGS_DEFAULT, slots,
  };
  PyObjectPtr type(PyType_FromSpec(&spec));
  ASSERT_NE(type, nullptr);
  ASSERT_EQ(PyType_CheckExact(type), 1);
  ASSERT_EQ(moduleSet("__main__", "Bar", type), 0);
}

TEST_F(TypeExtensionApiTest, MemberBoolPyro) {
  ASSERT_NO_FATAL_FAILURE(createBarTypeWithMembers());
  ASSERT_EQ(PyRun_SimpleString(R"(
b = Bar()
r1 = b.t_bool
b.t_bool = False
r2 = b.t_bool
)"),
            0);
  PyObjectPtr r1(moduleGet("__main__", "r1"));
  ASSERT_EQ(PyBool_Check(r1), 1);
  EXPECT_EQ(r1, Py_True);
  PyObjectPtr r2(moduleGet("__main__", "r2"));
  ASSERT_EQ(PyBool_Check(r2), 1);
  EXPECT_EQ(r2, Py_False);
  destroyBar();
}

TEST_F(TypeExtensionApiTest, MemberBytePyro) {
  ASSERT_NO_FATAL_FAILURE(createBarTypeWithMembers());
  ASSERT_EQ(PyRun_SimpleString(R"(
b = Bar()
r1 = b.t_byte
b.t_byte = 21
r2 = b.t_byte
)"),
            0);
  PyObjectPtr r1(moduleGet("__main__", "r1"));
  ASSERT_EQ(PyLong_Check(r1), 1);
  EXPECT_TRUE(isLongEqualsLong(r1, -12));
  PyObjectPtr r2(moduleGet("__main__", "r2"));
  ASSERT_EQ(PyLong_Check(r2), 1);
  EXPECT_TRUE(isLongEqualsLong(r2, 21));
  destroyBar();
}

TEST_F(TypeExtensionApiTest, MemberUBytePyro) {
  ASSERT_NO_FATAL_FAILURE(createBarTypeWithMembers());
  ASSERT_EQ(PyRun_SimpleString(R"(
b = Bar()
r1 = b.t_ubyte
b.t_ubyte = 21
r2 = b.t_ubyte
)"),
            0);
  PyObjectPtr r1(moduleGet("__main__", "r1"));
  ASSERT_EQ(PyLong_Check(r1), 1);
  EXPECT_TRUE(isLongEqualsLong(r1, std::numeric_limits<unsigned char>::max()));
  PyObjectPtr r2(moduleGet("__main__", "r2"));
  ASSERT_EQ(PyLong_Check(r2), 1);
  EXPECT_TRUE(isLongEqualsLong(r2, 21));
  destroyBar();
}

TEST_F(TypeExtensionApiTest, MemberShortPyro) {
  ASSERT_NO_FATAL_FAILURE(createBarTypeWithMembers());
  ASSERT_EQ(PyRun_SimpleString(R"(
b = Bar()
r1 = b.t_short
b.t_short = 21
r2 = b.t_short
)"),
            0);
  PyObjectPtr r1(moduleGet("__main__", "r1"));
  ASSERT_EQ(PyLong_Check(r1), 1);
  EXPECT_TRUE(isLongEqualsLong(r1, -12));
  PyObjectPtr r2(moduleGet("__main__", "r2"));
  ASSERT_EQ(PyLong_Check(r2), 1);
  EXPECT_TRUE(isLongEqualsLong(r2, 21));
  destroyBar();
}

TEST_F(TypeExtensionApiTest, MemberUShortPyro) {
  ASSERT_NO_FATAL_FAILURE(createBarTypeWithMembers());
  ASSERT_EQ(PyRun_SimpleString(R"(
b = Bar()
r1 = b.t_ushort
b.t_ushort = 21
r2 = b.t_ushort
)"),
            0);
  PyObjectPtr r1(moduleGet("__main__", "r1"));
  ASSERT_EQ(PyLong_Check(r1), 1);
  EXPECT_TRUE(isLongEqualsLong(r1, std::numeric_limits<unsigned short>::max()));
  PyObjectPtr r2(moduleGet("__main__", "r2"));
  ASSERT_EQ(PyLong_Check(r2), 1);
  EXPECT_TRUE(isLongEqualsLong(r2, 21));
  destroyBar();
}

TEST_F(TypeExtensionApiTest, MemberIntPyro) {
  ASSERT_NO_FATAL_FAILURE(createBarTypeWithMembers());
  ASSERT_EQ(PyRun_SimpleString(R"(
b = Bar()
r1 = b.t_int
b.t_int = 4321
r2 = b.t_int
)"),
            0);
  PyObjectPtr r1(moduleGet("__main__", "r1"));
  ASSERT_EQ(PyLong_Check(r1), 1);
  EXPECT_TRUE(isLongEqualsLong(r1, -1234));
  PyObjectPtr r2(moduleGet("__main__", "r2"));
  ASSERT_EQ(PyLong_Check(r2), 1);
  EXPECT_TRUE(isLongEqualsLong(r2, 4321));
  destroyBar();
}

TEST_F(TypeExtensionApiTest, MemberUIntPyro) {
  ASSERT_NO_FATAL_FAILURE(createBarTypeWithMembers());
  ASSERT_EQ(PyRun_SimpleString(R"(
b = Bar()
r1 = b.t_uint
b.t_uint = 4321
r2 = b.t_uint
)"),
            0);
  PyObjectPtr r1(moduleGet("__main__", "r1"));
  ASSERT_EQ(PyLong_Check(r1), 1);
  EXPECT_EQ(PyLong_AsUnsignedLong(r1),
            std::numeric_limits<unsigned int>::max());
  PyObjectPtr r2(moduleGet("__main__", "r2"));
  ASSERT_EQ(PyLong_Check(r2), 1);
  EXPECT_EQ(PyLong_AsUnsignedLong(r2), 4321);
  destroyBar();
}

TEST_F(TypeExtensionApiTest, MemberLongPyro) {
  ASSERT_NO_FATAL_FAILURE(createBarTypeWithMembers());
  ASSERT_EQ(PyRun_SimpleString(R"(
b = Bar()
r1 = b.t_long
b.t_long = 4321
r2 = b.t_long
)"),
            0);
  PyObjectPtr r1(moduleGet("__main__", "r1"));
  ASSERT_EQ(PyLong_Check(r1), 1);
  EXPECT_TRUE(isLongEqualsLong(r1, -1234));
  PyObjectPtr r2(moduleGet("__main__", "r2"));
  ASSERT_EQ(PyLong_Check(r2), 1);
  EXPECT_TRUE(isLongEqualsLong(r2, 4321));
  destroyBar();
}

TEST_F(TypeExtensionApiTest, MemberULongPyro) {
  ASSERT_NO_FATAL_FAILURE(createBarTypeWithMembers());
  ASSERT_EQ(PyRun_SimpleString(R"(
b = Bar()
r1 = b.t_ulong
b.t_ulong = 4321
r2 = b.t_ulong
)"),
            0);
  PyObjectPtr r1(moduleGet("__main__", "r1"));
  ASSERT_EQ(PyLong_Check(r1), 1);
  EXPECT_EQ(PyLong_AsUnsignedLong(r1),
            std::numeric_limits<unsigned long>::max());
  PyObjectPtr r2(moduleGet("__main__", "r2"));
  ASSERT_EQ(PyLong_Check(r2), 1);
  EXPECT_EQ(PyLong_AsUnsignedLong(r2), 4321);
  destroyBar();
}

TEST_F(TypeExtensionApiTest, MemberLongLongPyro) {
  ASSERT_NO_FATAL_FAILURE(createBarTypeWithMembers());
  ASSERT_EQ(PyRun_SimpleString(R"(
b = Bar()
r1 = b.t_longlong
b.t_longlong = -4321
r2 = b.t_longlong
)"),
            0);
  PyObjectPtr r1(moduleGet("__main__", "r1"));
  ASSERT_EQ(PyLong_Check(r1), 1);
  EXPECT_EQ(PyLong_AsLongLong(r1), std::numeric_limits<long long>::max());
  PyObjectPtr r2(moduleGet("__main__", "r2"));
  ASSERT_EQ(PyLong_Check(r2), 1);
  EXPECT_TRUE(isLongEqualsLong(r2, -4321));
  destroyBar();
}

TEST_F(TypeExtensionApiTest, MemberULongLongPyro) {
  ASSERT_NO_FATAL_FAILURE(createBarTypeWithMembers());
  ASSERT_EQ(PyRun_SimpleString(R"(
b = Bar()
r1 = b.t_ulonglong
b.t_ulonglong = 4321
r2 = b.t_ulonglong
)"),
            0);
  PyObjectPtr r1(moduleGet("__main__", "r1"));
  ASSERT_EQ(PyLong_Check(r1), 1);
  EXPECT_EQ(PyLong_AsUnsignedLongLong(r1),
            std::numeric_limits<unsigned long long>::max());
  PyObjectPtr r2(moduleGet("__main__", "r2"));
  ASSERT_EQ(PyLong_Check(r2), 1);
  EXPECT_EQ(PyLong_AsUnsignedLongLong(r2), 4321);
  destroyBar();
}

TEST_F(TypeExtensionApiTest, MemberFloatPyro) {
  ASSERT_NO_FATAL_FAILURE(createBarTypeWithMembers());
  ASSERT_EQ(PyRun_SimpleString(R"(
b = Bar()
r1 = b.t_float
b.t_float = 1.5
r2 = b.t_float
)"),
            0);
  PyObjectPtr r1(moduleGet("__main__", "r1"));
  ASSERT_EQ(PyFloat_Check(r1), 1);
  EXPECT_EQ(PyFloat_AsDouble(r1), 1.0);
  PyObjectPtr r2(moduleGet("__main__", "r2"));
  ASSERT_EQ(PyFloat_Check(r2), 1);
  EXPECT_EQ(PyFloat_AsDouble(r2), 1.5);
  destroyBar();
}

TEST_F(TypeExtensionApiTest, MemberDoublePyro) {
  ASSERT_NO_FATAL_FAILURE(createBarTypeWithMembers());
  ASSERT_EQ(PyRun_SimpleString(R"(
b = Bar()
r1 = b.t_double
b.t_double = 1.5
r2 = b.t_double
)"),
            0);
  PyObjectPtr r1(moduleGet("__main__", "r1"));
  ASSERT_EQ(PyFloat_Check(r1), 1);
  EXPECT_EQ(PyFloat_AsDouble(r1), 1.0);
  PyObjectPtr r2(moduleGet("__main__", "r2"));
  ASSERT_EQ(PyFloat_Check(r2), 1);
  EXPECT_EQ(PyFloat_AsDouble(r2), 1.5);
  destroyBar();
}

TEST_F(TypeExtensionApiTest, MemberCharPyro) {
  ASSERT_NO_FATAL_FAILURE(createBarTypeWithMembers());
  ASSERT_EQ(PyRun_SimpleString(R"(
b = Bar()
r1 = b.t_char
b.t_cahr = 'b'
r2 = b.t_char
)"),
            0);
  PyObjectPtr r1(moduleGet("__main__", "r1"));
  ASSERT_EQ(PyUnicode_Check(r1), 1);
  EXPECT_TRUE(isUnicodeEqualsCStr(r1, "a"));
  PyObjectPtr r2(moduleGet("__main__", "r2"));
  ASSERT_EQ(PyUnicode_Check(r2), 1);
  EXPECT_TRUE(isUnicodeEqualsCStr(r1, "a"));
  destroyBar();
}

TEST_F(TypeExtensionApiTest, MemberStringPyro) {
  ASSERT_NO_FATAL_FAILURE(createBarTypeWithMembers());
  ASSERT_EQ(PyRun_SimpleString(R"(
b = Bar()
r1 = b.t_string
)"),
            0);
  PyObjectPtr r1(moduleGet("__main__", "r1"));
  ASSERT_EQ(PyUnicode_Check(r1), 1);
  EXPECT_TRUE(isUnicodeEqualsCStr(r1, "foo"));
  destroyBar();
}

TEST_F(TypeExtensionApiTest, MemberStringRaisesTypeErrorPyro) {
  ASSERT_NO_FATAL_FAILURE(createBarTypeWithMembers());
  ASSERT_EQ(PyRun_SimpleString(R"(
b = Bar()
raised = False
try:
  b.t_string = "bar"
  raise RuntimeError("call didn't throw")
except TypeError:
  raised = True
r1 = b.t_string
)"),
            0);
  PyObjectPtr r1(moduleGet("__main__", "r1"));
  PyObjectPtr raised(moduleGet("__main__", "raised"));
  EXPECT_EQ(raised, Py_True);
  ASSERT_EQ(PyUnicode_Check(r1), 1);
  EXPECT_TRUE(isUnicodeEqualsCStr(r1, "foo"));
  destroyBar();
}

TEST_F(TypeExtensionApiTest, MemberObjectPyro) {
  ASSERT_NO_FATAL_FAILURE(createBarTypeWithMembers());
  ASSERT_EQ(PyRun_SimpleString(R"(
b = Bar()
b.t_object.append(9)
r1 = b.t_object
b.t_object = tuple()
r2 = b.t_object
)"),
            0);
  PyObjectPtr r1(moduleGet("__main__", "r1"));
  ASSERT_EQ(PyList_Check(r1), 1);
  EXPECT_EQ(PyList_Size(r1), 1);
  PyObject* item = PyList_GetItem(r1, 0);
  ASSERT_EQ(PyLong_Check(item), 1);
  EXPECT_TRUE(isLongEqualsLong(item, 9));
  PyObjectPtr r2(moduleGet("__main__", "r2"));
  ASSERT_EQ(PyTuple_Check(r2), 1);
  EXPECT_EQ(PyTuple_Size(r2), 0);
  destroyBar();
}

TEST_F(TypeExtensionApiTest, MemberObjectWithNullPyro) {
  ASSERT_NO_FATAL_FAILURE(createBarTypeWithMembers());
  ASSERT_EQ(PyRun_SimpleString(R"(
b = Bar()
r1 = b.t_object_null
)"),
            0);
  PyObjectPtr r1(moduleGet("__main__", "r1"));
  EXPECT_EQ(r1, Py_None);
  destroyBar();
}

TEST_F(TypeExtensionApiTest, MemberObjectExPyro) {
  ASSERT_NO_FATAL_FAILURE(createBarTypeWithMembers());
  ASSERT_EQ(PyRun_SimpleString(R"(
b = Bar()
b.t_objectex.append(9)
r1 = b.t_objectex
b.t_objectex = tuple()
r2 = b.t_objectex
)"),
            0);
  PyObjectPtr r1(moduleGet("__main__", "r1"));
  ASSERT_EQ(PyList_Check(r1), 1);
  EXPECT_EQ(PyList_Size(r1), 1);
  PyObject* item = PyList_GetItem(r1, 0);
  ASSERT_EQ(PyLong_Check(item), 1);
  EXPECT_TRUE(isLongEqualsLong(item, 9));
  PyObjectPtr r2(moduleGet("__main__", "r2"));
  ASSERT_EQ(PyTuple_Check(r2), 1);
  EXPECT_EQ(PyTuple_Size(r2), 0);
  destroyBar();
}

TEST_F(TypeExtensionApiTest, MemberObjectExWithNullRaisesAttributeErrorPyro) {
  ASSERT_NO_FATAL_FAILURE(createBarTypeWithMembers());
  ASSERT_EQ(PyRun_SimpleString(R"(
b = Bar()
raised = False
try:
  b.t_objectex_null
  raise RuntimeError("call didn't throw")
except AttributeError:
  raised = True
)"),
            0);
  PyObjectPtr raised(moduleGet("__main__", "raised"));
  EXPECT_EQ(raised, Py_True);
  destroyBar();
}

TEST_F(TypeExtensionApiTest, MemberPySsizeTPyro) {
  ASSERT_NO_FATAL_FAILURE(createBarTypeWithMembers());
  ASSERT_EQ(PyRun_SimpleString(R"(
b = Bar()
r1 = b.t_pyssize
b.t_pyssize = 4321
r2 = b.t_pyssize
)"),
            0);
  PyObjectPtr r1(moduleGet("__main__", "r1"));
  ASSERT_EQ(PyLong_Check(r1), 1);
  EXPECT_EQ(PyLong_AsSsize_t(r1), 1234);
  PyObjectPtr r2(moduleGet("__main__", "r2"));
  ASSERT_EQ(PyLong_Check(r2), 1);
  EXPECT_EQ(PyLong_AsSsize_t(r2), 4321);
  destroyBar();
}

TEST_F(TypeExtensionApiTest, MemberReadOnlyRaisesAttributeErrorPyro) {
  ASSERT_NO_FATAL_FAILURE(createBarTypeWithMembers());
  ASSERT_EQ(PyRun_SimpleString(R"(
b = Bar()
r1 = b.t_int_readonly
raised = False
try:
  b.t_int_readonly = 4321
  raise RuntimeError("call didn't throw")
except AttributeError:
  raised = True
)"),
            0);
  PyObjectPtr r1(moduleGet("__main__", "r1"));
  ASSERT_EQ(PyLong_Check(r1), 1);
  EXPECT_TRUE(isLongEqualsLong(r1, -1234));
  PyObjectPtr raised(moduleGet("__main__", "raised"));
  EXPECT_EQ(raised, Py_True);
  destroyBar();
}

TEST_F(TypeExtensionApiTest, MemberIntSetIncorrectTypeRaisesTypeErrorPyro) {
  ASSERT_NO_FATAL_FAILURE(createBarTypeWithMembers());
  ASSERT_EQ(PyRun_SimpleString(R"(
b = Bar()
raised = False
try:
  b.t_int = "foo"
  raise RuntimeError("call didn't throw")
except TypeError:
  raised = True
r1 = b.t_int
)"),
            0);
  PyObjectPtr r1(moduleGet("__main__", "r1"));
  PyObjectPtr raised(moduleGet("__main__", "raised"));
  EXPECT_EQ(raised, Py_True);
  ASSERT_EQ(PyLong_Check(r1), 1);
  EXPECT_TRUE(isLongEqualsLong(r1, -1234));
  destroyBar();
}

TEST_F(TypeExtensionApiTest, MemberCharIncorrectSizeRaisesTypeErrorPyro) {
  ASSERT_NO_FATAL_FAILURE(createBarTypeWithMembers());
  ASSERT_EQ(PyRun_SimpleString(R"(
b = Bar()
raised = False
try:
  b.t_char = "foo"
  raise RuntimeError("call didn't throw")
except TypeError:
  raised = True
r1 = b.t_char
)"),
            0);
  PyObjectPtr r1(moduleGet("__main__", "r1"));
  PyObjectPtr raised(moduleGet("__main__", "raised"));
  EXPECT_EQ(raised, Py_True);
  ASSERT_EQ(PyUnicode_Check(r1), 1);
  EXPECT_TRUE(isUnicodeEqualsCStr(r1, "a"));
  destroyBar();
}

TEST_F(TypeExtensionApiTest, MemberUnknownRaisesSystemErrorPyro) {
  int unknown_type = -1;
  // clang-format off
  struct BarObject {
    PyObject_HEAD
    int value;
  };
  // clang-format on
  static PyMemberDef members[2];
  members[0] = {const_cast<char*>("value"), unknown_type,
                offsetof(BarObject, value), 0, nullptr};
  members[1] = {nullptr};
  static PyType_Slot slots[2];
  slots[0] = {Py_tp_members, reinterpret_cast<void*>(members)};
  slots[1] = {0, nullptr};
  static PyType_Spec spec;
  spec = {
      "__main__.Bar", sizeof(BarObject), 0, Py_TPFLAGS_DEFAULT, slots,
  };
  PyObjectPtr type(PyType_FromSpec(&spec));
  ASSERT_EQ(type, nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

static void createBarTypeWithGetSetObject() {
  struct BarObject {
    PyObject_HEAD long attribute;
    long readonly_attribute;
  };

  getter attribute_getter = [](PyObject* self, void*) {
    return PyLong_FromLong(reinterpret_cast<BarObject*>(self)->attribute);
  };

  setter attribute_setter = [](PyObject* self, PyObject* value, void*) {
    reinterpret_cast<BarObject*>(self)->attribute = PyLong_AsLong(value);
    return 0;
  };

  getter readonly_attribute_getter = [](PyObject* self, void*) {
    return PyLong_FromLong(
        reinterpret_cast<BarObject*>(self)->readonly_attribute);
  };

  setter raise_attribute_setter = [](PyObject*, PyObject*, void*) {
    PyErr_BadArgument();
    return -1;
  };

  static PyGetSetDef getsets[4];
  getsets[0] = {const_cast<char*>("attribute"), attribute_getter,
                attribute_setter};
  getsets[1] = {const_cast<char*>("readonly_attribute"),
                readonly_attribute_getter, nullptr};
  getsets[2] = {const_cast<char*>("raise_attribute"), attribute_getter,
                raise_attribute_setter};
  getsets[3] = {nullptr};

  newfunc new_func = [](PyTypeObject* type, PyObject*, PyObject*) {
    void* slot = PyType_GetSlot(type, Py_tp_alloc);
    return reinterpret_cast<allocfunc>(slot)(type, 0);
  };
  initproc init_func = [](PyObject* self, PyObject*, PyObject*) {
    reinterpret_cast<BarObject*>(self)->attribute = 123;
    reinterpret_cast<BarObject*>(self)->readonly_attribute = 456;
    return 0;
  };
  destructor dealloc_func = [](PyObject* self) {
    PyObjectPtr type(PyObject_Type(self));
    void* slot =
        PyType_GetSlot(reinterpret_cast<PyTypeObject*>(type.get()), Py_tp_free);
    return reinterpret_cast<freefunc>(slot)(self);
  };
  static PyType_Slot slots[7];
  // TODO(T40540469): Most of functions should be inherited from object.
  // However, inheritance is not supported yet. For now, just set them manually.
  slots[0] = {Py_tp_new, reinterpret_cast<void*>(new_func)};
  slots[1] = {Py_tp_init, reinterpret_cast<void*>(init_func)};
  slots[2] = {Py_tp_alloc, reinterpret_cast<void*>(PyType_GenericAlloc)},
  slots[3] = {Py_tp_dealloc, reinterpret_cast<void*>(dealloc_func)};
  slots[4] = {Py_tp_free, reinterpret_cast<void*>(PyObject_Del)};
  slots[5] = {Py_tp_getset, reinterpret_cast<void*>(getsets)};
  slots[6] = {0, nullptr};
  static PyType_Spec spec;
  spec = {
      "__main__.Bar", sizeof(BarObject), 0, Py_TPFLAGS_DEFAULT, slots,
  };
  PyObjectPtr type(PyType_FromSpec(&spec));
  ASSERT_NE(type, nullptr);
  ASSERT_EQ(PyType_CheckExact(type), 1);
  ASSERT_EQ(moduleSet("__main__", "Bar", type), 0);
}

TEST_F(TypeExtensionApiTest, GetSetAttributePyro) {
  ASSERT_NO_FATAL_FAILURE(createBarTypeWithGetSetObject());
  ASSERT_EQ(PyRun_SimpleString(R"(
b = Bar()
r1 = b.attribute
b.attribute = 321
r2 = b.attribute
)"),
            0);
  PyObjectPtr r1(moduleGet("__main__", "r1"));
  ASSERT_EQ(PyLong_Check(r1), 1);
  EXPECT_EQ(PyLong_AsLong(r1), 123);
  PyObjectPtr r2(moduleGet("__main__", "r2"));
  ASSERT_EQ(PyLong_Check(r2), 1);
  EXPECT_EQ(PyLong_AsLong(r2), 321);
  destroyBar();
}

TEST_F(TypeExtensionApiTest, GetSetReadonlyAttributePyro) {
  ASSERT_NO_FATAL_FAILURE(createBarTypeWithGetSetObject());
  ASSERT_EQ(PyRun_SimpleString(R"(
b = Bar()
raised = False
try:
  b.readonly_attribute = 321
  raise RuntimeError("call didn't throw")
except AttributeError:
  raised = True
r1 = b.readonly_attribute
)"),
            0);
  PyObjectPtr r1(moduleGet("__main__", "r1"));
  PyObjectPtr raised(moduleGet("__main__", "raised"));
  EXPECT_EQ(raised, Py_True);
  ASSERT_EQ(PyLong_Check(r1), 1);
  EXPECT_EQ(PyLong_AsLong(r1), 456);
  destroyBar();
}

TEST_F(TypeExtensionApiTest, GetSetRaiseAttributePyro) {
  ASSERT_NO_FATAL_FAILURE(createBarTypeWithGetSetObject());
  ASSERT_EQ(PyRun_SimpleString(R"(
b = Bar()
raised = False
try:
  b.raise_attribute = 321
  raise SystemError("call didn't throw")
except TypeError:
  raised = True
r1 = b.raise_attribute
)"),
            0);
  PyObjectPtr r1(moduleGet("__main__", "r1"));
  PyObjectPtr raised(moduleGet("__main__", "raised"));
  EXPECT_EQ(raised, Py_True);
  ASSERT_EQ(PyLong_Check(r1), 1);
  EXPECT_EQ(PyLong_AsLong(r1), 123);
  destroyBar();
}

}  // namespace python
