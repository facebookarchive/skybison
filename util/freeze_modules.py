#!/usr/bin/env python3.6

import argparse
import importlib
import os
import pathlib
import re
import sys
import tempfile
from collections import namedtuple


NAMESPACE_HEADER = "namespace py {"
NAMESPACE_FOOTER = "}  // namespace py"
UPPERCASE_RE = re.compile("_([a-zA-Z])")


def to_char_array(byte_array):
    """Returns a string with a brace-enclosed list of C char array elements"""
    members = ", ".join(f"0x{int(byte):02x}" for byte in byte_array)
    return f"{{{members}}}"


def symbol_to_cpp(name, upcase):
    """
    Transcribe symbol to C++ symbol syntax. Examples:
      'foo' => 'foo'
      '_hello' => 'underHello'
      '__foo_bar__' => 'dunderFooBar'
    """
    words = []
    if name.startswith("__") and name.endswith("__"):
        words.append("dunder")
        name = name[2:-2]
    if name.startswith("_"):
        words.append("under")
        name = name[1:]
    assert not name.startswith("_")
    assert not name.endswith("_")
    words += name.split("_")
    word0 = words[0]
    if upcase:
        result = word0[0].upper() + word0[1:].lower()
    else:
        result = word0.lower()
    for word in words[1:]:
        result += word[0].upper() + word[1:].lower()
    return result


ModuleData = namedtuple("ModuleData", ("cpp_name", "initializer"))


def process_module(filename):
    module_name = os.path.splitext(os.path.basename(filename))[0]
    with open(filename) as fp:
        source = fp.read()
    module_code = compile(source, filename, "exec", dont_inherit=True, optimize=0)
    bytecode = importlib._bootstrap_external._code_to_bytecode(module_code)
    initializer = to_char_array(bytecode)
    module_cpp_name = symbol_to_cpp(module_name, upcase=True)
    return ModuleData(module_cpp_name, initializer)


def write_source(fp, modules):
    """Writes the frozen module definitions to a C++ source file."""
    fp.write(
        f"""\
#include "frozen-modules.h"

#include "globals.h"
#include "modules.h"

{NAMESPACE_HEADER}
"""
    )

    for module in modules:
        cpp_name = module.cpp_name
        fp.write(
            f"""
static const byte k{cpp_name}Data[] = {module.initializer};
const FrozenModule k{cpp_name}ModuleData = {{
  ARRAYSIZE(k{cpp_name}Data),
  k{cpp_name}Data,
}};"""
        )

    fp.write(
        f"""\
{NAMESPACE_FOOTER}
"""
    )


def write_header(fp, modules):
    """Writes the frozen module declarations to a C++ header file."""
    fp.write(
        f"""\
#include "modules.h"

{NAMESPACE_HEADER}
"""
    )

    for module in modules:
        fp.write(
            f"""\
extern const FrozenModule k{module.cpp_name}ModuleData;
"""
        )

    fp.write(
        f"""\
{NAMESPACE_FOOTER}
"""
    )


def main(argv):
    parser = argparse.ArgumentParser(prog=argv[0])
    parser.add_argument("dir", help="directory where output is written")
    parser.add_argument("files", nargs="+", help="list of files to freeze")
    args = parser.parse_args(argv[1:])
    os.makedirs(args.dir, exist_ok=True)
    modules = list(map(process_module, args.files))
    with open(pathlib.Path(args.dir, "frozen-modules.cpp"), "w") as f:
        write_source(f, modules)
    with open(pathlib.Path(args.dir, "frozen-modules.h"), "w") as f:
        write_header(f, modules)


if __name__ == "__main__":
    main(sys.argv)
