#pragma once

#include <cstdlib>

namespace python {

// A simple stateless allocator class that uses std::malloc
// and std::free to allocate and deallocate in memory.
template <typename T>
class SimpleAllocator {
 public:
  using value_type = T;

  SimpleAllocator() = default;
  SimpleAllocator(const SimpleAllocator&) = default;

  T* allocate(size_t n) {
    return reinterpret_cast<T*>(std::malloc(n));
  }

  // For this simple allocator, the size of the pointer is not needed.
  // std::free correctly deallocates the entire malloc'd space.
  void deallocate(T* p, size_t) {
    std::free(p);
  }
};

} // namespace python
