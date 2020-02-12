# Toolchain settings for facebook platform007
# This should be used with the `-C` option. Example:
#     $ cmake -C util/platform007-clang.cmake ..
#
# See also:
#   fbsource/tools/buckconfigs/fbcode/platforms/*/cxx.bcfg

# Determine fbcode checkout directory.
execute_process(COMMAND hg root WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}"
  OUTPUT_VARIABLE FBSOURCE_DIR OUTPUT_STRIP_TRAILING_WHITESPACE
  RESULT_VARIABLE RES)
if (NOT RES EQUAL 0)
  message(FATAL_ERROR "Could not determine fbsource checkout directory")
endif()

set(FBCODE_DIR "${FBSOURCE_DIR}/fbcode")
set(LLVM_BIN "${FBCODE_DIR}/third-party-buck/platform007/build/llvm-fb/bin")

set(CMAKE_C_COMPILER "/usr/local/fbcode/platform007/bin/clang.par" CACHE STRING "")
set(CMAKE_CXX_COMPILER "/usr/local/fbcode/platform007/bin/clang++.par" CACHE STRING "")
set(CMAKE_LINKER "${LLVM_BIN}/lld" CACHE STRING "")
set(CMAKE_AR "${LLVM_BIN}/llvm-ar" CACHE STRING "")
set(CMAKE_NM "${LLVM_BIN}/llvm-nm" CACHE STRING "")
set(CMAKE_RANLIB "/bin/true" CACHE STRING "")
set(CMAKE_OBJCOPY "${LLVM_BIN}/llvm-objcopy" CACHE STRING "")
set(CMAKE_OBJDUMP "${LLVM_BIN}/llvm-objdump" CACHE STRING "")

include(${CMAKE_CURRENT_LIST_DIR}/platform007-common.cmake)

string_join(" " PLATFORM_LINKER_FLAGS
  ${PLATFORM_LINKER_FLAGS}
  -fuse-ld=lld       # Use lld because gnu ld does not support clang LTO
  -B${LLVM_BIN}      # path to linker
)
