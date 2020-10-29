#!/usr/bin/env python3
#
# Compare "compiler" package output against reference data produced
# by test_testcorpus_prepare.py.
#
import glob
from subprocess import check_call


for t in sorted(glob.glob("testcorpus/*.py")):
    print(t)
    check_call("../../../python compiler_runtest.py %s > %s.out" % (t, t), shell=True)
    check_call("diff -u %s.exp %s.out" % (t, t), shell=True)
