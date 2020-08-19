# Common toolchain settings for facebook platform007
# This should be used by including this file in platform007-gcc/clang

set(SYSCONFIGDATA ${CMAKE_CURRENT_LIST_DIR}/_sysconfigdata__linux_.py)

execute_process(COMMAND hg root WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}"
  OUTPUT_VARIABLE FBSOURCE_DIR OUTPUT_STRIP_TRAILING_WHITESPACE
  RESULT_VARIABLE RES)
if (NOT RES EQUAL 0)
  message(FATAL_ERROR "Could not determine fbsource checkout directory")
endif()
set(THIRD_PARTY2_DIR ${FBSOURCE_DIR}/fbcode/third-party2)

# This is the same as `string(JOIN)` which is only available in cmake >= 3.12
function(string_join GLUE OUT_VAR)
  set(result "")
  set(glue "")
  foreach(e ${ARGN})
    set(result "${result}${glue}${e}")
    set(glue "${GLUE}")
  endforeach()
  set(${OUT_VAR} "${result}" PARENT_SCOPE)
endfunction()

string_join(" " PLATFORM_LINKER_FLAGS
  -Wl,--dynamic-linker,/usr/local/fbcode/platform007/lib/ld.so
  -Wl,-rpath,/usr/local/fbcode/platform007/lib
  -Wl,--emit-relocs  # needed for bolt
)
set(PLATFORM_COMPILER_FLAGS)
set(PLATFORM_LINKER_FLAGS "${PLATFORM_LINKER_FLAGS}")

string_join(" " FLAGS_RELEASE
  -O3
  -DNDEBUG
  -march=corei7
  -mtune=core-avx2
  -mpclmul
)
set(CMAKE_C_FLAGS_RELEASE "${FLAGS_RELEASE}" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "${FLAGS_RELEASE}" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${FLAGS_RELEASE} -g3" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${FLAGS_RELEASE} -g3" CACHE STRING "" FORCE)

# Used by third-party/benchmark; stop it from autodetecting the wrong path.
set(LIBRT -lrt CACHE STRING "" FORCE)

set(BZIP2_PREFIX ${THIRD_PARTY2_DIR}/bzip2/1.0.6/platform007/ca4da3d)
set(BZIP2_INCLUDE_DIRS ${BZIP2_PREFIX}/include)
set(BZIP2_LIBRARY_DIRS ${BZIP2_PREFIX}/lib)
set(BZIP2_LIBRARIES ${BZIP2_PREFIX}/lib/libbz2.a)
set(FFI_PREFIX ${THIRD_PARTY2_DIR}/libffi/3.2.1/platform007/ca4da3d)
set(FFI_INCLUDE_DIRS ${FFI_PREFIX}/lib/libffi-3.2.1/include)
set(FFI_LIBRARY_DIRS ${FFI_PREFIX}/lib)
set(FFI_LIBRARIES ${FFI_PREFIX}/lib/libffi.a)
set(XZ_PREFIX ${THIRD_PARTY2_DIR}/xz/5.2.2/platform007/ca4da3d)
set(XZ_INCLUDE_DIRS ${XZ_PREFIX}/include)
set(XZ_LIBRARY_DIRS ${XZ_PREFIX}/lib)
set(XZ_LIBRARIES ${XZ_PREFIX}/lib/liblzma.a)
set(NCURSES_PREFIX ${THIRD_PARTY2_DIR}/ncurses/5.9/platform007/ca4da3d)
set(OPENSSL_PREFIX ${THIRD_PARTY2_DIR}/openssl/1.1.0/platform007/ca4da3d)
set(OPENSSL_INCLUDE_DIRS ${OPENSSL_PREFIX}/include)
set(OPENSSL_LIBRARY_DIRS ${OPENSSL_PREFIX}/lib)
set(OPENSSL_LIBRARIES
    ${OPENSSL_PREFIX}/lib/libcrypto.so
    ${OPENSSL_PREFIX}/lib/libssl.so)
set(READLINE_PREFIX ${THIRD_PARTY2_DIR}/readline/6.3/platform007/3dffd10)
set(READLINE_INCLUDE_DIRS ${NCURSES_PREFIX}/include ${READLINE_PREFIX}/include)
set(READLINE_LIBRARY_DIRS ${NCURSES_PREFIX}/lib ${READLINE_PREFIX}/lib)
set(READLINE_LIBRARIES
    ${NCURSES_PREFIX}/lib/libncurses.so
    ${NCURSES_PREFIX}/lib/libtinfo.so
    ${READLINE_PREFIX}/lib/libreadline.so)
set(SQLITE_PREFIX ${THIRD_PARTY2_DIR}/sqlite/3.21.0/platform007/5007832)
set(SQLITE_INCLUDE_DIRS ${SQLITE_PREFIX}/include)
set(SQLITE_LIBRARY_DIRS ${SQLITE_PREFIX}/lib)
set(SQLITE_LIBRARIES ${SQLITE_PREFIX}/lib/libsqlite3.so)
set(ZLIB_PREFIX ${THIRD_PARTY2_DIR}/zlib/1.2.8/platform007/ca4da3d)
set(ZLIB_INCLUDE_DIRS ${ZLIB_PREFIX}/include)
set(ZLIB_LIBRARY_DIRS ${ZLIB_PREFIX}/lib)
set(ZLIB_LIBRARIES ${ZLIB_PREFIX}/lib/libz.so)
