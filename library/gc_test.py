#!/usr/bin/env python3
import sys
import unittest


class GCModuleTest(unittest.TestCase):
    def test_it_imports(self):
        import gc

        self.assertEqual(gc.__name__, "gc")

    @unittest.skipIf(sys.implementation.name != "pyro", "cpython does s not support")
    def test_can_immortalize(self):
        import gc

        value = (11, 22, 33)
        self.assertFalse(gc._is_immortal(value))
        self.assertFalse(gc._is_immortal(GCModuleTest))

        gc.immortalize_heap()
        self.assertTrue(gc._is_immortal(value))
        self.assertTrue(gc._is_immortal(GCModuleTest))


if __name__ == "__main__":
    unittest.main()
