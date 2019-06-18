#pragma once

#include "globals.h"
#include "handles.h"
#include "objects.h"

#include <cstring>

namespace python {

/**
 * TryBlock contains the unmarshaled block stack information.
 *
 * Block stack entries are encoded and stored on the stack as a single
 * SmallInt using the following format:
 *
 * Name    Size    Description
 * ----------------------------------------------------
 * Kind    2       The kind of block this entry represents.
 * Handler 30      Where to jump to find the handler
 * Level   25      Value stack level to pop to
 */
class TryBlock {
 public:
  // cpython stores the opcode that pushed the block as the block kind, but only
  // 4 opcodes actually push blocks. Store the same information with fewer bits.
  enum Kind {
    kLoop,
    kExceptHandler,
    kExcept,
    kFinally,
  };

  // Reason code for entering a finally block.
  enum class Why {
    kException,
    kReturn,
    kBreak,
    kContinue,
    kYield,
    kSilenced,
  };

  explicit TryBlock(RawObject value) {
    DCHECK(value.isSmallInt(), "expected small integer");
    value_ = value.raw();
  }

  TryBlock(Kind kind, word handler, word level) {
    DCHECK((handler & ~kHandlerMask) == 0, "handler too big");
    DCHECK((level & ~kLevelMask) == 0, "level too big");
    value_ = static_cast<uword>(kind) << kKindOffset |
             handler << kHandlerOffset | level << kLevelOffset;
  }

  RawObject asSmallInt() const;

  Kind kind() const;

  word handler() const;

  word level() const;

 private:
  uword value_;

  static const int kKindOffset = RawObject::kSmallIntTagBits;
  static const int kKindSize = 2;
  static const uword kKindMask = (1 << kKindSize) - 1;

  static const int kHandlerOffset = kKindOffset + kKindSize;  // 9
  static const int kHandlerSize = 30;
  static const uword kHandlerMask = (1 << kHandlerSize) - 1;

  static const int kLevelOffset = kHandlerOffset + kHandlerSize;  // 39
  static const int kLevelSize = 25;
  static const uword kLevelMask = (1 << kLevelSize) - 1;

  static const int kSize = kLevelOffset + kLevelSize;

  static_assert(kSize <= kBitsPerByte * sizeof(uword),
                "TryBlock must fit into a uword");
};

// TODO(mpage): Determine maximum block stack depth when the code object is
// loaded and dynamically allocate the minimum amount of space for the block
// stack.
const int kMaxBlockStackDepth = 20;

class BlockStack {
 public:
  void push(const TryBlock& block);

  word depth();

  TryBlock pop();

  TryBlock peek();

  void setDepth(word new_top);

  static const int kStackOffset = 0;
  static const int kTopOffset =
      kStackOffset + kMaxBlockStackDepth * kPointerSize;
  static const int kSize = kTopOffset + kPointerSize;

 private:
  uword address();
  RawObject at(int offset);
  void atPut(int offset, RawObject value);

  DISALLOW_IMPLICIT_CONSTRUCTORS(BlockStack);
};

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
 *     Implicit Globals[1]
 *     Function
 *     Arg 0 <------------------------------------------------+
 *     ...                                                    |
 *     Arg N                                                  |
 *     Locals 0                                               |
 *     ...                                                    |
 *     Locals N                                               |
 *     +-------------------------------+ Frame (fixed size)   |
 *     | Locals -----------------------|----------------------+
 *     | Num locals                    |
 *     |+----------------+ BlockStack  |
 *     || Blockstack top |             |
 *     || .              | ^           |
 *     || .              | |           |
 *     || . entries      | | growth    |
 *     |+----------------+             |
 *     | Virtual PC                    |
 *     | Value stack top --------------|--+
 *     | Previous frame ptr            |<-+ <--Frame pointer
 *     +-------------------------------+
 *     .                               .
 *     .                  | growth     .
 *     . Value stack      |            .
 *     .                  v            .
 *     +...............................+
 *
 * [1] Implicit Globals are only available for non-optimized functions started
 * via `Thread::runClassFunction()` or `Thread::exec()` such as module- or
 * class-bodies.
 */
class Frame {
 public:
  // Function arguments, local variables, cell variables, and free variables
  RawObject local(word idx);
  void setLocal(word idx, RawObject value);

  RawObject localWithReverseIndex(word reverse_idx);
  void setLocalWithReverseIndex(word reverse_idx, RawObject value);

  RawFunction function();

  void setNumLocals(word num_locals);
  word numLocals();

  BlockStack* blockStack();

