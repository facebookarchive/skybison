#!/usr/bin/env python3
# Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com)
import unittest


class FcntlModuleTest(unittest.TestCase):
    def test_it_imports(self):
        import fcntl

        self.assertEqual(fcntl.__name__, "fcntl")


if __name__ == "__main__":
    unittest.main()
