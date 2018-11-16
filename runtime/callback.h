#pragma once

namespace python {

template <typename T>
class Callback {
 public:
  virtual T call() = 0;
};

}  // namespace python
