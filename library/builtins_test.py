#!/usr/bin/env python3
import unittest


class ByteArrayTest(unittest.TestCase):
    def test_dunder_setitem_with_non_bytearray_raises_type_error(self):
        with self.assertRaises(TypeError):
            bytearray.__setitem__(None, 1, 5)

    def test_dunder_setitem_with_non_int_value_raises_type_error(self):
        with self.assertRaises(TypeError):
            ba = bytearray(b"foo")
            bytearray.__setitem__(ba, 1, b"x")

    def test_dunder_setitem_with_int_key_sets_value_at_index(self):
        ba = bytearray(b"foo")
        bytearray.__setitem__(ba, 1, 1)
        self.assertEqual(ba, bytearray(b"f\01o"))

    def test_find_with_bytes_self_raises_type_error(self):
        with self.assertRaises(TypeError):
            bytearray.find(b"", bytearray())

    def test_find_with_empty_sub_returns_start(self):
        haystack = bytearray(b"abc")
        needle = b""
        self.assertEqual(haystack.find(needle, 1), 1)

    def test_find_with_missing_returns_negative(self):
        haystack = bytearray(b"abc")
        needle = b"d"
        self.assertEqual(haystack.find(needle), -1)

    def test_find_with_missing_stays_within_bounds(self):
        haystack = bytearray(b"abc")
        needle = bytearray(b"c")
        self.assertEqual(haystack.find(needle, None, 2), -1)

    def test_find_with_large_start_returns_negative(self):
        haystack = bytearray(b"abc")
        needle = bytearray(b"")
        self.assertEqual(haystack.find(needle, 10), -1)

    def test_find_with_negative_bounds_returns_index(self):
        haystack = bytearray(b"ababa")
        needle = b"a"
        self.assertEqual(haystack.find(needle, -4, -1), 2)

    def test_find_with_multiple_matches_returns_first_index_in_range(self):
        haystack = bytearray(b"abbabbabba")
        needle = bytearray(b"abb")
        self.assertEqual(haystack.find(needle, 1), 3)

    def test_find_with_nonbyte_int_raises_value_error(self):
        haystack = bytearray()
        needle = 266
        with self.assertRaises(ValueError):
            haystack.find(needle)

    def test_find_with_string_raises_type_error(self):
        haystack = bytearray()
        needle = "133"
        with self.assertRaises(TypeError):
            haystack.find(needle)

    def test_find_with_non_number_index_raises_type_error(self):
        class Idx:
            def __index__(self):
                return ord("a")

        haystack = bytearray(b"abc")
        needle = Idx()
        with self.assertRaises(TypeError):
            haystack.find(needle)

    def test_find_with_dunder_int_calls_dunder_index(self):
        class Idx:
            def __int__(self):
                raise NotImplementedError("called __int__")

            def __index__(self):
                return ord("a")

        haystack = bytearray(b"abc")
        needle = Idx()
        self.assertEqual(haystack.find(needle), 0)

    def test_find_with_dunder_float_calls_dunder_index(self):
        class Idx:
            def __float__(self):
                raise NotImplementedError("called __float__")

            def __index__(self):
                return ord("a")

        haystack = bytearray(b"abc")
        needle = Idx()
        self.assertEqual(haystack.find(needle), 0)

    def test_index_with_bytes_self_raises_type_error(self):
        with self.assertRaises(TypeError):
            bytearray.index(b"", bytearray())

    def test_index_with_subsequence_returns_first_in_range(self):
        haystack = bytearray(b"-a---a-aa")
        needle = ord("a")
        self.assertEqual(haystack.index(needle, 3), 5)

    def test_index_with_missing_raises_value_error(self):
        haystack = bytearray(b"abc")
        needle = b"d"
        with self.assertRaises(ValueError) as context:
            haystack.index(needle)
        self.assertEqual(str(context.exception), "subsection not found")

    def test_index_outside_of_bounds_raises_value_error(self):
        haystack = bytearray(b"abc")
        needle = bytearray(b"c")
        with self.assertRaises(ValueError) as context:
            haystack.index(needle, 0, 2)
        self.assertEqual(str(context.exception), "subsection not found")


