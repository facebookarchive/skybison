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

# Obtain the list of all CPython headers and replace Python.h
file(
  GLOB_RECURSE
  CPYTHON_SOURCES
  ${CPYTHON_DIR}/Include/*
  ${CPYTHON_DIR}/Objects/*
  ${CPYTHON_DIR}/Python/*
  ${CPYTHON_DIR}/Parser/*)

# The following source files are not required
list(REMOVE_ITEM CPYTHON_SOURCES "${CPYTHON_DIR}/Python/strdup.c")
list(REMOVE_ITEM CPYTHON_SOURCES "${CPYTHON_DIR}/Python/dynload_aix.c")
list(REMOVE_ITEM CPYTHON_SOURCES "${CPYTHON_DIR}/Python/dynload_dl.c")
list(REMOVE_ITEM CPYTHON_SOURCES "${CPYTHON_DIR}/Python/dynload_hpux.c")
list(REMOVE_ITEM CPYTHON_SOURCES "${CPYTHON_DIR}/Python/dynload_next.c")
list(REMOVE_ITEM CPYTHON_SOURCES "${CPYTHON_DIR}/Python/dynload_stub.c")
list(REMOVE_ITEM CPYTHON_SOURCES "${CPYTHON_DIR}/Python/dynload_win.c")
list(REMOVE_ITEM CPYTHON_SOURCES "${CPYTHON_DIR}/Parser/pgenmain.c")
list(REMOVE_ITEM CPYTHON_SOURCES "${CPYTHON_DIR}/Python/sigcheck.c")

# Handpick the sources from Modules to get Objects and Python Compiled
list(APPEND CPYTHON_SOURCES "${CPYTHON_DIR}/Modules/hashtable.h")
list(APPEND CPYTHON_SOURCES "${CPYTHON_DIR}/Modules/hashtable.c")
list(APPEND CPYTHON_SOURCES "${CPYTHON_DIR}/Modules/gcmodule.c")
list(APPEND CPYTHON_SOURCES "${CPYTHON_DIR}/Modules/signalmodule.c")
list(APPEND CPYTHON_SOURCES "${CPYTHON_DIR}/Modules/getpath.c")
list(APPEND CPYTHON_SOURCES "${CPYTHON_DIR}/Modules/posixmodule.h")
list(APPEND CPYTHON_SOURCES "${CPYTHON_DIR}/Modules/posixmodule.c")
list(APPEND CPYTHON_SOURCES "${CPYTHON_DIR}/Modules/_tracemalloc.c")
list(APPEND CPYTHON_SOURCES "${CPYTHON_DIR}/Modules/faulthandler.c")
list(APPEND CPYTHON_SOURCES "${CPYTHON_DIR}/Modules/clinic/signalmodule.c.h")
list(APPEND CPYTHON_SOURCES "${CPYTHON_DIR}/Modules/clinic/posixmodule.c.h")
list(APPEND CPYTHON_SOURCES "${CPYTHON_DIR}/Modules/getbuildinfo.c")
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
  "-DPy_BUILD_CORE"
)
target_link_libraries(
  cpython
  PUBLIC
  util
  extension
  capi-headers
  Threads::Threads)
