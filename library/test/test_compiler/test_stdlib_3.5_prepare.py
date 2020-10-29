#!/usr/bin/env python3
#
# Compile CPython stdlib+tests (Lib/ in distribution) using CPython,
# to serve as comparison reference.
# Make sure you have built CPython3.5 with peephole optimizer disabled
# using build-cpython-compiler.sh script.
#
import glob
import os
import subprocess


CORPUS_PATH_IN = "../.."
CORPUS_PATH_OUT = "stdlib-3.6"

IGNORE_PATTERNS = (
    # Not a valid Python3 syntax
    "lib2to3/tests/data",
    "test/badsyntax_",
    "test/bad_coding",
)


def ensure_path(path):
    dirpath = os.path.dirname(path)
    try:
        os.makedirs(dirpath)
    except OSError:
        pass


for fname in glob.glob(CORPUS_PATH_IN + "/**/*.py", recursive=True):
    assert fname.startswith(CORPUS_PATH_IN + "/")
    ignore = False
    for p in IGNORE_PATTERNS:
        if p in fname:
            ignore = True
            break
    rel_fname = fname[len(CORPUS_PATH_IN) + 1 :]
    if ignore:
        # print("%s - ignored" % rel_fname)
        continue
    print(rel_fname)
    corpus_output = CORPUS_PATH_OUT + "/" + rel_fname
    if os.path.exists(corpus_output + ".exp"):
        continue
    ensure_path(corpus_output)
    subprocess.check_call(
        "../../../python dis_stable.py %s > %s.exp" % (fname, corpus_output), shell=True
    )
