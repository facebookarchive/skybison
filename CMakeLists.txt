cmake_minimum_required(VERSION 3.5)
project(python)

# Add -DPYRO_ASAN=1 to the cmake command line to enable asan.
if (${PYRO_ASAN})
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
  if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ldl -lpthread -lrt")
  endif()
endif()

set(BENCHMARK_ENABLE_TESTING CACHE BOOL OFF)
set(BENCHMARK_ENABLE_EXCEPTIONS CACHE BOOL OFF)
set(BENCHMARK_ENABLE_INSTALL CACHE BOOL OFF)
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

# DebugOpt build type: debug assertions/checks with -Os. Runs tests much more
# quickly than Debug, but isn't as debuggable as a Debug build in gdb or lldb.
set(CMAKE_C_FLAGS_DEBUGOPT "-Os -g3" CACHE STRING
    "Flags used by the C compiler during DebugOpt builds."
    FORCE)
set(CMAKE_CXX_FLAGS_DEBUGOPT "-Os -g3" CACHE STRING
    "Flags used by the C++ compiler during DebugOpt builds."
    FORCE)
set(CMAKE_EXE_LINKER_FLAGS_DEBUGOPT
    "${CMAKE_EXE_LINKER_FLAGS_DEBUG}" CACHE STRING
    "Flags used for linking binaries during DebugOpt builds."
    FORCE)
set(CMAKE_SHARED_LINKER_FLAGS_DEBUGOPT
    "${CMAKE_SHARED_LINKER_FLAGS_DEBUG}" CACHE STRING
    "Flags used by the shared libraries linker during DebugOpt builds."
    FORCE)
mark_as_advanced(
    CMAKE_CXX_FLAGS_DEBUGOPT
    CMAKE_C_FLAGS_DEBUGOPT
    CMAKE_EXE_LINKER_FLAGS_DEBUGOPT
    CMAKE_SHARED_LINKER_FLAGS_DEBUGOPT
)

# Update the documentation string of CMAKE_BUILD_TYPE for GUIs
SET(CMAKE_BUILD_TYPE "${CMAKE_BUILD_TYPE}" CACHE STRING
    "Choose the type of build, options are: \
None Debug DebugOpt Release RelWithDebInfo MinSizeRel."
    FORCE)

set(
  PYRO_COMPILE_OPTIONS
  -Wall
  -Werror
  -Wextra
  -pedantic)

