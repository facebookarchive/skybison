#!/usr/bin/env python3
import unittest
import warnings


class AttributesTest(unittest.TestCase):
    def test_cr_running(self):
        async def coro():
            return 1

        # TODO(bsimmers): Test more once we have coroutine.__await__()
        cr = coro()
        self.assertFalse(cr.cr_running)
        # TODO(T42623564): Remove this warning filter and call cr.close().
        warnings.filterwarnings(
            action="ignore",
            category=RuntimeWarning,
            message="coroutine.*was never awaited",
        )


class MethodTest(unittest.TestCase):
    def test_dunder_repr(self):
        async def foo():
            return 5

        cr = foo()
        # TODO(T42623564): Remove this warning filter and call cr.close().
        warnings.filterwarnings(
            action="ignore",
            category=RuntimeWarning,
            message="coroutine.*was never awaited",
        )
        self.assertTrue(
            cr.__repr__().startswith(
                "<coroutine object MethodTest.test_dunder_repr.<locals>.foo at "
            )
        )


if __name__ == "__main__":
    unittest.main()
