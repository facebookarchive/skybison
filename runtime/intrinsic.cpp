#include "intrinsic.h"

#include "runtime.h"

namespace py {

static bool underBoolCheck(Thread* thread) {
  thread->stackSetTop(Bool::fromBool(thread->stackPop().isBool()));
  return true;
}

static bool underBoolGuard(Thread* thread) {
  if (thread->stackTop().isBool()) {
    thread->stackPop();
    thread->stackSetTop(NoneType::object());
    return true;
  }
  return false;
}

static bool underBytearrayCheck(Thread* thread) {
  thread->stackSetTop(Bool::fromBool(
      thread->runtime()->isInstanceOfBytearray(thread->stackPop())));
  return true;
}

static bool underBytearrayGuard(Thread* thread) {
  if (thread->runtime()->isInstanceOfBytearray(thread->stackTop())) {
    thread->stackPop();
    thread->stackSetTop(NoneType::object());
    return true;
  }
  return false;
}

static bool underBytearrayLen(Thread* thread) {
  RawObject arg = thread->stackPop();
  if (arg.isBytearray()) {
    thread->stackSetTop(SmallInt::fromWord(Bytearray::cast(arg).numItems()));
    return true;
  }
  return false;
}

static bool underBytesCheck(Thread* thread) {
  thread->stackSetTop(
      Bool::fromBool(thread->runtime()->isInstanceOfBytes(thread->stackPop())));
  return true;
}

static bool underBytesGuard(Thread* thread) {
  if (thread->runtime()->isInstanceOfBytes(thread->stackTop())) {
    thread->stackPop();
    thread->stackSetTop(NoneType::object());
    return true;
  }
  return false;
}

static bool underBytesLen(Thread* thread) {
  RawObject arg = thread->stackPeek(0);
  if (arg.isBytes()) {
    thread->stackPop();
    thread->stackSetTop(SmallInt::fromWord(Bytes::cast(arg).length()));
    return true;
  }
  return false;
}

static bool underByteslikeCheck(Thread* thread) {
  thread->stackSetTop(
      Bool::fromBool(thread->runtime()->isByteslike(thread->stackPop())));
  return true;
}

static bool underByteslikeGuard(Thread* thread) {
  if (thread->runtime()->isByteslike(thread->stackTop())) {
    thread->stackPop();
    thread->stackSetTop(NoneType::object());
    return true;
  }
  return false;
}

static bool underComplexCheck(Thread* thread) {
  thread->stackSetTop(Bool::fromBool(
      thread->runtime()->isInstanceOfComplex(thread->stackPop())));
  return true;
}

static bool underDequeGuard(Thread* thread) {
  if (thread->runtime()->isInstanceOfDeque(thread->stackTop())) {
    thread->stackPop();
    thread->stackSetTop(NoneType::object());
    return true;
  }
  return false;
}

static bool underDictCheck(Thread* thread) {
  thread->stackSetTop(
      Bool::fromBool(thread->runtime()->isInstanceOfDict(thread->stackPop())));
  return true;
}

static bool underDictCheckExact(Thread* thread) {
  thread->stackSetTop(Bool::fromBool(thread->stackPop().isDict()));
  return true;
}

static bool underDictGuard(Thread* thread) {
  if (thread->runtime()->isInstanceOfDict(thread->stackTop())) {
    thread->stackPop();
    thread->stackSetTop(NoneType::object());
    return true;
  }
  return false;
}

static bool underDictLen(Thread* thread) {
  RawObject arg = thread->stackPeek(0);
  if (arg.isDict()) {
    thread->stackPop();
    thread->stackSetTop(SmallInt::fromWord(Dict::cast(arg).numItems()));
    return true;
  }
  return false;
}

static bool underFloatCheck(Thread* thread) {
  thread->stackSetTop(
      Bool::fromBool(thread->runtime()->isInstanceOfFloat(thread->stackPop())));
  return true;
}

static bool underFloatCheckExact(Thread* thread) {
  thread->stackSetTop(Bool::fromBool(thread->stackPop().isFloat()));
  return true;
}

static bool underFloatGuard(Thread* thread) {
  if (thread->runtime()->isInstanceOfFloat(thread->stackTop())) {
    thread->stackPop();
    thread->stackSetTop(NoneType::object());
    return true;
  }
  return false;
}

static bool underFrozensetCheck(Thread* thread) {
  thread->stackSetTop(Bool::fromBool(
      thread->runtime()->isInstanceOfFrozenSet(thread->stackPop())));
  return true;
}

static bool underFrozensetGuard(Thread* thread) {
  if (thread->runtime()->isInstanceOfFrozenSet(thread->stackTop())) {
    thread->stackPop();
    thread->stackSetTop(NoneType::object());
    return true;
  }
  return false;
}

static bool underFunctionGuard(Thread* thread) {
  if (thread->stackTop().isFunction()) {
    thread->stackPop();
    thread->stackSetTop(NoneType::object());
    return true;
  }
  return false;
}

static bool underIndex(Thread* thread) {
  RawObject value = thread->stackTop();
  if (thread->runtime()->isInstanceOfInt(value)) {
    thread->stackPop();
    thread->stackSetTop(value);
    return true;
  }
  return false;
}

static bool underIntCheck(Thread* thread) {
  thread->stackSetTop(
      Bool::fromBool(thread->runtime()->isInstanceOfInt(thread->stackPop())));
  return true;
}

static bool underIntCheckExact(Thread* thread) {
  RawObject arg = thread->stackPop();
  thread->stackSetTop(Bool::fromBool(arg.isSmallInt() || arg.isLargeInt()));
  return true;
}

static bool underIntGuard(Thread* thread) {
  if (thread->runtime()->isInstanceOfInt(thread->stackTop())) {
    thread->stackPop();
    thread->stackSetTop(NoneType::object());
    return true;
  }
  return false;
}

static bool underListCheck(Thread* thread) {
  thread->stackSetTop(
      Bool::fromBool(thread->runtime()->isInstanceOfList(thread->stackPop())));
  return true;
}

static bool underListCheckExact(Thread* thread) {
  thread->stackSetTop(Bool::fromBool(thread->stackPop().isList()));
  return true;
}

static bool underListGetitem(Thread* thread) {
  RawObject arg0 = thread->stackPeek(1);
  if (!arg0.isList()) {
    return false;
  }
  RawObject arg1 = thread->stackPeek(0);
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
    thread->stackDrop(2);
    thread->stackSetTop(self.at(idx));
    return true;
  }
  return false;
}

