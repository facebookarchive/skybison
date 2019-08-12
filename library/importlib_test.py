#!/usr/bin/env python3
import importlib
import unittest


class ImportlibTests(unittest.TestCase):
    def test_importlib_can_import(self):
        warnings_module = importlib.import_module("warnings")
        self.assertEqual(warnings_module.__name__, "warnings")


if __name__ == "__main__":
    unittest.main()
