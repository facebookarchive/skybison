/* module.c - the module itself
 *
 * Copyright (C) 2004-2010 Gerhard Häring <gh@ghaering.de>
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

#include "connection.h"
#include "statement.h"
#include "cursor.h"
#include "cache.h"
#include "prepare_protocol.h"
#include "microprotocols.h"
#include "row.h"

#if SQLITE_VERSION_NUMBER >= 3003003
#define HAVE_SHARED_CACHE
#endif

static PyObject* module_connect(PyObject* self, PyObject* args, PyObject*
        kwargs)
{
    /* Python seems to have no way of extracting a single keyword-arg at
     * C-level, so this code is redundant with the one in connection_init in
     * connection.c and must always be copied from there ... */

    static char *kwlist[] = {
        "database", "timeout", "detect_types", "isolation_level",
        "check_same_thread", "factory", "cached_statements", "uri",
        NULL
    };
    PyObject* database;
    int detect_types = 0;
    PyObject* isolation_level;
    PyObject* factory = NULL;
    int check_same_thread = 1;
    int cached_statements;
    int uri = 0;
    double timeout = 5.0;

    PyObject* result;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|diOiOip", kwlist,
                                     &database, &timeout, &detect_types,
                                     &isolation_level, &check_same_thread,
                                     &factory, &cached_statements, &uri))
    {
        return NULL;
    }

    if (factory == NULL) {
        factory = (PyObject*)pysqlite_global(ConnectionType);
    }

    result = PyObject_Call(factory, args, kwargs);

    return result;
}

PyDoc_STRVAR(module_connect_doc,
"connect(database[, timeout, detect_types, isolation_level,\n\
        check_same_thread, factory, cached_statements, uri])\n\
\n\
Opens a connection to the SQLite database file *database*. You can use\n\
\":memory:\" to open a database connection to a database that resides in\n\
RAM instead of on disk.");

static PyObject* module_complete(PyObject* self, PyObject* args, PyObject*
        kwargs)
{
    static char *kwlist[] = {"statement", NULL, NULL};
    char* statement;

    PyObject* result;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", kwlist, &statement))
    {
        return NULL;
    }

    if (sqlite3_complete(statement)) {
        result = Py_True;
    } else {
        result = Py_False;
    }

    Py_INCREF(result);

    return result;
}

PyDoc_STRVAR(module_complete_doc,
"complete_statement(sql)\n\
\n\
Checks if a string contains a complete SQL statement. Non-standard.");

#ifdef HAVE_SHARED_CACHE
static PyObject* module_enable_shared_cache(PyObject* self, PyObject* args, PyObject*
        kwargs)
{
    static char *kwlist[] = {"do_enable", NULL, NULL};
    int do_enable;
    int rc;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "i", kwlist, &do_enable))
    {
        return NULL;
    }

    rc = sqlite3_enable_shared_cache(do_enable);

    if (rc != SQLITE_OK) {
        PyErr_SetString(pysqlite_global(OperationalError), "Changing the shared_cache flag failed");
        return NULL;
    } else {
        Py_RETURN_NONE;
    }
}

PyDoc_STRVAR(module_enable_shared_cache_doc,
"enable_shared_cache(do_enable)\n\
\n\
Enable or disable shared cache mode for the calling thread.\n\
Experimental/Non-standard.");
#endif /* HAVE_SHARED_CACHE */

static PyObject* module_register_adapter(PyObject* self, PyObject* args)
{
    PyTypeObject* type;
    PyObject* caster;
    int rc;

    if (!PyArg_ParseTuple(args, "OO", &type, &caster)) {
        return NULL;
    }

    /* a basic type is adapted; there's a performance optimization if that's not the case
     * (99 % of all usages) */
    if (type == &PyLong_Type || type == &PyFloat_Type
            || type == &PyUnicode_Type || type == &PyByteArray_Type) {
        pysqlite_global(BaseTypeAdapted) = 1;
    }

    rc = pysqlite_microprotocols_add(type, (PyObject*)pysqlite_global(PrepareProtocolType), caster);
    if (rc == -1)
        return NULL;

    Py_RETURN_NONE;
}

PyDoc_STRVAR(module_register_adapter_doc,
"register_adapter(type, callable)\n\
\n\
Registers an adapter with pysqlite's adapter registry. Non-standard.");

