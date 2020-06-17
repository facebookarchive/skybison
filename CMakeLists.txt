cmake_minimum_required(VERSION 3.5)
project(python LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_TOOLCHAIN_FILE AND NOT SKIP_TOOLCHAIN_CHECK)
  message(SEND_ERROR "
")
  file(GLOB TOOLCHAIN_FILES RELATIVE ${CMAKE_SOURCE_DIR} util/*.cmake)
  list(FILTER TOOLCHAIN_FILES EXCLUDE REGEX util/.*-common\.cmake)
  string(REPLACE ";" "\n    " TOOLCHAIN_FILES "${TOOLCHAIN_FILES}")
  message(FATAL_ERROR "

Error: This project requires a toolchain file.
Please specify CMAKE_TOOLCHAIN_FILE. Example:

  $ mkdir build
  $ cd build
  $ cmake -GNinja -D CMAKE_TOOLCHAIN_FILE=../util/platform007-clang.cmake ..

Available toolchain files:

    ${TOOLCHAIN_FILES}
")
endif()

# Append flags set by util/*.cmake files.
# Note that setting CMAKE_C_FLAGS directly in util/*.cmake is problematic
# because using `-DCMAKE_C_FLAGS=` would unintentionally override the platform
# flags.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread ${PLATFORM_COMPILER_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread ${PLATFORM_COMPILER_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${PLATFORM_LINKER_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS
    "${CMAKE_SHARED_LINKER_FLAGS} ${PLATFORM_LINKER_FLAGS}")

# Add an option to disable cpython-tests to avoid source files getting compiled
# for two targets and confusing the compilation database used by clang-tidy.
option(ENABLE_CPYTHON_TESTS "Add cpython-tests target" ON)

# Add -DPYRO_ASAN=1 to the cmake command line to enable asan.
if (${PYRO_ASAN})
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
endif()

set(BENCHMARK_ENABLE_TESTING CACHE BOOL OFF)
set(BENCHMARK_ENABLE_EXCEPTIONS CACHE BOOL OFF)
set(BENCHMARK_ENABLE_INSTALL CACHE BOOL OFF)
add_subdirectory(third-party/benchmark-1.3.0)
add_subdirectory(third-party/googletest-1.8.1)
add_subdirectory(third-party/siphash-20170224)
add_subdirectory(third-party/valgrind-3.15.0)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
  set(WARN_MISSING_DECLARATIONS -Wmissing-prototypes)
else()
  set(WARN_MISSING_DECLARATIONS -Wmissing-declarations)
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  set(OS_DEFINE "-DOS_OSX" CACHE STRING "")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(OS_DEFINE "-DOS_LINUX" CACHE STRING "")
else()
  message("Unknown CMAKE_SYSTEM_NAME " ${CMAKE_SYSTEM_NAME})
endif()

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_VISIBILITY_PRESET hidden)

set(CMAKE_CXX_EXTENSIONS NO)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

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
    CMAKE_SHARED_LINKER_FLAGS_DEBUGOPT)
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
  "Debug" "DebugOpt" "Release" "RelWithDebInfo" "MinSizeRel")

# Set a default for CMAKE_BUILD_TYPE.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY VALUE "Debug")
endif()

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
  -fno-rtti)

set(
  C_API_HEADERS
  ${CMAKE_SOURCE_DIR}/capi/Python.h
  ${CMAKE_SOURCE_DIR}/capi/abstract.h
  ${CMAKE_SOURCE_DIR}/capi/bltinmodule.h
  ${CMAKE_SOURCE_DIR}/capi/boolobject.h
  ${CMAKE_SOURCE_DIR}/capi/bytearrayobject.h
  ${CMAKE_SOURCE_DIR}/capi/bytesobject.h
  ${CMAKE_SOURCE_DIR}/capi/cellobject.h
  ${CMAKE_SOURCE_DIR}/capi/ceval.h
  ${CMAKE_SOURCE_DIR}/capi/classobject.h
  ${CMAKE_SOURCE_DIR}/capi/code.h
  ${CMAKE_SOURCE_DIR}/capi/codecs.h
  ${CMAKE_SOURCE_DIR}/capi/compile.h
  ${CMAKE_SOURCE_DIR}/capi/complexobject.h
  ${CMAKE_SOURCE_DIR}/capi/cpython-data.h
  ${CMAKE_SOURCE_DIR}/capi/cpython-func.h
  ${CMAKE_SOURCE_DIR}/capi/cpython-types.h
  ${CMAKE_SOURCE_DIR}/capi/datetime.h
  ${CMAKE_SOURCE_DIR}/capi/descrobject.h
  ${CMAKE_SOURCE_DIR}/capi/dictobject.h
  ${CMAKE_SOURCE_DIR}/capi/dtoa.h
  ${CMAKE_SOURCE_DIR}/capi/enumobject.h
  ${CMAKE_SOURCE_DIR}/capi/eval.h
  ${CMAKE_SOURCE_DIR}/capi/fileobject.h
  ${CMAKE_SOURCE_DIR}/capi/fileutils.h
  ${CMAKE_SOURCE_DIR}/capi/floatobject.h
  ${CMAKE_SOURCE_DIR}/capi/funcobject.h
  ${CMAKE_SOURCE_DIR}/capi/genobject.h
  ${CMAKE_SOURCE_DIR}/capi/import.h
  ${CMAKE_SOURCE_DIR}/capi/intrcheck.h
  ${CMAKE_SOURCE_DIR}/capi/iterobject.h
  ${CMAKE_SOURCE_DIR}/capi/listobject.h
  ${CMAKE_SOURCE_DIR}/capi/longintrepr.h
  ${CMAKE_SOURCE_DIR}/capi/longobject.h
  ${CMAKE_SOURCE_DIR}/capi/memoryobject.h
  ${CMAKE_SOURCE_DIR}/capi/methodobject.h
  ${CMAKE_SOURCE_DIR}/capi/modsupport.h
  ${CMAKE_SOURCE_DIR}/capi/moduleobject.h
  ${CMAKE_SOURCE_DIR}/capi/namespaceobject.h
  ${CMAKE_SOURCE_DIR}/capi/object.h
  ${CMAKE_SOURCE_DIR}/capi/objimpl.h
  ${CMAKE_SOURCE_DIR}/capi/odictobject.h
  ${CMAKE_SOURCE_DIR}/capi/osmodule.h
  ${CMAKE_SOURCE_DIR}/capi/patchlevel.h
  ${CMAKE_SOURCE_DIR}/capi/pyarena.h
  ${CMAKE_SOURCE_DIR}/capi/pyatomic.h
  ${CMAKE_SOURCE_DIR}/capi/pycapsule.h
  ${CMAKE_SOURCE_DIR}/capi/pyconfig-linux.h
  ${CMAKE_SOURCE_DIR}/capi/pyconfig-osx.h
  ${CMAKE_SOURCE_DIR}/capi/pyconfig.h
  ${CMAKE_SOURCE_DIR}/capi/pyctype.h
  ${CMAKE_SOURCE_DIR}/capi/pydebug.h
  ${CMAKE_SOURCE_DIR}/capi/pyerrors.h
  ${CMAKE_SOURCE_DIR}/capi/pyfpe.h
  ${CMAKE_SOURCE_DIR}/capi/pyhash.h
  ${CMAKE_SOURCE_DIR}/capi/pylifecycle.h
  ${CMAKE_SOURCE_DIR}/capi/pymacconfig.h
  ${CMAKE_SOURCE_DIR}/capi/pymacro.h
  ${CMAKE_SOURCE_DIR}/capi/pymath.h
  ${CMAKE_SOURCE_DIR}/capi/pymem.h
  ${CMAKE_SOURCE_DIR}/capi/pyport.h
  ${CMAKE_SOURCE_DIR}/capi/pystate.h
  ${CMAKE_SOURCE_DIR}/capi/pystrcmp.h
  ${CMAKE_SOURCE_DIR}/capi/pystrtod.h
  ${CMAKE_SOURCE_DIR}/capi/pythonrun.h
  ${CMAKE_SOURCE_DIR}/capi/pythread.h
  ${CMAKE_SOURCE_DIR}/capi/pytime.h
  ${CMAKE_SOURCE_DIR}/capi/rangeobject.h
  ${CMAKE_SOURCE_DIR}/capi/setobject.h
  ${CMAKE_SOURCE_DIR}/capi/sliceobject.h
  ${CMAKE_SOURCE_DIR}/capi/structmember.h
  ${CMAKE_SOURCE_DIR}/capi/structseq.h
  ${CMAKE_SOURCE_DIR}/capi/sysmodule.h
  ${CMAKE_SOURCE_DIR}/capi/traceback.h
  ${CMAKE_SOURCE_DIR}/capi/tupleobject.h
  ${CMAKE_SOURCE_DIR}/capi/typeslots.h
  ${CMAKE_SOURCE_DIR}/capi/unicodeobject.h
  ${CMAKE_SOURCE_DIR}/capi/warnings.h
  ${CMAKE_SOURCE_DIR}/capi/weakrefobject.h)
add_library(capi-headers INTERFACE)
target_sources(
  capi-headers
  INTERFACE
  ${C_API_HEADERS})
target_include_directories(
  capi-headers
  INTERFACE
  capi)

set(BUILD_INCLUDE_DIR "${CMAKE_BINARY_DIR}/Include")
string(
  REGEX
  REPLACE "(${CMAKE_SOURCE_DIR}\/capi\/)" "${BUILD_INCLUDE_DIR}/"
  C_API_HEADERS_OUTPUT
  "${C_API_HEADERS}")
add_custom_command(
  OUTPUT ${C_API_HEADERS_OUTPUT}
  COMMAND
  ${CMAKE_COMMAND} -E make_directory ${BUILD_INCLUDE_DIR}
  COMMAND
  ${CMAKE_COMMAND} -E copy_if_different ${C_API_HEADERS} ${BUILD_INCLUDE_DIR}
  DEPENDS ${C_API_HEADERS}
  COMMENT "Copying C-API headers")
add_custom_target(copy-capi-headers ALL DEPENDS ${C_API_HEADERS_OUTPUT})

set(
  FROZEN_MODULE_INPUT
  library/_builtins.py
  library/_codecs.py
  library/_contextvars.py
  library/_ctypes.py
  library/_frozen_importlib.py
  library/_frozen_importlib_external.py
  library/_imp.py
  library/_io.py
  library/_os.py
  library/_path.py
  library/_signal.py
  library/_str_mod.py
  library/_thread.py
  library/_valgrind.py
  library/_warnings.py
  library/_weakref.py
  library/array.py
  library/builtins.py
  library/faulthandler.py
  library/marshal.py
  library/mmap.py
  library/operator.py
  library/sys.py
  library/unicodedata.py
  third-party/cpython/Lib/warnings.py
  third-party/cpython/Lib/zipimport.py)
set(
  FROZEN_MODULE_OUTPUT_DIR
  ${CMAKE_BINARY_DIR}/bootstrap)
set(
  FROZEN_MODULE_OUTPUT
  ${FROZEN_MODULE_OUTPUT_DIR}/builtins.cpp
  ${FROZEN_MODULE_OUTPUT_DIR}/builtins.h
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
  runtime/array-module.cpp
  runtime/array-module.h
  runtime/assembler-utils.cpp
  runtime/assembler-utils.h
  runtime/assembler-x64.cpp
  runtime/assembler-x64.h
  runtime/asserts.cpp
  runtime/asserts.h
  runtime/builtins-module.cpp
  runtime/builtins-module.h
  runtime/bytearray-builtins.cpp
  runtime/bytearray-builtins.h
  runtime/bytecode.cpp
  runtime/bytecode.h
  runtime/bytes-builtins.cpp
  runtime/bytes-builtins.h
  runtime/capi-handles.cpp
  runtime/capi-handles.h
  runtime/code-builtins.cpp
  runtime/code-builtins.h
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
  runtime/file.h
  runtime/float-builtins.cpp
  runtime/float-builtins.h
  runtime/float-conversion.cpp
  runtime/float-conversion.h
  runtime/formatter.cpp
  runtime/formatter.h
  runtime/frame-proxy-builtins.cpp
  runtime/frame-proxy-builtins.h
  runtime/frame.cpp
  runtime/frame.h
  runtime/function-builtins.cpp
  runtime/generator-builtins.cpp
  runtime/globals.h
  runtime/handles.cpp
  runtime/handles.h
  runtime/heap-profiler.cpp
  runtime/heap-profiler.h
  runtime/heap.cpp
  runtime/heap.h
  runtime/ic.cpp
  runtime/ic.h
  runtime/int-builtins.cpp
  runtime/int-builtins.h
  runtime/interpreter-gen.h
  runtime/interpreter.cpp
  runtime/interpreter.h
  runtime/intrinsic.cpp
  runtime/intrinsic.h
  runtime/iterator-builtins.cpp
  runtime/iterator-builtins.h
  runtime/layout-builtins.cpp
  runtime/layout-builtins.h
  runtime/layout.h
  runtime/list-builtins.cpp
  runtime/list-builtins.h
  runtime/mappingproxy-builtins.cpp
  runtime/mappingproxy-builtins.h
  runtime/marshal-module.cpp
  runtime/marshal.cpp
  runtime/marshal.h
  runtime/memory-region.cpp
  runtime/memory-region.h
  runtime/memoryview-builtins.cpp
  runtime/memoryview-builtins.h
  runtime/mmap-module.cpp
  runtime/mmap-module.h
  runtime/module-builtins.cpp
  runtime/module-builtins.h
  runtime/module-proxy-builtins.cpp
  runtime/module-proxy-builtins.h
  runtime/modules.cpp
  runtime/modules.h
  runtime/mro.cpp
  runtime/mro.h
  runtime/mutex.h
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
  runtime/slice-builtins.cpp
  runtime/slice-builtins.h
  runtime/space.cpp
  runtime/space.h
  runtime/str-builtins.cpp
  runtime/str-builtins.h
  runtime/strarray-builtins.cpp
  runtime/strarray-builtins.h
  runtime/structseq-builtins.cpp
  runtime/structseq-builtins.h
  runtime/super-builtins.cpp
  runtime/super-builtins.h
  runtime/symbols.cpp
  runtime/symbols.h
  runtime/sys-module.cpp
  runtime/sys-module.h
  runtime/thread.cpp
  runtime/thread.h
  runtime/traceback-builtins.cpp
  runtime/traceback-builtins.h
  runtime/trampolines.cpp
  runtime/trampolines.h
  runtime/tuple-builtins.cpp
  runtime/tuple-builtins.h
  runtime/type-builtins.cpp
  runtime/under-builtins-module.cpp
  runtime/under-codecs-module.cpp
  runtime/under-contextvars-module.cpp
  runtime/under-contextvars-module.h
  runtime/under-ctypes-module.cpp
  runtime/under-imp-module.cpp
  runtime/under-imp-module.h
  runtime/under-io-module.cpp
  runtime/under-io-module.h
  runtime/under-os-module.cpp
  runtime/under-path-module.cpp
  runtime/under-signal-module.cpp
  runtime/under-signal-module.h
  runtime/under-thread-module.cpp
  runtime/under-valgrind-module.cpp
  runtime/under-warnings-module.cpp
  runtime/under-weakref-module.cpp
  runtime/unicode-db.cpp
  runtime/unicode-db.h
  runtime/unicode.cpp
  runtime/unicode.h
  runtime/unicodedata-module.cpp
  runtime/utils.cpp
  runtime/utils.h
  runtime/valuecell-builtins.cpp
  runtime/valuecell-builtins.h
  runtime/vector.h
  runtime/version.cpp
  runtime/version.h
  runtime/view.h
  runtime/visitor.h
  ${FROZEN_MODULE_OUTPUT}
  ${RUNTIME_OS_SOURCES}
  ${RUNTIME_ARCH_SOURCES})
target_compile_options(
  runtime
  PRIVATE
  ${PYRO_COMPILE_OPTIONS}
  ${WARN_MISSING_DECLARATIONS})
target_include_directories(
  runtime
  INTERFACE
  runtime
  PRIVATE
  ${FFI_INCLUDE_DIRS})
target_link_libraries(
  runtime
  PUBLIC
  capi-headers
  PRIVATE
  dl
  modules
  siphash
  valgrind
  ${FFI_LIBRARIES})
# TODO(T42692043): This awkwardness should go away once we freeze the stdlib.
target_compile_definitions(runtime PRIVATE PYRO_BASEDIR="${CMAKE_SOURCE_DIR}")
target_include_directories(runtime PRIVATE ${FROZEN_MODULE_OUTPUT_DIR}
  ${CMAKE_SOURCE_DIR}/runtime)

# Prepare build dir with CPython's directories
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/Modules")
file(WRITE "${CMAKE_BINARY_DIR}/Modules/Setup.local" "Pretend Setup exists")

set(CPYTHON_DIR ${CMAKE_SOURCE_DIR}/third-party/cpython)
add_library(
  expat
  STATIC
  ${CPYTHON_DIR}/Modules/expat/ascii.h
  ${CPYTHON_DIR}/Modules/expat/asciitab.h
  ${CPYTHON_DIR}/Modules/expat/expat_config.h
  ${CPYTHON_DIR}/Modules/expat/expat_external.h
  ${CPYTHON_DIR}/Modules/expat/expat.h
  ${CPYTHON_DIR}/Modules/expat/iasciitab.h
  ${CPYTHON_DIR}/Modules/expat/internal.h
  ${CPYTHON_DIR}/Modules/expat/latin1tab.h
  ${CPYTHON_DIR}/Modules/expat/loadlibrary.c
  ${CPYTHON_DIR}/Modules/expat/nametab.h
  ${CPYTHON_DIR}/Modules/expat/pyexpatns.h
  ${CPYTHON_DIR}/Modules/expat/siphash.h
  ${CPYTHON_DIR}/Modules/expat/utf8tab.h
  ${CPYTHON_DIR}/Modules/expat/winconfig.h
  ${CPYTHON_DIR}/Modules/expat/xmlparse.c
  ${CPYTHON_DIR}/Modules/expat/xmlrole.c
  ${CPYTHON_DIR}/Modules/expat/xmlrole.h
  ${CPYTHON_DIR}/Modules/expat/xmltok.c
  ${CPYTHON_DIR}/Modules/expat/xmltok.h
  ${CPYTHON_DIR}/Modules/expat/xmltok_impl.c
  ${CPYTHON_DIR}/Modules/expat/xmltok_impl.h
  ${CPYTHON_DIR}/Modules/expat/xmltok_ns.c
)
target_compile_options(
  expat
  PRIVATE
  -DHAVE_EXPAT_CONFIG_H
  -DXML_POOR_ENTROPY)
target_link_libraries(
  expat
  PRIVATE
  capi-headers
)

add_library(
  extension-modules
  STATIC
  ${CPYTHON_DIR}/Modules/_blake2/blake2b_impl.c
  ${CPYTHON_DIR}/Modules/_blake2/blake2module.c
  ${CPYTHON_DIR}/Modules/_blake2/blake2s_impl.c
  ${CPYTHON_DIR}/Modules/_blake2/clinic/blake2b_impl.c.h
  ${CPYTHON_DIR}/Modules/_blake2/clinic/blake2s_impl.c.h
  ${CPYTHON_DIR}/Modules/_bz2module.c
  ${CPYTHON_DIR}/Modules/_csv.c
  ${CPYTHON_DIR}/Modules/_hashopenssl.c
  ${CPYTHON_DIR}/Modules/_localemodule.c
  ${CPYTHON_DIR}/Modules/_lzmamodule.c
  ${CPYTHON_DIR}/Modules/_math.c
  ${CPYTHON_DIR}/Modules/_math.h
  ${CPYTHON_DIR}/Modules/_multiprocessing/multiprocessing.h
  ${CPYTHON_DIR}/Modules/_multiprocessing/multiprocessing.c
  ${CPYTHON_DIR}/Modules/_multiprocessing/semaphore.c
  ${CPYTHON_DIR}/Modules/_posixsubprocess.c
  ${CPYTHON_DIR}/Modules/_randommodule.c
  ${CPYTHON_DIR}/Modules/_sha3/sha3module.c
  ${CPYTHON_DIR}/Modules/_ssl.c
  ${CPYTHON_DIR}/Modules/_ssl_data.h
  ${CPYTHON_DIR}/Modules/_stat.c
  ${CPYTHON_DIR}/Modules/_struct.c
  ${CPYTHON_DIR}/Modules/addrinfo.h
  ${CPYTHON_DIR}/Modules/atexitmodule.c
  ${CPYTHON_DIR}/Modules/audioop.c
  ${CPYTHON_DIR}/Modules/binascii.c
  ${CPYTHON_DIR}/Modules/clinic/_bz2module.c.h
  ${CPYTHON_DIR}/Modules/clinic/_hashopenssl.c.h
  ${CPYTHON_DIR}/Modules/clinic/_lzmamodule.c.h
  ${CPYTHON_DIR}/Modules/clinic/_ssl.c.h
  ${CPYTHON_DIR}/Modules/clinic/arraymodule.c.h
  ${CPYTHON_DIR}/Modules/clinic/binascii.c.h
  ${CPYTHON_DIR}/Modules/clinic/cmathmodule.c.h
  ${CPYTHON_DIR}/Modules/clinic/fcntlmodule.c.h
  ${CPYTHON_DIR}/Modules/clinic/grpmodule.c.h
  ${CPYTHON_DIR}/Modules/clinic/md5module.c.h
  ${CPYTHON_DIR}/Modules/clinic/posixmodule.c.h
  ${CPYTHON_DIR}/Modules/clinic/pwdmodule.c.h
  ${CPYTHON_DIR}/Modules/clinic/pyexpat.c.h
  ${CPYTHON_DIR}/Modules/clinic/sha1module.c.h
  ${CPYTHON_DIR}/Modules/clinic/sha256module.c.h
  ${CPYTHON_DIR}/Modules/clinic/sha512module.c.h
  ${CPYTHON_DIR}/Modules/clinic/symtablemodule.c.h
  ${CPYTHON_DIR}/Modules/clinic/zlibmodule.c.h
  ${CPYTHON_DIR}/Modules/errnomodule.c
  ${CPYTHON_DIR}/Modules/fcntlmodule.c
  ${CPYTHON_DIR}/Modules/grpmodule.c
  ${CPYTHON_DIR}/Modules/hashlib.h
  ${CPYTHON_DIR}/Modules/mathmodule.c
  ${CPYTHON_DIR}/Modules/posixmodule.c
  ${CPYTHON_DIR}/Modules/posixmodule.h
  ${CPYTHON_DIR}/Modules/pwdmodule.c
  ${CPYTHON_DIR}/Modules/pyexpat.c
  ${CPYTHON_DIR}/Modules/readline.c
  ${CPYTHON_DIR}/Modules/resource.c
  ${CPYTHON_DIR}/Modules/selectmodule.c
  ${CPYTHON_DIR}/Modules/socketmodule.c
  ${CPYTHON_DIR}/Modules/socketmodule.h
  ${CPYTHON_DIR}/Modules/symtablemodule.c
  ${CPYTHON_DIR}/Modules/syslogmodule.c
  ${CPYTHON_DIR}/Modules/termios.c
  ${CPYTHON_DIR}/Modules/timemodule.c
  ${CPYTHON_DIR}/Modules/unicodename_db.h
  ${CPYTHON_DIR}/Modules/zlibmodule.c
  ${CPYTHON_DIR}/Objects/accu.c
  ${CPYTHON_DIR}/Objects/capsule.c
  ${CPYTHON_DIR}/Objects/unicodectype.c
  ${CPYTHON_DIR}/Objects/unicodetype_db.h
  ${CPYTHON_DIR}/Parser/acceler.c
  ${CPYTHON_DIR}/Parser/bitset.c
  ${CPYTHON_DIR}/Parser/firstsets.c
  ${CPYTHON_DIR}/Parser/grammar.c
  ${CPYTHON_DIR}/Parser/grammar1.c
  ${CPYTHON_DIR}/Parser/listnode.c
  ${CPYTHON_DIR}/Parser/metagrammar.c
  ${CPYTHON_DIR}/Parser/myreadline.c
  ${CPYTHON_DIR}/Parser/node.c
  ${CPYTHON_DIR}/Parser/parser.c
  ${CPYTHON_DIR}/Parser/parser.h
  ${CPYTHON_DIR}/Parser/parsetok.c
  ${CPYTHON_DIR}/Parser/pgen.c
  ${CPYTHON_DIR}/Parser/printgrammar.c
  ${CPYTHON_DIR}/Parser/tokenizer.c
  ${CPYTHON_DIR}/Parser/tokenizer.h
  ${CPYTHON_DIR}/Python/Python-ast.c
  ${CPYTHON_DIR}/Python/asdl.c
  ${CPYTHON_DIR}/Python/ast.c
  ${CPYTHON_DIR}/Python/ast_opt.c
  ${CPYTHON_DIR}/Python/ast_unparse.c
  ${CPYTHON_DIR}/Python/dtoa.c
  ${CPYTHON_DIR}/Python/fileutils.c
  ${CPYTHON_DIR}/Python/future.c
  ${CPYTHON_DIR}/Python/graminit.c
  ${CPYTHON_DIR}/Python/mystrtoul.c
  ${CPYTHON_DIR}/Python/parser.c
  ${CPYTHON_DIR}/Python/pyarena.c
  ${CPYTHON_DIR}/Python/pyctype.c
  ${CPYTHON_DIR}/Python/pystrcmp.c
  ${CPYTHON_DIR}/Python/pystrhex.c
  ${CPYTHON_DIR}/Python/pystrtod.c
  ${CPYTHON_DIR}/Python/pytime.c
  ${CPYTHON_DIR}/Python/symtable.c
  ${CPYTHON_DIR}/Python/wordcode_helpers.h
  capi/cpython-internal/Python-ast.h
  capi/cpython-internal/accu.h
  capi/cpython-internal/asdl.h
  capi/cpython-internal/ast.h
  capi/cpython-internal/bitset.h
  capi/cpython-internal/errcode.h
  capi/cpython-internal/graminit.h
  capi/cpython-internal/grammar.h
  capi/cpython-internal/metagrammar.h
  capi/cpython-internal/node.h
  capi/cpython-internal/osdefs.h
  capi/cpython-internal/pgen.h
  capi/cpython-internal/pgenheaders.h
  capi/cpython-internal/pystrhex.h
  capi/cpython-internal/symtable.h
  capi/cpython-internal/token.h
  capi/cpython-internal/ucnhash.h)
target_include_directories(
  extension-modules
  PUBLIC
  capi/cpython-internal
  ${CPYTHON_DIR}/Modules
  ${CPYTHON_DIR}/Modules/_sha3
  ${CPYTHON_DIR}/Modules/expat
  ${BZIP2_INCLUDE_DIRS}
  ${OPENSSL_INCLUDE_DIRS}
  ${READLINE_INCLUDE_DIRS}
  ${XZ_INCLUDE_DIRS}
  ${ZLIB_INCLUDE_DIRS})
target_compile_definitions(extension-modules PRIVATE _GNU_SOURCE)
target_compile_options(
  extension-modules
  PRIVATE
  -Werror
  -Wstrict-prototypes
  -DPy_BUILD_CORE
  # TODO(T48643833): Remove this define to get _hashlib.scrypt.
  -DOPENSSL_NO_SCRYPT)
target_link_libraries(
  extension-modules
  PUBLIC
  capi-headers
  PRIVATE
  expat
  util
  ${BZIP2_LIBRARIES}
  ${OPENSSL_LIBRARIES}
  ${READLINE_LIBRARIES}
  ${XZ_LIBRARIES}
  ${ZLIB_LIBRARIES})

add_library(
  extension
  OBJECT
  ext/Modules/_compile/Python-ast.h
  ext/Modules/_compile/asdl.h
  ext/Modules/_compile/ast.h
  ext/Modules/_compile/compile.cpp
  ext/Modules/_compile/compile.h
  ext/Modules/_compile/module.cpp
  ext/Modules/_compile/node.h
  ext/Modules/_compile/opcode.h
  ext/Modules/_compile/peephole.cpp
  ext/Modules/_compile/symtable.h
  ext/Modules/_compile/wordcode_helpers.h
  ext/Modules/_datetimemodule.cpp
  ext/Modules/_sre/_sre.cpp
  ext/Modules/gcmodule.cpp
  ext/Modules/getbuildinfo.cpp
  ext/Modules/main.cpp
  ext/Modules/signalmodule.cpp
  ext/Objects/abstract.cpp
  ext/Objects/boolobject.cpp
  ext/Objects/bytearrayobject.cpp
  ext/Objects/bytesobject.cpp
  ext/Objects/classobject.cpp
  ext/Objects/codeobject.cpp
  ext/Objects/complexobject.cpp
  ext/Objects/descrobject.cpp
  ext/Objects/dictobject.cpp
  ext/Objects/exceptions.cpp
  ext/Objects/fileobject.cpp
  ext/Objects/floatobject.cpp
  ext/Objects/frameobject.cpp
  ext/Objects/funcobject.cpp
  ext/Objects/function-utils.cpp
  ext/Objects/function-utils.h
  ext/Objects/genobject.cpp
  ext/Objects/iterobject.cpp
  ext/Objects/listobject.cpp
  ext/Objects/longobject.cpp
  ext/Objects/memoryobject.cpp
  ext/Objects/methodobject.cpp
  ext/Objects/moduleobject.cpp
  ext/Objects/namespaceobject.cpp
  ext/Objects/object.cpp
  ext/Objects/obmalloc.cpp
  ext/Objects/odictobject.cpp
  ext/Objects/rangeobject.cpp
  ext/Objects/setobject.cpp
  ext/Objects/sliceobject.cpp
  ext/Objects/structseq.cpp
  ext/Objects/tupleobject.cpp
  ext/Objects/typeobject.cpp
  ext/Objects/unicodeobject.cpp
  ext/Objects/weakrefobject.cpp
  ext/Python/_warnings.cpp
  ext/Python/ceval.cpp
  ext/Python/codecs.cpp
  ext/Python/compile.cpp
  ext/Python/errors.cpp
  ext/Python/getargs.cpp
  ext/Python/getcompiler.cpp
  ext/Python/getcopyright.cpp
  ext/Python/getplatform.cpp
  ext/Python/getversion.cpp
  ext/Python/import.cpp
  ext/Python/modsupport-internal.h
  ext/Python/modsupport.cpp
  ext/Python/mysnprintf.cpp
  ext/Python/pathconfig.cpp
  ext/Python/pyhash.cpp
  ext/Python/pylifecycle.cpp
  ext/Python/pystate.cpp
  ext/Python/pythonrun.cpp
  ext/Python/random.cpp
  ext/Python/sysmodule.cpp
  ext/Python/thread.cpp
  ext/Python/traceback.cpp)
target_compile_options(
  extension
  PRIVATE
  ${OS_DEFINE}
  ${PYRO_COMPILE_OPTIONS})
target_include_directories(
  extension
  PRIVATE
  $<TARGET_PROPERTY:capi-headers,INTERFACE_INCLUDE_DIRECTORIES>
  $<TARGET_PROPERTY:gmock,INTERFACE_INCLUDE_DIRECTORIES>
  $<TARGET_PROPERTY:gtest,INTERFACE_INCLUDE_DIRECTORIES>
  runtime)

set(BINARY_DIR_LIB "${CMAKE_BINARY_DIR}/Lib")
set(CPYTHON_LIBRARY_DIR "${CMAKE_SOURCE_DIR}/third-party/cpython/Lib")
set(
  CPYTHON_MODULE_INPUT
  ${CPYTHON_LIBRARY_DIR}/__future__.py
  ${CPYTHON_LIBRARY_DIR}/__phello__.foo.py
  ${CPYTHON_LIBRARY_DIR}/_bootlocale.py
  ${CPYTHON_LIBRARY_DIR}/_bootsubprocess.py
  ${CPYTHON_LIBRARY_DIR}/_collections_abc.py
  ${CPYTHON_LIBRARY_DIR}/_compat_pickle.py
  ${CPYTHON_LIBRARY_DIR}/_compression.py
  ${CPYTHON_LIBRARY_DIR}/_dummy_thread.py
  ${CPYTHON_LIBRARY_DIR}/_markupbase.py
  ${CPYTHON_LIBRARY_DIR}/_osx_support.py
  ${CPYTHON_LIBRARY_DIR}/_py_abc.py
  ${CPYTHON_LIBRARY_DIR}/_pydecimal.py
  ${CPYTHON_LIBRARY_DIR}/_pyio.py
  ${CPYTHON_LIBRARY_DIR}/_sitebuiltins.py
  ${CPYTHON_LIBRARY_DIR}/_strptime.py
  ${CPYTHON_LIBRARY_DIR}/_threading_local.py
  ${CPYTHON_LIBRARY_DIR}/_weakrefset.py
  ${CPYTHON_LIBRARY_DIR}/abc.py
  ${CPYTHON_LIBRARY_DIR}/aifc.py
  ${CPYTHON_LIBRARY_DIR}/antigravity.py
  ${CPYTHON_LIBRARY_DIR}/argparse.py
  ${CPYTHON_LIBRARY_DIR}/ast.py
  ${CPYTHON_LIBRARY_DIR}/asynchat.py
  ${CPYTHON_LIBRARY_DIR}/asyncio/__init__.py
  ${CPYTHON_LIBRARY_DIR}/asyncio/base_events.py
  ${CPYTHON_LIBRARY_DIR}/asyncio/base_futures.py
  ${CPYTHON_LIBRARY_DIR}/asyncio/base_subprocess.py
  ${CPYTHON_LIBRARY_DIR}/asyncio/base_tasks.py
  ${CPYTHON_LIBRARY_DIR}/asyncio/constants.py
  ${CPYTHON_LIBRARY_DIR}/asyncio/coroutines.py
  ${CPYTHON_LIBRARY_DIR}/asyncio/events.py
  ${CPYTHON_LIBRARY_DIR}/asyncio/format_helpers.py
  ${CPYTHON_LIBRARY_DIR}/asyncio/futures.py
  ${CPYTHON_LIBRARY_DIR}/asyncio/locks.py
  ${CPYTHON_LIBRARY_DIR}/asyncio/log.py
  ${CPYTHON_LIBRARY_DIR}/asyncio/proactor_events.py
  ${CPYTHON_LIBRARY_DIR}/asyncio/protocols.py
  ${CPYTHON_LIBRARY_DIR}/asyncio/queues.py
  ${CPYTHON_LIBRARY_DIR}/asyncio/runners.py
  ${CPYTHON_LIBRARY_DIR}/asyncio/selector_events.py
  ${CPYTHON_LIBRARY_DIR}/asyncio/sslproto.py
  ${CPYTHON_LIBRARY_DIR}/asyncio/streams.py
  ${CPYTHON_LIBRARY_DIR}/asyncio/subprocess.py
  ${CPYTHON_LIBRARY_DIR}/asyncio/tasks.py
  ${CPYTHON_LIBRARY_DIR}/asyncio/transports.py
  ${CPYTHON_LIBRARY_DIR}/asyncio/unix_events.py
  ${CPYTHON_LIBRARY_DIR}/asyncio/windows_events.py
  ${CPYTHON_LIBRARY_DIR}/asyncio/windows_utils.py
  ${CPYTHON_LIBRARY_DIR}/asyncore.py
  ${CPYTHON_LIBRARY_DIR}/base64.py
  ${CPYTHON_LIBRARY_DIR}/bdb.py
  ${CPYTHON_LIBRARY_DIR}/binhex.py
  ${CPYTHON_LIBRARY_DIR}/bisect.py
  ${CPYTHON_LIBRARY_DIR}/bz2.py
  ${CPYTHON_LIBRARY_DIR}/cProfile.py
  ${CPYTHON_LIBRARY_DIR}/calendar.py
  ${CPYTHON_LIBRARY_DIR}/cgi.py
  ${CPYTHON_LIBRARY_DIR}/cgitb.py
  ${CPYTHON_LIBRARY_DIR}/chunk.py
  ${CPYTHON_LIBRARY_DIR}/cmd.py
  ${CPYTHON_LIBRARY_DIR}/code.py
  ${CPYTHON_LIBRARY_DIR}/codecs.py
  ${CPYTHON_LIBRARY_DIR}/codeop.py
  ${CPYTHON_LIBRARY_DIR}/colorsys.py
  ${CPYTHON_LIBRARY_DIR}/compileall.py
  ${CPYTHON_LIBRARY_DIR}/concurrent/__init__.py
  ${CPYTHON_LIBRARY_DIR}/concurrent/futures/__init__.py
  ${CPYTHON_LIBRARY_DIR}/concurrent/futures/_base.py
  ${CPYTHON_LIBRARY_DIR}/concurrent/futures/process.py
  ${CPYTHON_LIBRARY_DIR}/concurrent/futures/thread.py
  ${CPYTHON_LIBRARY_DIR}/configparser.py
  ${CPYTHON_LIBRARY_DIR}/contextlib.py
  ${CPYTHON_LIBRARY_DIR}/contextvars.py
  ${CPYTHON_LIBRARY_DIR}/copy.py
  ${CPYTHON_LIBRARY_DIR}/copyreg.py
  ${CPYTHON_LIBRARY_DIR}/crypt.py
  ${CPYTHON_LIBRARY_DIR}/csv.py
  ${CPYTHON_LIBRARY_DIR}/ctypes/__init__.py
  ${CPYTHON_LIBRARY_DIR}/ctypes/_aix.py
  ${CPYTHON_LIBRARY_DIR}/ctypes/_endian.py
  ${CPYTHON_LIBRARY_DIR}/ctypes/macholib/__init__.py
  ${CPYTHON_LIBRARY_DIR}/ctypes/macholib/dyld.py
  ${CPYTHON_LIBRARY_DIR}/ctypes/macholib/dylib.py
  ${CPYTHON_LIBRARY_DIR}/ctypes/macholib/framework.py
  ${CPYTHON_LIBRARY_DIR}/ctypes/util.py
  ${CPYTHON_LIBRARY_DIR}/ctypes/wintypes.py
  ${CPYTHON_LIBRARY_DIR}/curses/__init__.py
  ${CPYTHON_LIBRARY_DIR}/curses/ascii.py
  ${CPYTHON_LIBRARY_DIR}/curses/has_key.py
  ${CPYTHON_LIBRARY_DIR}/curses/panel.py
  ${CPYTHON_LIBRARY_DIR}/curses/textpad.py
  ${CPYTHON_LIBRARY_DIR}/datetime.py
  ${CPYTHON_LIBRARY_DIR}/dbm/__init__.py
  ${CPYTHON_LIBRARY_DIR}/dbm/dumb.py
  ${CPYTHON_LIBRARY_DIR}/dbm/gnu.py
  ${CPYTHON_LIBRARY_DIR}/dbm/ndbm.py
  ${CPYTHON_LIBRARY_DIR}/decimal.py
  ${CPYTHON_LIBRARY_DIR}/difflib.py
  ${CPYTHON_LIBRARY_DIR}/dis.py
  ${CPYTHON_LIBRARY_DIR}/distutils/__init__.py
  ${CPYTHON_LIBRARY_DIR}/distutils/_msvccompiler.py
  ${CPYTHON_LIBRARY_DIR}/distutils/archive_util.py
  ${CPYTHON_LIBRARY_DIR}/distutils/bcppcompiler.py
  ${CPYTHON_LIBRARY_DIR}/distutils/ccompiler.py
  ${CPYTHON_LIBRARY_DIR}/distutils/cmd.py
  ${CPYTHON_LIBRARY_DIR}/distutils/command/__init__.py
  ${CPYTHON_LIBRARY_DIR}/distutils/command/bdist.py
  ${CPYTHON_LIBRARY_DIR}/distutils/command/bdist_dumb.py
  ${CPYTHON_LIBRARY_DIR}/distutils/command/bdist_msi.py
  ${CPYTHON_LIBRARY_DIR}/distutils/command/bdist_rpm.py
  ${CPYTHON_LIBRARY_DIR}/distutils/command/bdist_wininst.py
  ${CPYTHON_LIBRARY_DIR}/distutils/command/build.py
  ${CPYTHON_LIBRARY_DIR}/distutils/command/build_clib.py
  ${CPYTHON_LIBRARY_DIR}/distutils/command/build_ext.py
  ${CPYTHON_LIBRARY_DIR}/distutils/command/build_py.py
  ${CPYTHON_LIBRARY_DIR}/distutils/command/build_scripts.py
  ${CPYTHON_LIBRARY_DIR}/distutils/command/check.py
  ${CPYTHON_LIBRARY_DIR}/distutils/command/clean.py
  ${CPYTHON_LIBRARY_DIR}/distutils/command/config.py
  ${CPYTHON_LIBRARY_DIR}/distutils/command/install.py
  ${CPYTHON_LIBRARY_DIR}/distutils/command/install_data.py
  ${CPYTHON_LIBRARY_DIR}/distutils/command/install_egg_info.py
  ${CPYTHON_LIBRARY_DIR}/distutils/command/install_headers.py
  ${CPYTHON_LIBRARY_DIR}/distutils/command/install_lib.py
  ${CPYTHON_LIBRARY_DIR}/distutils/command/install_scripts.py
  ${CPYTHON_LIBRARY_DIR}/distutils/command/register.py
  ${CPYTHON_LIBRARY_DIR}/distutils/command/sdist.py
  ${CPYTHON_LIBRARY_DIR}/distutils/command/upload.py
  ${CPYTHON_LIBRARY_DIR}/distutils/config.py
  ${CPYTHON_LIBRARY_DIR}/distutils/core.py
  ${CPYTHON_LIBRARY_DIR}/distutils/cygwinccompiler.py
  ${CPYTHON_LIBRARY_DIR}/distutils/debug.py
  ${CPYTHON_LIBRARY_DIR}/distutils/dep_util.py
  ${CPYTHON_LIBRARY_DIR}/distutils/dir_util.py
  ${CPYTHON_LIBRARY_DIR}/distutils/dist.py
  ${CPYTHON_LIBRARY_DIR}/distutils/errors.py
  ${CPYTHON_LIBRARY_DIR}/distutils/extension.py
  ${CPYTHON_LIBRARY_DIR}/distutils/fancy_getopt.py
  ${CPYTHON_LIBRARY_DIR}/distutils/file_util.py
  ${CPYTHON_LIBRARY_DIR}/distutils/filelist.py
  ${CPYTHON_LIBRARY_DIR}/distutils/log.py
  ${CPYTHON_LIBRARY_DIR}/distutils/msvc9compiler.py
  ${CPYTHON_LIBRARY_DIR}/distutils/msvccompiler.py
  ${CPYTHON_LIBRARY_DIR}/distutils/spawn.py
  ${CPYTHON_LIBRARY_DIR}/distutils/sysconfig.py
  ${CPYTHON_LIBRARY_DIR}/distutils/text_file.py
  ${CPYTHON_LIBRARY_DIR}/distutils/unixccompiler.py
  ${CPYTHON_LIBRARY_DIR}/distutils/util.py
  ${CPYTHON_LIBRARY_DIR}/distutils/version.py
  ${CPYTHON_LIBRARY_DIR}/distutils/versionpredicate.py
  ${CPYTHON_LIBRARY_DIR}/doctest.py
  ${CPYTHON_LIBRARY_DIR}/dummy_threading.py
  ${CPYTHON_LIBRARY_DIR}/email/__init__.py
  ${CPYTHON_LIBRARY_DIR}/email/_encoded_words.py
  ${CPYTHON_LIBRARY_DIR}/email/_header_value_parser.py
  ${CPYTHON_LIBRARY_DIR}/email/_parseaddr.py
  ${CPYTHON_LIBRARY_DIR}/email/_policybase.py
  ${CPYTHON_LIBRARY_DIR}/email/base64mime.py
  ${CPYTHON_LIBRARY_DIR}/email/charset.py
  ${CPYTHON_LIBRARY_DIR}/email/contentmanager.py
  ${CPYTHON_LIBRARY_DIR}/email/encoders.py
  ${CPYTHON_LIBRARY_DIR}/email/errors.py
  ${CPYTHON_LIBRARY_DIR}/email/feedparser.py
  ${CPYTHON_LIBRARY_DIR}/email/generator.py
  ${CPYTHON_LIBRARY_DIR}/email/header.py
  ${CPYTHON_LIBRARY_DIR}/email/headerregistry.py
  ${CPYTHON_LIBRARY_DIR}/email/iterators.py
  ${CPYTHON_LIBRARY_DIR}/email/message.py
  ${CPYTHON_LIBRARY_DIR}/email/mime/__init__.py
  ${CPYTHON_LIBRARY_DIR}/email/mime/application.py
  ${CPYTHON_LIBRARY_DIR}/email/mime/audio.py
  ${CPYTHON_LIBRARY_DIR}/email/mime/base.py
  ${CPYTHON_LIBRARY_DIR}/email/mime/image.py
  ${CPYTHON_LIBRARY_DIR}/email/mime/message.py
  ${CPYTHON_LIBRARY_DIR}/email/mime/multipart.py
  ${CPYTHON_LIBRARY_DIR}/email/mime/nonmultipart.py
  ${CPYTHON_LIBRARY_DIR}/email/mime/text.py
  ${CPYTHON_LIBRARY_DIR}/email/parser.py
  ${CPYTHON_LIBRARY_DIR}/email/policy.py
  ${CPYTHON_LIBRARY_DIR}/email/quoprimime.py
  ${CPYTHON_LIBRARY_DIR}/email/utils.py
  ${CPYTHON_LIBRARY_DIR}/ensurepip/__init__.py
  ${CPYTHON_LIBRARY_DIR}/ensurepip/__main__.py
  ${CPYTHON_LIBRARY_DIR}/ensurepip/_uninstall.py
  ${CPYTHON_LIBRARY_DIR}/enum.py
  ${CPYTHON_LIBRARY_DIR}/filecmp.py
  ${CPYTHON_LIBRARY_DIR}/fileinput.py
  ${CPYTHON_LIBRARY_DIR}/fnmatch.py
  ${CPYTHON_LIBRARY_DIR}/formatter.py
  ${CPYTHON_LIBRARY_DIR}/fractions.py
  ${CPYTHON_LIBRARY_DIR}/ftplib.py
  ${CPYTHON_LIBRARY_DIR}/functools.py
  ${CPYTHON_LIBRARY_DIR}/genericpath.py
  ${CPYTHON_LIBRARY_DIR}/getopt.py
  ${CPYTHON_LIBRARY_DIR}/getpass.py
  ${CPYTHON_LIBRARY_DIR}/gettext.py
  ${CPYTHON_LIBRARY_DIR}/glob.py
  ${CPYTHON_LIBRARY_DIR}/gzip.py
  ${CPYTHON_LIBRARY_DIR}/hashlib.py
  ${CPYTHON_LIBRARY_DIR}/heapq.py
  ${CPYTHON_LIBRARY_DIR}/hmac.py
  ${CPYTHON_LIBRARY_DIR}/html/__init__.py
  ${CPYTHON_LIBRARY_DIR}/html/entities.py
  ${CPYTHON_LIBRARY_DIR}/html/parser.py
  ${CPYTHON_LIBRARY_DIR}/http/__init__.py
  ${CPYTHON_LIBRARY_DIR}/http/client.py
  ${CPYTHON_LIBRARY_DIR}/http/cookiejar.py
  ${CPYTHON_LIBRARY_DIR}/http/cookies.py
  ${CPYTHON_LIBRARY_DIR}/http/server.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/__init__.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/__main__.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/autocomplete.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/autocomplete_w.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/autoexpand.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/browser.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/calltip.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/calltip_w.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/codecontext.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/colorizer.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/config.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/config_key.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/configdialog.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/debugger.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/debugger_r.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/debugobj.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/debugobj_r.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/delegator.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/dynoption.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/editor.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/filelist.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/grep.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/help.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/help_about.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/history.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/hyperparser.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/__init__.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/htest.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/mock_idle.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/mock_tk.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/template.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_autocomplete.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_autocomplete_w.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_autoexpand.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_browser.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_calltip.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_calltip_w.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_codecontext.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_colorizer.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_config.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_config_key.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_configdialog.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_debugger.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_debugger_r.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_debugobj.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_debugobj_r.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_delegator.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_editmenu.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_editor.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_filelist.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_grep.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_help.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_help_about.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_history.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_hyperparser.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_iomenu.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_macosx.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_mainmenu.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_multicall.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_outwin.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_paragraph.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_parenmatch.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_pathbrowser.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_percolator.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_pyparse.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_pyshell.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_query.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_redirector.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_replace.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_rpc.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_rstrip.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_run.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_runscript.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_scrolledlist.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_search.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_searchbase.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_searchengine.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_squeezer.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_stackviewer.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_statusbar.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_text.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_textview.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_tooltip.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_tree.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_undo.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_warning.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_window.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/idle_test/test_zoomheight.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/iomenu.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/macosx.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/mainmenu.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/multicall.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/outwin.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/paragraph.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/parenmatch.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/pathbrowser.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/percolator.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/pyparse.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/pyshell.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/query.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/redirector.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/replace.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/rpc.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/rstrip.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/run.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/runscript.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/scrolledlist.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/search.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/searchbase.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/searchengine.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/squeezer.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/stackviewer.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/statusbar.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/textview.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/tooltip.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/tree.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/undo.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/window.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/zoomheight.py
  ${CPYTHON_LIBRARY_DIR}/idlelib/zzdummy.py
  ${CPYTHON_LIBRARY_DIR}/imaplib.py
  ${CPYTHON_LIBRARY_DIR}/imghdr.py
  ${CPYTHON_LIBRARY_DIR}/imp.py
  ${CPYTHON_LIBRARY_DIR}/inspect.py
  ${CPYTHON_LIBRARY_DIR}/io.py
  ${CPYTHON_LIBRARY_DIR}/ipaddress.py
  ${CPYTHON_LIBRARY_DIR}/json/__init__.py
  ${CPYTHON_LIBRARY_DIR}/json/decoder.py
  ${CPYTHON_LIBRARY_DIR}/json/encoder.py
  ${CPYTHON_LIBRARY_DIR}/json/scanner.py
  ${CPYTHON_LIBRARY_DIR}/json/tool.py
  ${CPYTHON_LIBRARY_DIR}/keyword.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/Grammar.txt
  ${CPYTHON_LIBRARY_DIR}/lib2to3/PatternGrammar.txt
  ${CPYTHON_LIBRARY_DIR}/lib2to3/__init__.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/__main__.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/btm_matcher.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/btm_utils.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/fixer_base.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/fixer_util.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/fixes/__init__.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/fixes/fix_apply.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/fixes/fix_asserts.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/fixes/fix_basestring.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/fixes/fix_buffer.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/fixes/fix_dict.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/fixes/fix_except.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/fixes/fix_exec.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/fixes/fix_execfile.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/fixes/fix_exitfunc.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/fixes/fix_filter.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/fixes/fix_funcattrs.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/fixes/fix_future.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/fixes/fix_getcwdu.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/fixes/fix_has_key.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/fixes/fix_idioms.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/fixes/fix_import.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/fixes/fix_imports.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/fixes/fix_imports2.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/fixes/fix_input.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/fixes/fix_intern.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/fixes/fix_isinstance.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/fixes/fix_itertools.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/fixes/fix_itertools_imports.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/fixes/fix_long.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/fixes/fix_map.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/fixes/fix_metaclass.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/fixes/fix_methodattrs.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/fixes/fix_ne.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/fixes/fix_next.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/fixes/fix_nonzero.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/fixes/fix_numliterals.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/fixes/fix_operator.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/fixes/fix_paren.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/fixes/fix_print.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/fixes/fix_raise.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/fixes/fix_raw_input.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/fixes/fix_reduce.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/fixes/fix_reload.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/fixes/fix_renames.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/fixes/fix_repr.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/fixes/fix_set_literal.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/fixes/fix_standarderror.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/fixes/fix_sys_exc.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/fixes/fix_throw.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/fixes/fix_tuple_params.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/fixes/fix_types.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/fixes/fix_unicode.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/fixes/fix_urllib.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/fixes/fix_ws_comma.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/fixes/fix_xrange.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/fixes/fix_xreadlines.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/fixes/fix_zip.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/main.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/patcomp.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/pgen2/__init__.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/pgen2/conv.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/pgen2/driver.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/pgen2/grammar.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/pgen2/literals.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/pgen2/parse.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/pgen2/pgen.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/pgen2/token.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/pgen2/tokenize.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/pygram.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/pytree.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/refactor.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/tests/__init__.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/tests/__main__.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/tests/data/bom.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/tests/data/crlf.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/tests/data/different_encoding.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/tests/data/false_encoding.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/tests/data/fixers/bad_order.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/tests/data/fixers/myfixes/__init__.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/tests/data/fixers/myfixes/fix_explicit.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/tests/data/fixers/myfixes/fix_first.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/tests/data/fixers/myfixes/fix_last.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/tests/data/fixers/myfixes/fix_parrot.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/tests/data/fixers/myfixes/fix_preorder.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/tests/data/fixers/no_fixer_cls.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/tests/data/fixers/parrot_example.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/tests/data/infinite_recursion.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/tests/data/py2_test_grammar.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/tests/data/py3_test_grammar.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/tests/pytree_idempotency.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/tests/support.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/tests/test_all_fixers.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/tests/test_fixers.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/tests/test_main.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/tests/test_parser.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/tests/test_pytree.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/tests/test_refactor.py
  ${CPYTHON_LIBRARY_DIR}/lib2to3/tests/test_util.py
  ${CPYTHON_LIBRARY_DIR}/linecache.py
  ${CPYTHON_LIBRARY_DIR}/locale.py
  ${CPYTHON_LIBRARY_DIR}/lzma.py
  ${CPYTHON_LIBRARY_DIR}/macpath.py
  ${CPYTHON_LIBRARY_DIR}/mailbox.py
  ${CPYTHON_LIBRARY_DIR}/mailcap.py
  ${CPYTHON_LIBRARY_DIR}/mimetypes.py
  ${CPYTHON_LIBRARY_DIR}/modulefinder.py
  ${CPYTHON_LIBRARY_DIR}/msilib/__init__.py
  ${CPYTHON_LIBRARY_DIR}/msilib/schema.py
  ${CPYTHON_LIBRARY_DIR}/msilib/sequence.py
  ${CPYTHON_LIBRARY_DIR}/msilib/text.py
  ${CPYTHON_LIBRARY_DIR}/multiprocessing/__init__.py
  ${CPYTHON_LIBRARY_DIR}/multiprocessing/connection.py
  ${CPYTHON_LIBRARY_DIR}/multiprocessing/context.py
  ${CPYTHON_LIBRARY_DIR}/multiprocessing/dummy/__init__.py
  ${CPYTHON_LIBRARY_DIR}/multiprocessing/dummy/connection.py
  ${CPYTHON_LIBRARY_DIR}/multiprocessing/forkserver.py
  ${CPYTHON_LIBRARY_DIR}/multiprocessing/heap.py
  ${CPYTHON_LIBRARY_DIR}/multiprocessing/managers.py
  ${CPYTHON_LIBRARY_DIR}/multiprocessing/pool.py
  ${CPYTHON_LIBRARY_DIR}/multiprocessing/popen_fork.py
  ${CPYTHON_LIBRARY_DIR}/multiprocessing/popen_forkserver.py
  ${CPYTHON_LIBRARY_DIR}/multiprocessing/popen_spawn_posix.py
  ${CPYTHON_LIBRARY_DIR}/multiprocessing/popen_spawn_win32.py
  ${CPYTHON_LIBRARY_DIR}/multiprocessing/process.py
  ${CPYTHON_LIBRARY_DIR}/multiprocessing/queues.py
  ${CPYTHON_LIBRARY_DIR}/multiprocessing/reduction.py
  ${CPYTHON_LIBRARY_DIR}/multiprocessing/resource_sharer.py
  ${CPYTHON_LIBRARY_DIR}/multiprocessing/semaphore_tracker.py
  ${CPYTHON_LIBRARY_DIR}/multiprocessing/sharedctypes.py
  ${CPYTHON_LIBRARY_DIR}/multiprocessing/spawn.py
  ${CPYTHON_LIBRARY_DIR}/multiprocessing/synchronize.py
  ${CPYTHON_LIBRARY_DIR}/multiprocessing/util.py
  ${CPYTHON_LIBRARY_DIR}/netrc.py
  ${CPYTHON_LIBRARY_DIR}/nntplib.py
  ${CPYTHON_LIBRARY_DIR}/ntpath.py
  ${CPYTHON_LIBRARY_DIR}/nturl2path.py
  ${CPYTHON_LIBRARY_DIR}/numbers.py
  ${CPYTHON_LIBRARY_DIR}/opcode.py
  ${CPYTHON_LIBRARY_DIR}/optparse.py
  ${CPYTHON_LIBRARY_DIR}/os.py
  ${CPYTHON_LIBRARY_DIR}/pathlib.py
  ${CPYTHON_LIBRARY_DIR}/pdb.py
  ${CPYTHON_LIBRARY_DIR}/pickle.py
  ${CPYTHON_LIBRARY_DIR}/pickletools.py
  ${CPYTHON_LIBRARY_DIR}/pipes.py
  ${CPYTHON_LIBRARY_DIR}/pkgutil.py
  ${CPYTHON_LIBRARY_DIR}/platform.py
  ${CPYTHON_LIBRARY_DIR}/plistlib.py
  ${CPYTHON_LIBRARY_DIR}/poplib.py
  ${CPYTHON_LIBRARY_DIR}/posixpath.py
  ${CPYTHON_LIBRARY_DIR}/pprint.py
  ${CPYTHON_LIBRARY_DIR}/profile.py
  ${CPYTHON_LIBRARY_DIR}/pstats.py
  ${CPYTHON_LIBRARY_DIR}/pty.py
  ${CPYTHON_LIBRARY_DIR}/py_compile.py
  ${CPYTHON_LIBRARY_DIR}/pyclbr.py
  ${CPYTHON_LIBRARY_DIR}/pydoc.py
  ${CPYTHON_LIBRARY_DIR}/pydoc_data/__init__.py
  ${CPYTHON_LIBRARY_DIR}/pydoc_data/topics.py
  ${CPYTHON_LIBRARY_DIR}/queue.py
  ${CPYTHON_LIBRARY_DIR}/quopri.py
  ${CPYTHON_LIBRARY_DIR}/random.py
  ${CPYTHON_LIBRARY_DIR}/re.py
  ${CPYTHON_LIBRARY_DIR}/reprlib.py
  ${CPYTHON_LIBRARY_DIR}/rlcompleter.py
  ${CPYTHON_LIBRARY_DIR}/runpy.py
  ${CPYTHON_LIBRARY_DIR}/sched.py
  ${CPYTHON_LIBRARY_DIR}/secrets.py
  ${CPYTHON_LIBRARY_DIR}/selectors.py
  ${CPYTHON_LIBRARY_DIR}/shelve.py
  ${CPYTHON_LIBRARY_DIR}/shlex.py
  ${CPYTHON_LIBRARY_DIR}/shutil.py
  ${CPYTHON_LIBRARY_DIR}/signal.py
  ${CPYTHON_LIBRARY_DIR}/site.py
  ${CPYTHON_LIBRARY_DIR}/smtpd.py
  ${CPYTHON_LIBRARY_DIR}/smtplib.py
  ${CPYTHON_LIBRARY_DIR}/sndhdr.py
  ${CPYTHON_LIBRARY_DIR}/socketserver.py
  ${CPYTHON_LIBRARY_DIR}/sqlite3/__init__.py
  ${CPYTHON_LIBRARY_DIR}/sqlite3/dbapi2.py
  ${CPYTHON_LIBRARY_DIR}/sqlite3/dump.py
  ${CPYTHON_LIBRARY_DIR}/sqlite3/test/__init__.py
  ${CPYTHON_LIBRARY_DIR}/sqlite3/test/backup.py
  ${CPYTHON_LIBRARY_DIR}/sqlite3/test/dbapi.py
  ${CPYTHON_LIBRARY_DIR}/sqlite3/test/dump.py
  ${CPYTHON_LIBRARY_DIR}/sqlite3/test/factory.py
  ${CPYTHON_LIBRARY_DIR}/sqlite3/test/hooks.py
  ${CPYTHON_LIBRARY_DIR}/sqlite3/test/regression.py
  ${CPYTHON_LIBRARY_DIR}/sqlite3/test/transactions.py
  ${CPYTHON_LIBRARY_DIR}/sqlite3/test/types.py
  ${CPYTHON_LIBRARY_DIR}/sqlite3/test/userfunctions.py
  ${CPYTHON_LIBRARY_DIR}/sre_compile.py
  ${CPYTHON_LIBRARY_DIR}/sre_constants.py
  ${CPYTHON_LIBRARY_DIR}/sre_parse.py
  ${CPYTHON_LIBRARY_DIR}/ssl.py
  ${CPYTHON_LIBRARY_DIR}/stat.py
  ${CPYTHON_LIBRARY_DIR}/statistics.py
  ${CPYTHON_LIBRARY_DIR}/string.py
  ${CPYTHON_LIBRARY_DIR}/stringprep.py
  ${CPYTHON_LIBRARY_DIR}/struct.py
  ${CPYTHON_LIBRARY_DIR}/subprocess.py
  ${CPYTHON_LIBRARY_DIR}/sunau.py
  ${CPYTHON_LIBRARY_DIR}/symbol.py
  ${CPYTHON_LIBRARY_DIR}/symtable.py
  ${CPYTHON_LIBRARY_DIR}/sysconfig.py
  ${CPYTHON_LIBRARY_DIR}/tabnanny.py
  ${CPYTHON_LIBRARY_DIR}/tarfile.py
  ${CPYTHON_LIBRARY_DIR}/telnetlib.py
  ${CPYTHON_LIBRARY_DIR}/tempfile.py
  ${CPYTHON_LIBRARY_DIR}/textwrap.py
  ${CPYTHON_LIBRARY_DIR}/this.py
  ${CPYTHON_LIBRARY_DIR}/timeit.py
  ${CPYTHON_LIBRARY_DIR}/tkinter/__init__.py
  ${CPYTHON_LIBRARY_DIR}/tkinter/__main__.py
  ${CPYTHON_LIBRARY_DIR}/tkinter/colorchooser.py
  ${CPYTHON_LIBRARY_DIR}/tkinter/commondialog.py
  ${CPYTHON_LIBRARY_DIR}/tkinter/constants.py
  ${CPYTHON_LIBRARY_DIR}/tkinter/dialog.py
  ${CPYTHON_LIBRARY_DIR}/tkinter/dnd.py
  ${CPYTHON_LIBRARY_DIR}/tkinter/filedialog.py
  ${CPYTHON_LIBRARY_DIR}/tkinter/font.py
  ${CPYTHON_LIBRARY_DIR}/tkinter/messagebox.py
  ${CPYTHON_LIBRARY_DIR}/tkinter/scrolledtext.py
  ${CPYTHON_LIBRARY_DIR}/tkinter/simpledialog.py
  ${CPYTHON_LIBRARY_DIR}/tkinter/test/__init__.py
  ${CPYTHON_LIBRARY_DIR}/tkinter/test/runtktests.py
  ${CPYTHON_LIBRARY_DIR}/tkinter/test/support.py
  ${CPYTHON_LIBRARY_DIR}/tkinter/test/test_tkinter/__init__.py
  ${CPYTHON_LIBRARY_DIR}/tkinter/test/test_tkinter/test_font.py
  ${CPYTHON_LIBRARY_DIR}/tkinter/test/test_tkinter/test_geometry_managers.py
  ${CPYTHON_LIBRARY_DIR}/tkinter/test/test_tkinter/test_images.py
  ${CPYTHON_LIBRARY_DIR}/tkinter/test/test_tkinter/test_loadtk.py
  ${CPYTHON_LIBRARY_DIR}/tkinter/test/test_tkinter/test_misc.py
  ${CPYTHON_LIBRARY_DIR}/tkinter/test/test_tkinter/test_text.py
  ${CPYTHON_LIBRARY_DIR}/tkinter/test/test_tkinter/test_variables.py
  ${CPYTHON_LIBRARY_DIR}/tkinter/test/test_tkinter/test_widgets.py
  ${CPYTHON_LIBRARY_DIR}/tkinter/test/test_ttk/__init__.py
  ${CPYTHON_LIBRARY_DIR}/tkinter/test/test_ttk/test_extensions.py
  ${CPYTHON_LIBRARY_DIR}/tkinter/test/test_ttk/test_functions.py
  ${CPYTHON_LIBRARY_DIR}/tkinter/test/test_ttk/test_style.py
  ${CPYTHON_LIBRARY_DIR}/tkinter/test/test_ttk/test_widgets.py
  ${CPYTHON_LIBRARY_DIR}/tkinter/test/widget_tests.py
  ${CPYTHON_LIBRARY_DIR}/tkinter/tix.py
  ${CPYTHON_LIBRARY_DIR}/tkinter/ttk.py
  ${CPYTHON_LIBRARY_DIR}/token.py
  ${CPYTHON_LIBRARY_DIR}/tokenize.py
  ${CPYTHON_LIBRARY_DIR}/trace.py
  ${CPYTHON_LIBRARY_DIR}/traceback.py
  ${CPYTHON_LIBRARY_DIR}/tracemalloc.py
  ${CPYTHON_LIBRARY_DIR}/tty.py
  ${CPYTHON_LIBRARY_DIR}/turtle.py
  ${CPYTHON_LIBRARY_DIR}/turtledemo/__init__.py
  ${CPYTHON_LIBRARY_DIR}/turtledemo/__main__.py
  ${CPYTHON_LIBRARY_DIR}/turtledemo/bytedesign.py
  ${CPYTHON_LIBRARY_DIR}/turtledemo/chaos.py
  ${CPYTHON_LIBRARY_DIR}/turtledemo/clock.py
  ${CPYTHON_LIBRARY_DIR}/turtledemo/colormixer.py
  ${CPYTHON_LIBRARY_DIR}/turtledemo/forest.py
  ${CPYTHON_LIBRARY_DIR}/turtledemo/fractalcurves.py
  ${CPYTHON_LIBRARY_DIR}/turtledemo/lindenmayer.py
  ${CPYTHON_LIBRARY_DIR}/turtledemo/minimal_hanoi.py
  ${CPYTHON_LIBRARY_DIR}/turtledemo/nim.py
  ${CPYTHON_LIBRARY_DIR}/turtledemo/paint.py
  ${CPYTHON_LIBRARY_DIR}/turtledemo/peace.py
  ${CPYTHON_LIBRARY_DIR}/turtledemo/penrose.py
  ${CPYTHON_LIBRARY_DIR}/turtledemo/planet_and_moon.py
  ${CPYTHON_LIBRARY_DIR}/turtledemo/rosette.py
  ${CPYTHON_LIBRARY_DIR}/turtledemo/round_dance.py
  ${CPYTHON_LIBRARY_DIR}/turtledemo/sorting_animate.py
  ${CPYTHON_LIBRARY_DIR}/turtledemo/tree.py
  ${CPYTHON_LIBRARY_DIR}/turtledemo/two_canvases.py
  ${CPYTHON_LIBRARY_DIR}/turtledemo/yinyang.py
  ${CPYTHON_LIBRARY_DIR}/urllib/__init__.py
  ${CPYTHON_LIBRARY_DIR}/urllib/error.py
  ${CPYTHON_LIBRARY_DIR}/urllib/parse.py
  ${CPYTHON_LIBRARY_DIR}/urllib/request.py
  ${CPYTHON_LIBRARY_DIR}/urllib/response.py
  ${CPYTHON_LIBRARY_DIR}/urllib/robotparser.py
  ${CPYTHON_LIBRARY_DIR}/uu.py
  ${CPYTHON_LIBRARY_DIR}/uuid.py
  ${CPYTHON_LIBRARY_DIR}/venv/__init__.py
  ${CPYTHON_LIBRARY_DIR}/venv/__main__.py
  ${CPYTHON_LIBRARY_DIR}/wave.py
  ${CPYTHON_LIBRARY_DIR}/weakref.py
  ${CPYTHON_LIBRARY_DIR}/webbrowser.py
  ${CPYTHON_LIBRARY_DIR}/wsgiref/__init__.py
  ${CPYTHON_LIBRARY_DIR}/wsgiref/handlers.py
  ${CPYTHON_LIBRARY_DIR}/wsgiref/headers.py
  ${CPYTHON_LIBRARY_DIR}/wsgiref/simple_server.py
  ${CPYTHON_LIBRARY_DIR}/wsgiref/util.py
  ${CPYTHON_LIBRARY_DIR}/wsgiref/validate.py
  ${CPYTHON_LIBRARY_DIR}/xdrlib.py
  ${CPYTHON_LIBRARY_DIR}/xml/__init__.py
  ${CPYTHON_LIBRARY_DIR}/xml/dom/NodeFilter.py
  ${CPYTHON_LIBRARY_DIR}/xml/dom/__init__.py
  ${CPYTHON_LIBRARY_DIR}/xml/dom/domreg.py
  ${CPYTHON_LIBRARY_DIR}/xml/dom/expatbuilder.py
  ${CPYTHON_LIBRARY_DIR}/xml/dom/minicompat.py
  ${CPYTHON_LIBRARY_DIR}/xml/dom/minidom.py
  ${CPYTHON_LIBRARY_DIR}/xml/dom/pulldom.py
  ${CPYTHON_LIBRARY_DIR}/xml/dom/xmlbuilder.py
  ${CPYTHON_LIBRARY_DIR}/xml/etree/ElementInclude.py
  ${CPYTHON_LIBRARY_DIR}/xml/etree/ElementPath.py
  ${CPYTHON_LIBRARY_DIR}/xml/etree/ElementTree.py
  ${CPYTHON_LIBRARY_DIR}/xml/etree/__init__.py
  ${CPYTHON_LIBRARY_DIR}/xml/etree/cElementTree.py
  ${CPYTHON_LIBRARY_DIR}/xml/parsers/__init__.py
  ${CPYTHON_LIBRARY_DIR}/xml/parsers/expat.py
  ${CPYTHON_LIBRARY_DIR}/xml/sax/__init__.py
  ${CPYTHON_LIBRARY_DIR}/xml/sax/_exceptions.py
  ${CPYTHON_LIBRARY_DIR}/xml/sax/expatreader.py
  ${CPYTHON_LIBRARY_DIR}/xml/sax/handler.py
  ${CPYTHON_LIBRARY_DIR}/xml/sax/saxutils.py
  ${CPYTHON_LIBRARY_DIR}/xml/sax/xmlreader.py
  ${CPYTHON_LIBRARY_DIR}/xmlrpc/__init__.py
  ${CPYTHON_LIBRARY_DIR}/xmlrpc/client.py
  ${CPYTHON_LIBRARY_DIR}/xmlrpc/server.py
  ${CPYTHON_LIBRARY_DIR}/zipapp.py
  ${CPYTHON_LIBRARY_DIR}/zipfile.py
  ${CPYTHON_LIBRARY_DIR}/zipimport.py)
string(
  REGEX
  REPLACE "(${CPYTHON_LIBRARY_DIR}\/)" "${BINARY_DIR_LIB}/"
  CPYTHON_MODULE_OUTPUT "${CPYTHON_MODULE_INPUT}")
set(UNFROZEN_LIBRARY_DIR "${CMAKE_SOURCE_DIR}/library")
set(
  UNFROZEN_MODULE_INPUT
  ${UNFROZEN_LIBRARY_DIR}/_collections.py
  ${UNFROZEN_LIBRARY_DIR}/_operator.py
  ${UNFROZEN_LIBRARY_DIR}/_string.py
  ${UNFROZEN_LIBRARY_DIR}/_sysconfigdata__darwin_.py
  ${UNFROZEN_LIBRARY_DIR}/_sysconfigdata__linux_.py
  ${UNFROZEN_LIBRARY_DIR}/array.py
  ${UNFROZEN_LIBRARY_DIR}/collections/__init__.py
  ${UNFROZEN_LIBRARY_DIR}/collections/abc.py
  ${UNFROZEN_LIBRARY_DIR}/dataclasses.py
  ${UNFROZEN_LIBRARY_DIR}/encodings/__init__.py
  ${UNFROZEN_LIBRARY_DIR}/encodings/aliases.py
  ${UNFROZEN_LIBRARY_DIR}/encodings/ascii.py
  ${UNFROZEN_LIBRARY_DIR}/encodings/latin_1.py
  ${UNFROZEN_LIBRARY_DIR}/encodings/utf_8.py
  ${UNFROZEN_LIBRARY_DIR}/gc.py
  ${UNFROZEN_LIBRARY_DIR}/itertools.py
  ${UNFROZEN_LIBRARY_DIR}/logging/__init__.py
  ${UNFROZEN_LIBRARY_DIR}/logging/config.py
  ${UNFROZEN_LIBRARY_DIR}/logging/handlers.py
  ${UNFROZEN_LIBRARY_DIR}/socket.py
  ${UNFROZEN_LIBRARY_DIR}/threading.py
  ${UNFROZEN_LIBRARY_DIR}/types.py
  ${UNFROZEN_LIBRARY_DIR}/typing.py
  ${UNFROZEN_LIBRARY_DIR}/unittest/__init__.py
  ${UNFROZEN_LIBRARY_DIR}/unittest/__main__.py
  ${UNFROZEN_LIBRARY_DIR}/unittest/case.py
  ${UNFROZEN_LIBRARY_DIR}/unittest/loader.py
  ${UNFROZEN_LIBRARY_DIR}/unittest/main.py
  ${UNFROZEN_LIBRARY_DIR}/unittest/mock.py
  ${UNFROZEN_LIBRARY_DIR}/unittest/result.py
  ${UNFROZEN_LIBRARY_DIR}/unittest/runner.py
  ${UNFROZEN_LIBRARY_DIR}/unittest/signals.py
  ${UNFROZEN_LIBRARY_DIR}/unittest/suite.py
  ${UNFROZEN_LIBRARY_DIR}/unittest/util.py)
string(
  REGEX
  REPLACE "(${UNFROZEN_LIBRARY_DIR}\/)" "${BINARY_DIR_LIB}/"
  UNFROZEN_MODULE_OUTPUT "${UNFROZEN_MODULE_INPUT}")

foreach(src_file ${CPYTHON_MODULE_INPUT})
  string(
    REGEX
    REPLACE "(${CPYTHON_LIBRARY_DIR}\/)" "${BINARY_DIR_LIB}/"
    dst_file "${src_file}")
  get_filename_component(dst_dir ${dst_file} DIRECTORY)
  add_custom_command(
    OUTPUT ${dst_file}
    VERBATIM
    # Create the directory structure
    COMMAND mkdir -p "${dst_dir}"
    # Copy from the source files into its destination location
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${src_file}" "${dst_file}"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    DEPENDS ${src_file}
    COMMENT "Copying to ${dst_file}")
endforeach()

foreach(src_file ${UNFROZEN_MODULE_INPUT})
  string(
    REGEX
    REPLACE "(${UNFROZEN_LIBRARY_DIR}\/)" "${BINARY_DIR_LIB}/"
    dst_file "${src_file}")
  get_filename_component(dst_dir ${dst_file} DIRECTORY)
  add_custom_command(
    OUTPUT ${dst_file}
    VERBATIM
    # Create the directory structure
    COMMAND mkdir -p "${dst_dir}"
    # Copy from the source files into its destination location
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${src_file}" "${dst_file}"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    DEPENDS ${src_file}
    COMMENT "Copying to ${dst_file}")
endforeach()

add_library(
  modules
  STATIC
  ${CPYTHON_MODULE_OUTPUT}
  ${UNFROZEN_MODULE_OUTPUT}
  ext/Modules/config.cpp)
target_compile_options(
  modules
  PUBLIC
  ${OS_DEFINE})
target_link_libraries(
  modules
  PRIVATE
  extension-modules)

add_executable(
  python
  main.cpp
  $<TARGET_OBJECTS:extension>)
target_compile_options(
  python
  PRIVATE
  ${PYRO_COMPILE_OPTIONS})
target_include_directories(
  python
  PRIVATE
  runtime)
target_link_libraries(
  python
  PRIVATE
  capi-headers
  runtime
  modules)
set_target_properties(
  python
  PROPERTIES
  ENABLE_EXPORTS 1)

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
  OBJECT
  runtime/array-module-test.cpp
  runtime/assembler-x64-test.cpp
  runtime/builtins-module-test.cpp
  runtime/bytearray-builtins-test.cpp
  runtime/bytecode-test.cpp
  runtime/bytes-builtins-test.cpp
  runtime/capi-handles-test.cpp
  runtime/code-builtins-test.cpp
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
  runtime/heap-profiler-test.cpp
  runtime/ic-test.cpp
  runtime/importlib-test.cpp
  runtime/int-builtins-test.cpp
  runtime/interpreter-test.cpp
  runtime/layout-test.cpp
  runtime/list-builtins-test.cpp
  runtime/marshal-module-test.cpp
  runtime/marshal-test.cpp
  runtime/memoryview-builtins-test.cpp
  runtime/memory-region-test.cpp
  runtime/module-builtins-test.cpp
  runtime/mutex-test.cpp
  runtime/mro-test.cpp
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
  runtime/under-codecs-module-test.cpp
  runtime/under-imp-module-test.cpp
  runtime/under-os-module-test.cpp
  runtime/under-path-module-test.cpp
  runtime/under-signal-module-test.cpp
  runtime/under-valgrind-module-test.cpp
  runtime/under-warnings-module-test.cpp
  runtime/utils-test.cpp
  runtime/vector-test.cpp)
target_compile_options(
  runtime-tests
  PRIVATE
  ${PYRO_COMPILE_OPTIONS})
target_include_directories(
  runtime-tests
  PRIVATE
  $<TARGET_PROPERTY:benchmark,INTERFACE_INCLUDE_DIRECTORIES>
  $<TARGET_PROPERTY:gmock,INTERFACE_INCLUDE_DIRECTORIES>
  $<TARGET_PROPERTY:gtest,INTERFACE_INCLUDE_DIRECTORIES>
  $<TARGET_PROPERTY:runtime,INTERFACE_INCLUDE_DIRECTORIES>
  ${FROZEN_MODULE_OUTPUT_DIR})
add_dependencies(
  runtime-tests
  frozen-sources)

set(
  EXTENSION_TESTS
  ext/Modules/_datetimemodule-test.cpp
  ext/Modules/config-test.cpp
  ext/Modules/gcmodule-test.cpp
  ext/Modules/getbuildinfo-test.cpp
  ext/Modules/main-test.cpp
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
  ext/Objects/funcobject-test.cpp
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
  ext/Python/getcompiler-test.cpp
  ext/Python/getplatform-test.cpp
  ext/Python/getversion-test.cpp
  ext/Python/import-test.cpp
  ext/Python/modsupport-test.cpp
  ext/Python/mysnprintf-test.cpp
  ext/Python/pathconfig-test.cpp
  ext/Python/pyhash-test.cpp
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
  OBJECT
  ${EXTENSION_TESTS})
target_compile_options(
  extension-tests
  PRIVATE
  ${PYRO_COMPILE_OPTIONS})
target_include_directories(
  extension-tests
  PRIVATE
  ext/test
  $<TARGET_PROPERTY:capi-headers,INTERFACE_INCLUDE_DIRECTORIES>
  $<TARGET_PROPERTY:gmock,INTERFACE_INCLUDE_DIRECTORIES>
  $<TARGET_PROPERTY:gtest,INTERFACE_INCLUDE_DIRECTORIES>)

add_executable(
  python-tests
  runtime/python-test.cpp
  $<TARGET_OBJECTS:extension>
  $<TARGET_OBJECTS:runtime-tests>
  $<TARGET_OBJECTS:extension-tests>)
target_compile_options(
  python-tests
  PRIVATE
  ${PYRO_COMPILE_OPTIONS})
target_link_libraries(
  python-tests
  PRIVATE
  benchmark
  gtest
  runtime)

# Extension tests with CPython
set(CPYTHON_CONFIGURE_FLAGS)
set(CPYTHON_CFLAGS_NODIST)
set(CPYTHON_CPPFLAGS)
set(CPYTHON_LDFLAGS)
if (${CMAKE_BUILD_TYPE} MATCHES "^Debug")
  set(CPYTHON_CONFIGURE_FLAGS "${CPYTHON_CONFIGURE_FLAGS} --with-assertions")
  set(CPYTHON_CFLAGS_NODIST "${CPYTHON_CFLAGS_NODIST} -O0")
endif()
set(LIBPYTHON_BIN "libpython3.7m.a")
set(CPYTHON_BUILD_DIR ${CMAKE_BINARY_DIR}/third-party/cpython)
file(MAKE_DIRECTORY ${CPYTHON_BUILD_DIR})
file(GLOB_RECURSE CPYTHON_FILES ${CPYTHON_DIR}/*)
foreach(dir
    ${BZIP2_INCLUDE_DIRS}
    ${FFI_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIRS}
    ${READLINE_INCLUDE_DIRS}
    ${XZ_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIRS})
  set(CPYTHON_CPPFLAGS "${CPYTHON_CPPFLAGS} -I${dir}")
endforeach()
foreach(dir
    ${BZIP2_LIBRARY_DIRS}
    ${FFI_LIBRARY_DIRS}
    ${OPENSSL_LIBRARY_DIRS}
    ${READLINE_LIBRARY_DIRS}
    ${XZ_LIBRARY_DIRS}
    ${ZLIB_LIBRARY_DIRS})
  set(CPYTHON_LDFLAGS "${CPYTHON_LDFLAGS} -L${dir}")
endforeach()
# Write commands into a script to avoid quoting problems.
file(GENERATE OUTPUT ${CPYTHON_BUILD_DIR}/build.sh CONTENT "
export CC=\"${CMAKE_C_COMPILER}\"
export CXX=\"${CMAKE_CXX_COMPILER}\"
export CFLAGS_NODIST=\"${CPYTHON_CFLAGS_NODIST}\"
export CPPFLAGS=\"${CPYTHON_CPPFLAGS}\"
export LDFLAGS=\"${CPYTHON_LDFLAGS}\"
rm -f ${CPYTHON_BUILD_DIR}/Modules/Setup
${CPYTHON_DIR}/configure ${CPYTHON_CONFIGURE_FLAGS}
make -j
")
add_custom_command(
  OUTPUT ${CPYTHON_BUILD_DIR}/${LIBPYTHON_BIN}
  COMMAND cat ${CPYTHON_BUILD_DIR}/build.sh
  COMMAND /bin/sh ${CPYTHON_BUILD_DIR}/build.sh
  DEPENDS ${CPYTHON_FILES} ${CPYTHON_BUILD_DIR}/build.sh
  WORKING_DIRECTORY ${CPYTHON_BUILD_DIR}
  USES_TERMINAL)
add_custom_target(
  cpython
  DEPENDS ${CPYTHON_BUILD_DIR}/${LIBPYTHON_BIN})
add_library(libpython STATIC IMPORTED)
add_dependencies(libpython cpython)
set_target_properties(
  libpython
  PROPERTIES IMPORTED_LOCATION ${CPYTHON_BUILD_DIR}/${LIBPYTHON_BIN})

if (ENABLE_CPYTHON_TESTS)
  add_executable(
    cpython-tests
    EXCLUDE_FROM_ALL
    ${EXTENSION_TESTS}
    runtime/python-test.cpp)
  target_compile_options(
    cpython-tests
    PUBLIC
    PRIVATE
    ${PYRO_COMPILE_OPTIONS})
  target_include_directories(
    cpython-tests
    PRIVATE
    ${CMAKE_BINARY_DIR}/third-party/cpython
    ext/test
    third-party/cpython/Include)
  target_link_libraries(
    cpython-tests
    PRIVATE
    benchmark
    dl
    gmock
    gtest
    libpython
    util)
  set_target_properties(
    cpython-tests
    PROPERTIES
    ENABLE_EXPORTS 1)
  # Lib/ is needed for cpython-test, copy it into the build directory
  add_custom_command(
    TARGET cpython-tests
    POST_BUILD
    COMMAND cp -a ${CPYTHON_DIR}/Lib/. ${CPYTHON_BUILD_DIR}/Lib/)
endif()
