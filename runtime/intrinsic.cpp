#include "intrinsic.h"

#include "runtime.h"

namespace python {

static bool underByteArrayCheck(Thread* thread, Frame* frame) {
  frame->setTopValue(Bool::fromBool(
      thread->runtime()->isInstanceOfByteArray(frame->popValue())));
  return true;
}

static bool underByteArrayLen(Frame* frame) {
  RawObject arg = frame->popValue();
  if (arg.isByteArray()) {
    frame->setTopValue(SmallInt::fromWord(ByteArray::cast(arg).numItems()));
    return true;
  }
  return false;
}

static bool underBytesCheck(Thread* thread, Frame* frame) {
  frame->setTopValue(
      Bool::fromBool(thread->runtime()->isInstanceOfBytes(frame->popValue())));
  return true;
}

static bool underBytesLen(Frame* frame) {
  RawObject arg = frame->popValue();
  if (arg.isBytes()) {
    frame->setTopValue(SmallInt::fromWord(Bytes::cast(arg).length()));
    return true;
  }
  return false;
}

static bool underDictCheck(Thread* thread, Frame* frame) {
  frame->setTopValue(
      Bool::fromBool(thread->runtime()->isInstanceOfDict(frame->popValue())));
  return true;
}

static bool underDictLen(Frame* frame) {
  RawObject arg = frame->popValue();
  if (arg.isDict()) {
    frame->setTopValue(SmallInt::fromWord(Dict::cast(arg).numItems()));
    return true;
  }
  return false;
}

static bool underFloatCheck(Thread* thread, Frame* frame) {
  frame->setTopValue(
      Bool::fromBool(thread->runtime()->isInstanceOfFloat(frame->popValue())));
  return true;
}

static bool underFrozenSetCheck(Thread* thread, Frame* frame) {
  frame->setTopValue(Bool::fromBool(
      thread->runtime()->isInstanceOfFrozenSet(frame->popValue())));
  return true;
}

static bool underIntCheck(Thread* thread, Frame* frame) {
  frame->setTopValue(
      Bool::fromBool(thread->runtime()->isInstanceOfInt(frame->popValue())));
  return true;
}

static bool underListCheck(Thread* thread, Frame* frame) {
  frame->setTopValue(
      Bool::fromBool(thread->runtime()->isInstanceOfList(frame->popValue())));
  return true;
}

static bool underListCheckExact(Frame* frame) {
  frame->setTopValue(Bool::fromBool(frame->popValue().isList()));
  return true;
}

static bool underListGetItem(Frame* frame) {
  RawObject arg0 = frame->peek(1);
  if (!arg0.isList()) {
    return false;
  }
  RawObject arg1 = frame->peek(0);
  word idx;
  if (arg1.isSmallInt()) {
    idx = SmallInt::cast(arg1).value();
  } else if (arg1.isBool()) {
    idx = Bool::cast(arg1).value();
  } else {
    return false;
  }
  RawList self = List::cast(arg0);
  if (0 <= idx && idx < self.numItems()) {
    frame->dropValues(2);
    frame->setTopValue(self.at(idx));
    return true;
  }
  return false;
}

static bool underListLen(Frame* frame) {
  RawObject arg = frame->popValue();
  if (arg.isList()) {
    frame->setTopValue(SmallInt::fromWord(List::cast(arg).numItems()));
    return true;
  }
  return false;
}

static bool underSetCheck(Thread* thread, Frame* frame) {
  frame->setTopValue(
      Bool::fromBool(thread->runtime()->isInstanceOfSet(frame->popValue())));
  return true;
}

static bool underSetLen(Frame* frame) {
  RawObject arg = frame->popValue();
  if (arg.isSet()) {
    frame->setTopValue(SmallInt::fromWord(Set::cast(arg).numItems()));
    return true;
  }
  return false;
}

static bool underSliceCheck(Frame* frame) {
  frame->setTopValue(Bool::fromBool(frame->popValue().isSlice()));
  return true;
}

static bool underStrCheck(Thread* thread, Frame* frame) {
  frame->setTopValue(
      Bool::fromBool(thread->runtime()->isInstanceOfStr(frame->popValue())));
  return true;
}

static bool underStrLen(Frame* frame) {
  RawObject arg = frame->popValue();
  if (arg.isStr()) {
    frame->setTopValue(SmallInt::fromWord(Str::cast(arg).length()));
    return true;
  }
  return false;
}

static bool underTupleCheck(Thread* thread, Frame* frame) {
  frame->setTopValue(
      Bool::fromBool(thread->runtime()->isInstanceOfTuple(frame->popValue())));
  return true;
}

static bool underTupleCheckExact(Frame* frame) {
  frame->setTopValue(Bool::fromBool(frame->popValue().isList()));
  return true;
}

static bool underTupleLen(Frame* frame) {
  RawObject arg = frame->popValue();
  if (arg.isBytes()) {
    frame->setTopValue(SmallInt::fromWord(Tuple::cast(arg).length()));
    return true;
  }
  return false;
}

static bool underType(Thread* thread, Frame* frame) {
  frame->setTopValue(thread->runtime()->typeOf(frame->popValue()));
  return true;
}

static bool underTypeCheck(Thread* thread, Frame* frame) {
  frame->setTopValue(
      Bool::fromBool(thread->runtime()->isInstanceOfType(frame->popValue())));
  return true;
}

static bool underTypeCheckExact(Frame* frame) {
  frame->setTopValue(Bool::fromBool(frame->popValue().isType()));
  return true;
}

static bool isInstance(Thread* thread, Frame* frame) {
  if (thread->runtime()->typeOf(frame->peek(1)) == frame->peek(0)) {
    frame->dropValues(2);
    frame->setTopValue(Bool::trueObj());
    return true;
  }
  return false;
}

bool doIntrinsic(Thread* thread, Frame* frame, SymbolId name) {
  switch (name) {
    case SymbolId::kUnderByteArrayCheck:
      return underByteArrayCheck(thread, frame);
    case SymbolId::kUnderByteArrayLen:
      return underByteArrayLen(frame);
    case SymbolId::kUnderBytesCheck:
      return underBytesCheck(thread, frame);
    case SymbolId::kUnderBytesLen:
      return underBytesLen(frame);
    case SymbolId::kUnderDictCheck:
      return underDictCheck(thread, frame);
    case SymbolId::kUnderDictLen:
      return underDictLen(frame);
    case SymbolId::kUnderFloatCheck:
      return underFloatCheck(thread, frame);
    case SymbolId::kUnderFrozenSetCheck:
      return underFrozenSetCheck(thread, frame);
    case SymbolId::kUnderIntCheck:
      return underIntCheck(thread, frame);
    case SymbolId::kUnderListCheck:
      return underListCheck(thread, frame);
    case SymbolId::kUnderListCheckExact:
      return underListCheckExact(frame);
    case SymbolId::kUnderListGetitem:
      return underListGetItem(frame);
    case SymbolId::kUnderListLen:
      return underListLen(frame);
    case SymbolId::kUnderSetCheck:
      return underSetCheck(thread, frame);
    case SymbolId::kUnderSetLen:
      return underSetLen(frame);
    case SymbolId::kUnderSliceCheck:
      return underSliceCheck(frame);
    case SymbolId::kUnderStrCheck:
      return underStrCheck(thread, frame);
    case SymbolId::kUnderStrLen:
      return underStrLen(frame);
    case SymbolId::kUnderTupleCheck:
      return underTupleCheck(thread, frame);
    case SymbolId::kUnderTupleCheckExact:
      return underTupleCheckExact(frame);
    case SymbolId::kUnderTupleLen:
      return underTupleLen(frame);
    case SymbolId::kUnderType:
      return underType(thread, frame);
    case SymbolId::kUnderTypeCheck:
      return underTypeCheck(thread, frame);
    case SymbolId::kUnderTypeCheckExact:
      return underTypeCheckExact(frame);
    case SymbolId::kIsInstance:
      return isInstance(thread, frame);
    default:
      UNREACHABLE("function %s does not have an intrinsic implementation",
                  Symbols::predefinedSymbolAt(name));
  }
}

}  // namespace python
