#include <cstdarg>

#include "../Python/modsupport-internal.h"
#include "cpython-data.h"
#include "cpython-func.h"

#include "capi-handles.h"
#include "exception-builtins.h"
#include "frame.h"
#include "int-builtins.h"
#include "list-builtins.h"
#include "object-builtins.h"
#include "runtime.h"
#include "type-builtins.h"

namespace py {

static PyObject* nullError(Thread* thread) {
  if (!thread->hasPendingException()) {
    thread->raiseWithFmt(LayoutId::kSystemError,
                         "null argument to internal routine");
  }
  return nullptr;
}

static PyObject* doUnaryOp(SymbolId op, PyObject* obj) {
  Thread* thread = Thread::current();
  if (obj == nullptr) {
    return nullError(thread);
  }

  HandleScope scope(thread);
  Object object(&scope, ApiHandle::fromPyObject(obj)->asObject());
  Object result(&scope,
                thread->invokeFunction1(SymbolId::kOperator, op, object));
  return result.isError() ? nullptr : ApiHandle::newReference(thread, *result);
}

static PyObject* doBinaryOp(SymbolId op, PyObject* left, PyObject* right) {
  Thread* thread = Thread::current();
  DCHECK(left != nullptr && right != nullptr, "null argument to binary op %s",
         Symbols::predefinedSymbolAt(op));
  HandleScope scope(thread);
  Object left_obj(&scope, ApiHandle::fromPyObject(left)->asObject());
  Object right_obj(&scope, ApiHandle::fromPyObject(right)->asObject());
  Object result(&scope, thread->invokeFunction2(SymbolId::kOperator, op,
                                                left_obj, right_obj));
  return result.isError() ? nullptr : ApiHandle::newReference(thread, *result);
}

static Py_ssize_t objectLength(PyObject* pyobj) {
  Thread* thread = Thread::current();
  if (pyobj == nullptr) {
    nullError(thread);
    return -1;
  }

  HandleScope scope(thread);
  Object obj(&scope, ApiHandle::fromPyObject(pyobj)->asObject());
  Object len_index(&scope, thread->invokeMethod1(obj, SymbolId::kDunderLen));
  if (len_index.isError()) {
    if (len_index.isErrorNotFound()) {
      thread->raiseWithFmt(LayoutId::kTypeError, "object has no len()");
    }
    return -1;
  }
  Object len(&scope, intFromIndex(thread, len_index));
  if (len.isError()) {
    return -1;
  }
  Int index(&scope, intUnderlying(*len));
  if (index.numDigits() > 1) {
    thread->raiseWithFmt(LayoutId::kOverflowError,
                         "cannot fit '%T' into an index-sized integer",
                         &len_index);
    return -1;
  }
  if (index.isNegative()) {
    thread->raiseWithFmt(LayoutId::kValueError, "__len__() should return >= 0");
    return -1;
  }
  return index.asWord();
}

// Buffer Protocol

PY_EXPORT int PyBuffer_FillInfo(Py_buffer* view, PyObject* exporter, void* buf,
                                Py_ssize_t len, int readonly, int flags) {
  if (view == nullptr) {
    Thread::current()->raiseWithFmt(
        LayoutId::kBufferError,
        "PyBuffer_FillInfo: view==NULL argument is obsolete");
    return -1;
  }
  if ((flags & PyBUF_WRITABLE) == PyBUF_WRITABLE && readonly == 1) {
    Thread::current()->raiseWithFmt(LayoutId::kBufferError,
                                    "Object is not writable.");
    return -1;
  }

  if (exporter != nullptr) {
    Py_INCREF(exporter);
  }
  view->obj = exporter;
  view->buf = buf;
  view->len = len;
  view->readonly = readonly;
  view->itemsize = 1;
  view->format = nullptr;
  if ((flags & PyBUF_FORMAT) == PyBUF_FORMAT) {
    view->format = const_cast<char*>("B");
  }
  view->ndim = 1;
  view->shape = nullptr;
  if ((flags & PyBUF_ND) == PyBUF_ND) {
    view->shape = &(view->len);
  }
  view->strides = nullptr;
  if ((flags & PyBUF_STRIDES) == PyBUF_STRIDES) {
    view->strides = &(view->itemsize);
  }
  view->suboffsets = nullptr;
  view->internal = nullptr;
  return 0;
}

static bool isContiguousWithRowMajorOrder(const Py_buffer* view) {
  if (view->suboffsets != nullptr) return false;
  if (view->strides == nullptr) return true;
  if (view->len == 0) return true;

  Py_ssize_t dim_stride = view->itemsize;
  for (int d = view->ndim - 1; d >= 0; d--) {
    Py_ssize_t dim_size = view->shape[d];
    if (dim_size > 1 && view->strides[d] != dim_stride) {
      return false;
    }
    dim_stride *= dim_size;
  }
  return true;
}

static bool isContiguousWithColumnMajorOrder(const Py_buffer* view) {
  if (view->suboffsets != nullptr) return false;
  if (view->len == 0) return true;
  if (view->strides == nullptr) {
    if (view->ndim <= 1) return true;
    // Non-contiguous if there is more than 1 dimension with size > 0.
    bool had_nonempty_dim = false;
    for (int d = 0; d < view->ndim; d++) {
      if (view->shape[d] > 1) {
        if (had_nonempty_dim) return false;
        had_nonempty_dim = true;
      }
    }
    return true;
  }

  Py_ssize_t dim_stride = view->itemsize;
  for (int d = 0; d < view->ndim; d++) {
    Py_ssize_t dim_size = view->shape[d];
    if (dim_size > 1 && view->strides[d] != dim_stride) {
      return false;
    }
    dim_stride *= dim_size;
  }
  return true;
}

PY_EXPORT int PyBuffer_IsContiguous(const Py_buffer* view, char order) {
  if (order == 'C') {
    return isContiguousWithRowMajorOrder(view);
  }
  if (order == 'F') {
    return isContiguousWithColumnMajorOrder(view);
  }
  if (order == 'A') {
    return isContiguousWithRowMajorOrder(view) ||
           isContiguousWithColumnMajorOrder(view);
  }
  return false;
}

PY_EXPORT void PyBuffer_Release(Py_buffer* view) {
  DCHECK(view != nullptr, "view must not be nullptr");
  PyObject* pyobj = view->obj;
  if (pyobj == nullptr) return;

  // TODO(T38246066) call bf_releasebuffer type slot.
  DCHECK(PyBytes_Check(pyobj) || PyByteArray_Check(pyobj),
         "buffer protocol only implemented for bytes");

  view->obj = nullptr;
  Py_DECREF(pyobj);
}

// PyIndex_Check

PY_EXPORT int PyIndex_Check_Func(PyObject* obj) {
  DCHECK(obj != nullptr, "Got null argument");
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object num(&scope, ApiHandle::fromPyObject(obj)->asObject());
  Type type(&scope, thread->runtime()->typeOf(*num));
  return !typeLookupInMroById(thread, type, SymbolId::kDunderIndex)
              .isErrorNotFound();
}

// PyIter_Next

PY_EXPORT PyObject* PyIter_Next(PyObject* iter) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object iter_obj(&scope, ApiHandle::fromPyObject(iter)->asObject());
  Object next(&scope, thread->invokeMethod1(iter_obj, SymbolId::kDunderNext));
  if (thread->clearPendingStopIteration()) {
    // End of iterable
    return nullptr;
  }
  if (next.isError()) {
    // Method lookup or call failed
    if (next.isErrorNotFound()) {
      thread->raiseWithFmt(LayoutId::kTypeError,
                           "failed to call __next__ on iterable");
    }
    return nullptr;
  }
  return ApiHandle::newReference(thread, *next);
}

