#include "list-builtins.h"

#include "frame.h"
#include "globals.h"
#include "int-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "slice-builtins.h"
#include "thread.h"
#include "tuple-builtins.h"

namespace python {

RawObject listExtend(Thread* thread, const List& dst, const Object& iterable) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  if (runtime->isInstanceOfList(*iterable)) {
    // Special case for lists
    List src(&scope, *iterable);
    word old_length = dst.numItems();
    word new_length = old_length + src.numItems();
    if (new_length != old_length) {
      runtime->listEnsureCapacity(thread, dst, new_length);
      // Save the number of items before adding the two sizes together. This is
      // for the a.extend(a) case (src == dst).
      dst.setNumItems(new_length);
      Tuple dst_items(&scope, dst.items());
      Tuple src_items(&scope, src.items());
      dst_items.replaceFromWith(old_length, *src_items);
    }
    return NoneType::object();
  }
  // Special case for tuples
  DCHECK(runtime->isInstanceOfTuple(*iterable),
         "iterable must be list or tuple instance");
  Tuple src(&scope, tupleUnderlying(thread, iterable));
  word old_length = dst.numItems();
  word new_length = old_length + src.length();
  if (new_length != old_length) {
    runtime->listEnsureCapacity(thread, dst, new_length);
    dst.setNumItems(new_length);
    Tuple dst_items(&scope, dst.items());
    dst_items.replaceFromWith(old_length, *src);
  }
  return NoneType::object();
}

void listInsert(Thread* thread, const List& list, const Object& value,
                word index) {
  thread->runtime()->listAdd(thread, list, value);
  word last_index = list.numItems() - 1;
  if (index < 0) {
    index = last_index + index;
  }
  index = Utils::maximum(word{0}, Utils::minimum(last_index, index));
  for (word i = last_index; i > index; i--) {
    list.atPut(i, list.at(i - 1));
  }
  list.atPut(index, *value);
}

RawObject listPop(Thread* thread, const List& list, word index) {
  HandleScope scope(thread);
  Object popped(&scope, list.at(index));
  list.atPut(index, NoneType::object());
  word last_index = list.numItems() - 1;
  for (word i = index; i < last_index; i++) {
    list.atPut(i, list.at(i + 1));
  }
  list.setNumItems(list.numItems() - 1);
  return *popped;
}

RawObject listReplicate(Thread* thread, const List& list, word ntimes) {
  HandleScope scope(thread);
  word len = list.numItems();
  Runtime* runtime = thread->runtime();
  Tuple items(&scope, runtime->newTuple(ntimes * len));
  for (word i = 0; i < ntimes; i++) {
    for (word j = 0; j < len; j++) {
      items.atPut((i * len) + j, list.at(j));
    }
  }
  List result(&scope, runtime->newList());
  result.setItems(*items);
  result.setNumItems(items.length());
  return *result;
}

void listReverse(Thread* thread, const List& list) {
  if (list.numItems() == 0) {
    return;
  }
  HandleScope scope(thread);
  Object elt(&scope, NoneType::object());
  for (word low = 0, high = list.numItems() - 1; low < high; ++low, --high) {
    elt = list.at(low);
    list.atPut(low, list.at(high));
    list.atPut(high, *elt);
  }
}

RawObject listSlice(Thread* thread, const List& list, word start, word stop,
                    word step) {
  Runtime* runtime = thread->runtime();
  word length = Slice::length(start, stop, step);
  if (length == 0) {
    return runtime->newList();
  }

  HandleScope scope(thread);
  Tuple items(&scope, runtime->newTuple(length));
  for (word i = 0, j = start; i < length; i++, j += step) {
    items.atPut(i, list.at(j));
  }
  List result(&scope, runtime->newList());
  result.setItems(*items);
  result.setNumItems(length);
  return *result;
}

// TODO(T39107329): We should have a faster sorting algorithm than insertion
// sort. Re-write as Timsort.
RawObject listSort(Thread* thread, const List& list) {
  word length = list.numItems();
  HandleScope scope(thread);
  Object list_j(&scope, NoneType::object());
  Object list_i(&scope, NoneType::object());
  Object compare_result(&scope, NoneType::object());
  for (word i = 1; i < length; i++) {
    list_i = list.at(i);
    word j = i - 1;
    for (; j >= 0; j--) {
      list_j = list.at(j);
      compare_result = thread->invokeFunction2(SymbolId::kOperator,
                                               SymbolId::kLt, list_i, list_j);
      if (compare_result.isError()) {
        return *compare_result;
      }
      compare_result = Interpreter::isTrue(thread, *compare_result);
      if (compare_result.isError()) {
        return *compare_result;
      }
      if (!Bool::cast(*compare_result).value()) {
        break;
      }
      list.atPut(j + 1, *list_j);
    }
    list.atPut(j + 1, *list_i);
  }
  return NoneType::object();
}

