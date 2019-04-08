#include "tuple-builtins.h"

#include "frame.h"
#include "globals.h"
#include "interpreter.h"
#include "objects.h"
#include "runtime.h"
#include "slice-builtins.h"
#include "thread.h"
#include "trampolines-inl.h"

namespace python {

RawObject underStructseqGetAttr(Thread* thread, Frame* frame, word nargs) {
  // TODO(T40703284): Prevent structseq.__dict__["foo"] access
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  HeapObject structseq(&scope, args.get(0));
  Object name(&scope, args.get(1));
  return runtime->instanceAt(thread, structseq, name);
}

RawObject underStructseqSetAttr(Thread* thread, Frame* frame, word nargs) {
  // TODO(T40703284): Prevent structseq.__dict__["foo"] access
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  HeapObject structseq(&scope, args.get(0));
  Object name(&scope, args.get(1));
  Object value(&scope, args.get(2));
  return runtime->instanceAtPut(thread, structseq, name, value);
}

RawObject sequenceAsTuple(Thread* thread, const Object& seq) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  if (seq.isTuple()) return *seq;
  if (runtime->isInstanceOfList(*seq)) {
    List list(&scope, *seq);
    word len = list.numItems();
    Tuple ret(&scope, runtime->newTuple(len));
    for (word i = 0; i < len; i++) {
      ret.atPut(i, list.at(i));
    }
    return *ret;
  }

  // TODO(T40263865): Support arbitrary iterables.
  UNIMPLEMENTED("arbitrary iterables in sequenceAsTuple");
}

RawObject tupleIteratorNext(Thread* thread, const TupleIterator& iter) {
  word idx = iter.index();
  if (idx == iter.tupleLength()) {
    return RawError::object();
  }
  HandleScope scope(thread);
  Tuple underlying(&scope, iter.iterable());
  RawObject item = underlying.at(idx);
  iter.setIndex(idx + 1);
  return item;
}

