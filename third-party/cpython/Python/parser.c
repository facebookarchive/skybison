#include "Python.h"

#include "Python-ast.h"
#undef Yield /* undefine macro conflicting with winbase.h */
#include "grammar.h"
#include "node.h"
#include "token.h"
#include "parsetok.h"
#include "errcode.h"
#include "code.h"
#include "symtable.h"
#include "ast.h"

#ifdef __cplusplus
extern "C" {
#endif

extern grammar _PyParser_Grammar; /* From graminit.c */

static void err_input(perrdetail *);
static void err_free(perrdetail *);

/* compute parser flags based on compiler flags */
static int PARSER_FLAGS(PyCompilerFlags *flags)
{
    int parser_flags = 0;
    if (!flags)
        return 0;
    if (flags->cf_flags & PyCF_DONT_IMPLY_DEDENT)
        parser_flags |= PyPARSE_DONT_IMPLY_DEDENT;
    if (flags->cf_flags & PyCF_IGNORE_COOKIE)
        parser_flags |= PyPARSE_IGNORE_COOKIE;
    if (flags->cf_flags & CO_FUTURE_BARRY_AS_BDFL)
        parser_flags |= PyPARSE_BARRY_AS_BDFL;
    return parser_flags;
}

#if 0
/* Keep an example of flags with future keyword support. */
#define PARSER_FLAGS(flags) \
    ((flags) ? ((((flags)->cf_flags & PyCF_DONT_IMPLY_DEDENT) ? \
                  PyPARSE_DONT_IMPLY_DEDENT : 0) \
                | ((flags)->cf_flags & CO_FUTURE_WITH_STATEMENT ? \
                   PyPARSE_WITH_IS_KEYWORD : 0)) : 0)
#endif

struct symtable *
Py_SymtableStringObject(const char *str, PyObject *filename, int start)
{
    struct symtable *st;
    mod_ty mod;
    PyCompilerFlags flags;
    PyArena *arena;

    arena = PyArena_New();
    if (arena == NULL)
        return NULL;

    flags.cf_flags = 0;
    mod = PyParser_ASTFromStringObject(str, filename, start, &flags, arena);
    if (mod == NULL) {
        PyArena_Free(arena);
        return NULL;
    }
    st = PySymtable_BuildObject(mod, filename, 0);
    PyArena_Free(arena);
    return st;
}

struct symtable *
Py_SymtableString(const char *str, const char *filename_str, int start)
{
    PyObject *filename;
    struct symtable *st;

    filename = PyUnicode_DecodeFSDefault(filename_str);
    if (filename == NULL)
        return NULL;
    st = Py_SymtableStringObject(str, filename, start);
    Py_DECREF(filename);
    return st;
}

/* Preferred access to parser is through AST. */
mod_ty
PyParser_ASTFromStringObject(const char *s, PyObject *filename, int start,
                             PyCompilerFlags *flags, PyArena *arena)
{
    mod_ty mod;
    PyCompilerFlags localflags;
    perrdetail err;
    int iflags = PARSER_FLAGS(flags);

    node *n = PyParser_ParseStringObject(s, filename,
                                         &_PyParser_Grammar, start, &err,
                                         &iflags);
    if (flags == NULL) {
        localflags.cf_flags = 0;
        flags = &localflags;
    }
    if (n) {
        flags->cf_flags |= iflags & PyCF_MASK;
        mod = PyAST_FromNodeObject(n, flags, filename, arena);
        PyNode_Free(n);
    }
    else {
        err_input(&err);
        mod = NULL;
    }
    err_free(&err);
    return mod;
}

mod_ty
PyParser_ASTFromString(const char *s, const char *filename_str, int start,
                       PyCompilerFlags *flags, PyArena *arena)
{
    PyObject *filename;
    mod_ty mod;
    filename = PyUnicode_DecodeFSDefault(filename_str);
    if (filename == NULL)
        return NULL;
    mod = PyParser_ASTFromStringObject(s, filename, start, flags, arena);
    Py_DECREF(filename);
    return mod;
}

mod_ty
PyParser_ASTFromFileObject(FILE *fp, PyObject *filename, const char* enc,
                           int start, const char *ps1,
                           const char *ps2, PyCompilerFlags *flags, int *errcode,
                           PyArena *arena)
{
    mod_ty mod;
    PyCompilerFlags localflags;
    perrdetail err;
    int iflags = PARSER_FLAGS(flags);

    node *n = PyParser_ParseFileObject(fp, filename, enc,
                                       &_PyParser_Grammar,
                                       start, ps1, ps2, &err, &iflags);
    if (flags == NULL) {
        localflags.cf_flags = 0;
        flags = &localflags;
    }
    if (n) {
        flags->cf_flags |= iflags & PyCF_MASK;
        mod = PyAST_FromNodeObject(n, flags, filename, arena);
        PyNode_Free(n);
    }
    else {
        err_input(&err);
        if (errcode)
            *errcode = err.error;
        mod = NULL;
    }
    err_free(&err);
    return mod;
}

mod_ty
PyParser_ASTFromFile(FILE *fp, const char *filename_str, const char* enc,
                     int start, const char *ps1,
                     const char *ps2, PyCompilerFlags *flags, int *errcode,
                     PyArena *arena)
{
    mod_ty mod;
    PyObject *filename;
    filename = PyUnicode_DecodeFSDefault(filename_str);
    if (filename == NULL)
        return NULL;
    mod = PyParser_ASTFromFileObject(fp, filename, enc, start, ps1, ps2,
                                     flags, errcode, arena);
    Py_DECREF(filename);
    return mod;
}

/* Simplified interface to parsefile -- return node or set exception */

node *
PyParser_SimpleParseFileFlags(FILE *fp, const char *filename, int start, int flags)
{
    perrdetail err;
    node *n = PyParser_ParseFileFlags(fp, filename, NULL,
                                      &_PyParser_Grammar,
                                      start, NULL, NULL, &err, flags);
    if (n == NULL)
        err_input(&err);
    err_free(&err);

    return n;
}

/* Simplified interface to parsestring -- return node or set exception */

node *
PyParser_SimpleParseStringFlags(const char *str, int start, int flags)
{
    perrdetail err;
    node *n = PyParser_ParseStringFlags(str, &_PyParser_Grammar,
                                        start, &err, flags);
    if (n == NULL)
        err_input(&err);
    err_free(&err);
    return n;
}

node *
PyParser_SimpleParseStringFlagsFilename(const char *str, const char *filename,
                                        int start, int flags)
{
    perrdetail err;
    node *n = PyParser_ParseStringFlagsFilename(str, filename,
                            &_PyParser_Grammar, start, &err, flags);
    if (n == NULL)
        err_input(&err);
    err_free(&err);
    return n;
}

node *
PyParser_SimpleParseStringFilename(const char *str, const char *filename, int start)
{
    return PyParser_SimpleParseStringFlagsFilename(str, filename, start, 0);
}

/* May want to move a more generalized form of this to parsetok.c or
   even parser modules. */

void
PyParser_ClearError(perrdetail *err)
{
    err_free(err);
}

void
PyParser_SetError(perrdetail *err)
{
    err_input(err);
}

static void
err_free(perrdetail *err)
{
    Py_CLEAR(err->filename);
}

/* Set the error appropriate to the given input error code (see errcode.h) */

static void
err_input(perrdetail *err)
{
    PyObject *v, *w, *errtype, *errtext;
    PyObject *msg_obj = NULL;
    const char *msg = NULL;
    int offset = err->offset;

    errtype = PyExc_SyntaxError;
    switch (err->error) {
    case E_ERROR:
        goto cleanup;
    case E_SYNTAX:
        errtype = PyExc_IndentationError;
        if (err->expected == INDENT)
            msg = "expected an indented block";
        else if (err->token == INDENT)
            msg = "unexpected indent";
        else if (err->token == DEDENT)
            msg = "unexpected unindent";
        else if (err->expected == NOTEQUAL) {
            errtype = PyExc_SyntaxError;
            msg = "with Barry as BDFL, use '<>' instead of '!='";
        }
        else {
            errtype = PyExc_SyntaxError;
            msg = "invalid syntax";
        }
        break;
    case E_TOKEN:
        msg = "invalid token";
        break;
    case E_EOFS:
        msg = "EOF while scanning triple-quoted string literal";
        break;
    case E_EOLS:
        msg = "EOL while scanning string literal";
        break;
    case E_INTR:
        if (!PyErr_Occurred())
            PyErr_SetNone(PyExc_KeyboardInterrupt);
        goto cleanup;
    case E_NOMEM:
        PyErr_NoMemory();
        goto cleanup;
    case E_EOF:
        msg = "unexpected EOF while parsing";
        break;
    case E_TABSPACE:
        errtype = PyExc_TabError;
        msg = "inconsistent use of tabs and spaces in indentation";
        break;
    case E_OVERFLOW:
        msg = "expression too long";
        break;
    case E_DEDENT:
        errtype = PyExc_IndentationError;
        msg = "unindent does not match any outer indentation level";
        break;
    case E_TOODEEP:
        errtype = PyExc_IndentationError;
        msg = "too many levels of indentation";
        break;
    case E_DECODE: {
        PyObject *type, *value, *tb;
        PyErr_Fetch(&type, &value, &tb);
        msg = "unknown decode error";
        if (value != NULL)
            msg_obj = PyObject_Str(value);
        Py_XDECREF(type);
        Py_XDECREF(value);
        Py_XDECREF(tb);
        break;
    }
    case E_LINECONT:
        msg = "unexpected character after line continuation character";
        break;

    case E_IDENTIFIER:
        msg = "invalid character in identifier";
        break;
    case E_BADSINGLE:
        msg = "multiple statements found while compiling a single statement";
        break;
    default:
        fprintf(stderr, "error=%d\n", err->error);
        msg = "unknown parsing error";
        break;
    }
    /* err->text may not be UTF-8 in case of decoding errors.
       Explicitly convert to an object. */
    if (!err->text) {
        errtext = Py_None;
        Py_INCREF(Py_None);
    } else {
        errtext = PyUnicode_DecodeUTF8(err->text, err->offset,
                                       "replace");
        if (errtext != NULL) {
            Py_ssize_t len = strlen(err->text);
            offset = (int)PyUnicode_GET_LENGTH(errtext);
            if (len != err->offset) {
                Py_DECREF(errtext);
                errtext = PyUnicode_DecodeUTF8(err->text, len,
                                               "replace");
            }
        }
    }
    v = Py_BuildValue("(OiiN)", err->filename,
                      err->lineno, offset, errtext);
    if (v != NULL) {
        if (msg_obj)
            w = Py_BuildValue("(OO)", msg_obj, v);
        else
            w = Py_BuildValue("(sO)", msg, v);
    } else
        w = NULL;
    Py_XDECREF(v);
    PyErr_SetObject(errtype, w);
    Py_XDECREF(w);
cleanup:
    Py_XDECREF(msg_obj);
    if (err->text != NULL) {
        PyObject_FREE(err->text);
        err->text = NULL;
    }
}

/* Deprecated C API functions still provided for binary compatibility */

#undef PyParser_SimpleParseFile
PyAPI_FUNC(node *)
PyParser_SimpleParseFile(FILE *fp, const char *filename, int start)
{
    return PyParser_SimpleParseFileFlags(fp, filename, start, 0);
}

#undef PyParser_SimpleParseString
PyAPI_FUNC(node *)
PyParser_SimpleParseString(const char *str, int start)
{
    return PyParser_SimpleParseStringFlags(str, start, 0);
}

#ifdef __cplusplus
}
#endif