static bool underListGuard(Thread* thread) {
  if (thread->runtime()->isInstanceOfList(thread->stackTop())) {
    thread->stackPop();
    thread->stackSetTop(NoneType::object());
    return true;
  }
  return false;
}

static bool underListLen(Thread* thread) {
  RawObject arg = thread->stackPeek(0);
  if (arg.isList()) {
    thread->stackPop();
    thread->stackSetTop(SmallInt::fromWord(List::cast(arg).numItems()));
    return true;
  }
  return false;
}

static bool underListSetitem(Thread* thread) {
  RawObject arg0 = thread->stackPeek(2);
  if (!arg0.isList()) {
    return false;
  }
  RawObject arg1 = thread->stackPeek(1);
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
  self.atPut(idx, thread->stackPeek(0));
  thread->stackDrop(3);
  thread->stackSetTop(NoneType::object());
  return true;
}

static bool underMemoryviewGuard(Thread* thread) {
  if (thread->stackTop().isMemoryView()) {
    thread->stackPop();
    thread->stackSetTop(NoneType::object());
    return true;
  }
  return false;
}

static bool underNumberCheck(Thread* thread) {
  Runtime* runtime = thread->runtime();
  RawObject arg = thread->stackTop();
  if (runtime->isInstanceOfInt(arg) || runtime->isInstanceOfFloat(arg)) {
    thread->stackPop();
    thread->stackSetTop(Bool::trueObj());
    return true;
  }
  return false;
}

