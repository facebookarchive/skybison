#ifndef CPYTHON_FUNC_H
#define CPYTHON_FUNC_H

#include "cpython-types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PyAPI_FUNC(RTYPE) RTYPE

/* Singletons */
PyAPI_FUNC(PyObject*) PyExc_ArithmeticError_Ptr();
PyAPI_FUNC(PyObject*) PyExc_AssertionError_Ptr();
PyAPI_FUNC(PyObject*) PyExc_AttributeError_Ptr();
PyAPI_FUNC(PyObject*) PyExc_BaseException_Ptr();
PyAPI_FUNC(PyObject*) PyExc_BlockingIOError_Ptr();
PyAPI_FUNC(PyObject*) PyExc_BrokenPipeError_Ptr();
PyAPI_FUNC(PyObject*) PyExc_BufferError_Ptr();
PyAPI_FUNC(PyObject*) PyExc_BytesWarning_Ptr();
PyAPI_FUNC(PyObject*) PyExc_ChildProcessError_Ptr();
PyAPI_FUNC(PyObject*) PyExc_ConnectionAbortedError_Ptr();
PyAPI_FUNC(PyObject*) PyExc_ConnectionError_Ptr();
PyAPI_FUNC(PyObject*) PyExc_ConnectionRefusedError_Ptr();
PyAPI_FUNC(PyObject*) PyExc_ConnectionResetError_Ptr();
PyAPI_FUNC(PyObject*) PyExc_DeprecationWarning_Ptr();
PyAPI_FUNC(PyObject*) PyExc_EOFError_Ptr();
PyAPI_FUNC(PyObject*) PyExc_Exception_Ptr();
PyAPI_FUNC(PyObject*) PyExc_FileExistsError_Ptr();
PyAPI_FUNC(PyObject*) PyExc_FileNotFoundError_Ptr();
PyAPI_FUNC(PyObject*) PyExc_FloatingPointError_Ptr();
PyAPI_FUNC(PyObject*) PyExc_FutureWarning_Ptr();
PyAPI_FUNC(PyObject*) PyExc_GeneratorExit_Ptr();
PyAPI_FUNC(PyObject*) PyExc_ImportError_Ptr();
PyAPI_FUNC(PyObject*) PyExc_ImportWarning_Ptr();
PyAPI_FUNC(PyObject*) PyExc_IndentationError_Ptr();
PyAPI_FUNC(PyObject*) PyExc_IndexError_Ptr();
PyAPI_FUNC(PyObject*) PyExc_InterruptedError_Ptr();
PyAPI_FUNC(PyObject*) PyExc_IsADirectoryError_Ptr();
PyAPI_FUNC(PyObject*) PyExc_KeyError_Ptr();
PyAPI_FUNC(PyObject*) PyExc_KeyboardInterrupt_Ptr();
PyAPI_FUNC(PyObject*) PyExc_LookupError_Ptr();
PyAPI_FUNC(PyObject*) PyExc_MemoryError_Ptr();
PyAPI_FUNC(PyObject*) PyExc_ModuleNotFoundError_Ptr();
PyAPI_FUNC(PyObject*) PyExc_NameError_Ptr();
PyAPI_FUNC(PyObject*) PyExc_NotADirectoryError_Ptr();
PyAPI_FUNC(PyObject*) PyExc_NotImplementedError_Ptr();
PyAPI_FUNC(PyObject*) PyExc_OSError_Ptr();
PyAPI_FUNC(PyObject*) PyExc_OverflowError_Ptr();
PyAPI_FUNC(PyObject*) PyExc_PendingDeprecationWarning_Ptr();
PyAPI_FUNC(PyObject*) PyExc_PermissionError_Ptr();
PyAPI_FUNC(PyObject*) PyExc_ProcessLookupError_Ptr();
PyAPI_FUNC(PyObject*) PyExc_RecursionError_Ptr();
PyAPI_FUNC(PyObject*) PyExc_ReferenceError_Ptr();
PyAPI_FUNC(PyObject*) PyExc_ResourceWarning_Ptr();
PyAPI_FUNC(PyObject*) PyExc_RuntimeError_Ptr();
PyAPI_FUNC(PyObject*) PyExc_RuntimeWarning_Ptr();
PyAPI_FUNC(PyObject*) PyExc_StopAsyncIteration_Ptr();
PyAPI_FUNC(PyObject*) PyExc_StopIteration_Ptr();
PyAPI_FUNC(PyObject*) PyExc_SyntaxError_Ptr();
PyAPI_FUNC(PyObject*) PyExc_SyntaxWarning_Ptr();
PyAPI_FUNC(PyObject*) PyExc_SystemError_Ptr();
PyAPI_FUNC(PyObject*) PyExc_SystemExit_Ptr();
PyAPI_FUNC(PyObject*) PyExc_TabError_Ptr();
PyAPI_FUNC(PyObject*) PyExc_TimeoutError_Ptr();
PyAPI_FUNC(PyObject*) PyExc_TypeError_Ptr();
PyAPI_FUNC(PyObject*) PyExc_UnboundLocalError_Ptr();
PyAPI_FUNC(PyObject*) PyExc_UnicodeDecodeError_Ptr();
PyAPI_FUNC(PyObject*) PyExc_UnicodeEncodeError_Ptr();
PyAPI_FUNC(PyObject*) PyExc_UnicodeError_Ptr();
PyAPI_FUNC(PyObject*) PyExc_UnicodeTranslateError_Ptr();
PyAPI_FUNC(PyObject*) PyExc_UnicodeWarning_Ptr();
PyAPI_FUNC(PyObject*) PyExc_UserWarning_Ptr();
PyAPI_FUNC(PyObject*) PyExc_ValueError_Ptr();
PyAPI_FUNC(PyObject*) PyExc_Warning_Ptr();
PyAPI_FUNC(PyObject*) PyExc_ZeroDivisionError_Ptr();
PyAPI_FUNC(PyObject*) PyFalse_Ptr();
PyAPI_FUNC(PyObject*) PyNone_Ptr();
PyAPI_FUNC(PyObject*) PyTrue_Ptr();

