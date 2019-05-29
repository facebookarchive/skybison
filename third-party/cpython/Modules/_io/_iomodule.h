/*
 * Declarations shared between the different parts of the io module
 */

#include "accu.h"
#include "pythread.h"

/* ABCs */
extern PyType_Spec PyIOBase_Type_spec;
extern PyType_Spec PyRawIOBase_Type_spec;
extern PyType_Spec PyBufferedIOBase_Type_spec;
extern PyType_Spec PyTextIOBase_Type_spec;

/* Concrete classes */
extern PyType_Spec PyFileIO_Type_spec;
extern PyType_Spec PyBytesIO_Type_spec;
extern PyType_Spec PyStringIO_Type_spec;
extern PyType_Spec PyBufferedReader_Type_spec;
extern PyType_Spec PyBufferedWriter_Type_spec;
extern PyType_Spec PyBufferedRWPair_Type_spec;
extern PyType_Spec PyBufferedRandom_Type_spec;
extern PyType_Spec PyTextIOWrapper_Type_spec;
extern PyType_Spec PyIncrementalNewlineDecoder_Type_spec;

#ifndef Py_LIMITED_API
#ifdef MS_WINDOWS
#define PyWindowsConsoleIO_Check(op) (PyObject_TypeCheck((op), (PyTypeObject*)IO_MOD_STATE_GLOBAL->PyWindowsConsoleIO_Type))
#endif /* MS_WINDOWS */
#endif /* Py_LIMITED_API */

extern int _PyIO_ConvertSsize_t(PyObject *, void *);

/* These functions are used as METH_NOARGS methods, are normally called
 * with args=NULL, and return a new reference.
 * BUT when args=Py_True is passed, they return a borrowed reference.
 */
extern PyObject* _PyIOBase_check_readable(PyObject *self, PyObject *args);
extern PyObject* _PyIOBase_check_writable(PyObject *self, PyObject *args);
extern PyObject* _PyIOBase_check_seekable(PyObject *self, PyObject *args);
extern PyObject* _PyIOBase_check_closed(PyObject *self, PyObject *args);

/* Helper for finalization.
   This function will revive an object ready to be deallocated and try to
   close() it. It returns 0 if the object can be destroyed, or -1 if it
   is alive again. */
extern int _PyIOBase_finalize(PyObject *self);

/* Returns true if the given FileIO object is closed.
   Doesn't check the argument type, so be careful! */
extern int _PyFileIO_closed(PyObject *self);

/* Shortcut to the core of the IncrementalNewlineDecoder.decode method */
extern PyObject *_PyIncrementalNewlineDecoder_decode(
    PyObject *self, PyObject *input, int final);

/* Finds the first line ending between `start` and `end`.
   If found, returns the index after the line ending and doesn't touch
   `*consumed`.
   If not found, returns -1 and sets `*consumed` to the number of characters
   which can be safely put aside until another search.

   NOTE: for performance reasons, `end` must point to a NUL character ('\0').
   Otherwise, the function will scan further and return garbage.

   There are three modes, in order of priority:
   * translated: Only find \n (assume newlines already translated)
   * universal: Use universal newlines algorithm
   * Otherwise, the line ending is specified by readnl, a str object */
extern Py_ssize_t _PyIO_find_line_ending(
    int translated, int universal, PyObject *readnl,
    int kind, const char *start, const char *end, Py_ssize_t *consumed);

/* Return 1 if an EnvironmentError with errno == EINTR is set (and then
   clears the error indicator), 0 otherwise.
   Should only be called when PyErr_Occurred() is true.
*/
extern int _PyIO_trap_eintr(void);

#define DEFAULT_BUFFER_SIZE (8 * 1024)  /* bytes */

/*
 * Offset type for positioning.
 */

/* Printing a variable of type off_t (with e.g., PyUnicode_FromFormat)
   correctly and without producing compiler warnings is surprisingly painful.
   We identify an integer type whose size matches off_t and then: (1) cast the
   off_t to that integer type and (2) use the appropriate conversion
   specification.  The cast is necessary: gcc complains about formatting a
   long with "%lld" even when both long and long long have the same
   precision. */

#ifdef MS_WINDOWS

/* Windows uses long long for offsets */
typedef long long Py_off_t;
# define PyLong_AsOff_t     PyLong_AsLongLong
# define PyLong_FromOff_t   PyLong_FromLongLong
# define PY_OFF_T_MAX       LLONG_MAX
# define PY_OFF_T_MIN       LLONG_MIN
# define PY_OFF_T_COMPAT    long long    /* type compatible with off_t */
# define PY_PRIdOFF         "lld"        /* format to use for that type */

#else