class BytesTest(unittest.TestCase):
    def test_dunder_add_with_bytes_like_other_returns_bytes(self):
        self.assertEqual(b"123".__add__(bytearray(b"456")), b"123456")

    def test_dunder_add_with_non_bytes_like_other_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            b"".__add__(2)
        self.assertEqual(str(context.exception), "can't concat int to bytes")

    def test_dunder_new_with_str_without_encoding_raises_type_error(self):
        with self.assertRaises(TypeError):
            bytes("foo")

    def test_dunder_new_with_str_and_encoding_returns_bytes(self):
        self.assertEqual(bytes("foo", "ascii"), b"foo")

    def test_dunder_new_with_ignore_errors_returns_bytes(self):
        self.assertEqual(bytes("fo\x80o", "ascii", "ignore"), b"foo")

    def test_find_with_bytearray_self_raises_type_error(self):
        with self.assertRaises(TypeError):
            bytes.find(bytearray(), b"")

    def test_find_with_empty_sub_returns_start(self):
        haystack = b"abc"
        needle = bytearray()
        self.assertEqual(haystack.find(needle, 1), 1)

    def test_find_with_missing_returns_negative(self):
        haystack = b"abc"
        needle = b"d"
        self.assertEqual(haystack.find(needle), -1)

    def test_find_with_missing_stays_within_bounds(self):
        haystack = b"abc"
        needle = bytearray(b"c")
        self.assertEqual(haystack.find(needle, None, 2), -1)

    def test_find_with_large_start_returns_negative(self):
        haystack = b"abc"
        needle = bytearray(b"c")
        self.assertEqual(haystack.find(needle, 10), -1)

    def test_find_with_negative_bounds_returns_index(self):
        haystack = b"foobar"
        needle = bytearray(b"o")
        self.assertEqual(haystack.find(needle, -6, -1), 1)

    def test_find_with_multiple_matches_returns_first_index_in_range(self):
        haystack = b"abbabbabba"
        needle = bytearray(b"abb")
        self.assertEqual(haystack.find(needle, 1), 3)

    def test_find_with_nonbyte_int_raises_value_error(self):
        haystack = b""
        needle = 266
        with self.assertRaises(ValueError):
            haystack.find(needle)

    def test_find_with_string_raises_type_error(self):
        haystack = b""
        needle = "133"
        with self.assertRaises(TypeError):
            haystack.find(needle)

    def test_find_with_non_number_index_raises_type_error(self):
        class Idx:
            def __index__(self):
                return ord("a")

        haystack = b"abc"
        needle = Idx()
        with self.assertRaises(TypeError):
            haystack.find(needle)

    def test_find_with_raising_descriptor_dunder_int_does_not_call_dunder_get(self):
        dunder_get_called = False

        class Desc:
            def __get__(self, obj, type):
                nonlocal dunder_get_called
                dunder_get_called = True
                raise UserWarning("foo")

        class Idx:
            __int__ = Desc()

            def __index__(self):
                return ord("a")

        haystack = b"abc"
        needle = Idx()
        self.assertEqual(haystack.find(needle), 0)
        self.assertFalse(dunder_get_called)

    def test_find_with_raising_descriptor_dunder_float_does_not_call_dunder_get(self):
        dunder_get_called = False

        class Desc:
            def __get__(self, obj, type):
                nonlocal dunder_get_called
                dunder_get_called = True
                raise UserWarning("foo")

        class Idx:
            __float__ = Desc()

            def __index__(self):
                return ord("a")

        haystack = b"abc"
        needle = Idx()
        self.assertEqual(haystack.find(needle), 0)
        self.assertFalse(dunder_get_called)

    def test_find_with_dunder_int_calls_dunder_index(self):
        class Idx:
            def __int__(self):
                raise NotImplementedError("called __int__")

            def __index__(self):
                return ord("a")

        haystack = b"abc"
        needle = Idx()
        self.assertEqual(haystack.find(needle), 0)

    def test_find_with_dunder_float_calls_dunder_index(self):
        class Idx:
            def __float__(self):
                raise NotImplementedError("called __float__")

            def __index__(self):
                return ord("a")

        haystack = b"abc"
        needle = Idx()
        self.assertEqual(haystack.find(needle), 0)

    def test_index_with_bytearray_self_raises_type_error(self):
        with self.assertRaises(TypeError):
            bytes.index(bytearray(), b"")

    def test_index_with_subsequence_returns_first_in_range(self):
        haystack = b"-a---a-aa"
        needle = ord("a")
        self.assertEqual(haystack.index(needle, 3), 5)

    def test_index_with_missing_raises_value_error(self):
        haystack = b"abc"
        needle = b"d"
        with self.assertRaises(ValueError) as context:
            haystack.index(needle)
        self.assertEqual(str(context.exception), "subsection not found")

    def test_index_outside_of_bounds_raises_value_error(self):
        haystack = b"abc"
        needle = bytearray(b"c")
        with self.assertRaises(ValueError) as context:
            haystack.index(needle, 0, 2)
        self.assertEqual(str(context.exception), "subsection not found")


class DictTests(unittest.TestCase):
    def test_clear_with_non_dict_raises_type_error(self):
        with self.assertRaises(TypeError):
            dict.clear(None)

    def test_clear_removes_all_elements(self):
        d = {"a": 1}
        self.assertEqual(dict.clear(d), None)
        self.assertEqual(d.__len__(), 0)
        self.assertNotIn("a", d)

    def test_update_with_malformed_sequence_elt_raises_type_error(self):
        with self.assertRaises(ValueError):
            dict.update({}, [("a",)])

    def test_update_with_no_params_does_nothing(self):
        d = {"a": 1}
        d.update()
        self.assertEqual(len(d), 1)

    def test_update_with_mapping_adds_elements(self):
        d = {"a": 1}
        d.update([("a", "b"), ("c", "d")])
        self.assertIn("a", d)
        self.assertIn("c", d)
        self.assertEqual(d["a"], "b")
        self.assertEqual(d["c"], "d")

    def test_dunder_delitem_with_none_dunder_hash(self):
        class C:
            __hash__ = None

        with self.assertRaises(TypeError):
            dict.__delitem__({}, C())

    def test_dunder_delitem_propagates_exceptions_from_dunder_hash(self):
        class C:
            def __hash__(self):
                raise UserWarning("foo")

        with self.assertRaises(UserWarning):
            dict.__delitem__({}, C())

    def test_update_with_tuple_keys_propagates_exceptions_from_dunder_hash(self):
        class C:
            def __hash__(self):
                raise UserWarning("foo")

        class D:
            def keys(self):
                return (C(),)

            def __getitem__(self, key):
                return "foo"

        with self.assertRaises(UserWarning):
            dict.update({}, D())

    def test_update_with_list_keys_propagates_exceptions_from_dunder_hash(self):
        class C:
            def __hash__(self):
                raise UserWarning("foo")

        class D:
            def keys(self):
                return [C()]

            def __getitem__(self, key):
                return "foo"

        with self.assertRaises(UserWarning):
            dict.update({}, D())

    def test_update_with_iter_keys_propagates_exceptions_from_dunder_hash(self):
        class C:
            def __hash__(self):
                raise UserWarning("foo")

        class D:
            def keys(self):
                return [C()].__iter__()

            def __getitem__(self, key):
                return "foo"

        with self.assertRaises(UserWarning):
            dict.update({}, D())


