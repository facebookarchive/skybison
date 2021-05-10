#!/usr/bin/env python3
import unittest

from test_support import pyro_only


class GCModuleTest(unittest.TestCase):
    @pyro_only
    def test_gc_immortalize_moves_objects_to_immortal_partition(self):
        import gc

        from _builtins import _gc

        # make sure that garbage collection has run at least once
        # which ensures that any code object will be transported
        # to the immortal heap
        _gc()

        # That makes code objects and sub-objects immortal
        def test():
            return

        code = test.__code__
        self.assertTrue(gc._is_immortal(code))
        self.assertTrue(gc._is_immortal(code.co_consts))

        value = [11, 22, 33]
        self.assertFalse(gc._is_immortal(value))
        self.assertFalse(gc._is_immortal(GCModuleTest))

        # We can only call immortalize_heap in one test since its effect
        # is permanent
        gc.immortalize_heap()
        self.assertTrue(gc._is_immortal(value))
        self.assertTrue(gc._is_immortal(GCModuleTest))

        other_value = [44, 55, 66]
        self.assertFalse(gc._is_immortal(other_value))

        # ... but we can call it multiple times to move more things into
        # the immortal partition
        gc.immortalize_heap()
        self.assertTrue(gc._is_immortal(other_value))
        self.assertTrue(gc._is_immortal(value))
        self.assertTrue(gc._is_immortal(GCModuleTest))


if __name__ == "__main__":
    unittest.main()
