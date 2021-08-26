#!/usr/bin/env python3
# Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com)
import unittest


class TermiosModuleTest(unittest.TestCase):
    def test_it_imports(self):
        import termios

        self.assertEqual(termios.__name__, "termios")


if __name__ == "__main__":
    unittest.main()
