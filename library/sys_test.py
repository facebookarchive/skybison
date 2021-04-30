#!/usr/bin/env python3
import contextlib
import sys
import unittest
from _io import StringIO, TextIOWrapper

from test_support import pyro_only, cpython_only


# Tests for traceback printing in sys.excepthook
def panic():
    raise RuntimeError("PANIC!!!")


def call_panic():
    # empty line
    panic()


def raise_after_n_frames(n):
    if not n:
        raise RuntimeError("PANIC!!!")
    raise_after_n_frames(n - 1)


class DisplayhookTest(unittest.TestCase):
    def test_displayhook_with_none_does_not_set_underscore(self):
        import builtins

        if hasattr(builtins, "_"):
            del builtins._

        orig_out = sys.stdout
        out = StringIO()
        sys.stdout = out
        sys.__displayhook__(None)
        self.assertEqual(out.getvalue(), "")
        self.assertTrue(not hasattr(builtins, "_"))
        sys.stdout = orig_out

    def test_displayhook_with_int_sets_underscore(self):
        import builtins

        orig_out = sys.stdout
        out = StringIO()
        sys.stdout = out
        sys.__displayhook__(42)
        self.assertEqual(out.getvalue(), "42\n")
        self.assertEqual(builtins._, 42)
        sys.stdout = orig_out

    def test_has_displayhook(self):
        self.assertTrue(hasattr(sys, "displayhook"))

    def test_displayhook_initial_value(self):
        self.assertIs(sys.displayhook, sys.__displayhook__)

    def test_dunder_std_streams_are_text_io_wrappers(self):
        self.assertIsInstance(sys.__stderr__, TextIOWrapper)
        self.assertIsInstance(sys.__stdin__, TextIOWrapper)
        self.assertIsInstance(sys.__stdout__, TextIOWrapper)


class ExceptHookTests(unittest.TestCase):
    def test_traceback_with_sys_tracebacklimit_truncates_stack_trace(self):
        has_tracebacklimit = hasattr(sys, "tracebacklimit")
        if has_tracebacklimit:
            tmp = sys.tracebacklimit

        with StringIO() as stderr, contextlib.redirect_stderr(stderr):
            try:
                sys.tracebacklimit = 1
                call_panic()
            except Exception:
                sys.excepthook(*sys.exc_info())
            finally:
                if has_tracebacklimit:
                    sys.tracebacklimit = tmp
                else:
                    del sys.tracebacklimit
            self.assertRegex(
                stderr.getvalue(),
                r"""Traceback \(most recent call last\):
  File ".*/sys_test.py", line \d+, in panic
    raise RuntimeError\("PANIC!!!"\)
RuntimeError: PANIC!!!
""",
            )

    def test_traceback_without_sys_tracebacklimit_prints_entire_small_traceback(self):
        has_tracebacklimit = hasattr(sys, "tracebacklimit")
        if has_tracebacklimit:
            tmp = sys.tracebacklimit
            del sys.tracebacklimit

        with StringIO() as stderr, contextlib.redirect_stderr(stderr):
            try:
                call_panic()
            except Exception:
                sys.excepthook(*sys.exc_info())
            finally:
                if has_tracebacklimit:
                    sys.tracebacklimit = tmp
            self.assertRegex(
                stderr.getvalue(),
                r"""Traceback \(most recent call last\):
  File ".*/sys_test.py", line \d+, in test_traceback_without_sys_tracebacklimit_prints_entire_small_traceback
    call_panic\(\)
  File ".*/sys_test.py", line \d+, in call_panic
    panic\(\)
  File ".*/sys_test.py", line \d+, in panic
    raise RuntimeError\("PANIC!!!"\)
RuntimeError: PANIC!!!
""",
            )

    def test_traceback_cuts_recursion_at_3_repeated_lines(self):
        has_tracebacklimit = hasattr(sys, "tracebacklimit")
        if has_tracebacklimit:
            tmp = sys.tracebacklimit
            del sys.tracebacklimit

        with StringIO() as stderr, contextlib.redirect_stderr(stderr):
            try:
                raise_after_n_frames(4)
            except Exception:
                sys.excepthook(*sys.exc_info())
            finally:
                if has_tracebacklimit:
                    sys.tracebacklimit = tmp
            self.assertRegex(
                stderr.getvalue(),
                r"""Traceback \(most recent call last\):
  File ".*/sys_test.py", line \d+, in test_traceback_cuts_recursion_at_3_repeated_lines
    raise_after_n_frames\(4\)
  File ".*/sys_test.py", line \d+, in raise_after_n_frames
    raise_after_n_frames\(n - 1\)
  File ".*/sys_test.py", line \d+, in raise_after_n_frames
    raise_after_n_frames\(n - 1\)
  File ".*/sys_test.py", line \d+, in raise_after_n_frames
    raise_after_n_frames\(n - 1\)
  \[Previous line repeated 1 more time\]
  File ".*/sys_test.py", line \d+, in raise_after_n_frames
    raise RuntimeError\("PANIC!!!"\)
RuntimeError: PANIC!!!
""",
            )

    def test_traceback_limits_recursion_and_depth(self):
        has_tracebacklimit = hasattr(sys, "tracebacklimit")
        if has_tracebacklimit:
            tmp = sys.tracebacklimit

        with StringIO() as stderr, contextlib.redirect_stderr(stderr):
            try:
                sys.tracebacklimit = 10
                raise_after_n_frames(20)
            except Exception:
                sys.excepthook(*sys.exc_info())
            finally:
                if has_tracebacklimit:
                    sys.tracebacklimit = tmp
                else:
                    del sys.tracebacklimit
            self.assertRegex(
                stderr.getvalue(),
                r"""Traceback \(most recent call last\):
  File ".*/sys_test.py", line \d+, in raise_after_n_frames
    raise_after_n_frames\(n - 1\)
  File ".*/sys_test.py", line \d+, in raise_after_n_frames
    raise_after_n_frames\(n - 1\)
  File ".*/sys_test.py", line \d+, in raise_after_n_frames
    raise_after_n_frames\(n - 1\)
  \[Previous line repeated 6 more times\]
  File ".*/sys_test.py", line \d+, in raise_after_n_frames
    raise RuntimeError\("PANIC!!!"\)
RuntimeError: PANIC!!!
""",
            )


