#!/usr/bin/env python3
import sys
import unittest


class SysTests(unittest.TestCase):
    def test_getsizeof_without_dunder_sizeof_raises_type_error(self):
        class M(type):
            def mro(cls):
                return (cls,)

        class C(metaclass=M):
            __new__ = type.__new__
            __call__ = type.__call__

        with self.assertRaises(TypeError):
            sys.getsizeof(C())

    def test_getsizeof_with_non_int_without_default_raises_type_error(self):
        class C:
            def __sizeof__(self):
                return "not an integer"

        with self.assertRaises(TypeError):
            sys.getsizeof(C())

    def test_getsizeof_with_non_int_returns_default(self):
        class C:
            def __sizeof__(self):
                return "not an integer"

        self.assertEqual(sys.getsizeof(C(), 42), 42)

    def test_getsizeof_with_negative_raises_value_error(self):
        class C:
            def __sizeof__(self):
                return -1

        with self.assertRaises(ValueError):
            sys.getsizeof(C())

    def test_getsizeof_without_default_returns_size_int(self):
        class C:
            def __sizeof__(self):
                return 42

        self.assertEqual(sys.getsizeof(C()), 42)

    def test_getsizeof_with_default_returns_size_int(self):
        class C:
            def __sizeof__(self):
                return 42

        self.assertEqual(sys.getsizeof(C(), 3), 42)

    def test_getsizeof_with_int_subclass_returns_int(self):
        class N(int):
            pass

        class C:
            def __sizeof__(self):
                return N(42)

        result = sys.getsizeof(C())
        self.assertIs(type(result), int)
        self.assertEqual(result, 42)

    def test_getframe_code_returns_self(self):
        code = sys._getframe_code(0)
        self.assertEqual(code.co_name, "test_getframe_code_returns_self")

    def test_getframe_code_returns_class_run(self):
        code = sys._getframe_code(1)
        self.assertEqual(code.co_name, "run")

    def test_getframe_globals_returns_dict(self):
        self.assertIsInstance(sys._getframe_globals(0), dict)

    def test_getframe_lineno_returns_int(self):
        self.assertIsInstance(sys._getframe_lineno(0), int)


if __name__ == "__main__":
    unittest.main()
