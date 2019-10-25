#pragma once

namespace py {

template <typename T>
class Callback {
 public:
  virtual T call() = 0;
};

}  // namespace py
