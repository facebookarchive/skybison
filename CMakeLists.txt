cmake_minimum_required(VERSION 3.5)
project(python)

# Append flags set by util/*.cmake files
# Note that setting CMAKE_C_FLAGS directly in util/*.cmake is problematic
# because using `-DCMAKE_C_FLAGS=` would unintentionally override the platform
# flags.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${PLATFORM_COMPILER_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${PLATFORM_COMPILER_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${PLATFORM_LINKER_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS
    "${CMAKE_SHARED_LINKER_FLAGS} ${PLATFORM_LINKER_FLAGS}")

# Add -DPYRO_ASAN=1 to the cmake command line to enable asan.
if (${PYRO_ASAN})
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
  if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ldl -lpthread -lrt")
  endif()
endif()

option(PYRO_STATIC "Link all dependencies statically" OFF)
if (PYRO_STATIC)
  set(PYRO_STATIC_LINKING "-static")
endif()
option(PYRO_STATIC_LIBSTDCXX "Link libstdc++ statically" OFF)
if (PYRO_STATIC_LIBSTDCXX)
  set(PYRO_STATIC_LINKING "-static-libstdc++")
endif()

set(BENCHMARK_ENABLE_TESTING CACHE BOOL OFF)
set(BENCHMARK_ENABLE_EXCEPTIONS CACHE BOOL OFF)
set(BENCHMARK_ENABLE_INSTALL CACHE BOOL OFF)
add_subdirectory(third-party/benchmark-1.3.0)
add_subdirectory(third-party/googletest-1.8.1)
add_subdirectory(third-party/siphash-20170224)
add_subdirectory(third-party/valgrind-3.15.0)

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
  set(OS_DEFINE "-DOS_OSX")
  set(WHOLE_ARCHIVE -Wl,-force_load)
  set(NO_WHOLE_ARCHIVE)
  set(OPENSSL_ROOT_DIR /opt/homebrew)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set(OS_DEFINE "-DOS_LINUX")
  set(WHOLE_ARCHIVE -Wl,--whole-archive)
  set(NO_WHOLE_ARCHIVE -Wl,--no-whole-archive)
  # This is the path to OpenSSL on machines with gvfs. On other Linux systems,
  # CMake should automatically fall back to the system OpenSSL.
  set(OPENSSL_ROOT_DIR /mnt/gvfs/third-party2/openssl/08b454ad6354c8144da2168f7a333624e85bb30b/1.1.0/platform007/ca4da3d)
else()
  message("Unknown CMAKE_SYSTEM_NAME " ${CMAKE_SYSTEM_NAME})
endif()

find_package(OpenSSL REQUIRED)

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
  -Wno-missing-braces
  -Wno-missing-field-initializers
  -Wpedantic
  -Wshadow
  -fno-exceptions
  -fno-omit-frame-pointer
  -fno-optimize-sibling-calls
  -fno-rtti
)