static bool underRangeCheck(Thread* thread) {
  thread->stackSetTop(Bool::fromBool(thread->stackPop().isRange()));
  return true;
}

static bool underRangeGuard(Thread* thread) {
  if (thread->stackTop().isRange()) {
    thread->stackPop();
    thread->stackSetTop(NoneType::object());
    return true;
  }
  return false;
}

static bool underSeqIndex(Thread* thread) {
  thread->stackSetTop(
      SmallInt::fromWord(SeqIterator::cast(thread->stackPop()).index()));
  return true;
}

static bool underSeqIterable(Thread* thread) {
  thread->stackSetTop(SeqIterator::cast(thread->stackPop()).iterable());
  return true;
}

static bool underSeqSetIndex(Thread* thread) {
  RawObject seq_iter = thread->stackPop();
  RawObject index = thread->stackPop();
  SeqIterator::cast(seq_iter).setIndex(Int::cast(index).asWord());
  return true;
}

static bool underSeqSetIterable(Thread* thread) {
  RawObject seq_iter = thread->stackPop();
  RawObject iterable = thread->stackPop();
  SeqIterator::cast(seq_iter).setIterable(iterable);
  return true;
}

static bool underSetCheck(Thread* thread) {
  thread->stackSetTop(
      Bool::fromBool(thread->runtime()->isInstanceOfSet(thread->stackPop())));
  return true;
}

static bool underSetGuard(Thread* thread) {
  if (thread->runtime()->isInstanceOfSet(thread->stackTop())) {
    thread->stackPop();
    thread->stackSetTop(NoneType::object());
    return true;
  }
  return false;
}

static bool underSetLen(Thread* thread) {
  RawObject arg = thread->stackPeek(0);
  if (arg.isSet()) {
    thread->stackPop();
    thread->stackSetTop(SmallInt::fromWord(Set::cast(arg).numItems()));
    return true;
  }
  return false;
}

static bool underSliceCheck(Thread* thread) {
  thread->stackSetTop(Bool::fromBool(thread->stackPop().isSlice()));
  return true;
}

static bool underSliceGuard(Thread* thread) {
  if (thread->stackTop().isSlice()) {
    thread->stackPop();
    thread->stackSetTop(NoneType::object());
    return true;
  }
  return false;
}

static bool underSliceIndex(Thread* thread) {
  RawObject value = thread->stackPeek(0);
  if (value.isNoneType() || thread->runtime()->isInstanceOfInt(value)) {
    thread->stackPop();
    thread->stackSetTop(value);
    return true;
  }
  return false;
}

static bool underSliceIndexNotNone(Thread* thread) {
  RawObject value = thread->stackTop();
  if (thread->runtime()->isInstanceOfInt(value)) {
    thread->stackPop();
    thread->stackSetTop(value);
    return true;
  }
  return false;
}

static bool underStrCheck(Thread* thread) {
  thread->stackSetTop(
      Bool::fromBool(thread->runtime()->isInstanceOfStr(thread->stackPop())));
  return true;
}

static bool underStrCheckExact(Thread* thread) {
  thread->stackSetTop(Bool::fromBool(thread->stackPop().isStr()));
  return true;
}

static bool underStrGuard(Thread* thread) {
  if (thread->runtime()->isInstanceOfStr(thread->stackTop())) {
    thread->stackPop();
    thread->stackSetTop(NoneType::object());
    return true;
  }
  return false;
}

static bool underStrLen(Thread* thread) {
  RawObject arg = thread->stackPeek(0);
  if (arg.isStr()) {
    thread->stackPop();
    thread->stackSetTop(SmallInt::fromWord(Str::cast(arg).codePointLength()));
    return true;
  }
  return false;
}

