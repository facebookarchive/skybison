# Toolchain settings for a regular (non-Sandcastle) Linux build
# This should be used with the `-C` option. Example:
#     $ cmake -C util/linux.cmake ..

set(WHOLE_ARCHIVE -Wl,--whole-archive CACHE STRING "")
set(NO_WHOLE_ARCHIVE -Wl,--no-whole-archive CACHE STRING "")
set(BZIP2_LIBRARIES bz2 CACHE STRING "")
set(XZ_LIBRARIES lzma CACHE STRING "")
set(OPENSSL_PREFIX /usr CACHE STRING "")
set(OPENSSL_LIBRARIES crypto ssl CACHE STRING "")
