#pragma once

namespace python {

/*
 Define the set of bytes codes
 The first macro parameter, V, is for primary names
 of byte codes and the second ALIAS is used to capture a value
 or position in the enumeration as a secondary name.  The third
 parameter is the handler for the bytecode.
 */
#define FOREACH_BYTECODE(V)                                          \
  V(UNUSED_BYTECODE_0, 0, interpreter::INVALID_BYTECODE)             \
  V(POP_TOP, 1, interpreter::POP_TOP)                                \
  V(ROT_TWO, 2, interpreter::ROT_TWO)                                \
  V(ROT_THREE, 3, interpreter::NOT_IMPLEMENTED)                      \
  V(DUP_TOP, 4, interpreter::DUP_TOP)                                \
  V(DUP_TOP_TWO, 5, interpreter::NOT_IMPLEMENTED)                    \
  V(UNUSED_BYTECODE_6, 6, interpreter::INVALID_BYTECODE)             \
  V(UNUSED_BYTECODE_7, 7, interpreter::INVALID_BYTECODE)             \
  V(UNUSED_BYTECODE_8, 8, interpreter::INVALID_BYTECODE)             \
  V(NOP, 9, interpreter::NOT_IMPLEMENTED)                            \
  V(UNARY_POSITIVE, 10, interpreter::NOT_IMPLEMENTED)                \
  V(UNARY_NEGATIVE, 11, interpreter::NOT_IMPLEMENTED)                \
  V(UNARY_NOT, 12, interpreter::UNARY_NOT)                           \
  V(UNUSED_BYTECODE_13, 13, interpreter::INVALID_BYTECODE)           \
  V(UNUSED_BYTECODE_14, 14, interpreter::INVALID_BYTECODE)           \
  V(UNARY_INVERT, 15, interpreter::NOT_IMPLEMENTED)                  \
  V(BINARY_MATRIX_MULTIPLY, 16, interpreter::NOT_IMPLEMENTED)        \
  V(INPLACE_MATRIX_MULTIPLY, 17, interpreter::NOT_IMPLEMENTED)       \
  V(UNUSED_BYTECODE_18, 18, interpreter::INVALID_BYTECODE)           \
  V(BINARY_POWER, 19, interpreter::NOT_IMPLEMENTED)                  \
  V(BINARY_MULTIPLY, 20, interpreter::BINARY_MULTIPLY)               \
  V(UNUSED_BYTECODE_21, 21, interpreter::INVALID_BYTECODE)           \
  V(BINARY_MODULO, 22, interpreter::BINARY_MODULO)                   \
  V(BINARY_ADD, 23, interpreter::BINARY_ADD)                         \
  V(BINARY_SUBTRACT, 24, interpreter::BINARY_SUBTRACT)               \
  V(BINARY_SUBSCR, 25, interpreter::BINARY_SUBSCR)                   \
  V(BINARY_FLOOR_DIVIDE, 26, interpreter::BINARY_FLOOR_DIVIDE)       \
  V(BINARY_TRUE_DIVIDE, 27, interpreter::BINARY_TRUE_DIVIDE)         \
  V(INPLACE_FLOOR_DIVIDE, 28, interpreter::BINARY_FLOOR_DIVIDE)      \
  V(INPLACE_TRUE_DIVIDE, 29, interpreter::NOT_IMPLEMENTED)           \
  V(UNUSED_BYTECODE_30, 30, interpreter::INVALID_BYTECODE)           \
  V(UNUSED_BYTECODE_31, 31, interpreter::INVALID_BYTECODE)           \
  V(UNUSED_BYTECODE_32, 32, interpreter::INVALID_BYTECODE)           \
  V(UNUSED_BYTECODE_33, 33, interpreter::INVALID_BYTECODE)           \
  V(UNUSED_BYTECODE_34, 34, interpreter::INVALID_BYTECODE)           \
  V(UNUSED_BYTECODE_35, 35, interpreter::INVALID_BYTECODE)           \
  V(UNUSED_BYTECODE_36, 36, interpreter::INVALID_BYTECODE)           \
  V(UNUSED_BYTECODE_37, 37, interpreter::INVALID_BYTECODE)           \
  V(UNUSED_BYTECODE_38, 38, interpreter::INVALID_BYTECODE)           \
  V(UNUSED_BYTECODE_39, 39, interpreter::INVALID_BYTECODE)           \
  V(UNUSED_BYTECODE_40, 40, interpreter::INVALID_BYTECODE)           \
  V(UNUSED_BYTECODE_41, 41, interpreter::INVALID_BYTECODE)           \
  V(UNUSED_BYTECODE_42, 42, interpreter::INVALID_BYTECODE)           \
  V(UNUSED_BYTECODE_43, 43, interpreter::INVALID_BYTECODE)           \
  V(UNUSED_BYTECODE_44, 44, interpreter::INVALID_BYTECODE)           \
  V(UNUSED_BYTECODE_45, 45, interpreter::INVALID_BYTECODE)           \
  V(UNUSED_BYTECODE_46, 46, interpreter::INVALID_BYTECODE)           \
  V(UNUSED_BYTECODE_47, 47, interpreter::INVALID_BYTECODE)           \
  V(UNUSED_BYTECODE_48, 48, interpreter::INVALID_BYTECODE)           \
  V(UNUSED_BYTECODE_49, 49, interpreter::INVALID_BYTECODE)           \
  V(GET_AITER, 50, interpreter::NOT_IMPLEMENTED)                     \
  V(GET_ANEXT, 51, interpreter::NOT_IMPLEMENTED)                     \
  V(BEFORE_ASYNC_WITH, 52, interpreter::NOT_IMPLEMENTED)             \
  V(UNUSED_BYTECODE_53, 53, interpreter::INVALID_BYTECODE)           \
  V(UNUSED_BYTECODE_54, 54, interpreter::INVALID_BYTECODE)           \
  V(INPLACE_ADD, 55, interpreter::BINARY_ADD)                        \
  V(INPLACE_SUBTRACT, 56, interpreter::BINARY_SUBTRACT)              \
  V(INPLACE_MULTIPLY, 57, interpreter::BINARY_MULTIPLY)              \
  V(UNUSED_BYTECODE_58, 58, interpreter::INVALID_BYTECODE)           \
  V(INPLACE_MODULO, 59, interpreter::BINARY_MODULO)                  \
  V(STORE_SUBSCR, 60, interpreter::STORE_SUBSCR)                     \
  V(DELETE_SUBSCR, 61, interpreter::NOT_IMPLEMENTED)                 \
  V(BINARY_LSHIFT, 62, interpreter::NOT_IMPLEMENTED)                 \
  V(BINARY_RSHIFT, 63, interpreter::NOT_IMPLEMENTED)                 \
  V(BINARY_AND, 64, interpreter::BINARY_AND)                         \
  V(BINARY_XOR, 65, interpreter::BINARY_XOR)                         \
  V(BINARY_OR, 66, interpreter::NOT_IMPLEMENTED)                     \
  V(INPLACE_POWER, 67, interpreter::NOT_IMPLEMENTED)                 \
  V(GET_ITER, 68, interpreter::GET_ITER)                             \
  V(GET_YIELD_FROM_ITER, 69, interpreter::NOT_IMPLEMENTED)           \
  V(PRINT_EXPR, 70, interpreter::NOT_IMPLEMENTED)                    \
  V(LOAD_BUILD_CLASS, 71, interpreter::LOAD_BUILD_CLASS)             \
  V(YIELD_FROM, 72, interpreter::NOT_IMPLEMENTED)                    \
  V(GET_AWAITABLE, 73, interpreter::NOT_IMPLEMENTED)                 \
  V(UNUSED_BYTECODE_74, 74, interpreter::INVALID_BYTECODE)           \
  V(INPLACE_LSHIFT, 75, interpreter::NOT_IMPLEMENTED)                \
  V(INPLACE_RSHIFT, 76, interpreter::NOT_IMPLEMENTED)                \
  V(INPLACE_AND, 77, interpreter::BINARY_AND)                        \
  V(INPLACE_XOR, 78, interpreter::BINARY_XOR)                        \
  V(INPLACE_OR, 79, interpreter::NOT_IMPLEMENTED)                    \
  V(BREAK_LOOP, 80, interpreter::NOT_IMPLEMENTED)                    \
  V(WITH_CLEANUP_START, 81, interpreter::NOT_IMPLEMENTED)            \
  V(WITH_CLEANUP_FINISH, 82, interpreter::NOT_IMPLEMENTED)           \
  V(RETURN_VALUE, 83, interpreter::NOT_IMPLEMENTED)                  \
  V(IMPORT_STAR, 84, interpreter::NOT_IMPLEMENTED)                   \
  V(SETUP_ANNOTATIONS, 85, interpreter::NOT_IMPLEMENTED)             \
  V(YIELD_VALUE, 86, interpreter::NOT_IMPLEMENTED)                   \
  V(POP_BLOCK, 87, interpreter::POP_BLOCK)                           \
  V(END_FINALLY, 88, interpreter::NOT_IMPLEMENTED)                   \
  V(POP_EXCEPT, 89, interpreter::NOT_IMPLEMENTED)                    \
  V(STORE_NAME, 90, interpreter::STORE_NAME)                         \
  V(DELETE_NAME, 91, interpreter::NOT_IMPLEMENTED)                   \
  V(UNPACK_SEQUENCE, 92, interpreter::UNPACK_SEQUENCE)               \
  V(FOR_ITER, 93, interpreter::FOR_ITER)                             \
  V(UNPACK_EX, 94, interpreter::NOT_IMPLEMENTED)                     \
  V(STORE_ATTR, 95, interpreter::STORE_ATTR)                         \
  V(DELETE_ATTR, 96, interpreter::NOT_IMPLEMENTED)                   \
  V(STORE_GLOBAL, 97, interpreter::STORE_GLOBAL)                     \
  V(DELETE_GLOBAL, 98, interpreter::DELETE_GLOBAL)                   \
  V(UNUSED_BYTECODE_99, 99, interpreter::INVALID_BYTECODE)           \
  V(LOAD_CONST, 100, interpreter::LOAD_CONST)                        \
  V(LOAD_NAME, 101, interpreter::LOAD_NAME)                          \
  V(BUILD_TUPLE, 102, interpreter::BUILD_TUPLE)                      \
  V(BUILD_LIST, 103, interpreter::BUILD_LIST)                        \
  V(BUILD_SET, 104, interpreter::BUILD_SET)                          \
  V(BUILD_MAP, 105, interpreter::BUILD_MAP)                          \
  V(LOAD_ATTR, 106, interpreter::LOAD_ATTR)                          \
  V(COMPARE_OP, 107, interpreter::COMPARE_OP)                        \
  V(IMPORT_NAME, 108, interpreter::IMPORT_NAME)                      \
  V(IMPORT_FROM, 109, interpreter::NOT_IMPLEMENTED)                  \
  V(JUMP_FORWARD, 110, interpreter::JUMP_FORWARD)                    \
  V(JUMP_IF_FALSE_OR_POP, 111, interpreter::JUMP_IF_FALSE_OR_POP)    \
  V(JUMP_IF_TRUE_OR_POP, 112, interpreter::JUMP_IF_TRUE_OR_POP)      \
  V(JUMP_ABSOLUTE, 113, interpreter::JUMP_ABSOLUTE)                  \
  V(POP_JUMP_IF_FALSE, 114, interpreter::POP_JUMP_IF_FALSE)          \
  V(POP_JUMP_IF_TRUE, 115, interpreter::POP_JUMP_IF_TRUE)            \
  V(LOAD_GLOBAL, 116, interpreter::LOAD_GLOBAL)                      \
  V(UNUSED_BYTECODE_117, 117, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_118, 118, interpreter::INVALID_BYTECODE)         \
  V(CONTINUE_LOOP, 119, interpreter::NOT_IMPLEMENTED)                \
  V(SETUP_LOOP, 120, interpreter::SETUP_LOOP)                        \
  V(SETUP_EXCEPT, 121, interpreter::NOT_IMPLEMENTED)                 \
  V(SETUP_FINALLY, 122, interpreter::NOT_IMPLEMENTED)                \
  V(UNUSED_BYTECODE_123, 123, interpreter::INVALID_BYTECODE)         \
  V(LOAD_FAST, 124, interpreter::LOAD_FAST)                          \
  V(STORE_FAST, 125, interpreter::STORE_FAST)                        \
  V(DELETE_FAST, 126, interpreter::NOT_IMPLEMENTED)                  \
  V(STORE_ANNOTATION, 127, interpreter::NOT_IMPLEMENTED)             \
  V(UNUSED_BYTECODE_128, 128, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_129, 129, interpreter::INVALID_BYTECODE)         \
  V(RAISE_VARARGS, 130, interpreter::NOT_IMPLEMENTED)                \
  V(CALL_FUNCTION, 131, interpreter::CALL_FUNCTION)                  \
  V(MAKE_FUNCTION, 132, interpreter::MAKE_FUNCTION)                  \
  V(BUILD_SLICE, 133, interpreter::BUILD_SLICE)                      \
  V(UNUSED_BYTECODE_134, 134, interpreter::INVALID_BYTECODE)         \
  V(LOAD_CLOSURE, 135, interpreter::LOAD_CLOSURE)                    \
  V(LOAD_DEREF, 136, interpreter::LOAD_DEREF)                        \
  V(STORE_DEREF, 137, interpreter::STORE_DEREF)                      \
  V(DELETE_DEREF, 138, interpreter::NOT_IMPLEMENTED)                 \
  V(UNUSED_BYTECODE_139, 139, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_140, 140, interpreter::INVALID_BYTECODE)         \
  V(CALL_FUNCTION_KW, 141, interpreter::CALL_FUNCTION_KW)            \
  V(CALL_FUNCTION_EX, 142, interpreter::NOT_IMPLEMENTED)             \
  V(SETUP_WITH, 143, interpreter::NOT_IMPLEMENTED)                   \
  V(EXTENDED_ARG, 144, interpreter::NOT_IMPLEMENTED)                 \
  V(LIST_APPEND, 145, interpreter::LIST_APPEND)                      \
  V(SET_ADD, 146, interpreter::NOT_IMPLEMENTED)                      \
  V(MAP_ADD, 147, interpreter::NOT_IMPLEMENTED)                      \
  V(LOAD_CLASSDEREF, 148, interpreter::NOT_IMPLEMENTED)              \
  V(BUILD_LIST_UNPACK, 149, interpreter::NOT_IMPLEMENTED)            \
  V(BUILD_MAP_UNPACK, 150, interpreter::NOT_IMPLEMENTED)             \
  V(BUILD_MAP_UNPACK_WITH_CALL, 151, interpreter::NOT_IMPLEMENTED)   \
  V(BUILD_TUPLE_UNPACK, 152, interpreter::NOT_IMPLEMENTED)           \
  V(BUILD_SET_UNPACK, 153, interpreter::NOT_IMPLEMENTED)             \
  V(SETUP_ASYNC_WITH, 154, interpreter::NOT_IMPLEMENTED)             \
  V(FORMAT_VALUE, 155, interpreter::FORMAT_VALUE)                    \
  V(BUILD_CONST_KEY_MAP, 156, interpreter::BUILD_CONST_KEY_MAP)      \
  V(BUILD_STRING, 157, interpreter::BUILD_STRING)                    \
  V(BUILD_TUPLE_UNPACK_WITH_CALL, 158, interpreter::NOT_IMPLEMENTED) \
  V(UNUSED_BYTECODE_159, 159, interpreter::INVALID_BYTECODE)         \
  V(LOAD_METHOD, 160, interpreter::NOT_IMPLEMENTED)                  \
  V(CALL_METHOD, 161, interpreter::NOT_IMPLEMENTED)                  \
  V(UNUSED_BYTECODE_162, 162, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_163, 163, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_164, 164, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_165, 165, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_166, 166, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_167, 167, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_168, 168, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_169, 169, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_170, 170, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_171, 171, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_172, 172, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_173, 173, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_174, 174, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_175, 175, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_176, 176, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_177, 177, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_178, 178, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_179, 179, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_180, 180, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_181, 181, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_182, 182, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_183, 183, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_184, 184, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_185, 185, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_186, 186, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_187, 187, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_188, 188, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_189, 189, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_190, 190, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_191, 191, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_192, 192, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_193, 193, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_194, 194, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_195, 195, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_196, 196, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_197, 197, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_198, 198, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_199, 199, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_200, 200, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_201, 201, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_202, 202, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_203, 203, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_204, 204, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_205, 205, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_206, 206, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_207, 207, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_208, 208, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_209, 209, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_210, 210, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_211, 211, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_212, 212, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_213, 213, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_214, 214, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_215, 215, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_216, 216, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_217, 217, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_218, 218, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_219, 219, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_220, 220, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_221, 221, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_222, 222, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_223, 223, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_224, 224, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_225, 225, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_226, 226, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_227, 227, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_228, 228, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_229, 229, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_230, 230, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_231, 231, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_232, 232, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_233, 233, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_234, 234, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_235, 235, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_236, 236, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_237, 237, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_238, 238, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_239, 239, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_240, 240, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_241, 241, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_242, 242, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_243, 243, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_244, 244, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_245, 245, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_246, 246, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_247, 247, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_248, 248, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_249, 249, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_250, 250, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_251, 251, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_252, 252, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_253, 253, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_254, 254, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_255, 255, interpreter::INVALID_BYTECODE)         \
  V(UNUSED_BYTECODE_256, 256, interpreter::INVALID_BYTECODE)         \
  V(EXCEPT_HANDLER, 257, interpreter::INVALID_BYTECODE)

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
  FVC_MASK = 0x3,
  FVC_NONE = 0x0,
  FVC_STR = 0x1,
  FVC_REPR = 0x2,
  FVC_ASCII = 0x3,
  FVS_MASK = 0x4,
  FVS_HAVE_SPEC = 0x4
};

namespace bytecode {
const char* name(Bytecode);
}

} // namespace python
