import ast
import dis
import gc
import itertools
import re
import sys
import unittest
import warnings
from array import array
from compiler import consts, walk
from compiler.optimizer import AstOptimizer
from compiler.pycodegen import Python37CodeGenerator
from compiler.static import (
    BOOL_TYPE,
    BYTES_TYPE,
    COMPLEX_TYPE,
    DICT_TYPE,
    DYNAMIC,
    FLOAT_TYPE,
    INT_TYPE,
    NONE_TYPE,
    OBJECT_TYPE,
    PRIM_OP_ADD,
    PRIM_OP_DIV,
    PRIM_OP_GT,
    PRIM_OP_LT,
    STR_TYPE,
    DeclarationVisitor,
    Function,
    StaticCodeGenerator,
    SymbolTable,
    TypeBinder,
    TypedSyntaxError,
    Value,
)
from compiler.symbols import SymbolVisitor
from contextlib import contextmanager
from io import StringIO
from os import path
from textwrap import dedent
from types import MemberDescriptorType, ModuleType
from typing import Optional, Tuple
from unittest import skipIf

from .common import CompilerTest


try:
    import cinderjit
except ImportError:
    cinderjit = None

HAS_INVOKE_METHOD = "INVOKE_METHOD" in dis.opmap
HAS_FIELDS = "LOAD_FIELD" in dis.opmap
HAS_CAST = "CAST" in dis.opmap

RICHARDS_PATH = path.join(
    path.dirname(__file__),
    "..",
    "..",
    "..",
    "Tools",
    "benchmarks",
    "richards_static.py",
)


def type_mismatch(from_type: str, to_type: str) -> str:
    return re.escape(f"type mismatch: {from_type} cannot be assigned to {to_type}")


def optional(type: str) -> str:
    return f"Optional[{type}]"


class StaticTestBase(CompilerTest):
    @contextmanager
    def in_module(self, code, name=None, code_gen=StaticCodeGenerator):
        if name is None:
            # get our callers name to avoid duplicating names
            name = sys._getframe().f_back.f_back.f_code.co_name

        try:
            compiled = self.compile(code, code_gen, name)
            sys.modules[name] = d = {}
            exec(compiled, d)

            yield d
        finally:
            # don't throw a new exception if we failed to compile
            if name in sys.modules:
                del sys.modules[name]
                d.clear()
                gc.collect()

    @property
    def base_size(self):
        class C:
            __slots__ = ()

        return sys.getsizeof(C())

    @property
    def ptr_size(self):
        return 8 if sys.maxsize > 2 ** 32 else 4


