#pragma once

#include "globals.h"

#include <cassert>

namespace python {

template <typename T>
class View {
 public:
  View(const T* data, word length) : data_(data), length_(length) {}

  template <word N>
  View(const T (&data)[N]) : data_(data), length_(N) {}

  T get(word i) {
    assert(i >= 0);
    assert(i < length_);
    return data_[i];
  }

  const T* data() {
    return data_;
  }

  word length() {
    return length_;
  }

 private:
  const T* data_;
  word length_;
};

} // namespace python
