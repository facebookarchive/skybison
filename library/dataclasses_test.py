#!/usr/bin/env python3
import unittest

from test_support import pyro_only


# TODO(T61740328): Remove @pyro_only once 3.7 hits
@pyro_only
class DataclassesTest(unittest.TestCase):
    def test_simple_dataclass(self):
        from dataclasses import dataclass

        @dataclass
        class C:
            foo: str

        c = C("bar")
        self.assertEqual(c.foo, "bar")


if __name__ == "__main__":
    unittest.main()
