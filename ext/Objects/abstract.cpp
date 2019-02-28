#include <cstdarg>

#include "cpython-data.h"
#include "cpython-func.h"
#include "exception-builtins.h"
#include "frame.h"
#include "list-builtins.h"
#include "runtime.h"

namespace python {

static PyObject* nullError(Thread* thread) {
  if (!thread->hasPendingException()) {
    thread->raiseSystemErrorWithCStr("null argument to internal routine");
  }
  return nullptr;
}

static Py_ssize_t objectLength(PyObject* pyobj) {
  Thread* thread = Thread::currentThread();
  if (pyobj == nullptr) {
    nullError(thread);
    return -1;
  }

  HandleScope scope(thread);
  Object obj(&scope, ApiHandle::fromPyObject(pyobj)->asObject());
  Object len(&scope, thread->invokeMethod1(obj, SymbolId::kDunderLen));
  if (len.isError()) {
    if (!thread->hasPendingException()) {
      thread->raiseTypeErrorWithCStr("object has no __len__()");
    }
    return -1;
  }

  OptInt<Py_ssize_t> len_or_error;
  Runtime* runtime = thread->runtime();
  if (runtime->isInstanceOfInt(*len)) {
    len_or_error = RawInt::cast(*len).asInt<Py_ssize_t>();
  } else {
    Object len_index(&scope,
                     thread->invokeMethod1(len, SymbolId::kDunderIndex));
    if (len_index.isError()) {
      if (!thread->hasPendingException()) {
        thread->raiseTypeErrorWithCStr(
            "__len__() cannot be interpreted as an integer");
      }
      return -1;
    }
    if (!runtime->isInstanceOfInt(*len_index)) {
      thread->raiseTypeErrorWithCStr("__index__() returned non-int");
      return -1;
    }
    len_or_error = RawInt::cast(*len_index).asInt<Py_ssize_t>();
  }
  switch (len_or_error.error) {
    case CastError::None:
      if (len_or_error.value < 0) {
        thread->raiseValueErrorWithCStr("__len__() should be non-negative");
        return -1;
      }
      return len_or_error.value;
    case CastError::Overflow:
    case CastError::Underflow:
      thread->raiseOverflowErrorWithCStr(
          "cannot fit 'int' into an index-sized integer");
      return -1;
  }

  UNREACHABLE("RawInt::asInt() gave an invalid CastError");
}

PY_EXPORT int PyBuffer_FillInfo(Py_buffer* /* w */, PyObject* /* j */,
                                void* /* f */, Py_ssize_t /* n */, int /* y */,
                                int /* s */) {
  UNIMPLEMENTED("PyBuffer_FillInfo");
}

PY_EXPORT int PyBuffer_IsContiguous(const Py_buffer* /* w */, char /* r */) {
  UNIMPLEMENTED("PyBuffer_IsContiguous");
}

PY_EXPORT void PyBuffer_Release(Py_buffer* /* w */) {
  UNIMPLEMENTED("PyBuffer_Release");
}

PY_EXPORT int PyIndex_Check_Func(PyObject* obj) {
  DCHECK(obj != nullptr, "Got null argument");
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Object num(&scope, ApiHandle::fromPyObject(obj)->asObject());
  return !Interpreter::lookupMethod(thread, thread->currentFrame(), num,
                                    SymbolId::kDunderIndex)
              .isError();
}

PY_EXPORT PyObject* PyIter_Next(PyObject* iter) {
  Thread* thread = Thread::currentThread();
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

PY_EXPORT int PyMapping_Check(PyObject* py_obj) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Object obj(&scope, ApiHandle::fromPyObject(py_obj)->asObject());
  return thread->runtime()->isMapping(thread, obj);
}

static PyObject* getItem(Thread* thread, const Object& obj, const Object& key) {
  HandleScope scope(thread);
  Object result(&scope,
                thread->invokeMethod2(obj, SymbolId::kDunderGetItem, key));
  if (result.isError()) {
    if (!thread->hasPendingException()) {
      thread->raiseTypeErrorWithCStr("object is not subscriptable");
    }
    return nullptr;
  }
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT PyObject* PyMapping_GetItemString(PyObject* obj, const char* key) {
  Thread* thread = Thread::currentThread();
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
  if (runtime->lookupSymbolInMro(thread, type, SymbolId::kDunderNext)
          .isError()) {
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
          runtime->newStrFromFormat("could not call %s", method_name));
    }
    return nullptr;
  }
  Str msg(&scope,
          runtime->newStrFromFormat("%s are not iterable", method_name));
  result = sequenceFast(thread, result, msg);
  if (result.isError()) {
    return nullptr;
  }
  return ApiHandle::newReference(thread, *result);
}

