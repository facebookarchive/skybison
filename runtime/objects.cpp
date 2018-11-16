#include "objects.h"
#include "runtime.h"

#include <cassert>
#include <string>

namespace python {

// List

int List::allocationSize() {
  return Utils::roundUp(List::kSize, kPointerSize);
}

List* List::cast(Object* obj) {
  assert(obj->isList());
  return reinterpret_cast<List*>(obj);
}

void List::initialize(Object* elements) {
  atPut(List::kElemsOffset, elements);
}

ObjectArray* List::elems() {
  Object* e = at(kElemsOffset);
  if (e == nullptr) {
    return nullptr;
  }
  return ObjectArray::cast(e);
}

void List::setElems(ObjectArray* elems) {
  atPut(kElemsOffset, elems);
}

word List::capacity() {
  ObjectArray* es = elems();
  if (es == nullptr) {
    return 0;
  }
  return es->length();
}

void List::set(int index, Object* value) {
  assert(index >= 0);
  assert(index < length());
  Object* elems = at(kElemsOffset);
  ObjectArray::cast(elems)->set(index, value);
}

Object* List::get(int index) {
  return elems()->get(index);
}

void List::appendAndGrow(
    const Handle<List>& list,
    const Handle<Object>& value,
    Runtime* runtime) {
  word len = list->length();
  word cap = list->capacity();
  if (len < cap) {
    list->setLength(len + 1);
    list->set(len, *value);
    return;
  }
  intptr_t newCap = cap == 0 ? 4 : cap << 1;
  Object* rawNewElems = runtime->createObjectArray(newCap);
  ObjectArray* newElems = ObjectArray::cast(rawNewElems);
  ObjectArray* curElems = list->elems();
  if (curElems != nullptr) {
    curElems->copyTo(newElems);
  }

  list->setElems(newElems);
  list->setLength(len + 1);
  list->set(len, *value);
}

// Dictionary
// TODO(mpage): Needs handlizing

// Helper class for working with entries in the backing ObjectArray.
class DictItem {
 public:
  DictItem(ObjectArray* items, int bucket) : items_(items), bucket_(bucket){};

  inline Object* hash() {
    return items_->get(bucket_ + Dictionary::kBucketHashOffset);
  }

  inline Object* key() {
    return items_->get(bucket_ + Dictionary::kBucketKeyOffset);
  }

  inline Object* value() {
    return items_->get(bucket_ + Dictionary::kBucketValueOffset);
  }

  inline bool matches(Object* that) {
    return !hash()->isNone() && Object::equals(key(), that);
  }

  inline void set(Object* hash, Object* key, Object* value) {
    items_->set(bucket_ + Dictionary::kBucketHashOffset, hash);
    items_->set(bucket_ + Dictionary::kBucketKeyOffset, key);
    items_->set(bucket_ + Dictionary::kBucketValueOffset, value);
  }

  inline bool isTombstone() {
    return hash()->isNone() && !key()->isNone();
  }

  inline void setTombstone() {
    set(None::object(), Ellipsis::object(), None::object());
  }

  inline bool isEmpty() {
    return hash()->isNone() && key()->isNone();
  }

  inline void setEmpty() {
    set(None::object(), None::object(), None::object());
  }

  ObjectArray* items_;
  int bucket_;
};

void Dictionary::setNumItems(word numItems) {
  atPut(kNumItemsOffset, SmallInteger::fromWord(numItems));
}

ObjectArray* Dictionary::items() {
  return ObjectArray::cast(at(kItemsOffset));
}

void Dictionary::setItems(ObjectArray* items) {
  assert((items->length() % kPointersPerBucket) == 0);
  assert(Utils::isPowerOfTwo(items->length() / kPointersPerBucket));
  atPut(kItemsOffset, items);
}

bool Dictionary::itemAt(
    Object* dictObj,
    Object* key,
    Object* hash,
    Object** value) {
  Dictionary* dict = Dictionary::cast(dictObj);
  int bucket = -1;
  bool found = lookup(dict, key, hash, &bucket);
  if (found) {
    assert(bucket != -1);
    DictItem item(dict->items(), bucket);
    *value = item.value();
  }
  return found;
}

void Dictionary::itemAtPut(
    Object* dictObj,
    Object* key,
    Object* hash,
    Object* value,
    Runtime* runtime) {
  Dictionary* dict = Dictionary::cast(dictObj);
  int bucket = -1;
  bool found = lookup(dict, key, hash, &bucket);
  if (bucket == -1) {
    // TODO(mpage): Grow at a predetermined load factor, rather than when full
    grow(dict, runtime);
    lookup(dict, key, hash, &bucket);
    assert(bucket != -1);
  }
  DictItem item(dict->items(), bucket);
  item.set(hash, key, value);
  if (!found) {
    dict->setNumItems(dict->numItems() + 1);
  }
}

bool Dictionary::itemAtRemove(
    Object* dictObj,
    Object* key,
    Object* hash,
    Object** value) {
  Dictionary* dict = Dictionary::cast(dictObj);
  int bucket = -1;
  bool found = lookup(dict, key, hash, &bucket);
  if (found) {
    DictItem item(dict->items(), bucket);
    *value = item.value();
    item.setTombstone();
    dict->setNumItems(dict->numItems() - 1);
  }
  return found;
}

bool Dictionary::lookup(
    Dictionary* dict,
    Object* key,
    Object* hash,
    int* bucket) {
  word hashVal = SmallInteger::cast(hash)->value();
  int startIdx = kPointersPerBucket * (hashVal & (dict->capacity() - 1));
  int idx = startIdx;
  int nextFreeBucket = -1;
  ObjectArray* items = dict->items();

  assert(bucket != nullptr);

  // TODO(mpage) - Quadratic probing?
  do {
    DictItem item(items, idx);
    if (item.matches(key)) {
      *bucket = idx;
      return true;
    } else if (nextFreeBucket == -1 && item.isTombstone()) {
      nextFreeBucket = idx;
    } else if (item.isEmpty()) {
      if (nextFreeBucket == -1) {
        nextFreeBucket = idx;
      }
      break;
    }
    idx = (idx + kPointersPerBucket) % items->length();
  } while (idx != startIdx);

  *bucket = nextFreeBucket;

  return false;
}

void Dictionary::grow(Dictionary* dict, Runtime* runtime) {
  // Double the size of the backing store
  Object* obj = runtime->createObjectArray(dict->items()->length() * 2);
  assert(obj != nullptr);
  ObjectArray* newItems = ObjectArray::cast(obj);

  // Re-insert items
  ObjectArray* oldItems = dict->items();
  dict->setItems(newItems);
  for (int i = 0; i < oldItems->length(); i += kPointersPerBucket) {
    DictItem oldItem(oldItems, i);
    Object* oldHash = oldItem.hash();
    if (oldHash->isNone()) {
      continue;
    }
    int bucket = -1;
    lookup(dict, oldItem.key(), oldHash, &bucket);
    assert(bucket != -1);
    DictItem newItem(newItems, bucket);
    newItem.set(oldItem.key(), oldHash, oldItem.value());
  }
}

// String

Object* String::hash() {
  // TODO(mpage) - Take the hash algorithm from CPython
  auto rawStr = reinterpret_cast<char*>(address() + String::kSize);
  std::string str(reinterpret_cast<char*>(rawStr), length());
  std::hash<std::string> hashFunc;
  return SmallInteger::fromWord(hashFunc(str));
}

} // namespace python
