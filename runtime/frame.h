#pragma once

#include <cstring>

#include "bytecode.h"
#include "globals.h"
#include "handles.h"
#include "objects.h"

namespace py {

// TryBlock contains the unmarshaled block stack information.
//
// Block stack entries are encoded and stored on the stack as a single
// SmallInt using the following format:
//
// Name    Size    Description
// ----------------------------------------------------
// Kind    2       The kind of block this entry represents.
// Handler 30      Where to jump to find the handler
// Level   25      Value stack level to pop to
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

 private:
  uword value_;
};

// A stack frame.
//
// Prior to a function call, the stack will look like
//
//     Function
//     Arg 0
//     ...
//     Arg N
//            <- Top of stack / lower memory addresses
//
// The function prologue is responsible for reserving space for local variables
// and pushing other frame metadata needed by the interpreter onto the stack.
// After the prologue, and immediately before the interpreter is re-invoked,
// the stack looks like:
//
//     Implicit Globals[1]
//     Function
//     Arg 0 <------------------------------------------------+
//     ...                                                    |
//     Arg N                                                  |
//     Locals 0                                               |
//     ...                                                    |
//     Locals N                                               |
//     +-------------------------------+ Frame (fixed size)   |
//     |+----------------+ BlockStack  |                      |
//     || Blockstack top |             |                      |
//     || .              | ^           |                      |
//     || .              | |           |                      |
//     || . entries      | | growth    |                      |
//     |+----------------+             |                      |
//     | Blockstack Depth/Return Mode  |                      |
//     | Locals Offset ----------------|----------------------+
//     | Virtual PC                    |
//     | Previous frame ptr            |<-+ <--Frame pointer
//     +-------------------------------+
//     .                               .
//     .                  | growth     .
//     . Value stack      |            .
//     .                  v            .
//     +...............................+
//
// [1] Only available for non-optimized functions started via
// `Thread::runClassFunction()` or `Thread::exec()`. for example, module- and
// class-body function.
//
//
// Implicit Globals
// ================
// Python code started via `Thread::runClassFunction()` or `Thread::exec()`
// which is used for things like module- and class-bodies or `eval()` may store
// their local variables in arbitrary mapping objects. In this case the
// functions will have the OPTIMIZED and NEWLOCALS flags cleared and the
// bytecode will use STORE_NAME/LOAD_NAME rather than STORE_FAST/LOAD_FAST.
//
// We use the term implicit globals in accordance with the Python language
// reference. Note that CPython code and APIs often use the term "locals"
// instead. We do not use that term to avoid confusion with fast locals.
//
// In our system the implicit globals part of the frame only exists for
// functions that use them. It may contain an arbitrary mapping or `None`.
// `None` is a performance optimization in our system. It indicates that we
// directly write into the globals / `function().moduleObject()` instead of
// using the `implicitGlobals()` this way we can skip setting up a `ModuleProxy`
// object for this case and avoid the extra indirection.
class Frame {
 public:
  enum ReturnMode {
    kNormal = 0,
    kExitRecursiveInterpreter = 1 << 0,
  };

  // Returns true if this frame is for a built-in or extension function. This
  // means no bytecode exists and functions like virtualPC() or caches() must
  // not be used.
  bool isNative();

  // Function arguments, local variables, cell variables, and free variables
  RawObject local(word idx);
  void setLocal(word idx, RawObject value);

  RawObject localWithReverseIndex(word reverse_idx);
  void setLocalWithReverseIndex(word reverse_idx, RawObject value);

  RawFunction function();

  word blockStackDepthReturnMode();
  void setBlockStackDepthReturnMode(word value);

  bool blockStackEmpty();
  TryBlock blockStackPeek();
  TryBlock blockStackPop();
  void blockStackPush(TryBlock block);

  void addReturnMode(word mode);
  word returnMode();

  // Index in the bytecode array of the next instruction to be executed.
  word virtualPC();
  void setVirtualPC(word pc);

  word localsOffset();
  void setLocalsOffset(word locals_offset);

  // Index in the bytecode array of the instruction currently being executed.
  word currentPC();

  // The implicit globals namespace. This is only available when the
  // code does not have OPTIMIZED and NEWLOCALS flags set. See the class
  // comment for details.
  RawObject implicitGlobals();

  RawMutableBytes bytecode();
  void setBytecode(RawMutableBytes bytecode);

