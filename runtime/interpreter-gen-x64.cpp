#include "interpreter-gen.h"

#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <type_traits>

#include "assembler-x64.h"
#include "bytecode.h"
#include "frame.h"
#include "ic.h"
#include "interpreter.h"
#include "intrinsic.h"
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

const word kInstructionCacheLineSize = 64;

// Abbreviated x86-64 ABI:
constexpr Register kArgRegs[] = {RDI, RSI, RDX, RCX, R8, R9};
constexpr Register kReturnRegs[] = {RAX, RDX};

// Currently unused in code, but kept around for reference:
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
  Label opcode_handlers[kNumBytecodes];
  Label call_function_no_intrinsic_handler;
  Label call_handler;
  Label unwind_handler;
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

Immediate smallIntImmediate(word value) {
  return Immediate(SmallInt::fromWord(value).raw());
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
    __ movq(Address(kThreadReg, Thread::stackPointerOffset()), RSP);
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
    __ movq(RSP, Address(kThreadReg, Thread::stackPointerOffset()));
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
    case BINARY_ADD_SMALLINT:
    case BINARY_AND_SMALLINT:
    case BINARY_FLOORDIV_SMALLINT:
    case BINARY_SUB_SMALLINT:
    case BINARY_OR_SMALLINT:
    case COMPARE_EQ_SMALLINT:
    case COMPARE_LE_SMALLINT:
    case COMPARE_NE_SMALLINT:
    case COMPARE_GE_SMALLINT:
    case COMPARE_LT_SMALLINT:
    case COMPARE_GT_SMALLINT:
    case INPLACE_ADD_SMALLINT:
    case LOAD_ATTR_INSTANCE:
    case LOAD_ATTR_INSTANCE_TYPE_BOUND_METHOD:
    case LOAD_ATTR_POLYMORPHIC:
    case STORE_ATTR_INSTANCE:
    case STORE_ATTR_INSTANCE_OVERFLOW:
    case STORE_ATTR_POLYMORPHIC:
    case LOAD_METHOD_INSTANCE_FUNCTION:
    case LOAD_METHOD_POLYMORPHIC:
      return false;
    default:
      return true;
  }
}

template <typename FPtr>
void emitCall(EmitEnv* env, FPtr function) {
  // TODO(bsimmers): Augment Assembler so we can use the 0xe8 call opcode when
  // possible (this will also have implications on our allocation strategy).
  __ movq(RAX, Immediate(reinterpret_cast<int64_t>(function)));
  __ call(RAX);
}

void emitHandleContinue(EmitEnv* env, bool may_change_frame_pc) {
  Register r_result = kReturnRegs[0];

  Label handle_flow;
  static_assert(static_cast<int>(Interpreter::Continue::NEXT) == 0,
                "NEXT must be 0");
  __ testl(r_result, r_result);
  __ jcc(NOT_ZERO, &handle_flow, Assembler::kNearJump);

  emitRestoreInterpreterState(
      env, may_change_frame_pc ? kAllState : (kVMStack | kBytecode));
  emitNextOpcode(env);

  __ bind(&handle_flow);
  __ shll(r_result, Immediate(kHandlerSizeShift));
  __ leaq(r_result, Address(kHandlersBaseReg, r_result, TIMES_1,
                            -Interpreter::kNumContinues * kHandlerSize));
  __ jmp(r_result);
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

  emitCall<Interpreter::Continue (*)(Thread*, word)>(env, kCppHandlers[bc]);

  emitHandleContinue(env, mayChangeFramePC(bc));
}

Label* genericHandlerLabel(EmitEnv* env) {
  return &env->opcode_handlers[env->current_op];
}

// Jump to the generic handler for the Bytecode being currently emitted.
void emitJumpToGenericHandler(EmitEnv* env) {
  __ jmp(genericHandlerLabel(env), Assembler::kFarJump);
}

// Fallback handler for all unimplemented opcodes: call out to C++.
template <Bytecode bc>
void emitHandler(EmitEnv* env) {
  emitJumpToGenericHandler(env);
}

static void emitJumpIfSmallInt(EmitEnv* env, Register object, Label* target) {
  static_assert(Object::kSmallIntTag == 0, "unexpected tag for SmallInt");
  __ testb(object, Immediate(Object::kSmallIntTagMask));
  __ jcc(ZERO, target, Assembler::kNearJump);
}

static void emitJumpIfNotSmallInt(EmitEnv* env, Register object,
                                  Label* target) {
  static_assert(Object::kSmallIntTag == 0, "unexpected tag for SmallInt");
  __ testb(object, Immediate(Object::kSmallIntTagMask));
  __ jcc(NOT_ZERO, target, Assembler::kNearJump);
}

static void emitJumpIfNotBothSmallInt(EmitEnv* env, Register value0,
                                      Register value1, Register scratch,
                                      Label* target) {
  static_assert(Object::kSmallIntTag == 0, "unexpected tag for SmallInt");
  __ movq(scratch, value1);
  __ orq(scratch, value0);
  emitJumpIfNotSmallInt(env, scratch, target);
}

// Load the LayoutId of the RawObject in r_obj into r_dst as a SmallInt.
//
// Writes to r_dst.
void emitGetLayoutId(EmitEnv* env, Register r_dst, Register r_obj) {
  // Adding `(-kHeapObjectTag) & kPrimaryTagMask` will set the lowest
  // `kPrimaryTagBits` bits to zero iff the object had a `kHeapObjectTag`.
  __ leal(r_dst,
          Address(r_obj, (-Object::kHeapObjectTag) & Object::kPrimaryTagMask));
  __ testl(r_dst, Immediate(Object::kPrimaryTagMask));
  Label not_heap_object;
  __ jcc(NOT_ZERO, &not_heap_object, Assembler::kNearJump);

  // It is a HeapObject.
  static_assert(RawHeader::kLayoutIdOffset + Header::kLayoutIdBits <= 32,
                "expected layout id in lower 32 bits");
  __ movl(r_dst, Address(r_obj, heapObjectDisp(RawHeapObject::kHeaderOffset)));
  __ shrl(r_dst,
          Immediate(RawHeader::kLayoutIdOffset - Object::kSmallIntTagBits));
  __ andl(r_dst, Immediate(Header::kLayoutIdMask << Object::kSmallIntTagBits));
  Label done;
  __ jmp(&done, Assembler::kNearJump);

  __ bind(&not_heap_object);
  static_assert(static_cast<int>(LayoutId::kSmallInt) == 0,
                "Expected SmallInt LayoutId to be 0");
  __ xorl(r_dst, r_dst);
  static_assert(Object::kSmallIntTagBits == 1 && Object::kSmallIntTag == 0,
                "unexpected SmallInt tag");
  emitJumpIfSmallInt(env, r_obj, &done);

  // Immediate.
  __ movl(r_dst, r_obj);
  __ andl(r_dst, Immediate(Object::kImmediateTagMask));
  static_assert(Object::kSmallIntTag == 0, "Unexpected SmallInt tag");
  __ shll(r_dst, Immediate(Object::kSmallIntTagBits));

  __ bind(&done);
}