RawObject listIteratorNext(Thread* thread, const ListIterator& iter) {
  HandleScope scope(thread);
  word idx = iter.index();
  List underlying(&scope, iter.iterable());
  if (idx >= underlying.numItems()) {
    return Error::outOfBounds();
  }
  RawObject item = underlying.at(idx);
  iter.setIndex(idx + 1);
  return item;
}

const BuiltinAttribute ListBuiltins::kAttributes[] = {
    {SymbolId::kInvalid, RawList::kItemsOffset},
    {SymbolId::kInvalid, RawList::kAllocatedOffset},
    {SymbolId::kSentinelId, -1},
};

const BuiltinMethod ListBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderNew, dunderNew},
    {SymbolId::kDunderAdd, dunderAdd},
    {SymbolId::kDunderContains, dunderContains},
    {SymbolId::kDunderIter, dunderIter},
    {SymbolId::kDunderLen, dunderLen},
    {SymbolId::kDunderMul, dunderMul},
    {SymbolId::kDunderSetitem, dunderSetItem},
    {SymbolId::kAppend, append},
    {SymbolId::kClear, clear},
    {SymbolId::kInsert, insert},
    {SymbolId::kPop, pop},
    {SymbolId::kRemove, remove},
    {SymbolId::kSentinelId, nullptr},
};

RawObject ListBuiltins::dunderNew(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  if (!args.get(0).isType()) {
    return thread->raiseWithFmt(LayoutId::kTypeError, "not a type object");
  }
  HandleScope scope(thread);
  Type type(&scope, args.get(0));
  if (type.builtinBase() != LayoutId::kList) {
    return thread->raiseWithFmt(LayoutId::kTypeError, "not a subtype of list");
  }
  Layout layout(&scope, type.instanceLayout());
  List result(&scope, thread->runtime()->newInstance(layout));
  result.setNumItems(0);
  result.setItems(thread->runtime()->emptyTuple());
  return *result;
}

RawObject ListBuiltins::dunderAdd(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfList(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kList);
  }
  Object other_obj(&scope, args.get(1));
  if (!runtime->isInstanceOfList(*other_obj)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "can only concatenate list to list");
  }
  List self(&scope, *self_obj);
  List other(&scope, *other_obj);
  List new_list(&scope, runtime->newList());
  runtime->listEnsureCapacity(thread, new_list,
                              self.numItems() + other.numItems());
  Object result(&scope, listExtend(thread, new_list, self));
  if (result.isError()) return *result;
  result = listExtend(thread, new_list, other);
  if (result.isError()) return *result;
  return *new_list;
}

RawObject ListBuiltins::dunderContains(Thread* thread, Frame* frame,
                                       word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfList(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kList);
  }

  List self(&scope, *self_obj);
  Object value(&scope, args.get(1));
  Object item(&scope, NoneType::object());
  Object comp_result(&scope, NoneType::object());
  Object found(&scope, NoneType::object());
  for (word i = 0, num_items = self.numItems(); i < num_items; ++i) {
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

RawObject ListBuiltins::clear(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfList(*self)) {
    return thread->raiseRequiresType(self, SymbolId::kList);
  }
  List list(&scope, *self);
  list.clearFrom(0);
  return NoneType::object();
}

RawObject ListBuiltins::append(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfList(*self)) {
    return thread->raiseRequiresType(self, SymbolId::kList);
  }
  List list(&scope, *self);
  Object value(&scope, args.get(1));
  thread->runtime()->listAdd(thread, list, value);
  return NoneType::object();
}

RawObject ListBuiltins::dunderLen(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfList(*self)) {
    return thread->raiseRequiresType(self, SymbolId::kList);
  }
  List list(&scope, *self);
  return SmallInt::fromWord(list.numItems());
}

RawObject ListBuiltins::insert(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfList(*self)) {
    return thread->raiseRequiresType(self, SymbolId::kList);
  }
  List list(&scope, *self);
  Object index_obj(&scope, args.get(1));
  index_obj = intFromIndex(thread, index_obj);
  if (index_obj.isError()) return *index_obj;
  Int index_int(&scope, intUnderlying(thread, index_obj));
  if (index_int.isLargeInt()) {
    return thread->raiseWithFmt(LayoutId::kOverflowError,
                                "Python int too large to convert to C ssize_t");
  }
  word index = index_int.asWord();
  Object value(&scope, args.get(2));
  listInsert(thread, list, value, index);
  return NoneType::object();
}

