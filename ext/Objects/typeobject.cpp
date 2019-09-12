// typeobject.c implementation

#include "cpython-data.h"
#include "cpython-func.h"
#include "cpython-types.h"

#include <cinttypes>

#include "builtins-module.h"
#include "function-builtins.h"
#include "function-utils.h"
#include "handles.h"
#include "int-builtins.h"
#include "mro.h"
#include "objects.h"
#include "runtime.h"
#include "trampolines.h"
#include "tuple-builtins.h"
#include "utils.h"

namespace python {

enum class ExtensionSlot {
  kFlags,
  kBasicSize,
  kItemSize,
  kMapAssSubscript,
  kMapLength,
  kMapSubscript,
  kNumberAbsolute,
  kNumberAdd,
  kNumberAnd,
  kNumberBool,
  kNumberDivmod,
  kNumberFloat,
  kNumberFloorDivide,
  kNumberIndex,
  kNumberInplaceAdd,
  kNumberInplaceAnd,
  kNumberInplaceFloorDivide,
  kNumberInplaceLshift,
  kNumberInplaceMultiply,
  kNumberInplaceOr,
  kNumberInplacePower,
  kNumberInplaceRemainder,
  kNumberInplaceRshift,
  kNumberInplaceSubtract,
  kNumberInplaceTrueDivide,
  kNumberInplaceXor,
  kNumberInt,
  kNumberInvert,
  kNumberLshift,
  kNumberMultiply,
  kNumberNegative,
  kNumberOr,
  kNumberPositive,
  kNumberPower,
  kNumberRemainder,
  kNumberRshift,
  kNumberSubtract,
  kNumberTrueDivide,
  kNumberXor,
  kSequenceAssItem,
  kSequenceConcat,
  kSequenceContains,
  kSequenceInplaceConcat,
  kSequenceInplaceRepeat,
  kSequenceItem,
  kSequenceLength,
  kSequenceRepeat,
  kAlloc,
  kBase,
  kBases,
  kCall,
  kClear,
  kDealloc,
  kDel,
  kDescrGet,
  kDescrSet,
  kDoc,
  kGetattr,
  kGetattro,
  kHash,
  kInit,
  kIsGc,
  kIter,
  kIternext,
  kMethods,
  kNew,
  kRepr,
  kRichcompare,
  kSetattr,
  kSetattro,
  kStr,
  kTraverse,
  kMembers,
  kGetset,
  kFree,
  kNumberMatrixMultiply,
  kNumberInplaceMatrixMultiply,
  kAsyncAwait,
  kAsyncAiter,
  kAsyncAnext,
  kFinalize,
  kEnd,
};

PY_EXPORT int PyType_CheckExact_Func(PyObject* obj) {
  return ApiHandle::fromPyObject(obj)->asObject().isType();
}

PY_EXPORT int PyType_Check_Func(PyObject* obj) {
  return Thread::current()->runtime()->isInstanceOfType(
      ApiHandle::fromPyObject(obj)->asObject());
}

static RawObject extensionSlot(const Type& type, ExtensionSlot slot_id) {
  DCHECK(!type.extensionSlots().isNoneType(), "Type is not an extension");
  return Tuple::cast(type.extensionSlots()).at(static_cast<word>(slot_id));
}

static void setExtensionSlot(const Type& type, ExtensionSlot slot_id,
                             RawObject slot) {
  DCHECK(!type.extensionSlots().isNoneType(), "Type is not an extension");
  return Tuple::cast(type.extensionSlots())
      .atPut(static_cast<word>(slot_id), slot);
}

PY_EXPORT unsigned long PyType_GetFlags(PyTypeObject* type_obj) {
  CHECK(ApiHandle::isManaged(reinterpret_cast<PyObject*>(type_obj)),
        "Type is unmanaged. Please initialize using PyType_FromSpec");

  HandleScope scope;
  Type type(&scope,
            ApiHandle::fromPyObject(reinterpret_cast<PyObject*>(type_obj))
                ->asObject());
  if (type.isBuiltin()) return Py_TPFLAGS_DEFAULT;

  if (type.extensionSlots().isNoneType()) {
    UNIMPLEMENTED("GetFlags from types initialized through Python code");
  }

  Int flags(&scope, extensionSlot(type, ExtensionSlot::kFlags));
  return flags.asWord();
}

static ExtensionSlot slotToTypeSlot(int slot) {
  switch (slot) {
    case Py_mp_ass_subscript:
      return ExtensionSlot::kMapAssSubscript;
    case Py_mp_length:
      return ExtensionSlot::kMapLength;
    case Py_mp_subscript:
      return ExtensionSlot::kMapSubscript;
    case Py_nb_absolute:
      return ExtensionSlot::kNumberAbsolute;
    case Py_nb_add:
      return ExtensionSlot::kNumberAdd;
    case Py_nb_and:
      return ExtensionSlot::kNumberAnd;
    case Py_nb_bool:
      return ExtensionSlot::kNumberBool;
    case Py_nb_divmod:
      return ExtensionSlot::kNumberDivmod;
    case Py_nb_float:
      return ExtensionSlot::kNumberFloat;
    case Py_nb_floor_divide:
      return ExtensionSlot::kNumberFloorDivide;
    case Py_nb_index:
      return ExtensionSlot::kNumberIndex;
    case Py_nb_inplace_add:
      return ExtensionSlot::kNumberInplaceAdd;
    case Py_nb_inplace_and:
      return ExtensionSlot::kNumberInplaceAnd;
    case Py_nb_inplace_floor_divide:
      return ExtensionSlot::kNumberInplaceFloorDivide;
    case Py_nb_inplace_lshift:
      return ExtensionSlot::kNumberInplaceLshift;
    case Py_nb_inplace_multiply:
      return ExtensionSlot::kNumberInplaceMultiply;
    case Py_nb_inplace_or:
      return ExtensionSlot::kNumberInplaceOr;
    case Py_nb_inplace_power:
      return ExtensionSlot::kNumberInplacePower;
    case Py_nb_inplace_remainder:
      return ExtensionSlot::kNumberInplaceRemainder;
    case Py_nb_inplace_rshift:
      return ExtensionSlot::kNumberInplaceRshift;
    case Py_nb_inplace_subtract:
      return ExtensionSlot::kNumberInplaceSubtract;
    case Py_nb_inplace_true_divide:
      return ExtensionSlot::kNumberInplaceTrueDivide;
    case Py_nb_inplace_xor:
      return ExtensionSlot::kNumberInplaceXor;
    case Py_nb_int:
      return ExtensionSlot::kNumberInt;
    case Py_nb_invert:
      return ExtensionSlot::kNumberInvert;
    case Py_nb_lshift:
      return ExtensionSlot::kNumberLshift;
    case Py_nb_multiply:
      return ExtensionSlot::kNumberMultiply;
    case Py_nb_negative:
      return ExtensionSlot::kNumberNegative;
    case Py_nb_or:
      return ExtensionSlot::kNumberOr;
    case Py_nb_positive:
      return ExtensionSlot::kNumberPositive;
    case Py_nb_power:
      return ExtensionSlot::kNumberPower;
    case Py_nb_remainder:
      return ExtensionSlot::kNumberRemainder;
    case Py_nb_rshift:
      return ExtensionSlot::kNumberRshift;
    case Py_nb_subtract:
      return ExtensionSlot::kNumberSubtract;
    case Py_nb_true_divide:
      return ExtensionSlot::kNumberTrueDivide;
    case Py_nb_xor:
      return ExtensionSlot::kNumberXor;
    case Py_sq_ass_item:
      return ExtensionSlot::kSequenceAssItem;
    case Py_sq_concat:
      return ExtensionSlot::kSequenceConcat;
    case Py_sq_contains:
      return ExtensionSlot::kSequenceContains;
    case Py_sq_inplace_concat:
      return ExtensionSlot::kSequenceInplaceConcat;
    case Py_sq_inplace_repeat:
      return ExtensionSlot::kSequenceInplaceRepeat;
    case Py_sq_item:
      return ExtensionSlot::kSequenceItem;
    case Py_sq_length:
      return ExtensionSlot::kSequenceLength;
    case Py_sq_repeat:
      return ExtensionSlot::kSequenceRepeat;
    case Py_tp_alloc:
      return ExtensionSlot::kAlloc;
    case Py_tp_base:
      return ExtensionSlot::kBase;
    case Py_tp_bases:
      return ExtensionSlot::kBases;
    case Py_tp_call:
      return ExtensionSlot::kCall;
    case Py_tp_clear:
      return ExtensionSlot::kClear;
    case Py_tp_dealloc:
      return ExtensionSlot::kDealloc;
    case Py_tp_del:
      return ExtensionSlot::kDel;
    case Py_tp_descr_get:
      return ExtensionSlot::kDescrGet;
    case Py_tp_descr_set:
      return ExtensionSlot::kDescrSet;
    case Py_tp_doc:
      return ExtensionSlot::kDoc;
    case Py_tp_getattr:
      return ExtensionSlot::kGetattr;
    case Py_tp_getattro:
      return ExtensionSlot::kGetattro;
    case Py_tp_hash:
      return ExtensionSlot::kHash;
    case Py_tp_init:
      return ExtensionSlot::kInit;
    case Py_tp_is_gc:
      return ExtensionSlot::kIsGc;
    case Py_tp_iter:
      return ExtensionSlot::kIter;
    case Py_tp_iternext:
      return ExtensionSlot::kIternext;
    case Py_tp_methods:
      return ExtensionSlot::kMethods;
    case Py_tp_new:
      return ExtensionSlot::kNew;
    case Py_tp_repr:
      return ExtensionSlot::kRepr;
    case Py_tp_richcompare:
      return ExtensionSlot::kRichcompare;
    case Py_tp_setattr:
      return ExtensionSlot::kSetattr;
    case Py_tp_setattro:
      return ExtensionSlot::kSetattro;
    case Py_tp_str:
      return ExtensionSlot::kStr;
    case Py_tp_traverse:
      return ExtensionSlot::kTraverse;
    case Py_tp_members:
      return ExtensionSlot::kMembers;
    case Py_tp_getset:
      return ExtensionSlot::kGetset;
    case Py_tp_free:
      return ExtensionSlot::kFree;
    case Py_nb_matrix_multiply:
      return ExtensionSlot::kNumberMatrixMultiply;
    case Py_nb_inplace_matrix_multiply:
      return ExtensionSlot::kNumberInplaceMatrixMultiply;
    case Py_am_await:
      return ExtensionSlot::kAsyncAwait;
    case Py_am_aiter:
      return ExtensionSlot::kAsyncAiter;
    case Py_am_anext:
      return ExtensionSlot::kAsyncAnext;
    case Py_tp_finalize:
      return ExtensionSlot::kFinalize;
    default:
      return ExtensionSlot::kEnd;
  }
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

RawObject wrapUnaryfunc(Thread* thread, Frame* frame, word argc) {
  auto func = getNativeFunc<unaryfunc>(thread, frame);

  Arguments args(frame, argc);
  PyObject* o = ApiHandle::borrowedReference(thread, args.get(0));
  PyObject* result = (*func)(o);
  return ApiHandle::checkFunctionResult(thread, result);
}

// Common work for hashfunc, lenfunc, and inquiry, all of which take a single
// PyObject* and return an integral value.
template <class cfunc, class RetFunc>
RawObject wrapIntegralfunc(Thread* thread, Frame* frame, word argc,
                           RetFunc ret) {
  auto func = getNativeFunc<cfunc>(thread, frame);

  Arguments args(frame, argc);
  PyObject* o = ApiHandle::borrowedReference(thread, args.get(0));
  auto result = func(o);
  if (result == -1 && thread->hasPendingException()) return Error::exception();
  return ret(result);
}

RawObject wrapHashfunc(Thread* thread, Frame* frame, word argc) {
  return wrapIntegralfunc<hashfunc>(
      thread, frame, argc,
      [thread](Py_hash_t hash) { return thread->runtime()->newInt(hash); });
}

RawObject wrapLenfunc(Thread* thread, Frame* frame, word argc) {
  return wrapIntegralfunc<lenfunc>(
      thread, frame, argc,
      [thread](Py_ssize_t len) { return thread->runtime()->newInt(len); });
}

RawObject wrapInquirypred(Thread* thread, Frame* frame, word argc) {
  return wrapIntegralfunc<inquiry>(
      thread, frame, argc, [](int result) { return Bool::fromBool(result); });
}

RawObject wrapBinaryfuncImpl(Thread* thread, Frame* frame, word argc,
                             bool swap) {
  auto func = getNativeFunc<binaryfunc>(thread, frame);

  Arguments args(frame, argc);
  PyObject* o1 = ApiHandle::borrowedReference(thread, args.get(swap ? 1 : 0));
  PyObject* o2 = ApiHandle::borrowedReference(thread, args.get(swap ? 0 : 1));
  PyObject* result = (*func)(o1, o2);
  return ApiHandle::checkFunctionResult(thread, result);
}

RawObject wrapBinaryfunc(Thread* thread, Frame* frame, word argc) {
  return wrapBinaryfuncImpl(thread, frame, argc, false);
}

RawObject wrapBinaryfuncSwapped(Thread* thread, Frame* frame, word argc) {
  return wrapBinaryfuncImpl(thread, frame, argc, false);
}

RawObject wrapTernaryfuncImpl(Thread* thread, Frame* frame, word argc,
                              bool swap) {
  auto func = getNativeFunc<ternaryfunc>(thread, frame);

  Arguments args(frame, argc);
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
RawObject wrapTernaryfunc(Thread* thread, Frame* frame, word argc) {
  return wrapTernaryfuncImpl(thread, frame, argc, false);
}

RawObject wrapTernaryfuncSwapped(Thread* thread, Frame* frame, word argc) {
  return wrapTernaryfuncImpl(thread, frame, argc, true);
}

RawObject wrapVarkwTernaryfunc(Thread* thread, Frame* frame, word argc) {
  DCHECK(argc == 3, "Unexpected argc");
  auto func = getNativeFunc<ternaryfunc>(thread, frame);

  Arguments args(frame, argc);
  PyObject* self = ApiHandle::borrowedReference(thread, args.get(0));
  PyObject* varargs = ApiHandle::borrowedReference(thread, args.get(1));
  PyObject* kwargs = Dict::cast(args.get(2)).numItems() == 0
                         ? nullptr
                         : ApiHandle::borrowedReference(thread, args.get(2));
  PyObject* result = (*func)(self, varargs, kwargs);
  return ApiHandle::checkFunctionResult(thread, result);
}

RawObject wrapSetattr(Thread* thread, Frame* frame, word argc) {
  auto func = getNativeFunc<setattrofunc>(thread, frame);

  Arguments args(frame, argc);
  PyObject* self = ApiHandle::borrowedReference(thread, args.get(0));
  PyObject* name = ApiHandle::borrowedReference(thread, args.get(1));
  PyObject* value = ApiHandle::borrowedReference(thread, args.get(2));
  if (func(self, name, value) < 0) return Error::exception();
  return NoneType::object();
}

RawObject wrapDelattr(Thread* thread, Frame* frame, word argc) {
  auto func = getNativeFunc<setattrofunc>(thread, frame);

  Arguments args(frame, argc);
  PyObject* self = ApiHandle::borrowedReference(thread, args.get(0));
  PyObject* name = ApiHandle::borrowedReference(thread, args.get(1));
  if (func(self, name, nullptr) < 0) return Error::exception();
  return NoneType::object();
}

template <CompareOp op>
RawObject wrapRichcompare(Thread* thread, Frame* frame, word argc) {
  auto func = getNativeFunc<richcmpfunc>(thread, frame);

  Arguments args(frame, argc);
  PyObject* self = ApiHandle::borrowedReference(thread, args.get(0));
  PyObject* other = ApiHandle::borrowedReference(thread, args.get(1));
  PyObject* result = (*func)(self, other, op);
  return ApiHandle::checkFunctionResult(thread, result);
}

RawObject wrapNext(Thread* thread, Frame* frame, word argc) {
  auto func = getNativeFunc<unaryfunc>(thread, frame);

  Arguments args(frame, argc);
  PyObject* self = ApiHandle::borrowedReference(thread, args.get(0));
  PyObject* result = (*func)(self);
  if (result == nullptr && !thread->hasPendingException()) {
    return thread->raise(LayoutId::kStopIteration, NoneType::object());
  }
  return ApiHandle::checkFunctionResult(thread, result);
}

RawObject wrapDescrGet(Thread* thread, Frame* frame, word argc) {
  auto func = getNativeFunc<descrgetfunc>(thread, frame);

  Arguments args(frame, argc);
  PyObject* self = ApiHandle::borrowedReference(thread, args.get(0));
  PyObject* obj = nullptr;
  if (!args.get(1).isNoneType()) {
    obj = ApiHandle::borrowedReference(thread, args.get(1));
  }
  PyObject* type = nullptr;
  if (argc >= 3 && !args.get(2).isNoneType()) {
    type = ApiHandle::borrowedReference(thread, args.get(2));
  }
  if (obj == nullptr && type == nullptr) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "__get__(None, None), is invalid");
  }
  PyObject* result = (*func)(self, obj, type);
  return ApiHandle::checkFunctionResult(thread, result);
}

RawObject wrapDescrSet(Thread* thread, Frame* frame, word argc) {
  auto func = getNativeFunc<descrsetfunc>(thread, frame);

  Arguments args(frame, argc);
  PyObject* self = ApiHandle::borrowedReference(thread, args.get(0));
  PyObject* obj = ApiHandle::borrowedReference(thread, args.get(1));
  PyObject* value = ApiHandle::borrowedReference(thread, args.get(2));
  if (func(self, obj, value) < 0) return Error::exception();
  return NoneType::object();
}

RawObject wrapDescrDelete(Thread* thread, Frame* frame, word argc) {
  auto func = getNativeFunc<descrsetfunc>(thread, frame);

  Arguments args(frame, argc);
  PyObject* self = ApiHandle::borrowedReference(thread, args.get(0));
  PyObject* obj = ApiHandle::borrowedReference(thread, args.get(1));
  if (func(self, obj, nullptr) < 0) return Error::exception();
  return NoneType::object();
}

RawObject wrapInit(Thread* thread, Frame* frame, word argc) {
  DCHECK(argc == 3, "Unexpected argc");
  auto func = getNativeFunc<initproc>(thread, frame);

  Arguments args(frame, argc);
  PyObject* self = ApiHandle::borrowedReference(thread, args.get(0));
  PyObject* varargs = ApiHandle::borrowedReference(thread, args.get(1));
  PyObject* kwargs = Dict::cast(args.get(2)).numItems() == 0
                         ? nullptr
                         : ApiHandle::borrowedReference(thread, args.get(2));
  if (func(self, varargs, kwargs) < 0) return Error::exception();
  return NoneType::object();
}

RawObject wrapDel(Thread* thread, Frame* frame, word argc) {
  auto func = getNativeFunc<destructor>(thread, frame);

  Arguments args(frame, argc);
  PyObject* self = ApiHandle::borrowedReference(thread, args.get(0));
  func(self);
  return NoneType::object();
}

RawObject wrapObjobjargproc(Thread* thread, Frame* frame, word argc) {
  auto func = getNativeFunc<objobjargproc>(thread, frame);

  Arguments args(frame, argc);
  PyObject* self = ApiHandle::borrowedReference(thread, args.get(0));
  PyObject* key = ApiHandle::borrowedReference(thread, args.get(1));
  PyObject* value = ApiHandle::borrowedReference(thread, args.get(2));
  int res = func(self, key, value);
  if (res == -1 && thread->hasPendingException()) return Error::exception();
  return NoneType::object();
}

RawObject wrapObjobjproc(Thread* thread, Frame* frame, word argc) {
  auto func = getNativeFunc<objobjproc>(thread, frame);

  Arguments args(frame, argc);
  PyObject* self = ApiHandle::borrowedReference(thread, args.get(0));
  PyObject* value = ApiHandle::borrowedReference(thread, args.get(1));
  int res = func(self, value);
  if (res == -1 && thread->hasPendingException()) return Error::exception();
  return Bool::fromBool(res);
}

RawObject wrapDelitem(Thread* thread, Frame* frame, word argc) {
  auto func = getNativeFunc<objobjargproc>(thread, frame);

  Arguments args(frame, argc);
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
  Int i(&scope, intUnderlying(thread, converted));
  if (i.numDigits() != 1) {
    return thread->raiseWithFmt(LayoutId::kOverflowError,
                                "cannot fit '%T' into an index-sized integer",
                                &obj);
  }
  return *i;
}

RawObject wrapIndexargfunc(Thread* thread, Frame* frame, word argc) {
  auto func = getNativeFunc<ssizeargfunc>(thread, frame);

  HandleScope scope(thread);
  Arguments args(frame, argc);
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
  Object len(&scope, thread->invokeFunction1(SymbolId::kBuiltins,
                                             SymbolId::kLen, self));
  if (len.isError()) return *len;
  len = makeIndex(thread, len);
  if (len.isError()) return *len;
  i += Int::cast(*len).asWord();
  return thread->runtime()->newInt(i);
}

RawObject wrapSqItem(Thread* thread, Frame* frame, word argc) {
  auto func = getNativeFunc<ssizeargfunc>(thread, frame);

  HandleScope scope(thread);
  Arguments args(frame, argc);
  Object self(&scope, args.get(0));
  Object arg(&scope, args.get(1));
  arg = normalizeIndex(thread, self, arg);
  if (arg.isError()) return *arg;
  PyObject* py_self = ApiHandle::borrowedReference(thread, *self);
  PyObject* result = (*func)(py_self, Int::cast(*arg).asWord());
  return ApiHandle::checkFunctionResult(thread, result);
}

RawObject wrapSqSetitem(Thread* thread, Frame* frame, word argc) {
  auto func = getNativeFunc<ssizeobjargproc>(thread, frame);

  HandleScope scope(thread);
  Arguments args(frame, argc);
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

RawObject wrapSqDelitem(Thread* thread, Frame* frame, word argc) {
  auto func = getNativeFunc<ssizeobjargproc>(thread, frame);

  HandleScope scope(thread);
  Arguments args(frame, argc);
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
  ExtensionSlot id;

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

static const SymbolId kParamsSelfArgsKwargs[] = {
    SymbolId::kSelf, SymbolId::kArgs, SymbolId::kKwargs};
static const SymbolId kParamsSelfInstanceOwner[] = {
    SymbolId::kSelf, SymbolId::kInstance, SymbolId::kOwner};
static const SymbolId kParamsSelfInstanceValue[] = {
    SymbolId::kSelf, SymbolId::kInstance, SymbolId::kValue};
static const SymbolId kParamsSelfInstance[] = {SymbolId::kSelf,
                                               SymbolId::kInstance};
static const SymbolId kParamsSelfKeyValue[] = {SymbolId::kSelf, SymbolId::kKey,
                                               SymbolId::kValue};
static const SymbolId kParamsSelfKey[] = {SymbolId::kSelf, SymbolId::kKey};
static const SymbolId kParamsSelfNameValue[] = {
    SymbolId::kSelf, SymbolId::kName, SymbolId::kValue};
static const SymbolId kParamsSelfName[] = {SymbolId::kSelf, SymbolId::kName};
static const SymbolId kParamsSelfValueMod[] = {
    SymbolId::kSelf, SymbolId::kValue, SymbolId::kMod};
static const SymbolId kParamsSelfValue[] = {SymbolId::kSelf, SymbolId::kValue};
static const SymbolId kParamsSelf[] = {SymbolId::kSelf};
static const SymbolId kParamsTypeArgsKwargs[] = {
    SymbolId::kType, SymbolId::kArgs, SymbolId::kKwargs};

// These macros currently ignore the FUNCTION argument, which is still the
// function name inherited from CPython. This will be cleaned up when we add
// default slot implementations that delegate to the corresponding Python
// method, along with logic to update slots as needed when a user assigns to a
// type dict.
#define TPSLOT(NAME, SLOT, PARAMETERS, FUNCTION, WRAPPER, DOC)                 \
  {                                                                            \
    SymbolId::NAME, ExtensionSlot::SLOT, PARAMETERS, ARRAYSIZE(PARAMETERS),    \
        WRAPPER, 0, DOC                                                        \
  }
#define KWSLOT(NAME, SLOT, PARAMETERS, FUNCTION, WRAPPER, DOC)                 \
  {                                                                            \
    SymbolId::NAME, ExtensionSlot::SLOT, PARAMETERS, ARRAYSIZE(PARAMETERS),    \
        WRAPPER, Code::Flags::VARARGS | Code::Flags::VARKEYARGS, DOC           \
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
    TPSLOT(kDunderGetattribute, kGetattr, kParamsSelfName, nullptr,
           bit_cast<Function::Entry>(nullptr), ""),
    TPSLOT(kDunderGetattr, kGetattr, kParamsSelfName, nullptr, nullptr, ""),
    TPSLOT(kDunderSetattr, kSetattr, kParamsSelfNameValue, nullptr, nullptr,
           ""),
    TPSLOT(kDunderDelattr, kSetattr, kParamsSelfName, nullptr, nullptr, ""),
    TPSLOT(kDunderRepr, kRepr, kParamsSelf, slot_tp_repr, wrapUnaryfunc,
           "__repr__($self, /)\n--\n\nReturn repr(self)."),
    TPSLOT(kDunderHash, kHash, kParamsSelf, slot_tp_hash, wrapHashfunc,
           "__hash__($self, /)\n--\n\nReturn hash(self)."),
    KWSLOT(
        kDunderCall, kCall, kParamsSelfArgsKwargs, slot_tp_call,
        wrapVarkwTernaryfunc,
        "__call__($self, /, *args, **kwargs)\n--\n\nCall self as a function."),
    TPSLOT(kDunderStr, kStr, kParamsSelfArgsKwargs, slot_tp_str, wrapUnaryfunc,
           "__str__($self, /)\n--\n\nReturn str(self)."),
    TPSLOT(kDunderGetattribute, kGetattro, kParamsSelfName,
           slot_tp_getattr_hook, wrapBinaryfunc,
           "__getattribute__($self, name, /)\n--\n\nReturn getattr(self, "
           "name)."),
    TPSLOT(kDunderGetattr, kGetattro, kParamsSelfName, slot_tp_getattr_hook,
           nullptr, ""),
    TPSLOT(kDunderSetattr, kSetattro, kParamsSelfNameValue, slot_tp_setattro,
           wrapSetattr,
           "__setattr__($self, name, value, /)\n--\n\nImplement setattr(self, "
           "name, value)."),
    TPSLOT(kDunderDelattr, kSetattro, kParamsSelfName, slot_tp_setattro,
           wrapDelattr,
           "__delattr__($self, name, /)\n--\n\nImplement delattr(self, name)."),
    TPSLOT(kDunderLt, kRichcompare, kParamsSelfValue, slot_tp_richcompare,
           wrapRichcompare<LT>,
           "__lt__($self, value, /)\n--\n\nReturn self<value."),
    TPSLOT(kDunderLe, kRichcompare, kParamsSelfValue, slot_tp_richcompare,
           wrapRichcompare<LE>,
           "__le__($self, value, /)\n--\n\nReturn self<=value."),
    TPSLOT(kDunderEq, kRichcompare, kParamsSelfValue, slot_tp_richcompare,
           wrapRichcompare<EQ>,
           "__eq__($self, value, /)\n--\n\nReturn self==value."),
    TPSLOT(kDunderNe, kRichcompare, kParamsSelfValue, slot_tp_richcompare,
           wrapRichcompare<NE>,
           "__ne__($self, value, /)\n--\n\nReturn self!=value."),
    TPSLOT(kDunderGt, kRichcompare, kParamsSelfValue, slot_tp_richcompare,
           wrapRichcompare<GT>,
           "__gt__($self, value, /)\n--\n\nReturn self>value."),
    TPSLOT(kDunderGe, kRichcompare, kParamsSelfValue, slot_tp_richcompare,
           wrapRichcompare<GE>,
           "__ge__($self, value, /)\n--\n\nReturn self>=value."),
    TPSLOT(kDunderIter, kIter, kParamsSelfValue, slot_tp_iter, wrapUnaryfunc,
           "__iter__($self, /)\n--\n\nImplement iter(self)."),
    TPSLOT(kDunderNext, kIternext, kParamsSelf, slot_tp_iternext, wrapNext,
           "__next__($self, /)\n--\n\nImplement next(self)."),
    TPSLOT(kDunderGet, kDescrGet, kParamsSelfInstanceOwner, slot_tp_descr_get,
           wrapDescrGet,
           "__get__($self, instance, owner, /)\n--\n\nReturn an attribute of "
           "instance, which is of type owner."),
    TPSLOT(kDunderSet, kDescrSet, kParamsSelfInstanceValue, slot_tp_descr_set,
           wrapDescrSet,
           "__set__($self, instance, value, /)\n--\n\nSet an attribute of "
           "instance to value."),
    TPSLOT(kDunderDelete, kDescrSet, kParamsSelfInstance, slot_tp_descr_set,
           wrapDescrDelete,
           "__delete__($self, instance, /)\n--\n\nDelete an attribute of "
           "instance."),
    KWSLOT(kDunderInit, kInit, kParamsSelfArgsKwargs, slot_tp_init, wrapInit,
           "__init__($self, /, *args, **kwargs)\n--\n\nInitialize self.  See "
           "help(type(self)) for accurate signature."),
    KWSLOT(kDunderNew, kNew, kParamsTypeArgsKwargs, slot_tp_new,
           wrapVarkwTernaryfunc,
           "__new__(type, /, *args, **kwargs)\n--\n\n"
           "Create and return new object.  See help(type) for accurate "
           "signature."),
    TPSLOT(kDunderDel, kFinalize, kParamsSelf, slot_tp_finalize, wrapDel, ""),
    TPSLOT(kDunderAwait, kAsyncAwait, kParamsSelf, slot_am_await, wrapUnaryfunc,
           "__await__($self, /)\n--\n\nReturn an iterator to be used in await "
           "expression."),
    TPSLOT(kDunderAiter, kAsyncAiter, kParamsSelf, slot_am_aiter, wrapUnaryfunc,
           "__aiter__($self, /)\n--\n\nReturn an awaitable, that resolves in "
           "asynchronous iterator."),
    TPSLOT(kDunderAnext, kAsyncAnext, kParamsSelf, slot_am_anext, wrapUnaryfunc,
           "__anext__($self, /)\n--\n\nReturn a value or raise "
           "StopAsyncIteration."),
    BINSLOT(kDunderAdd, "__add__", kNumberAdd, slot_nb_add, "+"),
    RBINSLOT(kDunderRadd, "__radd__", kNumberAdd, slot_nb_add, "+"),
    BINSLOT(kDunderSub, "__sub__", kNumberSubtract, slot_nb_subtract, "-"),
    RBINSLOT(kDunderRsub, "__rsub__", kNumberSubtract, slot_nb_subtract, "-"),
    BINSLOT(kDunderMul, "__mul__", kNumberMultiply, slot_nb_multiply, "*"),
    RBINSLOT(kDunderRmul, "__rmul__", kNumberMultiply, slot_nb_multiply, "*"),
    BINSLOT(kDunderMod, "__mod__", kNumberRemainder, slot_nb_remainder, "%"),
    RBINSLOT(kDunderRmod, "__rmod__", kNumberRemainder, slot_nb_remainder, "%"),
    BINSLOTNOTINFIX(kDunderDivmod, "__divmod__", kNumberDivmod, slot_nb_divmod,
                    "Return divmod(self, value)."),
    RBINSLOTNOTINFIX(kDunderRdivmod, "__rdivmod__", kNumberDivmod,
                     slot_nb_divmod, "Return divmod(value, self)."),
    TPSLOT(kDunderPow, kNumberPower, kParamsSelfValueMod, slot_nb_power,
           wrapTernaryfunc,
           "__pow__($self, value, mod=None, /)\n--\n\nReturn pow(self, value, "
           "mod)."),
    TPSLOT(kDunderRpow, kNumberPower, kParamsSelfValueMod, slot_nb_power,
           wrapTernaryfuncSwapped,
           "__rpow__($self, value, mod=None, /)\n--\n\nReturn pow(value, self, "
           "mod)."),
    UNSLOT(kDunderNeg, "__neg__", kNumberNegative, slot_nb_negative,
           wrapUnaryfunc, "-self"),
    UNSLOT(kDunderPos, "__pos__", kNumberPositive, slot_nb_positive,
           wrapUnaryfunc, "+self"),
    UNSLOT(kDunderAbs, "__abs__", kNumberAbsolute, slot_nb_absolute,
           wrapUnaryfunc, "abs(self)"),
    UNSLOT(kDunderBool, "__bool__", kNumberBool, slot_nb_bool, wrapInquirypred,
           "self != 0"),
    UNSLOT(kDunderInvert, "__invert__", kNumberInvert, slot_nb_invert,
           wrapUnaryfunc, "~self"),
    BINSLOT(kDunderLshift, "__lshift__", kNumberLshift, slot_nb_lshift, "<<"),
    RBINSLOT(kDunderRlshift, "__rlshift__", kNumberLshift, slot_nb_lshift,
             "<<"),
    BINSLOT(kDunderRshift, "__rshift__", kNumberRshift, slot_nb_rshift, ">>"),
    RBINSLOT(kDunderRrshift, "__rrshift__", kNumberRshift, slot_nb_rshift,
             ">>"),
    BINSLOT(kDunderAnd, "__and__", kNumberAnd, slot_nb_and, "&"),
    RBINSLOT(kDunderRand, "__rand__", kNumberAnd, slot_nb_and, "&"),
    BINSLOT(kDunderXor, "__xor__", kNumberXor, slot_nb_xor, "^"),
    RBINSLOT(kDunderRxor, "__rxor__", kNumberXor, slot_nb_xor, "^"),
    BINSLOT(kDunderOr, "__or__", kNumberOr, slot_nb_or, "|"),
    RBINSLOT(kDunderRor, "__ror__", kNumberOr, slot_nb_or, "|"),
    UNSLOT(kDunderInt, "__int__", kNumberInt, slot_nb_int, wrapUnaryfunc,
           "int(self)"),
    UNSLOT(kDunderFloat, "__float__", kNumberFloat, slot_nb_float,
           wrapUnaryfunc, "float(self)"),
    IBSLOT(kDunderIadd, "__iadd__", kNumberInplaceAdd, slot_nb_inplace_add,
           wrapBinaryfunc, "+="),
    IBSLOT(kDunderIsub, "__isub__", kNumberInplaceSubtract,
           slot_nb_inplace_subtract, wrapBinaryfunc, "-="),
    IBSLOT(kDunderImul, "__imul__", kNumberInplaceMultiply,
           slot_nb_inplace_multiply, wrapBinaryfunc, "*="),
    IBSLOT(kDunderImod, "__imod__", kNumberInplaceRemainder,
           slot_nb_inplace_remainder, wrapBinaryfunc, "%="),
    IBSLOT(kDunderIpow, "__ipow__", kNumberInplacePower, slot_nb_inplace_power,
           wrapBinaryfunc, "**="),
    IBSLOT(kDunderIlshift, "__ilshift__", kNumberInplaceLshift,
           slot_nb_inplace_lshift, wrapBinaryfunc, "<<="),
    IBSLOT(kDunderIrshift, "__irshift__", kNumberInplaceRshift,
           slot_nb_inplace_rshift, wrapBinaryfunc, ">>="),
    IBSLOT(kDunderIand, "__iand__", kNumberInplaceAnd, slot_nb_inplace_and,
           wrapBinaryfunc, "&="),
    IBSLOT(kDunderIxor, "__ixor__", kNumberInplaceXor, slot_nb_inplace_xor,
           wrapBinaryfunc, "^="),
    IBSLOT(kDunderIor, "__ior__", kNumberInplaceOr, slot_nb_inplace_or,
           wrapBinaryfunc, "|="),
    BINSLOT(kDunderFloordiv, "__floordiv__", kNumberFloorDivide,
            slot_nb_floor_divide, "//"),
    RBINSLOT(kDunderRfloordiv, "__rfloordiv__", kNumberFloorDivide,
             slot_nb_floor_divide, "//"),
    BINSLOT(kDunderTruediv, "__truediv__", kNumberTrueDivide,
            slot_nb_true_divide, "/"),
    RBINSLOT(kDunderRtruediv, "__rtruediv__", kNumberTrueDivide,
             slot_nb_true_divide, "/"),
    IBSLOT(kDunderIfloordiv, "__ifloordiv__", kNumberInplaceFloorDivide,
           slot_nb_inplace_floor_divide, wrapBinaryfunc, "//="),
    IBSLOT(kDunderItruediv, "__itruediv__", kNumberInplaceTrueDivide,
           slot_nb_inplace_true_divide, wrapBinaryfunc, "/="),
    TPSLOT(kDunderIndex, kNumberIndex, kParamsSelf, slot_nb_index,
           wrapUnaryfunc,
           "__index__($self, /)\n--\n\n"
           "Return self converted to an integer, if self is suitable "
           "for use as an index into a list."),
    BINSLOT(kDunderMatmul, "__matmul__", kNumberMatrixMultiply,
            slot_nb_matrix_multiply, "@"),
    RBINSLOT(kDunderRmatmul, "__rmatmul__", kNumberMatrixMultiply,
             slot_nb_matrix_multiply, "@"),
    IBSLOT(kDunderImatmul, "__imatmul__", kNumberInplaceMatrixMultiply,
           slot_nb_inplace_matrix_multiply, wrapBinaryfunc, "@="),
    TPSLOT(kDunderLen, kMapLength, kParamsSelf, slot_mp_length, wrapLenfunc,
           "__len__($self, /)\n--\n\nReturn len(self)."),
    TPSLOT(kDunderGetitem, kMapSubscript, kParamsSelfKey, slot_mp_subscript,
           wrapBinaryfunc,
           "__getitem__($self, key, /)\n--\n\nReturn self[key]."),
    TPSLOT(kDunderSetitem, kMapAssSubscript, kParamsSelfKeyValue,
           slot_mp_ass_subscript, wrapObjobjargproc,
           "__setitem__($self, key, value, /)\n--\n\nSet self[key] to value."),
    TPSLOT(kDunderDelitem, kMapAssSubscript, kParamsSelfKey,
           slot_mp_ass_subscript, wrapDelitem,
           "__delitem__($self, key, /)\n--\n\nDelete self[key]."),
    TPSLOT(kDunderLen, kSequenceLength, kParamsSelf, slot_sq_length,
           wrapLenfunc, "__len__($self, /)\n--\n\nReturn len(self)."),
    TPSLOT(kDunderAdd, kSequenceConcat, kParamsSelfValue, nullptr,
           wrapBinaryfunc,
           "__add__($self, value, /)\n--\n\nReturn self+value."),
    TPSLOT(kDunderMul, kSequenceRepeat, kParamsSelfValue, nullptr,
           wrapIndexargfunc,
           "__mul__($self, value, /)\n--\n\nReturn self*value."),
    TPSLOT(kDunderRmul, kSequenceRepeat, kParamsSelfValue, nullptr,
           wrapIndexargfunc,
           "__rmul__($self, value, /)\n--\n\nReturn value*self."),
    TPSLOT(kDunderGetitem, kSequenceItem, kParamsSelfKey, slot_sq_item,
           wrapSqItem, "__getitem__($self, key, /)\n--\n\nReturn self[key]."),
    TPSLOT(kDunderSetitem, kSequenceAssItem, kParamsSelfKeyValue,
           slot_sq_ass_item, wrapSqSetitem,
           "__setitem__($self, key, value, /)\n--\n\nSet self[key] to value."),
    TPSLOT(kDunderDelitem, kSequenceAssItem, kParamsSelfKey, slot_sq_ass_item,
           wrapSqDelitem,
           "__delitem__($self, key, /)\n--\n\nDelete self[key]."),
    TPSLOT(kDunderContains, kSequenceContains, kParamsSelfKey, slot_sq_contains,
           wrapObjobjproc,
           "__contains__($self, key, /)\n--\n\nReturn key in self."),
    TPSLOT(kDunderIadd, kSequenceInplaceConcat, kParamsSelfValue, nullptr,
           wrapBinaryfunc,
           "__iadd__($self, value, /)\n--\n\nImplement self+=value."),
    TPSLOT(kDunderImul, kSequenceInplaceRepeat, kParamsSelfValue, nullptr,
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

  word argcount = num_parameters - ((flags & Code::Flags::VARARGS) != 0) -
                  ((flags & Code::Flags::VARKEYARGS) != 0);
  flags |= Code::Flags::OPTIMIZED | Code::Flags::NEWLOCALS;

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
  Dict dict(&scope, type.dict());
  Str type_name(&scope, type.name());

  for (const SlotDef& slot : kSlotdefs) {
    if (slot.wrapper == nullptr) continue;
    Object slot_value(&scope, extensionSlot(type, slot.id));
    if (slot_value.isNoneType()) continue;
    DCHECK(slot_value.isInt(), "unexpected slot type");

    Str slot_name(&scope, runtime->symbols()->at(slot.name));
    if (!runtime->typeDictAt(thread, dict, slot_name).isError()) continue;

    // When given PyObject_HashNotImplemented, put None in the type dict
    // rather than a wrapper. CPython does this regardless of which slot it
    // was given for, so we do too.
    if (Int::cast(*slot_value).asCPtr() ==
        bit_cast<void*>(&PyObject_HashNotImplemented)) {
      Object none(&scope, NoneType::object());
      runtime->typeDictAtPut(thread, dict, slot_name, none);
      return NoneType::object();
    }

    // Create the wrapper function.
    Str qualname(&scope,
                 runtime->newStrFromFmt("%S.%S", &type_name, &slot_name));
    Code code(&scope, newExtCode(thread, slot_name, slot.parameters,
                                 slot.num_parameters, slot.flags,
                                 bit_cast<void*>(slot.wrapper), slot_value));
    Object globals(&scope, NoneType::object());
    Function func(
        &scope, runtime->newFunctionWithCode(thread, qualname, code, globals));
    if (slot.id == ExtensionSlot::kNumberPower) {
      func.setDefaults(runtime->newTuple(1));
    }

    // __new__ is the one special-case static method, so wrap it
    // appropriately.
    Object func_obj(&scope, *func);
    if (slot.id == ExtensionSlot::kNew) {
      func_obj = thread->invokeFunction1(SymbolId::kBuiltins,
                                         SymbolId::kStaticMethod, func);
      if (func_obj.isError()) return *func_obj;
    }

    // Finally, put the wrapper in the type dict.
    runtime->typeDictAtPut(thread, dict, slot_name, func_obj);
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
  Tuple args_tuple(&scope, tupleUnderlying(thread, args_obj));
  MutableTuple new_args(&scope,
                        runtime->newMutableTuple(args_tuple.length() + 1));
  new_args.atPut(0, *type_obj);
  new_args.replaceFromWith(1, *args_tuple, args_tuple.length());

  // Call type.__new__(type, *args, **kwargs)
  Object dunder_new(
      &scope, runtime->attributeAtById(thread, type_obj, SymbolId::kDunderNew));
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
void* defaultSlot(ExtensionSlot slot) {
  switch (slot) {
    case ExtensionSlot::kNew:
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
  Type type(&scope,
            ApiHandle::fromPyObject(reinterpret_cast<PyObject*>(type_obj))
                ->asObject());
  if (type.isBuiltin()) {
    thread->raiseBadInternalCall();
    return nullptr;
  }

  // Extension module requesting slot from a future version
  ExtensionSlot field_id = slotToTypeSlot(slot);
  if (field_id >= ExtensionSlot::kEnd) {
    return nullptr;
  }

  if (type.extensionSlots().isNoneType()) {
    // The Type was not created by PyType_FromSpec(), so return a default slot
    // implementation that delegates to managed methods.
    return defaultSlot(field_id);
  }

  DCHECK(!type.extensionSlots().isNoneType(), "Type is not extension type");
  Object slot_obj(&scope, extensionSlot(type, field_id));
  if (slot_obj.isNoneType()) {
    return nullptr;
  }
  Int address(&scope, *slot_obj);
  return address.asCPtr();
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
      return thread->invokeFunction1(SymbolId::kBuiltins,
                                     SymbolId::kUnderNewMemberGetBool, offset);
    case T_BYTE:
      return thread->invokeFunction1(SymbolId::kBuiltins,
                                     SymbolId::kUnderNewMemberGetByte, offset);
    case T_UBYTE:
      return thread->invokeFunction1(SymbolId::kBuiltins,
                                     SymbolId::kUnderNewMemberGetUByte, offset);
    case T_SHORT:
      return thread->invokeFunction1(SymbolId::kBuiltins,
                                     SymbolId::kUnderNewMemberGetShort, offset);
    case T_USHORT:
      return thread->invokeFunction1(
          SymbolId::kBuiltins, SymbolId::kUnderNewMemberGetUShort, offset);
    case T_INT:
      return thread->invokeFunction1(SymbolId::kBuiltins,
                                     SymbolId::kUnderNewMemberGetInt, offset);
    case T_UINT:
      return thread->invokeFunction1(SymbolId::kBuiltins,
                                     SymbolId::kUnderNewMemberGetUInt, offset);
    case T_LONG:
      return thread->invokeFunction1(SymbolId::kBuiltins,
                                     SymbolId::kUnderNewMemberGetLong, offset);
    case T_ULONG:
      return thread->invokeFunction1(SymbolId::kBuiltins,
                                     SymbolId::kUnderNewMemberGetULong, offset);
    case T_PYSSIZET:
      return thread->invokeFunction1(SymbolId::kBuiltins,
                                     SymbolId::kUnderNewMemberGetULong, offset);
    case T_FLOAT:
      return thread->invokeFunction1(SymbolId::kBuiltins,
                                     SymbolId::kUnderNewMemberGetFloat, offset);
    case T_DOUBLE:
      return thread->invokeFunction1(
          SymbolId::kBuiltins, SymbolId::kUnderNewMemberGetDouble, offset);
    case T_LONGLONG:
      return thread->invokeFunction1(SymbolId::kBuiltins,
                                     SymbolId::kUnderNewMemberGetLong, offset);
    case T_ULONGLONG:
      return thread->invokeFunction1(SymbolId::kBuiltins,
                                     SymbolId::kUnderNewMemberGetULong, offset);
    case T_STRING:
      return thread->invokeFunction1(
          SymbolId::kBuiltins, SymbolId::kUnderNewMemberGetString, offset);
    case T_STRING_INPLACE:
      return thread->invokeFunction1(
          SymbolId::kBuiltins, SymbolId::kUnderNewMemberGetString, offset);
    case T_CHAR:
      return thread->invokeFunction1(SymbolId::kBuiltins,
                                     SymbolId::kUnderNewMemberGetChar, offset);
    case T_OBJECT:
      return thread->invokeFunction1(
          SymbolId::kBuiltins, SymbolId::kUnderNewMemberGetPyObject, offset);
    case T_OBJECT_EX:
      return thread->invokeFunction2(SymbolId::kBuiltins,
                                     SymbolId::kUnderNewMemberGetPyObject,
                                     offset, name);
    case T_NONE:
      return thread->invokeFunction1(
          SymbolId::kBuiltins, SymbolId::kUnderNewMemberGetPyObject, offset);
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
    Function setter(&scope, thread->invokeFunction1(
                                SymbolId::kBuiltins,
                                SymbolId::kUnderNewMemberSetReadonly, name));
    return *setter;
  }

  Int offset(&scope, runtime->newInt(member.offset));
  switch (member.type) {
    case T_BOOL:
      return thread->invokeFunction1(SymbolId::kBuiltins,
                                     SymbolId::kUnderNewMemberSetBool, offset);
    case T_BYTE: {
      Int num_bytes(&scope, runtime->newInt(1));
      Int min_value(&scope, runtime->newInt(std::numeric_limits<char>::min()));
      Int max_value(&scope, runtime->newInt(std::numeric_limits<char>::max()));
      Str primitive_type(&scope, runtime->newStrFromCStr("char"));
      Function setter(
          &scope, thread->invokeFunction5(
                      SymbolId::kBuiltins, SymbolId::kUnderNewMemberSetIntegral,
                      offset, num_bytes, min_value, max_value, primitive_type));
      return *setter;
    }
    case T_UBYTE: {
      Int num_bytes(&scope, runtime->newInt(1));
      Int min_value(&scope, runtime->newInt(0));
      Int max_value(&scope,
                    runtime->newInt(std::numeric_limits<unsigned char>::max()));
      Str primitive_type(&scope, runtime->newStrFromCStr("unsigned char"));
      Function setter(
          &scope, thread->invokeFunction5(
                      SymbolId::kBuiltins, SymbolId::kUnderNewMemberSetIntegral,
                      offset, num_bytes, min_value, max_value, primitive_type));
      return *setter;
    }
    case T_SHORT: {
      Int num_bytes(&scope, runtime->newInt(2));
      Int min_value(&scope, runtime->newInt(std::numeric_limits<short>::min()));
      Int max_value(&scope, runtime->newInt(std::numeric_limits<short>::max()));
      Str primitive_type(&scope, runtime->newStrFromCStr("short"));
      Function setter(
          &scope, thread->invokeFunction5(
                      SymbolId::kBuiltins, SymbolId::kUnderNewMemberSetIntegral,
                      offset, num_bytes, min_value, max_value, primitive_type));
      return *setter;
    }
    case T_USHORT: {
      Int num_bytes(&scope, runtime->newInt(2));
      Int min_value(&scope, runtime->newInt(0));
      Int max_value(
          &scope, runtime->newInt(std::numeric_limits<unsigned short>::max()));
      Str primitive_type(&scope, runtime->newStrFromCStr("unsigned short"));
      Function setter(
          &scope, thread->invokeFunction5(
                      SymbolId::kBuiltins, SymbolId::kUnderNewMemberSetIntegral,
                      offset, num_bytes, min_value, max_value, primitive_type));
      return *setter;
    }
    case T_INT: {
      Int num_bytes(&scope, runtime->newInt(4));
      Int min_value(&scope, runtime->newInt(std::numeric_limits<int>::min()));
      Int max_value(&scope, runtime->newInt(std::numeric_limits<int>::max()));
      Str primitive_type(&scope, runtime->newStrFromCStr("int"));
      Function setter(
          &scope, thread->invokeFunction5(
                      SymbolId::kBuiltins, SymbolId::kUnderNewMemberSetIntegral,
                      offset, num_bytes, min_value, max_value, primitive_type));
      return *setter;
    }
    case T_UINT: {
      Int num_bytes(&scope, runtime->newInt(4));
      Int min_value(&scope, runtime->newInt(0));
      Int max_value(&scope,
                    runtime->newInt(std::numeric_limits<unsigned int>::max()));
      Str primitive_type(&scope, runtime->newStrFromCStr("unsigned int"));
      Function setter(
          &scope, thread->invokeFunction5(
                      SymbolId::kBuiltins, SymbolId::kUnderNewMemberSetIntegral,
                      offset, num_bytes, min_value, max_value, primitive_type));
      return *setter;
    }
    case T_LONG: {
      Int num_bytes(&scope, runtime->newInt(8));
      Int min_value(&scope, runtime->newInt(std::numeric_limits<long>::min()));
      Int max_value(&scope, runtime->newInt(std::numeric_limits<long>::max()));
      Str primitive_type(&scope, runtime->newStrFromCStr("long"));
      Function setter(
          &scope, thread->invokeFunction5(
                      SymbolId::kBuiltins, SymbolId::kUnderNewMemberSetIntegral,
                      offset, num_bytes, min_value, max_value, primitive_type));
      return *setter;
    }
    case T_ULONG: {
      Int num_bytes(&scope, runtime->newInt(8));
      Int min_value(&scope, runtime->newInt(0));
      Int max_value(&scope,
                    runtime->newInt(std::numeric_limits<unsigned long>::max()));
      Str primitive_type(&scope, runtime->newStrFromCStr("unsigned long"));
      Function setter(
          &scope, thread->invokeFunction5(
                      SymbolId::kBuiltins, SymbolId::kUnderNewMemberSetIntegral,
                      offset, num_bytes, min_value, max_value, primitive_type));
      return *setter;
    }
    case T_PYSSIZET: {
      Int num_bytes(&scope, runtime->newInt(8));
      Int min_value(&scope, runtime->newInt(0));
      Int max_value(&scope,
                    runtime->newInt(std::numeric_limits<Py_ssize_t>::max()));
      Str primitive_type(&scope, runtime->newStrFromCStr("Py_ssize_t"));
      Function setter(
          &scope, thread->invokeFunction5(
                      SymbolId::kBuiltins, SymbolId::kUnderNewMemberSetIntegral,
                      offset, num_bytes, min_value, max_value, primitive_type));
      return *setter;
    }
    case T_FLOAT: {
      return thread->invokeFunction1(SymbolId::kBuiltins,
                                     SymbolId::kUnderNewMemberSetFloat, offset);
    }
    case T_DOUBLE: {
      return thread->invokeFunction1(
          SymbolId::kBuiltins, SymbolId::kUnderNewMemberSetDouble, offset);
    }
    case T_STRING: {
      Object name(&scope, runtime->newStrFromCStr(member.name));
      Function setter(&scope,
                      thread->invokeFunction1(
                          SymbolId::kBuiltins,
                          SymbolId::kUnderNewMemberSetReadonlyStrings, name));
      return *setter;
    }
    case T_STRING_INPLACE: {
      Object name(&scope, runtime->newStrFromCStr(member.name));
      Function setter(&scope,
                      thread->invokeFunction1(
                          SymbolId::kBuiltins,
                          SymbolId::kUnderNewMemberSetReadonlyStrings, name));
      return *setter;
    }
    case T_CHAR: {
      Function setter(&scope, thread->invokeFunction1(
                                  SymbolId::kBuiltins,
                                  SymbolId::kUnderNewMemberSetChar, offset));
      return *setter;
    }
    case T_OBJECT: {
      Function setter(
          &scope, thread->invokeFunction1(SymbolId::kBuiltins,
                                          SymbolId::kUnderNewMemberSetPyObject,
                                          offset));
      return *setter;
    }
    case T_OBJECT_EX: {
      Function setter(
          &scope, thread->invokeFunction1(SymbolId::kBuiltins,
                                          SymbolId::kUnderNewMemberSetPyObject,
                                          offset));
      return *setter;
    }
    case T_LONGLONG: {
      Int num_bytes(&scope, runtime->newInt(8));
      Int min_value(&scope,
                    runtime->newInt(std::numeric_limits<long long>::min()));
      Int max_value(&scope,
                    runtime->newInt(std::numeric_limits<long long>::max()));
      Str primitive_type(&scope, runtime->newStrFromCStr("long long"));
      Function setter(
          &scope, thread->invokeFunction5(
                      SymbolId::kBuiltins, SymbolId::kUnderNewMemberSetIntegral,
                      offset, num_bytes, min_value, max_value, primitive_type));
      return *setter;
    }
    case T_ULONGLONG: {
      Int num_bytes(&scope, runtime->newInt(8));
      Int min_value(&scope, runtime->newInt(0));
      Int max_value(
          &scope,
          runtime->newInt(std::numeric_limits<unsigned long long>::max()));
      Str primitive_type(&scope, runtime->newStrFromCStr("unsigned long long"));
      Function setter(
          &scope, thread->invokeFunction5(
                      SymbolId::kBuiltins, SymbolId::kUnderNewMemberSetIntegral,
                      offset, num_bytes, min_value, max_value, primitive_type));
      return *setter;
    }
    default:
      return thread->raiseWithFmt(LayoutId::kSystemError,
                                  "bad member name type");
  }
}

static RawObject getterWrapper(Thread* thread, Frame* frame, word argc) {
  auto func = getNativeFunc<getter>(thread, frame);
  Arguments args(frame, argc);
  PyObject* self = ApiHandle::borrowedReference(thread, args.get(0));
  PyObject* result = (*func)(self, nullptr);
  return ApiHandle::checkFunctionResult(thread, result);
}

static RawObject setterWrapper(Thread* thread, Frame* frame, word argc) {
  auto func = getNativeFunc<setter>(thread, frame);
  Arguments args(frame, argc);
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

RawObject addMembers(Thread* thread, const Type& type) {
  HandleScope scope(thread);
  Object slot_value(&scope, extensionSlot(type, ExtensionSlot::kMembers));
  if (slot_value.isNoneType()) return NoneType::object();
  DCHECK(slot_value.isInt(), "unexpected slot type");
  auto members = bit_cast<PyMemberDef*>(Int::cast(*slot_value).asCPtr());
  Dict dict(&scope, type.dict());
  Object none(&scope, NoneType::object());
  Runtime* runtime = thread->runtime();
  for (word i = 0; members[i].name != nullptr; i++) {
    Object name(&scope, runtime->newStrFromCStr(members[i].name));
    Object getter(&scope, memberGetter(thread, members[i]));
    if (getter.isError()) return *getter;
    Object setter(&scope, memberSetter(thread, members[i]));
    if (setter.isError()) return *setter;
    Object property(&scope, runtime->newProperty(getter, setter, none));
    runtime->typeDictAtPut(thread, dict, name, property);
  }
  return NoneType::object();
}

RawObject addGetSet(Thread* thread, const Type& type) {
  HandleScope scope(thread);
  Object slot_value(&scope, extensionSlot(type, ExtensionSlot::kGetset));
  if (slot_value.isNoneType()) return NoneType::object();
  DCHECK(slot_value.isInt(), "unexpected slot type");
  auto getsets = bit_cast<PyGetSetDef*>(Int::cast(*slot_value).asCPtr());
  Dict dict(&scope, type.dict());
  Object none(&scope, NoneType::object());
  Runtime* runtime = thread->runtime();
  for (word i = 0; getsets[i].name != nullptr; i++) {
    Object name(&scope, runtime->newStrFromCStr(getsets[i].name));
    Object getter(&scope, getSetGetter(thread, name, getsets[i]));
    if (getter.isError()) return *getter;
    Object setter(&scope, getSetSetter(thread, name, getsets[i]));
    if (setter.isError()) return *setter;
    Object property(&scope, runtime->newProperty(getter, setter, none));
    runtime->typeDictAtPut(thread, dict, name, property);
  }
  return NoneType::object();
}

static bool hasSlot(const Type& type, ExtensionSlot slot) {
  return !extensionSlot(type, slot).isNoneType();
}

static void* baseBaseSlot(const Type& base, ExtensionSlot slot) {
  if (!hasSlot(base, ExtensionSlot::kBase)) return nullptr;
  HandleScope scope(Thread::current());
  Int basebase_handle(&scope, extensionSlot(base, ExtensionSlot::kBase));
  Type basebase(
      &scope,
      reinterpret_cast<ApiHandle*>(basebase_handle.asCPtr())->asObject());
  if (basebase.extensionSlots().isNoneType() || !hasSlot(basebase, slot)) {
    return nullptr;
  }
  Int basebase_slot(&scope, extensionSlot(basebase, slot));
  return basebase_slot.asCPtr();
}

// Copy the slot from the base type if defined and it is the first type that
// defines it. If base's base type defines the same slot, then base inherited
// it. Thus, it is not the first type to define it.
static void copySlotIfImplementedInBase(const Type& type, const Type& base,
                                        int slot) {
  ExtensionSlot field_id = slotToTypeSlot(slot);
  if (!hasSlot(type, field_id) && hasSlot(base, field_id)) {
    RawObject base_slot = extensionSlot(base, field_id);
    void* basebase_slot = baseBaseSlot(base, field_id);
    if (basebase_slot == nullptr ||
        Int::cast(base_slot).asCPtr() != basebase_slot) {
      setExtensionSlot(type, field_id, base_slot);
    }
  }
}

// Copy the slot from the base type if it defined.
static void copySlot(const Type& type, const Type& base, int slot) {
  ExtensionSlot field_id = slotToTypeSlot(slot);
  if (!hasSlot(type, field_id) && hasSlot(base, field_id)) {
    setExtensionSlot(type, field_id, extensionSlot(base, field_id));
  }
}

static void inheritGCFlagsAndSlots(Thread* thread, const Type& type,
                                   const Type& base) {
  unsigned long type_flags =
      Int::cast(extensionSlot(type, ExtensionSlot::kFlags)).asWord();
  unsigned long base_flags =
      Int::cast(extensionSlot(base, ExtensionSlot::kFlags)).asWord();
  if (!(type_flags & Py_TPFLAGS_HAVE_GC) && (base_flags & Py_TPFLAGS_HAVE_GC) &&
      !hasSlot(type, ExtensionSlot::kTraverse) &&
      !hasSlot(type, ExtensionSlot::kClear)) {
    setExtensionSlot(
        type, ExtensionSlot::kFlags,
        thread->runtime()->newInt(type_flags | Py_TPFLAGS_HAVE_GC));
    if (!hasSlot(type, ExtensionSlot::kTraverse)) {
      copySlot(type, base, Py_tp_traverse);
    }
    if (!hasSlot(type, ExtensionSlot::kClear)) {
      copySlot(type, base, Py_tp_clear);
    }
  }
}

static void inheritNonFunctionSlots(const Type& type, const Type& base) {
  if (Int::cast(extensionSlot(base, ExtensionSlot::kBasicSize)).asWord() == 0) {
    setExtensionSlot(type, ExtensionSlot::kBasicSize,
                     extensionSlot(base, ExtensionSlot::kBasicSize));
  }
  setExtensionSlot(type, ExtensionSlot::kItemSize,
                   extensionSlot(base, ExtensionSlot::kItemSize));
}

// clang-format off
static const int kInheritableSlots[] = {
  // Number slots
  Py_nb_add,
  Py_nb_subtract,
  Py_nb_multiply,
  Py_nb_remainder,
  Py_nb_divmod,
  Py_nb_power,
  Py_nb_negative,
  Py_nb_positive,
  Py_nb_absolute,
  Py_nb_bool,
  Py_nb_invert,
  Py_nb_lshift,
  Py_nb_rshift,
  Py_nb_and,
  Py_nb_xor,
  Py_nb_or,
  Py_nb_int,
  Py_nb_float,
  Py_nb_inplace_add,
  Py_nb_inplace_subtract,
  Py_nb_inplace_multiply,
  Py_nb_inplace_remainder,
  Py_nb_inplace_power,
  Py_nb_inplace_lshift,
  Py_nb_inplace_rshift,
  Py_nb_inplace_and,
  Py_nb_inplace_xor,
  Py_nb_inplace_or,
  Py_nb_true_divide,
  Py_nb_floor_divide,
  Py_nb_inplace_true_divide,
  Py_nb_inplace_floor_divide,
  Py_nb_index,
  Py_nb_matrix_multiply,
  Py_nb_inplace_matrix_multiply,

  // Await slots
  Py_am_await,
  Py_am_aiter,
  Py_am_anext,

  // Sequence slots
  Py_sq_length,
  Py_sq_concat,
  Py_sq_repeat,
  Py_sq_item,
  Py_sq_ass_item,
  Py_sq_contains,
  Py_sq_inplace_concat,
  Py_sq_inplace_repeat,

  // Mapping slots
  Py_mp_length,
  Py_mp_subscript,
  Py_mp_ass_subscript,

  // Buffer protocol is not part of PEP-384

  // Type slots
  Py_tp_dealloc,
  Py_tp_repr,
  Py_tp_call,
  Py_tp_str,
  Py_tp_iter,
  Py_tp_iternext,
  Py_tp_descr_get,
  Py_tp_descr_set,
  Py_tp_init,
  Py_tp_alloc,
  Py_tp_is_gc,

  // Instance dictionary is not part of PEP-384

  // Weak reference support is not part of PEP-384
};
// clang-format on

static void inheritFinalize(const Type& type, unsigned long type_flags,
                            const Type& base, unsigned long base_flags) {
  if ((type_flags & Py_TPFLAGS_HAVE_FINALIZE) &&
      (base_flags & Py_TPFLAGS_HAVE_FINALIZE)) {
    copySlotIfImplementedInBase(type, base, Py_tp_finalize);
  }
  if ((type_flags & Py_TPFLAGS_HAVE_FINALIZE) &&
      (base_flags & Py_TPFLAGS_HAVE_FINALIZE)) {
    copySlotIfImplementedInBase(type, base, Py_tp_finalize);
  }
}

static void inheritFree(const Type& type, unsigned long type_flags,
                        const Type& base, unsigned long base_flags) {
  // Both child and base are GC or non GC
  if ((type_flags & Py_TPFLAGS_HAVE_GC) == (base_flags & Py_TPFLAGS_HAVE_GC)) {
    copySlotIfImplementedInBase(type, base, Py_tp_free);
    return;
  }

  DCHECK(!(base_flags & Py_TPFLAGS_HAVE_GC), "The child should not remove GC");

  // Only the child is GC
  // Set the free function if the base has a default free
  if ((type_flags & Py_TPFLAGS_HAVE_GC) &&
      !hasSlot(type, ExtensionSlot::kFree) &&
      hasSlot(base, ExtensionSlot::kFree)) {
    void* free_slot = reinterpret_cast<void*>(
        Int::cast(extensionSlot(base, ExtensionSlot::kFree)).asWord());
    if (free_slot == reinterpret_cast<void*>(PyObject_Free)) {
      setExtensionSlot(type, ExtensionSlot::kFree,
                       Thread::current()->runtime()->newIntFromCPtr(
                           reinterpret_cast<void*>(PyObject_GC_Del)));
    }
  }
}

static void inheritSlots(const Type& type, const Type& base) {
  // Heap allocated types are guaranteed to have slot space, no check is needed
  // i.e. CPython does: `if (type->tp_as_number != NULL)`
  // Only static types need to do this type of check.
  for (const int slot : kInheritableSlots) {
    copySlotIfImplementedInBase(type, base, slot);
  }

  // Inherit conditional type slots
  if (!hasSlot(type, ExtensionSlot::kGetattr) &&
      !hasSlot(type, ExtensionSlot::kGetattro)) {
    copySlot(type, base, Py_tp_getattr);
    copySlot(type, base, Py_tp_getattro);
  }
  if (!hasSlot(type, ExtensionSlot::kSetattr) &&
      !hasSlot(type, ExtensionSlot::kSetattro)) {
    copySlot(type, base, Py_tp_setattr);
    copySlot(type, base, Py_tp_setattro);
  }
  if (!hasSlot(type, ExtensionSlot::kRichcompare) &&
      !hasSlot(type, ExtensionSlot::kHash)) {
    copySlot(type, base, Py_tp_richcompare);
    copySlot(type, base, Py_tp_hash);
  }

  unsigned long type_flags =
      Int::cast(extensionSlot(type, ExtensionSlot::kFlags)).asWord();
  unsigned long base_flags =
      Int::cast(extensionSlot(base, ExtensionSlot::kFlags)).asWord();
  inheritFinalize(type, type_flags, base, base_flags);
  inheritFree(type, type_flags, base, base_flags);
}

static int isBuiltinType(PyTypeObject* type) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Type managed_type(
      &scope,
      ApiHandle::fromPyObject(reinterpret_cast<PyObject*>(type))->asObject());
  return managed_type.isBuiltin();
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
    if (Py_REFCNT(self) > 0) {
      return;  // Resurrected
    }
  }

  // Find the nearest base with a different tp_dealloc
  PyTypeObject* base_type = type;
  destructor base_dealloc = subtypeDealloc;
  while (base_dealloc == subtypeDealloc) {
    base_type =
        reinterpret_cast<PyTypeObject*>(PyType_GetSlot(base_type, Py_tp_base));
    if (isBuiltinType(base_type)) {
      base_dealloc = reinterpret_cast<destructor>(PyObject_Del);
    } else {
      base_dealloc = reinterpret_cast<destructor>(
          PyType_GetSlot(base_type, Py_tp_dealloc));
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

static RawObject addDefaultsForRequiredSlots(Thread* thread, const Type& type) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Dict dict(&scope, type.dict());
  Str type_name(&scope, type.name());

  // tp_basicsize -> sizeof(PyObject)
  DCHECK(hasSlot(type, ExtensionSlot::kBasicSize),
         "Basic size must always be present");
  Int basic_size(&scope, extensionSlot(type, ExtensionSlot::kBasicSize));
  if (basic_size.asWord() == 0) {
    basic_size = runtime->newInt(sizeof(PyObject));
    setExtensionSlot(type, ExtensionSlot::kBasicSize, *basic_size);
  }
  DCHECK(basic_size.asWord() >= static_cast<word>(sizeof(PyObject)),
         "sizeof(PyObject) is the minimum size required for an extension "
         "instance");

  // tp_new -> PyType_GenericNew
  if (!hasSlot(type, ExtensionSlot::kNew)) {
    Object default_new(
        &scope, runtime->newIntFromCPtr(bit_cast<void*>(&PyType_GenericNew)));
    setExtensionSlot(type, ExtensionSlot::kNew, *default_new);
    Str dunder_new_name(&scope, runtime->symbols()->at(SymbolId::kDunderNew));
    Str qualname(&scope,
                 runtime->newStrFromFmt("%S.%S", &type_name, &dunder_new_name));
    Code code(&scope,
              newExtCode(thread, dunder_new_name, kParamsTypeArgsKwargs,
                         ARRAYSIZE(kParamsTypeArgsKwargs),
                         Code::Flags::VARARGS | Code::Flags::VARKEYARGS,
                         bit_cast<void*>(&wrapVarkwTernaryfunc), default_new));
    Object globals(&scope, NoneType::object());
    Function func(
        &scope, runtime->newFunctionWithCode(thread, qualname, code, globals));
    Object func_obj(&scope,
                    thread->invokeFunction1(SymbolId::kBuiltins,
                                            SymbolId::kStaticMethod, func));
    if (func_obj.isError()) return *func;
    runtime->typeDictAtPut(thread, dict, dunder_new_name, func_obj);
  }

  // tp_alloc -> PyType_GenericAlloc
  if (!hasSlot(type, ExtensionSlot::kAlloc)) {
    Object default_alloc(
        &scope, runtime->newIntFromCPtr(bit_cast<void*>(&PyType_GenericAlloc)));
    setExtensionSlot(type, ExtensionSlot::kAlloc, *default_alloc);
  }

  // tp_dealloc -> subtypeDealloc
  if (!hasSlot(type, ExtensionSlot::kDealloc)) {
    Object default_dealloc(
        &scope, runtime->newIntFromCPtr(bit_cast<void*>(&subtypeDealloc)));
    setExtensionSlot(type, ExtensionSlot::kDealloc, *default_dealloc);
  }

  return NoneType::object();
}

PY_EXPORT PyObject* PyType_FromSpecWithBases(PyType_Spec* spec,
                                             PyObject* bases) {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  // Create a new type for the PyTypeObject
  Type type(&scope, runtime->newType());
  Dict dict(&scope, runtime->newDict());
  type.setDict(*dict);

  // Set the class name
  const char* class_name = strrchr(spec->name, '.');
  if (class_name == nullptr) {
    class_name = spec->name;
  } else {
    class_name++;
  }
  Object name_obj(&scope, runtime->newStrFromCStr(class_name));
  type.setName(*name_obj);
  Object dict_key(&scope, runtime->symbols()->DunderName());
  runtime->dictAtPutInValueCell(thread, dict, dict_key, name_obj);

  // Initialize the extension slots tuple
  Object extension_slots(
      &scope, runtime->newTuple(static_cast<int>(ExtensionSlot::kEnd)));
  type.setExtensionSlots(*extension_slots);

  // Set bases
  Tuple parents(&scope, runtime->emptyTuple());
  if (bases != nullptr) {
    parents = ApiHandle::fromPyObject(bases)->asObject();
  }
  PyObject* parents_handle = ApiHandle::borrowedReference(thread, *parents);
  setExtensionSlot(type, ExtensionSlot::kBases,
                   runtime->newIntFromCPtr(parents_handle));

  // Compute MRO
  Tuple mro(&scope, computeMro(thread, type, parents));
  type.setMro(*mro);
  Type base_type(&scope, mro.at(1));
  PyObject* base_handle = ApiHandle::borrowedReference(thread, *base_type);
  setExtensionSlot(type, ExtensionSlot::kBase,
                   runtime->newIntFromCPtr(base_handle));

  // Initialize instance Layout
  Layout layout(&scope,
                runtime->computeInitialLayout(thread, type, LayoutId::kObject));
  layout.setDescribedType(*type);
  type.setInstanceLayout(*layout);

  // Set the type slots
  for (PyType_Slot* slot = spec->slots; slot->slot; slot++) {
    void* slot_ptr = slot->pfunc;
    Object field(&scope, runtime->newIntFromCPtr(slot_ptr));
    ExtensionSlot field_id = slotToTypeSlot(slot->slot);
    if (field_id >= ExtensionSlot::kEnd) {
      thread->raiseWithFmt(LayoutId::kRuntimeError, "invalid slot offset");
      return nullptr;
    }
    setExtensionSlot(type, field_id, *field);
  }

  // Set size
  Object basic_size(&scope, runtime->newInt(spec->basicsize));
  Object item_size(&scope, runtime->newInt(spec->itemsize));
  setExtensionSlot(type, ExtensionSlot::kBasicSize, *basic_size);
  setExtensionSlot(type, ExtensionSlot::kItemSize, *item_size);

  // Set the class flags
  Object flags(&scope, runtime->newInt(spec->flags | Py_TPFLAGS_READY |
                                       Py_TPFLAGS_HEAPTYPE));
  setExtensionSlot(type, ExtensionSlot::kFlags, *flags);

  if (addOperators(thread, type).isError()) return nullptr;

  Object methods_ptr(&scope, extensionSlot(type, ExtensionSlot::kMethods));
  if (!methods_ptr.isNoneType()) {
    PyMethodDef* methods =
        reinterpret_cast<PyMethodDef*>(Int::cast(*methods_ptr).asCPtr());
    for (word i = 0; methods[i].ml_name != nullptr; i++) {
      Object name(&scope, runtime->newStrFromCStr(methods[i].ml_name));
      Object function(
          &scope, functionFromMethodDef(
                      thread, methods[i].ml_name,
                      bit_cast<void*>(methods[i].ml_meth), methods[i].ml_doc,
                      methodTypeFromMethodFlags(methods[i].ml_flags)));
      if (function.isError()) return nullptr;
      runtime->dictAtPutInValueCell(thread, dict, name, function);
    }
  }

  if (addMembers(thread, type).isError()) return nullptr;

  if (addGetSet(thread, type).isError()) return nullptr;

  // Inherit special slots from dominant base
  if (!base_type.extensionSlots().isNoneType()) {
    inheritGCFlagsAndSlots(thread, type, base_type);
    // !PyBaseObject_Type and Py_TPFLAGS_HEAPTYPE are guaranteed so skip check
    if (!hasSlot(type, ExtensionSlot::kNew)) {
      copySlot(type, base_type, Py_tp_new);
    }
    inheritNonFunctionSlots(type, base_type);
  }

  for (word i = 1; i < mro.length(); i++) {
    Type base(&scope, mro.at(i));
    // Skip inheritance if base does not define extensionSlots
    if (base.extensionSlots().isNoneType()) continue;
    // Bases must define Py_TPFLAGS_BASETYPE
    word base_flags =
        Int::cast(extensionSlot(base, ExtensionSlot::kFlags)).asWord();
    if ((base_flags & Py_TPFLAGS_BASETYPE) == 0) {
      thread->raiseWithFmt(LayoutId::kTypeError,
                           "type is not an acceptable base type");
      return nullptr;
    }
    inheritSlots(type, base);
  }

  // Finally, inherit all the default slots that would have been inherited
  // through PyBaseObject_Type in CPython
  if (addDefaultsForRequiredSlots(thread, type).isError()) return nullptr;

  return ApiHandle::newReference(thread, *type);
}

PY_EXPORT PyObject* PyType_GenericAlloc(PyTypeObject* type_obj,
                                        Py_ssize_t nitems) {
  DCHECK(ApiHandle::isManaged(reinterpret_cast<PyObject*>(type_obj)),
         "Type is unmanaged. Please initialize using PyType_FromSpec");

  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Type type(&scope,
            ApiHandle::fromPyObject(reinterpret_cast<PyObject*>(type_obj))
                ->asObject());
  DCHECK(!type.isBuiltin(),
         "Type is unmanaged. Please initialize using PyType_FromSpec");
  DCHECK(!type.extensionSlots().isNoneType(),
         "GenericAlloc from types initialized through Python code");

  // Allocate a new instance
  Int basic_size(&scope, extensionSlot(type, ExtensionSlot::kBasicSize));
  Int item_size(&scope, extensionSlot(type, ExtensionSlot::kItemSize));
  Py_ssize_t size = Utils::roundUp(
      nitems * item_size.asWord() + basic_size.asWord(), kWordSize);
  PyObject* result = static_cast<PyObject*>(PyObject_Calloc(1, size));
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
  return result;
}

PY_EXPORT Py_ssize_t _PyObject_SIZE_Func(PyObject* obj) {
  HandleScope scope;
  Type type(&scope, ApiHandle::fromPyObject(obj)->asObject());
  Int basic_size(&scope, extensionSlot(type, ExtensionSlot::kBasicSize));
  return basic_size.asWord();
}

PY_EXPORT Py_ssize_t _PyObject_VAR_SIZE_Func(PyObject* obj, Py_ssize_t nitems) {
  HandleScope scope;
  Type type(&scope, ApiHandle::fromPyObject(obj)->asObject());
  Int basic_size(&scope, extensionSlot(type, ExtensionSlot::kBasicSize));
  Int item_size(&scope, extensionSlot(type, ExtensionSlot::kItemSize));
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

PY_EXPORT int PyType_IsSubtype(PyTypeObject* a, PyTypeObject* b) {
  if (a == b) return 1;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Type a_obj(
      &scope,
      ApiHandle::fromPyObject(reinterpret_cast<PyObject*>(a))->asObject());
  Type b_obj(
      &scope,
      ApiHandle::fromPyObject(reinterpret_cast<PyObject*>(b))->asObject());
  return thread->runtime()->isSubclass(a_obj, b_obj) ? 1 : 0;
}

PY_EXPORT void PyType_Modified(PyTypeObject* /* e */) {
  UNIMPLEMENTED("PyType_Modified");
}

PY_EXPORT PyObject* _PyObject_LookupSpecial(PyObject* /* f */,
                                            _Py_Identifier* /* d */) {
  UNIMPLEMENTED("_PyObject_LookupSpecial");
}

PY_EXPORT const char* _PyType_Name(PyTypeObject* type) {
  Thread* thread = Thread::current();
  if (type == nullptr) {
    thread->raiseBadInternalCall();
    return nullptr;
  }
  HandleScope scope(thread);
  Object obj(
      &scope,
      ApiHandle::fromPyObject(reinterpret_cast<PyObject*>(type))->asObject());
  if (!thread->runtime()->isInstanceOfType(*obj)) {
    thread->raiseBadInternalCall();
    return nullptr;
  }
  Type type_obj(&scope, *obj);
  return PyUnicode_AsUTF8(
      ApiHandle::borrowedReference(thread, type_obj.name()));
}

PY_EXPORT PyObject* _PyType_Lookup(PyTypeObject* /* type */,
                                   PyObject* /* name */) {
  UNIMPLEMENTED("_PyType_Lookup");
}

}  // namespace python
