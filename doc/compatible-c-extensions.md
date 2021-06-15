# Compatible C Extensions

The Skybison Runtime has a very different object and execution model compared to
CPython. The C-API abstracts most of this. However, some of these internal
implementation details are still leaked. This makes the runtime incompatible
with native CPython modules. Fortunately, there's a CPython compatible way to
solve these issues by following already accepted PEPs. Using this, we'll make
the C Extensions more flexible to other implementations

* **Non-opaque types**: Objects in CPython have to be opaque and used through
  the C-API. Currently, most native modules rely on implementation details of
  several `PyObject`s (i.e. `PyTypeObject`, `PyUnicodeObject`). This
  incompatibility in object layout makes Skybison unable to run an extension
  module.
   * **PEP-384 Solution**: Describes the limited set of object structs that are
     API while leaving the rest as opaque. This PEP allows us to remove all
     implementation details to make modules rely on the C-API only.
* **Process global state**: Native modules have to dynamically create all their
  data and not use the binary's data segment to store state. Currently, most C
  Extensions rely on the use of static data to share state across the module
  itself. This breaks the ability to run multiple runtimes in the same process
  which is part of the execution strategy of Skybison.
   * **PEP-554, PEP-489, and PEP-3121 Solution**: All of these PEPs talk in
     some way or another of Module initialization and state. Following these
     PEPs, we can move all static state into the module instance itself while
     keeping it compatible with the C-API.

Note that the implementation of these PEPs is a specific non-goal for Skybison.
However, we can use these PEPs to achieve our goal of having opaque types and
no data segment state. The only problem is that these PEPs don't have an
implementation strategy. The following outlines a run book of steps to take to
make C Extension modules compliant while still being CPython compatible.

## Runbook:

