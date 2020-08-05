/* row.c - an enhanced tuple for database rows
 *
 * Copyright (C) 2005-2010 Gerhard Häring <gh@ghaering.de>
 *
 * This file is part of pysqlite.
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

#include "row.h"
#include "cursor.h"

void pysqlite_row_dealloc(pysqlite_Row* self)
{
    PyTypeObject* tp;
    freefunc func;

    Py_XDECREF(self->data);
    Py_XDECREF(self->description);

    tp = Py_TYPE(self);
    func = PyType_GetSlot(tp, Py_tp_free);
    Py_DECREF(tp);
    func(self);
}

static PyObject *
pysqlite_row_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    pysqlite_Row *self;
    PyObject* data;
    pysqlite_Cursor* cursor;

    assert(type != NULL && PyType_GetSlot(type, Py_tp_alloc) != NULL);

    if (kwargs != NULL) {
        if (!PyDict_CheckExact(kwargs)) {
            PyErr_BadInternalCall();
            return NULL;
        }
        if (PyDict_Size(kwargs) > 0) {
            PyErr_SetString(PyExc_TypeError, "Row() takes no keyword arguments");
            return NULL;
        }
    }

    if (!PyArg_ParseTuple(args, "OO", &cursor, &data))
        return NULL;

    if (!PyObject_TypeCheck((PyObject*)cursor, pysqlite_global(CursorType))) {
        PyErr_SetString(PyExc_TypeError, "instance of cursor required for first argument");
        return NULL;
    }

    if (!PyTuple_Check(data)) {
        PyErr_SetString(PyExc_TypeError, "tuple required for second argument");
        return NULL;
    }

    self = (pysqlite_Row *) PyType_GenericNew(type, NULL, NULL);
    if (self == NULL)
        return NULL;

    Py_INCREF(data);
    self->data = data;

    Py_INCREF(cursor->description);
    self->description = cursor->description;

    return (PyObject *) self;
}

PyObject* pysqlite_row_item(pysqlite_Row* self, Py_ssize_t idx)
{
   PyObject* item = PyTuple_GetItem(self->data, idx);
   Py_XINCREF(item);
   return item;
}

PyObject* pysqlite_row_subscript(pysqlite_Row* self, PyObject* idx)
{
    Py_ssize_t _idx;
    PyObject *key_bytes = NULL;
    const char *key;
    Py_ssize_t nitems, i;
    PyObject *compare_bytes = NULL;
    const char *compare_key;

    const char *p1;
    const char *p2;

    PyObject* item = NULL;

    if (PyLong_Check(idx)) {
        _idx = PyNumber_AsSsize_t(idx, PyExc_IndexError);
        if (_idx == -1 && PyErr_Occurred())
            return NULL;
        if (_idx < 0)
           _idx += PyTuple_Size(self->data);
        item = PyTuple_GetItem(self->data, _idx);
        Py_XINCREF(item);
        return item;
    } else if (PyUnicode_Check(idx)) {
        key_bytes = PyUnicode_AsUTF8String(idx);
        if (key_bytes == NULL)
            return NULL;

        key = PyBytes_AsString(key_bytes);
        if (key == NULL)
            goto finally;

        nitems = PyTuple_Size(self->description);

        for (i = 0; i < nitems; i++) {
            PyObject *obj;
            obj = PyTuple_GetItem(self->description, i);
            obj = PyTuple_GetItem(obj, 0);
            compare_bytes = PyUnicode_AsUTF8String(obj);
            if (compare_bytes == NULL)
                goto finally;

            compare_key = PyBytes_AsString(compare_bytes);
            if (compare_key == NULL)
                goto finally;

            p1 = key;
            p2 = compare_key;

            while (1) {
                if ((*p1 == (char)0) || (*p2 == (char)0)) {
                    break;
                }

                if ((*p1 | 0x20) != (*p2 | 0x20)) {
                    break;
                }

                p1++;
                p2++;
            }

            if ((*p1 == (char)0) && (*p2 == (char)0)) {
                /* found item */
                item = PyTuple_GetItem(self->data, i);
                Py_INCREF(item);
                goto finally;
            }

            Py_CLEAR(compare_bytes);
        }

        PyErr_SetString(PyExc_IndexError, "No item with that key");
finally:
        Py_XDECREF(key_bytes);
        Py_XDECREF(compare_bytes);
        return item;
    }
    else if (PySlice_Check(idx)) {
        return PyObject_GetItem(self->data, idx);
    }
    else {
        PyErr_SetString(PyExc_IndexError, "Index must be int or string");
        return NULL;
    }
}

static Py_ssize_t
pysqlite_row_length(pysqlite_Row* self)
{
    return PyTuple_Size(self->data);
}

PyObject* pysqlite_row_keys(pysqlite_Row* self, PyObject* args, PyObject* kwargs)
{
    PyObject* list;
    Py_ssize_t nitems, i;

    list = PyList_New(0);
    if (!list) {
        return NULL;
    }
    nitems = PyTuple_Size(self->description);

    for (i = 0; i < nitems; i++) {
        if (PyList_Append(list, PyTuple_GetItem(PyTuple_GetItem(self->description, i), 0)) != 0) {
            Py_DECREF(list);
            return NULL;
        }
    }

    return list;
}

static PyObject* pysqlite_iter(pysqlite_Row* self)
{
    return PyObject_GetIter(self->data);
}

static Py_hash_t pysqlite_row_hash(pysqlite_Row *self)
{
    return PyObject_Hash(self->description) ^ PyObject_Hash(self->data);
}

static PyObject* pysqlite_row_richcompare(pysqlite_Row *self, PyObject *_other, int opid)
{
    if (opid != Py_EQ && opid != Py_NE)
        Py_RETURN_NOTIMPLEMENTED;

    if (PyType_IsSubtype(Py_TYPE(_other), pysqlite_global(RowType))) {
        pysqlite_Row *other = (pysqlite_Row *)_other;
        PyObject *res = PyObject_RichCompare(self->description, other->description, opid);
        if ((opid == Py_EQ && res == Py_True)
            || (opid == Py_NE && res == Py_False)) {
            Py_DECREF(res);
            return PyObject_RichCompare(self->data, other->data, opid);
        }
    }
    Py_RETURN_NOTIMPLEMENTED;
}

static PyMethodDef pysqlite_row_methods[] = {
    {"keys", (PyCFunction)pysqlite_row_keys, METH_NOARGS,
        PyDoc_STR("Returns the keys of the row.")},
    {NULL, NULL}
};

static PyType_Slot pysqlite_RowType_slots[] = {
    {Py_mp_length, pysqlite_row_length},
    {Py_mp_subscript, pysqlite_row_subscript},
    {Py_sq_item, pysqlite_row_item},
    {Py_sq_length, pysqlite_row_length},
    {Py_tp_dealloc, pysqlite_row_dealloc},
    {Py_tp_hash, pysqlite_row_hash},
    {Py_tp_iter, pysqlite_iter},
    {Py_tp_methods, pysqlite_row_methods},
    {Py_tp_new, pysqlite_row_new},
    {Py_tp_richcompare, pysqlite_row_richcompare},
    {0, 0},
};

static PyType_Spec pysqlite_RowType_spec = {
    MODULE_NAME ".Row",
    sizeof(pysqlite_Row),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    pysqlite_RowType_slots,
};

extern PyObject* pysqlite_setup_RowType(void)
{
    return PyType_FromSpec(&pysqlite_RowType_spec);
}
