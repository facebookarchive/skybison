#!/usr/bin/env python3
# Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com)
import unittest


class SelectorsTest(unittest.TestCase):
    def test_import(self):
        import selectors

        self.assertEqual(selectors.__name__, "selectors")


if __name__ == "__main__":
    unittest.main()
