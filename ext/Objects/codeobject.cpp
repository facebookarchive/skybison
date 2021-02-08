#include <cmath>

#include "cpython-data.h"
#include "cpython-func.h"

#include "capi-handles.h"
#include "runtime.h"
#include "set-builtins.h"
#include "tuple-builtins.h"

namespace py {

static_assert(RawCode::Flags::kOptimized == CO_OPTIMIZED, "");
static_assert(RawCode::Flags::kNewlocals == CO_NEWLOCALS, "");
static_assert(RawCode::Flags::kVarargs == CO_VARARGS, "");
static_assert(RawCode::Flags::kVarkeyargs == CO_VARKEYWORDS, "");
static_assert(RawCode::Flags::kNested == CO_NESTED, "");
static_assert(RawCode::Flags::kGenerator == CO_GENERATOR, "");
static_assert(RawCode::Flags::kNofree == CO_NOFREE, "");
static_assert(RawCode::Flags::kCoroutine == CO_COROUTINE, "");
static_assert(RawCode::Flags::kIterableCoroutine == CO_ITERABLE_COROUTINE, "");
static_assert(RawCode::Flags::kAsyncGenerator == CO_ASYNC_GENERATOR, "");
static_assert(RawCode::Flags::kFutureDivision == CO_FUTURE_DIVISION, "");
static_assert(RawCode::Flags::kFutureAbsoluteImport ==
                  CO_FUTURE_ABSOLUTE_IMPORT,
              "");
static_assert(RawCode::Flags::kFutureWithStatement == CO_FUTURE_WITH_STATEMENT,
              "");
static_assert(RawCode::Flags::kFuturePrintFunction == CO_FUTURE_PRINT_FUNCTION,
              "");
static_assert(RawCode::Flags::kFutureUnicodeLiterals ==
                  CO_FUTURE_UNICODE_LITERALS,
              "");
static_assert(RawCode::Flags::kFutureBarryAsBdfl == CO_FUTURE_BARRY_AS_BDFL,
              "");
static_assert(RawCode::Flags::kFutureGeneratorStop == CO_FUTURE_GENERATOR_STOP,
              "");

PY_EXPORT int PyCode_Check_Func(PyObject* obj) {
  return ApiHandle::fromPyObject(obj)->asObject().isCode();
}

PY_EXPORT PyCodeObject* PyCode_NewWithPosOnlyArgs(
    int argcount, int posonlyargcount, int kwonlyargcount, int nlocals,
    int stacksize, int flags, PyObject* code, PyObject* consts, PyObject* names,
    PyObject* varnames, PyObject* freevars, PyObject* cellvars,
    PyObject* filename, PyObject* name, int firstlineno, PyObject* lnotab) {
  Thread* thread = Thread::current();
  if (argcount < 0 || posonlyargcount < 0 || kwonlyargcount < 0 ||
      nlocals < 0 || code == nullptr || consts == nullptr || names == nullptr ||
      varnames == nullptr || freevars == nullptr || cellvars == nullptr ||
      name == nullptr || filename == nullptr || lnotab == nullptr) {
    thread->raiseBadInternalCall();
    return nullptr;
  }
  HandleScope scope(thread);
  Object consts_obj(&scope, ApiHandle::fromPyObject(consts)->asObject());
  Object names_obj(&scope, ApiHandle::fromPyObject(names)->asObject());
  Object varnames_obj(&scope, ApiHandle::fromPyObject(varnames)->asObject());
  Object freevars_obj(&scope, ApiHandle::fromPyObject(freevars)->asObject());
  Object cellvars_obj(&scope, ApiHandle::fromPyObject(cellvars)->asObject());
  Object name_obj(&scope, ApiHandle::fromPyObject(name)->asObject());
  Object filename_obj(&scope, ApiHandle::fromPyObject(filename)->asObject());
  Object lnotab_obj(&scope, ApiHandle::fromPyObject(lnotab)->asObject());
  Object code_obj(&scope, ApiHandle::fromPyObject(code)->asObject());
  Runtime* runtime = thread->runtime();
  // Check argument types
  // TODO(emacs): Call equivalent of PyObject_CheckReadBuffer(code) instead of
  // isInstanceOfBytes
  if (!runtime->isInstanceOfBytes(*code_obj) ||
      !runtime->isInstanceOfTuple(*consts_obj) ||
      !runtime->isInstanceOfTuple(*names_obj) ||
      !runtime->isInstanceOfTuple(*varnames_obj) ||
      !runtime->isInstanceOfTuple(*freevars_obj) ||
      !runtime->isInstanceOfTuple(*cellvars_obj) ||
      !runtime->isInstanceOfStr(*name_obj) ||
      !runtime->isInstanceOfStr(*filename_obj) ||
      !runtime->isInstanceOfBytes(*lnotab_obj)) {
    thread->raiseBadInternalCall();
    return nullptr;
  }

  return reinterpret_cast<PyCodeObject*>(ApiHandle::newReference(
      runtime,
      runtime->newCode(argcount, posonlyargcount, kwonlyargcount, nlocals,
                       stacksize, flags, code_obj, consts_obj, names_obj,
                       varnames_obj, freevars_obj, cellvars_obj, filename_obj,
                       name_obj, firstlineno, lnotab_obj)));
}

PY_EXPORT PyCodeObject* PyCode_New(int argcount, int kwonlyargcount,
                                   int nlocals, int stacksize, int flags,
                                   PyObject* code, PyObject* consts,
                                   PyObject* names, PyObject* varnames,
                                   PyObject* freevars, PyObject* cellvars,
                                   PyObject* filename, PyObject* name,
                                   int firstlineno, PyObject* lnotab) {
  return PyCode_NewWithPosOnlyArgs(
      argcount, /*posonlyargcount=*/0, kwonlyargcount, nlocals, stacksize,
      flags, code, consts, names, varnames, freevars, cellvars, filename, name,
      firstlineno, lnotab);
}

PY_EXPORT PyCodeObject* PyCode_NewEmpty(const char* filename,
                                        const char* funcname, int firstlineno) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object empty_bytes(&scope, Bytes::empty());
  Object empty_tuple(&scope, runtime->emptyTuple());
  Object filename_obj(&scope, Runtime::internStrFromCStr(thread, filename));
  Object name_obj(&scope, Runtime::internStrFromCStr(thread, funcname));
  return reinterpret_cast<PyCodeObject*>(ApiHandle::newReference(
      runtime, runtime->newCode(/*argcount=*/0,
                                /*posonlyargcount=*/0,
                                /*kwonlyargcount=*/0,
                                /*nlocals=*/0,
                                /*stacksize=*/0,
                                /*flags=*/0,
                                /*code=*/empty_bytes,
                                /*consts=*/empty_tuple,
                                /*names=*/empty_tuple,
                                /*varnames=*/empty_tuple,
                                /*freevars=*/empty_tuple,
                                /*cellvars=*/empty_tuple,
                                /*filename=*/filename_obj,
                                /*name=*/name_obj,
                                /*firstlineno=*/firstlineno,
                                /*lnotab=*/empty_bytes)));
}