static bool underTupleCheck(Thread* thread) {
  thread->stackSetTop(
      Bool::fromBool(thread->runtime()->isInstanceOfTuple(thread->stackPop())));
  return true;
}

static bool underTupleCheckExact(Thread* thread) {
  thread->stackSetTop(Bool::fromBool(thread->stackPop().isTuple()));
  return true;
}

static bool underTupleGetitem(Thread* thread) {
  RawObject arg0 = thread->stackPeek(1);
  if (!arg0.isTuple()) {
    return false;
  }
  RawObject arg1 = thread->stackPeek(0);
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
    thread->stackDrop(2);
    thread->stackSetTop(self.at(idx));
    return true;
  }
  return false;
}

static bool underTupleGuard(Thread* thread) {
  if (thread->runtime()->isInstanceOfTuple(thread->stackTop())) {
    thread->stackPop();
    thread->stackSetTop(NoneType::object());
    return true;
  }
  return false;
}

static bool underTupleLen(Thread* thread) {
  RawObject arg = thread->stackPeek(0);
  if (arg.isTuple()) {
    thread->stackPop();
    thread->stackSetTop(SmallInt::fromWord(Tuple::cast(arg).length()));
    return true;
  }
  return false;
}

static bool underType(Thread* thread) {
  thread->stackSetTop(thread->runtime()->typeOf(thread->stackPop()));
  return true;
}

static bool underTypeCheck(Thread* thread) {
  thread->stackSetTop(
      Bool::fromBool(thread->runtime()->isInstanceOfType(thread->stackPop())));
  return true;
}

static bool underTypeCheckExact(Thread* thread) {
  thread->stackSetTop(Bool::fromBool(thread->stackPop().isType()));
  return true;
}

static bool underTypeGuard(Thread* thread) {
  if (thread->runtime()->isInstanceOfType(thread->stackTop())) {
    thread->stackPop();
    thread->stackSetTop(NoneType::object());
    return true;
  }
  return false;
}

static bool underTypeSubclassGuard(Thread* thread) {
  RawObject subclass = thread->stackPeek(0);
  RawObject superclass = thread->stackPeek(1);
  if (subclass == superclass && subclass.isType()) {
    thread->stackDrop(2);
    thread->stackSetTop(NoneType::object());
    return true;
  }
  return false;
}

static bool underWeakrefCheck(Thread* thread) {
  thread->stackSetTop(Bool::fromBool(
      thread->runtime()->isInstanceOfWeakRef(thread->stackPop())));
  return true;
}

static bool underWeakrefGuard(Thread* thread) {
  if (thread->runtime()->isInstanceOfWeakRef(thread->stackTop())) {
    thread->stackPop();
    thread->stackSetTop(NoneType::object());
    return true;
  }
  return false;
}

static bool isinstance(Thread* thread) {
  if (thread->runtime()->typeOf(thread->stackPeek(1)) == thread->stackPeek(0)) {
    thread->stackDrop(2);
    thread->stackSetTop(Bool::trueObj());
    return true;
  }
  return false;
}

static bool len(Thread* thread) {
  RawObject arg = thread->stackTop();
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
  thread->stackPop();
  thread->stackSetTop(SmallInt::fromWord(length));
  return true;
}

