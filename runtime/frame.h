#pragma once

#include "globals.h"
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
  explicit TryBlock(Object* value) {
    DCHECK(value->isSmallInt(), "expected small integer");
    value_ = reinterpret_cast<uword>(value);
  }

  TryBlock(word kind, word handler, word level) : value_(0) {
    setKind(kind);
    setHandler(handler);
    setLevel(level);
  }

  Object* asSmallInt() const;

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

  static const int kKindOffset = SmallInt::kTagSize;
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

// TODO: Determine maximum block stack depth when the code object is loaded and
//       dynamically allocate the minimum amount of space for the block stack.
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
  Object* at(int offset);
  void atPut(int offset, Object* value);

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
  Object* getLocal(word idx);
  void setLocal(word idx, Object* local);

  void setNumLocals(word num_locals);
  word numLocals();

  BlockStack* blockStack();

  // Index in the bytecode array of the last instruction that was executed
  word virtualPC();
  void setVirtualPC(word pc);

  // The builtins namespace (a Dict)
  Object* builtins();
  void setBuiltins(Object* builtins);

  // The (explicit) globals namespace (a Dict)
  Object* globals();
  void setGlobals(Object* globals);

  // The implicit globals namespace (a Dict)
  Object* implicitGlobals();
  void setImplicitGlobals(Object* implicit_globals);

  // The pre-computed object array provided fast globals access.
  // fastGlobals[arg] == globals[names[arg]]
  Object* fastGlobals();
  void setFastGlobals(Object* fast_globals);

  // The code object
  Object* code();
  void setCode(Object* code);

  // A pointer to the previous frame or nullptr if this is the first frame
  Frame* previousFrame();
  void setPreviousFrame(Frame* frame);

  // A pointer to the top of the value stack
  Object** valueStackTop();
  void setValueStackTop(Object** top);

  Object** valueStackBase();

  // Returns the number of items on the value stack
  word valueStackSize();

  // Push value on the stack.
  void pushValue(Object* value);
  // Insert value at offset on the stack.
  void insertValueAt(Object* value, word offset);
  // Set value at offset on the stack.
  void setValueAt(Object* value, word offset);
  // Pop the top value off the stack and return it.
  Object* popValue();
  // Pop n items off the stack.
  void dropValues(word count);
  // Return the top value of the stack.
  Object* topValue();
  // Set the top value of the stack.
  void setTopValue(Object* value);

  // Return the object at offset from the top of the value stack (e.g. peek(0)
  // returns the top of the stack)
  Object* peek(word offset);

  // Push locals at [offset, offset + count) onto the stack
  void pushLocals(word count, word offset);

  // A function lives immediately below the arguments on the value stack
  Function* function(word argc);

  bool isSentinelFrame();
  void makeSentinel();

  bool isNativeFrame();
  void* nativeFunctionPointer();
  void makeNativeFrame(Object* fn_pointer_as_int);

  // Adjust and/or save the values of internal pointers after copying this Frame
  // from the stack to the heap.
  void stashInternalPointers(Frame* old_frame);

  // Adjust and/or restore internal pointers after copying this Frame from the
  // heap to the stack.
  void unstashInternalPointers();

  // Compute the total space required for a frame object
  static word allocationSize(Object* code);

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
  Object* at(int offset);
  void atPut(int offset, Object* value);
  Object** locals();

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
class HeapFrame : public HeapObject {
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

  // Casting.
  static HeapFrame* cast(Object* object);

  // Sizing.
  static word numAttributes(word extra_words);

  // Layout.
  static const int kMaxStackSizeOffset = HeapObject::kSize;
  static const int kFrameOffset = kMaxStackSizeOffset + kPointerSize;

  // Number of words that aren't the Frame.
  static const int kNumOverheadWords = kFrameOffset / kPointerSize;
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

