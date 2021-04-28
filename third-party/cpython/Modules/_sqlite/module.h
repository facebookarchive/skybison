/* module.h - definitions for the module
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

#ifndef PYSQLITE_MODULE_H
#define PYSQLITE_MODULE_H
#define PY_SSIZE_T_CLEAN
#include "Python.h"

#define PYSQLITE_VERSION "2.6.0"

/* TODO(T71639364): fix this upstream or set variable in build */
#ifndef MODULE_NAME
#define MODULE_NAME "sqlite3"
#endif

typedef struct {
    /* SQLite types */
    PyTypeObject* CacheType;
    PyTypeObject* ConnectionType;
    PyTypeObject* CursorType;
    PyTypeObject* NodeType;
    PyTypeObject* PrepareProtocolType;
    PyTypeObject* RowType;
    PyTypeObject* StatementType;

      /* error types */
    PyObject* Error;
    PyObject* Warning;
    PyObject* InterfaceError;
    PyObject* DatabaseError;
    PyObject* InternalError;
    PyObject* OperationalError;
    PyObject* ProgrammingError;
    PyObject* IntegrityError;
    PyObject* DataError;
    PyObject* NotSupportedError;

    /* identifiers */
    PyObject* adapt;
    PyObject* conform;
    PyObject* cursor;
    PyObject* finalize;
    PyObject* upper;
    PyObject* iterdump;

    /* A dictionary, mapping column types (INTEGER, VARCHAR, etc.) to converter
    * functions, that convert the SQL value to the appropriate Python value.
    * The key is uppercase.
    */
    PyObject* converters;

    /* the adapters registry */
    PyObject* psyco_adapters;

    int enable_callback_tracebacks;
    int BaseTypeAdapted;
} pysqlite_state;

extern struct PyModuleDef _sqlite3module;

#define pysqlite_state(o) ((pysqlite_state*)PyModule_GetState(o))
#define pysqlite_global (pysqlite_state(PyState_FindModule(&_sqlite3module)))

#define PARSE_DECLTYPES 1
#define PARSE_COLNAMES 2
#endif
