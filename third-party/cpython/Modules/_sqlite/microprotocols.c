/* microprotocols.c - minimalist and non-validating protocols implementation
 *
 * Copyright (C) 2003-2004 Federico Di Gregorio <fog@debian.org>
 *
 * This file is part of psycopg and was adapted for pysqlite. Federico Di
 * Gregorio gave the permission to use it within pysqlite under the following
 * license:
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#include <Python.h>
#include <structmember.h>

#include "cursor.h"
#include "microprotocols.h"
#include "prepare_protocol.h"


/* pysqlite_microprotocols_init - initialize the adapters dictionary */

int
pysqlite_microprotocols_init(PyObject *module)
{
  PyObject* psyco_adapters;

    /* create adapters dictionary and put it in module namespace */
    if ((psyco_adapters = PyDict_New()) == NULL) {
        return -1;
    }

    pysqlite_state(module)->psyco_adapters = psyco_adapters;
    Py_INCREF(psyco_adapters);
    return PyModule_AddObject(module, "adapters", psyco_adapters);
}


/* pysqlite_microprotocols_add - add a reverse type-caster to the dictionary */

int
pysqlite_microprotocols_add(PyTypeObject *type, PyObject *proto, PyObject *cast)
{
    PyObject* key;
    int rc;

    if (proto == NULL) proto = (PyObject*)pysqlite_global->PrepareProtocolType;

    key = Py_BuildValue("(OO)", (PyObject*)type, proto);
    if (!key) {
        return -1;
    }

    rc = PyDict_SetItem(pysqlite_global->psyco_adapters, key, cast);
    Py_DECREF(key);

    return rc;
}

/* pysqlite_microprotocols_adapt - adapt an object to the built-in protocol */

PyObject *
pysqlite_microprotocols_adapt(PyObject *obj, PyObject *proto, PyObject *alt)
{
    PyObject *adapter, *key, *adapted;

    /* we don't check for exact type conformance as specified in PEP 246
       because the pysqlite_PrepareProtocolType type is abstract and there is no
       way to get a quotable object to be its instance */

    /* look for an adapter in the registry */
    key = Py_BuildValue("(OO)", Py_TYPE(obj), proto);
    if (!key) {
        return NULL;
    }
    adapter = PyDict_GetItemWithError(pysqlite_global->psyco_adapters, key);
    Py_DECREF(key);
    if (adapter) {
        Py_INCREF(adapter);
        adapted = PyObject_CallFunctionObjArgs(adapter, obj, NULL);
        Py_DECREF(adapter);
        return adapted;
    }
    if (PyErr_Occurred()) {
        return NULL;
    }

    /* try to have the protocol adapt this object */
    if (_PyObject_LookupAttr(proto, pysqlite_global->adapt, &adapter) < 0) {
        return NULL;
    }
    if (adapter) {
        adapted = PyObject_CallFunctionObjArgs(adapter, obj, NULL);
        Py_DECREF(adapter);

        if (adapted == Py_None) {
            Py_DECREF(adapted);
        }
        else if (adapted || !PyErr_ExceptionMatches(PyExc_TypeError)) {
            return adapted;
        }
        else {
            PyErr_Clear();
        }
    }

    /* and finally try to have the object adapt itself */
    if (_PyObject_LookupAttr(obj, pysqlite_global->conform, &adapter) < 0) {
        return NULL;
    }
    if (adapter) {
        adapted = PyObject_CallFunctionObjArgs(adapter, proto, NULL);
        Py_DECREF(adapter);

        if (adapted == Py_None) {
            Py_DECREF(adapted);
        }
        else if (adapted || !PyErr_ExceptionMatches(PyExc_TypeError)) {
            return adapted;
        }
        else {
            PyErr_Clear();
        }
    }

    if (alt) {
        Py_INCREF(alt);
        return alt;
    }
    /* else set the right exception and return NULL */
    PyErr_SetString(pysqlite_global->ProgrammingError, "can't adapt");
    return NULL;
}

/** module-level functions **/

PyObject *
pysqlite_adapt(pysqlite_Cursor *self, PyObject *args)
{
    PyObject *obj, *alt = NULL;
    PyObject *proto = (PyObject*)pysqlite_global->PrepareProtocolType;

    if (!PyArg_ParseTuple(args, "O|OO", &obj, &proto, &alt)) return NULL;
    return pysqlite_microprotocols_adapt(obj, proto, alt);
}