class StaticCompilationTests(StaticTestBase):
    def test_static_import_unknown(self) -> None:
        codestr = """
            from __static__ import does_not_exist
        """
        with self.assertRaises(TypedSyntaxError):
            self.compile(codestr, StaticCodeGenerator, modname="foo")

    def test_static_import_star(self) -> None:
        codestr = """
            from __static__ import *
        """
        with self.assertRaises(TypedSyntaxError):
            self.compile(codestr, StaticCodeGenerator, modname="foo")

    def test_redefine_local_type(self) -> None:
        codestr = """
            class C: pass
            class D: pass

            def f():
                x: C = C()
                x: D = D()
        """
        with self.assertRaises(TypedSyntaxError):
            self.compile(codestr, StaticCodeGenerator, modname="foo")

    def test_mixed_chain_assign(self) -> None:
        codestr = """
            class C: pass
            class D: pass

            def f():
                x: C = C()
                y: D = D()
                x = y = D()
        """
        with self.assertRaisesRegex(TypedSyntaxError, type_mismatch("foo.D", "foo.C")):
            self.compile(codestr, StaticCodeGenerator, modname="foo")

    def test_bool_cast(self) -> None:
        codestr = """
            from __static__ import cast
            class D: pass

            def f(x) -> bool:
                y: bool = cast(bool, x)
                return y
        """
        self.compile(codestr, StaticCodeGenerator, modname="foo")

    def test_mixed_binop(self):
        with self.assertRaisesRegex(
            TypedSyntaxError, "cannot add int64 and builtins.int instance"
        ):
            self.bind_module(
                """
                from __static__ import size_t

                def f():
                    x: size_t = 1
                    y = 1
                    x + y
            """
            )

        with self.assertRaisesRegex(
            TypedSyntaxError, "cannot add builtins.int instance and int64"
        ):
            self.bind_module(
                """
                from __static__ import size_t

                def f():
                    x: size_t = 1
                    y = 1
                    y + x
            """
            )

    def test_mixed_binop_okay(self):
        codestr = """
            from __static__ import size_t, box

            def f():
                x: size_t = 1
                y = x + 1
                return box(y)
        """
        with self.in_module(codestr) as mod:
            f = mod["f"]
            self.assertEqual(f(), 2)

    def test_mixed_binop_okay_1(self):
        codestr = """
            from __static__ import size_t, box

            def f():
                x: size_t = 1
                y = 1 + x
                return box(y)
        """
        with self.in_module(codestr) as mod:
            f = mod["f"]
            self.assertEqual(f(), 2)

    def test_inferred_primitive_type(self):
        codestr = """
        from __static__ import size_t, box

        def f():
            x: size_t = 1
            y = x
            return box(y)
        """
        with self.in_module(codestr) as mod:
            f = mod["f"]
            self.assertEqual(f(), 1)

    def test_subclass_binop(self):
        codestr = """
            class C: pass
            class D(C): pass

            def f(x: C, y: D):
                return x + y
        """
        code = self.compile(codestr, StaticCodeGenerator, modname="foo")
        f = self.find_code(code, "f")
        self.assertInBytecode(f, "BINARY_ADD")

    def test_int_bad_assign(self):
        with self.assertRaisesRegex(
            TypedSyntaxError, "type mismatch: builtins.str cannot be assigned to int64"
        ):
            self.compile(
                """
            from __static__ import size_t
            def f():
                x: size_t = 'abc'
            """,
                StaticCodeGenerator,
            )

    def test_sign_extend(self):
        codestr = """
            from __static__ import int16, int64, box
            def testfunc():
                x: int16 = -40
                y: int64 = x
                return box(y)
            """
        self.compile(codestr, StaticCodeGenerator)
        f = self.run_code(codestr, StaticCodeGenerator)["testfunc"]
        self.assertEqual(f(), -40)

    def test_field_size(self):
        for type in [
            "int8",
            "int16",
            "int32",
            "int64",
            "uint8",
            "uint16",
            "uint32",
            "uint64",
        ]:
            codestr = f"""
                from __static__ import {type}, box
                class C{type}:
                    def __init__(self):
                        self.a: {type} = 1
                        self.b: {type} = 1

                def testfunc(c: C{type}):
                    c.a = 2
                    return box(c.b)
                """
            with self.in_module(codestr) as mod:
                C = mod["C" + type]
                f = mod["testfunc"]
                self.assertEqual(f(C()), 1)

    def test_field_sign_ext(self):
        """tests that we do the correct sign extension when loading from a field"""
        for type, val in [
            ("int32", 65537),
            ("int16", 256),
            ("int8", 0x7F),
            ("uint32", 65537),
        ]:
            codestr = f"""
                from __static__ import {type}, box
                class C{type}:
                    def __init__(self):
                        self.value: {type} = {val}

                def testfunc(c: C{type}):
                    return box(c.value)
                """

            with self.in_module(codestr) as mod:
                C = mod["C" + type]
                f = mod["testfunc"]
                self.assertEqual(f(C()), val)

    def test_field_unsign_ext(self):
        """tests that we do the correct sign extension when loading from a field"""
        for type, val, test in [("uint32", 65537, -1)]:
            codestr = f"""
                from __static__ import {type}, int64, box
                class C{type}:
                    def __init__(self):
                        self.value: {type} = {val}

                def testfunc(c: C{type}):
                    z: int64 = {test}
                    if c.value < z:
                        return True
                    return False
                """

            with self.in_module(codestr) as mod:
                C = mod["C" + type]
                f = mod["testfunc"]
                self.assertEqual(f(C()), False)

    def test_field_sign_compare(self):
        for type, val, test in [("int32", -1, -1)]:
            codestr = f"""
                from __static__ import {type}, box
                class C{type}:
                    def __init__(self):
                        self.value: {type} = {val}

                def testfunc(c: C{type}):
                    if c.value == {test}:
                        return True
                    return False
                """

            with self.in_module(codestr) as mod:
                C = mod["C" + type]
                f = mod["testfunc"]
                self.assertTrue(f(C()))

    def test_mixed_binop_sign(self):
        """mixed signed/unsigned ops should be promoted to signed"""
        codestr = """
            from __static__ import int8, uint8, box
            def testfunc():
                x: uint8 = 42
                y: int8 = 2
                return box(x / y)
        """
        code = self.compile(codestr, StaticCodeGenerator)
        f = self.find_code(code)
        self.assertInBytecode(f, "INT_BINARY_OP", PRIM_OP_DIV)
        with self.in_module(codestr) as mod:
            f = mod["testfunc"]
            self.assertEqual(f(), 21)

        codestr = """
            from __static__ import int8, uint8, box
            def testfunc():
                x: int8 = 42
                y: uint8 = 2
                return box(x / y)
        """
        code = self.compile(codestr, StaticCodeGenerator)
        f = self.find_code(code)
        self.assertInBytecode(f, "INT_BINARY_OP", PRIM_OP_DIV)
        with self.in_module(codestr) as mod:
            f = mod["testfunc"]
            self.assertEqual(f(), 21)

    def test_mixed_cmpop_sign(self):
        """mixed signed/unsigned ops should be promoted to signed"""
        codestr = """
            from __static__ import int8, uint8, box
            def testfunc(tst=False):
                x: uint8 = 42
                y: int8 = 2
                if tst:
                    x += 1
                    y += 1

                if x < y:
                    return True
                return False
        """
        code = self.compile(codestr, StaticCodeGenerator)
        f = self.find_code(code)
        self.assertInBytecode(f, "INT_COMPARE_OP", PRIM_OP_LT)
        with self.in_module(codestr) as mod:
            f = mod["testfunc"]
            self.assertEqual(f(), False)

        codestr = """
            from __static__ import int8, uint8, box
            def testfunc(tst=False):
                x: int8 = 42
                y: uint8 = 2
                if tst:
                    x += 1
                    y += 1

                if x < y:
                    return True
                return False
        """
        code = self.compile(codestr, StaticCodeGenerator)
        f = self.find_code(code)
        self.assertInBytecode(f, "INT_COMPARE_OP", PRIM_OP_LT)
        with self.in_module(codestr) as mod:
            f = mod["testfunc"]
            self.assertEqual(f(), False)

    def test_mixed_add_reversed(self):
        """mixed signed/unsigned ops should be promoted to signed"""
        codestr = """
            from __static__ import int8, uint8, int64, box, int16
            def testfunc(tst=False):
                x: int8 = 42
                y: int16 = 2
                if tst:
                    x += 1
                    y += 1

                return box(y + x)
        """
        code = self.compile(codestr, StaticCodeGenerator)
        f = self.find_code(code)
        self.assertInBytecode(f, "INT_BINARY_OP", PRIM_OP_ADD)
        with self.in_module(codestr) as mod:
            f = mod["testfunc"]
            self.assertEqual(f(), 44)

    def test_mixed_tri_add(self):
        """mixed signed/unsigned ops should be promoted to signed"""
        codestr = """
            from __static__ import int8, uint8, int64, box
            def testfunc(tst=False):
                x: uint8 = 42
                y: int8 = 2
                z: int64 = 3
                if tst:
                    x += 1
                    y += 1

                return box(x + y + z)
        """
        code = self.compile(codestr, StaticCodeGenerator)
        f = self.find_code(code)
        self.assertInBytecode(f, "INT_BINARY_OP", PRIM_OP_ADD)
        with self.in_module(codestr) as mod:
            f = mod["testfunc"]
            self.assertEqual(f(), 47)

    def test_mixed_tri_add_unsigned(self):
        """promote int/uint to int, can't add to uint64"""

        codestr = """
            from __static__ import int8, uint8, uint64, box
            def testfunc(tst=False):
                x: uint8 = 42
                y: int8 = 2
                z: uint64 = 3

                return box(x + y + z)
        """

        with self.assertRaisesRegex(TypedSyntaxError, "cannot add int16 and uint64"):
            self.compile(codestr, StaticCodeGenerator)

    def test_store_signed_to_unsigned(self):
        """promote int/uint to int, can't add to uint64"""

        codestr = """
            from __static__ import int8, uint8, uint64, box
            def testfunc(tst=False):
                x: uint8 = 42
                y: int8 = 2
                x = y
        """
        with self.assertRaisesRegex(TypedSyntaxError, type_mismatch("int8", "uint8")):
            self.compile(codestr, StaticCodeGenerator)

    def test_store_unsigned_to_signed(self):
        """promote int/uint to int, can't add to uint64"""

        codestr = """
            from __static__ import int8, uint8, uint64, box
            def testfunc(tst=False):
                x: uint8 = 42
                y: int8 = 2
                y = x
        """
        with self.assertRaisesRegex(TypedSyntaxError, type_mismatch("uint8", "int8")):
            self.compile(codestr, StaticCodeGenerator)

    def test_mixed_assign_larger(self):
        """promote int/uint to int16"""

        codestr = """
            from __static__ import int8, uint8, int16, box
            def testfunc(tst=False):
                x: uint8 = 42
                y: int8 = 2
                z: int16 = x + y

                return box(z)
        """
        code = self.compile(codestr, StaticCodeGenerator)
        f = self.find_code(code)
        self.assertInBytecode(f, "INT_BINARY_OP", PRIM_OP_ADD)
        with self.in_module(codestr) as mod:
            f = mod["testfunc"]
            self.assertEqual(f(), 44)

    def test_mixed_assign_larger_2(self):
        """promote int/uint to int16"""

        codestr = """
            from __static__ import int8, uint8, int16, box
            def testfunc(tst=False):
                x: uint8 = 42
                y: int8 = 2
                z: int16
                z = x + y

                return box(z)
        """
        code = self.compile(codestr, StaticCodeGenerator)
        f = self.find_code(code)
        self.assertInBytecode(f, "INT_BINARY_OP", PRIM_OP_ADD)
        with self.in_module(codestr) as mod:
            f = mod["testfunc"]
            self.assertEqual(f(), 44)

    @skipIf(True, "this isn't implemented yet")
    def test_unwind(self):
        codestr = """
            from __static__ import int32
            def raises():
                raise IndexError()

            def testfunc():
                x: int32 = 1
                raises()
                print(x)
            """

        with self.in_module(codestr) as mod:
            f = mod["testfunc"]
            with self.assertRaises(IndexError):
                f()

    def test_int_constant_range(self):
        for type, val, low, high in [
            ("int8", 128, -128, 127),
            ("int8", -129, -128, 127),
            ("int16", 32768, -32768, 32767),
            ("int16", -32769, -32768, 32767),
            ("int32", 2147483648, -2147483648, 2147483647),
            ("int32", -2147483649, -2147483648, 2147483647),
            ("int64", 9223372036854775808, -9223372036854775808, 9223372036854775807),
            ("int64", -9223372036854775809, -9223372036854775808, 9223372036854775807),
            ("uint8", 257, 0, 256),
            ("uint8", -1, 0, 256),
            ("uint16", 65537, 0, 65536),
            ("uint16", -1, 0, 65536),
            ("uint32", 4294967297, 0, 4294967296),
            ("uint32", -1, 0, 4294967296),
            ("uint64", 18446744073709551617, 0, 18446744073709551616),
            ("uint64", -1, 0, 18446744073709551616),
        ]:
            codestr = f"""
                from __static__ import {type}
                def testfunc(tst):
                    x: {type} = {val}
            """
            with self.assertRaisesRegex(
                TypedSyntaxError,
                f"constant {val} is outside of the range {low} to {high} for {type}",
            ):
                self.compile(codestr, StaticCodeGenerator)

    def test_int_assign_float(self):
        codestr = """
            from __static__ import int8
            def testfunc(tst):
                x: int8 = 1.0
        """
        with self.assertRaisesRegex(
            TypedSyntaxError,
            "float cannot be used in a context where an int is expected",
        ):
            self.compile(codestr, StaticCodeGenerator)

    def test_int_assign_str_constant(self):
        codestr = """
            from __static__ import int8
            def testfunc(tst):
                x: int8 = 'abc' + 'def'
        """
        with self.assertRaisesRegex(
            TypedSyntaxError,
            "str cannot be used in a context where an int is expected",
        ):
            self.compile(codestr, StaticCodeGenerator)

    def test_int_large_int_constant(self):
        codestr = """
            from __static__ import int64
            def testfunc(tst):
                x: int64 = 0x7FFFFFFF + 1
        """
        code = self.compile(codestr, StaticCodeGenerator)
        f = self.find_code(code)
        self.assertInBytecode(f, "LOAD_CONST", 0x80000000)
        self.assertInBytecode(f, "INT_UNBOX")

    def test_int_int_constant(self):
        codestr = """
            from __static__ import int64
            def testfunc(tst):
                x: int64 = 0x7FFFFFFE + 1
        """
        code = self.compile(codestr, StaticCodeGenerator)
        f = self.find_code(code)
        self.assertInBytecode(f, "INT_LOAD_CONST", 0x7FFFFFFF)

    def test_int_add_mixed_64(self):
        codestr = """
            from __static__ import uint64, int64, box
            def testfunc(tst):
                x: uint64 = 0
                y: int64 = 1
                if tst:
                    x = x + 1
                    y = y + 2

                return box(x + y)
        """
        with self.assertRaisesRegex(TypedSyntaxError, "cannot add uint64 and int64"):
            self.compile(codestr, StaticCodeGenerator)

    def test_int_overflow_add(self):
        tests = [
            ("int8", 100, 100, -56),
            ("int16", 200, 200, 400),
            ("int32", 200, 200, 400),
            ("int64", 200, 200, 400),
            ("int16", 20000, 20000, -25536),
            ("int32", 40000, 40000, 80000),
            ("int64", 40000, 40000, 80000),
            ("int32", 2000000000, 2000000000, -294967296),
            ("int64", 2000000000, 2000000000, 4000000000),
            ("int8", 127, 127, -2),
            ("int16", 32767, 32767, -2),
            ("int32", 2147483647, 2147483647, -2),
            ("int64", 9223372036854775807, 9223372036854775807, -2),
            ("uint8", 200, 200, 144),
            ("uint16", 200, 200, 400),
            ("uint32", 200, 200, 400),
            ("uint64", 200, 200, 400),
            ("uint16", 40000, 40000, 14464),
            ("uint32", 40000, 40000, 80000),
            ("uint64", 40000, 40000, 80000),
            ("uint32", 2000000000, 2000000000, 4000000000),
            ("uint64", 2000000000, 2000000000, 4000000000),
            ("uint8", 1 << 7, 1 << 7, 0),
            ("uint16", 1 << 15, 1 << 15, 0),
            ("uint32", 1 << 31, 1 << 31, 0),
            ("uint64", 1 << 63, 1 << 63, 0),
            ("uint8", 1 << 6, 1 << 6, 128),
            ("uint16", 1 << 14, 1 << 14, 32768),
            ("uint32", 1 << 30, 1 << 30, 2147483648),
            ("uint64", 1 << 62, 1 << 62, 9223372036854775808),
        ]

        for type, x, y, res in tests:
            codestr = f"""
            from __static__ import {type}, box
            def f():
                x: {type} = {x}
                y: {type} = {y}
                z: {type} = x + y
                return box(z)
            """
            self.compile(codestr, StaticCodeGenerator)
            f = self.run_code(codestr, StaticCodeGenerator)["f"]
            self.assertEqual(f(), res, f"{type} {x} {y} {res}")

    def test_int_unary(self):
        tests = [
            ("int8", "-", 1, -1),
            ("uint8", "-", 1, (1 << 8) - 1),
            ("int16", "-", 1, -1),
            ("int16", "-", 256, -256),
            ("uint16", "-", 1, (1 << 16) - 1),
            ("int32", "-", 1, -1),
            ("int32", "-", 65536, -65536),
            ("uint32", "-", 1, (1 << 32) - 1),
            ("int64", "-", 1, -1),
            ("int64", "-", 1 << 32, -(1 << 32)),
            ("uint64", "-", 1, (1 << 64) - 1),
            ("int8", "~", 1, -2),
            ("uint8", "~", 1, (1 << 8) - 2),
            ("int16", "~", 1, -2),
            ("uint16", "~", 1, (1 << 16) - 2),
            ("int32", "~", 1, -2),
            ("uint32", "~", 1, (1 << 32) - 2),
            ("int64", "~", 1, -2),
            ("uint64", "~", 1, (1 << 64) - 2),
        ]
        for type, op, x, res in tests:
            codestr = f"""
            from __static__ import {type}, box
            def testfunc(tst):
                x: {type} = {x}
                if tst:
                    x = x + 1
                x = {op}x
                return box(x)
            """
            self.compile(codestr, StaticCodeGenerator)
            f = self.run_code(codestr, StaticCodeGenerator)["testfunc"]
            self.assertEqual(f(False), res, f"{type} {op} {x} {res}")

    def test_int_compare(self):
        tests = [
            ("int8", 1, 2, "==", False),
            ("int8", 1, 2, "!=", True),
            ("int8", 1, 2, "<", True),
            ("int8", 1, 2, "<=", True),
            ("int8", 2, 1, "<", False),
            ("int8", 2, 1, "<=", False),
            ("int8", -1, 2, "==", False),
            ("int8", -1, 2, "!=", True),
            ("int8", -1, 2, "<", True),
            ("int8", -1, 2, "<=", True),
            ("int8", 2, -1, "<", False),
            ("int8", 2, -1, "<=", False),
            ("uint8", 1, 2, "==", False),
            ("uint8", 1, 2, "!=", True),
            ("uint8", 1, 2, "<", True),
            ("uint8", 1, 2, "<=", True),
            ("uint8", 2, 1, "<", False),
            ("uint8", 2, 1, "<=", False),
            ("uint8", 255, 2, "==", False),
            ("uint8", 255, 2, "!=", True),
            ("uint8", 255, 2, "<", False),
            ("uint8", 255, 2, "<=", False),
            ("uint8", 2, 255, "<", True),
            ("uint8", 2, 255, "<=", True),
            ("int16", 1, 2, "==", False),
            ("int16", 1, 2, "!=", True),
            ("int16", 1, 2, "<", True),
            ("int16", 1, 2, "<=", True),
            ("int16", 2, 1, "<", False),
            ("int16", 2, 1, "<=", False),
            ("int16", -1, 2, "==", False),
            ("int16", -1, 2, "!=", True),
            ("int16", -1, 2, "<", True),
            ("int16", -1, 2, "<=", True),
            ("int16", 2, -1, "<", False),
            ("int16", 2, -1, "<=", False),
            ("uint16", 1, 2, "==", False),
            ("uint16", 1, 2, "!=", True),
            ("uint16", 1, 2, "<", True),
            ("uint16", 1, 2, "<=", True),
            ("uint16", 2, 1, "<", False),
            ("uint16", 2, 1, "<=", False),
            ("uint16", 65535, 2, "==", False),
            ("uint16", 65535, 2, "!=", True),
            ("uint16", 65535, 2, "<", False),
            ("uint16", 65535, 2, "<=", False),
            ("uint16", 2, 65535, "<", True),
            ("uint16", 2, 65535, "<=", True),
            ("int32", 1, 2, "==", False),
            ("int32", 1, 2, "!=", True),
            ("int32", 1, 2, "<", True),
            ("int32", 1, 2, "<=", True),
            ("int32", 2, 1, "<", False),
            ("int32", 2, 1, "<=", False),
            ("int32", -1, 2, "==", False),
            ("int32", -1, 2, "!=", True),
            ("int32", -1, 2, "<", True),
            ("int32", -1, 2, "<=", True),
            ("int32", 2, -1, "<", False),
            ("int32", 2, -1, "<=", False),
            ("uint32", 1, 2, "==", False),
            ("uint32", 1, 2, "!=", True),
            ("uint32", 1, 2, "<", True),
            ("uint32", 1, 2, "<=", True),
            ("uint32", 2, 1, "<", False),
            ("uint32", 2, 1, "<=", False),
            ("uint32", 4294967295, 2, "!=", True),
            ("uint32", 4294967295, 2, "<", False),
            ("uint32", 4294967295, 2, "<=", False),
            ("uint32", 2, 4294967295, "<", True),
            ("uint32", 2, 4294967295, "<=", True),
            ("int64", 1, 2, "==", False),
            ("int64", 1, 2, "!=", True),
            ("int64", 1, 2, "<", True),
            ("int64", 1, 2, "<=", True),
            ("int64", 2, 1, "<", False),
            ("int64", 2, 1, "<=", False),
            ("int64", -1, 2, "==", False),
            ("int64", -1, 2, "!=", True),
            ("int64", -1, 2, "<", True),
            ("int64", -1, 2, "<=", True),
            ("int64", 2, -1, "<", False),
            ("int64", 2, -1, "<=", False),
            ("uint64", 1, 2, "==", False),
            ("uint64", 1, 2, "!=", True),
            ("uint64", 1, 2, "<", True),
            ("uint64", 1, 2, "<=", True),
            ("uint64", 2, 1, "<", False),
            ("uint64", 2, 1, "<=", False),
            ("int64", 2, -1, ">", True),
            ("uint64", 2, 18446744073709551615, ">", False),
            ("int64", 2, -1, "<", False),
            ("uint64", 2, 18446744073709551615, "<", True),
            ("int64", 2, -1, ">=", True),
            ("uint64", 2, 18446744073709551615, ">=", False),
            ("int64", 2, -1, "<=", False),
            ("uint64", 2, 18446744073709551615, "<=", True),
        ]
        for type, x, y, op, res in tests:
            codestr = f"""
            from __static__ import {type}, box
            def testfunc(tst):
                x: {type} = {x}
                y: {type} = {y}
                if tst:
                    x = x + 1
                    y = y + 2

                if x {op} y:
                    return True
                return False
            """
            self.compile(codestr, StaticCodeGenerator)
            f = self.run_code(codestr, StaticCodeGenerator)["testfunc"]
            self.assertEqual(f(False), res, f"{type} {x} {op} {y} {res}")

    def test_int_compare_unboxed(self):
        codestr = """
        from __static__ import size_t, unbox
        def testfunc(x, y):
            x1: size_t = unbox(x)
            y1: size_t = unbox(y)

            if x1 > y1:
                return True
            return False
        """
        self.compile(codestr, StaticCodeGenerator)
        f = self.run_code(codestr, StaticCodeGenerator)["testfunc"]
        self.assertInBytecode(f, "POP_JUMP_IF_ZERO")
        self.assertEqual(f(1, 2), False)

    def test_int_compare_mixed(self):
        codestr = """
        from __static__ import size_t
        x = 1

        def testfunc():
            i: size_t = 0
            j = 0
            while i < 100 and x:
                i = i + 1
                j = j + 1
            return j
        """

        self.compile(codestr, StaticCodeGenerator)
        f = self.run_code(codestr, StaticCodeGenerator)["testfunc"]
        self.assertEqual(f(), 100)
        self.assert_jitted(f)

    def test_int_compare_or_dynamic(self):
        codestr = """
        from __static__ import size_t
        def cond():
            return True

        def testfunc():
            i: size_t = 0
            j = i > 0 or cond()
            return j
        """

        self.compile(codestr, StaticCodeGenerator)
        f = self.run_code(codestr, StaticCodeGenerator)["testfunc"]
        self.assertInBytecode(f, "JUMP_IF_NONZERO_OR_POP")
        self.assertEqual(f(), True)

    def test_int_compare_or_dynamic_false(self):
        codestr = """
        from __static__ import size_t
        def cond():
            return False

        def testfunc():
            i: size_t = 0
            j = i > 0 or cond()
            return j
        """

        self.compile(codestr, StaticCodeGenerator)
        f = self.run_code(codestr, StaticCodeGenerator)["testfunc"]
        self.assertInBytecode(f, "JUMP_IF_NONZERO_OR_POP")
        self.assertEqual(f(), False)

    def test_int_binop(self):
        tests = [
            ("int8", 1, 2, "/", 0),
            ("int8", 4, 2, "/", 2),
            ("int8", 4, -2, "/", -2),
            ("uint8", 0xFF, 0x7F, "/", 2),
            ("int16", 4, -2, "/", -2),
            ("uint16", 0xFF, 0x7F, "/", 2),
            ("uint32", 0xFFFF, 0x7FFF, "/", 2),
            ("int32", 4, -2, "/", -2),
            ("uint32", 0xFF, 0x7F, "/", 2),
            ("uint32", 0xFFFFFFFF, 0x7FFFFFFF, "/", 2),
            ("int64", 4, -2, "/", -2),
            ("uint64", 0xFF, 0x7F, "/", 2),
            ("uint64", 0xFFFFFFFFFFFFFFFF, 0x7FFFFFFFFFFFFFFF, "/", 2),
            ("int8", 1, -2, "-", 3),
            ("int8", 1, 2, "-", -1),
            ("int16", 1, -2, "-", 3),
            ("int16", 1, 2, "-", -1),
            ("int32", 1, -2, "-", 3),
            ("int32", 1, 2, "-", -1),
            ("int64", 1, -2, "-", 3),
            ("int64", 1, 2, "-", -1),
            ("int8", 1, -2, "*", -2),
            ("int8", 1, 2, "*", 2),
            ("int16", 1, -2, "*", -2),
            ("int16", 1, 2, "*", 2),
            ("int32", 1, -2, "*", -2),
            ("int32", 1, 2, "*", 2),
            ("int64", 1, -2, "*", -2),
            ("int64", 1, 2, "*", 2),
            ("int8", 1, -2, "&", 0),
            ("int8", 1, 3, "&", 1),
            ("int16", 1, 3, "&", 1),
            ("int16", 1, 3, "&", 1),
            ("int32", 1, 3, "&", 1),
            ("int32", 1, 3, "&", 1),
            ("int64", 1, 3, "&", 1),
            ("int64", 1, 3, "&", 1),
            ("int8", 1, 2, "|", 3),
            ("uint8", 1, 2, "|", 3),
            ("int16", 1, 2, "|", 3),
            ("uint16", 1, 2, "|", 3),
            ("int32", 1, 2, "|", 3),
            ("uint32", 1, 2, "|", 3),
            ("int64", 1, 2, "|", 3),
            ("uint64", 1, 2, "|", 3),
            ("int8", 1, 3, "^", 2),
            ("uint8", 1, 3, "^", 2),
            ("int16", 1, 3, "^", 2),
            ("uint16", 1, 3, "^", 2),
            ("int32", 1, 3, "^", 2),
            ("uint32", 1, 3, "^", 2),
            ("int64", 1, 3, "^", 2),
            ("uint64", 1, 3, "^", 2),
            ("int8", 1, 3, "%", 1),
            ("uint8", 1, 3, "%", 1),
            ("int16", 1, 3, "%", 1),
            ("uint16", 1, 3, "%", 1),
            ("int32", 1, 3, "%", 1),
            ("uint32", 1, 3, "%", 1),
            ("int64", 1, 3, "%", 1),
            ("uint64", 1, 3, "%", 1),
            ("int8", 1, -3, "%", 1),
            ("uint8", 1, 0xFF, "%", 1),
            ("int16", 1, -3, "%", 1),
            ("uint16", 1, 0xFFFF, "%", 1),
            ("int32", 1, -3, "%", 1),
            ("uint32", 1, 0xFFFFFFFF, "%", 1),
            ("int64", 1, -3, "%", 1),
            ("uint64", 1, 0xFFFFFFFFFFFFFFFF, "%", 1),
            ("int8", 1, 2, "<<", 4),
            ("uint8", 1, 2, "<<", 4),
            ("int16", 1, 2, "<<", 4),
            ("uint16", 1, 2, "<<", 4),
            ("int32", 1, 2, "<<", 4),
            ("uint32", 1, 2, "<<", 4),
            ("int64", 1, 2, "<<", 4),
            ("uint64", 1, 2, "<<", 4),
            ("int8", 4, 1, ">>", 2),
            ("int8", -1, 1, ">>", -1),
            ("uint8", 0xFF, 1, ">>", 127),
            ("int16", 4, 1, ">>", 2),
            ("int16", -1, 1, ">>", -1),
            ("uint16", 0xFFFF, 1, ">>", 32767),
            ("int32", 4, 1, ">>", 2),
            ("int32", -1, 1, ">>", -1),
            ("uint32", 0xFFFFFFFF, 1, ">>", 2147483647),
            ("int64", 4, 1, ">>", 2),
            ("int64", -1, 1, ">>", -1),
            ("uint64", 0xFFFFFFFFFFFFFFFF, 1, ">>", 9223372036854775807),
        ]
        for type, x, y, op, res in tests:
            codestr = f"""
            from __static__ import {type}, box
            def testfunc(tst):
                x: {type} = {x}
                y: {type} = {y}
                if tst:
                    x = x + 1
                    y = y + 2

                z: {type} = x {op} y
                return box(z), box(x {op} y)
            """
            self.compile(codestr, StaticCodeGenerator)
            f = self.run_code(codestr, StaticCodeGenerator)["testfunc"]
            self.assertEqual(f(False), (res, res), f"{type} {x} {op} {y} {res}")

    def test_int_compare_mixed_sign(self):
        tests = [
            ("uint16", 10000, "int16", -1, "<", False),
            ("uint16", 10000, "int16", -1, "<=", False),
            ("int16", -1, "uint16", 10000, ">", False),
            ("int16", -1, "uint16", 10000, ">=", False),
            ("uint32", 10000, "int16", -1, "<", False),
        ]
        for type1, x, type2, y, op, res in tests:
            codestr = f"""
            from __static__ import {type1}, {type2}, box
            def testfunc(tst):
                x: {type1} = {x}
                y: {type2} = {y}
                if tst:
                    x = x + 1
                    y = y + 2

                if x {op} y:
                    return True
                return False
            """
            self.compile(codestr, StaticCodeGenerator)
            f = self.run_code(codestr, StaticCodeGenerator)["testfunc"]
            self.assertEqual(f(False), res, f"{type} {x} {op} {y} {res}")

    def test_int_compare64_mixed_sign(self):
        codestr = """
            from __static__ import uint64, int64
            def testfunc(tst):
                x: uint64 = 0
                y: int64 = 1
                if tst:
                    x = x + 1
                    y = y + 2

                if x < y:
                    return True
                return False
        """
        with self.assertRaises(TypedSyntaxError):
            self.compile(codestr, StaticCodeGenerator)

    def test_compile_method(self):
        code = self.compile(
            """
        from __static__ import size_t
        def f():
            x: size_t = 42
        """,
            StaticCodeGenerator,
        )

        f = self.find_code(code)
        self.assertInBytecode(f, "INT_LOAD_CONST", 42)

    def test_mixed_compare(self):
        codestr = """
        from __static__ import size_t, box, unbox
        def f(a):
            x: size_t = 0
            while x != a:
                pass
        """
        with self.assertRaisesRegex(TypedSyntaxError, "can't compare int64 to dynamic"):
            self.compile(codestr, StaticCodeGenerator)

    def test_unbox(self):
        codestr = """
        from __static__ import size_t, box, unbox
        def f(x):
            y: size_t = unbox(x)
            y = y + 1
            return box(y)
        """

        code = self.compile(codestr, StaticCodeGenerator)
        f = self.find_code(code)
        f = self.run_code(codestr, StaticCodeGenerator)["f"]
        self.assertEqual(f(41), 42)

    def test_int_loop_inplace(self):
        codestr = """
        from __static__ import size_t, box
        def f():
            i: size_t = 0
            while i < 100:
                i += 1
            return box(i)
        """

        code = self.compile(codestr, StaticCodeGenerator)
        f = self.find_code(code)
        f = self.run_code(codestr, StaticCodeGenerator)["f"]
        self.assertEqual(f(), 100)

    def test_int_loop(self):
        codestr = """
        from __static__ import size_t, box
        def testfunc():
            i: size_t = 0
            while i < 100:
                i = i + 1
            return box(i)
        """

        code = self.compile(codestr, StaticCodeGenerator)
        f = self.find_code(code)

        f = self.run_code(codestr, StaticCodeGenerator)["testfunc"]
        self.assertEqual(f(), 100)

        self.assertInBytecode(f, "INT_LOAD_CONST", 0)
        self.assertInBytecode(f, "LOAD_LOCAL", (0, ("__static__", "int64")))
        self.assertInBytecode(f, "INT_BINARY_OP", PRIM_OP_ADD)
        self.assertInBytecode(f, "INT_COMPARE_OP", PRIM_OP_LT)
        self.assertInBytecode(f, "POP_JUMP_IF_ZERO")

    def test_int_assert(self):
        codestr = """
        from __static__ import size_t, box
        def testfunc():
            i: size_t = 0
            assert i == 0, "hello there"
        """

        code = self.compile(codestr, StaticCodeGenerator)
        f = self.find_code(code)
        self.assertInBytecode(f, "POP_JUMP_IF_NONZERO")

        f = self.run_code(codestr, StaticCodeGenerator)["testfunc"]
        self.assertEqual(f(), None)

    def test_int_assert_raises(self):
        codestr = """
        from __static__ import size_t, box
        def testfunc():
            i: size_t = 0
            assert i != 0, "hello there"
        """

        code = self.compile(codestr, StaticCodeGenerator)
        f = self.find_code(code)
        self.assertInBytecode(f, "POP_JUMP_IF_NONZERO")

        with self.assertRaises(AssertionError):
            f = self.run_code(codestr, StaticCodeGenerator)["testfunc"]
            self.assertEqual(f(), None)

    def test_int_loop_reversed(self):
        codestr = """
        from __static__ import size_t, box
        def testfunc():
            i: size_t = 0
            while 100 > i:
                i = i + 1
            return box(i)
        """

        code = self.compile(codestr, StaticCodeGenerator)
        f = self.find_code(code)
        f = self.run_code(codestr, StaticCodeGenerator)["testfunc"]
        self.assertEqual(f(), 100)

        self.assertInBytecode(f, "INT_LOAD_CONST", 0)
        self.assertInBytecode(f, "LOAD_LOCAL", (0, ("__static__", "int64")))
        self.assertInBytecode(f, "INT_BINARY_OP", PRIM_OP_ADD)
        self.assertInBytecode(f, "INT_COMPARE_OP", PRIM_OP_GT)
        self.assertInBytecode(f, "POP_JUMP_IF_ZERO")

    def test_int_loop_chained(self):
        codestr = """
        from __static__ import size_t, box
        def testfunc():
            i: size_t = 0
            while -1 < i < 100:
                i = i + 1
            return box(i)
        """

        code = self.compile(codestr, StaticCodeGenerator)
        f = self.find_code(code)
        f = self.run_code(codestr, StaticCodeGenerator)["testfunc"]
        self.assertEqual(f(), 100)

        self.assertInBytecode(f, "INT_LOAD_CONST", 0)
        self.assertInBytecode(f, "LOAD_LOCAL", (0, ("__static__", "int64")))
        self.assertInBytecode(f, "INT_BINARY_OP", PRIM_OP_ADD)
        self.assertInBytecode(f, "INT_COMPARE_OP", PRIM_OP_LT)
        self.assertInBytecode(f, "POP_JUMP_IF_ZERO")

    def test_compare_subclass(self):
        codestr = """
        class C: pass
        class D(C): pass

        x = C() > D()
        """
        code = self.compile(codestr, StaticCodeGenerator)
        self.assertInBytecode(code, "COMPARE_OP")

    def test_compat_int_math(self):
        codestr = """
        from __static__ import size_t, box
        def f():
            x: size_t = 42
            z: size_t = 1 + x
            return box(z)
        """

        code = self.compile(codestr, StaticCodeGenerator)
        f = self.find_code(code)
        self.assertInBytecode(f, "INT_LOAD_CONST", 42)
        self.assertInBytecode(f, "LOAD_LOCAL", (0, ("__static__", "int64")))
        self.assertInBytecode(f, "INT_BINARY_OP", PRIM_OP_ADD)
        f = self.run_code(codestr, StaticCodeGenerator)["f"]
        self.assertEqual(f(), 43)

    def test_unbox_long(self):
        codestr = """
        from __static__ import unbox, int64
        def f():
            x:int64 = unbox(1)
        """

        self.compile(codestr, StaticCodeGenerator)

    def test_uninit_value(self):
        codestr = """
        from __static__ import box, int64
        def f():
            x:int64
            return box(x)
            x = 0
        """
        f = self.run_code(codestr, StaticCodeGenerator)["f"]
        self.assertEqual(f(), 0)

    def test_uninit_value_2(self):
        codestr = """
        from __static__ import box, int64
        def testfunc(x):
            if x:
                y:int64 = 42
            return box(y)
        """
        f = self.run_code(codestr, StaticCodeGenerator)["testfunc"]
        self.assertEqual(f(False), 0)

    def test_bad_box(self):
        codestr = """
        from __static__ import box
        box('abc')
        """

        with self.assertRaisesRegex(
            TypedSyntaxError, "can't box non-primitive: builtins.str"
        ):
            self.compile(codestr, StaticCodeGenerator)

    def test_bad_unbox(self):
        codestr = """
        from __static__ import unbox, int64
        def f():
            x:int64 = 42
            unbox(x)
        """

        with self.assertRaisesRegex(
            TypedSyntaxError, type_mismatch("int64", "dynamic")
        ):
            self.compile(codestr, StaticCodeGenerator)

    def test_bad_box_2(self):
        codestr = """
        from __static__ import box
        box('abc', 'foo')
        """

        with self.assertRaisesRegex(
            TypedSyntaxError, "box only accepts a single argument"
        ):
            self.compile(codestr, StaticCodeGenerator)

    def test_bad_unbox_2(self):
        codestr = """
        from __static__ import unbox, int64
        def f():
            x:int64 = 42
            unbox(x, y)
        """

        with self.assertRaisesRegex(
            TypedSyntaxError, "unbox only accepts a single argument"
        ):
            self.compile(codestr, StaticCodeGenerator)

    def test_int_reassign(self):
        codestr = """
        from __static__ import size_t, box
        def f():
            x: size_t = 42
            z: size_t = 1 + x
            x = 100
            x = x + x
            return box(z)
        """

        code = self.compile(codestr, StaticCodeGenerator)
        f = self.find_code(code)
        self.assertInBytecode(f, "INT_LOAD_CONST", 42)
        self.assertInBytecode(f, "LOAD_LOCAL", (0, ("__static__", "int64")))
        self.assertInBytecode(f, "INT_BINARY_OP", PRIM_OP_ADD)
        f = self.run_code(codestr, StaticCodeGenerator)["f"]
        self.assertEqual(f(), 43)

    def test_global_call_add(self) -> None:
        codestr = """
            X = ord(42)
            def f():
                y = X + 1
        """
        self.compile(codestr, StaticCodeGenerator, modname="foo")

    def test_type_binder(self) -> None:
        self.assertEqual(self.bind_expr("42"), INT_TYPE.instance)
        self.assertEqual(self.bind_expr("42.0"), FLOAT_TYPE.instance)
        self.assertEqual(self.bind_expr("'abc'"), STR_TYPE.instance)
        self.assertEqual(self.bind_expr("b'abc'"), BYTES_TYPE.instance)
        self.assertEqual(self.bind_expr("3j"), COMPLEX_TYPE.instance)
        self.assertEqual(self.bind_expr("None"), NONE_TYPE.instance)
        self.assertEqual(self.bind_expr("True"), BOOL_TYPE.instance)
        self.assertEqual(self.bind_expr("False"), BOOL_TYPE.instance)
        self.assertEqual(self.bind_expr("..."), DYNAMIC)
        self.assertEqual(self.bind_expr("f''"), STR_TYPE.instance)
        self.assertEqual(self.bind_expr("f'{x}'"), STR_TYPE.instance)

        self.assertEqual(self.bind_expr("a"), DYNAMIC)
        self.assertEqual(self.bind_expr("a.b"), DYNAMIC)
        self.assertEqual(self.bind_expr("a + b"), DYNAMIC)

        self.assertEqual(self.bind_stmt("x = 1"), INT_TYPE.instance)
        # self.assertEqual(self.bind_stmt("x: foo = 1").target.comp_type, DYNAMIC)
        self.assertEqual(self.bind_stmt("x += 1"), DYNAMIC)
        self.assertEqual(self.bind_expr("a or b"), DYNAMIC)
        self.assertEqual(self.bind_expr("+a"), DYNAMIC)
        self.assertEqual(self.bind_expr("not a"), BOOL_TYPE.instance)
        self.assertEqual(self.bind_expr("lambda: 42"), DYNAMIC)
        self.assertEqual(self.bind_expr("a if b else c"), DYNAMIC)
        self.assertEqual(self.bind_expr("x > y"), DYNAMIC)
        self.assertEqual(self.bind_expr("x()"), DYNAMIC)
        self.assertEqual(self.bind_expr("x(y)"), DYNAMIC)
        self.assertEqual(self.bind_expr("x[y]"), DYNAMIC)
        self.assertEqual(self.bind_expr("x[1:2]"), DYNAMIC)
        self.assertEqual(self.bind_expr("x[1:2:3]"), DYNAMIC)
        self.assertEqual(self.bind_expr("x[:]"), DYNAMIC)
        self.assertEqual(self.bind_expr("{}"), DICT_TYPE.instance)
        self.assertEqual(self.bind_expr("{2:3}"), DICT_TYPE.instance)
        self.assertEqual(self.bind_expr("{1,2}"), DYNAMIC)
        self.assertEqual(self.bind_expr("[]"), DYNAMIC)
        self.assertEqual(self.bind_expr("[1,2]"), DYNAMIC)
        self.assertEqual(self.bind_expr("(1,2)"), DYNAMIC)

        self.assertEqual(self.bind_expr("[x for x in y]"), DYNAMIC)
        self.assertEqual(self.bind_expr("{x for x in y}"), DYNAMIC)
        self.assertEqual(self.bind_expr("{x:y for x in y}"), DICT_TYPE.instance)
        self.assertEqual(self.bind_expr("(x for x in y)"), DYNAMIC)

        def body_get(stmt):
            return stmt.body[0].value

        self.assertEqual(
            self.bind_stmt("def f(): return 42", getter=body_get), INT_TYPE.instance
        )
        self.assertEqual(self.bind_stmt("def f(): yield 42", getter=body_get), DYNAMIC)
        self.assertEqual(
            self.bind_stmt("def f(): yield from x", getter=body_get), DYNAMIC
        )
        self.assertEqual(
            self.bind_stmt("async def f(): await x", getter=body_get), DYNAMIC
        )

        self.assertEqual(self.bind_expr("object"), OBJECT_TYPE)

        self.assertEqual(self.bind_expr("1 + 2", optimize=True), DYNAMIC)

    def test_if_exp(self) -> None:
        mod, syms = self.bind_module(
            """
            class C: pass
            class D: pass

            x = C() if a else D()
        """
        )
        node = mod.body[-1]
        types = syms.modules["foo"].types
        self.assertEqual(types[node], DYNAMIC)

        mod, syms = self.bind_module(
            """
            class C: pass

            x = C() if a else C()
        """
        )
        node = mod.body[-1]
        types = syms.modules["foo"].types
        self.assertEqual(types[node.value].name, "foo.C instance")

    def test_cmpop(self):
        codestr = """
            from __static__ import int32
            def f():
                i: int32 = 0
                j: int = 0

                if i == 0:
                    return 0
                if j == 0:
                    return 1
        """
        code = self.compile(codestr, StaticCodeGenerator, modname="foo")
        x = self.find_code(code, "f")
        self.assertInBytecode(x, "INT_COMPARE_OP", 0)
        self.assertInBytecode(x, "COMPARE_OP", "==")

    def test_bind_instance(self) -> None:
        mod, syms = self.bind_module("class C: pass\na: C = C()")
        assign = mod.body[1]
        types = syms.modules["foo"].types
        self.assertEqual(types[assign.target].name, "foo.C instance")
        self.assertEqual(repr(types[assign.target]), "<foo.C instance>")

    def test_bind_func_def(self) -> None:
        mod, syms = self.bind_module(
            """
            def f(x: object = None, y: object = None):
                pass
        """
        )
        modtable = syms.modules["foo"]
        self.assertTrue(isinstance(modtable.children["f"], Function))

    def bind_stmt(
        self, code: str, optimize: bool = False, getter=lambda stmt: stmt
    ) -> ast.stmt:
        mod, syms = self.bind_module(code, optimize)
        assert len(mod.body) == 1
        types = syms.modules["foo"].types
        return types[getter(mod.body[0])]

    def bind_expr(self, code: str, optimize: bool = False) -> Value:
        mod, syms = self.bind_module(code, optimize)
        assert len(mod.body) == 1
        types = syms.modules["foo"].types
        return types[mod.body[0].value]

    def bind_module(
        self, code: str, optimize: bool = False
    ) -> Tuple[ast.Module, SymbolTable]:
        tree = ast.parse(dedent(code))
        if optimize:
            tree = AstOptimizer().visit(tree)

        symtable = SymbolTable()
        decl_visit = DeclarationVisitor("foo", "foo.py", symtable)
        decl_visit.visit(tree)
        decl_visit.module.finish_bind()

        s = SymbolVisitor()
        walk(tree, s)

        type_binder = TypeBinder(s, "foo.py", symtable, "foo")
        type_binder.visit(tree)

        # Make sure we can compile the code, just verifying all nodes are
        # visited.
        graph = StaticCodeGenerator.flow_graph("foo", "foo.py", s.scopes[tree])
        code_gen = StaticCodeGenerator(None, tree, s, graph, symtable, "foo", optimize)
        code_gen.visit(tree)

        return tree, symtable

    def test_cross_module(self) -> None:
        acode = """
            class C:
                def f(self):
                    return 42
        """
        bcode = """
            from a import C

            def f():
                x = C()
                return x.f()
        """
        symtable = SymbolTable()
        symtable.add_module("a", "a.py", ast.parse(dedent(acode)))
        symtable.add_module("b", "b.py", ast.parse(dedent(bcode)))
        bcomp = symtable.compile("b", "b.py", ast.parse(dedent(bcode)))
        x = self.find_code(bcomp, "f")
        self.assertInBytecode(x, "INVOKE_METHOD", (("a", "C", "f"), 0))

    def test_pseudo_strict_module(self) -> None:
        # simulate strict modules where the builtins come from <builtins>
        code = """
            def f(a):
                x: bool = a
        """
        builtins = ast.Assign(
            [ast.Name("bool", ast.Store())],
            ast.Subscript(
                ast.Name("<builtins>", ast.Load()),
                ast.Index(ast.Str("bool")),
                ast.Load(),
            ),
        )
        tree = ast.parse(dedent(code))
        tree.body.insert(0, builtins)

        symtable = SymbolTable()
        symtable.add_module("a", "a.py", tree)
        acomp = symtable.compile("a", "a.py", tree)
        x = self.find_code(acomp, "f")
        self.assertInBytecode(x, "CAST", ("builtins", "bool"))

    def test_cross_module_inheritance(self) -> None:
        acode = """
            class C:
                def f(self):
                    return 42
        """
        bcode = """
            from a import C

            class D(C):
                def f(self):
                    return 'abc'

            def f(y):
                x: C
                if y:
                    x = D()
                else:
                    x = C()
                return x.f()
        """
        symtable = SymbolTable()
        symtable.add_module("a", "a.py", ast.parse(dedent(acode)))
        symtable.add_module("b", "b.py", ast.parse(dedent(bcode)))
        bcomp = symtable.compile("b", "b.py", ast.parse(dedent(bcode)))
        x = self.find_code(bcomp, "f")
        self.assertInBytecode(x, "INVOKE_METHOD", (("a", "C", "f"), 0))

    @skipIf(not HAS_INVOKE_METHOD, "no invoke method support")
    def test_annotated_function(self):
        codestr = """
        class C:
            def f(self) -> int:
                return 1

        def x(c: C):
            x = c.f()
            x += c.f()
            return x
        """

        code = self.compile(codestr, StaticCodeGenerator, modname="foo")
        x = self.find_code(code, "x")
        self.assertInBytecode(x, "INVOKE_METHOD", (("foo", "C", "f"), 0))

        with self.in_module(codestr) as mod:
            x, C = mod["x"], mod["C"]
            c = C()
            self.assertEqual(x(c), 2)

    @skipIf(not HAS_INVOKE_METHOD, "no invoke method support")
    def test_invoke_new_derived(self):
        codestr = """
            class C:
                def f(self):
                    return 1

            def x(c: C):
                x = c.f()
                x += c.f()
                return x

            a = x(C())

            class D(C):
                def f(self):
                    return 2

            b = x(D())
        """

        code = self.compile(codestr, StaticCodeGenerator, modname="foo")
        x = self.find_code(code, "x")
        self.assertInBytecode(x, "INVOKE_METHOD", (("foo", "C", "f"), 0))

        with self.in_module(codestr) as mod:
            a, b = mod["a"], mod["b"]
            self.assertEqual(a, 2)
            self.assertEqual(b, 4)

    @skipIf(not HAS_INVOKE_METHOD, "no invoke method support")
    def test_invoke_explicit_slots(self):
        codestr = """
            class C:
                __slots__ = ()
                def f(self):
                    return 1

            def x(c: C):
                x = c.f()
                x += c.f()
                return x

            a = x(C())
        """

        code = self.compile(codestr, StaticCodeGenerator, modname="foo")
        x = self.find_code(code, "x")
        self.assertInBytecode(x, "INVOKE_METHOD", (("foo", "C", "f"), 0))

        with self.in_module(codestr) as mod:
            a = mod["a"]
            self.assertEqual(a, 2)

    @skipIf(not HAS_INVOKE_METHOD, "no invoke method support")
    def test_invoke_new_derived_nonfunc(self):
        codestr = """
            class C:
                def f(self):
                    return 1

            def x(c: C):
                x = c.f()
                x += c.f()
                return x
        """

        code = self.compile(codestr, StaticCodeGenerator, modname="foo")
        x = self.find_code(code, "x")
        self.assertInBytecode(x, "INVOKE_METHOD", (("foo", "C", "f"), 0))

        with self.in_module(codestr) as mod:
            x, C = mod["x"], mod["C"]
            self.assertEqual(x(C()), 2)

            class Callable:
                def __call__(self_, obj):  # noqa: B902
                    self.assertTrue(isinstance(obj, D))
                    return 42

            class D(C):
                f = Callable()

            d = D()
            self.assertEqual(x(d), 84)

    @skipIf(not HAS_INVOKE_METHOD, "no invoke method support")
    def test_invoke_new_derived_nonfunc_descriptor(self):
        codestr = """
            class C:
                def f(self):
                    return 1

            def x(c: C):
                x = c.f()
                x += c.f()
                return x
        """

        code = self.compile(codestr, StaticCodeGenerator, modname="foo")
        x = self.find_code(code, "x")
        self.assertInBytecode(x, "INVOKE_METHOD", (("foo", "C", "f"), 0))

        with self.in_module(codestr) as mod:
            x, C = mod["x"], mod["C"]
            self.assertEqual(x(C()), 2)

            class Callable:
                def __call__(self):
                    return 42

            class Descr:
                def __get__(self, inst, ctx):
                    return Callable()

            class D(C):
                f = Descr()

            d = D()
            self.assertEqual(x(d), 84)

    @skipIf(not HAS_INVOKE_METHOD, "no invoke method support")
    def test_invoke_new_derived_nonfunc_data_descriptor(self):
        codestr = """
            class C:
                def f(self):
                    return 1

            def x(c: C):
                x = c.f()
                x += c.f()
                return x
        """

        code = self.compile(codestr, StaticCodeGenerator, modname="foo")
        x = self.find_code(code, "x")
        self.assertInBytecode(x, "INVOKE_METHOD", (("foo", "C", "f"), 0))

        with self.in_module(codestr) as mod:
            x, C = mod["x"], mod["C"]
            self.assertEqual(x(C()), 2)

            class Callable:
                def __call__(self):
                    return 42

            class Descr:
                def __get__(self, inst, ctx):
                    return Callable()

                def __set__(self, inst, value):
                    raise ValueError("no way")

            class D(C):
                f = Descr()

            d = D()
            self.assertEqual(x(d), 84)

    @skipIf(not HAS_INVOKE_METHOD, "no invoke method support")
    def test_invoke_new_derived_nonfunc_descriptor_inst_override(self):
        codestr = """
            class C:
                def f(self):
                    return 1

            def x(c: C):
                x = c.f()
                x += c.f()
                return x
        """

        code = self.compile(codestr, StaticCodeGenerator, modname="foo")
        x = self.find_code(code, "x")
        self.assertInBytecode(x, "INVOKE_METHOD", (("foo", "C", "f"), 0))

        with self.in_module(codestr) as mod:
            x, C = mod["x"], mod["C"]
            self.assertEqual(x(C()), 2)

            class Callable:
                def __call__(self):
                    return 42

            class Descr:
                def __get__(self, inst, ctx):
                    return Callable()

            class D(C):
                f = Descr()

            d = D()
            self.assertEqual(x(d), 84)
            d.__dict__["f"] = lambda x: 100
            self.assertEqual(x(d), 200)

    @skipIf(not HAS_INVOKE_METHOD, "no invoke method support")
    def test_invoke_new_derived_nonfunc_descriptor_modified(self):
        codestr = """
            class C:
                def f(self):
                    return 1

            def x(c: C):
                x = c.f()
                x += c.f()
                return x
        """

        code = self.compile(codestr, StaticCodeGenerator, modname="foo")
        x = self.find_code(code, "x")
        self.assertInBytecode(x, "INVOKE_METHOD", (("foo", "C", "f"), 0))

        with self.in_module(codestr) as mod:
            x, C = mod["x"], mod["C"]
            self.assertEqual(x(C()), 2)

            class Callable:
                def __call__(self):
                    return 42

            class Descr:
                def __get__(self, inst, ctx):
                    return Callable()

                def __call__(self, arg):
                    return 23

            class D(C):
                f = Descr()

            d = D()
            self.assertEqual(x(d), 84)
            del Descr.__get__
            self.assertEqual(x(d), 46)

    @skipIf(not HAS_INVOKE_METHOD, "no invoke method support")
    def test_invoke_dict_override(self):
        codestr = """
            class C:
                def f(self):
                    return 1

            def x(c: C):
                x = c.f()
                x += c.f()
                return x
        """

        code = self.compile(codestr, StaticCodeGenerator, modname="foo")
        x = self.find_code(code, "x")
        self.assertInBytecode(x, "INVOKE_METHOD", (("foo", "C", "f"), 0))

        with self.in_module(codestr) as mod:
            x, C = mod["x"], mod["C"]
            self.assertEqual(x(C()), 2)

            class D(C):
                def __init__(self):
                    self.f = lambda: 42

            d = D()
            self.assertEqual(x(d), 84)

    @skipIf(not HAS_INVOKE_METHOD, "no invoke method support")
    def test_invoke_type_modified(self):
        codestr = """
            class C:
                def f(self):
                    return 1

            def x(c: C):
                x = c.f()
                x += c.f()
                return x
        """

        code = self.compile(codestr, StaticCodeGenerator, modname="foo")
        x = self.find_code(code, "x")
        self.assertInBytecode(x, "INVOKE_METHOD", (("foo", "C", "f"), 0))

        with self.in_module(codestr) as mod:
            x, C = mod["x"], mod["C"]
            self.assertEqual(x(C()), 2)
            C.f = lambda self: 42
            self.assertEqual(x(C()), 84)

    @skipIf(not HAS_INVOKE_METHOD, "no invoke method support")
    def test_annotated_function_derived(self):
        codestr = """
            class C:
                def f(self) -> int:
                    return 1

            class D(C):
                def f(self) -> int:
                    return 2

            class E(C):
                pass

            def x(c: C,):
                x = c.f()
                x += c.f()
                return x
        """

        code = self.compile(
            codestr, StaticCodeGenerator, modname="test_annotated_function_derived"
        )
        x = self.find_code(code, "x")
        self.assertInBytecode(
            x, "INVOKE_METHOD", (("test_annotated_function_derived", "C", "f"), 0)
        )

        with self.in_module(codestr) as mod:
            x = mod["x"]
            self.assertEqual(x(mod["C"]()), 2)
            self.assertEqual(x(mod["D"]()), 4)
            self.assertEqual(x(mod["E"]()), 2)

    def test_error_incompat_assign_local(self):
        codestr = """
            class C:
                def __init__(self):
                    self.x = None

                def f(self):
                    x: "C" = self.x
        """
        with self.in_module(codestr) as mod:
            C = mod["C"]
            with self.assertRaisesRegex(TypeError, "expected 'C', got 'NoneType'"):
                C().f()

    def test_error_incompat_field_non_dynamic(self):
        codestr = """
            class C:
                def __init__(self):
                    self.x: int = 'abc'
        """
        with self.assertRaises(TypedSyntaxError):
            self.compile(codestr, StaticCodeGenerator)

    def test_error_incompat_field(self):
        codestr = """
            class C:
                def __init__(self):
                    self.x: int = 100

                def f(self, x):
                    self.x = x
        """
        with self.in_module(codestr) as mod:
            C = mod["C"]
            C().f(42)
            with self.assertRaises(TypeError):
                C().f("abc")

    def test_error_incompat_assign_dynamic(self):
        with self.assertRaises(TypedSyntaxError):
            self.compile(
                """
            class C:
                x: "C"
                def __init__(self):
                    self.x = None
            """,
                StaticCodeGenerator,
            )

    def test_annotated_class_var(self):
        codestr = """
            class C:
                x: int
        """
        self.compile(codestr, StaticCodeGenerator, modname="test_annotated_class_var")

    @skipIf(not HAS_FIELDS, "no field support")
    def test_annotated_instance_var(self):
        codestr = """
            class C:
                def __init__(self):
                    self.x: str = 'abc'
        """
        code = self.compile(
            codestr, StaticCodeGenerator, modname="test_annotated_instance_var"
        )
        # get C from module, and then get __init__ from C
        code = self.find_code(self.find_code(code))
        self.assertInBytecode(code, "STORE_FIELD")

    @skipIf(not HAS_FIELDS, "no field support")
    def test_load_store_attr_value(self):
        codestr = """
            class C:
                x: int

                def __init__(self, value: int):
                    self.x = value

                def f(self):
                    return self.x
        """
        code = self.compile(codestr, StaticCodeGenerator, modname="foo")
        init = self.find_code(self.find_code(code), "__init__")
        self.assertInBytecode(init, "STORE_FIELD")
        f = self.find_code(self.find_code(code), "f")
        self.assertInBytecode(f, "LOAD_FIELD")
        with self.in_module(codestr) as mod:
            C = mod["C"]
            a = C(42)
            self.assertEqual(a.f(), 42)

    @skipIf(not HAS_FIELDS, "no field support")
    def test_load_store_attr(self):
        codestr = """
            class C:
                x: "C"

                def __init__(self):
                    self.x = self

                def g(self):
                    return 42

                def f(self):
                    return self.x.g()
        """
        self.compile(codestr, StaticCodeGenerator, modname="foo")

        with self.in_module(codestr) as mod:
            C = mod["C"]
            a = C()
            self.assertEqual(a.f(), 42)

    @skipIf(not HAS_FIELDS, "no field support")
    def test_load_store_attr_init(self):
        codestr = """
            class C:
                def __init__(self):
                    self.x: C = self

                def g(self):
                    return 42

                def f(self):
                    return self.x.g()
        """
        self.compile(codestr, StaticCodeGenerator, modname="foo")

        with self.in_module(codestr) as mod:
            C = mod["C"]
            a = C()
            self.assertEqual(a.f(), 42)

    @skipIf(not HAS_FIELDS, "no field support")
    def test_load_store_attr_init_no_ann(self):
        codestr = """
            class C:
                def __init__(self):
                    self.x = self

                def g(self):
                    return 42

                def f(self):
                    return self.x.g()
        """
        self.compile(codestr, StaticCodeGenerator, modname="foo")

        with self.in_module(codestr) as mod:
            C = mod["C"]
            a = C()
            self.assertEqual(a.f(), 42)

    def test_unknown_annotation(self):
        codestr = """
            def f(a):
                x: foo = a
                return x.bar
        """
        self.compile(codestr, StaticCodeGenerator, modname="foo")

        class C:
            bar = 42

        f = self.run_code(codestr, StaticCodeGenerator)["f"]
        self.assertEqual(f(C()), 42)

    @skipIf(not HAS_FIELDS, "no field support")
    def test_class_method_invoke(self):
        codestr = """
            class B:
                def __init__(self, value):
                    self.value = value

            class D(B):
                def __init__(self, value):
                    B.__init__(self, value)

                def f(self):
                    return self.value
        """
        code = self.compile(codestr, StaticCodeGenerator, modname="foo")

        b_init = self.find_code(self.find_code(code, "B"), "__init__")
        self.assertInBytecode(b_init, "STORE_FIELD", ("foo", "B", "value"))

        f = self.find_code(self.find_code(code, "D"), "f")
        self.assertInBytecode(f, "LOAD_FIELD", ("foo", "B", "value"))

        with self.in_module(codestr) as mod:
            D = mod["D"]
            d = D(42)
            self.assertEqual(d.f(), 42)

    @skipIf(not HAS_FIELDS, "no field support")
    def test_slotification(self):
        codestr = """
            class C:
                x: "unknown_type"
        """
        self.compile(codestr, StaticCodeGenerator, modname="foo")
        C = self.run_code(codestr, StaticCodeGenerator)["C"]
        self.assertEqual(type(C.x), MemberDescriptorType)

    @skipIf(not HAS_FIELDS, "no field support")
    def test_slotification_init(self):
        codestr = """
            class C:
                x: "unknown_type"
                def __init__(self):
                    self.x = 42
        """
        self.compile(codestr, StaticCodeGenerator, modname="foo")
        C = self.run_code(codestr, StaticCodeGenerator)["C"]
        self.assertEqual(type(C.x), MemberDescriptorType)

    @skipIf(not HAS_FIELDS, "no field support")
    def test_slotification_ann_init(self):
        codestr = """
            class C:
                x: "unknown_type"
                def __init__(self):
                    self.x: "unknown_type" = 42
        """
        self.compile(codestr, StaticCodeGenerator, modname="foo")
        C = self.run_code(codestr, StaticCodeGenerator)["C"]
        self.assertEqual(type(C.x), MemberDescriptorType)

    @skipIf(not HAS_FIELDS, "no field support")
    def test_slotification_typed(self):
        codestr = """
            class C:
                x: int
        """
        C = self.run_code(codestr, StaticCodeGenerator)["C"]
        self.assertNotEqual(type(C.x), MemberDescriptorType)

    @skipIf(not HAS_FIELDS, "no field support")
    def test_slotification_init_typed(self):
        codestr = """
            class C:
                x: int
                def __init__(self):
                    self.x = 42
        """
        with self.in_module(codestr) as mod:
            C = mod["C"]
            self.assertNotEqual(type(C.x), MemberDescriptorType)
            x = C()
            self.assertEqual(x.x, 42)
            with self.assertRaisesRegex(
                TypeError, "expected 'int', got 'str' for attribute 'x'"
            ):
                x.x = "abc"

    @skipIf(not HAS_FIELDS, "no field support")
    def test_slotification_ann_init_typed(self):
        codestr = """
            class C:
                x: int
                def __init__(self):
                    self.x: int = 42
        """
        C = self.run_code(codestr, StaticCodeGenerator)["C"]
        self.assertNotEqual(type(C.x), MemberDescriptorType)

    @skipIf(not HAS_FIELDS, "no field support")
    def test_slotification_conflicting_types(self):
        codestr = """
            class C:
                x: object
                def __init__(self):
                    self.x: int = 42
        """
        with self.assertRaisesRegex(
            TypedSyntaxError, "conflicting type definitions for slot x in class foo.C"
        ):
            self.compile(codestr, StaticCodeGenerator, modname="foo")

    @skipIf(not HAS_FIELDS, "no field support")
    def test_slotification_conflicting_members(self):
        codestr = """
            class C:
                def x(self): pass
                x: object
        """
        with self.assertRaisesRegex(
            TypedSyntaxError, "slot conflicts with other member x in class foo.C"
        ):
            self.compile(codestr, StaticCodeGenerator, modname="foo")

    @skipIf(not HAS_FIELDS, "no field support")
    def test_slotification_conflicting_function(self):
        codestr = """
            class C:
                x: object
                def x(self): pass
        """
        with self.assertRaisesRegex(
            TypedSyntaxError, "function conflicts with other member x in class foo.C"
        ):
            self.compile(codestr, StaticCodeGenerator, modname="foo")

    @skipIf(not HAS_FIELDS, "no field support")
    def test_slot_inheritance(self):
        codestr = """
            class B:
                def __init__(self):
                    self.x = 42

                def f(self):
                    return self.x

            class D(B):
                def __init__(self):
                    self.x = 100
        """
        self.compile(codestr, StaticCodeGenerator, modname="foo")
        with self.in_module(codestr) as mod:
            D = mod["D"]
            inst = D()
            self.assertEqual(inst.f(), 100)

    def test_del_slot(self):
        codestr = """
        class C:
            x: object

        def f(a: C):
            del a.x
        """
        code = self.compile(codestr, StaticCodeGenerator, modname="foo")
        code = self.find_code(code, name="f")
        self.assertInBytecode(code, "DELETE_ATTR", "x")

    def test_uninit_slot(self):
        codestr = """
        class C:
            x: object

        def f(a: C):
            return a.x
        """
        code = self.compile(codestr, StaticCodeGenerator, modname="foo")
        code = self.find_code(code, name="f")
        with self.in_module(codestr) as mod:
            with self.assertRaises(AttributeError) as e:
                f, C = mod["f"], mod["C"]
                f(C())

        self.assertEqual(e.exception.args[0], "x")

    @skipIf(not HAS_FIELDS, "no field support")
    def test_aug_add(self):
        codestr = """
        class C:
            def __init__(self):
                self.x = 1

        def f(a: C):
            a.x += 1
        """
        code = self.compile(codestr, StaticCodeGenerator, modname="foo")
        code = self.find_code(code, name="f")
        self.assertInBytecode(code, "LOAD_FIELD", ("foo", "C", "x"))
        self.assertInBytecode(code, "STORE_FIELD", ("foo", "C", "x"))

    def test_untyped_attr(self):
        codestr = """
        y = x.load
        x.store = 42
        del x.delete
        """
        code = self.compile(codestr, StaticCodeGenerator, modname="foo")
        self.assertInBytecode(code, "LOAD_ATTR", "load")
        self.assertInBytecode(code, "STORE_ATTR", "store")
        self.assertInBytecode(code, "DELETE_ATTR", "delete")

    def test_incompat_override(self):
        codestr = """
        class C:
            x: int

        class D(C):
            def x(self): pass
        """
        with self.assertRaises(TypedSyntaxError):
            self.compile(codestr, StaticCodeGenerator, modname="foo")

    def test_redefine_type(self):
        codestr = """
            class C: pass
            class D: pass

            def f(a):
                x: C = C()
                x: D = D()
        """
        with self.assertRaises(TypedSyntaxError):
            self.compile(codestr, StaticCodeGenerator, modname="foo")

    def test_optional_type(self):
        codestr = """
            from typing import Optional
            class C:
                x: Optional["C"]
                def __init__(self, set):
                    if set:
                        self.x = self
                    else:
                        self.x = None

                def f(self) -> Optional["C"]:
                    return self.x.x
        """
        code = self.compile(codestr, StaticCodeGenerator, modname="foo")
        c_f = self.find_code(self.find_code(code, "C"), "f")

        self.assertInBytecode(c_f, "RAISE_IF_NONE", "x")
        self.assertInBytecode(c_f, "LOAD_FIELD", ("foo", "C", "x"))
        c_init = self.find_code(self.find_code(code, "C"), "__init__")
        self.assertInBytecode(c_init, "STORE_FIELD", ("foo", "C", "x"))

        with self.in_module(codestr) as mod:
            C = mod["C"]
            c = C(True)
            self.assertEqual(c.f(), c)
            c = C(False)
            with self.assertRaises(AttributeError):
                c.f()

    def test_optional_assign(self):
        codestr = """
            from typing import Optional
            class C:
                def f(self, x: Optional["C"]):
                    if x is None:
                        return self
                    else:
                        p: Optional["C"] = x
        """
        code = self.compile(codestr, StaticCodeGenerator, modname="foo")
        f = self.find_code(self.find_code(code, "C"), "f")
        self.assertNotInBytecode(f, "RAISE_IF_NONE")

    def test_nonoptional_load(self):
        codestr = """
            from typing import Optional
            class C:
                x: C
                def __init__(self):
                    self.f()

                def f(self):
                    pass
        """
        code = self.compile(codestr, StaticCodeGenerator, modname="foo")
        f = self.find_code(self.find_code(code, "C"), "__init__")
        self.assertNotInBytecode(f, "RAISE_IF_NONE")

    def test_optional_assign_subclass(self):
        codestr = """
            from typing import Optional
            class B: pass
            class D(B): pass

            def f(x: D):
                a: Optional[B] = x
        """
        code = self.compile(codestr, StaticCodeGenerator, modname="foo")
        f = self.find_code(code, "f")
        self.assertNotInBytecode(f, "RAISE_IF_NONE")

    def test_optional_assign_subclass_opt(self):
        codestr = """
            from typing import Optional
            class B: pass
            class D(B): pass

            def f(x: Optional[D]):
                a: Optional[B] = x
        """
        code = self.compile(codestr, StaticCodeGenerator, modname="foo")
        f = self.find_code(code, "f")
        self.assertNotInBytecode(f, "RAISE_IF_NONE")

    def test_optional_assign_none(self):
        codestr = """
            from typing import Optional
            class B: pass

            def f(x: Optional[B]):
                a: Optional[B] = None
        """
        code = self.compile(codestr, StaticCodeGenerator, modname="foo")
        f = self.find_code(code, "f")
        self.assertNotInBytecode(f, "RAISE_IF_NONE")

    def test_return_int_boxed(self):
        codestr = """
        from __static__ import size_t
        def f():
            y: size_t = 1
            return y
        """
        with self.in_module(codestr) as mod:
            f = mod["f"]
            self.assertEqual(f(), 1)

    def test_error_mixed_math(self):
        with self.assertRaises(TypedSyntaxError):
            self.compile(
                """
            from __static__ import size_t
            def f():
                y = 1
                x: size_t = 42 + y
            """,
                StaticCodeGenerator,
            )

    def test_error_incompat_return(self):
        codestr = """
        class D: pass
        class C:
            def __init__(self):
                self.x = None

            def f(self) -> "C":
                return D()
        """
        with self.in_module(codestr) as mod:
            C = mod["C"]
            with self.assertRaises(TypeError):
                C().f()

    def assert_jitted(self, func):
        if cinderjit is None:
            return

        self.assertTrue(cinderjit.is_jit_compiled(func))

    @skipIf(not HAS_CAST, "no cast")
    def test_cast(self):
        for code_gen in (StaticCodeGenerator, Python37CodeGenerator):
            codestr = """
                from __static__ import cast
                class C:
                    pass

                a = C()

                def f() -> C:
                    return cast(C, a)
            """
            code = self.compile(codestr, code_gen)
            f = self.find_code(code, "f")
            if code_gen is StaticCodeGenerator:
                self.assertInBytecode(f, "CAST", ("<module>", "C"))
            with self.in_module(codestr) as mod:
                C = mod["C"]
                f = mod["f"]
                self.assertTrue(isinstance(f(), C))
                self.assert_jitted(f)

    @skipIf(not HAS_CAST, "no cast")
    def test_cast_fail(self):
        for code_gen in (StaticCodeGenerator, Python37CodeGenerator):
            codestr = """
                from __static__ import cast
                class C:
                    pass

                def f() -> C:
                    return cast(C, 42)
            """
            code = self.compile(codestr, code_gen)
            f = self.find_code(code, "f")
            if code_gen is StaticCodeGenerator:
                self.assertInBytecode(f, "CAST", ("<module>", "C"))
            with self.in_module(codestr) as mod:
                with self.assertRaises(TypeError):
                    f = mod["f"]
                    f()
                    self.assert_jitted(f)

    @skipIf(not HAS_CAST, "no cast")
    def test_cast_wrong_args(self):
        codestr = """
            from __static__ import cast
            def f():
                cast(42)
        """
        with self.assertRaises(TypedSyntaxError):
            self.compile(codestr, StaticCodeGenerator)

    @skipIf(not HAS_CAST, "no cast")
    def test_cast_unknown_type(self):
        codestr = """
            from __static__ import cast
            def f():
                cast(abc, 42)
        """
        with self.assertRaises(TypedSyntaxError):
            self.compile(codestr, StaticCodeGenerator)

    @skipIf(not HAS_CAST, "no cast")
    def test_cast_optional(self):
        for code_gen in (StaticCodeGenerator, Python37CodeGenerator):
            codestr = """
                from __static__ import cast
                from typing import Optional

                class C:
                    pass

                def f(x) -> Optional[C]:
                    return cast(Optional[C], x)
            """
            code = self.compile(codestr, code_gen)
            f = self.find_code(code, "f")
            if code_gen is StaticCodeGenerator:
                self.assertInBytecode(f, "CAST", ("<module>", "C", "?"))
            with self.in_module(codestr) as mod:
                C = mod["C"]
                f = mod["f"]
                self.assertTrue(isinstance(f(C()), C))
                self.assertEqual(f(None), None)
                self.assert_jitted(f)

    def test_code_flags(self):
        codestr = """
        def func():
            print("hi")

        func()
        """
        module = self.compile(codestr, StaticCodeGenerator)
        self.assertTrue(module.co_flags & consts.CO_STATICALLY_COMPILED)
        self.assertTrue(
            self.find_code(module, name="func").co_flags & consts.CO_STATICALLY_COMPILED
        )

    def test_invoke_kws(self):
        codestr = """
        class C:
            def f(self, a):
                return a

        def func():
            a = C()
            return a.f(a=2)

        """
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            f = mod["func"]
            self.assertEqual(f(), 2)

    def test_invoke_str_method(self):
        codestr = """
        def func():
            a = 'a b c'
            return a.split()

        """
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            f = mod["func"]
            self.assertInBytecode(f, "INVOKE_METHOD", (("builtins", "str", "split"), 0))
            self.assertEqual(f(), ["a", "b", "c"])

    def test_invoke_str_method_arg(self):
        codestr = """
        def func():
            a = 'a b c'
            return a.split('a')

        """
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            f = mod["func"]
            self.assertInBytecode(f, "INVOKE_METHOD", (("builtins", "str", "split"), 1))
            self.assertEqual(f(), ["", " b c"])

    def test_invoke_str_method_kwarg(self):
        codestr = """
        def func():
            a = 'a b c'
            return a.split(sep='a')

        """
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            f = mod["func"]
            self.assertNotInBytecode(f, "INVOKE_METHOD")
            self.assertEqual(f(), ["", " b c"])

    def test_invoke_int_method(self):
        codestr = """
        def func():
            a = 42
            return a.bit_length()

        """
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            f = mod["func"]
            self.assertInBytecode(
                f, "INVOKE_METHOD", (("builtins", "int", "bit_length"), 0)
            )
            self.assertEqual(f(), 6)

    def test_verify_positional_args(self):
        codestr = """
            def x(a: int, b: str) -> None:
                pass
            x("a", 2)
        """
        with self.assertRaisesRegex(TypedSyntaxError, "argument type mismatch"):
            self.compile(codestr, StaticCodeGenerator)

    def test_verify_positional_args_unordered(self):
        codestr = """
            def x(a: int, b: str) -> None:
                return y(a, b)
            def y(a: int, b: str) -> None:
                pass
        """
        self.compile(codestr, StaticCodeGenerator)

    def test_verify_kwargs(self):
        codestr = """
            def x(a: int=1, b: str="hunter2") -> None:
                return
            x(b="lol", a=23)
        """
        self.compile(codestr, StaticCodeGenerator)

    def test_verify_kwdefaults(self):
        codestr = """
            def x(*, b: str="hunter2"):
                return b
            z = x(b="lol")
        """
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            self.assertEqual(mod["z"], "lol")

    def test_verify_kwdefaults_no_value(self):
        codestr = """
            def x(*, b: str="hunter2"):
                return b
            a = x()
        """
        module = self.compile(codestr, StaticCodeGenerator)
        # we don't yet support optimized dispatch to kw-only functions
        self.assertInBytecode(module, "CALL_FUNCTION")
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            self.assertEqual(mod["a"], "hunter2")

    def test_verify_arg_dynamic_type(self):
        codestr = """
            def x(v:str):
                return 'abc'
            def y(v):
                return x(v)
        """
        self.compile(codestr, StaticCodeGenerator)
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            y = mod["y"]
            with self.assertRaises(TypeError):
                y(42)
            self.assertEqual(y("foo"), "abc")

    def test_verify_arg_unknown_type(self):
        codestr = """
            def x(x:foo):
                return b
            x('abc')
        """
        module = self.compile(codestr, StaticCodeGenerator)
        self.assertInBytecode(module, "INVOKE_FUNCTION")
        x = self.find_code(module)
        self.assertInBytecode(x, "CHECK_ARGS", ())

    def test_verify_kwarg_unknown_type(self):
        codestr = """
            def x(x:foo):
                return b
            x(x='abc')
        """
        module = self.compile(codestr, StaticCodeGenerator)
        self.assertInBytecode(module, "INVOKE_FUNCTION")
        x = self.find_code(module)
        self.assertInBytecode(x, "CHECK_ARGS", ())

    def test_verify_kwdefaults_too_many(self):
        codestr = """
            def x(*, b: str="hunter2") -> None:
                return
            x('abc')
        """
        with self.assertRaisesRegex(
            TypedSyntaxError, "x takes 0 positional args but 1 was given"
        ):
            self.compile(codestr, StaticCodeGenerator)

    def test_verify_kwdefaults_too_many_class(self):
        codestr = """
            class C:
                def x(self, *, b: str="hunter2") -> None:
                    return
            C().x('abc')
        """
        with self.assertRaisesRegex(
            TypedSyntaxError, "x takes 1 positional args but 2 were given"
        ):
            self.compile(codestr, StaticCodeGenerator)

    def test_verify_kwonly_failure(self):
        codestr = """
            def x(*, a: int=1, b: str="hunter2") -> None:
                return
            x(a="hi", b="lol")
        """
        with self.assertRaisesRegex(TypedSyntaxError, "keyword argument type mismatch"):
            self.compile(codestr, StaticCodeGenerator)

    def test_verify_kwonly_self_loaded_once(self):
        codestr = """
            class C:
                def x(self, *, a: int=1) -> int:
                    return 43

            def f():
                return C().x(a=1)
        """
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            f = mod["f"]
            io = StringIO()
            dis.dis(f, file=io)
            self.assertEqual(1, io.getvalue().count("LOAD_GLOBAL"))

    def test_call_function_unknown_ret_type(self):
        codestr = """
            from __future__ import annotations
            def g() -> foo:
                return 42

            def f():
                return g()
        """
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            f = mod["f"]
            self.assertEqual(f(), 42)

    def test_verify_kwargs_failure(self):
        codestr = """
            def x(a: int=1, b: str="hunter2") -> None:
                return
            x(a="hi", b="lol")
        """
        with self.assertRaisesRegex(TypedSyntaxError, "keyword argument type mismatch"):
            self.compile(codestr, StaticCodeGenerator)

    def test_verify_mixed_args(self):
        codestr = """
            def x(a: int=1, b: str="hunter2", c: int=14) -> None:
                return
            x(12, c=56, b="lol")
        """
        self.compile(codestr, StaticCodeGenerator)

    def test_kwarg_cast(self):
        codestr = """
            def x(a: int=1, b: str="hunter2", c: int=14) -> None:
                return

            def g(a):
                x(b=a)
        """
        code = self.find_code(self.compile(codestr, StaticCodeGenerator), "g")
        self.assertInBytecode(code, "CAST", ("builtins", "str"))

    def test_kwarg_nocast(self):
        codestr = """
            def x(a: int=1, b: str="hunter2", c: int=14) -> None:
                return

            def g():
                x(b='abc')
        """
        code = self.find_code(self.compile(codestr, StaticCodeGenerator), "g")
        self.assertNotInBytecode(code, "CAST", ("builtins", "str"))

    def test_verify_mixed_args_kw_failure(self):
        codestr = """
            def x(a: int=1, b: str="hunter2", c: int=14) -> None:
                return
            x(12, c="hi", b="lol")
        """
        with self.assertRaisesRegex(TypedSyntaxError, "keyword argument type mismatch"):
            self.compile(codestr, StaticCodeGenerator)

    def test_verify_mixed_args_positional_failure(self):
        codestr = """
            def x(a: int=1, b: str="hunter2", c: int=14) -> None:
                return
            x("hi", b="lol")
        """
        with self.assertRaisesRegex(
            TypedSyntaxError, "positional argument type mismatch"
        ):
            self.compile(codestr, StaticCodeGenerator)

    # Same tests as above, but for methods.
    def test_verify_positional_args_method(self):
        codestr = """
            class C:
                def x(self, a: int, b: str) -> None:
                    pass
            C().x(2, "hi")
        """
        self.compile(codestr, StaticCodeGenerator)

    def test_verify_positional_args_failure_method(self):
        codestr = """
            class C:
                def x(self, a: int, b: str) -> None:
                    pass
            C().x("a", 2)
        """
        with self.assertRaisesRegex(
            TypedSyntaxError, "positional argument type mismatch"
        ):
            self.compile(codestr, StaticCodeGenerator)

    def test_verify_mixed_args_method(self):
        codestr = """
            class C:
                def x(self, a: int=1, b: str="hunter2", c: int=14) -> None:
                    return
            C().x(12, c=56, b="lol")
        """
        self.compile(codestr, StaticCodeGenerator)

    def test_verify_mixed_args_kw_failure_method(self):
        codestr = """
            class C:
                def x(self, a: int=1, b: str="hunter2", c: int=14) -> None:
                    return
            C().x(12, c=b'lol', b="lol")
        """
        with self.assertRaisesRegex(TypedSyntaxError, "keyword argument type mismatch"):
            self.compile(codestr, StaticCodeGenerator)

    def test_verify_mixed_args_positional_failure_method(self):
        codestr = """
            class C:
                def x(self, a: int=1, b: str="hunter2", c: int=14) -> None:
                    return
            C().x("hi", b="lol")
        """
        with self.assertRaisesRegex(
            TypedSyntaxError, "positional argument type mismatch"
        ):
            self.compile(codestr, StaticCodeGenerator)

    def test_generic_kwargs_unsupported(self):
        codestr = """
        def f(a: int, b: str, **my_stuff) -> None:
            pass
        """
        with self.assertRaisesRegex(
            TypedSyntaxError,
            r"Cannot support generic keyword arguments: \*\*my_stuff.*",
        ):
            self.compile(codestr, StaticCodeGenerator)

    def test_generic_varargs_unsupported(self):
        codestr = """
        def f(a: int, b: str, *my_stuff) -> None:
            pass
        """
        with self.assertRaisesRegex(
            TypedSyntaxError, r"Cannot support generic varargs: \*my_stuff.*"
        ):
            self.compile(codestr, StaticCodeGenerator)

    def test_index_by_int(self):
        codestr = """
            from __static__ import int32
            def f(x):
                i: int32 = 0
                return x[i]
        """
        with self.assertRaises(TypedSyntaxError):
            self.compile(codestr, StaticCodeGenerator)

    def test_unknown_type_unary(self):
        codestr = """
            def x(y):
                z = -y
        """
        f = self.find_code(self.compile(codestr, StaticCodeGenerator, modname="foo"))
        self.assertInBytecode(f, "UNARY_NEGATIVE")

    def test_unknown_type_binary(self):
        codestr = """
            def x(a, b):
                z = a + b
        """
        f = self.find_code(self.compile(codestr, StaticCodeGenerator, modname="foo"))
        self.assertInBytecode(f, "BINARY_ADD")

    def test_unknown_type_compare(self):
        codestr = """
            def x(a, b):
                z = a > b
        """
        f = self.find_code(self.compile(codestr, StaticCodeGenerator, modname="foo"))
        self.assertInBytecode(f, "COMPARE_OP")

    def test_async_func_ret_type(self):
        codestr = """
            async def x(a) -> int:
                return a
        """
        f = self.find_code(self.compile(codestr, StaticCodeGenerator, modname="foo"))
        self.assertInBytecode(f, "CAST")

    def test_assign_prim_to_class(self):
        codestr = """
            from __static__ import int64
            class C: pass

            def f():
                x: C = C()
                y: int64 = 42
                x = y
        """
        with self.assertRaisesRegex(TypedSyntaxError, type_mismatch("int64", "foo.C")):
            self.compile(codestr, StaticCodeGenerator, modname="foo")

    def test_attr_generic_optional(self):
        codestr = """
            from typing import Optional
            def f(x: Optional):
                return x.foo
        """

        with self.assertRaisesRegex(
            TypedSyntaxError, "cannot access attribute from bare Optional"
        ):
            self.compile(codestr, StaticCodeGenerator, modname="foo")

    def test_assign_generic_optional(self):
        codestr = """
            from typing import Optional
            def f():
                x: Optional = 42
        """

        with self.assertRaises(TypedSyntaxError):
            self.compile(codestr, StaticCodeGenerator, modname="foo")

    def test_assign_generic_optional_2(self):
        codestr = """
            from typing import Optional
            def f():
                x: Optional = 42 + 1
        """

        with self.assertRaises(TypedSyntaxError):
            self.compile(codestr, StaticCodeGenerator, modname="foo")

    def test_assign_from_generic_optional(self):
        codestr = """
            from typing import Optional
            class C: pass

            def f(x: Optional):
                y: Optional[C] = x
        """

        with self.assertRaisesRegex(
            TypedSyntaxError, type_mismatch("Optional[T]", optional("foo.C"))
        ):
            self.compile(codestr, StaticCodeGenerator, modname="foo")

    def test_int_swap(self):
        codestr = """
            from __static__ import int64, box

            def test():
                x: int64 = 42
                y: int64 = 100
                x, y = y, x
                return box(x), box(y)
        """

        with self.assertRaisesRegex(
            TypedSyntaxError, type_mismatch("int64", "dynamic")
        ):
            self.compile(codestr, StaticCodeGenerator, modname="foo")

    def test_typed_swap(self):
        codestr = """
            def test(a):
                x: int
                y: str
                x, y = 1, a
        """

        f = self.find_code(self.compile(codestr, StaticCodeGenerator, modname="foo"))
        self.assertInBytecode(f, "CAST", ("builtins", "str"))
        self.assertNotInBytecode(f, "CAST", ("builtins", "int"))

    def test_typed_swap_2(self):
        codestr = """
            def test(a):
                x: int
                y: str
                x, y = a, 'abc'

        """

        f = self.find_code(self.compile(codestr, StaticCodeGenerator, modname="foo"))
        self.assertInBytecode(f, "CAST", ("builtins", "int"))
        self.assertNotInBytecode(f, "CAST", ("builtins", "str"))

    def test_typed_swap_member(self):
        codestr = """
            class C:
                def __init__(self):
                    self.x: int = 42

            def test(a):
                x: int
                y: str
                C().x, y = a, 'abc'

        """

        f = self.find_code(
            self.compile(codestr, StaticCodeGenerator, modname="foo"), "test"
        )
        self.assertInBytecode(f, "CAST", ("builtins", "int"))
        self.assertNotInBytecode(f, "CAST", ("builtins", "str"))

    def test_typed_swap_list(self):
        codestr = """
            def test(a):
                x: int
                y: str
                [x, y] = a, 'abc'

        """

        f = self.find_code(self.compile(codestr, StaticCodeGenerator, modname="foo"))
        self.assertInBytecode(f, "CAST", ("builtins", "int"))
        self.assertNotInBytecode(f, "CAST", ("builtins", "str"))

    def test_typed_swap_nested(self):
        codestr = """
            def test(a):
                x: int
                y: str
                z: str
                ((x, y), z) = (a, 'abc'), 'foo'

        """

        f = self.find_code(self.compile(codestr, StaticCodeGenerator, modname="foo"))
        self.assertInBytecode(f, "CAST", ("builtins", "int"))
        self.assertNotInBytecode(f, "CAST", ("builtins", "str"))

    def test_typed_swap_nested_2(self):
        codestr = """
            def test(a):
                x: int
                y: str
                z: str
                ((x, y), z) = (1, a), 'foo'

        """

        f = self.find_code(self.compile(codestr, StaticCodeGenerator, modname="foo"))
        self.assertInBytecode(f, "CAST", ("builtins", "str"))
        self.assertNotInBytecode(f, "CAST", ("builtins", "int"))

    def test_typed_swap_nested_3(self):
        codestr = """
            def test(a):
                x: int
                y: int
                z: str
                ((x, y), z) = (1, 2), a

        """

        f = self.find_code(self.compile(codestr, StaticCodeGenerator, modname="foo"))
        self.assertInBytecode(f, "CAST", ("builtins", "str"))
        # Currently because the tuple gets turned into a constant this is less than
        # ideal:
        self.assertInBytecode(f, "CAST", ("builtins", "int"))

    def test_if_optional(self):
        codestr = """
            from typing import Optional
            class C:
                def __init__(self):
                    self.field = 42

            def f(x: Optional[C]):
                if x is not None:
                    return x.field

                return None
        """

        code = self.compile(codestr, StaticCodeGenerator, modname="foo")
        f = self.find_code(code, "f")
        self.assertNotInBytecode(f, "RAISE_IF_NONE")

    def test_return_outside_func(self):
        codestr = """
            return 42
        """
        with self.assertRaisesRegex(SyntaxError, "'return' outside function"):
            self.compile(codestr, StaticCodeGenerator, modname="foo")

    def test_double_decl(self):
        codestr = """
            def f():
                x: int
                x: str
        """
        with self.assertRaisesRegex(
            TypedSyntaxError, "Cannot redefine local variable x"
        ):
            self.compile(codestr, StaticCodeGenerator, modname="foo")

        codestr = """
            def f():
                x = 42
                x: str
        """
        with self.assertRaisesRegex(
            TypedSyntaxError, "Cannot redefine local variable x"
        ):
            self.compile(codestr, StaticCodeGenerator, modname="foo")

        codestr = """
            def f():
                x = 42
                x: int
        """
        with self.assertRaisesRegex(
            TypedSyntaxError, "Cannot redefine local variable x"
        ):
            self.compile(codestr, StaticCodeGenerator, modname="foo")

    def test_if_else_optional(self):
        codestr = """
            from typing import Optional
            class C:
                def __init__(self):
                    self.field = self

            def g(x: C):
                pass

            def f(x: Optional[C], y: Optional[C]):
                if x is None:
                    x = y
                    if x is None:
                        return None
                    else:
                        return g(x)
                else:
                    return g(x)


                return None
        """

        code = self.compile(codestr, StaticCodeGenerator, modname="foo")
        f = self.find_code(code, "f")
        self.assertNotInBytecode(f, "RAISE_IF_NONE")

    def test_if_else_optional_return(self):
        codestr = """
            from typing import Optional
            class C:
                def __init__(self):
                    self.field = self

            def f(x: Optional[C]):
                if x is None:
                    return 0
                return x.field
        """

        code = self.compile(codestr, StaticCodeGenerator, modname="foo")
        f = self.find_code(code, "f")
        self.assertNotInBytecode(f, "RAISE_IF_NONE")

    def test_if_else_optional_return_two_branches(self):
        codestr = """
            from typing import Optional
            class C:
                def __init__(self):
                    self.field = self

            def f(x: Optional[C]):
                if x is None:
                    if a:
                        return 0
                    else:
                        return 2
                return x.field
        """

        code = self.compile(codestr, StaticCodeGenerator, modname="foo")
        f = self.find_code(code, "f")
        self.assertNotInBytecode(f, "RAISE_IF_NONE")

    def test_narrow_conditional(self):
        codestr = """
            class B:
                def f(self):
                    return 42
            class D(B):
                def f(self):
                    return 'abc'

            def testfunc(abc):
                x = B()
                if abc:
                    x = D()
                    return x.f()
        """

        code = self.compile(codestr, StaticCodeGenerator, modname="foo")
        f = self.find_code(code, "testfunc")
        self.assertInBytecode(f, "INVOKE_METHOD", (("foo", "D", "f"), 0))
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            test = mod["testfunc"]
            self.assertEqual(test(True), "abc")
            self.assertEqual(test(False), None)

    def test_no_narrow_to_dynamic(self):
        codestr = """
            def f():
                return 42

            def g():
                x: int = 100
                x = f()
                return x.bit_length()
        """

        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            g = mod["g"]
            self.assertInBytecode(g, "CAST", ("builtins", "int"))
            self.assertInBytecode(
                g, "INVOKE_METHOD", (("builtins", "int", "bit_length"), 0)
            )
            self.assertEqual(g(), 6)

    def test_narrow_conditional_widened(self):
        codestr = """
            class B:
                def f(self):
                    return 42
            class D(B):
                def f(self):
                    return 'abc'

            def testfunc(abc):
                x = B()
                if abc:
                    x = D()
                return x.f()
        """

        code = self.compile(codestr, StaticCodeGenerator, modname="foo")
        f = self.find_code(code, "testfunc")
        self.assertInBytecode(f, "INVOKE_METHOD", (("foo", "B", "f"), 0))
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            test = mod["testfunc"]
            self.assertEqual(test(True), "abc")
            self.assertEqual(test(False), 42)

    def test_assign_conditional_both_sides(self):
        codestr = """
            class B:
                def f(self):
                    return 42
            class D(B):
                def f(self):
                    return 'abc'

            def testfunc(abc):
                x = B()
                if abc:
                    x = D()
                else:
                    x = D()
                return x.f()
        """

        code = self.compile(codestr, StaticCodeGenerator, modname="foo")
        f = self.find_code(code, "testfunc")
        self.assertInBytecode(f, "INVOKE_METHOD", (("foo", "D", "f"), 0))
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            test = mod["testfunc"]
            self.assertEqual(test(True), "abc")

    def test_assign_conditional_invoke_in_else(self):
        codestr = """
            class B:
                def f(self):
                    return 42
            class D(B):
                def f(self):
                    return 'abc'

            def testfunc(abc):
                x = B()
                if abc:
                    x = D()
                else:
                    return x.f()
        """

        code = self.compile(codestr, StaticCodeGenerator, modname="foo")
        f = self.find_code(code, "testfunc")
        self.assertInBytecode(f, "INVOKE_METHOD", (("foo", "B", "f"), 0))
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            test = mod["testfunc"]
            self.assertEqual(test(True), None)

    def test_assign_else_only(self):
        codestr = """
            class B:
                def f(self):
                    return 42
            class D(B):
                def f(self):
                    return 'abc'

            def testfunc(abc):
                if abc:
                    pass
                else:
                    x = B()
                return x.f()
        """

        code = self.compile(codestr, StaticCodeGenerator, modname="foo")
        f = self.find_code(code, "testfunc")
        self.assertInBytecode(f, "INVOKE_METHOD", (("foo", "B", "f"), 0))
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            test = mod["testfunc"]
            self.assertEqual(test(False), 42)

    def test_assign_while(self):
        codestr = """
            class B:
                def f(self):
                    return 42
            class D(B):
                def f(self):
                    return 'abc'

            def testfunc(abc):
                x = B()
                while abc:
                    x = D()
                return x.f()
        """

        code = self.compile(codestr, StaticCodeGenerator, modname="foo")
        f = self.find_code(code, "testfunc")
        self.assertInBytecode(f, "INVOKE_METHOD", (("foo", "B", "f"), 0))
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            test = mod["testfunc"]
            self.assertEqual(test(False), 42)

    def test_assign_but_annotated(self):
        codestr = """
            class B:
                def f(self):
                    return 42
            class D(B):
                def f(self):
                    return 'abc'

            def testfunc(abc):
                x: B = D()
                return x.f()
        """

        code = self.compile(codestr, StaticCodeGenerator, modname="foo")
        f = self.find_code(code, "testfunc")
        self.assertInBytecode(f, "INVOKE_METHOD", (("foo", "D", "f"), 0))
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            test = mod["testfunc"]
            self.assertEqual(test(False), "abc")

    def test_assign_while_2(self):
        codestr = """
            class B:
                def f(self):
                    return 42
            class D(B):
                def f(self):
                    return 'abc'

            def testfunc(abc):
                x: B = D()
                while abc:
                    x = B()
                return x.f()
        """

        code = self.compile(codestr, StaticCodeGenerator, modname="foo")
        f = self.find_code(code, "testfunc")
        self.assertInBytecode(f, "INVOKE_METHOD", (("foo", "B", "f"), 0))
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            test = mod["testfunc"]
            self.assertEqual(test(False), "abc")

    def test_assign_while_else(self):
        codestr = """
            class B:
                def f(self):
                    return 42
            class D(B):
                def f(self):
                    return 'abc'

            def testfunc(abc):
                x = B()
                while abc:
                    pass
                else:
                    x = D()
                return x.f()
        """

        code = self.compile(codestr, StaticCodeGenerator, modname="foo")
        f = self.find_code(code, "testfunc")
        self.assertInBytecode(f, "INVOKE_METHOD", (("foo", "B", "f"), 0))
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            test = mod["testfunc"]
            self.assertEqual(test(False), "abc")

    def test_assign_while_else_2(self):
        codestr = """
            class B:
                def f(self):
                    return 42
            class D(B):
                def f(self):
                    return 'abc'

            def testfunc(abc):
                x: B = D()
                while abc:
                    pass
                else:
                    x = B()
                return x.f()
        """

        code = self.compile(codestr, StaticCodeGenerator, modname="foo")
        f = self.find_code(code, "testfunc")
        self.assertInBytecode(f, "INVOKE_METHOD", (("foo", "B", "f"), 0))
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            test = mod["testfunc"]
            self.assertEqual(test(False), 42)

    def test_assign_try_except_no_initial(self):
        codestr = """
            class B:
                def f(self):
                    return 42
            class D(B):
                def f(self):
                    return 'abc'

            def testfunc():
                try:
                    x: B = D()
                except:
                    x = B()
                return x.f()
        """

        code = self.compile(codestr, StaticCodeGenerator, modname="foo")
        f = self.find_code(code, "testfunc")
        self.assertInBytecode(f, "INVOKE_METHOD", (("foo", "B", "f"), 0))
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            test = mod["testfunc"]
            self.assertEqual(test(), "abc")

    def test_global_int(self):
        codestr = """
            X: int =  60 * 60 * 24
        """

        self.compile(codestr, StaticCodeGenerator, modname="foo")
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            X = mod["X"]
            self.assertEqual(X, 60 * 60 * 24)

    def test_with_traceback(self):
        codestr = """
            def f():
                x = Exception()
                return x.with_traceback(None)
        """

        self.compile(codestr, StaticCodeGenerator, modname="foo")
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            f = mod["f"]
            self.assertEqual(type(f()), Exception)
            self.assertInBytecode(
                f, "INVOKE_METHOD", (("builtins", "BaseException", "with_traceback"), 1)
            )

    def test_assign_num_to_object(self):
        codestr = """
            def f():
                x: object = 42
        """

        self.compile(codestr, StaticCodeGenerator, modname="foo")
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            f = mod["f"]
            self.assertNotInBytecode(f, "CAST", ("builtins", "object"))

    def test_assign_num_to_dynamic(self):
        codestr = """
            def f():
                x: foo = 42
        """

        self.compile(codestr, StaticCodeGenerator, modname="foo")
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            f = mod["f"]
            self.assertNotInBytecode(f, "CAST", ("builtins", "object"))

    def test_assign_dynamic_to_object(self):
        codestr = """
            def f(C):
                x: object = C()
        """

        self.compile(codestr, StaticCodeGenerator, modname="foo")
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            f = mod["f"]
            self.assertNotInBytecode(f, "CAST", ("builtins", "object"))

    def test_assign_dynamic_to_dynamic(self):
        codestr = """
            def f(C):
                x: unknown = C()
        """

        self.compile(codestr, StaticCodeGenerator, modname="foo")
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            f = mod["f"]
            self.assertNotInBytecode(f, "CAST", ("builtins", "object"))

    def test_assign_constant_to_object(self):
        codestr = """
            def f():
                x: object = 42 + 1
        """

        self.compile(codestr, StaticCodeGenerator, modname="foo")
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            f = mod["f"]
            self.assertNotInBytecode(f, "CAST", ("builtins", "object"))

    def test_assign_try_except_typing(self):
        codestr = """
            def testfunc():
                try:
                    pass
                except Exception as e:
                    pass
                return 42
        """

        # We don't do anything special w/ Exception type yet, but it should compile
        self.compile(codestr, StaticCodeGenerator, modname="foo")

    def test_assign_try_except_typing_predeclared(self):
        codestr = """
            def testfunc():
                e: Exception
                try:
                    pass
                except Exception as e:
                    pass
                return 42
        """
        # We don't do anything special w/ Exception type yet, but it should compile
        self.compile(codestr, StaticCodeGenerator, modname="foo")

    def test_assign_try_except_typing_narrowed(self):
        codestr = """
            class E(Exception):
                pass

            def testfunc():
                e: Exception
                try:
                    pass
                except E as e:
                    pass
                return 42
        """
        # We don't do anything special w/ Exception type yet, but it should compile
        self.compile(codestr, StaticCodeGenerator, modname="foo")

    def test_assign_try_except_typing_redeclared_after(self):
        codestr = """
            def testfunc():
                try:
                    pass
                except Exception as e:
                    pass
                e: int = 42
                return 42
        """
        # We don't do anything special w/ Exception type yet, but it should compile
        self.compile(codestr, StaticCodeGenerator, modname="foo")

    def test_assign_try_except_redeclare(self):
        codestr = """
            def testfunc():
                e: int
                try:
                    pass
                except Exception as e:
                    pass
                return 42
        """

        self.compile(codestr, StaticCodeGenerator, modname="foo")

    def test_assign_try_except_redeclare_unknown_type(self):
        codestr = """
            def testfunc():
                e: int
                try:
                    pass
                except UnknownException as e:
                    pass
                return 42
        """

        self.compile(codestr, StaticCodeGenerator, modname="foo")

    def test_assign_try_assign_in_except(self):
        codestr = """
            class B:
                def f(self):
                    return 42
            class D(B):
                def f(self):
                    return 'abc'

            def testfunc():
                x: B = D()
                try:
                    pass
                except:
                    x = B()
                return x.f()
        """

        code = self.compile(codestr, StaticCodeGenerator, modname="foo")
        f = self.find_code(code, "testfunc")
        self.assertInBytecode(f, "INVOKE_METHOD", (("foo", "B", "f"), 0))
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            test = mod["testfunc"]
            self.assertEqual(test(), "abc")

    def test_assign_try_assign_in_second_except(self):
        codestr = """
            class B:
                def f(self):
                    return 42
            class D(B):
                def f(self):
                    return 'abc'

            def testfunc():
                x: B = D()
                try:
                    pass
                except TypeError:
                    pass
                except:
                    x = B()
                return x.f()
        """

        code = self.compile(codestr, StaticCodeGenerator, modname="foo")
        f = self.find_code(code, "testfunc")
        self.assertInBytecode(f, "INVOKE_METHOD", (("foo", "B", "f"), 0))
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            test = mod["testfunc"]
            self.assertEqual(test(), "abc")

    def test_assign_try_assign_in_except_with_var(self):
        codestr = """
            class B:
                def f(self):
                    return 42
            class D(B):
                def f(self):
                    return 'abc'

            def testfunc():
                x: B = D()
                try:
                    pass
                except TypeError as e:
                    x = B()
                return x.f()
        """

        code = self.compile(codestr, StaticCodeGenerator, modname="foo")
        f = self.find_code(code, "testfunc")
        self.assertInBytecode(f, "INVOKE_METHOD", (("foo", "B", "f"), 0))
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            test = mod["testfunc"]
            self.assertEqual(test(), "abc")

    def test_try_except_finally(self):
        codestr = """
            class B:
                def f(self):
                    return 42
            class D(B):
                def f(self):
                    return 'abc'

            def testfunc():
                x: B = D()
                try:
                    pass
                except TypeError:
                    pass
                finally:
                    x = B()
                return x.f()
        """

        code = self.compile(codestr, StaticCodeGenerator, modname="foo")
        f = self.find_code(code, "testfunc")
        self.assertInBytecode(f, "INVOKE_METHOD", (("foo", "B", "f"), 0))
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            test = mod["testfunc"]
            self.assertEqual(test(), 42)

    def test_assign_try_assign_in_try(self):
        codestr = """
            class B:
                def f(self):
                    return 42
            class D(B):
                def f(self):
                    return 'abc'

            def testfunc():
                x: B = D()
                try:
                    x = B()
                except:
                    pass
                return x.f()
        """

        code = self.compile(codestr, StaticCodeGenerator, modname="foo")
        f = self.find_code(code, "testfunc")
        self.assertInBytecode(f, "INVOKE_METHOD", (("foo", "B", "f"), 0))
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            test = mod["testfunc"]
            self.assertEqual(test(), 42)

    def test_assign_try_assign_in_finally(self):
        codestr = """
            class B:
                def f(self):
                    return 42
            class D(B):
                def f(self):
                    return 'abc'

            def testfunc():
                x: B = D()
                try:
                    pass
                finally:
                    x = B()
                return x.f()
        """

        code = self.compile(codestr, StaticCodeGenerator, modname="foo")
        f = self.find_code(code, "testfunc")
        self.assertInBytecode(f, "INVOKE_METHOD", (("foo", "B", "f"), 0))
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            test = mod["testfunc"]
            self.assertEqual(test(), 42)

    def test_assign_try_assign_in_else(self):
        codestr = """
            class B:
                def f(self):
                    return 42
            class D(B):
                def f(self):
                    return 'abc'

            def testfunc():
                x: B = D()
                try:
                    pass
                except:
                    pass
                else:
                    x = B()
                return x.f()
        """

        code = self.compile(codestr, StaticCodeGenerator, modname="foo")
        f = self.find_code(code, "testfunc")
        self.assertInBytecode(f, "INVOKE_METHOD", (("foo", "B", "f"), 0))
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            test = mod["testfunc"]
            self.assertEqual(test(), 42)

    def test_if_optional_reassign(self):
        codestr = """
        class C: pass

        def testfunc(abc: Optional[C]):
            if abc is not None:
                abc = None
        """
        self.compile(codestr, StaticCodeGenerator, modname="foo")

    def test_widening_assign(self):
        codestr = """
            from __static__ import int8, int16, box

            def testfunc():
                x: int16
                y: int8
                x = y = 42
                return box(x), box(y)
        """
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            test = mod["testfunc"]
            self.assertEqual(test(), (42, 42))

    def test_unknown_imported_annotation(self):
        codestr = """
            from unknown_mod import foo

            def testfunc():
                x: foo = 42
                return x
        """
        self.compile(codestr, StaticCodeGenerator, modname="foo")

    def test_widening_assign_reassign(self):
        codestr = """
            from __static__ import int8, int16, box

            def testfunc():
                x: int16
                y: int8
                x = y = 42
                x = 257
                return box(x), box(y)
        """
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            test = mod["testfunc"]
            self.assertEqual(test(), (257, 42))

    def test_widening_assign_reassign_error(self):
        codestr = """
            from __static__ import int8, int16, box

            def testfunc():
                x: int16
                y: int8
                x = y = 42
                y = 128
                return box(x), box(y)
        """
        with self.assertRaisesRegex(
            TypedSyntaxError,
            "constant 128 is outside of the range -128 to 127 for int8",
        ):
            self.compile(codestr, StaticCodeGenerator, modname="foo")

    def test_narrowing_assign_literal(self):
        codestr = """
            from __static__ import int8, int16, box

            def testfunc():
                x: int8
                y: int16
                x = y = 42
                return box(x), box(y)
        """
        self.compile(codestr, StaticCodeGenerator, modname="foo")

    def test_narrowing_assign_out_of_range(self):
        codestr = """
            from __static__ import int8, int16, box

            def testfunc():
                x: int8
                y: int16
                x = y = 300
                return box(x), box(y)
        """
        with self.assertRaisesRegex(
            TypedSyntaxError,
            "constant 300 is outside of the range -128 to 127 for int8",
        ):
            self.compile(codestr, StaticCodeGenerator, modname="foo")

    def test_module_primitive(self):
        codestr = """
            from __static__ import int8
            x: int8
        """
        with self.assertRaisesRegex(
            TypedSyntaxError, "cannot use primitives in global or closure scope"
        ):
            self.compile(codestr, StaticCodeGenerator, modname="foo")

    def test_implicit_module_primitive(self):
        codestr = """
            from __static__ import int8
            x = y = int8(0)
        """
        with self.assertRaisesRegex(
            TypedSyntaxError, "cannot use primitives in global or closure scope"
        ):
            self.compile(codestr, StaticCodeGenerator, modname="foo")

    def test_chained_primitive_to_non_primitive(self):
        codestr = """
            from __static__ import int8
            def f():
                x: object
                y: int8 = 42
                x = y = 42
        """
        with self.assertRaisesRegex(
            TypedSyntaxError, "int8 cannot be assigned to builtins.object"
        ):
            self.compile(codestr, StaticCodeGenerator, modname="foo")

    def test_closure_primitive(self):
        codestr = """
            from __static__ import int8
            def f():
                x: int8 = 0
                def g():
                    return x
        """
        with self.assertRaisesRegex(
            TypedSyntaxError, "cannot use primitives in global or closure scope"
        ):
            self.compile(codestr, StaticCodeGenerator, modname="foo")

    def test_nonlocal_primitive(self):
        codestr = """
            from __static__ import int8
            def f():
                x: int8 = 0
                def g():
                    nonlocal x
                    x = 1
        """
        with self.assertRaisesRegex(
            TypedSyntaxError, "cannot use primitives in global or closure scope"
        ):
            self.compile(codestr, StaticCodeGenerator, modname="foo")

    def test_dynamic_chained_assign_param(self):
        codestr = """
            from __static__ import int16
            def testfunc(y):
                x: int16
                x = y = 42
                return box(x)
        """
        with self.assertRaisesRegex(
            TypedSyntaxError, type_mismatch("builtins.int", "int16")
        ):
            self.compile(codestr, StaticCodeGenerator, modname="foo")

    def test_dynamic_chained_assign_param_2(self):
        codestr = """
            from __static__ import int16
            def testfunc(y):
                x: int16
                y = x = 42
        """
        with self.assertRaisesRegex(
            TypedSyntaxError, type_mismatch("int16", "dynamic")
        ):
            self.compile(codestr, StaticCodeGenerator, modname="foo")

    def test_dynamic_chained_assign_1(self):
        codestr = """
            from __static__ import int16, box
            def testfunc():
                x: int16
                x = y = 42
                return box(x)
        """
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            test = mod["testfunc"]
            self.assertEqual(test(), 42)

    def test_dynamic_chained_assign_2(self):
        codestr = """
            from __static__ import int16, box
            def testfunc():
                x: int16
                y = x = 42
                return box(y)
        """
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            test = mod["testfunc"]
            self.assertEqual(test(), 42)

    def test_if_optional_cond(self):
        codestr = """
            from typing import Optional
            class C:
                def __init__(self):
                    self.field = 42

            def f(x: Optional[C]):
                return x.field if x is not None else None
        """

        code = self.compile(codestr, StaticCodeGenerator, modname="foo")
        f = self.find_code(code, "f")
        self.assertNotInBytecode(f, "RAISE_IF_NONE")

    def test_while_optional_cond(self):
        codestr = """
            from typing import Optional
            class C:
                def __init__(self):
                    self.field: Optional["C"] = self

            def f(x: Optional[C]):
                while x is not None:
                    val: Optional[C] = x.field
                    if val is not None:
                        x = val
        """

        code = self.compile(codestr, StaticCodeGenerator, modname="foo")
        f = self.find_code(code, "f")
        self.assertNotInBytecode(f, "RAISE_IF_NONE")

    def test_if_optional_dependent_conditions(self):
        codestr = """
            from typing import Optional
            class C:
                def __init__(self):
                    self.field: Optional[C] = None

            def f(x: Optional[C]):
                if x is not None and x.field is not None:
                    return 42

                return None
        """

        code = self.compile(codestr, StaticCodeGenerator, modname="foo")
        f = self.find_code(code, "f")
        self.assertNotInBytecode(f, "RAISE_IF_NONE")

    def test_array_import(self):
        codestr = """
            from __static__ import int64, Array

            def test() -> Array[int64]:
                x: Array[int64] = Array[int64]()
                x.append(1)
                return x
        """

        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            test = mod["test"]
            self.assertEqual(test(), array("L", [1]))

    def test_array_create(self):
        codestr = """
            from __static__ import int64, Array

            def test() -> Array[int64]:
                x: Array[int64] = Array[int64]([1, 3, 5])
                return x
        """

        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            test = mod["test"]
            self.assertEqual(test(), array("l", [1, 3, 5]))

    def test_array_create_failure(self):
        # todo - in the future we're going to support this, but for now fail it.
        codestr = """
            from __static__ import int64, Array

            class C: pass

            def test() -> Array[C]:
                return Array[C]([1, 3, 5])
        """
        with self.assertRaisesRegex(
            TypedSyntaxError, "Invalid array element type: foo.C"
        ):
            self.compile(codestr, StaticCodeGenerator, modname="foo")

    def test_array_assign_wrong_type(self):
        codestr = """
            from __static__ import int64, char, Array

            def test() -> None:
                x: Array[int64] = Array[char]([48])
        """
        with self.assertRaisesRegex(
            TypedSyntaxError,
            r"type mismatch: Array\[char\] cannot be assigned to Array\[int64\].*",
        ):
            self.compile(codestr, StaticCodeGenerator, modname="foo")

    def test_array_types(self):
        codestr = """
            from __static__ import (
                int8,
                int16,
                int32,
                int64,
                uint8,
                uint16,
                uint32,
                uint64,
                char,
                double,
                Array
            )
            from typing import Tuple

            def test() -> Tuple[Array[int64], Array[char], Array[double]]:
                x1: Array[int8] = Array[int8]([1, 3, -5])
                x2: Array[uint8] = Array[uint8]([1, 3, 5])
                x3: Array[int16] = Array[int16]([1, -3, 5])
                x4: Array[uint16] = Array[uint16]([1, 3, 5])
                x5: Array[int32] = Array[int32]([1, 3, 5])
                x6: Array[uint32] = Array[uint32]([1, 3, 5])
                x7: Array[int64] = Array[int64]([1, 3, 5])
                x8: Array[uint64] = Array[uint64]([1, 3, 5])
                x9: Array[char] = Array[char]([ord('a')])
                x10: Array[double] = Array[double]([1.1, 3.3, 5.5])
                x11: Array[float] = Array[float]([1.1, 3.3, 5.5])
                return (
                    x1,
                    x2,
                    x3,
                    x4,
                    x5,
                    x6,
                    x7,
                    x8,
                    x9,
                    x10,
                    x11,
                )
        """

        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            test = mod["test"]
            results = test()
            expectations = (
                ("h", [1, 3, -5]),
                ("H", [1, 3, 5]),
                ("i", [1, -3, 5]),
                ("I", [1, 3, 5]),
                ("l", [1, 3, 5]),
                ("L", [1, 3, 5]),
                ("q", [1, 3, 5]),
                ("Q", [1, 3, 5]),
                ("b", [ord("a")]),
                ("d", [1.1, 3.3, 5.5]),
                ("f", [1.1, 3.3, 5.5]),
            )
            for result, expectation in zip(results, expectations):
                self.assertEqual(result, array(*expectation))

    def test_assign_type_propagation(self):
        codestr = """
            def test() -> int:
                x = 5
                return x
        """
        self.compile(codestr, StaticCodeGenerator, modname="foo")

    def test_assign_subtype_handling(self):
        codestr = """
            class B: pass
            class D(B): pass

            def f():
                b = B()
                b = D()
                b = B()
        """
        self.compile(codestr, StaticCodeGenerator, modname="foo")

    def test_assign_subtype_handling_fail(self):
        codestr = """
            class B: pass
            class D(B): pass

            def f():
                d = D()
                d = B()
        """
        with self.assertRaisesRegex(TypedSyntaxError, type_mismatch("foo.B", "foo.D")):
            self.compile(codestr, StaticCodeGenerator, modname="foo")

    def test_assign_chained(self):
        codestr = """
            def test() -> str:
                x: str = "hi"
                y = x = "hello"
                return y
        """
        self.compile(codestr, StaticCodeGenerator, modname="foo")

    def test_assign_chained_failure_wrong_target_type(self):
        codestr = """
            def test() -> str:
                x = 1
                y = x = "hello"
                return y
        """
        with self.assertRaisesRegex(
            TypedSyntaxError, type_mismatch("builtins.str", "builtins.int")
        ):
            self.compile(codestr, StaticCodeGenerator, modname="foo")

    def test_chained_assign_type_propagation(self):
        codestr = """
            from __static__ import int64, char, Array

            def test2() -> Array[char]:
                x = y = Array[char]([48])
                return y
        """
        self.compile(codestr, StaticCodeGenerator, modname="foo")

    def test_chained_assign_type_propagation_failure_redefine(self):
        codestr = """
            from __static__ import int64, char, Array

            def test2() -> Array[char]:
                x = Array[int64]([54])
                x = y = Array[char]([48])
                return y
        """
        with self.assertRaisesRegex(
            TypedSyntaxError, type_mismatch("Array[char]", "Array[int64]")
        ):
            self.compile(codestr, StaticCodeGenerator, modname="foo")

    def test_chained_assign_type_propagation_failure_redefine_2(self):
        codestr = """
            from __static__ import int64, char, Array

            def test2() -> Array[char]:
                x = Array[int64]([54])
                y = x = Array[char]([48])
                return y
        """
        with self.assertRaisesRegex(
            TypedSyntaxError, type_mismatch("Array[char]", "Array[int64]")
        ):
            self.compile(codestr, StaticCodeGenerator, modname="foo")

    def test_chained_assign_type_inference(self):
        codestr = """
            from __static__ import int64, char, Array

            def test2():
                y = x = 4
                x = "hello"
                return y
        """
        with self.assertRaisesRegex(
            TypedSyntaxError, type_mismatch("builtins.str", "builtins.int")
        ):
            self.compile(codestr, StaticCodeGenerator, modname="foo")

    def test_chained_assign_type_inference_2(self):
        codestr = """
            from __static__ import int64, char, Array

            def test2():
                y = x = 4
                y = "hello"
                return x
        """
        with self.assertRaisesRegex(
            TypedSyntaxError, type_mismatch("builtins.str", "builtins.int")
        ):
            self.compile(codestr, StaticCodeGenerator, modname="foo")

    def test_array_subscripting(self):
        codestr = """
            from __static__ import Array, int8

            def m() -> int8:
                a = Array[int8]([1, 3, -5])
                return a[1]
        """
        self.compile(codestr, StaticCodeGenerator, modname="foo")

    def test_array_subscripting_slice(self):
        codestr = """
            from __static__ import Array, int8

            def m() -> Array[int8]:
                a = Array[int8]([1, 3, -5, -1, 7, 22])
                return a[1:3]
        """
        self.compile(codestr, StaticCodeGenerator, modname="foo")

    @skipIf(cinderjit is not None, "can't report error from JIT")
    def test_load_uninit_module(self):
        """verify we don't crash if we receive a module w/o a dictionary"""
        codestr = """
        class C:
            def __init__(self):
                self.x: Optional[C] = None

        """
        with self.in_module(codestr) as mod:
            C = mod["C"]

            class UninitModule(ModuleType):
                def __init__(self):
                    # don't call super init
                    pass

            sys.modules["test_load_uninit_module"] = UninitModule()
            with self.assertRaisesRegex(
                TypeError, "bad name provided for class loader: "
            ):
                C()

    def test_module_subclass(self):
        codestr = """
        class C:
            def __init__(self):
                self.x: Optional[C] = None

        """
        with self.in_module(codestr) as mod:
            C = mod["C"]

            class CustomModule(ModuleType):
                def __getattr__(self, name):
                    if name == "C":
                        return C

            sys.modules["test_module_subclass"] = CustomModule("test_module_subclass")
            c = C()
            self.assertEqual(c.x, None)

    def test_override_okay(self):
        codestr = """
        class B:
            def f(self) -> "B":
                return self

        def f(x: B):
            return x.f()
        """
        with self.in_module(codestr) as mod:
            B = mod["B"]
            f = mod["f"]

            class D(B):
                def f(self):
                    return self

            f(D())

    def test_override_override_inherited(self):
        codestr = """
        from typing import Optional
        class B:
            def f(self) -> "Optional[B]":
                return self

        class D(B):
            pass

        def f(x: B):
            return x.f()
        """
        with self.in_module(codestr) as mod:
            B = mod["B"]
            D = mod["D"]
            f = mod["f"]

            b = B()
            d = D()
            self.assertEqual(f(b), b)
            self.assertEqual(f(d), d)

            D.f = lambda self: None
            self.assertEqual(f(b), b)
            self.assertEqual(f(d), None)

    def test_override_bad_ret(self):
        codestr = """
        class B:
            def f(self) -> "B":
                return self

        def f(x: B):
            return x.f()
        """
        with self.in_module(codestr) as mod:
            B = mod["B"]
            f = mod["f"]

            class D(B):
                def f(self):
                    return 42

            with self.assertRaisesRegex(
                TypeError, "unexpected return type from D.f, expected B, got int"
            ):
                f(D())

    @skipIf(not HAS_INVOKE_METHOD, "no invoke method support")
    def test_invoke_type_modified(self):
        codestr = """
            class C:
                def f(self):
                    return 1

            def x(c: C):
                x = c.f()
                x += c.f()
                return x
        """

        code = self.compile(codestr, StaticCodeGenerator, modname="foo")
        x = self.find_code(code, "x")
        self.assertInBytecode(x, "INVOKE_METHOD", (("foo", "C", "f"), 0))

        with self.in_module(codestr) as mod:
            x, C = mod["x"], mod["C"]
            self.assertEqual(x(C()), 2)
            C.f = lambda self: 42
            self.assertEqual(x(C()), 84)

    @skipIf(not HAS_INVOKE_METHOD, "no invoke method support")
    def test_invoke_type_modified_pre_invoke(self):
        codestr = """
            class C:
                def f(self):
                    return 1

            def x(c: C):
                x = c.f()
                x += c.f()
                return x
        """

        code = self.compile(codestr, StaticCodeGenerator, modname="foo")
        x = self.find_code(code, "x")
        self.assertInBytecode(x, "INVOKE_METHOD", (("foo", "C", "f"), 0))

        with self.in_module(codestr) as mod:
            x, C = mod["x"], mod["C"]
            C.f = lambda self: 42
            self.assertEqual(x(C()), 84)

    @skipIf(not HAS_INVOKE_METHOD, "no invoke method support")
    def test_override_modified_base_class(self):
        codestr = """
        class B:
            def f(self):
                return 1

        def f(x: B):
            return x.f()
        """
        with self.in_module(codestr) as mod:
            B = mod["B"]
            f = mod["f"]
            B.f = lambda self: 2

            class D(B):
                def f(self):
                    return 3

            d = D()
            self.assertEqual(f(d), 3)

    @skipIf(not HAS_INVOKE_METHOD, "no invoke method support")
    def test_override_remove_base_method(self):
        codestr = """
        from typing import Optional
        class B:
            def f(self) -> "B":
                return self

        class D(B): pass

        def f(x: B):
            return x.f()
        """
        with self.in_module(codestr) as mod:
            B = mod["B"]
            D = mod["D"]
            f = mod["f"]
            b = B()
            d = D()
            self.assertEqual(f(b), b)
            self.assertEqual(f(d), d)
            del B.f

            with self.assertRaises(AttributeError):
                f(b)
            with self.assertRaises(AttributeError):
                f(d)

    @skipIf(not HAS_INVOKE_METHOD, "no invoke method support")
    def test_override_remove_derived_method(self):
        codestr = """
        from typing import Optional
        class B:
            def f(self) -> "Optional[B]":
                return self

        class D(B):
            def f(self) -> Optional["B"]:
                return None

        def f(x: B):
            return x.f()
        """
        with self.in_module(codestr) as mod:
            B = mod["B"]
            D = mod["D"]
            f = mod["f"]
            b = B()
            d = D()
            self.assertEqual(f(b), b)
            self.assertEqual(f(d), None)
            del D.f

            self.assertEqual(f(b), b)
            self.assertEqual(f(d), d)

    @skipIf(not HAS_INVOKE_METHOD, "no invoke method support")
    def test_override_remove_method(self):
        codestr = """
        from typing import Optional
        class B:
            def f(self) -> "Optional[B]":
                return self

        def f(x: B):
            return x.f()
        """
        with self.in_module(codestr) as mod:
            B = mod["B"]
            f = mod["f"]
            b = B()
            self.assertEqual(f(b), b)
            del B.f

            with self.assertRaises(AttributeError):
                f(b)

    @skipIf(not HAS_INVOKE_METHOD, "no invoke method support")
    def test_override_remove_method_add_type_check(self):
        codestr = """
        from typing import Optional
        class B:
            def f(self) -> "B":
                return self

        def f(x: B):
            return x.f()
        """
        with self.in_module(codestr) as mod:
            B = mod["B"]
            f = mod["f"]
            b = B()
            self.assertEqual(f(b), b)
            del B.f

            with self.assertRaises(AttributeError):
                f(b)

            B.f = lambda self: None
            with self.assertRaises(TypeError):
                f(b)

    @skipIf(not HAS_INVOKE_METHOD, "no invoke method support")
    def test_override_update_derived(self):
        codestr = """
        from typing import Optional
        class B:
            def f(self) -> "Optional[B]":
                return self

        class D(B):
            pass

        def f(x: B):
            return x.f()
        """
        with self.in_module(codestr) as mod:
            B = mod["B"]
            D = mod["D"]
            f = mod["f"]

            b = B()
            d = D()
            self.assertEqual(f(b), b)
            self.assertEqual(f(d), d)

            B.f = lambda self: None
            self.assertEqual(f(b), None)
            self.assertEqual(f(d), None)

    @skipIf(not HAS_INVOKE_METHOD, "no invoke method support")
    def test_override_update_derived_2(self):
        codestr = """
        from typing import Optional
        class B:
            def f(self) -> "Optional[B]":
                return self

        class D1(B): pass

        class D(D1):
            pass

        def f(x: B):
            return x.f()
        """
        with self.in_module(codestr) as mod:
            B = mod["B"]
            D = mod["D"]
            f = mod["f"]

            b = B()
            d = D()
            self.assertEqual(f(b), b)
            self.assertEqual(f(d), d)

            B.f = lambda self: None
            self.assertEqual(f(b), None)
            self.assertEqual(f(d), None)

    def test_method_prologue(self):
        codestr = """
        def f(x: str):
            return 42
        """
        with self.in_module(codestr) as mod:
            f = mod["f"]
            self.assertInBytecode(f, "CHECK_ARGS", (0, ("builtins", "str")))
            with self.assertRaisesRegex(
                TypeError, ".*expected 'str' for argument x, got 'int'"
            ):
                f(42)

    def test_method_prologue_2(self):
        codestr = """
        def f(x, y: str):
            return 42
        """
        with self.in_module(codestr) as mod:
            f = mod["f"]
            self.assertInBytecode(f, "CHECK_ARGS", (1, ("builtins", "str")))
            with self.assertRaisesRegex(
                TypeError, ".*expected 'str' for argument y, got 'int'"
            ):
                f("abc", 42)

    def test_method_prologue_3(self):
        codestr = """
        def f(x: int, y: str):
            return 42
        """
        with self.in_module(codestr) as mod:
            f = mod["f"]
            self.assertInBytecode(
                f, "CHECK_ARGS", (0, ("builtins", "int"), 1, ("builtins", "str"))
            )
            with self.assertRaisesRegex(
                TypeError, ".*expected 'str' for argument y, got 'int'"
            ):
                f(42, 42)

    def test_method_prologue_shadowcode(self):
        codestr = """
        def f(x, y: str):
            return 42
        """
        with self.in_module(codestr) as mod:
            f = mod["f"]
            self.assertInBytecode(f, "CHECK_ARGS", (1, ("builtins", "str")))
            for _i in range(100):
                self.assertEqual(f("abc", "abc"), 42)
            with self.assertRaisesRegex(
                TypeError, ".*expected 'str' for argument y, got 'int'"
            ):
                f("abc", 42)

    def test_method_prologue_shadowcode_2(self):
        codestr = """
        def f(x: str):
            return 42
        """
        with self.in_module(codestr) as mod:
            f = mod["f"]
            self.assertInBytecode(f, "CHECK_ARGS", (0, ("builtins", "str")))
            for _i in range(100):
                self.assertEqual(f("abc"), 42)
            with self.assertRaisesRegex(
                TypeError, ".*expected 'str' for argument x, got 'int'"
            ):
                f(42)

    def test_method_prologue_no_annotation(self):
        codestr = """
        def f(x):
            return 42
        """
        with self.in_module(codestr) as mod:
            f = mod["f"]
            self.assertInBytecode(f, "CHECK_ARGS", ())
            self.assertEqual(f("abc"), 42)

    def test_method_prologue_kwonly(self):
        codestr = """
        def f(*, x: str):
            return 42
        """
        with self.in_module(codestr) as mod:
            f = mod["f"]
            self.assertInBytecode(f, "CHECK_ARGS", (0, ("builtins", "str")))
            with self.assertRaisesRegex(
                TypeError, "f expected 'str' for argument x, got 'int'"
            ):
                f(x=42)

    def test_method_prologue_kwonly_2(self):
        codestr = """
        def f(x, *, y: str):
            return 42
        """
        with self.in_module(codestr) as mod:
            f = mod["f"]
            self.assertInBytecode(f, "CHECK_ARGS", (1, ("builtins", "str")))
            with self.assertRaisesRegex(
                TypeError, "f expected 'str' for argument y, got 'object'"
            ):
                f(1, y=object())

    def test_method_prologue_kwonly_no_annotation(self):
        codestr = """
        def f(*, x):
            return 42
        """
        with self.in_module(codestr) as mod:
            f = mod["f"]
            self.assertInBytecode(f, "CHECK_ARGS", ())
            f(x=42)

    def test_package_no_parent(self):
        codestr = """
            class C:
                def f(self):
                    return 42
        """
        with self.in_module(
            codestr, code_gen=StaticCodeGenerator, name="package_no_parent.child"
        ) as mod:
            C = mod["C"]
            self.assertInBytecode(
                C.f, "CHECK_ARGS", (0, ("package_no_parent.child", "C"))
            )
            self.assertEqual(C().f(), 42)

    def test_direct_super_init(self):
        codestr = """
            class Obj:
                pass

            class C:
                def __init__(self, x: Obj):
                    pass

            class D:
                def __init__(self):
                    C.__init__(None)
        """
        with self.assertRaisesRegex(
            TypedSyntaxError,
            "type mismatch: builtins.None positional argument type mismatch foo.C",
        ):
            self.compile(codestr, StaticCodeGenerator, modname="foo")

    def test_class_unknown_attr(self):
        codestr = """
            class C:
                pass

            def f():
                return C.foo
        """
        with self.in_module(codestr) as mod:
            f = mod["f"]
            self.assertInBytecode(f, "LOAD_ATTR", "foo")

    def test_descriptor_access(self):
        codestr = """
            class Obj:
                abc: int

            class C:
                x: Obj

            def f():
                return C.x.abc
        """
        with self.in_module(codestr) as mod:
            f = mod["f"]
            self.assertInBytecode(f, "LOAD_ATTR", "abc")
            self.assertNotInBytecode(f, "LOAD_FIELD")

    @skipIf(not path.exists(RICHARDS_PATH), "richards not found")
    def test_richards(self):
        with open(RICHARDS_PATH) as f:
            codestr = f.read()

        with self.in_module(codestr) as mod:
            Richards = mod["Richards"]
            self.assertTrue(Richards().run(1))

    def test_unknown_isinstance_bool_ret(self):
        codestr = """
            from typing import Any

            class C:
                def __init__(self, x: str):
                    self.x: str = x

                def __eq__(self, other: Any) -> bool:
                    return isinstance(other, C)

        """
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            C = mod["C"]
            x = C("abc")
            y = C("foo")
            self.assertTrue(x == y)

    def test_unknown_issubclass_bool_ret(self):
        codestr = """
            from typing import Any

            class C:
                def __init__(self, x: str):
                    self.x: str = x

                def __eq__(self, other: Any) -> bool:
                    return issubclass(type(other), C)

        """
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            C = mod["C"]
            x = C("abc")
            y = C("foo")
            self.assertTrue(x == y)

    def test_unknown_isinstance_narrows(self):
        codestr = """
            from typing import Any

            class C:
                def __init__(self, x: str):
                    self.x: str = x

            def testfunc(x):
                if isinstance(x, C):
                    return x.x
        """
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            testfunc = mod["testfunc"]
            self.assertInBytecode(
                testfunc, "LOAD_FIELD", ("test_unknown_isinstance_narrows", "C", "x")
            )

    def test_unknown_isinstance_narrows_class_attr(self):
        codestr = """
            from typing import Any

            class C:
                def __init__(self, x: str):
                    self.x: str = x

                def f(self, other) -> str:
                    if isinstance(other, self.__class__):
                        return other.x
                    return ''
        """
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            C = mod["C"]
            self.assertInBytecode(
                C.f,
                "LOAD_FIELD",
                ("test_unknown_isinstance_narrows_class_attr", "C", "x"),
            )

    def test_unknown_isinstance_narrows_class_attr_dynamic(self):
        codestr = """
            from typing import Any

            class C:
                def __init__(self, x: str):
                    self.x: str = x

                def f(self, other, unknown):
                    if isinstance(other, unknown.__class__):
                        return other.x
                    return ''
        """
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            C = mod["C"]
            self.assertInBytecode(C.f, "LOAD_ATTR", "x")

    def test_unknown_isinstance_narrows_else_correct(self):
        codestr = """
            from typing import Any

            class C:
                def __init__(self, x: str):
                    self.x: str = x

            def testfunc(x):
                if isinstance(x, C):
                    pass
                else:
                    return x.x
        """
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            testfunc = mod["testfunc"]
            self.assertNotInBytecode(
                testfunc, "LOAD_FIELD", ("test_unknown_isinstance_narrows", "C", "x")
            )

    def test_unknown_param_ann(self):
        codestr = """
            from typing import Any

            class C:
                def __init__(self, x: str):
                    self.x: str = x

                def __eq__(self, other: Any) -> bool:
                    return False

        """
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            C = mod["C"]
            x = C("abc")
            self.assertInBytecode(
                C.__eq__, "CHECK_ARGS", (0, ("test_unknown_param_ann", "C"))
            )
            self.assertNotEqual(x, x)

    def test_class_init_kw(self):
        codestr = """
            class C:
                def __init__(self, x: str):
                    self.x: str = x

            def f():
                x = C(x='abc')

        """
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            f = mod["f"]
            self.assertInBytecode(f, "CALL_FUNCTION_KW", 1)

    def test_str_ret_type(self):
        codestr = """
            from typing import Any

            def testfunc(x: str, y: str) -> bool:
                return x == y
        """
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            f = mod["testfunc"]
            self.assertEqual(f("abc", "abc"), True)
            self.assertInBytecode(f, "CAST", ("builtins", "bool"))

    def test_bind_boolop_type(self):
        codestr = """
            from typing import Any

            class C:
                def f(self) -> bool:
                    return True

                def g(self) -> bool:
                    return False

                def x(self) -> bool:
                    return self.f() and self.g()

                def y(self) -> bool:
                    return self.f() or self.g()
        """
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            C = mod["C"]
            c = C()
            self.assertEqual(c.x(), False)
            self.assertEqual(c.y(), True)

    def test_decorated_function_ignored_class(self):
        codestr = """
            class C:
                @property
                def x(self):
                    return lambda: 42

                def y(self):
                    return self.x()

        """
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            C = mod["C"]
            self.assertNotInBytecode(C.y, "INVOKE_METHOD")
            self.assertEqual(C().y(), 42)

    def test_decorated_function_ignored(self):
        codestr = """
            class C: pass

            def mydecorator(x):
                return C

            @mydecorator
            def f():
                return 42

            def g():
                return f()

        """
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            C = mod["C"]
            g = mod["g"]
            self.assertNotInBytecode(g, "INVOKE_FUNCTION")
            self.assertEqual(type(g()), C)

    def test_spamobj_no_params(self):
        codestr = """
            from xxclassloader import spamobj

            def f():
                x = spamobj()
        """
        with self.assertRaisesRegex(
            TypedSyntaxError,
            "cannot create instances of a generic type xxclassloader.spamobj\[T\]",  # noqa: W605
        ):
            self.compile(codestr, StaticCodeGenerator, modname="foo")

    def test_named_tuple(self):
        codestr = """
            from typing import NamedTuple

            class C(NamedTuple):
                x: int
                y: str

            def myfunc(x: C):
                return x.x
        """
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            f = mod["myfunc"]
            self.assertNotInBytecode(f, "LOAD_FIELD")


