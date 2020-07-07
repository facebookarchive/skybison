#pragma once

#include "cpython-types.h"

#include "globals.h"

namespace py {

namespace compile {

inline _Py_CODEUNIT PACKOPARG(unsigned char opcode, unsigned char oparg) {
  if (endian::native == endian::little) {
    return ((_Py_CODEUNIT)(((oparg) << 8) | (opcode)));
  } else {
    return ((_Py_CODEUNIT)(((opcode) << 8) | (oparg)));
  }
}

/* Minimum number of code units necessary to encode instruction with
   EXTENDED_ARGs */
inline int instrsize(unsigned int oparg) {
  return oparg <= 0xff ? 1 : oparg <= 0xffff ? 2 : oparg <= 0xffffff ? 3 : 4;
}

}  // namespace compile

}  // namespace py
