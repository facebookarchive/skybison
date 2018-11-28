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
 * Kind    8       The kind of block this entry represents. CPython uses the
 *                 raw opcode, but AFAICT, only 4 are actually used.
 * Handler 30      Where to jump to find the handler
 * Level   25      Value stack level to pop to
 */
class TryBlock {
 public:
  explicit TryBlock(RawObject value) {
    DCHECK(value->isSmallInt(), "expected small integer");
    value_ = value.raw();
  }

  TryBlock(word kind, word handler, word level) : value_(0) {
    setKind(kind);
    setHandler(handler);
    setLevel(level);
  }

  RawObject asSmallInt() const;

  word kind() const;
  void setKind(word kind);

  word handler() const;
  void setHandler(word handler);

  word level() const;
  void setLevel(word level);

 private:
  word valueBits(uword offset, uword mask) const;
  void setValueBits(uword offset, uword mask, word value);

  uword value_;

  static const int kKindOffset = RawSmallInt::kTagSize;
  static const int kKindSize = 8;
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

  TryBlock pop();

  TryBlock peek();

  static const int kStackOffset = 0;
  static const int kTopOffset =
      kStackOffset + kMaxBlockStackDepth * kPointerSize;
  static const int kSize = kTopOffset + kPointerSize;

 private:
  uword address();
  RawObject at(int offset);
  void atPut(int offset, RawObject value);

  word top();
  void setTop(word new_top);

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
 *     Function
 *     Arg 0 <------------------------------------------------+
 *     ...                                                    |
 *     Arg N                                                  |
 *     Locals 0                                               |
 *     ...                                                    |
 *     Locals N                                               |
 *     +-------------------------------+ Frame (fixed size)   |
 *     | Fast globals                  |                      |
 *     | Locals -----------------------|----------------------+
 *     | Num locals                    |
 *     |+----------------+ BlockStack  |
 *     || Blockstack top |             |
 *     || .              | ^           |
 *     || .              | |           |
 *     || . entries      | | growth    |
 *     |+----------------+             |
 *     | Virtual PC                    |
 *     | Builtins                      |
 *     | Implicit globals              |
 *     | Globals                       |
 *     | Code object                   |
 *     | Value stack top --------------|--+
 *     | Previous frame ptr            |<-+ <--Frame pointer
 *     +-------------------------------+
 *     .                               .
 *     .                  | growth     .
 *     . Value stack      |            .
 *     .                  v            .
 *     +...............................+
 */
class Frame {
 public:
  // Function arguments, local variables, cell variables, and free variables
  RawObject getLocal(word idx);
  void setLocal(word idx, RawObject local);

  void setNumLocals(word num_locals);
  word numLocals();

  BlockStack* blockStack();

  // Index in the bytecode array of the last instruction that was executed
  word virtualPC();
  void setVirtualPC(word pc);

  // The builtins namespace (a Dict)
  RawObject builtins();
  void setBuiltins(RawObject builtins);

  // The (explicit) globals namespace (a Dict)
  RawObject globals();
  void setGlobals(RawObject globals);

  // The implicit globals namespace (a Dict)
  RawObject implicitGlobals();
  void setImplicitGlobals(RawObject implicit_globals);

  // The pre-computed object array provided fast globals access.
  // fastGlobals[arg] == globals[names[arg]]
  RawObject fastGlobals();
  void setFastGlobals(RawObject fast_globals);

  // The code object
  RawObject code();
  void setCode(RawObject code);

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

  bool isNativeFrame();
  void* nativeFunctionPointer();
  void makeNativeFrame(RawObject fn_pointer_as_int);

  // Adjust and/or save the values of internal pointers after copying this Frame
  // from the stack to the heap.
  void stashInternalPointers(Frame* old_frame);

  // Adjust and/or restore internal pointers after copying this Frame from the
  // heap to the stack.
  void unstashInternalPointers();

  // Compute the total space required for a frame object
  static word allocationSize(RawObject code);

