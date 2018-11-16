/* Boolean object interface */

#ifndef Py_BOOLOBJECT_H
#define Py_BOOLOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

PyAPI_FUNC(PyTypeObject*) PyBool_Type_Ptr(void);
#define PyBool_Type (*PyBool_Type_Ptr()) /* built-in 'bool' */

#define PyBool_Check(x) (Py_TYPE(x) == &PyBool_Type)

/* Py_False and Py_True are the only two bools in existence.
Don't forget to apply Py_INCREF() when returning either!!! */

/* Use these macros */
PyAPI_FUNC(PyObject*) PyFalse_Ptr(void);
#define Py_False ((PyObject *) &(*PyFalse_Ptr()))
PyAPI_FUNC(PyObject*) PyTrue_Ptr(void);
#define Py_True ((PyObject *) &(*PyTrue_Ptr()))

/* Macros for returning Py_True or Py_False, respectively */
#define Py_RETURN_TRUE return Py_INCREF(Py_True), Py_True
#define Py_RETURN_FALSE return Py_INCREF(Py_False), Py_False

/* Function to return a bool from a C long */
PyAPI_FUNC(PyObject *) PyBool_FromLong(long);

#ifdef __cplusplus
}
#endif
#endif /* !Py_BOOLOBJECT_H */
