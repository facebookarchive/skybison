# Toolchain settings for facebook platform007
# This should be used with the `-C` option. Example:
#     $ cmake -C util/platform007-gcc.cmake ..
#
# See also:
#   fbsource/tools/buckconfigs/fbcode/platforms/*/cxx.bcfg

set(CMAKE_C_COMPILER "/usr/local/fbcode/platform007/bin/gcc.par" CACHE STRING "")
set(CMAKE_CXX_COMPILER "/usr/local/fbcode/platform007/bin/g++.par" CACHE STRING "")

include(${CMAKE_CURRENT_LIST_DIR}/platform007-common.cmake)

# There's some bug with platform007 gcc such that -fsanitize=address can't find
# libdl, librt, or libpthread
# TODO(emacs): Separate ASAN builds into their own platform files and remove
# this conditional.
if (${PYRO_ASAN})
    set(PLATFORM_COMPILER_FLAGS "${PLATFORM_COMPILER_FLAGS} -ldl -lrt -lpthread" CACHE STRING "" FORCE)
endif()

