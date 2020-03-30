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

set(BZIP2_INCLUDE_DIRS /mnt/gvfs/third-party2/bzip2/73a237ac5bc0a5f5d67b39b8d253cfebaab88684/1.0.6/platform007/ca4da3d/include)
set(BZIP2_LIBRARIES /mnt/gvfs/third-party2/bzip2/73a237ac5bc0a5f5d67b39b8d253cfebaab88684/1.0.6/platform007/ca4da3d/lib/libbz2.a)
set(FFI_INCLUDE_DIRS /mnt/gvfs/third-party2/libffi/012cdcb4f49722e133571d9b8bed4015ff873be9/3.2.1/platform007/ca4da3d/lib/libffi-3.2.1/include)
set(FFI_LIBRARIES /mnt/gvfs/third-party2/libffi/012cdcb4f49722e133571d9b8bed4015ff873be9/3.2.1/platform007/ca4da3d/lib/libffi.a)
set(XZ_INCLUDE_DIRS /mnt/gvfs/third-party2/xz/5bc7ad7b551b4f0054348fdfbd6535413c2519a7/5.2.2/platform007/ca4da3d/include)
set(XZ_LIBRARIES /mnt/gvfs/third-party2/xz/5bc7ad7b551b4f0054348fdfbd6535413c2519a7/5.2.2/platform007/ca4da3d/lib/liblzma.a)
set(OPENSSL_PREFIX /mnt/gvfs/third-party2/openssl/08b454ad6354c8144da2168f7a333624e85bb30b/1.1.0/platform007/ca4da3d)
set(OPENSSL_INCLUDE_DIRS ${OPENSSL_PREFIX}/include)
set(OPENSSL_LIBRARIES
    ${OPENSSL_PREFIX}/lib/libcrypto.so
    ${OPENSSL_PREFIX}/lib/libssl.so)
