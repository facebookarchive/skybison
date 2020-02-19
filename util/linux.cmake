# Toolchain settings for a regular (non-Sandcastle) Linux build
# This should be used with the `-C` option. Example:
#     $ cmake -C util/linux.cmake ..

set(BZIP2_LIBRARIES bz2 CACHE STRING "")
set(XZ_LIBRARIES lzma CACHE STRING "")
set(OPENSSL_PREFIX /usr CACHE STRING "")
set(OPENSSL_LIBRARIES crypto ssl CACHE STRING "")