class SysTests(unittest.TestCase):
    class Mgr:
        def __enter__(self):
            pass

        def __exit__(self, type, value, tb):
            return True

    def test_excepthook_initial_value(self):
        self.assertIs(sys.excepthook, sys.__excepthook__)

    def test_exit_raises_system_exit(self):
        with self.assertRaises(SystemExit) as ctx:
            sys.exit()

        self.assertEqual(ctx.exception.args, ())

    def test_exit_with_code_raises_system_exit_with_code(self):
        with self.assertRaises(SystemExit) as ctx:
            sys.exit("foo")

        self.assertEqual(ctx.exception.args, ("foo",))

    def test_exc_info_with_context_manager(self):
        try:
            raise RuntimeError()
        except RuntimeError:
            info1 = sys.exc_info()
            with self.Mgr():
                raise ValueError()
            info2 = sys.exc_info()
            self.assertEqual(info1, info2)

    def test_getdefaultencoding_returns_utf8(self):
        self.assertEqual(sys.getdefaultencoding(), "utf-8")

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

    def test_getsetrecursionlimit(self):
        limit = sys.getrecursionlimit()
        self.assertGreater(limit, 0)
        sys.setrecursionlimit(limit + 1)
        self.assertEqual(sys.getrecursionlimit(), limit + 1)
        sys.setrecursionlimit(limit)
        self.assertEqual(sys.getrecursionlimit(), limit)

    def test_implementation_cache_tag_matches_version_major_minor(self):
        name = sys.implementation.name
        major, minor = sys.version_info.major, sys.version_info.minor
        cache_tag = f"{name}-{major}{minor}"
        self.assertEqual(sys.implementation.cache_tag, cache_tag)

    def test_implementation_version_matches_module_version_info(self):
        self.assertEqual(sys.implementation.version, sys.version_info)

    def test_setrecursionlimit_with_large_limit_raises_overflowerror(self):
        with self.assertRaises(OverflowError) as context:
            sys.setrecursionlimit(230992039023490234904329023904239023)
        self.assertEqual(
            str(context.exception), "Python int too large to convert to C int"
        )

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

    def test_float_info_matches_cpython(self):
        float_info = sys.float_info
        self.assertEqual(float_info.max, 1.7976931348623157e308)
        self.assertEqual(float_info.max_exp, 1024)
        self.assertEqual(float_info.max_10_exp, 308)
        self.assertEqual(float_info.min, 2.2250738585072014e-308)
        self.assertEqual(float_info.min_exp, -1021)
        self.assertEqual(float_info.min_10_exp, -307)
        self.assertEqual(float_info.dig, 15)
        self.assertEqual(float_info.mant_dig, 53)
        self.assertEqual(float_info.epsilon, 2.220446049250313e-16)
        self.assertEqual(float_info.radix, 2)
        self.assertEqual(float_info.rounds, 1)

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

    def test_is_finalizing_before_shutdown_returns_false(self):
        self.assertEqual(sys.is_finalizing(), False)

    def test_stdio_initial_values(self):
        self.assertIs(sys.stderr, sys.__stderr__)
        self.assertIs(sys.stdin, sys.__stdin__)
        self.assertIs(sys.stdout, sys.__stdout__)

    def test_std_streams_are_utf_8_encoded(self):
        self.assertEqual(sys.stderr.encoding, "utf-8")
        self.assertEqual(sys.stdin.encoding, "utf-8")
        self.assertEqual(sys.stdout.encoding, "utf-8")

    def test_std_streams_have_correct_modes(self):
        self.assertEqual(sys.stderr.mode, "w")
        self.assertEqual(sys.stdin.mode, "r")
        self.assertEqual(sys.stdout.mode, "w")

    @pyro_only
    def test_std_streams_point_to_correct_fileno(self):
        self.assertEqual(sys.stderr.buffer.fileno(), sys._stderr_fd)
        self.assertEqual(sys.stdin.buffer.fileno(), sys._stdin_fd)
        self.assertEqual(sys.stdout.buffer.fileno(), sys._stdout_fd)

    # TODO(T89882231) enable test for Pyro
    @cpython_only
    def test_under_getframe_returns_frame(self):
        from types import ModuleType

        frame = sys._getframe(0)
        self.assertTrue(frame.f_globals is not None)
        self.assertEqual(frame.f_globals["__name__"], "__main__")
        self.assertTrue(frame.f_locals is not None)
        self.assertEqual(frame.f_locals["self"], self)
        builtins = __builtins__
        if isinstance(builtins, ModuleType):
            builtins = builtins.__dict__
        self.assertIs(frame.f_builtins, builtins)
        self.assertTrue(frame.f_code is not None)

    def test_under_getframe_with_noninteger_raises_typeerror(self):
        with self.assertRaises(TypeError):
            sys._getframe(None)

    # TODO(T89882231) enable test for Pyro
    @cpython_only
    def test_under_getframe_returns_frame_with_locals(self):
        def baz():
            return sys._getframe(1).f_locals

        def bar():
            foo = 1  # noqa: F841
            return baz()

        bar_locals = bar()
        self.assertEqual(len(bar_locals), 2)
        self.assertEqual(bar_locals["foo"], 1)
        self.assertEqual(bar_locals["baz"], baz)

    def test_under_getframe_from_function_cannot_modify_locals(self):
        def baz():
            sys._getframe(1).f_locals["foo"] = "wrong"

        def bar():
            foo = "correct"
            baz()
            return foo

        self.assertEqual(bar(), "correct")

    def test_under_getframe_from_global_scope_gets_locals(self):
        from types import ModuleType

        module = ModuleType("")
        module.sys = sys
        module.temp = "wrong"
        exec("sys._getframe(0).f_locals['temp'] = 'correct'", module.__dict__)
        self.assertEqual(module.temp, "correct")

    def test_under_getframe_with_high_depth_raises_valueerror(self):
        with self.assertRaises(ValueError) as context:
            sys._getframe(1000)
        self.assertEqual(str(context.exception), "call stack is not deep enough")

    def test_under_getframe_with_negative_integer_returns_top_frame(self):
        self.assertEqual(sys._getframe(-1).f_code, sys._getframe(0).f_code)

    def test_under_getframe_with_no_argument_returns_top_frame(self):
        self.assertEqual(sys._getframe().f_code, sys._getframe(0).f_code)

    def test_under_getframe_f_back_points_to_previous_frame(self):
        def baz():
            return sys._getframe(0)

        def bar():
            return baz()

        def foo():
            return bar()

        frame = foo()
        self.assertIs(frame.f_code, baz.__code__)
        self.assertIs(frame.f_back.f_code, bar.__code__)
        self.assertIs(frame.f_back.f_back.f_code, foo.__code__)

    def test_under_getframe_f_back_leads_to_module_frame(self):
        frame = sys._getframe()
        while True:
            if frame.f_back is None:
                break
            frame = frame.f_back
        self.assertIsNone(frame.f_back)
        self.assertIs(frame.f_globals, sys.modules[self.__module__].__dict__)

    @pyro_only
    def test_under_getframe_f_back_excludes_builtins_function(self):
        recorded_frame = None

        class C:
            def __hash__(self):
                nonlocal recorded_frame
                recorded_frame = sys._getframe()
                return 1234

        def foo():
            c = C()
            d = {}
            # Calling C.__hash__ via native code, dict.__delitem__.
            try:
                del d[c]
            except KeyError:
                pass

        foo()

        self.assertIs(recorded_frame.f_code, C.__hash__.__code__)
        # The result excludes dict.__delitem__.
        self.assertIs(recorded_frame.f_back.f_code, foo.__code__)

    def test_version(self):
        self.assertTrue(sys.version)
        self.assertEqual(len(sys.version_info), 5)

    def test_hexversion(self):
        self.assertIsInstance(sys.hexversion, int)
        self.assertEqual((sys.hexversion >> 24) & 0xFF, sys.version_info.major)
        self.assertEqual((sys.hexversion >> 16) & 0xFF, sys.version_info.minor)
        self.assertEqual((sys.hexversion >> 8) & 0xFF, sys.version_info.micro)
        release_level = (sys.hexversion >> 4) & 0xF
        release_level_str = {0xA: "alpha", 0xB: "beta", 0xC: "candidate", 0xF: "final"}
        self.assertEqual(
            release_level_str[release_level], sys.version_info.releaselevel
        )
        self.assertEqual(sys.hexversion & 0xF, sys.version_info.serial)

    def test_under_getframe_f_lineno(self):
        d = {}
        exec("import sys\n\nresult = sys._getframe().f_lineno", d)
        self.assertIs(d["result"], 3)

    def test_set_asyncgen_hooks_raises_type_error_on_non_none_non_callable_finalizer(
        self,
    ):
        with self.assertRaises(TypeError):
            sys.set_asyncgen_hooks(finalizer=1)

    def test_set_asyncgen_hooks_raises_type_error_on_non_none_non_callable_firstiter(
        self,
    ):
        with self.assertRaises(TypeError):
            sys.set_asyncgen_hooks(firstiter=1)

    def test_set_asyncgen_hooks_with_none_values(self):
        sys.set_asyncgen_hooks(None, None)
        hooks = sys.get_asyncgen_hooks()
        self.assertIsNone(hooks[0], None)
        self.assertIsNone(hooks[1], None)

    def test_set_asyncgen_hooks_with_callables(self):
        def f1():
            pass

        def f2():
            pass

        sys.set_asyncgen_hooks(f1, f2)
        hooks = sys.get_asyncgen_hooks()
        self.assertEqual(hooks[0], f1)
        self.assertEqual(hooks[1], f2)

    def test_set_asyncgen_hooks_with_only_named_firstiter(self):
        def f():
            pass

        # Clear any existing values
        sys.set_asyncgen_hooks(None, None)

        sys.set_asyncgen_hooks(firstiter=f)
        hooks = sys.get_asyncgen_hooks()
        self.assertEqual(hooks[0], f)
        self.assertEqual(hooks[1], None)

    def test_set_asyncgen_hooks_with_only_positional_firstiter(self):
        def f():
            pass

        # Clear any existing values
        sys.set_asyncgen_hooks(None, None)

        sys.set_asyncgen_hooks(f)
        hooks = sys.get_asyncgen_hooks()
        self.assertEqual(hooks[0], f)
        self.assertEqual(hooks[1], None)

    def test_set_asyncgen_hooks_with_only_named_finalizer(self):
        def f():
            pass

        # Clear any existing values
        sys.set_asyncgen_hooks(None, None)

        sys.set_asyncgen_hooks(finalizer=f)
        hooks = sys.get_asyncgen_hooks()
        self.assertEqual(hooks[0], None)
        self.assertEqual(hooks[1], f)

    def test_set_asyncgen_hooks_with_no_args(self):
        def f1():
            pass

        def f2():
            pass

        # Set initial values which shouldn't be affected
        sys.set_asyncgen_hooks(f1, f2)

        sys.set_asyncgen_hooks()
        hooks = sys.get_asyncgen_hooks()
        self.assertEqual(hooks[0], f1)
        self.assertEqual(hooks[1], f2)

    def test_asyncgen_hooks_attributes(self):
        def f1():
            pass

        def f2():
            pass

        sys.set_asyncgen_hooks(f1, f2)
        hooks = sys.get_asyncgen_hooks()
        self.assertEqual(hooks.firstiter, f1)
        self.assertEqual(hooks.finalizer, f2)


if __name__ == "__main__":
    unittest.main()
