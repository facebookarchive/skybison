/*
    An implementation of the new I/O lib as defined by PEP 3116 - "New I/O"

    Classes defined here: UnsupportedOperation, BlockingIOError.
    Functions defined here: open().

    Mostly written by Amaury Forgeot d'Arc
*/

#define PY_SSIZE_T_CLEAN
#include "Python.h"
#include "structmember.h"
#include "_iomodule.h"

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif /* HAVE_SYS_STAT_H */

#ifdef MS_WINDOWS
#include <consoleapi.h>
#endif

PyDoc_STRVAR(module_doc,
"The io module provides the Python interfaces to stream handling. The\n"
"builtin open function is defined in this module.\n"
"\n"
"At the top of the I/O hierarchy is the abstract base class IOBase. It\n"
"defines the basic interface to a stream. Note, however, that there is no\n"
"separation between reading and writing to streams; implementations are\n"
"allowed to raise an IOError if they do not support a given operation.\n"
"\n"
"Extending IOBase is RawIOBase which deals simply with the reading and\n"
"writing of raw bytes to a stream. FileIO subclasses RawIOBase to provide\n"
"an interface to OS files.\n"
"\n"
"BufferedIOBase deals with buffering on a raw byte stream (RawIOBase). Its\n"
"subclasses, BufferedWriter, BufferedReader, and BufferedRWPair buffer\n"
"streams that are readable, writable, and both respectively.\n"
"BufferedRandom provides a buffered interface to random access\n"
"streams. BytesIO is a simple stream of in-memory bytes.\n"
"\n"
"Another IOBase subclass, TextIOBase, deals with the encoding and decoding\n"
"of streams into text. TextIOWrapper, which extends it, is a buffered text\n"
"interface to a buffered raw stream (`BufferedIOBase`). Finally, StringIO\n"
"is an in-memory stream for text.\n"
"\n"
"Argument names are not part of the specification, and only the arguments\n"
"of open() are intended to be used as keyword arguments.\n"
"\n"
"data:\n"
"\n"
"DEFAULT_BUFFER_SIZE\n"
"\n"
"   An int containing the default buffer size used by the module's buffered\n"
"   I/O classes. open() uses the file's blksize (as obtained by os.stat) if\n"
"   possible.\n"
    );


/*
 * The main open() function
 */
