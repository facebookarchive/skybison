# Toolchain settings for facebook platform009
# This should be used with the `-C` option. Example:
#     $ cmake -C util/platform009.cmake ..
#
# See also:
#   fbsource/tools/buckconfigs/fbcode/platforms/*/cxx.bcfg

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

# Determine fbcode checkout directory.
execute_process(COMMAND hg root WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}"
  OUTPUT_VARIABLE FBSOURCE_DIR OUTPUT_STRIP_TRAILING_WHITESPACE
  RESULT_VARIABLE RES)
if (NOT RES EQUAL 0)
  message(FATAL_ERROR "Could not determine fbsource checkout directory")
endif()

set(FBCODE_DIR "${FBSOURCE_DIR}/fbcode")
set(LLVM_BIN "${FBCODE_DIR}/third-party-buck/platform009/build/llvm-fb/bin")

# Default to release build
set(CMAKE_BUILD_TYPE "Release" CACHE STRING "")

set(CMAKE_C_COMPILER "/usr/local/fbcode/platform009/bin/clang.par" CACHE STRING "")
set(CMAKE_CXX_COMPILER "/usr/local/fbcode/platform009/bin/clang++.par" CACHE STRING "")
set(CMAKE_LINKER "${LLVM_BIN}/lld" CACHE STRING "")
set(CMAKE_AR "${LLVM_BIN}/llvm-ar" CACHE STRING "")
set(CMAKE_NM "${LLVM_BIN}/llvm-nm" CACHE STRING "")
set(CMAKE_RANLIB "/bin/true" CACHE STRING "")
set(CMAKE_OBJCOPY "${LLVM_BIN}/llvm-objcopy" CACHE STRING "")
set(CMAKE_OBJDUMP "${LLVM_BIN}/llvm-objdump" CACHE STRING "")

string_join(" " PLATFORM_LINKER_FLAGS
  -Wl,--dynamic-linker,/usr/local/fbcode/platform009/lib/ld.so
  -Wl,-rpath,/usr/local/fbcode/platform009/lib
  -fuse-ld=lld       # Use lld because gnu ld does not support clang LTO
  -B${LLVM_BIN}      # path to linker
  -Wl,--emit-relocs  # needed for bolt
)
set(PLATFORM_COMPILER_FLAGS "${PLATFORM_COMPILER_FLAGS}" CACHE STRING "")
set(PLATFORM_LINKER_FLAGS "${PLATFORM_LINKER_FLAGS}" CACHE STRING "")

string_join(" " FLAGS_RELEASE
  -O3
  -DNDEBUG
  -march=corei7
  -mtune=skylake
  -mpclmul
  -fno-omit-frame-pointer
  -momit-leaf-frame-pointer
)
set(CMAKE_C_FLAGS_RELEASE "${FLAGS_RELEASE}" CACHE STRING "")
set(CMAKE_CXX_FLAGS_RELEASE "${FLAGS_RELEASE}" CACHE STRING "")

set(WHOLE_ARCHIVE -Wl,--whole-archive CACHE STRING "")
set(NO_WHOLE_ARCHIVE -Wl,--no-whole-archive CACHE STRING "")
set(BZIP2_INCLUDE_DIRS /mnt/gvfs/third-party2/bzip2/73a237ac5bc0a5f5d67b39b8d253cfebaab88684/1.0.6/platform009/7f3b187/include CACHE STRING "")
set(BZIP2_LIBRARIES /mnt/gvfs/third-party2/bzip2/73a237ac5bc0a5f5d67b39b8d253cfebaab88684/1.0.6/platform009/7f3b187/lib/libbz2.a CACHE STRING "")
set(FFI_INCLUDE_DIRS /mnt/gvfs/third-party2/libffi/012cdcb4f49722e133571d9b8bed4015ff873be9/3.2.1/platform009/7f3b187/lib/libffi-3.2.1/include CACHE STRING "")
set(FFI_LIBRARIES /mnt/gvfs/third-party2/libffi/012cdcb4f49722e133571d9b8bed4015ff873be9/3.2.1/platform009/7f3b187/lib/libffi.a CACHE STRING "")
set(XZ_INCLUDE_DIRS /mnt/gvfs/third-party2/xz/5bc7ad7b551b4f0054348fdfbd6535413c2519a7/5.2.2/platform009/7f3b187/include CACHE STRING "")
set(XZ_LIBRARIES /mnt/gvfs/third-party2/xz/5bc7ad7b551b4f0054348fdfbd6535413c2519a7/5.2.2/platform009/7f3b187/lib/liblzma.a CACHE STRING "")
set(OPENSSL_PREFIX /mnt/gvfs/third-party2/openssl/8a158fe898db0360047d8be7a5e21ff068243393/1.1.1/platform009/7f3b187 CACHE STRING "")
set(OPENSSL_INCLUDE_DIRS ${OPENSSL_PREFIX}/include CACHE STRING "")
set(OPENSSL_LIBRARIES
    ${OPENSSL_PREFIX}/lib/libcrypto.so
    ${OPENSSL_PREFIX}/lib/libssl.so CACHE STRING "")
