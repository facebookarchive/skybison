// floatobject.c implementation

#include <cfloat>
#include <cmath>

#include "cpython-data.h"
#include "cpython-func.h"

#include "api-handle.h"
#include "bytearrayobject-utils.h"
#include "bytesobject-utils.h"
#include "float-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "typeslots.h"

namespace py {
PY_EXPORT PyObject* PyFloat_FromDouble(double fval) {
  Runtime* runtime = Thread::current()->runtime();
  return ApiHandle::newReference(runtime, runtime->newFloat(fval));
}

PY_EXPORT double PyFloat_AsDouble(PyObject* op) {
  Thread* thread = Thread::current();
  if (op == nullptr) {
    thread->raiseBadArgument();
    return -1;
  }

  // Object is float
  HandleScope scope(thread);
  Object obj(&scope, ApiHandle::fromPyObject(op)->asObject());
  if (!thread->runtime()->isInstanceOfFloat(*obj)) {
    obj = thread->invokeFunction1(ID(builtins), ID(_float), obj);
    if (obj.isError()) return -1;
  }
  return floatUnderlying(*obj).value();
}

PY_EXPORT int PyFloat_CheckExact_Func(PyObject* obj) {
  return ApiHandle::fromPyObject(obj)->asObject().isFloat();
}

PY_EXPORT int PyFloat_Check_Func(PyObject* obj) {
  return Thread::current()->runtime()->isInstanceOfFloat(
      ApiHandle::fromPyObject(obj)->asObject());
}

PY_EXPORT PyObject* PyFloat_FromString(PyObject* obj) {
  Thread* thread = Thread::current();

  DCHECK(obj != nullptr,
         "null argument to internal routine PyFloat_FromString");

  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  ApiHandle* handle = ApiHandle::fromPyObject(obj);
  Object object(&scope, handle->asObject());

  // First, handle all string like items here
  if (runtime->isInstanceOfStr(*object) ||
      runtime->isInstanceOfBytes(*object) ||
      runtime->isInstanceOfBytearray(*object)) {
    object = thread->invokeFunction1(ID(builtins), ID(float), object);
    return object.isError() ? nullptr
                            : ApiHandle::newReference(runtime, *object);
  }

  if (object.isMemoryView()) {
    // Memoryviews are buffer like, but can be converted to bytes and then to a
    // float
    MemoryView memoryview(&scope, *object);
    Object buffer(&scope, memoryview.buffer());
    // Buffer is either a bytes object or a raw pointer
    if (runtime->isInstanceOfBytes(*buffer)) {
      Bytes bytes(&scope, bytesUnderlying(*object));
      object = thread->invokeFunction1(ID(builtins), ID(float), bytes);
      return object.isError() ? nullptr
                              : ApiHandle::newReference(runtime, *object);
    }
    Pointer underlying_pointer(&scope, *buffer);
    word length = memoryview.length();
    unique_c_ptr<char> copy(::strndup(
        reinterpret_cast<const char*>(underlying_pointer.cptr()), length));

    object = floatFromDigits(thread, copy.get(), length);
    return object.isError() ? nullptr
                            : ApiHandle::newReference(runtime, *object);
  }
  // Maybe it otherwise supports the buffer protocol
  Object bytes(&scope, newBytesFromBuffer(thread, object));
  if (!bytes.isError()) {
    object = thread->invokeFunction1(ID(builtins), ID(float), bytes);
    return object.isError() ? nullptr
                            : ApiHandle::newReference(runtime, *object);
  }

  thread->clearPendingException();
  thread->raiseWithFmt(
      LayoutId::kTypeError,
      "float() argument must be a string or a number, not '%T'", &object);
  return nullptr;
}

PY_EXPORT PyObject* PyFloat_GetInfo() { UNIMPLEMENTED("PyFloat_GetInfo"); }

PY_EXPORT double PyFloat_GetMax() { return DBL_MAX; }

PY_EXPORT double PyFloat_GetMin() { return DBL_MIN; }

PY_EXPORT PyTypeObject* PyFloat_Type_Ptr() {
  Runtime* runtime = Thread::current()->runtime();
  return reinterpret_cast<PyTypeObject*>(
      ApiHandle::borrowedReference(runtime, runtime->typeAt(LayoutId::kFloat)));
}

// _PyFloat_{Pack,Unpack}{2,4,8}.  See floatobject.h.
// To match the NPY_HALF_ROUND_TIES_TO_EVEN behavior in:
// https://github.com/numpy/numpy/blob/master/numpy/core/src/npymath/halffloat.c
// We use:
//       bits = (unsigned short)f;    Note the truncation
//       if ((f - bits > 0.5) || (f - bits == 0.5 && bits % 2)) {
//           bits++;
//       }
PY_EXPORT int _PyFloat_Pack2(double x, unsigned char* p, int little_endian) {
  int e;
  unsigned short bits;
  bool sign = std::signbit(x);
  if (x == 0.0) {
    e = 0;
    bits = 0;
  } else if (!std::isfinite(x)) {
    e = 0x1f;
    if (std::isinf(x)) {
      bits = 0;
    } else {
      DCHECK(std::isnan(x), "remaining case must be NaN");
      // There are 2046 distinct half-precision NaNs (1022 signaling and
      // 1024 quiet), but there are only two quiet NaNs that don't arise by
      // quieting a signaling NaN; we get those by setting the topmost bit
      // of the fraction field and clearing all other fraction bits. We
      // choose the one with the appropriate sign.
      bits = 512;
    }
  } else {
    if (sign) {
      x = -x;
    }

    double f = std::frexp(x, &e);
    if (f < 0.5 || f >= 1.0) {
      Thread::current()->raiseWithFmt(LayoutId::kSystemError,
                                      "std::frexp() result out of range");
      return -1;
    }

    // Normalize f to be in the range [1.0, 2.0)
    f *= 2.0;
    e--;

    if (e >= 16) {
      Thread::current()->raiseWithFmt(LayoutId::kOverflowError,
                                      "float too large to pack with e format");
      return -1;
    }

    if (e < -25) {
      // |x| < 2**-25. Underflow to zero.
      f = 0.0;
      e = 0;
    } else if (e < -14) {
      // |x| < 2**-14. Gradual underflow
      f = std::ldexp(f, 14 + e);
      e = 0;
    } else {  // if (!(e == 0 && f == 0.0))
      e += 15;
      f -= 1.0;  // Get rid of leading 1
    }

    f *= 1024.0;  // 2**10
    // Round to even
    bits = static_cast<unsigned short>(f);  // Note the truncation
    CHECK(bits < 1024, "Expected bits < 1024");
    CHECK(e < 31, "Expected e < 31");
    if ((f - bits > 0.5) || ((f - bits == 0.5) && (bits % 2 == 1))) {
      ++bits;
      if (bits == 1024) {
        // The carry propagated out of a string of 10 1 bits.
        bits = 0;
        ++e;
        if (e == 31) {
          Thread::current()->raiseWithFmt(
              LayoutId::kOverflowError,
              "float too large to pack with e format");
          return -1;
        }
      }
    }
  }

  bits |= (e << 10) | (sign << 15);

  // Write out result.
  int incr = 1;
  if (little_endian == (endian::native == endian::little)) {
    p += 1;
    incr = -1;
  }

  // First byte
  *p = static_cast<unsigned char>((bits >> 8) & 0xFF);
  p += incr;

  /* Second byte */
  *p = static_cast<unsigned char>(bits & 0xFF);
  return 0;
}

PY_EXPORT int _PyFloat_Pack4(double x, unsigned char* p, int little_endian) {
  // Assumes float format is ieee_little_endian_format
  float y = static_cast<float>(x);
  if (std::isinf(y) && !std::isinf(x)) {
    Thread::current()->raiseWithFmt(LayoutId::kOverflowError,
                                    "float too large to pack with f format");
    return -1;
  }

  if (little_endian == (endian::native == endian::little)) {
    std::memcpy(p, &y, sizeof(y));
  } else {
    int32_t as_int = bit_cast<uint32_t>(y);
    int32_t swapped = __builtin_bswap32(as_int);
    std::memcpy(p, &swapped, sizeof(swapped));
  }

  return 0;
}

PY_EXPORT int _PyFloat_Pack8(double x, unsigned char* p, int little_endian) {
  // Assumes double format is ieee_little_endian_format
  if (little_endian == (endian::native == endian::little)) {
    std::memcpy(p, &x, sizeof(x));
  } else {
    int64_t as_int = bit_cast<int64_t>(x);
    int64_t swapped = __builtin_bswap64(as_int);
    std::memcpy(p, &swapped, sizeof(swapped));
  }

  return 0;
}

PY_EXPORT double _PyFloat_Unpack2(const unsigned char* p, int little_endian) {
  int incr = 1;
  if (little_endian == (endian::native == endian::little)) {
    p += 1;
    incr = -1;
  }

  // First byte
  bool sign = (*p >> 7) & 1;
  int e = (*p & 0x7C) >> 2;
  unsigned int f = (*p & 0x03) << 8;
  p += incr;

  // Second byte
  f |= *p;

  if (e == 0x1f) {
    // Infinity
    if (f == 0) {
      return _Py_dg_infinity(sign);
    }
    // NaN
    return _Py_dg_stdnan(sign);
  }

  double x = static_cast<double>(f) / 1024.0;

  if (e == 0) {
    e = -14;
  } else {
    x += 1.0;
    e -= 15;
  }
  x = std::ldexp(x, e);

  return sign ? -x : x;
}

PY_EXPORT double _PyFloat_Unpack4(const unsigned char* p, int little_endian) {
  // Assumes float format is ieee_little_endian_format
  int32_t as_int = *reinterpret_cast<const int32_t*>(p);
  if (little_endian != (endian::native == endian::little)) {
    as_int = __builtin_bswap32(as_int);
  }
  return bit_cast<float>(as_int);
}

PY_EXPORT double _PyFloat_Unpack8(const unsigned char* p, int little_endian) {
  // Assumes double format is ieee_little_endian_format
  int64_t as_int = *reinterpret_cast<const int64_t*>(p);
  if (little_endian != (endian::native == endian::little)) {
    as_int = __builtin_bswap64(as_int);
  }
  return bit_cast<double>(as_int);
}

}  // namespace py
