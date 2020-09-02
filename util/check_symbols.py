#!/usr/bin/env python3.7
"""
This script checks pyro/runtime/symbols.h for unused symbols.
"""
import glob
import os
import re
import sys


symbols_file = "runtime/symbols.h"
cleaned_file = "/tmp/symbols.h.cleaned"
if not os.path.exists(symbols_file):
    sys.stderr.write(f"{symbols_file} not found\n")
    sys.stderr.write("You should run this script in the PyRo directory\n")
    sys.exit(1)

used = set()
use_re = re.compile(r"\bID\((?P<sym>[^\)]+)\)")
for path in glob.iglob("**", recursive=True):
    if path.endswith(".h") or path.endswith(".cpp"):
        with open(path) as fp:
            for line in fp:
                for m in use_re.finditer(line):
                    used.add(m.group("sym"))

sym_re = re.compile(r"\s*V\((?P<sym>[^\)]+)\)")

cpp_to_py = {}
had_unused = False
with open(symbols_file) as fp:
    with open(cleaned_file, "w") as fpo:
        for line in fp:
            m = sym_re.match(line)
            if not m:
                fpo.write(line)
                continue
            sym = m.group("sym")
            if sym in used:
                fpo.write(line)
            else:
                print(f"'{sym}' unused")
                had_unused = True

if had_unused:
    print("")
    print(f"Note: Created {cleaned_file} with unused symbols removed.")
