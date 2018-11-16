#pragma once

#include "globals.h"
#include "objects.h"

namespace python {

class Code;
class Object;

/**
 * TryBlock contains the unmarshaled block stack information.
 *
 * Block stack entries are encoded and stored on the stack as a single
 * SmallInteger using the following format:
 *
 * Name    Size    Description
 * ----------------------------------------------------
 * Kind    8       The kind of block this entry represents. CPython uses the
 *                 raw opcode, but AFAICT, only 4 are actually used.
 * Handler 30      Where to jump to find the handler
 * Level   25      Value stack level to pop to
 */
class TryBlock {
 public:
  TryBlock(word kind, word handler, word level)
      : kind_(kind), handler_(handler), level_(level) {}

  inline static TryBlock fromSmallInteger(Object* object);
  inline Object* asSmallInteger();

  word kind() {
    return kind_;
  }

  word handler() {
    return handler_;
  }

  word level() {
    return level_;
  }

 private:
  word kind_;
  word handler_;
  word level_;

  static const int kKindSize = 8;
  static const uword kKindMask = (1 << kKindSize) - 1;

  static const int kHandlerSize = 30;
  static const int kHandlerOffset = 8;
  static const uword kHandlerMask = (1 << kHandlerSize) - 1;

  static const int kLevelSize = 25;
  static const int kLevelOffset = 38;
  static const uword kLevelMask = (1 << kLevelSize) - 1;
};

// TODO: Determine maximum block stack depth when the code object is loaded and
//       dynamically allocate the minimum amount of space for the block stack.
const int kMaxBlockStackDepth = 20;

/**
 * A stack frame.
 *
 * Prior to a function call, the stack will look like
 *
 *     Function
 *     Arg 0
 *     ...
 *     Arg N
 *            <- Top of stack / lower memory addresses
 *
 * The function prologue is responsible for reserving space for local variables
 * and pushing other frame metadata needed by the interpreter onto the stack.
 * After the prologue, and immediately before the interpreter is re-invoked,
 * the stack looks like:
 *
 *     Function
 *     Arg 0
 *     ...
 *     Arg N
 *     Locals 0
 *     ...
 *     Locals N
 *     <Fixed size frame metadata>
 *     Builtins
 *     Globals
 *     Code Object
 *     Stack top
 *     Saved frame pointer  <- Frame pointer
 *                          <- Top of stack / lower memory addresses
 */
class Frame {
 public:
  // Function arguments, local variables, cell variables, and free variables
  // NB: This points at the last local, so you'll need to flip your array
  // indices around.
  inline Object** locals();

  // The block stack; its size is kMaxBlockStackSize. Entries are SmallInteger
  // objects that can be decoded into a TryBlock.
  inline Object** blockStack();

  // Index of the active block in the block stack; a SmallInteger
  inline Object* activeBlock();
  inline void setActiveBlock(Object* activeBlock);

  // Trace function
  inline Object* traceFunc();
  inline void setTraceFunc(Object* traceFunc);

  // Tracing flags: emit per-line/per-opcode events?
  inline Object* traceFlags();
  inline void setTraceFlags(Object* traceFlags);

  // Index in the bytecode array of the last instruction that was executed
  inline Object* lastInstruction();
  inline void setLastInstruction(Object* lastInstruction);

  // The builtins namespace (a Dictionary)
  inline Object* builtins();
  inline void setBuiltins(Object* builtins);

  // The globals namespace (a Dictionary)
  inline Object* globals();
  inline void setGlobals(Object* globals);

  // The code object
  inline Object* code();
  inline void setCode(Object* code);

  // A pointer to the previous frame or nullptr if this is the first frame
  inline Frame* previousFrame();
  inline void setPreviousFrame(Frame* frame);

  // A pointer to the top of the value stack
  inline Object** top();
  inline void setTop(Object** top);

  // A pointer to the base of the value stack
  inline Object** base();

  // Return the object at offset from the top of the value stack (e.g. peek(0)
  // returns the top of the stack)
  inline Object* peek(word offset);

  // A function lives immediately below the arguments on the value stack
  inline Function* function(word argc);

  inline bool isSentinelFrame();
  void makeSentinel();

  // Compute the total space required for a frame object
  static word allocationSize(Object* code);

