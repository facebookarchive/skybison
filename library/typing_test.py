#!/usr/bin/env python3
# flake8: noqa

import unittest


# Temporarily disabled until next change.
# from typing import Dict, List, NamedTuple, Optional, Union


@unittest.skip("temporarily disabled")
class TypingTests(unittest.TestCase):
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
