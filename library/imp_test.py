#!/usr/bin/env python3
# Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com)
import unittest


class ImpModuleTest(unittest.TestCase):
    def test_it_imports(self):
        import imp

        self.assertEqual(imp.__name__, "imp")


if __name__ == "__main__":
    unittest.main()
