#!/usr/bin/env python3

import os
import re
import shutil
import subprocess
import sys


LLDB_CMDS = os.path.join(os.path.dirname(__file__), "test_lldb_support.lldb")
CPP_FILE = os.path.join(os.path.dirname(__file__), "test_lldb_support.cpp")
EXP_LINE_RE = re.compile(r"^// (exp|re): (.+)$")


def main():
    if not shutil.which("lldb"):
        print("lldb not found; not running tests")
        return 0

    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <build root>", file=sys.stderr)
        return 1

    build_root = sys.argv[1]
    lldb_command = [
        "lldb",
        "-x",
        "-s",
        LLDB_CMDS,
        "--",
        os.path.join(build_root, "test_lldb_support"),
    ]
    proc = subprocess.run(
        lldb_command, encoding="utf-8", stdout=subprocess.PIPE, stdin=subprocess.DEVNULL
    )

    failed = []
    found = 0
    stdout = proc.stdout
    for line in open(CPP_FILE, "r"):
        line = line.strip()
        match = EXP_LINE_RE.match(line)
        if not match:
            continue
        kind = match[1]
        pattern = match[2]
        if kind == "exp":
            pattern = f"^{re.escape(pattern)}$"
        else:
            pattern = f"^{pattern}$"
        if re.search(pattern, stdout, re.MULTILINE):
            found += 1
        else:
            failed.append(pattern)

    if failed:
        print(f"Missing {len(failed)} patterns:", file=sys.stderr)
        for pattern in failed:
            print(f"  {pattern}", file=sys.stderr)
        print(
            f"\nRun the following command to reproduce:\n  > {' '.join(lldb_command)}",
            file=sys.stderr,
        )
        return 1

    print(f"{found} patterns found")
    return 0


if __name__ == "__main__":
    sys.exit(main())
