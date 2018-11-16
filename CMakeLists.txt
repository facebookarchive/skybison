cmake_minimum_required(VERSION 3.7)
project(python)

# Add -DPYRO_ASAN=1 to the cmake command line to enable asan.
# Older compilers may have trouble building or running asan
# properly. Try setting the environment variables:
#
#   CC==/usr/local/fbcode/gcc-5-glibc-2.23/bin/gcc
#   CXX=/usr/local/fbcode/gcc-5-glibc-2.23/bin/g++
#
# for cmake if you have trouble with asan.
if (${PYRO_ASAN})
  set(CMAKE_CXX_FLAGS "-fsanitize=address -lpthread")
endif()

set(PYRO_HOME ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(third-party/benchmark-1.3.0)
add_subdirectory(third-party/googletest-1.8.0)
add_subdirectory(third-party/siphash-20170224)

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
  set(OS_DEFINE "-DOS_OSX")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set(OS_DEFINE "-DOS_LINUX")
else()
  message("Unknown CMAKE_SYSTEM_NAME " ${CMAKE_SYSTEM_NAME})
endif()

set(
  PYRO_PROPERTIES
  CXX_EXTENSIONS NO
  CXX_STANDARD 11
  CXX_STANDARD_REQUIRED YES)
set(
  PYRO_COMPILE_OPTIONS
  ${OS_DEFINE}
  -Wall
  -Werror
  -Wextra
  -pedantic)

add_library(
  runtime
  STATIC
  runtime/builtins.cpp
  runtime/builtins.h
  runtime/bytecode.cpp
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
  runtime/layout.h
  runtime/marshal.cpp
  runtime/marshal.h
  runtime/mro.cpp
  runtime/mro.h
  runtime/objects.cpp
  runtime/objects.h
  runtime/os.cpp
  runtime/os.h
  runtime/runtime.cpp
  runtime/runtime.h
  runtime/scavenger.cpp
  runtime/scavenger.h
  runtime/space.cpp
  runtime/space.h
  runtime/symbols.cpp
  runtime/symbols.h
  runtime/thread.cpp
  runtime/thread.h
  runtime/trampolines.cpp
  runtime/trampolines.h
  runtime/trampolines-inl.h
  runtime/utils.cpp
  runtime/utils.h
  runtime/vector.h
  runtime/view.h
  runtime/visitor.h)
set_target_properties(
  runtime
  PROPERTIES
  ${PYRO_PROPERTIES})
target_compile_options(
  runtime
  PRIVATE
  ${PYRO_COMPILE_OPTIONS})
target_link_libraries(
  runtime
  dl
  siphash)

add_executable(
  python
  main.cpp)
set_target_properties(
  python
  PROPERTIES
  ${PYRO_PROPERTIES})
target_compile_options(
  runtime
  PRIVATE
  ${PYRO_COMPILE_OPTIONS})
target_link_libraries(
  python
  runtime)

add_executable(
  python-tests
  runtime/python-test.cpp
  runtime/attributes-test.cpp
  runtime/bytecode-test.cpp
  runtime/handles-test.cpp
  runtime/heap-test.cpp
  runtime/layout-test.cpp
  runtime/marshal-test.cpp
  runtime/objects-test.cpp
  runtime/os-test.cpp
  runtime/runtime-test.cpp
  runtime/scavenger-test.cpp
  runtime/space-test.cpp
  runtime/test-utils.cpp
  runtime/test-utils.h
  runtime/thread-test.cpp
  runtime/utils-test.cpp
  runtime/vector-test.cpp)
set_target_properties(
  python-tests
  PROPERTIES
  ${PYRO_PROPERTIES})
target_compile_options(
  python-tests
  PRIVATE
  ${PYRO_COMPILE_OPTIONS})
target_link_libraries(
  python-tests
  benchmark
  gtest
  runtime)