static PyObject* module_register_converter(PyObject* self, PyObject* args)
{
    PyObject* orig_name;
    PyObject* name = NULL;
    PyObject* callable;
    PyObject* retval = NULL;

    if (!PyArg_ParseTuple(args, "UO", &orig_name, &callable)) {
        return NULL;
    }

    /* convert the name to upper case */
    name = PyObject_CallMethodObjArgs(orig_name, pysqlite_global(upper), NULL);
    if (!name) {
        goto error;
    }

    if (PyDict_SetItem(pysqlite_global(converters), name, callable) != 0) {
        goto error;
    }

    Py_INCREF(Py_None);
    retval = Py_None;
error:
    Py_XDECREF(name);
    return retval;
}

PyDoc_STRVAR(module_register_converter_doc,
"register_converter(typename, callable)\n\
\n\
Registers a converter with pysqlite. Non-standard.");

static PyObject* enable_callback_tracebacks(PyObject* self, PyObject* args)
{
    if (!PyArg_ParseTuple(args, "i", &pysqlite_global(enable_callback_tracebacks))) {
        return NULL;
    }

    Py_RETURN_NONE;
}

PyDoc_STRVAR(enable_callback_tracebacks_doc,
"enable_callback_tracebacks(flag)\n\
\n\
Enable or disable callback functions throwing errors to stderr.");

static void converters_init(PyObject* module)
{
    PyObject* converters = PyDict_New();
    if (!converters) {
        return;
    }

    pysqlite_state(module)->converters = converters;
    Py_INCREF(converters);
    PyModule_AddObject(module, "converters", converters);
}

static PyMethodDef module_methods[] = {
    {"connect",  (PyCFunction)module_connect,
     METH_VARARGS | METH_KEYWORDS, module_connect_doc},
    {"complete_statement",  (PyCFunction)module_complete,
     METH_VARARGS | METH_KEYWORDS, module_complete_doc},
#ifdef HAVE_SHARED_CACHE
    {"enable_shared_cache",  (PyCFunction)module_enable_shared_cache,
     METH_VARARGS | METH_KEYWORDS, module_enable_shared_cache_doc},
#endif
    {"register_adapter", (PyCFunction)module_register_adapter,
     METH_VARARGS, module_register_adapter_doc},
    {"register_converter", (PyCFunction)module_register_converter,
     METH_VARARGS, module_register_converter_doc},
    {"adapt",  (PyCFunction)pysqlite_adapt, METH_VARARGS,
     pysqlite_adapt_doc},
    {"enable_callback_tracebacks",  (PyCFunction)enable_callback_tracebacks,
     METH_VARARGS, enable_callback_tracebacks_doc},
    {NULL, NULL}
};

struct _IntConstantPair {
    const char *constant_name;
    int constant_value;
};

typedef struct _IntConstantPair IntConstantPair;