// Mapping Protocol

PY_EXPORT int PyMapping_Check(PyObject* py_obj) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object obj(&scope, ApiHandle::fromPyObject(py_obj)->asObject());
  return thread->runtime()->isMapping(thread, obj);
}

PY_EXPORT PyObject* PyMapping_GetItemString(PyObject* obj, const char* key) {
  Thread* thread = Thread::current();
  if (obj == nullptr || key == nullptr) {
    return nullError(thread);
  }
  HandleScope scope(thread);
  Object object(&scope, ApiHandle::fromPyObject(obj)->asObject());
  Object key_obj(&scope, thread->runtime()->newStrFromCStr(key));
  Object result(&scope, objectGetItem(thread, object, key_obj));
  if (result.isErrorException()) return nullptr;
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT int PyMapping_HasKey(PyObject* obj, PyObject* key) {
  PyObject* v = PyObject_GetItem(obj, key);
  if (v != nullptr) {
    Py_DECREF(v);
    return 1;
  }
  PyErr_Clear();
  return 0;
}

PY_EXPORT int PyMapping_HasKeyString(PyObject* obj, const char* key) {
  PyObject* v = PyMapping_GetItemString(obj, key);
  if (v != nullptr) {
    Py_DECREF(v);
    return 1;
  }
  PyErr_Clear();
  return 0;
}

PY_EXPORT PyObject* PyMapping_Items(PyObject* mapping) {
  if (PyDict_CheckExact(mapping)) {
    return PyDict_Items(mapping);
  }
  PyObject* items = PyObject_CallMethod(mapping, "items", nullptr);
  if (items == nullptr) {
    return nullptr;
  }
  PyObject* fast = PySequence_Fast(items, "mapping.items() are not iterable");
  Py_DECREF(items);
  return fast;
}

PY_EXPORT PyObject* PyMapping_Keys(PyObject* mapping) {
  DCHECK(mapping != nullptr, "mapping was null");
  if (PyDict_CheckExact(mapping)) {
    return PyDict_Keys(mapping);
  }
  PyObject* keys = PyObject_CallMethod(mapping, "keys", nullptr);
  if (keys == nullptr) {
    return nullptr;
  }
  PyObject* fast = PySequence_Fast(keys, "mapping.keys() are not iterable");
  Py_DECREF(keys);
  return fast;
}

PY_EXPORT int PyMapping_SetItemString(PyObject* obj, const char* key,
                                      PyObject* value) {
  if (key == nullptr) {
    nullError(Thread::current());
    return -1;
  }
  PyObject* key_obj = PyUnicode_FromString(key);
  if (key_obj == nullptr) {
    return -1;
  }
  int r = PyObject_SetItem(obj, key_obj, value);
  Py_DECREF(key_obj);
  return r;
}

PY_EXPORT Py_ssize_t PyMapping_Size(PyObject* pyobj) {
  return objectLength(pyobj);
}

PY_EXPORT PyObject* PyMapping_Values(PyObject* mapping) {
  if (PyDict_CheckExact(mapping)) {
    return PyDict_Values(mapping);
  }
  PyObject* values = PyObject_CallMethod(mapping, "values", nullptr);
  if (values == nullptr) {
    return nullptr;
  }
  PyObject* fast = PySequence_Fast(values, "mapping.values() are not iterable");
  Py_DECREF(values);
  return fast;
}

// Number Protocol

PY_EXPORT PyObject* PyNumber_Absolute(PyObject* obj) {
  return doUnaryOp(SymbolId::kAbs, obj);
}

PY_EXPORT PyObject* PyNumber_Add(PyObject* left, PyObject* right) {
  return doBinaryOp(SymbolId::kAdd, left, right);
}

PY_EXPORT PyObject* PyNumber_And(PyObject* left, PyObject* right) {
  return doBinaryOp(SymbolId::kAndUnder, left, right);
}

PY_EXPORT Py_ssize_t PyNumber_AsSsize_t(PyObject* obj, PyObject* overflow_err) {
  Thread* thread = Thread::current();
  if (obj == nullptr) {
    nullError(thread);
    return -1;
  }
  HandleScope scope(thread);
  Object index(&scope, ApiHandle::fromPyObject(obj)->asObject());
  Object num(&scope, intFromIndex(thread, index));
  if (num.isError()) return -1;
  Int number(&scope, intUnderlying(*num));
  if (overflow_err == nullptr || number.numDigits() == 1) {
    // Overflows should be clipped, or value is already in range.
    return number.asWordSaturated();
  }
  // Value overflows, raise an exception.
  thread->setPendingExceptionType(
      ApiHandle::fromPyObject(overflow_err)->asObject());
  thread->setPendingExceptionValue(thread->runtime()->newStrFromFmt(
      "cannot fit '%T' into an index-sized integer", &index));
  return -1;
}

PY_EXPORT int PyNumber_Check(PyObject* obj) {
  if (obj == nullptr) {
    return false;
  }

  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object num(&scope, ApiHandle::fromPyObject(obj)->asObject());
  Type type(&scope, thread->runtime()->typeOf(*num));
  if (!typeLookupInMroById(thread, type, SymbolId::kDunderInt)
           .isErrorNotFound()) {
    return true;
  }
  if (!typeLookupInMroById(thread, type, SymbolId::kDunderFloat)
           .isErrorNotFound()) {
    return true;
  }
  return false;
}

PY_EXPORT PyObject* PyNumber_Divmod(PyObject* left, PyObject* right) {
  return doBinaryOp(SymbolId::kDivmod, left, right);
}

PY_EXPORT PyObject* PyNumber_Float(PyObject* obj) {
  Thread* thread = Thread::current();
  if (obj == nullptr) {
    return nullError(thread);
  }
  HandleScope scope(thread);
  Object object(&scope, ApiHandle::fromPyObject(obj)->asObject());
  Object flt(&scope, thread->invokeFunction1(SymbolId::kBuiltins,
                                             SymbolId::kFloat, object));
  return flt.isError() ? nullptr : ApiHandle::newReference(thread, *flt);
}

PY_EXPORT PyObject* PyNumber_FloorDivide(PyObject* left, PyObject* right) {
  return doBinaryOp(SymbolId::kFloordiv, left, right);
}

PY_EXPORT PyObject* PyNumber_Index(PyObject* item) {
  Thread* thread = Thread::current();
  if (item == nullptr) {
    return nullError(thread);
  }

  HandleScope scope(thread);
  Object obj(&scope, ApiHandle::fromPyObject(item)->asObject());
  Object index(&scope, intFromIndex(thread, obj));
  return index.isError() ? nullptr : ApiHandle::newReference(thread, *index);
}

PY_EXPORT PyObject* PyNumber_InPlaceAdd(PyObject* left, PyObject* right) {
  return doBinaryOp(SymbolId::kIadd, left, right);
}

PY_EXPORT PyObject* PyNumber_InPlaceAnd(PyObject* left, PyObject* right) {
  return doBinaryOp(SymbolId::kIand, left, right);
}

PY_EXPORT PyObject* PyNumber_InPlaceFloorDivide(PyObject* left,
                                                PyObject* right) {
  return doBinaryOp(SymbolId::kIfloordiv, left, right);
}

PY_EXPORT PyObject* PyNumber_InPlaceLshift(PyObject* left, PyObject* right) {
  return doBinaryOp(SymbolId::kIlshift, left, right);
}

PY_EXPORT PyObject* PyNumber_InPlaceMatrixMultiply(PyObject* left,
                                                   PyObject* right) {
  return doBinaryOp(SymbolId::kImatmul, left, right);
}

PY_EXPORT PyObject* PyNumber_InPlaceMultiply(PyObject* left, PyObject* right) {
  return doBinaryOp(SymbolId::kImul, left, right);
}

PY_EXPORT PyObject* PyNumber_InPlaceOr(PyObject* left, PyObject* right) {
  return doBinaryOp(SymbolId::kIor, left, right);
}

PY_EXPORT PyObject* PyNumber_InPlacePower(PyObject* base, PyObject* exponent,
                                          PyObject* divisor) {
  if (divisor == Py_None) {
    return doBinaryOp(SymbolId::kIpow, base, exponent);
  }
  UNIMPLEMENTED("ipow(base, exponent, divisor)");
}

PY_EXPORT PyObject* PyNumber_InPlaceRemainder(PyObject* left, PyObject* right) {
  return doBinaryOp(SymbolId::kImod, left, right);
}

PY_EXPORT PyObject* PyNumber_InPlaceRshift(PyObject* left, PyObject* right) {
  return doBinaryOp(SymbolId::kIrshift, left, right);
}

PY_EXPORT PyObject* PyNumber_InPlaceSubtract(PyObject* left, PyObject* right) {
  return doBinaryOp(SymbolId::kIsub, left, right);
}

PY_EXPORT PyObject* PyNumber_InPlaceTrueDivide(PyObject* left,
                                               PyObject* right) {
  return doBinaryOp(SymbolId::kItruediv, left, right);
}

PY_EXPORT PyObject* PyNumber_InPlaceXor(PyObject* left, PyObject* right) {
  return doBinaryOp(SymbolId::kIxor, left, right);
}

PY_EXPORT PyObject* PyNumber_Invert(PyObject* pyobj) {
  return doUnaryOp(SymbolId::kInvert, pyobj);
}

PY_EXPORT PyObject* PyNumber_Long(PyObject* obj) {
  Thread* thread = Thread::current();
  if (obj == nullptr) {
    return nullError(thread);
  }
  HandleScope scope(thread);
  Object object(&scope, ApiHandle::fromPyObject(obj)->asObject());
  Object result(&scope, thread->invokeFunction1(SymbolId::kBuiltins,
                                                SymbolId::kInt, object));
  if (result.isError()) {
    return nullptr;
  }
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT PyObject* PyNumber_Lshift(PyObject* left, PyObject* right) {
  return doBinaryOp(SymbolId::kLshift, left, right);
}

PY_EXPORT PyObject* PyNumber_MatrixMultiply(PyObject* left, PyObject* right) {
  return doBinaryOp(SymbolId::kMatmul, left, right);
}

PY_EXPORT PyObject* PyNumber_Multiply(PyObject* left, PyObject* right) {
  return doBinaryOp(SymbolId::kMul, left, right);
}

PY_EXPORT PyObject* PyNumber_Negative(PyObject* pyobj) {
  return doUnaryOp(SymbolId::kNeg, pyobj);
}

PY_EXPORT PyObject* PyNumber_Or(PyObject* left, PyObject* right) {
  return doBinaryOp(SymbolId::kOrUnder, left, right);
}

PY_EXPORT PyObject* PyNumber_Positive(PyObject* pyobj) {
  return doUnaryOp(SymbolId::kPos, pyobj);
}

PY_EXPORT PyObject* PyNumber_Power(PyObject* base, PyObject* exponent,
                                   PyObject* divisor) {
  if (divisor == Py_None) {
    return doBinaryOp(SymbolId::kPow, base, exponent);
  }
  UNIMPLEMENTED("pow(base, exponent, divisor)");
}

PY_EXPORT PyObject* PyNumber_Remainder(PyObject* left, PyObject* right) {
  return doBinaryOp(SymbolId::kMod, left, right);
}

PY_EXPORT PyObject* PyNumber_Rshift(PyObject* left, PyObject* right) {
  return doBinaryOp(SymbolId::kRshift, left, right);
}

PY_EXPORT PyObject* PyNumber_Subtract(PyObject* left, PyObject* right) {
  return doBinaryOp(SymbolId::kSub, left, right);
}

PY_EXPORT PyObject* PyNumber_ToBase(PyObject* /* n */, int /* e */) {
  UNIMPLEMENTED("PyNumber_ToBase");
}

PY_EXPORT PyObject* PyNumber_TrueDivide(PyObject* left, PyObject* right) {
  return doBinaryOp(SymbolId::kTruediv, left, right);
}

PY_EXPORT PyObject* PyNumber_Xor(PyObject* left, PyObject* right) {
  return doBinaryOp(SymbolId::kXor, left, right);
}

// Object Protocol

PY_EXPORT int PyObject_AsCharBuffer(PyObject* /* j */,
                                    const char** /* buffer */,
                                    Py_ssize_t* /* n */) {
  UNIMPLEMENTED("PyObject_AsCharBuffer");
}

PY_EXPORT int PyObject_AsReadBuffer(PyObject* /* j */,
                                    const void** /* buffer */,
                                    Py_ssize_t* /* n */) {
  UNIMPLEMENTED("PyObject_AsReadBuffer");
}

PY_EXPORT int PyObject_AsWriteBuffer(PyObject* /* j */, void** /* buffer */,
                                     Py_ssize_t* /* n */) {
  UNIMPLEMENTED("PyObject_AsWriteBuffer");
}

PY_EXPORT PyObject* PyObject_Call(PyObject* callable, PyObject* args,
                                  PyObject* kwargs) {
  Thread* thread = Thread::current();
  if (callable == nullptr) {
    return nullError(thread);
  }

  DCHECK(!thread->hasPendingException(),
         "may accidentally clear pending exception");

  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  word flags = 0;
  frame->pushValue(ApiHandle::fromPyObject(callable)->asObject());
  Object args_obj(&scope, ApiHandle::fromPyObject(args)->asObject());
  DCHECK(thread->runtime()->isInstanceOfTuple(*args_obj),
         "args mut be a tuple");
  frame->pushValue(*args_obj);
  if (kwargs != nullptr) {
    Object kwargs_obj(&scope, ApiHandle::fromPyObject(kwargs)->asObject());
    DCHECK(thread->runtime()->isInstanceOfDict(*kwargs_obj),
           "kwargs must be a dict");
    frame->pushValue(*kwargs_obj);
    flags |= CallFunctionExFlag::VAR_KEYWORDS;
  }

  // TODO(T30925218): Protect against native stack overflow.

  Object result(&scope, Interpreter::callEx(thread, frame, flags));
  if (result.isError()) return nullptr;
  return ApiHandle::newReference(thread, *result);
}

static word vaBuildValuePushFrame(Frame* frame, const char* format,
                                  std::va_list* va, int build_value_flags) {
  if (format == nullptr) return 0;
  word num_values = 0;
  Thread* thread = Thread::current();
  for (const char* f = format; *f != '\0';) {
    PyObject* value = makeValueFromFormat(&f, va, build_value_flags);
    if (value == nullptr) break;
    frame->pushValue(ApiHandle::stealReference(thread, value));
    num_values++;
  }
  return num_values;
}

static PyObject* callWithVarArgs(Thread* thread, const Object& callable,
                                 const char* format, std::va_list* va,
                                 int build_value_flags) {
  Frame* frame = thread->currentFrame();
  frame->pushValue(*callable);
  word nargs = vaBuildValuePushFrame(frame, format, va, build_value_flags);

  HandleScope scope(thread);
  Object result(&scope, Interpreter::call(thread, frame, nargs));
  if (result.isError()) return nullptr;
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT PyObject* PyObject_CallFunction(PyObject* callable,
                                          const char* format, ...) {
  Thread* thread = Thread::current();
  if (callable == nullptr) {
    return nullError(thread);
  }

  HandleScope scope(thread);
  Object callable_obj(&scope, ApiHandle::fromPyObject(callable)->asObject());
  va_list va;
  va_start(va, format);
  PyObject* result = callWithVarArgs(thread, callable_obj, format, &va, 0);
  va_end(va);
  return result;
}

static PyObject* callWithObjArgs(Thread* thread, const Object& callable,
                                 std::va_list va) {
  DCHECK(!thread->hasPendingException(),
         "may accidentally clear pending exception");

  Frame* frame = thread->currentFrame();
  frame->pushValue(*callable);
  word nargs = 0;
  for (PyObject* arg; (arg = va_arg(va, PyObject*)) != nullptr; nargs++) {
    frame->pushValue(ApiHandle::fromPyObject(arg)->asObject());
  }

  // TODO(T30925218): CPython tracks recursive calls before calling the function
  // through Py_EnterRecursiveCall, and we should probably do the same
  HandleScope scope(thread);
  Object result(&scope, Interpreter::call(thread, frame, nargs));
  if (result.isError()) return nullptr;
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT PyObject* PyObject_CallFunctionObjArgs(PyObject* callable, ...) {
  Thread* thread = Thread::current();
  if (callable == nullptr) {
    return nullError(thread);
  }
  HandleScope scope(thread);
  Object callable_obj(&scope, ApiHandle::fromPyObject(callable)->asObject());
  va_list va;
  va_start(va, callable);
  PyObject* result = callWithObjArgs(thread, callable_obj, va);
  va_end(va);
  return result;
}

PY_EXPORT PyObject* _PyObject_CallFunction_SizeT(PyObject* callable,
                                                 const char* format, ...) {
  Thread* thread = Thread::current();
  if (callable == nullptr) {
    return nullError(thread);
  }

  HandleScope scope(thread);
  Object callable_obj(&scope, ApiHandle::fromPyObject(callable)->asObject());
  va_list va;
  va_start(va, format);
  PyObject* result =
      callWithVarArgs(thread, callable_obj, format, &va, kFlagSizeT);
  va_end(va);
  return result;
}

PY_EXPORT PyObject* PyObject_CallMethod(PyObject* pyobj, const char* name,
                                        const char* format, ...) {
  Thread* thread = Thread::current();
  if (pyobj == nullptr) {
    return nullError(thread);
  }

  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object obj(&scope, ApiHandle::fromPyObject(pyobj)->asObject());
  Object callable(&scope, runtime->attributeAtByCStr(thread, obj, name));
  if (callable.isError()) return nullptr;

  va_list va;
  va_start(va, format);
  PyObject* result = callWithVarArgs(thread, callable, format, &va, 0);
  va_end(va);
  return result;
}

PY_EXPORT PyObject* PyObject_CallMethodObjArgs(PyObject* pyobj,
                                               PyObject* py_method_name, ...) {
  Thread* thread = Thread::current();
  if (pyobj == nullptr || py_method_name == nullptr) {
    return nullError(thread);
  }
  HandleScope scope(thread);
  Object obj(&scope, ApiHandle::fromPyObject(pyobj)->asObject());
  Object name(&scope, ApiHandle::fromPyObject(py_method_name)->asObject());
  name = attributeName(thread, name);
  if (name.isErrorException()) return nullptr;
  Object callable(&scope, thread->runtime()->attributeAt(thread, obj, name));
  if (callable.isError()) return nullptr;

  va_list va;
  va_start(va, py_method_name);
  PyObject* result = callWithObjArgs(thread, callable, va);
  va_end(va);
  return result;
}

PY_EXPORT PyObject* _PyObject_CallMethod_SizeT(PyObject* pyobj,
                                               const char* name,
                                               const char* format, ...) {
  Thread* thread = Thread::current();
  if (pyobj == nullptr) {
    return nullError(thread);
  }

  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object obj(&scope, ApiHandle::fromPyObject(pyobj)->asObject());
  Object callable(&scope, runtime->attributeAtByCStr(thread, obj, name));
  if (callable.isError()) return nullptr;

  va_list va;
  va_start(va, format);
  PyObject* result = callWithVarArgs(thread, callable, format, &va, kFlagSizeT);
  va_end(va);
  return result;
}

PY_EXPORT PyObject* PyObject_CallObject(PyObject* callable, PyObject* args) {
  Thread* thread = Thread::current();
  if (callable == nullptr) {
    return nullError(thread);
  }
  DCHECK(!thread->hasPendingException(),
         "may accidentally clear pending exception");
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  frame->pushValue(ApiHandle::fromPyObject(callable)->asObject());
  Object result(&scope, NoneType::object());
  if (args != nullptr) {
    Object args_obj(&scope, ApiHandle::fromPyObject(args)->asObject());
    if (!thread->runtime()->isInstanceOfTuple(*args_obj)) {
      thread->raiseWithFmt(LayoutId::kTypeError,
                           "argument list must be a tuple");
      return nullptr;
    }
    frame->pushValue(*args_obj);
    // TODO(T30925218): Protect against native stack overflow.
    result = Interpreter::callEx(thread, frame, 0);
  } else {
    result = Interpreter::call(thread, frame, 0);
  }
  if (result.isError()) return nullptr;
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT int PyObject_CheckBuffer_Func(PyObject* pyobj) {
  // TODO(T38246066): investigate the use of PyObjects as Buffers
  return PyBytes_Check(pyobj);
}

PY_EXPORT int PyObject_CheckReadBuffer(PyObject* /* j */) {
  UNIMPLEMENTED("PyObject_CheckReadBuffer");
}

PY_EXPORT int PyObject_DelItem(PyObject* /* o */, PyObject* /* y */) {
  UNIMPLEMENTED("PyObject_DelItem");
}

PY_EXPORT int PyObject_DelItemString(PyObject* /* o */, const char* /* y */) {
  UNIMPLEMENTED("PyObject_DelItemString");
}

PY_EXPORT PyObject* _PyObject_FastCallDict(PyObject* callable,
                                           PyObject** pyargs, Py_ssize_t n_args,
                                           PyObject* kwargs) {
  DCHECK(callable != nullptr, "callable must not be nullptr");
  Thread* thread = Thread::current();
  DCHECK(!thread->hasPendingException(),
         "may accidentally clear pending exception");
  DCHECK(n_args >= 0, "n_args must not be negative");

  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  frame->pushValue(ApiHandle::fromPyObject(callable)->asObject());
  DCHECK(n_args == 0 || pyargs != nullptr, "Args array must not be nullptr");
  Object result(&scope, NoneType::object());
  if (kwargs != nullptr) {
    Tuple args(&scope, thread->runtime()->newTuple(n_args));
    for (Py_ssize_t i = 0; i < n_args; i++) {
      args.atPut(i, ApiHandle::fromPyObject(pyargs[i])->asObject());
    }
    frame->pushValue(*args);
    Object kwargs_obj(&scope, ApiHandle::fromPyObject(kwargs)->asObject());
    DCHECK(thread->runtime()->isInstanceOfDict(*kwargs_obj),
           "kwargs must be a dict");
    frame->pushValue(*kwargs_obj);
    // TODO(T30925218): Protect against native stack overflow.
    result =
        Interpreter::callEx(thread, frame, CallFunctionExFlag::VAR_KEYWORDS);
  } else {
    for (Py_ssize_t i = 0; i < n_args; i++) {
      frame->pushValue(ApiHandle::fromPyObject(pyargs[i])->asObject());
    }
    // TODO(T30925218): Protect against native stack overflow.
    result = Interpreter::call(thread, frame, n_args);
  }
  if (result.isError()) return nullptr;
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT PyObject* _PyObject_FastCallKeywords(PyObject* /* e */,
                                               PyObject* const* /* k */,
                                               Py_ssize_t /* s */,
                                               PyObject* /* s */) {
  UNIMPLEMENTED("_PyObject_FastCallKeywords");
}

PY_EXPORT PyObject* PyObject_Format(PyObject* obj, PyObject* format_spec) {
  DCHECK(obj != nullptr, "obj should not be null");
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object object(&scope, ApiHandle::fromPyObject(obj)->asObject());
  Object format_spec_obj(&scope,
                         format_spec == nullptr
                             ? Str::empty()
                             : ApiHandle::fromPyObject(obj)->asObject());
  Object result(&scope,
                thread->invokeFunction2(SymbolId::kBuiltins, SymbolId::kFormat,
                                        object, format_spec_obj));
  if (result.isError()) {
    return nullptr;
  }
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT int PyObject_GetBuffer(PyObject* obj, Py_buffer* view, int flags) {
  DCHECK(obj != nullptr, "obj must not be nullptr");
  char* buffer;
  Py_ssize_t length;
  if (PyBytes_Check(obj)) {
    if (PyBytes_AsStringAndSize(obj, &buffer, &length) < 0) return -1;
  } else if (PyByteArray_Check(obj)) {
    // TODO(T54579154): This creates a copy of the object which does not stay in
    // sync. We should have a way to pin the memory to allow direct access.
    buffer = PyByteArray_AsString(obj);
    if (buffer == nullptr) return -1;
    length = PyByteArray_Size(obj);
  } else if (PyMemoryView_Check(obj)) {
    UNIMPLEMENTED("PyObject_GetBuffer() for memoryview");
  } else {
    Thread* thread = Thread::current();
    HandleScope scope(thread);
    Object obj_obj(&scope, ApiHandle::fromPyObject(obj)->asObject());
    Type type(&scope, thread->runtime()->typeOf(*obj_obj));
    if (type.isBuiltin()) {
      thread->raiseWithFmt(LayoutId::kTypeError,
                           "a bytes-like object is required, not '%T'",
                           &obj_obj);
      return -1;
    }
    // TODO(T38246066) handle subclasses or call bf_getbuffer type slot.
    UNIMPLEMENTED("subclasses / buffer protocol bf_getbuffer()");
  }
  return PyBuffer_FillInfo(view, obj, buffer, length, /*readonly=*/1, flags);
}

PY_EXPORT PyObject* PyObject_GetItem(PyObject* obj, PyObject* key) {
  Thread* thread = Thread::current();
  if (obj == nullptr || key == nullptr) {
    return nullError(thread);
  }
  HandleScope scope(thread);
  Object object(&scope, ApiHandle::fromPyObject(obj)->asObject());
  Object key_obj(&scope, ApiHandle::fromPyObject(key)->asObject());
  Object result(&scope, objectGetItem(thread, object, key_obj));
  if (result.isErrorException()) return nullptr;
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT PyObject* PyObject_GetIter(PyObject* pyobj) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object obj(&scope, ApiHandle::fromPyObject(pyobj)->asObject());
  Object result(
      &scope, Interpreter::createIterator(thread, thread->currentFrame(), obj));
  if (result.isError()) {
    return nullptr;
  }
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT int PyObject_IsInstance(PyObject* instance, PyObject* cls) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object object(&scope, ApiHandle::fromPyObject(instance)->asObject());
  Object classinfo(&scope, ApiHandle::fromPyObject(cls)->asObject());
  Object result(&scope, thread->invokeFunction2(SymbolId::kBuiltins,
                                                SymbolId::kIsinstance, object,
                                                classinfo));
  return result.isError() ? -1 : Bool::cast(*result).value();
}

PY_EXPORT int PyObject_IsSubclass(PyObject* derived, PyObject* cls) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object subclass(&scope, ApiHandle::fromPyObject(derived)->asObject());
  Object classinfo(&scope, ApiHandle::fromPyObject(cls)->asObject());
  Object result(&scope, thread->invokeFunction2(SymbolId::kBuiltins,
                                                SymbolId::kIssubclass, subclass,
                                                classinfo));
  return result.isError() ? -1 : Bool::cast(*result).value();
}

PY_EXPORT Py_ssize_t PyObject_Length(PyObject* pyobj) {
  return objectLength(pyobj);
}

PY_EXPORT Py_ssize_t PyObject_LengthHint(PyObject* obj,
                                         Py_ssize_t default_value) {
  Py_ssize_t res = objectLength(obj);
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  if (res < 0 && thread->hasPendingException()) {
    Object given_obj(&scope, thread->pendingExceptionType());
    Object exc_obj(&scope, runtime->typeAt(LayoutId::kTypeError));
    if (!givenExceptionMatches(thread, given_obj, exc_obj)) {
      return -1;
    }
    // Catch TypeError when obj does not have __len__.
    thread->clearPendingException();
  } else {
    return res;
  }

  Object object(&scope, ApiHandle::fromPyObject(obj)->asObject());
  Object length_hint(
      &scope, thread->invokeMethod1(object, SymbolId::kDunderLengthHint));
  if (length_hint.isErrorNotFound() || length_hint.isNotImplementedType()) {
    return default_value;
  }
  if (length_hint.isError()) {
    return -1;
  }
  if (!thread->runtime()->isInstanceOfInt(*length_hint)) {
    thread->raiseWithFmt(LayoutId::kTypeError,
                         "__length_hint__ must be an integer, not %T",
                         &length_hint);
    return -1;
  }
  Int index(&scope, intUnderlying(*length_hint));
  if (!index.isSmallInt()) {
    thread->raiseWithFmt(LayoutId::kOverflowError,
                         "cannot fit '%T' into an index-sized integer",
                         &length_hint);
    return -1;
  }
  if (index.isNegative()) {
    thread->raiseWithFmt(LayoutId::kValueError, "__len__() should return >= 0");
    return -1;
  }
  return index.asWord();
}

PY_EXPORT int PyObject_SetItem(PyObject* obj, PyObject* key, PyObject* value) {
  Thread* thread = Thread::current();
  if (obj == nullptr || key == nullptr || value == nullptr) {
    nullError(thread);
    return -1;
  }
  HandleScope scope(thread);
  Object object(&scope, ApiHandle::fromPyObject(obj)->asObject());
  Object key_obj(&scope, ApiHandle::fromPyObject(key)->asObject());
  Object value_obj(&scope, ApiHandle::fromPyObject(value)->asObject());
  Object result(&scope, objectSetItem(thread, object, key_obj, value_obj));
  return result.isErrorException() ? -1 : 0;
}

PY_EXPORT Py_ssize_t PyObject_Size(PyObject* pyobj) {
  return objectLength(pyobj);
}

PY_EXPORT PyTypeObject* Py_TYPE_Func(PyObject* pyobj) {
  Thread* thread = Thread::current();
  if (pyobj == nullptr) {
    nullError(thread);
    return nullptr;
  }

  HandleScope scope(thread);
  Object obj(&scope, ApiHandle::fromPyObject(pyobj)->asObject());
  return reinterpret_cast<PyTypeObject*>(
      ApiHandle::borrowedReference(thread, thread->runtime()->typeOf(*obj)));
}

PY_EXPORT PyObject* PyObject_Type(PyObject* pyobj) {
  Thread* thread = Thread::current();
  if (pyobj == nullptr) {
    return nullError(thread);
  }

  HandleScope scope(thread);
  Object obj(&scope, ApiHandle::fromPyObject(pyobj)->asObject());
  return ApiHandle::newReference(thread, thread->runtime()->typeOf(*obj));
}

PY_EXPORT const char* PyObject_TypeName(PyObject* /* obj */) {
  UNIMPLEMENTED("PyObject_TypeName");
}

// Sequence Protocol

PY_EXPORT void _Py_FreeCharPArray(char* const array[]) {
  for (Py_ssize_t i = 0; array[i] != nullptr; ++i) {
    PyMem_Free(array[i]);
  }
  PyMem_Free(const_cast<char**>(array));
}

PY_EXPORT char* const* _PySequence_BytesToCharpArray(PyObject* self) {
  Py_ssize_t argc = PySequence_Size(self);
  if (argc < 0) {
    DCHECK(argc == -1, "size cannot be negative (-1 denotes an error)");
    return nullptr;
  }

  if (argc > (kMaxWord / kPointerSize) - 1) {
    PyErr_NoMemory();
    return nullptr;
  }

  char** result = static_cast<char**>(PyMem_Malloc((argc + 1) * kPointerSize));
  if (result == nullptr) {
    PyErr_NoMemory();
    return nullptr;
  }

  for (Py_ssize_t i = 0; i < argc; ++i) {
    PyObject* item = PySequence_GetItem(self, i);
    if (item == nullptr) {
      // NULL terminate before freeing.
      result[i] = nullptr;
      _Py_FreeCharPArray(result);
      return nullptr;
    }
    char* data;
    if (PyBytes_AsStringAndSize(item, &data, nullptr) < 0) {
      // NULL terminate before freeing.
      result[i] = nullptr;
      Py_DECREF(item);
      _Py_FreeCharPArray(result);
      return nullptr;
    }
    Py_ssize_t size = PyBytes_GET_SIZE(item) + 1;
    result[i] = static_cast<char*>(PyMem_Malloc(size));
    if (result[i] == nullptr) {
      PyErr_NoMemory();
      Py_DECREF(item);
      _Py_FreeCharPArray(result);
      return nullptr;
    }
    std::memcpy(result[i], data, size);
    Py_DECREF(item);
  }

  result[argc] = nullptr;
  return result;
}

PY_EXPORT int PySequence_Check(PyObject* py_obj) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object obj(&scope, ApiHandle::fromPyObject(py_obj)->asObject());
  return thread->runtime()->isSequence(thread, obj);
}

PY_EXPORT PyObject* PySequence_Concat(PyObject* left, PyObject* right) {
  Thread* thread = Thread::current();
  if (left == nullptr || right == nullptr) {
    return nullError(thread);
  }
  if (!PySequence_Check(left) || !PySequence_Check(right)) {
    thread->raiseWithFmt(LayoutId::kTypeError,
                         "objects cannot be concatenated");
    return nullptr;
  }
  return PyNumber_Add(left, right);
}

PY_EXPORT int PySequence_Contains(PyObject* seq, PyObject* obj) {
  Thread* thread = Thread::current();
  if (seq == nullptr || obj == nullptr) {
    nullError(thread);
    return -1;
  }
  HandleScope scope(thread);
  Object seq_obj(&scope, ApiHandle::fromPyObject(seq)->asObject());
  Object object(&scope, ApiHandle::fromPyObject(obj)->asObject());
  Object result(
      &scope, thread->invokeFunction2(SymbolId::kOperator, SymbolId::kContains,
                                      seq_obj, object));
  if (result.isError()) {
    return -1;
  }
  return Bool::cast(*result).value() ? 1 : 0;
}

PY_EXPORT Py_ssize_t PySequence_Count(PyObject* seq, PyObject* obj) {
  Thread* thread = Thread::current();
  if (seq == nullptr || obj == nullptr) {
    nullError(thread);
    return -1;
  }
  HandleScope scope(thread);
  Object seq_obj(&scope, ApiHandle::fromPyObject(seq)->asObject());
  Object object(&scope, ApiHandle::fromPyObject(obj)->asObject());
  Object result(&scope,
                thread->invokeFunction2(SymbolId::kOperator, SymbolId::kCountOf,
                                        seq_obj, object));
  if (result.isError()) {
    return -1;
  }
  return SmallInt::cast(*result).value();
}

PY_EXPORT int PySequence_DelItem(PyObject* seq, Py_ssize_t idx) {
  Thread* thread = Thread::current();
  if (seq == nullptr) {
    return -1;
  }
  HandleScope scope(thread);
  Object seq_obj(&scope, ApiHandle::fromPyObject(seq)->asObject());
  Object idx_obj(&scope, thread->runtime()->newInt(idx));
  Object result(&scope, thread->invokeMethod2(seq_obj, SymbolId::kDunderDelitem,
                                              idx_obj));
  if (result.isError()) {
    return -1;
  }
  return 0;
}

static RawObject makeSlice(Thread* thread, Py_ssize_t low, Py_ssize_t high) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object start(&scope, runtime->newInt(low));
  Object stop(&scope, runtime->newInt(high));
  Object step(&scope, NoneType::object());
  return runtime->newSlice(start, stop, step);
}

PY_EXPORT int PySequence_DelSlice(PyObject* seq, Py_ssize_t low,
                                  Py_ssize_t high) {
  Thread* thread = Thread::current();
  if (seq == nullptr) {
    nullError(thread);
    return -1;
  }
  HandleScope scope(thread);
  Object slice(&scope, makeSlice(thread, low, high));
  Object seq_obj(&scope, ApiHandle::fromPyObject(seq)->asObject());
  Object result(
      &scope, thread->invokeMethod2(seq_obj, SymbolId::kDunderDelitem, slice));
  if (result.isError()) {
    if (result.isErrorNotFound()) {
      thread->raiseWithFmt(LayoutId::kTypeError,
                           "object does not support slice deletion");
    }
    return -1;
  }
  return 0;
}

PY_EXPORT PyObject* PySequence_Fast(PyObject* seq, const char* msg) {
  Thread* thread = Thread::current();
  if (seq == nullptr) {
    return nullError(thread);
  }
  HandleScope scope(thread);
  Object seq_obj(&scope, ApiHandle::fromPyObject(seq)->asObject());

  if (seq_obj.isList() || seq_obj.isTuple()) {
    return ApiHandle::newReference(thread, *seq_obj);
  }
  Object iter(&scope, Interpreter::createIterator(
                          thread, thread->currentFrame(), seq_obj));
  if (iter.isError()) {
    Object given(&scope, thread->pendingExceptionType());
    Runtime* runtime = thread->runtime();
    Object exc(&scope, runtime->typeAt(LayoutId::kTypeError));
    if (givenExceptionMatches(thread, given, exc)) {
      thread->setPendingExceptionValue(runtime->newStrFromCStr(msg));
    }
    return nullptr;
  }

  Object result(&scope, thread->invokeFunction1(SymbolId::kBuiltins,
                                                SymbolId::kList, seq_obj));
  if (result.isError()) {
    return nullptr;
  }
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT PyObject* PySequence_GetItem(PyObject* seq, Py_ssize_t idx) {
  Thread* thread = Thread::current();
  if (seq == nullptr) {
    return nullError(thread);
  }
  HandleScope scope(thread);
  Object seq_obj(&scope, ApiHandle::fromPyObject(seq)->asObject());
  Object idx_obj(&scope, thread->runtime()->newInt(idx));
  Object result(&scope, thread->invokeMethod2(seq_obj, SymbolId::kDunderGetitem,
                                              idx_obj));
  if (result.isError()) {
    if (result.isErrorNotFound()) {
      thread->raiseWithFmt(LayoutId::kTypeError, "could not call __getitem__");
    }
    return nullptr;
  }
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT PyObject* PySequence_ITEM_Func(PyObject* seq, Py_ssize_t i) {
  DCHECK(seq != nullptr, "sequence must not be nullptr");
  DCHECK(i >= 0, "index can't be negative");
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object seq_obj(&scope, ApiHandle::fromPyObject(seq)->asObject());
  DCHECK(thread->runtime()->isSequence(thread, seq_obj),
         "seq must be a sequence");
  Object idx(&scope, thread->runtime()->newInt(i));
  Object result(&scope,
                thread->invokeMethod2(seq_obj, SymbolId::kDunderGetitem, idx));
  if (result.isError()) return nullptr;
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT PyObject* PySequence_GetSlice(PyObject* seq, Py_ssize_t low,
                                        Py_ssize_t high) {
  Thread* thread = Thread::current();
  if (seq == nullptr) {
    return nullError(thread);
  }
  HandleScope scope(thread);
  Object slice(&scope, makeSlice(thread, low, high));
  Object seq_obj(&scope, ApiHandle::fromPyObject(seq)->asObject());
  Object result(
      &scope, thread->invokeMethod2(seq_obj, SymbolId::kDunderGetitem, slice));
  if (result.isError()) {
    if (result.isErrorNotFound()) {
      thread->raiseWithFmt(LayoutId::kTypeError, "could not call __getitem__");
    }
    return nullptr;
  }
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT int PySequence_In(PyObject* pyseq, PyObject* pyobj) {
  return PySequence_Contains(pyseq, pyobj);
}

PY_EXPORT Py_ssize_t PySequence_Index(PyObject* seq, PyObject* obj) {
  Thread* thread = Thread::current();
  if (seq == nullptr || obj == nullptr) {
    nullError(thread);
    return -1;
  }
  HandleScope scope(thread);
  Object seq_obj(&scope, ApiHandle::fromPyObject(seq)->asObject());
  Object object(&scope, ApiHandle::fromPyObject(obj)->asObject());
  Object result(&scope,
                thread->invokeFunction2(SymbolId::kOperator, SymbolId::kIndexOf,
                                        seq_obj, object));
  if (result.isError()) {
    return -1;
  }
  return SmallInt::cast(*result).value();
}

PY_EXPORT PyObject* PySequence_InPlaceConcat(PyObject* left, PyObject* right) {
  Thread* thread = Thread::current();
  if (left == nullptr || right == nullptr) {
    return nullError(thread);
  }
  HandleScope scope(thread);
  Object left_obj(&scope, ApiHandle::fromPyObject(left)->asObject());
  Object right_obj(&scope, ApiHandle::fromPyObject(right)->asObject());
  Object result(&scope,
                thread->invokeFunction2(SymbolId::kOperator, SymbolId::kIconcat,
                                        left_obj, right_obj));
  return result.isError() ? nullptr : ApiHandle::newReference(thread, *result);
}

PY_EXPORT PyObject* PySequence_InPlaceRepeat(PyObject* seq, Py_ssize_t count) {
  Thread* thread = Thread::current();
  if (seq == nullptr) {
    return nullError(thread);
  }
  HandleScope scope(thread);
  Object sequence(&scope, ApiHandle::fromPyObject(seq)->asObject());
  Object count_obj(&scope, thread->runtime()->newInt(count));
  Object result(&scope,
                thread->invokeFunction2(SymbolId::kOperator, SymbolId::kIrepeat,
                                        sequence, count_obj));
  return result.isError() ? nullptr : ApiHandle::newReference(thread, *result);
}

PY_EXPORT Py_ssize_t PySequence_Length(PyObject* pyobj) {
  return objectLength(pyobj);
}

PY_EXPORT PyObject* PySequence_List(PyObject* seq) {
  Thread* thread = Thread::current();
  if (seq == nullptr) {
    return nullError(thread);
  }
  HandleScope scope(thread);
  Object seq_obj(&scope, ApiHandle::fromPyObject(seq)->asObject());
  RawObject result =
      thread->invokeFunction1(SymbolId::kBuiltins, SymbolId::kList, seq_obj);
  return result.isError() ? nullptr : ApiHandle::newReference(thread, result);
}

PY_EXPORT PyObject* PySequence_Repeat(PyObject* pyseq, Py_ssize_t count) {
  Thread* thread = Thread::current();
  if (pyseq == nullptr) {
    return nullError(thread);
  }
  if (!PySequence_Check(pyseq)) {
    thread->raiseWithFmt(LayoutId::kTypeError, "object cannot be repeated");
    return nullptr;
  }
  PyObject* count_obj(PyLong_FromSsize_t(count));
  PyObject* result = PyNumber_Multiply(pyseq, count_obj);
  Py_DECREF(count_obj);
  return result;
}

PY_EXPORT int PySequence_SetItem(PyObject* seq, Py_ssize_t idx, PyObject* obj) {
  Thread* thread = Thread::current();
  if (seq == nullptr) {
    nullError(thread);
    return -1;
  }
  HandleScope scope(thread);
  Object seq_obj(&scope, ApiHandle::fromPyObject(seq)->asObject());
  Object idx_obj(&scope, thread->runtime()->newInt(idx));
  Object result(&scope, NoneType::object());
  if (obj == nullptr) {
    // Equivalent to PySequence_DelItem
    result = thread->invokeMethod2(seq_obj, SymbolId::kDunderDelitem, idx_obj);
  } else {
    Object object(&scope, ApiHandle::fromPyObject(obj)->asObject());
    result = thread->invokeMethod3(seq_obj, SymbolId::kDunderSetitem, idx_obj,
                                   object);
  }
  if (result.isError()) {
    if (result.isErrorNotFound()) {
      thread->raiseWithFmt(LayoutId::kTypeError, "object is not subscriptable");
    }
    return -1;
  }
  return 0;
}

PY_EXPORT int PySequence_SetSlice(PyObject* seq, Py_ssize_t low,
                                  Py_ssize_t high, PyObject* obj) {
  Thread* thread = Thread::current();
  if (seq == nullptr) {
    nullError(thread);
    return -1;
  }
  HandleScope scope(thread);
  Object slice(&scope, makeSlice(thread, low, high));
  Object seq_obj(&scope, ApiHandle::fromPyObject(seq)->asObject());
  Object result(&scope, NoneType::object());
  if (obj == nullptr) {
    result = thread->invokeMethod2(seq_obj, SymbolId::kDunderDelitem, slice);
  } else {
    Object object(&scope, ApiHandle::fromPyObject(obj)->asObject());
    result =
        thread->invokeMethod3(seq_obj, SymbolId::kDunderSetitem, slice, object);
  }
  if (result.isError()) {
    if (result.isErrorNotFound()) {
      thread->raiseWithFmt(LayoutId::kTypeError,
                           "object does not support slice assignment");
    }
    return -1;
  }
  return 0;
}

PY_EXPORT Py_ssize_t PySequence_Size(PyObject* pyobj) {
  return objectLength(pyobj);
}

PY_EXPORT PyObject* PySequence_Tuple(PyObject* seq) {
  Thread* thread = Thread::current();
  if (seq == nullptr) {
    return nullError(thread);
  }
  HandleScope scope(thread);
  Object seq_obj(&scope, ApiHandle::fromPyObject(seq)->asObject());
  if (seq_obj.isTuple()) {
    return ApiHandle::newReference(thread, *seq_obj);
  }
  Object result(&scope, thread->invokeFunction1(SymbolId::kBuiltins,
                                                SymbolId::kTuple, seq_obj));
  if (result.isError()) {
    return nullptr;
  }
  return ApiHandle::newReference(thread, *result);
}

}  // namespace py