/*[clinic input]
module _io

_io.open
    file: object
    mode: str = "r"
    buffering: int = -1
    encoding: str(accept={str, NoneType}) = NULL
    errors: str(accept={str, NoneType}) = NULL
    newline: str(accept={str, NoneType}) = NULL
    closefd: int(c_default="1") = True
    opener: object = None

Open file and return a stream.  Raise IOError upon failure.

file is either a text or byte string giving the name (and the path
if the file isn't in the current working directory) of the file to
be opened or an integer file descriptor of the file to be
wrapped. (If a file descriptor is given, it is closed when the
returned I/O object is closed, unless closefd is set to False.)

mode is an optional string that specifies the mode in which the file
is opened. It defaults to 'r' which means open for reading in text
mode.  Other common values are 'w' for writing (truncating the file if
it already exists), 'x' for creating and writing to a new file, and
'a' for appending (which on some Unix systems, means that all writes
append to the end of the file regardless of the current seek position).
In text mode, if encoding is not specified the encoding used is platform
dependent: locale.getpreferredencoding(False) is called to get the
current locale encoding. (For reading and writing raw bytes use binary
mode and leave encoding unspecified.) The available modes are:

========= ===============================================================
Character Meaning
--------- ---------------------------------------------------------------
'r'       open for reading (default)
'w'       open for writing, truncating the file first
'x'       create a new file and open it for writing
'a'       open for writing, appending to the end of the file if it exists
'b'       binary mode
't'       text mode (default)
'+'       open a disk file for updating (reading and writing)
'U'       universal newline mode (deprecated)
========= ===============================================================

The default mode is 'rt' (open for reading text). For binary random
access, the mode 'w+b' opens and truncates the file to 0 bytes, while
'r+b' opens the file without truncation. The 'x' mode implies 'w' and
raises an `FileExistsError` if the file already exists.

Python distinguishes between files opened in binary and text modes,
even when the underlying operating system doesn't. Files opened in
binary mode (appending 'b' to the mode argument) return contents as
bytes objects without any decoding. In text mode (the default, or when
't' is appended to the mode argument), the contents of the file are
returned as strings, the bytes having been first decoded using a
platform-dependent encoding or using the specified encoding if given.

'U' mode is deprecated and will raise an exception in future versions
of Python.  It has no effect in Python 3.  Use newline to control
universal newlines mode.

buffering is an optional integer used to set the buffering policy.
Pass 0 to switch buffering off (only allowed in binary mode), 1 to select
line buffering (only usable in text mode), and an integer > 1 to indicate
the size of a fixed-size chunk buffer.  When no buffering argument is
given, the default buffering policy works as follows:

* Binary files are buffered in fixed-size chunks; the size of the buffer
  is chosen using a heuristic trying to determine the underlying device's
  "block size" and falling back on `io.DEFAULT_BUFFER_SIZE`.
  On many systems, the buffer will typically be 4096 or 8192 bytes long.

* "Interactive" text files (files for which isatty() returns True)
  use line buffering.  Other text files use the policy described above
  for binary files.

encoding is the name of the encoding used to decode or encode the
file. This should only be used in text mode. The default encoding is
platform dependent, but any encoding supported by Python can be
passed.  See the codecs module for the list of supported encodings.

errors is an optional string that specifies how encoding errors are to
be handled---this argument should not be used in binary mode. Pass
'strict' to raise a ValueError exception if there is an encoding error
(the default of None has the same effect), or pass 'ignore' to ignore
errors. (Note that ignoring encoding errors can lead to data loss.)
See the documentation for codecs.register or run 'help(codecs.Codec)'
for a list of the permitted encoding error strings.

newline controls how universal newlines works (it only applies to text
mode). It can be None, '', '\n', '\r', and '\r\n'.  It works as
follows:

* On input, if newline is None, universal newlines mode is
  enabled. Lines in the input can end in '\n', '\r', or '\r\n', and
  these are translated into '\n' before being returned to the
  caller. If it is '', universal newline mode is enabled, but line
  endings are returned to the caller untranslated. If it has any of
  the other legal values, input lines are only terminated by the given
  string, and the line ending is returned to the caller untranslated.

* On output, if newline is None, any '\n' characters written are
  translated to the system default line separator, os.linesep. If
  newline is '' or '\n', no translation takes place. If newline is any
  of the other legal values, any '\n' characters written are translated
  to the given string.

If closefd is False, the underlying file descriptor will be kept open
when the file is closed. This does not work when a file name is given
and must be True in that case.

A custom opener can be used by passing a callable as *opener*. The
underlying file descriptor for the file object is then obtained by
calling *opener* with (*file*, *flags*). *opener* must return an open
file descriptor (passing os.open as *opener* results in functionality
similar to passing None).

open() returns a file object whose type depends on the mode, and
through which the standard file operations such as reading and writing
are performed. When open() is used to open a file in a text mode ('w',
'r', 'wt', 'rt', etc.), it returns a TextIOWrapper. When used to open
a file in a binary mode, the returned class varies: in read binary
mode, it returns a BufferedReader; in write binary and append binary
modes, it returns a BufferedWriter, and in read/write mode, it returns
a BufferedRandom.

It is also possible to use a string or bytearray as a file for both
reading and writing. For strings StringIO can be used like a file
opened in a text mode, and for bytes a BytesIO can be used like a file
opened in a binary mode.
[clinic start generated code]*/

static PyObject *
_io_open_impl(PyObject *module, PyObject *file, const char *mode,
              int buffering, const char *encoding, const char *errors,
              const char *newline, int closefd, PyObject *opener)
