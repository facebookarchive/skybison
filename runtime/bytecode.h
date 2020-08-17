#pragma once

#include "handles.h"
#include "objects.h"

namespace py {

// Define the set of bytecodes.
//
// Except for the *_CACHED instructions at the end of the list, these are all
// taken directly from CPython. The cached bytecodes perform the same operation
// as their CPython counterpart, but do it more quickly using an inline cache.
//
// `FOREACH_BYTECODE` calls V for each bytecode with 3 arguments:
//   1. The opcode's name.
//   2. The opcode's numeric value.
//   3. The opcode handler's name.
#define FOREACH_BYTECODE(V)                                                    \
  V(UNUSED_BYTECODE_0, 0, doInvalidBytecode)                                   \
  V(POP_TOP, 1, doPopTop)                                                      \
  V(ROT_TWO, 2, doRotTwo)                                                      \
  V(ROT_THREE, 3, doRotThree)                                                  \
  V(DUP_TOP, 4, doDupTop)                                                      \
  V(DUP_TOP_TWO, 5, doDupTopTwo)                                               \
  V(UNUSED_BYTECODE_6, 6, doInvalidBytecode)                                   \
  V(UNUSED_BYTECODE_7, 7, doInvalidBytecode)                                   \
  V(UNUSED_BYTECODE_8, 8, doInvalidBytecode)                                   \
  V(NOP, 9, doNop)                                                             \
  V(UNARY_POSITIVE, 10, doUnaryPositive)                                       \
  V(UNARY_NEGATIVE, 11, doUnaryNegative)                                       \
  V(UNARY_NOT, 12, doUnaryNot)                                                 \
  V(UNUSED_BYTECODE_13, 13, doInvalidBytecode)                                 \
  V(UNUSED_BYTECODE_14, 14, doInvalidBytecode)                                 \
  V(UNARY_INVERT, 15, doUnaryInvert)                                           \
  V(BINARY_MATRIX_MULTIPLY, 16, doBinaryMatrixMultiply)                        \
  V(INPLACE_MATRIX_MULTIPLY, 17, doInplaceMatrixMultiply)                      \
  V(UNUSED_BYTECODE_18, 18, doInvalidBytecode)                                 \
  V(BINARY_POWER, 19, doBinaryPower)                                           \
  V(BINARY_MULTIPLY, 20, doBinaryMultiply)                                     \
  V(UNUSED_BYTECODE_21, 21, doInvalidBytecode)                                 \
  V(BINARY_MODULO, 22, doBinaryModulo)                                         \
  V(BINARY_ADD, 23, doBinaryAdd)                                               \
  V(BINARY_SUBTRACT, 24, doBinarySubtract)                                     \
  V(BINARY_SUBSCR, 25, doBinarySubscr)                                         \
  V(BINARY_FLOOR_DIVIDE, 26, doBinaryFloorDivide)                              \
  V(BINARY_TRUE_DIVIDE, 27, doBinaryTrueDivide)                                \
  V(INPLACE_FLOOR_DIVIDE, 28, doInplaceFloorDivide)                            \
  V(INPLACE_TRUE_DIVIDE, 29, doInplaceTrueDivide)                              \
  V(UNUSED_BYTECODE_30, 30, doInvalidBytecode)                                 \
  V(UNUSED_BYTECODE_31, 31, doInvalidBytecode)                                 \
  V(UNUSED_BYTECODE_32, 32, doInvalidBytecode)                                 \
  V(UNUSED_BYTECODE_33, 33, doInvalidBytecode)                                 \
  V(UNUSED_BYTECODE_34, 34, doInvalidBytecode)                                 \
  V(UNUSED_BYTECODE_35, 35, doInvalidBytecode)                                 \
  V(UNUSED_BYTECODE_36, 36, doInvalidBytecode)                                 \
  V(UNUSED_BYTECODE_37, 37, doInvalidBytecode)                                 \
  V(UNUSED_BYTECODE_38, 38, doInvalidBytecode)                                 \
  V(UNUSED_BYTECODE_39, 39, doInvalidBytecode)                                 \
  V(UNUSED_BYTECODE_40, 40, doInvalidBytecode)                                 \
  V(UNUSED_BYTECODE_41, 41, doInvalidBytecode)                                 \
  V(UNUSED_BYTECODE_42, 42, doInvalidBytecode)                                 \
  V(UNUSED_BYTECODE_43, 43, doInvalidBytecode)                                 \
  V(UNUSED_BYTECODE_44, 44, doInvalidBytecode)                                 \
  V(UNUSED_BYTECODE_45, 45, doInvalidBytecode)                                 \
  V(UNUSED_BYTECODE_46, 46, doInvalidBytecode)                                 \
  V(UNUSED_BYTECODE_47, 47, doInvalidBytecode)                                 \
  V(UNUSED_BYTECODE_48, 48, doInvalidBytecode)                                 \
  V(UNUSED_BYTECODE_49, 49, doInvalidBytecode)                                 \
  V(GET_AITER, 50, doGetAiter)                                                 \
  V(GET_ANEXT, 51, doGetAnext)                                                 \
  V(BEFORE_ASYNC_WITH, 52, doBeforeAsyncWith)                                  \
  V(UNUSED_BYTECODE_53, 53, doInvalidBytecode)                                 \
  V(UNUSED_BYTECODE_54, 54, doInvalidBytecode)                                 \
  V(INPLACE_ADD, 55, doInplaceAdd)                                             \
  V(INPLACE_SUBTRACT, 56, doInplaceSubtract)                                   \
  V(INPLACE_MULTIPLY, 57, doInplaceMultiply)                                   \
  V(UNUSED_BYTECODE_58, 58, doInvalidBytecode)                                 \
  V(INPLACE_MODULO, 59, doInplaceModulo)                                       \
  V(STORE_SUBSCR, 60, doStoreSubscr)                                           \
  V(DELETE_SUBSCR, 61, doDeleteSubscr)                                         \
  V(BINARY_LSHIFT, 62, doBinaryLshift)                                         \
  V(BINARY_RSHIFT, 63, doBinaryRshift)                                         \
  V(BINARY_AND, 64, doBinaryAnd)                                               \
  V(BINARY_XOR, 65, doBinaryXor)                                               \
  V(BINARY_OR, 66, doBinaryOr)                                                 \
  V(INPLACE_POWER, 67, doInplacePower)                                         \
  V(GET_ITER, 68, doGetIter)                                                   \
  V(GET_YIELD_FROM_ITER, 69, doGetYieldFromIter)                               \
  V(PRINT_EXPR, 70, doPrintExpr)                                               \
  V(LOAD_BUILD_CLASS, 71, doLoadBuildClass)                                    \
  V(YIELD_FROM, 72, doYieldFrom)                                               \
  V(GET_AWAITABLE, 73, doGetAwaitable)                                         \
  V(UNUSED_BYTECODE_74, 74, doInvalidBytecode)                                 \
  V(INPLACE_LSHIFT, 75, doInplaceLshift)                                       \
  V(INPLACE_RSHIFT, 76, doInplaceRshift)                                       \
  V(INPLACE_AND, 77, doInplaceAnd)                                             \
  V(INPLACE_XOR, 78, doInplaceXor)                                             \
  V(INPLACE_OR, 79, doInplaceOr)                                               \
  V(BREAK_LOOP, 80, doBreakLoop)                                               \
  V(WITH_CLEANUP_START, 81, doWithCleanupStart)                                \
  V(WITH_CLEANUP_FINISH, 82, doWithCleanupFinish)                              \
  V(RETURN_VALUE, 83, doReturnValue)                                           \
  V(IMPORT_STAR, 84, doImportStar)                                             \
  V(SETUP_ANNOTATIONS, 85, doSetupAnnotations)                                 \
  V(YIELD_VALUE, 86, doYieldValue)                                             \
  V(POP_BLOCK, 87, doPopBlock)                                                 \
  V(END_FINALLY, 88, doEndFinally)                                             \
  V(POP_EXCEPT, 89, doPopExcept)                                               \
  V(STORE_NAME, 90, doStoreName)                                               \
  V(DELETE_NAME, 91, doDeleteName)                                             \
  V(UNPACK_SEQUENCE, 92, doUnpackSequence)                                     \
  V(FOR_ITER, 93, doForIter)                                                   \
  V(UNPACK_EX, 94, doUnpackEx)                                                 \
  V(STORE_ATTR, 95, doStoreAttr)                                               \
  V(DELETE_ATTR, 96, doDeleteAttr)                                             \
  V(STORE_GLOBAL, 97, doStoreGlobal)                                           \
  V(DELETE_GLOBAL, 98, doDeleteGlobal)                                         \
  V(UNUSED_BYTECODE_99, 99, doInvalidBytecode)                                 \
  V(LOAD_CONST, 100, doLoadConst)                                              \
  V(LOAD_NAME, 101, doLoadName)                                                \
  V(BUILD_TUPLE, 102, doBuildTuple)                                            \
  V(BUILD_LIST, 103, doBuildList)                                              \
  V(BUILD_SET, 104, doBuildSet)                                                \
  V(BUILD_MAP, 105, doBuildMap)                                                \
  V(LOAD_ATTR, 106, doLoadAttr)                                                \
  V(COMPARE_OP, 107, doCompareOp)                                              \
  V(IMPORT_NAME, 108, doImportName)                                            \
  V(IMPORT_FROM, 109, doImportFrom)                                            \
  V(JUMP_FORWARD, 110, doJumpForward)                                          \
  V(JUMP_IF_FALSE_OR_POP, 111, doJumpIfFalseOrPop)                             \
  V(JUMP_IF_TRUE_OR_POP, 112, doJumpIfTrueOrPop)                               \
  V(JUMP_ABSOLUTE, 113, doJumpAbsolute)                                        \
  V(POP_JUMP_IF_FALSE, 114, doPopJumpIfFalse)                                  \
  V(POP_JUMP_IF_TRUE, 115, doPopJumpIfTrue)                                    \
  V(LOAD_GLOBAL, 116, doLoadGlobal)                                            \
  V(UNUSED_BYTECODE_117, 117, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_118, 118, doInvalidBytecode)                               \
  V(CONTINUE_LOOP, 119, doContinueLoop)                                        \
  V(SETUP_LOOP, 120, doSetupLoop)                                              \
  V(SETUP_EXCEPT, 121, doSetupExcept)                                          \
  V(SETUP_FINALLY, 122, doSetupFinally)                                        \
  V(UNUSED_BYTECODE_123, 123, doInvalidBytecode)                               \
  V(LOAD_FAST, 124, doLoadFast)                                                \
  V(STORE_FAST, 125, doStoreFast)                                              \
  V(DELETE_FAST, 126, doDeleteFast)                                            \
  V(STORE_ANNOTATION, 127, doStoreAnnotation)                                  \
  V(UNUSED_BYTECODE_128, 128, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_129, 129, doInvalidBytecode)                               \
  V(RAISE_VARARGS, 130, doRaiseVarargs)                                        \
  V(CALL_FUNCTION, 131, doCallFunction)                                        \
  V(MAKE_FUNCTION, 132, doMakeFunction)                                        \
  V(BUILD_SLICE, 133, doBuildSlice)                                            \
  V(UNUSED_BYTECODE_134, 134, doInvalidBytecode)                               \
  V(LOAD_CLOSURE, 135, doLoadClosure)                                          \
  V(LOAD_DEREF, 136, doLoadDeref)                                              \
  V(STORE_DEREF, 137, doStoreDeref)                                            \
  V(DELETE_DEREF, 138, doDeleteDeref)                                          \
  V(UNUSED_BYTECODE_139, 139, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_140, 140, doInvalidBytecode)                               \
  V(CALL_FUNCTION_KW, 141, doCallFunctionKw)                                   \
  V(CALL_FUNCTION_EX, 142, doCallFunctionEx)                                   \
  V(SETUP_WITH, 143, doSetupWith)                                              \
  V(EXTENDED_ARG, 144, doInvalidBytecode)                                      \
  V(LIST_APPEND, 145, doListAppend)                                            \
  V(SET_ADD, 146, doSetAdd)                                                    \
  V(MAP_ADD, 147, doMapAdd)                                                    \
  V(LOAD_CLASSDEREF, 148, doLoadClassDeref)                                    \
  V(BUILD_LIST_UNPACK, 149, doBuildListUnpack)                                 \
  V(BUILD_MAP_UNPACK, 150, doBuildMapUnpack)                                   \
  V(BUILD_MAP_UNPACK_WITH_CALL, 151, doBuildMapUnpackWithCall)                 \
  V(BUILD_TUPLE_UNPACK, 152, doBuildTupleUnpack)                               \
  V(BUILD_SET_UNPACK, 153, doBuildSetUnpack)                                   \
  V(SETUP_ASYNC_WITH, 154, doSetupAsyncWith)                                   \
  V(FORMAT_VALUE, 155, doFormatValue)                                          \
  V(BUILD_CONST_KEY_MAP, 156, doBuildConstKeyMap)                              \
  V(BUILD_STRING, 157, doBuildString)                                          \
  V(BUILD_TUPLE_UNPACK_WITH_CALL, 158, doBuildTupleUnpack)                     \
  V(UNUSED_BYTECODE_159, 159, doInvalidBytecode)                               \
  V(LOAD_METHOD, 160, doLoadMethod)                                            \
  V(CALL_METHOD, 161, doCallMethod)                                            \
  V(UNUSED_BYTECODE_162, 162, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_163, 163, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_164, 164, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_165, 165, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_166, 166, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_167, 167, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_168, 168, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_169, 169, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_170, 170, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_171, 171, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_172, 172, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_173, 173, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_174, 174, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_175, 175, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_176, 176, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_177, 177, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_178, 178, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_179, 179, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_180, 180, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_181, 181, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_182, 182, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_183, 183, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_184, 184, doInvalidBytecode)                               \
  V(STORE_SUBSCR_LIST, 185, doStoreSubscrList)                                 \
  V(BINARY_SUBSCR_LIST, 186, doBinarySubscrList)                               \
  V(LOAD_ATTR_INSTANCE_SLOT_DESCR, 187, doLoadAttrInstanceSlotDescr)           \
  V(COMPARE_IN_LIST, 188, doCompareInList)                                     \
  V(COMPARE_IN_DICT, 189, doCompareInDict)                                     \
  V(COMPARE_IN_TUPLE, 190, doCompareInTuple)                                   \
  V(COMPARE_IN_STR, 191, doCompareInStr)                                       \
  V(COMPARE_IN_POLYMORPHIC, 192, doCompareInPolymorphic)                       \
  V(COMPARE_IN_MONOMORPHIC, 193, doCompareInMonomorphic)                       \
  V(COMPARE_IN_ANAMORPHIC, 194, doCompareInAnamorphic)                         \
  V(BINARY_FLOORDIV_SMALLINT, 195, doBinaryFloordivSmallInt)                   \
  V(BINARY_AND_SMALLINT, 196, doBinaryAndSmallInt)                             \
  V(FOR_ITER_STR, 197, doForIterStr)                                           \
  V(FOR_ITER_RANGE, 198, doForIterRange)                                       \
  V(FOR_ITER_TUPLE, 199, doForIterTuple)                                       \
  V(FOR_ITER_DICT, 200, doForIterDict)                                         \
  V(FOR_ITER_LIST, 201, doForIterList)                                         \
  V(INPLACE_ADD_SMALLINT, 202, doInplaceAddSmallInt)                           \
  V(COMPARE_EQ_STR, 203, doCompareEqStr)                                       \
  V(COMPARE_LE_SMALLINT, 204, doCompareLeSmallInt)                             \
  V(COMPARE_NE_SMALLINT, 205, doCompareNeSmallInt)                             \
  V(COMPARE_GE_SMALLINT, 206, doCompareGeSmallInt)                             \
  V(COMPARE_LT_SMALLINT, 207, doCompareLtSmallInt)                             \
  V(COMPARE_GT_SMALLINT, 208, doCompareGtSmallInt)                             \
  V(COMPARE_EQ_SMALLINT, 209, doCompareEqSmallInt)                             \
  V(BINARY_OR_SMALLINT, 210, doBinaryOrSmallInt)                               \
  V(BINARY_SUB_SMALLINT, 211, doBinarySubSmallInt)                             \
  V(BINARY_ADD_SMALLINT, 212, doBinaryAddSmallInt)                             \
  V(INPLACE_OP_POLYMORPHIC, 213, doInplaceOpPolymorphic)                       \
  V(INPLACE_OP_MONOMORPHIC, 214, doInplaceOpMonomorphic)                       \
  V(COMPARE_OP_POLYMORPHIC, 215, doCompareOpPolymorphic)                       \
  V(COMPARE_OP_MONOMORPHIC, 216, doCompareOpMonomorphic)                       \
  V(BINARY_OP_POLYMORPHIC, 217, doBinaryOpPolymorphic)                         \
  V(BINARY_OP_MONOMORPHIC, 218, doBinaryOpMonomorphic)                         \
  V(STORE_SUBSCR_POLYMORPHIC, 219, doStoreSubscrPolymorphic)                   \
  V(STORE_SUBSCR_MONOMORPHIC, 220, doStoreSubscrMonomorphic)                   \
  V(FOR_ITER_POLYMORPHIC, 221, doForIterPolymorphic)                           \
  V(FOR_ITER_MONOMORPHIC, 222, doForIterMonomorphic)                           \
  V(BINARY_SUBSCR_POLYMORPHIC, 223, doBinarySubscrPolymorphic)                 \
  V(BINARY_SUBSCR_MONOMORPHIC, 224, doBinarySubscrMonomorphic)                 \
  V(STORE_ATTR_INSTANCE_OVERFLOW_UPDATE, 225,                                  \
    doStoreAttrInstanceOverflowUpdate)                                         \
  V(STORE_ATTR_INSTANCE_OVERFLOW, 226, doStoreAttrInstanceOverflow)            \
  V(STORE_ATTR_INSTANCE, 227, doStoreAttrInstance)                             \
  V(STORE_ATTR_POLYMORPHIC, 228, doStoreAttrPolymorphic)                       \
  V(LOAD_ATTR_POLYMORPHIC, 229, doLoadAttrPolymorphic)                         \
  V(LOAD_ATTR_INSTANCE_TYPE_BOUND_METHOD, 230,                                 \
    doLoadAttrInstanceTypeBoundMethod)                                         \
  V(LOAD_ATTR_INSTANCE, 231, doLoadAttrInstance)                               \
  V(LOAD_METHOD_POLYMORPHIC, 232, doLoadMethodPolymorphic)                     \
  V(LOAD_METHOD_INSTANCE_FUNCTION, 233, doLoadMethodInstanceFunction)          \
  V(STORE_SUBSCR_ANAMORPHIC, 234, doStoreSubscrAnamorphic)                     \
  V(LOAD_ATTR_INSTANCE_TYPE, 235, doLoadAttrInstanceType)                      \
  V(LOAD_ATTR_INSTANCE_TYPE_DESCR, 236, doLoadAttrInstanceTypeDescr)           \
  V(LOAD_ATTR_INSTANCE_PROPERTY, 237, doLoadAttrInstanceProperty)              \
  V(STORE_ATTR_INSTANCE_UPDATE, 238, doStoreAttrInstanceUpdate)                \
  V(LOAD_ATTR_TYPE, 239, doLoadAttrType)                                       \
  V(LOAD_ATTR_MODULE, 240, doLoadAttrModule)                                   \
  V(COMPARE_IS_NOT, 241, doCompareIsNot)                                       \
  V(COMPARE_IS, 242, doCompareIs)                                              \
  V(LOAD_IMMEDIATE, 243, doLoadImmediate)                                      \
  V(STORE_FAST_REVERSE, 244, doStoreFastReverse)                               \
  V(LOAD_FAST_REVERSE, 245, doLoadFastReverse)                                 \
  V(LOAD_METHOD_ANAMORPHIC, 246, doLoadMethodAnamorphic)                       \
  V(STORE_GLOBAL_CACHED, 247, doStoreGlobalCached)                             \
  V(LOAD_GLOBAL_CACHED, 248, doLoadGlobalCached)                               \
  V(FOR_ITER_ANAMORPHIC, 249, doForIterAnamorphic)                             \
  V(COMPARE_OP_ANAMORPHIC, 250, doCompareOpAnamorphic)                         \
  V(INPLACE_OP_ANAMORPHIC, 251, doInplaceOpAnamorphic)                         \
  V(BINARY_OP_ANAMORPHIC, 252, doBinaryOpAnamorphic)                           \
  V(BINARY_SUBSCR_ANAMORPHIC, 253, doBinarySubscrAnamorphic)                   \
  V(STORE_ATTR_ANAMORPHIC, 254, doStoreAttrAnamorphic)                         \
  V(LOAD_ATTR_ANAMORPHIC, 255, doLoadAttrAnamorphic)

const word kNumBytecodes = 256;

enum Bytecode {
#define ENUM(name, value, handler) name = value,
  FOREACH_BYTECODE(ENUM)
#undef ENUM
};

enum CompareOp {
  LT = 0,
  LE = 1,
  EQ = 2,
  NE = 3,
  GT = 4,
  GE = 5,
  IN,
  NOT_IN,
  IS,
  IS_NOT,
  EXC_MATCH,
  BAD
};

enum class FormatValueConv {
  kNone = 0,
  kStr = 1,
  kRepr = 2,
  kAscii = 3,
};

const word kFormatValueConvMask = 0x3;
const word kFormatValueHasSpecBit = 0x4;

enum MakeFunctionFlag {
  DEFAULT = 0x01,
  DEFAULT_KW = 0x02,
  ANNOTATION_DICT = 0x04,
  CLOSURE = 0x08,
};

enum CallFunctionExFlag {
  VAR_KEYWORDS = 0x01,
};

// Size of opcode + argument in bytecode (called `_Py_CODEUNIT` in cpython).
const word kCodeUnitSize = 2;

extern const char* const kBytecodeNames[];

extern const CompareOp kSwappedCompareOp[];

struct BytecodeOp {
  Bytecode bc;
  int32_t arg;
};

BytecodeOp nextBytecodeOp(const MutableBytes& bytecode, word* index);

inline bool isByteCodeWithCache(const Bytecode bc) {
  // TODO(T45720638): Add all caching opcodes here once they are supported for
  // cache invalidation.
  switch (bc) {
    case BINARY_OP_MONOMORPHIC:
    case BINARY_OP_POLYMORPHIC:
    case BINARY_OP_ANAMORPHIC:
    case BINARY_SUBSCR_ANAMORPHIC:
    case BINARY_SUBSCR_MONOMORPHIC:
    case BINARY_SUBSCR_POLYMORPHIC:
    case COMPARE_OP_MONOMORPHIC:
    case COMPARE_OP_POLYMORPHIC:
    case COMPARE_OP_ANAMORPHIC:
    case FOR_ITER_MONOMORPHIC:
    case FOR_ITER_POLYMORPHIC:
    case FOR_ITER_ANAMORPHIC:
    case INPLACE_OP_MONOMORPHIC:
    case INPLACE_OP_POLYMORPHIC:
    case INPLACE_OP_ANAMORPHIC:
    case LOAD_ATTR_INSTANCE:
    case LOAD_ATTR_INSTANCE_PROPERTY:
    case LOAD_ATTR_INSTANCE_SLOT_DESCR:
    case LOAD_ATTR_INSTANCE_TYPE:
    case LOAD_ATTR_INSTANCE_TYPE_BOUND_METHOD:
    case LOAD_ATTR_INSTANCE_TYPE_DESCR:
    case LOAD_ATTR_MODULE:
    case LOAD_ATTR_TYPE:
    case LOAD_ATTR_ANAMORPHIC:
    case LOAD_METHOD_ANAMORPHIC:
    case LOAD_METHOD_INSTANCE_FUNCTION:
    case LOAD_METHOD_POLYMORPHIC:
    case STORE_ATTR_INSTANCE:
    case STORE_ATTR_INSTANCE_OVERFLOW:
    case STORE_ATTR_INSTANCE_OVERFLOW_UPDATE:
    case STORE_ATTR_INSTANCE_UPDATE:
    case STORE_ATTR_POLYMORPHIC:
    case STORE_ATTR_ANAMORPHIC:
    case STORE_SUBSCR_ANAMORPHIC:
      return true;
    default:
      return false;
  }
}

// Convert an immediate RawObject into a byte, and back to the original
// object. If the object fits in a byte,
// objectFromOparg(opargFromObject(obj) == obj will hold.
int8_t opargFromObject(RawObject object);

inline RawObject objectFromOparg(word arg) {
  return RawObject(static_cast<uword>(static_cast<int8_t>(arg)));
}

// Prepares bytecode for caching: Adds a rewritten variant of the bytecode to
// `function`. It has the arguments of opcodes that use the cache replaced with
// a cache index. The previous arguments are moved to a separate tuple and can
// be retrieved with `icOriginalArg()`. Also adds a correctly sized `caches`
// tuple to `function`.
void rewriteBytecode(Thread* thread, const Function& function);

// Returns the original argument of bytecode operations that were rewritten by
// `rewriteBytecode()`.
inline word originalArg(RawFunction function, word index) {
  return SmallInt::cast(RawTuple::cast(function.originalArguments()).at(index))
      .value();
}

}  // namespace py
