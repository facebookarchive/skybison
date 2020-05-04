// typeobject.c implementation

#include <cinttypes>

#include "cpython-data.h"
#include "cpython-func.h"
#include "cpython-types.h"

#include "builtins-module.h"
#include "capi-handles.h"
#include "dict-builtins.h"
#include "function-builtins.h"
#include "function-utils.h"
#include "handles.h"
#include "int-builtins.h"
#include "mro.h"
#include "objects.h"
#include "runtime.h"
#include "str-builtins.h"
#include "trampolines.h"
#include "type-builtins.h"
#include "utils.h"

namespace py {

PY_EXPORT PyTypeObject* PySuper_Type_Ptr() {
  Thread* thread = Thread::current();
  return reinterpret_cast<PyTypeObject*>(ApiHandle::borrowedReference(
      thread, thread->runtime()->typeAt(LayoutId::kSuper)));
}

PY_EXPORT int PyType_CheckExact_Func(PyObject* obj) {
  return ApiHandle::fromPyObject(obj)->asObject().isType();
}

PY_EXPORT int PyType_Check_Func(PyObject* obj) {
  return Thread::current()->runtime()->isInstanceOfType(
      ApiHandle::fromPyObject(obj)->asObject());
}

PY_EXPORT unsigned long PyType_GetFlags(PyTypeObject* type_obj) {
  CHECK(ApiHandle::isManaged(reinterpret_cast<PyObject*>(type_obj)),
        "Type is unmanaged. Please initialize using PyType_FromSpec");

  HandleScope scope;
  Type type(&scope, ApiHandle::fromPyTypeObject(type_obj)->asObject());
  if (type.isBuiltin()) return Py_TPFLAGS_DEFAULT;

  if (type.slots().isNoneType()) {
    UNIMPLEMENTED("GetFlags from types initialized through Python code");
  }

  Int flags(&scope, type.slot(Type::Slot::kFlags));
  return flags.asWord();
}

namespace {

// PyType_FromSpec() operator support
//
// The functions and data in this namespace, culminating in addOperators(), are
// used to add Python-visible wrappers for type slot C functions (e.g., passing
// a Py_nb_add slot will result in a __add__() method being added to the type).
// The wrapper functions (wrapUnaryfunc(), wrapBinaryfunc(), etc...) handle
// translating between incoming/outgoing RawObject/PyObject* values, along with
// various bits of slot-specific logic.
//
// As other builtins the function's Code object has a pointer to the
// appropriate wrapper function as its code field, and its consts field
// is a 1-element tuple containing a pointer to the slot function provided by
// the user. If this multi-step lookup ever becomes a performance problem, we
// can easily template the trampolines and/or the wrapper functions, but this
// keeps the code compact for now.

// Get an appropriately-typed function pointer out of the consts tuple in the
// Code object in the given Frame.
template <typename Func>
Func getNativeFunc(Thread* thread, Frame* frame) {
  HandleScope scope(thread);
  Code code(&scope, frame->code());
  Tuple consts(&scope, code.consts());
  DCHECK(consts.length() == 1, "Unexpected tuple length");
  Int raw_fn(&scope, consts.at(0));
  return bit_cast<Func>(raw_fn.asCPtr());
}

RawObject wrapUnaryfunc(Thread* thread, Frame* frame, word nargs) {
  auto func = getNativeFunc<unaryfunc>(thread, frame);

  Arguments args(frame, nargs);
  PyObject* o = ApiHandle::borrowedReference(thread, args.get(0));
  PyObject* result = (*func)(o);
  return ApiHandle::checkFunctionResult(thread, result);
}

// Common work for hashfunc, lenfunc, and inquiry, all of which take a single
// PyObject* and return an integral value.
template <class cfunc, class RetFunc>
RawObject wrapIntegralfunc(Thread* thread, Frame* frame, word nargs,
                           RetFunc ret) {
  auto func = getNativeFunc<cfunc>(thread, frame);

  Arguments args(frame, nargs);
  PyObject* o = ApiHandle::borrowedReference(thread, args.get(0));
  auto result = func(o);
  if (result == -1 && thread->hasPendingException()) return Error::exception();
  return ret(result);
}

RawObject wrapHashfunc(Thread* thread, Frame* frame, word nargs) {
  return wrapIntegralfunc<hashfunc>(
      thread, frame, nargs,
      [thread](Py_hash_t hash) { return thread->runtime()->newInt(hash); });
}

RawObject wrapLenfunc(Thread* thread, Frame* frame, word nargs) {
  return wrapIntegralfunc<lenfunc>(
      thread, frame, nargs,
      [thread](Py_ssize_t len) { return thread->runtime()->newInt(len); });
}

RawObject wrapInquirypred(Thread* thread, Frame* frame, word nargs) {
  return wrapIntegralfunc<inquiry>(
      thread, frame, nargs, [](int result) { return Bool::fromBool(result); });
}

RawObject wrapBinaryfuncImpl(Thread* thread, Frame* frame, word nargs,
                             bool swap) {
  auto func = getNativeFunc<binaryfunc>(thread, frame);

  Arguments args(frame, nargs);
  PyObject* o1 = ApiHandle::borrowedReference(thread, args.get(swap ? 1 : 0));
  PyObject* o2 = ApiHandle::borrowedReference(thread, args.get(swap ? 0 : 1));
  PyObject* result = (*func)(o1, o2);
  return ApiHandle::checkFunctionResult(thread, result);
}

RawObject wrapBinaryfunc(Thread* thread, Frame* frame, word nargs) {
  return wrapBinaryfuncImpl(thread, frame, nargs, false);
}

RawObject wrapBinaryfuncSwapped(Thread* thread, Frame* frame, word nargs) {
  return wrapBinaryfuncImpl(thread, frame, nargs, false);
}

RawObject wrapTernaryfuncImpl(Thread* thread, Frame* frame, word nargs,
                              bool swap) {
  auto func = getNativeFunc<ternaryfunc>(thread, frame);

  Arguments args(frame, nargs);
  PyObject* self = ApiHandle::borrowedReference(thread, args.get(swap ? 1 : 0));
  PyObject* value =
      ApiHandle::borrowedReference(thread, args.get(swap ? 0 : 1));
  PyObject* mod = ApiHandle::borrowedReference(thread, args.get(2));
  PyObject* result = (*func)(self, value, mod);
  return ApiHandle::checkFunctionResult(thread, result);
}

// wrapTernaryfunc() vs. wrapVarkwTernaryfunc():
// - wrapTernaryfunc(Swapped)(): Wraps a C function expecting exactly 3
//   normal arguments, with the 3rd argument defaulting to None.
// - wrapVarkwTernaryfunc(): Wraps a C function expecting a self argument, a
//   tuple of positional arguments and an optional dict of keyword arguments.
RawObject wrapTernaryfunc(Thread* thread, Frame* frame, word nargs) {
  return wrapTernaryfuncImpl(thread, frame, nargs, false);
}

RawObject wrapTernaryfuncSwapped(Thread* thread, Frame* frame, word nargs) {
  return wrapTernaryfuncImpl(thread, frame, nargs, true);
}

RawObject wrapVarkwTernaryfunc(Thread* thread, Frame* frame, word nargs) {
  DCHECK(nargs == 3, "Unexpected nargs");
  auto func = getNativeFunc<ternaryfunc>(thread, frame);

  Arguments args(frame, nargs);
  PyObject* self = ApiHandle::borrowedReference(thread, args.get(0));
  PyObject* varargs = ApiHandle::borrowedReference(thread, args.get(1));
  PyObject* kwargs = Dict::cast(args.get(2)).numItems() == 0
                         ? nullptr
                         : ApiHandle::borrowedReference(thread, args.get(2));
  PyObject* result = (*func)(self, varargs, kwargs);
  return ApiHandle::checkFunctionResult(thread, result);
}

RawObject wrapSetattr(Thread* thread, Frame* frame, word nargs) {
  auto func = getNativeFunc<setattrofunc>(thread, frame);

  Arguments args(frame, nargs);
  PyObject* self = ApiHandle::borrowedReference(thread, args.get(0));
  PyObject* name = ApiHandle::borrowedReference(thread, args.get(1));
  PyObject* value = ApiHandle::borrowedReference(thread, args.get(2));
  if (func(self, name, value) < 0) return Error::exception();
  return NoneType::object();
}

RawObject wrapDelattr(Thread* thread, Frame* frame, word nargs) {
  auto func = getNativeFunc<setattrofunc>(thread, frame);

  Arguments args(frame, nargs);
  PyObject* self = ApiHandle::borrowedReference(thread, args.get(0));
  PyObject* name = ApiHandle::borrowedReference(thread, args.get(1));
  if (func(self, name, nullptr) < 0) return Error::exception();
  return NoneType::object();
}

template <CompareOp op>
RawObject wrapRichcompare(Thread* thread, Frame* frame, word nargs) {
  auto func = getNativeFunc<richcmpfunc>(thread, frame);

  Arguments args(frame, nargs);
  PyObject* self = ApiHandle::borrowedReference(thread, args.get(0));
  PyObject* other = ApiHandle::borrowedReference(thread, args.get(1));
  PyObject* result = (*func)(self, other, op);
  return ApiHandle::checkFunctionResult(thread, result);
}

RawObject wrapNext(Thread* thread, Frame* frame, word nargs) {
  auto func = getNativeFunc<unaryfunc>(thread, frame);

  Arguments args(frame, nargs);
  PyObject* self = ApiHandle::borrowedReference(thread, args.get(0));
  PyObject* result = (*func)(self);
  if (result == nullptr && !thread->hasPendingException()) {
    return thread->raise(LayoutId::kStopIteration, NoneType::object());
  }
  return ApiHandle::checkFunctionResult(thread, result);
}

RawObject wrapDescrGet(Thread* thread, Frame* frame, word nargs) {
  auto func = getNativeFunc<descrgetfunc>(thread, frame);

  Arguments args(frame, nargs);
  PyObject* self = ApiHandle::borrowedReference(thread, args.get(0));
  PyObject* obj = nullptr;
  if (!args.get(1).isNoneType()) {
    obj = ApiHandle::borrowedReference(thread, args.get(1));
  }
  PyObject* type = nullptr;
  if (nargs >= 3 && !args.get(2).isNoneType()) {
    type = ApiHandle::borrowedReference(thread, args.get(2));
  }
  if (obj == nullptr && type == nullptr) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "__get__(None, None), is invalid");
  }
  PyObject* result = (*func)(self, obj, type);
  return ApiHandle::checkFunctionResult(thread, result);
}

RawObject wrapDescrSet(Thread* thread, Frame* frame, word nargs) {
  auto func = getNativeFunc<descrsetfunc>(thread, frame);

  Arguments args(frame, nargs);
  PyObject* self = ApiHandle::borrowedReference(thread, args.get(0));
  PyObject* obj = ApiHandle::borrowedReference(thread, args.get(1));
  PyObject* value = ApiHandle::borrowedReference(thread, args.get(2));
  if (func(self, obj, value) < 0) return Error::exception();
  return NoneType::object();
}

