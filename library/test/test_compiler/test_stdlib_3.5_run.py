#!/usr/bin/env python3
#
# Compare "compiler" package output against reference data produced
# by test_stdlib_3.5_prepare.py.
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
    # Some really weird __class__ usage, gave up to add adhoc exceptions for
    # now, should revisit after rewriting symbols.py
    "test_super.py",
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
    if ignore:
        continue
    rel_fname = fname[len(CORPUS_PATH_IN) + 1 :]
    corpus_output = CORPUS_PATH_OUT + "/" + rel_fname

    if os.path.exists("%s.done" % corpus_output):
        print(rel_fname, "already done")
        continue
    if os.path.exists("%s.skip" % corpus_output):
        print(rel_fname, "skipped")
        continue

    print(rel_fname)
    subprocess.check_call(
        "../../../python compiler_runtest.py %s > %s.out" % (fname, corpus_output),
        shell=True,
    )
    subprocess.check_call(
        "diff -u %s.exp %s.out" % (corpus_output, corpus_output), shell=True
    )
    with open("%s.done" % corpus_output, "w"):
        pass