/*[clinic end generated code: output=aefafc4ce2b46dc0 input=f4e1ca75223987bc]*/
{
    unsigned i;

    int creating = 0, reading = 0, writing = 0, appending = 0, updating = 0;
    int text = 0, binary = 0, universal = 0;

    char rawmode[6], *m;
    int line_buffering, is_number;
    long isatty;

    PyObject *raw, *modeobj = NULL, *buffer, *wrapper, *result = NULL, *path_or_fd = NULL;

    is_number = PyNumber_Check(file);

    if (is_number) {
        path_or_fd = file;
        Py_INCREF(path_or_fd);
    } else {
        path_or_fd = PyOS_FSPath(file);
        if (path_or_fd == NULL) {
            return NULL;
        }
    }

    if (!is_number &&
        !PyUnicode_Check(path_or_fd) &&
        !PyBytes_Check(path_or_fd)) {
        PyErr_Format(PyExc_TypeError, "invalid file: %R", file);
        goto error;
    }

    /* Decode mode */
    for (i = 0; i < strlen(mode); i++) {
        char c = mode[i];

        switch (c) {
        case 'x':
            creating = 1;
            break;
        case 'r':
            reading = 1;
            break;
        case 'w':
            writing = 1;
            break;
        case 'a':
            appending = 1;
            break;
        case '+':
            updating = 1;
            break;
        case 't':
            text = 1;
            break;
        case 'b':
            binary = 1;
            break;
        case 'U':
            universal = 1;
            reading = 1;
            break;
        default:
            goto invalid_mode;
        }

        /* c must not be duplicated */
        if (strchr(mode+i+1, c)) {
          invalid_mode:
            PyErr_Format(PyExc_ValueError, "invalid mode: '%s'", mode);
            goto error;
        }

    }

    m = rawmode;
    if (creating)  *(m++) = 'x';
    if (reading)   *(m++) = 'r';
    if (writing)   *(m++) = 'w';
    if (appending) *(m++) = 'a';
    if (updating)  *(m++) = '+';
    *m = '\0';

    /* Parameters validation */
    if (universal) {
        if (creating || writing || appending || updating) {
            PyErr_SetString(PyExc_ValueError,
                            "mode U cannot be combined with x', 'w', 'a', or '+'");
            goto error;
        }
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                         "'U' mode is deprecated", 1) < 0)
            goto error;
        reading = 1;
    }

    if (text && binary) {
        PyErr_SetString(PyExc_ValueError,
                        "can't have text and binary mode at once");
        goto error;
    }

    if (creating + reading + writing + appending > 1) {
        PyErr_SetString(PyExc_ValueError,
                        "must have exactly one of create/read/write/append mode");
        goto error;
    }

    if (binary && encoding != NULL) {
        PyErr_SetString(PyExc_ValueError,
                        "binary mode doesn't take an encoding argument");
        goto error;
    }

    if (binary && errors != NULL) {
        PyErr_SetString(PyExc_ValueError,
                        "binary mode doesn't take an errors argument");
        goto error;
    }

    if (binary && newline != NULL) {
        PyErr_SetString(PyExc_ValueError,
                        "binary mode doesn't take a newline argument");
        goto error;
    }

    /* Create the Raw file stream */
    {
        PyObject *RawIO_class = IO_MOD_STATE_GLOBAL->PyFileIO_Type;
#ifdef MS_WINDOWS
        if (!Py_LegacyWindowsStdioFlag && _PyIO_get_console_type(path_or_fd) != '\0') {
            RawIO_class = IO_MOD_STATE_GLOBAL->PyWindowsConsoleIO_Type;
            encoding = "utf-8";
        }
#endif
        raw = PyObject_CallFunction(RawIO_class,
                                    "OsiO", path_or_fd, rawmode, closefd, opener);
    }

    if (raw == NULL)
        goto error;
    result = raw;

    Py_DECREF(path_or_fd);
    path_or_fd = NULL;

    modeobj = PyUnicode_FromString(mode);
    if (modeobj == NULL)
        goto error;

    /* buffering */
    {
        PyObject *res = PyObject_CallMethodObjArgs(raw, IO_MOD_STATE_GLOBAL->isatty, NULL);
        if (res == NULL)
            goto error;
        isatty = PyLong_AsLong(res);
        Py_DECREF(res);
        if (isatty == -1 && PyErr_Occurred())
            goto error;
    }

    if (buffering == 1 || (buffering < 0 && isatty)) {
        buffering = -1;
        line_buffering = 1;
    }
    else
        line_buffering = 0;

    if (buffering < 0) {
        PyObject *blksize_obj;
        blksize_obj = PyObject_GetAttr(raw, IO_MOD_STATE_GLOBAL->_blksize);
        if (blksize_obj == NULL)
            goto error;
        buffering = PyLong_AsLong(blksize_obj);
        Py_DECREF(blksize_obj);
        if (buffering == -1 && PyErr_Occurred())
            goto error;
    }
    if (buffering < 0) {
        PyErr_SetString(PyExc_ValueError,
                        "invalid buffering size");
        goto error;
    }

    /* if not buffering, returns the raw file object */
    if (buffering == 0) {
        if (!binary) {
            PyErr_SetString(PyExc_ValueError,
                            "can't have unbuffered text I/O");
            goto error;
        }

        Py_DECREF(modeobj);
        return result;
    }

    /* wraps into a buffered file */
    {
        PyObject *Buffered_class;

        if (updating)
            Buffered_class = IO_MOD_STATE_GLOBAL->PyBufferedRandom_Type;
        else if (creating || writing || appending)
            Buffered_class = IO_MOD_STATE_GLOBAL->PyBufferedWriter_Type;
        else if (reading)
            Buffered_class = IO_MOD_STATE_GLOBAL->PyBufferedReader_Type;
        else {
            PyErr_Format(PyExc_ValueError,
                         "unknown mode: '%s'", mode);
            goto error;
        }

        buffer = PyObject_CallFunction(Buffered_class, "Oi", raw, buffering);
    }
    if (buffer == NULL)
        goto error;
    result = buffer;
    Py_DECREF(raw);


    /* if binary, returns the buffered file */
    if (binary) {
        Py_DECREF(modeobj);
        return result;
    }

    /* wraps into a TextIOWrapper */
    wrapper = PyObject_CallFunction(IO_MOD_STATE_GLOBAL->PyTextIOWrapper_Type,
                                    "Osssi",
                                    buffer,
                                    encoding, errors, newline,
                                    line_buffering);
    if (wrapper == NULL)
        goto error;
    result = wrapper;
    Py_DECREF(buffer);

    if (PyObject_SetAttr(wrapper, IO_MOD_STATE_GLOBAL->mode, modeobj) < 0)
        goto error;
    Py_DECREF(modeobj);
    return result;

  error:
    if (result != NULL) {
        PyObject *exc, *val, *tb, *close_result;
        PyErr_Fetch(&exc, &val, &tb);
        close_result = PyObject_CallMethodObjArgs(result, IO_MOD_STATE_GLOBAL->close, NULL);
        _PyErr_ChainExceptions(exc, val, tb);
        Py_XDECREF(close_result);
        Py_DECREF(result);
    }
    Py_XDECREF(path_or_fd);
    Py_XDECREF(modeobj);
    return NULL;
}

