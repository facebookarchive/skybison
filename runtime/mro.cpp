#include "mro.h"
#include "runtime.h"

namespace python {

static word populateMergeLists(const Handle<ObjectArray>& merge_lists,
                               const Handle<ObjectArray>& parents,
                               Vector<word>* merge_list_indices /* out */) {
  HandleScope scope;
  // MROs contain at least the class itself, and Object.
  word new_mro_length = 2;
  for (word i = 0; i < parents->length(); i++) {
    Handle<Type> parent_class(&scope, parents->at(i));
    Handle<ObjectArray> parent_mro(&scope, parent_class->mro());

    new_mro_length += parent_mro->length();
    merge_lists->atPut(i, *parent_mro);
    merge_list_indices->push_back(0);
  }
  merge_lists->atPut(parents->length(), *parents);
  merge_list_indices->push_back(0);
  new_mro_length -= parents->length();  // all parent MROs end with Object.
  return new_mro_length;
}

// Returns true if there is an i such that mro->at(i) == cls, i > head_idx.
static bool tailContains(const Handle<ObjectArray>& mro,
                         const Handle<Object>& cls, word head_idx) {
  auto const len = mro->length();
  if (head_idx >= len) {
    return false;
  }
  for (word i = head_idx + 1; i < len; i++) {
    if (Type::cast(mro->at(i)) == *cls) {
      return true;
    }
  }
  return false;
}

// Looks for a head class in merge_lists (i.e. the class indicated by the
// corresponding index in merge_list_indices) which does not appear in any of
// the merge_lists at a position *after* the head class of that list.
static RawObject findNext(const Handle<ObjectArray>& merge_lists,
                          const Vector<word>& merge_list_indices) {
  HandleScope scope;
  for (word i = 0; i < merge_list_indices.size(); i++) {
    auto cur_idx = merge_list_indices[i];
    Handle<ObjectArray> cur_mro(&scope, merge_lists->at(i));

    if (cur_idx >= cur_mro->length()) {
      continue;
    }

    Handle<Object> candidate_head(&scope, cur_mro->at(cur_idx));

    for (word j = 0; j < merge_list_indices.size(); j++) {
      if (j == i) {
        continue;
      }
      Handle<ObjectArray> other_mro(&scope, merge_lists->at(j));
      if (tailContains(other_mro, candidate_head, merge_list_indices[j])) {
        candidate_head = Error::object();
        break;
      }
    }
    if (!candidate_head->isError()) {
      return *candidate_head;
    }
  }
  return Error::object();
}

RawObject computeMro(Thread* thread, const Handle<Type>& type,
                     const Handle<ObjectArray>& parents) {
  Runtime* runtime = thread->runtime();
  HandleScope scope;

  Handle<Object> object_class(
      &scope,
      Layout::cast(runtime->layoutAt(LayoutId::kObject))->describedClass());

  // Special case for no explicit ancestors.
  if (parents->length() == 0) {
    Handle<ObjectArray> new_mro(&scope, runtime->newObjectArray(2));
    new_mro->atPut(0, *type);
    new_mro->atPut(1, *object_class);
    return *new_mro;
  }

  Vector<word> merge_list_indices;
  Handle<ObjectArray> merge_lists(
      &scope, runtime->newObjectArray(parents->length() + 1));
  word new_mro_length =
      populateMergeLists(merge_lists, parents, &merge_list_indices);

  // The length of new_mro will be longer than necessary when there is overlap
  // between the MROs of the parents.
  Handle<ObjectArray> new_mro(&scope, runtime->newObjectArray(new_mro_length));

  word next_idx = 0;
  new_mro->atPut(next_idx, *type);
  next_idx++;

  // To compute the MRO, repeatedly look for a head of one or more
  // MROs, which is not in the tail of any other MROs, and consume
  // all matching heads. This is O(n^2) as implemented, but so is
  // CPython's implementation, so we can rest assured no real program
  // is going to cause a major problem here.
  RawObject next_head = Error::object();
  while (!(next_head = findNext(merge_lists, merge_list_indices))->isError()) {
    Handle<Type> next_head_cls(&scope, next_head);
    for (word i = 0; i < merge_list_indices.size(); i++) {
      auto& cur_idx = merge_list_indices[i];
      Handle<ObjectArray> cur_mro(&scope, merge_lists->at(i));
      if (cur_idx < cur_mro->length() &&
          Type::cast(cur_mro->at(cur_idx)) == *next_head_cls) {
        cur_idx++;
      }
    }
    new_mro->atPut(next_idx, *next_head_cls);
    next_idx++;
  }

  for (word i = 0; i < merge_list_indices.size(); i++) {
    if (merge_list_indices[i] !=
        ObjectArray::cast(merge_lists->at(i))->length()) {
      // TODO: list bases in error message.
      return thread->raiseTypeErrorWithCStr(
          "Cannot create a consistent method resolution order (MRO)");
    }
  }

  // Copy the mro to an array of exact size. (new_mro_length is an upper bound).
  Handle<ObjectArray> ret_mro(&scope, runtime->newObjectArray(next_idx));
  for (word i = 0; i < next_idx; i++) {
    ret_mro->atPut(i, new_mro->at(i));
  }

  return *ret_mro;
}

}  // namespace python
