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
  explicit TryBlock(Object* value) {
    DCHECK(value->isSmallInteger(), "expected small integer");
    value_ = reinterpret_cast<uword>(value);
  }

  TryBlock(word kind, word handler, word level) : value_(0) {
    setKind(kind);
    setHandler(handler);
    setLevel(level);
  }

  inline Object* asSmallInteger() const;

  inline word kind() const;
  inline void setKind(word kind);

  inline word handler() const;
  inline void setHandler(word handler);

  inline word level() const;
  inline void setLevel(word level);

 private:
  inline word valueBits(uword offset, uword mask) const;
  inline void setValueBits(uword offset, uword mask, word value);

  uword value_;

  static const int kKindOffset = SmallInteger::kTagSize;
  static const int kKindSize = 8;
  static const uword kKindMask = (1 << kKindSize) - 1;

  static const int kHandlerOffset = kKindOffset + kKindSize; // 9
  static const int kHandlerSize = 30;
  static const uword kHandlerMask = (1 << kHandlerSize) - 1;

  static const int kLevelOffset = kHandlerOffset + kHandlerSize; // 39
  static const int kLevelSize = 25;
  static const uword kLevelMask = (1 << kLevelSize) - 1;

  static const int kSize = kLevelOffset + kLevelSize;

  static_assert(
      kSize <= kBitsPerByte * sizeof(uword),
      "TryBlock must fit into a uword");
};

// TODO: Determine maximum block stack depth when the code object is loaded and
//       dynamically allocate the minimum amount of space for the block stack.
const int kMaxBlockStackDepth = 20;

class BlockStack {
 public:
  inline void push(const TryBlock& block);

  inline TryBlock pop();

  static const int kStackOffset = 0;
  static const int kTopOffset =
      kStackOffset + kMaxBlockStackDepth * kPointerSize;
  static const int kSize = kTopOffset + kPointerSize;

 private:
  inline uword address();
  inline Object* at(int offset);
  inline void atPut(int offset, Object* value);

  inline word top();
  inline void setTop(word newTop);

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
  inline Object* getLocal(word idx);
  inline void setLocal(word idx, Object* local);

  inline void setNumLocals(word numLocals);
  inline word numLocals();

  inline BlockStack* blockStack();

  // Index in the bytecode array of the last instruction that was executed
  inline word virtualPC();
  inline void setVirtualPC(word pc);

  // The builtins namespace (a Dictionary)
  inline Object* builtins();
  inline void setBuiltins(Object* builtins);

  // The (explicit) globals namespace (a Dictionary)
  inline Object* globals();
  inline void setGlobals(Object* globals);

  // The implicit globals namespace (a Dictionary)
  inline Object* implicitGlobals();
  inline void setImplicitGlobals(Object* implicit_globals);

  // The pre-computed object array provided fast globals access.
  // fastGlobals[arg] == globals[names[arg]]
  inline Object* fastGlobals();
  inline void setFastGlobals(Object* fast_globals);

  // The code object
  inline Object* code();
  inline void setCode(Object* code);

  // A pointer to the previous frame or nullptr if this is the first frame
  inline Frame* previousFrame();
  inline void setPreviousFrame(Frame* frame);

  // A pointer to the top of the value stack
  inline Object** valueStackTop();
  inline void setValueStackTop(Object** top);

  inline Object** valueStackBase();

  // Return the object at offset from the top of the value stack (e.g. peek(0)
  // returns the top of the stack)
  inline Object* peek(word offset);

  // A function lives immediately below the arguments on the value stack
  inline Function* function(word argc);

  inline bool isSentinelFrame();
  void makeSentinel();

  inline bool isNativeFrame();
  inline void* nativeFunctionPointer();
  inline void makeNativeFrame(Object* fnPointerAsInt);

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
  inline uword address();
  inline Object* at(int offset);
  inline void atPut(int offset, Object* value);
  inline Object** locals();

  DISALLOW_COPY_AND_ASSIGN(Frame);
};

