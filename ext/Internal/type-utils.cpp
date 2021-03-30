#include "type-utils.h"

#include "capi-handles.h"
#include "globals.h"
#include "handles.h"
#include "runtime.h"
#include "symbols.h"
#include "thread.h"

namespace py {

static const SymbolId kParamsSelfValue[] = {ID(self), ID(value)};
static const SymbolId kParamsSelf[] = {ID(self)};

RawObject newExtCode(Thread* thread, const Object& name,
                     View<SymbolId> parameters, word flags,
                     BuiltinFunction function, void* slot_value) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object code_code(&scope,
                   SmallInt::fromAlignedCPtr(bit_cast<void*>(function)));
  Tuple empty_tuple(&scope, runtime->emptyTuple());
  Object varnames_obj(&scope, *empty_tuple);

  word num_parameters = parameters.length();
  if (num_parameters > 0) {
    MutableTuple varnames(&scope, runtime->newMutableTuple(num_parameters));
    for (word i = 0; i < num_parameters; i++) {
      varnames.atPut(i, runtime->symbols()->at(parameters.get(i)));
    }
    varnames_obj = varnames.becomeImmutable();
  }

  word argcount = num_parameters - ((flags & Code::Flags::kVarargs) != 0) -
                  ((flags & Code::Flags::kVarkeyargs) != 0);
  flags |= Code::Flags::kOptimized | Code::Flags::kNewlocals;

  Object filename(&scope, Str::empty());
  Object lnotab(&scope, Bytes::empty());
  Object ptr(&scope, runtime->newIntFromCPtr(slot_value));
  Tuple consts(&scope, runtime->newTupleWith1(ptr));
  return runtime->newCode(argcount, /*posonlyargcount=*/num_parameters,
                          /*kwonlyargcount=*/0,
                          /*nlocals=*/num_parameters,
                          /*stacksize=*/0, flags, code_code, consts,
                          /*names=*/empty_tuple, varnames_obj,
                          /*freevars=*/empty_tuple, /*cellvars=*/empty_tuple,
                          filename, name, /*firstlineno=*/0, lnotab);
}

static ALIGN_16 RawObject getterWrapper(Thread* thread, Arguments args) {
  getter func = reinterpret_cast<getter>(getNativeFunc(thread));
  PyObject* self = ApiHandle::newReference(thread->runtime(), args.get(0));
  PyObject* result = (*func)(self, nullptr);
  ApiHandle::fromPyObject(self)->decref();
  return ApiHandle::checkFunctionResult(thread, result);
}

static RawObject getSetGetter(Thread* thread, const Object& name,
                              PyGetSetDef* def) {
  if (def->get == nullptr) return NoneType::object();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Code code(&scope, newExtCode(thread, name, kParamsSelf,
                               /*flags=*/0, getterWrapper,
                               reinterpret_cast<void*>(def->get)));
  Object globals(&scope, NoneType::object());
  Function function(&scope,
                    runtime->newFunctionWithCode(thread, name, code, globals));
  if (def->doc != nullptr) {
    Object doc(&scope, runtime->newStrFromCStr(def->doc));
    function.setDoc(*doc);
  }
  return *function;
}

static ALIGN_16 RawObject setterWrapper(Thread* thread, Arguments args) {
  setter func = reinterpret_cast<setter>(getNativeFunc(thread));
  Runtime* runtime = thread->runtime();
  PyObject* self = ApiHandle::newReference(runtime, args.get(0));
  PyObject* value = ApiHandle::newReference(runtime, args.get(1));
  int result = func(self, value, nullptr);
  ApiHandle::fromPyObject(self)->decref();
  ApiHandle::fromPyObject(value)->decref();
  if (result < 0) return Error::exception();
  return NoneType::object();
}

static RawObject getSetSetter(Thread* thread, const Object& name,
                              PyGetSetDef* def) {
  if (def->set == nullptr) return NoneType::object();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Code code(&scope, newExtCode(thread, name, kParamsSelfValue,
                               /*flags=*/0, setterWrapper,
                               reinterpret_cast<void*>(def->set)));
  Object globals(&scope, NoneType::object());
  Function function(&scope,
                    runtime->newFunctionWithCode(thread, name, code, globals));
  if (def->doc != nullptr) {
    Object doc(&scope, runtime->newStrFromCStr(def->doc));
    function.setDoc(*doc);
  }
  return *function;
}

RawObject newGetSet(Thread* thread, const Object& name, PyGetSetDef* def) {
  HandleScope scope(thread);
  Object getter(&scope, getSetGetter(thread, name, def));
  Object setter(&scope, getSetSetter(thread, name, def));
  Object none(&scope, NoneType::object());
  return thread->runtime()->newProperty(getter, setter, none);
}

}  // namespace py