RawObject wrapDescrDelete(Thread* thread, Frame* frame, word nargs) {
  auto func = getNativeFunc<descrsetfunc>(thread, frame);

  Arguments args(frame, nargs);
  PyObject* self = ApiHandle::borrowedReference(thread, args.get(0));
  PyObject* obj = ApiHandle::borrowedReference(thread, args.get(1));
  if (func(self, obj, nullptr) < 0) return Error::exception();
  return NoneType::object();
}

RawObject wrapInit(Thread* thread, Frame* frame, word nargs) {
  DCHECK(nargs == 3, "Unexpected nargs");
  auto func = getNativeFunc<initproc>(thread, frame);

  Arguments args(frame, nargs);
  PyObject* self = ApiHandle::borrowedReference(thread, args.get(0));
  PyObject* varargs = ApiHandle::borrowedReference(thread, args.get(1));
  PyObject* kwargs = Dict::cast(args.get(2)).numItems() == 0
                         ? nullptr
                         : ApiHandle::borrowedReference(thread, args.get(2));
  if (func(self, varargs, kwargs) < 0) return Error::exception();
  return NoneType::object();
}

RawObject wrapDel(Thread* thread, Frame* frame, word nargs) {
  auto func = getNativeFunc<destructor>(thread, frame);

  Arguments args(frame, nargs);
  PyObject* self = ApiHandle::borrowedReference(thread, args.get(0));
  func(self);
  return NoneType::object();
}

RawObject wrapObjobjargproc(Thread* thread, Frame* frame, word nargs) {
  auto func = getNativeFunc<objobjargproc>(thread, frame);

  Arguments args(frame, nargs);
  PyObject* self = ApiHandle::borrowedReference(thread, args.get(0));
  PyObject* key = ApiHandle::borrowedReference(thread, args.get(1));
  PyObject* value = ApiHandle::borrowedReference(thread, args.get(2));
  int res = func(self, key, value);
  if (res == -1 && thread->hasPendingException()) return Error::exception();
  return NoneType::object();
}

RawObject wrapObjobjproc(Thread* thread, Frame* frame, word nargs) {
  auto func = getNativeFunc<objobjproc>(thread, frame);

  Arguments args(frame, nargs);
  PyObject* self = ApiHandle::borrowedReference(thread, args.get(0));
  PyObject* value = ApiHandle::borrowedReference(thread, args.get(1));
  int res = func(self, value);
  if (res == -1 && thread->hasPendingException()) return Error::exception();
  return Bool::fromBool(res);
}

RawObject wrapDelitem(Thread* thread, Frame* frame, word nargs) {
  auto func = getNativeFunc<objobjargproc>(thread, frame);

  Arguments args(frame, nargs);
  PyObject* self = ApiHandle::borrowedReference(thread, args.get(0));
  PyObject* key = ApiHandle::borrowedReference(thread, args.get(1));
  int res = func(self, key, nullptr);
  if (res == -1 && thread->hasPendingException()) return Error::exception();
  return NoneType::object();
}

// Convert obj into a word-sized int or raise an OverflowError, in the style of
// PyNumber_AsSsize_t().
static RawObject makeIndex(Thread* thread, const Object& obj) {
  HandleScope scope(thread);
  Object converted(&scope, intFromIndex(thread, obj));
  if (converted.isError()) return *converted;
  Int i(&scope, intUnderlying(*converted));
  if (i.numDigits() != 1) {
    return thread->raiseWithFmt(LayoutId::kOverflowError,
                                "cannot fit '%T' into an index-sized integer",
                                &obj);
  }
  return *i;
}

RawObject wrapIndexargfunc(Thread* thread, Frame* frame, word nargs) {
  auto func = getNativeFunc<ssizeargfunc>(thread, frame);

  HandleScope scope(thread);
  Arguments args(frame, nargs);
  PyObject* self = ApiHandle::borrowedReference(thread, args.get(0));
  Object arg(&scope, args.get(1));
  arg = makeIndex(thread, arg);
  if (arg.isError()) return *arg;
  PyObject* result = (*func)(self, Int::cast(*arg).asWord());
  return ApiHandle::checkFunctionResult(thread, result);
}

// First, convert arg to a word-sized int using makeIndex(). Then, if the result
// is negative, add len(self) to normalize it.
static RawObject normalizeIndex(Thread* thread, const Object& self,
                                const Object& arg) {
  HandleScope scope(thread);
  Object index(&scope, makeIndex(thread, arg));
  if (index.isError()) return *index;
  word i = Int::cast(*index).asWord();
  if (i >= 0) {
    return *index;
  }
  Object len(&scope, thread->invokeFunction1(ID(builtins), ID(len), self));
  if (len.isError()) return *len;
  len = makeIndex(thread, len);
  if (len.isError()) return *len;
  i += Int::cast(*len).asWord();
  return thread->runtime()->newInt(i);
}

RawObject wrapSqItem(Thread* thread, Frame* frame, word nargs) {
  auto func = getNativeFunc<ssizeargfunc>(thread, frame);

  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Object arg(&scope, args.get(1));
  arg = normalizeIndex(thread, self, arg);
  if (arg.isError()) return *arg;
  PyObject* py_self = ApiHandle::borrowedReference(thread, *self);
  PyObject* result = (*func)(py_self, Int::cast(*arg).asWord());
  return ApiHandle::checkFunctionResult(thread, result);
}

RawObject wrapSqSetitem(Thread* thread, Frame* frame, word nargs) {
  auto func = getNativeFunc<ssizeobjargproc>(thread, frame);

  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Object arg(&scope, args.get(1));
  arg = normalizeIndex(thread, self, arg);
  if (arg.isError()) return *arg;
  PyObject* py_self = ApiHandle::borrowedReference(thread, *self);
  PyObject* py_value = ApiHandle::borrowedReference(thread, args.get(2));
  int result = func(py_self, Int::cast(*arg).asWord(), py_value);
  if (result == -1 && thread->hasPendingException()) return Error::exception();
  return NoneType::object();
}

RawObject wrapSqDelitem(Thread* thread, Frame* frame, word nargs) {
  auto func = getNativeFunc<ssizeobjargproc>(thread, frame);

  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Object arg(&scope, args.get(1));
  arg = normalizeIndex(thread, self, arg);
  if (arg.isError()) return *arg;
  PyObject* py_self = ApiHandle::borrowedReference(thread, *self);
  int result = func(py_self, Int::cast(*arg).asWord(), nullptr);
  if (result == -1 && thread->hasPendingException()) return Error::exception();
  return NoneType::object();
}

// Information about a single type slot.
struct SlotDef {
  // The name of the method in managed code.
  SymbolId name;

  // Our equivalent of the slot id from PyType_Slot.
  Type::Slot id;

  // List of parameter names/symbols.
  const SymbolId* parameters;
  word num_parameters;

  // The wrapper function for this slot.
  Function::Entry wrapper;

  // RawCode::Flags to be set on slot function.
  word flags;

  // Doc string for the function.
  const char* doc;
};

static const SymbolId kParamsSelfArgsKwargs[] = {ID(self), ID(args),
                                                 ID(kwargs)};
static const SymbolId kParamsSelfInstanceOwner[] = {ID(self), ID(instance),
                                                    ID(owner)};
static const SymbolId kParamsSelfInstanceValue[] = {ID(self), ID(instance),
                                                    ID(value)};
static const SymbolId kParamsSelfInstance[] = {ID(self), ID(instance)};
static const SymbolId kParamsSelfKeyValue[] = {ID(self), ID(key), ID(value)};
static const SymbolId kParamsSelfKey[] = {ID(self), ID(key)};
static const SymbolId kParamsSelfNameValue[] = {ID(self), ID(name), ID(value)};
static const SymbolId kParamsSelfName[] = {ID(self), ID(name)};
static const SymbolId kParamsSelfValueMod[] = {ID(self), ID(value), ID(mod)};
static const SymbolId kParamsSelfValue[] = {ID(self), ID(value)};
static const SymbolId kParamsSelf[] = {ID(self)};
static const SymbolId kParamsTypeArgsKwargs[] = {ID(type), ID(args),
                                                 ID(kwargs)};

// These macros currently ignore the FUNCTION argument, which is still the
// function name inherited from CPython. This will be cleaned up when we add
// default slot implementations that delegate to the corresponding Python
// method, along with logic to update slots as needed when a user assigns to a
// type dict.
#define TPSLOT(NAME, SLOT, PARAMETERS, FUNCTION, WRAPPER, DOC)                 \
  { NAME, Type::Slot::SLOT, PARAMETERS, ARRAYSIZE(PARAMETERS), WRAPPER, 0, DOC }
#define KWSLOT(NAME, SLOT, PARAMETERS, FUNCTION, WRAPPER, DOC)                 \
  {                                                                            \
    NAME, Type::Slot::SLOT, PARAMETERS, ARRAYSIZE(PARAMETERS), WRAPPER,        \
        Code::Flags::kVarargs | Code::Flags::kVarkeyargs, DOC                  \
  }
#define UNSLOT(NAME, C_NAME, SLOT, FUNCTION, WRAPPER, DOC)                     \
  TPSLOT(NAME, SLOT, kParamsSelf, FUNCTION, WRAPPER,                           \
         C_NAME "($self, /)\n--\n\n" DOC)
#define IBSLOT(NAME, C_NAME, SLOT, FUNCTION, WRAPPER, DOC)                     \
  TPSLOT(NAME, SLOT, kParamsSelfValue, FUNCTION, WRAPPER,                      \
         C_NAME "($self, value, /)\n--\n\nReturn self" DOC "value.")
#define BINSLOT(NAME, C_NAME, SLOT, FUNCTION, DOC)                             \
  TPSLOT(NAME, SLOT, kParamsSelfValue, FUNCTION, wrapBinaryfunc,               \
         C_NAME "($self, value, /)\n--\n\nReturn self" DOC "value.")
#define RBINSLOT(NAME, C_NAME, SLOT, FUNCTION, DOC)                            \
  TPSLOT(NAME, SLOT, kParamsSelfValue, FUNCTION, wrapBinaryfuncSwapped,        \
         C_NAME "($self, value, /)\n--\n\nReturn value" DOC "self.")
#define BINSLOTNOTINFIX(NAME, C_NAME, SLOT, FUNCTION, DOC)                     \
  TPSLOT(NAME, SLOT, kParamsSelfValue, FUNCTION, wrapBinaryfunc,               \
         C_NAME "($self, value, /)\n--\n\n" DOC)
#define RBINSLOTNOTINFIX(NAME, C_NAME, SLOT, FUNCTION, DOC)                    \
  TPSLOT(NAME, SLOT, kParamsSelfValue, FUNCTION, wrapBinaryfuncSwapped,        \
         C_NAME "($self, value, /)\n--\n\n" DOC)