/*
 * Private helpers for the io module.
 */

Py_off_t
PyNumber_AsOff_t(PyObject *item, PyObject *err)
{
    Py_off_t result;
    PyObject *runerr;
    PyObject *value = PyNumber_Index(item);
    if (value == NULL)
        return -1;

    /* We're done if PyLong_AsSsize_t() returns without error. */
    result = PyLong_AsOff_t(value);
    if (result != -1 || !(runerr = PyErr_Occurred()))
        goto finish;

    /* Error handling code -- only manage OverflowError differently */
    if (!PyErr_GivenExceptionMatches(runerr, PyExc_OverflowError))
        goto finish;

    PyErr_Clear();
    /* If no error-handling desired then the default clipping
       is sufficient.
     */
    if (!err) {
        assert(PyLong_Check(value));
        /* Whether or not it is less than or equal to
           zero is determined by the sign of ob_size
        */
        if (_PyLong_Sign(value) < 0)
            result = PY_OFF_T_MIN;
        else
            result = PY_OFF_T_MAX;
    }
    else {
        /* Otherwise replace the error with caller's error object. */
        PyErr_Format(err,
                     "cannot fit '%.200s' into an offset-sized integer",
                     item->ob_type->tp_name);
    }

 finish:
    Py_DECREF(value);
    return result;
}


/* Basically the "n" format code with the ability to turn None into -1. */
int
_PyIO_ConvertSsize_t(PyObject *obj, void *result) {
    Py_ssize_t limit;
    if (obj == Py_None) {
        limit = -1;
    }
    else if (PyNumber_Check(obj)) {
        limit = PyNumber_AsSsize_t(obj, PyExc_OverflowError);
        if (limit == -1 && PyErr_Occurred())
            return 0;
    }
    else {
        PyErr_Format(PyExc_TypeError,
                     "integer argument expected, got '%.200s'",
                     _PyType_Name(Py_TYPE(obj)));
        return 0;
    }
    *((Py_ssize_t *)result) = limit;
    return 1;
}


_PyIO_State *
_PyIO_get_module_state(void)
{
    PyObject *mod = PyState_FindModule(&_PyIO_Module);
    _PyIO_State *state;
    if (mod == NULL || (state = IO_MOD_STATE(mod)) == NULL) {
        PyErr_SetString(PyExc_RuntimeError,
                        "could not find io module state "
                        "(interpreter shutdown?)");
        return NULL;
    }
    return state;
}

PyObject *
_PyIO_get_locale_module(_PyIO_State *state)
{
    PyObject *mod;
    if (state->locale_module != NULL) {
        assert(PyWeakref_CheckRef(state->locale_module));
        mod = PyWeakref_GET_OBJECT(state->locale_module);
        if (mod != Py_None) {
            Py_INCREF(mod);
            return mod;
        }
        Py_CLEAR(state->locale_module);
    }
    mod = PyImport_ImportModule("_bootlocale");
    if (mod == NULL)
        return NULL;
    state->locale_module = PyWeakref_NewRef(mod, NULL);
    if (state->locale_module == NULL) {
        Py_DECREF(mod);
        return NULL;
    }
    return mod;
}