# Define the `cpython` target
set(
  CPYTHON_MODIFIED_SOURCES
  ${CMAKE_SOURCE_DIR}/capi/cpython-data.h
  ${CMAKE_SOURCE_DIR}/capi/cpython-func.h
  ${CMAKE_SOURCE_DIR}/capi/cpython-types.h
  ${CMAKE_SOURCE_DIR}/ext/Objects/abstract.cpp
  ${CMAKE_SOURCE_DIR}/ext/Objects/boolobject.cpp
  ${CMAKE_SOURCE_DIR}/ext/Objects/bytearrayobject.cpp
  ${CMAKE_SOURCE_DIR}/ext/Objects/bytesobject.cpp
  ${CMAKE_SOURCE_DIR}/ext/Objects/capsule.cpp
  ${CMAKE_SOURCE_DIR}/ext/Objects/classobject.cpp
  ${CMAKE_SOURCE_DIR}/ext/Objects/codeobject.cpp
  ${CMAKE_SOURCE_DIR}/ext/Objects/complexobject.cpp
  ${CMAKE_SOURCE_DIR}/ext/Objects/descrobject.cpp
  ${CMAKE_SOURCE_DIR}/ext/Objects/dictobject.cpp
  ${CMAKE_SOURCE_DIR}/ext/Objects/exceptions.cpp
  ${CMAKE_SOURCE_DIR}/ext/Objects/fileobject.cpp
  ${CMAKE_SOURCE_DIR}/ext/Objects/floatobject.cpp
  ${CMAKE_SOURCE_DIR}/ext/Objects/frameobject.cpp
  ${CMAKE_SOURCE_DIR}/ext/Objects/funcobject.cpp
  ${CMAKE_SOURCE_DIR}/ext/Objects/iterobject.cpp
  ${CMAKE_SOURCE_DIR}/ext/Objects/listobject.cpp
  ${CMAKE_SOURCE_DIR}/ext/Objects/longobject.cpp
  ${CMAKE_SOURCE_DIR}/ext/Objects/memoryobject.cpp
  ${CMAKE_SOURCE_DIR}/ext/Objects/methodobject.cpp
  ${CMAKE_SOURCE_DIR}/ext/Objects/moduleobject.cpp
  ${CMAKE_SOURCE_DIR}/ext/Objects/object.cpp
  ${CMAKE_SOURCE_DIR}/ext/Objects/obmalloc.cpp
  ${CMAKE_SOURCE_DIR}/ext/Objects/odictobject.cpp
  ${CMAKE_SOURCE_DIR}/ext/Objects/setobject.cpp
  ${CMAKE_SOURCE_DIR}/ext/Objects/sliceobject.cpp
  ${CMAKE_SOURCE_DIR}/ext/Objects/structseq.cpp
  ${CMAKE_SOURCE_DIR}/ext/Objects/tupleobject.cpp
  ${CMAKE_SOURCE_DIR}/ext/Objects/typeobject.cpp
  ${CMAKE_SOURCE_DIR}/ext/Objects/unicodeobject.cpp
  ${CMAKE_SOURCE_DIR}/ext/Objects/weakrefobject.cpp
  ${CMAKE_SOURCE_DIR}/ext/Python/_warnings.cpp
  ${CMAKE_SOURCE_DIR}/ext/Python/ceval.cpp
  ${CMAKE_SOURCE_DIR}/ext/Python/codecs.cpp
  ${CMAKE_SOURCE_DIR}/ext/Python/errors.cpp
  ${CMAKE_SOURCE_DIR}/ext/Python/fileutils.cpp
  ${CMAKE_SOURCE_DIR}/ext/Python/getargs.cpp
  ${CMAKE_SOURCE_DIR}/ext/Python/getcompiler.cpp
  ${CMAKE_SOURCE_DIR}/ext/Python/getcopyright.cpp
  ${CMAKE_SOURCE_DIR}/ext/Python/getplatform.cpp
  ${CMAKE_SOURCE_DIR}/ext/Python/getversion.cpp
  ${CMAKE_SOURCE_DIR}/ext/Python/import.cpp
  ${CMAKE_SOURCE_DIR}/ext/Python/modsupport.cpp
  ${CMAKE_SOURCE_DIR}/ext/Python/mysnprintf.cpp
  ${CMAKE_SOURCE_DIR}/ext/Python/mystrtoul.cpp
  ${CMAKE_SOURCE_DIR}/ext/Python/pathconfig.cpp
  ${CMAKE_SOURCE_DIR}/ext/Python/pyarena.cpp
  ${CMAKE_SOURCE_DIR}/ext/Python/pylifecycle.cpp
  ${CMAKE_SOURCE_DIR}/ext/Python/pystate.cpp
  ${CMAKE_SOURCE_DIR}/ext/Python/pystrcmp.cpp
  ${CMAKE_SOURCE_DIR}/ext/Python/pystrtod.cpp
  ${CMAKE_SOURCE_DIR}/ext/Python/pythonrun.cpp
  ${CMAKE_SOURCE_DIR}/ext/Python/sysmodule.cpp
  ${CMAKE_SOURCE_DIR}/ext/Python/traceback.cpp)
include(CPythonBuild.cmake)

add_library(capi-headers INTERFACE)
target_sources(
  capi-headers
  INTERFACE
  ${CPYTHON_MODIFIED_HEADERS}
)
target_include_directories(
  capi-headers
  INTERFACE
  capi)

set(
  FROZEN_MODULE_INPUT
  library/builtins.py)
set(
  FROZEN_MODULE_OUTPUT_DIR
  ${CMAKE_BINARY_DIR}/bootstrap)
set(
  FROZEN_MODULE_OUTPUT
  ${FROZEN_MODULE_OUTPUT_DIR}/frozen-modules.cpp
  ${FROZEN_MODULE_OUTPUT_DIR}/frozen-modules.h)
add_custom_command(
  OUTPUT
  ${FROZEN_MODULE_OUTPUT}
  COMMAND
  util/freeze_modules.py ${FROZEN_MODULE_OUTPUT_DIR} ${FROZEN_MODULE_INPUT}
  WORKING_DIRECTORY
  ${CMAKE_SOURCE_DIR}
  DEPENDS util/freeze_modules.py ${FROZEN_MODULE_INPUT})
add_custom_target(
  frozen-sources
  DEPENDS
  ${FROZEN_MODULE_OUTPUT})
add_library(
  frozen-modules
  STATIC
  ${FROZEN_MODULE_OUTPUT})