/* Other platforms use off_t */
typedef off_t Py_off_t;
#if (SIZEOF_OFF_T == SIZEOF_SIZE_T)
# define PyLong_AsOff_t     PyLong_AsSsize_t
# define PyLong_FromOff_t   PyLong_FromSsize_t
# define PY_OFF_T_MAX       PY_SSIZE_T_MAX
# define PY_OFF_T_MIN       PY_SSIZE_T_MIN
# define PY_OFF_T_COMPAT    Py_ssize_t
# define PY_PRIdOFF         "zd"
#elif (SIZEOF_OFF_T == SIZEOF_LONG_LONG)
# define PyLong_AsOff_t     PyLong_AsLongLong
# define PyLong_FromOff_t   PyLong_FromLongLong
# define PY_OFF_T_MAX       LLONG_MAX
# define PY_OFF_T_MIN       LLONG_MIN
# define PY_OFF_T_COMPAT    long long
# define PY_PRIdOFF         "lld"
#elif (SIZEOF_OFF_T == SIZEOF_LONG)
# define PyLong_AsOff_t     PyLong_AsLong
# define PyLong_FromOff_t   PyLong_FromLong
# define PY_OFF_T_MAX       LONG_MAX
# define PY_OFF_T_MIN       LONG_MIN
# define PY_OFF_T_COMPAT    long
# define PY_PRIdOFF         "ld"
#else
# error off_t does not match either size_t, long, or long long!
#endif

#endif

extern Py_off_t PyNumber_AsOff_t(PyObject *item, PyObject *err);

/* Implementation details */

/* IO module structure */

extern PyModuleDef _PyIO_Module;

typedef struct {
    int initialized;
    PyObject *locale_module;
    PyObject *unsupported_operation;
    PyObject *PyIOBase_Type;
    PyObject *PyRawIOBase_Type;
    PyObject *PyBufferedIOBase_Type;
    PyObject *PyTextIOBase_Type;
    PyObject *PyFileIO_Type;
    PyObject *PyBytesIO_Type;
    PyObject *PyStringIO_Type;
#ifdef MS_WINDOWS
    PyObject *PyWindowsConsoleIO_Type;
#endif /* MS_WINDOWS */
    PyObject *PyBufferedReader_Type;
    PyObject *PyBufferedWriter_Type;
    PyObject *PyBufferedRWPair_Type;
    PyObject *PyBufferedRandom_Type;
    PyObject *PyTextIOWrapper_Type;
    PyObject *PyIncrementalNewlineDecoder_Type;
    PyObject *__IOBase_closed;
    PyObject *_blksize;
    PyObject *_dealloc_warn;
    PyObject *_finalizing;
    PyObject *close;
    PyObject *closed;
    PyObject *decode;
    PyObject *empty_bytes;
    PyObject *empty_str;
    PyObject *encode;
    PyObject *extend;
    PyObject *fileno;
    PyObject *flush;
    PyObject *getpreferredencoding;
    PyObject *getstate;
    PyObject *isatty;
    PyObject *mode;
    PyObject *name;
    PyObject *newlines;
    PyObject *nl;
    PyObject *peek;
    PyObject *raw;
    PyObject *read1;
    PyObject *read;
    PyObject *readable;
    PyObject *readall;
    PyObject *readinto1;
    PyObject *readinto;
    PyObject *readline;
    PyObject *replace;
    PyObject *reset;
    PyObject *seek;
    PyObject *seekable;
    PyObject *setstate;
    PyObject *strict;
    PyObject *tell;
    PyObject *truncate;
    PyObject *writable;
    PyObject *write;
    PyObject *zero;
} _PyIO_State;

#define IO_MOD_STATE(mod) ((_PyIO_State *)PyModule_GetState(mod))
#define IO_MOD_STATE_GLOBAL ((_PyIO_State *)PyModule_GetState(PyState_FindModule(&_PyIO_Module)))
#define IO_STATE() _PyIO_get_module_state()

extern _PyIO_State *_PyIO_get_module_state(void);
extern PyObject *_PyIO_get_locale_module(_PyIO_State *);

#ifdef MS_WINDOWS
extern char _PyIO_get_console_type(PyObject *);
#endif

extern PyTypeObject _PyBytesIOBuffer_Type;

/* Only for weakreflist offset and dict offset */
typedef struct {
    PyObject_HEAD
    int fd;
    unsigned int created : 1;
    unsigned int readable : 1;
    unsigned int writable : 1;
    unsigned int appending : 1;
    signed int seekable : 2; /* -1 means unknown */
    unsigned int closefd : 1;
    char finalizing;
    unsigned int blksize;
    PyObject *weakreflist;
    PyObject *dict;
} fileio;

