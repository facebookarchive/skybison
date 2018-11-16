#include "bytecode.h"

namespace python {

const char* const kBytecodeNames[] = {
#define NAME(name, value, handler) #name,
    FOREACH_BYTECODE(NAME)
#undef NAME
};

const CompareOp kSwappedCompareOp[] = {GT, GE, EQ, NE, LT, LE};

}  // namespace python
