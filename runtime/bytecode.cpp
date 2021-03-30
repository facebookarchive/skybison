#include "bytecode.h"

#include "ic.h"
#include "runtime.h"

namespace py {

const char* const kBytecodeNames[] = {
#define NAME(name, value, handler) #name,
    FOREACH_BYTECODE(NAME)
#undef NAME
};

BytecodeOp nextBytecodeOp(const MutableBytes& bytecode, word* index) {
  word i = *index;
  Bytecode bc = rewrittenBytecodeOpAt(bytecode, i);
  int32_t arg = rewrittenBytecodeArgAt(bytecode, i);
  uint16_t cache = rewrittenBytecodeCacheAt(bytecode, i);
  i++;
  while (bc == Bytecode::EXTENDED_ARG) {
    bc = rewrittenBytecodeOpAt(bytecode, i);
    arg = (arg << kBitsPerByte) | rewrittenBytecodeArgAt(bytecode, i);
    i++;
  }
  DCHECK(i - *index <= 8, "EXTENDED_ARG-encoded arg must fit in int32_t");
  *index = i;
  return BytecodeOp{bc, arg, cache};
}

const word kOpcodeOffset = 0;
const word kArgOffset = 1;
const word kCacheOffset = 2;

word bytecodeLength(const Bytes& bytecode) {
  return bytecode.length() / kCompilerCodeUnitSize;
}

Bytecode bytecodeOpAt(const Bytes& bytecode, word index) {
  return static_cast<Bytecode>(
      bytecode.byteAt(index * kCompilerCodeUnitSize + kOpcodeOffset));
}

byte bytecodeArgAt(const Bytes& bytecode, word index) {
  return bytecode.byteAt(index * kCompilerCodeUnitSize + kArgOffset);
}

word rewrittenBytecodeLength(const MutableBytes& bytecode) {
  return bytecode.length() / kCodeUnitSize;
}

Bytecode rewrittenBytecodeOpAt(const MutableBytes& bytecode, word index) {
  return static_cast<Bytecode>(
      bytecode.byteAt(index * kCodeUnitSize + kOpcodeOffset));
}

void rewrittenBytecodeOpAtPut(const MutableBytes& bytecode, word index,
                              Bytecode op) {
  bytecode.byteAtPut(index * kCodeUnitSize + kOpcodeOffset,
                     static_cast<byte>(op));
}

byte rewrittenBytecodeArgAt(const MutableBytes& bytecode, word index) {
  return bytecode.byteAt(index * kCodeUnitSize + kArgOffset);
}

void rewrittenBytecodeArgAtPut(const MutableBytes& bytecode, word index,
                               byte arg) {
  bytecode.byteAtPut(index * kCodeUnitSize + kArgOffset, arg);
}

uint16_t rewrittenBytecodeCacheAt(const MutableBytes& bytecode, word index) {
  return bytecode.uint16At(index * kCodeUnitSize + kCacheOffset);
}

void rewrittenBytecodeCacheAtPut(const MutableBytes& bytecode, word index,
                                 uint16_t cache) {
  bytecode.uint16AtPut(index * kCodeUnitSize + kCacheOffset, cache);
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

static RewrittenOp rewriteOperation(const Function& function, BytecodeOp op,
                                    bool use_load_fast_reverse_unchecked) {
  auto cached_binop = [](Interpreter::BinaryOp bin_op) {
    return RewrittenOp{BINARY_OP_ANAMORPHIC, static_cast<int32_t>(bin_op),
                       true};
  };
  auto cached_inplace = [](Interpreter::BinaryOp bin_op) {
    return RewrittenOp{INPLACE_OP_ANAMORPHIC, static_cast<int32_t>(bin_op),
                       true};
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
      return RewrittenOp{BINARY_SUBSCR_ANAMORPHIC, op.arg, true};
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
          return RewrittenOp{COMPARE_OP_ANAMORPHIC, op.arg, true};
        case CompareOp::IN:
          return RewrittenOp{COMPARE_IN_ANAMORPHIC, 0, true};
        // TODO(T61327107): Implement COMPARE_NOT_IN.
        case CompareOp::IS:
          return RewrittenOp{COMPARE_IS, 0, false};
        case CompareOp::IS_NOT:
          return RewrittenOp{COMPARE_IS_NOT, 0, false};
      }
      break;
    case CALL_FUNCTION:
      return RewrittenOp{CALL_FUNCTION_ANAMORPHIC, op.arg, true};
    case FOR_ITER:
      return RewrittenOp{FOR_ITER_ANAMORPHIC, op.arg, true};
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
      return RewrittenOp{LOAD_ATTR_ANAMORPHIC, op.arg, true};
    case LOAD_FAST: {
      CHECK(op.arg < Code::cast(function.code()).nlocals(),
            "unexpected local number");
      word total_locals = function.totalLocals();
      int32_t reverse_arg = total_locals - op.arg - 1;
      // TODO(T66255738): Use a more complete static analysis to capture all
      // bound local variables other than just arguments.
      return RewrittenOp{
          // Arguments are always bound.
          (op.arg < function.totalArgs() && use_load_fast_reverse_unchecked)
              ? LOAD_FAST_REVERSE_UNCHECKED
              : LOAD_FAST_REVERSE,
          reverse_arg, false};
    }
    case LOAD_METHOD:
      return RewrittenOp{LOAD_METHOD_ANAMORPHIC, op.arg, true};
    case STORE_ATTR:
      return RewrittenOp{STORE_ATTR_ANAMORPHIC, op.arg, true};
    case STORE_FAST: {
      CHECK(op.arg < Code::cast(function.code()).nlocals(),
            "unexpected local number");
      word total_locals = function.totalLocals();
      int32_t reverse_arg = total_locals - op.arg - 1;
      return RewrittenOp{STORE_FAST_REVERSE, reverse_arg, false};
    }
    case STORE_SUBSCR:
      return RewrittenOp{STORE_SUBSCR_ANAMORPHIC, op.arg, true};
    case LOAD_CONST: {
      RawObject arg_obj =
          Tuple::cast(Code::cast(function.code()).consts()).at(op.arg);
      if (!arg_obj.isHeapObject()) {
        if (arg_obj.isBool()) {
          // We encode true/false not as 1/0 but as 0x80/0 to save an x86
          // assembly instruction; moving the value to the 2nd byte can be done
          // with a multiplication by 2 as part of an address expression rather
          // than needing a separate shift by 8 in the 1/0 variant.
          return RewrittenOp{LOAD_BOOL, Bool::cast(arg_obj).value() ? 0x80 : 0,
                             false};
        }
        // This condition is true only the object fits in a byte. Some
        // immediate values of SmallInt and SmallStr do not satify this
        // condition.
        if (arg_obj == objectFromOparg(opargFromObject(arg_obj))) {
          return RewrittenOp{LOAD_IMMEDIATE, opargFromObject(arg_obj), false};
        }
      }
    } break;
    case BINARY_OP_ANAMORPHIC:
    case COMPARE_OP_ANAMORPHIC:
    case FOR_ITER_ANAMORPHIC:
    case INPLACE_OP_ANAMORPHIC:
    case LOAD_ATTR_ANAMORPHIC:
    case LOAD_FAST_REVERSE:
    case LOAD_METHOD_ANAMORPHIC:
    case STORE_ATTR_ANAMORPHIC:
    case STORE_FAST_REVERSE:
      UNREACHABLE("should not have cached opcode in input");
    default:
      break;
  }
  return RewrittenOp{UNUSED_BYTECODE_0, 0, false};
}

static const word kMaxCaches = 65536;

void rewriteBytecode(Thread* thread, const Function& function) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  // Add cache entries for global variables.
  // TODO(T58223091): This is going to over allocate somewhat in order
  // to simplify the indexing arithmetic.  Not all names are used for
  // globals, some are used for attributes.  This is good enough for
  // now.
  word names_length = Tuple::cast(Code::cast(function.code()).names()).length();
  word num_global_caches = Utils::roundUpDiv(names_length, kIcPointersPerEntry);
  if (!function.hasOptimizedOrNewlocals()) {
    if (num_global_caches > 0) {
      MutableTuple caches(&scope, runtime->newMutableTuple(
                                      num_global_caches * kIcPointersPerEntry));
      caches.fill(NoneType::object());
      function.setCaches(*caches);
    }
    return;
  }
  MutableBytes bytecode(&scope, function.rewrittenBytecode());
  word num_opcodes = rewrittenBytecodeLength(bytecode);
  bool use_load_fast_reverse_unchecked = true;
  // Scan bytecode to figure out how many caches we need and if we can use
  // LOAD_FAST_REVERSE_UNCHECKED.
  word num_caches = num_global_caches;
  for (word i = 0; i < num_opcodes;) {
    BytecodeOp op = nextBytecodeOp(bytecode, &i);
    if (op.bc == DELETE_FAST) {
      use_load_fast_reverse_unchecked = false;
      continue;
    }
    RewrittenOp rewritten = rewriteOperation(function, op, false);
    if (rewritten.needs_inline_cache) {
      num_caches++;
    }
  }
  if (num_caches > kMaxCaches) {
    // Populate global variable caches unconditionally since the interpreter
    // assumes their existence.
    if (num_global_caches > 0) {
      MutableTuple caches(&scope, runtime->newMutableTuple(
                                      num_global_caches * kIcPointersPerEntry));
      caches.fill(NoneType::object());
      function.setCaches(*caches);
    }
    return;
  }
  word cache = num_global_caches;
  for (word i = 0; i < num_opcodes;) {
    BytecodeOp op = nextBytecodeOp(bytecode, &i);
    word previous_index = i - 1;
    RewrittenOp rewritten =
        rewriteOperation(function, op, use_load_fast_reverse_unchecked);
    if (rewritten.bc == UNUSED_BYTECODE_0) continue;
    if (rewritten.needs_inline_cache) {
      rewrittenBytecodeOpAtPut(bytecode, previous_index, rewritten.bc);
      rewrittenBytecodeArgAtPut(bytecode, previous_index,
                                static_cast<byte>(rewritten.arg));
      rewrittenBytecodeCacheAtPut(bytecode, previous_index, cache);

      cache++;
    } else if (rewritten.arg != op.arg || rewritten.bc != op.bc) {
      rewrittenBytecodeOpAtPut(bytecode, previous_index, rewritten.bc);
      rewrittenBytecodeArgAtPut(bytecode, previous_index,
                                static_cast<byte>(rewritten.arg));
    }
  }
  DCHECK(cache == num_caches, "cache size mismatch");
  if (cache > 0) {
    MutableTuple caches(&scope,
                        runtime->newMutableTuple(cache * kIcPointersPerEntry));
    caches.fill(NoneType::object());
    function.setCaches(*caches);
  }
}

}  // namespace py