  Object* get(word n) const {
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
  KwArguments(Frame* frame, word nargs) : Arguments(frame, nargs) {
    kwnames_ = ObjectArray::cast(frame->getLocal(nargs - 1));
    num_keywords_ = kwnames_->length();
    num_args_ = nargs - num_keywords_ - 1;
  }

  Object* getKw(Object* name) const {
    for (word i = 0; i < num_keywords_; i++) {
      if (Str::cast(name)->equals(kwnames_->at(i))) {
        return frame_->getLocal(num_args_ + i);
      }
    }
    return Error::object();
  }

  word numKeywords() const { return num_keywords_; }

 private:
  word num_keywords_;
  ObjectArray* kwnames_;
};

inline uword Frame::address() { return reinterpret_cast<uword>(this); }

inline Object* Frame::at(int offset) {
  return *reinterpret_cast<Object**>(address() + offset);
}

inline void Frame::atPut(int offset, Object* value) {
  *reinterpret_cast<Object**>(address() + offset) = value;
}

inline BlockStack* Frame::blockStack() {
  return reinterpret_cast<BlockStack*>(address() + kBlockStackOffset);
}

inline word Frame::virtualPC() {
  return SmallInt::cast(at(kVirtualPCOffset))->value();
}

inline void Frame::setVirtualPC(word pc) {
  atPut(kVirtualPCOffset, SmallInt::fromWord(pc));
}

inline Object* Frame::builtins() { return at(kBuiltinsOffset); }

inline void Frame::setBuiltins(Object* builtins) {
  atPut(kBuiltinsOffset, builtins);
}

inline Object* Frame::globals() { return at(kGlobalsOffset); }

inline void Frame::setGlobals(Object* globals) {
  atPut(kGlobalsOffset, globals);
}

inline Object* Frame::implicitGlobals() { return at(kImplicitGlobalsOffset); }

inline void Frame::setImplicitGlobals(Object* implicit_globals) {
  atPut(kImplicitGlobalsOffset, implicit_globals);
}

inline Object* Frame::fastGlobals() { return at(kFastGlobalsOffset); }

inline void Frame::setFastGlobals(Object* fast_globals) {
  atPut(kFastGlobalsOffset, fast_globals);
}

inline Object* Frame::code() { return at(kCodeOffset); }

inline void Frame::setCode(Object* code) { atPut(kCodeOffset, code); }

inline Object** Frame::locals() {
  return reinterpret_cast<Object**>(at(kLocalsOffset));
}

inline Object* Frame::getLocal(word idx) {
  DCHECK_INDEX(idx, numLocals());
  return *(locals() - idx);
}

inline void Frame::setLocal(word idx, Object* object) {
  DCHECK_INDEX(idx, numLocals());
  *(locals() - idx) = object;
}

inline void Frame::setNumLocals(word num_locals) {
  atPut(kNumLocalsOffset, SmallInt::fromWord(num_locals));
  resetLocals(num_locals);
}

inline void Frame::resetLocals(word num_locals) {
  // Bias locals by 1 word to avoid doing so during {get,set}Local
  Object* locals = reinterpret_cast<Object*>(address() + Frame::kSize +
                                             ((num_locals - 1) * kPointerSize));
  DCHECK(locals->isSmallInt(), "expected small integer");
  atPut(kLocalsOffset, locals);
}

inline word Frame::numLocals() {
  return SmallInt::cast(at(kNumLocalsOffset))->value();
}

inline Frame* Frame::previousFrame() {
  Object* frame = at(kPreviousFrameOffset);
  return reinterpret_cast<Frame*>(SmallInt::cast(frame)->value());
}

inline void Frame::setPreviousFrame(Frame* frame) {
  atPut(kPreviousFrameOffset,
        SmallInt::fromWord(reinterpret_cast<uword>(frame)));
}

inline Object** Frame::valueStackBase() {
  return reinterpret_cast<Object**>(this);
}

inline Object** Frame::valueStackTop() {
  Object* top = at(kValueStackTopOffset);
  return reinterpret_cast<Object**>(SmallInt::cast(top)->value());
}

inline void Frame::setValueStackTop(Object** top) {
  atPut(kValueStackTopOffset, SmallInt::fromWord(reinterpret_cast<uword>(top)));
}

inline word Frame::valueStackSize() {
  return valueStackBase() - valueStackTop();
}

inline void Frame::pushValue(Object* value) {
  Object** top = valueStackTop();
  *--top = value;
  setValueStackTop(top);
}

inline void Frame::insertValueAt(Object* value, word offset) {
  DCHECK(valueStackTop() + offset <= valueStackBase(), "offset %ld overflows",
         offset);
  Object** sp = valueStackTop() - 1;
  for (word i = 0; i < offset; i++) {
    sp[i] = sp[i + 1];
  }
  sp[offset] = value;
  setValueStackTop(sp);
}

inline void Frame::setValueAt(Object* value, word offset) {
  DCHECK(valueStackTop() + offset < valueStackBase(), "offset %ld overflows",
         offset);
  *(valueStackTop() + offset) = value;
}

inline Object* Frame::popValue() {
  DCHECK(valueStackTop() + 1 <= valueStackBase(), "offset %d overflows", 1);
  Object* result = *valueStackTop();
  setValueStackTop(valueStackTop() + 1);
  return result;
}

inline void Frame::dropValues(word count) {
  DCHECK(valueStackTop() + count <= valueStackBase(), "count %ld overflows",
         count);
  setValueStackTop(valueStackTop() + count);
}

inline Object* Frame::topValue() { return peek(0); }

inline void Frame::setTopValue(Object* value) { *valueStackTop() = value; }

inline void Frame::pushLocals(word count, word offset) {
  DCHECK(offset + count <= numLocals(), "locals overflow");
  for (word i = offset; i < offset + count; i++) {
    pushValue(getLocal(i));
  }
}

inline Object* Frame::peek(word offset) {
  DCHECK(valueStackTop() + offset < valueStackBase(), "offset %ld overflows",
         offset);
  return *(valueStackTop() + offset);
}

inline Function* Frame::function(word argc) {
  return Function::cast(peek(argc));
}

inline bool Frame::isSentinelFrame() { return previousFrame() == nullptr; }

inline void* Frame::nativeFunctionPointer() {
  DCHECK(isNativeFrame(), "not native frame");
  return Int::cast(code())->asCPtr();
}

inline bool Frame::isNativeFrame() { return code()->isInt(); }

inline void Frame::makeNativeFrame(Object* fn_pointer_as_int) {
  DCHECK(fn_pointer_as_int->isInt(), "expected integer");
  setCode(fn_pointer_as_int);
}

inline void Frame::stashInternalPointers(Frame* old_frame) {
  // Replace ValueStackTop with the stack depth while this Frame is on the heap,
  // to survive being moved by the GC.
  setValueStackTop(reinterpret_cast<Object**>(old_frame->valueStackSize()));
}

inline void Frame::unstashInternalPointers() {
  auto stashed_size = reinterpret_cast<word>(valueStackTop());
  setValueStackTop(valueStackBase() - stashed_size);
  resetLocals(numLocals());
}

// HeapFrame

inline Frame* HeapFrame::frame() {
  return reinterpret_cast<Frame*>(address() + kFrameOffset +
                                  maxStackSize() * kPointerSize);
}

inline word HeapFrame::numFrameWords() {
  return header()->count() - kNumOverheadWords;
}

inline word HeapFrame::maxStackSize() {
  return SmallInt::cast(instanceVariableAt(kMaxStackSizeOffset))->value();
}

inline void HeapFrame::setMaxStackSize(word offset) {
  instanceVariableAtPut(kMaxStackSizeOffset, SmallInt::fromWord(offset));
}

inline Frame* HeapFrame::copyToNewStackFrame(Frame* caller_frame) {
  auto src_base = reinterpret_cast<Object**>(address() + kFrameOffset);
  word frame_words = numFrameWords();
  Object** dest_base = caller_frame->valueStackTop() - frame_words;
  std::memcpy(dest_base, src_base, frame_words * kPointerSize);

  auto live_frame = reinterpret_cast<Frame*>(dest_base + maxStackSize());
  live_frame->unstashInternalPointers();
  return live_frame;
}

inline void HeapFrame::copyFromStackFrame(Frame* live_frame) {
  auto dest_base = reinterpret_cast<Object**>(address() + kFrameOffset);
  Object** src_base = live_frame->valueStackBase() - maxStackSize();
  std::memcpy(dest_base, src_base, numFrameWords() * kPointerSize);
  frame()->stashInternalPointers(live_frame);
}

inline HeapFrame* HeapFrame::cast(Object* object) {
  DCHECK(object->isHeapFrame(), "invalid cast, expected HeapFrame");
  return reinterpret_cast<HeapFrame*>(object);
}

inline word HeapFrame::numAttributes(word extra_words) {
  return kNumOverheadWords + Frame::kSize / kPointerSize + extra_words;
}

inline Object* TryBlock::asSmallInt() const {
  auto obj = reinterpret_cast<Object*>(value_);
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

inline Object* BlockStack::at(int offset) {
  return *reinterpret_cast<Object**>(address() + offset);
}

inline void BlockStack::atPut(int offset, Object* value) {
  *reinterpret_cast<Object**>(address() + offset) = value;
}

inline word BlockStack::top() {
  return SmallInt::cast(at(kTopOffset))->value();
}

inline void BlockStack::setTop(word new_top) {
  DCHECK_INDEX(new_top, kMaxBlockStackDepth);
  atPut(kTopOffset, SmallInt::fromWord(new_top));
}

inline TryBlock BlockStack::peek() {
  word stack_top = top() - 1;
  DCHECK(stack_top > -1, "block stack underflow %ld", stack_top);
  Object* block = at(kStackOffset + stack_top * kPointerSize);
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
  Object* block = at(kStackOffset + stack_top * kPointerSize);
  setTop(stack_top);
  return TryBlock(block);
}

}  // namespace python