static const IntConstantPair _int_constants[] = {
    {"PARSE_DECLTYPES", PARSE_DECLTYPES},
    {"PARSE_COLNAMES", PARSE_COLNAMES},

    {"SQLITE_OK", SQLITE_OK},
    {"SQLITE_DENY", SQLITE_DENY},
    {"SQLITE_IGNORE", SQLITE_IGNORE},
    {"SQLITE_CREATE_INDEX", SQLITE_CREATE_INDEX},
    {"SQLITE_CREATE_TABLE", SQLITE_CREATE_TABLE},
    {"SQLITE_CREATE_TEMP_INDEX", SQLITE_CREATE_TEMP_INDEX},
    {"SQLITE_CREATE_TEMP_TABLE", SQLITE_CREATE_TEMP_TABLE},
    {"SQLITE_CREATE_TEMP_TRIGGER", SQLITE_CREATE_TEMP_TRIGGER},
    {"SQLITE_CREATE_TEMP_VIEW", SQLITE_CREATE_TEMP_VIEW},
    {"SQLITE_CREATE_TRIGGER", SQLITE_CREATE_TRIGGER},
    {"SQLITE_CREATE_VIEW", SQLITE_CREATE_VIEW},
    {"SQLITE_DELETE", SQLITE_DELETE},
    {"SQLITE_DROP_INDEX", SQLITE_DROP_INDEX},
    {"SQLITE_DROP_TABLE", SQLITE_DROP_TABLE},
    {"SQLITE_DROP_TEMP_INDEX", SQLITE_DROP_TEMP_INDEX},
    {"SQLITE_DROP_TEMP_TABLE", SQLITE_DROP_TEMP_TABLE},
    {"SQLITE_DROP_TEMP_TRIGGER", SQLITE_DROP_TEMP_TRIGGER},
    {"SQLITE_DROP_TEMP_VIEW", SQLITE_DROP_TEMP_VIEW},
    {"SQLITE_DROP_TRIGGER", SQLITE_DROP_TRIGGER},
    {"SQLITE_DROP_VIEW", SQLITE_DROP_VIEW},
    {"SQLITE_INSERT", SQLITE_INSERT},
    {"SQLITE_PRAGMA", SQLITE_PRAGMA},
    {"SQLITE_READ", SQLITE_READ},
    {"SQLITE_SELECT", SQLITE_SELECT},
    {"SQLITE_TRANSACTION", SQLITE_TRANSACTION},
    {"SQLITE_UPDATE", SQLITE_UPDATE},
    {"SQLITE_ATTACH", SQLITE_ATTACH},
    {"SQLITE_DETACH", SQLITE_DETACH},
#if SQLITE_VERSION_NUMBER >= 3002001
    {"SQLITE_ALTER_TABLE", SQLITE_ALTER_TABLE},
    {"SQLITE_REINDEX", SQLITE_REINDEX},
#endif
#if SQLITE_VERSION_NUMBER >= 3003000
    {"SQLITE_ANALYZE", SQLITE_ANALYZE},
#endif
#if SQLITE_VERSION_NUMBER >= 3003007
    {"SQLITE_CREATE_VTABLE", SQLITE_CREATE_VTABLE},
    {"SQLITE_DROP_VTABLE", SQLITE_DROP_VTABLE},
#endif
#if SQLITE_VERSION_NUMBER >= 3003008
    {"SQLITE_FUNCTION", SQLITE_FUNCTION},
#endif
#if SQLITE_VERSION_NUMBER >= 3006008
    {"SQLITE_SAVEPOINT", SQLITE_SAVEPOINT},
#endif
#if SQLITE_VERSION_NUMBER >= 3008003
    {"SQLITE_RECURSIVE", SQLITE_RECURSIVE},
#endif
#if SQLITE_VERSION_NUMBER >= 3006011
    {"SQLITE_DONE", SQLITE_DONE},
#endif
    {(char*)NULL, 0}
};

static int _sqlite3module_traverse(PyObject *m, visitproc visit, void *arg)
{
    pysqlite_state* state = pysqlite_state(m);
    Py_VISIT(state->CacheType);
    Py_VISIT(state->ConnectionType);
    Py_VISIT(state->CursorType);
    Py_VISIT(state->NodeType);
    Py_VISIT(state->PrepareProtocolType);
    Py_VISIT(state->RowType);
    Py_VISIT(state->StatementType);
    Py_VISIT(state->Error);
    Py_VISIT(state->Warning);
    Py_VISIT(state->InterfaceError);
    Py_VISIT(state->DatabaseError);
    Py_VISIT(state->InternalError);
    Py_VISIT(state->OperationalError);
    Py_VISIT(state->ProgrammingError);
    Py_VISIT(state->IntegrityError);
    Py_VISIT(state->DataError);
    Py_VISIT(state->NotSupportedError);
    Py_VISIT(state->adapt);
    Py_VISIT(state->conform);
    Py_VISIT(state->cursor);
    Py_VISIT(state->finalize);
    Py_VISIT(state->upper);
    Py_VISIT(state->converters);
    Py_VISIT(state->psyco_adapters);
    return 0;
}

static int _sqlite3module_clear(PyObject *m)
{
    pysqlite_state* state = pysqlite_state(m);
    Py_CLEAR(state->CacheType);
    Py_CLEAR(state->ConnectionType);
    Py_CLEAR(state->CursorType);
    Py_CLEAR(state->NodeType);
    Py_CLEAR(state->PrepareProtocolType);
    Py_CLEAR(state->RowType);
    Py_CLEAR(state->StatementType);
    Py_CLEAR(state->Error);
    Py_CLEAR(state->Warning);
    Py_CLEAR(state->InterfaceError);
    Py_CLEAR(state->DatabaseError);
    Py_CLEAR(state->InternalError);
    Py_CLEAR(state->OperationalError);
    Py_CLEAR(state->ProgrammingError);
    Py_CLEAR(state->IntegrityError);
    Py_CLEAR(state->DataError);
    Py_CLEAR(state->NotSupportedError);
    Py_CLEAR(state->adapt);
    Py_CLEAR(state->conform);
    Py_CLEAR(state->cursor);
    Py_CLEAR(state->finalize);
    Py_CLEAR(state->upper);
    Py_CLEAR(state->converters);
    Py_CLEAR(state->psyco_adapters);
    return 0;
}

