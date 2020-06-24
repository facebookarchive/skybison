#!/usr/bin/env python3
# In cpython this file contains a dump of all variables in the Makefile used
# to build cpython. We just hardcode a selection of the values for our runtime
# here.

build_time_vars = {
    "ABIFLAGS": "",
    "AR": "",
    "ARFLAGS": "",
    "CC": "gcc",
    "CCSHARED": "-fPIC",
    "CFLAGS": "-Wno-unused-result -Wsign-compare -g -Og -Wall",
    "CXX": "g++",
    "EXT_SUFFIX": ".pyro.so",
    "LDCXXSHARED": "g++ -pthread -shared",
    "LDSHARED": "gcc -pthread -shared",
    "OPT": "",
    "SHLIB_SUFFIX": ".so",
    "SOABI": "pyro",
}
