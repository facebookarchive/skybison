#include "bytecode.h"

namespace python {

const char* const kBytecodeNames[] = {
#define NAME(name, value, handler) #name,
    FOREACH_BYTECODE(NAME)
#undef NAME
};

const CompareOp kSwappedCompareOp[] = {GT, GE, EQ, NE, LT, LE};

BytecodeOp nextBytecodeOp(const MutableBytes& bytecode, word* index) {
  word i = *index;
  Bytecode bc = static_cast<Bytecode>(bytecode.byteAt(i++));
  int32_t arg = bytecode.byteAt(i++);
  while (bc == Bytecode::EXTENDED_ARG) {
    bc = static_cast<Bytecode>(bytecode.byteAt(i++));
    arg = (arg << kBitsPerByte) | bytecode.byteAt(i++);
  }
  DCHECK(i - *index <= 8, "EXTENDED_ARG-encoded arg must fit in int32_t");
  *index = i;
  return BytecodeOp{bc, arg};
}

int8_t opargFromObject(RawObject object) {
  DCHECK(!object.isHeapObject(), "Heap objects are disallowed");
  return static_cast<int8_t>(object.raw());
}

RawObject objectFromOparg(word arg) {
  return RawObject(static_cast<uword>(static_cast<int8_t>(arg)));
}

}  // namespace python
