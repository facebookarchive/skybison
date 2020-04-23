#include <cmath>

#include "cpython-data.h"
#include "cpython-func.h"

#include "float-builtins.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace py {

// Make sure constants are kept in sync between pyro and C-API.
static_assert(static_cast<word>(_PyHASH_INF) == kHashInf, "constant mismatch");
static_assert(static_cast<word>(_PyHASH_NAN) == kHashNan, "constant mismatch");
static_assert(static_cast<word>(_PyHASH_IMAG) == kHashImag,
              "constant mismatch");
static_assert(_PyHASH_BITS == kArithmeticHashBits, "constant mismatch");
static_assert(static_cast<word>(_PyHASH_MODULUS) == kArithmeticHashModulus,
              "constant mismatch");

PY_EXPORT Py_hash_t _Py_HashDouble(double v) { return doubleHash(v); }

PY_EXPORT Py_hash_t _Py_HashPointer(void* p) {
  size_t y = bit_cast<size_t>(p);
  /* bottom 3 or 4 bits are likely to be 0; rotate y by 4 to avoid
     excessive hash collisions for dicts and sets */
  y = (y >> 4) | (y << (kBitsPerPointer - 4));
  Py_hash_t x = (Py_hash_t)y;
  if (x == -1) x = -2;
  return x;
}

PY_EXPORT Py_hash_t _Py_HashBytes(const void* src, Py_ssize_t len) {
  DCHECK(len >= 0, "invalid len");
  View<byte> bytes(reinterpret_cast<const byte*>(src), len);
  if (len <= SmallBytes::kMaxLength) {
    return SmallBytes::fromBytes(bytes).hash();
  }
  return Thread::current()->runtime()->bytesHash(bytes);
}

PY_EXPORT void _PyHash_Fini(void) {}

PY_EXPORT const _Py_HashSecret_t* _Py_HashSecret_Ptr() {
  static_assert(
      kWordSize * Runtime::kHashSecretSize >= sizeof(_Py_HashSecret_t),
      "hash secret too small");
  return reinterpret_cast<const _Py_HashSecret_t*>(
      Thread::current()->runtime()->hashSecret());
}

}  // namespace py
