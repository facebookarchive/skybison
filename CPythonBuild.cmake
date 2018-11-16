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
	${CPYTHON_DIR}/Modules/_blake2/*
	${CPYTHON_DIR}/Modules/_io/*
	${CPYTHON_DIR}/Modules/cjkcodecs/*
	${CPYTHON_DIR}/Modules/clinic/*
  ${CPYTHON_DIR}/Include/*
  ${CPYTHON_DIR}/Objects/*
  ${CPYTHON_DIR}/Parser/*
  ${CPYTHON_DIR}/Python/*)

macro(remove_cpython_source value)
  list(REMOVE_ITEM CPYTHON_SOURCES "${CPYTHON_DIR}/${value}")
endmacro()

# Remove the main function from the Parser
remove_cpython_source("Parser/pgenmain.c")

# Remove these sources to avoid function dupes
remove_cpython_source("Python/sigcheck.c")
remove_cpython_source("Python/strdup.c")

# Remove the dynamic loaders for unused architectures
remove_cpython_source("Python/dynload_aix.c")
remove_cpython_source("Python/dynload_dl.c")
remove_cpython_source("Python/dynload_hpux.c")
remove_cpython_source("Python/dynload_next.c")
remove_cpython_source("Python/dynload_stub.c")
remove_cpython_source("Python/dynload_win.c")

# Add all the core Modules sources
set(
	CPYTHON_SOURCES
	${CPYTHON_SOURCES}
  "${CPYTHON_DIR}/Modules/_asynciomodule.c"
  "${CPYTHON_DIR}/Modules/_bisectmodule.c"
  "${CPYTHON_DIR}/Modules/_codecsmodule.c"
  "${CPYTHON_DIR}/Modules/_collectionsmodule.c"
  "${CPYTHON_DIR}/Modules/_cryptmodule.c"
  "${CPYTHON_DIR}/Modules/_csv.c"
  "${CPYTHON_DIR}/Modules/_datetimemodule.c"
  "${CPYTHON_DIR}/Modules/_functoolsmodule.c"
  "${CPYTHON_DIR}/Modules/_heapqmodule.c"
  "${CPYTHON_DIR}/Modules/_json.c"
  "${CPYTHON_DIR}/Modules/_localemodule.c"
  "${CPYTHON_DIR}/Modules/_lsprof.c"
  "${CPYTHON_DIR}/Modules/_math.c"
  "${CPYTHON_DIR}/Modules/_math.h"
  "${CPYTHON_DIR}/Modules/_opcode.c"
  "${CPYTHON_DIR}/Modules/_operator.c"
  "${CPYTHON_DIR}/Modules/_posixsubprocess.c"
  "${CPYTHON_DIR}/Modules/_randommodule.c"
  "${CPYTHON_DIR}/Modules/_sre.c"
  "${CPYTHON_DIR}/Modules/_stat.c"
  "${CPYTHON_DIR}/Modules/_struct.c"
  "${CPYTHON_DIR}/Modules/_testbuffer.c"
  "${CPYTHON_DIR}/Modules/_testimportmultiple.c"
  "${CPYTHON_DIR}/Modules/_testmultiphase.c"
  "${CPYTHON_DIR}/Modules/_threadmodule.c"
  "${CPYTHON_DIR}/Modules/_tracemalloc.c"
  "${CPYTHON_DIR}/Modules/_weakref.c"
  "${CPYTHON_DIR}/Modules/addrinfo.h"
  "${CPYTHON_DIR}/Modules/arraymodule.c"
  "${CPYTHON_DIR}/Modules/atexitmodule.c"
  "${CPYTHON_DIR}/Modules/audioop.c"
  "${CPYTHON_DIR}/Modules/binascii.c"
  "${CPYTHON_DIR}/Modules/cmathmodule.c"
  "${CPYTHON_DIR}/Modules/errnomodule.c"
  "${CPYTHON_DIR}/Modules/faulthandler.c"
  "${CPYTHON_DIR}/Modules/fcntlmodule.c"
  "${CPYTHON_DIR}/Modules/fpectlmodule.c"
  "${CPYTHON_DIR}/Modules/fpetestmodule.c"
  "${CPYTHON_DIR}/Modules/gcmodule.c"
  "${CPYTHON_DIR}/Modules/getbuildinfo.c"
  "${CPYTHON_DIR}/Modules/getpath.c"
  "${CPYTHON_DIR}/Modules/grpmodule.c"
  "${CPYTHON_DIR}/Modules/hashlib.h"
  "${CPYTHON_DIR}/Modules/hashtable.c"
  "${CPYTHON_DIR}/Modules/hashtable.h"
  "${CPYTHON_DIR}/Modules/itertoolsmodule.c"
  "${CPYTHON_DIR}/Modules/mathmodule.c"
  "${CPYTHON_DIR}/Modules/md5module.c"
  "${CPYTHON_DIR}/Modules/mmapmodule.c"
  "${CPYTHON_DIR}/Modules/parsermodule.c"
  "${CPYTHON_DIR}/Modules/posixmodule.c"
  "${CPYTHON_DIR}/Modules/posixmodule.h"
  "${CPYTHON_DIR}/Modules/pwdmodule.c"
  "${CPYTHON_DIR}/Modules/resource.c"
  "${CPYTHON_DIR}/Modules/rotatingtree.c"
  "${CPYTHON_DIR}/Modules/rotatingtree.h"
  "${CPYTHON_DIR}/Modules/selectmodule.c"
  "${CPYTHON_DIR}/Modules/sha1module.c"
  "${CPYTHON_DIR}/Modules/sha256module.c"
  "${CPYTHON_DIR}/Modules/sha512module.c"
  "${CPYTHON_DIR}/Modules/signalmodule.c"
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
  "${CPYTHON_DIR}/Modules/unicodedata.c"
  "${CPYTHON_DIR}/Modules/unicodedata_db.h"
  "${CPYTHON_DIR}/Modules/unicodename_db.h"
  "${CPYTHON_DIR}/Modules/xxlimited.c"
  "${CPYTHON_DIR}/Modules/xxmodule.c"
  "${CPYTHON_DIR}/Modules/xxsubtype.c"
  "${CPYTHON_DIR}/Modules/zipimport.c")

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
add_custom_target(cpython-sources DEPENDS ${CPYTHON_OUTPUT_SOURCES})

find_package(Threads)
add_library(
  cpython
  STATIC
  ${CPYTHON_OUTPUT_SOURCES})
target_include_directories(
  cpython
  PUBLIC
  ${CPYTHON_GEN_DIR}/Include
  ext/config)
target_compile_options(
  cpython
  PUBLIC
  ${OS_DEFINE}
  "-DNDEBUG"
  "-DPy_BUILD_CORE")
target_link_libraries(
  cpython
  PUBLIC
  util
  extension
  capi-headers
 	Threads::Threads)
