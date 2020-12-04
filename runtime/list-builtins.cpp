#include "list-builtins.h"

#include "builtins.h"
#include "frame.h"
#include "globals.h"
#include "int-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "slice-builtins.h"
#include "thread.h"
#include "tuple-builtins.h"
#include "type-builtins.h"

namespace py {

void listExtend(Thread* thread, const List& dst, const Tuple& src,
                word src_length) {
  if (src_length == 0) {
    return;
  }

  word old_length = dst.numItems();
  word new_length = old_length + src_length;
  thread->runtime()->listEnsureCapacity(thread, dst, new_length);
  dst.setNumItems(new_length);
  MutableTuple::cast(dst.items()).replaceFromWith(old_length, *src, src_length);
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
  if (index < last_index) {
    list.replaceFromWithStartAt(index, *list, last_index - index, index + 1);
  }
  list.setNumItems(last_index);
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

// TODO(T63900795): Investigate this threshold on a realistic benchmark.
static const int kListInsertionSortSize = 8;

static RawObject objectLessThan(Thread* thread, const Object& left,
                                const Object& right,
                                const Object& compare_func) {
  if (left.isSmallInt() && right.isSmallInt()) {
    return Bool::fromBool(SmallInt::cast(*left).value() <
                          SmallInt::cast(*right).value());
  }
  if (left.isStr() && right.isStr()) {
    return Bool::fromBool(Str::cast(*left).compare(Str::cast(*right)) < 0);
  }
  return Interpreter::call2(thread, compare_func, left, right);
}

static RawObject listInsertionSort(Thread* thread, const MutableTuple& data,
                                   const Object& compare_func, word start,
                                   word end) {
  HandleScope scope(thread);
  Object item_i(&scope, NoneType::object());
  Object item_j(&scope, NoneType::object());
  Object compare_result(&scope, NoneType::object());
  for (word i = start + 1; i < end; i++) {
    item_i = data.at(i);
    word j = i - 1;
    for (; j >= start; j--) {
      item_j = data.at(j);
      compare_result = objectLessThan(thread, item_i, item_j, compare_func);
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
      data.atPut(j + 1, *item_j);
    }
    data.atPut(j + 1, *item_i);
  }
  return NoneType::object();
}

// Merge 2 sublists, input[left: right], input[right: end] into
// output[left: end].
static RawObject listMerge(Thread* thread, const MutableTuple& input,
                           const MutableTuple& output,
                           const Object& compare_func, word left, word right,
                           word end) {
  HandleScope scope(thread);
  Object item_i(&scope, NoneType::object());
  Object item_j(&scope, NoneType::object());
  Object compare_result(&scope, NoneType::object());
  word i = left;
  word j = right;
  word k = left;
  while (i < right && j < end) {
    item_i = input.at(i);
    item_j = input.at(j);
    compare_result = objectLessThan(thread, item_j, item_i, compare_func);
    if (compare_result.isError()) {
      return *compare_result;
    }
    compare_result = Interpreter::isTrue(thread, *compare_result);
    if (compare_result.isError()) {
      return *compare_result;
    }
    if (*compare_result == Bool::trueObj()) {
      output.atPut(k++, *item_j);
      j++;
    } else {
      DCHECK(compare_result == Bool::falseObj(), "expected to be false");
      output.atPut(k++, *item_i);
      i++;
    }
  }
  for (; i < right; i++) {
    output.atPut(k++, input.at(i));
  }
  for (; j < end; j++) {
    output.atPut(k++, input.at(j));
  }
  DCHECK(k == end, "sublists were not fully copied");
  return NoneType::object();
}

RawObject listSort(Thread* thread, const List& list) {
  return listSortWithCompareMethod(thread, list, ID(_lt));
}

// This uses an adaptation of merge sort. It sorts sublists of size
// kListInsertionSortSize or less using insertion sort first. Afterwards, it is
// using a bottom-up merge sort to increase the sublists' size by power of 2.
// Also, this function allocates a temporary space to ease merging and swaps
// `input` and `output` to avoid further allocation.
// TODO(T39107329): Consider using Timsort for further optimization.
RawObject listSortWithCompareMethod(Thread* thread, const List& list,
                                    SymbolId compare_method) {
  word num_items = list.numItems();
  if (num_items == 0) {
    return NoneType::object();
  }
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object compare_func(&scope, runtime->lookupNameInModule(thread, ID(_builtins),
                                                          compare_method));
  if (compare_func.isError()) return *compare_func;
  MutableTuple input(&scope, list.items());
  Object compare_result(&scope, NoneType::object());
  for (word left = 0; left < num_items; left += kListInsertionSortSize) {
    word right = Utils::minimum(left + kListInsertionSortSize, num_items);
    compare_result =
        listInsertionSort(thread, input, compare_func, left, right);
    if (compare_result.isError()) {
      return *compare_result;
    }
  }
  if (num_items <= kListInsertionSortSize) {
    // The input list is small enough to be sorted by insertion sort.
    return NoneType::object();
  }
  MutableTuple output(&scope, runtime->newMutableTuple(input.length()));
  Object tmp(&scope, NoneType::object());
  for (word width = kListInsertionSortSize; width < num_items; width *= 2) {
    for (word left = 0; left < num_items; left += width * 2) {
      word right = Utils::minimum(num_items, left + width);
      word end = Utils::minimum(num_items, left + width * 2);
      compare_result =
          listMerge(thread, input, output, compare_func, left, right, end);
      if (compare_result.isError()) {
        return *compare_result;
      }
    }
    tmp = *output;
    output = *input;
    input = MutableTuple::cast(*tmp);
  }
  list.setItems(*input);
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

static const BuiltinAttribute kListAttributes[] = {
    {ID(_list__items), RawList::kItemsOffset, AttributeFlags::kHidden},
    {ID(_list__num_items), RawList::kNumItemsOffset, AttributeFlags::kHidden},
};

static const BuiltinAttribute kListIteratorAttributes[] = {
    {ID(_list_iterator__iterable), RawListIterator::kIterableOffset,
     AttributeFlags::kHidden},
    {ID(_list_iterator__index), RawListIterator::kIndexOffset,
     AttributeFlags::kHidden},
};

void initializeListTypes(Thread* thread) {
  addBuiltinType(thread, ID(list), LayoutId::kList,
                 /*superclass_id=*/LayoutId::kObject, kListAttributes,
                 List::kSize, /*basetype=*/true);

  addBuiltinType(thread, ID(list_iterator), LayoutId::kListIterator,
                 /*superclass_id=*/LayoutId::kObject, kListIteratorAttributes,
                 ListIterator::kSize, /*basetype=*/false);
}

RawObject METH(list, __new__)(Thread* thread, Arguments args) {
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

RawObject METH(list, __add__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfList(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(list));
  }
  Object other_obj(&scope, args.get(1));
  if (!runtime->isInstanceOfList(*other_obj)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "can only concatenate list to list");
  }
  List self(&scope, *self_obj);
  List other(&scope, *other_obj);
  List new_list(&scope, runtime->newList());
  word self_len = self.numItems();
  word other_len = other.numItems();
  runtime->listEnsureCapacity(thread, new_list, self_len + other_len);
  Tuple self_items(&scope, self.items());
  Tuple other_items(&scope, other.items());
  listExtend(thread, new_list, self_items, self_len);
  listExtend(thread, new_list, other_items, other_len);
  return *new_list;
}

RawObject listContains(Thread* thread, const List& list, const Object& key) {
  for (word i = 0, num_items = list.numItems(); i < num_items; ++i) {
    RawObject result = Runtime::objectEquals(thread, *key, list.at(i));
    if (result == Bool::trueObj()) {
      return Bool::trueObj();
    }
    if (result.isErrorException()) return result;
  }
  return Bool::falseObj();
}

RawObject METH(list, __contains__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfList(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(list));
  }
  List self(&scope, *self_obj);
  Object key(&scope, args.get(1));
  return listContains(thread, self, key);
}

RawObject METH(list, __eq__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfList(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(list));
  }
  Object other_obj(&scope, args.get(1));
  if (!runtime->isInstanceOfList(*other_obj)) {
    return NotImplementedType::object();
  }
  if (self_obj == other_obj) {
    return Bool::trueObj();
  }
  List self(&scope, *self_obj);
  List other(&scope, *other_obj);
  word num_items = self.numItems();
  if (num_items != other.numItems()) {
    return Bool::falseObj();
  }
  Tuple self_items(&scope, self.items());
  Tuple other_items(&scope, other.items());
  for (word i = 0; i < num_items; i++) {
    RawObject self_item = self_items.at(i);
    RawObject other_item = other_items.at(i);
    if (self_item != other_item) {
      RawObject equals = Runtime::objectEquals(thread, self_item, other_item);
      if (equals != Bool::trueObj()) {
        return equals;
      }
    }
  }
  return Bool::trueObj();
}

