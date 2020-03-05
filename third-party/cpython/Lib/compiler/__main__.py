import sys
import re
import ast
import dis
import argparse

from . import pycodegen


# https://www.python.org/dev/peps/pep-0263/
coding_re = re.compile(rb"^[ \t\f]*#.*?coding[:=][ \t]*([-_.a-zA-Z0-9]+)")

def open_with_coding(fname):
    with open(fname, "rb") as f:
        l = f.readline()
        m = coding_re.match(l)
        if not m:
            l = f.readline()
            m = coding_re.match(l)
        encoding = "utf-8"
        if m:
            encoding = m.group(1).decode()
    return open(fname, encoding=encoding)


argparser = argparse.ArgumentParser(
    prog="compiler",
    description="Compile/execute a Python3 source file",
    epilog="""\
By default, compile source code into in-memory code object and execute it.
If -c is specified, instead of executing write .pyc file.

WARNING: This is WIP, co_stacksize may be calculated incorrectly and code
may crash.
"""
)
argparser.add_argument("-c", action="store_true", help="compile into .pyc file instead of executing")
argparser.add_argument("--dis", action="store_true", help="disassemble compiled code")
argparser.add_argument("-o", metavar="file", help="output .pyc file")
argparser.add_argument("input", help="source .py file")
args = argparser.parse_args()

with open_with_coding(args.input) as f:
    source = f.read()

gen = pycodegen.Module(source, args.input)
gen.compile()
codeobj = gen.getCode()

if args.dis:
    dis.dis(codeobj)

if args.c:
    name = args.o or args.input.rsplit(".", 1)[0] + ".pyc"
    with open(name, "wb") as f:
        gen.dump(f)
else:
    exec(codeobj)
