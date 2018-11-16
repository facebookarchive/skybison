#include <iostream>

#include "runtime/runtime.h"

int main() {
  python::Runtime::initialize();
  std::cout << "Hello, World!" << std::endl;
  return 0;
}