# Define the `cpython` target
set(
  CPYTHON_MODIFIED_SOURCES
  ${EXTENSION_API_HEADERS}
  ${CMAKE_SOURCE_DIR}/ext/Modules/_compile/Python-ast.h
  ${CMAKE_SOURCE_DIR}/ext/Modules/_compile/asdl.h
  ${CMAKE_SOURCE_DIR}/ext/Modules/_compile/ast.h
  ${CMAKE_SOURCE_DIR}/ext/Modules/_compile/compile.cpp
  ${CMAKE_SOURCE_DIR}/ext/Modules/_compile/compile.h
  ${CMAKE_SOURCE_DIR}/ext/Modules/_compile/module.cpp
  ${CMAKE_SOURCE_DIR}/ext/Modules/_compile/node.h
  ${CMAKE_SOURCE_DIR}/ext/Modules/_compile/opcode.h
  ${CMAKE_SOURCE_DIR}/ext/Modules/_compile/peephole.cpp
  ${CMAKE_SOURCE_DIR}/ext/Modules/_compile/symtable.h
  ${CMAKE_SOURCE_DIR}/ext/Modules/_compile/wordcode_helpers.h
  ${CMAKE_SOURCE_DIR}/ext/Modules/gcmodule.cpp
  ${CMAKE_SOURCE_DIR}/ext/Modules/signalmodule.cpp
  ${CMAKE_SOURCE_DIR}/ext/Objects/abstract.cpp
  ${CMAKE_SOURCE_DIR}/ext/Objects/boolobject.cpp
  ${CMAKE_SOURCE_DIR}/ext/Objects/bytearrayobject.cpp
  ${CMAKE_SOURCE_DIR}/ext/Objects/bytesobject.cpp
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
  ${CMAKE_SOURCE_DIR}/ext/Objects/function-utils.cpp
  ${CMAKE_SOURCE_DIR}/ext/Objects/function-utils.h
  ${CMAKE_SOURCE_DIR}/ext/Objects/genobject.cpp
  ${CMAKE_SOURCE_DIR}/ext/Objects/iterobject.cpp
  ${CMAKE_SOURCE_DIR}/ext/Objects/listobject.cpp
  ${CMAKE_SOURCE_DIR}/ext/Objects/longobject.cpp
  ${CMAKE_SOURCE_DIR}/ext/Objects/memoryobject.cpp
  ${CMAKE_SOURCE_DIR}/ext/Objects/methodobject.cpp
  ${CMAKE_SOURCE_DIR}/ext/Objects/moduleobject.cpp
  ${CMAKE_SOURCE_DIR}/ext/Objects/namespaceobject.cpp
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
  ${CMAKE_SOURCE_DIR}/ext/Python/compile.cpp
  ${CMAKE_SOURCE_DIR}/ext/Python/errors.cpp
  ${CMAKE_SOURCE_DIR}/ext/Python/getargs.cpp
  ${CMAKE_SOURCE_DIR}/ext/Python/getcompiler.cpp
  ${CMAKE_SOURCE_DIR}/ext/Python/getcopyright.cpp
  ${CMAKE_SOURCE_DIR}/ext/Python/getplatform.cpp
  ${CMAKE_SOURCE_DIR}/ext/Python/getversion.cpp
  ${CMAKE_SOURCE_DIR}/ext/Python/import.cpp
  ${CMAKE_SOURCE_DIR}/ext/Python/modsupport-internal.h
  ${CMAKE_SOURCE_DIR}/ext/Python/modsupport.cpp
  ${CMAKE_SOURCE_DIR}/ext/Python/mysnprintf.cpp
  ${CMAKE_SOURCE_DIR}/ext/Python/pathconfig.cpp
  ${CMAKE_SOURCE_DIR}/ext/Python/pylifecycle.cpp
  ${CMAKE_SOURCE_DIR}/ext/Python/pystate.cpp
  ${CMAKE_SOURCE_DIR}/ext/Python/pythonrun.cpp
  ${CMAKE_SOURCE_DIR}/ext/Python/random.cpp
  ${CMAKE_SOURCE_DIR}/ext/Python/sysmodule.cpp
  ${CMAKE_SOURCE_DIR}/ext/Python/thread.cpp
  ${CMAKE_SOURCE_DIR}/ext/Python/traceback.cpp
)
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
  library/_builtins.py
  library/_codecs.py
  library/_imp.py
  library/_io.py
  library/_namedtuple.py
  library/_os.py
  library/_str_mod.py
  library/_thread.py
  library/_valgrind.py
  library/_warnings.py
  library/_weakref.py
  library/builtins.py
  library/faulthandler.py
  library/importlib/_bootstrap.py
  library/importlib/_bootstrap_external.py
  library/marshal.py
  library/operator.py
  library/sys.py
  library/warnings.py)
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
# This target is built in util/run_clang_tidy.py:create_compile_commands() to
# ensure all generated files are present when clang-tidy runs.
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
  set(
    RUNTIME_OS_SOURCES
    runtime/file-darwin.cpp
    runtime/os-darwin.cpp
    runtime/mutex-darwin.cpp)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set(
    RUNTIME_OS_SOURCES
    runtime/file-linux.cpp
    runtime/os-linux.cpp
    runtime/mutex-linux.cpp)
