#pragma once

#include "globals.h"

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <utility>

namespace python {

// A partial clone of std::vector<> that supports on-stack allocation and type
// erasure. Only supports POD types for now. Based on llvm's SmallVector, but
// simpler.
//
// Use `const Vector<T>&` in functions that wish to accept a vector reference.
// You can pass a FixedVector<T, N> to such functions without requiring them to
// depend on the value of N.
//
// TODO: custom allocator support.
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
  ~Vector() {
    release();
  }

  Vector(const Vector& other) {
    *this = other;
  }

  Vector& operator=(const Vector& other) {
    clear();
    auto const s = other.size();
    reserve(s);
    end_ = begin_ + s;
    assert(end_ <= end_storage_);
    std::memcpy(begin_, other.begin_, s * sizeof(T));
    return *this;
  }

  Vector(Vector&& other) {
    *this = std::move(other);
  }

  Vector& operator=(Vector&& other) {
    if (!other.is_small()) {
      // If the other vector's storage is heap allocated, the simplest possible
      // thing is to just take ownership of its storage. Even if the current
      // vector is small, and has sufficient capacity, the malloc has already
      // been paid for.
      release();
      std::swap(begin_, other.begin_);
      std::swap(end_, other.end_);
      std::swap(end_storage_, other.end_storage_);
    } else {
      // if the other vector is small, we have to copy its data, since that
      // memory can't be stolen.
      *this = other;
      // We don't *need* to release other's storage, but lets do it anyway for
      // consistent behavior.
      other.release();
    }
    return *this;
  }

  iterator begin() {
    return begin_;
  }
  const_iterator begin() const {
    return begin_;
  }
  iterator end() {
    return end_;
  }
  const_iterator end() const {
    return end_;
  }

  T& operator[](size_type idx) {
    assert(idx < size());
    return begin()[idx];
  }
  const T& operator[](size_type idx) const {
    assert(idx < size());
    return begin()[idx];
  }

  T& front() {
    assert(!empty());
    return begin()[0];
  }
  const T& front() const {
    assert(!empty());
    return begin()[0];
  }

  T& back() {
    assert(!empty());
    return end()[-1];
  }
  const T& back() const {
    assert(!empty());
    return end()[-1];
  }

  size_type size() const {
    return end_ - begin_;
  }

  bool empty() const {
    return size() == 0;
  }

  size_type capacity() const {
    return end_storage_ - begin_;
  }

  void reserve(size_type new_capacity) {
    assert(new_capacity > 0);
    if (new_capacity <= 0) {
      return;
    }
    grow(new_capacity);
  }

  void clear() {
    end_ = begin_;
  }

  void push_back(const T& t) {
    if (end_ >= end_storage_) {
      grow(0);
    }
    *end_ = t;
    end_++;
  }

  void pop_back() {
    assert(!empty());
    end_--;
  }

 protected:
  // Used by FixedVector
  Vector(T* begin, T* end_storage)
      : begin_(begin), end_storage_(end_storage), end_(begin) {}

 private:
  static constexpr int growthFactor = 2;

  bool is_small() const {
    return begin_ == &storage_[0];
  }

  void release() {
    if (!is_small() and begin_ != nullptr) {
      std::free(begin_);
    }
    begin_ = nullptr;
    end_ = nullptr;
    end_storage_ = nullptr;
  }

  void grow(size_type new_cap) {
    auto const old_cap = capacity();
    auto const old_size = size();
    auto const small = is_small();

    if (new_cap == 0) {
      if (old_cap == 0) {
        new_cap = 4;
      } else {
        new_cap = old_cap * growthFactor;
      }
    }

    if (old_cap >= new_cap) {
      return;
    }

    auto old_begin = begin_;

    // TODO: custom allocator support.
    begin_ = reinterpret_cast<T*>(std::malloc(new_cap * sizeof(T)));
    assert(begin_ != nullptr);
    end_storage_ = begin_ + new_cap;
    end_ = begin_ + old_size;

    if (old_begin != nullptr) {
      std::memcpy(begin_, old_begin, old_size * sizeof(T));

      if (!small) {
        std::free(old_begin);
      }
    }
  }

  T* begin_ = nullptr;
  T* end_storage_ = nullptr;
  T* end_ = nullptr;

  // If this is a FixedVector, begin_ will initially point here, which lets
  // us know not to free to the storage when growing.
  // C++ does not allow 0 length arrays so we have to pay for 1 element in the
  // base class.
 protected:
  T storage_[1];

  // Do not add members after storage_!
};

template <typename T, typename Vector<T>::size_type Size>
class FixedVector : public Vector<T> {
 public:
  FixedVector()
      : Vector<T>(&Vector<T>::storage_[0], &Vector<T>::storage_[0] + Size) {}

  FixedVector(const Vector<T>& other) : Vector<T>(other) {}
  FixedVector(Vector<T>&& other) : Vector<T>(std::forward<Vector<T>>(other)) {}

 private:
  // TODO: this gets much messier if we want to support non-POD types.
  T array_[Size - 1];
};

} // namespace python