static const SlotDef kSlotdefs[] = {
    TPSLOT(ID(__getattribute__), kGetattr, kParamsSelfName, nullptr,
           bit_cast<Function::Entry>(nullptr), ""),
    TPSLOT(ID(__getattr__), kGetattr, kParamsSelfName, nullptr, nullptr, ""),
    TPSLOT(ID(__setattr__), kSetattr, kParamsSelfNameValue, nullptr, nullptr,
           ""),
    TPSLOT(ID(__delattr__), kSetattr, kParamsSelfName, nullptr, nullptr, ""),
    TPSLOT(ID(__repr__), kRepr, kParamsSelf, slot_tp_repr, wrapUnaryfunc,
           "__repr__($self, /)\n--\n\nReturn repr(self)."),
    TPSLOT(ID(__hash__), kHash, kParamsSelf, slot_tp_hash, wrapHashfunc,
           "__hash__($self, /)\n--\n\nReturn hash(self)."),
    KWSLOT(
        ID(__call__), kCall, kParamsSelfArgsKwargs, slot_tp_call,
        wrapVarkwTernaryfunc,
        "__call__($self, /, *args, **kwargs)\n--\n\nCall self as a function."),
    TPSLOT(ID(__str__), kStr, kParamsSelfArgsKwargs, slot_tp_str, wrapUnaryfunc,
           "__str__($self, /)\n--\n\nReturn str(self)."),
    TPSLOT(ID(__getattribute__), kGetattro, kParamsSelfName,
           slot_tp_getattr_hook, wrapBinaryfunc,
           "__getattribute__($self, name, /)\n--\n\nReturn getattr(self, "
           "name)."),
    TPSLOT(ID(__getattr__), kGetattro, kParamsSelfName, slot_tp_getattr_hook,
           nullptr, ""),
    TPSLOT(ID(__setattr__), kSetattro, kParamsSelfNameValue, slot_tp_setattro,
           wrapSetattr,
           "__setattr__($self, name, value, /)\n--\n\nImplement setattr(self, "
           "name, value)."),
    TPSLOT(ID(__delattr__), kSetattro, kParamsSelfName, slot_tp_setattro,
           wrapDelattr,
           "__delattr__($self, name, /)\n--\n\nImplement delattr(self, name)."),
    TPSLOT(ID(__lt__), kRichcompare, kParamsSelfValue, slot_tp_richcompare,
           wrapRichcompare<LT>,
           "__lt__($self, value, /)\n--\n\nReturn self<value."),
    TPSLOT(ID(__le__), kRichcompare, kParamsSelfValue, slot_tp_richcompare,
           wrapRichcompare<LE>,
           "__le__($self, value, /)\n--\n\nReturn self<=value."),
    TPSLOT(ID(__eq__), kRichcompare, kParamsSelfValue, slot_tp_richcompare,
           wrapRichcompare<EQ>,
           "__eq__($self, value, /)\n--\n\nReturn self==value."),
    TPSLOT(ID(__ne__), kRichcompare, kParamsSelfValue, slot_tp_richcompare,
           wrapRichcompare<NE>,
           "__ne__($self, value, /)\n--\n\nReturn self!=value."),
    TPSLOT(ID(__gt__), kRichcompare, kParamsSelfValue, slot_tp_richcompare,
           wrapRichcompare<GT>,
           "__gt__($self, value, /)\n--\n\nReturn self>value."),
    TPSLOT(ID(__ge__), kRichcompare, kParamsSelfValue, slot_tp_richcompare,
           wrapRichcompare<GE>,
           "__ge__($self, value, /)\n--\n\nReturn self>=value."),
    TPSLOT(ID(__iter__), kIter, kParamsSelf, slot_tp_iter, wrapUnaryfunc,
           "__iter__($self, /)\n--\n\nImplement iter(self)."),
    TPSLOT(ID(__next__), kIternext, kParamsSelf, slot_tp_iternext, wrapNext,
           "__next__($self, /)\n--\n\nImplement next(self)."),
    TPSLOT(ID(__get__), kDescrGet, kParamsSelfInstanceOwner, slot_tp_descr_get,
           wrapDescrGet,
           "__get__($self, instance, owner, /)\n--\n\nReturn an attribute of "
           "instance, which is of type owner."),
    TPSLOT(ID(__set__), kDescrSet, kParamsSelfInstanceValue, slot_tp_descr_set,
           wrapDescrSet,
           "__set__($self, instance, value, /)\n--\n\nSet an attribute of "
           "instance to value."),
    TPSLOT(ID(__delete__), kDescrSet, kParamsSelfInstance, slot_tp_descr_set,
           wrapDescrDelete,
           "__delete__($self, instance, /)\n--\n\nDelete an attribute of "
           "instance."),
    KWSLOT(ID(__init__), kInit, kParamsSelfArgsKwargs, slot_tp_init, wrapInit,
           "__init__($self, /, *args, **kwargs)\n--\n\nInitialize self.  See "
           "help(type(self)) for accurate signature."),
    KWSLOT(ID(__new__), kNew, kParamsTypeArgsKwargs, slot_tp_new,
           wrapVarkwTernaryfunc,
           "__new__(type, /, *args, **kwargs)\n--\n\n"
           "Create and return new object.  See help(type) for accurate "
           "signature."),
    TPSLOT(ID(__del__), kFinalize, kParamsSelf, slot_tp_finalize, wrapDel, ""),
    TPSLOT(ID(__await__), kAsyncAwait, kParamsSelf, slot_am_await,
           wrapUnaryfunc,
           "__await__($self, /)\n--\n\nReturn an iterator to be used in await "
           "expression."),
    TPSLOT(ID(__aiter__), kAsyncAiter, kParamsSelf, slot_am_aiter,
           wrapUnaryfunc,
           "__aiter__($self, /)\n--\n\nReturn an awaitable, that resolves in "
           "asynchronous iterator."),
    TPSLOT(ID(__anext__), kAsyncAnext, kParamsSelf, slot_am_anext,
           wrapUnaryfunc,
           "__anext__($self, /)\n--\n\nReturn a value or raise "
           "StopAsyncIteration."),
    BINSLOT(ID(__add__), "__add__", kNumberAdd, slot_nb_add, "+"),
    RBINSLOT(ID(__radd__), "__radd__", kNumberAdd, slot_nb_add, "+"),
    BINSLOT(ID(__sub__), "__sub__", kNumberSubtract, slot_nb_subtract, "-"),
    RBINSLOT(ID(__rsub__), "__rsub__", kNumberSubtract, slot_nb_subtract, "-"),
    BINSLOT(ID(__mul__), "__mul__", kNumberMultiply, slot_nb_multiply, "*"),
    RBINSLOT(ID(__rmul__), "__rmul__", kNumberMultiply, slot_nb_multiply, "*"),
    BINSLOT(ID(__mod__), "__mod__", kNumberRemainder, slot_nb_remainder, "%"),
    RBINSLOT(ID(__rmod__), "__rmod__", kNumberRemainder, slot_nb_remainder,
             "%"),
    BINSLOTNOTINFIX(ID(__divmod__), "__divmod__", kNumberDivmod, slot_nb_divmod,
                    "Return divmod(self, value)."),
    RBINSLOTNOTINFIX(ID(__rdivmod__), "__rdivmod__", kNumberDivmod,
                     slot_nb_divmod, "Return divmod(value, self)."),
    TPSLOT(ID(__pow__), kNumberPower, kParamsSelfValueMod, slot_nb_power,
           wrapTernaryfunc,
           "__pow__($self, value, mod=None, /)\n--\n\nReturn pow(self, value, "
           "mod)."),
    TPSLOT(ID(__rpow__), kNumberPower, kParamsSelfValueMod, slot_nb_power,
           wrapTernaryfuncSwapped,
           "__rpow__($self, value, mod=None, /)\n--\n\nReturn pow(value, self, "
           "mod)."),
    UNSLOT(ID(__neg__), "__neg__", kNumberNegative, slot_nb_negative,
           wrapUnaryfunc, "-self"),
    UNSLOT(ID(__pos__), "__pos__", kNumberPositive, slot_nb_positive,
           wrapUnaryfunc, "+self"),
    UNSLOT(ID(__abs__), "__abs__", kNumberAbsolute, slot_nb_absolute,
           wrapUnaryfunc, "abs(self)"),
    UNSLOT(ID(__bool__), "__bool__", kNumberBool, slot_nb_bool, wrapInquirypred,
           "self != 0"),
    UNSLOT(ID(__invert__), "__invert__", kNumberInvert, slot_nb_invert,
           wrapUnaryfunc, "~self"),
    BINSLOT(ID(__lshift__), "__lshift__", kNumberLshift, slot_nb_lshift, "<<"),
    RBINSLOT(ID(__rlshift__), "__rlshift__", kNumberLshift, slot_nb_lshift,
             "<<"),
    BINSLOT(ID(__rshift__), "__rshift__", kNumberRshift, slot_nb_rshift, ">>"),
    RBINSLOT(ID(__rrshift__), "__rrshift__", kNumberRshift, slot_nb_rshift,
             ">>"),
    BINSLOT(ID(__and__), "__and__", kNumberAnd, slot_nb_and, "&"),
    RBINSLOT(ID(__rand__), "__rand__", kNumberAnd, slot_nb_and, "&"),
    BINSLOT(ID(__xor__), "__xor__", kNumberXor, slot_nb_xor, "^"),
    RBINSLOT(ID(__rxor__), "__rxor__", kNumberXor, slot_nb_xor, "^"),
    BINSLOT(ID(__or__), "__or__", kNumberOr, slot_nb_or, "|"),
    RBINSLOT(ID(__ror__), "__ror__", kNumberOr, slot_nb_or, "|"),
    UNSLOT(ID(__int__), "__int__", kNumberInt, slot_nb_int, wrapUnaryfunc,
           "int(self)"),
    UNSLOT(ID(__float__), "__float__", kNumberFloat, slot_nb_float,
           wrapUnaryfunc, "float(self)"),
    IBSLOT(ID(__iadd__), "__iadd__", kNumberInplaceAdd, slot_nb_inplace_add,
           wrapBinaryfunc, "+="),
    IBSLOT(ID(__isub__), "__isub__", kNumberInplaceSubtract,
           slot_nb_inplace_subtract, wrapBinaryfunc, "-="),
    IBSLOT(ID(__imul__), "__imul__", kNumberInplaceMultiply,
           slot_nb_inplace_multiply, wrapBinaryfunc, "*="),
    IBSLOT(ID(__imod__), "__imod__", kNumberInplaceRemainder,
           slot_nb_inplace_remainder, wrapBinaryfunc, "%="),
    IBSLOT(ID(__ipow__), "__ipow__", kNumberInplacePower, slot_nb_inplace_power,
           wrapBinaryfunc, "**="),
    IBSLOT(ID(__ilshift__), "__ilshift__", kNumberInplaceLshift,
           slot_nb_inplace_lshift, wrapBinaryfunc, "<<="),
    IBSLOT(ID(__irshift__), "__irshift__", kNumberInplaceRshift,
           slot_nb_inplace_rshift, wrapBinaryfunc, ">>="),
    IBSLOT(ID(__iand__), "__iand__", kNumberInplaceAnd, slot_nb_inplace_and,
           wrapBinaryfunc, "&="),
    IBSLOT(ID(__ixor__), "__ixor__", kNumberInplaceXor, slot_nb_inplace_xor,
           wrapBinaryfunc, "^="),
    IBSLOT(ID(__ior__), "__ior__", kNumberInplaceOr, slot_nb_inplace_or,
           wrapBinaryfunc, "|="),
    BINSLOT(ID(__floordiv__), "__floordiv__", kNumberFloorDivide,
            slot_nb_floor_divide, "//"),
    RBINSLOT(ID(__rfloordiv__), "__rfloordiv__", kNumberFloorDivide,
             slot_nb_floor_divide, "//"),
    BINSLOT(ID(__truediv__), "__truediv__", kNumberTrueDivide,
            slot_nb_true_divide, "/"),
    RBINSLOT(ID(__rtruediv__), "__rtruediv__", kNumberTrueDivide,
             slot_nb_true_divide, "/"),
    IBSLOT(ID(__ifloordiv__), "__ifloordiv__", kNumberInplaceFloorDivide,
           slot_nb_inplace_floor_divide, wrapBinaryfunc, "//="),
    IBSLOT(ID(__itruediv__), "__itruediv__", kNumberInplaceTrueDivide,
           slot_nb_inplace_true_divide, wrapBinaryfunc, "/="),
    TPSLOT(ID(__index__), kNumberIndex, kParamsSelf, slot_nb_index,
           wrapUnaryfunc,
           "__index__($self, /)\n--\n\n"
           "Return self converted to an integer, if self is suitable "
           "for use as an index into a list."),
    BINSLOT(ID(__matmul__), "__matmul__", kNumberMatrixMultiply,
            slot_nb_matrix_multiply, "@"),
    RBINSLOT(ID(__rmatmul__), "__rmatmul__", kNumberMatrixMultiply,
             slot_nb_matrix_multiply, "@"),
    IBSLOT(ID(__imatmul__), "__imatmul__", kNumberInplaceMatrixMultiply,
           slot_nb_inplace_matrix_multiply, wrapBinaryfunc, "@="),
    TPSLOT(ID(__len__), kMapLength, kParamsSelf, slot_mp_length, wrapLenfunc,
           "__len__($self, /)\n--\n\nReturn len(self)."),
    TPSLOT(ID(__getitem__), kMapSubscript, kParamsSelfKey, slot_mp_subscript,
           wrapBinaryfunc,
           "__getitem__($self, key, /)\n--\n\nReturn self[key]."),
    TPSLOT(ID(__setitem__), kMapAssSubscript, kParamsSelfKeyValue,
           slot_mp_ass_subscript, wrapObjobjargproc,
           "__setitem__($self, key, value, /)\n--\n\nSet self[key] to value."),
    TPSLOT(ID(__delitem__), kMapAssSubscript, kParamsSelfKey,
           slot_mp_ass_subscript, wrapDelitem,
           "__delitem__($self, key, /)\n--\n\nDelete self[key]."),
    TPSLOT(ID(__len__), kSequenceLength, kParamsSelf, slot_sq_length,
           wrapLenfunc, "__len__($self, /)\n--\n\nReturn len(self)."),
    TPSLOT(ID(__add__), kSequenceConcat, kParamsSelfValue, nullptr,
           wrapBinaryfunc,
           "__add__($self, value, /)\n--\n\nReturn self+value."),
    TPSLOT(ID(__mul__), kSequenceRepeat, kParamsSelfValue, nullptr,
           wrapIndexargfunc,
           "__mul__($self, value, /)\n--\n\nReturn self*value."),
    TPSLOT(ID(__rmul__), kSequenceRepeat, kParamsSelfValue, nullptr,
           wrapIndexargfunc,
           "__rmul__($self, value, /)\n--\n\nReturn value*self."),
    TPSLOT(ID(__getitem__), kSequenceItem, kParamsSelfKey, slot_sq_item,
           wrapSqItem, "__getitem__($self, key, /)\n--\n\nReturn self[key]."),
    TPSLOT(ID(__setitem__), kSequenceAssItem, kParamsSelfKeyValue,
           slot_sq_ass_item, wrapSqSetitem,
           "__setitem__($self, key, value, /)\n--\n\nSet self[key] to value."),
    TPSLOT(ID(__delitem__), kSequenceAssItem, kParamsSelfKey, slot_sq_ass_item,
           wrapSqDelitem,
           "__delitem__($self, key, /)\n--\n\nDelete self[key]."),
    TPSLOT(ID(__contains__), kSequenceContains, kParamsSelfKey,
           slot_sq_contains, wrapObjobjproc,
           "__contains__($self, key, /)\n--\n\nReturn key in self."),
    TPSLOT(ID(__iadd__), kSequenceInplaceConcat, kParamsSelfValue, nullptr,
           wrapBinaryfunc,
           "__iadd__($self, value, /)\n--\n\nImplement self+=value."),
    TPSLOT(ID(__imul__), kSequenceInplaceRepeat, kParamsSelfValue, nullptr,
           wrapIndexargfunc,
           "__imul__($self, value, /)\n--\n\nImplement self*=value."),
};