class ExceptionTests(unittest.TestCase):
    def test_maybe_unbound_attributes(self):
        exc = BaseException()
        exc2 = BaseException()
        self.assertIs(exc.__cause__, None)
        self.assertIs(exc.__context__, None)
        self.assertIs(exc.__traceback__, None)

        # Test setter for __cause__.
        self.assertRaises(TypeError, setattr, exc, "__cause__", 123)
        exc.__cause__ = exc2
        self.assertIs(exc.__cause__, exc2)
        exc.__cause__ = None
        self.assertIs(exc.__cause__, None)

        # Test setter for __context__.
        self.assertRaises(TypeError, setattr, exc, "__context__", 456)
        exc.__context__ = exc2
        self.assertIs(exc.__context__, exc2)
        exc.__context__ = None
        self.assertIs(exc.__context__, None)

        # Test setter for __traceback__.
        self.assertRaises(TypeError, setattr, "__traceback__", "some string")
        # TODO(bsimmers): Set a real traceback once we support them.
        exc.__traceback__ = None
        self.assertIs(exc.__traceback__, None)

    def test_context_chaining(self):
        inner_exc = None
        outer_exc = None
        try:
            try:
                raise RuntimeError("whoops")
            except RuntimeError as exc:
                inner_exc = exc
                raise TypeError("darn")
        except TypeError as exc:
            outer_exc = exc

        self.assertIsInstance(inner_exc, RuntimeError)
        self.assertIsInstance(outer_exc, TypeError)
        self.assertIs(outer_exc.__context__, inner_exc)
        self.assertIsNone(inner_exc.__context__)

    def test_context_chaining_cycle_avoidance(self):
        exc0 = None
        exc1 = None
        exc2 = None
        exc3 = None
        try:
            try:
                try:
                    try:
                        raise RuntimeError("inner")
                    except RuntimeError as exc:
                        exc0 = exc
                        raise RuntimeError("middle")
                except RuntimeError as exc:
                    exc1 = exc
                    raise RuntimeError("outer")
            except RuntimeError as exc:
                exc2 = exc
                # The __context__ link between exc1 and exc0 should be broken
                # by this raise.
                raise exc0
        except RuntimeError as exc:
            exc3 = exc

        self.assertIs(exc3, exc0)
        self.assertIs(exc3.__context__, exc2)
        self.assertIs(exc2.__context__, exc1)
        self.assertIs(exc1.__context__, None)


class GeneratorTests(unittest.TestCase):
    def test_managed_stop_iteration(self):
        def inner_gen():
            yield None
            raise StopIteration("hello")

        def outer_gen():
            val = yield from inner_gen()
            yield val

        g = outer_gen()
        self.assertIs(next(g), None)
        self.assertEqual(next(g), "hello")

    def test_gi_running(self):
        def gen():
            self.assertTrue(g.gi_running)
            yield 1
            self.assertTrue(g.gi_running)
            yield 2

        g = gen()
        self.assertFalse(g.gi_running)
        next(g)
        self.assertFalse(g.gi_running)
        next(g)
        self.assertFalse(g.gi_running)

    def test_gi_running_readonly(self):
        def gen():
            yield None

        g = gen()
        self.assertRaises(AttributeError, setattr, g, "gi_running", 1234)

    def test_running_gen_raises(self):
        def gen():
            self.assertRaises(ValueError, next, g)
            yield "done"

        g = gen()
        self.assertEqual(next(g), "done")

    class MyError(Exception):
        pass

    @staticmethod
    def simple_gen():
        yield 1
        yield 2

    @staticmethod
    def catching_gen():
        try:
            yield 1
        except GeneratorTests.MyError:
            yield "caught"

    @staticmethod
    def catching_returning_gen():
        try:
            yield 1
        except GeneratorTests.MyError:
            return "all done!"  # noqa

    @staticmethod
    def delegate_gen(g):
        r = yield from g
        yield r

    def test_throw(self):
        g = self.simple_gen()
        self.assertRaises(GeneratorTests.MyError, g.throw, GeneratorTests.MyError())

    def test_throw_caught(self):
        g = self.catching_gen()
        self.assertEqual(next(g), 1)
        self.assertEqual(g.throw(GeneratorTests.MyError()), "caught")

    def test_throw_type(self):
        g = self.catching_gen()
        self.assertEqual(next(g), 1)
        self.assertEqual(g.throw(GeneratorTests.MyError), "caught")

    def test_throw_type_and_value(self):
        g = self.catching_gen()
        self.assertEqual(next(g), 1)
        self.assertEqual(
            g.throw(GeneratorTests.MyError, GeneratorTests.MyError()), "caught"
        )

    def test_throw_uncaught_type(self):
        g = self.catching_gen()
        self.assertEqual(next(g), 1)
        self.assertRaises(RuntimeError, g.throw, RuntimeError)

    def test_throw_finished(self):
        g = self.catching_returning_gen()
        self.assertEqual(next(g), 1)
        self.assertRaises(StopIteration, g.throw, GeneratorTests.MyError)

    def test_throw_two_values(self):
        g = self.catching_gen()
        self.assertEqual(next(g), 1)
        self.assertRaises(
            TypeError, g.throw, GeneratorTests.MyError(), GeneratorTests.MyError()
        )

    def test_throw_bad_traceback(self):
        g = self.catching_gen()
        self.assertEqual(next(g), 1)
        self.assertRaises(
            TypeError, g.throw, GeneratorTests.MyError, GeneratorTests.MyError(), 5
        )

    def test_throw_bad_type(self):
        g = self.catching_gen()
        self.assertEqual(next(g), 1)
        self.assertRaises(TypeError, g.throw, 1234)

    def test_throw_not_started(self):
        g = self.simple_gen()
        self.assertRaises(GeneratorTests.MyError, g.throw, GeneratorTests.MyError())
        self.assertRaises(StopIteration, next, g)

    def test_throw_stopped(self):
        g = self.simple_gen()
        self.assertEqual(next(g), 1)
        self.assertEqual(next(g), 2)
        self.assertRaises(StopIteration, next, g)
        self.assertRaises(GeneratorTests.MyError, g.throw, GeneratorTests.MyError())

    def test_throw_yield_from(self):
        g = self.delegate_gen(self.simple_gen())
        self.assertEqual(next(g), 1)
        self.assertRaises(GeneratorTests.MyError, g.throw, GeneratorTests.MyError)

    def test_throw_yield_from_caught(self):
        g = self.delegate_gen(self.catching_gen())
        self.assertEqual(next(g), 1)
        self.assertEqual(g.throw(GeneratorTests.MyError), "caught")

    def test_throw_yield_from_finishes(self):
        g = self.delegate_gen(self.catching_returning_gen())
        self.assertEqual(next(g), 1)
        self.assertEqual(g.throw(GeneratorTests.MyError), "all done!")

    def test_throw_yield_from_non_gen(self):
        g = self.delegate_gen([1, 2, 3, 4])
        self.assertEqual(next(g), 1)
        self.assertRaises(RuntimeError, g.throw, RuntimeError)

    def test_dunder_repr(self):
        def foo():
            yield 5

        self.assertTrue(
            foo()
            .__repr__()
            .startswith(
                "<generator object GeneratorTests.test_dunder_repr.<locals>.foo at "
            )
        )