  // Index in the bytecode array of the last instruction that was executed
  word virtualPC();
  void setVirtualPC(word pc);

  // The implicit globals namespace (a Dict). This is only available when the
  // code does not have OPTIMIZED and NEWLOCALS flags set.
  RawObject implicitGlobals();

  RawMutableBytes bytecode();
  void setBytecode(RawMutableBytes bytecode);

  RawTuple caches();
  void setCaches(RawTuple caches);

  RawObject code();

  // A pointer to the previous frame or nullptr if this is the first frame
  Frame* previousFrame();
  void setPreviousFrame(Frame* frame);

  // A pointer to the top of the value stack
  RawObject* valueStackTop();
  void setValueStackTop(RawObject* top);

  RawObject* valueStackBase();

  // Returns the number of items on the value stack
  word valueStackSize();

  // Push value on the stack.
  void pushValue(RawObject value);
  // Insert value at offset on the stack.
  void insertValueAt(RawObject value, word offset);
  // Set value at offset on the stack.
  void setValueAt(RawObject value, word offset);
  // Pop the top value off the stack and return it.
  RawObject popValue();
  // Pop n items off the stack.
  void dropValues(word count);
  // Return the top value of the stack.
  RawObject topValue();
  // Set the top value of the stack.
  void setTopValue(RawObject value);

  // Return the object at offset from the top of the value stack (e.g. peek(0)
  // returns the top of the stack)
  RawObject peek(word offset);

  // Push locals at [offset, offset + count) onto the stack
  void pushLocals(word count, word offset);

  bool isSentinelFrame();
  void makeSentinel();

  // Versions of valueStackTop() and popValue() for a Frame that's had
  // stashInternalPointers() called on it.
  RawObject* stashedValueStackTop();
  RawObject stashedPopValue();

  // Adjust and/or save the values of internal pointers after copying this Frame
  // from the stack to the heap.
  void stashInternalPointers(Frame* old_frame);

  // Adjust and/or restore internal pointers after copying this Frame from the
  // heap to the stack.
  void unstashInternalPointers();

  // Compute the total space required for a frame object
  static word allocationSize(RawObject code);

  // Returns nullptr if the frame is well formed, otherwise an error message.
  const char* isInvalid();

  static const int kBytecodeOffset = 0;
  static const int kCachesOffset = kBytecodeOffset + kPointerSize;
  static const int kPreviousFrameOffset = kCachesOffset + kPointerSize;
  static const int kValueStackTopOffset = kPreviousFrameOffset + kPointerSize;
  static const int kVirtualPCOffset = kValueStackTopOffset + kPointerSize;
  static const int kBlockStackOffset = kVirtualPCOffset + kPointerSize;
  static const int kNumLocalsOffset = kBlockStackOffset + BlockStack::kSize;
  static const int kLocalsOffset = kNumLocalsOffset + kPointerSize;
  static const int kSize = kLocalsOffset + kPointerSize;

  static const int kFunctionOffsetFromLocals = 1;
  static const int kImplicitGlobalsOffsetFromLocals = 2;

  // A large PC value represents finished generators. It must be an even number
  // to fit the constraints of `setVirtualPC()`/`virtualPD()`.
  static const word kFinishedGeneratorPC = RawSmallInt::kMaxValue - 1;
  static const word kCodeUnitSize = 2;

 private:
  uword address();
  RawObject at(int offset);
  void atPut(int offset, RawObject value);
  RawObject* locals();

  // Re-compute the locals pointer based on this and num_locals.
  void resetLocals(word num_locals);

  DISALLOW_COPY_AND_ASSIGN(Frame);
};

class FrameVisitor {
 public:
  virtual bool visit(Frame* frame) = 0;
  virtual ~FrameVisitor() = default;
};

class Arguments {
 public:
  Arguments(Frame* frame, word nargs) {
    frame_ = frame;
    num_args_ = nargs;
  }

  RawObject get(word n) const {
    DCHECK(n < num_args_, "index out of range");
    return frame_->localWithReverseIndex(num_args_ - n - 1);
  }

  word numArgs() const { return num_args_; }

