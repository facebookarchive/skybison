#include "tuple-builtins.h"

#include "frame.h"
#include "globals.h"
#include "int-builtins.h"
#include "interpreter.h"
#include "object-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "slice-builtins.h"
#include "thread.h"
#include "type-builtins.h"

namespace python {

RawObject sequenceAsTuple(Thread* thread, const Object& seq) {
  Runtime* runtime = thread->runtime();

  if (seq.isTuple()) return *seq;
  if (runtime->isInstanceOfList(*seq)) {
    HandleScope scope(thread);
    List list(&scope, *seq);
    word len = list.numItems();
    Tuple ret(&scope, runtime->newTuple(len));
    for (word i = 0; i < len; i++) {
      ret.atPut(i, list.at(i));
    }
    return *ret;
  }

  return thread->invokeFunction1(SymbolId::kBuiltins, SymbolId::kTuple, seq);
}

RawObject tupleIteratorNext(Thread* thread, const TupleIterator& iter) {
  word idx = iter.index();
  if (idx == iter.tupleLength()) {
    return Error::noMoreItems();
  }
  HandleScope scope(thread);
  Tuple underlying(&scope, iter.iterable());
  RawObject item = underlying.at(idx);
  iter.setIndex(idx + 1);
  return item;
}

RawObject tupleUnderlying(Thread* thread, const Object& obj) {
  if (obj.isTuple()) return *obj;
  DCHECK(thread->runtime()->isInstanceOfTuple(*obj), "Non-tuple value");
  HandleScope scope(thread);
  UserTupleBase base(&scope, *obj);
  return base.tupleValue();
}

const BuiltinAttribute TupleBuiltins::kAttributes[] = {
    {SymbolId::kInvalid, UserTupleBase::kTupleOffset},
    {SymbolId::kSentinelId, -1},
};

const BuiltinMethod TupleBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderAdd, dunderAdd},
    {SymbolId::kDunderContains, dunderContains},
    {SymbolId::kDunderEq, dunderEq},
    {SymbolId::kDunderGetitem, dunderGetItem},
    {SymbolId::kDunderHash, dunderHash},
    {SymbolId::kDunderIter, dunderIter},
    {SymbolId::kDunderLen, dunderLen},
    {SymbolId::kDunderMul, dunderMul},
    {SymbolId::kSentinelId, nullptr},
};

RawObject TupleBuiltins::dunderAdd(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object lhs(&scope, args.get(0));
  if (!runtime->isInstanceOfTuple(*lhs)) {
    return thread->raiseRequiresType(lhs, SymbolId::kTuple);
  }
  Tuple left(&scope, tupleUnderlying(thread, lhs));
  Object rhs(&scope, args.get(1));
  if (!runtime->isInstanceOfTuple(*rhs)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "can only concatenate tuple to tuple, got %T",
                                &rhs);
  }
  Tuple right(&scope, tupleUnderlying(thread, rhs));
  word llength = left.length();
  word rlength = right.length();

  word new_length = llength + rlength;
  if (new_length > kMaxWord) {
    return thread->raiseWithFmt(LayoutId::kOverflowError,
                                "cannot fit 'int' into an index-sized integer");
  }

  Tuple new_tuple(&scope, runtime->newTuple(new_length));
  for (word i = 0; i < llength; ++i) {
    new_tuple.atPut(i, left.at(i));
  }
  for (word j = 0; j < rlength; ++j) {
    new_tuple.atPut(llength + j, right.at(j));
  }
  return *new_tuple;
}