endif()

if ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")
  set(
    RUNTIME_ARCH_SOURCES
    runtime/interpreter-gen-x64.cpp
  )
else()
  message(FATAL_ERROR "Unsupported architecture ${CMAKE_SYSTEM_PROCESSOR}")
endif()

add_library(
  runtime
  STATIC
  runtime/assembler-utils.cpp
  runtime/assembler-utils.h
  runtime/assembler-x64.cpp
  runtime/assembler-x64.h
  runtime/builtins-module.cpp
  runtime/builtins-module.h
  runtime/bytearray-builtins.cpp
  runtime/bytearray-builtins.h
  runtime/bytecode.cpp
  runtime/bytecode.h
  runtime/bytes-builtins.cpp
  runtime/bytes-builtins.h
  runtime/callback.h
  runtime/capi-handles.cpp
  runtime/capi-handles.h
  runtime/code-builtins.cpp
  runtime/code-builtins.h
  runtime/codecs-module.cpp
  runtime/codecs-module.h
  runtime/complex-builtins.cpp
  runtime/complex-builtins.h
  runtime/debugging.cpp
  runtime/debugging.h
  runtime/descriptor-builtins.cpp
  runtime/descriptor-builtins.h
  runtime/dict-builtins.cpp
  runtime/dict-builtins.h
  runtime/exception-builtins.cpp
  runtime/exception-builtins.h
  runtime/faulthandler-module.cpp
  runtime/faulthandler-module.h
  runtime/file.h
  runtime/float-builtins.cpp
  runtime/float-builtins.h
  runtime/float-conversion.cpp
  runtime/float-conversion.h
  runtime/formatter.cpp
  runtime/formatter.h
  runtime/frame.cpp
  runtime/frame.h
  runtime/function-builtins.cpp
  runtime/generator-builtins.cpp
  runtime/globals.h
  runtime/handles.cpp
  runtime/handles.h
  runtime/heap.cpp
  runtime/heap.h
  runtime/ic.cpp
  runtime/ic.h
  runtime/imp-module.cpp
  runtime/imp-module.h
  runtime/int-builtins.cpp
  runtime/int-builtins.h
  runtime/interpreter-gen.h
  runtime/interpreter.cpp
  runtime/interpreter.h
  runtime/intrinsic.cpp
  runtime/intrinsic.h
  runtime/iterator-builtins.cpp
  runtime/iterator-builtins.h
  runtime/io-module.cpp
  runtime/io-module.h
  runtime/layout.h
  runtime/list-builtins.cpp
  runtime/list-builtins.h
  runtime/marshal-module.cpp
  runtime/marshal-module.h
  runtime/marshal.cpp
  runtime/marshal.h
  runtime/memory-region.cpp
  runtime/memory-region.h
  runtime/memoryview-builtins.cpp
  runtime/memoryview-builtins.h
  runtime/module-builtins.cpp
  runtime/module-builtins.h
  runtime/mutex.h
  runtime/mro.cpp
  runtime/mro.h
  runtime/namedtuple-module.cpp
  runtime/namedtuple-module.h
  runtime/object-builtins.cpp
  runtime/objects.cpp
  runtime/objects.h
  runtime/operator-module.cpp
  runtime/operator-module.h
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
  runtime/slice-builtins.cpp
  runtime/slice-builtins.h
  runtime/space.cpp
  runtime/space.h
  runtime/str-builtins.cpp
  runtime/str-builtins.h
  runtime/strarray-builtins.cpp
  runtime/strarray-builtins.h
  runtime/super-builtins.cpp
  runtime/super-builtins.h
  runtime/symbols.cpp
  runtime/symbols.h
  runtime/sys-module.cpp
  runtime/sys-module.h
  runtime/thread.cpp
  runtime/thread.h
  runtime/trampolines.cpp
  runtime/trampolines.h
  runtime/tuple-builtins.cpp
  runtime/tuple-builtins.h
  runtime/type-builtins.cpp
  runtime/under-builtins-module.cpp
  runtime/under-builtins-module.h
  runtime/under-os-module.cpp
  runtime/under-os-module.h
  runtime/under-str-mod-module.cpp
  runtime/under-str-mod-module.h
  runtime/under-thread-module.cpp
  runtime/under-thread-module.h
  runtime/under-valgrind-module.cpp
  runtime/under-valgrind-module.h
  runtime/unicode.h
  runtime/utils.cpp
  runtime/utils.h
  runtime/vector.h
  runtime/view.h
  runtime/visitor.h
  runtime/warnings-module.cpp
  runtime/warnings-module.h
  runtime/weakref-module.cpp
  runtime/weakref-module.h
  ${RUNTIME_OS_SOURCES}
  ${RUNTIME_ARCH_SOURCES})
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
  siphash
  valgrind)
