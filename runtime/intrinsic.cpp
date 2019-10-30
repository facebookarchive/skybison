#include "intrinsic.h"

#include "runtime.h"

namespace py {

static bool underBoolCheck(Frame* frame) {
  frame->setTopValue(Bool::fromBool(frame->popValue().isBool()));
  return true;
}

static bool underByteArrayCheck(Thread* thread, Frame* frame) {
  frame->setTopValue(Bool::fromBool(
      thread->runtime()->isInstanceOfByteArray(frame->popValue())));
  return true;
}

static bool underByteArrayGuard(Thread* thread, Frame* frame) {
  if (thread->runtime()->isInstanceOfByteArray(frame->topValue())) {
    frame->popValue();
    frame->setTopValue(NoneType::object());
    return true;
  }
  return false;
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

static bool underBytesGuard(Thread* thread, Frame* frame) {
  if (thread->runtime()->isInstanceOfBytes(frame->topValue())) {
    frame->popValue();
    frame->setTopValue(NoneType::object());
    return true;
  }
  return false;
}

static bool underBytesLen(Frame* frame) {
  RawObject arg = frame->peek(0);
  if (arg.isBytes()) {
    frame->popValue();
    frame->setTopValue(SmallInt::fromWord(Bytes::cast(arg).length()));
    return true;
  }
  return false;
}

static bool underByteslikeCheck(Thread* thread, Frame* frame) {
  frame->setTopValue(
      Bool::fromBool(thread->runtime()->isByteslike(frame->popValue())));
  return true;
}

static bool underByteslikeGuard(Thread* thread, Frame* frame) {
  if (thread->runtime()->isByteslike(frame->topValue())) {
    frame->popValue();
    frame->setTopValue(NoneType::object());
    return true;
  }
  return false;
}

static bool underComplexCheck(Thread* thread, Frame* frame) {
  frame->setTopValue(Bool::fromBool(
      thread->runtime()->isInstanceOfComplex(frame->popValue())));
  return true;
}

static bool underDictCheck(Thread* thread, Frame* frame) {
  frame->setTopValue(
      Bool::fromBool(thread->runtime()->isInstanceOfDict(frame->popValue())));
  return true;
}

static bool underDictCheckExact(Frame* frame) {
  frame->setTopValue(Bool::fromBool(frame->popValue().isDict()));
  return true;
}

static bool underDictGuard(Thread* thread, Frame* frame) {
  if (thread->runtime()->isInstanceOfDict(frame->topValue())) {
    frame->popValue();
    frame->setTopValue(NoneType::object());
    return true;
  }
  return false;
}

static bool underDictLen(Frame* frame) {
  RawObject arg = frame->peek(0);
  if (arg.isDict()) {
    frame->popValue();
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

static bool underFloatGuard(Thread* thread, Frame* frame) {
  if (thread->runtime()->isInstanceOfFloat(frame->topValue())) {
    frame->popValue();
    frame->setTopValue(NoneType::object());
    return true;
  }
  return false;
}

static bool underFrozenSetCheck(Thread* thread, Frame* frame) {
  frame->setTopValue(Bool::fromBool(
      thread->runtime()->isInstanceOfFrozenSet(frame->popValue())));
  return true;
}

static bool underFrozenSetGuard(Thread* thread, Frame* frame) {
  if (thread->runtime()->isInstanceOfFrozenSet(frame->topValue())) {
    frame->popValue();
    frame->setTopValue(NoneType::object());
    return true;
  }
  return false;
}

static bool underFunctionGuard(Thread*, Frame* frame) {
  if (frame->topValue().isFunction()) {
    frame->popValue();
    frame->setTopValue(NoneType::object());
    return true;
  }
  return false;
}

static bool underIndex(Thread* thread, Frame* frame) {
  RawObject value = frame->topValue();
  if (thread->runtime()->isInstanceOfInt(value)) {
    frame->popValue();
    frame->setTopValue(value);
    return true;
  }
  return false;
}

static bool underIntCheck(Thread* thread, Frame* frame) {
  frame->setTopValue(
      Bool::fromBool(thread->runtime()->isInstanceOfInt(frame->popValue())));
  return true;
}

static bool underIntCheckExact(Frame* frame) {
  RawObject arg = frame->popValue();
  frame->setTopValue(Bool::fromBool(arg.isSmallInt() || arg.isLargeInt()));
  return true;
}

static bool underIntGuard(Thread* thread, Frame* frame) {
  if (thread->runtime()->isInstanceOfInt(frame->topValue())) {
    frame->popValue();
    frame->setTopValue(NoneType::object());
    return true;
  }
  return false;
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

static bool underListGuard(Thread* thread, Frame* frame) {
  if (thread->runtime()->isInstanceOfList(frame->topValue())) {
    frame->popValue();
    frame->setTopValue(NoneType::object());
    return true;
  }
  return false;
}

static bool underListLen(Frame* frame) {
  RawObject arg = frame->peek(0);
  if (arg.isList()) {
    frame->popValue();
    frame->setTopValue(SmallInt::fromWord(List::cast(arg).numItems()));
    return true;
  }
  return false;
}

static bool underMemoryviewGuard(Frame* frame) {
  if (frame->topValue().isMemoryView()) {
    frame->popValue();
    frame->setTopValue(NoneType::object());
    return true;
  }
  return false;
}

static bool underNumberCheck(Thread* thread, Frame* frame) {
  Runtime* runtime = thread->runtime();
  RawObject arg = frame->topValue();
  if (runtime->isInstanceOfInt(arg) || runtime->isInstanceOfFloat(arg)) {
    frame->popValue();
    frame->setTopValue(Bool::trueObj());
    return true;
  }
  return false;
}

static bool underRangeCheck(Frame* frame) {
  frame->setTopValue(Bool::fromBool(frame->popValue().isRange()));
  return true;
}

static bool underRangeGuard(Frame* frame) {
  if (frame->topValue().isRange()) {
    frame->popValue();
    frame->setTopValue(NoneType::object());
    return true;
  }
  return false;
}

static bool underSeqIndex(Frame* frame) {
  frame->setTopValue(
      SmallInt::fromWord(SeqIterator::cast(frame->popValue()).index()));
  return true;
}

static bool underSeqIterable(Frame* frame) {
  frame->setTopValue(SeqIterator::cast(frame->popValue()).iterable());
  return true;
}

static bool underSeqSetIndex(Frame* frame) {
  RawObject seq_iter = frame->popValue();
  RawObject index = frame->popValue();
  SeqIterator::cast(seq_iter).setIndex(Int::cast(index).asWord());
  return true;
}

static bool underSeqSetIterable(Frame* frame) {
  RawObject seq_iter = frame->popValue();
  RawObject iterable = frame->popValue();
  SeqIterator::cast(seq_iter).setIterable(iterable);
  return true;
}

static bool underSetCheck(Thread* thread, Frame* frame) {
  frame->setTopValue(
      Bool::fromBool(thread->runtime()->isInstanceOfSet(frame->popValue())));
  return true;
}

static bool underSetGuard(Thread* thread, Frame* frame) {
  if (thread->runtime()->isInstanceOfSet(frame->topValue())) {
    frame->popValue();
    frame->setTopValue(NoneType::object());
    return true;
  }
  return false;
}

static bool underSetLen(Frame* frame) {
  RawObject arg = frame->peek(0);
  if (arg.isSet()) {
    frame->popValue();
    frame->setTopValue(SmallInt::fromWord(Set::cast(arg).numItems()));
    return true;
  }
  return false;
}

static bool underSliceCheck(Frame* frame) {
  frame->setTopValue(Bool::fromBool(frame->popValue().isSlice()));
  return true;
}

static bool underSliceGuard(Frame* frame) {
  if (frame->topValue().isSlice()) {
    frame->popValue();
    frame->setTopValue(NoneType::object());
    return true;
  }
  return false;
}

static bool underSliceIndex(Thread* thread, Frame* frame) {
  RawObject value = frame->peek(0);
  if (value.isNoneType() || thread->runtime()->isInstanceOfInt(value)) {
    frame->popValue();
    frame->setTopValue(value);
    return true;
  }
  return false;
}

static bool underSliceIndexNotNone(Thread* thread, Frame* frame) {
  RawObject value = frame->topValue();
  if (thread->runtime()->isInstanceOfInt(value)) {
    frame->popValue();
    frame->setTopValue(value);
    return true;
  }
  return false;
}

static bool underStrCheck(Thread* thread, Frame* frame) {
  frame->setTopValue(
      Bool::fromBool(thread->runtime()->isInstanceOfStr(frame->popValue())));
  return true;
}

static bool underStrCheckExact(Frame* frame) {
  frame->setTopValue(Bool::fromBool(frame->popValue().isStr()));
  return true;
}

static bool underStrGuard(Thread* thread, Frame* frame) {
  if (thread->runtime()->isInstanceOfStr(frame->topValue())) {
    frame->popValue();
    frame->setTopValue(NoneType::object());
    return true;
  }
  return false;
}

static bool underStrLen(Frame* frame) {
  RawObject arg = frame->peek(0);
  if (arg.isStr()) {
    frame->popValue();
    frame->setTopValue(SmallInt::fromWord(Str::cast(arg).codePointLength()));
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
  frame->setTopValue(Bool::fromBool(frame->popValue().isTuple()));
  return true;
}

static bool underTupleGetItem(Frame* frame) {
  RawObject arg0 = frame->peek(1);
  if (!arg0.isTuple()) {
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
  RawTuple self = Tuple::cast(arg0);
  if (0 <= idx && idx < self.length()) {
    frame->dropValues(2);
    frame->setTopValue(self.at(idx));
    return true;
  }
  return false;
}

static bool underTupleGuard(Thread* thread, Frame* frame) {
  if (thread->runtime()->isInstanceOfTuple(frame->topValue())) {
    frame->popValue();
    frame->setTopValue(NoneType::object());
    return true;
  }
  return false;
}

static bool underTupleLen(Frame* frame) {
  RawObject arg = frame->peek(0);
  if (arg.isBytes()) {
    frame->popValue();
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

static bool underTypeGuard(Thread* thread, Frame* frame) {
  if (thread->runtime()->isInstanceOfType(frame->topValue())) {
    frame->popValue();
    frame->setTopValue(NoneType::object());
    return true;
  }
  return false;
}

static bool isInstance(Thread* thread, Frame* frame) {
  if (thread->runtime()->typeOf(frame->peek(1)) == frame->peek(0)) {
    frame->dropValues(2);
    frame->setTopValue(Bool::trueObj());
    return true;
  }
  return false;
}

static bool len(Frame* frame) {
  RawObject arg = frame->topValue();
  switch (arg.layoutId()) {
    case LayoutId::kByteArray:
      frame->popValue();
      frame->setTopValue(SmallInt::fromWord(ByteArray::cast(arg).numItems()));
      return true;
    case LayoutId::kBytes:
      frame->popValue();
      frame->setTopValue(SmallInt::fromWord(Bytes::cast(arg).length()));
      return true;
    case LayoutId::kDict:
      frame->popValue();
      frame->setTopValue(SmallInt::fromWord(Dict::cast(arg).numItems()));
      return true;
    case LayoutId::kFrozenSet:
      frame->popValue();
      frame->setTopValue(SmallInt::fromWord(FrozenSet::cast(arg).numItems()));
      return true;
    case LayoutId::kList:
      frame->popValue();
      frame->setTopValue(SmallInt::fromWord(List::cast(arg).numItems()));
      return true;
    case LayoutId::kSet:
      frame->popValue();
      frame->setTopValue(SmallInt::fromWord(Set::cast(arg).numItems()));
      return true;
    case LayoutId::kStr:
      frame->popValue();
      frame->setTopValue(SmallInt::fromWord(Str::cast(arg).codePointLength()));
      return true;
    case LayoutId::kTuple:
      frame->popValue();
      frame->setTopValue(SmallInt::fromWord(Tuple::cast(arg).length()));
      return true;
    default:
      return false;
  }
}

bool doIntrinsic(Thread* thread, Frame* frame, SymbolId name) {
  switch (name) {
    case SymbolId::kUnderBoolCheck:
      return underBoolCheck(frame);
    case SymbolId::kUnderByteArrayCheck:
      return underByteArrayCheck(thread, frame);
    case SymbolId::kUnderByteArrayGuard:
      return underByteArrayGuard(thread, frame);
    case SymbolId::kUnderByteArrayLen:
      return underByteArrayLen(frame);
    case SymbolId::kUnderBytesCheck:
      return underBytesCheck(thread, frame);
    case SymbolId::kUnderBytesGuard:
      return underBytesGuard(thread, frame);
    case SymbolId::kUnderBytesLen:
      return underBytesLen(frame);
    case SymbolId::kUnderByteslikeCheck:
      return underByteslikeCheck(thread, frame);
    case SymbolId::kUnderByteslikeGuard:
      return underByteslikeGuard(thread, frame);
    case SymbolId::kUnderComplexCheck:
      return underComplexCheck(thread, frame);
    case SymbolId::kUnderDictCheck:
      return underDictCheck(thread, frame);
    case SymbolId::kUnderDictCheckExact:
      return underDictCheckExact(frame);
    case SymbolId::kUnderDictGuard:
      return underDictGuard(thread, frame);
    case SymbolId::kUnderDictLen:
      return underDictLen(frame);
    case SymbolId::kUnderFloatCheck:
      return underFloatCheck(thread, frame);
    case SymbolId::kUnderFloatGuard:
      return underFloatGuard(thread, frame);
    case SymbolId::kUnderFrozenSetCheck:
      return underFrozenSetCheck(thread, frame);
    case SymbolId::kUnderFrozenSetGuard:
      return underFrozenSetGuard(thread, frame);
    case SymbolId::kUnderFunctionGuard:
      return underFunctionGuard(thread, frame);
    case SymbolId::kUnderIndex:
      return underIndex(thread, frame);
    case SymbolId::kUnderIntCheck:
      return underIntCheck(thread, frame);
    case SymbolId::kUnderIntCheckExact:
      return underIntCheckExact(frame);
    case SymbolId::kUnderIntGuard:
      return underIntGuard(thread, frame);
    case SymbolId::kUnderListCheck:
      return underListCheck(thread, frame);
    case SymbolId::kUnderListCheckExact:
      return underListCheckExact(frame);
    case SymbolId::kUnderListGetitem:
      return underListGetItem(frame);
    case SymbolId::kUnderListGuard:
      return underListGuard(thread, frame);
    case SymbolId::kUnderListLen:
      return underListLen(frame);
    case SymbolId::kUnderMemoryviewGuard:
      return underMemoryviewGuard(frame);
    case SymbolId::kUnderNumberCheck:
      return underNumberCheck(thread, frame);
    case SymbolId::kUnderRangeCheck:
      return underRangeCheck(frame);
    case SymbolId::kUnderRangeGuard:
      return underRangeGuard(frame);
    case SymbolId::kUnderSeqIndex:
      return underSeqIndex(frame);
    case SymbolId::kUnderSeqIterable:
      return underSeqIterable(frame);
    case SymbolId::kUnderSeqSetIndex:
      return underSeqSetIndex(frame);
    case SymbolId::kUnderSeqSetIterable:
      return underSeqSetIterable(frame);
    case SymbolId::kUnderSetCheck:
      return underSetCheck(thread, frame);
    case SymbolId::kUnderSetGuard:
      return underSetGuard(thread, frame);
    case SymbolId::kUnderSetLen:
      return underSetLen(frame);
    case SymbolId::kUnderSliceCheck:
      return underSliceCheck(frame);
    case SymbolId::kUnderSliceGuard:
      return underSliceGuard(frame);
    case SymbolId::kUnderSliceIndex:
      return underSliceIndex(thread, frame);
    case SymbolId::kUnderSliceIndexNotNone:
      return underSliceIndexNotNone(thread, frame);
    case SymbolId::kUnderStrCheck:
      return underStrCheck(thread, frame);
    case SymbolId::kUnderStrCheckExact:
      return underStrCheckExact(frame);
    case SymbolId::kUnderStrGuard:
      return underStrGuard(thread, frame);
    case SymbolId::kUnderStrLen:
      return underStrLen(frame);
    case SymbolId::kUnderTupleCheck:
      return underTupleCheck(thread, frame);
    case SymbolId::kUnderTupleCheckExact:
      return underTupleCheckExact(frame);
    case SymbolId::kUnderTupleGetitem:
      return underTupleGetItem(frame);
    case SymbolId::kUnderTupleGuard:
      return underTupleGuard(thread, frame);
    case SymbolId::kUnderTupleLen:
      return underTupleLen(frame);
    case SymbolId::kUnderType:
      return underType(thread, frame);
    case SymbolId::kUnderTypeCheck:
      return underTypeCheck(thread, frame);
    case SymbolId::kUnderTypeCheckExact:
      return underTypeCheckExact(frame);
    case SymbolId::kUnderTypeGuard:
      return underTypeGuard(thread, frame);
    case SymbolId::kIsInstance:
      return isInstance(thread, frame);
    case SymbolId::kLen:
      return len(frame);
    default:
      UNREACHABLE("function %s does not have an intrinsic implementation",
                  Symbols::predefinedSymbolAt(name));
  }
}

}  // namespace py
