#include "interpreter-gen.h"

#include <cinttypes>
#include <cstdio>
#include <cstring>

#include "assembler-x64.h"
#include "bytecode.h"
#include "frame.h"
#include "ic.h"
#include "interpreter.h"
#include "memory-region.h"
#include "os.h"
#include "runtime.h"
#include "thread.h"

// This file generates an assembly version of our interpreter. The default
// implementation for all opcodes calls back to the C++ version, with
// hand-written assembly versions of perf-critical opcodes. More details are
// inline with the relevant constants and functions.
//
// Assumptions made throughout this file without static_assert()ing every single
// time:
// - PC (as an offset) fits in a uint32_t.
// - Immediate objects fit in 8 bits.

namespace py {

namespace {

using namespace x64;

// Abbreviated x86-64 ABI:
constexpr Register kArgRegs[] = {RDI, RSI, RDX, RCX, R8, R9};

// Currently unused in code, but kept around for reference:
// return registers: {RAX, RDX}
// callee-saved registers: {RSP, RBP, RBX, R12, R13, R14, R15}
// caller-saved registers: {RAX, RCX, RDX, RDI, RSI, R8, R9, R10, R11}

// During normal execution, the following values are live:

// Current bytecode, a RawMutableBytes.
const Register kBCReg = RCX;

// Current PC, as an index into the bytecode.
const Register kPCReg = R14;

// Current opcode argument, as a uint32_t.
const Register kOpargReg = kArgRegs[1];

// Current Frame*.
const Register kFrameReg = RBX;

// Current Thread*.
const Register kThreadReg = R12;

// Handler base address (see below for more about the handlers).
const Register kHandlersBaseReg = R13;

// The native frame/stack looks like this:
// +-------------+
// | return addr |
// | saved %rbp  | <- %rbp
// | ...         |
// | ...         | <- callee-saved registers
// | ...         |
// | entry_frame |
// | padding     | <- native %rsp, when materialized for a C++ call
// +-------------+
constexpr Register kUsedCalleeSavedRegs[] = {RBX, R12, R13, R14};
const word kNumCalleeSavedRegs = ARRAYSIZE(kUsedCalleeSavedRegs);
const word kEntryFrameOffset = -(kNumCalleeSavedRegs + 1) * kPointerSize;
const word kPaddingBytes = (kEntryFrameOffset % 16) == 0 ? 0 : kPointerSize;
const word kNativeStackFrameSize = -kEntryFrameOffset + kPaddingBytes;
static_assert(kNativeStackFrameSize % 16 == 0,
              "native frame size must be multiple of 16");

// The interpreter code itself is a prologue followed by an array of
// regularly-sized opcode handlers, spaced such that the address of a handler
// can be computed with a base address and the opcode's value. A few special
// pseudo-handlers are at negative offsets from the base address, which are used
// to handle control flow such as exceptions and returning.
//
// +----------------------+
// | prologue, setup code | <- interpreter entry point
// |----------------------+
// | UNWIND handler       | <- handlers_base - 3 * kHandlerSize
// +----------------------+
// | RETURN handler       | <- handlers_base - 2 * kHandlerSize
// +----------------------+
// | YIELD handler        | <- handlers_base - 1 * kHandlerSize
// +----------------------+
// | opcode 0 handler     | <- handlers_base + 0 * kHandlerSize
// +----------------------+
// | etc...               |
// +----------------------+
// | opcode 255 handler   | <- handlers_base + 255 * kHandlerSize
// +----------------------+
const word kHandlerSizeShift = 8;
const word kHandlerSize = 1 << kHandlerSizeShift;

const Interpreter::OpcodeHandler kCppHandlers[] = {
#define OP(name, id, handler) Interpreter::handler,
    FOREACH_BYTECODE(OP)
#undef OP
};

// Environment shared by all emit functions.
struct EmitEnv {
  Assembler as;
  Bytecode current_op;
  const char* current_handler;
  Label call_handlers[kNumBytecodes];
};

// This macro helps instruction-emitting code stand out while staying compact.
#define __ env->as.

// RAII helper to ensure that a region of code is nop-padded to a specific size,
// with checks that it doesn't overflow the limit.
class HandlerSizer {
 public:
  HandlerSizer(EmitEnv* env, word size)
      : env_(env), size_(size), start_cursor_(env->as.codeSize()) {}

  ~HandlerSizer() {
    word padding = start_cursor_ + size_ - env_->as.codeSize();
    CHECK(padding >= 0, "Handler for %s overflowed by %" PRIdPTR " bytes",
          env_->current_handler, -padding);
    env_->as.nops(padding);
  }