# TODO(T42692043): This awkwardness should go away once we freeze the stdlib.
target_compile_definitions(runtime PRIVATE PYRO_BASEDIR="${CMAKE_SOURCE_DIR}")

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
  PRIVATE
  runtime)
target_link_libraries(
  extension
  PUBLIC
  runtime
  PRIVATE
  cpython)

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
  cpython)

add_executable(
  python
  main.cpp)
target_link_libraries(
  python
  PRIVATE
  runtime
  modules
  ${PYRO_STATIC_LINKING})

add_executable(
  test_lldb_support
  EXCLUDE_FROM_ALL
  util/test_lldb_support.cpp)
target_link_libraries(
  test_lldb_support
  PRIVATE
  runtime
  modules)
target_include_directories(
  test_lldb_support
  PRIVATE
  runtime)

add_library(
  runtime-tests
  STATIC
  runtime/assembler-x64-test.cpp
  runtime/builtins-module-test.cpp
  runtime/bytearray-builtins-test.cpp
  runtime/bytecode-test.cpp
  runtime/bytes-builtins-test.cpp
  runtime/capi-handles-test.cpp
  runtime/code-builtins-test.cpp
  runtime/codecs-module-test.cpp
  runtime/complex-builtins-test.cpp
  runtime/debugging-test.cpp
  runtime/descriptor-builtins-test.cpp
  runtime/dict-builtins-test.cpp
  runtime/exception-builtins-test.cpp
  runtime/faulthandler-module-test.cpp
  runtime/float-builtins-test.cpp
  runtime/float-conversion-test.cpp
  runtime/function-builtins-test.cpp
  runtime/generator-test.cpp
  runtime/handles-test.cpp
  runtime/heap-test.cpp
  runtime/ic-test.cpp
  runtime/imp-module-test.cpp
  runtime/importlib-test.cpp
  runtime/int-builtins-test.cpp
  runtime/interpreter-test.cpp
  runtime/io-module-test.cpp
  runtime/layout-test.cpp
  runtime/list-builtins-test.cpp
  runtime/marshal-module-test.cpp
  runtime/marshal-test.cpp
  runtime/memoryview-builtins-test.cpp
  runtime/memory-region-test.cpp
  runtime/module-builtins-test.cpp
  runtime/mutex-test.cpp
  runtime/mro-test.cpp
  runtime/namedtuple-module-test.cpp
  runtime/object-builtins-test.cpp
  runtime/objects-test.cpp
  runtime/os-test.cpp
  runtime/range-builtins-test.cpp
  runtime/ref-builtins-test.cpp
  runtime/runtime-test.cpp
  runtime/scavenger-test.cpp
  runtime/set-builtins-test.cpp
  runtime/slice-builtins-test.cpp
  runtime/space-test.cpp
  runtime/str-builtins-test.cpp
  runtime/strarray-builtins-test.cpp
  runtime/super-builtins-test.cpp
  runtime/sys-module-test.cpp
  runtime/test-utils-test.cpp
  runtime/test-utils.cpp
  runtime/test-utils.h
  runtime/thread-test.cpp
  runtime/trampolines-test.cpp
  runtime/tuple-builtins-test.cpp
  runtime/type-builtins-test.cpp
  runtime/under-builtins-module-test.cpp
  runtime/under-os-module-test.cpp
  runtime/under-valgrind-module-test.cpp
  runtime/utils-test.cpp
  runtime/vector-test.cpp
  runtime/warnings-module-test.cpp)
