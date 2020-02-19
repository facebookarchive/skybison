/* Wrap void * pointers to be passed between C modules */

#include "Python.h"

/* Internal structure of PyCapsule */
typedef struct {
    PyObject_HEAD
    void *pointer;
    const char *name;
    void *context;
    PyCapsule_Destructor destructor;
} PyCapsule;

typedef struct {
    PyObject *PyCapsule_Type;
} capsulestate;

#define capsulestate(o) ((capsulestate *)PyModule_GetState(o))

#define capsulestate_global ((capsulestate *)PyModule_GetState(PyState_FindModule(&capsulemodule)))

static int capsule_clear(PyObject *module) {
    Py_CLEAR(capsulestate(module)->PyCapsule_Type);
    return 0;
}

static int capsule_traverse(PyObject *module, visitproc visit, void* arg) {
    Py_VISIT(capsulestate(module)->PyCapsule_Type);
    return 0;
}

static void capsule_free(void *module) {
    capsule_clear((PyObject *)module);
}

static struct PyModuleDef capsulemodule = {
    PyModuleDef_HEAD_INIT,
    "_capsule",
    NULL,
    sizeof(capsulestate),
    NULL,
    NULL,
    capsule_traverse,
    capsule_clear,
    capsule_free,
};

static int
_is_legal_capsule(PyCapsule *capsule, const char *invalid_capsule)
{
    if (!capsule || !PyCapsule_CheckExact(capsule) || capsule->pointer == NULL) {
        PyErr_SetString(PyExc_ValueError, invalid_capsule);
        return 0;
    }
    return 1;
}

#define is_legal_capsule(capsule, name) \
    (_is_legal_capsule(capsule, \
     name " called with invalid PyCapsule object"))


static int
name_matches(const char *name1, const char *name2) {
    /* if either is NULL, */
    if (!name1 || !name2) {
        /* they're only the same if they're both NULL. */
        return name1 == name2;
    }
    return !strcmp(name1, name2);
}



PyObject *
PyCapsule_New(void *pointer, const char *name, PyCapsule_Destructor destructor)
{
    PyCapsule *capsule;

    if (!pointer) {
        PyErr_SetString(PyExc_ValueError, "PyCapsule_New called with null pointer");
        return NULL;
    }

    if (PyState_FindModule(&capsulemodule) == NULL) {
        if (PyImport_ImportModule("_capsule") == NULL) {
            return NULL;
        }
    }
    capsule = PyObject_NEW(PyCapsule, (PyTypeObject *)capsulestate_global->PyCapsule_Type);
    if (capsule == NULL) {
        return NULL;
    }

    capsule->pointer = pointer;
    capsule->name = name;
    capsule->context = NULL;
    capsule->destructor = destructor;

    return (PyObject *)capsule;
}


int
PyCapsule_CheckExact_Func(PyObject *op)
{
    return Py_TYPE(op) == (PyTypeObject *)capsulestate_global->PyCapsule_Type;
}


int
PyCapsule_IsValid(PyObject *o, const char *name)
{
    PyCapsule *capsule = (PyCapsule *)o;

    return (capsule != NULL &&
            PyCapsule_CheckExact(capsule) &&
            capsule->pointer != NULL &&
            name_matches(capsule->name, name));
}


void *
PyCapsule_GetPointer(PyObject *o, const char *name)
{
    PyCapsule *capsule = (PyCapsule *)o;

    if (!is_legal_capsule(capsule, "PyCapsule_GetPointer")) {
        return NULL;
    }

    if (!name_matches(name, capsule->name)) {
        PyErr_SetString(PyExc_ValueError, "PyCapsule_GetPointer called with incorrect name");
        return NULL;
    }

    return capsule->pointer;
}


const char *
PyCapsule_GetName(PyObject *o)
{
    PyCapsule *capsule = (PyCapsule *)o;

    if (!is_legal_capsule(capsule, "PyCapsule_GetName")) {
        return NULL;
    }
    return capsule->name;
}


PyCapsule_Destructor
PyCapsule_GetDestructor(PyObject *o)
{
    PyCapsule *capsule = (PyCapsule *)o;

    if (!is_legal_capsule(capsule, "PyCapsule_GetDestructor")) {
        return NULL;
    }
    return capsule->destructor;
}


void *
PyCapsule_GetContext(PyObject *o)
{
    PyCapsule *capsule = (PyCapsule *)o;

    if (!is_legal_capsule(capsule, "PyCapsule_GetContext")) {
        return NULL;
    }
    return capsule->context;
}


int
PyCapsule_SetPointer(PyObject *o, void *pointer)
{
    PyCapsule *capsule = (PyCapsule *)o;

    if (!pointer) {
        PyErr_SetString(PyExc_ValueError, "PyCapsule_SetPointer called with null pointer");
        return -1;
    }

    if (!is_legal_capsule(capsule, "PyCapsule_SetPointer")) {
        return -1;
    }

    capsule->pointer = pointer;
    return 0;
}