static RawObject newExtCode(Thread* thread, const Object& name,
                            const SymbolId* parameters, word num_parameters,
                            word flags, void* fptr, const Object& slot_value) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object code_code(&scope, runtime->newIntFromCPtr(fptr));
  Tuple empty_tuple(&scope, runtime->emptyTuple());

  // TODO(T41024929): Many of the slot parameters should be positional only,
  // but we cannot express this in code objects (yet).
  Tuple varnames(&scope, runtime->newTuple(num_parameters));
  for (word i = 0; i < num_parameters; i++) {
    varnames.atPut(i, runtime->symbols()->at(parameters[i]));
  }

  word argcount = num_parameters - ((flags & Code::Flags::kVarargs) != 0) -
                  ((flags & Code::Flags::kVarkeyargs) != 0);
  flags |= Code::Flags::kOptimized | Code::Flags::kNewlocals;

  Object filename(&scope, Str::empty());
  Object lnotab(&scope, Bytes::empty());
  Tuple consts(&scope, runtime->newTuple(1));
  consts.atPut(0, *slot_value);
  return runtime->newCode(argcount, /*posonlyargcount=*/num_parameters,
                          /*kwonlyargcount=*/0,
                          /*nlocals=*/num_parameters,
                          /*stacksize=*/0, flags, code_code, consts,
                          /*names=*/empty_tuple, varnames,
                          /*freevars=*/empty_tuple, /*cellvars=*/empty_tuple,
                          filename, name, /*firstlineno=*/0, lnotab);
}

// For every entry in kSlotdefs with a non-null wrapper function, a slot id
// that was provided by the user, and no preexisting entry in the type dict, add
// a wrapper function to call the slot from Python.
//
// Returns Error if an exception was raised at any point, None otherwise.
RawObject addOperators(Thread* thread, const Type& type) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Str type_name(&scope, type.name());

  for (const SlotDef& slot : kSlotdefs) {
    if (slot.wrapper == nullptr) continue;
    Object slot_value(&scope, type.slot(slot.id));
    if (slot_value.isNoneType()) continue;
    DCHECK(slot_value.isInt(), "unexpected slot type");

    // Unlike most slots, we always allow __new__ to be overwritten by a subtype
    if (slot.id != Type::Slot::kNew &&
        !typeAtById(thread, type, slot.name).isError()) {
      continue;
    }

    // When given PyObject_HashNotImplemented, put None in the type dict
    // rather than a wrapper. CPython does this regardless of which slot it
    // was given for, so we do too.
    void* slot_value_ptr = Int::cast(*slot_value).asCPtr();
    if (slot_value_ptr == bit_cast<void*>(&PyObject_HashNotImplemented)) {
      Object none(&scope, NoneType::object());
      typeAtPutById(thread, type, slot.name, none);
      return NoneType::object();
    }

    Object func_obj(&scope, NoneType::object());
    if (slot_value_ptr == bit_cast<void*>(&PyObject_GenericGetAttr)) {
      func_obj = runtime->objectDunderGetattribute();
    } else if (slot_value_ptr == bit_cast<void*>(&PyObject_GenericSetAttr)) {
      func_obj = runtime->objectDunderSetattr();
    } else {
      // Create the wrapper function.
      Str slot_name(&scope, runtime->symbols()->at(slot.name));
      Str qualname(&scope,
                   runtime->newStrFromFmt("%S.%S", &type_name, &slot_name));
      Code code(&scope, newExtCode(thread, slot_name, slot.parameters,
                                   slot.num_parameters, slot.flags,
                                   bit_cast<void*>(slot.wrapper), slot_value));
      Object globals(&scope, NoneType::object());
      Function func(&scope, runtime->newFunctionWithCode(thread, qualname, code,
                                                         globals));
      if (slot.id == Type::Slot::kNumberPower) {
        func.setDefaults(runtime->newTuple(1));
      }

      // __new__ is the one special-case static method, so wrap it
      // appropriately.
      func_obj = *func;
      if (slot.id == Type::Slot::kNew) {
        func_obj =
            thread->invokeFunction1(ID(builtins), ID(staticmethod), func);
        if (func_obj.isError()) return *func_obj;
      }
    }

    // Finally, put the wrapper in the type dict.
    typeAtPutById(thread, type, slot.name, func_obj);
  }

  return NoneType::object();
}

// tp_new slot implementation that delegates to a Type's __new__ attribute.
PyObject* slotTpNew(PyObject* type, PyObject* args, PyObject* kwargs) {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Object type_obj(&scope, ApiHandle::fromPyObject(type)->asObject());
  Object args_obj(&scope, ApiHandle::fromPyObject(args)->asObject());
  DCHECK(runtime->isInstanceOfTuple(*args_obj),
         "Slot __new__ expected tuple args");
  Object kwargs_obj(&scope, kwargs ? ApiHandle::fromPyObject(kwargs)->asObject()
                                   : NoneType::object());
  DCHECK(kwargs == nullptr || runtime->isInstanceOfDict(*kwargs_obj),
         "Slot __new__ expected nullptr or dict kwargs");

  // Construct a new args tuple with type at the front.
  Tuple args_tuple(&scope, tupleUnderlying(*args_obj));
  MutableTuple new_args(&scope,
                        runtime->newMutableTuple(args_tuple.length() + 1));
  new_args.atPut(0, *type_obj);
  new_args.replaceFromWith(1, *args_tuple, args_tuple.length());

  // Call type.__new__(type, *args, **kwargs)
  Object dunder_new(&scope,
                    runtime->attributeAtById(thread, type_obj, ID(__new__)));
  if (dunder_new.isError()) return nullptr;
  Frame* frame = thread->currentFrame();
  frame->pushValue(*dunder_new);
  frame->pushValue(new_args.becomeImmutable());
  word flags = 0;
  if (kwargs != nullptr) {
    frame->pushValue(*kwargs_obj);
    flags = CallFunctionExFlag::VAR_KEYWORDS;
  }
  Object result(&scope, Interpreter::callEx(thread, frame, flags));
  if (result.isError()) return nullptr;
  return ApiHandle::newReference(thread, *result);
}

// Return a default slot wrapper for the given slot, or abort if it's not yet
// supported.
//
// This performs similar duties to update_one_slot() in CPython, but it's
// drastically simpler. This is intentional, and will only change if a real C
// extension exercises slots in a way that exposes the differences.
void* defaultSlot(Type::Slot slot) {
  switch (slot) {
    case Type::Slot::kNew:
      return bit_cast<void*>(&slotTpNew);
    default:
      UNIMPLEMENTED("Unsupported default slot %d", static_cast<int>(slot));
  }
}

}  // namespace