RawObject TupleBuiltins::dunderContains(Thread* thread, Frame* frame,
                                        word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfTuple(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kTuple);
  }

  Tuple self(&scope, tupleUnderlying(thread, self_obj));
  Object value(&scope, args.get(1));
  Object item(&scope, NoneType::object());
  Object comp_result(&scope, NoneType::object());
  Object found(&scope, NoneType::object());
  for (word i = 0, num_items = self.length(); i < num_items; ++i) {
    item = self.at(i);
    if (*value == *item) {
      return Bool::trueObj();
    }
    comp_result = thread->invokeFunction2(SymbolId::kOperator, SymbolId::kEq,
                                          value, item);
    if (comp_result.isError()) return *comp_result;
    found = Interpreter::isTrue(thread, *comp_result);
    if (found.isError()) return *found;
    if (found == Bool::trueObj()) {
      return *found;
    }
  }
  return Bool::falseObj();
}

RawObject TupleBuiltins::dunderEq(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Object other_obj(&scope, args.get(1));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfTuple(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kTuple);
  }
  if (!runtime->isInstanceOfTuple(*other_obj)) {
    return NotImplementedType::object();
  }

  Tuple self(&scope, tupleUnderlying(thread, self_obj));
  Tuple other(&scope, tupleUnderlying(thread, other_obj));
  if (self.length() != other.length()) {
    return Bool::falseObj();
  }
  Object left(&scope, NoneType::object());
  Object right(&scope, NoneType::object());
  word length = self.length();
  for (word i = 0; i < length; i++) {
    left = self.at(i);
    right = other.at(i);
    RawObject result =
        Interpreter::compareOperation(thread, frame, EQ, left, right);
    if (result == Bool::falseObj()) {
      return result;
    }
  }
  return Bool::trueObj();
}

RawObject TupleBuiltins::slice(Thread* thread, const Tuple& tuple,
                               const Slice& slice) {
  HandleScope scope(thread);
  word start, stop, step;
  Object err(&scope, sliceUnpack(thread, slice, &start, &stop, &step));
  if (err.isError()) return *err;
  return sliceWithWords(thread, tuple, start, stop, step);
}

RawObject TupleBuiltins::sliceWithWords(Thread* thread, const Tuple& tuple,
                                        word start, word stop, word step) {
  word length = Slice::adjustIndices(tuple.length(), &start, &stop, step);
  if (start == 0 && stop >= tuple.length() && step == 1) {
    return *tuple;
  }

  HandleScope scope(thread);
  Tuple items(&scope, thread->runtime()->newTuple(length));
  for (word i = 0, index = start; i < length; i++, index += step) {
    items.atPut(i, tuple.at(index));
  }
  return *items;
}

RawObject TupleBuiltins::dunderGetItem(Thread* thread, Frame* frame,
                                       word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfTuple(*self)) {
    return thread->raiseRequiresType(self, SymbolId::kTuple);
  }

  Tuple tuple(&scope, tupleUnderlying(thread, self));
  Object index(&scope, args.get(1));
  if (index.isSmallInt()) {
    word idx = SmallInt::cast(*index).value();
    if (idx < 0) {
      idx += tuple.length();
    }
    if (idx < 0 || idx >= tuple.length()) {
      return thread->raiseWithFmt(LayoutId::kIndexError,
                                  "tuple index out of range");
    }
    return tuple.at(idx);
  }
  if (index.isSlice()) {
    Slice tuple_slice(&scope, *index);
    return slice(thread, tuple, tuple_slice);
  }
  return thread->raiseWithFmt(LayoutId::kTypeError,
                              "tuple indices must be integers or slices");
}

RawObject TupleBuiltins::dunderHash(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfTuple(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kTuple);
  }
  Tuple self(&scope, tupleUnderlying(thread, self_obj));
  Object elt(&scope, NoneType::object());
  Object hash_result_obj(&scope, NoneType::object());
  uword result = 0x345678UL;
  uword mult = 1000003UL /* 0xf4243 */;
  word len = self.length();
  for (word i = len - 1; i >= 0; i--) {
    elt = self.at(i);
    hash_result_obj = thread->invokeMethod1(elt, SymbolId::kDunderHash);
    if (hash_result_obj.isError()) {
      return *hash_result_obj;
    }
    if (!runtime->isInstanceOfInt(*hash_result_obj)) {
      return thread->raiseWithFmt(LayoutId::kTypeError,
                                  "__hash__ method should return an integer");
    }
    // TODO(T44339224): Remove this.
    DCHECK(hash_result_obj.isSmallInt(), "hash result must be smallint");
    word hash_result = SmallInt::cast(*hash_result_obj).value();
    result = (result ^ hash_result) * mult;
    mult += word{82520} + len + len;
  }
  result += 97531UL;
  if (result == kMaxUword) {
    return SmallInt::fromWord(-2);
  }
  return SmallInt::fromWordTruncated(result);
}