class FrameVisitor {
 public:
  virtual void visit(Frame* frame) = 0;
  virtual ~FrameVisitor() = default;
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

BlockStack* Frame::blockStack() {
  return reinterpret_cast<BlockStack*>(address() + kBlockStackOffset);
}

word Frame::virtualPC() {
  return SmallInteger::cast(at(kVirtualPCOffset))->value();
}

void Frame::setVirtualPC(word pc) {
  atPut(kVirtualPCOffset, SmallInteger::fromWord(pc));
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

Object* Frame::implicitGlobals() {
  return at(kImplicitGlobalsOffset);
}

void Frame::setImplicitGlobals(Object* implicit_globals) {
  atPut(kImplicitGlobalsOffset, implicit_globals);
}

Object* Frame::fastGlobals() {
  return at(kFastGlobalsOffset);
}

void Frame::setFastGlobals(Object* fast_globals) {
  atPut(kFastGlobalsOffset, fast_globals);
}

Object* Frame::code() {
  return at(kCodeOffset);
}

void Frame::setCode(Object* code) {
  atPut(kCodeOffset, code);
}

Object** Frame::locals() {
  return reinterpret_cast<Object**>(at(kLocalsOffset));
}

Object* Frame::getLocal(word idx) {
  DCHECK_INDEX(idx, numLocals());
  return *(locals() - idx);
}

void Frame::setLocal(word idx, Object* object) {
  DCHECK_INDEX(idx, numLocals());
  *(locals() - idx) = object;
}

void Frame::setNumLocals(word numLocals) {
  atPut(kNumLocalsOffset, SmallInteger::fromWord(numLocals));
  // Bias locals by 1 word to avoid doing so during {get,set}Local
  Object* locals = reinterpret_cast<Object*>(
      address() + Frame::kSize + ((numLocals - 1) * kPointerSize));
  DCHECK(locals->isSmallInteger(), "expected small integer");
  atPut(kLocalsOffset, locals);
}

word Frame::numLocals() {
  return SmallInteger::cast(at(kNumLocalsOffset))->value();
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

Object** Frame::valueStackBase() {
  return reinterpret_cast<Object**>(this);
}

Object** Frame::valueStackTop() {
  Object* top = at(kValueStackTopOffset);
  return reinterpret_cast<Object**>(SmallInteger::cast(top)->value());
}

void Frame::setValueStackTop(Object** top) {
  atPut(
      kValueStackTopOffset,
      SmallInteger::fromWord(reinterpret_cast<uword>(top)));
}

Object* Frame::peek(word offset) {
  DCHECK(
      valueStackTop() + offset < valueStackBase(),
      "offset %ld overflows",
      offset);
  return *(valueStackTop() + offset);
}

Function* Frame::function(word argc) {
  return Function::cast(peek(argc));
}

bool Frame::isSentinelFrame() {
  return previousFrame() == nullptr;
}

void* Frame::nativeFunctionPointer() {
  DCHECK(isNativeFrame(), "not native frame");
  return Integer::cast(code())->asCPointer();
}

bool Frame::isNativeFrame() {
  return code()->isInteger();
}

void Frame::makeNativeFrame(Object* fnPointerAsInt) {
  DCHECK(fnPointerAsInt->isInteger(), "expected integer");
  setCode(fnPointerAsInt);
}

Object* TryBlock::asSmallInteger() const {
  auto obj = reinterpret_cast<Object*>(value_);
  DCHECK(obj->isSmallInteger(), "expected small integer");
  return obj;
}

word TryBlock::kind() const {
  return valueBits(kKindOffset, kKindMask);
}

void TryBlock::setKind(word kind) {
  setValueBits(kKindOffset, kKindMask, kind);
}

word TryBlock::handler() const {
  return valueBits(kHandlerOffset, kHandlerMask);
}

void TryBlock::setHandler(word handler) {
  setValueBits(kHandlerOffset, kHandlerMask, handler);
}

word TryBlock::level() const {
  return valueBits(kLevelOffset, kLevelMask);
}

void TryBlock::setLevel(word level) {
  setValueBits(kLevelOffset, kLevelMask, level);
}

word TryBlock::valueBits(uword offset, uword mask) const {
  return (value_ >> offset) & mask;
}

void TryBlock::setValueBits(uword offset, uword mask, word value) {
  value_ |= (value & mask) << offset;
}

uword BlockStack::address() {
  return reinterpret_cast<uword>(this);
}

Object* BlockStack::at(int offset) {
  return *reinterpret_cast<Object**>(address() + offset);
}

void BlockStack::atPut(int offset, Object* value) {
  *reinterpret_cast<Object**>(address() + offset) = value;
}

word BlockStack::top() {
  return SmallInteger::cast(at(kTopOffset))->value();
}

void BlockStack::setTop(word newTop) {
  DCHECK_INDEX(newTop, kMaxBlockStackDepth);
  atPut(kTopOffset, SmallInteger::fromWord(newTop));
}

void BlockStack::push(const TryBlock& block) {
  word stackTop = top();
  atPut(kStackOffset + stackTop * kPointerSize, block.asSmallInteger());
  setTop(stackTop + 1);
}

TryBlock BlockStack::pop() {
  word stackTop = top() - 1;
  DCHECK(stackTop > -1, "block stack underflow %ld", stackTop);
  Object* block = at(kStackOffset + stackTop * kPointerSize);
  setTop(stackTop);
  return TryBlock(block);
}

} // namespace python
