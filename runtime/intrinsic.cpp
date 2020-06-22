#include "intrinsic.h"

#include "runtime.h"

namespace py {

static bool underBoolCheck(Frame* frame) {
  frame->setTopValue(Bool::fromBool(frame->popValue().isBool()));
  return true;
}

static bool underBoolGuard(Frame* frame) {
  if (frame->topValue().isBool()) {
    frame->popValue();
    frame->setTopValue(NoneType::object());
    return true;
  }
  return false;
}

static bool underBytearrayCheck(Thread* thread, Frame* frame) {
  frame->setTopValue(Bool::fromBool(
      thread->runtime()->isInstanceOfBytearray(frame->popValue())));
  return true;
}

static bool underBytearrayGuard(Thread* thread, Frame* frame) {
  if (thread->runtime()->isInstanceOfBytearray(frame->topValue())) {
    frame->popValue();
    frame->setTopValue(NoneType::object());
    return true;
  }
  return false;
}

static bool underBytearrayLen(Frame* frame) {
  RawObject arg = frame->popValue();
  if (arg.isBytearray()) {
    frame->setTopValue(SmallInt::fromWord(Bytearray::cast(arg).numItems()));
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

static bool underDequeGuard(Thread* thread, Frame* frame) {
  if (thread->runtime()->isInstanceOfDeque(frame->topValue())) {
    frame->popValue();
    frame->setTopValue(NoneType::object());
    return true;
  }
  return false;
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

static bool underFloatCheckExact(Frame* frame) {
  frame->setTopValue(Bool::fromBool(frame->popValue().isFloat()));
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

static bool underFrozensetCheck(Thread* thread, Frame* frame) {
  frame->setTopValue(Bool::fromBool(
      thread->runtime()->isInstanceOfFrozenSet(frame->popValue())));
  return true;
}

static bool underFrozensetGuard(Thread* thread, Frame* frame) {
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

static bool underListGetitem(Frame* frame) {
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

static bool underListSetitem(Frame* frame) {
  RawObject arg0 = frame->peek(2);
  if (!arg0.isList()) {
    return false;
  }
  RawObject arg1 = frame->peek(1);
  word idx;
  if (arg1.isSmallInt()) {
    idx = SmallInt::cast(arg1).value();
  } else if (arg1.isBool()) {
    idx = Bool::cast(arg1).value();
  } else {
    return false;
  }
  RawList self = List::cast(arg0);
  if (idx < 0 || idx >= self.numItems()) {
    return false;
  }
  self.atPut(idx, frame->peek(0));
  frame->dropValues(3);
  frame->setTopValue(NoneType::object());
  return true;
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

static bool underTupleGetitem(Frame* frame) {
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
  if (arg.isTuple()) {
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

static bool underTypeSubclassGuard(Frame* frame) {
  RawObject subclass = frame->peek(0);
  RawObject superclass = frame->peek(1);
  if (subclass == superclass && subclass.isType()) {
    frame->dropValues(2);
    frame->setTopValue(NoneType::object());
    return true;
  }
  return false;
}

static bool underWeakrefCheck(Thread* thread, Frame* frame) {
  frame->setTopValue(Bool::fromBool(
      thread->runtime()->isInstanceOfWeakRef(frame->popValue())));
  return true;
}

static bool underWeakrefGuard(Thread* thread, Frame* frame) {
  if (thread->runtime()->isInstanceOfWeakRef(frame->topValue())) {
    frame->popValue();
    frame->setTopValue(NoneType::object());
    return true;
  }
  return false;
}

static bool isinstance(Thread* thread, Frame* frame) {
  if (thread->runtime()->typeOf(frame->peek(1)) == frame->peek(0)) {
    frame->dropValues(2);
    frame->setTopValue(Bool::trueObj());
    return true;
  }
  return false;
}

static bool len(Frame* frame) {
  RawObject arg = frame->topValue();
  word length;
  switch (arg.layoutId()) {
    case LayoutId::kBytearray:
      length = Bytearray::cast(arg).numItems();
      break;
    case LayoutId::kDict:
      length = Dict::cast(arg).numItems();
      break;
    case LayoutId::kFrozenSet:
      length = FrozenSet::cast(arg).numItems();
      break;
    case LayoutId::kLargeBytes:
      length = LargeBytes::cast(arg).length();
      break;
    case LayoutId::kLargeStr:
      length = LargeStr::cast(arg).codePointLength();
      break;
    case LayoutId::kList:
      length = List::cast(arg).numItems();
      break;
    case LayoutId::kSet:
      length = Set::cast(arg).numItems();
      break;
    case LayoutId::kSmallBytes:
      length = SmallBytes::cast(arg).length();
      break;
    case LayoutId::kSmallStr:
      length = SmallStr::cast(arg).codePointLength();
      break;
    case LayoutId::kTuple:
      length = Tuple::cast(arg).length();
      break;
    default:
      return false;
  }
  frame->popValue();
  frame->setTopValue(SmallInt::fromWord(length));
  return true;
}

bool doIntrinsic(Thread* thread, Frame* frame, SymbolId name) {
  switch (name) {
    case ID(_bool_check):
      return underBoolCheck(frame);
    case ID(_bool_guard):
      return underBoolGuard(frame);
    case ID(_bytearray_check):
      return underBytearrayCheck(thread, frame);
    case ID(_bytearray_guard):
      return underBytearrayGuard(thread, frame);
    case ID(_bytearray_len):
      return underBytearrayLen(frame);
    case ID(_bytes_check):
      return underBytesCheck(thread, frame);
    case ID(_bytes_guard):
      return underBytesGuard(thread, frame);
    case ID(_bytes_len):
      return underBytesLen(frame);
    case ID(_byteslike_check):
      return underByteslikeCheck(thread, frame);
    case ID(_byteslike_guard):
      return underByteslikeGuard(thread, frame);
    case ID(_complex_check):
      return underComplexCheck(thread, frame);
    case ID(_deque_guard):
      return underDequeGuard(thread, frame);
    case ID(_dict_check):
      return underDictCheck(thread, frame);
    case ID(_dict_check_exact):
      return underDictCheckExact(frame);
    case ID(_dict_guard):
      return underDictGuard(thread, frame);
    case ID(_dict_len):
      return underDictLen(frame);
    case ID(_float_check):
      return underFloatCheck(thread, frame);
    case ID(_float_check_exact):
      return underFloatCheckExact(frame);
    case ID(_float_guard):
      return underFloatGuard(thread, frame);
    case ID(_frozenset_check):
      return underFrozensetCheck(thread, frame);
    case ID(_frozenset_guard):
      return underFrozensetGuard(thread, frame);
    case ID(_function_guard):
      return underFunctionGuard(thread, frame);
    case ID(_index):
      return underIndex(thread, frame);
    case ID(_int_check):
      return underIntCheck(thread, frame);
    case ID(_int_check_exact):
      return underIntCheckExact(frame);
    case ID(_int_guard):
      return underIntGuard(thread, frame);
    case ID(_list_check):
      return underListCheck(thread, frame);
    case ID(_list_check_exact):
      return underListCheckExact(frame);
    case ID(_list_getitem):
      return underListGetitem(frame);
    case ID(_list_guard):
      return underListGuard(thread, frame);
    case ID(_list_len):
      return underListLen(frame);
    case ID(_list_setitem):
      return underListSetitem(frame);
    case ID(_memoryview_guard):
      return underMemoryviewGuard(frame);
    case ID(_number_check):
      return underNumberCheck(thread, frame);
    case ID(_range_check):
      return underRangeCheck(frame);
    case ID(_range_guard):
      return underRangeGuard(frame);
    case ID(_seq_index):
      return underSeqIndex(frame);
    case ID(_seq_iterable):
      return underSeqIterable(frame);
    case ID(_seq_set_index):
      return underSeqSetIndex(frame);
    case ID(_seq_set_iterable):
      return underSeqSetIterable(frame);
    case ID(_set_check):
      return underSetCheck(thread, frame);
    case ID(_set_guard):
      return underSetGuard(thread, frame);
    case ID(_set_len):
      return underSetLen(frame);
    case ID(_slice_check):
      return underSliceCheck(frame);
    case ID(_slice_guard):
      return underSliceGuard(frame);
    case ID(_slice_index):
      return underSliceIndex(thread, frame);
    case ID(_slice_index_not_none):
      return underSliceIndexNotNone(thread, frame);
    case ID(_str_check):
      return underStrCheck(thread, frame);
    case ID(_str_check_exact):
      return underStrCheckExact(frame);
    case ID(_str_guard):
      return underStrGuard(thread, frame);
    case ID(_str_len):
      return underStrLen(frame);
    case ID(_tuple_check):
      return underTupleCheck(thread, frame);
    case ID(_tuple_check_exact):
      return underTupleCheckExact(frame);
    case ID(_tuple_getitem):
      return underTupleGetitem(frame);
    case ID(_tuple_guard):
      return underTupleGuard(thread, frame);
    case ID(_tuple_len):
      return underTupleLen(frame);
    case ID(_type):
      return underType(thread, frame);
    case ID(_type_check):
      return underTypeCheck(thread, frame);
    case ID(_type_check_exact):
      return underTypeCheckExact(frame);
    case ID(_type_guard):
      return underTypeGuard(thread, frame);
    case ID(_type_subclass_guard):
      return underTypeSubclassGuard(frame);
    case ID(_weakref_check):
      return underWeakrefCheck(thread, frame);
    case ID(_weakref_guard):
      return underWeakrefGuard(thread, frame);
    case ID(isinstance):
      return isinstance(thread, frame);
    case ID(len):
      return len(frame);
    default:
      UNREACHABLE("function %s does not have an intrinsic implementation",
                  Symbols::predefinedSymbolAt(name));
  }
}

}  // namespace py