RawObject TupleBuiltins::dunderLen(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfTuple(*obj)) {
    return thread->raiseRequiresType(obj, SymbolId::kTuple);
  }
  Tuple self(&scope, tupleUnderlying(thread, obj));
  return runtime->newInt(self.length());
}

RawObject TupleBuiltins::dunderMul(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfTuple(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kTuple);
  }
  Tuple self(&scope, tupleUnderlying(thread, self_obj));
  Object rhs(&scope, args.get(1));
  Object rhs_index(&scope, intFromIndex(thread, rhs));
  if (rhs_index.isError()) return *rhs_index;
  Int right(&scope, intUnderlying(thread, rhs_index));
  if (right.isLargeInt()) {
    return thread->raiseWithFmt(LayoutId::kOverflowError,
                                "cannot fit '%T' into an index-sized integer",
                                &rhs);
  }
  word length = self.length();
  word times = right.asWord();
  if (length == 0 || times <= 0) {
    return runtime->emptyTuple();
  }
  if (times == 1) {
    return *self;
  }

  word new_length = length * times;
  // If the new length overflows, raise an OverflowError.
  if ((new_length / length) != times) {
    return thread->raiseWithFmt(LayoutId::kOverflowError,
                                "cannot fit 'int' into an index-sized integer");
  }

  Tuple new_tuple(&scope, runtime->newTuple(new_length));
  if (length == 1) {
    // Fast path for single-element tuples
    new_tuple.fill(self.at(0));
    return *new_tuple;
  }
  for (word i = 0; i < times; i++) {
    for (word j = 0; j < length; j++) {
      new_tuple.atPut(i * length + j, self.at(j));
    }
  }
  return *new_tuple;
}

RawObject TupleBuiltins::dunderIter(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfTuple(*self)) {
    return thread->raiseRequiresType(self, SymbolId::kTuple);
  }
  Tuple tuple(&scope, tupleUnderlying(thread, self));
  return runtime->newTupleIterator(tuple, tuple.length());
}

const BuiltinMethod TupleIteratorBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderIter, dunderIter},
    {SymbolId::kDunderLengthHint, dunderLengthHint},
    {SymbolId::kDunderNext, dunderNext},
    {SymbolId::kSentinelId, nullptr},
};

RawObject TupleIteratorBuiltins::dunderIter(Thread* thread, Frame* frame,
                                            word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isTupleIterator()) {
    return thread->raiseRequiresType(self, SymbolId::kTupleIterator);
  }
  return *self;
}

RawObject TupleIteratorBuiltins::dunderNext(Thread* thread, Frame* frame,
                                            word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isTupleIterator()) {
    return thread->raiseRequiresType(self_obj, SymbolId::kTupleIterator);
  }
  TupleIterator self(&scope, *self_obj);
  Object value(&scope, tupleIteratorNext(thread, self));
  if (value.isError()) {
    return thread->raise(LayoutId::kStopIteration, NoneType::object());
  }
  return *value;
}

RawObject TupleIteratorBuiltins::dunderLengthHint(Thread* thread, Frame* frame,
                                                  word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isTupleIterator()) {
    return thread->raiseRequiresType(self, SymbolId::kTupleIterator);
  }
  TupleIterator tuple_iterator(&scope, *self);
  Tuple tuple(&scope, tuple_iterator.iterable());
  return SmallInt::fromWord(tuple.length() - tuple_iterator.index());
}

}  // namespace python