  RawObject caches();
  void setCaches(RawObject caches);

  RawObject code();

  // A pointer to the previous frame or nullptr if this is the first frame
  Frame* previousFrame();
  void setPreviousFrame(Frame* frame);

  // Returns a pointer to the end of the frame including locals / parameters.
  RawObject* frameEnd() {
    // The locals() pointer points at the first local, so we need + 1 to skip
    // the first local and another +1 to skip the function reference before.
    return locals() + kFunctionOffsetFromLocals + 1;
  }

  bool isSentinel();

  // Versions of valueStackTop() and popValue() for a Frame that's had
  // stashStackSize() called on it.
  RawObject* stashedValueStackTop();
  RawObject stashedPopValue();

  // Encode value stack size into the "previous frame" field. This
  // representation is used for paused GeneratorFrame objects on the heap.
  void stashStackSize(word size);

  // Compute the total space required for a frame object
  static word allocationSize(RawObject code);

  // Returns nullptr if the frame is well formed, otherwise an error message.
  const char* isInvalid();

  static const int kMaxBlockStackDepth = 20;

  static const int kBytecodeOffset = 0;
  static const int kCachesOffset = kBytecodeOffset + kPointerSize;
  static const int kPreviousFrameOffset = kCachesOffset + kPointerSize;
  static const int kVirtualPCOffset = kPreviousFrameOffset + kPointerSize;
  static const int kLocalsOffsetOffset = kVirtualPCOffset + kPointerSize;
  static const int kBlockStackDepthReturnModeOffset =
      kLocalsOffsetOffset + kPointerSize;
  static const int kBlockStackOffset =
      kBlockStackDepthReturnModeOffset + kPointerSize;
  static const int kSize =
      kBlockStackOffset + (kMaxBlockStackDepth * kPointerSize);

  static const int kFunctionOffsetFromLocals = 0;
  static const int kImplicitGlobalsOffsetFromLocals = 1;

  // A large PC value represents finished generators. It must be an even number
  // to fit the constraints of `setVirtualPC()`/`virtualPD()`.
  static const word kFinishedGeneratorPC = RawSmallInt::kMaxValue - 1;

  static const int kBlockStackDepthBits = 32;
  static const word kBlockStackDepthMask =
      (word{1} << kBlockStackDepthBits) - 1;
  static const word kReturnModeOffset = 32;

  // Returns a pointer to the "begin" of where the arguments + locals are stored
  // on the stack. For example local 0 can be found at `locals()[-1]` local 1
  // at `locals()[-2]`.
  RawObject* locals();

 private:
  uword address();
  RawObject at(int offset);
  void atPut(int offset, RawObject value);

  DISALLOW_COPY_AND_ASSIGN(Frame);
};

class FrameVisitor {
 public:
  virtual bool visit(Frame* frame) = 0;
  virtual ~FrameVisitor() = default;
};

class Arguments {
 public:
  Arguments(Frame* frame) : locals_(frame->locals()) {}

  RawObject get(word n) const { return *(locals_ - n - 1); }

 protected:
  RawObject* locals_;
};

RawObject frameLocals(Thread* thread, Frame* frame);

inline bool Frame::isNative() {
  return !code().isCode() || Code::cast(code()).isNative();
}

inline uword Frame::address() { return reinterpret_cast<uword>(this); }

inline RawObject Frame::at(int offset) {
  return *reinterpret_cast<RawObject*>(address() + offset);
}

inline void Frame::atPut(int offset, RawObject value) {
  *reinterpret_cast<RawObject*>(address() + offset) = value;
}

inline word Frame::blockStackDepthReturnMode() {
  return SmallInt::cast(at(kBlockStackDepthReturnModeOffset))
      .asReinterpretedWord();
}

inline void Frame::setBlockStackDepthReturnMode(word value) {
  atPut(kBlockStackDepthReturnModeOffset,
        SmallInt::fromReinterpretedWord(value));
}

inline bool Frame::blockStackEmpty() {
  return (blockStackDepthReturnMode() & kBlockStackDepthMask) == 0;
}

inline TryBlock Frame::blockStackPeek() {
  word depth = blockStackDepthReturnMode() & kBlockStackDepthMask;
  DCHECK(depth > 0, "cannot peek into empty blockstack");
  return TryBlock(at(kBlockStackOffset + depth - kPointerSize));
}

