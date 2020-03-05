import ast
import sys
import dis
import glob
from io import TextIOWrapper
from tokenize import detect_encoding
from unittest import TestCase
from compiler.pycodegen import compile as py_compile
from os import path
import re
from io import StringIO
from .common import get_repo_root, glob_test
from .dis_stable import Disassembler

IGNORE_PATTERNS = (
    # Not a valid Python3 syntax
    "lib2to3/tests/data",
    "test/badsyntax_",
    "test/bad_coding",
)


class SbsCompileTests(TestCase):
    pass


# Add a test case for each standard library file to SbsCompileTests.  Individual
# tests can be run with:
#  python -m test.test_compiler SbsCompileTests.test_Lib_test_test_unary
def add_test(modname, fname):
    assert fname.startswith(libpath + "/")
    for p in IGNORE_PATTERNS:
        if p in fname:
            return

    modname = path.relpath(fname, REPO_ROOT)

    def test_stdlib(self):
        with open(fname, "rb") as inp:
            encoding, _lines = detect_encoding(inp.readline)
            code = b"".join(_lines + inp.readlines()).decode(encoding)
            node = ast.parse(code, modname, "exec")
            node.filename = modname

            orig = compile(node, modname, "exec")
            origdump = StringIO()
            Disassembler().dump_code(orig, origdump)

            codeobj = py_compile(node, modname, "exec")
            newdump = StringIO()
            Disassembler().dump_code(codeobj, newdump)

            self.assertEqual(
                origdump.getvalue().split("\n"), newdump.getvalue().split("\n")
            )

    test_stdlib.__name__ = "test_stdlib_" + modname.replace("/", "_")[:-3]
    setattr(SbsCompileTests, test_stdlib.__name__, test_stdlib)


REPO_ROOT = get_repo_root()
libpath = path.join(REPO_ROOT, "Lib")
glob_test(libpath, "**/*.py", add_test)