RawObject METH(list, clear)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfList(*self)) {
    return thread->raiseRequiresType(self, ID(list));
  }
  List list(&scope, *self);
  list.clearFrom(0);
  return NoneType::object();
}

RawObject METH(list, __len__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfList(*self)) {
    return thread->raiseRequiresType(self, ID(list));
  }
  List list(&scope, *self);
  return SmallInt::fromWord(list.numItems());
}

RawObject METH(list, insert)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfList(*self)) {
    return thread->raiseRequiresType(self, ID(list));
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

RawObject METH(list, __mul__)(Thread* thread, Arguments args) {
  RawObject other = args.get(1);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfList(*self)) {
    return thread->raiseRequiresType(self, ID(list));
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

RawObject METH(list, pop)(Thread* thread, Arguments args) {
  if (!args.get(1).isUnbound() && !args.get(1).isSmallInt()) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "index object cannot be interpreted as an integer");
  }

  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfList(*self)) {
    return thread->raiseRequiresType(self, ID(list));
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

RawObject METH(list, remove)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfList(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(list));
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
    comp_result =
        Interpreter::compareOperation(thread, CompareOp::EQ, item, value);
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

RawObject METH(list, __imul__)(Thread* thread, Arguments args) {
  RawObject other = args.get(1);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfList(*self)) {
    return thread->raiseRequiresType(self, ID(list));
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

RawObject METH(list, __iter__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfList(*self)) {
    return thread->raiseRequiresType(self, ID(list));
  }
  return thread->runtime()->newListIterator(self);
}

RawObject METH(list_iterator, __iter__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isListIterator()) {
    return thread->raiseRequiresType(self, ID(list_iterator));
  }
  return *self;
}

RawObject METH(list_iterator, __next__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isListIterator()) {
    return thread->raiseRequiresType(self_obj, ID(list_iterator));
  }
  ListIterator self(&scope, *self_obj);
  Object value(&scope, listIteratorNext(thread, self));
  if (value.isError()) {
    return thread->raise(LayoutId::kStopIteration, NoneType::object());
  }
  return *value;
}

RawObject METH(list_iterator, __length_hint__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isListIterator()) {
    return thread->raiseRequiresType(self, ID(list_iterator));
  }
  ListIterator list_iterator(&scope, *self);
  List list(&scope, list_iterator.iterable());
  return SmallInt::fromWord(list.numItems() - list_iterator.index());
}

}  // namespace py
