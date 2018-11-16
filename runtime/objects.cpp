#include "objects.h"
#include "runtime.h"

#include <cassert>
#include <string>

namespace python {

// Dictionary

// TODO(mpage): Needs handlizing

// Helper class for working with entries in the backing ObjectArray.
class DictItem {
 public:
  DictItem(Object* items, int bucket)
      : items_(ObjectArray::cast(items)), bucket_(bucket){};

  inline Object* hash() {
    return items_->at(bucket_ + Dictionary::kBucketHashOffset);
  }

  inline Object* key() {
    return items_->at(bucket_ + Dictionary::kBucketKeyOffset);
  }

  inline Object* value() {
    return items_->at(bucket_ + Dictionary::kBucketValueOffset);
  }

  inline bool matches(Object* that) {
    return !hash()->isNone() && Object::equals(key(), that);
  }

  inline void set(Object* hash, Object* key, Object* value) {
    items_->atPut(bucket_ + Dictionary::kBucketHashOffset, hash);
    items_->atPut(bucket_ + Dictionary::kBucketKeyOffset, key);
    items_->atPut(bucket_ + Dictionary::kBucketValueOffset, value);
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

bool Dictionary::at(
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

void Dictionary::atPut(
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

bool Dictionary::remove(
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
  Object* items = dict->items();

  assert(bucket != nullptr);

  // TODO(mpage) - Quadratic probing?
  int length = ObjectArray::cast(items)->length();
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
    idx = (idx + kPointersPerBucket) % length;
  } while (idx != startIdx);

  *bucket = nextFreeBucket;

  return false;
}

void Dictionary::grow(Dictionary* dict, Runtime* runtime) {
  // Double the size of the backing store
  word newLength = ObjectArray::cast(dict->items())->length() * 2;
  Object* obj = runtime->newObjectArray(newLength);
  assert(obj != nullptr);
  ObjectArray* newItems = ObjectArray::cast(obj);
  assert(newItems->length() == newLength);

  // Re-insert items
  ObjectArray* oldItems = ObjectArray::cast(dict->items());
  dict->setItems(newItems);
  for (word i = 0; i < oldItems->length(); i += kPointersPerBucket) {
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

bool String::equals(Object* that) {
  if (!that->isString()) {
    return false;
  }
  String* thatStr = String::cast(that);
  if (length() != thatStr->length()) {
    return false;
  }
  auto s1 = reinterpret_cast<void*>(address() + String::kSize);
  auto s2 = reinterpret_cast<void*>(thatStr->address() + String::kSize);
  return memcmp(s1, s2, length()) == 0;
}

bool String::equalsCString(const char* c_string) {
  const char* cp = c_string;
  for (word i = 0; i < length(); i++, cp++) {
    char ch = *cp;
    if (ch == '\0' || ch != charAt(i)) {
      return false;
    }
  }
  return *cp == '\0';
}

Object* String::hash() {
  // TODO(mpage) - Take the hash algorithm from CPython
  auto rawStr = reinterpret_cast<char*>(address() + String::kSize);
  std::string str(reinterpret_cast<char*>(rawStr), length());
  std::hash<std::string> hashFunc;
  return SmallInteger::fromWord(hashFunc(str));
}

} // namespace python
