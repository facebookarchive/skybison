#!/usr/bin/env python3
# Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com)
import unittest


class SubprocessTest(unittest.TestCase):
    def test_import(self):
        import subprocess

        self.assertEqual(subprocess.__name__, "subprocess")


if __name__ == "__main__":
    unittest.main()