bool doIntrinsic(Thread* thread, Frame*, SymbolId name) {
  switch (name) {
    case ID(_bool_check):
      return underBoolCheck(thread);
    case ID(_bool_guard):
      return underBoolGuard(thread);
    case ID(_bytearray_check):
      return underBytearrayCheck(thread);
    case ID(_bytearray_guard):
      return underBytearrayGuard(thread);
    case ID(_bytearray_len):
      return underBytearrayLen(thread);
    case ID(_bytes_check):
      return underBytesCheck(thread);
    case ID(_bytes_guard):
      return underBytesGuard(thread);
    case ID(_bytes_len):
      return underBytesLen(thread);
    case ID(_byteslike_check):
      return underByteslikeCheck(thread);
    case ID(_byteslike_guard):
      return underByteslikeGuard(thread);
    case ID(_complex_check):
      return underComplexCheck(thread);
    case ID(_deque_guard):
      return underDequeGuard(thread);
    case ID(_dict_check):
      return underDictCheck(thread);
    case ID(_dict_check_exact):
      return underDictCheckExact(thread);
    case ID(_dict_guard):
      return underDictGuard(thread);
    case ID(_dict_len):
      return underDictLen(thread);
    case ID(_float_check):
      return underFloatCheck(thread);
    case ID(_float_check_exact):
      return underFloatCheckExact(thread);
    case ID(_float_guard):
      return underFloatGuard(thread);
    case ID(_frozenset_check):
      return underFrozensetCheck(thread);
    case ID(_frozenset_guard):
      return underFrozensetGuard(thread);
    case ID(_function_guard):
      return underFunctionGuard(thread);
    case ID(_index):
      return underIndex(thread);
    case ID(_int_check):
      return underIntCheck(thread);
    case ID(_int_check_exact):
      return underIntCheckExact(thread);
    case ID(_int_guard):
      return underIntGuard(thread);
    case ID(_list_check):
      return underListCheck(thread);
    case ID(_list_check_exact):
      return underListCheckExact(thread);
    case ID(_list_getitem):
      return underListGetitem(thread);
    case ID(_list_guard):
      return underListGuard(thread);
    case ID(_list_len):
      return underListLen(thread);
    case ID(_list_setitem):
      return underListSetitem(thread);
    case ID(_memoryview_guard):
      return underMemoryviewGuard(thread);
    case ID(_number_check):
      return underNumberCheck(thread);
    case ID(_range_check):
      return underRangeCheck(thread);
    case ID(_range_guard):
      return underRangeGuard(thread);
    case ID(_seq_index):
      return underSeqIndex(thread);
    case ID(_seq_iterable):
      return underSeqIterable(thread);
    case ID(_seq_set_index):
      return underSeqSetIndex(thread);
    case ID(_seq_set_iterable):
      return underSeqSetIterable(thread);
    case ID(_set_check):
      return underSetCheck(thread);
    case ID(_set_guard):
      return underSetGuard(thread);
    case ID(_set_len):
      return underSetLen(thread);
    case ID(_slice_check):
      return underSliceCheck(thread);
    case ID(_slice_guard):
      return underSliceGuard(thread);
    case ID(_slice_index):
      return underSliceIndex(thread);
    case ID(_slice_index_not_none):
      return underSliceIndexNotNone(thread);
    case ID(_str_check):
      return underStrCheck(thread);
    case ID(_str_check_exact):
      return underStrCheckExact(thread);
    case ID(_str_guard):
      return underStrGuard(thread);
    case ID(_str_len):
      return underStrLen(thread);
    case ID(_tuple_check):
      return underTupleCheck(thread);
    case ID(_tuple_check_exact):
      return underTupleCheckExact(thread);
    case ID(_tuple_getitem):
      return underTupleGetitem(thread);
    case ID(_tuple_guard):
      return underTupleGuard(thread);
    case ID(_tuple_len):
      return underTupleLen(thread);
    case ID(_type):
      return underType(thread);
    case ID(_type_check):
      return underTypeCheck(thread);
    case ID(_type_check_exact):
      return underTypeCheckExact(thread);
    case ID(_type_guard):
      return underTypeGuard(thread);
    case ID(_type_subclass_guard):
      return underTypeSubclassGuard(thread);
    case ID(_weakref_check):
      return underWeakrefCheck(thread);
    case ID(_weakref_guard):
      return underWeakrefGuard(thread);
    case ID(isinstance):
      return isinstance(thread);
    case ID(len):
      return len(thread);
    default:
      UNREACHABLE("function %s does not have an intrinsic implementation",
                  Symbols::predefinedSymbolAt(name));
  }
}

}  // namespace py
