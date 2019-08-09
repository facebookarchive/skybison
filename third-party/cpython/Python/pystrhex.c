/* bytes to hex implementation */

#include "Python.h"

static const char hexdigits[] = "0123456789abcdef";

static PyObject *_Py_strhex_impl(const char* argbuf, const Py_ssize_t arglen,
                                 int return_bytes)
{
    PyObject *retval;
    Py_UCS1* retbuf;
    Py_ssize_t i, j;

    assert(arglen >= 0);
    if (arglen > PY_SSIZE_T_MAX / 2)
        return PyErr_NoMemory();

    retbuf = (Py_UCS1*) PyMem_Malloc(arglen*2);
    if (retbuf == NULL) {
        return PyErr_NoMemory();
    }

    /* make hex version of string, taken from shamodule.c */
    for (i=j=0; i < arglen; i++) {
        unsigned char c;
        c = (argbuf[i] >> 4) & 0xf;
        retbuf[j++] = hexdigits[c];
        c = argbuf[i] & 0xf;
        retbuf[j++] = hexdigits[c];
    }

    if (return_bytes) {
        retval = PyBytes_FromStringAndSize((const char *)retbuf, arglen*2);
    } else {
	retval = PyUnicode_FromStringAndSize((const char *)retbuf, arglen*2);
    }
    PyMem_Free(retbuf);

    return retval;
}

PyAPI_FUNC(PyObject *) _Py_strhex(const char* argbuf, const Py_ssize_t arglen)
{
    return _Py_strhex_impl(argbuf, arglen, 0);
}

/* Same as above but returns a bytes() instead of str() to avoid the
 * need to decode the str() when bytes are needed. */
PyAPI_FUNC(PyObject *) _Py_strhex_bytes(const char* argbuf, const Py_ssize_t arglen)
{
    return _Py_strhex_impl(argbuf, arglen, 1);
}