static int
iomodule_traverse(PyObject *mod, visitproc visit, void *arg) {
    _PyIO_State *state = IO_MOD_STATE(mod);
    if (!state->initialized)
        return 0;
    if (state->locale_module != NULL) {
        Py_VISIT(state->locale_module);
    }
    Py_VISIT(state->unsupported_operation);
    Py_VISIT(state->PyIOBase_Type);
    Py_VISIT(state->PyRawIOBase_Type);
    Py_VISIT(state->PyBufferedIOBase_Type);
    Py_VISIT(state->PyTextIOBase_Type);
    Py_VISIT(state->PyFileIO_Type);
    Py_VISIT(state->PyBytesIO_Type);
    Py_VISIT(state->PyStringIO_Type);
#ifdef MS_WINDOWS
    Py_VISIT(state->PyWindowsConsoleIO_Type);
#endif /* MS_WINDOWS */
    Py_VISIT(state->PyBufferedReader_Type);
    Py_VISIT(state->PyBufferedWriter_Type);
    Py_VISIT(state->PyBufferedRWPair_Type);
    Py_VISIT(state->PyBufferedRandom_Type);
    Py_VISIT(state->PyTextIOWrapper_Type);
    Py_VISIT(state->PyIncrementalNewlineDecoder_Type);
    Py_VISIT(state->__IOBase_closed);
    Py_VISIT(state->_blksize);
    Py_VISIT(state->_dealloc_warn);
    Py_VISIT(state->_finalizing);
    Py_VISIT(state->close);
    Py_VISIT(state->closed);
    Py_VISIT(state->decode);
    Py_VISIT(state->empty_bytes);
    Py_VISIT(state->empty_str);
    Py_VISIT(state->encode);
    Py_VISIT(state->extend);
    Py_VISIT(state->fileno);
    Py_VISIT(state->flush);
    Py_VISIT(state->getpreferredencoding);
    Py_VISIT(state->getstate);
    Py_VISIT(state->isatty);
    Py_VISIT(state->mode);
    Py_VISIT(state->name);
    Py_VISIT(state->newlines);
    Py_VISIT(state->nl);
    Py_VISIT(state->peek);
    Py_VISIT(state->raw);
    Py_VISIT(state->read1);
    Py_VISIT(state->read);
    Py_VISIT(state->readable);
    Py_VISIT(state->readall);
    Py_VISIT(state->readinto1);
    Py_VISIT(state->readinto);
    Py_VISIT(state->readline);
    Py_VISIT(state->replace);
    Py_VISIT(state->reset);
    Py_VISIT(state->seek);
    Py_VISIT(state->seekable);
    Py_VISIT(state->setstate);
    Py_VISIT(state->strict);
    Py_VISIT(state->tell);
    Py_VISIT(state->truncate);
    Py_VISIT(state->writable);
    Py_VISIT(state->write);
    Py_VISIT(state->zero);
    return 0;
}


static int
iomodule_clear(PyObject *mod) {
    _PyIO_State *state = IO_MOD_STATE(mod);
    if (!state->initialized)
        return 0;
    if (state->locale_module != NULL)
        Py_CLEAR(state->locale_module);
    Py_CLEAR(state->unsupported_operation);
    Py_CLEAR(state->PyIOBase_Type);
    Py_CLEAR(state->PyRawIOBase_Type);
    Py_CLEAR(state->PyBufferedIOBase_Type);
    Py_CLEAR(state->PyTextIOBase_Type);
    Py_CLEAR(state->PyFileIO_Type);
    Py_CLEAR(state->PyBytesIO_Type);
    Py_CLEAR(state->PyStringIO_Type);
#ifdef MS_WINDOWS
    Py_CLEAR(state->PyWindowsConsoleIO_Type);
#endif /* MS_WINDOWS */
    Py_CLEAR(state->PyBufferedReader_Type);
    Py_CLEAR(state->PyBufferedWriter_Type);
    Py_CLEAR(state->PyBufferedRWPair_Type);
    Py_CLEAR(state->PyBufferedRandom_Type);
    Py_CLEAR(state->PyTextIOWrapper_Type);
    Py_CLEAR(state->PyIncrementalNewlineDecoder_Type);
    Py_CLEAR(state->__IOBase_closed);
    Py_CLEAR(state->_blksize);
    Py_CLEAR(state->_dealloc_warn);
    Py_CLEAR(state->_finalizing);
    Py_CLEAR(state->close);
    Py_CLEAR(state->closed);
    Py_CLEAR(state->decode);
    Py_CLEAR(state->empty_bytes);
    Py_CLEAR(state->empty_str);
    Py_CLEAR(state->encode);
    Py_CLEAR(state->extend);
    Py_CLEAR(state->fileno);
    Py_CLEAR(state->flush);
    Py_CLEAR(state->getpreferredencoding);
    Py_CLEAR(state->getstate);
    Py_CLEAR(state->isatty);
    Py_CLEAR(state->mode);
    Py_CLEAR(state->name);
    Py_CLEAR(state->newlines);
    Py_CLEAR(state->nl);
    Py_CLEAR(state->peek);
    Py_CLEAR(state->raw);
    Py_CLEAR(state->read1);
    Py_CLEAR(state->read);
    Py_CLEAR(state->readable);
    Py_CLEAR(state->readall);
    Py_CLEAR(state->readinto1);
    Py_CLEAR(state->readinto);
    Py_CLEAR(state->readline);
    Py_CLEAR(state->replace);
    Py_CLEAR(state->reset);
    Py_CLEAR(state->seek);
    Py_CLEAR(state->seekable);
    Py_CLEAR(state->setstate);
    Py_CLEAR(state->strict);
    Py_CLEAR(state->tell);
    Py_CLEAR(state->truncate);
    Py_CLEAR(state->writable);
    Py_CLEAR(state->write);
    Py_CLEAR(state->zero);
    return 0;
}

static void
iomodule_free(PyObject *mod) {
    iomodule_clear(mod);
}


/*
 * Module definition
 */

#include "clinic/_iomodule.c.h"

static PyMethodDef module_methods[] = {
    _IO_OPEN_METHODDEF
    {NULL, NULL}
};

