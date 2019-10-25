#!/usr/bin/env python3.6

import argparse
import ctypes
import os
import pathlib
import py_compile
import re
import sys
import tempfile


NAMESPACE_HEADER = "namespace py {"
NAMESPACE_FOOTER = "}  // namespace py"
UPPERCASE_RE = re.compile("_([a-zA-Z])")


def compile_file(filename):
    """Compiles a source file and returns its byte-code as a bytes object."""
    with tempfile.TemporaryDirectory() as dirname:
        compiled = pathlib.Path(dirname, f"{os.path.basename(filename)}c")
        py_compile.compile(filename, cfile=compiled)
        return compiled.read_bytes()


def to_char_array(byte_array):
    """Returns a string with a brace-enclosed list of C char array elements"""
    members = ", ".join(f"{ctypes.c_int8(byte).value}" for byte in byte_array)
    return f"{{{members}}}"


def decl(filename):
    """Returns a declaration for the module data constant."""
    symbol = os.path.splitext(os.path.basename(filename).capitalize())[0]
    symbol = re.sub(UPPERCASE_RE, lambda m: f"Under{m.group(1).upper()}", symbol)
    return f"extern const char k{symbol}ModuleData[]"


def write_source(source_filename, module_filenames):
    """Writes the frozen module definitions to a C++ source file."""
    with open(source_filename, "w") as f:
        print(NAMESPACE_HEADER, file=f)
        for filename in module_filenames:
            byte_code = compile_file(filename)
            initializer = to_char_array(byte_code)
            print(f"{decl(filename)} = {initializer};", file=f)
        print(NAMESPACE_FOOTER, file=f)


def write_header(header_filename, module_filenames):
    """Writes the frozen module declarations to a C++ header file."""
    with open(header_filename, "w") as f:
        print(NAMESPACE_HEADER, file=f)
        for filename in module_filenames:
            print(f"{decl(filename)};", file=f)
        print(NAMESPACE_FOOTER, file=f)


def main(argv):
    parser = argparse.ArgumentParser(prog=argv[0])
    parser.add_argument("dir", help="directory where output is written")
    parser.add_argument("files", nargs="+", help="list of files to freeze")
    args = parser.parse_args(argv[1:])
    os.makedirs(args.dir, exist_ok=True)
    write_source(pathlib.Path(args.dir, "frozen-modules.cpp"), args.files)
    write_header(pathlib.Path(args.dir, "frozen-modules.h"), args.files)


if __name__ == "__main__":
    main(sys.argv)