PY_EXPORT void* PyType_GetSlot(PyTypeObject* type_obj, int slot) {
  Thread* thread = Thread::current();
  if (slot < 0) {
    thread->raiseBadInternalCall();
    return nullptr;
  }

  if (!ApiHandle::isManaged(reinterpret_cast<PyObject*>(type_obj))) {
    thread->raiseBadInternalCall();
    return nullptr;
  }

  HandleScope scope(thread);
  Type type(&scope, ApiHandle::fromPyTypeObject(type_obj)->asObject());
  if (type.isBuiltin()) {
    thread->raiseBadInternalCall();
    return nullptr;
  }

  // Extension module requesting slot from a future version
  Type::Slot field_id = slotToTypeSlot(slot);
  if (field_id >= Type::Slot::kEnd) {
    return nullptr;
  }

  if (!type.hasSlots()) {
    // The Type was not created by PyType_FromSpec(), so return a default slot
    // implementation that delegates to managed methods.
    return defaultSlot(field_id);
  }

  DCHECK(type.hasSlots(), "Type is not extension type");
  Object slot_obj(&scope, type.slot(field_id));
  if (slot_obj.isNoneType()) {
    return nullptr;
  }
  if (slot_obj.isInt()) {
    Int address(&scope, *slot_obj);
    return address.asCPtr();
  }
  return ApiHandle::borrowedReference(thread, *slot_obj);
}

PY_EXPORT int PyType_Ready(PyTypeObject*) {
  UNIMPLEMENTED("This function will never be implemented");
}

PY_EXPORT PyObject* PyType_FromSpec(PyType_Spec* spec) {
  return PyType_FromSpecWithBases(spec, nullptr);
}

static RawObject memberGetter(Thread* thread, PyMemberDef& member) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object name(&scope, runtime->newStrFromCStr(member.name));
  Tuple consts(&scope, runtime->newTuple(1));
  consts.atPut(0, runtime->newInt(member.offset));
  Int offset(&scope, runtime->newInt(member.offset));
  switch (member.type) {
    case T_BOOL:
      return thread->invokeFunction1(ID(builtins), ID(_new_member_get_bool),
                                     offset);
    case T_BYTE:
      return thread->invokeFunction1(ID(builtins), ID(_new_member_get_byte),
                                     offset);
    case T_UBYTE:
      return thread->invokeFunction1(ID(builtins), ID(_new_member_get_ubyte),
                                     offset);
    case T_SHORT:
      return thread->invokeFunction1(ID(builtins), ID(_new_member_get_short),
                                     offset);
    case T_USHORT:
      return thread->invokeFunction1(ID(builtins), ID(_new_member_get_ushort),
                                     offset);
    case T_INT:
      return thread->invokeFunction1(ID(builtins), ID(_new_member_get_int),
                                     offset);
    case T_UINT:
      return thread->invokeFunction1(ID(builtins), ID(_new_member_get_uint),
                                     offset);
    case T_LONG:
      return thread->invokeFunction1(ID(builtins), ID(_new_member_get_long),
                                     offset);
    case T_ULONG:
      return thread->invokeFunction1(ID(builtins), ID(_new_member_get_ulong),
                                     offset);
    case T_PYSSIZET:
      return thread->invokeFunction1(ID(builtins), ID(_new_member_get_ulong),
                                     offset);
    case T_FLOAT:
      return thread->invokeFunction1(ID(builtins), ID(_new_member_get_float),
                                     offset);
    case T_DOUBLE:
      return thread->invokeFunction1(ID(builtins), ID(_new_member_get_double),
                                     offset);
    case T_LONGLONG:
      return thread->invokeFunction1(ID(builtins), ID(_new_member_get_long),
                                     offset);
    case T_ULONGLONG:
      return thread->invokeFunction1(ID(builtins), ID(_new_member_get_ulong),
                                     offset);
    case T_STRING:
      return thread->invokeFunction1(ID(builtins), ID(_new_member_get_string),
                                     offset);
    case T_STRING_INPLACE:
      return thread->invokeFunction1(ID(builtins), ID(_new_member_get_string),
                                     offset);
    case T_CHAR:
      return thread->invokeFunction1(ID(builtins), ID(_new_member_get_char),
                                     offset);
    case T_OBJECT:
      return thread->invokeFunction1(ID(builtins), ID(_new_member_get_pyobject),
                                     offset);
    case T_OBJECT_EX:
      return thread->invokeFunction2(ID(builtins), ID(_new_member_get_pyobject),
                                     offset, name);
    case T_NONE:
      return thread->invokeFunction1(ID(builtins), ID(_new_member_get_pyobject),
                                     offset);
    default:
      return thread->raiseWithFmt(LayoutId::kSystemError,
                                  "bad member name type");
  }
}

static RawObject memberSetter(Thread* thread, PyMemberDef& member) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  if (member.flags & READONLY) {
    Object name(&scope, runtime->newStrFromCStr(member.name));
    Function setter(
        &scope, thread->invokeFunction1(ID(builtins),
                                        ID(_new_member_set_readonly), name));
    return *setter;
  }

  Int offset(&scope, runtime->newInt(member.offset));
  switch (member.type) {
    case T_BOOL:
      return thread->invokeFunction1(ID(builtins), ID(_new_member_set_bool),
                                     offset);
    case T_BYTE: {
      Int num_bytes(&scope, runtime->newInt(1));
      Int min_value(&scope, runtime->newInt(std::numeric_limits<char>::min()));
      Int max_value(&scope, runtime->newInt(std::numeric_limits<char>::max()));
      Str primitive_type(&scope, runtime->newStrFromCStr("char"));
      Function setter(&scope,
                      thread->invokeFunction5(
                          ID(builtins), ID(_new_member_set_integral), offset,
                          num_bytes, min_value, max_value, primitive_type));
      return *setter;
    }
    case T_UBYTE: {
      Int num_bytes(&scope, runtime->newInt(1));
      Int min_value(&scope, runtime->newInt(0));
      Int max_value(&scope, runtime->newIntFromUnsigned(
                                std::numeric_limits<unsigned char>::max()));
      Str primitive_type(&scope, runtime->newStrFromCStr("unsigned char"));
      Function setter(&scope,
                      thread->invokeFunction5(
                          ID(builtins), ID(_new_member_set_integral), offset,
                          num_bytes, min_value, max_value, primitive_type));
      return *setter;
    }
    case T_SHORT: {
      Int num_bytes(&scope, runtime->newInt(2));
      Int min_value(&scope, runtime->newInt(std::numeric_limits<short>::min()));
      Int max_value(&scope, runtime->newInt(std::numeric_limits<short>::max()));
      Str primitive_type(&scope, runtime->newStrFromCStr("short"));
      Function setter(&scope,
                      thread->invokeFunction5(
                          ID(builtins), ID(_new_member_set_integral), offset,
                          num_bytes, min_value, max_value, primitive_type));
      return *setter;
    }
    case T_USHORT: {
      Int num_bytes(&scope, runtime->newInt(2));
      Int min_value(&scope, runtime->newInt(0));
      Int max_value(&scope, runtime->newIntFromUnsigned(
                                std::numeric_limits<unsigned short>::max()));
      Str primitive_type(&scope, runtime->newStrFromCStr("unsigned short"));
      Function setter(&scope,
                      thread->invokeFunction5(
                          ID(builtins), ID(_new_member_set_integral), offset,
                          num_bytes, min_value, max_value, primitive_type));
      return *setter;
    }
    case T_INT: {
      Int num_bytes(&scope, runtime->newInt(4));
      Int min_value(&scope, runtime->newInt(std::numeric_limits<int>::min()));
      Int max_value(&scope, runtime->newInt(std::numeric_limits<int>::max()));
      Str primitive_type(&scope, runtime->newStrFromCStr("int"));
      Function setter(&scope,
                      thread->invokeFunction5(
                          ID(builtins), ID(_new_member_set_integral), offset,
                          num_bytes, min_value, max_value, primitive_type));
      return *setter;
    }
    case T_UINT: {
      Int num_bytes(&scope, runtime->newInt(4));
      Int min_value(&scope, runtime->newInt(0));
      Int max_value(&scope, runtime->newIntFromUnsigned(
                                std::numeric_limits<unsigned int>::max()));
      Str primitive_type(&scope, runtime->newStrFromCStr("unsigned int"));
      Function setter(&scope,
                      thread->invokeFunction5(
                          ID(builtins), ID(_new_member_set_integral), offset,
                          num_bytes, min_value, max_value, primitive_type));
      return *setter;
    }
    case T_LONG: {
      Int num_bytes(&scope, runtime->newInt(8));
      Int min_value(&scope, runtime->newInt(std::numeric_limits<long>::min()));
      Int max_value(&scope, runtime->newInt(std::numeric_limits<long>::max()));
      Str primitive_type(&scope, runtime->newStrFromCStr("long"));
      Function setter(&scope,
                      thread->invokeFunction5(
                          ID(builtins), ID(_new_member_set_integral), offset,
                          num_bytes, min_value, max_value, primitive_type));
      return *setter;
    }
    case T_ULONG: {
      Int num_bytes(&scope, runtime->newInt(8));
      Int min_value(&scope, runtime->newInt(0));
      Int max_value(&scope, runtime->newIntFromUnsigned(
                                std::numeric_limits<unsigned long>::max()));
      Str primitive_type(&scope, runtime->newStrFromCStr("unsigned long"));
      Function setter(&scope,
                      thread->invokeFunction5(
                          ID(builtins), ID(_new_member_set_integral), offset,
                          num_bytes, min_value, max_value, primitive_type));
      return *setter;
    }
    case T_PYSSIZET: {
      Int num_bytes(&scope, runtime->newInt(8));
      Int min_value(&scope, runtime->newInt(0));
      Int max_value(&scope,
                    runtime->newInt(std::numeric_limits<Py_ssize_t>::max()));
      Str primitive_type(&scope, runtime->newStrFromCStr("Py_ssize_t"));
      Function setter(&scope,
                      thread->invokeFunction5(
                          ID(builtins), ID(_new_member_set_integral), offset,
                          num_bytes, min_value, max_value, primitive_type));
      return *setter;
    }
    case T_FLOAT: {
      return thread->invokeFunction1(ID(builtins), ID(_new_member_set_float),
                                     offset);
    }
    case T_DOUBLE: {
      return thread->invokeFunction1(ID(builtins), ID(_new_member_set_double),
                                     offset);
    }
    case T_STRING: {
      Object name(&scope, runtime->newStrFromCStr(member.name));
      Function setter(&scope, thread->invokeFunction1(
                                  ID(builtins),
                                  ID(_new_member_set_readonly_strings), name));
      return *setter;
    }
    case T_STRING_INPLACE: {
      Object name(&scope, runtime->newStrFromCStr(member.name));
      Function setter(&scope, thread->invokeFunction1(
                                  ID(builtins),
                                  ID(_new_member_set_readonly_strings), name));
      return *setter;
    }
    case T_CHAR: {
      Function setter(
          &scope, thread->invokeFunction1(ID(builtins),
                                          ID(_new_member_set_char), offset));
      return *setter;
    }
    case T_OBJECT: {
      Function setter(&scope,
                      thread->invokeFunction1(
                          ID(builtins), ID(_new_member_set_pyobject), offset));
      return *setter;
    }
    case T_OBJECT_EX: {
      Function setter(&scope,
                      thread->invokeFunction1(
                          ID(builtins), ID(_new_member_set_pyobject), offset));
      return *setter;
    }
    case T_LONGLONG: {
      Int num_bytes(&scope, runtime->newInt(8));
      Int min_value(&scope,
                    runtime->newInt(std::numeric_limits<long long>::min()));
      Int max_value(&scope,
                    runtime->newInt(std::numeric_limits<long long>::max()));
      Str primitive_type(&scope, runtime->newStrFromCStr("long long"));
      Function setter(&scope,
                      thread->invokeFunction5(
                          ID(builtins), ID(_new_member_set_integral), offset,
                          num_bytes, min_value, max_value, primitive_type));
      return *setter;
    }
    case T_ULONGLONG: {
      Int num_bytes(&scope, runtime->newInt(8));
      Int min_value(&scope, runtime->newInt(0));
      Int max_value(&scope,
                    runtime->newIntFromUnsigned(
                        std::numeric_limits<unsigned long long>::max()));
      Str primitive_type(&scope, runtime->newStrFromCStr("unsigned long long"));
      Function setter(&scope,
                      thread->invokeFunction5(
                          ID(builtins), ID(_new_member_set_integral), offset,
                          num_bytes, min_value, max_value, primitive_type));
      return *setter;
    }
    default:
      return thread->raiseWithFmt(LayoutId::kSystemError,
                                  "bad member name type");
  }
}

