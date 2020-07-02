#!/usr/bin/env python3
import glob
import os.path
import sys
from lib2to3.main import main as main_2to3


FIXER_PKG = "lib2to3.fixes"


def bench_2to3():
    datadir = os.path.join(os.path.dirname(__file__), "data", "2to3")
    pyfiles = glob.glob(os.path.join(datadir, "*.py.txt"))
    argv_original = sys.argv
    sys.argv = [sys.argv[0], "-f", "all"] + pyfiles
    main_2to3(FIXER_PKG)
    sys.argv = argv_original


def run():
    bench_2to3()


if __name__ == "__main__":
    bench_2to3()
