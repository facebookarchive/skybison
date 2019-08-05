#include "bytecode.h"

#include "runtime.h"

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

struct RewrittenOp {
  Bytecode bc;
  int32_t arg;
  bool needs_inline_cache;
};

static void rewriteZeroArgMethodCallsUsingLoadMethodCached(
    const MutableBytes& bytecode) {
  word bytecode_length = bytecode.length();
  Bytecode prev = LOAD_METHOD;
  word prev_index = 0;
  for (word i = 0; i < bytecode_length;) {
    BytecodeOp op = nextBytecodeOp(bytecode, &i);
    if (prev == LOAD_ATTR_CACHED && op.bc == CALL_FUNCTION && op.arg == 0) {
      bytecode.byteAtPut(prev_index, LOAD_METHOD_CACHED);
      bytecode.byteAtPut(i - 2, CALL_METHOD);
    }
    prev = op.bc;
    prev_index = i - 2;
  }
}

static RewrittenOp rewriteOperation(const Function& function, BytecodeOp op) {
  auto cached_binop = [](Interpreter::BinaryOp bin_op) {
    return RewrittenOp{BINARY_OP_CACHED, static_cast<int32_t>(bin_op), true};
  };
  auto cached_inplace = [](Interpreter::BinaryOp bin_op) {
    return RewrittenOp{INPLACE_OP_CACHED, static_cast<int32_t>(bin_op), true};
  };
  switch (op.bc) {
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
      return RewrittenOp{BINARY_SUBSCR_CACHED, op.arg, true};
    case BINARY_SUBTRACT:
      return cached_binop(Interpreter::BinaryOp::SUB);
    case BINARY_TRUE_DIVIDE:
      return cached_binop(Interpreter::BinaryOp::TRUEDIV);
    case BINARY_XOR:
      return cached_binop(Interpreter::BinaryOp::XOR);
    case COMPARE_OP:
      switch (op.arg) {
        case CompareOp::LT:
        case CompareOp::LE:
        case CompareOp::EQ:
        case CompareOp::NE:
        case CompareOp::GT:
        case CompareOp::GE:
          return RewrittenOp{COMPARE_OP_CACHED, op.arg, true};
        case CompareOp::IS:
          return RewrittenOp{COMPARE_IS, 0, false};
        case CompareOp::IS_NOT:
          return RewrittenOp{COMPARE_IS_NOT, 0, false};
      }
      break;
    case FOR_ITER:
      return RewrittenOp{FOR_ITER_CACHED, op.arg, true};
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
    case LOAD_ATTR:
      return RewrittenOp{LOAD_ATTR_CACHED, op.arg, true};
    case LOAD_FAST: {
      CHECK(op.arg < Code::cast(function.code()).nlocals(),
            "unexpected local number");
      word total_locals = function.totalLocals();
      int32_t reverse_arg = total_locals - op.arg - 1;
      return RewrittenOp{LOAD_FAST_REVERSE, reverse_arg, false};
    }
    case LOAD_METHOD:
      return RewrittenOp{LOAD_METHOD_CACHED, op.arg, true};
    case STORE_ATTR:
      return RewrittenOp{STORE_ATTR_CACHED, op.arg, true};
    case STORE_FAST: {
      CHECK(op.arg < Code::cast(function.code()).nlocals(),
            "unexpected local number");
      word total_locals = function.totalLocals();
      int32_t reverse_arg = total_locals - op.arg - 1;
      return RewrittenOp{STORE_FAST_REVERSE, reverse_arg, false};
    }
    case LOAD_CONST: {
      RawObject arg_obj =
          Tuple::cast(Code::cast(function.code()).consts()).at(op.arg);
      if (!arg_obj.isHeapObject() &&
          // This condition is true only the object fits in a byte. Some
          // immediate values of SmallInt and SmallStr do not satify this
          // condition.
          arg_obj == objectFromOparg(opargFromObject(arg_obj))) {
        return RewrittenOp{LOAD_IMMEDIATE, opargFromObject(arg_obj), false};
      }
    } break;
    case BINARY_OP_CACHED:
    case COMPARE_OP_CACHED:
    case FOR_ITER_CACHED:
    case INPLACE_OP_CACHED:
    case LOAD_ATTR_CACHED:
    case LOAD_FAST_REVERSE:
    case LOAD_METHOD_CACHED:
    case STORE_ATTR_CACHED:
    case STORE_FAST_REVERSE:
      UNREACHABLE("should not have cached opcode in input");
    default:
      break;
  }
  return RewrittenOp{op.bc, op.arg, false};
}

void rewriteBytecode(Thread* thread, const Function& function) {
  HandleScope scope(thread);
  word num_caches = 0;

  // Scan bytecode to figure out how many caches we need.
  MutableBytes bytecode(&scope, function.rewrittenBytecode());
  word bytecode_length = bytecode.length();
  for (word i = 0; i < bytecode_length;) {
    BytecodeOp op = nextBytecodeOp(bytecode, &i);
    RewrittenOp rewritten = rewriteOperation(function, op);
    if (rewritten.needs_inline_cache) {
      num_caches++;
    }
  }

  // Add cache entries for global variables.  This is going to over allocate
  // somewhat in order to simplify the indexing arithmetic.  Not all names are
  // used for globals, some are used for attributes.  This is good enough for
  // now.
  word names_length = Tuple::cast(Code::cast(function.code()).names()).length();
  word num_global_caches = Utils::roundUpDiv(names_length, kIcPointersPerCache);
  num_caches += num_global_caches;

  Runtime* runtime = thread->runtime();
  Tuple original_arguments(&scope, runtime->newTuple(num_caches));
  for (word i = 0, cache = num_global_caches; i < bytecode_length;) {
    word begin = i;
    BytecodeOp op = nextBytecodeOp(bytecode, &i);
    RewrittenOp rewritten = rewriteOperation(function, op);
    if (rewritten.needs_inline_cache) {
      // Replace opcode arg with a cache index and zero EXTENDED_ARG args.
      CHECK(cache < 256, "more than 256 entries may require bytecode resizing");
      for (word j = begin; j < i - 2; j += 2) {
        bytecode.byteAtPut(j, static_cast<byte>(Bytecode::EXTENDED_ARG));
        bytecode.byteAtPut(j + 1, 0);
      }
      bytecode.byteAtPut(i - 2, static_cast<byte>(rewritten.bc));
      bytecode.byteAtPut(i - 1, static_cast<byte>(cache));

      // Remember original arg.
      original_arguments.atPut(cache, SmallInt::fromWord(rewritten.arg));
      cache++;
    } else if (rewritten.arg != op.arg || rewritten.bc != op.bc) {
      CHECK(rewritten.arg < 256,
            "more than 256 entries may require bytecode resizing");
      for (word j = begin; j < i - 2; j += 2) {
        bytecode.byteAtPut(j, static_cast<byte>(Bytecode::EXTENDED_ARG));
        bytecode.byteAtPut(j + 1, 0);
      }
      bytecode.byteAtPut(i - 2, static_cast<byte>(rewritten.bc));
      bytecode.byteAtPut(i - 1, static_cast<byte>(rewritten.arg));
    }
  }

  // TODO(T45428069): Remove this once the compiler starts emitting the opcodes.
  rewriteZeroArgMethodCallsUsingLoadMethodCached(bytecode);
  function.setCaches(runtime->newTuple(num_caches * kIcPointersPerCache));
  function.setOriginalArguments(*original_arguments);
}

}  // namespace python
