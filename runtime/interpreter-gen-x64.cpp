#include "interpreter-gen.h"

#include <cinttypes>
#include <cstdio>
#include <cstring>

#include "assembler-x64.h"
#include "bytecode.h"
#include "frame.h"
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

// This macro helps instruction-emitting code stand out a bit.
#define __ as->

namespace python {

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

// RAII helper to ensure that a region of code is nop-padded to a specific size,
// with checks that it doesn't overflow the limit.
class HandlerSizer {
 public:
  HandlerSizer(Assembler* as, word size)
      : as_(as), size_(size), start_cursor_(as->codeSize()) {}

  ~HandlerSizer() {
    word padding = start_cursor_ + size_ - as_->codeSize();
    CHECK(padding >= 0, "Handler overflow by %" PRIdPTR " bytes", -padding);
    as_->nops(padding);
  }

 private:
  Assembler* as_;
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
void emitNextOpcode(Assembler* as) {
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

void emitSaveInterpreterState(Assembler* as, SaveRestoreFlags flags) {
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

void emitRestoreInterpreterState(Assembler* as, SaveRestoreFlags flags) {
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

// When calling a C++ handler, most state is synced back to the Thread/Frame
// before the call and reloaded after the call. We could be smarter about this
// by auditing which opcodes can read/write the PC and other bits of VM state,
// but hopefully it will become a non-issue as we implement more and more
// opcodes in assembly.
void emitGenericHandler(Assembler* as, Bytecode bc) {
  __ movq(kArgRegs[0], kThreadReg);
  static_assert(kOpargReg == kArgRegs[1], "oparg expect to be in rsi");

  // Sync VM state to memory and restore native stack pointer.
  emitSaveInterpreterState(as, kVMPC | kVMStack | kVMFrame);

  // TODO(bsimmers): Augment Assembler so we can use the 0xe8 call opcode when
  // possible (this will also have implications on our allocation strategy).
  __ movq(RAX, Immediate(reinterpret_cast<int64_t>(kCppHandlers[bc])));
  __ call(RAX);

  Label handle_flow;
  static_assert(static_cast<int>(Interpreter::Continue::NEXT) == 0,
                "NEXT must be 0");
  __ testl(RAX, RAX);
  __ jcc(NOT_ZERO, &handle_flow, true);

  emitRestoreInterpreterState(
      as, mayChangeFramePC(bc) ? kAllState : (kVMStack | kBytecode));
  emitNextOpcode(as);

  __ bind(&handle_flow);
  __ shll(RAX, Immediate(kHandlerSizeShift));
  __ leaq(RAX, Address(kHandlersBaseReg, RAX, TIMES_1,
                       -Interpreter::kNumContinues * kHandlerSize));
  __ jmp(RAX);
}

// Fallback handler for all unimplemented opcodes: call out to C++.
template <Bytecode bc>
void emitHandler(Assembler* as) {
  emitGenericHandler(as, bc);
}

template <>
void emitHandler<LOAD_FAST_REVERSE>(Assembler* as) {
  Label not_found;
  Register r_scratch = RAX;

  __ movq(r_scratch, Address(kFrameReg, kOpargReg, TIMES_8, Frame::kSize));
  __ cmpb(r_scratch, Immediate(Error::notFound().raw()));
  __ jcc(EQUAL, &not_found, true);
  __ pushq(r_scratch);
  emitNextOpcode(as);

  __ bind(&not_found);
  emitGenericHandler(as, LOAD_FAST_REVERSE);
}

template <>
void emitHandler<STORE_FAST_REVERSE>(Assembler* as) {
  __ popq(Address(kFrameReg, kOpargReg, TIMES_8, Frame::kSize));
  emitNextOpcode(as);
}

template <>
void emitHandler<LOAD_IMMEDIATE>(Assembler* as) {
  __ movsbq(RAX, kOpargReg);
  __ pushq(RAX);
  emitNextOpcode(as);
}

template <>
void emitHandler<LOAD_GLOBAL_CACHED>(Assembler* as) {
  __ movq(RAX, Address(kFrameReg, Frame::kCachesOffset));
  __ movq(RAX, Address(RAX, kOpargReg, TIMES_8, heapObjectDisp(0)));
  __ pushq(Address(RAX, heapObjectDisp(ValueCell::kValueOffset)));
  emitNextOpcode(as);
}

void emitPopJumpIfBool(Assembler* as, Bytecode bc, bool jump_value) {
  Label next;
  Label slow_path;
  Register r_scratch = RAX;

  // Handle RawBools directly; fall back to C++ for other types.
  __ popq(r_scratch);
  __ cmpb(r_scratch, boolImmediate(!jump_value));
  __ jcc(EQUAL, &next, true);
  __ cmpb(r_scratch, boolImmediate(jump_value));
  __ jcc(NOT_EQUAL, &slow_path, true);
  __ movq(kPCReg, kOpargReg);
  __ bind(&next);
  emitNextOpcode(as);

  __ bind(&slow_path);
  __ pushq(r_scratch);
  emitGenericHandler(as, bc);
}

template <>
void emitHandler<POP_JUMP_IF_FALSE>(Assembler* as) {
  emitPopJumpIfBool(as, POP_JUMP_IF_FALSE, false);
}

template <>
void emitHandler<POP_JUMP_IF_TRUE>(Assembler* as) {
  emitPopJumpIfBool(as, POP_JUMP_IF_TRUE, true);
}

void emitJumpIfBoolOrPop(Assembler* as, Bytecode bc, bool jump_value) {
  Label next;
  Label slow_path;
  Register r_scratch = RAX;

  // Handle RawBools directly; fall back to C++ for other types.
  __ popq(r_scratch);
  __ cmpb(r_scratch, boolImmediate(!jump_value));
  __ jcc(EQUAL, &next, true);
  __ cmpb(r_scratch, boolImmediate(jump_value));
  __ jcc(NOT_EQUAL, &slow_path, true);
  __ pushq(r_scratch);
  __ movl(kPCReg, kOpargReg);
  __ bind(&next);
  emitNextOpcode(as);

  __ bind(&slow_path);
  __ pushq(r_scratch);
  emitGenericHandler(as, bc);
}

template <>
void emitHandler<JUMP_IF_FALSE_OR_POP>(Assembler* as) {
  emitJumpIfBoolOrPop(as, JUMP_IF_FALSE_OR_POP, false);
}

template <>
void emitHandler<JUMP_IF_TRUE_OR_POP>(Assembler* as) {
  emitJumpIfBoolOrPop(as, JUMP_IF_TRUE_OR_POP, true);
}

template <>
void emitHandler<JUMP_ABSOLUTE>(Assembler* as) {
  __ movl(kPCReg, kOpargReg);
  emitNextOpcode(as);
}

template <>
void emitHandler<JUMP_FORWARD>(Assembler* as) {
  __ addl(kPCReg, kOpargReg);
  emitNextOpcode(as);
}

template <>
void emitHandler<DUP_TOP>(Assembler* as) {
  __ pushq(Address(RSP, 0));
  emitNextOpcode(as);
}

template <>
void emitHandler<ROT_TWO>(Assembler* as) {
  __ popq(RAX);
  __ pushq(Address(RSP, 0));
  __ movq(Address(RSP, 8), RAX);
  emitNextOpcode(as);
}

template <>
void emitHandler<POP_TOP>(Assembler* as) {
  __ popq(RAX);
  emitNextOpcode(as);
}

template <>
void emitHandler<EXTENDED_ARG>(Assembler* as) {
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

void emitCompareIs(Assembler* as, bool eq_value) {
  __ popq(R8);
  __ popq(R9);
  __ movl(RAX, boolImmediate(eq_value));
  __ movl(RDI, boolImmediate(!eq_value));
  __ cmpq(R8, R9);
  __ cmovnel(RAX, RDI);
  __ pushq(RAX);
  emitNextOpcode(as);
}

template <>
void emitHandler<COMPARE_IS>(Assembler* as) {
  emitCompareIs(as, true);
}

template <>
void emitHandler<COMPARE_IS_NOT>(Assembler* as) {
  emitCompareIs(as, false);
}

template <>
void emitHandler<RETURN_VALUE>(Assembler* as) {
  Label slow_path;
  Register r_scratch = RAX;

  // TODO(bsimmers): Until we emit smarter RETURN_* opcodes from the compiler,
  // check for the common case here:
  // Go to slow_path if frame == entry_frame...
  __ cmpq(kFrameReg, Address(RBP, kEntryFrameOffset));
  __ jcc(EQUAL, &slow_path);

  // or frame->blockStack()->depth() != 0...
  __ cmpq(
      Address(kFrameReg, Frame::kBlockStackOffset + BlockStack::kDepthOffset),
      Immediate(SmallInt::fromWord(0).raw()));
  __ jcc(NOT_EQUAL, &slow_path);

  // or frame->function()->isGenerator().
  __ movq(r_scratch, Address(kFrameReg, Frame::kLocalsOffset));
  __ movq(r_scratch,
          Address(r_scratch, Frame::kFunctionOffsetFromLocals * kPointerSize));
  __ testq(Address(r_scratch, heapObjectDisp(Function::kFlagsOffset)),
           Immediate(SmallInt::fromWord(Function::kGenerator).raw()));
  __ jcc(NOT_ZERO, &slow_path);

  // Fast path: pop return value, restore caller frame, push return value.
  __ popq(RAX);
  __ movq(kFrameReg, Address(kFrameReg, Frame::kPreviousFrameOffset));
  emitRestoreInterpreterState(as, kVMStack | kBytecode | kVMPC);
  __ pushq(RAX);
  emitNextOpcode(as);

  __ bind(&slow_path);
  emitSaveInterpreterState(as, kVMStack | kVMFrame);
  const word handler_offset =
      -(Interpreter::kNumContinues -
        static_cast<int>(Interpreter::Continue::RETURN)) *
      kHandlerSize;
  __ leaq(RAX, Address(kHandlersBaseReg, handler_offset));
  __ jmp(RAX);
}

template <typename T>
T readBytes(void* addr) {
  T dest;
  std::memcpy(&dest, addr, sizeof(dest));
  return dest;
}

template <typename T>
void writeBytes(void* addr, T value) {
  std::memcpy(addr, &value, sizeof(value));
}

}  // namespace

Interpreter::AsmInterpreter generateInterpreter() {
  Assembler assembler;
  Assembler* as = &assembler;  // For __ macro

  // Set up a frame and save callee-saved registers we'll use.
  __ pushq(RBP);
  __ movq(RBP, RSP);
  for (Register r : kUsedCalleeSavedRegs) {
    __ pushq(r);
  }
  __ pushq(kArgRegs[1]);  // entry_frame

  __ movq(kThreadReg, kArgRegs[0]);
  __ movq(kFrameReg, Address(kThreadReg, Thread::currentFrameOffset()));

  // Materialize the handler base address into a register. The offset will be
  // patched right before emitting the first handler.
  const int32_t dummy_offset = 0xdeadbeef;
  __ leaq(kHandlersBaseReg, Address::addressRIPRelative(dummy_offset));
  word post_lea_size = as->codeSize();
  char* lea_offset_addr = reinterpret_cast<char*>(
      as->codeAddress(as->codeSize() - sizeof(int32_t)));
  CHECK(readBytes<int32_t>(lea_offset_addr) == dummy_offset,
        "Unexpected leaq encoding");

  // Load VM state into registers and jump to the first opcode handler.
  emitRestoreInterpreterState(as, kAllState);
  emitNextOpcode(as);

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
    HandlerSizer sizer(as, kHandlerSize);
    __ movq(kArgRegs[0], kThreadReg);
    __ movq(kArgRegs[1], Address(RBP, kEntryFrameOffset));
    __ movq(RAX, Immediate(reinterpret_cast<word>(Interpreter::unwind)));
    __ call(RAX);
    __ cmpb(RAX, Immediate(0));
    __ jcc(NOT_EQUAL, &do_return, false);
    emitRestoreInterpreterState(as, kAllState);
    emitNextOpcode(as);
  }

  // RETURN pseudo-handler
  static_assert(static_cast<int>(Interpreter::Continue::RETURN) == 2,
                "Unexpected RETURN value");
  {
    HandlerSizer sizer(as, kHandlerSize);
    __ movq(kArgRegs[0], kThreadReg);
    __ movq(kArgRegs[1], Address(RBP, kEntryFrameOffset));
    __ movq(RAX, Immediate(reinterpret_cast<word>(Interpreter::handleReturn)));
    __ call(RAX);
    __ cmpb(RAX, Immediate(0));
    __ jcc(NOT_EQUAL, &do_return, false);
    emitRestoreInterpreterState(as, kAllState);
    emitNextOpcode(as);
  }

  // YIELD pseudo-handler
  static_assert(static_cast<int>(Interpreter::Continue::YIELD) == 3,
                "Unexpected YIELD value");
  {
    HandlerSizer sizer(as, kHandlerSize);
    __ jmp(&do_return, false);
  }

  // Mark the beginning of the opcode handlers and emit them at regular
  // intervals.
  writeBytes<int32_t>(lea_offset_addr, as->codeSize() - post_lea_size);
#define BC(name, i, handler)                                                   \
  {                                                                            \
    HandlerSizer sizer(as, kHandlerSize);                                      \
    emitHandler<name>(as);                                                     \
  }
  FOREACH_BYTECODE(BC)
#undef BC

  // Finalize the code.
  word size = as->codeSize();
  byte* code = OS::allocateMemory(size, &size);
  as->finalizeInstructions(MemoryRegion(code, size));
  OS::protectMemory(code, size, OS::kReadExecute);
  return reinterpret_cast<Interpreter::AsmInterpreter>(code);
}

}  // namespace python
