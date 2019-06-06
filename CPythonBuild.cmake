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
  ${CPYTHON_SOURCES_WITH_CLINIC_ANNOTATIONS}
  "${CPYTHON_DIR}/Include/Python-ast.h"
  "${CPYTHON_DIR}/Include/Python.h"
  "${CPYTHON_DIR}/Include/abstract.h"
  "${CPYTHON_DIR}/Include/accu.h"
  "${CPYTHON_DIR}/Include/asdl.h"
  "${CPYTHON_DIR}/Include/ast.h"
  "${CPYTHON_DIR}/Include/bitset.h"
  "${CPYTHON_DIR}/Include/bltinmodule.h"
  "${CPYTHON_DIR}/Include/boolobject.h"
  "${CPYTHON_DIR}/Include/bytearrayobject.h"
  "${CPYTHON_DIR}/Include/bytes_methods.h"
  "${CPYTHON_DIR}/Include/bytesobject.h"
  "${CPYTHON_DIR}/Include/cellobject.h"
  "${CPYTHON_DIR}/Include/ceval.h"
  "${CPYTHON_DIR}/Include/classobject.h"
  "${CPYTHON_DIR}/Include/code.h"
  "${CPYTHON_DIR}/Include/codecs.h"
  "${CPYTHON_DIR}/Include/compile.h"
  "${CPYTHON_DIR}/Include/complexobject.h"
  "${CPYTHON_DIR}/Include/datetime.h"
  "${CPYTHON_DIR}/Include/descrobject.h"
  "${CPYTHON_DIR}/Include/dictobject.h"
  "${CPYTHON_DIR}/Include/dtoa.h"
  "${CPYTHON_DIR}/Include/dynamic_annotations.h"
  "${CPYTHON_DIR}/Include/enumobject.h"
  "${CPYTHON_DIR}/Include/errcode.h"
  "${CPYTHON_DIR}/Include/eval.h"
  "${CPYTHON_DIR}/Include/fileobject.h"
  "${CPYTHON_DIR}/Include/fileutils.h"
  "${CPYTHON_DIR}/Include/floatobject.h"
  "${CPYTHON_DIR}/Include/frameobject.h"
  "${CPYTHON_DIR}/Include/funcobject.h"
  "${CPYTHON_DIR}/Include/genobject.h"
  "${CPYTHON_DIR}/Include/graminit.h"
  "${CPYTHON_DIR}/Include/grammar.h"
  "${CPYTHON_DIR}/Include/import.h"
  "${CPYTHON_DIR}/Include/intrcheck.h"
  "${CPYTHON_DIR}/Include/iterobject.h"
  "${CPYTHON_DIR}/Include/listobject.h"
  "${CPYTHON_DIR}/Include/longintrepr.h"
  "${CPYTHON_DIR}/Include/longobject.h"
  "${CPYTHON_DIR}/Include/marshal.h"
  "${CPYTHON_DIR}/Include/memoryobject.h"
  "${CPYTHON_DIR}/Include/metagrammar.h"
  "${CPYTHON_DIR}/Include/methodobject.h"
  "${CPYTHON_DIR}/Include/modsupport.h"
  "${CPYTHON_DIR}/Include/moduleobject.h"
  "${CPYTHON_DIR}/Include/namespaceobject.h"
  "${CPYTHON_DIR}/Include/node.h"
  "${CPYTHON_DIR}/Include/object.h"
  "${CPYTHON_DIR}/Include/objimpl.h"
  "${CPYTHON_DIR}/Include/odictobject.h"
  "${CPYTHON_DIR}/Include/opcode.h"
  "${CPYTHON_DIR}/Include/osdefs.h"
  "${CPYTHON_DIR}/Include/osmodule.h"
  "${CPYTHON_DIR}/Include/parsetok.h"
  "${CPYTHON_DIR}/Include/patchlevel.h"
  "${CPYTHON_DIR}/Include/pgen.h"
  "${CPYTHON_DIR}/Include/pgenheaders.h"
  "${CPYTHON_DIR}/Include/py_curses.h"
  "${CPYTHON_DIR}/Include/pyarena.h"
  "${CPYTHON_DIR}/Include/pyatomic.h"
  "${CPYTHON_DIR}/Include/pycapsule.h"
  "${CPYTHON_DIR}/Include/pyctype.h"
  "${CPYTHON_DIR}/Include/pydebug.h"
  "${CPYTHON_DIR}/Include/pydtrace.h"
  "${CPYTHON_DIR}/Include/pyerrors.h"
  "${CPYTHON_DIR}/Include/pyexpat.h"
  "${CPYTHON_DIR}/Include/pyfpe.h"
  "${CPYTHON_DIR}/Include/pygetopt.h"
  "${CPYTHON_DIR}/Include/pyhash.h"
  "${CPYTHON_DIR}/Include/pylifecycle.h"
  "${CPYTHON_DIR}/Include/pymacconfig.h"
  "${CPYTHON_DIR}/Include/pymacro.h"
  "${CPYTHON_DIR}/Include/pymath.h"
  "${CPYTHON_DIR}/Include/pymem.h"
  "${CPYTHON_DIR}/Include/pyport.h"
  "${CPYTHON_DIR}/Include/pystate.h"
  "${CPYTHON_DIR}/Include/pystrcmp.h"
  "${CPYTHON_DIR}/Include/pystrhex.h"
  "${CPYTHON_DIR}/Include/pystrtod.h"
  "${CPYTHON_DIR}/Include/pythonrun.h"
  "${CPYTHON_DIR}/Include/pythread.h"
  "${CPYTHON_DIR}/Include/pytime.h"
  "${CPYTHON_DIR}/Include/rangeobject.h"
  "${CPYTHON_DIR}/Include/setobject.h"
  "${CPYTHON_DIR}/Include/sliceobject.h"
  "${CPYTHON_DIR}/Include/structmember.h"
  "${CPYTHON_DIR}/Include/structseq.h"
  "${CPYTHON_DIR}/Include/symtable.h"
  "${CPYTHON_DIR}/Include/sysmodule.h"
  "${CPYTHON_DIR}/Include/token.h"
  "${CPYTHON_DIR}/Include/traceback.h"
  "${CPYTHON_DIR}/Include/tupleobject.h"
  "${CPYTHON_DIR}/Include/typeslots.h"
  "${CPYTHON_DIR}/Include/ucnhash.h"
  "${CPYTHON_DIR}/Include/unicodeobject.h"
  "${CPYTHON_DIR}/Include/warnings.h"
  "${CPYTHON_DIR}/Include/weakrefobject.h"
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
  "${CPYTHON_DIR}/Modules/xxsubtype.c"
  "${CPYTHON_DIR}/Objects/accu.c"
  "${CPYTHON_DIR}/Parser/acceler.c"
  "${CPYTHON_DIR}/Parser/bitset.c"
  "${CPYTHON_DIR}/Parser/firstsets.c"
  "${CPYTHON_DIR}/Parser/grammar.c"
  "${CPYTHON_DIR}/Parser/grammar1.c"
  "${CPYTHON_DIR}/Parser/listnode.c"
  "${CPYTHON_DIR}/Parser/metagrammar.c"
  "${CPYTHON_DIR}/Parser/myreadline.c"
  "${CPYTHON_DIR}/Parser/node.c"
  "${CPYTHON_DIR}/Parser/parser.c"
  "${CPYTHON_DIR}/Parser/parser.h"
  "${CPYTHON_DIR}/Parser/parsetok.c"
  "${CPYTHON_DIR}/Parser/parsetok_pgen.c"
  "${CPYTHON_DIR}/Parser/pgen.c"
  "${CPYTHON_DIR}/Parser/printgrammar.c"
  "${CPYTHON_DIR}/Parser/tokenizer.c"
  "${CPYTHON_DIR}/Parser/tokenizer.h"
  "${CPYTHON_DIR}/Parser/tokenizer_pgen.c"
  "${CPYTHON_DIR}/Python/Python-ast.c"
  "${CPYTHON_DIR}/Python/asdl.c"
  "${CPYTHON_DIR}/Python/ast.c"
  "${CPYTHON_DIR}/Python/dtoa.c"
  "${CPYTHON_DIR}/Python/fileutils.c"
  "${CPYTHON_DIR}/Python/graminit.c"
  "${CPYTHON_DIR}/Python/mystrtoul.c"
  "${CPYTHON_DIR}/Python/pyarena.c"
  "${CPYTHON_DIR}/Python/pyctype.c"
  "${CPYTHON_DIR}/Python/pystrcmp.c"
  "${CPYTHON_DIR}/Python/pystrhex.c"
  "${CPYTHON_DIR}/Python/pystrtod.c"
  "${CPYTHON_DIR}/Python/pytime.c"
)

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
