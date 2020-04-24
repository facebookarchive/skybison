#!/usr/bin/env python3
#
# The python build does not fail when an optional module failed to compile which
# makes it easy to miss error in CI.  This script checks that all expected
# modules were built.
import glob
import sys
import os

expected_modules = set(
    """
_asyncio
_bisect
_blake2
_bz2
_codecs_cn
_codecs_hk
_codecs_iso2022
_codecs_jp
_codecs_kr
_codecs_tw
_contextvars
_crypt
_csv
_ctypes
_ctypes_test
_curses
_curses_panel
_datetime
_dbm
_decimal
_elementtree
_gdbm
_hashlib
_heapq
_json
_lsprof
_lzma
_md5
_multibytecodec
_multiprocessing
_opcode
_pickle
_posixsubprocess
_queue
_random
_sha1
_sha256
_sha3
_sha512
_socket
_sqlite3
_ssl
_struct
_testbuffer
_testcapi
_testimportmultiple
_testmultiphase
_uuid
_xxtestfuzz
array
audioop
binascii
cmath
fcntl
grp
math
mmap
nis
ossaudiodev
parser
pyexpat
readline
resource
select
spwd
syslog
termios
unicodedata
zlib
""".strip().splitlines()
)
expected_modules.update(sys.argv[1:])


def main():
    with open("pybuilddir.txt") as fp:
        builddir = fp.read().strip()
    available_modules = set()
    os.chdir(builddir)
    for file in glob.glob("*.so"):
        modulename = file.partition(".")[0]
        available_modules.add(modulename)

    missing = expected_modules - available_modules
    unexpected = available_modules - expected_modules

    rc = 0
    if missing:
        sys.stderr.write("Error: Could not find all expected modules; missing:\n")
        for m in sorted(missing):
            sys.stderr.write(f"    {m}\n")
        sys.stderr.write("Please check for build errors in those modules.\n")
        rc = 1
    if unexpected:
        sys.stderr.write("Error: Found unexpected additional modules:\n")
        for m in sorted(unexpected):
            sys.stderr.write(f"    {m}\n")
        sys.stderr.write("If you added a new module update '{__file__}'.\n")
        rc = 1

    sys.exit(rc)


if __name__ == "__main__":
    main()