RawObject ListBuiltins::dunderMul(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  RawObject other = args.get(1);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfList(*self)) {
    return thread->raiseRequiresType(self, SymbolId::kList);
  }
  if (other.isSmallInt()) {
    word ntimes = SmallInt::cast(other).value();
    List list(&scope, *self);
    return listReplicate(thread, list, ntimes);
  }
  return thread->raiseWithFmt(LayoutId::kTypeError,
                              "can't multiply list by non-int");
}

RawObject ListBuiltins::pop(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  if (!args.get(1).isUnbound() && !args.get(1).isSmallInt()) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "index object cannot be interpreted as an integer");
  }

  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfList(*self)) {
    return thread->raiseRequiresType(self, SymbolId::kList);
  }
  List list(&scope, *self);
  word length = list.numItems();
  if (length == 0) {
    return thread->raiseWithFmt(LayoutId::kIndexError, "pop from empty list");
  }
  word index = length - 1;
  if (!args.get(1).isUnbound()) {
    index = SmallInt::cast(args.get(1)).value();
    if (index < 0) index += length;
  }
  if (index < 0 || index >= length) {
    return thread->raiseWithFmt(LayoutId::kIndexError,
                                "pop index out of range");
  }

  return listPop(thread, list, index);
}

RawObject ListBuiltins::remove(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfList(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kList);
  }
  Object value(&scope, args.get(1));
  List self(&scope, *self_obj);
  Object item(&scope, NoneType::object());
  Object comp_result(&scope, NoneType::object());
  Object found(&scope, NoneType::object());
  for (word i = 0, num_items = self.numItems(); i < num_items; ++i) {
    item = self.at(i);
    if (*value == *item) {
      listPop(thread, self, i);
      return NoneType::object();
    }
    comp_result = Interpreter::compareOperation(thread, frame, CompareOp::EQ,
                                                item, value);
    if (comp_result.isError()) return *comp_result;
    found = Interpreter::isTrue(thread, *comp_result);
    if (found.isError()) return *found;
    if (found == Bool::trueObj()) {
      listPop(thread, self, i);
      return NoneType::object();
    }
  }
  return thread->raiseWithFmt(LayoutId::kValueError,
                              "list.remove(x) x not in list");
}

RawObject ListBuiltins::dunderIter(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfList(*self)) {
    return thread->raiseRequiresType(self, SymbolId::kList);
  }
  return thread->runtime()->newListIterator(self);
}

const BuiltinMethod ListIteratorBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderIter, dunderIter},
    {SymbolId::kDunderLengthHint, dunderLengthHint},
    {SymbolId::kDunderNext, dunderNext},
    {SymbolId::kSentinelId, nullptr},
};

RawObject ListIteratorBuiltins::dunderIter(Thread* thread, Frame* frame,
                                           word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isListIterator()) {
    return thread->raiseRequiresType(self, SymbolId::kListIterator);
  }
  return *self;
}

RawObject ListIteratorBuiltins::dunderNext(Thread* thread, Frame* frame,
                                           word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isListIterator()) {
    return thread->raiseRequiresType(self_obj, SymbolId::kListIterator);
  }
  ListIterator self(&scope, *self_obj);
  Object value(&scope, listIteratorNext(thread, self));
  if (value.isError()) {
    return thread->raise(LayoutId::kStopIteration, NoneType::object());
  }
  return *value;
}

RawObject ListIteratorBuiltins::dunderLengthHint(Thread* thread, Frame* frame,
                                                 word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isListIterator()) {
    return thread->raiseRequiresType(self, SymbolId::kListIterator);
  }
  ListIterator list_iterator(&scope, *self);
  List list(&scope, list_iterator.iterable());
  return SmallInt::fromWord(list.numItems() - list_iterator.index());
}

