#!/usr/bin/env python3
# Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com)
import unittest


class PlatformTest(unittest.TestCase):
    def test_import(self):
        import platform

        self.assertEqual(platform.__name__, "platform")


if __name__ == "__main__":
    unittest.main()
