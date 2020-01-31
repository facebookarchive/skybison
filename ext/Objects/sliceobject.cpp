#include "cpython-func.h"

#include "capi-handles.h"
#include "runtime.h"
#include "slice-builtins.h"

namespace py {

PY_EXPORT int PySlice_Check_Func(PyObject* pyobj) {
  return ApiHandle::fromPyObject(pyobj)->asObject().isSlice();
}

PY_EXPORT PyObject* PySlice_New(PyObject* start, PyObject* stop,
                                PyObject* step) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object start_obj(&scope, NoneType::object());
  Object stop_obj(&scope, NoneType::object());
  Object step_obj(&scope, NoneType::object());
  if (start != nullptr) {
    start_obj = ApiHandle::fromPyObject(start)->asObject();
  }
  if (stop != nullptr) {
    stop_obj = ApiHandle::fromPyObject(stop)->asObject();
  }
  if (step != nullptr) {
    step_obj = ApiHandle::fromPyObject(step)->asObject();
  }
  return ApiHandle::newReference(
      thread, thread->runtime()->newSlice(start_obj, stop_obj, step_obj));
}

PY_EXPORT Py_ssize_t PySlice_AdjustIndices(Py_ssize_t length,
                                           Py_ssize_t* start_ptr,
                                           Py_ssize_t* stop_ptr,
                                           Py_ssize_t step) {
  DCHECK(step != 0, "step cannot be 0");
  DCHECK(step >= -SmallInt::kMaxValue, "step must allow for safe reversal");
  DCHECK(length >= 0, "length cannot be negative");
  word start = static_cast<word>(*start_ptr);
  word stop = static_cast<word>(*stop_ptr);
  word slice_length = RawSlice::adjustIndices(static_cast<word>(length), &start,
                                              &stop, static_cast<word>(step));
  *start_ptr = static_cast<Py_ssize_t>(start);
  *stop_ptr = static_cast<Py_ssize_t>(stop);
  return static_cast<Py_ssize_t>(slice_length);
}

PY_EXPORT int PySlice_GetIndices(PyObject*, Py_ssize_t, Py_ssize_t*,
                                 Py_ssize_t*, Py_ssize_t*) {
  UNIMPLEMENTED("PySlice_GetIndices is not supported");
}

PY_EXPORT int PySlice_GetIndicesEx(PyObject* slice, Py_ssize_t length,
                                   Py_ssize_t* start, Py_ssize_t* stop,
                                   Py_ssize_t* step, Py_ssize_t* slicelength) {
  if (PySlice_Unpack(slice, start, stop, step) < 0) {
    return -1;
  }
  *slicelength = PySlice_AdjustIndices(length, start, stop, *step);
  return 0;
}

PY_EXPORT int PySlice_Unpack(PyObject* pyobj, Py_ssize_t* start_ptr,
                             Py_ssize_t* stop_ptr, Py_ssize_t* step_ptr) {
  DCHECK(SmallInt::kMinValue + 1 == -SmallInt::kMaxValue,
         "expected two's complement SmallInt (got min %ld, max %ld)",
         SmallInt::kMinValue, SmallInt::kMaxValue);
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object obj(&scope, ApiHandle::fromPyObject(pyobj)->asObject());
  if (!obj.isSlice()) {
    thread->raiseBadInternalCall();
    return -1;
  }

  Slice slice(&scope, *obj);
  Runtime* runtime = thread->runtime();

  word step = 1;
  Object step_obj(&scope, slice.step());
  if (!step_obj.isNoneType()) {
    if (!runtime->isInstanceOfInt(*step_obj)) {
      step_obj =
          thread->invokeFunction1(ID(builtins), ID(_slice_index), step_obj);
      if (step_obj.isError()) return -1;
    }
    step = intUnderlying(*step_obj).asWordSaturated();
    if (step == 0) {
      thread->raiseWithFmt(LayoutId::kValueError, "slice step cannot be zero");
      return -1;
    }
    if (step <= SmallInt::kMinValue) {
      // Here *step might be -SmallInt::kMaxValue - 1. In that case, we replace
      // it with -SmallInt::kMaxValue. This doesn't affect the semantics, and it
      // guards against later undefined behaviour resulting from code that does
      // "step = -step" as part of a slice reversal.
      step = -SmallInt::kMaxValue;
    } else if (step > SmallInt::kMaxValue) {
      step = SmallInt::kMaxValue;
    }
  }

  word start;
  Object start_obj(&scope, slice.start());
  if (start_obj.isNoneType()) {
    start = step < 0 ? SmallInt::kMaxValue : 0;
  } else {
    if (!runtime->isInstanceOfInt(*start_obj)) {
      start_obj =
          thread->invokeFunction1(ID(builtins), ID(_slice_index), start_obj);
      if (start_obj.isError()) return -1;
    }
    start = intUnderlying(*start_obj).asWordSaturated();
    if (start < SmallInt::kMinValue) {
      start = SmallInt::kMinValue;
    } else if (start > SmallInt::kMaxValue) {
      start = SmallInt::kMaxValue;
    }
  }

  word stop;
  Object stop_obj(&scope, slice.stop());
  if (stop_obj.isNoneType()) {
    stop = step < 0 ? SmallInt::kMinValue : SmallInt::kMaxValue;
  } else {
    if (!runtime->isInstanceOfInt(*stop_obj)) {
      stop_obj =
          thread->invokeFunction1(ID(builtins), ID(_slice_index), stop_obj);
      if (stop_obj.isError()) return -1;
    }
    stop = intUnderlying(*stop_obj).asWordSaturated();
    if (stop < SmallInt::kMinValue) {
      stop = SmallInt::kMinValue;
    } else if (stop > SmallInt::kMaxValue) {
      stop = SmallInt::kMaxValue;
    }
  }

  *start_ptr = static_cast<Py_ssize_t>(start);
  *stop_ptr = static_cast<Py_ssize_t>(stop);
  *step_ptr = static_cast<Py_ssize_t>(step);
  return 0;
}

}  // namespace py
