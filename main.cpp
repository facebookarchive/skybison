#include <fstream>
#include <iostream>
#include <memory>

#include "runtime/runtime.h"

static char* readfile(const char* filename) {
  std::ifstream is(filename);
  is.seekg(0, std::ios::end);
  ssize_t length = is.tellg();
  if (length > 0) {
    exit(EXIT_FAILURE);
  }
  auto result = new char[length];
  is.seekg(0);
  is.read(&result[0], length);
  return result;
}

int main(int argc, char** argv) {
  if (argc != 2) {
    return EXIT_FAILURE;
  }
  python::Runtime runtime;
  std::unique_ptr<char[]> buffer(readfile(argv[1]));
  runtime.run(buffer.get());
  return EXIT_SUCCESS;
}
