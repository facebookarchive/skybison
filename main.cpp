#include <iostream>

#include "runtime/Runtime.h"

int main() {
  python::Runtime::initialize();
  std::cout << "Hello, World!" << std::endl;
  return 0;
}