class IntTests(unittest.TestCase):
    def test_dunder_pow_with_zero_returns_one(self):
        self.assertEqual(int.__pow__(4, 0), 1)

    def test_dunder_pow_with_one_returns_self(self):
        self.assertEqual(int.__pow__(4, 1), 4)

    def test_dunder_pow_with_two_squares_number(self):
        self.assertEqual(int.__pow__(4, 2), 16)

    def test_dunder_pow_with_mod_equals_one_returns_zero(self):
        self.assertEqual(int.__pow__(4, 2, 1), 0)

    def test_dunder_pow_with_negative_power_and_mod_raises_value_error(self):
        with self.assertRaises(ValueError):
            int.__pow__(4, -2, 1)

    def test_dunder_pow_with_mod(self):
        self.assertEqual(int.__pow__(4, 8, 10), 6)

    def test_dunder_pow_with_negative_base_calls_float_dunder_pow(self):
        self.assertLess((int.__pow__(2, -1) - 0.5).__abs__(), 0.00001)

    def test_dunder_pow_with_non_int_self_raises_type_error(self):
        with self.assertRaises(TypeError):
            int.__pow__(None, 1, 1)

    def test_dunder_pow_with_non_int_power_returns_not_implemented(self):
        self.assertEqual(int.__pow__(1, None), NotImplemented)

    def test_from_bytes_returns_int(self):
        self.assertEqual(int.from_bytes(b"\xca\xfe", "little"), 0xFECA)

    def test_from_bytes_with_kwargs_returns_int(self):
        self.assertEqual(
            int.from_bytes(bytes=b"\xca\xfe", byteorder="big", signed=True), -13570
        )

    def test_from_bytes_with_bytes_convertible_returns_int(self):
        class C:
            def __bytes__(self):
                return b"*"

        i = C()
        self.assertEqual(int.from_bytes(i, "big"), 42)

    def test_from_bytes_with_invalid_bytes_raises_type_error(self):
        with self.assertRaisesRegex(TypeError, "cannot convert 'str' object to bytes"):
            int.from_bytes("not a bytes object", "big")

    def test_from_bytes_with_invalid_byteorder_raises_value_error(self):
        with self.assertRaisesRegex(
            ValueError, "byteorder must be either 'little' or 'big'"
        ):
            int.from_bytes(b"*", byteorder="medium")

    def test_from_bytes_with_invalid_byteorder_raises_type_error(self):
        with self.assertRaisesRegex(
            TypeError, "from_bytes\\(\\) argument 2 must be str, not int"
        ):
            int.from_bytes(b"*", byteorder=42)

    def test_new_with_base_without_str_raises_type_error(self):
        with self.assertRaises(TypeError):
            int(base=8)

    def test_new_with_bool_returns_int(self):
        self.assertIs(int(False), 0)
        self.assertIs(int(True), 1)

    def test_new_with_bytearray_returns_int(self):
        self.assertEqual(int(bytearray(b"23")), 23)
        self.assertEqual(int(bytearray(b"-23"), 8), -0o23)
        self.assertEqual(int(bytearray(b"abc"), 16), 0xABC)
        self.assertEqual(int(bytearray(b"0xabc"), 0), 0xABC)

    def test_new_with_bytes_returns_int(self):
        self.assertEqual(int(b"-23"), -23)
        self.assertEqual(int(b"23", 8), 0o23)
        self.assertEqual(int(b"abc", 16), 0xABC)
        self.assertEqual(int(b"0xabc", 0), 0xABC)

    def test_new_with_empty_bytearray_raises_value_error(self):
        with self.assertRaises(ValueError):
            int(bytearray())

    def test_new_with_empty_bytes_raises_value_error(self):
        with self.assertRaises(ValueError):
            int(b"")

    def test_new_with_empty_str_raises_value_error(self):
        with self.assertRaises(ValueError):
            int("")

    def test_new_with_int_returns_int(self):
        self.assertEqual(int(23), 23)

    def test_new_with_int_and_base_raises_type_error(self):
        with self.assertRaises(TypeError):
            int(4, 5)

    def test_new_with_invalid_base_raises_value_error(self):
        with self.assertRaises(ValueError):
            int("0", 1)

    def test_new_with_invalid_chars_raises_value_error(self):
        with self.assertRaises(ValueError):
            int("&2*")

    def test_new_with_invalid_digits_raises_value_error(self):
        with self.assertRaises(ValueError):
            int(b"789", 6)

    def test_new_with_str_returns_int(self):
        self.assertEqual(int("23"), 23)
        self.assertEqual(int("-23", 8), -0o23)
        self.assertEqual(int("-abc", 16), -0xABC)
        self.assertEqual(int("0xabc", 0), 0xABC)

    def test_new_with_zero_args_returns_zero(self):
        self.assertIs(int(), 0)