void emitJumpIfNotHeapObjectWithLayoutId(EmitEnv* env, Register r_obj,
                                         Register r_scratch, LayoutId layout_id,
                                         Label* target) {
  // Adding `(-kHeapObjectTag) & kPrimaryTagMask` will set the lowest
  // `kPrimaryTagBits` bits to zero iff the object had a `kHeapObjectTag`.
  __ leal(r_scratch,
          Address(r_obj, (-Object::kHeapObjectTag) & Object::kPrimaryTagMask));
  __ testl(r_scratch, Immediate(Object::kPrimaryTagMask));
  __ jcc(NOT_ZERO, target, Assembler::kNearJump);

  // It is a HeapObject.
  static_assert(RawHeader::kLayoutIdOffset + Header::kLayoutIdBits <= 32,
                "expected layout id in lower 32 bits");
  __ movl(r_scratch,
          Address(r_obj, heapObjectDisp(RawHeapObject::kHeaderOffset)));
  __ andl(r_scratch,
          Immediate(Header::kLayoutIdMask << RawHeader::kLayoutIdOffset));
  __ cmpl(r_scratch, Immediate(static_cast<word>(layout_id)
                               << RawHeader::kLayoutIdOffset));
  __ jcc(NOT_EQUAL, target, Assembler::kNearJump);
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
void emitIcLookupPolymorphic(EmitEnv* env, Label* not_found, Register r_dst,
                             Register r_layout_id, Register r_caches,
                             Register r_index, Register r_scratch) {
  // Set r_caches = r_caches + r_index * kPointerSize * kPointersPerEntry,
  // without modifying r_index.
  static_assert(kIcPointersPerEntry == 2, "Unexpected kIcPointersPerEntry");
  // Read the first value as the polymorphic cache.
  __ leaq(r_scratch, Address(r_index, TIMES_2, 0));
  __ movq(r_caches,
          Address(r_caches, r_scratch, TIMES_8,
                  heapObjectDisp(0) + kIcEntryValueOffset * kPointerSize));
  __ leaq(r_caches, Address(r_caches, heapObjectDisp(0)));
  Label done;
  for (int i = 0; i < kIcPointersPerPolyCache; i += kIcPointersPerEntry) {
    bool is_last = i + kIcPointersPerEntry == kIcPointersPerPolyCache;
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

void emitIcLookupMonomorphic(EmitEnv* env, Label* not_found, Register r_dst,
                             Register r_layout_id, Register r_caches,
                             Register r_index, Register r_scratch) {
  // Set r_caches = r_caches + r_index * kPointerSize * kPointersPerEntry,
  // without modifying r_index.
  static_assert(kIcPointersPerEntry == 2, "Unexpected kIcPointersPerEntry");
  __ leaq(r_scratch, Address(r_index, TIMES_2, 0));
  __ leaq(r_caches, Address(r_caches, r_scratch, TIMES_8, heapObjectDisp(0)));
  __ cmpl(Address(r_caches, kIcEntryKeyOffset * kPointerSize), r_layout_id);
  __ jcc(NOT_EQUAL, not_found, Assembler::kNearJump);
  __ movq(r_dst, Address(r_caches, kIcEntryValueOffset * kPointerSize));
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
  __ movq(Address(r_scratch, 0), Immediate(header.raw()));
  __ leaq(r_scratch, Address(r_scratch, -RawBoundMethod::kHeaderOffset +
                                            Object::kHeapObjectTag));
  __ movq(Address(r_scratch, heapObjectDisp(RawBoundMethod::kSelfOffset)),
          r_self);
  __ movq(Address(r_scratch, heapObjectDisp(RawBoundMethod::kFunctionOffset)),
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
  // Load layouts_[r_layout_id]
  __ movq(r_dst, Address(r_dst, r_layout_id, TIMES_4, heapObjectDisp(0)));
  // Load layout.numInObjectAttributes
  __ movq(
      r_dst,
      Address(r_dst, heapObjectDisp(RawLayout::kNumInObjectAttributesOffset)));
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
void emitHandler<BINARY_ADD_SMALLINT>(EmitEnv* env) {
  Register r_right = RAX;
  Register r_left = RDX;
  Register r_result = RDI;
  Label slow_path;
  __ popq(r_right);
  __ popq(r_left);
  emitJumpIfNotBothSmallInt(env, r_left, r_right, r_result, &slow_path);
  // Preserve argument values in case of overflow.
  __ movq(r_result, r_left);
  __ addq(r_result, r_right);
  __ jcc(YES_OVERFLOW, &slow_path, Assembler::kNearJump);
  __ pushq(r_result);
  emitNextOpcode(env);

  __ bind(&slow_path);
  __ pushq(r_left);
  __ pushq(r_right);
  __ movq(kArgRegs[0], kThreadReg);
  static_assert(kOpargReg == kArgRegs[1], "oparg expect to be in rsi");
  emitSaveInterpreterState(env, kVMPC | kVMStack | kVMFrame);
  emitCall<Interpreter::Continue (*)(Thread*, word)>(
      env, Interpreter::binaryOpUpdateCache);
  emitHandleContinue(env, /*may_change_frame_pc=*/true);
}

template <>
void emitHandler<BINARY_AND_SMALLINT>(EmitEnv* env) {
  Register r_right = RAX;
  Register r_left = RDX;
  Register r_result = RDI;
  Label slow_path;
  __ popq(r_right);
  __ popq(r_left);
  emitJumpIfNotBothSmallInt(env, r_left, r_right, r_result, &slow_path);
  __ movq(r_result, r_left);
  __ andq(r_result, r_right);
  __ pushq(r_result);
  emitNextOpcode(env);

  __ bind(&slow_path);
  __ pushq(r_left);
  __ pushq(r_right);
  __ movq(kArgRegs[0], kThreadReg);
  static_assert(kOpargReg == kArgRegs[1], "oparg expect to be in rsi");
  emitSaveInterpreterState(env, kVMPC | kVMStack | kVMFrame);
  emitCall<Interpreter::Continue (*)(Thread*, word)>(
      env, Interpreter::binaryOpUpdateCache);
  emitHandleContinue(env, /*may_change_frame_pc=*/true);
}

template <>
void emitHandler<BINARY_SUB_SMALLINT>(EmitEnv* env) {
  Register r_right = RAX;
  Register r_left = R8;
  Register r_result = RDI;
  Label slow_path;
  __ popq(r_right);
  __ popq(r_left);
  emitJumpIfNotBothSmallInt(env, r_left, r_right, r_result, &slow_path);
  // Preserve argument values in case of overflow.
  __ movq(r_result, r_left);
  __ subq(r_result, r_right);
  __ jcc(YES_OVERFLOW, &slow_path, Assembler::kNearJump);
  __ pushq(r_result);
  emitNextOpcode(env);

  __ bind(&slow_path);
  __ pushq(r_left);
  __ pushq(r_right);
  __ movq(kArgRegs[0], kThreadReg);
  static_assert(kOpargReg == kArgRegs[1], "oparg expect to be in rsi");
  emitSaveInterpreterState(env, kVMPC | kVMStack | kVMFrame);
  emitCall<Interpreter::Continue (*)(Thread*, word)>(
      env, Interpreter::binaryOpUpdateCache);
  emitHandleContinue(env, /*may_change_frame_pc=*/true);
}

template <>
void emitHandler<BINARY_OR_SMALLINT>(EmitEnv* env) {
  Register r_right = RAX;
  Register r_left = R8;
  Register r_result = RDI;
  Label slow_path;
  __ popq(r_right);
  __ popq(r_left);
  emitJumpIfNotBothSmallInt(env, r_left, r_right, r_result, &slow_path);
  // There is not __ orq instruction here because it is in the
  // emitJumpIfNotSmallInt implementation.
  __ pushq(r_result);
  emitNextOpcode(env);

  __ bind(&slow_path);
  __ pushq(r_left);
  __ pushq(r_right);
  __ movq(kArgRegs[0], kThreadReg);
  static_assert(kOpargReg == kArgRegs[1], "oparg expect to be in rsi");
  emitSaveInterpreterState(env, kVMPC | kVMStack | kVMFrame);
  emitCall<Interpreter::Continue (*)(Thread*, word)>(
      env, Interpreter::binaryOpUpdateCache);
  emitHandleContinue(env, /*may_change_frame_pc=*/true);
}

template <>
void emitHandler<BINARY_SUBSCR_LIST>(EmitEnv* env) {
  Register r_container = RAX;
  Register r_key = R8;
  Register r_layout_id = RDI;
  Label slow_path;
  __ popq(r_key);
  __ popq(r_container);
  // if (container.isList() && key.isSmallInt()) {
  emitJumpIfNotHeapObjectWithLayoutId(env, r_container, r_layout_id,
                                      LayoutId::kList, &slow_path);
  emitJumpIfNotSmallInt(env, r_key, &slow_path);

  // if (0 <= index && index < length) {
  // length >= 0 always holds. Therefore, ABOVE_EQUAL == NOT_CARRY if r_key
  // contains a negative value (sign bit == 1) or r_key >= r_list_length.
  __ cmpq(r_key,
          Address(r_container, heapObjectDisp(0) + List::kNumItemsOffset));
  __ jcc(ABOVE_EQUAL, &slow_path, Assembler::kNearJump);

  // list.at(index)
  __ movq(r_container,
          Address(r_container, heapObjectDisp(0) + List::kItemsOffset));
  // r_key is a SmallInt, so r_key already stores the index value * 2.
  // Therefore, applying TIMES_4 will compute index * 8.
  static_assert(Object::kSmallIntTag == 0, "unexpected tag for SmallInt");
  static_assert(Object::kSmallIntTagBits == 1, "unexpected tag for SmallInt");
  __ pushq(Address(r_container, r_key, TIMES_4, heapObjectDisp(0)));

  emitNextOpcode(env);

  __ bind(&slow_path);
  __ pushq(r_container);
  __ pushq(r_key);
  __ movq(kArgRegs[0], kThreadReg);
  static_assert(kOpargReg == kArgRegs[1], "oparg expect to be in rsi");
  emitSaveInterpreterState(env, kVMPC | kVMStack | kVMFrame);
  emitCall<Interpreter::Continue (*)(Thread*, word)>(
      env, Interpreter::binarySubscrUpdateCache);
  emitHandleContinue(env, /*may_change_frame_pc=*/true);
}

template <>
void emitHandler<STORE_SUBSCR_LIST>(EmitEnv* env) {
  Register r_container = RAX;
  Register r_key = R8;
  Register r_layout_id = RDI;
  Label slow_path_non_list, slow_path;
  __ popq(r_key);
  __ popq(r_container);
  // if (container.isList() && key.isSmallInt()) {
  emitJumpIfNotHeapObjectWithLayoutId(env, r_container, r_layout_id,
                                      LayoutId::kList, &slow_path_non_list);
  emitJumpIfNotSmallInt(env, r_key, &slow_path);

  // Re-use r_layout_id to store the value (right hand side).
  __ popq(r_layout_id);

  // if (0 <= index && index < length) {
  // length >= 0 always holds. Therefore, ABOVE_EQUAL == NOT_CARRY if r_key
  // contains a negative value (sign bit == 1) or r_key >= r_list_length.
  __ cmpq(r_key,
          Address(r_container, heapObjectDisp(0) + List::kNumItemsOffset));
  __ jcc(ABOVE_EQUAL, &slow_path, Assembler::kNearJump);

  // &list.at(index)
  __ movq(r_container,
          Address(r_container, heapObjectDisp(0) + List::kItemsOffset));
  // r_key is a SmallInt, so r_key already stores the index value * 2.
  // Therefore, applying TIMES_4 will compute index * 8.
  static_assert(Object::kSmallIntTag == 0, "unexpected tag for SmallInt");
  static_assert(Object::kSmallIntTagBits == 1, "unexpected tag for SmallInt");
  __ movq(Address(r_container, r_key, TIMES_4, heapObjectDisp(0)), r_layout_id);

  emitNextOpcode(env);

  __ bind(&slow_path);
  __ pushq(r_layout_id);
  __ bind(&slow_path_non_list);
  __ pushq(r_container);
  __ pushq(r_key);
  __ movq(kArgRegs[0], kThreadReg);
  static_assert(kOpargReg == kArgRegs[1], "oparg expect to be in rsi");
  emitSaveInterpreterState(env, kVMPC | kVMStack | kVMFrame);
  emitCall<Interpreter::Continue (*)(Thread*, word)>(
      env, Interpreter::storeSubscrUpdateCache);
  emitHandleContinue(env, /*may_change_frame_pc=*/true);
}

// TODO(T59397957): Split this into two opcodes.
template <>
void emitHandler<LOAD_ATTR_INSTANCE>(EmitEnv* env) {
  Register r_base = RAX;
  Register r_layout_id = R8;
  Register r_scratch = RDI;
  Register r_scratch2 = R9;
  Register r_caches = RDX;
  Label slow_path;
  __ popq(r_base);
  emitGetLayoutId(env, r_layout_id, r_base);
  __ movq(r_caches, Address(kFrameReg, Frame::kCachesOffset));
  emitIcLookupMonomorphic(env, &slow_path, r_scratch, r_layout_id, r_caches,
                          kOpargReg, r_scratch2);

  Label next;
  emitAttrWithOffset(env, &Assembler::pushq, &next, r_base, r_scratch,
                     r_layout_id, r_scratch2);

  __ bind(&slow_path);
  __ pushq(r_base);
  emitJumpToGenericHandler(env);
}

template <>
void emitHandler<LOAD_ATTR_INSTANCE_TYPE_BOUND_METHOD>(EmitEnv* env) {
  Register r_base = RAX;
  Register r_layout_id = R8;
  Register r_scratch = RDI;
  Register r_scratch2 = R9;
  Register r_caches = RDX;
  Label slow_path;
  __ popq(r_base);
  emitGetLayoutId(env, r_layout_id, r_base);
  __ movq(r_caches, Address(kFrameReg, Frame::kCachesOffset));
  emitIcLookupMonomorphic(env, &slow_path, r_scratch, r_layout_id, r_caches,
                          kOpargReg, r_scratch2);
  emitPushBoundMethod(env, &slow_path, r_base, r_scratch, r_caches, R8);
  emitNextOpcode(env);

  __ bind(&slow_path);
  __ pushq(r_base);
  emitJumpToGenericHandler(env);
}

template <>
void emitHandler<LOAD_ATTR_POLYMORPHIC>(EmitEnv* env) {
  Register r_base = RAX;
  Register r_layout_id = R8;
  Register r_scratch = RDI;
  Register r_scratch2 = R9;
  Register r_caches = RDX;
  Label slow_path;
  __ popq(r_base);
  emitGetLayoutId(env, r_layout_id, r_base);
  __ movq(r_caches, Address(kFrameReg, Frame::kCachesOffset));
  emitIcLookupPolymorphic(env, &slow_path, r_scratch, r_layout_id, r_caches,
                          kOpargReg, r_scratch2);

  Label is_function;
  Label next;
  emitJumpIfNotSmallInt(env, r_scratch, &is_function);
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
void emitHandler<LOAD_CONST>(EmitEnv* env) {
  Register r_scratch = RAX;
  __ movq(r_scratch, Address(kFrameReg, Frame::kLocalsOffset));
  __ movq(r_scratch,
          Address(r_scratch, Frame::kFunctionOffsetFromLocals * kPointerSize));
  __ movq(r_scratch,
          Address(r_scratch, heapObjectDisp(RawFunction::kCodeOffset)));
  __ movq(r_scratch,
          Address(r_scratch, heapObjectDisp(RawCode::kConstsOffset)));
  __ movq(r_scratch, Address(r_scratch, kOpargReg, TIMES_8, heapObjectDisp(0)));
  __ pushq(r_scratch);
  emitNextOpcode(env);
}

template <>
void emitHandler<LOAD_METHOD_INSTANCE_FUNCTION>(EmitEnv* env) {
  Register r_base = RAX;
  Register r_layout_id = R8;
  Register r_scratch = RDI;
  Register r_scratch2 = R9;
  Register r_caches = RDX;
  Label slow_path;
  __ popq(r_base);
  emitGetLayoutId(env, r_layout_id, r_base);
  __ movq(r_caches, Address(kFrameReg, Frame::kCachesOffset));
  emitIcLookupMonomorphic(env, &slow_path, r_scratch, r_layout_id, r_caches,
                          kOpargReg, r_scratch2);

  // Only functions are cached.
  __ pushq(r_scratch);
  __ pushq(r_base);
  emitNextOpcode(env);

  __ bind(&slow_path);
  __ pushq(r_base);
  emitJumpToGenericHandler(env);
}

template <>
void emitHandler<LOAD_METHOD_POLYMORPHIC>(EmitEnv* env) {
  Register r_base = RAX;
  Register r_layout_id = R8;
  Register r_scratch = RDI;
  Register r_scratch2 = R9;
  Register r_caches = RDX;
  Label slow_path;
  __ popq(r_base);
  emitGetLayoutId(env, r_layout_id, r_base);
  __ movq(r_caches, Address(kFrameReg, Frame::kCachesOffset));
  emitIcLookupPolymorphic(env, &slow_path, r_scratch, r_layout_id, r_caches,
                          kOpargReg, r_scratch2);

  // Only functions are cached.
  __ pushq(r_scratch);
  __ pushq(r_base);
  emitNextOpcode(env);

  __ bind(&slow_path);
  __ pushq(r_base);
  emitJumpToGenericHandler(env);
}

template <>
void emitHandler<STORE_ATTR_INSTANCE>(EmitEnv* env) {
  Register r_base = RAX;
  Register r_layout_id = R8;
  Register r_cache_value = RDI;
  Register r_scratch = R9;
  Register r_caches = RDX;
  Label slow_path;
  __ popq(r_base);
  emitGetLayoutId(env, r_layout_id, r_base);
  __ movq(r_caches, Address(kFrameReg, Frame::kCachesOffset));
  emitIcLookupMonomorphic(env, &slow_path, r_cache_value, r_layout_id, r_caches,
                          kOpargReg, r_scratch);
  emitConvertFromSmallInt(env, r_cache_value);
  __ popq(Address(r_base, r_cache_value, TIMES_1, heapObjectDisp(0)));
  emitNextOpcode(env);

  __ bind(&slow_path);
  __ pushq(r_base);
  emitJumpToGenericHandler(env);
}

template <>
void emitHandler<STORE_ATTR_INSTANCE_OVERFLOW>(EmitEnv* env) {
  Register r_base = RAX;
  Register r_layout_id = R8;
  Register r_cache_value = RDI;
  Register r_scratch = R9;
  Register r_caches = RDX;
  Label slow_path;
  __ popq(r_base);
  emitGetLayoutId(env, r_layout_id, r_base);
  __ movq(r_caches, Address(kFrameReg, Frame::kCachesOffset));
  emitIcLookupMonomorphic(env, &slow_path, r_cache_value, r_layout_id, r_caches,
                          kOpargReg, r_scratch);
  emitConvertFromSmallInt(env, r_cache_value);
  emitLoadOverflowTuple(env, r_scratch, r_layout_id, r_base);
  // The real tuple index is -offset - 1, which is the same as ~offset.
  __ notq(r_cache_value);
  __ popq(Address(r_scratch, r_cache_value, TIMES_8, heapObjectDisp(0)));
  emitNextOpcode(env);

  __ bind(&slow_path);
  __ pushq(r_base);
  emitJumpToGenericHandler(env);
}

template <>
void emitHandler<STORE_ATTR_POLYMORPHIC>(EmitEnv* env) {
  Register r_base = RAX;
  Register r_layout_id = R8;
  Register r_scratch = RDI;
  Register r_scratch2 = R9;
  Register r_caches = RDX;
  Label slow_path;
  __ popq(r_base);
  emitGetLayoutId(env, r_layout_id, r_base);
  __ movq(r_caches, Address(kFrameReg, Frame::kCachesOffset));
  emitIcLookupPolymorphic(env, &slow_path, r_scratch, r_layout_id, r_caches,
                          kOpargReg, r_scratch2);

  Label next;
  // We only cache SmallInt values for STORE_ATTR.
  emitAttrWithOffset(env, &Assembler::popq, &next, r_base, r_scratch,
                     r_layout_id, r_scratch2);

  __ bind(&slow_path);
  __ pushq(r_base);
  emitJumpToGenericHandler(env);
}

static void emitPushCallFrame(EmitEnv* env, Register r_callable,
                              Label* stack_overflow) {
  Register r_total_vars = R8;
  Register r_initial_size = R9;
  Register r_max_size = RAX;

  __ movq(r_total_vars,
          Address(r_callable, heapObjectDisp(RawFunction::kTotalVarsOffset)));
  static_assert(kPointerSize == 8, "unexpected size");
  static_assert(Object::kSmallIntTag == 0 && Object::kSmallIntTagBits == 1,
                "unexpected tag");
  // Note: SmallInt::cast(r_total_vars).value() * kPointerSize
  //    <=> r_total_vars * 4!
  __ leaq(r_initial_size, Address(r_total_vars, TIMES_4, Frame::kSize));
  __ movq(r_max_size,
          Address(r_callable, heapObjectDisp(RawFunction::kStacksizeOffset)));
  // Same reasoning as above.
  __ leaq(r_max_size, Address(r_initial_size, r_max_size, TIMES_4, 0));

  // if (sp - max_size >= thread->start_) { goto stack_overflow; }
  Register r_scratch = r_max_size;
  __ negq(r_scratch);
  __ addq(r_scratch, RSP);
  __ cmpq(r_scratch, Address(kThreadReg, Thread::limitOffset()));
  __ jcc(BELOW, stack_overflow, Assembler::kFarJump);

  __ subq(RSP, r_initial_size);

  // Setup the new frame:

  // total_locals = total_vars - total_args
  // Note that the involved registers contain smallints.
  Register r_total_locals = r_total_vars;
  __ movq(r_scratch,
          Address(r_callable, heapObjectDisp(RawFunction::kTotalArgsOffset)));
  __ addq(r_total_locals, r_scratch);
  // new_frame.setLocalsOffset(sp + kSize + (total_locals - 1) * kPointerSize)
  __ leaq(r_scratch,
          Address(RSP, r_total_locals, TIMES_4, Frame::kSize - kPointerSize));
  __ movq(Address(RSP, Frame::kLocalsOffset), r_scratch);
  // new_frame.blockStack()->setDepth(0)
  __ movq(Address(RSP, Frame::kBlockStackOffset + BlockStack::kDepthOffset),
          Immediate(0));
  // new_frame.setPreviousFrame(kFrameReg)
  __ movq(Address(RSP, Frame::kPreviousFrameOffset), kFrameReg);
  // kBCReg = callable.rewritteBytecode(); new_frame.setBytecode(kBCReg);
  __ movq(kBCReg,
          Address(r_callable,
                  heapObjectDisp(RawFunction::kRewrittenBytecodeOffset)));
  __ movq(Address(RSP, Frame::kBytecodeOffset), kBCReg);
  // new_frame.setCaches(callable.caches())
  __ movq(r_scratch,
          Address(r_callable, heapObjectDisp(RawFunction::kCachesOffset)));
  __ movq(Address(RSP, Frame::kCachesOffset), r_scratch);
  // caller_frame.setVirtualPC(kPCReg); kPCReg = 0
  emitSaveInterpreterState(env, SaveRestoreFlags::kVMPC);
  __ xorl(kPCReg, kPCReg);

  // kFrameReg = new_frame
  __ movq(kFrameReg, RSP);
}

void emitPrepareCallable(EmitEnv* env, Register r_callable,
                         Register r_layout_id,
                         Label* prepare_callable_immediate, Label* prepared) {
  Register r_scratch = RAX;

  __ cmpl(r_layout_id, Immediate(static_cast<word>(LayoutId::kBoundMethod)
                                 << RawHeader::kLayoutIdOffset));
  Label slow_path;
  __ jcc(NOT_EQUAL, &slow_path, Assembler::kFarJump);

  // RCX = callable_idx = kOpArgReg
  // frame->insertValueAt(callable_idx, BoundMethod::cast(callable).function());
  Register r_oparg_saved = R8;
  Register r_saved_callable = R9;
  __ movl(r_oparg_saved, kOpargReg);
  __ movq(r_saved_callable, r_callable);
  // Use `rep movsq` to copy RCX words from RSI to RDI.
  __ movl(RCX, kOpargReg);
  __ movq(RSI, RSP);
  __ subq(RSP, Immediate(kPointerSize));
  __ movq(RDI, RSP);
  __ repMovsq();
  // restore and increment kOparg.
  __ leaq(kOpargReg, Address(r_oparg_saved, 1));
  // Insert bound_method.function() and bound_method.self().
  __ movq(r_scratch, Address(r_saved_callable,
                             heapObjectDisp(RawBoundMethod::kSelfOffset)));
  __ movq(Address(RSP, kOpargReg, TIMES_8, -kPointerSize), r_scratch);
  __ movq(r_callable, Address(r_saved_callable,
                              heapObjectDisp(RawBoundMethod::kFunctionOffset)));
  __ movq(Address(RSP, kOpargReg, TIMES_8, 0), r_callable);
  // Check whether bound_method.function() is a heap object.
  // Adding `(-kHeapObjectTag) & kPrimaryTagMask` will set the lowest
  // `kPrimaryTagBits` bits to zero iff the object had a `kHeapObjectTag`.
  __ leal(r_scratch, Address(r_callable, (-Object::kHeapObjectTag) &
                                             Object::kPrimaryTagMask));
  __ testl(r_scratch, Immediate(Object::kPrimaryTagMask));
  // If it is an immediate, jump to the slow path, which raises an exception.
  __ jcc(NOT_ZERO, prepare_callable_immediate, Assembler::kFarJump);
  // Check whether bound_method.function() is a function.
  static_assert(Header::kLayoutIdMask <= kMaxInt32, "big layout id mask");
  __ movl(r_scratch,
          Address(r_callable, heapObjectDisp(RawHeapObject::kHeaderOffset)));
  __ andl(r_scratch,
          Immediate(Header::kLayoutIdMask << RawHeader::kLayoutIdOffset));
  __ cmpl(r_scratch, Immediate(static_cast<word>(LayoutId::kFunction)
                               << RawHeader::kLayoutIdOffset));
  // If it is a function, jump to the fast path.
  __ jcc(EQUAL, prepared, Assembler::kFarJump);

  // res = Interpreter::prepareCallableDunderCall(thread, nargs, nargs)
  // callable = res.function
  // nargs = res.nargs
  __ bind(prepare_callable_immediate);
  __ bind(&slow_path);
  emitSaveInterpreterState(env, kVMPC | kVMStack | kVMFrame);
  __ movq(kArgRegs[0], kThreadReg);
  static_assert(kArgRegs[1] == kOpargReg, "mismatch");
  __ movq(kArgRegs[2], kOpargReg);
  emitCall<Interpreter::PrepareCallableResult (*)(Thread*, word, word)>(
      env, Interpreter::prepareCallableCallDunderCall);
  static_assert(Object::kImmediateTagBits + Error::kKindBits <= 8,
                "tag should fit a byte for cmpb");
  __ cmpb(kReturnRegs[0], Immediate(Error::exception().raw()));
  __ jcc(EQUAL, &env->unwind_handler, Assembler::kFarJump);
  emitRestoreInterpreterState(env, kVMStack | kBytecode);
  __ movq(r_callable, kReturnRegs[0]);
  __ movq(kOpargReg, kReturnRegs[1]);
  __ jmp(prepared, Assembler::kFarJump);
}

void emitCallFunctionNoIntrinsicHandler(EmitEnv* env, Register r_callable,
                                        Label* next_opcode) {
  const Register r_scratch = RAX;
  const Register r_flags = RDX;

  // Check whether the call is interpreted.
  __ movl(r_flags,
          Address(r_callable, heapObjectDisp(RawFunction::kFlagsOffset)));
  __ testl(r_flags, smallIntImmediate(Function::Flags::kInterpreted));
  Label call_trampoline;
  __ jcc(ZERO, &call_trampoline, Assembler::kFarJump);

  // We do not support freevar/cellvar setup in the assembly interpreter.
  __ testl(r_flags, smallIntImmediate(Function::Flags::kNofree));
  Label call_interpreted_slow_path;
  __ jcc(ZERO, &call_interpreted_slow_path, Assembler::kFarJump);

  // prepareDefaultArgs.
  __ movl(r_scratch,
          Address(r_callable, heapObjectDisp(RawFunction::kArgcountOffset)));
  __ shrl(r_scratch, Immediate(SmallInt::kSmallIntTagBits));
  __ cmpl(r_scratch, kOpargReg);
  __ jcc(NOT_EQUAL, &call_interpreted_slow_path, Assembler::kFarJump);
  __ testl(r_flags, smallIntImmediate(Function::Flags::kSimpleCall));
  __ jcc(ZERO, &call_interpreted_slow_path, Assembler::kFarJump);

  emitPushCallFrame(env, r_callable, &call_interpreted_slow_path);

  __ bind(next_opcode);
  emitNextOpcode(env);

  // Function::cast(callable).entry()(thread, nargs);
  __ bind(&call_trampoline);
  emitSaveInterpreterState(env, kVMPC | kVMStack | kVMFrame);
  __ movq(r_scratch,
          Address(r_callable, heapObjectDisp(RawFunction::kEntryOffset)));
  __ movq(kArgRegs[0], kThreadReg);
  static_assert(kArgRegs[1] == kOpargReg, "register mismatch");
  static_assert(std::is_same<Function::Entry, RawObject (*)(Thread*, word)>(),
                "type mismatch");
  __ call(r_scratch);
  // if (kReturnRegs[0].isErrorException()) return UNWIND;
  static_assert(Object::kImmediateTagBits + Error::kKindBits <= 8,
                "tag should fit a byte for cmpb");
  __ cmpb(kReturnRegs[0], Immediate(Error::exception().raw()));
  __ jcc(EQUAL, &env->unwind_handler, Assembler::kFarJump);
  emitRestoreInterpreterState(env, kVMStack | kBytecode);
  __ pushq(kReturnRegs[0]);
  emitNextOpcode(env);

  // Interpreter::callInterpreted(thread, nargs, function)
  __ bind(&call_interpreted_slow_path);
  __ movq(kArgRegs[2], r_callable);
  __ movq(kArgRegs[0], kThreadReg);
  static_assert(kArgRegs[1] == kOpargReg, "reg mismatch");
  emitSaveInterpreterState(env, kVMPC | kVMStack | kVMFrame);
  emitCall<Interpreter::Continue (*)(Thread*, word, RawFunction)>(
      env, Interpreter::callInterpreted);
  emitHandleContinue(env, /*may_change_frame_pc=*/true);
}

void emitCallHandler(EmitEnv* env) {
  const Register r_callable = RDI;
  const Register r_intrinsic_id = RDX;

  // Check callable.
  __ movq(r_callable, Address(RSP, kOpargReg, TIMES_8, 0));
  // Check whether callable is a heap object.
  static_assert(Object::kHeapObjectTag == 1, "unexpected tag");
  Register r_layout_id = RAX;
  __ movl(r_layout_id, r_callable);
  __ andl(r_layout_id, Immediate(Object::kPrimaryTagMask));
  __ cmpl(r_layout_id, Immediate(Object::kHeapObjectTag));
  Label prepare_callable_immediate;
  __ jcc(NOT_EQUAL, &prepare_callable_immediate, Assembler::kFarJump);
  // Check whether callable is a function.
  static_assert(Header::kLayoutIdMask <= kMaxInt32, "big layout id mask");
  __ movl(r_layout_id,
          Address(r_callable, heapObjectDisp(RawHeapObject::kHeaderOffset)));
  __ andl(r_layout_id,
          Immediate(Header::kLayoutIdMask << RawHeader::kLayoutIdOffset));
  __ cmpl(r_layout_id, Immediate(static_cast<word>(LayoutId::kFunction)
                                 << RawHeader::kLayoutIdOffset));
  Label prepare_callable_generic;
  __ jcc(NOT_EQUAL, &prepare_callable_generic, Assembler::kFarJump);
  Label prepared;
  __ bind(&prepared);

  // Check whether we have intrinsic code for the function.
  static_assert(
      RawFunction::kIntrinsicIdOffset + SmallInt::kSmallIntTagBits == 32,
      "unexpected intrinsic id offset");
  __ movl(r_intrinsic_id,
          Address(r_callable, heapObjectDisp(RawFunction::kFlagsOffset) + 4));
  __ cmpl(r_intrinsic_id, Immediate(static_cast<word>(SymbolId::kInvalid)));
  Label no_intrinsic;
  __ jcc(EQUAL, &no_intrinsic, Assembler::kNearJump);

  // if (doIntrinsic(thread, id)) return Continue::NEXT;
  emitSaveInterpreterState(env, kVMPC | kVMStack | kVMFrame);
  __ pushq(r_callable);
  __ pushq(kOpargReg);
  __ movq(kArgRegs[0], kThreadReg);
  __ movq(kArgRegs[1], r_intrinsic_id);
  emitCall<bool (*)(Thread*, SymbolId)>(env, doIntrinsic);
  __ popq(kOpargReg);
  __ popq(r_callable);
  emitRestoreInterpreterState(env, kVMStack | kBytecode);
  __ testb(kReturnRegs[0], kReturnRegs[0]);
  Label next_opcode;
  __ jcc(NOT_ZERO, &next_opcode, Assembler::kFarJump);

  __ bind(&no_intrinsic);

  emitCallFunctionNoIntrinsicHandler(env, r_callable, &next_opcode);

  __ bind(&prepare_callable_generic);
  emitPrepareCallable(env, r_callable, r_layout_id, &prepare_callable_immediate,
                      &prepared);
}

template <>
void emitHandler<CALL_FUNCTION>(EmitEnv* env) {
  // The CALL_FUNCTION handler is generated out-of-line after the handler table.
  __ jmp(&env->call_handler, Assembler::kFarJump);
}

template <>
void emitHandler<CALL_METHOD>(EmitEnv* env) {
  Register r_scratch = RAX;
  Label remove_value_and_call;

  // if (thread->stackPeek(arg + 1).isUnbound()) goto remove_value_and_call;
  __ movl(r_scratch, Address(RSP, kOpargReg, TIMES_8, kPointerSize));
  __ cmpl(r_scratch, Immediate(Unbound::object().raw()));
  __ jcc(EQUAL, &remove_value_and_call, Assembler::kNearJump);

  // Increment argument count by 1 and jump into a call handler .
  // Since CALL_METHOD always calls a function object from attribute lookups,
  // the function is guaranteed to be a function object without an intrinsic
  // since Pyro currently does not support to attach intrinsics to  a type
  // attribute.
  __ incl(kOpargReg);
  __ jmp(&env->call_function_no_intrinsic_handler, Assembler::kFarJump);

  // thread->removeValueAt(arg + 1)
  __ bind(&remove_value_and_call);
  Register r_saved_rdi = R8;
  Register r_saved_rsi = R9;
  __ movq(r_saved_rdi, RDI);
  __ movq(r_saved_rsi, RSI);
  // Use `rep movsq` to copy RCX words from RSI to RDI.
  __ std();
  __ leaq(RCX, Address(kOpargReg, 1));
  __ leaq(RSI, Address(RSP, kOpargReg, TIMES_8, 0));
  __ leaq(RDI, Address(RSI, kPointerSize));
  __ repMovsq();
  __ cld();
  __ addq(RSP, Immediate(kPointerSize));
  __ movq(RDI, r_saved_rdi);
  __ movq(RSI, r_saved_rsi);
  __ jmp(&env->call_handler, Assembler::kFarJump);
}

template <>
void emitHandler<LOAD_FAST_REVERSE>(EmitEnv* env) {
  Register r_scratch = RAX;

  __ movq(r_scratch, Address(kFrameReg, kOpargReg, TIMES_8, Frame::kSize));
  __ cmpb(r_scratch, Immediate(Error::notFound().raw()));
  __ jcc(EQUAL, genericHandlerLabel(env), Assembler::kFarJump);
  __ pushq(r_scratch);
  emitNextOpcode(env);
}

template <>
void emitHandler<LOAD_FAST_REVERSE_UNCHECKED>(EmitEnv* env) {
  __ pushq(Address(kFrameReg, kOpargReg, TIMES_8, Frame::kSize));
  emitNextOpcode(env);
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
  __ pushq(Address(RAX, heapObjectDisp(RawValueCell::kValueOffset)));
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
  Register r_scratch2 = RDX;

  // Handle RawBools directly; fall back to C++ for other types
  __ popq(r_scratch);
  // Add (-kBoolTag) & kImmediateTagMask. This will set the lowest
  // kImmediateTagBits to 0 iff they were kBoolTag before.
  __ leal(r_scratch2, Address(r_scratch, (-RawObject::kBoolTag) &
                                             (RawObject::kImmediateTagMask)));
  __ testl(r_scratch2, Immediate(RawObject::kImmediateTagMask));
  // If it had kBoolTag, then negate and push.
  __ jcc(NOT_ZERO, &slow_path, Assembler::kNearJump);
  __ xorl(r_scratch,
          Immediate(RawBool::trueObj().raw() ^ RawBool::falseObj().raw()));
  __ pushq(r_scratch);
  emitNextOpcode(env);

  // Fall back to Interpreter::isTrue
  __ bind(&slow_path);
  __ pushq(r_scratch);
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

static void emitCompareOpSmallIntHandler(EmitEnv* env, Condition cond) {
  Register r_right = RAX;
  Register r_left = RDI;
  Register r_true = RDX;
  Register r_result = R8;
  Label slow_path;
  __ popq(r_right);
  __ popq(r_left);
  // Use the fast path only when both arguments are SmallInt.
  emitJumpIfNotBothSmallInt(env, r_left, r_right, r_result, &slow_path);
  __ movq(r_true, boolImmediate(true));
  __ movq(r_result, boolImmediate(false));
  __ cmpq(r_left, r_right);
  switch (cond) {
    case EQUAL:
      __ cmoveq(r_result, r_true);
      break;
    case NOT_EQUAL:
      __ cmovneq(r_result, r_true);
      break;
    case GREATER:
      __ cmovgq(r_result, r_true);
      break;
    case GREATER_EQUAL:
      __ cmovgeq(r_result, r_true);
      break;
    case LESS:
      __ cmovlq(r_result, r_true);
      break;
    case LESS_EQUAL:
      __ cmovleq(r_result, r_true);
      break;
    default:
      UNREACHABLE("unhandled cond");
  }
  __ pushq(r_result);
  emitNextOpcode(env);

  __ bind(&slow_path);
  __ pushq(r_left);
  __ pushq(r_right);
  __ movq(kArgRegs[0], kThreadReg);
  static_assert(kOpargReg == kArgRegs[1], "oparg expect to be in rsi");
  emitSaveInterpreterState(env, kVMPC | kVMStack | kVMFrame);
  emitCall<Interpreter::Continue (*)(Thread*, word)>(
      env, Interpreter::compareOpUpdateCache);
  emitHandleContinue(env, /*may_change_frame_pc=*/true);
}

template <>
void emitHandler<COMPARE_EQ_SMALLINT>(EmitEnv* env) {
  emitCompareOpSmallIntHandler(env, EQUAL);
}

template <>
void emitHandler<COMPARE_NE_SMALLINT>(EmitEnv* env) {
  emitCompareOpSmallIntHandler(env, NOT_EQUAL);
}

template <>
void emitHandler<COMPARE_GT_SMALLINT>(EmitEnv* env) {
  emitCompareOpSmallIntHandler(env, GREATER);
}

template <>
void emitHandler<COMPARE_GE_SMALLINT>(EmitEnv* env) {
  emitCompareOpSmallIntHandler(env, GREATER_EQUAL);
}

template <>
void emitHandler<COMPARE_LT_SMALLINT>(EmitEnv* env) {
  emitCompareOpSmallIntHandler(env, LESS);
}

template <>
void emitHandler<COMPARE_LE_SMALLINT>(EmitEnv* env) {
  emitCompareOpSmallIntHandler(env, LESS_EQUAL);
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
void emitHandler<INPLACE_ADD_SMALLINT>(EmitEnv* env) {
  Register r_right = RAX;
  Register r_left = RDX;
  Register r_result = RDI;
  Label slow_path;
  __ popq(r_right);
  __ popq(r_left);
  emitJumpIfNotBothSmallInt(env, r_left, r_right, r_result, &slow_path);
  // Preserve argument values in case of overflow.
  __ movq(r_result, r_left);
  __ addq(r_result, r_right);
  __ jcc(YES_OVERFLOW, &slow_path, Assembler::kNearJump);
  __ pushq(r_result);
  emitNextOpcode(env);

  __ bind(&slow_path);
  __ pushq(r_left);
  __ pushq(r_right);
  __ movq(kArgRegs[0], kThreadReg);
  static_assert(kOpargReg == kArgRegs[1], "oparg expect to be in rsi");
  emitSaveInterpreterState(env, kVMPC | kVMStack | kVMFrame);
  emitCall<Interpreter::Continue (*)(Thread*, word)>(
      env, Interpreter::inplaceOpUpdateCache);
  emitHandleContinue(env, /*may_change_frame_pc=*/true);
}

template <>
void emitHandler<RETURN_VALUE>(EmitEnv* env) {
  Label slow_path;

  // Go to slow_path if frame == entry_frame...
  __ cmpq(kFrameReg, Address(RBP, kEntryFrameOffset));
  __ jcc(EQUAL, &slow_path, Assembler::kNearJump);

  // or frame->blockStack()->depth() != 0...
  __ cmpq(
      Address(kFrameReg, Frame::kBlockStackOffset + BlockStack::kDepthOffset),
      smallIntImmediate(0));
  __ jcc(NOT_EQUAL, &slow_path, Assembler::kNearJump);

  // Fast path: pop return value, restore caller frame, push return value.
  __ popq(RAX);
  // RSP = frame->frameEnd(); ( = locals() + 2)
  __ movq(RSP, Address(kFrameReg, Frame::kLocalsOffset));
  __ addq(RSP, Immediate(2 * kPointerSize));
  __ movq(kFrameReg, Address(kFrameReg, Frame::kPreviousFrameOffset));
  emitRestoreInterpreterState(env, kBytecode | kVMPC);
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

template <>
void emitHandler<POP_BLOCK>(EmitEnv* env) {
  Register r_depth = RAX;
  Register r_block = R9;

  // frame->blockstack()->pop()
  __ movl(r_depth, Address(kFrameReg, Frame::kBlockStackOffset +
                                          BlockStack::kDepthOffset));
  __ subl(r_depth, Immediate(2));
  __ movq(r_block,
          Address(kFrameReg, r_depth, TIMES_4,
                  Frame::kBlockStackOffset + BlockStack::kStackOffset));
  __ movl(
      Address(kFrameReg, Frame::kBlockStackOffset + BlockStack::kDepthOffset),
      r_depth);

  // thread->setStackPointer(thread->valueStackBase() - block.level());
  // =>   RSP = kFrameReg - (block.level() * kPointerSize)
  __ shrq(r_block, Immediate(TryBlock::kLevelOffset));
  __ andl(r_block, Immediate(TryBlock::kLevelMask));
  __ negq(r_block);
  __ leaq(RSP, Address(kFrameReg, r_block, TIMES_8, 0));
  emitNextOpcode(env);
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

  __ align(kInstructionCacheLineSize);

  // UNWIND pseudo-handler
  static_assert(static_cast<int>(Interpreter::Continue::UNWIND) == 1,
                "Unexpected UNWIND value");
  {
    env->current_handler = "UNWIND pseudo-handler";
    HandlerSizer sizer(env, kHandlerSize);
    __ bind(&env->unwind_handler);
    __ movq(kArgRegs[0], kThreadReg);
    __ movq(kArgRegs[1], Address(RBP, kEntryFrameOffset));

    emitCall<bool (*)(Thread*, Frame*)>(env, Interpreter::unwind);
    __ testb(RAX, RAX);
    __ jcc(NOT_ZERO, &return_with_error_exception, Assembler::kFarJump);
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
    emitCall<RawObject (*)(Thread*, Frame*)>(env, Interpreter::handleReturn);
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
    // RAX = thread->stackPop()
    Register r_scratch_top = RCX;
    __ movq(r_scratch_top, Address(kThreadReg, Thread::stackPointerOffset()));
    __ movq(RAX, Address(r_scratch_top, 0));
    __ addq(r_scratch_top, Immediate(kPointerSize));
    __ movq(Address(kThreadReg, Thread::stackPointerOffset()), r_scratch_top);

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

  __ bind(&env->call_handler);
  emitCallHandler(env);

  __ bind(&env->call_function_no_intrinsic_handler);
  {
    const Register r_callable = RDI;
    Label next_opcode;
    __ movq(r_callable, Address(RSP, kOpargReg, TIMES_8, 0));
    emitCallFunctionNoIntrinsicHandler(env, r_callable, &next_opcode);
  }

  // Emit the generic handler stubs at the end, out of the way of the
  // interesting code.
  for (word i = 0; i < 256; ++i) {
    __ bind(&env->opcode_handlers[i]);
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

Interpreter* createAsmInterpreter() { return new X64Interpreter; }

}  // namespace py