  static const int kPreviousFrameOffset = 0;
  static const int kTopOffset = kPreviousFrameOffset + kPointerSize;
  static const int kCodeOffset = kTopOffset + kPointerSize;
  static const int kGlobalsOffset = kCodeOffset + kPointerSize;
  static const int kBuiltinsOffset = kGlobalsOffset + kPointerSize;
  static const int kLastInstructionOffset = kBuiltinsOffset + kPointerSize;
  static const int kTraceFlagsOffset = kLastInstructionOffset + kPointerSize;
  static const int kTraceFuncOffset = kTraceFlagsOffset + kPointerSize;
  static const int kActiveBlockOffset = kTraceFuncOffset + kPointerSize;
  static const int kBlockStackOffset = kActiveBlockOffset + kPointerSize;
  static const int kSize =
      kBlockStackOffset + kMaxBlockStackDepth * kPointerSize;

 private:
  inline uword address();
  inline Object* at(int offset);
  inline void atPut(int offset, Object* value);

  DISALLOW_COPY_AND_ASSIGN(Frame);
};

uword Frame::address() {
  return reinterpret_cast<uword>(this);
}

Object* Frame::at(int offset) {
  return *reinterpret_cast<Object**>(address() + offset);
}

void Frame::atPut(int offset, Object* value) {
  *reinterpret_cast<Object**>(address() + offset) = value;
}

Object** Frame::blockStack() {
  return reinterpret_cast<Object**>(address() + kBlockStackOffset);
}

Object* Frame::activeBlock() {
  return at(kActiveBlockOffset);
}

void Frame::setActiveBlock(Object* activeBlock) {
  atPut(kActiveBlockOffset, activeBlock);
}

Object* Frame::traceFunc() {
  return at(kTraceFuncOffset);
}

void Frame::setTraceFunc(Object* traceFunc) {
  atPut(kTraceFuncOffset, traceFunc);
}

Object* Frame::traceFlags() {
  return at(kTraceFlagsOffset);
}

void Frame::setTraceFlags(Object* traceFlags) {
  atPut(kTraceFlagsOffset, traceFlags);
}

Object* Frame::lastInstruction() {
  return at(kLastInstructionOffset);
}

void Frame::setLastInstruction(Object* lastInstruction) {
  atPut(kLastInstructionOffset, lastInstruction);
}

Object* Frame::builtins() {
  return at(kBuiltinsOffset);
}

void Frame::setBuiltins(Object* builtins) {
  atPut(kBuiltinsOffset, builtins);
}

Object* Frame::globals() {
  return at(kGlobalsOffset);
}

void Frame::setGlobals(Object* globals) {
  atPut(kGlobalsOffset, globals);
}

Object* Frame::code() {
  return at(kCodeOffset);
}

void Frame::setCode(Object* code) {
  atPut(kCodeOffset, code);
}

Object** Frame::locals() {
  return reinterpret_cast<Object**>(address() + kSize);
}

Frame* Frame::previousFrame() {
  Object* frame = at(kPreviousFrameOffset);
  return reinterpret_cast<Frame*>(SmallInteger::cast(frame)->value());
}

void Frame::setPreviousFrame(Frame* frame) {
  atPut(
      kPreviousFrameOffset,
      SmallInteger::fromWord(reinterpret_cast<uword>(frame)));
}

Object** Frame::base() {
  return reinterpret_cast<Object**>(this);
}

Object** Frame::top() {
  Object* top = at(kTopOffset);
  return reinterpret_cast<Object**>(SmallInteger::cast(top)->value());
}

void Frame::setTop(Object** top) {
  atPut(kTopOffset, SmallInteger::fromWord(reinterpret_cast<uword>(top)));
}

Object* Frame::peek(word offset) {
  assert(top() + offset < base());
  return *(top() + offset);
}

Function* Frame::function(word argc) {
  return Function::cast(peek(argc));
}

bool Frame::isSentinelFrame() {
  return previousFrame() == nullptr;
}

TryBlock TryBlock::fromSmallInteger(Object* object) {
  word encoded = SmallInteger::cast(object)->value();
  word kind = encoded & kKindMask;
  word handler = (encoded >> kHandlerOffset) & kHandlerMask;
  word level = (encoded >> kLevelOffset) & kLevelMask;
  return {kind, handler, level};
}

Object* TryBlock::asSmallInteger() {
  word encoded = kind();
  encoded |= (handler() << kHandlerOffset);
  encoded |= (level() << kLevelOffset);
  return SmallInteger::fromWord(encoded);
}

} // namespace python