// TODO(T40432322): Copy over wholesale from CPython 3.6
PY_EXPORT PyObject* PyMapping_Items(PyObject* mapping) {
  DCHECK(mapping != nullptr, "mapping was null");
  Thread* thread = Thread::currentThread();
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
  Thread* thread = Thread::currentThread();
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
    nullError(Thread::currentThread());
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
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Object map(&scope, ApiHandle::fromPyObject(mapping)->asObject());
  if (map.isDict()) {
    Dict dict(&scope, *map);
    return ApiHandle::newReference(thread,
                                   thread->runtime()->dictValues(thread, dict));
  }
  return callMappingMethod(thread, map, SymbolId::kValues, "o.values()");
}

PY_EXPORT PyObject* PyNumber_Absolute(PyObject* obj) {
  Thread* thread = Thread::currentThread();
  if (obj == nullptr) {
    return nullError(thread);
  }
  HandleScope scope(thread);
  Object object(&scope, ApiHandle::fromPyObject(obj)->asObject());
  Object result(&scope, thread->invokeFunction1(SymbolId::kOperator,
                                                SymbolId::kAbs, object));
  if (result.isError()) {
    return nullptr;
  }
  return ApiHandle::newReference(thread, *result);
}

static RawObject doBinaryOpImpl(Thread* thread, Interpreter::BinaryOp op,
                                const Object& left, const Object& right) {
  Frame* caller = thread->currentFrame();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  SymbolId selector = runtime->binaryOperationSelector(op);
  Object left_method(&scope,
                     Interpreter::lookupMethod(thread, caller, left, selector));

  SymbolId swapped_selector = runtime->swappedBinaryOperationSelector(op);
  Object left_reversed_method(
      &scope,
      Interpreter::lookupMethod(thread, caller, left, swapped_selector));
  Object right_reversed_method(
      &scope,
      Interpreter::lookupMethod(thread, caller, right, swapped_selector));

  bool try_other = true;
  if (!left_method.isError()) {
    if (runtime->shouldReverseBinaryOperation(
            thread, left, left_reversed_method, right, right_reversed_method)) {
      Object result(
          &scope, Interpreter::callMethod2(thread, caller,
                                           right_reversed_method, right, left));
      if (!result.isNotImplemented()) return *result;
      try_other = false;
    }
    Object result(&scope, Interpreter::callMethod2(thread, caller, left_method,
                                                   left, right));
    if (!result.isNotImplemented()) return *result;
  }
  if (try_other && !right_reversed_method.isError()) {
    Object result(
        &scope, Interpreter::callMethod2(thread, caller, right_reversed_method,
                                         right, left));
    if (!result.isNotImplemented()) return *result;
  }
  return Error::object();
}

static PyObject* doBinaryOp(PyObject* v, PyObject* w,
                            Interpreter::BinaryOp op) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Object left(&scope, ApiHandle::fromPyObject(v)->asObject());
  Object right(&scope, ApiHandle::fromPyObject(w)->asObject());
  Object result(&scope, doBinaryOpImpl(thread, op, left, right));
  if (!result.isError()) {
    return ApiHandle::newReference(thread, *result);
  }

  // TODO(T32655200): Once we have a real string formatter, use that instead of
  // converting the names to C strings here.
  Str ltype(&scope, Type::cast(runtime->typeOf(*left))->name());
  Str rtype(&scope, Type::cast(runtime->typeOf(*right))->name());
  unique_c_ptr<char> ltype_name(ltype.toCStr());
  unique_c_ptr<char> rtype_name(rtype.toCStr());
  thread->raiseTypeError(runtime->newStrFromFormat(
      "Cannot do binary op %ld for types '%s' and '%s'", static_cast<word>(op),
      ltype_name.get(), rtype_name.get()));
  return nullptr;
}

PY_EXPORT PyObject* PyNumber_Add(PyObject* v, PyObject* w) {
  return doBinaryOp(v, w, Interpreter::BinaryOp::ADD);
}