class IsInstanceTests(unittest.TestCase):
    def test_isinstance_with_same_types_returns_true(self):
        self.assertIs(isinstance(1, int), True)

    def test_isinstance_with_subclass_returns_true(self):
        self.assertIs(isinstance(False, int), True)

    def test_isinstance_with_superclass_returns_false(self):
        self.assertIs(isinstance(2, bool), False)

    def test_isinstance_with_type_and_metaclass_returns_true(self):
        self.assertIs(isinstance(list, type), True)

    def test_isinstance_with_type_returns_true(self):
        self.assertIs(isinstance(type, type), True)

    def test_isinstance_with_object_type_returns_true(self):
        self.assertIs(isinstance(object, object), True)

    def test_isinstance_with_int_type_returns_false(self):
        self.assertIs(isinstance(int, int), False)

    def test_isinstance_with_unrelated_types_returns_false(self):
        self.assertIs(isinstance(int, (dict, bytes, str)), False)

    def test_isinstance_with_superclass_tuple_returns_true(self):
        self.assertIs(isinstance(True, (int, "bad - not a type")), True)

    def test_isinstance_with_non_type_superclass_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            isinstance(4, "bad - not a type")
        self.assertEqual(
            str(context.exception),
            "isinstance() arg 2 must be a type or tuple of types",
        )

    def test_isinstance_with_non_type_in_tuple_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            isinstance(5, ("bad - not a type", int))
        self.assertEqual(
            str(context.exception),
            "isinstance() arg 2 must be a type or tuple of types",
        )

    def test_isinstance_with_multiple_inheritance_returns_true(self):
        class A:
            pass

        class B(A):
            pass

        class C(A):
            pass

        class D(B, C):
            pass

        d = D()

        # D() is an instance of all specified superclasses
        self.assertIs(isinstance(d, A), True)
        self.assertIs(isinstance(d, B), True)
        self.assertIs(isinstance(d, C), True)
        self.assertIs(isinstance(d, D), True)

        # D() is not an instance of builtin types except object
        self.assertIs(isinstance(d, object), True)
        self.assertIs(isinstance(d, list), False)

        # D is an instance type, but D() is not
        self.assertIs(isinstance(D, type), True)
        self.assertIs(isinstance(d, type), False)

    def test_isinstance_with_type_checks_instance_type_and_dunder_class(self):
        class A(int):
            __class__ = list

        a = A()
        self.assertIs(isinstance(a, int), True)
        self.assertIs(isinstance(a, list), True)

    def test_isinstance_with_nontype_checks_dunder_bases_and_dunder_class(self):
        class A:
            __bases__ = ()

        a = A()

        class B:
            __bases__ = (a,)

        b = B()

        class C(int):
            __class__ = b
            __bases__ = (int,)

        c = C()
        self.assertIs(isinstance(c, a), True)
        self.assertIs(isinstance(c, b), True)
        self.assertIs(isinstance(c, c), False)

    def test_isinstance_with_non_tuple_dunder_bases_raises_type_error(self):
        class A:
            __bases__ = 5

        with self.assertRaises(TypeError) as context:
            isinstance(5, A())
        self.assertEqual(
            str(context.exception),
            "isinstance() arg 2 must be a type or tuple of types",
        )

    def test_isinstance_calls_custom_instancecheck_true(self):
        class Meta(type):
            def __instancecheck__(cls, obj):
                return [1]

        class A(metaclass=Meta):
            pass

        self.assertIs(isinstance(0, A), True)

    def test_isinstance_calls_custom_instancecheck_false(self):
        class Meta(type):
            def __instancecheck__(cls, obj):
                return None

        class A(metaclass=Meta):
            pass

        class B(A):
            pass

        self.assertIs(isinstance(A(), A), True)
        self.assertIs(isinstance(B(), A), False)


class IsSubclassTests(unittest.TestCase):
    def test_issubclass_with_same_types_returns_true(self):
        self.assertIs(issubclass(int, int), True)

    def test_issubclass_with_subclass_returns_true(self):
        self.assertIs(issubclass(bool, int), True)

    def test_issubclass_with_superclass_returns_false(self):
        self.assertIs(issubclass(int, bool), False)

    def test_issubclass_with_unrelated_types_returns_false(self):
        self.assertIs(issubclass(int, (dict, bytes, str)), False)

    def test_issubclass_with_superclass_tuple_returns_true(self):
        self.assertIs(issubclass(bool, (int, "bad - not a type")), True)

    def test_issubclass_with_non_type_subclass_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            issubclass("bad - not a type", str)
        self.assertEqual(str(context.exception), "issubclass() arg 1 must be a class")

    def test_issubclass_with_non_type_superclass_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            issubclass(int, "bad - not a type")
        self.assertEqual(
            str(context.exception),
            "issubclass() arg 2 must be a class or tuple of classes",
        )

    def test_issubclass_with_non_type_in_tuple_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            issubclass(bool, ("bad - not a type", int))
        self.assertEqual(
            str(context.exception),
            "issubclass() arg 2 must be a class or tuple of classes",
        )

    def test_issubclass_with_non_type_subclass_uses_bases(self):
        class A:
            __bases__ = (list,)

        self.assertIs(issubclass(A(), list), True)


class IterTests(unittest.TestCase):
    def test_iter_with_no_dunder_iter_raises_type_error(self):
        class C:
            pass

        c = C()

        with self.assertRaises(TypeError) as context:
            iter(c)

        self.assertEqual(str(context.exception), "'C' object is not iterable")

    def test_iter_with_dunder_iter_calls_dunder_iter(self):
        class C:
            def __iter__(self):
                raise UserWarning("foo")

        c = C()

        with self.assertRaises(UserWarning) as context:
            iter(c)

        self.assertEqual(str(context.exception), "foo")

    def test_iter_with_raising_descriptor_dunder_iter_raises_type_error(self):
        dunder_get_called = False

        class Desc:
            def __get__(self, obj, type):
                nonlocal dunder_get_called
                dunder_get_called = True
                raise UserWarning("foo")

        class C:
            __iter__ = Desc()

        c = C()

        with self.assertRaises(TypeError) as context:
            iter(c)

        self.assertEqual(str(context.exception), "'C' object is not iterable")
        self.assertTrue(dunder_get_called)


class ObjectTests(unittest.TestCase):
    def test_dunder_subclasshook_returns_not_implemented(self):
        self.assertIs(object.__subclasshook__(), NotImplemented)
        self.assertIs(object.__subclasshook__(int), NotImplemented)


class ReversedTests(unittest.TestCase):
    def test_reversed_iterates_backwards_over_iterable(self):
        it = reversed([1, 2, 3])
        self.assertEqual(it.__next__(), 3)
        self.assertEqual(it.__next__(), 2)
        self.assertEqual(it.__next__(), 1)
        with self.assertRaises(StopIteration):
            it.__next__()

    def test_reversed_calls_dunder_reverse(self):
        class C:
            def __reversed__(self):
                return "foo"

        self.assertEqual(reversed(C()), "foo")

    def test_reversed_with_none_dunder_reverse_raises_type_error(self):
        class C:
            __reversed__ = None

        with self.assertRaises(TypeError):
            reversed(C())

    def test_reversed_length_hint(self):
        it = reversed([1, 2, 3])
        self.assertEqual(it.__length_hint__(), 3)
        it.__next__()
        self.assertEqual(it.__length_hint__(), 2)
        it.__next__()
        self.assertEqual(it.__length_hint__(), 1)
        it.__next__()
        self.assertEqual(it.__length_hint__(), 0)


