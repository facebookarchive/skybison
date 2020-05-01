# Common toolchain settings for facebook platform007
# This should be used by including this file in platform007-gcc/clang

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

string_join(" " PLATFORM_COMPILER_FLAGS
  --platform=platform007
)
string_join(" " PLATFORM_LINKER_FLAGS
  --platform=platform007
  -Wl,--dynamic-linker,/usr/local/fbcode/platform007/lib/ld.so
  -Wl,-rpath,/usr/local/fbcode/platform007/lib
  -Wl,--emit-relocs  # needed for bolt
)
set(PLATFORM_COMPILER_FLAGS "${PLATFORM_COMPILER_FLAGS}")
set(PLATFORM_LINKER_FLAGS "${PLATFORM_LINKER_FLAGS}")

string_join(" " FLAGS_RELEASE
  -O3
  -DNDEBUG
  -march=corei7
  -mtune=core-avx2
  -mpclmul
)
set(CMAKE_C_FLAGS_RELEASE "${FLAGS_RELEASE}")
set(CMAKE_CXX_FLAGS_RELEASE "${FLAGS_RELEASE}")

set(BZIP2_PREFIX /mnt/gvfs/third-party2/bzip2/73a237ac5bc0a5f5d67b39b8d253cfebaab88684/1.0.6/platform007/ca4da3d)
set(BZIP2_INCLUDE_DIRS ${BZIP2_PREFIX}/include)
set(BZIP2_LIBRARY_DIRS ${BZIP2_PREFIX}/lib)
set(BZIP2_LIBRARIES ${BZIP2_PREFIX}/lib/libbz2.a)
set(FFI_PREFIX /mnt/gvfs/third-party2/libffi/012cdcb4f49722e133571d9b8bed4015ff873be9/3.2.1/platform007/ca4da3d)
set(FFI_INCLUDE_DIRS ${FFI_PREFIX}/lib/libffi-3.2.1/include)
set(FFI_LIBRARY_DIRS ${FFI_PREFIX}/lib)
set(FFI_LIBRARIES ${FFI_PREFIX}/lib/libffi.a)
set(XZ_PREFIX /mnt/gvfs/third-party2/xz/5bc7ad7b551b4f0054348fdfbd6535413c2519a7/5.2.2/platform007/ca4da3d)
set(XZ_INCLUDE_DIRS ${XZ_PREFIX}/include)
set(XZ_LIBRARY_DIRS ${XZ_PREFIX}/lib)
set(XZ_LIBRARIES ${XZ_PREFIX}/lib/liblzma.a)
set(NCURSES_PREFIX /mnt/gvfs/third-party2/ncurses/69dde6a41f684aaa734053f5809585a8296aa82d/5.9/platform007/ca4da3d)
set(OPENSSL_PREFIX /mnt/gvfs/third-party2/openssl/08b454ad6354c8144da2168f7a333624e85bb30b/1.1.0/platform007/ca4da3d)
set(OPENSSL_INCLUDE_DIRS ${OPENSSL_PREFIX}/include)
set(OPENSSL_LIBRARY_DIRS ${OPENSSL_PREFIX}/lib)
set(OPENSSL_LIBRARIES
    ${OPENSSL_PREFIX}/lib/libcrypto.so
    ${OPENSSL_PREFIX}/lib/libssl.so)
set(READLINE_PREFIX /mnt/gvfs/third-party2/readline/fb6f3492a8fd9a591cb7450ad43e5ad619f26ab2/6.3/platform007/3dffd10)
set(READLINE_INCLUDE_DIRS ${NCURSES_PREFIX}/include ${READLINE_PREFIX}/include)
set(READLINE_LIBRARY_DIRS ${NCURSES_PREFIX}/lib ${READLINE_PREFIX}/lib)
set(READLINE_LIBRARIES
    ${NCURSES_PREFIX}/lib/libncurses.so
    ${NCURSES_PREFIX}/lib/libtinfo.so
    ${READLINE_PREFIX}/lib/libreadline.so)
set(ZLIB_PREFIX /mnt/gvfs/third-party2/zlib/3c160ac5c67e257501e24c6c1d00ad5e01d73db6/1.2.8/platform007/ca4da3d)
set(ZLIB_INCLUDE_DIRS ${ZLIB_PREFIX}/include)
set(ZLIB_LIBRARY_DIRS ${ZLIB_PREFIX}/lib)
set(ZLIB_LIBRARIES ${ZLIB_PREFIX}/lib/libz.so)
