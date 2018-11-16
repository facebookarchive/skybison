#ifndef Py_ABSTRACTOBJECT_H
#define Py_ABSTRACTOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

// public:

// Call a callable Python object, callable_object, with arguments and
// keywords arguments. The 'args' argument can not be NULL.
PyAPI_FUNC(PyObject*)
    PyObject_Call(PyObject* callable_object, PyObject* args, PyObject* kwargs);

// private:

PyAPI_FUNC(PyObject*) _Py_CheckFunctionResult(
    PyObject* func,
    PyObject* result,
    const char* where);
#ifdef __cplusplus
}
#endif
#endif /* Py_ABSTRACTOBJECT_H */
