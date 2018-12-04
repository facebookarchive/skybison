#include "cpython-func.h"
#include "runtime.h"

namespace python {

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

PY_EXPORT PyObject* PyIter_Next(PyObject* /* r */) {
  UNIMPLEMENTED("PyIter_Next");
}

PY_EXPORT int PyMapping_Check(PyObject* py_obj) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Object obj(&scope, ApiHandle::fromPyObject(py_obj)->asObject());

  Runtime* runtime = thread->runtime();
  Type type(&scope, runtime->typeOf(obj));
  Object getitem(&scope, runtime->lookupSymbolInMro(thread, type,
                                                    SymbolId::kDunderGetItem));

  return !getitem->isError();
}

PY_EXPORT PyObject* PyMapping_GetItemString(PyObject* /* o */,
                                            const char* /* y */) {
  UNIMPLEMENTED("PyMapping_GetItemString");
}

PY_EXPORT int PyMapping_HasKey(PyObject* /* o */, PyObject* /* y */) {
  UNIMPLEMENTED("PyMapping_HasKey");
}

PY_EXPORT int PyMapping_HasKeyString(PyObject* /* o */, const char* /* y */) {
  UNIMPLEMENTED("PyMapping_HasKeyString");
}

PY_EXPORT PyObject* PyMapping_Items(PyObject* /* o */) {
  UNIMPLEMENTED("PyMapping_Items");
}

PY_EXPORT PyObject* PyMapping_Keys(PyObject* /* o */) {
  UNIMPLEMENTED("PyMapping_Keys");
}

PY_EXPORT Py_ssize_t PyMapping_Length(PyObject* /* o */) {
  UNIMPLEMENTED("PyMapping_Length");
}

PY_EXPORT int PyMapping_SetItemString(PyObject* /* o */, const char* /* y */,
                                      PyObject* /* e */) {
  UNIMPLEMENTED("PyMapping_SetItemString");
}

PY_EXPORT Py_ssize_t PyMapping_Size(PyObject* /* o */) {
  UNIMPLEMENTED("PyMapping_Size");
}

PY_EXPORT PyObject* PyMapping_Values(PyObject* /* o */) {
  UNIMPLEMENTED("PyMapping_Values");
}

