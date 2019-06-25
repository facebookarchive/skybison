#include "mro.h"
#include "runtime.h"

namespace python {

// Fills merge_lists with the MROs of the parents, followed by the parents list.
static word populateMergeLists(Thread* thread, const Tuple& merge_lists,
                               const Tuple& parents,
                               Vector<word>* merge_list_indices /* out */) {
  word num_parents = parents.length();
  DCHECK(merge_lists.length() == num_parents + 1,
         "merge the linearizations of each parent with the parent list");
  HandleScope scope(thread);
  word new_mro_length = 2;  // C + ... + object
  for (word i = 0; i < num_parents; i++) {
    Type parent_class(&scope, parents.at(i));      // B_i
    Tuple parent_mro(&scope, parent_class.mro());  // L[B_i]

    new_mro_length += parent_mro.length();
    merge_lists.atPut(i, *parent_mro);
    merge_list_indices->push_back(0);
  }
  merge_lists.atPut(num_parents, *parents);  // B_1 B_2 ... B_n
  merge_list_indices->push_back(0);
  return new_mro_length - num_parents;  // all parent MROs end with object
}

// Returns true if there is an i such that mro->at(i) == cls, i > head_idx.
static bool tailContains(const Tuple& mro, const Object& cls, word head_idx) {
  for (word i = head_idx + 1, length = mro.length(); i < length; i++) {
    if (mro.at(i) == *cls) {
      return true;
    }
  }
  return false;
}

// Looks for a head class in merge_lists (i.e. the class indicated by the
// corresponding index in merge_list_indices) which does not appear in any of
// the merge_lists at a position *after* the head class of that list.
static RawObject findNext(Thread* thread, const Tuple& merge_lists,
                          const Vector<word>& merge_list_indices) {
  HandleScope scope(thread);
  Object candidate_head(&scope, Unbound::object());
  for (word i = 0, length = merge_lists.length(); i < length; i++) {
    word cur_idx = merge_list_indices[i];
    Tuple cur_mro(&scope, merge_lists.at(i));

    if (cur_idx >= cur_mro.length()) {
      continue;
    }

    candidate_head = cur_mro.at(cur_idx);
    for (word j = 0; j < length; j++) {
      if (j == i) {
        continue;
      }
      Tuple other_mro(&scope, merge_lists.at(j));
      if (tailContains(other_mro, candidate_head, merge_list_indices[j])) {
        candidate_head = Error::notFound();
        break;
      }
    }
    if (!candidate_head.isError()) {
      return *candidate_head;
    }
  }
  return Error::notFound();
}

RawObject computeMro(Thread* thread, const Type& type, const Tuple& parents) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();

  // Special case for no explicit ancestors.
  word num_parents = parents.length();
  if (num_parents == 0) {
    Tuple new_mro(&scope, runtime->newTuple(2));
    new_mro.atPut(0, *type);
    new_mro.atPut(1, runtime->typeAt(LayoutId::kObject));
    return *new_mro;
  }

  Vector<word> merge_list_indices;
  word merge_list_length = num_parents + 1;
  merge_list_indices.reserve(merge_list_length);
  Tuple merge_lists(&scope, runtime->newTuple(merge_list_length));
  word new_mro_length =
      populateMergeLists(thread, merge_lists, parents, &merge_list_indices);
  DCHECK(merge_list_indices.size() == merge_list_length,
         "expected %ld indices, got %ld", merge_list_length,
         merge_list_indices.size());

  // The length of new_mro will be longer than necessary when there is overlap
  // between the MROs of the parents.
  Tuple new_mro(&scope, runtime->newTuple(new_mro_length));

  word next_idx = 0;
  new_mro.atPut(next_idx, *type);
  next_idx++;

  // To compute the MRO, repeatedly look for a head of one or more
  // MROs, which is not in the tail of any other MROs, and consume
  // all matching heads. This is O(n^2) as implemented, but so is
  // CPython's implementation, so we can rest assured no real program
  // is going to cause a major problem here.
  RawObject next_head = Error::notFound();
  while (!(next_head = findNext(thread, merge_lists, merge_list_indices))
              .isError()) {
    Type next_head_cls(&scope, next_head);
    for (word i = 0; i < merge_list_length; i++) {
      auto& cur_idx = merge_list_indices[i];
      Tuple cur_mro(&scope, merge_lists.at(i));
      if (cur_idx < cur_mro.length() && cur_mro.at(cur_idx) == *next_head_cls) {
        cur_idx++;
      }
    }
    new_mro.atPut(next_idx, *next_head_cls);
    next_idx++;
  }

  for (word i = 0; i < merge_list_length; i++) {
    if (merge_list_indices[i] != Tuple::cast(merge_lists.at(i)).length()) {
      Tuple names(&scope, runtime->newTuple(num_parents));
      Type parent(&scope, runtime->typeAt(LayoutId::kNoneType));
      for (word j = 0; j < num_parents; j++) {
        parent = parents.at(j);
        names.atPut(j, parent.name());
      }
      Str sep(&scope, SmallStr::fromCStr(", "));
      Object bases(&scope, runtime->strJoin(thread, sep, names, num_parents));
      return thread->raiseWithFmt(LayoutId::kTypeError,
                                  "Cannot create a consistent method "
                                  "resolution order (MRO) for bases %S",
                                  &bases);
    }
  }

  // Copy the mro to an array of exact size. (new_mro_length is an upper bound).
  return runtime->tupleSubseq(thread, new_mro, 0, next_idx);
}

}  // namespace python