typedef struct {
    PyObject_HEAD

    PyObject *raw;
    int ok;    /* Initialized? */
    int detached;
    int readable;
    int writable;
    char finalizing;

    /* True if this is a vanilla Buffered object (rather than a user derived
       class) *and* the raw stream is a vanilla FileIO object. */
    int fast_closed_checks;

    /* Absolute position inside the raw stream (-1 if unknown). */
    Py_off_t abs_pos;

    /* A static buffer of size `buffer_size` */
    char *buffer;
    /* Current logical position in the buffer. */
    Py_off_t pos;
    /* Position of the raw stream in the buffer. */
    Py_off_t raw_pos;

    /* Just after the last buffered byte in the buffer, or -1 if the buffer
       isn't ready for reading. */
    Py_off_t read_end;

    /* Just after the last byte actually written */
    Py_off_t write_pos;
    /* Just after the last byte waiting to be written, or -1 if the buffer
       isn't ready for writing. */
    Py_off_t write_end;

#ifdef WITH_THREAD
    PyThread_type_lock lock;
    volatile long owner;
#endif

    Py_ssize_t buffer_size;
    Py_ssize_t buffer_mask;

    PyObject *dict;
    PyObject *weakreflist;
} buffered;

/* Only for weakreflist offset and dict offset */
typedef struct {
    PyObject_HEAD
    PyObject *buf;
    Py_ssize_t pos;
    Py_ssize_t string_size;
    PyObject *dict;
    PyObject *weakreflist;
    Py_ssize_t exports;
} bytesio;

typedef struct {
    PyObject_HEAD

    PyObject *dict;
    PyObject *weakreflist;
} iobase;

typedef struct {
    PyObject_HEAD
    buffered *reader;
    buffered *writer;
    PyObject *dict;
    PyObject *weakreflist;
} rwpair;

typedef struct {
    PyObject_HEAD
    Py_UCS4 *buf;
    Py_ssize_t pos;
    Py_ssize_t string_size;
    size_t buf_size;

    /* The stringio object can be in two states: accumulating or realized.
       In accumulating state, the internal buffer contains nothing and
       the contents are given by the embedded _PyAccu structure.
       In realized state, the internal buffer is meaningful and the
       _PyAccu is destroyed.
    */
    int state;
    _PyAccu accu;

    char ok; /* initialized? */
    char closed;
    char readuniversal;
    char readtranslate;
    PyObject *decoder;
    PyObject *readnl;
    PyObject *writenl;

    PyObject *dict;
    PyObject *weakreflist;
} stringio;

typedef PyObject *
        (*encodefunc_t)(PyObject *, PyObject *);

typedef struct
{
    PyObject_HEAD
    int ok; /* initialized? */
    int detached;
    Py_ssize_t chunk_size;
    PyObject *buffer;
    PyObject *encoding;
    PyObject *encoder;
    PyObject *decoder;
    PyObject *readnl;
    PyObject *errors;
    const char *writenl; /* ASCII-encoded; NULL stands for \n */
    char line_buffering;
    char write_through;
    char readuniversal;
    char readtranslate;
    char writetranslate;
    char seekable;
    char has_read1;
    char telling;
    char finalizing;
    /* Specialized encoding func (see below) */
    encodefunc_t encodefunc;
    /* Whether or not it's the start of the stream */
    char encoding_start_of_stream;

    /* Reads and writes are internally buffered in order to speed things up.
       However, any read will first flush the write buffer if itsn't empty.

       Please also note that text to be written is first encoded before being
       buffered. This is necessary so that encoding errors are immediately
       reported to the caller, but it unfortunately means that the
       IncrementalEncoder (whose encode() method is always written in Python)
       becomes a bottleneck for small writes.
    */
    PyObject *decoded_chars;       /* buffer for text returned from decoder */
    Py_ssize_t decoded_chars_used; /* offset into _decoded_chars for read() */
    PyObject *pending_bytes;       /* list of bytes objects waiting to be
                                      written, or NULL */
    Py_ssize_t pending_bytes_count;

    /* snapshot is either NULL, or a tuple (dec_flags, next_input) where
     * dec_flags is the second (integer) item of the decoder state and
     * next_input is the chunk of input bytes that comes next after the
     * snapshot point.  We use this to reconstruct decoder states in tell().
     */
    PyObject *snapshot;
    /* Bytes-to-characters ratio for the current chunk. Serves as input for
       the heuristic in tell(). */
    double b2cratio;

    /* Cache raw object if it's a FileIO object */
    PyObject *raw;

    PyObject *weakreflist;
    PyObject *dict;
} textio;

#ifdef MS_WINDOWS

typedef struct {
    PyObject_HEAD
    HANDLE handle;
    int fd;
    unsigned int created : 1;
    unsigned int readable : 1;
    unsigned int writable : 1;
    unsigned int closehandle : 1;
    char finalizing;
    unsigned int blksize;
    PyObject *weakreflist;
    PyObject *dict;
    char buf[SMALLBUF];
    wchar_t wbuf;
} winconsoleio;

#endif /* MS_WINDOWS */
