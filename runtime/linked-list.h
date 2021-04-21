#pragma once

namespace py {

struct ListEntry {
  ListEntry* prev;
  ListEntry* next;
};

// Inserts an entry into the linked list as the new root
bool listEntryInsert(ListEntry* entry, ListEntry** root);

// Removes an entry from the linked list
bool listEntryRemove(ListEntry* entry, ListEntry** root);

inline bool listEntryInsert(ListEntry* entry, ListEntry** root) {
  // If already tracked, do nothing.
  if (entry->prev != nullptr || entry->next != nullptr || entry == *root) {
    return false;
  }
  entry->prev = nullptr;
  entry->next = *root;
  if (*root != nullptr) {
    (*root)->prev = entry;
  }
  *root = entry;
  return true;
}

inline bool listEntryRemove(ListEntry* entry, ListEntry** root) {
  // The node is the first node of the list.
  if (*root == entry) {
    *root = entry->next;
  } else if (entry->prev == nullptr && entry->next == nullptr) {
    // This is an already untracked object.
    return false;
  }
  if (entry->prev != nullptr) {
    entry->prev->next = entry->next;
  }
  if (entry->next != nullptr) {
    entry->next->prev = entry->prev;
  }
  entry->prev = nullptr;
  entry->next = nullptr;
  return true;
}

}  // namespace py