struct PyModuleDef _PyIO_Module = {
    PyModuleDef_HEAD_INIT,
    "io",
    module_doc,
    sizeof(_PyIO_State),
    module_methods,
    NULL,
    iomodule_traverse,
    iomodule_clear,
    (freefunc)iomodule_free,
};

PyMODINIT_FUNC
PyInit__io(void)
{
    PyObject* bases;
    PyObject *m = PyState_FindModule(&_PyIO_Module);
    if (m != NULL)
        return m;

    m = PyModule_Create(&_PyIO_Module);
    _PyIO_State *state = NULL;
    if (m == NULL)
        return NULL;
    state = IO_MOD_STATE(m);
    state->initialized = 0;

    /* Initialize module strings */
    if ((state->__IOBase_closed = PyUnicode_FromString("__IOBase_closed")) == NULL)
        goto fail;
    if ((state->_blksize = PyUnicode_FromString("_blksize")) == NULL)
        goto fail;
    if ((state->_dealloc_warn = PyUnicode_FromString("_dealloc_warn")) == NULL)
        goto fail;
    if ((state->_finalizing = PyUnicode_FromString("_finalizing")) == NULL)
        goto fail;
    if ((state->close = PyUnicode_FromString("close")) == NULL)
        goto fail;
    if ((state->closed = PyUnicode_FromString("closed")) == NULL)
        goto fail;
    if ((state->decode = PyUnicode_FromString("decode")) == NULL)
        goto fail;
    if ((state->empty_str = PyUnicode_FromStringAndSize(NULL, 0)) == NULL)
        goto fail;
    if ((state->empty_bytes = PyBytes_FromStringAndSize(NULL, 0)) == NULL)
        goto fail;
    if ((state->encode = PyUnicode_FromString("encode")) == NULL)
        goto fail;
    if ((state->extend = PyUnicode_FromString("extend")) == NULL)
        goto fail;
    if ((state->fileno = PyUnicode_FromString("fileno")) == NULL)
        goto fail;
    if ((state->flush = PyUnicode_FromString("flush")) == NULL)
        goto fail;
    if ((state->getpreferredencoding = PyUnicode_FromString("getpreferredencoding")) == NULL)
        goto fail;
    if ((state->getstate = PyUnicode_FromString("getstate")) == NULL)
        goto fail;
    if ((state->isatty = PyUnicode_FromString("isatty")) == NULL)
        goto fail;
    if ((state->mode = PyUnicode_FromString("mode")) == NULL)
        goto fail;
    if ((state->name = PyUnicode_FromString("name")) == NULL)
        goto fail;
    if ((state->newlines = PyUnicode_FromString("newlines")) == NULL)
        goto fail;
    if ((state->nl = PyUnicode_FromString("\n")) == NULL)
        goto fail;
    if ((state->peek = PyUnicode_FromString("peek")) == NULL)
        goto fail;
    if ((state->raw = PyUnicode_FromString("raw")) == NULL)
        goto fail;
    if ((state->read1 = PyUnicode_FromString("read1")) == NULL)
        goto fail;
    if ((state->read = PyUnicode_FromString("read")) == NULL)
        goto fail;
    if ((state->readable = PyUnicode_FromString("readable")) == NULL)
        goto fail;
    if ((state->readall = PyUnicode_FromString("readall")) == NULL)
        goto fail;
    if ((state->readinto1 = PyUnicode_FromString("readinto1")) == NULL)
        goto fail;
    if ((state->readinto = PyUnicode_FromString("readinto")) == NULL)
        goto fail;
    if ((state->readline = PyUnicode_FromString("readline")) == NULL)
        goto fail;
    if ((state->replace = PyUnicode_FromString("replace")) == NULL)
        goto fail;
    if ((state->reset = PyUnicode_FromString("reset")) == NULL)
        goto fail;
    if ((state->seek = PyUnicode_FromString("seek")) == NULL)
        goto fail;
    if ((state->seekable = PyUnicode_FromString("seekable")) == NULL)
        goto fail;
    if ((state->setstate = PyUnicode_FromString("setstate")) == NULL)
        goto fail;
    if ((state->strict = PyUnicode_FromString("strict")) == NULL)
        goto fail;
    if ((state->tell = PyUnicode_FromString("tell")) == NULL)
        goto fail;
    if ((state->truncate = PyUnicode_FromString("truncate")) == NULL)
        goto fail;
    if ((state->write = PyUnicode_FromString("write")) == NULL)
        goto fail;
    if ((state->writable = PyUnicode_FromString("writable")) == NULL)
        goto fail;
    if ((state->zero = PyLong_FromLong(0L)) == NULL)
        goto fail;

#define ADD_TYPE_SPEC(type, spec, name) \
    PyTypeObject *type = (PyTypeObject *)PyType_FromSpec(&spec); \
    if (type == NULL) \
        goto fail; \
    Py_INCREF(type); \
    if (PyModule_AddObject(m, name, (PyObject *)type) < 0) {  \
        Py_DECREF(type); \
        goto fail; \
    } \
    state->type = (PyObject *)type; \
    Py_INCREF(state->type);

#define ADD_TYPE_SPEC_WITH_BASES(type, spec, name, bases) \
    if (bases == NULL) { \
        goto fail; \
    } \
    PyTypeObject *type = (PyTypeObject *)PyType_FromSpecWithBases(&spec, bases); \
    Py_DECREF(bases); \
    if (type == NULL) \
        goto fail; \
    Py_INCREF(type); \
    if (PyModule_AddObject(m, name, (PyObject *)type) < 0) {  \
        Py_DECREF(type); \
        goto fail; \
    } \
    state->type = (PyObject *)type; \
    Py_INCREF(state->type);

    /* DEFAULT_BUFFER_SIZE */
    if (PyModule_AddIntMacro(m, DEFAULT_BUFFER_SIZE) < 0)
        goto fail;

    /* UnsupportedOperation inherits from ValueError and IOError */
    state->unsupported_operation = PyObject_CallFunction(
        (PyObject *)&PyType_Type, "s(OO){}",
        "UnsupportedOperation", PyExc_OSError, PyExc_ValueError);
    if (state->unsupported_operation == NULL)
        goto fail;
    Py_INCREF(state->unsupported_operation);
    if (PyModule_AddObject(m, "UnsupportedOperation",
                           state->unsupported_operation) < 0)
        goto fail;

    /* BlockingIOError, for compatibility */
    Py_INCREF(PyExc_BlockingIOError);
    if (PyModule_AddObject(m, "BlockingIOError",
                           (PyObject *) PyExc_BlockingIOError) < 0)
        goto fail;

    /* Concrete base types of the IO ABCs.
       (the ABCs themselves are declared through inheritance in io.py)
    */
    /* Facebook: Fix the types that are setting weaklist and dict offset */
    ADD_TYPE_SPEC(PyIOBase_Type, PyIOBase_Type_spec, "_IOBase");
    ((PyTypeObject *)state->PyIOBase_Type)->tp_weaklistoffset = offsetof(iobase, weakreflist);
    bases = PyTuple_Pack(1, state->PyIOBase_Type);
    ADD_TYPE_SPEC_WITH_BASES(
        PyRawIOBase_Type, PyRawIOBase_Type_spec, "_RawIOBase", bases);
    bases = PyTuple_Pack(1, state->PyIOBase_Type);
    ADD_TYPE_SPEC_WITH_BASES(
        PyBufferedIOBase_Type, PyBufferedIOBase_Type_spec, "_BufferedIOBase", bases);
    bases = PyTuple_Pack(1, state->PyIOBase_Type);
    ADD_TYPE_SPEC_WITH_BASES(
        PyTextIOBase_Type, PyTextIOBase_Type_spec, "_TextIOBase", bases);

    /* Implementation of concrete IO objects. */
    /* FileIO */
    bases = PyTuple_Pack(1, state->PyRawIOBase_Type);
    ADD_TYPE_SPEC_WITH_BASES(
        PyFileIO_Type, PyFileIO_Type_spec, "FileIO", bases);
    ((PyTypeObject *)state->PyFileIO_Type)->tp_weaklistoffset = offsetof(fileio, weakreflist);

    /* BytesIO */
    bases = PyTuple_Pack(1, state->PyBufferedIOBase_Type);
    ADD_TYPE_SPEC_WITH_BASES(
        PyBytesIO_Type, PyBytesIO_Type_spec, "BytesIO", bases);
    ((PyTypeObject *)state->PyBytesIO_Type)->tp_weaklistoffset = offsetof(bytesio, weakreflist);
    if (PyType_Ready(&_PyBytesIOBuffer_Type) < 0)
        goto fail;

    /* StringIO */
    bases = PyTuple_Pack(1, state->PyTextIOBase_Type);
    ADD_TYPE_SPEC_WITH_BASES(
        PyStringIO_Type, PyStringIO_Type_spec, "StringIO", bases);
    ((PyTypeObject *)state->PyStringIO_Type)->tp_weaklistoffset = offsetof(stringio, weakreflist);

#ifdef MS_WINDOWS
    /* WindowsConsoleIO */
    bases = PyTuple_Pack(1, state->PyRawIOBase_Type);
    ADD_TYPE_SPEC_WITH_BASES(
        PyWindowsConsoleIO_Type, _WindowsConsoleIO_Type_spec, "_WindowsConsoleIO", bases);
    ((PyTypeObject *)state->PyWindowsConsoleIO_Type)->tp_weaklistoffset = offsetof(winconsoleio, weakreflist);
#endif

    /* BufferedReader */
    bases = PyTuple_Pack(1, state->PyBufferedIOBase_Type);
    ADD_TYPE_SPEC_WITH_BASES(
        PyBufferedReader_Type, PyBufferedReader_Type_spec, "BufferedReader", bases);
    ((PyTypeObject *)state->PyBufferedReader_Type)->tp_weaklistoffset = offsetof(buffered, weakreflist);

    /* BufferedWriter */
    bases = PyTuple_Pack(1, state->PyBufferedIOBase_Type);
    ADD_TYPE_SPEC_WITH_BASES(
        PyBufferedWriter_Type, PyBufferedWriter_Type_spec, "BufferedWriter", bases);
    ((PyTypeObject *)state->PyBufferedWriter_Type)->tp_weaklistoffset = offsetof(buffered, weakreflist);

    /* BufferedRWPair */
    bases = PyTuple_Pack(1, state->PyBufferedIOBase_Type);
    ADD_TYPE_SPEC_WITH_BASES(
        PyBufferedRWPair_Type, PyBufferedRWPair_Type_spec, "BufferedRWPair", bases);
    ((PyTypeObject *)state->PyBufferedRWPair_Type)->tp_weaklistoffset = offsetof(rwpair, weakreflist);

    /* BufferedRandom */
    bases = PyTuple_Pack(1, state->PyBufferedIOBase_Type);
    ADD_TYPE_SPEC_WITH_BASES(
        PyBufferedRandom_Type, PyBufferedRandom_Type_spec, "BufferedRandom", bases);
    ((PyTypeObject *)state->PyBufferedRandom_Type)->tp_weaklistoffset = offsetof(buffered, weakreflist);

    /* TextIOWrapper */
    bases = PyTuple_Pack(1, state->PyTextIOBase_Type);
    ADD_TYPE_SPEC_WITH_BASES(
        PyTextIOWrapper_Type, PyTextIOWrapper_Type_spec, "TextIOWrapper", bases);
    ((PyTypeObject *)state->PyTextIOWrapper_Type)->tp_weaklistoffset = offsetof(textio, weakreflist);

    /* IncrementalNewlineDecoder */
    ADD_TYPE_SPEC(
        PyIncrementalNewlineDecoder_Type, PyIncrementalNewlineDecoder_Type_spec, "IncrementalNewlineDecoder");

    state->initialized = 1;
    PyState_AddModule(m, &_PyIO_Module);
    return m;

  fail:
    Py_XDECREF(state->unsupported_operation);
    Py_XDECREF(state->PyIOBase_Type);
    Py_XDECREF(state->PyRawIOBase_Type);
    Py_XDECREF(state->PyBufferedIOBase_Type);
    Py_XDECREF(state->PyTextIOBase_Type);
    Py_XDECREF(state->PyFileIO_Type);
    Py_XDECREF(state->PyBytesIO_Type);
    Py_XDECREF(state->PyStringIO_Type);
#ifdef MS_WINDOWS
    Py_XDECREF(state->PyWindowsConsoleIO_Type);
#endif /* MS_WINDOWS */
    Py_XDECREF(state->PyBufferedReader_Type);
    Py_XDECREF(state->PyBufferedWriter_Type);
    Py_XDECREF(state->PyBufferedRWPair_Type);
    Py_XDECREF(state->PyBufferedRandom_Type);
    Py_XDECREF(state->PyTextIOWrapper_Type);
    Py_XDECREF(state->PyIncrementalNewlineDecoder_Type);
    Py_XDECREF(state->__IOBase_closed);
    Py_XDECREF(state->_blksize);
    Py_XDECREF(state->_dealloc_warn);
    Py_XDECREF(state->_finalizing);
    Py_XDECREF(state->close);
    Py_XDECREF(state->closed);
    Py_XDECREF(state->decode);
    Py_XDECREF(state->empty_bytes);
    Py_XDECREF(state->empty_str);
    Py_XDECREF(state->encode);
    Py_XDECREF(state->extend);
    Py_XDECREF(state->fileno);
    Py_XDECREF(state->flush);
    Py_XDECREF(state->getpreferredencoding);
    Py_XDECREF(state->getstate);
    Py_XDECREF(state->isatty);
    Py_XDECREF(state->mode);
    Py_XDECREF(state->name);
    Py_XDECREF(state->newlines);
    Py_XDECREF(state->nl);
    Py_XDECREF(state->peek);
    Py_XDECREF(state->raw);
    Py_XDECREF(state->read1);
    Py_XDECREF(state->read);
    Py_XDECREF(state->readable);
    Py_XDECREF(state->readall);
    Py_XDECREF(state->readinto1);
    Py_XDECREF(state->readinto);
    Py_XDECREF(state->readline);
    Py_XDECREF(state->replace);
    Py_XDECREF(state->reset);
    Py_XDECREF(state->seek);
    Py_XDECREF(state->seekable);
    Py_XDECREF(state->setstate);
    Py_XDECREF(state->strict);
    Py_XDECREF(state->tell);
    Py_XDECREF(state->truncate);
    Py_XDECREF(state->writable);
    Py_XDECREF(state->write);
    Py_XDECREF(state->zero);
    Py_DECREF(m);
    return NULL;
}
