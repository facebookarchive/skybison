cmake_minimum_required(VERSION 3.7)
project(python)

set(PYTHON_EXECUTABLE /usr/bin/python)

set(CMAKE_CXX_STANDARD 11)

add_subdirectory(third-party/googletest-1.8.0)
include_directories(third-party/googletest-1.8.0/googletest/include)
include_directories(third-party/googletest-1.8.0/googlemock/include)

add_library(
  runtime
  STATIC
  runtime/bytecode.h
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

add_executable(
  python
  main.cpp)
target_link_libraries(
  python
  runtime)

add_executable(
  python-tests
  runtime/handles-test.cpp
  runtime/heap-test.cpp
  runtime/marshal-test.cpp
  runtime/objects-test.cpp
  runtime/os-test.cpp
  runtime/runtime-test.cpp
  runtime/space-test.cpp
  runtime/thread-test.cpp)
target_link_libraries(
  python-tests
  gtest
  gtest_main
  runtime)