static RawObject getterWrapper(Thread* thread, Frame* frame, word nargs) {
  auto func = getNativeFunc<getter>(thread, frame);
  Arguments args(frame, nargs);
  PyObject* self = ApiHandle::borrowedReference(thread, args.get(0));
  PyObject* result = (*func)(self, nullptr);
  return ApiHandle::checkFunctionResult(thread, result);
}

static RawObject setterWrapper(Thread* thread, Frame* frame, word nargs) {
  auto func = getNativeFunc<setter>(thread, frame);
  Arguments args(frame, nargs);
  PyObject* self = ApiHandle::borrowedReference(thread, args.get(0));
  PyObject* value = ApiHandle::borrowedReference(thread, args.get(1));
  if (func(self, value, nullptr) < 0) return Error::exception();
  return NoneType::object();
}

static RawObject getSetGetter(Thread* thread, const Object& name,
                              PyGetSetDef& def) {
  if (def.get == nullptr) return NoneType::object();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object value(&scope, runtime->newIntFromCPtr(bit_cast<void*>(def.get)));
  Code code(&scope,
            newExtCode(thread, name, kParamsSelf, ARRAYSIZE(kParamsSelf),
                       /*flags=*/0, bit_cast<void*>(&getterWrapper), value));
  Object globals(&scope, NoneType::object());
  Function function(&scope,
                    runtime->newFunctionWithCode(thread, name, code, globals));
  if (def.doc != nullptr) {
    Object doc(&scope, runtime->newStrFromCStr(def.doc));
    function.setDoc(*doc);
  }
  return *function;
}

static RawObject getSetSetter(Thread* thread, const Object& name,
                              PyGetSetDef& def) {
  if (def.set == nullptr) return NoneType::object();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object value(&scope, runtime->newIntFromCPtr(bit_cast<void*>(def.set)));
  Code code(&scope, newExtCode(thread, name, kParamsSelfValue,
                               ARRAYSIZE(kParamsSelfValue), /*flags=*/0,
                               bit_cast<void*>(&setterWrapper), value));
  Object globals(&scope, NoneType::object());
  Function function(&scope,
                    runtime->newFunctionWithCode(thread, name, code, globals));
  if (def.doc != nullptr) {
    Object doc(&scope, runtime->newStrFromCStr(def.doc));
    function.setDoc(*doc);
  }
  return *function;
}

RawObject addMethods(Thread* thread, const Type& type) {
  HandleScope scope(thread);
  Object slot_value(&scope, type.slot(Type::Slot::kMethods));
  if (slot_value.isNoneType()) return NoneType::object();
  DCHECK(slot_value.isInt(), "unexpected slot type");
  auto methods = bit_cast<PyMethodDef*>(Int::cast(*slot_value).asCPtr());
  Object none(&scope, NoneType::object());
  Object unbound(&scope, Unbound::object());
  Object name(&scope, NoneType::object());
  Object member(&scope, NoneType::object());
  for (PyMethodDef* method = methods; method->ml_name != nullptr; method++) {
    name = Runtime::internStrFromCStr(thread, method->ml_name);
    int flags = method->ml_flags;
    if (!(flags & METH_COEXIST) && !typeAt(type, name).isErrorNotFound()) {
      continue;
    }
    if (flags & METH_CLASS) {
      if (flags & METH_STATIC) {
        return thread->raiseWithFmt(LayoutId::kValueError,
                                    "method cannot be both class and static");
      }
      member = newClassMethod(thread, method, name, type);
    } else if (flags & METH_STATIC) {
      member = newCFunction(thread, method, name, unbound, none);
    } else {
      member = newMethod(thread, method, name, type);
    }
    typeAtPut(thread, type, name, member);
  }
  return NoneType::object();
}

RawObject addMembers(Thread* thread, const Type& type) {
  HandleScope scope(thread);
  Object slot_value(&scope, type.slot(Type::Slot::kMembers));
  if (slot_value.isNoneType()) return NoneType::object();
  DCHECK(slot_value.isInt(), "unexpected slot type");
  auto members = bit_cast<PyMemberDef*>(Int::cast(*slot_value).asCPtr());
  Object none(&scope, NoneType::object());
  Runtime* runtime = thread->runtime();
  for (word i = 0; members[i].name != nullptr; i++) {
    Object name(&scope, Runtime::internStrFromCStr(thread, members[i].name));
    Object getter(&scope, memberGetter(thread, members[i]));
    if (getter.isError()) return *getter;
    Object setter(&scope, memberSetter(thread, members[i]));
    if (setter.isError()) return *setter;
    Object property(&scope, runtime->newProperty(getter, setter, none));
    typeAtPut(thread, type, name, property);
  }
  return NoneType::object();
}

RawObject addGetSet(Thread* thread, const Type& type) {
  HandleScope scope(thread);
  Object slot_value(&scope, type.slot(Type::Slot::kGetset));
  if (slot_value.isNoneType()) return NoneType::object();
  DCHECK(slot_value.isInt(), "unexpected slot type");
  auto getsets = bit_cast<PyGetSetDef*>(Int::cast(*slot_value).asCPtr());
  Object none(&scope, NoneType::object());
  Runtime* runtime = thread->runtime();
  for (word i = 0; getsets[i].name != nullptr; i++) {
    Object name(&scope, Runtime::internStrFromCStr(thread, getsets[i].name));
    Object getter(&scope, getSetGetter(thread, name, getsets[i]));
    if (getter.isError()) return *getter;
    Object setter(&scope, getSetSetter(thread, name, getsets[i]));
    if (setter.isError()) return *setter;
    Object property(&scope, runtime->newProperty(getter, setter, none));
    typeAtPut(thread, type, name, property);
  }
  return NoneType::object();
}

// The default tp_dealloc slot for all C Types. This loosely follows CPython's
// subtype_dealloc.
static void subtypeDealloc(PyObject* self) {
  PyTypeObject* type = reinterpret_cast<PyTypeObject*>(PyObject_Type(self));
  Py_DECREF(type);  // Borrow the reference

  DCHECK(PyType_GetFlags(type) & Py_TPFLAGS_HEAPTYPE,
         "subtypeDealloc requires an instance from a heap allocated C "
         "extension type");

  auto finalize_func =
      reinterpret_cast<destructor>(PyType_GetSlot(type, Py_tp_finalize));
  if (finalize_func != nullptr) {
    if (PyObject_CallFinalizerFromDealloc(self) < 0) {
      return;  // Resurrected
    }
  }

  auto del_func = reinterpret_cast<destructor>(PyType_GetSlot(type, Py_tp_del));
  if (del_func != nullptr) {
    (*del_func)(self);
    if (Py_REFCNT(self) > 1) {
      return;  // Resurrected
    }
  }

  // Find the nearest base with a different tp_dealloc
  PyTypeObject* base_type = type;
  destructor base_dealloc = subtypeDealloc;
  while (base_dealloc == subtypeDealloc) {
    base_type =
        reinterpret_cast<PyTypeObject*>(PyType_GetSlot(base_type, Py_tp_base));
    if (Type::cast(ApiHandle::fromPyTypeObject(base_type)->asObject())
            .isExtensionType()) {
      base_dealloc = reinterpret_cast<destructor>(
          PyType_GetSlot(base_type, Py_tp_dealloc));
    } else {
      base_dealloc = reinterpret_cast<destructor>(PyObject_Del);
    }
  }

  // Extract the type; tp_del may have changed it
  type = reinterpret_cast<PyTypeObject*>(PyObject_Type(self));
  Py_DECREF(type);  // Borrow the reference
  (*base_dealloc)(self);
  if (PyType_GetFlags(type) & Py_TPFLAGS_HEAPTYPE &&
      !(PyType_GetFlags(base_type) & Py_TPFLAGS_HEAPTYPE)) {
    Py_DECREF(type);
  }
}

// Copy the slot from the base type if it defined.
static void copySlot(const Type& type, const Type& base, Type::Slot slot_id) {
  if (!type.hasSlot(slot_id) && base.hasSlot(slot_id)) {
    type.setSlot(slot_id, base.slot(slot_id));
  }
}

static void* baseBaseSlot(const Type& base, Type::Slot slot) {
  if (!base.hasSlot(Type::Slot::kBase)) return nullptr;
  HandleScope scope(Thread::current());
  Type basebase(&scope, base.slot(Type::Slot::kBase));
  if (!basebase.hasSlots() || !basebase.hasSlot(slot)) {
    return nullptr;
  }
  Int basebase_slot(&scope, basebase.slot(slot));
  return basebase_slot.asCPtr();
}

// Copy the slot from the base type if defined and it is the first type that
// defines it. If base's base type defines the same slot, then base inherited
// it. Thus, it is not the first type to define it.
static void copySlotIfImplementedInBase(const Type& type, const Type& base,
                                        Type::Slot slot_id) {
  if (!type.hasSlot(slot_id) && base.hasSlot(slot_id)) {
    RawObject base_slot = base.slot(slot_id);
    void* basebase_slot = baseBaseSlot(base, slot_id);
    if (basebase_slot == nullptr ||
        Int::cast(base_slot).asCPtr() != basebase_slot) {
      type.setSlot(slot_id, base_slot);
    }
  }
}

static void inheritFinalize(const Type& type, unsigned long type_flags,
                            const Type& base, unsigned long base_flags) {
  if ((type_flags & Py_TPFLAGS_HAVE_FINALIZE) &&
      (base_flags & Py_TPFLAGS_HAVE_FINALIZE)) {
    copySlotIfImplementedInBase(type, base, Type::Slot::kFinalize);
  }
  if ((type_flags & Py_TPFLAGS_HAVE_FINALIZE) &&
      (base_flags & Py_TPFLAGS_HAVE_FINALIZE)) {
    copySlotIfImplementedInBase(type, base, Type::Slot::kFinalize);
  }
}

