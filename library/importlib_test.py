#!/usr/bin/env python3
import importlib
import unittest


class ImportlibTests(unittest.TestCase):
    def test_importlib_can_import(self):
        warnings_module = importlib.import_module("warnings")
        self.assertEqual(warnings_module.__name__, "warnings")

    def test_import_machinery(self):
        import importlib.machinery as machinery_module

        self.assertEqual(machinery_module.__name__, "importlib.machinery")


if __name__ == "__main__":
    unittest.main()
