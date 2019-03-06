// longobject.c implementation

#include "handles.h"
#include "int-builtins.h"
#include "objects.h"
#include "runtime.h"

// Table of digit values for 8-bit string -> integer conversion.
// '0' maps to 0, ..., '9' maps to 9.
// 'a' and 'A' map to 10, ..., 'z' and 'Z' map to 35.
// All other indices map to 37.
// Note that when converting a base B string, a char c is a legitimate
// base B digit iff _PyLong_DigitValue[Py_CHARMASK(c)] < B.
// clang-format off
unsigned char _PyLong_DigitValue[256] = {
    37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37,
    37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37,
    37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37,
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  37, 37, 37, 37, 37, 37,
    37, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
    25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 37, 37, 37, 37, 37,
    37, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
    25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 37, 37, 37, 37, 37,
    37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37,
    37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37,
    37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37,
    37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37,
    37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37,
    37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37,
    37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37,
    37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37,
};
// clang-format on

namespace python {

PY_EXPORT int PyLong_CheckExact_Func(PyObject* obj) {
  return ApiHandle::fromPyObject(obj)->asObject()->isInt();
}

PY_EXPORT int PyLong_Check_Func(PyObject* obj) {
  return Thread::currentThread()->runtime()->isInstanceOfInt(
      ApiHandle::fromPyObject(obj)->asObject());
}

// Converting from signed ints.

PY_EXPORT PyObject* PyLong_FromLong(long ival) {
  Thread* thread = Thread::currentThread();
  return ApiHandle::newReference(thread, thread->runtime()->newInt(ival));
}

PY_EXPORT PyObject* PyLong_FromLongLong(long long ival) {
  static_assert(sizeof(ival) <= sizeof(long), "Unsupported long long size");
  return PyLong_FromLong(ival);
}

PY_EXPORT PyObject* PyLong_FromSsize_t(Py_ssize_t ival) {
  static_assert(sizeof(ival) <= sizeof(long), "Unsupported Py_ssize_t size");
  return PyLong_FromLong(ival);
}

// Converting from unsigned ints.

PY_EXPORT PyObject* PyLong_FromUnsignedLong(unsigned long ival) {
  static_assert(sizeof(ival) <= sizeof(uword),
                "Unsupported unsigned long type");
  Thread* thread = Thread::currentThread();
  return ApiHandle::newReference(thread,
                                 thread->runtime()->newIntFromUnsigned(ival));
}

PY_EXPORT PyObject* PyLong_FromUnsignedLongLong(unsigned long long ival) {
  static_assert(sizeof(ival) <= sizeof(unsigned long),
                "Unsupported unsigned long long size");
  return PyLong_FromUnsignedLong(ival);
}

PY_EXPORT PyObject* PyLong_FromSize_t(size_t ival) {
  static_assert(sizeof(ival) <= sizeof(unsigned long),
                "Unsupported size_t size");
  return PyLong_FromUnsignedLong(ival);
}

// Attempt to convert the given PyObject to T. When overflow != nullptr,
// *overflow will be set to -1, 1, or 0 to indicate underflow, overflow, or
// neither, respectively. When under/overflow occurs, -1 is returned; otherwise,
// the value is returned.
//
// When overflow == nullptr, an exception will be raised and -1 is returned if
// the value doesn't fit in T.
template <typename T>
static T asInt(PyObject* pylong, const char* type_name, int* overflow) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);

  if (pylong == nullptr) {
    thread->raiseBadInternalCall();
    return -1;
  }
  Object object(&scope, ApiHandle::fromPyObject(pylong)->asObject());

  Object longobj(&scope, asIntObject(thread, object));
  if (longobj.isError()) {
    return -1;
  }

  auto const result = RawInt::cast(*longobj)->asInt<T>();
  if (result.error == CastError::None) {
    if (overflow) *overflow = 0;
    return result.value;
  }

  if (overflow) {
    *overflow = (result.error == CastError::Underflow) ? -1 : 1;
  } else if (result.error == CastError::Underflow &&
             std::is_unsigned<T>::value) {
    thread->raiseOverflowErrorWithCStr(
        "can't convert negative value to unsigned");
  } else {
    thread->raiseOverflowError(thread->runtime()->newStrFromFormat(
        "Python int too big to convert to C %s", type_name));
  }
  return -1;
}

template <typename T>
static T asIntWithoutOverflowCheck(PyObject* pylong) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);

  if (pylong == nullptr) {
    thread->raiseBadInternalCall();
    return -1;
  }
  Object object(&scope, ApiHandle::fromPyObject(pylong)->asObject());

  Object longobj(&scope, asIntObject(thread, object));
  if (longobj.isError()) {
    return -1;
  }
  static_assert(sizeof(T) <= sizeof(word), "T requires multiple digits");
  Int intobj(&scope, *longobj);
  return intobj.digitAt(0);
}

// Converting to signed ints.

PY_EXPORT int _PyLong_AsInt(PyObject* pylong) {
  return asInt<int>(pylong, "int", nullptr);
}