static void inheritFree(const Type& type, unsigned long type_flags,
                        const Type& base, unsigned long base_flags) {
  // Both child and base are GC or non GC
  if ((type_flags & Py_TPFLAGS_HAVE_GC) == (base_flags & Py_TPFLAGS_HAVE_GC)) {
    copySlotIfImplementedInBase(type, base, Type::Slot::kFree);
    return;
  }

  DCHECK(!(base_flags & Py_TPFLAGS_HAVE_GC), "The child should not remove GC");

  // Only the child is GC
  // Set the free function if the base has a default free
  if ((type_flags & Py_TPFLAGS_HAVE_GC) && !type.hasSlot(Type::Slot::kFree) &&
      base.hasSlot(Type::Slot::kFree)) {
    void* free_slot = reinterpret_cast<void*>(
        Int::cast(base.slot(Type::Slot::kFree)).asWord());
    if (free_slot == reinterpret_cast<void*>(PyObject_Free)) {
      type.setSlot(Type::Slot::kFree,
                   Thread::current()->runtime()->newIntFromCPtr(
                       reinterpret_cast<void*>(PyObject_GC_Del)));
    }
  }
}

static void inheritGCFlagsAndSlots(Thread* thread, const Type& type,
                                   const Type& base) {
  unsigned long type_flags = Int::cast(type.slot(Type::Slot::kFlags)).asWord();
  unsigned long base_flags = Int::cast(base.slot(Type::Slot::kFlags)).asWord();
  if (!(type_flags & Py_TPFLAGS_HAVE_GC) && (base_flags & Py_TPFLAGS_HAVE_GC) &&
      !type.hasSlot(Type::Slot::kTraverse) &&
      !type.hasSlot(Type::Slot::kClear)) {
    type.setSlot(Type::Slot::kFlags,
                 thread->runtime()->newInt(type_flags | Py_TPFLAGS_HAVE_GC));
    if (!type.hasSlot(Type::Slot::kTraverse)) {
      copySlot(type, base, Type::Slot::kTraverse);
    }
    if (!type.hasSlot(Type::Slot::kClear)) {
      copySlot(type, base, Type::Slot::kClear);
    }
  }
}

static void inheritNonFunctionSlots(const Type& type, const Type& base) {
  if (Int::cast(type.slot(Type::Slot::kBasicSize)).asWord() == 0) {
    type.setSlot(Type::Slot::kBasicSize, base.slot(Type::Slot::kBasicSize));
  }
  type.setSlot(Type::Slot::kItemSize, base.slot(Type::Slot::kItemSize));
}

// clang-format off
static const Type::Slot kInheritableSlots[] = {
  // Number slots
  Type::Slot::kNumberAdd,
  Type::Slot::kNumberSubtract,
  Type::Slot::kNumberMultiply,
  Type::Slot::kNumberRemainder,
  Type::Slot::kNumberDivmod,
  Type::Slot::kNumberPower,
  Type::Slot::kNumberNegative,
  Type::Slot::kNumberPositive,
  Type::Slot::kNumberAbsolute,
  Type::Slot::kNumberBool,
  Type::Slot::kNumberInvert,
  Type::Slot::kNumberLshift,
  Type::Slot::kNumberRshift,
  Type::Slot::kNumberAnd,
  Type::Slot::kNumberXor,
  Type::Slot::kNumberOr,
  Type::Slot::kNumberInt,
  Type::Slot::kNumberFloat,
  Type::Slot::kNumberInplaceAdd,
  Type::Slot::kNumberInplaceSubtract,
  Type::Slot::kNumberInplaceMultiply,
  Type::Slot::kNumberInplaceRemainder,
  Type::Slot::kNumberInplacePower,
  Type::Slot::kNumberInplaceLshift,
  Type::Slot::kNumberInplaceRshift,
  Type::Slot::kNumberInplaceAnd,
  Type::Slot::kNumberInplaceXor,
  Type::Slot::kNumberInplaceOr,
  Type::Slot::kNumberTrueDivide,
  Type::Slot::kNumberFloorDivide,
  Type::Slot::kNumberInplaceTrueDivide,
  Type::Slot::kNumberInplaceFloorDivide,
  Type::Slot::kNumberIndex,
  Type::Slot::kNumberMatrixMultiply,
  Type::Slot::kNumberInplaceMatrixMultiply,

  // Await slots
  Type::Slot::kAsyncAwait,
  Type::Slot::kAsyncAiter,
  Type::Slot::kAsyncAnext,

  // Sequence slots
  Type::Slot::kSequenceLength,
  Type::Slot::kSequenceConcat,
  Type::Slot::kSequenceRepeat,
  Type::Slot::kSequenceItem,
  Type::Slot::kSequenceAssItem,
  Type::Slot::kSequenceContains,
  Type::Slot::kSequenceInplaceConcat,
  Type::Slot::kSequenceInplaceRepeat,

  // Mapping slots
  Type::Slot::kMapLength,
  Type::Slot::kMapSubscript,
  Type::Slot::kMapAssSubscript,

  // Buffer protocol is not part of PEP-384

  // Type slots
  Type::Slot::kDealloc,
  Type::Slot::kRepr,
  Type::Slot::kCall,
  Type::Slot::kStr,
  Type::Slot::kIter,
  Type::Slot::kIternext,
  Type::Slot::kDescrGet,
  Type::Slot::kDescrSet,
  Type::Slot::kInit,
  Type::Slot::kAlloc,
  Type::Slot::kIsGc,

  // Instance dictionary is not part of PEP-384

  // Weak reference support is not part of PEP-384
};
// clang-format on

static void inheritSlots(const Type& type, const Type& base) {
  // Heap allocated types are guaranteed to have slot space, no check is needed
  // i.e. CPython does: `if (type->tp_as_number != NULL)`
  // Only static types need to do this type of check.
  for (const Type::Slot slot : kInheritableSlots) {
    copySlotIfImplementedInBase(type, base, slot);
  }

  // Inherit conditional type slots
  if (!type.hasSlot(Type::Slot::kGetattr) &&
      !type.hasSlot(Type::Slot::kGetattro)) {
    copySlot(type, base, Type::Slot::kGetattr);
    copySlot(type, base, Type::Slot::kGetattro);
  }
  if (!type.hasSlot(Type::Slot::kSetattr) &&
      !type.hasSlot(Type::Slot::kSetattro)) {
    copySlot(type, base, Type::Slot::kSetattr);
    copySlot(type, base, Type::Slot::kSetattro);
  }
  if (!type.hasSlot(Type::Slot::kRichcompare) &&
      !type.hasSlot(Type::Slot::kHash)) {
    copySlot(type, base, Type::Slot::kRichcompare);
    copySlot(type, base, Type::Slot::kHash);
  }

  unsigned long type_flags = Int::cast(type.slot(Type::Slot::kFlags)).asWord();
  unsigned long base_flags = Int::cast(base.slot(Type::Slot::kFlags)).asWord();
  inheritFinalize(type, type_flags, base, base_flags);
  inheritFree(type, type_flags, base, base_flags);
}

static int objectInit(PyObject*, PyObject*, PyObject*) {
  UNIMPLEMENTED("should not directly call tp_init");
}

static RawObject addDefaultsForRequiredSlots(Thread* thread, const Type& type) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Str type_name(&scope, type.name());

  // tp_basicsize -> sizeof(PyObject)
  DCHECK(type.hasSlot(Type::Slot::kBasicSize),
         "Basic size must always be present");
  Int basic_size(&scope, type.slot(Type::Slot::kBasicSize));
  if (basic_size.asWord() == 0) {
    basic_size = runtime->newInt(sizeof(PyObject));
    type.setSlot(Type::Slot::kBasicSize, *basic_size);
  }
  DCHECK(basic_size.asWord() >= static_cast<word>(sizeof(PyObject)),
         "sizeof(PyObject) is the minimum size required for an extension "
         "instance");

  // tp_dealloc -> subtypeDealloc
  if (!type.hasSlot(Type::Slot::kDealloc)) {
    Object default_dealloc(
        &scope, runtime->newIntFromCPtr(bit_cast<void*>(&subtypeDealloc)));
    type.setSlot(Type::Slot::kDealloc, *default_dealloc);
  }

  // tp_repr -> PyObject_Repr
  if (!type.hasSlot(Type::Slot::kRepr)) {
    Object default_repr(
        &scope, runtime->newIntFromCPtr(bit_cast<void*>(&PyObject_Repr)));
    type.setSlot(Type::Slot::kRepr, *default_repr);
    // PyObject_Repr delegates its job to type.__repr__().
    DCHECK(!typeLookupInMroById(thread, type, ID(__repr__)).isErrorNotFound(),
           "__repr__ is expected");
  }

  // tp_str -> object_str
  if (!type.hasSlot(Type::Slot::kStr)) {
    Object default_str(&scope,
                       runtime->newIntFromCPtr(bit_cast<void*>(&PyObject_Str)));
    type.setSlot(Type::Slot::kStr, *default_str);
    // PyObject_Str delegates its job to type.__str__().
    DCHECK(!typeLookupInMroById(thread, type, ID(__str__)).isErrorNotFound(),
           "__str__ is expected");
  }

  // tp_init -> object_init
  if (!type.hasSlot(Type::Slot::kInit)) {
    Object default_init(&scope,
                        runtime->newIntFromCPtr(bit_cast<void*>(&objectInit)));
    type.setSlot(Type::Slot::kInit, *default_init);
  }

  // tp_alloc -> PyType_GenericAlloc
  if (!type.hasSlot(Type::Slot::kAlloc)) {
    Object default_alloc(
        &scope, runtime->newIntFromCPtr(bit_cast<void*>(&PyType_GenericAlloc)));
    type.setSlot(Type::Slot::kAlloc, *default_alloc);
  }

  // tp_new -> PyType_GenericNew
  if (!type.hasSlot(Type::Slot::kNew)) {
    Object default_new(
        &scope, runtime->newIntFromCPtr(bit_cast<void*>(&PyType_GenericNew)));
    type.setSlot(Type::Slot::kNew, *default_new);
    Str dunder_new_name(&scope, runtime->symbols()->at(ID(__new__)));
    Str qualname(&scope,
                 runtime->newStrFromFmt("%S.%S", &type_name, &dunder_new_name));
    Code code(&scope,
              newExtCode(thread, dunder_new_name, kParamsTypeArgsKwargs,
                         ARRAYSIZE(kParamsTypeArgsKwargs),
                         Code::Flags::kVarargs | Code::Flags::kVarkeyargs,
                         bit_cast<void*>(&wrapVarkwTernaryfunc), default_new));
    Object globals(&scope, NoneType::object());
    Function func(
        &scope, runtime->newFunctionWithCode(thread, qualname, code, globals));
    Object func_obj(
        &scope, thread->invokeFunction1(ID(builtins), ID(staticmethod), func));
    if (func_obj.isError()) return *func;
    typeAtPutById(thread, type, ID(__new__), func_obj);
  }

  // tp_free.
  if (!type.hasSlot(Type::Slot::kFree)) {
    unsigned long type_flags =
        Int::cast(type.slot(Type::Slot::kFlags)).asWord();
    Object default_free(
        &scope, runtime->newIntFromCPtr(bit_cast<void*>(
                    (type_flags & Py_TPFLAGS_HAVE_GC) ? &PyObject_GC_Del
                                                      : &PyObject_Del)));
    type.setSlot(Type::Slot::kFree, *default_free);
  }

  return NoneType::object();
}