target_include_directories(
  frozen-modules
  INTERFACE
  ${FROZEN_MODULE_OUTPUT_DIR})

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
  set(RUNTIME_OS_SOURCES runtime/os-darwin.cpp)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set(RUNTIME_OS_SOURCES runtime/os-linux.cpp)
endif()
add_library(
  runtime
  STATIC
  runtime/builtins-module.cpp
  runtime/builtins-module.h
  runtime/bytecode.cpp
  runtime/bytecode.h
  runtime/callback.h
  runtime/capi-handles.cpp
  runtime/capi-handles.h
  runtime/complex-builtins.cpp
  runtime/complex-builtins.h
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
  runtime/generator-builtins.cpp
  runtime/globals.h
  runtime/handles.cpp
  runtime/handles.h
  runtime/heap.cpp
  runtime/heap.h
  runtime/imp-module.cpp
  runtime/imp-module.h
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
  runtime/range-builtins.cpp
  runtime/range-builtins.h
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
  runtime/visitor.h
  ${RUNTIME_OS_SOURCES})
target_compile_options(
  runtime
  PRIVATE
  ${PYRO_COMPILE_OPTIONS})
target_link_libraries(
  runtime
  PUBLIC
  capi-headers
  PRIVATE
  frozen-modules
  dl
  siphash)

add_library(
  extension
  STATIC
  ${CPYTHON_MODIFIED_SOURCES})
target_compile_options(
  extension
  PUBLIC
  ${OS_DEFINE}
  PRIVATE
  ${PYRO_COMPILE_OPTIONS})
target_include_directories(
  extension
  PUBLIC
  runtime)
target_link_libraries(
  extension
  PUBLIC
  cpython
  runtime)

add_library(
  modules
  STATIC
  ext/Modules/config.cpp)
target_compile_options(
  modules
  PUBLIC
  ${OS_DEFINE})
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
  runtime/capi-handles-test.cpp
  runtime/complex-builtins-test.cpp
  runtime/descriptor-builtins-test.cpp
  runtime/dict-builtins-test.cpp
  runtime/exception-builtins-test.cpp
  runtime/float-builtins-test.cpp
  runtime/generator-test.cpp
  runtime/handles-test.cpp
  runtime/heap-test.cpp
  runtime/imp-module-test.cpp
  runtime/int-builtins-test.cpp
  runtime/interpreter-test.cpp
  runtime/layout-test.cpp
  runtime/list-builtins-test.cpp
  runtime/marshal-test.cpp
  runtime/object-builtins-test.cpp
  runtime/objects-test.cpp
  runtime/os-test.cpp
  runtime/range-builtins-test.cpp
  runtime/ref-builtins-test.cpp
  runtime/runtime-test.cpp
  runtime/scavenger-test.cpp
  runtime/set-builtins-test.cpp
  runtime/space-test.cpp
  runtime/str-builtins-test.cpp
  runtime/super-builtins-test.cpp
  runtime/sys-module-test.cpp
  runtime/test-utils-test.cpp
  runtime/test-utils.cpp
  runtime/test-utils.h
  runtime/thread-module-test.cpp
  runtime/thread-test.cpp
  runtime/time-module-test.cpp
  runtime/trampolines-test.cpp
  runtime/tuple-builtins-test.cpp
  runtime/type-builtins-test.cpp
  runtime/utils-test.cpp
  runtime/vector-test.cpp
  runtime/warnings-module-test.cpp)
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
  ext/Objects/classobject-test.cpp
  ext/Objects/dictobject-test.cpp
  ext/Objects/floatobject-test.cpp
  ext/Objects/listobject-test.cpp
  ext/Objects/longobject-test.cpp
  ext/Objects/moduleobject-test.cpp
  ext/Objects/object-test.cpp
  ext/Objects/setobject-test.cpp
  ext/Objects/tupleobject-test.cpp
  ext/Objects/typeobject-test.cpp
  ext/Objects/unicodeobject-test.cpp
  ext/Python/errors-test.cpp
  ext/Python/getargs-test.cpp
  ext/Python/import-test.cpp
  ext/Python/modsupport-test.cpp
  ext/test/capi-fixture.h
  ext/test/capi-testing-test.cpp
  ext/test/capi-testing.cpp
  ext/test/capi-testing.h)
target_include_directories(
  extension-tests
  PRIVATE
  runtime
  ext/test)
target_link_libraries(
  extension-tests
  PUBLIC
  benchmark
  gtest
  PRIVATE
  cpython
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
