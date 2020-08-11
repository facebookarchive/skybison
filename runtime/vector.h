#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <type_traits>
#include <utility>

#include "globals.h"
#include "utils.h"

namespace py {

// A partial clone of std::vector<>. Only supports POD types.  Based on llvm's
// SmallVector, but simpler.
//
// Use `const Vector<T>&` in functions that wish to accept a vector reference.
template <typename T>
class Vector {
  // We can add support for non-POD types but its a lot harder to get right, so
  // we'll start with the simple thing.
  static_assert(IS_TRIVIALLY_COPYABLE(T), "Vector only supports POD types");

 public:
  using size_type = word;
  using iterator = T*;
  using const_iterator = const T*;

  Vector() = default;

  ~Vector() { release(); }

  Vector(const Vector& other) { *this = other; }

  Vector& operator=(const Vector& other) {
    clear();
    auto const s = other.size();
    reserve(s);
    end_ = begin_ + s;
    DCHECK(end_ <= end_storage_, "vector assignment overflow");
    std::memcpy(begin_, other.begin_, s * sizeof(T));
    return *this;
  }

  Vector(Vector&& other) { *this = std::move(other); }

  Vector& operator=(Vector&& other) {
    // If the other vector's storage is heap allocated, the simplest possible
    // thing is to just take ownership of its storage. Even if the current
    // vector is small, and has sufficient capacity, the malloc has already
    // been paid for.
    release();
    std::swap(begin_, other.begin_);
    std::swap(end_, other.end_);
    std::swap(end_storage_, other.end_storage_);
    return *this;
  }

  iterator begin() { return begin_; }
  const_iterator begin() const { return begin_; }
  iterator end() { return end_; }
  const_iterator end() const { return end_; }

  T& operator[](size_type idx) {
    DCHECK_INDEX(idx, size());
    return begin()[idx];
  }
  const T& operator[](size_type idx) const {
    DCHECK_INDEX(idx, size());
    return begin()[idx];
  }

  T& front() {
    DCHECK(!empty(), "front on empty");
    return begin()[0];
  }
  const T& front() const {
    DCHECK(!empty(), "front on empty");
    return begin()[0];
  }

  T& back() {
    DCHECK(!empty(), "back on empty");
    return end()[-1];
  }
  const T& back() const {
    DCHECK(!empty(), "back on empty");
    return end()[-1];
  }

  size_type size() const { return end_ - begin_; }

  bool empty() const { return size() == 0; }

  size_type capacity() const { return end_storage_ - begin_; }

  void reserve(size_type new_capacity) {
    DCHECK(new_capacity > 0, "invalid new_capacity %ld", new_capacity);
    if (new_capacity <= 0) {
      return;
    }
    grow(new_capacity);
  }

  void clear() { end_ = begin_; }

  void push_back(const T& t) {  // NOLINT
    if (end_ >= end_storage_) {
      grow(0);
    }
    *end_ = t;
    end_++;
  }

  void pop_back() {  // NOLINT
    DCHECK(!empty(), "pop back on empty");
    end_--;
  }

  void release() {
    std::free(begin_);
    begin_ = nullptr;
    end_ = nullptr;
    end_storage_ = nullptr;
  }

 private:
  static constexpr int kGrowthFactor = 2;

  void grow(size_type new_cap) {
    auto const old_cap = capacity();
    auto const old_size = size();

    if (new_cap == 0) {
      if (old_cap == 0) {
        new_cap = 4;
      } else {
        new_cap = old_cap * kGrowthFactor;
      }
    }

    if (old_cap >= new_cap) {
      return;
    }

    auto old_begin = begin_;

    begin_ = static_cast<T*>(std::malloc(new_cap * sizeof(T)));
    DCHECK(begin_ != nullptr, "out of memory");
    end_storage_ = begin_ + new_cap;
    end_ = begin_ + old_size;

    if (old_begin != nullptr) {
      std::memcpy(begin_, old_begin, old_size * sizeof(T));
      std::free(old_begin);
    }
  }

  T* begin_ = nullptr;
  T* end_storage_ = nullptr;
  T* end_ = nullptr;
};

}  // namespace py
