#!/usr/bin/env python3
import sys
import unittest
from typing import Dict, Generic, List, NamedTuple, Optional, TypeVar, Union
from unittest import skipIf


class TypingTests(unittest.TestCase):
    @skipIf(
        sys.implementation.name == "cpython" and sys.version_info[:2] < (3, 7),
        "requires at least CPython 3.7",
    )
    def test_generic_subclass_with_metaclass(self):
        class M(type):
            pass

        A = TypeVar("A")
        B = TypeVar("B")

        class C(Generic[A, B], metaclass=M):
            pass

        self.assertIs(type(C), M)
        self.assertTrue(issubclass(C, Generic))
        self.assertTrue(isinstance(C(), Generic))

    def test_typing_parameterized_generics(self):
        def fn(val: Optional[Union[Dict, List]]):
            return 5

        self.assertEqual(fn(None), 5)

    def test_namedtuple_keeps_textual_order_of_fields(self):
        class C(NamedTuple):
            zoozle: int
            aardvark: str = ""
            foo: int = 1
            bar: int = 2
            baz: int = 3

        c = C(999, "abc", -1, -2, -3)
        self.assertIs(c.zoozle, 999)
        self.assertIs(c.aardvark, "abc")
        self.assertIs(c.foo, -1)
        self.assertIs(c.bar, -2)
        self.assertIs(c.baz, -3)

    def test_namedtuple_populates_default_values(self):
        class C(NamedTuple):
            zoozle: int
            aardvark: str = ""
            foo: int = 1
            bar: int = 2
            baz: int = 3

        c = C(999)
        self.assertIs(c.zoozle, 999)
        self.assertIs(c.aardvark, "")
        self.assertIs(c.foo, 1)
        self.assertIs(c.bar, 2)
        self.assertIs(c.baz, 3)


if __name__ == "__main__":
    unittest.main()
