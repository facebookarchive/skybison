cmake_minimum_required(VERSION 3.5)
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

find_program(CLANG_TIDY_PROGRAM clang-tidy)
if(CLANG_TIDY_PROGRAM)
  set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_PROGRAM})
else()
  message("Could not find clang-tidy")
endif()
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
  runtime/bool-builtins.cpp
  runtime/bool-builtins.h
  runtime/builtins-module.cpp
  runtime/builtins-module.h
  runtime/bytecode.cpp
  runtime/bytecode.h
  runtime/callback.h
  runtime/capi-handles.cpp
  runtime/capi-handles.h
  runtime/descriptor-builtins.cpp
  runtime/descriptor-builtins.h
  runtime/dict-builtins.cpp
  runtime/dict-builtins.h
  runtime/exception-builtins.cpp
  runtime/exception-builtins.h
  runtime/float-builtins.cpp
  runtime/float-builtins.h
  runtime/frame.cpp
  runtime/frame.h
  runtime/function-builtins.cpp
  runtime/globals.h
  runtime/handles.cpp
  runtime/handles.h
  runtime/heap.cpp
  runtime/heap.h
  runtime/int-builtins.cpp
  runtime/int-builtins.h
  runtime/interpreter.cpp
  runtime/interpreter.h
  runtime/layout.h
  runtime/list-builtins.cpp
  runtime/list-builtins.h
  runtime/marshal.cpp
  runtime/marshal.h
  runtime/mro.cpp
  runtime/mro.h
  runtime/object-builtins.cpp
  runtime/objects.cpp
  runtime/objects.h
  runtime/os.cpp
  runtime/os.h
  runtime/ref-builtins.cpp
  runtime/ref-builtins.h
  runtime/runtime.cpp
  runtime/runtime.h
  runtime/scavenger.cpp
  runtime/scavenger.h
  runtime/set-builtins.cpp
  runtime/set-builtins.h
  runtime/space.cpp
  runtime/space.h
  runtime/str-builtins.cpp
  runtime/str-builtins.h
  runtime/super-builtins.cpp
  runtime/super-builtins.h
  runtime/symbols.cpp
  runtime/symbols.h
  runtime/sys-module.cpp
  runtime/sys-module.h
  runtime/thread.cpp
  runtime/thread.h
  runtime/time-module.cpp
  runtime/time-module.h
  runtime/tracked-allocation.cpp
  runtime/tracked-allocation.h
  runtime/trampolines-inl.h
  runtime/trampolines.cpp
  runtime/trampolines.h
  runtime/tuple-builtins.cpp
  runtime/tuple-builtins.h
  runtime/type-builtins.cpp
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
  ext/Objects/abstract.cpp
  ext/Objects/boolobject.cpp
  ext/Objects/descrobject.cpp
  ext/Objects/dictobject.cpp
  ext/Objects/exceptions.cpp
  ext/Objects/floatobject.cpp
  ext/Objects/listobject.cpp
  ext/Objects/longobject.cpp
  ext/Objects/moduleobject.cpp
  ext/Objects/object.cpp
  ext/Objects/obmalloc.cpp
  ext/Objects/tupleobject.cpp
  ext/Objects/typeobject.cpp
  ext/Objects/unicodeobject.cpp
  ext/Python/_warnings.cpp
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
  runtime/builtins-module-test.cpp
  runtime/descriptor-builtins-test.cpp
  runtime/exception-builtins-test.cpp
  runtime/float-builtins-test.cpp
  runtime/handles-test.cpp
  runtime/heap-test.cpp
  runtime/int-builtins-test.cpp
  runtime/interpreter-test.cpp
  runtime/layout-test.cpp
  runtime/list-builtins-test.cpp
  runtime/marshal-test.cpp
  runtime/object-builtins-test.cpp
  runtime/objects-test.cpp
  runtime/os-test.cpp
  runtime/ref-builtins-test.cpp
  runtime/runtime-test.cpp
  runtime/scavenger-test.cpp
  runtime/set-builtins-test.cpp
  runtime/space-test.cpp
  runtime/str-builtins-test.cpp
  runtime/super-builtins-test.cpp
  runtime/sys-module-test.cpp
  runtime/test-utils.cpp
  runtime/test-utils.h
  runtime/thread-test.cpp
  runtime/time-module-test.cpp
  runtime/tracked-allocation-test.cpp
  runtime/trampolines-test.cpp
  runtime/tuple-builtins-test.cpp
  runtime/type-builtins-test.cpp
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
  ext/Objects/abstract-test.cpp
  ext/Objects/boolobject-test.cpp
  ext/Objects/floatobject-test.cpp
  ext/Objects/dictobject-test.cpp
  ext/Objects/listobject-test.cpp
  ext/Objects/moduleobject-test.cpp
  ext/Objects/object-test.cpp
  ext/Objects/tupleobject-test.cpp
  ext/Objects/typeobject-test.cpp
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
