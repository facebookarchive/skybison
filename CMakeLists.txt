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

set(BINARY_DIR_HEADERS "${CMAKE_BINARY_DIR}/Include")
file(MAKE_DIRECTORY "${BINARY_DIR_HEADERS}")
set(
  C_API_HEADERS_INPUT
  ${CMAKE_SOURCE_DIR}/capi/cpython-data.h
  ${CMAKE_SOURCE_DIR}/capi/cpython-func.h
  ${CMAKE_SOURCE_DIR}/capi/cpython-types.h
  ${CMAKE_SOURCE_DIR}/ext/Include/Python.h
  ${CMAKE_SOURCE_DIR}/ext/config/pyconfig-linux.h
  ${CMAKE_SOURCE_DIR}/ext/config/pyconfig-osx.h
  ${CMAKE_SOURCE_DIR}/ext/config/pyconfig.h)
string(
  REGEX
  REPLACE "(${CMAKE_SOURCE_DIR}\/)" "${CMAKE_BINARY_DIR}/"
  C_API_HEADERS_OUTPUT "${C_API_HEADERS_INPUT}")
add_custom_command(
  OUTPUT ${C_API_HEADERS_OUTPUT}
  COMMAND
  cmake -E copy_if_different ${C_API_HEADERS_INPUT} ${BINARY_DIR_HEADERS}
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  DEPENDS ${C_API_HEADERS_INPUT}
  COMMENT "Generating Pyro Headers")

add_library(capi-headers INTERFACE)
target_sources(
  capi-headers
  INTERFACE
  ${C_API_HEADERS_OUTPUT})
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
  runtime/module-proxy-builtins.cpp
  runtime/module-proxy-builtins.h
  runtime/mutex.h
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
  ${PYRO_COMPILE_OPTIONS}
  -Wmissing-declarations)
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

# Prepare build dir with CPython's directories
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/Modules")
file(WRITE "${CMAKE_BINARY_DIR}/Modules/Setup.local" "Pretend Setup exists")

