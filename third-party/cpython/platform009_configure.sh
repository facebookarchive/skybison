#!/bin/bash
#
# Note that Cinder has a similar Tools/scripts/configure_with_fb_toolchain.py,
# should we switch to their script?
set -eu

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

CC_PAR=/usr/local/fbcode/platform009/bin/clang.par
CXX_PAR=/usr/local/fbcode/platform009/bin/clang++.par

${CC_PAR} --platform=platform009 --gen-script clang.par.sh
${CXX_PAR} --platform=platform009 --gen-script clang++.par.sh

# The following paths will probably need to be synced occasionally with fbsource/fbcode/third-party2
BZIP2_DIR="/mnt/gvfs/third-party2/bzip2/73a237ac5bc0a5f5d67b39b8d253cfebaab88684/1.0.6/platform009/7f3b187"
GDBM_DIR="/mnt/gvfs/third-party2/gdbm/adde7e4cfda6dfc0335844845128d4d7c5f5cd08/1.11/platform009/7f3b187"
LIBFFI_DIR="/mnt/gvfs/third-party2/libffi/012cdcb4f49722e133571d9b8bed4015ff873be9/3.2.1/platform009/7f3b187"
LIBUUID_DIR="/mnt/gvfs/third-party2/libuuid/1ebe8a3ca66b979e1f355d5ccc1fcb032986aa47/1.0.2/platform009/7f3b187"
NCURSES_DIR="/mnt/gvfs/third-party2/ncurses/69dde6a41f684aaa734053f5809585a8296aa82d/6.1/platform009/7f3b187"
OPENSSL_DIR="/mnt/gvfs/third-party2/openssl/6b9926ca4983143a8df1268c7566be1390a7afe5/1.1.1/platform009/7f3b187"
READLINE_DIR="/mnt/gvfs/third-party2/readline/fb6f3492a8fd9a591cb7450ad43e5ad619f26ab2/8.0/platform009/48d25e2"
SQLITE_DIR="/mnt/gvfs/third-party2/sqlite/f4cefe73ccc18408b9c851b3ca0901e8f0c0a8d9/3.30/platform009/a6271c4"
XZ_DIR="/mnt/gvfs/third-party2/xz/5bc7ad7b551b4f0054348fdfbd6535413c2519a7/5.2.2/platform009/7f3b187"
ZLIB_DIR="/mnt/gvfs/third-party2/zlib/3c160ac5c67e257501e24c6c1d00ad5e01d73db6/1.2.8/platform009/7f3b187"

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

CFLAGS_NODIST+=" -Werror -Wno-error=unreachable-code"
export CPPFLAGS
export LDFLAGS
export CFLAGS_NODIST
export CC=$(pwd)/clang.par.sh
export CXX=$(pwd)/clang++.par.sh
CONFIGURE_FLAGS=""
CONFIGURE_FLAGS+=" --with-openssl=${OPENSSL_DIR}"
${SCRIPT_DIR}/configure $CONFIGURE_FLAGS "$@"

