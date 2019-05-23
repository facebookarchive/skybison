#include "ic.h"

#include "bytearray-builtins.h"
#include "bytecode.h"
#include "runtime.h"
#include "utils.h"

namespace python {

struct BytecodeArgPair {
  Bytecode bc;
  word arg;
};

static BytecodeArgPair rewriteOperation(Bytecode bc, word arg) {
  auto cached_binop = [](Interpreter::BinaryOp op) {
    return BytecodeArgPair{BINARY_OP_CACHED, static_cast<word>(op)};
  };
  auto cached_inplace = [](Interpreter::BinaryOp op) {
    return BytecodeArgPair{INPLACE_OP_CACHED, static_cast<word>(op)};
  };
  switch (bc) {
    // Binary operations.
    case BINARY_ADD:
      return cached_binop(Interpreter::BinaryOp::ADD);
    case BINARY_AND:
      return cached_binop(Interpreter::BinaryOp::AND);
    case BINARY_FLOOR_DIVIDE:
      return cached_binop(Interpreter::BinaryOp::FLOORDIV);
    case BINARY_LSHIFT:
      return cached_binop(Interpreter::BinaryOp::LSHIFT);
    case BINARY_MATRIX_MULTIPLY:
      return cached_binop(Interpreter::BinaryOp::MATMUL);
    case BINARY_MODULO:
      return cached_binop(Interpreter::BinaryOp::MOD);
    case BINARY_MULTIPLY:
      return cached_binop(Interpreter::BinaryOp::MUL);
    case BINARY_OR:
      return cached_binop(Interpreter::BinaryOp::OR);
    case BINARY_POWER:
      return cached_binop(Interpreter::BinaryOp::POW);
    case BINARY_RSHIFT:
      return cached_binop(Interpreter::BinaryOp::RSHIFT);
    case BINARY_SUBSCR:
      return BytecodeArgPair{BINARY_SUBSCR_CACHED, arg};
    case BINARY_SUBTRACT:
      return cached_binop(Interpreter::BinaryOp::SUB);
    case BINARY_TRUE_DIVIDE:
      return cached_binop(Interpreter::BinaryOp::TRUEDIV);
    case BINARY_XOR:
      return cached_binop(Interpreter::BinaryOp::XOR);
    case COMPARE_OP:
      switch (arg) {
        case CompareOp::LT:
        case CompareOp::LE:
        case CompareOp::EQ:
        case CompareOp::NE:
        case CompareOp::GT:
        case CompareOp::GE:
          return BytecodeArgPair{COMPARE_OP_CACHED, arg};
      }
      break;
    // Inplace operations.
    case INPLACE_ADD:
      return cached_inplace(Interpreter::BinaryOp::ADD);
    case INPLACE_AND:
      return cached_inplace(Interpreter::BinaryOp::AND);
    case INPLACE_FLOOR_DIVIDE:
      return cached_inplace(Interpreter::BinaryOp::FLOORDIV);
    case INPLACE_LSHIFT:
      return cached_inplace(Interpreter::BinaryOp::LSHIFT);
    case INPLACE_MATRIX_MULTIPLY:
      return cached_inplace(Interpreter::BinaryOp::MATMUL);
    case INPLACE_MODULO:
      return cached_inplace(Interpreter::BinaryOp::MOD);
    case INPLACE_MULTIPLY:
      return cached_inplace(Interpreter::BinaryOp::MUL);
    case INPLACE_OR:
      return cached_inplace(Interpreter::BinaryOp::OR);
    case INPLACE_POWER:
      return cached_inplace(Interpreter::BinaryOp::POW);
    case INPLACE_RSHIFT:
      return cached_inplace(Interpreter::BinaryOp::RSHIFT);
    case INPLACE_SUBTRACT:
      return cached_inplace(Interpreter::BinaryOp::SUB);
    case INPLACE_TRUE_DIVIDE:
      return cached_inplace(Interpreter::BinaryOp::TRUEDIV);
    case INPLACE_XOR:
      return cached_inplace(Interpreter::BinaryOp::XOR);
    // Attribute accessors.
    case LOAD_ATTR:
      return BytecodeArgPair{LOAD_ATTR_CACHED, arg};
    case STORE_ATTR:
      return BytecodeArgPair{STORE_ATTR_CACHED, arg};

    case BINARY_OP_CACHED:
    case COMPARE_OP_CACHED:
    case INPLACE_OP_CACHED:
    case LOAD_ATTR_CACHED:
    case STORE_ATTR_CACHED:
      UNREACHABLE("should not have cached opcode in input");
    default:
      break;
  }
  return BytecodeArgPair{bc, arg};
}

void icRewriteBytecode(Thread* thread, const Function& function) {
  HandleScope scope(thread);
  word num_caches = 0;

  // Scan bytecode to figure out how many caches we need.
  Bytes bytecode(&scope, Code::cast(function.code()).code());
  word bytecode_length = bytecode.length();
  for (word i = 0; i < bytecode_length;) {
    Bytecode bc = static_cast<Bytecode>(bytecode.byteAt(i++));
    int32_t arg = bytecode.byteAt(i++);
    while (bc == Bytecode::EXTENDED_ARG) {
      bc = static_cast<Bytecode>(bytecode.byteAt(i++));
      arg = (arg << kBitsPerByte) | bytecode.byteAt(i++);
    }

    BytecodeArgPair rewritten = rewriteOperation(bc, arg);
    if (rewritten.bc != bc) {
      num_caches++;
    }
  }

  Runtime* runtime = thread->runtime();
  Tuple original_arguments(&scope, runtime->newTuple(num_caches));

  // Rewrite bytecode.
  ByteArray result(&scope, runtime->newByteArray());
  runtime->byteArrayEnsureCapacity(thread, result, bytecode_length);
  result.setNumItems(bytecode_length);
  for (word i = 0, cache = 0; i < bytecode_length;) {
    word begin = i;
    Bytecode bc = static_cast<Bytecode>(bytecode.byteAt(i++));
    int32_t arg = bytecode.byteAt(i++);
    while (bc == Bytecode::EXTENDED_ARG) {
      bc = static_cast<Bytecode>(bytecode.byteAt(i++));
      arg = (arg << kBitsPerByte) | bytecode.byteAt(i++);
    }

    BytecodeArgPair rewritten = rewriteOperation(bc, arg);
    if (rewritten.bc != bc) {
      // Replace opcode arg with a cache index and zero EXTENDED_ARG args.
      CHECK(cache < 256,
            "more than 256 entries may require bytecode stretching");
      for (word j = begin; j < i - 2; j += 2) {
        result.byteAtPut(j, static_cast<byte>(Bytecode::EXTENDED_ARG));
        result.byteAtPut(j + 1, 0);
      }
      result.byteAtPut(i - 2, static_cast<byte>(rewritten.bc));
      result.byteAtPut(i - 1, static_cast<byte>(cache));

      // Remember original arg.
      original_arguments.atPut(cache, SmallInt::fromWord(rewritten.arg));
      cache++;
    } else {
      for (word j = begin; j < i; j++) {
        result.byteAtPut(j, bytecode.byteAt(j));
      }
    }
  }

  function.setCaches(runtime->newTuple(num_caches * kIcPointersPerCache));
  function.setRewrittenBytecode(byteArrayAsBytes(thread, runtime, result));
  function.setOriginalArguments(*original_arguments);
}

void icUpdate(Thread* thread, const Tuple& caches, word index,
              LayoutId layout_id, const Object& value) {
  HandleScope scope(thread);
  SmallInt key(&scope, SmallInt::fromWord(static_cast<word>(layout_id)));
  Object entry_key(&scope, NoneType::object());
  for (word i = index * kIcPointersPerCache, end = i + kIcPointersPerCache;
       i < end; i += kIcPointersPerEntry) {
    entry_key = caches.at(i + kIcEntryKeyOffset);
    if (entry_key.isNoneType() || entry_key == key) {
      caches.atPut(i + kIcEntryKeyOffset, *key);
      caches.atPut(i + kIcEntryValueOffset, *value);
      return;
    }
  }
}

void icUpdateBinop(Thread* thread, const Tuple& caches, word index,
                   LayoutId left_layout_id, LayoutId right_layout_id,
                   const Object& value, IcBinopFlags flags) {
  HandleScope scope(thread);
  word key_high_bits = static_cast<word>(left_layout_id)
                           << Header::kLayoutIdBits |
                       static_cast<word>(right_layout_id);
  Object entry_key(&scope, NoneType::object());
  for (word i = index * kIcPointersPerCache, end = i + kIcPointersPerCache;
       i < end; i += kIcPointersPerEntry) {
    entry_key = caches.at(i + kIcEntryKeyOffset);
    if (entry_key.isNoneType() ||
        SmallInt::cast(*entry_key).value() >> kBitsPerByte == key_high_bits) {
      caches.atPut(i + kIcEntryKeyOffset,
                   SmallInt::fromWord(key_high_bits << kBitsPerByte |
                                      static_cast<word>(flags)));
      caches.atPut(i + kIcEntryValueOffset, *value);
    }
  }
}

}  // namespace python
