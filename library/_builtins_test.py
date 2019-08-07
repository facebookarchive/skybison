#!/usr/bin/env python3
import unittest


# These values are injected by our boot process. flake8 has no knowledge about
# their definitions and will complain without these circular assignments.
_number_check = _number_check  # noqa: F821


class UnderNumberCheckTests(unittest.TestCase):
    def test_number_check_with_builtin_number_returns_true(self):
        self.assertTrue(_number_check(2))
        self.assertTrue(_number_check(False))
        self.assertTrue(_number_check(5.0))

    def test_number_check_without_class_method_returns_false(self):
        class Foo:
            pass

        foo = Foo()
        foo.__float__ = lambda: 1.0
        foo.__int__ = lambda: 2
        self.assertFalse(_number_check(foo))

    def test_number_check_with_dunder_index_descriptor_does_not_call(self):
        class Raise:
            def __get__(self, obj, type):
                raise AttributeError("bad")

        class FloatLike:
            __float__ = Raise()

        class IntLike:
            __int__ = Raise()

        self.assertTrue(_number_check(FloatLike()))
        self.assertTrue(_number_check(IntLike()))
