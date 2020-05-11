#!/usr/bin/env python3
# In cpython this file contains a dump of all variables in the Makefile used
# to build cpython. We just hardcode a selection of the values for our runtime
# here.

build_time_vars = {
    "ABIFLAGS": "",
    "AR": "",
    "ARFLAGS": "",
    "CC": "cc",
    "CCSHARED": "-fPIC",
    "CFLAGS": "-Wno-unused-result -Wsign-compare -g -Wall",
    "CXX": "c++",
    "EXT_SUFFIX": ".pyro.so",
    "LDSHARED": "cc -bundle -undefined dynamic_lookup",
    "OPT": "",
    "SHLIB_SUFFIX": ".so",
    "SOABI": "pyro",
}