set(CPYTHON_DIR ${CMAKE_SOURCE_DIR}/third-party/cpython)
set(
  CPYTHON_HEADERS_INPUT
  ${CPYTHON_DIR}/Include/Python-ast.h
  ${CPYTHON_DIR}/Include/abstract.h
  ${CPYTHON_DIR}/Include/accu.h
  ${CPYTHON_DIR}/Include/asdl.h
  ${CPYTHON_DIR}/Include/ast.h
  ${CPYTHON_DIR}/Include/bitset.h
  ${CPYTHON_DIR}/Include/bltinmodule.h
  ${CPYTHON_DIR}/Include/boolobject.h
  ${CPYTHON_DIR}/Include/bytearrayobject.h
  ${CPYTHON_DIR}/Include/bytes_methods.h
  ${CPYTHON_DIR}/Include/bytesobject.h
  ${CPYTHON_DIR}/Include/cellobject.h
  ${CPYTHON_DIR}/Include/ceval.h
  ${CPYTHON_DIR}/Include/classobject.h
  ${CPYTHON_DIR}/Include/code.h
  ${CPYTHON_DIR}/Include/codecs.h
  ${CPYTHON_DIR}/Include/compile.h
  ${CPYTHON_DIR}/Include/complexobject.h
  ${CPYTHON_DIR}/Include/datetime.h
  ${CPYTHON_DIR}/Include/descrobject.h
  ${CPYTHON_DIR}/Include/dictobject.h
  ${CPYTHON_DIR}/Include/dtoa.h
  ${CPYTHON_DIR}/Include/dynamic_annotations.h
  ${CPYTHON_DIR}/Include/enumobject.h
  ${CPYTHON_DIR}/Include/errcode.h
  ${CPYTHON_DIR}/Include/eval.h
  ${CPYTHON_DIR}/Include/fileobject.h
  ${CPYTHON_DIR}/Include/fileutils.h
  ${CPYTHON_DIR}/Include/floatobject.h
  ${CPYTHON_DIR}/Include/frameobject.h
  ${CPYTHON_DIR}/Include/funcobject.h
  ${CPYTHON_DIR}/Include/genobject.h
  ${CPYTHON_DIR}/Include/graminit.h
  ${CPYTHON_DIR}/Include/grammar.h
  ${CPYTHON_DIR}/Include/import.h
  ${CPYTHON_DIR}/Include/intrcheck.h
  ${CPYTHON_DIR}/Include/iterobject.h
  ${CPYTHON_DIR}/Include/listobject.h
  ${CPYTHON_DIR}/Include/longintrepr.h
  ${CPYTHON_DIR}/Include/longobject.h
  ${CPYTHON_DIR}/Include/marshal.h
  ${CPYTHON_DIR}/Include/memoryobject.h
  ${CPYTHON_DIR}/Include/metagrammar.h
  ${CPYTHON_DIR}/Include/methodobject.h
  ${CPYTHON_DIR}/Include/modsupport.h
  ${CPYTHON_DIR}/Include/moduleobject.h
  ${CPYTHON_DIR}/Include/namespaceobject.h
  ${CPYTHON_DIR}/Include/node.h
  ${CPYTHON_DIR}/Include/object.h
  ${CPYTHON_DIR}/Include/objimpl.h
  ${CPYTHON_DIR}/Include/odictobject.h
  ${CPYTHON_DIR}/Include/opcode.h
  ${CPYTHON_DIR}/Include/osdefs.h
  ${CPYTHON_DIR}/Include/osmodule.h
  ${CPYTHON_DIR}/Include/parsetok.h
  ${CPYTHON_DIR}/Include/patchlevel.h
  ${CPYTHON_DIR}/Include/pgen.h
  ${CPYTHON_DIR}/Include/pgenheaders.h
  ${CPYTHON_DIR}/Include/py_curses.h
  ${CPYTHON_DIR}/Include/pyarena.h
  ${CPYTHON_DIR}/Include/pyatomic.h
  ${CPYTHON_DIR}/Include/pycapsule.h
  ${CPYTHON_DIR}/Include/pyctype.h
  ${CPYTHON_DIR}/Include/pydebug.h
  ${CPYTHON_DIR}/Include/pydtrace.h
  ${CPYTHON_DIR}/Include/pyerrors.h
  ${CPYTHON_DIR}/Include/pyexpat.h
  ${CPYTHON_DIR}/Include/pyfpe.h
  ${CPYTHON_DIR}/Include/pygetopt.h
  ${CPYTHON_DIR}/Include/pyhash.h
  ${CPYTHON_DIR}/Include/pylifecycle.h
  ${CPYTHON_DIR}/Include/pymacro.h
  ${CPYTHON_DIR}/Include/pymath.h
  ${CPYTHON_DIR}/Include/pymem.h
  ${CPYTHON_DIR}/Include/pyport.h
  ${CPYTHON_DIR}/Include/pystate.h
  ${CPYTHON_DIR}/Include/pystrcmp.h
  ${CPYTHON_DIR}/Include/pystrhex.h
  ${CPYTHON_DIR}/Include/pystrtod.h
  ${CPYTHON_DIR}/Include/pythonrun.h
  ${CPYTHON_DIR}/Include/pythread.h
  ${CPYTHON_DIR}/Include/pytime.h
  ${CPYTHON_DIR}/Include/rangeobject.h
  ${CPYTHON_DIR}/Include/setobject.h
  ${CPYTHON_DIR}/Include/sliceobject.h
  ${CPYTHON_DIR}/Include/structmember.h
  ${CPYTHON_DIR}/Include/structseq.h
  ${CPYTHON_DIR}/Include/symtable.h
  ${CPYTHON_DIR}/Include/sysmodule.h
  ${CPYTHON_DIR}/Include/token.h
  ${CPYTHON_DIR}/Include/traceback.h
  ${CPYTHON_DIR}/Include/tupleobject.h
  ${CPYTHON_DIR}/Include/typeslots.h
  ${CPYTHON_DIR}/Include/ucnhash.h
  ${CPYTHON_DIR}/Include/unicodeobject.h
  ${CPYTHON_DIR}/Include/warnings.h
  ${CPYTHON_DIR}/Include/weakrefobject.h)