1. [Create a module state to hold all static data](#create-a-module-state-to)
2. [Modify all the uses of the statics](#modify-all-the-uses-of-s)
3. [Remove static declarations of PyTypeObjects](#remove-static-declaratio)
4. [Fix all the member access of a PyTypeObject](#remove-all-pyobjecttype)
5. [Remove all Py_IDENTIFIERS](#remove-all-py-identifier)
6. [Remove calls to non-Limited-API functions](#remove-calls-to-non-limi)
6. [Replace unicode and bytes internals with Writers](#replace-unicode-and-byte)
7. [Update the PyModuleDef](#update-the-pymoduledef)
8. [Add a module search to PyInit_<module>](#add-a-module-search-to-p)
9. [Initialize all previously static data in PyInit_<module>](#initialize-all-previousl)
10. [Never pull the Module dictionary out](#never-pull-the-module-di)

## No Global Variables

Since a module can be instantiated multiple times by different interpreters,
global variables can not be used. Object references (like `PyObject*`or
`PyTypeObject`) or globals with changing values will not work. Global `const`
variables are not a problem.

Global variable need to be moved into a module state struct as described in the
next sections.

Examples:

```
static const int NUM_XXX = 123;  // ok
static const char* XXX_NAME = "...";   // ok
static PyTypeObject xxx_Type;   // must be moved to module state
static PyObject *yyy;   // must be moved to module state
static int nnn;    // must be moved to module state
```

### Create a module state to hold all static data

The first step is to create a place where all the static data can be held.
Since static data can't be used, we'll rely on the module to provide some
memory space for this. PEP-3121 introduced the module state APIs which we'll be
using to support this.

```
// Put PyModuleDef here

typedef struct {
    PyObject *Foo;
    PyObject *Bar;
} modulestate;
#define modulestate(o) ((modulestate *)PyModule_GetState(o))
#define modulestate_global modulestate(PyState_FindModule(&module))
```

Static data that should be used to module state:

* A `static PyTypeObject foo = {};`
* A top-level `static PyObject* foo;`
* All `_Py_IDENTIFIER`s since they create top level static strings.

### Modify all the uses of statics

Since all the static data was moved out of the global scope, all of these
references need to be updated to now pull the object out of the module state.
This is easily done through the macros defined in the previous point.

```
static void foo_func()
{
    // Before:
    bar_func(Bar);
    // After (use modulestate(mod) if mod is available):
    bar_func(modulestate_global->Bar);
}
```

Look out for calls and directives that may modify the interpreter state,
especially `Py_BEGIN_ALLOW_THREADS`. These can make it impossible for
`modulestate_global` to find the module. To avoid this issue, store the state
locally:

```
static void foo_func()
{
    // Before:
    Py_BEGIN_ALLOW_THREADS
    bar_call(Bar);
    Py_END_ALLOW_THREADS
    // After:
    modulestate *state = modulestate_global;
    Py_BEGIN_ALLOW_THREADS
    bar_call(state->Bar);
    Py_END_ALLOW_THREADS
}
```

### Remove static declarations of PyTypeObject

PEP-384 outlines all the types that are allowed to be static data. This
outlines that `PyTypeObject` should be made into opaque types. These steps will
take any static `PyTypeObject` into a heap allocated `PyTypeObject`.

* Create a `PyType_Spec` and a `PyType_Slot` and migrate all the values from
  the `PyTypeObject`
* Modify the type initialization to use `PyType_FromSpec` instead of
  `PyType_Ready`.
* If the type is inheriting from a parent, use `PyType_FromSpecWithBases`

```
// Before:
static PyTypeObject sometype = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "sometype",                                 /* tp_name */
    sizeof(sometypeobject),                     /* tp_basicsize */
    0,                                          /* tp_itemsize */
    (destruct)sometype_dealloc,                 /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
    0,                                          /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,    /* tp_flags */
    0,                                          /* tp_doc */
};
PyType_Ready(&sometype);

// After:
static PyType_Slot sometype_slots[] = {
    {Py_tp_dealloc, sometype_dealloc},
    {Py_tp_getattro, PyObject_GenericGetAttr},
    {0, 0},
};
static PyType_Spec sometype_spec = {
    "module.sometype",
    sizeof(sometypeobject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    sometype_slots
};
PyObject *sometype = PyType_FromSpec(&sometype_spec);
```

Note in some cases `PyType_Slot` subsumes methods which were previously broken
out into separate structs. For example:

```
// Before:
static PyMappingMethods sometype_as_mapping = {
    (lenfunc)some_tp_len,             /* mp_length */
    (binaryfunc)some_tp_subscript,    /* mp_subscript */
}
static PyTypeObject sometype = {
...
    sometype_as_mapping,  /* tp_as_mapping */
...
}
// After:
static PyType_Slot sometype_slots[] = {
...
    {Py_mp_len, some_tp_len},
    {Py_mp_subscript, some_tp_subscript},
...
}
```

#### Destructors

We are turning a type in a global variable into a type with
`Py_TPFLAGS_HEAPTYPE` set. One of the effects is that instances now own a
reference to the type and `Py_DECREF` must be used on the type in the
`tp_dealloc` implementation.

```
// Before
static void sometype_dealloc(sometypeobject *self) {
    // ... some code freeing things
    Py_TYPE(self)->tp_free(self);
}

// After
static void sometype_dealloc(sometypeobject *self) {
    PyTypeObject *tp = Py_TYPE(self);
    // .. some code freeing things
    ((freefunc)PyType_GetSlot(tp, Py_tp_free))(self);
    Py_DECREF(tp);
}
```
You should also look at the "tp_free" section below!

#### **Non Instantiable Types:**

In CPython, if a static `PyTypeObject` has `tp_new = NULL`, it will make the
type into a non-instantiable type through Python code. However, migrating this
into a heap allocated type will cause it to inherit `tp_new` from its parent.
To avoid this, a `Py_tp_new` has to be defined which immediately throws an
exception.

```
static PyObject *sometype_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    PyErr_Format(PyExc_TypeError, "Cannot create '%.200s' objects", _PyType_Name(type));
    return NULL;
}
```

#### **Non Pickleable Types:**

Pickling has some rules that relies on some of slots of the type objects. Thus,
migrating an object from a static `PyTypeObject` into a heap allocated type
might suddenly make previously un-pickable types into pickable types. The types
affected are those that have a `tp_new = NULL` and a `tp_basicsize == 0`. In
those cases, both `__reduce__` and `__reduce_ex__` have to be defined in
`Py_tp_methods` to immediately throw an exception as well.

```
static PyObject *sometype_reduce(sometypeobject *self) {
    PyErr_Format(PyExc_TypeError, "cannot pickle '%.100s' instances", _PyType_Name(type));
    return NULL;
}
static PyObject *sometype_reduceex(sometypeobject *self, int /* protocol */) {
    return sometype_reduce(self);
}
```
### Remove all PyTypeObject member access

Given that PyTypeObjects are now opaque, all slot access has to be done through
the C-API.

#### **Slot Access:**

`PyTypeObject` is an opaque type in the limited API, so direct accesses to type
slots must be replaced with function calls.

Many slots have corresponding functions that can be used instead. If there is
no function for a slot, then `PyType_GetSlot` can be used to retrieve the value
of the slot.

| Slot | Replacement |
---------------------
| `tp_alloc(type, 0)` | `PyType_GenericNew(type, NULL, NULL)` |
| `tp_as_buffer` | See Buffer Protocol |
| `type->tp_as_async->am_xxx()` | `PyType_GetSlot(type, Py_am_xxx)(...)` |
| `tp_as_mapping->mp_xxx(...)` | `PyMapping_Xxx(...)` |
| `tp_as_number->nb_xxx(...)` | `PyNumber_Xxx(...)` |
| `tp_as_sequence->sq_xxx(...)` | `PySequence_Xxx(...)` |
| `type->tp_dictoffset =` | See Dictoffset and Weaklistoffset
| `type->tp_flags` | `PyType_GetFlags(type)` |
| `type->tp_free(obj)` | `PyType_GetSlot(type, Py_tp_free)(obj)` (see also free) |
| `PyType(obj)->tp_getattro(obj, key)` | `PyObject_GetAttr(obj, key)`
| `PyType(obj)->tp_iter(obj)` | `PyObject_GetIter(obj)` |
| `PyType(obj)->tp_iternext(obj)` | `PyIter_Next(obj)` |
| `PyType(obj)->tp_str(obj)` | `PyObject_Str(obj)` |
| `PyType(obj)->tp_repr(obj)` | `PyObject_Repr(obj)` |
| `PyType(obj)->tp_setattro(obj, key, value)` | `PyObject_SetAttr(obj, key, value)`
| `type->tp_name` | See Type Name
| `type->tp_new(...)` | `PyObject_Call(type, ...)` note that the call also runs `tp_init` if available
| `type->tp_weaklistoffset =` | See Dictoffset and Weaklistoffset


See also the section on destructors above for how to deal with `tp_free`.

#### **Type Name:**

```
// Before:
PyErr_Format(PyExc_TypeError,
  "Did not expect type: %.200s", type->tp_name);

// After: strict PEP-384
PyObject *type_name = PyObject_GetAttrString((PyObject *)type, "__name__");
if (type_name == NULL) {
    PyErr_Clear();
}
PyErr_Format(PyExc_TypeError,
   "Did not expect type: %V", type_name, "?"));
Py_XDECREF(type_name);

// Alternative (this isn't strictly PEP-384 though, we only do this in third-party/cpython but not for external code).
// WARNING: do not use for types where `tp_name` contains a period
PyErr_Format(PyExc_TypeError,
  "Did not expect type: %.200s", _PyType_Name(type));
```

#### tp_free

Generally calls to `tp_free` can be replaced with `PyType_GetSlot`. Example:

```
// Before:
tp->tp_free(self);
// After:
freefunc tp_free = PyType_GetSlot(tp, Py_tp_free);
tp_free(self);
```

If the type does not have `Py_TPFLAGS_BASE` set and cannot have subclasses,
then it might be known upfront whether the tp_free slot contains `PyObject_Del`
or `PyObject_GC_Del` and a direct call to that could be used. Generally this is
not recommended though!

#### **dictoffset and weakrefoffset:**

Currently the heap-allocated type infrastructure in CPython does not allow to
pass in direct values to `tp_dictoffset` and `tp_weaklistoffset` using PEP-384.
Instead, `PyMemberDef` will be used to pass in the right offset value.
`PyType_FromSpec` will read these values and clear the member so that it's not
exposed to the user.

```
// Before:
static PyTypeObject sometype = {
     PyVarObject_HEAD_INIT(&PyType_Type, 0)
     "sometype",                               /* tp_name */
     sizeof(sometypeobject),                   /* tp_basicsize */
     ...
     offsetof(sometypeobject, dict)            /* tp_dictoffset */
     ...
}

// After (same for __weaklistoffset__):
static PyMemberDef sometype_members[] = {
    {"__dictoffset__", T_INT, offsetof(sometypeobject, dict)},
    {NULL}
};
static PyType_Slot sometype_slots[] = {
    {Py_tp_members, sometype_members},
    {0, 0},
};
static PyType_Spec sometype_spec = {
    "module.sometype",
    ...
    sometype_slots
};
```
#### **Buffer Protocol:**

If you run into the need to use a buffer protocol, you will **NOT** be able to
PEP-384 that file as the buffer interface has been omitted from the stable ABI.
See: https://www.python.org/dev/peps/pep-0384/#the-buffer-interface

### Remove all Py_IDENTIFIERS

Python Identifiers use static C strings as part of their implementation.
Therefore, they need to be dealt with in one of two ways:

1. If the function is internal to the module and deals with strings that are
   initialized once and only used for ID comparison, make them static C strings
   (`const char*`, not `_Py_IDENTIFIER`).
2. In other cases, the strings should be heap-allocated and stored in the
   module state.  This also means that all C-API calls that have Python
   Identifiers as part of their arguments, need to be replaced with C-APIs
   using `PyObject*`.

To fix the usage of `_Py_IDENTIFIER`, we first need to remove all the
declarations and move the initialization to the module init and saving it to
the state.

```
// Before:
_Py_IDENTIFIER(str);
some_func(&PyId_str);

// After:
// Inside PyInit_mod
modulestate(m)->str = PyUnicode_InternFromString("str");
// At the original PyId_reason call site
some_func(modulestate_global->str); // Fix to now accept a `PyObject *`
```
If the function using the `PyId_str` is a C-API function, use an appropriate replacement:

* `_PyObject_GetAttrId`: Replace with `PyObject_GetAttr` and module state
* `_PyObject_SetAttrId`: Replace with `PyObject_SetAttr` and module state
* `_PyObject_LookupAttrId`: Replace with `_PyObject_LookupAttr` and module
  state
* `_PyUnicode_FromId`: Remove and use the module state member.
* `_PyObject_CallMethodIdObjArgs`: Replace with `PyObject_CallMethodObjArgs`
  and module state.
* `_PyObject_CallMethodId`:
   * If no `const char *format` is defined, replace with
     `PyObject_CallMethodObjArgs` and module state
   * Otherwise, initialize all `const char *` from the module state and replace
     with `PyObject_CallMethod`.

### Remove calls to non-Limited-API functions

`PyRun_String` can be replaced with:

```
static PyObject* your_friendly_neighborhood_PyRun_String(const char* code, int start, PyObject* globals, PyObject* locals) {
  PyObject* compiled = Py_CompileString(code, "<string>", start);
  if (compiled == NULL) {
    return NULL;
  }
  PyObject* result = PyEval_EvalCode(compiled, globals, locals);
  Py_DECREF(compiled);
  return result;
}
```

### Replace unicode and bytes internals with Writers

Every use case that relies on the internal implementation of bytes or unicode
has to be rewritten. Examples of functions that do so include:

* `PyUnicode_WRITE`
* `PyBytes_FromStringAndSize` when the first argument is a NULL and size > 0
* `PyUnicode_FromStringAndSize` when the first argument is a NULL and size > 0

Most cases simply use these functions to build up a string or bytes object by
appending to them. These can be rewritten using a `_PyBytesWriter` or a
`_PyUnicodeWriter`. These writers allow the construction of a string or a bytes
object through C-APIs rather than directly writing to the internal buffer
pointer.

> NOTE: In order to optimize memory allocation on the stack it is suggested
> that the _PyBytesWriter variable is the last declared variables within the
> function

```
// Before:
PyObject *bytes = PyBytes_FromStringAndSize(NULL, 3);
char *c = PyBytes_AS_STRING(result);
c[0] = "f";
c[1] = "o";
c[2] = "o";

// After:
_PyBytesWriter writer;
_PyBytesWriter_Init(&writer);
char *buf = _PyBytesWriter_Alloc(&writer, 3);
static char *entries = "foo";
buf = _PyBytesWriter_WriteBytes(&writer, buf, *entries, strlen(entry));
PyObject *bytes = _PyBytesWriter_Finish(&writer, buf);
```

However, the cases which write to more than just the end of the string cannot
be replaced by the `Writer` APIs, and must be dealt with on a case by case
basis. Supporting them in Skybison would require mutable strings, and we have
not found a compelling reason to add those yet.

### Update the PyModuleDef

The module definition has to be updated to reflect all the changes that have
been done up to this point. First, add the size of memory that it has to
allocate for the module state. Then, provide the `traverse`, `clear`, and
`free` methods to be able to clear the module when it goes out of scope.

```
// Before:
static struct PyModuleDef module = {
    PyModuleDef_HEAD_INIT,
    "module",
    module_doc,
    -1,
}

// After:
static int module_traverse(PyObject *m, visitproc visit, void *arg);
static int module_clear(PyObject *m);
static void module_free(void *m);
static struct PyModuleDef module_def = {
    PyModuleDef_HEAD_INIT,
    "my_module",
    module_doc,
    sizeof(modulestate),
    NULL,
    NULL,
    module_traverse,
    module_clear,
    module_free,
};

static int module_traverse(PyObject *m, visitproc visit, void *arg) {
    Py_VISIT(modulestate(m)->Foo);
    return 0;
}
static int module_clear(PyObject *m) {
    Py_CLEAR(modulestate(m)->Foo);
    return 0;
}
static void module_free(void *m) {
    module_clear((PyObject *)m);
}
```

### Add a module search to PyInit_<module>

Explained in detail in PEP-554 and PEP-489, modules will be be reinitialized if
called from a sub-interpreters. Thus a  `PyState_FindModule` check should be
added to retrieve the module. This will allow the changes to be fully
compatible with sub-interpreters and it will make it easier to merge upstream.

```
// Before:
PyMODINIT_FUNC PyInit_module(void) {
    PyObject *m = PyModule_Create(&module);
    ...
    return m;
}

// After:
PyMODINIT_FUNC PyInit_module(void) {
    PyObject *m = PyState_FindModule(&timemodule);
    if (m != NULL) {
        Py_INCREF(m);
        return m;
     }
     m = PyModule_Create(&module);
     ...
     return m;
}
```

### Initialize all previously static data in PyInit_<module>

All types and instances that were previously static should now be initialized
in `PyInit_<module>` and stored in the module state. Don't forget to increase
the reference count of the instance is added to the module dictionary using
`PyModule_AddObject` since that C-API steals a reference.

```
// Before:
static PyObject *Foo;
PyMODINIT_FUNC PyInit_module(void) {
    Foo = PyErr_NewException("module.Foo", NULL, NULL);
    PyModule_AddObject(m, "Foo", Foo);
}

// After:
PyMODINIT_FUNC PyInit_module(void) {
    m = PyModule_Create(&module);
    PyObject *Foo = PyErr_NewException("module.Foo", NULL, NULL);
    modulestate(m)->Foo = Foo;
    Py_INCREF(Foo);
    PyModule_AddObject(m, "Foo", Foo);
}
```

### Never use the Module dictionary directly

A module dictionary should never be used directly as that relies on
implementation details of the module. Instead, inserting into the dictionary
should be done through the `PyModule` C-APIs (`PyModule_AddObject`).

```
// Before:
PyMODINIT_FUNC PyInit_module(void) {
    PyObject *m = PyModule_Create(&module);
    PyObject *d = PyModule_GetDict(m);
    PyObject *Foo = ...;
    PyDict_SetItemString(d, "Foo", Foo);
}

// After:
PyMODINIT_FUNC PyInit_module(void) {
    PyObject *m = PyModule_Create(&module);
    PyObject *Foo = ...;
    PyModule_AddObject(m, "Foo", Foo);
}
```