target_compile_options(
  runtime-tests
  PRIVATE
  ${PYRO_COMPILE_OPTIONS})
target_link_libraries(
  runtime-tests
  PRIVATE
  benchmark
  gmock
  gtest
  runtime
  modules)

set(
  EXTENSION_TESTS
  ext/Modules/config-test.cpp
  ext/Modules/gcmodule-test.cpp
  ext/Modules/posixmodule-test.cpp
  ext/Objects/abstract-test.cpp
  ext/Objects/boolobject-test.cpp
  ext/Objects/bytearrayobject-test.cpp
  ext/Objects/bytesobject-test.cpp
  ext/Objects/classobject-test.cpp
  ext/Objects/codeobject-test.cpp
  ext/Objects/complexobject-test.cpp
  ext/Objects/descrobject-test.cpp
  ext/Objects/dictobject-test.cpp
  ext/Objects/exceptions-test.cpp
  ext/Objects/fileobject-test.cpp
  ext/Objects/floatobject-test.cpp
  ext/Objects/frameobject-test.cpp
  ext/Objects/funcobject-test.cpp
  ext/Objects/genobject-test.cpp
  ext/Objects/iterobject-test.cpp
  ext/Objects/listobject-test.cpp
  ext/Objects/longobject-test.cpp
  ext/Objects/memoryobject-test.cpp
  ext/Objects/methodobject-test.cpp
  ext/Objects/moduleobject-test.cpp
  ext/Objects/namespaceobject-test.cpp
  ext/Objects/object-test.cpp
  ext/Objects/obmalloc-test.cpp
  ext/Objects/setobject-test.cpp
  ext/Objects/sliceobject-test.cpp
  ext/Objects/structseq-test.cpp
  ext/Objects/tupleobject-test.cpp
  ext/Objects/typeobject-test.cpp
  ext/Objects/unicodeobject-test.cpp
  ext/Objects/weakrefobject-test.cpp
  ext/Python/ceval-test.cpp
  ext/Python/codecs-test.cpp
  ext/Python/compile-test.cpp
  ext/Python/errors-test.cpp
  ext/Python/getargs-ssizet-test.cpp
  ext/Python/getargs-test.cpp
  ext/Python/getversion-test.cpp
  ext/Python/import-test.cpp
  ext/Python/modsupport-test.cpp
  ext/Python/mysnprintf-test.cpp
  ext/Python/pylifecycle-test.cpp
  ext/Python/pystate-test.cpp
  ext/Python/pythonrun-test.cpp
  ext/Python/random-test.cpp
  ext/Python/sysmodule-test.cpp
  ext/Python/thread-test.cpp
  ext/Python/_warnings-test.cpp
  ext/test/capi-fixture.h
  ext/test/capi-testing-test.cpp
  ext/test/capi-testing.cpp
  ext/test/capi-testing.h)
add_library(
  extension-tests
  STATIC
  ${EXTENSION_TESTS})
target_compile_options(
  extension-tests
  PRIVATE
  ${PYRO_COMPILE_OPTIONS})
target_include_directories(
  extension-tests
  PRIVATE
  ext/test)
target_link_libraries(
  extension-tests
  PRIVATE
  benchmark
  gmock
  gtest
  cpython
  modules)

add_executable(
  python-tests
  runtime/python-test.cpp)