inline TryBlock Frame::blockStackPop() {
  word new_depth_return_mode = blockStackDepthReturnMode() - kPointerSize;
  word new_depth = new_depth_return_mode & kBlockStackDepthMask;
  DCHECK(new_depth >= 0, "block stack underflow");
  TryBlock result(at(kBlockStackOffset + new_depth));
  setBlockStackDepthReturnMode(new_depth_return_mode);
  return result;
}

inline void Frame::blockStackPush(TryBlock block) {
  word depth_return_mode = blockStackDepthReturnMode();
  word depth = depth_return_mode & kBlockStackDepthMask;
  DCHECK(depth < kMaxBlockStackDepth * kPointerSize, "block stack overflow");
  atPut(kBlockStackOffset + depth, block.asSmallInt());
  setBlockStackDepthReturnMode(depth_return_mode + kPointerSize);
}

inline void Frame::addReturnMode(word mode) {
  DCHECK(!isNative(), "Cannot set return mode on native frames");
  word blockstack_depth_return_mode = blockStackDepthReturnMode();
  setBlockStackDepthReturnMode(blockstack_depth_return_mode |
                               (mode << kReturnModeOffset));
}

inline word Frame::returnMode() {
  return static_cast<ReturnMode>(blockStackDepthReturnMode() >>
                                 kReturnModeOffset);
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

inline word Frame::localsOffset() {
  return SmallInt::cast(at(kLocalsOffsetOffset)).asReinterpretedWord();
}

inline void Frame::setLocalsOffset(word locals_offset) {
  atPut(kLocalsOffsetOffset, SmallInt::fromReinterpretedWord(locals_offset));
}

inline word Frame::currentPC() {
  return SmallInt::cast(at(kVirtualPCOffset)).asReinterpretedWord() -
         kCodeUnitSize;
}

inline RawObject Frame::implicitGlobals() {
  DCHECK(previousFrame() != nullptr, "must not be called on initial frame");
  DCHECK(!function().hasOptimizedOrNewlocals(),
         "implicit globals not available");
  // Thread::exec() and Thread::runClassFunction() place implicit globals there.
  return *(locals() + kImplicitGlobalsOffsetFromLocals);
}

inline RawObject Frame::code() { return function().code(); }

inline RawObject* Frame::locals() {
  return reinterpret_cast<RawObject*>(reinterpret_cast<char*>(this) +
                                      localsOffset());
}

inline RawObject Frame::local(word idx) {
  DCHECK_INDEX(idx, function().totalLocals());
  return *(locals() - idx - 1);
}

inline RawObject Frame::localWithReverseIndex(word reverse_idx) {
  DCHECK_INDEX(reverse_idx, function().totalLocals());
  RawObject* locals_end = reinterpret_cast<RawObject*>(address() + kSize);
  return locals_end[reverse_idx];
}

inline void Frame::setLocal(word idx, RawObject value) {
  DCHECK_INDEX(idx, function().totalLocals());
  *(locals() - idx - 1) = value;
}

inline void Frame::setLocalWithReverseIndex(word reverse_idx, RawObject value) {
  DCHECK_INDEX(reverse_idx, function().totalLocals());
  RawObject* locals_end = reinterpret_cast<RawObject*>(address() + kSize);
  locals_end[reverse_idx] = value;
}

inline RawObject Frame::caches() { return at(kCachesOffset); }

inline void Frame::setCaches(RawObject caches) { atPut(kCachesOffset, caches); }

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

inline bool Frame::isSentinel() {
  // This is the same as `previousFrame() == nullptr` but will not fail
  // assertion checks if the field is not a SmallInt.
  return at(kPreviousFrameOffset) == SmallInt::fromWord(0);
}

inline RawObject* Frame::stashedValueStackTop() {
  word depth = SmallInt::cast(at(kPreviousFrameOffset)).value();
  return reinterpret_cast<RawObject*>(this) - depth;
}

inline RawObject Frame::stashedPopValue() {
  RawObject result = *stashedValueStackTop();
  // valueStackTop() contains the stack depth as a RawSmallInt rather than a
  // pointer, so decrement it by 1.
  word depth = SmallInt::cast(at(kPreviousFrameOffset)).value();
  atPut(kPreviousFrameOffset, SmallInt::fromWord(depth - 1));
  return result;
}

inline void Frame::stashStackSize(word size) {
  atPut(kPreviousFrameOffset, SmallInt::fromWord(size));
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

}  // namespace py
