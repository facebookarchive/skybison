# Toolchain settings for facebook platform007
# This should be used with the `-C` option. Example:
#     $ cmake -C util/platform007.cmake ..
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
set(LLVM_BIN "${FBCODE_DIR}/third-party-buck/platform007/build/llvm-fb/bin")

# Default to release build
set(CMAKE_BUILD_TYPE "Release" CACHE STRING "")

set(CMAKE_C_COMPILER "/usr/local/fbcode/platform007/bin/clang.par" CACHE STRING "")
set(CMAKE_CXX_COMPILER "/usr/local/fbcode/platform007/bin/clang++.par" CACHE STRING "")
set(CMAKE_LINKER "${LLVM_BIN}/lld" CACHE STRING "")
set(CMAKE_AR "${LLVM_BIN}/llvm-ar" CACHE STRING "")
set(CMAKE_NM "${LLVM_BIN}/llvm-nm" CACHE STRING "")
set(CMAKE_RANLIB "/bin/true" CACHE STRING "")
set(CMAKE_OBJCOPY "${LLVM_BIN}/llvm-objcopy" CACHE STRING "")
set(CMAKE_OBJDUMP "${LLVM_BIN}/llvm-objdump" CACHE STRING "")

string_join(" " PLATFORM_COMPILER_FLAGS
  --platform=platform007
)
string_join(" " PLATFORM_LINKER_FLAGS
  --platform=platform007
  -Wl,--dynamic-linker,/usr/local/fbcode/platform007/lib/ld.so
  -Wl,-rpath,/usr/local/fbcode/platform007/lib
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
  -mtune=core-avx2
  -mpclmul
)
set(CMAKE_C_FLAGS_RELEASE "${FLAGS_RELEASE}" CACHE STRING "")
set(CMAKE_CXX_FLAGS_RELEASE "${FLAGS_RELEASE}" CACHE STRING "")