RawObject addInheritedSlots(const Type& type) {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Type base_type(&scope, Tuple::cast(type.mro()).at(1));

  if (!type.hasSlots()) {
    type.setSlots(runtime->newTuple(static_cast<int>(Type::Slot::kEnd)));
    if (base_type.hasSlots()) {
      type.setSlot(Type::Slot::kFlags, base_type.slot(Type::Slot::kFlags));
    } else {
      type.setSlot(Type::Slot::kFlags, SmallInt::fromWord(0));
    }
    type.setSlot(Type::Slot::kBasicSize, SmallInt::fromWord(0));
    type.setSlot(Type::Slot::kItemSize, SmallInt::fromWord(0));
    type.setSlot(Type::Slot::kBase, *base_type);
  }

  // Inherit special slots from dominant base
  if (base_type.hasSlots()) {
    inheritGCFlagsAndSlots(thread, type, base_type);
    if (!type.hasSlot(Type::Slot::kNew)) {
      copySlot(type, base_type, Type::Slot::kNew);
    }
    inheritNonFunctionSlots(type, base_type);
  }

  // Inherit slots from the MRO
  Tuple mro(&scope, type.mro());
  for (word i = 1; i < mro.length(); i++) {
    Type base(&scope, mro.at(i));
    // Skip inheritance if base does not define slots
    if (!base.hasSlots()) continue;
    // Bases must define Py_TPFLAGS_BASETYPE
    word base_flags = Int::cast(base.slot(Type::Slot::kFlags)).asWord();
    if ((base_flags & Py_TPFLAGS_BASETYPE) == 0) {
      thread->raiseWithFmt(LayoutId::kTypeError,
                           "type is not an acceptable base type");
      return Error::exception();
    }
    inheritSlots(type, base);
  }

  // Inherit all the default slots that would have been inherited
  // through the base object type in CPython
  Object result(&scope, addDefaultsForRequiredSlots(thread, type));
  if (result.isError()) return *result;

  return NoneType::object();
}

PY_EXPORT PyObject* PyType_FromSpecWithBases(PyType_Spec* spec,
                                             PyObject* bases) {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  // Define the type name
  Object module_name(&scope, NoneType::object());
  const char* class_name = strrchr(spec->name, '.');
  if (class_name == nullptr) {
    class_name = spec->name;
  } else {
    module_name = Runtime::internStrFromAll(
        thread, View<byte>(reinterpret_cast<const byte*>(spec->name),
                           class_name - spec->name));
    class_name++;
  }
  Str type_name(&scope, Runtime::internStrFromCStr(thread, class_name));

  // Create a new type for the PyTypeObject with an instance layout
  // matching the layout of RawNativeProxy
  // TODO(T53922464): Set the layout to the dict overflow state
  // TODO(T54277314): Fill the dictionary before creating the type
  Tuple bases_obj(&scope, runtime->implicitBases());
  if (bases != nullptr) bases_obj = ApiHandle::fromPyObject(bases)->asObject();
  Dict dict(&scope, runtime->newDict());
  if (!module_name.isNoneType()) {
    dictAtPutById(thread, dict, ID(__module__), module_name);
  }
  dictAtPutById(thread, dict, ID(__qualname__), type_name);
  word flags = Type::Flag::kIsNativeProxy;
  if (spec->flags & Py_TPFLAGS_HAVE_GC) {
    flags |= Type::Flag::kHasCycleGC;
  }
  Object type_obj(&scope, typeNew(thread, LayoutId::kType, type_name, bases_obj,
                                  dict, static_cast<Type::Flag>(flags)));
  if (type_obj.isError()) return nullptr;
  Type type(&scope, *type_obj);

  // Initialize the extension slots tuple
  Object extension_slots(&scope,
                         runtime->newTuple(static_cast<int>(Type::Slot::kEnd)));
  type.setSlots(*extension_slots);

  // Set Py_tp_bases and Py_tp_base
  type.setSlot(Type::Slot::kBases, *bases_obj);
  type.setSlot(Type::Slot::kBase, Tuple::cast(type.mro()).at(1));

  // Set tp_flags
  Object tp_flags(&scope, runtime->newInt(spec->flags | Py_TPFLAGS_READY |
                                          Py_TPFLAGS_HEAPTYPE));
  type.setSlot(Type::Slot::kFlags, *tp_flags);

  // Set the native instance size
  Object basic_size(&scope, runtime->newInt(spec->basicsize));
  Object item_size(&scope, runtime->newInt(spec->itemsize));
  type.setSlot(Type::Slot::kBasicSize, *basic_size);
  type.setSlot(Type::Slot::kItemSize, *item_size);

  // Set the type slots
  bool has_default_dealloc = true;
  for (PyType_Slot* slot = spec->slots; slot->slot; slot++) {
    void* slot_ptr = slot->pfunc;
    Object field(&scope, runtime->newIntFromCPtr(slot_ptr));
    Type::Slot field_id = slotToTypeSlot(slot->slot);
    if (field_id >= Type::Slot::kEnd) {
      thread->raiseWithFmt(LayoutId::kRuntimeError, "invalid slot offset");
      return nullptr;
    }
    if (field_id == Type::Slot::kDealloc || field_id == Type::Slot::kDel ||
        field_id == Type::Slot::kFinalize) {
      has_default_dealloc = false;
    }
    type.setSlot(field_id, *field);
  }
  if (has_default_dealloc) {
    type.setFlags(
        static_cast<Type::Flag>(type.flags() | Type::Flag::kHasDefaultDealloc));
  }

  if (addOperators(thread, type).isError()) return nullptr;

  if (addMethods(thread, type).isError()) return nullptr;

  if (addMembers(thread, type).isError()) return nullptr;

  if (addGetSet(thread, type).isError()) return nullptr;

  if (addInheritedSlots(type).isError()) return nullptr;

  return ApiHandle::newReference(thread, *type);
}

PY_EXPORT PyObject* PyType_GenericAlloc(PyTypeObject* type_obj,
                                        Py_ssize_t nitems) {
  DCHECK(ApiHandle::isManaged(reinterpret_cast<PyObject*>(type_obj)),
         "Type is unmanaged. Please initialize using PyType_FromSpec");

  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Type type(&scope, ApiHandle::fromPyTypeObject(type_obj)->asObject());
  DCHECK(!type.isBuiltin(),
         "Type is unmanaged. Please initialize using PyType_FromSpec");
  DCHECK(type.hasSlots(),
         "GenericAlloc from types initialized through Python code");

  // Allocate a new instance
  Int basic_size(&scope, type.slot(Type::Slot::kBasicSize));
  Int item_size(&scope, type.slot(Type::Slot::kItemSize));
  Py_ssize_t size = Utils::roundUp(
      nitems * item_size.asWord() + basic_size.asWord(), kWordSize);

  PyObject* result = nullptr;
  if (type.hasFlag(Type::Flag::kHasCycleGC)) {
    result = static_cast<PyObject*>(_PyObject_GC_Calloc(size));
  } else {
    result = static_cast<PyObject*>(PyObject_Calloc(1, size));
  }
  if (result == nullptr) {
    thread->raiseMemoryError();
    return nullptr;
  }

  // Initialize the newly-allocated instance
  if (item_size.asWord() == 0) {
    PyObject_Init(result, type_obj);
  } else {
    PyObject_InitVar(reinterpret_cast<PyVarObject*>(result), type_obj, nitems);
  }

  // Track object in native GC queue
  if (type.hasFlag(Type::Flag::kHasCycleGC)) {
    PyObject_GC_Track(result);
  }

  return result;
}

PY_EXPORT Py_ssize_t _PyObject_SIZE_Func(PyObject* obj) {
  HandleScope scope;
  Type type(&scope, ApiHandle::fromPyObject(obj)->asObject());
  Int basic_size(&scope, type.slot(Type::Slot::kBasicSize));
  return basic_size.asWord();
}

PY_EXPORT Py_ssize_t _PyObject_VAR_SIZE_Func(PyObject* obj, Py_ssize_t nitems) {
  HandleScope scope;
  Type type(&scope, ApiHandle::fromPyObject(obj)->asObject());
  Int basic_size(&scope, type.slot(Type::Slot::kBasicSize));
  Int item_size(&scope, type.slot(Type::Slot::kItemSize));
  return Utils::roundUp(nitems * item_size.asWord() + basic_size.asWord(),
                        kWordSize);
}

PY_EXPORT unsigned int PyType_ClearCache() {
  UNIMPLEMENTED("PyType_ClearCache");
}

PY_EXPORT PyObject* PyType_GenericNew(PyTypeObject* type, PyObject*,
                                      PyObject*) {
  auto alloc_func =
      reinterpret_cast<allocfunc>(PyType_GetSlot(type, Py_tp_alloc));
  return alloc_func(type, 0);
}

PY_EXPORT int PyObject_TypeCheck_Func(PyObject* obj, PyTypeObject* type) {
  PyTypeObject* obj_type = reinterpret_cast<PyTypeObject*>(PyObject_Type(obj));
  int res = PyType_IsSubtype(obj_type, type);
  Py_DECREF(obj_type);
  return res;
}

PY_EXPORT int PyType_IsSubtype(PyTypeObject* a, PyTypeObject* b) {
  if (a == b) return 1;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Type a_obj(&scope, ApiHandle::fromPyTypeObject(a)->asObject());
  Type b_obj(&scope, ApiHandle::fromPyTypeObject(b)->asObject());
  return typeIsSubclass(a_obj, b_obj) ? 1 : 0;
}

PY_EXPORT void PyType_Modified(PyTypeObject* /* e */) {
  UNIMPLEMENTED("PyType_Modified");
}

PY_EXPORT PyTypeObject* PyType_Type_Ptr() {
  Thread* thread = Thread::current();
  return reinterpret_cast<PyTypeObject*>(ApiHandle::borrowedReference(
      thread, thread->runtime()->typeAt(LayoutId::kType)));
}

PY_EXPORT PyObject* _PyObject_LookupSpecial(PyObject* /* f */,
                                            _Py_Identifier* /* d */) {
  UNIMPLEMENTED("_Py_Identifiers are not supported");
}

PY_EXPORT const char* _PyType_Name(PyTypeObject* type) {
  Thread* thread = Thread::current();
  if (type == nullptr) {
    thread->raiseBadInternalCall();
    return nullptr;
  }
  HandleScope scope(thread);
  Object obj(&scope, ApiHandle::fromPyTypeObject(type)->asObject());
  if (!thread->runtime()->isInstanceOfType(*obj)) {
    thread->raiseBadInternalCall();
    return nullptr;
  }
  Type type_obj(&scope, *obj);
  return PyUnicode_AsUTF8(
      ApiHandle::borrowedReference(thread, type_obj.name()));
}

PY_EXPORT PyObject* _PyType_Lookup(PyTypeObject* type, PyObject* name) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Type type_obj(
      &scope,
      ApiHandle::fromPyObject(reinterpret_cast<PyObject*>(type))->asObject());
  Object name_obj(&scope, ApiHandle::fromPyObject(name)->asObject());
  name_obj = attributeNameNoException(thread, name_obj);
  if (name_obj.isErrorError()) return nullptr;
  Object res(&scope, typeLookupInMro(thread, type_obj, name_obj));
  if (res.isErrorNotFound()) {
    return nullptr;
  }
  return ApiHandle::borrowedReference(thread, *res);
}

}  // namespace py
