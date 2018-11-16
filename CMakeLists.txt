cmake_minimum_required(VERSION 3.7)
project(python)

set(PYTHON_EXECUTABLE /usr/bin/python)

add_subdirectory(third-party/benchmark-1.3.0)
add_subdirectory(third-party/googletest-1.8.0)
add_subdirectory(third-party/siphash-20170224)

add_library(
  runtime
  STATIC
  runtime/builtins.cpp
  runtime/builtins.h
  runtime/bytecode.h
  runtime/callback.h
  runtime/frame.cpp
  runtime/frame.h
  runtime/globals.h
  runtime/handles.cpp
  runtime/handles.h
  runtime/heap.cpp
  runtime/heap.h
  runtime/interpreter.cpp
  runtime/interpreter.h
  runtime/marshal.cpp
  runtime/marshal.h
  runtime/objects.cpp
  runtime/objects.h
  runtime/os.cpp
  runtime/os.h
  runtime/runtime.cpp
  runtime/runtime.h
  runtime/space.cpp
  runtime/space.h
  runtime/thread.cpp
  runtime/thread.h
  runtime/trampolines.cpp
  runtime/trampolines.h
  runtime/utils.h
  runtime/visitor.h)
set_target_properties(
  runtime
  PROPERTIES
  CXX_EXTENSIONS NO
  CXX_STANDARD 11
  CXX_STANDARD_REQUIRED YES)
target_compile_options(
  runtime
  PRIVATE
  -Wall -Werror -Wextra -pedantic)
target_link_libraries(
  runtime
  siphash)

add_executable(
  python
  main.cpp)
set_target_properties(
  python
  PROPERTIES
  CXX_EXTENSIONS NO
  CXX_STANDARD 11
  CXX_STANDARD_REQUIRED YES)
target_compile_options(
  runtime
  PRIVATE
  -Wall -Werror -Wextra -pedantic)
target_link_libraries(
  python
  runtime)

add_executable(
  python-tests
  runtime/python-test.cpp
  runtime/handles-test.cpp
  runtime/heap-test.cpp
  runtime/marshal-test.cpp
  runtime/objects-test.cpp
  runtime/os-test.cpp
  runtime/runtime-test.cpp
  runtime/space-test.cpp
  runtime/thread-test.cpp
  runtime/utils-test.cpp
  runtime/vector-test.cpp)
set_target_properties(
  python-tests
  PROPERTIES
  CXX_EXTENSIONS NO
  CXX_STANDARD 11
  CXX_STANDARD_REQUIRED YES)
target_compile_options(
  python-tests
  PRIVATE
  -Wall -Werror -Wextra -pedantic)
target_link_libraries(
  python-tests
  gtest
  benchmark
  runtime)

add_custom_target(
  check
  COMMAND python-tests
  DEPENDS python-tests python)