string(
  REGEX
  REPLACE "(${CPYTHON_DIR}/Include\/)" "${BINARY_DIR_HEADERS}/"
  CPYTHON_HEADERS_OUTPUT "${CPYTHON_HEADERS_INPUT}")
add_custom_command(
  OUTPUT
    ${CPYTHON_HEADERS_OUTPUT}
  COMMAND
    python3 util/generate_cpython_sources.py
    -sources ${CPYTHON_HEADERS_INPUT}
    -modified ${C_API_HEADERS_INPUT}
    -output_dir ${CMAKE_BINARY_DIR}
  WORKING_DIRECTORY
    ${CMAKE_SOURCE_DIR}
  DEPENDS
    ${CPYTHON_HEADERS_INPUT}
    ${C_API_HEADERS_INPUT}
    util/generate_cpython_sources.py
  COMMENT "Generating CPython Headers")

set(
  CPYTHON_SOURCES
  ${CPYTHON_DIR}/Modules/_asynciomodule.c
  ${CPYTHON_DIR}/Modules/_bisectmodule.c
  ${CPYTHON_DIR}/Modules/_codecsmodule.c
  ${CPYTHON_DIR}/Modules/_collectionsmodule.c
  ${CPYTHON_DIR}/Modules/_cryptmodule.c
  ${CPYTHON_DIR}/Modules/_csv.c
  ${CPYTHON_DIR}/Modules/_datetimemodule.c
  ${CPYTHON_DIR}/Modules/_functoolsmodule.c
  ${CPYTHON_DIR}/Modules/_hashopenssl.c
  ${CPYTHON_DIR}/Modules/_heapqmodule.c
  ${CPYTHON_DIR}/Modules/_localemodule.c
  ${CPYTHON_DIR}/Modules/_math.c
  ${CPYTHON_DIR}/Modules/_math.h
  ${CPYTHON_DIR}/Modules/_opcode.c
  ${CPYTHON_DIR}/Modules/_operator.c
  ${CPYTHON_DIR}/Modules/_posixsubprocess.c
  ${CPYTHON_DIR}/Modules/_randommodule.c
  ${CPYTHON_DIR}/Modules/_sre.c
  ${CPYTHON_DIR}/Modules/_ssl.c
  ${CPYTHON_DIR}/Modules/_ssl_data.h
  ${CPYTHON_DIR}/Modules/_stat.c
  ${CPYTHON_DIR}/Modules/_struct.c
  ${CPYTHON_DIR}/Modules/_testimportmultiple.c
  ${CPYTHON_DIR}/Modules/_testmultiphase.c
  ${CPYTHON_DIR}/Modules/_weakref.c
  ${CPYTHON_DIR}/Modules/addrinfo.h
  ${CPYTHON_DIR}/Modules/atexitmodule.c
  ${CPYTHON_DIR}/Modules/audioop.c
  ${CPYTHON_DIR}/Modules/binascii.c
  ${CPYTHON_DIR}/Modules/clinic/_asynciomodule.c.h
  ${CPYTHON_DIR}/Modules/clinic/_codecsmodule.c.h
  ${CPYTHON_DIR}/Modules/clinic/_cryptmodule.c.h
  ${CPYTHON_DIR}/Modules/clinic/_datetimemodule.c.h
  ${CPYTHON_DIR}/Modules/clinic/_hashopenssl.c.h
  ${CPYTHON_DIR}/Modules/clinic/_opcode.c.h
  ${CPYTHON_DIR}/Modules/clinic/_sre.c.h
  ${CPYTHON_DIR}/Modules/clinic/_ssl.c.h
  ${CPYTHON_DIR}/Modules/clinic/_weakref.c.h
  ${CPYTHON_DIR}/Modules/clinic/arraymodule.c.h
  ${CPYTHON_DIR}/Modules/clinic/audioop.c.h
  ${CPYTHON_DIR}/Modules/clinic/binascii.c.h
  ${CPYTHON_DIR}/Modules/clinic/cmathmodule.c.h
  ${CPYTHON_DIR}/Modules/clinic/fcntlmodule.c.h
  ${CPYTHON_DIR}/Modules/clinic/grpmodule.c.h
  ${CPYTHON_DIR}/Modules/clinic/md5module.c.h
  ${CPYTHON_DIR}/Modules/clinic/posixmodule.c.h
  ${CPYTHON_DIR}/Modules/clinic/pwdmodule.c.h
  ${CPYTHON_DIR}/Modules/clinic/sha1module.c.h
  ${CPYTHON_DIR}/Modules/clinic/sha256module.c.h
  ${CPYTHON_DIR}/Modules/clinic/sha512module.c.h
  ${CPYTHON_DIR}/Modules/clinic/unicodedata.c.h
  ${CPYTHON_DIR}/Modules/clinic/zlibmodule.c.h
  ${CPYTHON_DIR}/Modules/cmathmodule.c
  ${CPYTHON_DIR}/Modules/errnomodule.c
  ${CPYTHON_DIR}/Modules/fcntlmodule.c
  ${CPYTHON_DIR}/Modules/fpectlmodule.c
  ${CPYTHON_DIR}/Modules/fpetestmodule.c
  ${CPYTHON_DIR}/Modules/getbuildinfo.c
  ${CPYTHON_DIR}/Modules/getpath.c
  ${CPYTHON_DIR}/Modules/grpmodule.c
  ${CPYTHON_DIR}/Modules/hashlib.h
  ${CPYTHON_DIR}/Modules/hashtable.c
  ${CPYTHON_DIR}/Modules/hashtable.h
  ${CPYTHON_DIR}/Modules/mathmodule.c
  ${CPYTHON_DIR}/Modules/mmapmodule.c
  ${CPYTHON_DIR}/Modules/posixmodule.c
  ${CPYTHON_DIR}/Modules/posixmodule.h
  ${CPYTHON_DIR}/Modules/pwdmodule.c
  ${CPYTHON_DIR}/Modules/resource.c
  ${CPYTHON_DIR}/Modules/rotatingtree.c
  ${CPYTHON_DIR}/Modules/rotatingtree.h
  ${CPYTHON_DIR}/Modules/selectmodule.c
  ${CPYTHON_DIR}/Modules/socketmodule.c
  ${CPYTHON_DIR}/Modules/socketmodule.h
  ${CPYTHON_DIR}/Modules/sre.h
  ${CPYTHON_DIR}/Modules/sre_constants.h
  ${CPYTHON_DIR}/Modules/sre_lib.h
  ${CPYTHON_DIR}/Modules/symtablemodule.c
  ${CPYTHON_DIR}/Modules/syslogmodule.c
  ${CPYTHON_DIR}/Modules/termios.c
  ${CPYTHON_DIR}/Modules/testcapi_long.h
  ${CPYTHON_DIR}/Modules/timemodule.c
  ${CPYTHON_DIR}/Modules/unicodedata.c
  ${CPYTHON_DIR}/Modules/unicodedata_db.h
  ${CPYTHON_DIR}/Modules/unicodename_db.h
  ${CPYTHON_DIR}/Modules/xxlimited.c
  ${CPYTHON_DIR}/Modules/xxmodule.c
  ${CPYTHON_DIR}/Modules/xxsubtype.c
  ${CPYTHON_DIR}/Modules/zlib/adler32.c
  ${CPYTHON_DIR}/Modules/zlib/crc32.c
  ${CPYTHON_DIR}/Modules/zlib/deflate.c
  ${CPYTHON_DIR}/Modules/zlib/inffast.c
  ${CPYTHON_DIR}/Modules/zlib/inflate.c
  ${CPYTHON_DIR}/Modules/zlib/inftrees.c
  ${CPYTHON_DIR}/Modules/zlib/trees.c
  ${CPYTHON_DIR}/Modules/zlib/zconf.h
  ${CPYTHON_DIR}/Modules/zlib/zlib.h
  ${CPYTHON_DIR}/Modules/zlib/zutil.c
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
  ${CPYTHON_DIR}/Parser/parsetok_pgen.c
  ${CPYTHON_DIR}/Parser/pgen.c
  ${CPYTHON_DIR}/Parser/printgrammar.c
  ${CPYTHON_DIR}/Parser/tokenizer.c
  ${CPYTHON_DIR}/Parser/tokenizer.h
  ${CPYTHON_DIR}/Parser/tokenizer_pgen.c
  ${CPYTHON_DIR}/Python/Python-ast.c
  ${CPYTHON_DIR}/Python/asdl.c
  ${CPYTHON_DIR}/Python/ast.c
  ${CPYTHON_DIR}/Python/dtoa.c
  ${CPYTHON_DIR}/Python/fileutils.c
  ${CPYTHON_DIR}/Python/future.c
  ${CPYTHON_DIR}/Python/graminit.c
  ${CPYTHON_DIR}/Python/mystrtoul.c
  ${CPYTHON_DIR}/Python/pyarena.c
  ${CPYTHON_DIR}/Python/pyctype.c
  ${CPYTHON_DIR}/Python/pyhash.c
  ${CPYTHON_DIR}/Python/pystrcmp.c
  ${CPYTHON_DIR}/Python/pystrhex.c
  ${CPYTHON_DIR}/Python/pystrtod.c
  ${CPYTHON_DIR}/Python/pytime.c
  ${CPYTHON_DIR}/Python/symtable.c
  ${CPYTHON_DIR}/Python/wordcode_helpers.h)