/* C-API functions */
PyAPI_FUNC(int) PyArena_AddPyObject(PyArena*, PyObject*);
PyAPI_FUNC(void) PyArena_Free(PyArena*);
PyAPI_FUNC(void*) PyArena_Malloc(PyArena*, size_t);
PyAPI_FUNC(PyArena*) PyArena_New();
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
PyAPI_FUNC(Py_complex) PyComplex_AsCComplex(PyObject*);
PyAPI_FUNC(PyObject*) PyComplex_FromCComplex(Py_complex);
PyAPI_FUNC(PyObject*) PyComplex_FromDoubles(double, double);
PyAPI_FUNC(double) PyComplex_ImagAsDouble(PyObject*);
PyAPI_FUNC(double) PyComplex_RealAsDouble(PyObject*);
PyAPI_FUNC(PyObject*) PyDescr_NewClassMethod(PyTypeObject*, PyMethodDef*);
PyAPI_FUNC(PyObject*) PyDescr_NewGetSet(PyTypeObject*, PyGetSetDef*);
PyAPI_FUNC(PyObject*) PyDescr_NewMember(PyTypeObject*, PyMemberDef*);
PyAPI_FUNC(PyObject*) PyDescr_NewMethod(PyTypeObject*, PyMethodDef*);
PyAPI_FUNC(PyObject*) PyDictProxy_New(PyObject*);
PyAPI_FUNC(void) PyDict_Clear(PyObject*);
PyAPI_FUNC(int) PyDict_ClearFreeList();
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
PyAPI_FUNC(PyObject*) PyDict_New();
PyAPI_FUNC(int) PyDict_Next(PyObject*, Py_ssize_t*, PyObject**, PyObject**);
PyAPI_FUNC(int) PyDict_SetItem(PyObject*, PyObject*, PyObject*);
PyAPI_FUNC(int) PyDict_SetItemString(PyObject*, const char*, PyObject*);
PyAPI_FUNC(Py_ssize_t) PyDict_Size(PyObject*);
PyAPI_FUNC(int) PyDict_Update(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyDict_Values(PyObject*);
PyAPI_FUNC(int) PyErr_BadArgument();
PyAPI_FUNC(void) PyErr_BadInternalCall();
PyAPI_FUNC(void) _PyErr_BadInternalCall(const char*, int);
PyAPI_FUNC(int) PyErr_CheckSignals();
PyAPI_FUNC(void) PyErr_Clear();
PyAPI_FUNC(void) PyErr_Display(PyObject*, PyObject*, PyObject*);
PyAPI_FUNC(int) PyErr_ExceptionMatches(PyObject*);
PyAPI_FUNC(void) PyErr_Fetch(PyObject**, PyObject**, PyObject**);
PyAPI_FUNC(PyObject*) PyErr_Format(PyObject*, const char*, ...);
PyAPI_FUNC(PyObject*) _PyErr_FormatFromCause(PyObject*, const char*, ...);
PyAPI_FUNC(PyObject*) PyErr_FormatV(PyObject*, const char*, va_list);
PyAPI_FUNC(void) PyErr_GetExcInfo(PyObject**, PyObject**, PyObject**);
PyAPI_FUNC(int) PyErr_GivenExceptionMatches(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyErr_NewException(const char*, PyObject*, PyObject*);
PyAPI_FUNC(PyObject*)
    PyErr_NewExceptionWithDoc(const char*, const char*, PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyErr_NoMemory();
PyAPI_FUNC(void) PyErr_NormalizeException(PyObject**, PyObject**, PyObject**);
PyAPI_FUNC(PyObject*) PyErr_Occurred();
PyAPI_FUNC(void) PyErr_Print();
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
PyAPI_FUNC(void) PyErr_SetInterrupt();
PyAPI_FUNC(void) PyErr_SetNone(PyObject*);
PyAPI_FUNC(void) PyErr_SetObject(PyObject*, PyObject*);
PyAPI_FUNC(void) PyErr_SetString(PyObject*, const char*);
PyAPI_FUNC(void) PyErr_SyntaxLocation(const char*, int);
PyAPI_FUNC(void) PyErr_SyntaxLocationEx(const char*, int, int);
PyAPI_FUNC(int) PyErr_WarnEx(PyObject*, const char*, Py_ssize_t);
PyAPI_FUNC(int) PyErr_WarnExplicit(PyObject*, const char*, const char*, int,
                                   const char*, PyObject*);
PyAPI_FUNC(int) PyErr_WarnFormat(PyObject*, Py_ssize_t, const char*, ...);
PyAPI_FUNC(void) PyErr_WriteUnraisable(PyObject*);
PyAPI_FUNC(void) PyEval_AcquireLock();
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
PyAPI_FUNC(PyObject*) _PyEval_EvalFrameDefault(PyFrameObject*, int);
PyAPI_FUNC(PyObject*) PyEval_EvalFrameEx(PyFrameObject*, int);
PyAPI_FUNC(PyObject*) PyEval_GetBuiltins();
PyAPI_FUNC(PyFrameObject*) PyEval_GetFrame();
PyAPI_FUNC(const char*) PyEval_GetFuncDesc(PyObject*);
PyAPI_FUNC(const char*) PyEval_GetFuncName(PyObject*);
PyAPI_FUNC(PyObject*) PyEval_GetGlobals();
PyAPI_FUNC(PyObject*) PyEval_GetLocals();
PyAPI_FUNC(void) PyEval_InitThreads();
PyAPI_FUNC(int) PyEval_MergeCompilerFlags(PyCompilerFlags*);
PyAPI_FUNC(void) PyEval_ReInitThreads();
PyAPI_FUNC(void) PyEval_ReleaseLock();
PyAPI_FUNC(void) PyEval_ReleaseThread(PyThreadState*);
PyAPI_FUNC(void) PyEval_RestoreThread(PyThreadState*);
PyAPI_FUNC(PyThreadState*) PyEval_SaveThread();
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
PyAPI_FUNC(int) PyFloat_ClearFreeList();
PyAPI_FUNC(PyObject*) PyFloat_FromDouble(double);
PyAPI_FUNC(PyObject*) PyFloat_FromString(PyObject*);
PyAPI_FUNC(PyObject*) PyFloat_GetInfo();
PyAPI_FUNC(double) PyFloat_GetMax();
PyAPI_FUNC(double) PyFloat_GetMin();
PyAPI_FUNC(void) PyFrame_FastToLocals(PyFrameObject*);
PyAPI_FUNC(int) PyFrame_GetLineNumber(PyFrameObject*);
PyAPI_FUNC(void) PyFrame_LocalsToFast(PyFrameObject*, int);
PyAPI_FUNC(PyFrameObject*)
    PyFrame_New(PyThreadState*, PyCodeObject*, PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyFrozenSet_New(PyObject*);
PyAPI_FUNC(Py_ssize_t) PyGC_Collect();
PyAPI_FUNC(PyObject*) PyImport_AddModule(const char*);
PyAPI_FUNC(PyObject*) PyImport_AddModuleObject(PyObject*);
PyAPI_FUNC(int) PyImport_AppendInittab(const char*, PyObject* (*)(void));
PyAPI_FUNC(void) PyImport_Cleanup();
PyAPI_FUNC(PyObject*) PyImport_ExecCodeModule(const char*, PyObject*);
PyAPI_FUNC(PyObject*)
    PyImport_ExecCodeModuleEx(const char*, PyObject*, const char*);
PyAPI_FUNC(PyObject*)
    PyImport_ExecCodeModuleObject(PyObject*, PyObject*, PyObject*, PyObject*);
PyAPI_FUNC(PyObject*)
    PyImport_ExecCodeModuleWithPathnames(const char*, PyObject*, const char*,
                                         const char*);
PyAPI_FUNC(long) PyImport_GetMagicNumber();
PyAPI_FUNC(const char*) PyImport_GetMagicTag();
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
PyAPI_FUNC(PyInterpreterState*) PyInterpreterState_Head();
PyAPI_FUNC(PyInterpreterState*) PyInterpreterState_Next(PyInterpreterState*);
PyAPI_FUNC(PyThreadState*) PyInterpreterState_ThreadHead(PyInterpreterState*);
PyAPI_FUNC(PyObject*) PyIter_Next(PyObject*);
PyAPI_FUNC(int) PyList_Append(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyList_AsTuple(PyObject*);
PyAPI_FUNC(int) PyList_ClearFreeList();
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
PyAPI_FUNC(PyObject*) PyLong_FromSize_t(size_t);
PyAPI_FUNC(PyObject*) PyLong_FromSsize_t(Py_ssize_t);
PyAPI_FUNC(PyObject*) PyLong_FromString(const char*, char**, int);
PyAPI_FUNC(PyObject*) PyLong_FromUnsignedLong(unsigned long);
PyAPI_FUNC(PyObject*) PyLong_FromUnsignedLongLong(unsigned long long);
PyAPI_FUNC(PyObject*) PyLong_FromVoidPtr(void*);
PyAPI_FUNC(PyObject*) PyLong_GetInfo();
PyAPI_FUNC(int) PyMapping_Check(PyObject*);
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
PyAPI_FUNC(int) PyMethod_ClearFreeList();
PyAPI_FUNC(PyObject*) PyMethod_New(PyObject*, PyObject*);
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
PyAPI_FUNC(PyObject*) PyNumber_Absolute(PyObject*);
PyAPI_FUNC(PyObject*) PyNumber_Add(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyNumber_And(PyObject*, PyObject*);
PyAPI_FUNC(Py_ssize_t) PyNumber_AsSsize_t(PyObject*, PyObject*);
PyAPI_FUNC(int) PyNumber_Check(PyObject*);
PyAPI_FUNC(PyObject*) PyNumber_Divmod(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyNumber_Float(PyObject*);
PyAPI_FUNC(PyObject*) PyNumber_FloorDivide(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyNumber_InPlaceAdd(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyNumber_InPlaceFloorDivide(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyNumber_InPlaceMatrixMultiply(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyNumber_InPlaceMultiply(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyNumber_InPlacePower(PyObject*, PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyNumber_InPlaceRemainder(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyNumber_InPlaceTrueDivide(PyObject*, PyObject*);
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
PyAPI_FUNC(PyObject*) PyODict_New();
PyAPI_FUNC(int) PyODict_SetItem(PyObject*, PyObject*, PyObject*);
PyAPI_FUNC(void) PyOS_AfterFork();
PyAPI_FUNC(int) PyOS_CheckStack();
PyAPI_FUNC(PyObject*) PyOS_FSPath(PyObject*);
PyAPI_FUNC(void) PyOS_InitInterrupts();
PyAPI_FUNC(int) PyOS_InterruptOccurred();
PyAPI_FUNC(PyOS_sighandler_t) PyOS_getsig(int);
PyAPI_FUNC(int) PyOS_mystricmp(const char*, const char*);
PyAPI_FUNC(int) PyOS_mystrnicmp(const char*, const char*, Py_ssize_t);
PyAPI_FUNC(PyOS_sighandler_t) PyOS_setsig(int, PyOS_sighandler_t);
PyAPI_FUNC(int) PyOS_snprintf(char*, size_t, const char*, ...);
PyAPI_FUNC(double) PyOS_string_to_double(const char*, char**, PyObject*);
PyAPI_FUNC(long) PyOS_strtol(const char*, char**, int);
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
PyAPI_FUNC(int) PyObject_DelItem(PyObject*, PyObject*);
PyAPI_FUNC(int) PyObject_DelItemString(PyObject*, const char*);
PyAPI_FUNC(PyObject*) PyObject_Dir(PyObject*);
PyAPI_FUNC(PyObject*) PyObject_Format(PyObject*, PyObject*);
PyAPI_FUNC(void) PyObject_Free(void*);
PyAPI_FUNC(void) PyObject_GC_Del(void*);
PyAPI_FUNC(PyObject*) _PyObject_GC_Malloc(size_t);
PyAPI_FUNC(PyObject*) _PyObject_GC_New(PyTypeObject*);
PyAPI_FUNC(PyVarObject*) _PyObject_GC_NewVar(PyTypeObject*, Py_ssize_t);
PyAPI_FUNC(PyVarObject*) _PyObject_GC_Resize(PyVarObject*, Py_ssize_t);
PyAPI_FUNC(void) PyObject_GC_Track(void*);
PyAPI_FUNC(void) PyObject_GC_UnTrack(void*);
PyAPI_FUNC(PyObject*) PyObject_GenericGetAttr(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyObject_GenericGetDict(PyObject*, void*);
PyAPI_FUNC(int) PyObject_GenericSetAttr(PyObject*, PyObject*, PyObject*);
PyAPI_FUNC(int) PyObject_GenericSetDict(PyObject*, PyObject*, void*);
PyAPI_FUNC(PyObject*) PyObject_GetAttr(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) _PyObject_GetAttrId(PyObject*, struct _Py_Identifier*);
PyAPI_FUNC(PyObject*) PyObject_GetAttrString(PyObject*, const char*);
PyAPI_FUNC(int) PyObject_GetBuffer(PyObject*, Py_buffer*, int);
PyAPI_FUNC(PyObject*) PyObject_GetItem(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyObject_GetIter(PyObject*);
PyAPI_FUNC(int) PyObject_HasAttr(PyObject*, PyObject*);
PyAPI_FUNC(int) _PyObject_HasAttrId(PyObject*, struct _Py_Identifier*);
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
PyAPI_FUNC(PyObject*)
    _PyObject_LookupSpecial(PyObject*, struct _Py_Identifier*);
PyAPI_FUNC(void*) PyObject_Malloc(size_t);
PyAPI_FUNC(PyObject*) _PyObject_New(PyTypeObject*);
PyAPI_FUNC(PyVarObject*) _PyObject_NewVar(PyTypeObject*, Py_ssize_t);
PyAPI_FUNC(int) PyObject_Not(PyObject*);
PyAPI_FUNC(int) PyObject_Print(PyObject*, FILE*, int);
PyAPI_FUNC(void*) PyObject_Realloc(void*, size_t);
PyAPI_FUNC(PyObject*) PyObject_Repr(PyObject*);
PyAPI_FUNC(PyObject*) PyObject_RichCompare(PyObject*, PyObject*, int);
PyAPI_FUNC(int) PyObject_RichCompareBool(PyObject*, PyObject*, int);
PyAPI_FUNC(PyObject*) PyObject_SelfIter(PyObject*);
PyAPI_FUNC(int) PyObject_SetAttr(PyObject*, PyObject*, PyObject*);
PyAPI_FUNC(int)
    _PyObject_SetAttrId(PyObject*, struct _Py_Identifier*, PyObject*);
PyAPI_FUNC(int) PyObject_SetAttrString(PyObject*, const char*, PyObject*);
PyAPI_FUNC(int) PyObject_SetItem(PyObject*, PyObject*, PyObject*);
PyAPI_FUNC(Py_ssize_t) PyObject_Size(PyObject*);
PyAPI_FUNC(PyObject*) PyObject_Str(PyObject*);
PyAPI_FUNC(PyObject*) PyObject_Type(PyObject*);
PyAPI_FUNC(struct _node*)
    PyParser_SimpleParseFileFlags(FILE*, const char*, int, int);
PyAPI_FUNC(struct _node*)
    PyParser_SimpleParseStringFlags(const char*, int, int);
PyAPI_FUNC(struct _node*)
    PyParser_SimpleParseStringFlagsFilename(const char*, const char*, int, int);
PyAPI_FUNC(PyObject*) PyRun_FileExFlags(FILE*, const char*, int, PyObject*,
                                        PyObject*, int, PyCompilerFlags*);
PyAPI_FUNC(int) PyRun_SimpleStringFlags(const char*, PyCompilerFlags*);
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
PyAPI_FUNC(int) PySet_ClearFreeList();
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
PyAPI_FUNC(int) _PyState_AddModule(PyObject*, struct PyModuleDef*);
PyAPI_FUNC(void) _PyState_ClearModules();
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
PyAPI_FUNC(PyObject*) PySys_GetXOptions();
PyAPI_FUNC(int) PySys_HasWarnOptions();
PyAPI_FUNC(void) PySys_ResetWarnOptions();
PyAPI_FUNC(void) PySys_SetArgv(int, wchar_t**);
PyAPI_FUNC(void) PySys_SetArgvEx(int, wchar_t**, int);
PyAPI_FUNC(int) PySys_SetObject(const char*, PyObject*);
PyAPI_FUNC(void) PySys_SetPath(const wchar_t*);
PyAPI_FUNC(void) PySys_WriteStderr(const char*, ...);
PyAPI_FUNC(void) PySys_WriteStdout(const char*, ...);
PyAPI_FUNC(void) PyThreadState_Clear(PyThreadState*);
PyAPI_FUNC(void) PyThreadState_Delete(PyThreadState*);
PyAPI_FUNC(void) PyThreadState_DeleteCurrent();
PyAPI_FUNC(PyThreadState*) PyThreadState_Get();
PyAPI_FUNC(PyObject*) PyThreadState_GetDict();
PyAPI_FUNC(void) _PyThreadState_Init(PyThreadState*);
PyAPI_FUNC(PyThreadState*) PyThreadState_New(PyInterpreterState*);
PyAPI_FUNC(PyThreadState*) _PyThreadState_Prealloc(PyInterpreterState*);
PyAPI_FUNC(int) PyThreadState_SetAsyncExc(long, PyObject*);
PyAPI_FUNC(PyThreadState*) PyThreadState_Swap(PyThreadState*);
PyAPI_FUNC(int) PyTraceBack_Here(PyFrameObject*);
PyAPI_FUNC(int) PyTraceBack_Print(PyObject*, PyObject*);
PyAPI_FUNC(void) _PyTrash_deposit_object(PyObject*);
PyAPI_FUNC(void) _PyTrash_destroy_chain();
PyAPI_FUNC(void) _PyTrash_thread_deposit_object(PyObject*);
PyAPI_FUNC(void) _PyTrash_thread_destroy_chain();
PyAPI_FUNC(int) PyTuple_ClearFreeList();
PyAPI_FUNC(PyObject*) PyTuple_GetItem(PyObject*, Py_ssize_t);
PyAPI_FUNC(PyObject*) PyTuple_GetSlice(PyObject*, Py_ssize_t, Py_ssize_t);
PyAPI_FUNC(PyObject*) PyTuple_New(Py_ssize_t);
PyAPI_FUNC(PyObject*) PyTuple_Pack(Py_ssize_t, ...);
PyAPI_FUNC(int) PyTuple_SetItem(PyObject*, Py_ssize_t, PyObject*);
PyAPI_FUNC(Py_ssize_t) PyTuple_Size(PyObject*);
PyAPI_FUNC(unsigned int) PyType_ClearCache();
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
PyAPI_FUNC(char*) PyUnicode_AsUTF8(PyObject*);
PyAPI_FUNC(char*) PyUnicode_AsUTF8AndSize(PyObject*, Py_ssize_t*);
PyAPI_FUNC(PyObject*) PyUnicode_AsUTF8String(PyObject*);
PyAPI_FUNC(Py_UNICODE*) PyUnicode_AsUnicode(PyObject*);
PyAPI_FUNC(Py_UNICODE*) PyUnicode_AsUnicodeAndSize(PyObject*, Py_ssize_t*);
PyAPI_FUNC(PyObject*) PyUnicode_AsUnicodeEscapeString(PyObject*);
PyAPI_FUNC(Py_ssize_t) PyUnicode_AsWideChar(PyObject*, wchar_t*, Py_ssize_t);
PyAPI_FUNC(wchar_t*) PyUnicode_AsWideCharString(PyObject*, Py_ssize_t*);
PyAPI_FUNC(PyObject*) PyUnicode_BuildEncodingMap(PyObject*);
PyAPI_FUNC(int) PyUnicode_ClearFreeList();
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
PyAPI_FUNC(int) _PyUnicode_EQ(PyObject*, PyObject*);
PyAPI_FUNC(PyObject*) PyUnicode_EncodeCodePage(int, PyObject*, const char*);
PyAPI_FUNC(PyObject*) PyUnicode_EncodeFSDefault(PyObject*);
PyAPI_FUNC(PyObject*) PyUnicode_EncodeLocale(PyObject*, const char*);
PyAPI_FUNC(int) _PyUnicode_EqualToASCIIString(PyObject*, const char*);
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
PyAPI_FUNC(const char*) PyUnicode_GetDefaultEncoding();
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
PyAPI_FUNC(int) _PyUnicode_Ready(PyObject*);
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
PyAPI_FUNC(int) _Py_CheckRecursiveCall(const char*);
PyAPI_FUNC(void) Py_DecRef(PyObject*);
PyAPI_FUNC(wchar_t*) Py_DecodeLocale(const char*, size_t*);
PyAPI_FUNC(char*) Py_EncodeLocale(const wchar_t*, size_t*);
PyAPI_FUNC(void) Py_EndInterpreter(PyThreadState*);
PyAPI_FUNC(void) Py_Exit(int);
PyAPI_FUNC(void) Py_FatalError(const char*);
PyAPI_FUNC(void) Py_Finalize();
PyAPI_FUNC(int) Py_FinalizeEx();
PyAPI_FUNC(const char*) Py_GetBuildInfo();
PyAPI_FUNC(const char*) Py_GetCompiler();
PyAPI_FUNC(const char*) Py_GetCopyright();
PyAPI_FUNC(wchar_t*) Py_GetExecPrefix();
PyAPI_FUNC(wchar_t*) Py_GetPath();
PyAPI_FUNC(const char*) Py_GetPlatform();
PyAPI_FUNC(wchar_t*) Py_GetPrefix();
PyAPI_FUNC(wchar_t*) Py_GetProgramFullPath();
PyAPI_FUNC(wchar_t*) Py_GetProgramName();
PyAPI_FUNC(wchar_t*) Py_GetPythonHome();
PyAPI_FUNC(int) Py_GetRecursionLimit();
PyAPI_FUNC(const char*) Py_GetVersion();
PyAPI_FUNC(void) Py_IncRef(PyObject*);
PyAPI_FUNC(void) Py_Initialize();
PyAPI_FUNC(void) Py_InitializeEx(int);
PyAPI_FUNC(int) Py_IsInitialized();
PyAPI_FUNC(int) Py_Main(int, wchar_t** argv);
PyAPI_FUNC(int) Py_MakePendingCalls();
PyAPI_FUNC(PyThreadState*) Py_NewInterpreter();
PyAPI_FUNC(int) Py_ReprEnter(PyObject*);
PyAPI_FUNC(void) Py_ReprLeave(PyObject*);
PyAPI_FUNC(void) Py_SetPath(const wchar_t*);
PyAPI_FUNC(void) Py_SetProgramName(wchar_t*);
PyAPI_FUNC(void) Py_SetPythonHome(wchar_t*);
PyAPI_FUNC(void) Py_SetRecursionLimit(int);
PyAPI_FUNC(struct symtable*) Py_SymtableString(const char*, const char*, int);
PyAPI_FUNC(size_t) Py_UNICODE_strlen(const Py_UNICODE*);
PyAPI_FUNC(char*) Py_UniversalNewlineFgets(char*, int, FILE*, PyObject*);
PyAPI_FUNC(PyObject*) Py_VaBuildValue(const char*, va_list);

/* Non C-API functions */
PyAPI_FUNC(int) PyBool_Check_Func(PyObject*);
PyAPI_FUNC(int) PyByteArray_Check_Func(PyObject*);
PyAPI_FUNC(int) PyBytes_CheckExact_Func(PyObject*);
PyAPI_FUNC(int) PyBytes_Check_Func(PyObject*);
PyAPI_FUNC(int) PyComplex_CheckExact_Func(PyObject*);
PyAPI_FUNC(int) PyComplex_Check_Func(PyObject*);
PyAPI_FUNC(int) PyDict_CheckExact_Func(PyObject*);
PyAPI_FUNC(int) PyDict_Check_Func(PyObject*);
PyAPI_FUNC(int) PyFloat_CheckExact_Func(PyObject*);
PyAPI_FUNC(int) PyFloat_Check_Func(PyObject*);
PyAPI_FUNC(int) PyList_CheckExact_Func(PyObject*);
PyAPI_FUNC(int) PyList_Check_Func(PyObject*);
PyAPI_FUNC(int) PyLong_CheckExact_Func(PyObject*);
PyAPI_FUNC(int) PyLong_Check_Func(PyObject*);
PyAPI_FUNC(int) PyModule_CheckExact_Func(PyObject*);
PyAPI_FUNC(int) PyModule_Check_Func(PyObject*);
PyAPI_FUNC(int) PyObject_CheckBuffer_Func(PyObject*);
PyAPI_FUNC(int) PyTuple_CheckExact_Func(PyObject*);
PyAPI_FUNC(int) PyTuple_Check_Func(PyObject*);
PyAPI_FUNC(int) PyType_CheckExact_Func(PyObject*);
PyAPI_FUNC(int) PyType_Check_Func(PyObject*);
PyAPI_FUNC(int) PyUnicode_CheckExact_Func(PyObject*);
PyAPI_FUNC(int) PyUnicode_Check_Func(PyObject*);

PyAPI_FUNC(void) Py_DECREF_Func(PyObject*);
PyAPI_FUNC(void) Py_INCREF_Func(PyObject*);
PyAPI_FUNC(Py_ssize_t) Py_REFCNT_Func(PyObject*);
PyAPI_FUNC(void) Py_XDECREF_Func(PyObject*);
PyAPI_FUNC(void) Py_XINCREF_Func(PyObject*);

PyAPI_FUNC(void*) PyMem_New_Func(size_t size, size_t n);
PyAPI_FUNC(void*) PyMem_Resize_Func(void* p, size_t size, size_t n);

PyAPI_FUNC(int) PyUnicode_KIND_Func(PyObject*);
PyAPI_FUNC(void*) PyUnicode_DATA_Func(PyObject*);
PyAPI_FUNC(Py_UCS4) PyUnicode_READ_Func(int, void*, Py_ssize_t);
PyAPI_FUNC(Py_UCS4) PyUnicode_READ_CHAR_Func(PyObject*, Py_ssize_t);
PyAPI_FUNC(void)
    PyUnicode_WRITE_Func(enum PyUnicode_Kind, void*, Py_ssize_t, Py_UCS4);

/* Macros */
#define _Py_Dealloc (*_Py_Dealloc_Func)

/* Parentheses around the whole expressions are needed to compile
 * "if PyDict_CheckExact(other) {". Parentheses around "op" are needed for
 * "PyBytes_Check(state = PyTuple_GET_ITEM(args, 0))" */
#define PyBool_Check(op) (PyBool_Check_Func((PyObject*)(op)))
#define PyByteArray_Check(op) (PyByteArray_Check_Func((PyObject*)(op)))
#define PyBytes_Check(op) (PyBytes_Check_Func((PyObject*)(op)))
#define PyBytes_CheckExact(op) (PyBytes_CheckExact_Func((PyObject*)(op)))
#define PyComplex_Check(op) (PyComplex_Check_Func((PyObject*)(op)))
#define PyComplex_CheckExact(op) (PyComplex_CheckExact_Func((PyObject*)(op)))
#define PyDict_Check(op) (PyDict_Check_Func((PyObject*)(op)))
#define PyDict_CheckExact(op) (PyDict_CheckExact_Func((PyObject*)(op)))
#define PyFloat_Check(op) (PyFloat_Check_Func((PyObject*)(op)))
#define PyFloat_CheckExact(op) (PyFloat_CheckExact_Func((PyObject*)(op)))
#define PyList_Check(op) (PyList_Check_Func((PyObject*)(op)))
#define PyList_CheckExact(op) (PyList_CheckExact_Func((PyObject*)(op)))
#define PyLong_Check(op) (PyLong_Check_Func((PyObject*)(op)))
#define PyLong_CheckExact(op) (PyLong_CheckExact_Func((PyObject*)(op)))
#define PyModule_Check(op) (PyModule_Check_Func((PyObject*)(op)))
#define PyModule_CheckExact(op) (PyModule_CheckExact_Func((PyObject*)(op)))
#define PyObject_CheckBuffer(op) (PyObject_CheckBuffer_Func((PyObject*)(op)))
#define PyTuple_Check(op) (PyTuple_Check_Func((PyObject*)(op)))
#define PyTuple_CheckExact(op) (PyTuple_CheckExact_Func((PyObject*)(op)))
#define PyType_Check(op) (PyType_Check_Func((PyObject*)(op)))
#define PyType_CheckExact(op) (PyType_CheckExact_Func((PyObject*)(op)))
#define PyUnicode_Check(op) (PyUnicode_Check_Func((PyObject*)(op)))
#define PyUnicode_CheckExact(op) (PyUnicode_CheckExact_Func((PyObject*)(op)))

#define PYTHON_API_VERSION 1013
#define PyModule_AddIntMacro(m, c) PyModule_AddIntConstant(m, #c, c)
#define PyModule_Create(module) PyModule_Create2(module, PYTHON_API_VERSION)

#define PyBytes_AS_STRING(op) PyBytes_AsString((PyObject*)op)
#define PyBytes_GET_SIZE(op) PyBytes_Size((PyObject*)op)
#define PyByteArray_AS_STRING(op) PyByteArray_AsString((PyObject*)op)

#define PyList_GET_ITEM(op, i) PyList_GetItem((PyObject*)op, i)
#define PyList_GET_SIZE(op) PyList_Size((PyObject*)op)

#define PySet_GET_SIZE(op) PySet_Size((PyObject*)op)

#define PyTuple_GET_SIZE(op) PyTuple_Size((PyObject*)op)
#define PyTuple_GET_ITEM(op, i) PyTuple_GetItem((PyObject*)op, i)
#define PyTuple_SET_ITEM(op, i, v) PyTuple_SetItem((PyObject*)op, i, v)

#define PyUnicode_GET_LENGTH(op) PyUnicode_GetLength((PyObject*)op)
#define PyUnicode_GET_SIZE(op) PyUnicode_GetSize((PyObject*)op)
#define PyUnicode_KIND(op) PyUnicode_KIND_Func((PyObject*)op)
#define PyUnicode_DATA(op) PyUnicode_DATA_Func((PyObject*)op)
#define PyUnicode_READ(kind, data, index)                                      \
  PyUnicode_READ_Func(kind, (void*)data, index)
#define PyUnicode_READ_CHAR(op, index)                                         \
  PyUnicode_READ_CHAR_Func((PyObject*)op, index)
#define PyUnicode_READY(op) 0
#define PyUnicode_WRITE(kind, data, index, value)                              \
  PyUnicode_WRITE_Func(kind, data, index, value)

#define Py_MIN(x, y) (((x) > (y)) ? (y) : (x))
#define Py_MAX(x, y) (((x) > (y)) ? (x) : (y))
#define Py_ABS(x) ((x) < 0 ? -(x) : (x))

#define Py_DECREF(op) Py_DECREF_Func((PyObject*)op)
#define Py_INCREF(op) Py_INCREF_Func((PyObject*)op)
#define Py_REFCNT(op) Py_REFCNT_Func((PyObject*)op)
#define Py_XDECREF(op) Py_DecRef((PyObject*)op)
#define Py_XINCREF(op) Py_IncRef((PyObject*)op)

#define PyObject_INIT(op, typeobj)                                             \
  PyObject_Init((PyObject*)op, (PyTypeObject*)typeobj)
#define PyObject_INIT_VAR(op, typeobj, size)                                   \
  PyObject_InitVar((PyVarObject*)op, (PyTypeObject*)typeobj, size)

/* Memory macros from pymem.h */
#define PyMem_DEL(p) PyMem_Del(p)
#define PyMem_FREE(p) PyMem_Free(p)
#define PyMem_MALLOC(n) PyMem_Malloc(n)
#define PyMem_New(type, n) ((type*)PyMem_New_Func(sizeof(type), n))
#define PyMem_NEW(type, n) PyMem_New(type, n)
#define PyMem_REALLOC(p, n) PyMem_Realloc(p, n)
#define PyMem_Resize(p, type, n) ((type*)PyMem_Resize_Func(p, sizeof(type), n))
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

#ifdef __cplusplus
}
#endif

#endif /* !CPYTHON_FUNC_H */