class ListTests(unittest.TestCase):
    def test_extend_with_iterator_that_raises_partway_through_has_sideeffect(self):
        class C:
            def __init__(self):
                self.n = 0

            def __iter__(self):
                return self

            def __next__(self):
                if self.n > 4:
                    raise UserWarning("foo")
                self.n += 1
                return self.n

        result = [0]
        with self.assertRaises(UserWarning):
            result.extend(C())
        self.assertEqual(result, [0, 1, 2, 3, 4, 5])

    def test_delitem_with_int_subclass_does_not_call_dunder_index(self):
        class C(int):
            def __index__(self):
                raise ValueError("foo")

        ls = list(range(5))
        del ls[C(0)]
        self.assertEqual(ls, [1, 2, 3, 4])

    def test_delitem_with_dunder_index_calls_dunder_index(self):
        class C:
            def __index__(self):
                return 2

        ls = list(range(5))
        del ls[C()]
        self.assertEqual(ls, [0, 1, 3, 4])

    def test_delslice_negative_indexes_removes_first_element(self):
        a = [0, 1]
        del a[-2:-1]
        self.assertEqual(a, [1])

    def test_delslice_negative_start_no_stop_removes_trailing_elements(self):
        a = [0, 1]
        del a[-1:]
        self.assertEqual(a, [0])

    def test_delslice_with_negative_step_deletes_every_other_element(self):
        a = [0, 1, 2, 3, 4]
        del a[::-2]
        self.assertEqual(a, [1, 3])

    def test_delslice_with_start_and_negative_step_deletes_every_other_element(self):
        a = [0, 1, 2, 3, 4]
        del a[1::-2]
        self.assertEqual(a, [0, 2, 3, 4])

    def test_delslice_with_large_step_deletes_last_element(self):
        a = [0, 1, 2, 3, 4]
        del a[4 :: 1 << 333]
        self.assertEqual(a, [0, 1, 2, 3])

    def test_getitem_with_int_subclass_does_not_call_dunder_index(self):
        class C(int):
            def __index__(self):
                raise ValueError("foo")

        ls = list(range(5))
        self.assertEqual(ls[C(3)], 3)

    def test_getitem_with_string_raises_type_error(self):
        ls = list(range(5))
        with self.assertRaises(TypeError) as context:
            ls["3"]
        self.assertEqual(
            str(context.exception), "list indices must be integers or slices, not str"
        )

    def test_getitem_with_dunder_index_calls_dunder_index(self):
        class C:
            def __index__(self):
                return 2

        ls = list(range(5))
        self.assertEqual(ls[C()], 2)

    def test_getslice_with_valid_indices_returns_sublist(self):
        ls = list(range(5))
        self.assertEqual(ls[2:-1:1], [2, 3])

    def test_getslice_with_negative_start_returns_trailing(self):
        ls = list(range(5))
        self.assertEqual(ls[-2:], [3, 4])

    def test_getslice_with_positive_stop_returns_leading(self):
        ls = list(range(5))
        self.assertEqual(ls[:2], [0, 1])

    def test_getslice_with_negative_stop_returns_all_but_trailing(self):
        ls = list(range(5))
        self.assertEqual(ls[:-2], [0, 1, 2])

    def test_getslice_with_positive_step_returns_forwards_list(self):
        ls = list(range(5))
        self.assertEqual(ls[::2], [0, 2, 4])

    def test_getslice_with_negative_step_returns_backwards_list(self):
        ls = list(range(5))
        self.assertEqual(ls[::-2], [4, 2, 0])

    def test_getslice_with_large_negative_start_returns_copy(self):
        ls = list(range(5))
        self.assertEqual(ls[-10:], ls)

    def test_getslice_with_large_positive_start_returns_empty(self):
        ls = list(range(5))
        self.assertEqual(ls[10:], [])

    def test_getslice_with_large_negative_start_returns_empty(self):
        ls = list(range(5))
        self.assertEqual(ls[:-10], [])

    def test_getslice_with_large_positive_start_returns_copy(self):
        ls = list(range(5))
        self.assertEqual(ls[:10], ls)

    def test_getslice_with_identity_slice_returns_copy(self):
        ls = list(range(5))
        copy = ls[::]
        self.assertEqual(copy, ls)
        self.assertIsNot(copy, ls)


class SetTests(unittest.TestCase):
    def test_discard_with_non_set_raises_type_error(self):
        with self.assertRaises(TypeError):
            set.discard(None, 1)

    def test_discard_with_non_member_returns_none(self):
        self.assertIs(set.discard(set(), 1), None)

    def test_discard_with_member_removes_element(self):
        s = {1, 2, 3}
        self.assertIn(1, s)
        self.assertIs(set.discard(s, 1), None)
        self.assertNotIn(1, s)

    def test_remove_with_non_set_raises_type_error(self):
        with self.assertRaises(TypeError):
            set.remove(None, 1)

    def test_remove_with_non_member_raises_key_error(self):
        with self.assertRaises(KeyError):
            set.remove(set(), 1)

    def test_remove_with_member_removes_element(self):
        s = {1, 2, 3}
        self.assertIn(1, s)
        set.remove(s, 1)
        self.assertNotIn(1, s)

    def test_inplace_with_non_set_raises_type_error(self):
        a = frozenset()
        self.assertRaises(TypeError, set.__ior__, a, set())

    def test_inplace_with_non_set_as_other_returns_unimplemented(self):
        a = set()
        result = set.__ior__(a, 1)
        self.assertEqual(len(a), 0)
        self.assertIs(result, NotImplemented)

    def test_inplace_or_modifies_self(self):
        a = set()
        b = {"foo"}
        result = set.__ior__(a, b)
        self.assertIs(result, a)
        self.assertEqual(len(a), 1)
        self.assertIn("foo", a)


