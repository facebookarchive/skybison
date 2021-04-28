#!/usr/bin/env python3
import unittest

from test_support import pyro_only


class GCModuleTest(unittest.TestCase):
    def test_it_imports(self):
        import gc

        self.assertEqual(gc.__name__, "gc")

    @pyro_only
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