PY_EXPORT PyObject* PyNumber_Absolute(PyObject* /* o */) {
  UNIMPLEMENTED("PyNumber_Absolute");
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
  if (!left_method->isError()) {
    if (runtime->shouldReverseBinaryOperation(
            thread, left, left_reversed_method, right, right_reversed_method)) {
      Object result(
          &scope, Interpreter::callMethod2(thread, caller,
                                           right_reversed_method, right, left));
      if (!result->isNotImplemented()) return *result;
      try_other = false;
    }
    Object result(&scope, Interpreter::callMethod2(thread, caller, left_method,
                                                   left, right));
    if (!result->isNotImplemented()) return *result;
  }
  if (try_other && !right_reversed_method->isError()) {
    Object result(
        &scope, Interpreter::callMethod2(thread, caller, right_reversed_method,
                                         right, left));
    if (!result->isNotImplemented()) return *result;
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
  if (!result->isError()) {
    return ApiHandle::newReference(thread, result);
  }

  // TODO(T32655200): Once we have a real string formatter, use that instead of
  // converting the names to C strings here.
  Str ltype(&scope, Type::cast(runtime->typeOf(*left))->name());
  Str rtype(&scope, Type::cast(runtime->typeOf(*right))->name());
  unique_c_ptr<char> ltype_name(ltype->toCStr());
  unique_c_ptr<char> rtype_name(rtype->toCStr());
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

PY_EXPORT int PyNumber_Check(PyObject* /* o */) {
  UNIMPLEMENTED("PyNumber_Check");
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
  HandleScope scope(thread);

  if (item == nullptr) {
    thread->raiseSystemErrorWithCStr("null argument to internal routine");
    return nullptr;
  }

  Object longobj(&scope, ApiHandle::fromPyObject(item)->asObject());
  if (longobj->isInt()) {
    Py_INCREF(item);
    return item;
  }

  Frame* frame = thread->currentFrame();
  Object index_meth(&scope, Interpreter::lookupMethod(thread, frame, longobj,
                                                      SymbolId::kDunderIndex));
  if (index_meth->isError()) {
    thread->raiseTypeErrorWithCStr(
        "object cannot be interpreted as an integer");
    return nullptr;
  }
  Object int_obj(&scope,
                 Interpreter::callMethod1(thread, frame, index_meth, longobj));
  if (!int_obj->isInt()) {
    thread->raiseTypeErrorWithCStr("__index__ returned non-int");
    return nullptr;
  }
  return ApiHandle::newReference(thread, *int_obj);
}

PY_EXPORT PyObject* PyNumber_Invert(PyObject* /* o */) {
  UNIMPLEMENTED("PyNumber_Invert");
}

PY_EXPORT PyObject* PyNumber_Long(PyObject* /* o */) {
  UNIMPLEMENTED("PyNumber_Long");
}

PY_EXPORT PyObject* PyNumber_Negative(PyObject* /* o */) {
  UNIMPLEMENTED("PyNumber_Negative");
}

PY_EXPORT PyObject* PyNumber_Positive(PyObject* /* o */) {
  UNIMPLEMENTED("PyNumber_Positive");
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

PY_EXPORT PyObject* PyObject_CallFunctionObjArgs(PyObject* /* e */, ...) {
  UNIMPLEMENTED("PyObject_CallFunctionObjArgs");
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

PY_EXPORT PyObject* PyObject_Format(PyObject* /* j */, PyObject* /* c */) {
  UNIMPLEMENTED("PyObject_Format");
}

PY_EXPORT int PyObject_GetBuffer(PyObject* /* j */, Py_buffer* /* w */,
                                 int /* s */) {
  UNIMPLEMENTED("PyObject_GetBuffer");
}

PY_EXPORT PyObject* PyObject_GetItem(PyObject* /* o */, PyObject* /* y */) {
  UNIMPLEMENTED("PyObject_GetItem");
}

PY_EXPORT PyObject* PyObject_GetIter(PyObject* /* o */) {
  UNIMPLEMENTED("PyObject_GetIter");
}

PY_EXPORT int PyObject_IsInstance(PyObject* /* t */, PyObject* /* s */) {
  UNIMPLEMENTED("PyObject_IsInstance");
}

PY_EXPORT int PyObject_IsSubclass(PyObject* /* d */, PyObject* /* s */) {
  UNIMPLEMENTED("PyObject_IsSubclass");
}

PY_EXPORT Py_ssize_t PyObject_Length(PyObject* /* o */) {
  UNIMPLEMENTED("PyObject_Length");
}

PY_EXPORT Py_ssize_t PyObject_LengthHint(PyObject* /* o */,
                                         Py_ssize_t /* defaultvalue */) {
  UNIMPLEMENTED("PyObject_LengthHint");
}

PY_EXPORT int PyObject_SetItem(PyObject* /* o */, PyObject* /* y */,
                               PyObject* /* e */) {
  UNIMPLEMENTED("PyObject_SetItem");
}

PY_EXPORT Py_ssize_t PyObject_Size(PyObject* /* o */) {
  UNIMPLEMENTED("PyObject_Size");
}

PY_EXPORT PyObject* PyObject_Type(PyObject* /* o */) {
  UNIMPLEMENTED("PyObject_Type");
}

PY_EXPORT int PySequence_Check(PyObject* py_obj) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);

  Object obj(&scope, ApiHandle::fromPyObject(py_obj)->asObject());
  Runtime* runtime = thread->runtime();
  if (runtime->isInstanceOfDict(obj)) {
    return 0;
  }

  Type type(&scope, runtime->typeOf(obj));
  Object getitem(&scope, runtime->lookupSymbolInMro(thread, type,
                                                    SymbolId::kDunderGetItem));

  return !getitem->isError();
}

PY_EXPORT PyObject* PySequence_Concat(PyObject* /* s */, PyObject* /* o */) {
  UNIMPLEMENTED("PySequence_Concat");
}

PY_EXPORT int PySequence_Contains(PyObject* /* q */, PyObject* /* b */) {
  UNIMPLEMENTED("PySequence_Contains");
}

PY_EXPORT Py_ssize_t PySequence_Count(PyObject* /* s */, PyObject* /* o */) {
  UNIMPLEMENTED("PySequence_Count");
}

PY_EXPORT int PySequence_DelItem(PyObject* /* s */, Py_ssize_t /* i */) {
  UNIMPLEMENTED("PySequence_DelItem");
}

PY_EXPORT int PySequence_DelSlice(PyObject* /* s */, Py_ssize_t /* 1 */,
                                  Py_ssize_t /* 2 */) {
  UNIMPLEMENTED("PySequence_DelSlice");
}

PY_EXPORT PyObject* PySequence_Fast(PyObject* /* v */, const char* /* m */) {
  UNIMPLEMENTED("PySequence_Fast");
}

PY_EXPORT PyObject* PySequence_GetItem(PyObject* /* s */, Py_ssize_t /* i */) {
  UNIMPLEMENTED("PySequence_GetItem");
}

PY_EXPORT PyObject* PySequence_GetSlice(PyObject* /* s */, Py_ssize_t /* 1 */,
                                        Py_ssize_t /* 2 */) {
  UNIMPLEMENTED("PySequence_GetSlice");
}

PY_EXPORT int PySequence_In(PyObject* /* w */, PyObject* /* v */) {
  UNIMPLEMENTED("PySequence_In");
}

PY_EXPORT Py_ssize_t PySequence_Index(PyObject* /* s */, PyObject* /* o */) {
  UNIMPLEMENTED("PySequence_Index");
}

PY_EXPORT PyObject* PySequence_InPlaceConcat(PyObject* /* s */,
                                             PyObject* /* o */) {
  UNIMPLEMENTED("PySequence_InPlaceConcat");
}

PY_EXPORT PyObject* PySequence_InPlaceRepeat(PyObject* /* o */,
                                             Py_ssize_t /* t */) {
  UNIMPLEMENTED("PySequence_InPlaceRepeat");
}

PY_EXPORT Py_ssize_t PySequence_Length(PyObject* /* s */) {
  UNIMPLEMENTED("PySequence_Length");
}

PY_EXPORT PyObject* PySequence_List(PyObject* /* v */) {
  UNIMPLEMENTED("PySequence_List");
}

PY_EXPORT PyObject* PySequence_Repeat(PyObject* /* o */, Py_ssize_t /* t */) {
  UNIMPLEMENTED("PySequence_Repeat");
}

PY_EXPORT int PySequence_SetItem(PyObject* /* s */, Py_ssize_t /* i */,
                                 PyObject* /* o */) {
  UNIMPLEMENTED("PySequence_SetItem");
}

PY_EXPORT int PySequence_SetSlice(PyObject* /* s */, Py_ssize_t /* 1 */,
                                  Py_ssize_t /* 2 */, PyObject* /* o */) {
  UNIMPLEMENTED("PySequence_SetSlice");
}

PY_EXPORT Py_ssize_t PySequence_Size(PyObject* /* s */) {
  UNIMPLEMENTED("PySequence_Size");
}

PY_EXPORT PyObject* PySequence_Tuple(PyObject* /* v */) {
  UNIMPLEMENTED("PySequence_Tuple");
}

}  // namespace python