static void _sqlite3module_free(void *m)
{
    _sqlite3module_clear((PyObject *)m);
}

struct PyModuleDef _sqlite3module = {
        PyModuleDef_HEAD_INIT,
        "_sqlite3",
        NULL,
        sizeof(pysqlite_state),
        module_methods,
        NULL,
        _sqlite3module_traverse,
        _sqlite3module_clear,
        _sqlite3module_free,
};

PyMODINIT_FUNC PyInit__sqlite3(void)
{
    PyObject *module;
    pysqlite_state *state;
    PyObject *cache_type;
    PyObject *connection_type;
    PyObject *cursor_type;
    PyObject *node_type;
    PyObject *prepare_protocol_type;
    PyObject *row_type;
    PyObject *statement_type;
    PyObject *error_type;
    PyObject *warning_type;
    PyObject *interface_error_type;
    PyObject *database_error_type;
    PyObject *internal_error_type;
    PyObject *operational_error_type;
    PyObject *programming_error_type;
    PyObject *integrity_error_type;
    PyObject *data_error_type;
    PyObject *not_supported_error_type;
    PyObject *tmp_obj;
    int i;

    module = PyState_FindModule(&_sqlite3module);
    if (module != NULL) {
        Py_INCREF(module);
        return module;
    }

    module = PyModule_Create(&_sqlite3module);

    if (module == NULL) {
        return NULL;
    }

    state = pysqlite_state(module);

    cache_type = pysqlite_setup_CacheType();
    if (cache_type == NULL) goto error;
    state->CacheType = (PyTypeObject*)cache_type;
    Py_INCREF(cache_type);
    PyModule_AddObject(module, "Cache", cache_type);

    connection_type = pysqlite_setup_ConnectionType();
    if (connection_type == NULL) goto error;
    state->ConnectionType = (PyTypeObject*)connection_type;
    Py_INCREF(connection_type);
    PyModule_AddObject(module, "Connection", connection_type);

    cursor_type = pysqlite_setup_CursorType();
    if (cursor_type == NULL) goto error;
    state->CursorType = (PyTypeObject*)cursor_type;
    Py_INCREF(cursor_type);
    PyModule_AddObject(module, "Cursor", cursor_type);

    node_type = pysqlite_setup_NodeType();
    if (node_type == NULL) goto error;
    state->NodeType = (PyTypeObject*)node_type;

    prepare_protocol_type = pysqlite_setup_PrepareProtocolType();
    if (prepare_protocol_type == NULL) goto error;
    state->PrepareProtocolType = (PyTypeObject*)prepare_protocol_type;
    Py_INCREF(prepare_protocol_type);
    PyModule_AddObject(module, "PrepareProtocol", prepare_protocol_type);

    row_type = pysqlite_setup_RowType();
    if (row_type == NULL) goto error;
    state->RowType = (PyTypeObject*)row_type;
    Py_INCREF(row_type);
    PyModule_AddObject(module, "Row", row_type);

    statement_type = pysqlite_setup_StatementType();
    if (statement_type == NULL) goto error;
    state->StatementType = (PyTypeObject*)statement_type;
    Py_INCREF(statement_type);
    PyModule_AddObject(module, "Statement", statement_type);

    /* Create DB-API Exception hierarchy */

    error_type = PyErr_NewException(MODULE_NAME ".Error", PyExc_Exception, NULL);
    if (error_type == NULL) {
        goto error;
    }
    state->Error = error_type;
    Py_INCREF(error_type);
    PyModule_AddObject(module, "Error", error_type);

    warning_type = PyErr_NewException(MODULE_NAME ".Warning", PyExc_Exception, NULL);
    if (warning_type == NULL) {
        goto error;
    }
    state->Warning = warning_type;
    Py_INCREF(warning_type);
    PyModule_AddObject(module, "Warning", warning_type);

    /* Error subclasses */

    interface_error_type = PyErr_NewException(MODULE_NAME ".InterfaceError", error_type, NULL);
    if (interface_error_type == NULL) {
        goto error;
    }
    state->InterfaceError = interface_error_type;
    Py_INCREF(interface_error_type);
    PyModule_AddObject(module, "InterfaceError", interface_error_type);

    database_error_type = PyErr_NewException(MODULE_NAME ".DatabaseError", error_type, NULL);
    if (database_error_type == NULL) {
        goto error;
    }
    state->DatabaseError = database_error_type;
    Py_INCREF(database_error_type);
    PyModule_AddObject(module, "DatabaseError", database_error_type);

    /* DatabaseError subclasses */

    internal_error_type = PyErr_NewException(MODULE_NAME ".InternalError", database_error_type, NULL);
    if (internal_error_type == NULL) {
        goto error;
    }
    state->InternalError = internal_error_type;
    Py_INCREF(internal_error_type);
    PyModule_AddObject(module, "InternalError", internal_error_type);

    operational_error_type = PyErr_NewException(MODULE_NAME ".OperationalError", database_error_type, NULL);
    if (operational_error_type == NULL) {
        goto error;
    }
    state->OperationalError = operational_error_type;
    Py_INCREF(operational_error_type);
    PyModule_AddObject(module, "OperationalError", operational_error_type);

    programming_error_type = PyErr_NewException(MODULE_NAME ".ProgrammingError", database_error_type, NULL);
    if (programming_error_type == NULL) {
        goto error;
    }
    state->ProgrammingError = programming_error_type;
    Py_INCREF(programming_error_type);
    PyModule_AddObject(module, "ProgrammingError", programming_error_type);

    integrity_error_type = PyErr_NewException(MODULE_NAME ".IntegrityError", database_error_type, NULL);
    if (integrity_error_type == NULL) {
        goto error;
    }
    state->IntegrityError = integrity_error_type;
    Py_INCREF(integrity_error_type);
    PyModule_AddObject(module, "IntegrityError", integrity_error_type);

    data_error_type = PyErr_NewException(MODULE_NAME ".DataError", database_error_type, NULL);
    if (data_error_type == NULL) {
        goto error;
    }
    state->DataError = data_error_type;
    Py_INCREF(data_error_type);
    PyModule_AddObject(module, "DataError", data_error_type);

    not_supported_error_type = PyErr_NewException(MODULE_NAME ".NotSupportedError", database_error_type, NULL);
    if (not_supported_error_type == NULL) {
        goto error;
    }
    state->NotSupportedError = not_supported_error_type;
    Py_INCREF(not_supported_error_type);
    PyModule_AddObject(module, "NotSupportedError", not_supported_error_type);

    /* In Python 2.x, setting Connection.text_factory to
       OptimizedUnicode caused Unicode objects to be returned for
       non-ASCII data and bytestrings to be returned for ASCII data.
       Now OptimizedUnicode is an alias for str, so it has no
       effect. */
    Py_INCREF((PyObject*)&PyUnicode_Type);
    PyModule_AddObject(module, "OptimizedUnicode", (PyObject*)&PyUnicode_Type);

    /* Set integer constants */
    for (i = 0; _int_constants[i].constant_name != NULL; i++) {
        tmp_obj = PyLong_FromLong(_int_constants[i].constant_value);
        if (!tmp_obj) {
            goto error;
        }
        PyModule_AddObject(module, _int_constants[i].constant_name, tmp_obj);
    }

    if (!(tmp_obj = PyUnicode_FromString(PYSQLITE_VERSION))) {
        goto error;
    }
    PyModule_AddObject(module, "version", tmp_obj);

    if (!(tmp_obj = PyUnicode_FromString(sqlite3_libversion()))) {
        goto error;
    }
    PyModule_AddObject(module, "sqlite_version", tmp_obj);

    /* initialize microprotocols layer */
    pysqlite_microprotocols_init(module);

    /* initialize the default converters */
    converters_init(module);

    state->adapt = PyUnicode_InternFromString("__adapt__");
    state->conform = PyUnicode_InternFromString("__conform__");
    state->cursor = PyUnicode_InternFromString("cursor");
    state->finalize = PyUnicode_InternFromString("finalize");
    state->upper = PyUnicode_InternFromString("upper");

error:
    if (PyErr_Occurred())
    {
        PyErr_SetString(PyExc_ImportError, MODULE_NAME ": init failed");
        _sqlite3module_clear(module);
        Py_CLEAR(module);
    }
    return module;
}