class StaticRuntimeTests(StaticTestBase):
    def test_typed_slots_bad_inst(self):
        class C:
            __slots__ = ("a",)
            __slot_types__ = {"a": ("__static__", "int32")}

        class D:
            pass

        with self.assertRaises(TypeError):
            C.a.__get__(D(), D)

    def test_typed_slots_bad_slots(self):
        with self.assertRaises(TypeError):

            class C:
                __slots__ = ("a",)
                __slot_types__ = None

    def test_typed_slots_bad_slot_dict(self):
        with self.assertRaises(TypeError):

            class C:
                __slots__ = ("__dict__",)
                __slot_types__ = {"__dict__": "object"}

    def test_typed_slots_bad_slot_weakerf(self):
        with self.assertRaises(TypeError):

            class C:
                __slots__ = ("__weakref__",)
                __slot_types__ = {"__weakref__": "object"}

    def test_typed_slots_object(self):
        codestr = """
            class C:
                __slots__ = ('a', )
                __slot_types__ = {'a': ('test_typed_slots_object', 'C')}

            inst = C()
        """

        with self.in_module(codestr, code_gen=Python37CodeGenerator) as mod:
            inst, C = mod["inst"], mod["C"]
            self.assertEqual(C.a.__class__.__name__, "typed_descriptor")
            with self.assertRaises(TypeError):
                # type is checked
                inst.a = 42
            with self.assertRaises(TypeError):
                inst.a = None
            with self.assertRaises(AttributeError):
                # is initially unassigned
                inst.a

            # can assign correct type
            inst.a = inst

            # __sizeof__ doesn't include GC header size
            self.assertEqual(inst.__sizeof__(), self.base_size + self.ptr_size)
            # size is +2 words for GC header, one word for reference
            self.assertEqual(sys.getsizeof(inst), self.base_size + (self.ptr_size * 3))

            # subclasses are okay
            class D(C):
                pass

            inst.a = D()

    def test_generic_type_def_no_create(self):
        from xxclassloader import spamobj

        with self.assertRaises(TypeError):
            spamobj()

    def test_generic_type_def_bad_args(self):
        from xxclassloader import spamobj

        with self.assertRaises(TypeError):
            spamobj[str, int]

    def test_generic_type_def_non_type(self):
        from xxclassloader import spamobj

        with self.assertRaises(TypeError):
            spamobj[42]

    def test_generic_type_inst_okay(self):
        from xxclassloader import spamobj

        o = spamobj[str]()
        o.setstate("abc")

    def test_generic_type_inst_optional_okay(self):
        from xxclassloader import spamobj

        o = spamobj[Optional[str]]()
        o.setstate("abc")
        o.setstate(None)

    def test_generic_type_inst_non_optional_error(self):
        from xxclassloader import spamobj

        o = spamobj[str]()
        with self.assertRaises(TypeError):
            o.setstate(None)

    def test_generic_type_inst_bad_type(self):
        from xxclassloader import spamobj

        o = spamobj[str]()
        with self.assertRaises(TypeError):
            o.setstate(42)

    def test_generic_type_inst_name(self):
        from xxclassloader import spamobj

        self.assertEqual(spamobj[str].__name__, "spamobj[str]")

    def test_generic_type_inst_name_optional(self):
        from xxclassloader import spamobj

        self.assertEqual(spamobj[Optional[str]].__name__, "spamobj[Optional[str]]")

    def test_typed_slots_one_missing(self):
        codestr = """
            class C:
                __slots__ = ('a', 'b')
                __slot_types__ = {'a': ('test_typed_slots_object', 'C')}

            inst = C()
        """

        with self.in_module(codestr, code_gen=Python37CodeGenerator) as mod:
            inst, C = mod["inst"], mod["C"]
            self.assertEqual(C.a.__class__.__name__, "typed_descriptor")
            with self.assertRaises(TypeError):
                # type is checked
                inst.a = 42

    def test_typed_slots_optional_object(self):
        codestr = """
            class C:
                __slots__ = ('a', )
                __slot_types__ = {'a': ('test_typed_slots_optional_object', 'C', '?')}

            inst = C()
        """

        with self.in_module(codestr, code_gen=Python37CodeGenerator) as mod:
            inst = mod["inst"]
            inst.a = None
            self.assertEqual(inst.a, None)

    def test_typed_slots_private(self):
        codestr = """
            class C:
                __slots__ = ('__a', )
                __slot_types__ = {'__a': ('test_typed_slots_private', 'C', '?')}
                def __init__(self):
                    self.__a = None

            inst = C()
        """

        with self.in_module(codestr, code_gen=Python37CodeGenerator) as mod:
            inst = mod["inst"]
            self.assertEqual(inst._C__a, None)
            inst._C__a = inst
            self.assertEqual(inst._C__a, inst)
            inst._C__a = None
            self.assertEqual(inst._C__a, None)

    def test_typed_slots_optional_not_defined(self):
        codestr = """
            class C:
                __slots__ = ('a', )
                __slot_types__ = {'a': ('test_typed_slots_optional_object', 'D', '?')}

                def __init__(self):
                    self.a = None

            inst = C()

            class D:
                pass
        """

        with self.in_module(codestr, code_gen=Python37CodeGenerator) as mod:
            inst = mod["inst"]
            inst.a = None
            self.assertEqual(inst.a, None)

    def test_typed_slots_alignment(self):
        return
        codestr = """
            class C:
                __slots__ = ('a', 'b')
                __slot_types__ {'a': ('__static__', 'int16')}

            inst = C()
        """

        with self.in_module(codestr, code_gen=Python37CodeGenerator) as mod:
            inst = mod["inst"]
            inst.a = None
            self.assertEqual(inst.a, None)

    def test_typed_slots_primitives(self):
        slot_types = [
            # signed
            (
                ("__static__", "byte"),
                0,
                1,
                [(1 << 7) - 1, -(1 << 7)],
                [1 << 8],
                ["abc"],
            ),
            (
                ("__static__", "int8"),
                0,
                1,
                [(1 << 7) - 1, -(1 << 7)],
                [1 << 8],
                ["abc"],
            ),
            (
                ("__static__", "int16"),
                0,
                2,
                [(1 << 15) - 1, -(1 << 15)],
                [1 << 15, -(1 << 15) - 1],
                ["abc"],
            ),
            (
                ("__static__", "int32"),
                0,
                4,
                [(1 << 31) - 1, -(1 << 31)],
                [1 << 31, -(1 << 31) - 1],
                ["abc"],
            ),
            (("__static__", "int64"), 0, 8, [(1 << 63) - 1, -(1 << 63)], [], [1 << 63]),
            # unsigned
            (("__static__", "uint8"), 0, 1, [(1 << 8) - 1, 0], [1 << 8, -1], ["abc"]),
            (
                ("__static__", "uint16"),
                0,
                2,
                [(1 << 16) - 1, 0],
                [1 << 16, -1],
                ["abc"],
            ),
            (
                ("__static__", "uint32"),
                0,
                4,
                [(1 << 32) - 1, 0],
                [1 << 32, -1],
                ["abc"],
            ),
            (("__static__", "uint64"), 0, 8, [(1 << 64) - 1, 0], [], [1 << 64]),
            # pointer
            (
                ("__static__", "ssizet"),
                0,
                self.ptr_size,
                [1, sys.maxsize, -sys.maxsize - 1],
                [],
                [sys.maxsize + 1, -sys.maxsize - 2],
            ),
            # floating point
            (("__static__", "single"), 0.0, 4, [1.0], [], ["abc"]),
            (("__static__", "double"), 0.0, 8, [1.0], [], ["abc"]),
            # misc
            (("__static__", "char"), "\x00", 1, ["a"], [], ["abc"]),
            (("__static__", "cbool"), False, 1, [True], [], ["abc", 1]),
        ]

        base_size = self.base_size
        for type_spec, default, size, test_vals, warn_vals, err_vals in slot_types:

            class C:
                __slots__ = ("a",)
                __slot_types__ = {"a": type_spec}

            a = C()
            self.assertEqual(sys.getsizeof(a), base_size + size, type)
            self.assertEqual(a.a, default)
            self.assertEqual(type(a.a), type(default))
            for val in test_vals:
                a.a = val
                self.assertEqual(a.a, val)

            with warnings.catch_warnings():
                warnings.simplefilter("error", category=RuntimeWarning)
                for val in warn_vals:
                    with self.assertRaises(RuntimeWarning):
                        a.a = val

            for val in err_vals:
                with self.assertRaises((TypeError, OverflowError)):
                    a.a = val

    def test_load_iterable_arg(self):
        codestr = """
        def x(a: int, b: int, c: str, d: float, e: float) -> int:
            return 7

        def y() -> int:
            p = ("hi", 0.1, 0.2)
            return x(1, 3, *p)
        """
        y = self.find_code(
            self.compile(codestr, StaticCodeGenerator, modname="foo"), name="y"
        )
        self.assertInBytecode(y, "LOAD_ITERABLE_ARG", 0)
        self.assertInBytecode(y, "LOAD_ITERABLE_ARG", 1)
        self.assertInBytecode(y, "LOAD_ITERABLE_ARG", 2)
        self.assertNotInBytecode(y, "LOAD_ITERABLE_ARG", 3)
        with self.in_module(codestr) as mod:
            y_callable = mod["y"]
            self.assertEqual(y_callable(), 7)

    def test_load_iterable_arg_default_overridden(self):
        codestr = """
            def x(a: int, b: int, c: str, d: float = 10.1, e: float = 20.1) -> bool:
                return bool(
                    a == 1
                    and b == 3
                    and c == "hi"
                    and d == 0.1
                    and e == 0.2
                )

            def y() -> bool:
                p = ("hi", 0.1, 0.2)
                return x(1, 3, *p)
        """
        y = self.find_code(
            self.compile(codestr, StaticCodeGenerator, modname="foo"), name="y"
        )
        self.assertNotInBytecode(y, "LOAD_ITERABLE_ARG", 3)
        self.assertNotInBytecode(y, "LOAD_MAPPING_ARG", 3)
        with self.in_module(codestr) as mod:
            y_callable = mod["y"]
            self.assertTrue(y_callable())

    def test_load_iterable_arg_multi_star(self):
        codestr = """
        def x(a: int, b: int, c: str, d: float, e: float) -> int:
            return 7

        def y() -> int:
            p = (1, 3)
            q = ("hi", 0.1, 0.2)
            return x(*p, *q)
        """
        y = self.find_code(
            self.compile(codestr, StaticCodeGenerator, modname="foo"), name="y"
        )
        # we should fallback to the py37 compiler for this
        self.assertNotInBytecode(y, "LOAD_ITERABLE_ARG")
        with self.in_module(codestr) as mod:
            y_callable = mod["y"]
            self.assertEqual(y_callable(), 7)

    def test_load_iterable_arg_failure(self):
        codestr = """
        def x(a: int, b: int, c: str, d: float, e: float) -> int:
            return 7

        def y() -> int:
            p = ("hi", 0.1)
            return x(1, 3, *p)
        """
        y = self.find_code(
            self.compile(codestr, StaticCodeGenerator, modname="foo"), name="y"
        )
        self.assertInBytecode(y, "LOAD_ITERABLE_ARG", 0)
        self.assertInBytecode(y, "LOAD_ITERABLE_ARG", 1)
        self.assertInBytecode(y, "LOAD_ITERABLE_ARG", 2)
        self.assertNotInBytecode(y, "LOAD_ITERABLE_ARG", 3)
        with self.in_module(codestr) as mod:
            y_callable = mod["y"]
            with self.assertRaises(IndexError):
                y_callable()

    def test_load_iterable_arg_sequence(self):
        codestr = """
        def x(a: int, b: int, c: str, d: float, e: float) -> int:
            return 7

        def y() -> int:
            p = ["hi", 0.1, 0.2]
            return x(1, 3, *p)
        """
        y = self.find_code(
            self.compile(codestr, StaticCodeGenerator, modname="foo"), name="y"
        )
        self.assertInBytecode(y, "LOAD_ITERABLE_ARG", 0)
        self.assertInBytecode(y, "LOAD_ITERABLE_ARG", 1)
        self.assertInBytecode(y, "LOAD_ITERABLE_ARG", 2)
        self.assertNotInBytecode(y, "LOAD_ITERABLE_ARG", 3)
        with self.in_module(codestr) as mod:
            y_callable = mod["y"]
            self.assertEqual(y_callable(), 7)

    def test_load_iterable_arg_sequence_1(self):
        codestr = """
        def x(a: int, b: int, c: str, d: float, e: float) -> int:
            return 7

        def gen():
            for i in ["hi", 0.05, 0.2]:
                yield i

        def y() -> int:
            g = gen()
            return x(1, 3, *g)
        """
        y = self.find_code(
            self.compile(codestr, StaticCodeGenerator, modname="foo"), name="y"
        )
        self.assertInBytecode(y, "LOAD_ITERABLE_ARG", 0)
        self.assertInBytecode(y, "LOAD_ITERABLE_ARG", 1)
        self.assertInBytecode(y, "LOAD_ITERABLE_ARG", 2)
        self.assertNotInBytecode(y, "LOAD_ITERABLE_ARG", 3)
        with self.in_module(codestr) as mod:
            y_callable = mod["y"]
            self.assertEqual(y_callable(), 7)

    def test_load_iterable_arg_sequence_failure(self):
        codestr = """
        def x(a: int, b: int, c: str, d: float, e: float) -> int:
            return 7

        def y() -> int:
            p = ["hi", 0.1]
            return x(1, 3, *p)
        """
        y = self.find_code(
            self.compile(codestr, StaticCodeGenerator, modname="foo"), name="y"
        )
        self.assertInBytecode(y, "LOAD_ITERABLE_ARG", 0)
        self.assertInBytecode(y, "LOAD_ITERABLE_ARG", 1)
        self.assertInBytecode(y, "LOAD_ITERABLE_ARG", 2)
        self.assertNotInBytecode(y, "LOAD_ITERABLE_ARG", 3)
        with self.in_module(codestr) as mod:
            y_callable = mod["y"]
            with self.assertRaises(IndexError):
                y_callable()

    def test_load_mapping_arg(self):
        codestr = """
        def x(a: int, b: int, c: str, d: float=-0.1, e: float=1.1, f: str="something") -> bool:
            return bool(f == "yo" and d == 1.0 and e == 1.1)

        def y() -> bool:
            d = {"d": 1.0}
            return x(1, 3, "hi", f="yo", **d)
        """
        y = self.find_code(
            self.compile(codestr, StaticCodeGenerator, modname="foo"), name="y"
        )
        self.assertInBytecode(y, "LOAD_MAPPING_ARG", 3)
        with self.in_module(codestr) as mod:
            y_callable = mod["y"]
            self.assertTrue(y_callable())

    def test_load_mapping_and_iterable_args_failure_1(self):
        """
        Fails because we don't supply enough positional args
        """

        codestr = """
        def x(a: int, b: int, c: str, d: float=2.2, e: float=1.1, f: str="something") -> bool:
            return bool(a == 1 and b == 3 and f == "yo" and d == 2.2 and e == 1.1)

        def y() -> bool:
            return x(1, 3, f="yo")
        """
        with self.assertRaisesRegex(
            SyntaxError, "Function foo.x expects a value for argument c"
        ):
            self.compile(codestr, StaticCodeGenerator, modname="foo")

    def test_load_mapping_arg_failure(self):
        """
        Fails because we supply an extra kwarg
        """
        codestr = """
        def x(a: int, b: int, c: str, d: float=2.2, e: float=1.1, f: str="something") -> bool:
            return bool(a == 1 and b == 3 and f == "yo" and d == 2.2 and e == 1.1)

        def y() -> bool:
            return x(1, 3, "hi", f="yo", g="lol")
        """
        with self.assertRaisesRegex(
            TypedSyntaxError,
            "Given argument g does not exist in the definition of foo.x",
        ):
            self.compile(codestr, StaticCodeGenerator, modname="foo")

    def test_load_mapping_arg_custom_class(self):
        """
        Fails because we supply a custom class for the mapped args, instead of a dict
        """
        codestr = """
        def x(a: int, b: int, c: str="hello") -> bool:
            return bool(a == 1 and b == 3 and c == "hello")

        class C:
            def __getitem__(self, key: str) -> str:
                if key == "c":
                    return "hi"

            def keys(self):
                return ["c"]

        def y() -> bool:
            return x(1, 3, **C())
        """
        with self.in_module(codestr) as mod:
            y_callable = mod["y"]
            with self.assertRaisesRegex(
                TypeError, r"argument after \*\* must be a dict, not C"
            ):
                self.assertTrue(y_callable())

    def test_load_mapping_arg_use_defaults(self):
        codestr = """
        def x(a: int, b: int, c: str, d: float=-0.1, e: float=1.1, f: str="something") -> bool:
            return bool(f == "yo" and d == -0.1 and e == 1.1)

        def y() -> bool:
            d = {"d": 1.0}
            return x(1, 3, "hi", f="yo")
        """
        y = self.find_code(
            self.compile(codestr, StaticCodeGenerator, modname="foo"), name="y"
        )
        self.assertInBytecode(y, "LOAD_CONST", 1.1)
        with self.in_module(codestr) as mod:
            y_callable = mod["y"]
            self.assertTrue(y_callable())

    def test_default_arg_non_const(self):
        codestr = """
        class C: pass
        def x(val=C()) -> C:
            return val

        def f() -> C:
            return x()
        """
        with self.in_module(codestr) as mod:
            f = mod["f"]
            self.assertInBytecode(f, "CALL_FUNCTION")

    def test_default_arg_non_const_kw_provided(self):
        codestr = """
        class C: pass
        def x(val:object=C()):
            return val

        def f():
            return x(val=42)
        """

        with self.in_module(codestr) as mod:
            f = mod["f"]
            self.assertEqual(f(), 42)

    def test_load_mapping_arg_order(self):
        codestr = """
        def x(a: int, b: int, c: str, d: float=-0.1, e: float=1.1, f: str="something") -> bool:
            return bool(
                a == 1
                and b == 3
                and c == "hi"
                and d == 1.1
                and e == 3.3
                and f == "hmm"
            )

        stuff = []
        def q() -> float:
            stuff.append("q")
            return 1.1

        def r() -> float:
            stuff.append("r")
            return 3.3

        def s() -> str:
            stuff.append("s")
            return "hmm"

        def y() -> bool:
            return x(1, 3, "hi", f=s(), d=q(), e=r())
        """
        y = self.find_code(
            self.compile(codestr, StaticCodeGenerator, modname="foo"), name="y"
        )
        self.assertInBytecode(y, "STORE_FAST", "_pystatic_.0._tmp__d")
        self.assertInBytecode(y, "LOAD_FAST", "_pystatic_.0._tmp__d")
        with self.in_module(codestr) as mod:
            y_callable = mod["y"]
            self.assertTrue(y_callable())
            self.assertEqual(["s", "q", "r"], mod["stuff"])

    def test_load_mapping_arg_order_with_variadic_kw_args(self):
        codestr = """
        def x(a: int, b: int, c: str, d: float=-0.1, e: float=1.1, f: str="something", g: str="look-here") -> bool:
            return bool(
                a == 1
                and b == 3
                and c == "hi"
                and d == 1.1
                and e == 3.3
                and f == "hmm"
                and g == "overridden"
            )

        stuff = []
        def q() -> float:
            stuff.append("q")
            return 1.1

        def r() -> float:
            stuff.append("r")
            return 3.3

        def s() -> str:
            stuff.append("s")
            return "hmm"

        def y() -> bool:
            kw = {"g": "overridden"}
            return x(1, 3, "hi", f=s(), **kw, d=q(), e=r())
        """
        y = self.find_code(
            self.compile(codestr, StaticCodeGenerator, modname="foo"), name="y"
        )
        self.assertInBytecode(y, "STORE_FAST", "_pystatic_.0._tmp__d")
        self.assertInBytecode(y, "LOAD_FAST", "_pystatic_.0._tmp__d")
        with self.in_module(codestr) as mod:
            y_callable = mod["y"]
            self.assertTrue(y_callable())
            self.assertEqual(["s", "q", "r"], mod["stuff"])

    def test_load_mapping_arg_order_with_variadic_kw_args_one_positional(self):
        codestr = """
        def x(a: int, b: int, c: str, d: float=-0.1, e: float=1.1, f: str="something", g: str="look-here") -> bool:
            return bool(
                a == 1
                and b == 3
                and c == "hi"
                and d == 1.1
                and e == 3.3
                and f == "hmm"
                and g == "overridden"
            )

        stuff = []
        def q() -> float:
            stuff.append("q")
            return 1.1

        def r() -> float:
            stuff.append("r")
            return 3.3

        def s() -> str:
            stuff.append("s")
            return "hmm"


        def y() -> bool:
            kw = {"g": "overridden"}
            return x(1, 3, "hi", 1.1, f=s(), **kw, e=r())
        """
        y = self.find_code(
            self.compile(codestr, StaticCodeGenerator, modname="foo"), name="y"
        )
        self.assertNotInBytecode(y, "STORE_FAST", "_pystatic_.0._tmp__d")
        self.assertNotInBytecode(y, "LOAD_FAST", "_pystatic_.0._tmp__d")
        with self.in_module(codestr) as mod:
            y_callable = mod["y"]
            self.assertTrue(y_callable())
            self.assertEqual(["s", "r"], mod["stuff"])

    def test_array_len(self):
        codestr = """
            from __static__ import int64, char, double, Array
            from array import array

            def y():
                return len(Array[int64]([1, 3, 5]))
        """
        y = self.find_code(
            self.compile(codestr, StaticCodeGenerator, modname="foo"), name="y"
        )
        self.assertInBytecode(y, "FAST_LEN")
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            y = mod["y"]
            self.assertEqual(y(), 3)

    def test_nonarray_len(self):
        codestr = """
            from __static__ import int64, char, double, Array
            from array import array

            def y():
                return len([1, 3, 5])
        """
        y = self.find_code(
            self.compile(codestr, StaticCodeGenerator, modname="foo"), name="y"
        )
        self.assertNotInBytecode(y, "FAST_LEN")
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            y = mod["y"]
            self.assertEqual(y(), 3)

    def test_return_primitive(self):
        codestr = """
            from __static__ import int16

            def y() -> int16:
                x: int16 = -12
                return x
        """
        y = self.find_code(
            self.compile(codestr, StaticCodeGenerator, modname="foo"), name="y"
        )
        self.assertInBytecode(y, "INT_BOX")
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            y = mod["y"]
            self.assertEqual(y(), -12)

    def test_load_int_const_signed(self):
        int_types = [
            "int8",
            "int16",
            "int32",
            "int64",
        ]
        signs = ["-", ""]
        values = [12]

        for type, sign, value in itertools.product(int_types, signs, values):
            expected = value if sign == "" else -value

            codestr = f"""
                from __static__ import {type}

                def y() -> {type}:
                    return {type}({sign}{value})
            """
            y = self.find_code(
                self.compile(codestr, StaticCodeGenerator, modname="foo"), name="y"
            )
            self.assertNotInBytecode(y, "CALL_FUNCTION")
            self.assertNotInBytecode(y, "LOAD_GLOBAL")
            self.assertInBytecode(y, "INT_LOAD_CONST")
            if sign:
                self.assertInBytecode(y, "INT_UNARY_OP")
            else:
                self.assertNotInBytecode(y, "INT_UNARY_OP")
            self.assertInBytecode(y, "INT_BOX")
            with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
                y = mod["y"]
                self.assertEqual(y(), expected)

    def test_load_int_const_unsigned(self):
        int_types = [
            "uint8",
            "uint16",
            "uint32",
            "uint64",
        ]
        values = [12]

        for type, value in itertools.product(int_types, values):
            expected = value
            codestr = f"""
                from __static__ import {type}

                def y() -> {type}:
                    return {type}({value})
            """
            y = self.find_code(
                self.compile(codestr, StaticCodeGenerator, modname="foo"), name="y"
            )
            self.assertNotInBytecode(y, "CALL_FUNCTION")
            self.assertNotInBytecode(y, "LOAD_GLOBAL")
            self.assertInBytecode(y, "INT_LOAD_CONST")
            self.assertNotInBytecode(y, "INT_UNARY_OP")
            self.assertInBytecode(y, "INT_BOX")
            with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
                y = mod["y"]
                self.assertEqual(y(), expected)

    def test_load_int_indirectly(self):
        value = 42
        expected = value
        codestr = f"""
            from __static__ import int32

            def x() -> int32:
                i: int32 = {value}
                return i

            def y() -> int32:
                return int32(x())
        """
        y = self.find_code(
            self.compile(codestr, StaticCodeGenerator, modname="foo"), name="y"
        )
        self.assertNotInBytecode(y, "INT_LOAD_CONST")
        self.assertInBytecode(y, "INVOKE_FUNCTION")
        with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
            y = mod["y"]
            self.assertEqual(y(), expected)

    def test_primitive_conversions(self):
        cases = [
            ("int8", "int8", 5, 5),
            ("int8", "int16", 5, 5),
            ("int8", "int32", 5, 5),
            ("int8", "int64", 5, 5),
            ("int8", "uint8", -1, 255),
            ("int8", "uint8", 12, 12),
            ("int8", "uint16", -1, 65535),
            ("int8", "uint16", 12, 12),
            ("int8", "uint32", -1, 4294967295),
            ("int8", "uint32", 12, 12),
            ("int8", "uint64", -1, 18446744073709551615),
            ("int8", "uint64", 12, 12),
            ("int16", "int8", 5, 5),
            ("int16", "int8", -1, -1),
            ("int16", "int8", 65535, -1),
            ("int16", "int16", 5, 5),
            ("int16", "int32", -5, -5),
            ("int16", "int64", -6, -6),
            ("int16", "uint8", 65535, 255),
            ("int16", "uint8", -1, 255),
            ("int16", "uint16", 65535, 65535),
            ("int16", "uint16", -1, 65535),
            ("int16", "uint32", 1000, 1000),
            ("int16", "uint32", -1, 4294967295),
            ("int16", "uint64", 1414, 1414),
            ("int16", "uint64", -1, 18446744073709551615),
            ("int32", "int8", 5, 5),
            ("int32", "int8", -1, -1),
            ("int32", "int8", 4294967295, -1),
            ("int32", "int16", 5, 5),
            ("int32", "int16", -1, -1),
            ("int32", "int16", 4294967295, -1),
            ("int32", "int32", 5, 5),
            ("int32", "int64", 5, 5),
            ("int32", "uint8", 5, 5),
            ("int32", "uint8", 65535, 255),
            ("int32", "uint8", -1, 255),
            ("int32", "uint16", 5, 5),
            ("int32", "uint16", 4294967295, 65535),
            ("int32", "uint16", -1, 65535),
            ("int32", "uint32", 5, 5),
            ("int32", "uint32", -1, 4294967295),
            ("int32", "uint64", 5, 5),
            ("int32", "uint64", -1, 18446744073709551615),
            ("int64", "int8", 5, 5),
            ("int64", "int8", -1, -1),
            ("int64", "int8", 65535, -1),
            ("int64", "int16", 5, 5),
            ("int64", "int16", -1, -1),
            ("int64", "int16", 4294967295, -1),
            ("int64", "int32", 5, 5),
            ("int64", "int32", -1, -1),
            ("int64", "int32", 18446744073709551615, -1),
            ("int64", "int64", 5, 5),
            ("int64", "uint8", 5, 5),
            ("int64", "uint8", 65535, 255),
            ("int64", "uint8", -1, 255),
            ("int64", "uint16", 5, 5),
            ("int64", "uint16", 4294967295, 65535),
            ("int64", "uint16", -1, 65535),
            ("int64", "uint32", 5, 5),
            ("int64", "uint32", 18446744073709551615, 4294967295),
            ("int64", "uint32", -1, 4294967295),
            ("int64", "uint64", 5, 5),
            ("int64", "uint64", -1, 18446744073709551615),
            ("uint8", "int8", 5, 5),
            ("uint8", "int8", 255, -1),
            ("uint8", "int16", 255, 255),
            ("uint8", "int32", 255, 255),
            ("uint8", "int64", 255, 255),
            ("uint8", "uint8", 5, 5),
            ("uint8", "uint16", 255, 255),
            ("uint8", "uint32", 255, 255),
            ("uint8", "uint64", 255, 255),
            ("uint16", "int8", 5, 5),
            ("uint16", "int8", 65535, -1),
            ("uint16", "int16", 5, 5),
            ("uint16", "int16", 65535, -1),
            ("uint16", "int32", 65535, 65535),
            ("uint16", "int64", 65535, 65535),
            ("uint16", "uint8", 65535, 255),
            ("uint16", "uint16", 65535, 65535),
            ("uint16", "uint32", 65535, 65535),
            ("uint16", "uint64", 65535, 65535),
            ("uint32", "int8", 4, 4),
            ("uint32", "int8", 4294967295, -1),
            ("uint32", "int16", 5, 5),
            ("uint32", "int16", 4294967295, -1),
            ("uint32", "int32", 65535, 65535),
            ("uint32", "int32", 4294967295, -1),
            ("uint32", "int64", 4294967295, 4294967295),
            ("uint32", "uint8", 4, 4),
            ("uint32", "uint8", 65535, 255),
            ("uint32", "uint16", 4294967295, 65535),
            ("uint32", "uint32", 5, 5),
            ("uint32", "uint64", 4294967295, 4294967295),
            ("uint64", "int8", 4, 4),
            ("uint64", "int8", 18446744073709551615, -1),
            ("uint64", "int16", 4, 4),
            ("uint64", "int16", 18446744073709551615, -1),
            ("uint64", "int32", 4, 4),
            ("uint64", "int32", 18446744073709551615, -1),
            ("uint64", "int64", 4, 4),
            ("uint64", "int64", 18446744073709551615, -1),
            ("uint64", "uint8", 5, 5),
            ("uint64", "uint8", 65535, 255),
            ("uint64", "uint16", 4294967295, 65535),
            ("uint64", "uint32", 18446744073709551615, 4294967295),
            ("uint64", "uint64", 5, 5),
        ]

        for src, dest, val, expected in cases:
            codestr = f"""
                from __static__ import {src}, {dest}

                def y() -> {dest}:
                    x = {dest}({src}({val}))
                    return x
            """
            with self.in_module(codestr, code_gen=StaticCodeGenerator) as mod:
                y = mod["y"]
                actual = y()
                self.assertEqual(
                    actual,
                    expected,
                    f"failing case: {[src, dest, val, actual, expected]}",
                )


if __name__ == "__main__":
    unittest.main()
