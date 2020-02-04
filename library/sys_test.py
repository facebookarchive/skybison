#!/usr/bin/env python3
import sys
import unittest

from _io import StringIO
from test_support import pyro_only


class DisplayhookTest(unittest.TestCase):
    def test_displayhook_with_none_does_not_set_underscore(self):
        import builtins

        if hasattr(builtins, "_"):
            del builtins._

        orig_out = sys.stdout
        out = StringIO()
        sys.stdout = out
        sys.displayhook(None)
        self.assertEqual(out.getvalue(), "")
        self.assertTrue(not hasattr(builtins, "_"))
        sys.stdout = orig_out

    def test_displayhook_with_int_sets_underscore(self):
        import builtins

        orig_out = sys.stdout
        out = StringIO()
        sys.stdout = out
        sys.displayhook(42)
        self.assertEqual(out.getvalue(), "42\n")
        self.assertEqual(builtins._, 42)
        sys.stdout = orig_out


class SysTests(unittest.TestCase):
    class Mgr:
        def __enter__(self):
            pass

        def __exit__(self, type, value, tb):
            return True

    def test_exc_info_with_context_manager(self):
        try:
            raise RuntimeError()
        except RuntimeError:
            info1 = sys.exc_info()
            with self.Mgr():
                raise ValueError()
            info2 = sys.exc_info()
            self.assertEqual(info1, info2)

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

    @pyro_only
    def test_getsizeof_without_default_returns_size_int(self):
        class C:
            def __sizeof__(self):
                return 42

        self.assertEqual(sys.getsizeof(C()), 42)

    @pyro_only
    def test_getsizeof_with_default_returns_size_int(self):
        class C:
            def __sizeof__(self):
                return 42

        self.assertEqual(sys.getsizeof(C(), 3), 42)

    @pyro_only
    def test_getsizeof_with_int_subclass_returns_int(self):
        class N(int):
            pass

        class C:
            def __sizeof__(self):
                return N(42)

        result = sys.getsizeof(C())
        self.assertIs(type(result), int)
        self.assertEqual(result, 42)

    def test_hash_info_is_plausible(self):
        def is_power_of_two(x):
            return x & (x - 1) == 0

        hash_info = sys.hash_info
        max_value = (1 << (hash_info.width - 1)) - 1
        self.assertTrue(hash_info.modulus <= max_value)
        self.assertTrue(is_power_of_two(hash_info.modulus + 1))
        self.assertTrue(hash_info.inf <= max_value)
        self.assertTrue(hash_info.nan <= max_value)
        self.assertTrue(hash_info.imag <= max_value)
        self.assertIsInstance(hash_info.algorithm, str)
        self.assertTrue(hash_info.hash_bits >= hash_info.width)
        self.assertTrue(hash_info.seed_bits >= hash_info.hash_bits)
        self.assertIs(hash_info.width, hash_info[0])
        self.assertIs(hash_info.modulus, hash_info[1])
        self.assertIs(hash_info.inf, hash_info[2])
        self.assertIs(hash_info.nan, hash_info[3])
        self.assertIs(hash_info.imag, hash_info[4])
        self.assertIs(hash_info.algorithm, hash_info[5])
        self.assertIs(hash_info.hash_bits, hash_info[6])
        self.assertIs(hash_info.seed_bits, hash_info[7])
        self.assertIs(hash_info.cutoff, hash_info[8])

    def test_hash_info_matches_cpython(self):
        # We should not deviate from cpython without a good reason.
        hash_info = sys.hash_info
        self.assertEqual(hash_info.modulus, (1 << 61) - 1)
        self.assertEqual(hash_info.inf, 314159)
        self.assertEqual(hash_info.nan, 0)
        self.assertEqual(hash_info.imag, 1000003)
        self.assertEqual(hash_info.algorithm, "siphash24")
        self.assertEqual(hash_info.hash_bits, 64)
        self.assertEqual(hash_info.seed_bits, 128)

    def test_intern_returns_str(self):
        self.assertEqual(sys.intern("id"), "id")
        self.assertEqual(sys.intern("long identifier"), "long identifier")

    def test_intern_with_nonstr_raises_typeerror(self):
        with self.assertRaises(TypeError):
            sys.intern(12345)

    def test_intern_with_str_subclass_raises_typeerror(self):
        class NewString(str):
            pass

        with self.assertRaises(TypeError) as context:
            sys.intern(NewString("identifier"))

        self.assertEqual(str(context.exception), "can't intern NewString")

    def test_under_getframe_returns_frame(self):
        frame = sys._getframe(0)
        self.assertTrue(frame.f_globals is not None)
        self.assertEqual(frame.f_globals["__name__"], "__main__")
        self.assertTrue(frame.f_builtins is not None)
        self.assertEqual(frame.f_builtins["__name__"], "builtins")
        self.assertTrue(frame.f_code is not None)

    def test_under_getframe_with_noninteger_raises_typeerror(self):
        with self.assertRaises(TypeError):
            sys._getframe(None)

    def test_under_getframe_with_high_depth_raises_valueerror(self):
        with self.assertRaises(ValueError) as context:
            sys._getframe(1000)
        self.assertEqual(str(context.exception), "call stack is not deep enough")

    def test_under_getframe_with_negative_integer_returns_top_frame(self):
        self.assertEqual(sys._getframe(-1), sys._getframe(0))

    def test_under_getframe_with_no_argument_returns_top_frame(self):
        self.assertEqual(sys._getframe(), sys._getframe(0))

    def test_version(self):
        self.assertTrue(sys.version)
        self.assertEqual(len(sys.version_info), 5)


if __name__ == "__main__":
    unittest.main()