PY_EXPORT PyObject* PyNumber_Subtract(PyObject* v, PyObject* w) {
  return doBinaryOp(v, w, Interpreter::BinaryOp::SUB);
}

PY_EXPORT PyObject* PyNumber_Multiply(PyObject* v, PyObject* w) {
  return doBinaryOp(v, w, Interpreter::BinaryOp::MUL);
}

PY_EXPORT PyObject* PyNumber_MatrixMultiply(PyObject* v, PyObject* w) {
  return doBinaryOp(v, w, Interpreter::BinaryOp::MATMUL);
}

PY_EXPORT PyObject* PyNumber_FloorDivide(PyObject* v, PyObject* w) {
  return doBinaryOp(v, w, Interpreter::BinaryOp::FLOORDIV);
}

PY_EXPORT PyObject* PyNumber_TrueDivide(PyObject* v, PyObject* w) {
  return doBinaryOp(v, w, Interpreter::BinaryOp::TRUEDIV);
}

PY_EXPORT PyObject* PyNumber_Remainder(PyObject* v, PyObject* w) {
  return doBinaryOp(v, w, Interpreter::BinaryOp::MOD);
}

PY_EXPORT PyObject* PyNumber_Divmod(PyObject* v, PyObject* w) {
  return doBinaryOp(v, w, Interpreter::BinaryOp::DIVMOD);
}

PY_EXPORT PyObject* PyNumber_Lshift(PyObject* v, PyObject* w) {
  return doBinaryOp(v, w, Interpreter::BinaryOp::LSHIFT);
}

PY_EXPORT PyObject* PyNumber_Rshift(PyObject* v, PyObject* w) {
  return doBinaryOp(v, w, Interpreter::BinaryOp::RSHIFT);
}

PY_EXPORT PyObject* PyNumber_And(PyObject* v, PyObject* w) {
  return doBinaryOp(v, w, Interpreter::BinaryOp::AND);
}

PY_EXPORT PyObject* PyNumber_Or(PyObject* v, PyObject* w) {
  return doBinaryOp(v, w, Interpreter::BinaryOp::OR);
}

PY_EXPORT PyObject* PyNumber_Xor(PyObject* v, PyObject* w) {
  return doBinaryOp(v, w, Interpreter::BinaryOp::XOR);
}

