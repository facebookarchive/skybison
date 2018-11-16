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
# ``CPYTHON_MODIFIED_SOURCES``
#   A list of source files that contain redifinitions of symbols in CPython.
#

set(
  CPYTHON_SOURCES
  ext/Include/abstract.h
  ext/Include/bltinmodule.h
  ext/Include/boolobject.h
  ext/Include/bytearrayobject.h
  ext/Include/bytesobject.h
  ext/Include/cellobject.h
  ext/Include/ceval.h
  ext/Include/classobject.h
  ext/Include/codecs.h
  ext/Include/compile.h
  ext/Include/complexobject.h
  ext/Include/descrobject.h
  ext/Include/dictobject.h
  ext/Include/dtoa.h
  ext/Include/enumobject.h
  ext/Include/eval.h
  ext/Include/fileobject.h
  ext/Include/fileutils.h
  ext/Include/floatobject.h
  ext/Include/frameobject.h
  ext/Include/funcobject.h
  ext/Include/genobject.h
  ext/Include/import.h
  ext/Include/intrcheck.h
  ext/Include/iterobject.h
  ext/Include/listobject.h
  ext/Include/longintrepr.h
  ext/Include/longobject.h
  ext/Include/memoryobject.h
  ext/Include/methodobject.h
  ext/Include/modsupport.h
  ext/Include/moduleobject.h
  ext/Include/namespaceobject.h
  ext/Include/object.h
  ext/Include/objimpl.h
  ext/Include/odictobject.h
  ext/Include/osmodule.h
  ext/Include/patchlevel.h
  ext/Include/pyarena.h
  ext/Include/pyatomic.h
  ext/Include/pycapsule.h
  ext/Include/pyctype.h
  ext/Include/pydebug.h
  ext/Include/pydtrace.h
  ext/Include/pyerrors.h
  ext/Include/pyfpe.h
  ext/Include/pyhash.h
  ext/Include/pylifecycle.h
  ext/Include/pymacconfig.h
  ext/Include/pymacro.h
  ext/Include/pymath.h
  ext/Include/pymem.h
  ext/Include/pyport.h
  ext/Include/pystate.h
  ext/Include/pystrcmp.h
  ext/Include/pystrtod.h
  ext/Include/Python.h
  ext/Include/pythonrun.h
  ext/Include/pytime.h
  ext/Include/rangeobject.h
  ext/Include/setobject.h
  ext/Include/sliceobject.h
  ext/Include/structmember.h
  ext/Include/structseq.h
  ext/Include/sysmodule.h
  ext/Include/traceback.h
  ext/Include/tupleobject.h
  ext/Include/typeslots.h
  ext/Include/unicodeobject.h
  ext/Include/warnings.h
  ext/Include/weakrefobject.h)

string(
  REGEX REPLACE "([^;]+)"
  "${CMAKE_BINARY_DIR}/gen/\\1"
  CPYTHON_OUTPUT_SOURCES
  "${CPYTHON_SOURCES}")

add_custom_command(
  OUTPUT ${CPYTHON_OUTPUT_SOURCES}
  COMMAND
    python3 util/generate_cpython_sources.py
    -sources ${CPYTHON_SOURCES}
    -modified ${CPYTHON_MODIFIED_SOURCES}
    -output_dir ${CMAKE_BINARY_DIR}/gen
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  DEPENDS ${CPYTHON_SOURCES} ${CPYTHON_MODIFIED_SOURCES}
  COMMENT "Generating CPython source files")

add_library(cpython INTERFACE)
target_sources(
  cpython
  INTERFACE
  ${CPYTHON_OUTPUT_SOURCES}
)
target_include_directories(
  cpython
  INTERFACE
  ${CMAKE_BINARY_DIR}/gen/ext/Include)
