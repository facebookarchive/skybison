# Toolchain settings for a regular (non-Sandcastle) Linux build

set(SYSCONFIGDATA ${CMAKE_CURRENT_LIST_DIR}/linux/_sysconfigdata__linux_.py)

set(BZIP2_LIBRARIES bz2)
set(FFI_LIBRARIES ffi)
set(XZ_LIBRARIES lzma)
set(OPENSSL_PREFIX /usr)
set(OPENSSL_LIBRARIES crypto ssl)
set(READLINE_LIBRARIES ncurses readline tinfo)
set(SQLITE_LIBRARIES sqlite3)
set(ZLIB_LIBRARIES z)
