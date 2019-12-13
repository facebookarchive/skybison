#!/usr/bin/env python3

build_time_vars = {
    "AR": "",
    "ARFLAGS": "",
    "CC": "cc",
    "CCSHARED": "-fPIC",
    "CFLAGS": "-Wno-unused-result -Wsign-compare -g -Wall",
    "CXX": "c++",
    "EXT_SUFFIX": ".so",
    "LDSHARED": "cc -bundle -undefined dynamic_lookup",
    "OPT": "",
    "SHLIB_SUFFIX": ".so",
}
