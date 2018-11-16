#ifndef CPYTHON_TYPES_H
#define CPYTHON_TYPES_H

#include <stdio.h> /* For FILE */

#ifdef __cplusplus
extern "C" {
#endif

typedef ssize_t Py_ssize_t;

typedef Py_ssize_t Py_hash_t;

#define _PyObject_HEAD_EXTRA void *reference_;

#define _PyObject_EXTRA_INIT 0,

#define PyObject_HEAD PyObject ob_base;

#define PyObject_VAR_HEAD PyVarObject ob_base;

/* clang-format off */
#define PyObject_HEAD_INIT(type) \
    { _PyObject_EXTRA_INIT \
    1, type },

/* The modification of PyVarObject_HEAD_INIT is described in detail here:
 * https://mail.python.org/pipermail/python-dev/2018-August/154946.html */
#define PyVarObject_HEAD_INIT(type, size) \
    { PyObject_HEAD_INIT(NULL) 0 },
/* clang-format on */

#define Py_TPFLAGS_READY (1UL << 12)
#define Py_TPFLAGS_READYING (1UL << 13)
#define Py_TPFLAGS_HAVE_GC (1UL << 14)

typedef struct _object {
  _PyObject_HEAD_EXTRA Py_ssize_t ob_refcnt;
  struct _typeobject *ob_type;
} PyObject;

typedef struct {
  PyObject ob_base;
  Py_ssize_t ob_size; /* Number of items in variable part */
} PyVarObject;

typedef struct bufferinfo {
  void *buf;
  PyObject *obj; /* owned reference */
  Py_ssize_t len;
  Py_ssize_t itemsize; /* This is Py_ssize_t so it can be
                          pointed to by strides in simple case.*/
  int readonly;
  int ndim;
  char *format;
  Py_ssize_t *shape;
  Py_ssize_t *strides;
  Py_ssize_t *suboffsets;
  void *internal;
} Py_buffer;

struct _typeobject;
typedef void (*freefunc)(void *);
typedef void (*destructor)(PyObject *);
typedef int (*printfunc)(PyObject *, FILE *, int);
typedef PyObject *(*getattrfunc)(PyObject *, char *);
typedef PyObject *(*getattrofunc)(PyObject *, PyObject *);
typedef int (*setattrfunc)(PyObject *, char *, PyObject *);
typedef int (*setattrofunc)(PyObject *, PyObject *, PyObject *);
typedef PyObject *(*reprfunc)(PyObject *);
typedef Py_hash_t (*hashfunc)(PyObject *);
typedef PyObject *(*richcmpfunc)(PyObject *, PyObject *, int);
typedef PyObject *(*getiterfunc)(PyObject *);
typedef PyObject *(*iternextfunc)(PyObject *);
typedef PyObject *(*descrgetfunc)(PyObject *, PyObject *, PyObject *);
typedef int (*descrsetfunc)(PyObject *, PyObject *, PyObject *);
typedef int (*initproc)(PyObject *, PyObject *, PyObject *);
typedef PyObject *(*newfunc)(struct _typeobject *, PyObject *, PyObject *);
typedef PyObject *(*allocfunc)(struct _typeobject *, Py_ssize_t);

typedef PyObject *(*unaryfunc)(PyObject *);
typedef PyObject *(*binaryfunc)(PyObject *, PyObject *);
typedef PyObject *(*ternaryfunc)(PyObject *, PyObject *, PyObject *);
typedef int (*inquiry)(PyObject *);
typedef Py_ssize_t (*lenfunc)(PyObject *);
typedef PyObject *(*ssizeargfunc)(PyObject *, Py_ssize_t);
typedef PyObject *(*ssizessizeargfunc)(PyObject *, Py_ssize_t, Py_ssize_t);
typedef int (*ssizeobjargproc)(PyObject *, Py_ssize_t, PyObject *);
typedef int (*objobjargproc)(PyObject *, PyObject *, PyObject *);

typedef int (*objobjproc)(PyObject *, PyObject *);
typedef int (*visitproc)(PyObject *, void *);
typedef int (*traverseproc)(PyObject *, visitproc, void *);

typedef int (*getbufferproc)(PyObject *, Py_buffer *, int);
typedef void (*releasebufferproc)(PyObject *, Py_buffer *);

typedef PyObject *(*getter)(PyObject *, void *);
typedef int (*setter)(PyObject *, PyObject *, void *);

typedef PyObject *(*PyCFunction)(PyObject *, PyObject *);

typedef struct {
  /* Number implementations must check *both*
     arguments for proper type and implement the necessary conversions
     in the slot functions themselves. */

  binaryfunc nb_add;
  binaryfunc nb_subtract;
  binaryfunc nb_multiply;
  binaryfunc nb_remainder;
  binaryfunc nb_divmod;
  ternaryfunc nb_power;
  unaryfunc nb_negative;
  unaryfunc nb_positive;
  unaryfunc nb_absolute;
  inquiry nb_bool;
  unaryfunc nb_invert;
  binaryfunc nb_lshift;
  binaryfunc nb_rshift;
  binaryfunc nb_and;
  binaryfunc nb_xor;
  binaryfunc nb_or;
  unaryfunc nb_int;
  void *nb_reserved; /* the slot formerly known as nb_long */
  unaryfunc nb_float;

  binaryfunc nb_inplace_add;
  binaryfunc nb_inplace_subtract;
  binaryfunc nb_inplace_multiply;
  binaryfunc nb_inplace_remainder;
  ternaryfunc nb_inplace_power;
  binaryfunc nb_inplace_lshift;
  binaryfunc nb_inplace_rshift;
  binaryfunc nb_inplace_and;
  binaryfunc nb_inplace_xor;
  binaryfunc nb_inplace_or;

  binaryfunc nb_floor_divide;
  binaryfunc nb_true_divide;
  binaryfunc nb_inplace_floor_divide;
  binaryfunc nb_inplace_true_divide;

  unaryfunc nb_index;

  binaryfunc nb_matrix_multiply;
  binaryfunc nb_inplace_matrix_multiply;
} PyNumberMethods;

typedef struct {
  lenfunc sq_length;
  binaryfunc sq_concat;
  ssizeargfunc sq_repeat;
  ssizeargfunc sq_item;
  void *was_sq_slice;
  ssizeobjargproc sq_ass_item;
  void *was_sq_ass_slice;
  objobjproc sq_contains;

  binaryfunc sq_inplace_concat;
  ssizeargfunc sq_inplace_repeat;
} PySequenceMethods;

typedef struct {
  lenfunc mp_length;
  binaryfunc mp_subscript;
  objobjargproc mp_ass_subscript;
} PyMappingMethods;

typedef struct {
  getbufferproc bf_getbuffer;
  releasebufferproc bf_releasebuffer;
} PyBufferProcs;

struct PyMethodDef {
  const char *ml_name; /* The name of the built-in function/method */
  PyCFunction ml_meth; /* The C function that implements it */
  int ml_flags;        /* Combination of METH_xxx flags, which mostly
                          describe the args expected by the C func */
  const char *ml_doc;  /* The __doc__ attribute, or NULL */
};
typedef struct PyMethodDef PyMethodDef;

typedef struct PyModuleDef_Base {
  PyObject_HEAD PyObject *(*m_init)(void);
  Py_ssize_t m_index;
  PyObject *m_copy;
} PyModuleDef_Base;

typedef struct PyModuleDef {
  PyModuleDef_Base m_base;
  const char *m_name;
  const char *m_doc;
  Py_ssize_t m_size;
  PyMethodDef *m_methods;
  struct PyModuleDef_Slot *m_slots;
  traverseproc m_traverse;
  inquiry m_clear;
  freefunc m_free;
} PyModuleDef;

typedef struct PyMemberDef {
  char *name;
  int type;
  Py_ssize_t offset;
  int flags;
  char *doc;
} PyMemberDef;

typedef struct PyGetSetDef {
  char *name;
  getter get;
  setter set;
  char *doc;
  void *closure;
} PyGetSetDef;

typedef struct {
  unaryfunc am_await;
  unaryfunc am_aiter;
  unaryfunc am_anext;
} PyAsyncMethods;

typedef struct _typeobject {
  PyObject_VAR_HEAD const char
      *tp_name; /* For printing, in format "<module>.<name>" */
  Py_ssize_t tp_basicsize, tp_itemsize; /* For allocation */

  /* Methods to implement standard operations */

  destructor tp_dealloc;
  printfunc tp_print;
  getattrfunc tp_getattr;
  setattrfunc tp_setattr;
  PyAsyncMethods *tp_as_async; /* formerly known as tp_compare (Python 2)
                                  or tp_reserved (Python 3) */
  reprfunc tp_repr;

  /* Method suites for standard classes */

  PyNumberMethods *tp_as_number;
  PySequenceMethods *tp_as_sequence;
  PyMappingMethods *tp_as_mapping;

  /* More standard operations (here for binary compatibility) */

  hashfunc tp_hash;
  ternaryfunc tp_call;
  reprfunc tp_str;
  getattrofunc tp_getattro;
  setattrofunc tp_setattro;

  /* Functions to access object as input/output buffer */
  PyBufferProcs *tp_as_buffer;

  /* Flags to define presence of optional/expanded features */
  unsigned long tp_flags;

  const char *tp_doc; /* Documentation string */

  /* Assigned meaning in release 2.0 */
  /* call function for all accessible objects */
  traverseproc tp_traverse;

  /* delete references to contained objects */
  inquiry tp_clear;

  /* Assigned meaning in release 2.1 */
  /* rich comparisons */
  richcmpfunc tp_richcompare;

  /* weak reference enabler */
  Py_ssize_t tp_weaklistoffset;

  /* Iterators */
  getiterfunc tp_iter;
  iternextfunc tp_iternext;

  /* Attribute descriptor and subclassing stuff */
  struct PyMethodDef *tp_methods;
  struct PyMemberDef *tp_members;
  struct PyGetSetDef *tp_getset;
  struct _typeobject *tp_base;
  PyObject *tp_dict;
  descrgetfunc tp_descr_get;
  descrsetfunc tp_descr_set;
  Py_ssize_t tp_dictoffset;
  initproc tp_init;
  allocfunc tp_alloc;
  newfunc tp_new;
  freefunc tp_free; /* Low-level free-memory routine */
  inquiry tp_is_gc; /* For PyObject_IS_GC */
  PyObject *tp_bases;
  PyObject *tp_mro; /* method resolution order */
  PyObject *tp_cache;
  PyObject *tp_subclasses;
  PyObject *tp_weaklist;
  destructor tp_del;

  /* Type attribute cache version tag. Added in version 2.6 */
  unsigned int tp_version_tag;

  destructor tp_finalize;
} PyTypeObject;

typedef struct {
  int cf_flags; /* bitmask of CO_xxx flags relevant to future */
} PyCompilerFlags;

struct _inittab {
  const char *name;
  PyObject *(*initfunc)();
};

#ifdef __cplusplus
}
#endif

#endif /* !CPYTHON_TYPES_H */
