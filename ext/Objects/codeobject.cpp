#include <cmath>

#include "capi-handles.h"
#include "cpython-data.h"
#include "cpython-func.h"
#include "runtime.h"
#include "set-builtins.h"
#include "tuple-builtins.h"

namespace py {

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
      thread,
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
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object code_obj(&scope, ApiHandle::fromPyObject(code)->asObject());
  DCHECK(code_obj.isCode(), "code should be a code object");
  Code code_code(&scope, *code_obj);
  return ApiHandle::newReference(thread, code_code.name());
}

PY_EXPORT PyObject* PyCode_GetFreevars_Func(PyObject* code) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object code_obj(&scope, ApiHandle::fromPyObject(code)->asObject());
  DCHECK(code_obj.isCode(), "code should be a code object");
  Code code_code(&scope, *code_obj);
  return ApiHandle::newReference(thread, code_code.freevars());
}

static RawObject constantKey(Thread* thread, const Object& obj) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  if (obj.isNoneType() || obj.isEllipsis() || obj.isInt() || obj.isBool() ||
      obj.isBytes() || obj.isStr() || obj.isCode()) {
    Tuple key(&scope, runtime->newTuple(2));
    key.atPut(0, runtime->typeOf(*obj));
    key.atPut(1, *obj);
    return *key;
  }
  if (obj.isFloat()) {
    double d = Float::cast(*obj).value();
    if (d == 0.0 && std::signbit(d)) {
      Tuple key(&scope, runtime->newTuple(3));
      key.atPut(0, runtime->typeOf(*obj));
      key.atPut(1, *obj);
      key.atPut(2, NoneType::object());
      return *key;
    }
    Tuple key(&scope, runtime->newTuple(2));
    key.atPut(0, runtime->typeOf(*obj));
    key.atPut(1, *obj);
    return *key;
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
    if (real_negzero && imag_negzero) {
      Tuple key(&scope, runtime->newTuple(3));
      key.atPut(0, runtime->typeOf(*obj));
      key.atPut(1, *obj);
      key.atPut(2, Bool::trueObj());
      return *key;
    }
    if (imag_negzero) {
      Tuple key(&scope, runtime->newTuple(3));
      key.atPut(0, runtime->typeOf(*obj));
      key.atPut(1, *obj);
      key.atPut(2, Bool::falseObj());
      return *key;
    }
    if (real_negzero) {
      Tuple key(&scope, runtime->newTuple(3));
      key.atPut(0, runtime->typeOf(*obj));
      key.atPut(1, *obj);
      key.atPut(2, NoneType::object());
      return *key;
    }
    Tuple key(&scope, runtime->newTuple(2));
    key.atPut(0, runtime->typeOf(*obj));
    key.atPut(1, *obj);
    return *key;
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
    Tuple key(&scope, runtime->newTuple(2));
    key.atPut(0, *result);
    key.atPut(1, *obj);
    return *key;
  }
  if (obj.isFrozenSet()) {
    FrozenSet set(&scope, *obj);
    Tuple data(&scope, set.data());
    Tuple seq(&scope, runtime->newTuple(set.numItems()));
    Object item(&scope, NoneType::object());
    Object item_key(&scope, NoneType::object());
    for (word j = 0, idx = Set::Bucket::kFirst;
         Set::Bucket::nextItem(*data, &idx); j++) {
      item = Set::Bucket::value(*data, idx);
      item_key = constantKey(thread, item);
      if (item_key.isError()) return *item_key;
      seq.atPut(j, *item_key);
    }
    FrozenSet result(&scope, runtime->newFrozenSet());
    result = setUpdate(thread, result, seq);
    if (result.isError()) return *result;
    Tuple key(&scope, runtime->newTuple(2));
    key.atPut(0, *result);
    key.atPut(1, *obj);
    return *key;
  }
  PyObject* ptr = ApiHandle::borrowedReference(thread, *obj);
  Object obj_id(&scope, runtime->newInt(reinterpret_cast<word>(ptr)));
  Tuple key(&scope, runtime->newTuple(2));
  key.atPut(0, *obj_id);
  key.atPut(1, *obj);
  return *key;
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
  return ApiHandle::newReference(thread, *result);
}

}  // namespace py