 private:
  EmitEnv* env_;
  word size_;
  word start_cursor_;
};

// Shorthand for the Immediate corresponding to a Bool value.
Immediate boolImmediate(bool value) {
  return Immediate(Bool::fromBool(value).raw());
}

// The offset to use to access a given offset with a HeapObject, accounting for
// the tag bias.
int32_t heapObjectDisp(int32_t offset) {
  return -Object::kHeapObjectTag + offset;
}

// Load the next opcode, advance PC, and jump to the appropriate handler.
void emitNextOpcode(EmitEnv* env) {
  Register r_scratch = RAX;
  __ movzbl(r_scratch, Address(kBCReg, kPCReg, TIMES_1, heapObjectDisp(0)));
  __ movzbl(kOpargReg, Address(kBCReg, kPCReg, TIMES_1, heapObjectDisp(1)));
  __ addl(kPCReg, Immediate(2));
  __ shll(r_scratch, Immediate(kHandlerSizeShift));
  __ addq(r_scratch, kHandlersBaseReg);
  __ jmp(r_scratch);
  // Hint to the branch predictor that the indirect jmp never falls through to
  // here.
  __ ud2();
}

enum SaveRestoreFlags {
  kVMStack = 1 << 0,
  kVMFrame = 1 << 1,
  kBytecode = 1 << 2,
  kVMPC = 1 << 3,
  kAllState = kVMStack | kVMFrame | kBytecode | kVMPC,
};

SaveRestoreFlags operator|(SaveRestoreFlags a, SaveRestoreFlags b) {
  return static_cast<SaveRestoreFlags>(static_cast<int>(a) |
                                       static_cast<int>(b));
}

void emitSaveInterpreterState(EmitEnv* env, SaveRestoreFlags flags) {
  if (flags & kVMFrame) {
    __ movq(Address(kThreadReg, Thread::currentFrameOffset()), kFrameReg);
  }
  if (flags & kVMStack) {
    __ movq(Address(kFrameReg, Frame::kValueStackTopOffset), RSP);
    __ leaq(RSP, Address(RBP, -kNativeStackFrameSize));
  }
  DCHECK((flags & kBytecode) == 0, "Storing bytecode not supported");
  if (flags & kVMPC) {
    __ movq(Address(kFrameReg, Frame::kVirtualPCOffset), kPCReg);
  }
}

void emitRestoreInterpreterState(EmitEnv* env, SaveRestoreFlags flags) {
  if (flags & kVMFrame) {
    __ movq(kFrameReg, Address(kThreadReg, Thread::currentFrameOffset()));
  }
  if (flags & kVMStack) {
    __ movq(RSP, Address(kFrameReg, Frame::kValueStackTopOffset));
  }
  if (flags & kBytecode) {
    __ movq(kBCReg, Address(kFrameReg, Frame::kBytecodeOffset));
  }
  if (flags & kVMPC) {
    __ movl(kPCReg, Address(kFrameReg, Frame::kVirtualPCOffset));
  }
}

bool mayChangeFramePC(Bytecode bc) {
  // These opcodes have been manually vetted to ensure that they don't change
  // the current frame or PC (or if they do, it's through something like
  // Interpreter::callMethodN(), which restores the previous frame when it's
  // finished). This lets us avoid reloading the frame after calling their C++
  // implementations.
  switch (bc) {
    case LOAD_ATTR_CACHED:
    case STORE_ATTR_CACHED:
    case LOAD_METHOD_CACHED:
      return false;
    default:
      return true;
  }
}

// Emit a call to the C++ implementation of the given Bytecode, saving and
// restoring appropriate interpreter state before and after the call. This code
// is emitted as a series of stubs after the main set of handlers; it's used
// from the hot path with emitJumpToGenericHandler().
void emitGenericHandler(EmitEnv* env, Bytecode bc) {
  __ movq(kArgRegs[0], kThreadReg);
  static_assert(kOpargReg == kArgRegs[1], "oparg expect to be in rsi");

  // Sync VM state to memory and restore native stack pointer.
  emitSaveInterpreterState(env, kVMPC | kVMStack | kVMFrame);

  // TODO(bsimmers): Augment Assembler so we can use the 0xe8 call opcode when
  // possible (this will also have implications on our allocation strategy).
  __ movq(RAX, Immediate(reinterpret_cast<int64_t>(kCppHandlers[bc])));
  __ call(RAX);

  Label handle_flow;
  static_assert(static_cast<int>(Interpreter::Continue::NEXT) == 0,
                "NEXT must be 0");
  __ testl(RAX, RAX);
  __ jcc(NOT_ZERO, &handle_flow, Assembler::kNearJump);

  emitRestoreInterpreterState(
      env, mayChangeFramePC(bc) ? kAllState : (kVMStack | kBytecode));
  emitNextOpcode(env);

  __ bind(&handle_flow);
  __ shll(RAX, Immediate(kHandlerSizeShift));
  __ leaq(RAX, Address(kHandlersBaseReg, RAX, TIMES_1,
                       -Interpreter::kNumContinues * kHandlerSize));
  __ jmp(RAX);
}

// Jump to the generic handler for the Bytecode being currently emitted.
void emitJumpToGenericHandler(EmitEnv* env) {
  __ jmp(&env->call_handlers[env->current_op], Assembler::kFarJump);
}

// Fallback handler for all unimplemented opcodes: call out to C++.
template <Bytecode bc>
void emitHandler(EmitEnv* env) {
  emitJumpToGenericHandler(env);
}

// Load the LayoutId of the RawObject in r_obj into r_dst.
//
// Writes to r_dst.
void emitGetLayoutId(EmitEnv* env, Register r_dst, Register r_obj) {
  Label done;
  Label immediate;

  static_assert(static_cast<int>(LayoutId::kSmallInt) == 0,
                "Expected SmallInt LayoutId to be 0");
  __ xorl(r_dst, r_dst);
  __ testq(r_obj, Immediate(Object::kSmallIntTagMask));
  __ jcc(ZERO, &done, Assembler::kNearJump);

  __ testq(r_obj,
           Immediate(Object::kPrimaryTagMask & ~Object::kSmallIntTagMask));
  __ jcc(NOT_ZERO, &immediate, Assembler::kNearJump);
  __ movq(r_dst, Address(r_obj, heapObjectDisp(HeapObject::kHeaderOffset)));
  __ shrl(r_dst, Immediate(Header::kLayoutIdOffset));
  __ andl(r_dst, Immediate(Header::kLayoutIdMask));
  __ jmp(&done, Assembler::kNearJump);

  __ bind(&immediate);
  __ movl(r_dst, r_obj);
  __ andl(r_dst, Immediate(Object::kImmediateTagMask));

  __ bind(&done);
}

// Convert the given register from an int to a SmallInt, assuming it fits.
void emitConvertToSmallInt(EmitEnv* env, Register reg) {
  static_assert(Object::kSmallIntTag == 0, "Unexpected SmallInt tag");
  __ shlq(reg, Immediate(Object::kSmallIntTagBits));
}

// Convert the given register from a SmallInt to an int.
void emitConvertFromSmallInt(EmitEnv* env, Register reg) {
  __ sarq(reg, Immediate(Object::kSmallIntTagBits));
}

// Look up an inline cache entry, like icLookup(). If found, the result will be
// stored in r_dst. If not found, r_dst will be unmodified and the code will
// jump to not_found. r_layout_id should contain the output of
// emitGetLayoutId(), r_caches should hold the RawTuple of caches for the
// current function, r_index should contain the opcode argument for the current
// instruction, and r_scratch is used as scratch.
//
// Writes to r_dst, r_layout_id (to turn it into a SmallInt), r_caches, and
// r_scratch.
void emitIcLookup(EmitEnv* env, Label* not_found, Register r_dst,
                  Register r_layout_id, Register r_caches, Register r_index,
                  Register r_scratch) {
  emitConvertToSmallInt(env, r_layout_id);

  // Set r_caches = r_caches + r_index * kPointerSize * kPointersPerCache,
  // without modifying r_index.
  static_assert(kIcPointersPerCache * kPointerSize == 64,
                "Unexpected kIcPointersPerCache");
  __ leaq(r_scratch, Address(r_index, TIMES_8, 0));
  __ leaq(r_caches, Address(r_caches, r_scratch, TIMES_8, heapObjectDisp(0)));
  Label done;
  for (int i = 0; i < kIcPointersPerCache; i += kIcPointersPerEntry) {
    bool is_last = i + kIcPointersPerEntry == kIcPointersPerCache;
    __ cmpl(Address(r_caches, (i + kIcEntryKeyOffset) * kPointerSize),
            r_layout_id);
    if (is_last) {
      __ jcc(NOT_EQUAL, not_found, Assembler::kFarJump);
      __ movq(r_dst,
              Address(r_caches, (i + kIcEntryValueOffset) * kPointerSize));
    } else {
      __ cmoveq(r_dst,
                Address(r_caches, (i + kIcEntryValueOffset) * kPointerSize));
      __ jcc(EQUAL, &done, Assembler::kNearJump);
    }
  }
  __ bind(&done);
}

// Allocate and push a BoundMethod on the stack. If the heap is full and a GC
// is needed, jump to slow_path instead. r_self and r_function will be used to
// populate the BoundMethod. r_space and r_scratch are used as scratch
// registers.
//
// Writes to r_space and r_scratch.
void emitPushBoundMethod(EmitEnv* env, Label* slow_path, Register r_self,
                         Register r_function, Register r_space,
                         Register r_scratch) {
  __ movq(r_space, Address(kThreadReg, Thread::runtimeOffset()));
  __ movq(r_space,
          Address(r_space, Runtime::heapOffset() + Heap::spaceOffset()));

  __ movq(r_scratch, Address(r_space, Space::fillOffset()));
  int num_attrs = BoundMethod::kSize / kPointerSize;
  __ addq(r_scratch, Immediate(Space::roundAllocationSize(
                         Instance::allocationSize(num_attrs))));
  __ cmpq(r_scratch, Address(r_space, Space::endOffset()));
  __ jcc(GREATER, slow_path, Assembler::kFarJump);
  __ xchgq(r_scratch, Address(r_space, Space::fillOffset()));
  RawHeader header = Header::from(num_attrs, 0, LayoutId::kBoundMethod,
                                  ObjectFormat::kObjects);
  __ movq(r_space, Immediate(header.raw()));
  __ movq(Address(r_scratch, 0), r_space);
  __ leaq(r_scratch, Address(r_scratch, -BoundMethod::kHeaderOffset +
                                            Object::kHeapObjectTag));
  __ movq(Address(r_scratch, heapObjectDisp(BoundMethod::kSelfOffset)), r_self);
  __ movq(Address(r_scratch, heapObjectDisp(BoundMethod::kFunctionOffset)),
          r_function);
  __ pushq(r_scratch);
}

// Given a RawObject in r_obj and its LayoutId (as a SmallInt) in r_layout_id,
// load its overflow RawTuple into r_dst.
//
// Writes to r_dst.
void emitLoadOverflowTuple(EmitEnv* env, Register r_dst, Register r_layout_id,
                           Register r_obj) {
  // Both uses of TIMES_4 in this function are a shortcut to multiply the value
  // of a SmallInt by kPointerSize.
  static_assert(kPointerSize >> Object::kSmallIntTagBits == 4,
                "Unexpected values of kPointerSize and/or kSmallIntTagBits");

  // TODO(bsimmers): This sequence of loads is pretty gross. See if we can make
  // the information more accessible.

  // Load thread->runtime()
  __ movq(r_dst, Address(kThreadReg, Thread::runtimeOffset()));
  // Load runtime->layouts_
  __ movq(r_dst, Address(r_dst, Runtime::layoutsOffset()));
  // Load layouts.items
  __ movq(r_dst, Address(r_dst, heapObjectDisp(List::kItemsOffset)));
  // Load items[r_layout_id]
  __ movq(r_dst, Address(r_dst, r_layout_id, TIMES_4, heapObjectDisp(0)));
  // Load layout.numInObjectAttributes
  __ movq(r_dst,
          Address(r_dst, heapObjectDisp(Layout::kNumInObjectAttributesOffset)));
  __ movq(r_dst, Address(r_obj, r_dst, TIMES_4, heapObjectDisp(0)));
}

// Push/pop from/into an attribute of r_obj, given a SmallInt offset in r_offset
// (which may be negative to signal an overflow attribute). r_layout_id should
// contain the object's LayoutId as a SmallInt and is used to look up the
// overflow tuple offset if needed.
//
// Emits the "next opcode" sequence after the in-object attribute case, binding
// next at that location, and jumps to next at the end of the overflow attribute
// case.
//
// Writes to r_offset and r_scratch.
void emitAttrWithOffset(EmitEnv* env, void (Assembler::*asm_op)(Address),
                        Label* next, Register r_obj, Register r_offset,
                        Register r_layout_id, Register r_scratch) {
  Label is_overflow;
  emitConvertFromSmallInt(env, r_offset);
  __ testq(r_offset, r_offset);
  __ jcc(SIGN, &is_overflow, Assembler::kNearJump);
  // In-object attribute. For now, asm_op is always pushq or popq.
  (env->as.*asm_op)(Address(r_obj, r_offset, TIMES_1, heapObjectDisp(0)));
  __ bind(next);
  emitNextOpcode(env);

  __ bind(&is_overflow);
  emitLoadOverflowTuple(env, r_scratch, r_layout_id, r_obj);
  // The real tuple index is -offset - 1, which is the same as ~offset.
  __ notq(r_offset);
  (env->as.*asm_op)(Address(r_scratch, r_offset, TIMES_8, heapObjectDisp(0)));
  __ jmp(next, Assembler::kNearJump);
}

template <>
void emitHandler<LOAD_ATTR_CACHED>(EmitEnv* env) {
  Register r_base = RAX;
  Register r_layout_id = R8;
  Register r_scratch = RDI;
  Register r_scratch2 = R9;
  Register r_caches = RDX;
  Label slow_path;
  __ popq(r_base);
  emitGetLayoutId(env, r_layout_id, r_base);
  __ movq(r_caches, Address(kFrameReg, Frame::kCachesOffset));
  emitIcLookup(env, &slow_path, r_scratch, r_layout_id, r_caches, kOpargReg,
               r_scratch2);

  Label is_function;
  Label next;
  __ testq(r_scratch, Immediate(Object::kSmallIntTagMask));
  __ jcc(NOT_ZERO, &is_function, Assembler::kNearJump);
  emitAttrWithOffset(env, &Assembler::pushq, &next, r_base, r_scratch,
                     r_layout_id, r_scratch2);

  __ bind(&is_function);
  emitPushBoundMethod(env, &slow_path, r_base, r_scratch, r_caches, R8);
  __ jmp(&next, Assembler::kNearJump);

  __ bind(&slow_path);
  __ pushq(r_base);
  emitJumpToGenericHandler(env);
}

template <>
void emitHandler<LOAD_METHOD_CACHED>(EmitEnv* env) {
  Register r_base = RAX;
  Register r_layout_id = R8;
  Register r_scratch = RDI;
  Register r_scratch2 = R9;
  Register r_caches = RDX;
  Label slow_path;
  __ popq(r_base);
  emitGetLayoutId(env, r_layout_id, r_base);
  __ movq(r_caches, Address(kFrameReg, Frame::kCachesOffset));
  emitIcLookup(env, &slow_path, r_scratch, r_layout_id, r_caches, kOpargReg,
               r_scratch2);

  Label is_smallint;
  Label next;
  // r_scratch contains either a SmallInt or a Function.
  __ testq(r_scratch, Immediate(Object::kSmallIntTagMask));
  __ jcc(ZERO, &is_smallint, Assembler::kNearJump);
  __ pushq(r_scratch);
  __ pushq(r_base);
  __ jmp(&next, Assembler::kNearJump);

  __ bind(&is_smallint);
  __ pushq(Immediate(Unbound::object().raw()));
  emitAttrWithOffset(env, &Assembler::pushq, &next, r_base, r_scratch,
                     r_layout_id, r_scratch2);

  __ bind(&slow_path);
  __ pushq(r_base);
  emitJumpToGenericHandler(env);
}

template <>
void emitHandler<STORE_ATTR_CACHED>(EmitEnv* env) {
  Register r_base = RAX;
  Register r_layout_id = R8;
  Register r_scratch = RDI;
  Register r_scratch2 = R9;
  Register r_caches = RDX;
  Label slow_path;
  __ popq(r_base);
  emitGetLayoutId(env, r_layout_id, r_base);
  __ movq(r_caches, Address(kFrameReg, Frame::kCachesOffset));
  emitIcLookup(env, &slow_path, r_scratch, r_layout_id, r_caches, kOpargReg,
               r_scratch2);

  Label next;
  // We only cache SmallInt values for STORE_ATTR.
  emitAttrWithOffset(env, &Assembler::popq, &next, r_base, r_scratch,
                     r_layout_id, r_scratch2);

  __ bind(&slow_path);
  __ pushq(r_base);
  emitJumpToGenericHandler(env);
}

template <>
void emitHandler<LOAD_FAST_REVERSE>(EmitEnv* env) {
  Label not_found;
  Register r_scratch = RAX;

  __ movq(r_scratch, Address(kFrameReg, kOpargReg, TIMES_8, Frame::kSize));
  __ cmpb(r_scratch, Immediate(Error::notFound().raw()));
  __ jcc(EQUAL, &not_found, Assembler::kNearJump);
  __ pushq(r_scratch);
  emitNextOpcode(env);

  __ bind(&not_found);
  emitJumpToGenericHandler(env);
}

template <>
void emitHandler<STORE_FAST_REVERSE>(EmitEnv* env) {
  __ popq(Address(kFrameReg, kOpargReg, TIMES_8, Frame::kSize));
  emitNextOpcode(env);
}

template <>
void emitHandler<LOAD_IMMEDIATE>(EmitEnv* env) {
  __ movsbq(RAX, kOpargReg);
  __ pushq(RAX);
  emitNextOpcode(env);
}

template <>
void emitHandler<LOAD_GLOBAL_CACHED>(EmitEnv* env) {
  __ movq(RAX, Address(kFrameReg, Frame::kCachesOffset));
  __ movq(RAX, Address(RAX, kOpargReg, TIMES_8, heapObjectDisp(0)));
  __ pushq(Address(RAX, heapObjectDisp(ValueCell::kValueOffset)));
  emitNextOpcode(env);
}

void emitPopJumpIfBool(EmitEnv* env, bool jump_value) {
  Label next;
  Label slow_path;
  Register r_scratch = RAX;

  // Handle RawBools directly; fall back to C++ for other types.
  __ popq(r_scratch);
  __ cmpb(r_scratch, boolImmediate(!jump_value));
  __ jcc(EQUAL, &next, Assembler::kNearJump);
  __ cmpb(r_scratch, boolImmediate(jump_value));
  __ jcc(NOT_EQUAL, &slow_path, Assembler::kNearJump);
  __ movq(kPCReg, kOpargReg);
  __ bind(&next);
  emitNextOpcode(env);

  __ bind(&slow_path);
  __ pushq(r_scratch);
  emitJumpToGenericHandler(env);
}

template <>
void emitHandler<UNARY_NOT>(EmitEnv* env) {
  Label slow_path;
  Register r_scratch = RAX;

  // Handle RawBools directly; fall back to C++ for other types
  __ popq(r_scratch);
  __ movq(RDX, r_scratch);
  // We only care about the bottom bits anyway, so andl is equivalent to andq
  // for our purposes.
  __ andb(r_scratch, Immediate(RawObject::kImmediateTagMask));
  // If it's a boolean, negate and push.
  __ cmpb(r_scratch, Immediate(RawObject::kBoolTag));
  __ jcc(NOT_EQUAL, &slow_path, Assembler::kNearJump);
  __ xorb(RDX, Immediate(RawBool::trueObj().raw() - RawBool::falseObj().raw()));
  __ pushq(RDX);
  emitNextOpcode(env);

  // Fall back to Interpreter::isTrue
  __ bind(&slow_path);
  __ pushq(RDX);
  emitGenericHandler(env, UNARY_NOT);
}

template <>
void emitHandler<POP_JUMP_IF_FALSE>(EmitEnv* env) {
  emitPopJumpIfBool(env, false);
}

template <>
void emitHandler<POP_JUMP_IF_TRUE>(EmitEnv* env) {
  emitPopJumpIfBool(env, true);
}

void emitJumpIfBoolOrPop(EmitEnv* env, bool jump_value) {
  Label next;
  Label slow_path;
  Register r_scratch = RAX;

  // Handle RawBools directly; fall back to C++ for other types.
  __ popq(r_scratch);
  __ cmpb(r_scratch, boolImmediate(!jump_value));
  __ jcc(EQUAL, &next, Assembler::kNearJump);
  __ cmpb(r_scratch, boolImmediate(jump_value));
  __ jcc(NOT_EQUAL, &slow_path, Assembler::kNearJump);
  __ pushq(r_scratch);
  __ movl(kPCReg, kOpargReg);
  __ bind(&next);
  emitNextOpcode(env);

  __ bind(&slow_path);
  __ pushq(r_scratch);
  emitJumpToGenericHandler(env);
}

template <>
void emitHandler<JUMP_IF_FALSE_OR_POP>(EmitEnv* env) {
  emitJumpIfBoolOrPop(env, false);
}

template <>
void emitHandler<JUMP_IF_TRUE_OR_POP>(EmitEnv* env) {
  emitJumpIfBoolOrPop(env, true);
}

template <>
void emitHandler<JUMP_ABSOLUTE>(EmitEnv* env) {
  __ movl(kPCReg, kOpargReg);
  emitNextOpcode(env);
}

template <>
void emitHandler<JUMP_FORWARD>(EmitEnv* env) {
  __ addl(kPCReg, kOpargReg);
  emitNextOpcode(env);
}

template <>
void emitHandler<DUP_TOP>(EmitEnv* env) {
  __ pushq(Address(RSP, 0));
  emitNextOpcode(env);
}

template <>
void emitHandler<ROT_TWO>(EmitEnv* env) {
  __ popq(RAX);
  __ pushq(Address(RSP, 0));
  __ movq(Address(RSP, 8), RAX);
  emitNextOpcode(env);
}

template <>
void emitHandler<POP_TOP>(EmitEnv* env) {
  __ popq(RAX);
  emitNextOpcode(env);
}

template <>
void emitHandler<EXTENDED_ARG>(EmitEnv* env) {
  __ shll(kOpargReg, Immediate(8));
  Register r_scratch = RAX;
  __ movzbl(r_scratch, Address(kBCReg, kPCReg, TIMES_1, heapObjectDisp(0)));
  __ movb(kOpargReg, Address(kBCReg, kPCReg, TIMES_1, heapObjectDisp(1)));
  __ shll(r_scratch, Immediate(kHandlerSizeShift));
  __ addl(kPCReg, Immediate(2));
  __ addq(r_scratch, kHandlersBaseReg);
  __ jmp(r_scratch);
  // Hint to the branch predictor that the indirect jmp never falls through to
  // here.
  __ ud2();
}

void emitCompareIs(EmitEnv* env, bool eq_value) {
  __ popq(R8);
  __ popq(R9);
  __ movl(RAX, boolImmediate(eq_value));
  __ movl(RDI, boolImmediate(!eq_value));
  __ cmpq(R8, R9);
  __ cmovnel(RAX, RDI);
  __ pushq(RAX);
  emitNextOpcode(env);
}

template <>
void emitHandler<COMPARE_IS>(EmitEnv* env) {
  emitCompareIs(env, true);
}

template <>
void emitHandler<COMPARE_IS_NOT>(EmitEnv* env) {
  emitCompareIs(env, false);
}

template <>
void emitHandler<RETURN_VALUE>(EmitEnv* env) {
  Label slow_path;

  // TODO(bsimmers): Until we emit smarter RETURN_* opcodes from the compiler,
  // check for the common case here:
  // Go to slow_path if frame == entry_frame...
  __ cmpq(kFrameReg, Address(RBP, kEntryFrameOffset));
  __ jcc(EQUAL, &slow_path, Assembler::kNearJump);

  // or frame->blockStack()->depth() != 0...
  __ cmpq(
      Address(kFrameReg, Frame::kBlockStackOffset + BlockStack::kDepthOffset),
      Immediate(SmallInt::fromWord(0).raw()));
  __ jcc(NOT_EQUAL, &slow_path, Assembler::kNearJump);

  // Fast path: pop return value, restore caller frame, push return value.
  __ popq(RAX);
  __ movq(kFrameReg, Address(kFrameReg, Frame::kPreviousFrameOffset));
  emitRestoreInterpreterState(env, kVMStack | kBytecode | kVMPC);
  __ pushq(RAX);
  emitNextOpcode(env);

  __ bind(&slow_path);
  emitSaveInterpreterState(env, kVMStack | kVMFrame);
  const word handler_offset =
      -(Interpreter::kNumContinues -
        static_cast<int>(Interpreter::Continue::RETURN)) *
      kHandlerSize;
  __ leaq(RAX, Address(kHandlersBaseReg, handler_offset));
  __ jmp(RAX);
}

template <typename T>
void writeBytes(void* addr, T value) {
  std::memcpy(addr, &value, sizeof(value));
}

void emitInterpreter(EmitEnv* env) {
  // Set up a frame and save callee-saved registers we'll use.
  __ pushq(RBP);
  __ movq(RBP, RSP);
  for (Register r : kUsedCalleeSavedRegs) {
    __ pushq(r);
  }

  __ movq(kThreadReg, kArgRegs[0]);
  __ movq(kFrameReg, Address(kThreadReg, Thread::currentFrameOffset()));
  __ pushq(kFrameReg);  // entry_frame

  // Materialize the handler base address into a register. The offset will be
  // patched right before emitting the first handler.
  const int32_t dummy_offset = 0xdeadbeef;
  __ leaq(kHandlersBaseReg, Address::addressRIPRelative(dummy_offset));
  word post_lea_size = env->as.codeSize();

  // Load VM state into registers and jump to the first opcode handler.
  emitRestoreInterpreterState(env, kAllState);
  emitNextOpcode(env);

  Label return_with_error_exception;
  __ bind(&return_with_error_exception);
  __ movq(RAX, Immediate(Error::exception().raw()));

  Label do_return;
  __ bind(&do_return);
  __ leaq(RSP, Address(RBP, -kNumCalleeSavedRegs * int{kPointerSize}));
  for (word i = kNumCalleeSavedRegs - 1; i >= 0; --i) {
    __ popq(kUsedCalleeSavedRegs[i]);
  }
  __ popq(RBP);
  __ ret();

  // UNWIND pseudo-handler
  static_assert(static_cast<int>(Interpreter::Continue::UNWIND) == 1,
                "Unexpected UNWIND value");
  {
    env->current_handler = "UNWIND pseudo-handler";
    HandlerSizer sizer(env, kHandlerSize);
    __ movq(kArgRegs[0], kThreadReg);
    __ movq(kArgRegs[1], Address(RBP, kEntryFrameOffset));
    __ movq(RAX, Immediate(reinterpret_cast<word>(Interpreter::unwind)));
    __ call(RAX);
    __ cmpb(RAX, Immediate(0));
    __ jcc(NOT_EQUAL, &return_with_error_exception, Assembler::kFarJump);
    emitRestoreInterpreterState(env, kAllState);
    emitNextOpcode(env);
  }

  // RETURN pseudo-handler
  static_assert(static_cast<int>(Interpreter::Continue::RETURN) == 2,
                "Unexpected RETURN value");
  {
    env->current_handler = "RETURN pseudo-handler";
    HandlerSizer sizer(env, kHandlerSize);
    __ movq(kArgRegs[0], kThreadReg);
    __ movq(kArgRegs[1], Address(RBP, kEntryFrameOffset));
    __ movq(RAX, Immediate(reinterpret_cast<word>(Interpreter::handleReturn)));
    __ call(RAX);
    // Check RAX.isErrorError()
    static_assert(Object::kImmediateTagBits + Error::kKindBits <= 8,
                  "tag should fit a byte for cmpb");
    __ cmpb(RAX, Immediate(Error::error().raw()));
    __ jcc(NOT_EQUAL, &do_return, Assembler::kFarJump);
    emitRestoreInterpreterState(env, kAllState);
    emitNextOpcode(env);
  }

  // YIELD pseudo-handler
  static_assert(static_cast<int>(Interpreter::Continue::YIELD) == 3,
                "Unexpected YIELD value");
  {
    env->current_handler = "YIELD pseudo-handler";
    HandlerSizer sizer(env, kHandlerSize);
    // RAX = thread->currentFrame()->popValue()
    Register r_scratch_frame = RDX;
    Register r_scratch_top = RCX;
    __ movq(r_scratch_frame, Address(kThreadReg, Thread::currentFrameOffset()));
    __ movq(r_scratch_top,
            Address(r_scratch_frame, Frame::kValueStackTopOffset));
    __ movq(RAX, Address(r_scratch_top, 0));
    __ addq(r_scratch_top, Immediate(kPointerSize));
    __ movq(Address(r_scratch_frame, Frame::kValueStackTopOffset),
            r_scratch_top);

    __ jmp(&do_return, Assembler::kFarJump);
  }

  // Mark the beginning of the opcode handlers and emit them at regular
  // intervals.
  char* lea_offset_addr = reinterpret_cast<char*>(
      env->as.codeAddress(post_lea_size - sizeof(int32_t)));
  CHECK(Utils::readBytes<int32_t>(lea_offset_addr) == dummy_offset,
        "Unexpected leaq encoding");
  writeBytes<int32_t>(lea_offset_addr, env->as.codeSize() - post_lea_size);
#define BC(name, i, handler)                                                   \
  {                                                                            \
    env->current_op = name;                                                    \
    env->current_handler = #name;                                              \
    HandlerSizer sizer(env, kHandlerSize);                                     \
    emitHandler<name>(env);                                                    \
  }
  FOREACH_BYTECODE(BC)
#undef BC

  // Emit the generic handler stubs at the end, out of the way of the
  // interesting code.
  for (word i = 0; i < 256; ++i) {
    __ bind(&env->call_handlers[i]);
    emitGenericHandler(env, static_cast<Bytecode>(i));
  }
}

class X64Interpreter : public Interpreter {
 public:
  X64Interpreter();
  ~X64Interpreter() override;
  void setupThread(Thread* thread) override;

 private:
  byte* code_;
  word size_;
};

X64Interpreter::X64Interpreter() {
  EmitEnv env;
  emitInterpreter(&env);

  // Finalize the code.
  size_ = env.as.codeSize();
  code_ = OS::allocateMemory(size_, &size_);
  env.as.finalizeInstructions(MemoryRegion(code_, size_));
  OS::protectMemory(code_, size_, OS::kReadExecute);
}

X64Interpreter::~X64Interpreter() { OS::freeMemory(code_, size_); }

void X64Interpreter::setupThread(Thread* thread) {
  thread->setInterpreterFunc(reinterpret_cast<Thread::InterpreterFunc>(code_));
}

}  // namespace

Interpreter* createAsmInterpreter() { return new X64Interpreter(); }

}  // namespace py
