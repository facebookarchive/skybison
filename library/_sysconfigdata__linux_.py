#!/usr/bin/env python3

build_time_vars = {
    "AR": "",
    "ARFLAGS": "",
    "CC": "gcc.par",
    "CCSHARED": "-fPIC",
    "CFLAGS": "-Wno-unused-result -Wsign-compare -g -Og -Wall",
    "CXX": "g++.par",
    "EXT_SUFFIX": ".so",
    "LDSHARED": "gcc.par -pthread -shared",
    "OPT": "",
    "SHLIB_SUFFIX": ".so",
}
