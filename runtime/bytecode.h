#pragma once

namespace python {

// Define the set of bytes codes
// The first macro parameter, V, is for primary names of byte codes
// and the second ALIAS is used to capture a value or position in the
// enumeration as a secondary name.  The third parameter is the
// handler for the bytecode.

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
  V(BINARY_POWER, 19, doNotImplemented)                                        \
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
  V(GET_AITER, 50, doNotImplemented)                                           \
  V(GET_ANEXT, 51, doNotImplemented)                                           \
  V(BEFORE_ASYNC_WITH, 52, doNotImplemented)                                   \
  V(UNUSED_BYTECODE_53, 53, doInvalidBytecode)                                 \
  V(UNUSED_BYTECODE_54, 54, doInvalidBytecode)                                 \
  V(INPLACE_ADD, 55, doInplaceAdd)                                             \
  V(INPLACE_SUBTRACT, 56, doInplaceSubtract)                                   \
  V(INPLACE_MULTIPLY, 57, doInplaceMultiply)                                   \
  V(UNUSED_BYTECODE_58, 58, doInvalidBytecode)                                 \
  V(INPLACE_MODULO, 59, doInplaceModulo)                                       \
  V(STORE_SUBSCR, 60, doStoreSubscr)                                           \
  V(DELETE_SUBSCR, 61, doNotImplemented)                                       \
  V(BINARY_LSHIFT, 62, doBinaryLshift)                                         \
  V(BINARY_RSHIFT, 63, doBinaryRshift)                                         \
  V(BINARY_AND, 64, doBinaryAnd)                                               \
  V(BINARY_XOR, 65, doBinaryXor)                                               \
  V(BINARY_OR, 66, doBinaryOr)                                                 \
  V(INPLACE_POWER, 67, doNotImplemented)                                       \
  V(GET_ITER, 68, doGetIter)                                                   \
  V(GET_YIELD_FROM_ITER, 69, doNotImplemented)                                 \
  V(PRINT_EXPR, 70, doNotImplemented)                                          \
  V(LOAD_BUILD_CLASS, 71, doLoadBuildClass)                                    \
  V(YIELD_FROM, 72, doNotImplemented)                                          \
  V(GET_AWAITABLE, 73, doNotImplemented)                                       \
  V(UNUSED_BYTECODE_74, 74, doInvalidBytecode)                                 \
  V(INPLACE_LSHIFT, 75, doInplaceLshift)                                       \
  V(INPLACE_RSHIFT, 76, doInplaceRshift)                                       \
  V(INPLACE_AND, 77, doInplaceAnd)                                             \
  V(INPLACE_XOR, 78, doInplaceXor)                                             \
  V(INPLACE_OR, 79, doInplaceOr)                                               \
  V(BREAK_LOOP, 80, doBreakLoop)                                               \
  V(WITH_CLEANUP_START, 81, doWithCleanupStart)                                \
  V(WITH_CLEANUP_FINISH, 82, doWithCleanupFinish)                              \
  V(RETURN_VALUE, 83, doInvalidBytecode)                                       \
  V(IMPORT_STAR, 84, doNotImplemented)                                         \
  V(SETUP_ANNOTATIONS, 85, doNotImplemented)                                   \
  V(YIELD_VALUE, 86, doNotImplemented)                                         \
  V(POP_BLOCK, 87, doPopBlock)                                                 \
  V(END_FINALLY, 88, doEndFinally)                                             \
  V(POP_EXCEPT, 89, doNotImplemented)                                          \
  V(STORE_NAME, 90, doStoreName)                                               \
  V(DELETE_NAME, 91, doNotImplemented)                                         \
  V(UNPACK_SEQUENCE, 92, doUnpackSequence)                                     \
  V(FOR_ITER, 93, doForIter)                                                   \
  V(UNPACK_EX, 94, doNotImplemented)                                           \
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
  V(SETUP_FINALLY, 122, doNotImplemented)                                      \
  V(UNUSED_BYTECODE_123, 123, doInvalidBytecode)                               \
  V(LOAD_FAST, 124, doLoadFast)                                                \
  V(STORE_FAST, 125, doStoreFast)                                              \
  V(DELETE_FAST, 126, doNotImplemented)                                        \
  V(STORE_ANNOTATION, 127, doNotImplemented)                                   \
  V(UNUSED_BYTECODE_128, 128, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_129, 129, doInvalidBytecode)                               \
  V(RAISE_VARARGS, 130, doNotImplemented)                                      \
  V(CALL_FUNCTION, 131, doCallFunction)                                        \
  V(MAKE_FUNCTION, 132, doMakeFunction)                                        \
  V(BUILD_SLICE, 133, doBuildSlice)                                            \
  V(UNUSED_BYTECODE_134, 134, doInvalidBytecode)                               \
  V(LOAD_CLOSURE, 135, doLoadClosure)                                          \
  V(LOAD_DEREF, 136, doLoadDeref)                                              \
  V(STORE_DEREF, 137, doStoreDeref)                                            \
  V(DELETE_DEREF, 138, doNotImplemented)                                       \
  V(UNUSED_BYTECODE_139, 139, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_140, 140, doInvalidBytecode)                               \
  V(CALL_FUNCTION_KW, 141, doCallFunctionKw)                                   \
  V(CALL_FUNCTION_EX, 142, doCallFunctionEx)                                   \
  V(SETUP_WITH, 143, doSetupWith)                                              \
  V(EXTENDED_ARG, 144, doInvalidBytecode)                                      \
  V(LIST_APPEND, 145, doListAppend)                                            \
  V(SET_ADD, 146, doSetAdd)                                                    \
  V(MAP_ADD, 147, doMapAdd)                                                    \
  V(LOAD_CLASSDEREF, 148, doNotImplemented)                                    \
  V(BUILD_LIST_UNPACK, 149, doBuildListUnpack)                                 \
  V(BUILD_MAP_UNPACK, 150, doNotImplemented)                                   \
  V(BUILD_MAP_UNPACK_WITH_CALL, 151, doNotImplemented)                         \
  V(BUILD_TUPLE_UNPACK, 152, doBuildTupleUnpack)                               \
  V(BUILD_SET_UNPACK, 153, doBuildSetUnpack)                                   \
  V(SETUP_ASYNC_WITH, 154, doNotImplemented)                                   \
  V(FORMAT_VALUE, 155, doFormatValue)                                          \
  V(BUILD_CONST_KEY_MAP, 156, doBuildConstKeyMap)                              \
  V(BUILD_STRING, 157, doBuildString)                                          \
  V(BUILD_TUPLE_UNPACK_WITH_CALL, 158, doBuildTupleUnpack)                     \
  V(UNUSED_BYTECODE_159, 159, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_160, 160, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_161, 161, doInvalidBytecode)                               \
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
  V(UNUSED_BYTECODE_185, 185, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_186, 186, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_187, 187, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_188, 188, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_189, 189, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_190, 190, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_191, 191, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_192, 192, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_193, 193, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_194, 194, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_195, 195, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_196, 196, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_197, 197, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_198, 198, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_199, 199, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_200, 200, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_201, 201, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_202, 202, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_203, 203, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_204, 204, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_205, 205, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_206, 206, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_207, 207, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_208, 208, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_209, 209, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_210, 210, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_211, 211, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_212, 212, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_213, 213, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_214, 214, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_215, 215, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_216, 216, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_217, 217, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_218, 218, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_219, 219, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_220, 220, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_221, 221, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_222, 222, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_223, 223, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_224, 224, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_225, 225, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_226, 226, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_227, 227, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_228, 228, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_229, 229, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_230, 230, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_231, 231, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_232, 232, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_233, 233, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_234, 234, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_235, 235, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_236, 236, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_237, 237, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_238, 238, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_239, 239, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_240, 240, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_241, 241, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_242, 242, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_243, 243, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_244, 244, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_245, 245, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_246, 246, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_247, 247, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_248, 248, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_249, 249, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_250, 250, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_251, 251, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_252, 252, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_253, 253, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_254, 254, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_255, 255, doInvalidBytecode)                               \
  V(UNUSED_BYTECODE_256, 256, doInvalidBytecode)                               \
  V(EXCEPT_HANDLER, 257, doInvalidBytecode)

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

enum FormatValueFlag {
  FVC_MASK_FLAG = 0x3,
  FVC_NONE_FLAG = 0x0,
  FVC_STR_FLAG = 0x1,
  FVC_REPR_FLAG = 0x2,
  FVC_ASCII_FLAG = 0x3,
  FVS_MASK_FLAG = 0x4,
  FVS_HAVE_SPEC_FLAG = 0x4
};

enum MakeFunctionFlag {
  DEFAULT = 0x01,
  DEFAULT_KW = 0x02,
  ANNOTATION_DICT = 0x04,
  CLOSURE = 0x08,
};

enum CallFunctionExFlag {
  VAR_KEYWORDS = 0x01,
};

extern const char* const kBytecodeNames[];

extern const CompareOp kSwappedCompareOp[];

}  // namespace python
