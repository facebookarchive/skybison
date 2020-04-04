# Toolchain settings for a regular (non-Sandcastle) Linux build
if(NOT CMAKE_C_COMPILER)
  set(CMAKE_C_COMPILER /usr/bin/cc)
endif()
if(NOT CMAKE_CXX_COMPILER)
  set(CMAKE_CXX_COMPILER /usr/bin/c++)
endif()

set(BZIP2_LIBRARIES bz2)
set(FFI_LIBRARIES ffi)
set(XZ_LIBRARIES lzma)
set(OPENSSL_PREFIX /usr)
set(OPENSSL_LIBRARIES crypto ssl)
set(ZLIB_LIBRARIES z)
