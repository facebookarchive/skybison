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

add_subdirectory(third-party/benchmark-1.3.0)
add_subdirectory(third-party/googletest-1.8.0)
add_subdirectory(third-party/siphash-20170224)

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
  set(OS_DEFINE "-DOS_OSX")
  set(WHOLE_ARCHIVE -Wl,-force_load)
  set(NO_WHOLE_ARCHIVE)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set(OS_DEFINE "-DOS_LINUX")
  set(WHOLE_ARCHIVE -Wl,--whole-archive)
  set(NO_WHOLE_ARCHIVE -Wl,--no-whole-archive)
else()
  message("Unknown CMAKE_SYSTEM_NAME " ${CMAKE_SYSTEM_NAME})
endif()

set(CMAKE_C_STANDARD 99)

set(CMAKE_CXX_EXTENSIONS NO)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

set(
  PYRO_COMPILE_OPTIONS
  -Wall
  -Werror
  -Wextra
  -pedantic)

add_library(
  runtime
  STATIC
  runtime/allocator.h
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
target_compile_options(
  runtime
  PRIVATE
  ${PYRO_COMPILE_OPTIONS})
target_link_libraries(
  runtime
  PRIVATE
  dl
  siphash)

add_library(
  extension
  STATIC
  ext/Include/Python.h
  ext/Include/modsupport.h
  ext/Include/pyport.h
  ext/Objects/dictobject.cpp
  ext/Objects/exceptions.cpp
  ext/Objects/longobject.cpp
  ext/Objects/moduleobject.cpp
  ext/Objects/unicodeobject.cpp
  ext/Python/errors.cpp)
target_compile_options(
  extension
  PUBLIC
  ${OS_DEFINE}
  PRIVATE
  ${PYRO_COMPILE_OPTIONS})
target_include_directories(
  extension
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ext/config
  ext/Include)
target_link_libraries(
  extension
  PRIVATE
  runtime)

add_library(
  modules
  STATIC
  ext/Modules/config.cpp
  third-party/cpython/Modules/errnomodule.c)
target_compile_options(
  modules
  PUBLIC
  ${OS_DEFINE})
target_include_directories(
  modules
  PRIVATE
  ext/config
  ext/Include)
target_link_libraries(
  modules
  PRIVATE
  extension)

# TODO(eelizondo): remove this when extensions are completely standalone
if(${CPYTHON_MODULES})
    message("Building CPython...")
    add_definitions(-DCPYTHON_MODULES)
    execute_process(
      COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/util/cpython_build.py
              ${CMAKE_BINARY_DIR}/cpython_build
              ${CMAKE_CURRENT_SOURCE_DIR}/third-party/cpython
              ${CMAKE_CURRENT_SOURCE_DIR}/ext/Objects
      OUTPUT_VARIABLE libpython
      ERROR_VARIABLE _unused)
    string(STRIP "${libpython}" libpython)
    find_package(Threads)
    set(libpython "util ${CMAKE_THREAD_LIBS_INIT} ${libpython}")
endif(${CPYTHON_MODULES})

add_executable(
  python
  main.cpp)
target_link_libraries(
  python
  PRIVATE
  runtime
  modules)

add_library(
  runtime-tests
  STATIC
  runtime/attributes-test.cpp
  runtime/trampolines-test.cpp
  runtime/handles-test.cpp
  runtime/heap-test.cpp
  runtime/interpreter-test.cpp
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
target_link_libraries(
  runtime-tests
  PUBLIC
  benchmark
  gtest
  PRIVATE
  runtime
  modules)

add_library(
  extension-tests
  STATIC
  ext/Objects/dictobject-test.cpp
  ext/Objects/moduleobject-test.cpp
  ext/Objects/unicodeobject-test.cpp)
target_include_directories(
  extension-tests
  PRIVATE
  ext/Include
  ext/config
  ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
  extension-tests
  PUBLIC
  benchmark
  gtest
  PRIVATE
  runtime
  modules)

add_executable(
  python-tests
  runtime/python-test.cpp)
target_link_libraries(
  python-tests
  PRIVATE
  ${WHOLE_ARCHIVE} runtime-tests ${NO_WHOLE_ARCHIVE}
  ${WHOLE_ARCHIVE} extension-tests ${NO_WHOLE_ARCHIVE}
  runtime
  modules)