PY_EXPORT PyTypeObject* PyCode_Type_Ptr() {
  Runtime* runtime = Thread::current()->runtime();
  return reinterpret_cast<PyTypeObject*>(
      ApiHandle::borrowedReference(runtime, runtime->typeAt(LayoutId::kCode)));
}

PY_EXPORT Py_ssize_t PyCode_GetNumFree_Func(PyObject* code) {
  DCHECK(code != nullptr, "code must not be null");
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object code_obj(&scope, ApiHandle::fromPyObject(code)->asObject());
  DCHECK(code_obj.isCode(), "code must be a code object");
  Code code_code(&scope, *code_obj);
  Tuple freevars(&scope, code_code.freevars());
  return freevars.length();
}

PY_EXPORT PyObject* PyCode_GetName_Func(PyObject* code) {
  return ApiHandle::newReference(
      Thread::current()->runtime(),
      Code::cast(ApiHandle::fromPyObject(code)->asObject()).name());
}

PY_EXPORT PyObject* PyCode_GetFreevars_Func(PyObject* code) {
  return ApiHandle::newReference(
      Thread::current()->runtime(),
      Code::cast(ApiHandle::fromPyObject(code)->asObject()).freevars());
}

static RawObject constantKey(Thread* thread, const Object& obj) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  if (obj.isNoneType() || obj.isEllipsis() || obj.isInt() || obj.isBool() ||
      obj.isBytes() || obj.isStr() || obj.isCode()) {
    Object type(&scope, runtime->typeOf(*obj));
    return runtime->newTupleWith2(type, obj);
  }
  if (obj.isFloat()) {
    double d = Float::cast(*obj).value();
    Object type(&scope, runtime->typeOf(*obj));
    if (d == 0.0 && std::signbit(d)) {
      Object none(&scope, NoneType::object());
      return runtime->newTupleWith3(type, obj, none);
    }
    return runtime->newTupleWith2(type, obj);
  }
  if (obj.isComplex()) {
    Complex c(&scope, *obj);
    Py_complex z;
    z.real = c.real();
    z.imag = c.imag();
    // For the complex case we must make complex(x, 0.)
    // different from complex(x, -0.) and complex(0., y)
    // different from complex(-0., y), for any x and y.
    // All four complex zeros must be distinguished.
    bool real_negzero = z.real == 0.0 && std::signbit(z.real);
    bool imag_negzero = z.imag == 0.0 && std::signbit(z.imag);
    // use True, False and None singleton as tags for the real and imag sign,
    // to make tuples different
    Object type(&scope, runtime->typeOf(*obj));
    if (real_negzero && imag_negzero) {
      Object tru(&scope, Bool::trueObj());
      return runtime->newTupleWith3(type, obj, tru);
    }
    if (imag_negzero) {
      Object fals(&scope, Bool::falseObj());
      return runtime->newTupleWith3(type, obj, fals);
    }
    if (real_negzero) {
      Object none(&scope, NoneType::object());
      return runtime->newTupleWith3(type, obj, none);
    }
    return runtime->newTupleWith2(type, obj);
  }
  if (obj.isTuple()) {
    Tuple tuple(&scope, *obj);
    Tuple result(&scope, runtime->newTuple(tuple.length()));
    Object item(&scope, NoneType::object());
    Object item_key(&scope, NoneType::object());
    for (word i = 0; i < tuple.length(); i++) {
      item = tuple.at(i);
      item_key = constantKey(thread, item);
      if (item_key.isError()) return *item_key;
      result.atPut(i, *item_key);
    }
    return runtime->newTupleWith2(result, obj);
  }
  if (obj.isFrozenSet()) {
    FrozenSet set(&scope, *obj);
    Tuple seq(&scope, runtime->newTuple(set.numItems()));
    Object item(&scope, NoneType::object());
    Object item_key(&scope, NoneType::object());
    for (word j = 0, idx = 0; setNextItem(set, &idx, &item); j++) {
      item_key = constantKey(thread, item);
      if (item_key.isError()) return *item_key;
      seq.atPut(j, *item_key);
    }
    FrozenSet result(&scope, runtime->newFrozenSet());
    result = setUpdate(thread, result, seq);
    if (result.isError()) return *result;
    return runtime->newTupleWith2(result, obj);
  }
  PyObject* ptr = ApiHandle::borrowedReference(runtime, *obj);
  Object obj_id(&scope, runtime->newInt(reinterpret_cast<word>(ptr)));
  return runtime->newTupleWith2(obj_id, obj);
}

PY_EXPORT PyObject* _PyCode_ConstantKey(PyObject* op) {
  DCHECK(op != nullptr, "op must not be null");
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object obj(&scope, ApiHandle::fromPyObject(op)->asObject());
  Object result(&scope, constantKey(thread, obj));
  if (result.isError()) {
    return nullptr;
  }
  return ApiHandle::newReference(thread->runtime(), *result);
}

}  // namespace py