const BuiltinAttribute TupleBuiltins::kAttributes[] = {
    {SymbolId::kInvalid, RawUserTupleBase::kTupleOffset},
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
    {SymbolId::kDunderNew, dunderNew},
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
  if (!lhs.isTuple()) {
    UserTupleBase lhs_user(&scope, *lhs);
    lhs = lhs_user.tupleValue();
  }
  Tuple left(&scope, *lhs);
  Object rhs(&scope, args.get(1));
  if (!runtime->isInstanceOfTuple(*rhs)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "can only concatenate tuple to tuple, got %T",
                                &rhs);
  }
  if (!rhs.isTuple()) {
    UserTupleBase rhs_user(&scope, *rhs);
    rhs = rhs_user.tupleValue();
  }
  Tuple right(&scope, *rhs);
  word llength = left.length();
  word rlength = right.length();

  word new_length = llength + rlength;
  if (new_length > kMaxWord) {
    return thread->raiseOverflowErrorWithCStr(
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

  Tuple self(&scope, *self_obj);
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
    found = Interpreter::isTrue(thread, frame, comp_result);
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

  if (!self_obj.isTuple()) {
    UserTupleBase user_self(&scope, *self_obj);
    self_obj = user_self.tupleValue();
  }
  if (!other_obj.isTuple()) {
    UserTupleBase user_other(&scope, *other_obj);
    other_obj = user_other.tupleValue();
  }

  Tuple self(&scope, *self_obj);
  Tuple other(&scope, *other_obj);
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

  if (!self.isTuple()) {
    UserTupleBase user_tuple(&scope, *self);
    self = user_tuple.tupleValue();
  }

  Tuple tuple(&scope, *self);
  Object index(&scope, args.get(1));
  if (index.isSmallInt()) {
    word idx = RawSmallInt::cast(*index).value();
    if (idx < 0) {
      idx += tuple.length();
    }
    if (idx < 0 || idx >= tuple.length()) {
      return thread->raiseIndexErrorWithCStr("tuple index out of range");
    }
    return tuple.at(idx);
  }
  if (index.isSlice()) {
    Slice tuple_slice(&scope, *index);
    return slice(thread, tuple, tuple_slice);
  }
  return thread->raiseTypeErrorWithCStr(
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
  Tuple self(&scope, *self_obj);
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
    DCHECK(hash_result_obj.isSmallInt(), "hash result must be smallint");
    word hash_result = SmallInt::cast(*hash_result_obj).value();
    result = (result ^ hash_result) * mult;
    mult += static_cast<word>(82520UL + len + len);
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
  if (!obj.isTuple()) {
    UserTupleBase user_tuple(&scope, *obj);
    obj = user_tuple.tupleValue();
  }
  Tuple self(&scope, *obj);
  return runtime->newInt(self.length());
}

RawObject TupleBuiltins::dunderMul(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isTuple()) {
    UserTupleBase user_tuple(&scope, *self_obj);
    self_obj = user_tuple.tupleValue();
  }
  Tuple self(&scope, *self_obj);
  Object rhs(&scope, args.get(1));
  if (!rhs.isInt()) {
    return thread->raiseTypeErrorWithCStr("can't multiply sequence by non-int");
  }
  if (!rhs.isSmallInt()) {
    return thread->raiseOverflowErrorWithCStr(
        "cannot fit 'int' into an index-sized integer");
  }
  SmallInt right(&scope, *rhs);
  word length = self.length();
  word times = right.value();
  if (length == 0 || times <= 0) {
    return thread->runtime()->newTuple(0);
  }
  if (length == 1 || times == 1) {
    return *self;
  }

  word new_length = length * times;
  // If the new length overflows, raise an OverflowError.
  if ((new_length / length) != times) {
    return thread->raiseOverflowErrorWithCStr(
        "cannot fit 'int' into an index-sized integer");
  }

  Tuple new_tuple(&scope, thread->runtime()->newTuple(new_length));
  for (word i = 0; i < times; i++) {
    for (word j = 0; j < length; j++) {
      new_tuple.atPut(i * length + j, self.at(j));
    }
  }
  return *new_tuple;
}

static RawObject newTupleOrUserSubclass(Thread* thread, const Tuple& tuple,
                                        const Type& type) {
  if (type.isBuiltin()) return *tuple;
  HandleScope scope(thread);
  Layout layout(&scope, type.instanceLayout());
  UserTupleBase instance(&scope, thread->runtime()->newInstance(layout));
  instance.setTupleValue(*tuple);
  return *instance;
}

RawObject TupleBuiltins::dunderNew(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object type_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfType(*type_obj)) {
    return thread->raiseTypeErrorWithCStr(
        "tuple.__new__(X): X is not a type object");
  }

  Type type(&scope, *type_obj);
  if (type.builtinBase() != LayoutId::kTuple) {
    return thread->raiseTypeErrorWithCStr(
        "tuple.__new__(X): X is not a subclass of tuple");
  }

  // If no iterable is given as an argument, return an empty zero tuple.
  if (args.get(1).isUnbound()) {
    Tuple tuple(&scope, runtime->newTuple(0));
    return newTupleOrUserSubclass(thread, tuple, type);
  }

  // Construct a new tuple from the iterable.
  Object iterable(&scope, args.get(1));
  Object dunder_iter(&scope, Interpreter::lookupMethod(thread, frame, iterable,
                                                       SymbolId::kDunderIter));
  if (dunder_iter.isError()) {
    return thread->raiseTypeErrorWithCStr("object is not iterable");
  }
  Object iterator(
      &scope, Interpreter::callMethod1(thread, frame, dunder_iter, iterable));
  Object dunder_next(&scope, Interpreter::lookupMethod(thread, frame, iterator,
                                                       SymbolId::kDunderNext));
  word max_len = 10;
  // If the iterator has a __length_hint__, use that as max_len to avoid
  // resizes.
  Type iter_type(&scope, runtime->typeOf(*iterator));
  Object length_hint(&scope,
                     runtime->lookupSymbolInMro(thread, iter_type,
                                                SymbolId::kDunderLengthHint));
  if (length_hint.isSmallInt()) {
    max_len = RawSmallInt::cast(*length_hint).value();
  }

  word curr = 0;
  Tuple result(&scope, runtime->newTuple(max_len));
  // Iterate through the iterable, copying elements into the tuple.
  for (;;) {
    Object elem(&scope,
                Interpreter::callMethod1(thread, frame, dunder_next, iterator));
    if (elem.isError()) {
      if (thread->clearPendingStopIteration()) break;
      return *elem;
    }
    // If the capacity of the current result is reached, create a new larger
    // tuple and copy over the contents.
    if (curr == max_len) {
      max_len *= 2;
      Tuple new_tuple(&scope, runtime->newTuple(max_len));
      for (word i = 0; i < curr; i++) {
        new_tuple.atPut(i, result.at(i));
      }
      result = *new_tuple;
    }
    result.atPut(curr++, *elem);
  }

  // If the result is perfectly sized, return it.
  if (curr == max_len) {
    return newTupleOrUserSubclass(thread, result, type);
  }

  // The result was over-allocated, shrink it.
  Tuple new_tuple(&scope, runtime->newTuple(curr));
  for (word i = 0; i < curr; i++) {
    new_tuple.atPut(i, result.at(i));
  }
  return newTupleOrUserSubclass(thread, new_tuple, type);
}

RawObject TupleBuiltins::dunderIter(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfTuple(*self)) {
    return thread->raiseRequiresType(self, SymbolId::kTuple);
  }
  if (!self.isTuple()) {
    UserTupleBase user_tuple(&scope, *self);
    self = user_tuple.tupleValue();
  }
  Tuple tuple(&scope, *self);
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
    return thread->raiseStopIteration(NoneType::object());
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