int
PyCapsule_SetName(PyObject *o, const char *name)
{
    PyCapsule *capsule = (PyCapsule *)o;

    if (!is_legal_capsule(capsule, "PyCapsule_SetName")) {
        return -1;
    }

    capsule->name = name;
    return 0;
}


int
PyCapsule_SetDestructor(PyObject *o, PyCapsule_Destructor destructor)
{
    PyCapsule *capsule = (PyCapsule *)o;

    if (!is_legal_capsule(capsule, "PyCapsule_SetDestructor")) {
        return -1;
    }

    capsule->destructor = destructor;
    return 0;
}


int
PyCapsule_SetContext(PyObject *o, void *context)
{
    PyCapsule *capsule = (PyCapsule *)o;

    if (!is_legal_capsule(capsule, "PyCapsule_SetContext")) {
        return -1;
    }

    capsule->context = context;
    return 0;
}


void *
PyCapsule_Import(const char *name, int no_block)
{
    PyObject *object = NULL;
    void *return_value = NULL;
    char *trace;
    size_t name_length = (strlen(name) + 1) * sizeof(char);
    char *name_dup = (char *)PyMem_MALLOC(name_length);

    if (!name_dup) {
        return PyErr_NoMemory();
    }

    memcpy(name_dup, name, name_length);

    trace = name_dup;
    while (trace) {
        char *dot = strchr(trace, '.');
        if (dot) {
            *dot++ = '\0';
        }

        if (object == NULL) {
            if (no_block) {
                object = PyImport_ImportModuleNoBlock(trace);
            } else {
                object = PyImport_ImportModule(trace);
                if (!object) {
                    PyErr_Format(PyExc_ImportError, "PyCapsule_Import could not import module \"%s\"", trace);
                }
            }
        } else {
            PyObject *object2 = PyObject_GetAttrString(object, trace);
            Py_DECREF(object);
            object = object2;
        }
        if (!object) {
            goto EXIT;
        }

        trace = dot;
    }

    /* compare attribute name to module.name by hand */
    if (PyCapsule_IsValid(object, name)) {
        PyCapsule *capsule = (PyCapsule *)object;
        return_value = capsule->pointer;
    } else {
        PyErr_Format(PyExc_AttributeError,
            "PyCapsule_Import \"%s\" is not valid",
            name);
    }

EXIT:
    Py_XDECREF(object);
    if (name_dup) {
        PyMem_FREE(name_dup);
    }
    return return_value;
}


static void
capsule_dealloc(PyObject *o)
{
    PyTypeObject *tp = Py_TYPE(o);
    PyCapsule *capsule = (PyCapsule *)o;
    if (capsule->destructor) {
        capsule->destructor(o);
    }
    PyObject_DEL(o);
    Py_DECREF(tp);
}


static PyObject *
capsule_repr(PyObject *o)
{
    PyCapsule *capsule = (PyCapsule *)o;
    const char *name;
    const char *quote;

    if (capsule->name) {
        quote = "\"";
        name = capsule->name;
    } else {
        quote = "";
        name = "NULL";
    }

    return PyUnicode_FromFormat("<capsule object %s%s%s at %p>",
        quote, name, quote, capsule);
}



PyDoc_STRVAR(PyCapsule_Type__doc__,
"Capsule objects let you wrap a C \"void *\" pointer in a Python\n\
object.  They're a way of passing data through the Python interpreter\n\
without creating your own custom type.\n\
\n\
Capsules are used for communication between extension modules.\n\
They provide a way for an extension module to export a C interface\n\
to other extension modules, so that extension modules can use the\n\
Python import mechanism to link to one another.\n\
");

static PyType_Slot PyCapsule_Type_slots[] = {
    {Py_tp_dealloc, capsule_dealloc},
    {Py_tp_repr, capsule_repr},
    {Py_tp_doc, PyCapsule_Type__doc__},
    {0, 0},
};

static PyType_Spec PyCapsule_Type_spec = {
    "PyCapsule",
    sizeof(PyCapsule),
    0,
    Py_TPFLAGS_DEFAULT,
    PyCapsule_Type_slots
};

PyMODINIT_FUNC PyInit__capsule(void) {
    PyObject *mod;
    PyTypeObject *PyCapsule_Type;

    mod = PyState_FindModule(&capsulemodule);
    if (mod != NULL) {
        Py_INCREF(mod);
        return mod;
    }
    mod = PyModule_Create(&capsulemodule);
    if (mod == NULL) {
        return NULL;
    }
    PyCapsule_Type = (PyTypeObject*)PyType_FromSpec(&PyCapsule_Type_spec);
    if (PyCapsule_Type == NULL) {
        return NULL;
    }
    capsulestate(mod)->PyCapsule_Type = (PyObject*)PyCapsule_Type;

    PyState_AddModule(mod, &capsulemodule);
    return mod;
}
