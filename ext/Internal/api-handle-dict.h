#pragma once

#include "globals.h"
#include "objects.h"

namespace py {

class PointerVisitor;

// Dictionary associating `RawObject` with `ApiHandle*` (aka `PyObject*`).
// Also used to associate `RawObject` to cached values (like representations of
// the object as `const char*`).
class ApiHandleDict {
 public:
  ApiHandleDict() {}

  ~ApiHandleDict() {
    std::free(indices());
    std::free(keys());
    std::free(values());
    setIndices(nullptr);
    setKeys(nullptr);
    setValues(nullptr);
  }

  // Looks up the value associated with key, or nullptr if not found.
  void* at(RawObject key);

  // Lookup value of entry at `item_index` as returned by `atPutLookup`.
  void* atIndex(int32_t item_index);

  void atPut(RawObject key, void* value);

  // Looks for `key` in the dictionary. Returns `true` when a new entry for
  // `key` was inserted, `false` if one already existed. Sets `*item_index` to
  // the index of the entry. `atPutValue` must be used to set the value of a new
  // entry before the next lookup.
  bool atPutLookup(RawObject key, int32_t* item_index);

  // Inserts `value` at entry at `item_index` as returned by `atPutLookup`.
  void atPutValue(int32_t item_index, void* value);

  void grow();

  void initialize(word num_indices);

  // Rehash the items into new storage with the given number of indices.
  void rehash(word new_num_indices);

  void* remove(RawObject key);

  void visitKeys(PointerVisitor* visitor);

  // Getters and setters
  int32_t capacity() { return capacity_; }
  void setCapacity(int32_t capacity) { capacity_ = capacity; }

  int32_t* indices() { return indices_; }
  void setIndices(int32_t* indices) { indices_ = indices; }

  RawObject* keys() { return keys_; }
  void setKeys(RawObject* keys) { keys_ = keys; }

  int32_t nextIndex() { return next_index_; }
  void setNextIndex(int32_t next_index) { next_index_ = next_index; }

  word numIndices() { return num_indices_; }
  void setNumIndices(word num_indices) { num_indices_ = num_indices; }

  int32_t numItems() { return num_items_; }
  void decrementNumItems() { num_items_--; }
  void incrementNumItems() { num_items_++; }

  void** values() { return values_; }
  void setValues(void** values) { values_ = values; }

 private:
  int32_t capacity_ = 0;
  int32_t* indices_ = nullptr;
  RawObject* keys_ = nullptr;
  int32_t next_index_ = 0;
  word num_indices_ = 0;
  int32_t num_items_ = 0;
  void** values_ = nullptr;

  static const int kGrowthFactor = 2;
  static const int kShrinkFactor = 4;

  // Returns true if there is enough room in the dense arrays for another item.
  bool hasUsableItem() { return nextIndex() < capacity(); }

  // Returns true and sets the indices if the key was found.
  bool lookup(RawObject key, word* sparse, int32_t* dense);

  DISALLOW_HEAP_ALLOCATION();
  DISALLOW_COPY_AND_ASSIGN(ApiHandleDict);
};

}  // namespace py