set_property(
  SOURCE ${CPYTHON_DIR}/Python/dynload_shlib.c
  PROPERTY COMPILE_DEFINITIONS SOABI="3.6.5")
set_property(
  SOURCE ${CPYTHON_DIR}/Modules/getpath.c
  PROPERTY COMPILE_DEFINITIONS
      PREFIX=""
      EXEC_PREFIX=""
      VERSION="3.6"
      VPATH="."
      PYTHONPATH="")

# This target is built in util/run_clang_tidy.py:create_compile_commands() to
# ensure all generated files are present when clang-tidy runs.
add_custom_target(
  cpython-sources
  DEPENDS
  ${CPYTHON_HEADERS_OUTPUT}
  ${CPYTHON_SOURCES})

find_package(Threads)
add_library(
  extension-modules
  STATIC
  ${CPYTHON_HEADERS_OUTPUT}
  ${CPYTHON_SOURCES}
  ${PYRO_PUBLIC_HEADERS_OUTPUT})
add_dependencies(extension-modules cpython-sources)
target_include_directories(
  extension-modules
  PUBLIC
  ${BINARY_DIR_HEADERS}
  ${CPYTHON_DIR}/Modules
  ${CPYTHON_DIR}/Modules/zlib
  ${OPENSSL_INCLUDE_DIR})
