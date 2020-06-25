#ifndef CPYTHON_FUNC_H
#define CPYTHON_FUNC_H

#include <math.h>

#include "cpython-types.h"

#include "pyconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PyAPI_FUNC(RTYPE) __attribute__((visibility("default"))) RTYPE

#ifdef __cplusplus
#define PyMODINIT_FUNC extern "C" PyObject*
#else
#define PyMODINIT_FUNC PyObject*
#endif
#define Py_LOCAL(type) static type
#define Py_LOCAL_INLINE(type) static inline type

#define Py_GCC_ATTRIBUTE(x) __attribute__(x)
#define Py_ALIGNED(x) __attribute__((aligned(x)))
#define Py_DEPRECATED(VERSION_UNUSED) __attribute__((__deprecated__))
#define Py_UNUSED(name) _unused_##name __attribute__((unused))
#define _Py_NO_RETURN __attribute__((__noreturn__))

#define Py_UNREACHABLE() abort()

/* Singletons */
PyAPI_FUNC(PyTypeObject*) PyAsyncGen_Type_Ptr(void);
PyAPI_FUNC(PyTypeObject*) PyBaseObject_Type_Ptr(void);
PyAPI_FUNC(PyTypeObject*) PyBool_Type_Ptr(void);
PyAPI_FUNC(PyTypeObject*) PyByteArrayIter_Type_Ptr(void);
PyAPI_FUNC(PyTypeObject*) PyByteArray_Type_Ptr(void);
PyAPI_FUNC(PyTypeObject*) PyBytesIter_Type_Ptr(void);
PyAPI_FUNC(PyTypeObject*) PyBytes_Type_Ptr(void);
PyAPI_FUNC(PyTypeObject*) PyClassMethod_Type_Ptr(void);
PyAPI_FUNC(PyTypeObject*) PyCode_Type_Ptr(void);
PyAPI_FUNC(PyTypeObject*) PyComplex_Type_Ptr(void);
PyAPI_FUNC(PyTypeObject*) PyCoro_Type_Ptr(void);
PyAPI_FUNC(PyTypeObject*) PyDictItems_Type_Ptr(void);
PyAPI_FUNC(PyTypeObject*) PyDictIterItem_Type_Ptr(void);
PyAPI_FUNC(PyTypeObject*) PyDictIterKey_Type_Ptr(void);
PyAPI_FUNC(PyTypeObject*) PyDictIterValue_Type_Ptr(void);
PyAPI_FUNC(PyTypeObject*) PyDictKeys_Type_Ptr(void);
PyAPI_FUNC(PyTypeObject*) PyDictProxy_Type_Ptr(void);
PyAPI_FUNC(PyTypeObject*) PyDictValues_Type_Ptr(void);
PyAPI_FUNC(PyTypeObject*) PyDict_Type_Ptr(void);
PyAPI_FUNC(PyObject*) PyEllipsis_Ptr(void);
PyAPI_FUNC(PyTypeObject*) PyEllipsis_Type_Ptr(void);
PyAPI_FUNC(PyTypeObject*) PyEnum_Type_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_ArithmeticError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_AssertionError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_AttributeError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_BaseException_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_BlockingIOError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_BrokenPipeError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_BufferError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_BytesWarning_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_ChildProcessError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_ConnectionAbortedError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_ConnectionError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_ConnectionRefusedError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_ConnectionResetError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_DeprecationWarning_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_EOFError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_Exception_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_FileExistsError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_FileNotFoundError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_FloatingPointError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_FutureWarning_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_GeneratorExit_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_ImportError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_ImportWarning_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_IndentationError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_IndexError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_InterruptedError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_IsADirectoryError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_KeyError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_KeyboardInterrupt_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_LookupError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_MemoryError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_ModuleNotFoundError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_NameError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_NotADirectoryError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_NotImplementedError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_OSError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_OverflowError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_PendingDeprecationWarning_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_PermissionError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_ProcessLookupError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_RecursionError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_ReferenceError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_ResourceWarning_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_RuntimeError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_RuntimeWarning_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_StopAsyncIteration_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_StopIteration_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_SyntaxError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_SyntaxWarning_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_SystemError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_SystemExit_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_TabError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_TimeoutError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_TypeError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_UnboundLocalError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_UnicodeDecodeError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_UnicodeEncodeError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_UnicodeError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_UnicodeTranslateError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_UnicodeWarning_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_UserWarning_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_ValueError_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_Warning_Ptr(void);
PyAPI_FUNC(PyObject*) PyExc_ZeroDivisionError_Ptr(void);
PyAPI_FUNC(PyObject*) PyFalse_Ptr(void);
PyAPI_FUNC(PyTypeObject*) PyFloat_Type_Ptr(void);
PyAPI_FUNC(PyTypeObject*) PyFrozenSet_Type_Ptr(void);
PyAPI_FUNC(PyTypeObject*) PyFunction_Type_Ptr(void);
PyAPI_FUNC(PyTypeObject*) PyGen_Type_Ptr(void);
PyAPI_FUNC(PyTypeObject*) PyListIter_Type_Ptr(void);
PyAPI_FUNC(PyTypeObject*) PyList_Type_Ptr(void);
PyAPI_FUNC(PyTypeObject*) PyLongRangeIter_Type_Ptr(void);
PyAPI_FUNC(PyTypeObject*) PyLong_Type_Ptr(void);
PyAPI_FUNC(PyTypeObject*) PyMemoryView_Type_Ptr(void);
PyAPI_FUNC(PyTypeObject*) PyMethod_Type_Ptr(void);
PyAPI_FUNC(PyTypeObject*) PyModule_Type_Ptr(void);
PyAPI_FUNC(PyObject*) PyNone_Ptr(void);
PyAPI_FUNC(PyObject*) PyNotImplemented_Ptr(void);
PyAPI_FUNC(PyTypeObject*) PyProperty_Type_Ptr(void);
PyAPI_FUNC(PyTypeObject*) PyRangeIter_Type_Ptr(void);
PyAPI_FUNC(PyTypeObject*) PyRange_Type_Ptr(void);
PyAPI_FUNC(PyTypeObject*) PySeqIter_Type_Ptr(void);
PyAPI_FUNC(PyTypeObject*) PySetIter_Type_Ptr(void);
PyAPI_FUNC(PyTypeObject*) PySet_Type_Ptr(void);
PyAPI_FUNC(PyTypeObject*) PySlice_Type_Ptr(void);
PyAPI_FUNC(PyTypeObject*) PyStaticMethod_Type_Ptr(void);
PyAPI_FUNC(PyTypeObject*) PySuper_Type_Ptr(void);
PyAPI_FUNC(PyObject*) PyTrue_Ptr(void);
PyAPI_FUNC(PyTypeObject*) PyTupleIter_Type_Ptr(void);
PyAPI_FUNC(PyTypeObject*) PyTuple_Type_Ptr(void);
PyAPI_FUNC(PyTypeObject*) PyType_Type_Ptr(void);
PyAPI_FUNC(PyTypeObject*) PyUnicodeIter_Type_Ptr(void);
PyAPI_FUNC(PyTypeObject*) PyUnicode_Type_Ptr(void);
PyAPI_FUNC(const _Py_HashSecret_t*) _Py_HashSecret_Ptr(void);
PyAPI_FUNC(PyObject*) _PyLong_One_Ptr(void);
PyAPI_FUNC(PyObject*) _PyLong_Zero_Ptr(void);
PyAPI_FUNC(PyTypeObject*) _PyNone_Type_Ptr(void);
PyAPI_FUNC(PyTypeObject*) _PyNotImplemented_Type_Ptr(void);

/* Macro helpers */
PyAPI_FUNC(int) PyBool_Check_Func(PyObject*);
PyAPI_FUNC(int) PyByteArray_CheckExact_Func(PyObject*);
PyAPI_FUNC(int) PyByteArray_Check_Func(PyObject*);
PyAPI_FUNC(int) PyBytes_CheckExact_Func(PyObject*);
PyAPI_FUNC(int) PyBytes_Check_Func(PyObject*);
PyAPI_FUNC(int) PyCFunction_Check_Func(PyObject*);
PyAPI_FUNC(int) PyCapsule_CheckExact_Func(PyObject*);
PyAPI_FUNC(int) PyCode_Check_Func(PyObject*);
PyAPI_FUNC(PyObject*) PyCode_GetFreevars_Func(PyObject*);
PyAPI_FUNC(PyObject*) PyCode_GetName_Func(PyObject*);
PyAPI_FUNC(Py_ssize_t) PyCode_GetNumFree_Func(PyObject*);
PyAPI_FUNC(int) PyComplex_CheckExact_Func(PyObject*);
PyAPI_FUNC(int) PyComplex_Check_Func(PyObject*);
PyAPI_FUNC(int) PyDict_CheckExact_Func(PyObject*);
PyAPI_FUNC(int) PyDict_Check_Func(PyObject*);
PyAPI_FUNC(Py_ssize_t) PyDict_GET_SIZE_Func(PyObject*);
PyAPI_FUNC(int) PyExceptionClass_Check_Func(PyObject*);
PyAPI_FUNC(int) PyExceptionInstance_Check_Func(PyObject*);
PyAPI_FUNC(int) PyFloat_CheckExact_Func(PyObject*);
PyAPI_FUNC(int) PyFloat_Check_Func(PyObject*);
PyAPI_FUNC(int) PyFrozenSet_CheckExact_Func(PyObject*);
PyAPI_FUNC(int) PyFrozenSet_Check_Func(PyObject*);
PyAPI_FUNC(int) PyIndex_Check_Func(PyObject*);
PyAPI_FUNC(int) PyIter_Check_Func(PyObject*);
PyAPI_FUNC(int) PyList_CheckExact_Func(PyObject*);
PyAPI_FUNC(int) PyList_Check_Func(PyObject*);
PyAPI_FUNC(int) PyList_SET_ITEM_Func(PyObject*, Py_ssize_t, PyObject*);
PyAPI_FUNC(int) PyLong_CheckExact_Func(PyObject*);
PyAPI_FUNC(int) PyLong_Check_Func(PyObject*);
PyAPI_FUNC(void*) PyMem_New_Func(size_t size, size_t n);
PyAPI_FUNC(int) PyMemoryView_Check_Func(PyObject*);
PyAPI_FUNC(int) PyMethod_Check_Func(PyObject*);
PyAPI_FUNC(int) PyModule_CheckExact_Func(PyObject*);
PyAPI_FUNC(int) PyModule_Check_Func(PyObject*);
PyAPI_FUNC(int) PyObject_CheckBuffer_Func(PyObject*);
PyAPI_FUNC(int) PyObject_TypeCheck_Func(PyObject*, PyTypeObject*);
PyAPI_FUNC(int) PySet_Check_Func(PyObject*);
PyAPI_FUNC(PyObject*) PySequence_Fast_GET_ITEM_Func(PyObject*, Py_ssize_t);
PyAPI_FUNC(Py_ssize_t) PySequence_Fast_GET_SIZE_Func(PyObject*);
PyAPI_FUNC(PyObject*) PySequence_ITEM_Func(PyObject*, Py_ssize_t);
PyAPI_FUNC(int) PySlice_Check_Func(PyObject*);
PyAPI_FUNC(PyObject*)
    PyStructSequence_SET_ITEM_Func(PyObject*, Py_ssize_t, PyObject*);
PyAPI_FUNC(int) PyTraceBack_Check_Func(PyObject*);
PyAPI_FUNC(int) PyTuple_CheckExact_Func(PyObject*);
PyAPI_FUNC(int) PyTuple_Check_Func(PyObject*);
PyAPI_FUNC(int) PyTuple_SET_ITEM_Func(PyObject*, Py_ssize_t, PyObject*);
PyAPI_FUNC(int) PyType_CheckExact_Func(PyObject*);
PyAPI_FUNC(int) PyType_Check_Func(PyObject*);
PyAPI_FUNC(int) PyUnicode_CheckExact_Func(PyObject*);
PyAPI_FUNC(int) PyUnicode_Check_Func(PyObject*);
PyAPI_FUNC(void*) PyUnicode_DATA_Func(PyObject*);
PyAPI_FUNC(int) PyUnicode_IS_ASCII_Func(PyObject*);
PyAPI_FUNC(int) PyUnicode_KIND_Func(PyObject*);
PyAPI_FUNC(Py_UCS4) PyUnicode_READ_CHAR_Func(PyObject*, Py_ssize_t);
PyAPI_FUNC(Py_UCS4) PyUnicode_READ_Func(int, void*, Py_ssize_t);
PyAPI_FUNC(void)
    PyUnicode_WRITE_Func(enum PyUnicode_Kind, void*, Py_ssize_t, Py_UCS4);
PyAPI_FUNC(int) PyWeakref_Check_Func(PyObject*);
PyAPI_FUNC(PyObject*) PyWeakref_GET_OBJECT_Func(PyObject*);
PyAPI_FUNC(void) Py_DECREF_Func(PyObject*);
PyAPI_FUNC(int) Py_EnterRecursiveCall_Func(const char*);
PyAPI_FUNC(void) Py_INCREF_Func(PyObject*);
PyAPI_FUNC(void) Py_LeaveRecursiveCall_Func(void);
PyAPI_FUNC(Py_ssize_t*) Py_REFCNT_Func(PyObject*);
PyAPI_FUNC(Py_ssize_t*) Py_SIZE_Func(PyVarObject*);
PyAPI_FUNC(PyTypeObject*) Py_TYPE_Func(PyObject* obj);
PyAPI_FUNC(void) Py_XDECREF_Func(PyObject*);
PyAPI_FUNC(void) Py_XINCREF_Func(PyObject*);
PyAPI_FUNC(PyObject*) _PyCode_ConstantKey(PyObject*);
PyAPI_FUNC(int) _PyObject_DebugMallocStats(FILE*);
PyAPI_FUNC(Py_ssize_t) _PyObject_SIZE_Func(PyObject*);
PyAPI_FUNC(Py_ssize_t) _PyObject_VAR_SIZE_Func(PyObject*, Py_ssize_t);

/* C-API functions */
PyAPI_FUNC(int) _PyAST_Optimize(struct _mod*, PyArena* arena, int optimize);
PyAPI_FUNC(PyCodeObject*)
    PyAST_Compile(struct _mod*, const char*, PyCompilerFlags*, PyArena*);
PyAPI_FUNC(PyCodeObject*)
    PyAST_CompileEx(struct _mod*, const char*, PyCompilerFlags*, int, PyArena*);
PyAPI_FUNC(PyCodeObject*) PyAST_CompileObject(struct _mod*, PyObject*,
                                              PyCompilerFlags*, int, PyArena*);
PyAPI_FUNC(int) PyArena_AddPyObject(PyArena*, PyObject*);
PyAPI_FUNC(void) PyArena_Free(PyArena*);
PyAPI_FUNC(void*) PyArena_Malloc(PyArena*, size_t);
PyAPI_FUNC(PyArena*) PyArena_New(void);
PyAPI_FUNC(int) PyArg_Parse(PyObject*, const char*, ...);
PyAPI_FUNC(int) PyArg_ParseTuple(PyObject* args, const char* format, ...);
PyAPI_FUNC(int)
    PyArg_ParseTupleAndKeywords(PyObject*, PyObject*, const char*, char**, ...);
PyAPI_FUNC(int)
    PyArg_UnpackTuple(PyObject*, const char*, Py_ssize_t, Py_ssize_t, ...);
PyAPI_FUNC(int) PyArg_VaParse(PyObject*, const char*, va_list);
PyAPI_FUNC(int) PyArg_VaParseTupleAndKeywords(PyObject*, PyObject*, const char*,
                                              char**, va_list);
PyAPI_FUNC(int) PyArg_ValidateKeywordArguments(PyObject*);
PyAPI_FUNC(PyObject*) PyBool_FromLong(long);
PyAPI_FUNC(int)
    PyBuffer_FillInfo(Py_buffer*, PyObject*, void*, Py_ssize_t, int, int);
