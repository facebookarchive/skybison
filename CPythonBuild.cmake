# Build a modified CPython library.
#
# Imported targets
# ^^^^^^^^^^^^^^^^
#
# This module defines the following targets:
#
# ``cpython``
#   The CPython library with the modified symbols.
#
# Cache variables
# ^^^^^^^^^^^^^^^
#
# The following cache variables may also be set:
#
# ``CPYTHON_MODIFIED_HEADERS``
#   A list of headers files that contain redifinitions of symbols in CPython.
#

# DIR to the CPython directory
set(CPYTHON_DIR ${CMAKE_SOURCE_DIR}/third-party/cpython)

# DIR to the generated CPython headers
set(CPYTHON_GEN_DIR ${CMAKE_BINARY_DIR}/cpython)

# Obtain the list of all CPython headers and sources
file(
  GLOB_RECURSE
  CPYTHON_SOURCES
  ${CPYTHON_DIR}/Include/*
  ${CPYTHON_DIR}/Parser/*)

macro(remove_cpython_source value)
  list(REMOVE_ITEM CPYTHON_SOURCES "${CPYTHON_DIR}/${value}")
endmacro()

# Remove the main function from the Parser
remove_cpython_source("Parser/pgenmain.c")

# Modules sources that need to be processed with the argument clinic
set(
  CPYTHON_SOURCES_WITH_CLINIC_ANNOTATIONS
  "${CPYTHON_DIR}/Modules/_asynciomodule.c"
  "${CPYTHON_DIR}/Modules/_blake2/blake2b_impl.c"
  "${CPYTHON_DIR}/Modules/_blake2/blake2s_impl.c"
  "${CPYTHON_DIR}/Modules/_codecsmodule.c"
  "${CPYTHON_DIR}/Modules/_cryptmodule.c"
  "${CPYTHON_DIR}/Modules/_datetimemodule.c"
  "${CPYTHON_DIR}/Modules/_io/_iomodule.c"
  "${CPYTHON_DIR}/Modules/_io/bufferedio.c"
  "${CPYTHON_DIR}/Modules/_io/bytesio.c"
  "${CPYTHON_DIR}/Modules/_io/fileio.c"
  "${CPYTHON_DIR}/Modules/_io/iobase.c"
  "${CPYTHON_DIR}/Modules/_io/stringio.c"
  "${CPYTHON_DIR}/Modules/_io/textio.c"
  "${CPYTHON_DIR}/Modules/_opcode.c"
  "${CPYTHON_DIR}/Modules/_sre.c"
  "${CPYTHON_DIR}/Modules/_weakref.c"
  "${CPYTHON_DIR}/Modules/arraymodule.c"
  "${CPYTHON_DIR}/Modules/audioop.c"
  "${CPYTHON_DIR}/Modules/binascii.c"
  "${CPYTHON_DIR}/Modules/cjkcodecs/multibytecodec.c"
  "${CPYTHON_DIR}/Modules/cmathmodule.c"
  "${CPYTHON_DIR}/Modules/fcntlmodule.c"
  "${CPYTHON_DIR}/Modules/grpmodule.c"
  "${CPYTHON_DIR}/Modules/md5module.c"
  "${CPYTHON_DIR}/Modules/posixmodule.c"
  "${CPYTHON_DIR}/Modules/pwdmodule.c"
  "${CPYTHON_DIR}/Modules/sha1module.c"
  "${CPYTHON_DIR}/Modules/sha256module.c"
  "${CPYTHON_DIR}/Modules/sha512module.c"
  "${CPYTHON_DIR}/Modules/unicodedata.c")

# Add some Python files all the core Modules sources
set(
  CPYTHON_SOURCES
  ${CPYTHON_SOURCES}
  ${CPYTHON_SOURCES_WITH_CLINIC_ANNOTATIONS}
  "${CPYTHON_DIR}/Objects/accu.c"
  "${CPYTHON_DIR}/Python/dtoa.c"
  "${CPYTHON_DIR}/Python/fileutils.c"
  "${CPYTHON_DIR}/Python/mystrtoul.c"
  "${CPYTHON_DIR}/Python/pyarena.c"
  "${CPYTHON_DIR}/Python/pyctype.c"
  "${CPYTHON_DIR}/Python/pystrcmp.c"
  "${CPYTHON_DIR}/Python/pystrhex.c"
  "${CPYTHON_DIR}/Python/pystrtod.c"
  "${CPYTHON_DIR}/Python/pytime.c"
  "${CPYTHON_DIR}/Modules/_bisectmodule.c"
  "${CPYTHON_DIR}/Modules/_blake2/blake2module.c"
  "${CPYTHON_DIR}/Modules/_blake2/blake2ns.h"
  "${CPYTHON_DIR}/Modules/_blake2/impl/blake2-config.h"
  "${CPYTHON_DIR}/Modules/_blake2/impl/blake2-impl.h"
  "${CPYTHON_DIR}/Modules/_blake2/impl/blake2.h"
  "${CPYTHON_DIR}/Modules/_blake2/impl/blake2b-load-sse2.h"
  "${CPYTHON_DIR}/Modules/_blake2/impl/blake2b-load-sse41.h"
  "${CPYTHON_DIR}/Modules/_blake2/impl/blake2b-ref.c"
  "${CPYTHON_DIR}/Modules/_blake2/impl/blake2b-round.h"
  "${CPYTHON_DIR}/Modules/_blake2/impl/blake2b.c"
  "${CPYTHON_DIR}/Modules/_blake2/impl/blake2s-load-sse2.h"
  "${CPYTHON_DIR}/Modules/_blake2/impl/blake2s-load-sse41.h"
  "${CPYTHON_DIR}/Modules/_blake2/impl/blake2s-load-xop.h"
  "${CPYTHON_DIR}/Modules/_blake2/impl/blake2s-ref.c"
  "${CPYTHON_DIR}/Modules/_blake2/impl/blake2s-round.h"
  "${CPYTHON_DIR}/Modules/_blake2/impl/blake2s.c"
  "${CPYTHON_DIR}/Modules/_collectionsmodule.c"
  "${CPYTHON_DIR}/Modules/_csv.c"
  "${CPYTHON_DIR}/Modules/_functoolsmodule.c"
  "${CPYTHON_DIR}/Modules/_heapqmodule.c"
  "${CPYTHON_DIR}/Modules/_io/_iomodule.h"
  "${CPYTHON_DIR}/Modules/_localemodule.c"
  "${CPYTHON_DIR}/Modules/_math.c"
  "${CPYTHON_DIR}/Modules/_math.h"
  "${CPYTHON_DIR}/Modules/_operator.c"
  "${CPYTHON_DIR}/Modules/_posixsubprocess.c"
  "${CPYTHON_DIR}/Modules/_randommodule.c"
  "${CPYTHON_DIR}/Modules/_stat.c"
  "${CPYTHON_DIR}/Modules/_struct.c"
  "${CPYTHON_DIR}/Modules/_testbuffer.c"
  "${CPYTHON_DIR}/Modules/_testimportmultiple.c"
  "${CPYTHON_DIR}/Modules/_testmultiphase.c"
  "${CPYTHON_DIR}/Modules/addrinfo.h"
  "${CPYTHON_DIR}/Modules/atexitmodule.c"
  "${CPYTHON_DIR}/Modules/cjkcodecs/_codecs_cn.c"
  "${CPYTHON_DIR}/Modules/cjkcodecs/_codecs_hk.c"
  "${CPYTHON_DIR}/Modules/cjkcodecs/_codecs_iso2022.c"
  "${CPYTHON_DIR}/Modules/cjkcodecs/_codecs_kr.c"
  "${CPYTHON_DIR}/Modules/cjkcodecs/_codecs_tw.c"
  "${CPYTHON_DIR}/Modules/cjkcodecs/alg_jisx0201.h"
  "${CPYTHON_DIR}/Modules/cjkcodecs/cjkcodecs.h"
  "${CPYTHON_DIR}/Modules/cjkcodecs/emu_jisx0213_2000.h"
  "${CPYTHON_DIR}/Modules/cjkcodecs/mappings_cn.h"
  "${CPYTHON_DIR}/Modules/cjkcodecs/mappings_hk.h"
  "${CPYTHON_DIR}/Modules/cjkcodecs/mappings_jisx0213_pair.h"
  "${CPYTHON_DIR}/Modules/cjkcodecs/mappings_jp.h"
  "${CPYTHON_DIR}/Modules/cjkcodecs/mappings_kr.h"
  "${CPYTHON_DIR}/Modules/cjkcodecs/mappings_tw.h"
  "${CPYTHON_DIR}/Modules/cjkcodecs/multibytecodec.h"
  "${CPYTHON_DIR}/Modules/errnomodule.c"
  "${CPYTHON_DIR}/Modules/fpectlmodule.c"
  "${CPYTHON_DIR}/Modules/fpetestmodule.c"
  "${CPYTHON_DIR}/Modules/getbuildinfo.c"
  "${CPYTHON_DIR}/Modules/getpath.c"
  "${CPYTHON_DIR}/Modules/hashlib.h"
  "${CPYTHON_DIR}/Modules/hashtable.c"
  "${CPYTHON_DIR}/Modules/hashtable.h"
  "${CPYTHON_DIR}/Modules/itertoolsmodule.c"
  "${CPYTHON_DIR}/Modules/mathmodule.c"
  "${CPYTHON_DIR}/Modules/mmapmodule.c"
  "${CPYTHON_DIR}/Modules/parsermodule.c"
  "${CPYTHON_DIR}/Modules/posixmodule.h"
  "${CPYTHON_DIR}/Modules/resource.c"
  "${CPYTHON_DIR}/Modules/rotatingtree.c"
  "${CPYTHON_DIR}/Modules/rotatingtree.h"
  "${CPYTHON_DIR}/Modules/selectmodule.c"
  "${CPYTHON_DIR}/Modules/socketmodule.c"
  "${CPYTHON_DIR}/Modules/socketmodule.h"
  "${CPYTHON_DIR}/Modules/sre.h"
  "${CPYTHON_DIR}/Modules/sre_constants.h"
  "${CPYTHON_DIR}/Modules/sre_lib.h"
  "${CPYTHON_DIR}/Modules/symtablemodule.c"
  "${CPYTHON_DIR}/Modules/syslogmodule.c"
  "${CPYTHON_DIR}/Modules/termios.c"
  "${CPYTHON_DIR}/Modules/testcapi_long.h"
  "${CPYTHON_DIR}/Modules/timemodule.c"
  "${CPYTHON_DIR}/Modules/unicodedata_db.h"
  "${CPYTHON_DIR}/Modules/unicodename_db.h"
  "${CPYTHON_DIR}/Modules/xxlimited.c"
  "${CPYTHON_DIR}/Modules/xxmodule.c"
  "${CPYTHON_DIR}/Modules/xxsubtype.c")

set_property(
  SOURCE ${CPYTHON_GEN_DIR}/Python/dynload_shlib.c
  PROPERTY COMPILE_DEFINITIONS SOABI="3.6.5")

set_property(
  SOURCE ${CPYTHON_GEN_DIR}/Modules/getpath.c
  PROPERTY COMPILE_DEFINITIONS
      PREFIX=""
      EXEC_PREFIX=""
      VERSION="3.6"
      VPATH="."
      PYTHONPATH="")

string(
  REGEX
  REPLACE "(${CPYTHON_DIR}\/)" "${CPYTHON_GEN_DIR}/"
  CPYTHON_OUTPUT_SOURCES "${CPYTHON_SOURCES}")

list(APPEND CPYTHON_SOURCES "ext/Include/Python.h")

add_custom_command(
  OUTPUT ${CPYTHON_OUTPUT_SOURCES}
  COMMAND
    python3 util/generate_cpython_sources.py
    -sources ${CPYTHON_SOURCES}
    -modified ${CPYTHON_MODIFIED_SOURCES}
    -output_dir ${CMAKE_BINARY_DIR}
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  DEPENDS ${CPYTHON_SOURCES} ${CPYTHON_MODIFIED_SOURCES} util/generate_cpython_sources.py
  COMMENT "Generating CPython source files")

string(
  REGEX
  REPLACE "(${CPYTHON_DIR}\/)" "${CPYTHON_GEN_DIR}/"
  CPYTHON_CLINIC_OUTPUT_SOURCES "${CPYTHON_SOURCES_WITH_CLINIC_ANNOTATIONS}")

string(
  REGEX REPLACE "([a-z0-9_]*\\.c)" "clinic/\\1.h"
  CPYTHON_CLINIC_OUTPUT_HEADERS "${CPYTHON_CLINIC_OUTPUT_SOURCES}")

# Regenerate headers from module sources with argument clinic annotations
add_custom_command(
  OUTPUT ${CPYTHON_CLINIC_OUTPUT_HEADERS}
  COMMAND third-party/cpython/Tools/clinic/clinic.py ${CPYTHON_CLINIC_OUTPUT_SOURCES}
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  DEPENDS ${CPYTHON_OUTPUT_SOURCES} third-party/cpython/Tools/clinic/clinic.py
  COMMENT "Regenerate headers from module sources with argument clinic annotations")

add_custom_target(
  cpython-sources
  DEPENDS
    ${CPYTHON_OUTPUT_SOURCES}
    ${CPYTHON_CLINIC_OUTPUT_HEADERS})

find_package(Threads)
add_library(
  cpython
  STATIC
  ${CPYTHON_OUTPUT_SOURCES}
  ${CPYTHON_CLINIC_OUTPUT_HEADERS})
add_dependencies(cpython cpython-sources)
target_include_directories(
  cpython
  PUBLIC
  ${CPYTHON_GEN_DIR}/Include
  ext/config)
target_compile_options(
  cpython
  PUBLIC
  ${OS_DEFINE}
  PRIVATE
  "-Werror"
  "-DPy_BUILD_CORE")
target_link_libraries(
  cpython
  PUBLIC
  util
  extension
  capi-headers
  Threads::Threads)