target_compile_options(
  extension-modules
  PUBLIC
  ${OS_DEFINE}
  PRIVATE
  "-Werror"
  "-DPy_BUILD_CORE"
  # TODO(T48643833): Remove this define to get _hashlib.scrypt.
  "-DOPENSSL_NO_SCRYPT")
target_link_libraries(
  extension-modules
  PUBLIC
  util
  extension
  capi-headers
  Threads::Threads
  OpenSSL::Crypto
  OpenSSL::SSL)

add_library(
  extension
  STATIC
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
  ext/Modules/gcmodule.cpp
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
  ext/Python/pylifecycle.cpp
  ext/Python/pystate.cpp
  ext/Python/pythonrun.cpp
  ext/Python/random.cpp
  ext/Python/sysmodule.cpp
  ext/Python/thread.cpp
  ext/Python/traceback.cpp)
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
  extension-modules)

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
  extension-modules)

# TODO(T58756349): Move ext/Modules/main.cpp to extension once circular
# dependencies no longer exist with CPython.
add_executable(
  python
  main.cpp
  ext/Modules/main.cpp)
target_compile_options(
  python
  PRIVATE
  ${PYRO_COMPILE_OPTIONS})
target_include_directories(
  python
  INTERFACE
  capi
  PRIVATE
  runtime)
target_link_libraries(
  python
  PRIVATE
  runtime
  modules
  ${PYRO_STATIC_LINKING})
set_target_properties(
  python
  PROPERTIES
  ENABLE_EXPORTS 1
)

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
  ext/Python/getplatform-test.cpp
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
  extension-modules
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