  static const int kPreviousFrameOffset = 0;
  static const int kValueStackTopOffset = kPreviousFrameOffset + kPointerSize;
  static const int kCodeOffset = kValueStackTopOffset + kPointerSize;
  static const int kGlobalsOffset = kCodeOffset + kPointerSize;
  static const int kImplicitGlobalsOffset = kGlobalsOffset + kPointerSize;
  static const int kBuiltinsOffset = kImplicitGlobalsOffset + kPointerSize;
  static const int kVirtualPCOffset = kBuiltinsOffset + kPointerSize;
  static const int kBlockStackOffset = kVirtualPCOffset + kPointerSize;
  static const int kNumLocalsOffset = kBlockStackOffset + BlockStack::kSize;
  static const int kLocalsOffset = kNumLocalsOffset + kPointerSize;
  static const int kFastGlobalsOffset = kLocalsOffset + kPointerSize;
  static const int kSize = kFastGlobalsOffset + kPointerSize;

 private:
  uword address();
  RawObject at(int offset);
  void atPut(int offset, RawObject value);
  RawObject* locals();

  // Re-compute the locals pointer based on this and num_locals.
  void resetLocals(word num_locals);

  DISALLOW_COPY_AND_ASSIGN(Frame);
};

/**
 * A Frame in a HeapObject, with space allocated before and after for stack and
 * locals, respectively. It looks almost exactly like the ascii art diagram for
 * Frame, except that there is a fixed amount of space allocated for the value
 * stack, which comes from stacksize() on the Code object this is created from:
 *
 *   +----------------------+  <--+
 *   | Arg 0                |     |
 *   | ...                  |     |
 *   | Arg N                |     |
 *   | Local 0              |     | frame()-code()->totalVars() * kPointerSize
 *   | ...                  |     |
 *   | Local N              |     |
 *   +----------------------+  <--+
 *   |                      |     |
 *   | Frame                |     | Frame::kSize
 *   |                      |     |
 *   +----------------------+  <--+  <-- frame()
 *   |                      |     |
 *   | Value stack          |     | maxStackSize() * kPointerSize
 *   |                      |     |
 *   +----------------------+  <--+
 *   | maxStackSize         |
 *   +----------------------+
 */
class RawHeapFrame : public RawHeapObject {
 public:
  // The Frame contained in this HeapFrame.
  Frame* frame();

  // The size of the embedded frame + stack and locals, in words.
  word numFrameWords();

  // Get or set the number of words allocated for the value stack. Used to
  // derive a pointer to the Frame inside this HeapFrame.
  word maxStackSize();
  void setMaxStackSize(word offset);

  // Push a new Frame below caller_frame, and copy this HeapFrame into it. Stack
  // overflow checks must be done by the caller. Returns a pointer to the new
  // stack Frame.
  Frame* copyToNewStackFrame(Frame* caller_frame);

  // Copy a Frame from the stack back into this HeapFrame.
  void copyFromStackFrame(Frame* frame);

  // Sizing.
  static word numAttributes(word extra_words);

  // Layout.
  static const int kMaxStackSizeOffset = RawHeapObject::kSize;
  static const int kFrameOffset = kMaxStackSizeOffset + kPointerSize;

  // Number of words that aren't the Frame.
  static const int kNumOverheadWords = kFrameOffset / kPointerSize;

  RAW_OBJECT_COMMON(HeapFrame);
};

class FrameVisitor {
 public:
  virtual void visit(Frame* frame) = 0;
  virtual ~FrameVisitor() = default;
};

class Arguments {
 public:
  Arguments(Frame* frame, word nargs) {
    frame_ = frame;
    num_args_ = nargs;
  }

  RawObject get(word n) const {
    CHECK(n < num_args_, "index out of range");
    return frame_->getLocal(n);
  }

  word numArgs() const { return num_args_; }

 protected:
  Frame* frame_;
  word num_args_;
};

class KwArguments : public Arguments {
 public:
  KwArguments(Frame* frame, word nargs)
      : Arguments(frame, nargs),
        kwnames_(RawTuple::cast(frame->getLocal(nargs - 1))),
        num_keywords_(kwnames_->length()) {
    num_args_ = nargs - num_keywords_ - 1;
  }

  RawObject getKw(RawObject name) const {
    for (word i = 0; i < num_keywords_; i++) {
      if (RawStr::cast(name)->equals(kwnames_->at(i))) {
        return frame_->getLocal(num_args_ + i);
      }
    }
    return Error::object();
  }

  word numKeywords() const { return num_keywords_; }

