#include "gtest/gtest.h"

#include "Python.h"
#include "runtime/runtime.h"
#include "runtime/test-utils.h"

namespace python {

typedef struct {
  PyObject_HEAD;
} CallableObject;

static PyObject* CallableObject_new(PyTypeObject* type, PyObject* args,
                                    PyObject* kwds) {
  return type->tp_alloc(type, 0);
}

static void CallableObject_dealloc(CallableObject* self) {
  Py_TYPE(self)->tp_free(static_cast<void*>(self));
}

static PyObject* CallableObject_call(CallableObject* self, PyObject* args,
                                     PyObject* kwargs) {
  return PyLong_FromLong(42);
}

TEST(Abstract, ObjectCallNoArgumentsReturnsValue) {
  Runtime runtime;
  HandleScope scope;

  // Instantiate Class
  PyTypeObject callable_type{PyObject_HEAD_INIT(nullptr)};
  callable_type.tp_name = "ValidCallable";
  callable_type.tp_basicsize = sizeof(CallableObject);
  callable_type.tp_new = CallableObject_new;
  callable_type.tp_dealloc =
      reinterpret_cast<destructor>(CallableObject_dealloc);
  callable_type.tp_call = reinterpret_cast<ternaryfunc>(CallableObject_call);
  PyType_Ready(&callable_type);

  PyObject* custom_obj = PyType_GenericAlloc(&callable_type, 0);
  PyObject* result = PyObject_Call(custom_obj, PyTuple_New(0), nullptr);

  Handle<Object> result_handle(&scope,
                               ApiHandle::fromPyObject(result)->asObject());
  ASSERT_TRUE(result_handle->isSmallInteger());
  ASSERT_EQ(SmallInteger::cast(*result_handle)->value(), 42);
}

}  // namespace python