static RawObject setItemSlice(Thread* thread, const List& list,
                              const Slice& slice, const Object& src) {
  HandleScope scope(thread);
  word start, stop, step;
  Object err(&scope, sliceUnpack(thread, slice, &start, &stop, &step));
  if (err.isError()) return *err;
  word slice_length =
      Slice::adjustIndices(list.numItems(), &start, &stop, step);
  word suffix_start = Utils::maximum(start + 1, stop);

  // degenerate case of a bad slice; behaves like a single index:
  if ((step >= 0) && (start > stop)) {
    stop = start;
    suffix_start = start;  // do not drop any items
  }

  // Potential optimization:
  // We could check for __length_hint__ or __len__ and attempt to
  // do in-place update or at least have a good estimate of
  // capacity for result_list
  Runtime* runtime = thread->runtime();
  List result_list(&scope, runtime->newList());

  // Note: initial capacity for result array here is an optimistic guess
  // since we don't yet know length iterator
  // Potential optimization: use __length_hint__ of src
  runtime->listEnsureCapacity(thread, result_list, list.numItems());

  // copy prefix into result array:
  word prefix_length = Utils::minimum(start, stop + 1);
  result_list.setNumItems(prefix_length);
  for (word i = 0; i < prefix_length; i++) {
    result_list.atPut(i, list.at(i));
  }
  if (step == 1) {
    if (src.isList() || src.isTuple()) {
      // Fast path for exact lists or tuples
      Object result(&scope, listExtend(thread, result_list, src));
      if (result.isError()) return *result;
    } else {
      // Subclasses of list or tuple or other iterables fall back to
      // list.extend
      Object result(&scope,
                    thread->invokeMethodStatic2(
                        LayoutId::kList, SymbolId::kExtend, result_list, src));
      if (result.isError()) return *result;
    }
  } else {
    Object iter_method(
        &scope, Interpreter::lookupMethod(thread, thread->currentFrame(), src,
                                          SymbolId::kDunderIter));
    if (iter_method.isError()) {
      return thread->raiseWithFmt(LayoutId::kTypeError,
                                  "object is not iterable");
    }
    Object iterator(&scope,
                    Interpreter::callMethod1(thread, thread->currentFrame(),
                                             iter_method, src));
    if (iterator.isError()) {
      return thread->raiseWithFmt(LayoutId::kTypeError,
                                  "object is not iterable");
    }
    Object next_method(
        &scope, Interpreter::lookupMethod(thread, thread->currentFrame(),
                                          iterator, SymbolId::kDunderNext));
    if (next_method.isError()) {
      return thread->raiseWithFmt(LayoutId::kTypeError,
                                  "iter() returned a non-iterator");
    }

    Object iter_length_val(&scope,
                           runtime->iteratorLengthHint(thread, iterator));
    if (iter_length_val.isError()) {
      return thread->raiseWithFmt(
          LayoutId::kTypeError,
          "slice assignment: unable to get length of assigned sequence");
    }
    word iter_length = SmallInt::cast(*iter_length_val).value();
    if (slice_length != iter_length) {
      return thread->raiseWithFmt(
          LayoutId::kValueError,
          "attempt to assign sequence of size %w to extended slice of size "
          "%w",
          iter_length, slice_length);
    }

    Object value(&scope, NoneType::object());

    result_list.setNumItems(result_list.numItems() + std::abs(stop - start));
    for (word i = start, incr = (step > 0) ? 1 : -1, count = 0; i != stop;
         i += incr, count++) {
      if ((count % step) == 0) {
        value = Interpreter::callMethod1(thread, thread->currentFrame(),
                                         next_method, iterator);
        if (value.isError()) {
          return *value;
        }
      } else {
        value = list.at(i);
      }
      result_list.atPut(i, *value);
    }
  }

  // And now copy suffix:
  word suffix_length = list.numItems() - suffix_start;
  word result_length = result_list.numItems() + suffix_length;
  runtime->listEnsureCapacity(thread, result_list, result_length);

  word index = result_list.numItems();
  result_list.setNumItems(result_length);
  for (word i = suffix_start; i < list.numItems(); i++, index++) {
    result_list.atPut(index, list.at(i));
  }

  list.setItems(result_list.items());
  list.setNumItems(result_list.numItems());
  return NoneType::object();
}

RawObject ListBuiltins::dunderSetItem(Thread* thread, Frame* frame,
                                      word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));

  if (!thread->runtime()->isInstanceOfList(*self)) {
    return thread->raiseRequiresType(self, SymbolId::kList);
  }

  List list(&scope, *self);
  Object index(&scope, args.get(1));
  Object src(&scope, args.get(2));

  if (index.isSmallInt()) {
    word idx = SmallInt::cast(*index).value();
    word length = list.numItems();
    if (idx < 0) {
      idx += length;
    }
    if (idx < 0 || idx >= length) {
      return thread->raiseWithFmt(LayoutId::kIndexError,
                                  "list assignment index out of range");
    }
    list.atPut(idx, *src);
    return NoneType::object();
  }
  if (index.isSlice()) {
    Slice slice(&scope, *index);
    return setItemSlice(thread, list, slice, src);
  }
  return thread->raiseWithFmt(LayoutId::kTypeError,
                              "list indices must be integers or slices");
}

}  // namespace python