class StrTests(unittest.TestCase):
    def test_format_single_open_curly_brace_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            str.format("{")
        self.assertEqual(
            str(context.exception), "Single '{' encountered in format string"
        )

    def test_format_single_close_curly_brace_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            str.format("}")
        self.assertEqual(
            str(context.exception), "Single '}' encountered in format string"
        )

    def test_format_index_out_of_args_raises_index_error(self):
        with self.assertRaises(IndexError) as context:
            str.format("{1}", 4)
        self.assertEqual(str(context.exception), "tuple index out of range")

    def test_format_not_existing_key_in_kwargs_raises_key_error(self):
        with self.assertRaises(KeyError) as context:
            str.format("{a}", b=4)
        self.assertEqual(str(context.exception), "'a'")

    def test_format_not_index_field_not_in_kwargs_raises_key_error(self):
        with self.assertRaises(KeyError) as context:
            str.format("{-1}", b=4)
        self.assertEqual(str(context.exception), "'-1'")

    def test_format_str_without_field_returns_itself(self):
        result = str.format("no field added")
        self.assertEqual(result, "no field added")

    def test_format_double_open_curly_braces_returns_single(self):
        result = str.format("{{")
        self.assertEqual(result, "{")

    def test_format_double_close_open_curly_braces_returns_single(self):
        result = str.format("}}")
        self.assertEqual(result, "}")

    def test_format_auto_index_field_returns_str_replaced_for_matching_args(self):
        result = str.format("a{}b{}c", 0, 1)
        self.assertEqual(result, "a0b1c")

    def test_format_auto_index_field_with_explicit_index_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            str.format("a{}b{0}c", 0)
        self.assertEqual(
            str(context.exception),
            "cannot switch from automatic field numbering to manual "
            "field specification",
        )

    def test_format_auto_index_field_with_keyword_returns_formatted_str(self):
        result = str.format("a{}b{keyword}c{}d", 0, 1, keyword=888)
        self.assertEqual(result, "a0b888c1d")

    def test_format_explicit_index_fields_returns_formatted_str(self):
        result = str.format("a{2}b{1}c{0}d{1}e", 0, 1, 2)
        self.assertEqual(result, "a2b1c0d1e")

    def test_format_keyword_fields_returns_formatted_str(self):
        result = str.format("1{a}2{b}3{c}4{b}5", a="a", b="b", c="c")
        self.assertEqual(result, "1a2b3c4b5")

    def test_join_with_raising_descriptor_dunder_iter_raises_type_error(self):
        class Desc:
            def __get__(self, obj, type):
                raise UserWarning("foo")

        class C:
            __iter__ = Desc()

        with self.assertRaises(TypeError):
            str.join("", C())

    def test_join_with_non_string_in_items_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            str.join(",", ["hello", 1])
        self.assertEqual(
            str(context.exception), "sequence item 1: expected str instance, int found"
        )

    def test_join_with_non_string_separator_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            str.join(None, ["hello", "world"])
        self.assertTrue(
            str(context.exception).endswith(
                "'join' requires a 'str' object but received a 'NoneType'"
            )
        )

    def test_join_with_empty_items_returns_empty_string(self):
        result = str.join(",", ())
        self.assertEqual(result, "")

    def test_join_with_one_item_returns_item(self):
        result = str.join(",", ("foo",))
        self.assertEqual(result, "foo")

    def test_join_with_empty_separator_uses_empty_string(self):
        result = str.join("", ("1", "2", "3"))
        self.assertEqual(result, "123")

    def test_join_with_tuple_joins_elements(self):
        result = str.join(",", ("1", "2", "3"))
        self.assertEqual(result, "1,2,3")

    def test_join_with_tuple_subclass_calls_dunder_iter(self):
        class C(tuple):
            def __iter__(self):
                return ("a", "b", "c").__iter__()

        elements = C(("p", "q", "r"))
        result = "".join(elements)
        self.assertEqual(result, "abc")

    def test_splitlines_with_non_str_raises_type_error(self):
        with self.assertRaises(TypeError):
            str.splitlines(None)

    def test_splitlines_with_float_keepends_raises_type_error(self):
        with self.assertRaises(TypeError):
            str.splitlines("hello", 0.4)

    def test_splitlines_with_non_int_calls_dunder_int_non_bool(self):
        class C:
            def __int__(self):
                return 100

        self.assertEqual(str.splitlines("\n", C()), ["\n"])

    def test_splitlines_with_non_int_calls_dunder_int_true(self):
        class C:
            def __int__(self):
                return 1

        self.assertEqual(str.splitlines("\n", C()), ["\n"])

    def test_splitlines_with_non_int_calls_dunder_int_false(self):
        class C:
            def __int__(self):
                return 0

        self.assertEqual(str.splitlines("\n", C()), [""])

    def test_str_new_with_bytes_and_no_encoding_returns_str(self):
        decoded = str(b"abc")
        self.assertEqual(decoded, "b'abc'")

    def test_str_new_with_str_raises_type_error(self):
        with self.assertRaises(TypeError):
            str("", encoding="utf_8")

    def test_str_new_with_non_bytes_raises_type_error(self):
        with self.assertRaises(TypeError):
            str(1, encoding="utf_8")

    def test_str_new_with_bytes_and_encoding_returns_decoded_str(self):
        decoded = str(b"abc", encoding="ascii")
        self.assertEqual(decoded, "abc")

    def test_str_new_with_no_object_and_encoding_returns_empty_string(self):
        self.assertEqual(str(encoding="ascii"), "")

    def test_str_new_with_class_without_dunder_str_returns_str(self):
        class A:
            def __repr__(self):
                return "test"

        self.assertEqual(str(A()), "test")

    def test_str_new_with_class_with_faulty_dunder_str_raises_type_error(self):
        with self.assertRaises(TypeError):

            class A:
                def __str__(self):
                    return 1

            str(A())

    def test_str_new_with_class_with_proper_duner_str_returns_str(self):
        class A:
            def __str__(self):
                return "test"

        self.assertEqual(str(A()), "test")


