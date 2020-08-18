# Toolchain settings for facebook platform007
#
# See also:
#   fbsource/tools/buckconfigs/fbcode/platforms/*/cxx.bcfg

execute_process(COMMAND hg root WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}"
  OUTPUT_VARIABLE FBSOURCE_DIR OUTPUT_STRIP_TRAILING_WHITESPACE
  RESULT_VARIABLE RES)
if (NOT RES EQUAL 0)
  message(FATAL_ERROR "Could not determine fbsource checkout directory")
endif()

set(GCC_PAR /usr/local/fbcode/platform007/bin/gcc.par)
set(GXX_PAR /usr/local/fbcode/platform007/bin/g++.par)
execute_process(
  COMMAND ${GCC_PAR} --platform=platform007 --gen-script=${CMAKE_BINARY_DIR}/gcc
  OUTPUT_QUIET
)
execute_process(
  COMMAND ${GXX_PAR} --platform=platform007 --gen-script=${CMAKE_BINARY_DIR}/g++
  OUTPUT_QUIET
)

set(FBCODE_DIR "${FBSOURCE_DIR}/fbcode")
set(BINUTILS_BIN "${FBCODE_DIR}/third-party-buck/platform007/tools/binutils/bin")

set(CMAKE_C_COMPILER "${CMAKE_BINARY_DIR}/gcc" CACHE STRING "" FORCE)
set(CMAKE_CXX_COMPILER "${CMAKE_BINARY_DIR}/g++" CACHE STRING "" FORCE)
set(CMAKE_AR "${BINUTILS_BIN}/ar" CACHE STRING "" FORCE)
set(CMAKE_NM "${BINUTILS_BIN}/nm" CACHE STRING "" FORCE)
set(CMAKE_RANLIB "${BINUTILS_BIN}/ranlib" CACHE STRING "" FORCE)
set(CMAKE_OBJCOPY "${BINUTILS_BIN}/objcopy" CACHE STRING "" FORCE)
set(CMAKE_OBJDUMP "${BINUTILS_BIN}/objdump" CACHE STRING "" FORCE)

include(${CMAKE_CURRENT_LIST_DIR}/platform007/common.cmake)

# There's some bug with platform007 gcc such that -fsanitize=address can't find
# libdl, librt, or libpthread
if (${PYRO_ASAN})
    set(PLATFORM_COMPILER_FLAGS "${PLATFORM_COMPILER_FLAGS} -ldl -lrt -pthread -fsanitize=address" CACHE STRING "" FORCE)
endif()
