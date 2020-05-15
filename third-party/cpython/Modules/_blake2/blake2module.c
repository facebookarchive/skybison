/*
 * Written in 2013 by Dmitry Chestnykh <dmitry@codingrobots.com>
 * Modified for CPython by Christian Heimes <christian@python.org>
 *
 * To the extent possible under law, the author have dedicated all
 * copyright and related and neighboring rights to this software to
 * the public domain worldwide. This software is distributed without
 * any warranty. http://creativecommons.org/publicdomain/zero/1.0/
 */

#include "Python.h"

#include "impl/blake2.h"

static struct PyModuleDef blake2_module;
extern PyType_Spec PyBlake2_BLAKE2bType_spec;
extern PyType_Spec PyBlake2_BLAKE2sType_spec;

PyDoc_STRVAR(blake2mod__doc__,
"_blake2b provides BLAKE2b for hashlib\n"
);


static struct PyMethodDef blake2mod_functions[] = {
    {NULL, NULL}
};

static int blake2_module_traverse(PyObject *m, visitproc visit, void *arg) {
    Py_VISIT(blake2_modulestate(m)->PyBlake2_BLAKE2bType);
    Py_VISIT(blake2_modulestate(m)->PyBlake2_BLAKE2sType);
    return 0;
}
static int blake2_module_clear(PyObject *m) {
    Py_CLEAR(blake2_modulestate(m)->PyBlake2_BLAKE2bType);
    Py_CLEAR(blake2_modulestate(m)->PyBlake2_BLAKE2sType);
    return 0;
}
static void blake2_module_free(void *m) {
    blake2_module_clear((PyObject *)m);
}

static struct PyModuleDef blake2_module = {
    PyModuleDef_HEAD_INIT,
    "_blake2",
    blake2mod__doc__,
    sizeof(blake2_modulestate),
    blake2mod_functions,
    NULL,
    blake2_module_traverse,
    blake2_module_clear,
    blake2_module_free,
};

#define ADD_INT(d, name, value) do { \
    PyObject *x = PyLong_FromLong(value); \
    if (!x) { \
        Py_DECREF(m); \
        return NULL; \
    } \
    if (PyObject_SetAttrString(d, name, x) < 0) { \
        Py_DECREF(m); \
        return NULL; \
    } \
    Py_DECREF(x); \
} while(0)


PyMODINIT_FUNC
PyInit__blake2(void)
{
    PyObject *m, *PyBlake2_BLAKE2bType, *PyBlake2_BLAKE2sType;

    m = PyState_FindModule(&blake2_module);
    if (m != NULL) {
        Py_INCREF(m);
        return m;
    }
    m = PyModule_Create(&blake2_module);
    if (m == NULL) {
        return NULL;
    }

    /* BLAKE2b */
    PyBlake2_BLAKE2bType = PyType_FromSpec(&PyBlake2_BLAKE2bType_spec);
    if (PyBlake2_BLAKE2bType == NULL) {
        return NULL;
    }
    Py_INCREF(PyBlake2_BLAKE2bType);
    PyModule_AddObject(m, "blake2b", PyBlake2_BLAKE2bType);
    blake2_modulestate(m)->PyBlake2_BLAKE2bType = PyBlake2_BLAKE2bType;

    ADD_INT(PyBlake2_BLAKE2bType, "SALT_SIZE", BLAKE2B_SALTBYTES);
    ADD_INT(PyBlake2_BLAKE2bType, "PERSON_SIZE", BLAKE2B_PERSONALBYTES);
    ADD_INT(PyBlake2_BLAKE2bType, "MAX_KEY_SIZE", BLAKE2B_KEYBYTES);
    ADD_INT(PyBlake2_BLAKE2bType, "MAX_DIGEST_SIZE", BLAKE2B_OUTBYTES);

    PyModule_AddIntConstant(m, "BLAKE2B_SALT_SIZE", BLAKE2B_SALTBYTES);
    PyModule_AddIntConstant(m, "BLAKE2B_PERSON_SIZE", BLAKE2B_PERSONALBYTES);
    PyModule_AddIntConstant(m, "BLAKE2B_MAX_KEY_SIZE", BLAKE2B_KEYBYTES);
    PyModule_AddIntConstant(m, "BLAKE2B_MAX_DIGEST_SIZE", BLAKE2B_OUTBYTES);

    /* BLAKE2s */
    PyBlake2_BLAKE2sType = PyType_FromSpec(&PyBlake2_BLAKE2sType_spec);
    if (PyBlake2_BLAKE2sType == NULL) {
        return NULL;
    }
    Py_INCREF(PyBlake2_BLAKE2sType);
    PyModule_AddObject(m, "blake2s", PyBlake2_BLAKE2sType);
    blake2_modulestate(m)->PyBlake2_BLAKE2sType = PyBlake2_BLAKE2sType;

    ADD_INT(PyBlake2_BLAKE2sType, "SALT_SIZE", BLAKE2S_SALTBYTES);
    ADD_INT(PyBlake2_BLAKE2sType, "PERSON_SIZE", BLAKE2S_PERSONALBYTES);
    ADD_INT(PyBlake2_BLAKE2sType, "MAX_KEY_SIZE", BLAKE2S_KEYBYTES);
    ADD_INT(PyBlake2_BLAKE2sType, "MAX_DIGEST_SIZE", BLAKE2S_OUTBYTES);

    PyModule_AddIntConstant(m, "BLAKE2S_SALT_SIZE", BLAKE2S_SALTBYTES);
    PyModule_AddIntConstant(m, "BLAKE2S_PERSON_SIZE", BLAKE2S_PERSONALBYTES);
    PyModule_AddIntConstant(m, "BLAKE2S_MAX_KEY_SIZE", BLAKE2S_KEYBYTES);
    PyModule_AddIntConstant(m, "BLAKE2S_MAX_DIGEST_SIZE", BLAKE2S_OUTBYTES);

    return m;
}
