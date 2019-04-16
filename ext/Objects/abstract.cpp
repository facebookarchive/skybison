#include <cstdarg>

#include "../Python/modsupport-internal.h"
#include "cpython-data.h"
#include "cpython-func.h"
#include "exception-builtins.h"
#include "frame.h"
#include "int-builtins.h"
#include "list-builtins.h"
#include "runtime.h"
#include "type-builtins.h"

namespace python {

static PyObject* nullError(Thread* thread) {
  if (!thread->hasPendingException()) {
    thread->raiseSystemErrorWithCStr("null argument to internal routine");
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
         thread->runtime()->symbols()->literalAt(op));
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
  Object len(&scope, thread->invokeMethod1(obj, SymbolId::kDunderLen));
  if (len.isError()) {
    if (!thread->hasPendingException()) {
      thread->raiseTypeErrorWithCStr("object has no len()");
    }
    return -1;
  }
  len = intFromIndex(thread, len);
  if (len.isError()) {
    return -1;
  }
  Int index(&scope, *len);
  if (index.numDigits() > 1) {
    thread->raiseOverflowErrorWithCStr(
        "cannot fit 'int' into an index-sized integer");
    return -1;
  }
  if (index.isNegative()) {
    thread->raiseValueErrorWithCStr("__len__() should return >= 0");
    return -1;
  }
  return index.asWord();
}

// Buffer Protocol

PY_EXPORT int PyBuffer_FillInfo(Py_buffer* view, PyObject* exporter, void* buf,
                                Py_ssize_t len, int readonly, int flags) {
  if (view == nullptr) {
    Thread::current()->raiseBufferErrorWithCStr(
        "PyBuffer_FillInfo: view==NULL argument is obsolete");
    return -1;
  }
  if ((flags & PyBUF_WRITABLE) == PyBUF_WRITABLE && readonly == 1) {
    Thread::current()->raiseBufferErrorWithCStr("Object is not writable.");
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

PY_EXPORT int PyBuffer_IsContiguous(const Py_buffer* /* w */, char /* r */) {
  UNIMPLEMENTED("PyBuffer_IsContiguous");
}

PY_EXPORT void PyBuffer_Release(Py_buffer* view) {
  DCHECK(view != nullptr, "view must not be nullptr");
  PyObject* pyobj = view->obj;
  if (pyobj == nullptr) return;

  // TODO(T38246066) call bf_releasebuffer type slot.
  DCHECK(PyBytes_Check(pyobj), "buffer protocol only implemented for bytes");

  view->obj = nullptr;
  Py_DECREF(pyobj);
}

// PyIndex_Check

PY_EXPORT int PyIndex_Check_Func(PyObject* obj) {
  DCHECK(obj != nullptr, "Got null argument");
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object num(&scope, ApiHandle::fromPyObject(obj)->asObject());
  return !Interpreter::lookupMethod(thread, thread->currentFrame(), num,
                                    SymbolId::kDunderIndex)
              .isError();
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
    if (!thread->hasPendingException()) {
      thread->raiseTypeErrorWithCStr("failed to call __next__ on iterable");
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

static PyObject* getItem(Thread* thread, const Object& obj, const Object& key) {
  HandleScope scope(thread);
  Object result(&scope,
                thread->invokeMethod2(obj, SymbolId::kDunderGetitem, key));
  if (result.isError()) {
    if (!thread->hasPendingException()) {
      thread->raiseTypeErrorWithCStr("object is not subscriptable");
    }
    return nullptr;
  }
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT PyObject* PyMapping_GetItemString(PyObject* obj, const char* key) {
  Thread* thread = Thread::current();
  if (obj == nullptr || key == nullptr) {
    return nullError(thread);
  }
  HandleScope scope(thread);
  Object object(&scope, ApiHandle::fromPyObject(obj)->asObject());
  Object key_obj(&scope, thread->runtime()->newStrFromCStr(key));
  return getItem(thread, object, key_obj);
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

// TODO(T40432322): Re-inline into PyObject_GetIter
static RawObject getIter(Thread* thread, const Object& obj) {
  HandleScope scope(thread);
  Object iter(&scope, thread->invokeMethod1(obj, SymbolId::kDunderIter));
  Runtime* runtime = thread->runtime();
  if (iter.isError()) {
    // If the object is a sequence, make a new sequence iterator. It doesn't
    // need to have __iter__.
    if (runtime->isSequence(thread, obj)) {
      return runtime->newSeqIterator(obj);
    }
    if (!thread->hasPendingException()) {
      thread->raiseTypeErrorWithCStr("object is not iterable");
    }
    return Error::object();
  }
  // If the object has __iter__, ensure that the resulting object has __next__.
  Type type(&scope, runtime->typeOf(*iter));
  if (typeLookupSymbolInMro(thread, type, SymbolId::kDunderNext).isError()) {
    return thread->raiseTypeErrorWithCStr("iter() returned non-iterator");
  }
  return *iter;
}

// TODO(T40432322): Re-inline into PySequence_Fast
static RawObject sequenceFast(Thread* thread, const Object& seq,
                              const Str& msg) {
  if (seq.isList() || seq.isTuple()) {
    return *seq;
  }
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object iter(&scope, getIter(thread, seq));
  if (iter.isError()) {
    Object given(&scope, thread->pendingExceptionType());
    Object exc(&scope, runtime->typeAt(LayoutId::kTypeError));
    if (givenExceptionMatches(thread, given, exc)) {
      thread->setPendingExceptionValue(*msg);
    }
    return Error::object();
  }
  // TODO(T40274012): Re-write this function in terms of builtins.list
  List result(&scope, runtime->newList());
  if (listExtend(thread, result, seq).isError()) {
    return Error::object();
  }
  return *result;
}

// TODO(T40432322): Delete
static PyObject* callMappingMethod(Thread* thread, const Object& map,
                                   SymbolId method, const char* method_name) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object result(&scope, thread->invokeMethod1(map, method));
  if (result.isError()) {
    if (!thread->hasPendingException()) {
      thread->raiseAttributeError(
          runtime->newStrFromFmt("could not call %s", method_name));
    }
    return nullptr;
  }
  Str msg(&scope, runtime->newStrFromFmt("%s are not iterable", method_name));
  result = sequenceFast(thread, result, msg);
  if (result.isError()) {
    return nullptr;
  }
  return ApiHandle::newReference(thread, *result);
}

// TODO(T40432322): Copy over wholesale from CPython 3.6
PY_EXPORT PyObject* PyMapping_Items(PyObject* mapping) {
  DCHECK(mapping != nullptr, "mapping was null");
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object map(&scope, ApiHandle::fromPyObject(mapping)->asObject());
  if (map.isDict()) {
    Dict dict(&scope, *map);
    return ApiHandle::newReference(thread,
                                   thread->runtime()->dictItems(thread, dict));
  }
  return callMappingMethod(thread, map, SymbolId::kItems, "o.items()");
}

// TODO(T40432322): Copy over wholesale from CPython 3.6
PY_EXPORT PyObject* PyMapping_Keys(PyObject* mapping) {
  DCHECK(mapping != nullptr, "mapping was null");
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object map(&scope, ApiHandle::fromPyObject(mapping)->asObject());
  if (map.isDict()) {
    Dict dict(&scope, *map);
    return ApiHandle::newReference(thread, thread->runtime()->dictKeys(dict));
  }
  return callMappingMethod(thread, map, SymbolId::kKeys, "o.keys()");
}

PY_EXPORT Py_ssize_t PyMapping_Length(PyObject* pyobj) {
  return objectLength(pyobj);
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

// TODO(T40432322): Copy over wholesale from CPython 3.6
PY_EXPORT PyObject* PyMapping_Values(PyObject* mapping) {
  DCHECK(mapping != nullptr, "mapping was null");
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object map(&scope, ApiHandle::fromPyObject(mapping)->asObject());
  if (map.isDict()) {
    Dict dict(&scope, *map);
    return ApiHandle::newReference(thread,
                                   thread->runtime()->dictValues(thread, dict));
  }
  return callMappingMethod(thread, map, SymbolId::kValues, "o.values()");
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
  index = intFromIndex(thread, index);
  if (index.isError()) return -1;
  Int number(&scope, *index);
  if (overflow_err == nullptr || number.numDigits() == 1) {
    // Overflows should be clipped, or value is already in range.
    return number.asWordSaturated();
  }
  // Value overflows, raise an exception.
  PyErr_Format(overflow_err, "cannot fit index into an index-sized integer");
  return -1;
}

PY_EXPORT int PyNumber_Check(PyObject* obj) {
  if (obj == nullptr) {
    return false;
  }

  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Frame* frame = thread->currentFrame();
  Object num(&scope, ApiHandle::fromPyObject(obj)->asObject());
  if (!Interpreter::lookupMethod(thread, frame, num, SymbolId::kDunderInt)
           .isError()) {
    return true;
  }
  if (!Interpreter::lookupMethod(thread, frame, num, SymbolId::kDunderFloat)
           .isError()) {
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
  for (const char* f = format; *f != '\0';) {
    PyObject* value = makeValueFromFormat(&f, va, build_value_flags);
    if (value == nullptr) break;
    frame->pushValue(ApiHandle::fromPyObject(value)->asObject());
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
  Object callable(&scope, runtime->attributeAtWithCStr(thread, obj, name));
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
  Object name_obj(&scope, ApiHandle::fromPyObject(py_method_name)->asObject());
  Object callable(&scope,
                  thread->runtime()->attributeAt(thread, obj, name_obj));
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
  Object callable(&scope, runtime->attributeAtWithCStr(thread, obj, name));
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
      thread->raiseTypeErrorWithCStr("argument list must be a tuple");
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
  if (PyBytes_Check(obj)) {
    char* buffer;
    Py_ssize_t length;
    if (PyBytes_AsStringAndSize(obj, &buffer, &length) < 0) return -1;
    return PyBuffer_FillInfo(view, obj, buffer, length, 1 /* readonly */,
                             flags);
  }
  // TODO(T38246066) call bf_getbuffer type slot.
  UNIMPLEMENTED("buffer protocol bf_getbuffer()");
}

PY_EXPORT PyObject* PyObject_GetItem(PyObject* obj, PyObject* key) {
  Thread* thread = Thread::current();
  if (obj == nullptr || key == nullptr) {
    return nullError(thread);
  }
  HandleScope scope(thread);
  Object object(&scope, ApiHandle::fromPyObject(obj)->asObject());
  Object key_obj(&scope, ApiHandle::fromPyObject(key)->asObject());
  return getItem(thread, object, key_obj);
}

PY_EXPORT PyObject* PyObject_GetIter(PyObject* pyobj) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object obj(&scope, ApiHandle::fromPyObject(pyobj)->asObject());
  Object result(&scope, getIter(thread, obj));
  if (result.isError()) {
    return nullptr;
  }
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT int PyObject_IsInstance(PyObject* /* t */, PyObject* /* s */) {
  UNIMPLEMENTED("PyObject_IsInstance");
}

PY_EXPORT int PyObject_IsSubclass(PyObject* /* d */, PyObject* /* s */) {
  UNIMPLEMENTED("PyObject_IsSubclass");
}

PY_EXPORT Py_ssize_t PyObject_Length(PyObject* pyobj) {
  return objectLength(pyobj);
}

PY_EXPORT Py_ssize_t PyObject_LengthHint(PyObject* /* o */,
                                         Py_ssize_t /* defaultvalue */) {
  UNIMPLEMENTED("PyObject_LengthHint");
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
  Object result(&scope, thread->invokeMethod3(object, SymbolId::kDunderSetitem,
                                              key_obj, value_obj));
  if (result.isError()) {
    if (!thread->hasPendingException()) {
      thread->raiseTypeErrorWithCStr("object does not support item assignment");
    }
    return -1;
  }
  return 0;
}

PY_EXPORT Py_ssize_t PyObject_Size(PyObject* pyobj) {
  return objectLength(pyobj);
}

PY_EXPORT PyObject* PyObject_Type(PyObject* pyobj) {
  Thread* thread = Thread::current();
  if (pyobj == nullptr) {
    return nullError(thread);
  }

  HandleScope scope(thread);
  Object obj(&scope, ApiHandle::fromPyObject(pyobj)->asObject());

  Runtime* runtime = thread->runtime();
  Type type(&scope, runtime->typeOf(*obj));
  return ApiHandle::newReference(thread, *type);
}

// Sequence Protocol

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
    thread->raiseTypeErrorWithCStr("objects cannot be concatenated");
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
  return RawBool::cast(*result).value() ? 1 : 0;
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
  return RawSmallInt::cast(*result).value();
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
  Slice slice(&scope, runtime->newSlice());
  slice.setStart(runtime->newInt(low));
  slice.setStop(runtime->newInt(high));
  return *slice;
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
    if (!thread->hasPendingException()) {
      thread->raiseTypeErrorWithCStr("object does not support slice deletion");
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
  Str msg_obj(&scope, thread->runtime()->newStrFromCStr(msg));
  Object result(&scope, sequenceFast(thread, seq_obj, msg_obj));
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
    if (!thread->hasPendingException()) {
      thread->raiseTypeErrorWithCStr("could not call __getitem__");
    }
    return nullptr;
  }
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
    if (!thread->hasPendingException()) {
      thread->raiseTypeErrorWithCStr("could not call __getitem__");
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
  return RawSmallInt::cast(*result).value();
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
  // TODO(T40274012): Re-write this function in terms of builtins.list
  Object result_obj(&scope, thread->runtime()->newList());
  if (result_obj.isError()) {
    return nullptr;
  }
  List result(&scope, *result_obj);
  Object seq_obj(&scope, ApiHandle::fromPyObject(seq)->asObject());
  result_obj = listExtend(thread, result, seq_obj);
  if (result_obj.isError()) {
    return nullptr;
  }
  return ApiHandle::newReference(thread, *result_obj);
}

PY_EXPORT PyObject* PySequence_Repeat(PyObject* pyseq, Py_ssize_t count) {
  Thread* thread = Thread::current();
  if (pyseq == nullptr) {
    return nullError(thread);
  }
  if (!PySequence_Check(pyseq)) {
    thread->raiseTypeErrorWithCStr("object cannot be repeated");
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
    if (!thread->hasPendingException()) {
      thread->raiseTypeErrorWithCStr("object is not subscriptable");
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
    if (!thread->hasPendingException()) {
      thread->raiseTypeErrorWithCStr(
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

}  // namespace python