class SumTests(unittest.TestCase):
    def test_str_as_start_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            sum([], "")
        self.assertEqual(
            str(context.exception), "sum() can't sum strings [use ''.join(seq) instead]"
        )

    def test_bytes_as_start_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            sum([], b"")
        self.assertEqual(
            str(context.exception), "sum() can't sum bytes [use b''.join(seq) instead]"
        )

    def test_bytearray_as_start_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            sum([], bytearray())
        self.assertEqual(
            str(context.exception),
            "sum() can't sum bytearray [use b''.join(seq) instead]",
        )

    def test_non_iterable_raises_type_error(self):
        with self.assertRaises(TypeError):
            sum(None)
        # TODO(T43043290): Check for the exception message.

    def test_int_list_without_start_returns_sum(self):
        result = sum([1, 2, 3])
        self.assertEqual(result, 6)

    def test_int_list_with_start_returns_sum(self):
        result = sum([1, 2, 3], -6)
        self.assertEqual(result, 0)


class TupleTests(unittest.TestCase):
    def test_dunder_new_with_no_iterable_arg_returns_empty_tuple(self):
        result = tuple.__new__(tuple)
        self.assertIs(result, ())

    def test_dunder_new_with_iterable_returns_tuple_with_elements(self):
        result = tuple.__new__(tuple, [1, 2, 3])
        self.assertEqual(result, (1, 2, 3))

    def test_dunder_new_with_raising_dunder_iter_descriptor_raises_type_error(self):
        class Desc:
            def __get__(self, obj, type):
                raise UserWarning("foo")

        class C:
            __iter__ = Desc()

        with self.assertRaises(TypeError) as context:
            tuple(C())
        self.assertEqual(str(context.exception), "'C' object is not iterable")

    def test_dunder_new_with_raising_dunder_next_descriptor_propagates_exception(self):
        class Desc:
            def __get__(self, obj, type):
                raise UserWarning("foo")

        class C:
            def __iter__(self):
                return self

            __next__ = Desc()

        with self.assertRaises(UserWarning):
            tuple(C())


class TypeTests(unittest.TestCase):
    def test_abstract_methods_get_with_builtin_type_raises_attribute_error(self):
        with self.assertRaises(AttributeError) as context:
            type.__abstractmethods__
        self.assertEqual(str(context.exception), "__abstractmethods__")

    def test_abstract_methods_get_with_type_subclass_raises_attribute_error(self):
        class Foo(type):
            pass

        with self.assertRaises(AttributeError) as context:
            Foo.__abstractmethods__
        self.assertEqual(str(context.exception), "__abstractmethods__")

    def test_abstract_methods_set_with_builtin_type_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            int.__abstractmethods__ = ["foo"]
        self.assertEqual(
            str(context.exception),
            "can't set attributes of built-in/extension type 'int'",
        )

    def test_abstract_methods_get_with_type_subclass_sets_attribute(self):
        class Foo(type):
            pass

        Foo.__abstractmethods__ = 1
        self.assertEqual(Foo.__abstractmethods__, 1)

    def test_abstract_methods_del_with_builtin_type_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            del str.__abstractmethods__
        self.assertEqual(
            str(context.exception),
            "can't set attributes of built-in/extension type 'str'",
        )

    def test_abstract_methods_del_unset_with_type_subclass_raises_attribute_error(self):
        class Foo(type):
            pass

        with self.assertRaises(AttributeError) as context:
            del Foo.__abstractmethods__
        self.assertEqual(str(context.exception), "__abstractmethods__")

    def test_dunder_bases_del_with_builtin_type_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            del object.__bases__
        self.assertEqual(
            str(context.exception),
            "can't set attributes of built-in/extension type 'object'",
        )

    def test_dunder_bases_del_with_user_type_raises_type_error(self):
        class C:
            pass

        with self.assertRaises(TypeError) as context:
            del C.__bases__
        self.assertEqual(str(context.exception), "can't delete C.__bases__")

    def test_dunder_bases_get_with_builtin_type_returns_tuple(self):
        self.assertEqual(object.__bases__, ())
        self.assertEqual(type.__bases__, (object,))
        self.assertEqual(int.__bases__, (object,))
        self.assertEqual(bool.__bases__, (int,))

    def test_dunder_bases_get_with_user_type_returns_tuple(self):
        class A:
            pass

        class B:
            pass

        class C(A):
            pass

        class D(C, B):
            pass

        self.assertEqual(A.__bases__, (object,))
        self.assertEqual(B.__bases__, (object,))
        self.assertEqual(C.__bases__, (A,))
        self.assertEqual(D.__bases__, (C, B))

    def test_dunder_subclasses_with_leaf_type_returns_empty_list(self):
        class C:
            pass

        self.assertEqual(C.__subclasses__(), [])

    def test_dunder_subclasses_with_supertype_returns_list(self):
        class C:
            pass

        class D(C):
            pass

        self.assertEqual(C.__subclasses__(), [D])

    def test_dunder_subclasses_returns_new_list(self):
        class C:
            pass

        self.assertIsNot(C.__subclasses__(), C.__subclasses__())

    def test_mro_with_custom_method_propagates_exception(self):
        class Meta(type):
            def mro(cls):
                raise KeyError

        with self.assertRaises(KeyError):

            class Foo(metaclass=Meta):
                pass

    def test_setattr_with_metaclass_does_not_abort(self):
        class Meta(type):
            pass

        class C(metaclass=Meta):
            __slots__ = "attr"

            def __init__(self, data):
                self.attr = data

        m = C("foo")
        self.assertEqual(m.attr, "foo")
        m.attr = "bar"
        self.assertEqual(m.attr, "bar")


class RangeTests(unittest.TestCase):
    def test_range_attrs_are_set(self):
        obj = range(0, 1, 2)
        self.assertEqual(obj.start, 0)
        self.assertEqual(obj.stop, 1)
        self.assertEqual(obj.step, 2)

    def test_range_start_is_readonly(self):
        with self.assertRaises(AttributeError):
            range(0, 1, 2).start = 2

    def test_range_step_is_readonly(self):
        with self.assertRaises(AttributeError):
            range(0, 1, 2).step = 2

    def test_range_stop_is_readonly(self):
        with self.assertRaises(AttributeError):
            range(0, 1, 2).stop = 2


if __name__ == "__main__":
    unittest.main()