target_compile_options(
  python-tests
  PRIVATE
  ${PYRO_COMPILE_OPTIONS})
target_link_libraries(
  python-tests
  PRIVATE
  benchmark
  gtest
  ${WHOLE_ARCHIVE} runtime-tests ${NO_WHOLE_ARCHIVE}
  ${WHOLE_ARCHIVE} extension-tests ${NO_WHOLE_ARCHIVE}
  runtime
  modules)

# Extension tests with CPython
set(CPYTHON_ASSERTIONS_FLAG "")
set(CPYTHON_CFLAGS "")
set(CPYTHON_LDFLAGS "")
set(CPYTHON_OPENSSL_FLAG "--with-openssl=${OPENSSL_ROOT_DIR}")
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set(CPYTHON_LDFLAGS "LDFLAGS='-Wl,-rpath=${OPENSSL_ROOT_DIR}/lib'")
endif()
if (${CMAKE_BUILD_TYPE} MATCHES "^Debug")
  set(CPYTHON_ASSERTIONS_FLAG "--with-assertions")
  set(CPYTHON_CFLAGS "CFLAGS=-O0")
endif()
set(LIBPYTHON_BIN "libpython3.6m.a")
set(CPYTHON_BUILD_DIR ${CMAKE_BINARY_DIR}/third-party/cpython)
file(MAKE_DIRECTORY ${CPYTHON_BUILD_DIR})
file(GLOB_RECURSE CPYTHON_FILES ${CPYTHON_DIR}/*)
add_custom_command(
  OUTPUT ${CPYTHON_BUILD_DIR}/${LIBPYTHON_BIN}
  COMMAND ${CPYTHON_CFLAGS} ${CPYTHON_DIR}/configure ${CPYTHON_OPENSSL_FLAG}
    ${CPYTHON_LDFLAGS} ${CPYTHON_ASSERTIONS_FLAG}
  COMMAND make -j
  DEPENDS ${CPYTHON_FILES}
  WORKING_DIRECTORY ${CPYTHON_BUILD_DIR})
add_custom_target(
  compile-cpython
  DEPENDS ${CPYTHON_BUILD_DIR}/${LIBPYTHON_BIN})
add_library(libpython STATIC IMPORTED)
add_dependencies(libpython compile-cpython)
set_target_properties(
  libpython
  PROPERTIES IMPORTED_LOCATION ${CPYTHON_BUILD_DIR}/${LIBPYTHON_BIN})

add_library(
  cpython-extension-tests
  STATIC
  EXCLUDE_FROM_ALL
  ${EXTENSION_TESTS})
target_compile_options(
  cpython-extension-tests
  PUBLIC
  ${OS_DEFINE}
  PRIVATE
  ${PYRO_COMPILE_OPTIONS})

target_include_directories(
  cpython-extension-tests
  PRIVATE
  ext/test
  ext/config
  third-party/cpython/Include)
target_link_libraries(
  cpython-extension-tests
  PRIVATE
  benchmark
  gmock
  gtest
  libpython)

add_executable(
  cpython-tests
  EXCLUDE_FROM_ALL
  runtime/python-test.cpp)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set(CPYTHON_LINK_FLAGS -lpthread -ldl -lutil -lm -Xlinker -export-dynamic)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
  set(CPYTHON_LINK_FLAGS -ldl)
else()
  message(FATAL_ERROR "Unsupported system")
endif()
target_link_libraries(
  cpython-tests
  PRIVATE
  benchmark
  gtest
  ${WHOLE_ARCHIVE} cpython-extension-tests ${NO_WHOLE_ARCHIVE}
  ${CPYTHON_LINK_FLAGS})
# Lib/ is needed for cpython-test, copy it into the build directory
add_custom_command(
  TARGET cpython-tests
  POST_BUILD
  COMMAND cp -a ${CPYTHON_DIR}/Lib/. ${CPYTHON_BUILD_DIR}/Lib/)