PY_EXPORT int PyNumber_Check(PyObject* obj) {
  if (obj == nullptr) {
    return false;
  }

  Thread* thread = Thread::currentThread();
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

PY_EXPORT PyObject* PyNumber_Float(PyObject* /* o */) {
  UNIMPLEMENTED("PyNumber_Float");
}

PY_EXPORT PyObject* PyNumber_InPlaceAdd(PyObject* /* v */, PyObject* /* w */) {
  UNIMPLEMENTED("PyNumber_InPlaceAdd");
}

PY_EXPORT PyObject* PyNumber_InPlaceMultiply(PyObject* /* v */,
                                             PyObject* /* w */) {
  UNIMPLEMENTED("PyNumber_InPlaceMultiply");
}

PY_EXPORT PyObject* PyNumber_Index(PyObject* item) {
  Thread* thread = Thread::currentThread();
  if (item == nullptr) {
    return nullError(thread);
  }

  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  ApiHandle* handle = ApiHandle::fromPyObject(item);
  Object obj(&scope, handle->asObject());
  if (runtime->isInstanceOfInt(*obj)) {
    handle->incref();
    return item;
  }
  Object index(&scope, thread->invokeMethod1(obj, SymbolId::kDunderIndex));
  if (index.isError()) {
    if (!thread->hasPendingException()) {
      thread->raiseTypeErrorWithCStr(
          "object cannot be interpreted as an integer");
    }
    return nullptr;
  }
  if (!runtime->isInstanceOfInt(*index)) {
    thread->raiseTypeErrorWithCStr("__index__() returned non-int");
    return nullptr;
  }
  return ApiHandle::newReference(thread, *index);
}

PY_EXPORT PyObject* PyNumber_Invert(PyObject* pyobj) {
  Thread* thread = Thread::currentThread();
  if (pyobj == nullptr) {
    return nullError(thread);
  }
  HandleScope scope(thread);
  Object object(&scope, ApiHandle::fromPyObject(pyobj)->asObject());
  Object result(&scope, thread->invokeFunction1(SymbolId::kOperator,
                                                SymbolId::kInvert, object));
  if (result.isError()) {
    return nullptr;
  }
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT PyObject* PyNumber_Long(PyObject* /* o */) {
  UNIMPLEMENTED("PyNumber_Long");
}

PY_EXPORT PyObject* PyNumber_Negative(PyObject* pyobj) {
  Thread* thread = Thread::currentThread();
  if (pyobj == nullptr) {
    return nullError(thread);
  }
  HandleScope scope(thread);
  Object object(&scope, ApiHandle::fromPyObject(pyobj)->asObject());
  Object result(&scope, thread->invokeFunction1(SymbolId::kOperator,
                                                SymbolId::kNeg, object));
  if (result.isError()) {
    return nullptr;
  }
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT PyObject* PyNumber_Positive(PyObject* pyobj) {
  Thread* thread = Thread::currentThread();
  if (pyobj == nullptr) {
    return nullError(thread);
  }
  HandleScope scope(thread);
  Object object(&scope, ApiHandle::fromPyObject(pyobj)->asObject());
  Object result(&scope, thread->invokeFunction1(SymbolId::kOperator,
                                                SymbolId::kPos, object));
  if (result.isError()) {
    return nullptr;
  }
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT Py_ssize_t PyNumber_AsSsize_t(PyObject* /* m */, PyObject* /* r */) {
  UNIMPLEMENTED("PyNumber_AsSsize_t");
}

PY_EXPORT PyObject* PyNumber_InPlaceFloorDivide(PyObject* /* v */,
                                                PyObject* /* w */) {
  UNIMPLEMENTED("PyNumber_InPlaceFloorDivide");
}

PY_EXPORT PyObject* PyNumber_InPlaceMatrixMultiply(PyObject* /* v */,
                                                   PyObject* /* w */) {
  UNIMPLEMENTED("PyNumber_InPlaceMatrixMultiply");
}

PY_EXPORT PyObject* PyNumber_InPlacePower(PyObject* /* v */, PyObject* /* w */,
                                          PyObject* /* z */) {
  UNIMPLEMENTED("PyNumber_InPlacePower");
}

PY_EXPORT PyObject* PyNumber_InPlaceRemainder(PyObject* /* v */,
                                              PyObject* /* w */) {
  UNIMPLEMENTED("PyNumber_InPlaceRemainder");
}

PY_EXPORT PyObject* PyNumber_InPlaceTrueDivide(PyObject* /* v */,
                                               PyObject* /* w */) {
  UNIMPLEMENTED("PyNumber_InPlaceTrueDivide");
}

PY_EXPORT PyObject* PyNumber_Power(PyObject* /* v */, PyObject* /* w */,
                                   PyObject* /* z */) {
  UNIMPLEMENTED("PyNumber_Power");
}

PY_EXPORT PyObject* PyNumber_ToBase(PyObject* /* n */, int /* e */) {
  UNIMPLEMENTED("PyNumber_ToBase");
}

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

PY_EXPORT PyObject* PyObject_Call(PyObject* /* e */, PyObject* /* s */,
                                  PyObject* /* s */) {
  UNIMPLEMENTED("PyObject_Call");
}

PY_EXPORT PyObject* PyObject_CallFunction(PyObject* /* e */,
                                          const char* /* t */, ...) {
  UNIMPLEMENTED("PyObject_CallFunction");
}

PY_EXPORT PyObject* PyObject_CallFunctionObjArgs(PyObject* callable, ...) {
  Thread* thread = Thread::currentThread();
  if (callable == nullptr) {
    return nullError(thread);
  }

  DCHECK(!thread->hasPendingException(),
         "This function should not be called with an exception set as it might "
         "be cleared");

  HandleScope scope(thread);
  Object function(&scope, ApiHandle::fromPyObject(callable)->asObject());
  Frame* frame = thread->currentFrame();
  frame->pushValue(*function);

  word nargs = 0;
  {
    va_list vargs;
    va_start(vargs, callable);
    for (PyObject* arg; (arg = va_arg(vargs, PyObject*)) != nullptr; nargs++) {
      frame->pushValue(ApiHandle::fromPyObject(arg)->asObject());
    }
    va_end(vargs);
  }

  // TODO(T30925218): CPython tracks recursive calls before calling the function
  // through Py_EnterRecursiveCall, and we should probably do the same
  Object result(&scope, Interpreter::call(thread, frame, nargs));
  if (result.isError()) return nullptr;
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT PyObject* _PyObject_CallFunction_SizeT(PyObject* /* e */,
                                                 const char* /* t */, ...) {
  UNIMPLEMENTED("_PyObject_CallFunction_SizeT");
}

PY_EXPORT PyObject* PyObject_CallMethod(PyObject* /* j */, const char* /* e */,
                                        const char* /* t */, ...) {
  UNIMPLEMENTED("PyObject_CallMethod");
}

PY_EXPORT PyObject* PyObject_CallMethodObjArgs(PyObject* /* e */,
                                               PyObject* /* e */, ...) {
  UNIMPLEMENTED("PyObject_CallMethodObjArgs");
}

PY_EXPORT PyObject* _PyObject_CallMethod_SizeT(PyObject* /* j */,
                                               const char* /* e */,
                                               const char* /* t */, ...) {
  UNIMPLEMENTED("_PyObject_CallMethod_SizeT");
}

PY_EXPORT PyObject* PyObject_CallObject(PyObject* /* e */, PyObject* /* s */) {
  UNIMPLEMENTED("PyObject_CallObject");
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

PY_EXPORT PyObject* _PyObject_FastCallDict(PyObject* /* e */,
                                           PyObject* const* /* s */,
                                           Py_ssize_t /* s */,
                                           PyObject* /* s */) {
  UNIMPLEMENTED("_PyObject_FastCallDict");
}

PY_EXPORT PyObject* _PyObject_FastCallKeywords(PyObject* /* e */,
                                               PyObject* const* /* k */,
                                               Py_ssize_t /* s */,
                                               PyObject* /* s */) {
  UNIMPLEMENTED("_PyObject_FastCallKeywords");
}

PY_EXPORT PyObject* PyObject_Format(PyObject* obj, PyObject* format_spec) {
  DCHECK(obj != nullptr, "obj should not be null");
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object object(&scope, ApiHandle::fromPyObject(obj)->asObject());
  Object format_spec_obj(&scope,
                         format_spec == nullptr
                             ? runtime->newStrFromCStr("")
                             : ApiHandle::fromPyObject(obj)->asObject());
  Object result(&scope,
                thread->invokeFunction2(SymbolId::kBuiltins, SymbolId::kFormat,
                                        object, format_spec_obj));
  if (result.isError()) {
    return nullptr;
  }
  return ApiHandle::newReference(thread, *result);
}

PY_EXPORT int PyObject_GetBuffer(PyObject* /* j */, Py_buffer* /* w */,
                                 int /* s */) {
  UNIMPLEMENTED("PyObject_GetBuffer");
}

PY_EXPORT PyObject* PyObject_GetItem(PyObject* obj, PyObject* key) {
  Thread* thread = Thread::currentThread();
  if (obj == nullptr || key == nullptr) {
    return nullError(thread);
  }
  HandleScope scope(thread);
  Object object(&scope, ApiHandle::fromPyObject(obj)->asObject());
  Object key_obj(&scope, ApiHandle::fromPyObject(key)->asObject());
  return getItem(thread, object, key_obj);
}

PY_EXPORT PyObject* PyObject_GetIter(PyObject* pyobj) {
  Thread* thread = Thread::currentThread();
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
  Thread* thread = Thread::currentThread();
  if (obj == nullptr || key == nullptr || value == nullptr) {
    nullError(thread);
    return -1;
  }
  HandleScope scope(thread);
  Object object(&scope, ApiHandle::fromPyObject(obj)->asObject());
  Object key_obj(&scope, ApiHandle::fromPyObject(key)->asObject());
  Object value_obj(&scope, ApiHandle::fromPyObject(value)->asObject());
  Object result(&scope, thread->invokeMethod3(object, SymbolId::kDunderSetItem,
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
  Thread* thread = Thread::currentThread();
  if (pyobj == nullptr) {
    return nullError(thread);
  }

  HandleScope scope(thread);
  Object obj(&scope, ApiHandle::fromPyObject(pyobj)->asObject());

  Runtime* runtime = thread->runtime();
  Type type(&scope, runtime->typeOf(*obj));
  return ApiHandle::newReference(thread, *type);
}

PY_EXPORT int PySequence_Check(PyObject* py_obj) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Object obj(&scope, ApiHandle::fromPyObject(py_obj)->asObject());
  return thread->runtime()->isSequence(thread, obj);
}

PY_EXPORT PyObject* PySequence_Concat(PyObject* left, PyObject* right) {
  Thread* thread = Thread::currentThread();
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
  Thread* thread = Thread::currentThread();
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
  Thread* thread = Thread::currentThread();
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
  Thread* thread = Thread::currentThread();
  if (seq == nullptr) {
    return -1;
  }
  HandleScope scope(thread);
  Object seq_obj(&scope, ApiHandle::fromPyObject(seq)->asObject());
  Object idx_obj(&scope, thread->runtime()->newInt(idx));
  Object result(&scope, thread->invokeMethod2(seq_obj, SymbolId::kDunderDelItem,
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
  Thread* thread = Thread::currentThread();
  if (seq == nullptr) {
    nullError(thread);
    return -1;
  }
  HandleScope scope(thread);
  Object slice(&scope, makeSlice(thread, low, high));
  Object seq_obj(&scope, ApiHandle::fromPyObject(seq)->asObject());
  Object result(
      &scope, thread->invokeMethod2(seq_obj, SymbolId::kDunderDelItem, slice));
  if (result.isError()) {
    if (!thread->hasPendingException()) {
      thread->raiseTypeErrorWithCStr("object does not support slice deletion");
    }
    return -1;
  }
  return 0;
}

PY_EXPORT PyObject* PySequence_Fast(PyObject* seq, const char* msg) {
  Thread* thread = Thread::currentThread();
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
  Thread* thread = Thread::currentThread();
  if (seq == nullptr) {
    return nullError(thread);
  }
  HandleScope scope(thread);
  Object seq_obj(&scope, ApiHandle::fromPyObject(seq)->asObject());
  Object idx_obj(&scope, thread->runtime()->newInt(idx));
  Object result(&scope, thread->invokeMethod2(seq_obj, SymbolId::kDunderGetItem,
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
  Thread* thread = Thread::currentThread();
  if (seq == nullptr) {
    return nullError(thread);
  }
  HandleScope scope(thread);
  Object slice(&scope, makeSlice(thread, low, high));
  Object seq_obj(&scope, ApiHandle::fromPyObject(seq)->asObject());
  Object result(
      &scope, thread->invokeMethod2(seq_obj, SymbolId::kDunderGetItem, slice));
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
  Thread* thread = Thread::currentThread();
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

PY_EXPORT PyObject* PySequence_InPlaceConcat(PyObject* /* s */,
                                             PyObject* /* o */) {
  UNIMPLEMENTED("PySequence_InPlaceConcat");
}

PY_EXPORT PyObject* PySequence_InPlaceRepeat(PyObject* /* o */,
                                             Py_ssize_t /* t */) {
  UNIMPLEMENTED("PySequence_InPlaceRepeat");
}

PY_EXPORT Py_ssize_t PySequence_Length(PyObject* pyobj) {
  return objectLength(pyobj);
}

PY_EXPORT PyObject* PySequence_List(PyObject* seq) {
  Thread* thread = Thread::currentThread();
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
  Thread* thread = Thread::currentThread();
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
  Thread* thread = Thread::currentThread();
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
    result = thread->invokeMethod2(seq_obj, SymbolId::kDunderDelItem, idx_obj);
  } else {
    Object object(&scope, ApiHandle::fromPyObject(obj)->asObject());
    result = thread->invokeMethod3(seq_obj, SymbolId::kDunderSetItem, idx_obj,
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
  Thread* thread = Thread::currentThread();
  if (seq == nullptr) {
    nullError(thread);
    return -1;
  }
  HandleScope scope(thread);
  Object slice(&scope, makeSlice(thread, low, high));
  Object seq_obj(&scope, ApiHandle::fromPyObject(seq)->asObject());
  Object result(&scope, NoneType::object());
  if (obj == nullptr) {
    result = thread->invokeMethod2(seq_obj, SymbolId::kDunderDelItem, slice);
  } else {
    Object object(&scope, ApiHandle::fromPyObject(obj)->asObject());
    result =
        thread->invokeMethod3(seq_obj, SymbolId::kDunderSetItem, slice, object);
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
  Thread* thread = Thread::currentThread();
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
