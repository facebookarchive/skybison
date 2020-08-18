# Toolchain settings for facebook platform007
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

set(CLANG_PAR /usr/local/fbcode/platform007/bin/clang.par)
set(CLANGXX_PAR /usr/local/fbcode/platform007/bin/clang++.par)
execute_process(
  COMMAND ${CLANG_PAR} --platform=platform007 --gen-script=${CMAKE_BINARY_DIR}/clang
  OUTPUT_QUIET
)
execute_process(
  COMMAND ${CLANGXX_PAR} --platform=platform007 --gen-script=${CMAKE_BINARY_DIR}/clang++
  OUTPUT_QUIET
)

set(FBCODE_DIR "${FBSOURCE_DIR}/fbcode")
set(LLVM_BIN "${FBCODE_DIR}/third-party-buck/platform007/build/llvm-fb/bin")

set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/clang" CACHE STRING "" FORCE)
set(CMAKE_CXX_COMPILER "${CMAKE_BINARY_DIR}/clang++" CACHE STRING "" FORCE)
set(CMAKE_LINKER "${LLVM_BIN}/lld" CACHE STRING "" FORCE)
set(CMAKE_AR "${LLVM_BIN}/llvm-ar" CACHE STRING "" FORCE)
set(CMAKE_NM "${LLVM_BIN}/llvm-nm" CACHE STRING "" FORCE)
set(CMAKE_RANLIB "/bin/true" CACHE STRING "" FORCE)
set(CMAKE_OBJCOPY "${LLVM_BIN}/llvm-objcopy" CACHE STRING "" FORCE)
set(CMAKE_OBJDUMP "${LLVM_BIN}/llvm-objdump" CACHE STRING "" FORCE)

include(${CMAKE_CURRENT_LIST_DIR}/platform007/common.cmake)

string_join(" " PLATFORM_LINKER_FLAGS
  ${PLATFORM_LINKER_FLAGS}
  -fuse-ld=lld       # Use lld because gnu ld does not support clang LTO
  # work around eden fs performance problem
  # https://fb.workplace.com/groups/eden.users/permalink/3059823790734215/
  -Xlinker --no-mmap-output-file
)