PyAPI_FUNC(int) PyBuffer_IsContiguous(const Py_buffer*, char);
PyAPI_FUNC(void) PyBuffer_Release(Py_buffer*);
PyAPI_FUNC(char*) PyByteArray_AsString(PyObject*);
PyAPI_FUNC(PyObject*) PyByteArray_Concat(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyByteArray_FromObject(PyObject*);
PyAPI_FUNC(PyObject*) PyByteArray_FromStringAndSize(const char*, Py_ssize_t);
PyAPI_FUNC(int) PyByteArray_Resize(PyObject*, Py_ssize_t);
PyAPI_FUNC(Py_ssize_t) PyByteArray_Size(PyObject*);
PyAPI_FUNC(char*) PyBytes_AsString(PyObject*);
PyAPI_FUNC(int) PyBytes_AsStringAndSize(PyObject*, char**, Py_ssize_t*);
PyAPI_FUNC(void) PyBytes_Concat(PyObject**, PyObject*);
PyAPI_FUNC(void) PyBytes_ConcatAndDel(PyObject**, PyObject*);
PyAPI_FUNC(PyObject*) PyBytes_DecodeEscape(const char*, Py_ssize_t, const char*,
                                           Py_ssize_t, const char*);
PyAPI_FUNC(PyObject*) PyBytes_FromFormat(const char*, ...);
PyAPI_FUNC(PyObject*) PyBytes_FromFormatV(const char*, va_list);
PyAPI_FUNC(PyObject*) PyBytes_FromObject(PyObject*);
PyAPI_FUNC(PyObject*) PyBytes_FromString(const char*);
PyAPI_FUNC(PyObject*) PyBytes_FromStringAndSize(const char*, Py_ssize_t);
PyAPI_FUNC(PyObject*) PyBytes_Repr(PyObject*, int);
PyAPI_FUNC(Py_ssize_t) PyBytes_Size(PyObject*);
PyAPI_FUNC(PyObject*) PyCFunction_Call(PyObject*, PyObject*, PyObject*);
PyAPI_FUNC(int) PyCFunction_GetFlags(PyObject*);
PyAPI_FUNC(PyCFunction) PyCFunction_GetFunction(PyObject*);
PyAPI_FUNC(PyObject*) PyCFunction_GetSelf(PyObject*);
PyAPI_FUNC(PyObject*) PyCFunction_New(PyMethodDef*, PyObject*);
PyAPI_FUNC(PyObject*) PyCFunction_NewEx(PyMethodDef*, PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyCallIter_New(PyObject*, PyObject*);
PyAPI_FUNC(int) PyCallable_Check(PyObject*);
PyAPI_FUNC(void*) PyCapsule_GetContext(PyObject*);
PyAPI_FUNC(PyCapsule_Destructor) PyCapsule_GetDestructor(PyObject*);
PyAPI_FUNC(const char*) PyCapsule_GetName(PyObject*);
PyAPI_FUNC(void*) PyCapsule_GetPointer(PyObject*, const char*);
PyAPI_FUNC(void*) PyCapsule_Import(const char*, int);
PyAPI_FUNC(int) PyCapsule_IsValid(PyObject*, const char*);
PyAPI_FUNC(PyObject*) PyCapsule_New(void*, const char*, PyCapsule_Destructor);
PyAPI_FUNC(int) PyCapsule_SetContext(PyObject*, void*);
PyAPI_FUNC(int) PyCapsule_SetDestructor(PyObject*, PyCapsule_Destructor);
PyAPI_FUNC(int) PyCapsule_SetName(PyObject*, const char*);
PyAPI_FUNC(int) PyCapsule_SetPointer(PyObject*, void*);
PyAPI_FUNC(PyObject*) PyClassMethod_New(PyObject*);
PyAPI_FUNC(PyCodeObject*)
    PyCode_New(int, int, int, int, int, PyObject*, PyObject*, PyObject*,
               PyObject*, PyObject*, PyObject*, PyObject*, PyObject*, int,
               PyObject*);
PyAPI_FUNC(PyCodeObject*) PyCode_NewEmpty(const char*, const char*, int);
PyAPI_FUNC(PyCodeObject*)
    PyCode_NewWithPosOnlyArgs(int, int, int, int, int, int, PyObject*,
                              PyObject*, PyObject*, PyObject*, PyObject*,
                              PyObject*, PyObject*, PyObject*, int, PyObject*);
PyAPI_FUNC(PyObject*) PyCodec_BackslashReplaceErrors(PyObject*);
PyAPI_FUNC(PyObject*) PyCodec_Decode(PyObject*, const char*, const char*);
PyAPI_FUNC(PyObject*) PyCodec_Decoder(const char*);
PyAPI_FUNC(PyObject*) PyCodec_Encode(PyObject*, const char*, const char*);
PyAPI_FUNC(PyObject*) PyCodec_Encoder(const char*);
PyAPI_FUNC(PyObject*) PyCodec_IgnoreErrors(PyObject*);
PyAPI_FUNC(PyObject*) PyCodec_IncrementalDecoder(const char*, const char*);
PyAPI_FUNC(PyObject*) PyCodec_IncrementalEncoder(const char*, const char*);
PyAPI_FUNC(int) PyCodec_KnownEncoding(const char*);
PyAPI_FUNC(PyObject*) PyCodec_LookupError(const char*);
PyAPI_FUNC(PyObject*) PyCodec_NameReplaceErrors(PyObject*);
PyAPI_FUNC(int) PyCodec_Register(PyObject*);
PyAPI_FUNC(int) PyCodec_RegisterError(const char*, PyObject*);
PyAPI_FUNC(PyObject*) PyCodec_ReplaceErrors(PyObject*);
PyAPI_FUNC(PyObject*) PyCodec_StreamReader(const char*, PyObject*, const char*);
PyAPI_FUNC(PyObject*) PyCodec_StreamWriter(const char*, PyObject*, const char*);
PyAPI_FUNC(PyObject*) PyCodec_StrictErrors(PyObject*);
PyAPI_FUNC(PyObject*) PyCodec_XMLCharRefReplaceErrors(PyObject*);
PyAPI_FUNC(int) PyCompile_OpcodeStackEffect(int, int);
PyAPI_FUNC(Py_complex) PyComplex_AsCComplex(PyObject*);
PyAPI_FUNC(PyObject*) PyComplex_FromCComplex(Py_complex);
PyAPI_FUNC(PyObject*) PyComplex_FromDoubles(double, double);
PyAPI_FUNC(double) PyComplex_ImagAsDouble(PyObject*);
PyAPI_FUNC(double) PyComplex_RealAsDouble(PyObject*);
PyAPI_FUNC(PyObject*) PyDescr_NewClassMethod(PyTypeObject*, PyMethodDef*);
PyAPI_FUNC(PyObject*) PyDescr_NewGetSet(PyTypeObject*, PyGetSetDef*);
PyAPI_FUNC(PyObject*) PyDescr_NewMember(PyTypeObject*, struct PyMemberDef*);
PyAPI_FUNC(PyObject*) PyDescr_NewMethod(PyTypeObject*, PyMethodDef*);
PyAPI_FUNC(PyObject*) PyDictProxy_New(PyObject*);
PyAPI_FUNC(void) PyDict_Clear(PyObject*);
PyAPI_FUNC(int) PyDict_Contains(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyDict_Copy(PyObject*);
PyAPI_FUNC(int) PyDict_DelItem(PyObject*, PyObject*);
PyAPI_FUNC(int) PyDict_DelItemString(PyObject*, const char*);
PyAPI_FUNC(PyObject*) PyDict_GetItem(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyDict_GetItemString(PyObject*, const char*);
PyAPI_FUNC(PyObject*) PyDict_GetItemWithError(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyDict_Items(PyObject*);
PyAPI_FUNC(PyObject*) PyDict_Keys(PyObject*);
PyAPI_FUNC(int) PyDict_Merge(PyObject*, PyObject*, int);
PyAPI_FUNC(int) PyDict_MergeFromSeq2(PyObject*, PyObject*, int);
PyAPI_FUNC(PyObject*) PyDict_New(void);
PyAPI_FUNC(int) PyDict_Next(PyObject*, Py_ssize_t*, PyObject**, PyObject**);
PyAPI_FUNC(int) PyDict_SetItem(PyObject*, PyObject*, PyObject*);
PyAPI_FUNC(int) PyDict_SetItemString(PyObject*, const char*, PyObject*);
PyAPI_FUNC(Py_ssize_t) PyDict_Size(PyObject*);
PyAPI_FUNC(int) PyDict_Update(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyDict_Values(PyObject*);
PyAPI_FUNC(int) PyErr_BadArgument(void);
PyAPI_FUNC(void) PyErr_BadInternalCall(void);
PyAPI_FUNC(int) PyErr_CheckSignals(void);
PyAPI_FUNC(void) PyErr_Clear(void);
PyAPI_FUNC(void) PyErr_Display(PyObject*, PyObject*, PyObject*);
PyAPI_FUNC(int) PyErr_ExceptionMatches(PyObject*);
PyAPI_FUNC(void) PyErr_Fetch(PyObject**, PyObject**, PyObject**);
PyAPI_FUNC(PyObject*) PyErr_Format(PyObject*, const char*, ...);
PyAPI_FUNC(PyObject*) PyErr_FormatV(PyObject*, const char*, va_list);
PyAPI_FUNC(void) PyErr_GetExcInfo(PyObject**, PyObject**, PyObject**);
PyAPI_FUNC(int) PyErr_GivenExceptionMatches(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyErr_NewException(const char*, PyObject*, PyObject*);
PyAPI_FUNC(PyObject*)
    PyErr_NewExceptionWithDoc(const char*, const char*, PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyErr_NoMemory(void);
PyAPI_FUNC(void) PyErr_NormalizeException(PyObject**, PyObject**, PyObject**);
PyAPI_FUNC(PyObject*) PyErr_Occurred(void);
PyAPI_FUNC(void) PyErr_Print(void);
PyAPI_FUNC(void) PyErr_PrintEx(int);
PyAPI_FUNC(PyObject*) PyErr_ProgramText(const char*, int);
PyAPI_FUNC(PyObject*) PyErr_ProgramTextObject(PyObject*, int);
PyAPI_FUNC(int) PyErr_ResourceWarning(PyObject*, Py_ssize_t, const char*, ...);
PyAPI_FUNC(void) PyErr_Restore(PyObject*, PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyErr_SetExcFromWindowsErr(PyObject*, int);
PyAPI_FUNC(PyObject*)
    PyErr_SetExcFromWindowsErrWithFilename(PyObject*, int, const char*);
PyAPI_FUNC(PyObject*)
    PyErr_SetExcFromWindowsErrWithFilenameObject(PyObject*, int, PyObject*);
PyAPI_FUNC(PyObject*)
    PyErr_SetExcFromWindowsErrWithFilenameObjects(PyObject*, int, PyObject*,
                                                  PyObject*);
PyAPI_FUNC(void) PyErr_SetExcInfo(PyObject*, PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyErr_SetFromErrno(PyObject*);
PyAPI_FUNC(PyObject*) PyErr_SetFromErrnoWithFilename(PyObject*, const char*);
PyAPI_FUNC(PyObject*)
    PyErr_SetFromErrnoWithFilenameObject(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*)
    PyErr_SetFromErrnoWithFilenameObjects(PyObject*, PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyErr_SetFromWindowsErr(int);
PyAPI_FUNC(PyObject*) PyErr_SetFromWindowsErrWithFilename(int, const char*);
PyAPI_FUNC(PyObject*) PyErr_SetImportError(PyObject*, PyObject*, PyObject*);
PyAPI_FUNC(PyObject*)
    PyErr_SetImportErrorSubclass(PyObject*, PyObject*, PyObject*, PyObject*);
PyAPI_FUNC(void) PyErr_SetInterrupt(void);
PyAPI_FUNC(void) PyErr_SetNone(PyObject*);
PyAPI_FUNC(void) PyErr_SetObject(PyObject*, PyObject*);
PyAPI_FUNC(void) PyErr_SetString(PyObject*, const char*);
PyAPI_FUNC(void) PyErr_SyntaxLocation(const char*, int);
PyAPI_FUNC(void) PyErr_SyntaxLocationEx(const char*, int, int);
PyAPI_FUNC(void) PyErr_SyntaxLocationObject(PyObject*, int, int);
PyAPI_FUNC(int) PyErr_WarnEx(PyObject*, const char*, Py_ssize_t);
PyAPI_FUNC(int) PyErr_WarnExplicit(PyObject*, const char*, const char*, int,
                                   const char*, PyObject*);
PyAPI_FUNC(int) PyErr_WarnExplicitObject(PyObject*, PyObject*, PyObject*, int,
                                         PyObject*, PyObject*);
PyAPI_FUNC(int) PyErr_WarnFormat(PyObject*, Py_ssize_t, const char*, ...);
PyAPI_FUNC(void) PyErr_WriteUnraisable(PyObject*);
PyAPI_FUNC(void) PyEval_AcquireLock(void);
PyAPI_FUNC(void) PyEval_AcquireThread(PyThreadState*);
PyAPI_FUNC(PyObject*) PyEval_CallFunction(PyObject*, const char*, ...);
PyAPI_FUNC(PyObject*)
    PyEval_CallMethod(PyObject*, const char*, const char*, ...);
PyAPI_FUNC(PyObject*)
    PyEval_CallObjectWithKeywords(PyObject*, PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyEval_EvalCode(PyObject*, PyObject*, PyObject*);
PyAPI_FUNC(PyObject*)
    PyEval_EvalCodeEx(PyObject*, PyObject*, PyObject*, PyObject**, int,
                      PyObject**, int, PyObject**, int, PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyEval_EvalFrame(PyFrameObject*);
PyAPI_FUNC(PyObject*) PyEval_EvalFrameEx(PyFrameObject*, int);
PyAPI_FUNC(PyObject*) PyEval_GetBuiltins(void);
PyAPI_FUNC(PyFrameObject*) PyEval_GetFrame(void);
PyAPI_FUNC(const char*) PyEval_GetFuncDesc(PyObject*);
PyAPI_FUNC(const char*) PyEval_GetFuncName(PyObject*);
PyAPI_FUNC(PyObject*) PyEval_GetGlobals(void);
PyAPI_FUNC(PyObject*) PyEval_GetLocals(void);
PyAPI_FUNC(void) PyEval_InitThreads(void);
PyAPI_FUNC(int) PyEval_MergeCompilerFlags(PyCompilerFlags*);
PyAPI_FUNC(void) PyEval_ReInitThreads(void);
PyAPI_FUNC(void) PyEval_ReleaseLock(void);
PyAPI_FUNC(void) PyEval_ReleaseThread(PyThreadState*);
PyAPI_FUNC(void) PyEval_RestoreThread(PyThreadState*);
PyAPI_FUNC(PyThreadState*) PyEval_SaveThread(void);
PyAPI_FUNC(void) PyEval_SetProfile(Py_tracefunc, PyObject*);
PyAPI_FUNC(void) PyEval_SetTrace(Py_tracefunc, PyObject*);
PyAPI_FUNC(PyObject*) PyException_GetCause(PyObject*);
PyAPI_FUNC(PyObject*) PyException_GetContext(PyObject*);
PyAPI_FUNC(PyObject*) PyException_GetTraceback(PyObject*);
PyAPI_FUNC(void) PyException_SetCause(PyObject*, PyObject*);
PyAPI_FUNC(void) PyException_SetContext(PyObject*, PyObject*);
PyAPI_FUNC(int) PyException_SetTraceback(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyFile_GetLine(PyObject*, int);
PyAPI_FUNC(int) PyFile_WriteObject(PyObject*, PyObject*, int);
PyAPI_FUNC(int) PyFile_WriteString(const char*, PyObject*);
PyAPI_FUNC(double) PyFloat_AsDouble(PyObject*);
PyAPI_FUNC(PyObject*) PyFloat_FromDouble(double);
PyAPI_FUNC(PyObject*) PyFloat_FromString(PyObject*);
PyAPI_FUNC(PyObject*) PyFloat_GetInfo(void);
PyAPI_FUNC(double) PyFloat_GetMax(void);
PyAPI_FUNC(double) PyFloat_GetMin(void);
PyAPI_FUNC(void) PyFrame_FastToLocals(PyFrameObject*);
PyAPI_FUNC(int) PyFrame_GetLineNumber(PyFrameObject*);
PyAPI_FUNC(void) PyFrame_LocalsToFast(PyFrameObject*, int);
PyAPI_FUNC(PyFrameObject*)
    PyFrame_New(PyThreadState*, PyCodeObject*, PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyFrozenSet_New(PyObject*);
PyAPI_FUNC(PyFutureFeatures*) PyFuture_FromAST(struct _mod*, const char*);
PyAPI_FUNC(PyFutureFeatures*) PyFuture_FromASTObject(struct _mod*, PyObject*);
PyAPI_FUNC(Py_ssize_t) PyGC_Collect(void);
PyAPI_FUNC(int) PyGILState_Check(void);
PyAPI_FUNC(PyGILState_STATE) PyGILState_Ensure(void);
PyAPI_FUNC(PyThreadState*) PyGILState_GetThisThreadState(void);
PyAPI_FUNC(void) PyGILState_Release(PyGILState_STATE);
PyAPI_FUNC(PyObject*) PyImport_AddModule(const char*);
PyAPI_FUNC(PyObject*) PyImport_AddModuleObject(PyObject*);
PyAPI_FUNC(int) PyImport_AppendInittab(const char*, PyObject* (*)(void));
PyAPI_FUNC(void) PyImport_Cleanup(void);
PyAPI_FUNC(PyObject*) PyImport_ExecCodeModule(const char*, PyObject*);
PyAPI_FUNC(PyObject*)
    PyImport_ExecCodeModuleEx(const char*, PyObject*, const char*);
PyAPI_FUNC(PyObject*)
    PyImport_ExecCodeModuleObject(PyObject*, PyObject*, PyObject*, PyObject*);
PyAPI_FUNC(PyObject*)
    PyImport_ExecCodeModuleWithPathnames(const char*, PyObject*, const char*,
                                         const char*);
PyAPI_FUNC(long) PyImport_GetMagicNumber(void);
PyAPI_FUNC(const char*) PyImport_GetMagicTag(void);
PyAPI_FUNC(PyObject*) PyImport_GetModule(PyObject* name);
PyAPI_FUNC(PyObject*) PyImport_GetModuleDict(void);
PyAPI_FUNC(PyObject*) PyImport_Import(PyObject*);
PyAPI_FUNC(int) PyImport_ImportFrozenModule(const char*);
PyAPI_FUNC(int) PyImport_ImportFrozenModuleObject(PyObject*);
PyAPI_FUNC(PyObject*) PyImport_ImportModule(const char*);
PyAPI_FUNC(PyObject*) PyImport_ImportModuleLevel(const char*, PyObject*,
                                                 PyObject*, PyObject*, int);
PyAPI_FUNC(PyObject*)
    PyImport_ImportModuleLevelObject(PyObject*, PyObject*, PyObject*, PyObject*,
                                     int);
PyAPI_FUNC(PyObject*) PyImport_ImportModuleNoBlock(const char*);
PyAPI_FUNC(PyObject*) PyImport_ReloadModule(PyObject*);
PyAPI_FUNC(PyObject*) PyInstanceMethod_New(PyObject*);
PyAPI_FUNC(void) PyInterpreterState_Clear(PyInterpreterState*);
PyAPI_FUNC(void) PyInterpreterState_Delete(PyInterpreterState*);
PyAPI_FUNC(PyInterpreterState*) PyInterpreterState_Head(void);
PyAPI_FUNC(PyInterpreterState*) PyInterpreterState_Next(PyInterpreterState*);
PyAPI_FUNC(PyThreadState*) PyInterpreterState_ThreadHead(PyInterpreterState*);
PyAPI_FUNC(PyObject*) PyIter_Next(PyObject*);
PyAPI_FUNC(int) PyList_Append(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyList_AsTuple(PyObject*);
PyAPI_FUNC(PyObject*) PyList_GetItem(PyObject*, Py_ssize_t);
PyAPI_FUNC(PyObject*) PyList_GetSlice(PyObject*, Py_ssize_t, Py_ssize_t);
PyAPI_FUNC(int) PyList_Insert(PyObject*, Py_ssize_t, PyObject*);
PyAPI_FUNC(PyObject*) PyList_New(Py_ssize_t);
PyAPI_FUNC(int) PyList_Reverse(PyObject*);
PyAPI_FUNC(int) PyList_SetItem(PyObject*, Py_ssize_t, PyObject*);
PyAPI_FUNC(int) PyList_SetSlice(PyObject*, Py_ssize_t, Py_ssize_t, PyObject*);
PyAPI_FUNC(Py_ssize_t) PyList_Size(PyObject*);
PyAPI_FUNC(int) PyList_Sort(PyObject*);
PyAPI_FUNC(double) PyLong_AsDouble(PyObject*);
PyAPI_FUNC(long) PyLong_AsLong(PyObject*);
PyAPI_FUNC(long) PyLong_AsLongAndOverflow(PyObject*, int*);
PyAPI_FUNC(long long) PyLong_AsLongLong(PyObject*);
PyAPI_FUNC(long long) PyLong_AsLongLongAndOverflow(PyObject*, int*);
PyAPI_FUNC(pid_t) PyLong_AsPid(PyObject*);
PyAPI_FUNC(size_t) PyLong_AsSize_t(PyObject*);
PyAPI_FUNC(Py_ssize_t) PyLong_AsSsize_t(PyObject*);
PyAPI_FUNC(unsigned long) PyLong_AsUnsignedLong(PyObject*);
PyAPI_FUNC(unsigned long long) PyLong_AsUnsignedLongLong(PyObject*);
PyAPI_FUNC(unsigned long long) PyLong_AsUnsignedLongLongMask(PyObject*);
PyAPI_FUNC(unsigned long) PyLong_AsUnsignedLongMask(PyObject*);
PyAPI_FUNC(void*) PyLong_AsVoidPtr(PyObject*);
PyAPI_FUNC(PyObject*) PyLong_FromDouble(double);
PyAPI_FUNC(PyObject*) PyLong_FromLong(long);
PyAPI_FUNC(PyObject*) PyLong_FromLongLong(long long);
PyAPI_FUNC(PyObject*) PyLong_FromPid(pid_t);
PyAPI_FUNC(PyObject*) PyLong_FromSize_t(size_t);
PyAPI_FUNC(PyObject*) PyLong_FromSsize_t(Py_ssize_t);
PyAPI_FUNC(PyObject*) PyLong_FromString(const char*, char**, int);
PyAPI_FUNC(PyObject*) PyLong_FromUnsignedLong(unsigned long);
PyAPI_FUNC(PyObject*) PyLong_FromUnsignedLongLong(unsigned long long);
PyAPI_FUNC(PyObject*) PyLong_FromVoidPtr(void*);
PyAPI_FUNC(PyObject*) PyLong_GetInfo(void);
PyAPI_FUNC(int) PyMapping_Check(PyObject*);
PyAPI_FUNC(int) PyMapping_DelItem(PyObject*, PyObject*);
PyAPI_FUNC(int) PyMapping_DelItemString(PyObject*, const char*);
PyAPI_FUNC(PyObject*) PyMapping_GetItemString(PyObject*, const char*);
PyAPI_FUNC(int) PyMapping_HasKey(PyObject*, PyObject*);
PyAPI_FUNC(int) PyMapping_HasKeyString(PyObject*, const char*);
PyAPI_FUNC(PyObject*) PyMapping_Items(PyObject*);
PyAPI_FUNC(PyObject*) PyMapping_Keys(PyObject*);
PyAPI_FUNC(Py_ssize_t) PyMapping_Length(PyObject*);
PyAPI_FUNC(int) PyMapping_SetItemString(PyObject*, const char*, PyObject*);
PyAPI_FUNC(Py_ssize_t) PyMapping_Size(PyObject*);
PyAPI_FUNC(PyObject*) PyMapping_Values(PyObject*);
PyAPI_FUNC(void*) PyMem_Calloc(size_t, size_t);
PyAPI_FUNC(void) PyMem_Del(void*);
PyAPI_FUNC(void) PyMem_Free(void*);
PyAPI_FUNC(void*) PyMem_Malloc(size_t);
PyAPI_FUNC(void*) PyMem_RawCalloc(size_t, size_t);
PyAPI_FUNC(void) PyMem_RawFree(void*);
PyAPI_FUNC(void*) PyMem_RawMalloc(size_t);
PyAPI_FUNC(void*) PyMem_RawRealloc(void*, size_t);
PyAPI_FUNC(void*) PyMem_Realloc(void*, size_t);
PyAPI_FUNC(PyObject*) PyMemoryView_FromMemory(char*, Py_ssize_t, int);
PyAPI_FUNC(PyObject*) PyMemoryView_FromObject(PyObject*);
PyAPI_FUNC(PyObject*) PyMemoryView_GetContiguous(PyObject*, int, char);
PyAPI_FUNC(PyObject*) PyMethod_Function(PyObject*);
PyAPI_FUNC(PyObject*) PyMethod_GET_FUNCTION_Func(PyObject*);
PyAPI_FUNC(PyObject*) PyMethod_GET_SELF_Func(PyObject*);
PyAPI_FUNC(PyObject*) PyMethod_New(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyMethod_Self(PyObject*);
PyAPI_FUNC(PyObject*) PyModuleDef_Init(struct PyModuleDef*);
PyAPI_FUNC(int) PyModule_AddFunctions(PyObject*, PyMethodDef*);
PyAPI_FUNC(int) PyModule_AddIntConstant(PyObject*, const char*, long);
PyAPI_FUNC(int) PyModule_AddObject(PyObject*, const char*, PyObject*);
PyAPI_FUNC(int) PyModule_AddStringConstant(PyObject*, const char*, const char*);
PyAPI_FUNC(PyObject*) PyModule_Create2(struct PyModuleDef*, int);
PyAPI_FUNC(int) PyModule_ExecDef(PyObject*, PyModuleDef*);
PyAPI_FUNC(PyObject*)
    PyModule_FromDefAndSpec2(struct PyModuleDef*, PyObject*, int);
PyAPI_FUNC(PyModuleDef*) PyModule_GetDef(PyObject*);
PyAPI_FUNC(PyObject*) PyModule_GetDict(PyObject*);
PyAPI_FUNC(const char*) PyModule_GetFilename(PyObject*);
PyAPI_FUNC(PyObject*) PyModule_GetFilenameObject(PyObject*);
PyAPI_FUNC(const char*) PyModule_GetName(PyObject*);
PyAPI_FUNC(PyObject*) PyModule_GetNameObject(PyObject*);
PyAPI_FUNC(void*) PyModule_GetState(PyObject*);
PyAPI_FUNC(PyObject*) PyModule_New(const char*);
PyAPI_FUNC(PyObject*) PyModule_NewObject(PyObject*);
PyAPI_FUNC(int) PyModule_SetDocString(PyObject*, const char*);
PyAPI_FUNC(PyCodeObject*) PyNode_Compile(struct _node*, const char*);
PyAPI_FUNC(PyObject*) PyNumber_Absolute(PyObject*);
PyAPI_FUNC(PyObject*) PyNumber_Add(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyNumber_And(PyObject*, PyObject*);
PyAPI_FUNC(Py_ssize_t) PyNumber_AsSsize_t(PyObject*, PyObject*);
PyAPI_FUNC(int) PyNumber_Check(PyObject*);
PyAPI_FUNC(PyObject*) PyNumber_Divmod(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyNumber_Float(PyObject*);
PyAPI_FUNC(PyObject*) PyNumber_FloorDivide(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyNumber_InPlaceAdd(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyNumber_InPlaceAnd(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyNumber_InPlaceFloorDivide(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyNumber_InPlaceLshift(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyNumber_InPlaceMatrixMultiply(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyNumber_InPlaceMultiply(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyNumber_InPlaceOr(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyNumber_InPlacePower(PyObject*, PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyNumber_InPlaceRemainder(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyNumber_InPlaceRshift(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyNumber_InPlaceSubtract(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyNumber_InPlaceTrueDivide(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyNumber_InPlaceXor(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyNumber_Index(PyObject*);
PyAPI_FUNC(PyObject*) PyNumber_Invert(PyObject*);
PyAPI_FUNC(PyObject*) PyNumber_Long(PyObject*);
PyAPI_FUNC(PyObject*) PyNumber_Lshift(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyNumber_MatrixMultiply(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyNumber_Multiply(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyNumber_Negative(PyObject*);
PyAPI_FUNC(PyObject*) PyNumber_Or(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyNumber_Positive(PyObject*);
PyAPI_FUNC(PyObject*) PyNumber_Power(PyObject*, PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyNumber_Remainder(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyNumber_Rshift(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyNumber_Subtract(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyNumber_ToBase(PyObject*, int);
PyAPI_FUNC(PyObject*) PyNumber_TrueDivide(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyNumber_Xor(PyObject*, PyObject*);
PyAPI_FUNC(int) PyODict_DelItem(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyODict_New(void);
PyAPI_FUNC(int) PyODict_SetItem(PyObject*, PyObject*, PyObject*);
PyAPI_FUNC(void) PyOS_AfterFork(void);
PyAPI_FUNC(void) PyOS_AfterFork_Child(void);
PyAPI_FUNC(void) PyOS_AfterFork_Parent(void);
PyAPI_FUNC(void) PyOS_BeforeFork(void);
PyAPI_FUNC(int) PyOS_CheckStack(void);
PyAPI_FUNC(PyObject*) PyOS_FSPath(PyObject*);
PyAPI_FUNC(void) PyOS_InitInterrupts(void);
PyAPI_FUNC(int) PyOS_InterruptOccurred(void);
PyAPI_FUNC(char*) PyOS_double_to_string(double, char, int, int, int*);
PyAPI_FUNC(PyOS_sighandler_t) PyOS_getsig(int);
PyAPI_FUNC(int) PyOS_mystricmp(const char*, const char*);
PyAPI_FUNC(int) PyOS_mystrnicmp(const char*, const char*, Py_ssize_t);
PyAPI_FUNC(PyOS_sighandler_t) PyOS_setsig(int, PyOS_sighandler_t);
PyAPI_FUNC(int) PyOS_snprintf(char*, size_t, const char*, ...);
PyAPI_FUNC(double) PyOS_string_to_double(const char*, char**, PyObject*);
PyAPI_FUNC(long) PyOS_strtol(const char*, char**, int);
PyAPI_FUNC(unsigned long) PyOS_strtoul(const char*, char**, int);
PyAPI_FUNC(int) PyOS_vsnprintf(char*, size_t, const char*, va_list);
PyAPI_FUNC(PyObject*) PyObject_ASCII(PyObject*);
PyAPI_FUNC(int) PyObject_AsCharBuffer(PyObject*, const char**, Py_ssize_t*);
PyAPI_FUNC(int) PyObject_AsFileDescriptor(PyObject*);
PyAPI_FUNC(int) PyObject_AsReadBuffer(PyObject*, const void**, Py_ssize_t*);
PyAPI_FUNC(int) PyObject_AsWriteBuffer(PyObject*, void**, Py_ssize_t*);
PyAPI_FUNC(PyObject*) PyObject_Bytes(PyObject*);
PyAPI_FUNC(PyObject*) PyObject_Call(PyObject*, PyObject*, PyObject*);
PyAPI_FUNC(int) PyObject_CallFinalizerFromDealloc(PyObject*);
PyAPI_FUNC(PyObject*) PyObject_CallFunction(PyObject*, const char*, ...);
PyAPI_FUNC(PyObject*) PyObject_CallFunctionObjArgs(PyObject*, ...);
PyAPI_FUNC(PyObject*)
    PyObject_CallMethod(PyObject*, const char*, const char*, ...);
PyAPI_FUNC(PyObject*) PyObject_CallMethodObjArgs(PyObject*, PyObject*, ...);
PyAPI_FUNC(PyObject*) PyObject_CallObject(PyObject*, PyObject*);
PyAPI_FUNC(void*) PyObject_Calloc(size_t, size_t);
PyAPI_FUNC(int) PyObject_CheckReadBuffer(PyObject*);
PyAPI_FUNC(void) PyObject_ClearWeakRefs(PyObject*);
PyAPI_FUNC(int) PyObject_DelAttr(PyObject*, PyObject*);
PyAPI_FUNC(int) PyObject_DelAttrString(PyObject*, const char*);
PyAPI_FUNC(int) PyObject_DelItem(PyObject*, PyObject*);
PyAPI_FUNC(int) PyObject_DelItemString(PyObject*, const char*);
PyAPI_FUNC(PyObject*) PyObject_Dir(PyObject*);
PyAPI_FUNC(PyObject*) PyObject_Format(PyObject*, PyObject*);
PyAPI_FUNC(void) PyObject_Free(void*);
PyAPI_FUNC(void) PyObject_GC_Del(void*);
PyAPI_FUNC(void) PyObject_GC_Track(void*);
PyAPI_FUNC(void) PyObject_GC_UnTrack(void*);
PyAPI_FUNC(PyObject*) PyObject_GenericGetAttr(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyObject_GenericGetDict(PyObject*, void*);
PyAPI_FUNC(int) PyObject_GenericSetAttr(PyObject*, PyObject*, PyObject*);
PyAPI_FUNC(int) PyObject_GenericSetDict(PyObject*, PyObject*, void*);
PyAPI_FUNC(PyObject*) PyObject_GetAttr(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyObject_GetAttrString(PyObject*, const char*);
PyAPI_FUNC(int) PyObject_GetBuffer(PyObject*, Py_buffer*, int);
PyAPI_FUNC(PyObject*) PyObject_GetItem(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyObject_GetIter(PyObject*);
PyAPI_FUNC(int) PyObject_HasAttr(PyObject*, PyObject*);
PyAPI_FUNC(int) PyObject_HasAttrString(PyObject*, const char*);
PyAPI_FUNC(Py_hash_t) PyObject_Hash(PyObject*);
PyAPI_FUNC(Py_hash_t) PyObject_HashNotImplemented(PyObject*);
PyAPI_FUNC(PyObject*) PyObject_Init(PyObject*, PyTypeObject*);
PyAPI_FUNC(PyVarObject*)
    PyObject_InitVar(PyVarObject*, PyTypeObject*, Py_ssize_t);
PyAPI_FUNC(int) PyObject_IsInstance(PyObject*, PyObject*);
PyAPI_FUNC(int) PyObject_IsSubclass(PyObject*, PyObject*);
PyAPI_FUNC(int) PyObject_IsTrue(PyObject*);
PyAPI_FUNC(Py_ssize_t) PyObject_Length(PyObject*);
PyAPI_FUNC(Py_ssize_t) PyObject_LengthHint(PyObject*, Py_ssize_t);
PyAPI_FUNC(void*) PyObject_Malloc(size_t);
PyAPI_FUNC(int) PyObject_Not(PyObject*);
PyAPI_FUNC(int) PyObject_Print(PyObject*, FILE*, int);
PyAPI_FUNC(void*) PyObject_Realloc(void*, size_t);
PyAPI_FUNC(PyObject*) PyObject_Repr(PyObject*);
PyAPI_FUNC(PyObject*) PyObject_RichCompare(PyObject*, PyObject*, int);
PyAPI_FUNC(int) PyObject_RichCompareBool(PyObject*, PyObject*, int);
PyAPI_FUNC(PyObject*) PyObject_SelfIter(PyObject*);
PyAPI_FUNC(int) PyObject_SetAttr(PyObject*, PyObject*, PyObject*);
PyAPI_FUNC(int) PyObject_SetAttrString(PyObject*, const char*, PyObject*);
PyAPI_FUNC(int) PyObject_SetItem(PyObject*, PyObject*, PyObject*);
PyAPI_FUNC(Py_ssize_t) PyObject_Size(PyObject*);
PyAPI_FUNC(PyObject*) PyObject_Str(PyObject*);
PyAPI_FUNC(PyObject*) PyObject_Type(PyObject*);
PyAPI_FUNC(const char*) PyObject_TypeName(PyObject*);
PyAPI_FUNC(struct _mod*)
    PyParser_ASTFromFile(FILE*, const char*, const char*, int, const char*,
                         const char*, PyCompilerFlags*, int*, PyArena*);
PyAPI_FUNC(struct _mod*)
    PyParser_ASTFromFileObject(FILE*, PyObject*, const char*, int, const char*,
                               const char*, PyCompilerFlags*, int*, PyArena*);
PyAPI_FUNC(struct _mod*) PyParser_ASTFromString(const char*, const char*, int,
                                                PyCompilerFlags*, PyArena*);
PyAPI_FUNC(struct _mod*)
    PyParser_ASTFromStringObject(const char*, PyObject*, int, PyCompilerFlags*,
                                 PyArena*);
PyAPI_FUNC(struct _node*)
    PyParser_SimpleParseFileFlags(FILE*, const char*, int, int);
PyAPI_FUNC(struct _node*)
    PyParser_SimpleParseStringFlags(const char*, int, int);
PyAPI_FUNC(struct _node*)
    PyParser_SimpleParseStringFlagsFilename(const char*, const char*, int, int);
PyAPI_FUNC(int) PyRun_AnyFile(FILE*, const char*);
PyAPI_FUNC(int) PyRun_AnyFileEx(FILE*, const char*, int);
PyAPI_FUNC(int) PyRun_AnyFileExFlags(FILE*, const char*, int, PyCompilerFlags*);
PyAPI_FUNC(int) PyRun_AnyFileFlags(FILE*, const char*, PyCompilerFlags*);
PyAPI_FUNC(PyObject*) PyRun_File(FILE*, const char*, int, PyObject*, PyObject*);
PyAPI_FUNC(PyObject*)
    PyRun_FileEx(FILE*, const char*, int, PyObject*, PyObject*, int);
PyAPI_FUNC(PyObject*) PyRun_FileExFlags(FILE*, const char*, int, PyObject*,
                                        PyObject*, int, PyCompilerFlags*);
PyAPI_FUNC(PyObject*) PyRun_FileFlags(FILE*, const char*, int, PyObject*,
                                      PyObject*, PyCompilerFlags*);
PyAPI_FUNC(int) PyRun_InteractiveLoop(FILE*, const char*);
PyAPI_FUNC(int)
    PyRun_InteractiveLoopFlags(FILE*, const char*, PyCompilerFlags*);
PyAPI_FUNC(int) PyRun_SimpleFile(FILE*, const char*);
PyAPI_FUNC(int) PyRun_SimpleFileEx(FILE*, const char*, int);
PyAPI_FUNC(int)
    PyRun_SimpleFileExFlags(FILE*, const char*, int, PyCompilerFlags*);
PyAPI_FUNC(int) PyRun_SimpleString(const char*);
PyAPI_FUNC(int) PyRun_SimpleStringFlags(const char*, PyCompilerFlags*);
PyAPI_FUNC(PyObject*) PyRun_String(const char*, int, PyObject*, PyObject*);
PyAPI_FUNC(PyObject*)
    PyRun_StringFlags(const char*, int, PyObject*, PyObject*, PyCompilerFlags*);
PyAPI_FUNC(PyObject*) PySeqIter_New(PyObject*);
PyAPI_FUNC(int) PySequence_Check(PyObject*);
PyAPI_FUNC(PyObject*) PySequence_Concat(PyObject*, PyObject*);
PyAPI_FUNC(int) PySequence_Contains(PyObject*, PyObject*);
PyAPI_FUNC(Py_ssize_t) PySequence_Count(PyObject*, PyObject*);
PyAPI_FUNC(int) PySequence_DelItem(PyObject*, Py_ssize_t);
PyAPI_FUNC(int) PySequence_DelSlice(PyObject*, Py_ssize_t, Py_ssize_t);
PyAPI_FUNC(PyObject*) PySequence_Fast(PyObject*, const char*);
PyAPI_FUNC(PyObject*) PySequence_GetItem(PyObject*, Py_ssize_t);
PyAPI_FUNC(PyObject*) PySequence_GetSlice(PyObject*, Py_ssize_t, Py_ssize_t);
PyAPI_FUNC(int) PySequence_In(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PySequence_InPlaceConcat(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PySequence_InPlaceRepeat(PyObject*, Py_ssize_t);
PyAPI_FUNC(Py_ssize_t) PySequence_Index(PyObject*, PyObject*);
PyAPI_FUNC(Py_ssize_t) PySequence_Length(PyObject*);
PyAPI_FUNC(PyObject*) PySequence_List(PyObject*);
PyAPI_FUNC(PyObject*) PySequence_Repeat(PyObject*, Py_ssize_t);
PyAPI_FUNC(int) PySequence_SetItem(PyObject*, Py_ssize_t, PyObject*);
PyAPI_FUNC(int)
    PySequence_SetSlice(PyObject*, Py_ssize_t, Py_ssize_t, PyObject*);
PyAPI_FUNC(Py_ssize_t) PySequence_Size(PyObject*);
PyAPI_FUNC(PyObject*) PySequence_Tuple(PyObject*);
PyAPI_FUNC(int) PySet_Add(PyObject*, PyObject*);
PyAPI_FUNC(int) PySet_Clear(PyObject*);
PyAPI_FUNC(int) PySet_Contains(PyObject*, PyObject*);
PyAPI_FUNC(int) PySet_Discard(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PySet_New(PyObject*);
PyAPI_FUNC(PyObject*) PySet_Pop(PyObject*);
PyAPI_FUNC(Py_ssize_t) PySet_Size(PyObject*);
PyAPI_FUNC(Py_ssize_t)
    PySlice_AdjustIndices(Py_ssize_t, Py_ssize_t*, Py_ssize_t*, Py_ssize_t);
PyAPI_FUNC(int) PySlice_GetIndices(PyObject*, Py_ssize_t, Py_ssize_t*,
                                   Py_ssize_t*, Py_ssize_t*);
PyAPI_FUNC(int) PySlice_GetIndicesEx(PyObject*, Py_ssize_t, Py_ssize_t*,
                                     Py_ssize_t*, Py_ssize_t*, Py_ssize_t*);
PyAPI_FUNC(PyObject*) PySlice_New(PyObject*, PyObject*, PyObject*);
PyAPI_FUNC(int)
    PySlice_Unpack(PyObject*, Py_ssize_t*, Py_ssize_t*, Py_ssize_t*);
PyAPI_FUNC(int) PyState_AddModule(PyObject*, struct PyModuleDef*);
PyAPI_FUNC(PyObject*) PyState_FindModule(struct PyModuleDef*);
PyAPI_FUNC(int) PyState_RemoveModule(struct PyModuleDef*);
PyAPI_FUNC(PyObject*) PyStaticMethod_New(PyObject*);
PyAPI_FUNC(PyObject*) PyStructSequence_GetItem(PyObject*, Py_ssize_t);
PyAPI_FUNC(int)
    PyStructSequence_InitType2(PyTypeObject*, PyStructSequence_Desc*);
PyAPI_FUNC(PyObject*) PyStructSequence_New(PyTypeObject*);
PyAPI_FUNC(PyTypeObject*) PyStructSequence_NewType(PyStructSequence_Desc*);
PyAPI_FUNC(void) PyStructSequence_SetItem(PyObject*, Py_ssize_t, PyObject*);
PyAPI_FUNC(void) PySys_AddWarnOption(const wchar_t*);
PyAPI_FUNC(void) PySys_AddWarnOptionUnicode(PyObject*);
PyAPI_FUNC(void) PySys_AddXOption(const wchar_t*);
PyAPI_FUNC(void) PySys_FormatStderr(const char*, ...);
PyAPI_FUNC(void) PySys_FormatStdout(const char*, ...);
PyAPI_FUNC(PyObject*) PySys_GetObject(const char*);
PyAPI_FUNC(PyObject*) PySys_GetXOptions(void);
PyAPI_FUNC(int) PySys_HasWarnOptions(void);
PyAPI_FUNC(void) PySys_ResetWarnOptions(void);
PyAPI_FUNC(void) PySys_SetArgv(int, wchar_t**);
PyAPI_FUNC(void) PySys_SetArgvEx(int, wchar_t**, int);
PyAPI_FUNC(int) PySys_SetObject(const char*, PyObject*);
PyAPI_FUNC(void) PySys_SetPath(const wchar_t*);
PyAPI_FUNC(void) PySys_WriteStderr(const char*, ...);
PyAPI_FUNC(void) PySys_WriteStdout(const char*, ...);
PyAPI_FUNC(void) PyThreadState_Clear(PyThreadState*);
PyAPI_FUNC(void) PyThreadState_Delete(PyThreadState*);
PyAPI_FUNC(void) PyThreadState_DeleteCurrent(void);
PyAPI_FUNC(PyThreadState*) PyThreadState_Get(void);
PyAPI_FUNC(PyObject*) PyThreadState_GetDict(void);
PyAPI_FUNC(PyThreadState*) PyThreadState_New(PyInterpreterState*);
PyAPI_FUNC(PyThreadState*) PyThreadState_Next(PyThreadState*);
PyAPI_FUNC(int) PyThreadState_SetAsyncExc(unsigned long, PyObject*);
PyAPI_FUNC(PyThreadState*) PyThreadState_Swap(PyThreadState*);
PyAPI_FUNC(PyObject*) PyThread_GetInfo(void);
PyAPI_FUNC(void) PyThread_ReInitTLS(void);
PyAPI_FUNC(int) PyThread_acquire_lock(PyThread_type_lock, int);
PyAPI_FUNC(PyLockStatus)
    PyThread_acquire_lock_timed(PyThread_type_lock, PY_TIMEOUT_T, int);
PyAPI_FUNC(PyThread_type_lock) PyThread_allocate_lock(void);
PyAPI_FUNC(int) PyThread_create_key(void);
PyAPI_FUNC(void) PyThread_delete_key(int);
PyAPI_FUNC(void) PyThread_delete_key_value(int);
PyAPI_FUNC(void) PyThread_exit_thread(void);
PyAPI_FUNC(void) PyThread_free_lock(PyThread_type_lock);
PyAPI_FUNC(void*) PyThread_get_key_value(int);
PyAPI_FUNC(size_t) PyThread_get_stacksize(void);
PyAPI_FUNC(unsigned long) PyThread_get_thread_ident(void);
PyAPI_FUNC(void) PyThread_init_thread(void);
PyAPI_FUNC(void) PyThread_release_lock(PyThread_type_lock);
PyAPI_FUNC(int) PyThread_set_key_value(int, void*);
PyAPI_FUNC(int) PyThread_set_stacksize(size_t);
PyAPI_FUNC(long) PyThread_start_new_thread(void (*)(void*), void*);
PyAPI_FUNC(int) PyTraceBack_Here(PyFrameObject*);
PyAPI_FUNC(int) PyTraceBack_Print(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyTuple_GetItem(PyObject*, Py_ssize_t);
PyAPI_FUNC(PyObject*) PyTuple_GetSlice(PyObject*, Py_ssize_t, Py_ssize_t);
PyAPI_FUNC(PyObject*) PyTuple_New(Py_ssize_t);
PyAPI_FUNC(PyObject*) PyTuple_Pack(Py_ssize_t, ...);
PyAPI_FUNC(int) PyTuple_SetItem(PyObject*, Py_ssize_t, PyObject*);
PyAPI_FUNC(Py_ssize_t) PyTuple_Size(PyObject*);
PyAPI_FUNC(unsigned int) PyType_ClearCache(void);
PyAPI_FUNC(PyObject*) PyType_FromSpec(PyType_Spec*);
PyAPI_FUNC(PyObject*) PyType_FromSpecWithBases(PyType_Spec*, PyObject*);
PyAPI_FUNC(PyObject*) PyType_GenericAlloc(PyTypeObject*, Py_ssize_t);
PyAPI_FUNC(PyObject*) PyType_GenericNew(PyTypeObject*, PyObject*, PyObject*);
PyAPI_FUNC(unsigned long) PyType_GetFlags(PyTypeObject*);
PyAPI_FUNC(void*) PyType_GetSlot(PyTypeObject*, int);
PyAPI_FUNC(int) PyType_IsSubtype(PyTypeObject*, PyTypeObject*);
PyAPI_FUNC(void) PyType_Modified(PyTypeObject*);
PyAPI_FUNC(int) PyType_Ready(PyTypeObject*);
PyAPI_FUNC(PyObject*)
    PyUnicodeDecodeError_Create(const char*, const char*, Py_ssize_t,
                                Py_ssize_t, Py_ssize_t, const char*);
PyAPI_FUNC(PyObject*) PyUnicodeDecodeError_GetEncoding(PyObject*);
PyAPI_FUNC(int) PyUnicodeDecodeError_GetEnd(PyObject*, Py_ssize_t*);
PyAPI_FUNC(PyObject*) PyUnicodeDecodeError_GetObject(PyObject*);
PyAPI_FUNC(PyObject*) PyUnicodeDecodeError_GetReason(PyObject*);
PyAPI_FUNC(int) PyUnicodeDecodeError_GetStart(PyObject*, Py_ssize_t*);
PyAPI_FUNC(int) PyUnicodeDecodeError_SetEnd(PyObject*, Py_ssize_t);
PyAPI_FUNC(int) PyUnicodeDecodeError_SetReason(PyObject*, const char*);
PyAPI_FUNC(int) PyUnicodeDecodeError_SetStart(PyObject*, Py_ssize_t);
PyAPI_FUNC(PyObject*) PyUnicodeEncodeError_GetEncoding(PyObject*);
PyAPI_FUNC(int) PyUnicodeEncodeError_GetEnd(PyObject*, Py_ssize_t*);
PyAPI_FUNC(PyObject*) PyUnicodeEncodeError_GetObject(PyObject*);
PyAPI_FUNC(PyObject*) PyUnicodeEncodeError_GetReason(PyObject*);
PyAPI_FUNC(int) PyUnicodeEncodeError_GetStart(PyObject*, Py_ssize_t*);
PyAPI_FUNC(int) PyUnicodeEncodeError_SetEnd(PyObject*, Py_ssize_t);
PyAPI_FUNC(int) PyUnicodeEncodeError_SetReason(PyObject*, const char*);
PyAPI_FUNC(int) PyUnicodeEncodeError_SetStart(PyObject*, Py_ssize_t);
PyAPI_FUNC(int) PyUnicodeTranslateError_GetEnd(PyObject*, Py_ssize_t*);
PyAPI_FUNC(PyObject*) PyUnicodeTranslateError_GetObject(PyObject*);
PyAPI_FUNC(PyObject*) PyUnicodeTranslateError_GetReason(PyObject*);
PyAPI_FUNC(int) PyUnicodeTranslateError_GetStart(PyObject*, Py_ssize_t*);
PyAPI_FUNC(int) PyUnicodeTranslateError_SetEnd(PyObject*, Py_ssize_t);
PyAPI_FUNC(int) PyUnicodeTranslateError_SetReason(PyObject*, const char*);
PyAPI_FUNC(int) PyUnicodeTranslateError_SetStart(PyObject*, Py_ssize_t);
PyAPI_FUNC(void) PyUnicode_Append(PyObject**, PyObject*);
PyAPI_FUNC(void) PyUnicode_AppendAndDel(PyObject**, PyObject*);
PyAPI_FUNC(PyObject*) PyUnicode_AsASCIIString(PyObject*);
PyAPI_FUNC(PyObject*) PyUnicode_AsCharmapString(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*)
    PyUnicode_AsDecodedObject(PyObject*, const char*, const char*);
PyAPI_FUNC(PyObject*)
    PyUnicode_AsDecodedUnicode(PyObject*, const char*, const char*);
PyAPI_FUNC(PyObject*)
    PyUnicode_AsEncodedObject(PyObject*, const char*, const char*);
PyAPI_FUNC(PyObject*)
    PyUnicode_AsEncodedString(PyObject*, const char*, const char*);
PyAPI_FUNC(PyObject*)
    PyUnicode_AsEncodedUnicode(PyObject*, const char*, const char*);
PyAPI_FUNC(PyObject*) PyUnicode_AsLatin1String(PyObject*);
PyAPI_FUNC(PyObject*) PyUnicode_AsMBCSString(PyObject*);
PyAPI_FUNC(PyObject*) PyUnicode_AsRawUnicodeEscapeString(PyObject*);
PyAPI_FUNC(Py_UCS4*) PyUnicode_AsUCS4(PyObject*, Py_UCS4*, Py_ssize_t, int);
PyAPI_FUNC(Py_UCS4*) PyUnicode_AsUCS4Copy(PyObject*);
PyAPI_FUNC(PyObject*) PyUnicode_AsUTF16String(PyObject*);
PyAPI_FUNC(PyObject*) PyUnicode_AsUTF32String(PyObject*);
PyAPI_FUNC(const char*) PyUnicode_AsUTF8(PyObject*);
PyAPI_FUNC(const char*) PyUnicode_AsUTF8AndSize(PyObject*, Py_ssize_t*);
PyAPI_FUNC(PyObject*) PyUnicode_AsUTF8String(PyObject*);
PyAPI_FUNC(Py_UNICODE*) PyUnicode_AsUnicode(PyObject*);
PyAPI_FUNC(Py_UNICODE*) PyUnicode_AsUnicodeAndSize(PyObject*, Py_ssize_t*);
PyAPI_FUNC(PyObject*) PyUnicode_AsUnicodeEscapeString(PyObject*);
PyAPI_FUNC(Py_ssize_t) PyUnicode_AsWideChar(PyObject*, wchar_t*, Py_ssize_t);
PyAPI_FUNC(wchar_t*) PyUnicode_AsWideCharString(PyObject*, Py_ssize_t*);
PyAPI_FUNC(PyObject*) PyUnicode_BuildEncodingMap(PyObject*);
PyAPI_FUNC(int) PyUnicode_Compare(PyObject*, PyObject*);
PyAPI_FUNC(int) PyUnicode_CompareWithASCIIString(PyObject*, const char*);
PyAPI_FUNC(PyObject*) PyUnicode_Concat(PyObject*, PyObject*);
PyAPI_FUNC(int) PyUnicode_Contains(PyObject*, PyObject*);
PyAPI_FUNC(Py_ssize_t)
    PyUnicode_Count(PyObject*, PyObject*, Py_ssize_t, Py_ssize_t);
PyAPI_FUNC(PyObject*)
    PyUnicode_Decode(const char*, Py_ssize_t, const char*, const char*);
PyAPI_FUNC(PyObject*)
    PyUnicode_DecodeASCII(const char*, Py_ssize_t, const char*);
PyAPI_FUNC(PyObject*)
    PyUnicode_DecodeCharmap(const char*, Py_ssize_t, PyObject*, const char*);
PyAPI_FUNC(PyObject*)
    PyUnicode_DecodeCodePageStateful(int, const char*, Py_ssize_t, const char*,
                                     Py_ssize_t*);
PyAPI_FUNC(PyObject*) PyUnicode_DecodeFSDefault(const char*);
PyAPI_FUNC(PyObject*) PyUnicode_DecodeFSDefaultAndSize(const char*, Py_ssize_t);
PyAPI_FUNC(PyObject*)
    PyUnicode_DecodeLatin1(const char*, Py_ssize_t, const char*);
PyAPI_FUNC(PyObject*) PyUnicode_DecodeLocale(const char*, const char*);
PyAPI_FUNC(PyObject*)
    PyUnicode_DecodeLocaleAndSize(const char*, Py_ssize_t, const char*);
PyAPI_FUNC(PyObject*)
    PyUnicode_DecodeMBCS(const char*, Py_ssize_t, const char*);
PyAPI_FUNC(PyObject*) PyUnicode_DecodeMBCSStateful(const char*, Py_ssize_t,
                                                   const char*, Py_ssize_t*);
PyAPI_FUNC(PyObject*)
    PyUnicode_DecodeRawUnicodeEscape(const char*, Py_ssize_t, const char*);
PyAPI_FUNC(PyObject*)
    PyUnicode_DecodeUTF16(const char*, Py_ssize_t, const char*, int*);
PyAPI_FUNC(PyObject*)
    PyUnicode_DecodeUTF16Stateful(const char*, Py_ssize_t, const char*, int*,
                                  Py_ssize_t*);
PyAPI_FUNC(PyObject*)
    PyUnicode_DecodeUTF32(const char*, Py_ssize_t, const char*, int*);
PyAPI_FUNC(PyObject*)
    PyUnicode_DecodeUTF32Stateful(const char*, Py_ssize_t, const char*, int*,
                                  Py_ssize_t*);
PyAPI_FUNC(PyObject*)
    PyUnicode_DecodeUTF7(const char*, Py_ssize_t, const char*);
PyAPI_FUNC(PyObject*) PyUnicode_DecodeUTF7Stateful(const char*, Py_ssize_t,
                                                   const char*, Py_ssize_t*);
PyAPI_FUNC(PyObject*)
    PyUnicode_DecodeUTF8(const char*, Py_ssize_t, const char*);
PyAPI_FUNC(PyObject*) PyUnicode_DecodeUTF8Stateful(const char*, Py_ssize_t,
                                                   const char*, Py_ssize_t*);
PyAPI_FUNC(PyObject*)
    PyUnicode_DecodeUnicodeEscape(const char*, Py_ssize_t, const char*);
PyAPI_FUNC(PyObject*) PyUnicode_EncodeCodePage(int, PyObject*, const char*);
PyAPI_FUNC(PyObject*) PyUnicode_EncodeFSDefault(PyObject*);
PyAPI_FUNC(PyObject*) PyUnicode_EncodeLocale(PyObject*, const char*);
PyAPI_FUNC(PyObject*)
    PyUnicode_EncodeUTF16(const Py_UNICODE*, Py_ssize_t, const char*, int);
PyAPI_FUNC(PyObject*)
    PyUnicode_EncodeUTF32(const Py_UNICODE*, Py_ssize_t, const char*, int);
PyAPI_FUNC(int) PyUnicode_FSConverter(PyObject*, void*);
PyAPI_FUNC(int) PyUnicode_FSDecoder(PyObject*, void*);
PyAPI_FUNC(Py_ssize_t)
    PyUnicode_Find(PyObject*, PyObject*, Py_ssize_t, Py_ssize_t, int);
PyAPI_FUNC(Py_ssize_t)
    PyUnicode_FindChar(PyObject*, Py_UCS4, Py_ssize_t, Py_ssize_t, int);
PyAPI_FUNC(PyObject*) PyUnicode_Format(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*)
    PyUnicode_FromEncodedObject(PyObject*, const char*, const char*);
PyAPI_FUNC(PyObject*) PyUnicode_FromFormat(const char*, ...);
PyAPI_FUNC(PyObject*) PyUnicode_FromFormatV(const char*, va_list);
PyAPI_FUNC(PyObject*) PyUnicode_FromKindAndData(int, const void*, Py_ssize_t);
PyAPI_FUNC(PyObject*) PyUnicode_FromObject(PyObject*);
PyAPI_FUNC(PyObject*) PyUnicode_FromOrdinal(int);
PyAPI_FUNC(PyObject*) PyUnicode_FromString(const char*);
PyAPI_FUNC(PyObject*) PyUnicode_FromStringAndSize(const char*, Py_ssize_t);
PyAPI_FUNC(PyObject*) PyUnicode_FromUnicode(const Py_UNICODE*, Py_ssize_t);
PyAPI_FUNC(PyObject*) PyUnicode_FromWideChar(const wchar_t*, Py_ssize_t);
PyAPI_FUNC(const char*) PyUnicode_GetDefaultEncoding(void);
PyAPI_FUNC(Py_ssize_t) PyUnicode_GetLength(PyObject*);
PyAPI_FUNC(Py_ssize_t) PyUnicode_GetSize(PyObject*);
PyAPI_FUNC(PyObject*) PyUnicode_InternFromString(const char*);
PyAPI_FUNC(void) PyUnicode_InternImmortal(PyObject**);
PyAPI_FUNC(void) PyUnicode_InternInPlace(PyObject**);
PyAPI_FUNC(int) PyUnicode_IsIdentifier(PyObject*);
PyAPI_FUNC(PyObject*) PyUnicode_Join(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyUnicode_New(Py_ssize_t, Py_UCS4);
PyAPI_FUNC(PyObject*) PyUnicode_Partition(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyUnicode_RPartition(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyUnicode_RSplit(PyObject*, PyObject*, Py_ssize_t);
PyAPI_FUNC(Py_UCS4) PyUnicode_ReadChar(PyObject*, Py_ssize_t);
PyAPI_FUNC(PyObject*)
    PyUnicode_Replace(PyObject*, PyObject*, PyObject*, Py_ssize_t);
PyAPI_FUNC(int) PyUnicode_Resize(PyObject**, Py_ssize_t);
PyAPI_FUNC(PyObject*) PyUnicode_RichCompare(PyObject*, PyObject*, int);
PyAPI_FUNC(PyObject*) PyUnicode_Split(PyObject*, PyObject*, Py_ssize_t);
PyAPI_FUNC(PyObject*) PyUnicode_Splitlines(PyObject*, int);
PyAPI_FUNC(PyObject*) PyUnicode_Substring(PyObject*, Py_ssize_t, Py_ssize_t);
PyAPI_FUNC(Py_ssize_t)
    PyUnicode_Tailmatch(PyObject*, PyObject*, Py_ssize_t, Py_ssize_t, int);
PyAPI_FUNC(PyObject*) PyUnicode_Translate(PyObject*, PyObject*, const char*);
PyAPI_FUNC(int) PyUnicode_WriteChar(PyObject*, Py_ssize_t, Py_UCS4);
PyAPI_FUNC(PyObject*) PyWeakref_GetObject(PyObject*);
PyAPI_FUNC(PyObject*) PyWeakref_NewProxy(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyWeakref_NewRef(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyWrapper_New(PyObject*, PyObject*);
PyAPI_FUNC(int) Py_AddPendingCall(int (*)(void*), void*);
PyAPI_FUNC(int) Py_AtExit(void (*)(void));
PyAPI_FUNC(PyObject*) Py_BuildValue(const char*, ...);
PyAPI_FUNC(int) Py_BytesMain(int, char**);
PyAPI_FUNC(PyObject*) Py_CompileString(const char*, const char*, int);
PyAPI_FUNC(void) Py_DecRef(PyObject*);
PyAPI_FUNC(wchar_t*) Py_DecodeLocale(const char*, size_t*);
PyAPI_FUNC(char*) Py_EncodeLocale(const wchar_t*, size_t*);
PyAPI_FUNC(void) Py_EndInterpreter(PyThreadState*);
PyAPI_FUNC(void) Py_Exit(int);
PyAPI_FUNC(void) Py_FatalError(const char*) _Py_NO_RETURN;
PyAPI_FUNC(void) Py_Finalize(void);
PyAPI_FUNC(int) Py_FdIsInteractive(FILE*, const char*);
PyAPI_FUNC(int) Py_FinalizeEx(void);
PyAPI_FUNC(const char*) Py_GetBuildInfo(void);
PyAPI_FUNC(const char*) Py_GetCompiler(void);
PyAPI_FUNC(const char*) Py_GetCopyright(void);
PyAPI_FUNC(wchar_t*) Py_GetExecPrefix(void);
PyAPI_FUNC(wchar_t*) Py_GetPath(void);
PyAPI_FUNC(const char*) Py_GetPlatform(void);
PyAPI_FUNC(wchar_t*) Py_GetPrefix(void);
PyAPI_FUNC(wchar_t*) Py_GetProgramFullPath(void);
PyAPI_FUNC(wchar_t*) Py_GetProgramName(void);
PyAPI_FUNC(wchar_t*) Py_GetPythonHome(void);
PyAPI_FUNC(int) Py_GetRecursionLimit(void);
PyAPI_FUNC(const char*) Py_GetVersion(void);
PyAPI_FUNC(void) Py_IncRef(PyObject*);
PyAPI_FUNC(void) Py_Initialize(void);
PyAPI_FUNC(void) Py_InitializeEx(int);
PyAPI_FUNC(int) Py_IsInitialized(void);
PyAPI_FUNC(int) Py_Main(int, wchar_t**);
PyAPI_FUNC(int) Py_MakePendingCalls(void);
PyAPI_FUNC(PyThreadState*) Py_NewInterpreter(void);
PyAPI_FUNC(int) Py_ReprEnter(PyObject*);
PyAPI_FUNC(void) Py_ReprLeave(PyObject*);
PyAPI_FUNC(void) Py_SetPath(const wchar_t*);
PyAPI_FUNC(void) Py_SetProgramName(wchar_t*);
PyAPI_FUNC(void) Py_SetPythonHome(wchar_t*);
PyAPI_FUNC(void) Py_SetRecursionLimit(int);
PyAPI_FUNC(struct symtable*) Py_SymtableString(const char*, const char*, int);
PyAPI_FUNC(struct symtable*)
    Py_SymtableStringObject(const char* str, PyObject* filename, int start);
PyAPI_FUNC(size_t) Py_UNICODE_strlen(const Py_UNICODE*);
PyAPI_FUNC(char*) Py_UniversalNewlineFgets(char*, int, FILE*, PyObject*);
PyAPI_FUNC(PyObject*) Py_VaBuildValue(const char*, va_list);
PyAPI_FUNC(void) _PyArg_Fini(void);
PyAPI_FUNC(int) _PyArg_NoKeywords(const char* funcname, PyObject* kw);
PyAPI_FUNC(int) _PyArg_NoPositional(const char* funcname, PyObject* args);
PyAPI_FUNC(int) _PyArg_ParseStack(PyObject* const* args, Py_ssize_t nargs,
                                  const char* format, ...);
PyAPI_FUNC(int) _PyArg_ParseStack_SizeT(PyObject* const* args, Py_ssize_t nargs,
                                        const char* format, ...);
PyAPI_FUNC(int)
    _PyArg_ParseStackAndKeywords(PyObject* const*, Py_ssize_t, PyObject*,
                                 struct _PyArg_Parser*, ...);
PyAPI_FUNC(int)
    _PyArg_ParseStackAndKeywords_SizeT(PyObject* const*, Py_ssize_t, PyObject*,
                                       struct _PyArg_Parser*, ...);
PyAPI_FUNC(int) _PyArg_ParseTupleAndKeywordsFast(PyObject*, PyObject*,
                                                 struct _PyArg_Parser*, ...);
PyAPI_FUNC(int)
    _PyArg_ParseTupleAndKeywordsFast_SizeT(PyObject*, PyObject*,
                                           struct _PyArg_Parser*, ...);
PyAPI_FUNC(int) _PyArg_ParseTupleAndKeywords_SizeT(PyObject*, PyObject*,
                                                   const char*, char**, ...);
PyAPI_FUNC(int)
    _PyArg_ParseTuple_SizeT(PyObject* args, const char* format, ...);
PyAPI_FUNC(int) _PyArg_Parse_SizeT(PyObject*, const char*, ...);
PyAPI_FUNC(int) _PyArg_UnpackStack(PyObject* const*, Py_ssize_t, const char*,
                                   Py_ssize_t, Py_ssize_t, ...);
PyAPI_FUNC(int)
    _PyArg_VaParseTupleAndKeywordsFast(PyObject*, PyObject*,
                                       struct _PyArg_Parser*, va_list);
PyAPI_FUNC(int)
    _PyArg_VaParseTupleAndKeywordsFast_SizeT(PyObject*, PyObject*,
                                             struct _PyArg_Parser*, va_list);
PyAPI_FUNC(int)
    _PyArg_VaParseTupleAndKeywords_SizeT(PyObject*, PyObject*, const char*,
                                         char**, va_list);
PyAPI_FUNC(int) _PyArg_VaParse_SizeT(PyObject*, const char*, va_list);
PyAPI_FUNC(void*) _PyBytesWriter_Alloc(_PyBytesWriter* writer, Py_ssize_t size);
PyAPI_FUNC(void) _PyBytesWriter_Dealloc(_PyBytesWriter* writer);
PyAPI_FUNC(PyObject*) _PyBytesWriter_Finish(_PyBytesWriter* writer, void* str);
PyAPI_FUNC(void) _PyBytesWriter_Init(_PyBytesWriter* writer);
PyAPI_FUNC(void*) _PyBytesWriter_Prepare(_PyBytesWriter* writer, void* str,
                                         Py_ssize_t growth);
PyAPI_FUNC(void*) _PyBytesWriter_Resize(_PyBytesWriter* writer, void* str,
                                        Py_ssize_t new_size);
PyAPI_FUNC(void*) _PyBytesWriter_WriteBytes(_PyBytesWriter* writer, void* str,
                                            const void* bytes, Py_ssize_t len);
PyAPI_FUNC(PyObject*)
    _PyBytes_DecodeEscape(const char*, Py_ssize_t, const char*, Py_ssize_t,
                          const char*, const char**);
PyAPI_FUNC(PyObject*) _PyBytes_Join(PyObject*, PyObject*);
PyAPI_FUNC(int) _PyBytes_Resize(PyObject**, Py_ssize_t);
PyAPI_FUNC(PyObject*)
    _PyCodecInfo_GetIncrementalDecoder(PyObject*, const char*);
PyAPI_FUNC(PyObject*)
    _PyCodecInfo_GetIncrementalEncoder(PyObject*, const char*);
PyAPI_FUNC(PyObject*) _PyCodec_LookupTextEncoding(const char*, const char*);
PyAPI_FUNC(PyObject*)
    _PyDict_GetItem_KnownHash(PyObject* pydict, PyObject* key, Py_hash_t hash);
PyAPI_FUNC(int)
    _PyDict_Next(PyObject*, Py_ssize_t*, PyObject**, PyObject**, Py_hash_t*);
PyAPI_FUNC(int) _PyDict_SetItem_KnownHash(PyObject* pydict, PyObject* key,
                                          PyObject* value, Py_hash_t hash);
PyAPI_FUNC(void) _PyErr_BadInternalCall(const char*, int);
PyAPI_FUNC(void) _PyErr_ChainExceptions(PyObject*, PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) _PyErr_FormatFromCause(PyObject*, const char*, ...);
PyAPI_FUNC(PyObject*) _PyEval_EvalFrameDefault(PyFrameObject*, int);
PyAPI_FUNC(int) _PyFloat_Pack2(double x, unsigned char* p, int le);
PyAPI_FUNC(int) _PyFloat_Pack4(double x, unsigned char* p, int le);
PyAPI_FUNC(int) _PyFloat_Pack8(double x, unsigned char* p, int le);
PyAPI_FUNC(double) _PyFloat_Unpack2(const unsigned char* p, int le);
PyAPI_FUNC(double) _PyFloat_Unpack4(const unsigned char* p, int le);
PyAPI_FUNC(double) _PyFloat_Unpack8(const unsigned char* p, int le);
PyAPI_FUNC(void) _PyGILState_Reinit(void);
PyAPI_FUNC(void) _PyImport_AcquireLock(void);
PyAPI_FUNC(void) _PyImport_ReInitLock(void);
PyAPI_FUNC(int) _PyImport_ReleaseLock(void);
PyAPI_FUNC(int)
    _PyLong_AsByteArray(PyLongObject*, unsigned char*, size_t, int, int);
PyAPI_FUNC(int) _PyLong_AsInt(PyObject*);
PyAPI_FUNC(time_t) _PyLong_AsTime_t(PyObject*);
PyAPI_FUNC(double) _PyLong_Frexp(PyLongObject*, Py_ssize_t*);
PyAPI_FUNC(PyObject*)
    _PyLong_FromByteArray(const unsigned char*, size_t, int, int);
PyAPI_FUNC(PyLongObject*) _PyLong_FromNbInt(PyObject*);
PyAPI_FUNC(PyObject*) _PyLong_FromTime_t(time_t);
PyAPI_FUNC(PyObject*) _PyLong_GCD(PyObject*, PyObject*);
PyAPI_FUNC(size_t) _PyLong_NumBits(PyObject*);
PyAPI_FUNC(int) _PyLong_Sign(PyObject*);
PyAPI_FUNC(char*) _PyMem_RawStrdup(const char*);
PyAPI_FUNC(char*) _PyMem_Strdup(const char*);
PyAPI_FUNC(PyObject*) _PyNamespace_New(PyObject* kwds);
PyAPI_FUNC(int) _PyOS_URandom(void*, Py_ssize_t);
PyAPI_FUNC(int) _PyOS_URandomNonblock(void*, Py_ssize_t);
PyAPI_FUNC(PyObject*)
    _PyObject_CallMethod_SizeT(PyObject*, const char*, const char*, ...);
PyAPI_FUNC(PyObject*) _PyObject_CallNoArg(PyObject*);
PyAPI_FUNC(PyObject*) _PyObject_FastCall(PyObject*, PyObject**, Py_ssize_t);
PyAPI_FUNC(PyObject*)
    _PyObject_FastCallDict(PyObject*, PyObject**, Py_ssize_t, PyObject*);
PyAPI_FUNC(PyObject*)
    _PyObject_FastCallKeywords(PyObject*, PyObject**, Py_ssize_t, PyObject*);
PyAPI_FUNC(PyObject*) _PyObject_GC_Calloc(size_t);
PyAPI_FUNC(PyObject*) _PyObject_GC_Malloc(size_t);
PyAPI_FUNC(PyObject*) _PyObject_GC_New(PyTypeObject*);
PyAPI_FUNC(PyVarObject*) _PyObject_GC_NewVar(PyTypeObject*, Py_ssize_t);
PyAPI_FUNC(PyVarObject*) _PyObject_GC_Resize(PyVarObject*, Py_ssize_t);
PyAPI_FUNC(PyObject*) _PyObject_GetAttrId(PyObject*, struct _Py_Identifier*);
PyAPI_FUNC(int) _PyObject_HasAttrId(PyObject*, struct _Py_Identifier*);
PyAPI_FUNC(int) _PyObject_LookupAttr(PyObject*, PyObject*, PyObject**);
PyAPI_FUNC(PyObject*)
    _PyObject_LookupSpecial(PyObject*, struct _Py_Identifier*);
PyAPI_FUNC(PyObject*) _PyObject_New(PyTypeObject*);
PyAPI_FUNC(PyVarObject*) _PyObject_NewVar(PyTypeObject*, Py_ssize_t);
PyAPI_FUNC(int)
    _PyObject_SetAttrId(PyObject*, struct _Py_Identifier*, PyObject*);
PyAPI_FUNC(char* const*) _PySequence_BytesToCharpArray(PyObject*);
PyAPI_FUNC(int) _PySet_NextEntry(PyObject* pyset, Py_ssize_t* ppos,
                                 PyObject** pkey, Py_hash_t* phash);
PyAPI_FUNC(void) _PySignal_AfterFork(void);
PyAPI_FUNC(int) _PyState_AddModule(PyObject*, struct PyModuleDef*);
PyAPI_FUNC(void) _PyState_ClearModules(void);
PyAPI_FUNC(size_t) _PySys_GetSizeOf(PyObject*);
PyAPI_FUNC(int) _PyThreadState_GetRecursionDepth(PyThreadState*);
PyAPI_FUNC(void) _PyThreadState_Init(PyThreadState*);
PyAPI_FUNC(PyThreadState*) _PyThreadState_Prealloc(PyInterpreterState*);
PyAPI_FUNC(_PyTime_t) _PyTime_AsMicroseconds(_PyTime_t, _PyTime_round_t);
PyAPI_FUNC(_PyTime_t) _PyTime_AsMilliseconds(_PyTime_t, _PyTime_round_t);
PyAPI_FUNC(PyObject*) _PyTime_AsNanosecondsObject(_PyTime_t);
PyAPI_FUNC(double) _PyTime_AsSecondsDouble(_PyTime_t);
PyAPI_FUNC(int) _PyTime_AsTimespec(_PyTime_t, struct timespec*);
PyAPI_FUNC(int) _PyTime_AsTimeval(_PyTime_t, struct timeval*, _PyTime_round_t);
PyAPI_FUNC(int)
    _PyTime_AsTimevalTime_t(_PyTime_t, time_t*, int*, _PyTime_round_t);
PyAPI_FUNC(int)
    _PyTime_AsTimeval_noraise(_PyTime_t, struct timeval*, _PyTime_round_t);
PyAPI_FUNC(int)
    _PyTime_FromMillisecondsObject(_PyTime_t*, PyObject*, _PyTime_round_t);
PyAPI_FUNC(_PyTime_t) _PyTime_FromNanoseconds(_PyTime_t ns);
PyAPI_FUNC(int) _PyTime_FromNanosecondsObject(_PyTime_t* t, PyObject* obj);
PyAPI_FUNC(_PyTime_t) _PyTime_FromSeconds(int seconds);
PyAPI_FUNC(int)
    _PyTime_FromSecondsObject(_PyTime_t*, PyObject*, _PyTime_round_t);
PyAPI_FUNC(int) _PyTime_FromTimespec(_PyTime_t* tp, struct timespec* ts);
PyAPI_FUNC(int) _PyTime_FromTimeval(_PyTime_t* tp, struct timeval* tv);
PyAPI_FUNC(_PyTime_t) _PyTime_GetMonotonicClock(void);
PyAPI_FUNC(int)
    _PyTime_GetMonotonicClockWithInfo(_PyTime_t*, _Py_clock_info_t*);
PyAPI_FUNC(_PyTime_t) _PyTime_GetPerfCounter(void);
PyAPI_FUNC(int)
    _PyTime_GetPerfCounterWithInfo(_PyTime_t* t, _Py_clock_info_t* info);
PyAPI_FUNC(_PyTime_t) _PyTime_GetSystemClock(void);
PyAPI_FUNC(int) _PyTime_GetSystemClockWithInfo(_PyTime_t*, _Py_clock_info_t*);
PyAPI_FUNC(int) _PyTime_Init(void);
PyAPI_FUNC(_PyTime_t)
    _PyTime_MulDiv(_PyTime_t ticks, _PyTime_t mul, _PyTime_t div);
PyAPI_FUNC(int) _PyTime_ObjectToTime_t(PyObject*, time_t*, _PyTime_round_t);
PyAPI_FUNC(int)
    _PyTime_ObjectToTimespec(PyObject*, time_t*, long*, _PyTime_round_t);
PyAPI_FUNC(int)
    _PyTime_ObjectToTimeval(PyObject*, time_t*, long*, _PyTime_round_t);
PyAPI_FUNC(int) _PyTime_gmtime(time_t, struct tm*);
PyAPI_FUNC(int) _PyTime_localtime(time_t, struct tm*);
PyAPI_FUNC(void) _PyTraceback_Add(const char*, const char*, int);
PyAPI_FUNC(void) _PyTrash_deposit_object(PyObject*);
PyAPI_FUNC(void) _PyTrash_destroy_chain(void);
PyAPI_FUNC(void) _PyTrash_thread_deposit_object(PyObject*);
PyAPI_FUNC(void) _PyTrash_thread_destroy_chain(void);
PyAPI_FUNC(PyObject*) _PyType_Lookup(PyTypeObject*, PyObject*);
PyAPI_FUNC(const char*) _PyType_Name(PyTypeObject*);
PyAPI_FUNC(void) _PyUnicodeWriter_Dealloc(_PyUnicodeWriter*);
PyAPI_FUNC(PyObject*) _PyUnicodeWriter_Finish(_PyUnicodeWriter*);
PyAPI_FUNC(void) _PyUnicodeWriter_Init(_PyUnicodeWriter*);
PyAPI_FUNC(int)
    _PyUnicodeWriter_Prepare(_PyUnicodeWriter*, Py_ssize_t, Py_UCS4);
PyAPI_FUNC(int) _PyUnicodeWriter_WriteASCIIString(_PyUnicodeWriter*,
                                                  const char*, Py_ssize_t);
PyAPI_FUNC(int) _PyUnicodeWriter_WriteChar(_PyUnicodeWriter*, Py_UCS4);
PyAPI_FUNC(int) _PyUnicodeWriter_WriteCharInline(_PyUnicodeWriter*, Py_UCS4);
PyAPI_FUNC(int) _PyUnicodeWriter_WriteLatin1String(_PyUnicodeWriter*,
                                                   const char*, Py_ssize_t);
PyAPI_FUNC(int) _PyUnicodeWriter_WriteStr(_PyUnicodeWriter*, PyObject*);
PyAPI_FUNC(int) _PyUnicodeWriter_WriteSubstring(_PyUnicodeWriter*, PyObject*,
                                                Py_ssize_t, Py_ssize_t);
PyAPI_FUNC(PyObject*) _PyUnicode_AsASCIIString(PyObject*, const char*);
PyAPI_FUNC(PyObject*) _PyUnicode_AsLatin1String(PyObject*, const char*);
PyAPI_FUNC(PyObject*) _PyUnicode_AsUTF8String(PyObject*, const char*);
PyAPI_FUNC(PyObject*) _PyUnicode_DecodeUnicodeEscape(const char*, Py_ssize_t,
                                                     const char*, const char**);
PyAPI_FUNC(PyObject*) _PyUnicode_DecodeUnicodeEscape(const char*, Py_ssize_t,
                                                     const char*, const char**);
PyAPI_FUNC(int) _PyUnicode_EQ(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) _PyUnicode_EncodeUTF16(PyObject*, const char*, int);
PyAPI_FUNC(PyObject*) _PyUnicode_EncodeUTF32(PyObject*, const char*, int);
PyAPI_FUNC(int) _PyUnicode_EqualToASCIIString(PyObject*, const char*);
PyAPI_FUNC(int) _PyUnicode_IsAlpha(Py_UCS4);
PyAPI_FUNC(int) _PyUnicode_IsCaseIgnorable(Py_UCS4);
PyAPI_FUNC(int) _PyUnicode_IsCased(Py_UCS4);
PyAPI_FUNC(int) _PyUnicode_IsDecimalDigit(Py_UCS4);
PyAPI_FUNC(int) _PyUnicode_IsDigit(Py_UCS4);
PyAPI_FUNC(int) _PyUnicode_IsLinebreak(Py_UCS4);
PyAPI_FUNC(int) _PyUnicode_IsLowercase(Py_UCS4);
PyAPI_FUNC(int) _PyUnicode_IsNumeric(Py_UCS4);
PyAPI_FUNC(int) _PyUnicode_IsPrintable(Py_UCS4);
PyAPI_FUNC(int) _PyUnicode_IsTitlecase(Py_UCS4);
PyAPI_FUNC(int) _PyUnicode_IsUppercase(Py_UCS4);
PyAPI_FUNC(int) _PyUnicode_IsWhitespace(Py_UCS4);
PyAPI_FUNC(int) _PyUnicode_IsXidContinue(Py_UCS4);
PyAPI_FUNC(int) _PyUnicode_IsXidStart(Py_UCS4);
PyAPI_FUNC(int) _PyUnicode_Ready(PyObject*);
PyAPI_FUNC(int) _PyUnicode_ToDecimalDigit(Py_UCS4);
PyAPI_FUNC(int) _PyUnicode_ToDigit(Py_UCS4);
PyAPI_FUNC(int) _PyUnicode_ToFoldedFull(Py_UCS4, Py_UCS4*);
PyAPI_FUNC(int) _PyUnicode_ToLowerFull(Py_UCS4, Py_UCS4*);
PyAPI_FUNC(Py_UCS4) _PyUnicode_ToLowercase(Py_UCS4);
PyAPI_FUNC(double) _PyUnicode_ToNumeric(Py_UCS4);
PyAPI_FUNC(int) _PyUnicode_ToTitleFull(Py_UCS4, Py_UCS4*);
PyAPI_FUNC(Py_UCS4) _PyUnicode_ToTitlecase(Py_UCS4);
PyAPI_FUNC(int) _PyUnicode_ToUpperFull(Py_UCS4, Py_UCS4*);
PyAPI_FUNC(Py_UCS4) _PyUnicode_ToUppercase(Py_UCS4);
PyAPI_FUNC(PyObject*) _Py_BuildValue_SizeT(const char*, ...);
PyAPI_FUNC(int) _Py_CheckRecursiveCall(const char*);
PyAPI_FUNC(void) _Py_Dealloc(PyObject*);
PyAPI_FUNC(int) _Py_DecodeLocaleEx(const char* arg, wchar_t** wstr,
                                   size_t* wlen, const char** reason,
                                   int current_locale, int surrogateescape);
PyAPI_FUNC(wchar_t*) _Py_DecodeUTF8_surrogateescape(const char*, Py_ssize_t);
PyAPI_FUNC(int)
    _Py_DecodeUTF8Ex(const char* c_str, Py_ssize_t size, wchar_t** result,
                     size_t* wlen, const char** reason, int surrogateescape);
PyAPI_FUNC(int) _Py_EncodeLocaleEx(const wchar_t* text, char** str,
                                   size_t* error_pos, const char** reason,
                                   int current_locale, int surrogateescape);
PyAPI_FUNC(int)
    _Py_EncodeUTF8Ex(const wchar_t* text, char** str, size_t* error_pos,
                     const char** reason, int raw_malloc, int surrogateescape);
PyAPI_FUNC(void) _Py_FreeCharPArray(char* const array[]);
PyAPI_FUNC(int) _Py_GetLocaleconvNumeric(PyObject**, PyObject**, const char**);
PyAPI_FUNC(Py_hash_t) _Py_HashBytes(const void*, Py_ssize_t);
PyAPI_FUNC(Py_hash_t) _Py_HashDouble(double);
PyAPI_FUNC(Py_hash_t) _Py_HashPointer(void*);
PyAPI_FUNC(PyObject*) _Py_Mangle(PyObject*, PyObject*);
PyAPI_FUNC(void) _Py_NewReference(PyObject*);
PyAPI_FUNC(void) _Py_PyAtExit(void (*func)(PyObject*), PyObject*);
PyAPI_FUNC(void) _Py_RestoreSignals(void);
PyAPI_FUNC(char*) _Py_SetLocaleFromEnv(int);
PyAPI_FUNC(PyObject*) _Py_VaBuildValue_SizeT(const char*, va_list);
PyAPI_FUNC(double) _Py_c_abs(Py_complex);
PyAPI_FUNC(Py_complex) _Py_c_diff(Py_complex, Py_complex);
PyAPI_FUNC(Py_complex) _Py_c_neg(Py_complex);
PyAPI_FUNC(Py_complex) _Py_c_pow(Py_complex, Py_complex);
PyAPI_FUNC(Py_complex) _Py_c_prod(Py_complex, Py_complex);
PyAPI_FUNC(Py_complex) _Py_c_quot(Py_complex, Py_complex);
PyAPI_FUNC(Py_complex) _Py_c_sum(Py_complex, Py_complex);
PyAPI_FUNC(PyObject*) _Py_device_encoding(int);
PyAPI_FUNC(char*) _Py_dg_dtoa(double, int, int, int*, int*, char**);
PyAPI_FUNC(void) _Py_dg_freedtoa(char*);
PyAPI_FUNC(double) _Py_dg_infinity(int sign);
PyAPI_FUNC(double) _Py_dg_stdnan(int sign);
PyAPI_FUNC(double) _Py_dg_strtod(const char*, char**);
PyAPI_FUNC(int) _Py_dup(int);
PyAPI_FUNC(FILE*) _Py_fopen(const char*, const char*);
PyAPI_FUNC(FILE*) _Py_fopen_obj(PyObject*, const char*);
PyAPI_FUNC(int) _Py_fstat(int, struct _Py_stat_struct*);
PyAPI_FUNC(int) _Py_fstat_noraise(int, struct _Py_stat_struct*);
PyAPI_FUNC(int) _Py_get_blocking(int);
PyAPI_FUNC(int) _Py_get_inheritable(int);
PyAPI_FUNC(int) _Py_normalize_encoding(const char*, char*, size_t);
PyAPI_FUNC(int) _Py_open(const char*, int);
PyAPI_FUNC(int) _Py_open_noraise(const char*, int);
PyAPI_FUNC(double) _Py_parse_inf_or_nan(const char*, char**);
PyAPI_FUNC(Py_ssize_t) _Py_read(int, void*, size_t);
PyAPI_FUNC(int) _Py_set_blocking(int, int);
PyAPI_FUNC(int) _Py_set_inheritable(int, int, int*);
PyAPI_FUNC(int) _Py_set_inheritable_async_safe(int, int, int*);
PyAPI_FUNC(int) _Py_stat(PyObject*, struct stat*);
PyAPI_FUNC(PyObject*)
    _Py_string_to_number_with_underscores(const char*, Py_ssize_t, const char*,
                                          PyObject*, void*,
                                          PyObject* (*)(const char*, Py_ssize_t,
                                                        void*));
PyAPI_FUNC(FILE*) _Py_wfopen(const wchar_t*, const wchar_t*);
PyAPI_FUNC(wchar_t*) _Py_wgetcwd(wchar_t*, size_t);
PyAPI_FUNC(int) _Py_wreadlink(const wchar_t*, wchar_t*, size_t);
PyAPI_FUNC(wchar_t*) _Py_wrealpath(const wchar_t*, wchar_t*, size_t);
PyAPI_FUNC(Py_ssize_t) _Py_write(int, const void*, size_t);
PyAPI_FUNC(Py_ssize_t) _Py_write_noraise(int, const void*, size_t);

/* Macros */

#ifdef PY_SSIZE_T_CLEAN
#define PyArg_Parse _PyArg_Parse_SizeT
#define PyArg_ParseTuple _PyArg_ParseTuple_SizeT
#define PyArg_ParseTupleAndKeywords _PyArg_ParseTupleAndKeywords_SizeT
#define PyArg_VaParse _PyArg_VaParse_SizeT
#define PyArg_VaParseTupleAndKeywords _PyArg_VaParseTupleAndKeywords_SizeT
#define Py_BuildValue _Py_BuildValue_SizeT
#define Py_VaBuildValue _Py_VaBuildValue_SizeT
#define _PyArg_ParseStack _PyArg_ParseStack_SizeT
#define _PyArg_ParseStackAndKeywords _PyArg_ParseStackAndKeywords_SizeT
#define _PyArg_ParseTupleAndKeywordsFast _PyArg_ParseTupleAndKeywordsFast_SizeT
#define _PyArg_VaParseTupleAndKeywordsFast                                     \
  _PyArg_VaParseTupleAndKeywordsFast_SizeT
#endif

/* Parentheses around the whole expressions are needed to compile
 * "if PyDict_CheckExact(other) {". Parentheses around "op" are needed for
 * "PyBytes_Check(state = PyTuple_GET_ITEM(args, 0))" */
#define PyBool_Check(op) (PyBool_Check_Func((PyObject*)(op)))
#define PyByteArray_Check(op) (PyByteArray_Check_Func((PyObject*)(op)))
#define PyByteArray_CheckExact(op)                                             \
  (PyByteArray_CheckExact_Func((PyObject*)(op)))
#define PyBytes_Check(op) (PyBytes_Check_Func((PyObject*)(op)))
#define PyBytes_CheckExact(op) (PyBytes_CheckExact_Func((PyObject*)(op)))
#define PyCapsule_CheckExact(op) (PyCapsule_CheckExact_Func((PyObject*)(op)))
#define PyCFunction_Check(op) (PyCFunction_Check_Func((PyObject*)(op)))
#define PyCode_Check(op) (PyCode_Check_Func((PyObject*)(op)))
#define PyCode_GetNumFree(op) PyCode_GetNumFree_Func((PyObject*)op)
#define PyCode_GetName(op) PyCode_GetName_Func(op)
#define PyCode_GetFreevars(op) PyCode_GetFreevars_Func(op)
#define PyComplex_Check(op) (PyComplex_Check_Func((PyObject*)(op)))
#define PyComplex_CheckExact(op) (PyComplex_CheckExact_Func((PyObject*)(op)))
#define PyDict_Check(op) (PyDict_Check_Func((PyObject*)(op)))
#define PyDict_CheckExact(op) (PyDict_CheckExact_Func((PyObject*)(op)))
#define PyFloat_Check(op) (PyFloat_Check_Func((PyObject*)(op)))
#define PyFloat_CheckExact(op) (PyFloat_CheckExact_Func((PyObject*)(op)))
#define PyFrozenSet_Check(op) (PyFrozenSet_Check_Func((PyObject*)(op)))
#define PyFrozenSet_CheckExact(op)                                             \
  (PyFrozenSet_CheckExact_Func((PyObject*)(op)))
#define PyIndex_Check(op) (PyIndex_Check_Func((PyObject*)(op)))
#define PyIter_Check(op) (PyIter_Check_Func((PyObject*)(op)))
#define PyList_Check(op) (PyList_Check_Func((PyObject*)(op)))
#define PyList_CheckExact(op) (PyList_CheckExact_Func((PyObject*)(op)))
#define PyLong_Check(op) (PyLong_Check_Func((PyObject*)(op)))
#define PyLong_CheckExact(op) (PyLong_CheckExact_Func((PyObject*)(op)))
#define PyMemoryView_Check(op) (PyMemoryView_Check_Func((PyObject*)(op)))
#define PyMethod_Check(op) (PyMethod_Check_Func((PyObject*)(op)))
#define PyModule_Check(op) (PyModule_Check_Func((PyObject*)(op)))
#define PyModule_CheckExact(op) (PyModule_CheckExact_Func((PyObject*)(op)))
#define PyObject_CheckBuffer(op) (PyObject_CheckBuffer_Func((PyObject*)(op)))
#define PyObject_TypeCheck(op, tp)                                             \
  (PyObject_TypeCheck_Func((PyObject*)(op), (PyTypeObject*)(tp)))
#define PySet_Check(op) (PySet_Check_Func((PyObject*)(op)))
#define PySlice_Check(op) (PySlice_Check_Func((PyObject*)(op)))
#define PyTraceBack_Check(op) (PyTraceBack_Check_Func((PyObject*)(op)))
#define PyTuple_Check(op) (PyTuple_Check_Func((PyObject*)(op)))
#define PyTuple_CheckExact(op) (PyTuple_CheckExact_Func((PyObject*)(op)))
#define PyType_Check(op) (PyType_Check_Func((PyObject*)(op)))
#define PyType_CheckExact(op) (PyType_CheckExact_Func((PyObject*)(op)))
#define PyUnicode_Check(op) (PyUnicode_Check_Func((PyObject*)(op)))
#define PyUnicode_CheckExact(op) (PyUnicode_CheckExact_Func((PyObject*)(op)))
#define PyWeakref_Check(op) (PyWeakref_Check_Func((PyObject*)(op)))

#define PyFPE_START_PROTECT(err_string, leave_stmt)
#define PyFPE_END_PROTECT(v)

#define _Py_BEGIN_SUPPRESS_IPH
#define _Py_END_SUPPRESS_IPH

#define _Py_SET_53BIT_PRECISION_HEADER
#define _Py_SET_53BIT_PRECISION_START
#define _Py_SET_53BIT_PRECISION_END

#define PyModule_AddIntMacro(m, c) PyModule_AddIntConstant(m, #c, c)
#define PyModule_Create(module) PyModule_Create2(module, PYTHON_API_VERSION)

#define PyByteArray_AS_STRING(op) PyByteArray_AsString((PyObject*)op)
#define PyByteArray_GET_SIZE(op) PyByteArray_Size((PyObject*)op)
#define PyBytes_AS_STRING(op) PyBytes_AsString((PyObject*)op)
#define PyBytes_GET_SIZE(op) PyBytes_Size((PyObject*)op)

#define PyDict_GET_SIZE(op) PyDict_GET_SIZE_Func((PyObject*)op)

#define PyEval_CallObject(func, arg)                                           \
  PyEval_CallObjectWithKeywords(func, arg, (PyObject*)NULL)

#define PyFloat_AS_DOUBLE(op) PyFloat_AsDouble((PyObject*)op)

#define PyList_GET_ITEM(op, i) PyList_GetItem((PyObject*)op, i)
#define PyList_GET_SIZE(op) PyList_Size((PyObject*)op)
#define PyList_SET_ITEM(op, i, v) PyList_SET_ITEM_Func((PyObject*)op, i, v)

#define PyLong_AS_LONG(op) PyLong_AsLong(op)

#define PyMethod_GET_FUNCTION(op) (PyMethod_GET_FUNCTION_Func((PyObject*)op))
#define PyMethod_GET_SELF(op) (PyMethod_GET_SELF_Func((PyObject*)op))

#define PySequence_Fast_GET_SIZE(op)                                           \
  PySequence_Fast_GET_SIZE_Func((PyObject*)op)
#define PySequence_Fast_GET_ITEM(op, i)                                        \
  PySequence_Fast_GET_ITEM_Func((PyObject*)op, i)
#define PySequence_ITEM(op, i) PySequence_ITEM_Func((PyObject*)op, i)

#define PySet_GET_SIZE(op) PySet_Size((PyObject*)op)

#define PyTuple_GET_SIZE(op) PyTuple_Size((PyObject*)op)
#define PyTuple_GET_ITEM(op, i) PyTuple_GetItem((PyObject*)op, i)
#define PyTuple_SET_ITEM(op, i, v) PyTuple_SET_ITEM_Func((PyObject*)op, i, v)

#define PyType_HasFeature(t, f) ((PyType_GetFlags(t) & (f)) != 0)
#define PyType_IS_GC(t) PyType_HasFeature((t), Py_TPFLAGS_HAVE_GC)

#define PyStructSequence_GET_ITEM(op, i)                                       \
  PyStructSequence_GetItem((PyObject*)op, i)
#define PyStructSequence_SET_ITEM(op, i, v)                                    \
  PyStructSequence_SET_ITEM_Func((PyObject*)op, i, v)

#define PyUnicode_GET_LENGTH(op) PyUnicode_GetLength((PyObject*)op)
#define PyUnicode_GET_SIZE(op) PyUnicode_GetSize((PyObject*)op)
#define PyUnicode_IS_READY(op) 1
#define PyUnicode_KIND(op) PyUnicode_KIND_Func((PyObject*)op)
#define PyUnicode_DATA(op) PyUnicode_DATA_Func((PyObject*)op)
#define PyUnicode_1BYTE_DATA(op) ((Py_UCS1*)PyUnicode_DATA(op))
#define PyUnicode_2BYTE_DATA(op) ((Py_UCS2*)PyUnicode_DATA(op))
#define PyUnicode_4BYTE_DATA(op) ((Py_UCS4*)PyUnicode_DATA(op))
#define PyUnicode_READ(kind, data, index)                                      \
  PyUnicode_READ_Func(kind, (void*)data, index)
#define PyUnicode_READ_CHAR(op, index)                                         \
  PyUnicode_READ_CHAR_Func((PyObject*)op, index)
#define PyUnicode_READY(op) 0
#define PyUnicode_WRITE(kind, data, index, value)                              \
  PyUnicode_WRITE_Func(kind, data, index, value)
#define PyUnicode_IS_ASCII(op) PyUnicode_IS_ASCII_Func((PyObject*)op)
#define PyUnicode_IS_COMPACT_ASCII(op) PyUnicode_IS_ASCII_Func((PyObject*)op)

#define PyWeakref_GET_OBJECT(ref)                                              \
  PyWeakref_GET_OBJECT_Func((PyWeakReference*)ref)

#define Py_MIN(x, y) (((x) > (y)) ? (y) : (x))
#define Py_MAX(x, y) (((x) > (y)) ? (x) : (y))
#define Py_ABS(x) ((x) < 0 ? -(x) : (x))

#define Py_BUILD_ASSERT_EXPR(cond) (sizeof(char[1 - 2 * !(cond)]) - 1)
#define Py_BUILD_ASSERT(cond)                                                  \
  do {                                                                         \
    (void)Py_BUILD_ASSERT_EXPR(cond);                                          \
  } while (0)

#define _Py_SIZE_ROUND_DOWN(n, a) ((size_t)(n) & ~(size_t)((a)-1))
#define _Py_SIZE_ROUND_UP(n, a)                                                \
  (((size_t)(n) + (size_t)((a)-1)) & ~(size_t)((a)-1))
#define _Py_ALIGN_DOWN(p, a) ((void*)((uintptr_t)(p) & ~(uintptr_t)((a)-1)))
#define _Py_ALIGN_UP(p, a)                                                     \
  ((void*)(((uintptr_t)(p) + (uintptr_t)((a)-1)) & ~(uintptr_t)((a)-1)))
#define _Py_IS_ALIGNED(p, a) (!((uintptr_t)(p) & (uintptr_t)((a)-1)))

#define Py_CLEAR(op)                                                           \
  do {                                                                         \
    PyObject* _py_tmp = (PyObject*)(op);                                       \
    if (_py_tmp != NULL) {                                                     \
      (op) = NULL;                                                             \
      Py_DECREF_Func(_py_tmp);                                                 \
    }                                                                          \
  } while (0)
#define Py_DECREF(op) Py_DECREF_Func((PyObject*)op)
#define Py_INCREF(op) Py_INCREF_Func((PyObject*)op)
#define Py_REFCNT(op) (*Py_REFCNT_Func(((PyObject*)op)))
#define Py_XDECREF(op) Py_DecRef((PyObject*)op)
#define Py_XINCREF(op) Py_IncRef((PyObject*)op)
#define Py_SETREF(op, op2)                                                     \
  do {                                                                         \
    PyObject* _py_tmp = (PyObject*)(op);                                       \
    (op) = (op2);                                                              \
    Py_DECREF(_py_tmp);                                                        \
  } while (0)
#define Py_XSETREF(op, op2)                                                    \
  do {                                                                         \
    PyObject* _py_tmp = (PyObject*)(op);                                       \
    (op) = (op2);                                                              \
    Py_XDECREF(_py_tmp);                                                       \
  } while (0)

#define PyObject_MALLOC PyObject_Malloc
#define PyObject_REALLOC PyObject_Realloc
#define PyObject_FREE PyObject_Free
#define PyObject_Del PyObject_Free
#define PyObject_DEL PyObject_Free
#define _PyObject_SIZE(typeobj) _PyObject_SIZE_Func((PyObject*)typeobj)
#define _PyObject_VAR_SIZE(typeobj, nitems)                                    \
  _PyObject_VAR_SIZE_Func((PyObject*)typeobj, nitems)
#define PyObject_INIT(op, typeobj)                                             \
  PyObject_Init((PyObject*)op, (PyTypeObject*)typeobj)
#define PyObject_INIT_VAR(op, typeobj, size)                                   \
  PyObject_InitVar((PyVarObject*)op, (PyTypeObject*)typeobj, size)
#define PyObject_New(type, typeobj)                                            \
  ((type*)_PyObject_New((PyTypeObject*)typeobj))
#define PyObject_NEW(type, typeobj) PyObject_New(type, typeobj)
#define PyObject_NewVar(type, typeobj, n)                                      \
  ((type*)_PyObject_NewVar((PyTypeObject*)typeobj, n))
#define PyObject_NEW_VAR(type, typeobj, n) PyObject_NewVar(type, typeobj, n)
#define Py_SIZE(obj) (*Py_SIZE_Func((PyVarObject*)(obj)))
#define Py_TYPE(obj) Py_TYPE_Func((PyObject*)(obj))
#define PyExceptionClass_Check(obj)                                            \
  PyExceptionClass_Check_Func((PyObject*)(obj))
#define PyExceptionInstance_Class(obj)                                         \
  ((PyObject*)Py_TYPE_Func((PyObject*)(obj)))
#define PyExceptionInstance_Check(x) PyExceptionInstance_Check_Func(x)

#define PyObject_GC_New(type, typeobj) ((type*)_PyObject_GC_New(typeobj))
#define PyObject_GC_NewVar(type, typeobj, n)                                   \
  ((type*)_PyObject_GC_NewVar((typeobj), (n)))
#define Py_VISIT(op)                                                           \
  do {                                                                         \
    if (op) {                                                                  \
      int vret = visit((PyObject*)(op), arg);                                  \
      if (vret) return vret;                                                   \
    }                                                                          \
  } while (0)

/* Memory macros from pymem.h */
#define PyMem_DEL(p) PyMem_Del(p)
#define PyMem_FREE(p) PyMem_Free(p)
#define PyMem_MALLOC(n) PyMem_Malloc(n)
#define PyMem_New(type, n) ((type*)PyMem_New_Func(sizeof(type), n))
#define PyMem_NEW(type, n) PyMem_New(type, n)
#define PyMem_REALLOC(p, n) PyMem_Realloc(p, n)
#define PyMem_Resize(p, type, n)                                               \
  ((p) = ((size_t)(n) > PY_SSIZE_T_MAX / sizeof(type))                         \
             ? NULL                                                            \
             : (type*)PyMem_Realloc((p), (n) * sizeof(type)))
#define PyMem_RESIZE(p, type, n) PyMem_Resize(p, type, n)

/* Character macros from pyctype.h */
#define Py_CHARMASK(c) ((unsigned char)((c)&0xff))
#define Py_ISALNUM(c) (_Py_ctype_table[Py_CHARMASK(c)] & PY_CTF_ALNUM)
#define Py_ISALPHA(c) (_Py_ctype_table[Py_CHARMASK(c)] & PY_CTF_ALPHA)
#define Py_ISDIGIT(c) (_Py_ctype_table[Py_CHARMASK(c)] & PY_CTF_DIGIT)
#define Py_ISLOWER(c) (_Py_ctype_table[Py_CHARMASK(c)] & PY_CTF_LOWER)
#define Py_ISSPACE(c) (_Py_ctype_table[Py_CHARMASK(c)] & PY_CTF_SPACE)
#define Py_ISUPPER(c) (_Py_ctype_table[Py_CHARMASK(c)] & PY_CTF_UPPER)
#define Py_ISXDIGIT(c) (_Py_ctype_table[Py_CHARMASK(c)] & PY_CTF_XDIGIT)
#define Py_TOLOWER(c) (_Py_ctype_tolower[Py_CHARMASK(c)])
#define Py_TOUPPER(c) (_Py_ctype_toupper[Py_CHARMASK(c)])

#define Py_SAFE_DOWNCAST(VALUE, WIDE, NARROW) (NARROW)(VALUE)

#define Py_EnterRecursiveCall(where) Py_EnterRecursiveCall_Func(where)
#define Py_LeaveRecursiveCall() Py_LeaveRecursiveCall_Func()

#define _PyIsSelectable_fd(FD) ((unsigned int)(FD) < (unsigned int)FD_SETSIZE)

#define _PYTIME_FROMSECONDS(seconds) _PyTime_FromSeconds(seconds)

#define Py_UNICODE_ISALNUM(ch)                                                 \
  (Py_UNICODE_ISALPHA(ch) || Py_UNICODE_ISDECIMAL(ch) ||                       \
   Py_UNICODE_ISDIGIT(ch) || Py_UNICODE_ISNUMERIC(ch))
#define Py_UNICODE_ISALPHA(ch) _PyUnicode_IsAlpha(ch)
#define Py_UNICODE_ISDECIMAL(ch) _PyUnicode_IsDecimalDigit(ch)
#define Py_UNICODE_ISDIGIT(ch) _PyUnicode_IsDigit(ch)
#define Py_UNICODE_ISLINEBREAK(ch) _PyUnicode_IsLinebreak(ch)
#define Py_UNICODE_ISLOWER(ch) _PyUnicode_IsLowercase(ch)
#define Py_UNICODE_ISNUMERIC(ch) _PyUnicode_IsNumeric(ch)
#define Py_UNICODE_ISPRINTABLE(ch) _PyUnicode_IsPrintable(ch)
#define Py_UNICODE_ISSPACE(ch)                                                 \
  ((ch) < 128U ? _Py_ascii_whitespace[(ch)] : _PyUnicode_IsWhitespace(ch))
#define Py_UNICODE_ISTITLE(ch) _PyUnicode_IsTitlecase(ch)
#define Py_UNICODE_ISUPPER(ch) _PyUnicode_IsUppercase(ch)
#define Py_UNICODE_IS_HIGH_SURROGATE(ch) (0xD800 <= (ch) && (ch) <= 0xDBFF)
#define Py_UNICODE_IS_LOW_SURROGATE(ch) (0xDC00 <= (ch) && (ch) <= 0xDFFF)
#define Py_UNICODE_IS_SURROGATE(ch) (0xD800 <= (ch) && (ch) <= 0xDFFF)
#define Py_UNICODE_TODECIMAL(ch) _PyUnicode_ToDecimalDigit(ch)
#define Py_UNICODE_TODIGIT(ch) _PyUnicode_ToDigit(ch)
#define Py_UNICODE_TOLOWER(ch) _PyUnicode_ToLowercase(ch)
#define Py_UNICODE_TONUMERIC(ch) _PyUnicode_ToNumeric(ch)
#define Py_UNICODE_TOTITLE(ch) _PyUnicode_ToTitlecase(ch)
#define Py_UNICODE_TOUPPER(ch) _PyUnicode_ToUppercase(ch)
#define Py_UNICODE_JOIN_SURROGATES(high, low)                                  \
  (((((Py_UCS4)(high)&0x03FF) << 10) | ((Py_UCS4)(low)&0x03FF)) + 0x10000)
#define Py_UNICODE_HIGH_SURROGATE(ch) (0xD800 - (0x10000 >> 10) + ((ch) >> 10))
#define Py_UNICODE_LOW_SURROGATE(ch) (0xDC00 + ((ch)&0x3FF))

/* Define identity macro so `generate_cpython_sources.py` deletes the cpython
 * macro. TODO(T56488016): Remove this macro when the generator is gone. */
#define _PyUnicodeWriter_Prepare(WRITER, LENGTH, MAXCHAR)                      \
  _PyUnicodeWriter_Prepare(WRITER, LENGTH, MAXCHAR)

#define Py_MEMCPY memcpy
#define Py_VA_COPY va_copy
#define Py_ARITHMETIC_RIGHT_SHIFT(TYPE, I, J) ((I) >> (J))
#define Py_SAFE_DOWNCAST(VALUE, WIDE, NARROW) (NARROW)(VALUE)

#define _Py_SET_EDOM_FOR_NAN(X) ;

#define Py_SET_ERRNO_ON_MATH_ERROR(X)                                          \
  do {                                                                         \
    if (errno == 0) {                                                          \
      if ((X) == Py_HUGE_VAL || (X) == -Py_HUGE_VAL)                           \
        errno = ERANGE;                                                        \
      else                                                                     \
        _Py_SET_EDOM_FOR_NAN(X)                                                \
    }                                                                          \
  } while (0)

#define Py_SET_ERANGE_IF_OVERFLOW(X) Py_SET_ERRNO_ON_MATH_ERROR(X)

#define Py_ADJUST_ERANGE1(X)                                                   \
  do {                                                                         \
    if (errno == 0) {                                                          \
      if ((X) == Py_HUGE_VAL || (X) == -Py_HUGE_VAL) errno = ERANGE;           \
    } else if (errno == ERANGE && (X) == 0.0)                                  \
      errno = 0;                                                               \
  } while (0)

#define Py_ADJUST_ERANGE2(X, Y)                                                \
  do {                                                                         \
    if ((X) == Py_HUGE_VAL || (X) == -Py_HUGE_VAL || (Y) == Py_HUGE_VAL ||     \
        (Y) == -Py_HUGE_VAL) {                                                 \
      if (errno == 0) errno = ERANGE;                                          \
    } else if (errno == ERANGE)                                                \
      errno = 0;                                                               \
  } while (0)

#define Py_FORCE_DOUBLE(X) (X)
#define Py_IS_NAN(X) isnan(X)
#define Py_IS_INFINITY(X) isinf(X)
#define Py_IS_FINITE(X) isfinite(X)
#define Py_OVERFLOWED(X)                                                       \
  ((X) != 0.0 && (errno == ERANGE || (X) == Py_HUGE_VAL || (X) == -Py_HUGE_VAL))

#define _Py_IntegralTypeSigned(type) ((type)(-1) < 0)
#define _Py_IntegralTypeMax(type)                                              \
  ((_Py_IntegralTypeSigned(type))                                              \
       ? (((((type)1 << (sizeof(type) * CHAR_BIT - 2)) - 1) << 1) + 1)         \
       : ~(type)0)
#define _Py_IntegralTypeMin(type)                                              \
  ((_Py_IntegralTypeSigned(type)) ? -_Py_IntegralTypeMax(type) - 1 : 0)
#define _Py_InIntegralTypeRange(type, v)                                       \
  (_Py_IntegralTypeMin(type) <= v && v <= _Py_IntegralTypeMax(type))

#define Py_RETURN_FALSE                                                        \
  do {                                                                         \
    PyObject* return_value_ = Py_False;                                        \
    Py_INCREF(return_value_);                                                  \
    return return_value_;                                                      \
  } while (0)
#define Py_RETURN_INF(sign)                                                    \
  return PyFloat_FromDouble(copysign(Py_HUGE_VAL, (sign)))
#define Py_RETURN_NAN return PyFloat_FromDouble(Py_NAN)
#define Py_RETURN_NONE                                                         \
  do {                                                                         \
    PyObject* return_value_ = Py_None;                                         \
    Py_INCREF(return_value_);                                                  \
    return return_value_;                                                      \
  } while (0)
#define Py_RETURN_NOTIMPLEMENTED                                               \
  do {                                                                         \
    PyObject* return_value_ = Py_NotImplemented;                               \
    Py_INCREF(return_value_);                                                  \
    return return_value_;                                                      \
  } while (0)
#define Py_RETURN_RICHCOMPARE(val1, val2, op)                                  \
  do {                                                                         \
    switch (op) {                                                              \
      case Py_EQ:                                                              \
        if ((val1) == (val2)) Py_RETURN_TRUE;                                  \
        Py_RETURN_FALSE;                                                       \
      case Py_NE:                                                              \
        if ((val1) != (val2)) Py_RETURN_TRUE;                                  \
        Py_RETURN_FALSE;                                                       \
      case Py_LT:                                                              \
        if ((val1) < (val2)) Py_RETURN_TRUE;                                   \
        Py_RETURN_FALSE;                                                       \
      case Py_GT:                                                              \
        if ((val1) > (val2)) Py_RETURN_TRUE;                                   \
        Py_RETURN_FALSE;                                                       \
      case Py_LE:                                                              \
        if ((val1) <= (val2)) Py_RETURN_TRUE;                                  \
        Py_RETURN_FALSE;                                                       \
      case Py_GE:                                                              \
        if ((val1) >= (val2)) Py_RETURN_TRUE;                                  \
        Py_RETURN_FALSE;                                                       \
      default:                                                                 \
        Py_UNREACHABLE();                                                      \
    }                                                                          \
  } while (0)
#define Py_RETURN_TRUE                                                         \
  do {                                                                         \
    PyObject* return_value_ = Py_True;                                         \
    Py_INCREF(return_value_);                                                  \
    return return_value_;                                                      \
  } while (0)

#define Py_BEGIN_ALLOW_THREADS                                                 \
  {                                                                            \
    PyThreadState* _save;                                                      \
    _save = PyEval_SaveThread();
#define Py_BLOCK_THREADS PyEval_RestoreThread(_save);
#define Py_UNBLOCK_THREADS _save = PyEval_SaveThread();
#define Py_END_ALLOW_THREADS                                                   \
  PyEval_RestoreThread(_save);                                                 \
  }

#ifdef __cplusplus
}
#endif

#endif /* !CPYTHON_FUNC_H */
