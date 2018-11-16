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
set(CPYTHON_OUTPUT_HEADERS_DIR ${CMAKE_BINARY_DIR}/gen/include)

# Obtain the list of all CPython headers and replace Python.h
file(GLOB CPYTHON_HEADERS ${CPYTHON_DIR}/Include/*.h)
list(REMOVE_ITEM CPYTHON_HEADERS "${CPYTHON_DIR}/Include/Python.h")
list(APPEND CPYTHON_HEADERS "ext/Include/Python.h")

# Replace the DIR to generate the output sources
# i.e. third-parth/cpython/Include/object.h -> build/gen/include/object.h
string(
  REGEX
  REPLACE "([^;]+\/)" "${CPYTHON_OUTPUT_HEADERS_DIR}/"
  CPYTHON_OUTPUT_HEADERS "${CPYTHON_HEADERS}")

add_custom_command(
  OUTPUT ${CPYTHON_OUTPUT_HEADERS}
  COMMAND
    python3 util/generate_cpython_sources.py
    -sources ${CPYTHON_HEADERS}
    -modified ${CPYTHON_MODIFIED_HEADERS}
    -output_dir ${CMAKE_BINARY_DIR}
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  DEPENDS ${CPYTHON_HEADERS} ${CPYTHON_MODIFIED_HEADERS}
  COMMENT "Generating CPython source files")
add_custom_target(cpython-sources DEPENDS ${CPYTHON_OUTPUT_HEADERS})

add_library(cpython INTERFACE)
target_sources(
  cpython
  INTERFACE
  ${CPYTHON_OUTPUT_HEADERS}
)
target_include_directories(
  cpython
  INTERFACE
  ext/config
  ${CPYTHON_OUTPUT_HEADERS_DIR})