PY_EXPORT long PyLong_AsLong(PyObject* pylong) {
  return asInt<long>(pylong, "long", nullptr);
}

PY_EXPORT long long PyLong_AsLongLong(PyObject* val) {
  return asInt<long long>(val, "long long", nullptr);
}

PY_EXPORT Py_ssize_t PyLong_AsSsize_t(PyObject* val) {
  return asInt<Py_ssize_t>(val, "ssize_t", nullptr);
}

// Converting to unsigned ints.

PY_EXPORT unsigned long PyLong_AsUnsignedLong(PyObject* val) {
  return asInt<unsigned long>(val, "unsigned long", nullptr);
}

PY_EXPORT unsigned long long PyLong_AsUnsignedLongLong(PyObject* val) {
  return asInt<unsigned long long>(val, "unsigned long long", nullptr);
}

PY_EXPORT size_t PyLong_AsSize_t(PyObject* val) {
  return asInt<size_t>(val, "size_t", nullptr);
}

PY_EXPORT long PyLong_AsLongAndOverflow(PyObject* pylong, int* overflow) {
  return asInt<long>(pylong, "", overflow);
}

PY_EXPORT long long PyLong_AsLongLongAndOverflow(PyObject* pylong,
                                                 int* overflow) {
  return asInt<long long>(pylong, "", overflow);
}

PY_EXPORT PyObject* PyLong_FromDouble(double /* l */) {
  UNIMPLEMENTED("PyLong_FromDouble");
}

PY_EXPORT PyObject* PyLong_FromString(const char* /* r */, char** /* pend */,
                                      int /* e */) {
  UNIMPLEMENTED("PyLong_FromString");
}

PY_EXPORT double PyLong_AsDouble(PyObject* /* v */) {
  UNIMPLEMENTED("PyLong_AsDouble");
}

PY_EXPORT unsigned long long PyLong_AsUnsignedLongLongMask(PyObject* op) {
  return asIntWithoutOverflowCheck<unsigned long long>(op);
}

PY_EXPORT unsigned long PyLong_AsUnsignedLongMask(PyObject* op) {
  return asIntWithoutOverflowCheck<unsigned long>(op);
}

PY_EXPORT void* PyLong_AsVoidPtr(PyObject* /* v */) {
  UNIMPLEMENTED("PyLong_AsVoidPtr");
}

PY_EXPORT PyObject* PyLong_FromVoidPtr(void* /* p */) {
  UNIMPLEMENTED("PyLong_FromVoidPtr");
}

PY_EXPORT PyObject* PyLong_GetInfo() { UNIMPLEMENTED("PyLong_GetInfo"); }

PY_EXPORT int _PyLong_AsByteArray(PyLongObject* longobj, unsigned char* dst,
                                  size_t n, int little_endian, int is_signed) {
  DCHECK(longobj != nullptr, "null argument to _PyLong_AsByteArray");
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  PyObject* pyobj = reinterpret_cast<PyObject*>(longobj);
  Int self(&scope, ApiHandle::fromPyObject(pyobj)->asObject());
  if (!is_signed && self.isNegative()) {
    thread->raiseOverflowErrorWithCStr(
        "can't convert negative int to unsigned");
    return -1;
  }
  word length = static_cast<word>(n);
  endian endianness = little_endian ? endian::little : endian::big;
  Bytes result(&scope, runtime->intToBytes(thread, self, length, endianness));
  result.copyTo(dst, length);

  // Check for overflow.
  word num_digits = self.numDigits();
  uword high_digit = self.digitAt(num_digits - 1);
  word bit_length =
      num_digits * kBitsPerWord - Utils::numRedundantSignBits(high_digit);
  if (bit_length > length * kBitsPerByte + !is_signed) {
    thread->raiseOverflowErrorWithCStr("int too big to convert");
    return -1;
  }
  return 0;
}

PY_EXPORT PyObject* _PyLong_FromByteArray(const unsigned char* bytes, size_t n,
                                          int little_endian, int is_signed) {
  if (n == 0) return PyLong_FromLong(0);
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  // This copies the bytes an extra time, but it is more important for the
  // runtime to accommodate int.from_bytes(), so allow the extra copy.
  Bytes source(&scope, runtime->newBytesWithAll(
                           View<byte>(bytes, static_cast<word>(n))));
  endian endianness = little_endian ? endian::little : endian::big;
  Object result(&scope,
                runtime->bytesToInt(thread, source, endianness, is_signed));
  return result.isError() ? nullptr : ApiHandle::newReference(thread, *result);
}

PY_EXPORT int _PyLong_Sign(PyObject* vv) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Object obj(&scope, ApiHandle::fromPyObject(vv)->asObject());
  Runtime* runtime = thread->runtime();
  DCHECK(runtime->isInstanceOfInt(*obj), "requires an integer");
  if (!obj.isInt()) {
    UNIMPLEMENTED("int subclass");
  }
  Int value(&scope, *obj);
  return value.isZero() ? 0 : (value.isNegative() ? -1 : 1);
}

}  // namespace python
