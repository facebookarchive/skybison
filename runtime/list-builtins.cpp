#include "list-builtins.h"

#include "frame.h"
#include "globals.h"
#include "int-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "slice-builtins.h"
#include "thread.h"
#include "tuple-builtins.h"

namespace py {

RawObject listExtend(Thread* thread, const List& dst, const Object& iterable) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Tuple src_tuple(&scope, runtime->emptyTuple());
  word src_length = 0;
  DCHECK(runtime->isInstanceOfList(*iterable) ||
             runtime->isInstanceOfTuple(*iterable),
         "iterable must be list or tuple instance");
  if (runtime->isInstanceOfList(*iterable)) {
    List src(&scope, *iterable);
    src_tuple = src.items();
    src_length = src.numItems();
  } else {
    src_tuple = tupleUnderlying(*iterable);
    src_length = src_tuple.length();
  }
  word old_length = dst.numItems();
  word new_length = old_length + src_length;
  if (new_length != old_length) {
    runtime->listEnsureCapacity(thread, dst, new_length);
    dst.setNumItems(new_length);
    MutableTuple dst_items(&scope, dst.items());
    dst_items.replaceFromWith(old_length, *src_tuple, src_length);
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
  // Shift elements over to the right
  list.replaceFromWithStartAt(index + 1, *list, last_index - index, index);
  list.atPut(index, *value);
}

RawObject listPop(Thread* thread, const List& list, word index) {
  HandleScope scope(thread);
  Object popped(&scope, list.at(index));
  list.atPut(index, NoneType::object());
  word last_index = list.numItems() - 1;
  list.replaceFromWithStartAt(index, *list, last_index - index, index + 1);
  list.setNumItems(list.numItems() - 1);
  return *popped;
}

RawObject listReplicate(Thread* thread, const List& list, word ntimes) {
  Runtime* runtime = thread->runtime();
  word len = list.numItems();
  word result_len = ntimes * len;
  if (result_len == 0) {
    return runtime->newList();
  }
  HandleScope scope(thread);
  Tuple list_items(&scope, list.items());
  MutableTuple items(&scope, runtime->newMutableTuple(result_len));
  for (word i = 0; i < result_len; i += len) {
    items.replaceFromWith(i, *list_items, len);
  }
  List result(&scope, runtime->newList());
  result.setItems(*items);
  result.setNumItems(result_len);
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
  MutableTuple items(&scope, runtime->newMutableTuple(length));
  Tuple src(&scope, list.items());
  for (word i = 0, j = start; i < length; i++, j += step) {
    items.atPut(i, src.at(j));
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
      compare_result = thread->invokeFunction2(
          SymbolId::kUnderBuiltins, SymbolId::kUnderLt, list_i, list_j);
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
    {SymbolId::kDunderImul, dunderImul},
    {SymbolId::kDunderIter, dunderIter},
    {SymbolId::kDunderLen, dunderLen},
    {SymbolId::kDunderMul, dunderMul},
    {SymbolId::kAppend, append},
    {SymbolId::kClear, clear},
    {SymbolId::kInsert, insert},
    {SymbolId::kPop, pop},
    {SymbolId::kRemove, remove},
    {SymbolId::kSentinelId, nullptr},
};

RawObject ListBuiltins::dunderNew(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object type_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfType(*type_obj)) {
    return thread->raiseWithFmt(LayoutId::kTypeError, "not a type object");
  }
  Type type(&scope, *type_obj);
  if (type.builtinBase() != LayoutId::kList) {
    return thread->raiseWithFmt(LayoutId::kTypeError, "not a subtype of list");
  }
  Layout layout(&scope, type.instanceLayout());
  List result(&scope, runtime->newInstance(layout));
  result.setNumItems(0);
  result.setItems(runtime->emptyTuple());
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
  for (word i = 0, num_items = self.numItems(); i < num_items; ++i) {
    RawObject eq = Runtime::objectEquals(thread, *value, self.at(i));
    if (eq != Bool::falseObj()) return eq;
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
  Int index_int(&scope, intUnderlying(*index_obj));
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
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfList(*self)) {
    return thread->raiseRequiresType(self, SymbolId::kList);
  }
  if (other.isSmallInt()) {
    word ntimes = SmallInt::cast(other).value();
    if (ntimes <= 0) return runtime->newList();
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

RawObject ListBuiltins::dunderImul(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  RawObject other = args.get(1);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfList(*self)) {
    return thread->raiseRequiresType(self, SymbolId::kList);
  }
  Object count_index(&scope, args.get(1));
  Object count_obj(&scope, intFromIndex(thread, count_index));
  if (count_obj.isError()) return *count_obj;
  word count = intUnderlying(*count_obj).asWordSaturated();
  if (!SmallInt::isValid(count)) {
    return thread->raiseWithFmt(LayoutId::kOverflowError,
                                "cannot fit '%T' into an index-sized integer",
                                &count_index);
  }
  word ntimes = SmallInt::cast(other).value();
  if (ntimes == 1) {
    return *self;
  }
  List list(&scope, *self);
  if (ntimes <= 0) {
    list.clearFrom(0);
    return *list;
  }
  word new_length;
  word len = list.numItems();
  if (__builtin_mul_overflow(len, ntimes, &new_length) ||
      !SmallInt::isValid(new_length)) {
    return thread->raiseMemoryError();
  }
  if (new_length == len) {
    return *list;
  }
  runtime->listEnsureCapacity(thread, list, new_length);
  list.setNumItems(new_length);
  for (word i = 0; i < ntimes; i++) {
    list.replaceFromWith(i * len, *list, len);
  }
  return *list;
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

}  // namespace py
