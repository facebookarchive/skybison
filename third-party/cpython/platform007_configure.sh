#!/bin/bash
#
# Note that Cinder has a similar Tools/scripts/configure_with_fb_toolchain.py,
# should we switch to their script?
set -eu

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

CC_PAR=/usr/local/fbcode/platform007/bin/clang.par
CXX_PAR=/usr/local/fbcode/platform007/bin/clang++.par

${CC_PAR} --gen-script clang.par.sh
${CXX_PAR} --gen-script clang++.par.sh

# The following paths will probably need to be synced occasionally with fbsource/fbcode/third-party2
BZIP2_DIR="/mnt/gvfs/third-party2/bzip2/73a237ac5bc0a5f5d67b39b8d253cfebaab88684/1.0.6/platform007/ca4da3d"
GDBM_DIR="/mnt/gvfs/third-party2/gdbm/e98a73eabee86b9324c14527f9e985ec8f2be9ef/1.11/platform007/ca4da3d"
LIBFFI_DIR="/mnt/gvfs/third-party2/libffi/012cdcb4f49722e133571d9b8bed4015ff873be9/3.2.1/platform007/ca4da3d"
LIBUUID_DIR="/mnt/gvfs/third-party2/libuuid/e2bdc84f4f867e82294c3d288689b7842f9b765c/1.0.2/platform007/ca4da3d"
NCURSES_DIR="/mnt/gvfs/third-party2/ncurses/73f099a14b15630a8cc9f8556bf5801346891790/5.9/platform007/ca4da3d"
OPENSSL_DIR="/mnt/gvfs/third-party2/openssl/08b454ad6354c8144da2168f7a333624e85bb30b/1.1.0/platform007/ca4da3d"
READLINE_DIR="/mnt/gvfs/third-party2/readline/54cbbc50eddf04fc7f1c0177df504754ad39e606/6.3/platform007/3dffd10"
SQLITE_DIR="/mnt/gvfs/third-party2/sqlite/408d5d6ddccd5451f16ba834d0679aa2187d9cc0/3.21.0/platform007/5007832"
XZ_DIR="/mnt/gvfs/third-party2/xz/5bc7ad7b551b4f0054348fdfbd6535413c2519a7/5.2.2/platform007/ca4da3d"
ZLIB_DIR="/mnt/gvfs/third-party2/zlib/3c160ac5c67e257501e24c6c1d00ad5e01d73db6/1.2.8/platform007/ca4da3d"

CPPFLAGS+=" -I ${BZIP2_DIR}/include"
CPPFLAGS+=" -I ${GDBM_DIR}/include/gdbm"
CPPFLAGS+=" -I ${LIBFFI_DIR}/lib/libffi-3.2.1/include"
CPPFLAGS+=" -I ${LIBUUID_DIR}/include"
CPPFLAGS+=" -I ${NCURSES_DIR}/include"
CPPFLAGS+=" -I ${READLINE_DIR}/include"
CPPFLAGS+=" -I ${SQLITE_DIR}/include"
CPPFLAGS+=" -I ${XZ_DIR}/include"
CPPFLAGS+=" -I ${ZLIB_DIR}/include"
LDFLAGS+=" -L ${BZIP2_DIR}/lib"
LDFLAGS+=" -L ${GDBM_DIR}/lib"
LDFLAGS+=" -L ${LIBFFI_DIR}/lib"
LDFLAGS+=" -L ${LIBUUID_DIR}/lib"
LDFLAGS+=" -L ${NCURSES_DIR}/lib"
LDFLAGS+=" -L ${READLINE_DIR}/lib"
LDFLAGS+=" -L ${SQLITE_DIR}/lib"
LDFLAGS+=" -L ${XZ_DIR}/lib"
LDFLAGS+=" -L ${ZLIB_DIR}/lib"

# This avoids readline headers adding (unused) legacy typedefs that trigger
# -Wstrict-prototype warnings.
CFLAGS_NODIST+=" -D_FUNCTION_DEF -DHAVE_STDARG_H"

CFLAGS_NODIST+=" -Werror -Wno-error=unreachable-code"
export CPPFLAGS
export LDFLAGS
export CFLAGS_NODIST
export CC=$(pwd)/clang.par.sh
export CXX=$(pwd)/clang++.par.sh
CONFIGURE_FLAGS=""
CONFIGURE_FLAGS+=" --with-openssl=${OPENSSL_DIR}"
${SCRIPT_DIR}/configure $CONFIGURE_FLAGS "$@"
