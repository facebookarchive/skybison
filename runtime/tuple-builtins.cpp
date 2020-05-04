#include "tuple-builtins.h"

#include "builtins.h"
#include "dict-builtins.h"
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

namespace py {

RawObject tupleIteratorNext(Thread* thread, const TupleIterator& iter) {
  word idx = iter.index();
  if (idx == iter.length()) {
    return Error::noMoreItems();
  }
  HandleScope scope(thread);
  Tuple underlying(&scope, iter.iterable());
  RawObject item = underlying.at(idx);
  iter.setIndex(idx + 1);
  return item;
}

RawObject tupleSlice(Thread* thread, const Tuple& tuple, word start, word stop,
                     word step) {
  if (start == 0 && stop >= tuple.length() && step == 1) {
    return *tuple;
  }

  HandleScope scope(thread);
  word length = Slice::length(start, stop, step);
  MutableTuple items(&scope, thread->runtime()->newMutableTuple(length));
  for (word i = 0, index = start; i < length; i++, index += step) {
    items.atPut(i, tuple.at(index));
  }
  return items.becomeImmutable();
}

RawObject tupleHash(Thread* thread, const Tuple& tuple) {
  HandleScope scope(thread);
  Object elt(&scope, NoneType::object());
  Object elt_hash(&scope, NoneType::object());
  uword result = 0x345678UL;
  uword mult = 1000003UL;  // 0xf4243
  word len = tuple.length();
  for (word i = len - 1; i >= 0; i--) {
    elt = tuple.at(i);
    elt_hash = Interpreter::hash(thread, elt);
    if (elt_hash.isErrorException()) return *elt_hash;
    word hash_result = SmallInt::cast(*elt_hash).value();
    result = (result ^ hash_result) * mult;
    mult += word{82520} + len + len;
  }
  result += 97531UL;
  if (result == kMaxUword) {
    return SmallInt::fromWord(-2);
  }
  return SmallInt::fromWordTruncated(result);
}

const BuiltinAttribute TupleBuiltins::kAttributes[] = {
    {ID(_UserTuple__value), UserTupleBase::kValueOffset,
     AttributeFlags::kHidden},
    {SymbolId::kSentinelId, -1},
};

RawObject METH(tuple, __add__)(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object lhs(&scope, args.get(0));
  if (!runtime->isInstanceOfTuple(*lhs)) {
    return thread->raiseRequiresType(lhs, ID(tuple));
  }
  Tuple left(&scope, tupleUnderlying(*lhs));
  Object rhs(&scope, args.get(1));
  if (!runtime->isInstanceOfTuple(*rhs)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "can only concatenate tuple to tuple, got %T",
                                &rhs);
  }
  Tuple right(&scope, tupleUnderlying(*rhs));
  word llength = left.length();
  word rlength = right.length();

  word new_length = llength + rlength;
  if (new_length > SmallInt::kMaxValue) {
    return thread->raiseWithFmt(LayoutId::kOverflowError,
                                "cannot fit 'int' into an index-sized integer");
  }
  if (new_length == 0) {
    return runtime->emptyTuple();
  }
  MutableTuple new_tuple(&scope, runtime->newMutableTuple(new_length));
  new_tuple.replaceFromWith(0, *left, llength);
  new_tuple.replaceFromWith(llength, *right, rlength);
  return new_tuple.becomeImmutable();
}

RawObject tupleContains(Thread* thread, const Tuple& tuple,
                        const Object& value) {
  for (word i = 0, num_items = tuple.length(); i < num_items; ++i) {
    RawObject eq = Runtime::objectEquals(thread, *value, tuple.at(i));
    if (eq != Bool::falseObj()) return eq;
  }
  return Bool::falseObj();
}

RawObject METH(tuple, __contains__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfTuple(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(tuple));
  }
  Tuple self(&scope, tupleUnderlying(*self_obj));
  Object value(&scope, args.get(1));
  return tupleContains(thread, self, value);
}

RawObject METH(tuple, __hash__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfTuple(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(tuple));
  }
  Tuple self(&scope, tupleUnderlying(*self_obj));
  return tupleHash(thread, self);
}

RawObject METH(tuple, __len__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfTuple(*obj)) {
    return thread->raiseRequiresType(obj, ID(tuple));
  }
  Tuple self(&scope, tupleUnderlying(*obj));
  return runtime->newInt(self.length());
}

RawObject METH(tuple, __mul__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfTuple(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(tuple));
  }
  Tuple self(&scope, tupleUnderlying(*self_obj));
  Object rhs(&scope, args.get(1));
  Object rhs_index(&scope, intFromIndex(thread, rhs));
  if (rhs_index.isError()) return *rhs_index;
  Int right(&scope, intUnderlying(*rhs_index));
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

  MutableTuple new_tuple(&scope, runtime->newMutableTuple(new_length));
  if (length == 1) {
    // Fast path for single-element tuples
    new_tuple.fill(self.at(0));
    return new_tuple.becomeImmutable();
  }
  for (word i = 0; i < times; i++) {
    new_tuple.replaceFromWith(i * length, *self, length);
  }
  return new_tuple.becomeImmutable();
}

RawObject METH(tuple, __iter__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfTuple(*self)) {
    return thread->raiseRequiresType(self, ID(tuple));
  }
  Tuple tuple(&scope, tupleUnderlying(*self));
  return runtime->newTupleIterator(tuple, tuple.length());
}

RawObject METH(tuple_iterator, __iter__)(Thread* thread, Frame* frame,
                                         word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isTupleIterator()) {
    return thread->raiseRequiresType(self, ID(tuple_iterator));
  }
  return *self;
}

RawObject METH(tuple_iterator, __next__)(Thread* thread, Frame* frame,
                                         word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isTupleIterator()) {
    return thread->raiseRequiresType(self_obj, ID(tuple_iterator));
  }
  TupleIterator self(&scope, *self_obj);
  Object value(&scope, tupleIteratorNext(thread, self));
  if (value.isError()) {
    return thread->raise(LayoutId::kStopIteration, NoneType::object());
  }
  return *value;
}

RawObject METH(tuple_iterator, __length_hint__)(Thread* thread, Frame* frame,
                                                word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isTupleIterator()) {
    return thread->raiseRequiresType(self, ID(tuple_iterator));
  }
  TupleIterator tuple_iterator(&scope, *self);
  Tuple tuple(&scope, tuple_iterator.iterable());
  return SmallInt::fromWord(tuple.length() - tuple_iterator.index());
}

}  // namespace py
