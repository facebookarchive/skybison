#!/usr/bin/env python3
#
# Compile short programs in testcorpus/ using CPython,
# to serve as comparison reference.
# Make sure you have built CPython3.5 with peephole optimizer disabled
# using build-cpython-compiler.sh script.
#

import glob
from subprocess import check_call


for t in glob.glob("testcorpus/*.py"):
    check_call("../../../python dis_stable.py %s > %s.exp" % (t, t), shell=True)
