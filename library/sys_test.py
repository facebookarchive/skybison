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

    @pyro_only
    def test_getframe_code_with_int_subclass(self):
        class C(int):
            pass

        code = sys._getframe_code(C(0))
        self.assertEqual(code.co_name, "test_getframe_code_with_int_subclass")

    @pyro_only
    def test_getframe_code_returns_self(self):
        code = sys._getframe_code(0)
        self.assertEqual(code.co_name, "test_getframe_code_returns_self")

    @pyro_only
    def test_getframe_code_returns_class_run(self):
        code = sys._getframe_code(1)
        self.assertEqual(code.co_name, "run")

    @pyro_only
    def test_getframe_globals_with_int_subclass(self):
        class C(int):
            pass

        self.assertIsInstance(sys._getframe_globals(C(0)), dict)

    @pyro_only
    def test_getframe_globals_returns_dict(self):
        self.assertIsInstance(sys._getframe_globals(0), dict)

    @pyro_only
    def test_getframe_lineno_returns_int(self):
        self.assertIsInstance(sys._getframe_lineno(0), int)

    @pyro_only
    def test_getframe_locals_returns_local_vars(self):
        def foo():
            a = 4
            a = a  # noqa: F841
            b = 5
            b = b  # noqa: F841
            return sys._getframe_locals()

        result = foo()
        self.assertIsInstance(result, dict)
        self.assertEqual(len(result), 2)
        self.assertEqual(result["a"], 4)
        self.assertEqual(result["b"], 5)

    @pyro_only
    def test_getframe_locals_returns_free_vars(self):
        def foo():
            a = 4

            def bar():
                nonlocal a
                a = 5
                return sys._getframe_locals()

            return bar()

        result = foo()
        self.assertIsInstance(result, dict)
        self.assertEqual(len(result), 1)
        self.assertEqual(result["a"], 5)

    @pyro_only
    def test_getframe_locals_returns_cell_vars(self):
        def foo():
            a = 4

            def bar(b):
                return a + b

            return sys._getframe_locals()

        result = foo()
        self.assertIsInstance(result, dict)
        self.assertEqual(len(result), 2)
        self.assertEqual(result["a"], 4)
        from types import FunctionType

        self.assertIsInstance(result["bar"], FunctionType)

    @pyro_only
    def test_getframe_locals_in_class_body_returns_dict_with_class_variables(self):
        class C:
            foo = 4
            bar = 5
            baz = sys._getframe_locals()

        result = C.baz
        self.assertIsInstance(result, dict)
        self.assertEqual(result["foo"], 4)
        self.assertEqual(result["bar"], 5)

    @pyro_only
    def test_getframe_locals_in_exec_scope_returns_given_locals_instance(self):
        result_key = None
        result_value = None

        class C:
            def __getitem__(self, key):
                if key == "_getframe_locals":
                    return sys._getframe_locals
                raise Exception

            def __setitem__(self, key, value):
                nonlocal result_key
                nonlocal result_value
                result_key = key
                result_value = value

        c = C()
        exec("result = _getframe_locals()", {}, c)
        self.assertEqual(result_key, "result")
        self.assertIs(result_value, c)

    @pyro_only
    def test_getframe_locals_with_too_large_depth_raise_value_error(self):
        with self.assertRaises(ValueError) as context:
            sys._getframe_locals(100)
        self.assertIn("call stack is not deep enough", str(context.exception))

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


if __name__ == "__main__":
    unittest.main()