 protected:
  Frame* frame_;
  word num_args_;
};

inline uword Frame::address() { return reinterpret_cast<uword>(this); }

inline RawObject Frame::at(int offset) {
  return *reinterpret_cast<RawObject*>(address() + offset);
}

inline void Frame::atPut(int offset, RawObject value) {
  *reinterpret_cast<RawObject*>(address() + offset) = value;
}

inline BlockStack* Frame::blockStack() {
  return reinterpret_cast<BlockStack*>(address() + kBlockStackOffset);
}

inline RawFunction Frame::function() {
  DCHECK(previousFrame() != nullptr, "must not be called on initial frame");
  return Function::cast(*(locals() + kFunctionOffsetFromLocals));
}

inline word Frame::virtualPC() {
  return SmallInt::cast(at(kVirtualPCOffset)).asReinterpretedWord();
}

inline void Frame::setVirtualPC(word pc) {
  // We re-interpret the PC value as a small int. This works because it must
  // be an even number and naturally has the lowest bit cleared.
  atPut(kVirtualPCOffset, SmallInt::fromReinterpretedWord(pc));
}

inline RawObject Frame::implicitGlobals() {
  DCHECK(previousFrame() != nullptr, "must not be called on initial frame");
  DCHECK(!function().hasOptimizedOrNewLocals(),
         "implicit globals not available");
  // Thread::exec() and Thread::runClassFunction() place implicit globals there.
  return *(locals() + kImplicitGlobalsOffsetFromLocals);
}

inline RawObject Frame::code() { return function().code(); }

inline RawObject* Frame::locals() {
  return static_cast<RawObject*>(
      SmallInt::cast(at(kLocalsOffset)).asAlignedCPtr());
}

inline RawObject Frame::local(word idx) {
  DCHECK_INDEX(idx, numLocals());
  return *(locals() - idx);
}

inline RawObject Frame::localWithReverseIndex(word reverse_idx) {
  DCHECK_INDEX(reverse_idx, numLocals());
  RawObject* locals_end = reinterpret_cast<RawObject*>(address() + kSize);
  return locals_end[reverse_idx];
}

inline void Frame::setLocal(word idx, RawObject value) {
  DCHECK_INDEX(idx, numLocals());
  *(locals() - idx) = value;
}

inline void Frame::setLocalWithReverseIndex(word reverse_idx, RawObject value) {
  DCHECK_INDEX(reverse_idx, numLocals());
  RawObject* locals_end = reinterpret_cast<RawObject*>(address() + kSize);
  locals_end[reverse_idx] = value;
}

inline void Frame::setNumLocals(word num_locals) {
  atPut(kNumLocalsOffset, SmallInt::fromWord(num_locals));
  resetLocals(num_locals);
}

inline void Frame::resetLocals(word num_locals) {
  // Bias locals by 1 word to avoid doing so during {get,set}Local
  RawObject* locals = reinterpret_cast<RawObject*>(
      address() + Frame::kSize + ((num_locals - 1) * kPointerSize));
  atPut(kLocalsOffset, SmallInt::fromAlignedCPtr(locals));
}

inline word Frame::numLocals() {
  return SmallInt::cast(at(kNumLocalsOffset)).value();
}

inline RawTuple Frame::caches() { return RawTuple::cast(at(kCachesOffset)); }

inline void Frame::setCaches(RawTuple caches) { atPut(kCachesOffset, caches); }

inline RawMutableBytes Frame::bytecode() {
  return RawMutableBytes::cast(at(kBytecodeOffset));
}

inline void Frame::setBytecode(RawMutableBytes bytecode) {
  atPut(kBytecodeOffset, bytecode);
}

inline Frame* Frame::previousFrame() {
  RawObject frame = at(kPreviousFrameOffset);
  return static_cast<Frame*>(SmallInt::cast(frame).asAlignedCPtr());
}

inline void Frame::setPreviousFrame(Frame* frame) {
  atPut(kPreviousFrameOffset,
        SmallInt::fromAlignedCPtr(reinterpret_cast<void*>(frame)));
}

inline RawObject* Frame::valueStackBase() {
  return reinterpret_cast<RawObject*>(this);
}

inline RawObject* Frame::valueStackTop() {
  RawObject top = at(kValueStackTopOffset);
  return static_cast<RawObject*>(SmallInt::cast(top).asAlignedCPtr());
}

inline void Frame::setValueStackTop(RawObject* top) {
  atPut(kValueStackTopOffset, SmallInt::fromAlignedCPtr(top));
}

inline word Frame::valueStackSize() {
  return valueStackBase() - valueStackTop();
}

inline void Frame::pushValue(RawObject value) {
  RawObject* top = valueStackTop();
  *--top = value;
  setValueStackTop(top);
}

inline void Frame::insertValueAt(RawObject value, word offset) {
  DCHECK(valueStackTop() + offset <= valueStackBase(), "offset %ld overflows",
         offset);
  RawObject* sp = valueStackTop() - 1;
  for (word i = 0; i < offset; i++) {
    sp[i] = sp[i + 1];
  }
  sp[offset] = value;
  setValueStackTop(sp);
}

inline void Frame::setValueAt(RawObject value, word offset) {
  DCHECK(valueStackTop() + offset < valueStackBase(), "offset %ld overflows",
         offset);
  *(valueStackTop() + offset) = value;
}

inline RawObject Frame::popValue() {
  DCHECK(valueStackTop() + 1 <= valueStackBase(), "offset %d overflows", 1);
  RawObject result = *valueStackTop();
  setValueStackTop(valueStackTop() + 1);
  return result;
}

inline void Frame::dropValues(word count) {
  DCHECK(valueStackTop() + count <= valueStackBase(), "count %ld overflows",
         count);
  setValueStackTop(valueStackTop() + count);
}

inline RawObject Frame::topValue() { return peek(0); }

inline void Frame::setTopValue(RawObject value) { *valueStackTop() = value; }

inline void Frame::pushLocals(word count, word offset) {
  DCHECK(offset + count <= numLocals(), "locals overflow");
  for (word i = offset; i < offset + count; i++) {
    pushValue(local(i));
  }
}

inline RawObject Frame::peek(word offset) {
  DCHECK(valueStackTop() + offset < valueStackBase(), "offset %ld overflows",
         offset);
  return *(valueStackTop() + offset);
}

inline bool Frame::isSentinelFrame() {
  // This is the same as `previousFrame() == nullptr` but will not fail
  // assertion checks if the field is not a SmallInt.
  return at(kPreviousFrameOffset) == SmallInt::fromWord(0);
}

inline RawObject* Frame::stashedValueStackTop() {
  word depth = SmallInt::cast(at(kValueStackTopOffset)).value();
  return valueStackBase() - depth;
}

inline RawObject Frame::stashedPopValue() {
  RawObject result = *stashedValueStackTop();
  // valueStackTop() contains the stack depth as a RawSmallInt rather than a
  // pointer, so decrement it by 1.
  word depth = SmallInt::cast(at(kValueStackTopOffset)).value();
  atPut(kValueStackTopOffset, SmallInt::fromWord(depth - 1));
  return result;
}

inline void Frame::stashInternalPointers(Frame* old_frame) {
  // Replace ValueStackTop with the stack depth while this Frame is on the heap,
  // to survive being moved by the GC.
  word depth = old_frame->valueStackSize();
  atPut(kValueStackTopOffset, SmallInt::fromWord(depth));
}

inline void Frame::unstashInternalPointers() {
  setValueStackTop(stashedValueStackTop());
  resetLocals(numLocals());
}

inline RawObject TryBlock::asSmallInt() const {
  RawObject obj{value_};
  DCHECK(obj.isSmallInt(), "expected small integer");
  return obj;
}

inline TryBlock::Kind TryBlock::kind() const {
  return static_cast<Kind>((value_ >> kKindOffset) & kKindMask);
}

inline word TryBlock::handler() const {
  return (value_ >> kHandlerOffset) & kHandlerMask;
}

inline word TryBlock::level() const {
  return (value_ >> kLevelOffset) & kLevelMask;
}

inline uword BlockStack::address() { return reinterpret_cast<uword>(this); }

inline RawObject BlockStack::at(int offset) {
  return *reinterpret_cast<RawObject*>(address() + offset);
}

inline void BlockStack::atPut(int offset, RawObject value) {
  *reinterpret_cast<RawObject*>(address() + offset) = value;
}

inline word BlockStack::depth() {
  return SmallInt::cast(at(kTopOffset)).value();
}

inline void BlockStack::setDepth(word new_top) {
  DCHECK_INDEX(new_top, kMaxBlockStackDepth);
  atPut(kTopOffset, SmallInt::fromWord(new_top));
}

inline TryBlock BlockStack::peek() {
  word stack_top = depth() - 1;
  DCHECK(stack_top > -1, "block stack underflow %ld", stack_top);
  RawObject block = at(kStackOffset + stack_top * kPointerSize);
  return TryBlock(block);
}

inline void BlockStack::push(const TryBlock& block) {
  word stack_top = depth();
  atPut(kStackOffset + stack_top * kPointerSize, block.asSmallInt());
  setDepth(stack_top + 1);
}

inline TryBlock BlockStack::pop() {
  word stack_top = depth() - 1;
  DCHECK(stack_top > -1, "block stack underflow %ld", stack_top);
  RawObject block = at(kStackOffset + stack_top * kPointerSize);
  setDepth(stack_top);
  return TryBlock(block);
}

}  // namespace python