 private:
  RawTuple kwnames_;
  word num_keywords_;
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

inline word Frame::virtualPC() {
  return RawSmallInt::cast(at(kVirtualPCOffset))->value();
}

inline void Frame::setVirtualPC(word pc) {
  atPut(kVirtualPCOffset, SmallInt::fromWord(pc));
}

inline RawObject Frame::builtins() { return at(kBuiltinsOffset); }

inline void Frame::setBuiltins(RawObject builtins) {
  atPut(kBuiltinsOffset, builtins);
}

inline RawObject Frame::globals() { return at(kGlobalsOffset); }

inline void Frame::setGlobals(RawObject globals) {
  atPut(kGlobalsOffset, globals);
}

inline RawObject Frame::implicitGlobals() { return at(kImplicitGlobalsOffset); }

inline void Frame::setImplicitGlobals(RawObject implicit_globals) {
  atPut(kImplicitGlobalsOffset, implicit_globals);
}

inline RawObject Frame::fastGlobals() { return at(kFastGlobalsOffset); }

inline void Frame::setFastGlobals(RawObject fast_globals) {
  atPut(kFastGlobalsOffset, fast_globals);
}

inline RawObject Frame::code() { return at(kCodeOffset); }

inline void Frame::setCode(RawObject code) { atPut(kCodeOffset, code); }

inline RawObject* Frame::locals() {
  return reinterpret_cast<RawObject*>(at(kLocalsOffset).raw());
}

inline RawObject Frame::getLocal(word idx) {
  DCHECK_INDEX(idx, numLocals());
  return *(locals() - idx);
}

inline void Frame::setLocal(word idx, RawObject object) {
  DCHECK_INDEX(idx, numLocals());
  *(locals() - idx) = object;
}

inline void Frame::setNumLocals(word num_locals) {
  atPut(kNumLocalsOffset, SmallInt::fromWord(num_locals));
  resetLocals(num_locals);
}

inline void Frame::resetLocals(word num_locals) {
  // Bias locals by 1 word to avoid doing so during {get,set}Local
  RawObject locals{address() + Frame::kSize +
                   ((num_locals - 1) * kPointerSize)};
  DCHECK(locals->isSmallInt(), "expected small integer");
  atPut(kLocalsOffset, locals);
}

inline word Frame::numLocals() {
  return RawSmallInt::cast(at(kNumLocalsOffset))->value();
}

inline Frame* Frame::previousFrame() {
  RawObject frame = at(kPreviousFrameOffset);
  return static_cast<Frame*>(RawSmallInt::cast(frame)->asCPtr());
}

inline void Frame::setPreviousFrame(Frame* frame) {
  atPut(kPreviousFrameOffset,
        SmallInt::fromWord(reinterpret_cast<uword>(frame)));
}

inline RawObject* Frame::valueStackBase() {
  return reinterpret_cast<RawObject*>(this);
}

inline RawObject* Frame::valueStackTop() {
  RawObject top = at(kValueStackTopOffset);
  return static_cast<RawObject*>(RawSmallInt::cast(top)->asCPtr());
}

inline void Frame::setValueStackTop(RawObject* top) {
  atPut(kValueStackTopOffset, SmallInt::fromWord(reinterpret_cast<uword>(top)));
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
    pushValue(getLocal(i));
  }
}

inline RawObject Frame::peek(word offset) {
  DCHECK(valueStackTop() + offset < valueStackBase(), "offset %ld overflows",
         offset);
  return *(valueStackTop() + offset);
}

inline bool Frame::isSentinelFrame() { return previousFrame() == nullptr; }

inline void* Frame::nativeFunctionPointer() {
  DCHECK(isNativeFrame(), "not native frame");
  return RawInt::cast(code())->asCPtr();
}

inline bool Frame::isNativeFrame() { return code()->isInt(); }

inline void Frame::makeNativeFrame(RawObject fn_pointer_as_int) {
  DCHECK(fn_pointer_as_int->isInt(), "expected integer");
  setCode(fn_pointer_as_int);
}

inline void Frame::stashInternalPointers(Frame* old_frame) {
  // Replace ValueStackTop with the stack depth while this Frame is on the heap,
  // to survive being moved by the GC.
  setValueStackTop(reinterpret_cast<RawObject*>(old_frame->valueStackSize()));
}

inline void Frame::unstashInternalPointers() {
  auto stashed_size = reinterpret_cast<word>(valueStackTop());
  setValueStackTop(valueStackBase() - stashed_size);
  resetLocals(numLocals());
}

// HeapFrame

inline Frame* RawHeapFrame::frame() {
  return reinterpret_cast<Frame*>(address() + kFrameOffset +
                                  maxStackSize() * kPointerSize);
}

inline word RawHeapFrame::numFrameWords() {
  return header()->count() - kNumOverheadWords;
}

inline word RawHeapFrame::maxStackSize() {
  return RawSmallInt::cast(instanceVariableAt(kMaxStackSizeOffset))->value();
}

inline void RawHeapFrame::setMaxStackSize(word offset) {
  instanceVariableAtPut(kMaxStackSizeOffset, SmallInt::fromWord(offset));
}

inline Frame* RawHeapFrame::copyToNewStackFrame(Frame* caller_frame) {
  auto src_base = reinterpret_cast<RawObject*>(address() + kFrameOffset);
  word frame_words = numFrameWords();
  RawObject* dest_base = caller_frame->valueStackTop() - frame_words;
  std::memcpy(dest_base, src_base, frame_words * kPointerSize);

  auto live_frame = reinterpret_cast<Frame*>(dest_base + maxStackSize());
  live_frame->unstashInternalPointers();
  return live_frame;
}

inline void RawHeapFrame::copyFromStackFrame(Frame* live_frame) {
  auto dest_base = reinterpret_cast<RawObject*>(address() + kFrameOffset);
  RawObject* src_base = live_frame->valueStackBase() - maxStackSize();
  std::memcpy(dest_base, src_base, numFrameWords() * kPointerSize);
  frame()->stashInternalPointers(live_frame);
}

inline word RawHeapFrame::numAttributes(word extra_words) {
  return kNumOverheadWords + Frame::kSize / kPointerSize + extra_words;
}

inline RawObject TryBlock::asSmallInt() const {
  RawObject obj{value_};
  DCHECK(obj->isSmallInt(), "expected small integer");
  return obj;
}

inline word TryBlock::kind() const { return valueBits(kKindOffset, kKindMask); }

inline void TryBlock::setKind(word kind) {
  setValueBits(kKindOffset, kKindMask, kind);
}

inline word TryBlock::handler() const {
  return valueBits(kHandlerOffset, kHandlerMask);
}

inline void TryBlock::setHandler(word handler) {
  setValueBits(kHandlerOffset, kHandlerMask, handler);
}

inline word TryBlock::level() const {
  return valueBits(kLevelOffset, kLevelMask);
}

inline void TryBlock::setLevel(word level) {
  setValueBits(kLevelOffset, kLevelMask, level);
}

inline word TryBlock::valueBits(uword offset, uword mask) const {
  return (value_ >> offset) & mask;
}

inline void TryBlock::setValueBits(uword offset, uword mask, word value) {
  value_ |= (value & mask) << offset;
}

inline uword BlockStack::address() { return reinterpret_cast<uword>(this); }

inline RawObject BlockStack::at(int offset) {
  return *reinterpret_cast<RawObject*>(address() + offset);
}

inline void BlockStack::atPut(int offset, RawObject value) {
  *reinterpret_cast<RawObject*>(address() + offset) = value;
}

inline word BlockStack::top() {
  return RawSmallInt::cast(at(kTopOffset))->value();
}

inline void BlockStack::setTop(word new_top) {
  DCHECK_INDEX(new_top, kMaxBlockStackDepth);
  atPut(kTopOffset, SmallInt::fromWord(new_top));
}

inline TryBlock BlockStack::peek() {
  word stack_top = top() - 1;
  DCHECK(stack_top > -1, "block stack underflow %ld", stack_top);
  RawObject block = at(kStackOffset + stack_top * kPointerSize);
  return TryBlock(block);
}

inline void BlockStack::push(const TryBlock& block) {
  word stack_top = top();
  atPut(kStackOffset + stack_top * kPointerSize, block.asSmallInt());
  setTop(stack_top + 1);
}

inline TryBlock BlockStack::pop() {
  word stack_top = top() - 1;
  DCHECK(stack_top > -1, "block stack underflow %ld", stack_top);
  RawObject block = at(kStackOffset + stack_top * kPointerSize);
  setTop(stack_top);
  return TryBlock(block);
}

}  // namespace python
