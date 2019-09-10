#!/usr/bin/env python3
import unittest
from collections import defaultdict, deque, namedtuple


class DefaultdictTests(unittest.TestCase):
    def test_dunder_init_returns_dict_subclass(self):
        result = defaultdict()
        self.assertIsInstance(result, dict)
        self.assertIsInstance(result, defaultdict)
        self.assertIsNone(result.default_factory)

    def test_dunder_init_with_non_callable_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            defaultdict(5)

        self.assertIn("must be callable or None", str(context.exception))

    def test_dunder_init_sets_default_factory(self):
        def foo():
            pass

        result = defaultdict(foo)
        self.assertIs(result.default_factory, foo)

    def test_dunder_getitem_calls_dunder_missing(self):
        def foo():
            return "value"

        result = defaultdict(foo)
        self.assertEqual(len(result), 0)
        self.assertEqual(result["hello"], "value")
        self.assertEqual(result[5], "value")
        self.assertEqual(len(result), 2)

    def test_dunder_missing_with_none_default_factory_raises_key_error(self):
        result = defaultdict()
        with self.assertRaises(KeyError) as context:
            result.__missing__("hello")

        self.assertEqual(context.exception.args, ("hello",))

    def test_dunder_missing_calls_default_factory_function(self):
        def foo():
            raise UserWarning("foo")

        result = defaultdict(foo)
        with self.assertRaises(UserWarning) as context:
            result.__missing__("hello")

        self.assertEqual(context.exception.args, ("foo",))

    def test_dunder_missing_calls_default_factory_callable(self):
        class A:
            def __call__(self):
                raise UserWarning("foo")

        result = defaultdict(A())
        with self.assertRaises(UserWarning) as context:
            result.__missing__("hello")

        self.assertEqual(context.exception.args, ("foo",))

    def test_dunder_missing_sets_value_returned_from_default_factory(self):
        def foo():
            return 5

        result = defaultdict(foo)
        self.assertEqual(result, {})
        self.assertEqual(result.__missing__("hello"), 5)
        self.assertEqual(result, {"hello": 5})

    def test_dunder_repr_with_no_default_factory(self):
        empty = defaultdict()
        self.assertEqual(empty.__repr__(), "defaultdict(None, {})")

    def test_dunder_repr_with_default_factory_calls_factory_repr(self):
        class A:
            def __call__(self):
                pass

            def __repr__(self):
                return "foo"

            def __str__(self):
                return "bar"

        empty = defaultdict(A())
        self.assertEqual(empty.__repr__(), "defaultdict(foo, {})")

    def test_dunder_repr_stringifies_elements(self):
        result = defaultdict()
        result["a"] = "b"
        self.assertEqual(result.__repr__(), "defaultdict(None, {'a': 'b'})")

    def test_clear_removes_elements(self):
        def foo():
            pass

        result = defaultdict(foo)
        result["a"] = "b"
        self.assertEqual(len(result), 1)
        self.assertIsNone(result.clear())
        self.assertIs(result.default_factory, foo)
        self.assertEqual(len(result), 0)


class DequeTests(unittest.TestCase):
    def test_dunder_bool_with_empty_returns_false(self):
        result = deque()
        self.assertEqual(result.__bool__(), False)

    def test_dunder_bool_with_non_empty_returns_true(self):
        result = deque([1, 2, 3])
        self.assertEqual(result.__bool__(), True)

    def test_dunder_contains_with_empty_returns_false(self):
        result = deque()
        self.assertEqual(result.__contains__(1), False)

    def test_dunder_contains_with_non_empty_returns_true(self):
        result = deque([1, 2, 3])
        self.assertEqual(result.__contains__(2), True)

    def test_dunder_contains_checks_identity(self):
        class C:
            def __eq__(self, other):
                return False

        c = C()
        result = deque([1, c, 3])
        self.assertEqual(result.__contains__(c), True)

    def test_dunder_new_calls_super_dunder_new(self):
        class C(deque):
            def __new__(cls):
                raise UserWarning("foo")

        with self.assertRaises(UserWarning):
            C()

    def test_dunder_new_does_not_call_super_clear(self):
        class C(deque):
            def clear(self):
                raise UserWarning("foo")

        C()

    def test_dunder_init_with_negative_maxlen_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            deque(maxlen=-1)

        self.assertEqual(str(context.exception), "maxlen must be non-negative")

    def test_dunder_init_with_non_int_maxlen_raises_type_error(self):
        with self.assertRaises(TypeError):
            deque(maxlen="hello")

    def test_dunder_init_sets_maxlen(self):
        result = deque(maxlen=5)
        self.assertEqual(result.maxlen, 5)

    def test_dunder_init_calls_dunder_iter_on_iterable(self):
        class C:
            def __iter__(self):
                raise UserWarning("foo")

        iterable = C()
        with self.assertRaises(UserWarning):
            deque(iterable=iterable)

    def test_dunder_init_adds_elements_from_iterable(self):
        result = deque(iterable=[1, 2, 3])
        self.assertEqual(len(result), 3)
        self.assertIn(1, result)
        self.assertIn(2, result)
        self.assertIn(3, result)

    def test_dunder_copy_with_non_deque_raises_type_error(self):
        self.assertRaises(TypeError, deque.__copy__, None)
        with self.assertRaises(TypeError) as context:
            deque.__copy__(None)

        self.assertIn(
            "requires a 'collections.deque' object but received a 'NoneType'",
            str(context.exception),
        )

    def test_dunder_copy_returns_copy(self):
        orig = deque([1, 2, 3])
        copy = orig.__copy__()
        self.assertEqual(orig, copy)
        self.assertIsNot(orig, copy)

    def test_dunder_delitem_with_int_subclass_does_not_call_dunder_index(self):
        class C(int):
            def __index__(self):
                raise ValueError("foo")

        result = deque([1, 2, 3])
        deque.__delitem__(result, C(0))
        self.assertEqual(list(result), [2, 3])

    def test_dunder_delitem_with_raising_descriptor_propagates_exception(self):
        class Desc:
            def __get__(self, obj, type):
                raise AttributeError("foo")

        class C:
            __index__ = Desc()

        result = deque([1, 2, 3])
        with self.assertRaises(AttributeError) as context:
            result.__delitem__(C())
        self.assertEqual(str(context.exception), "foo")

    def test_dunder_delitem_with_non_int_raises_type_error(self):
        result = deque([1, 2, 3])
        with self.assertRaises(TypeError) as context:
            result.__delitem__("3")
        self.assertEqual(
            str(context.exception), "'str' object cannot be interpreted as an integer"
        )

    def test_dunder_delitem_with_dunder_index_calls_dunder_index(self):
        class C:
            def __index__(self):
                return 2

        result = deque([1, 2, 3])
        result.__delitem__(C())
        self.assertEqual(list(result), [1, 2])

    def test_dunder_delitem_index_too_small_raises_index_error(self):
        result = deque()
        with self.assertRaises(IndexError) as context:
            result.__delitem__(-1)
        self.assertEqual(str(context.exception), "deque index out of range")

    def test_dunder_delitem_index_too_large_raises_index_error(self):
        result = deque([1, 2, 3])
        with self.assertRaises(IndexError) as context:
            result.__delitem__(3)
        self.assertEqual(str(context.exception), "deque index out of range")

    def test_dunder_delitem_negative_index_relative_to_end(self):
        result = deque([1, 2, 3])
        result.__delitem__(-3)
        self.assertEqual(list(result), [2, 3])

    def test_dunder_delitem_with_slice_raises_type_error(self):
        result = deque([1, 2, 3, 4, 5])
        self.assertRaises(TypeError, result.__delitem__, slice(0, 1))

    def test_dunder_eq_with_different_length_returns_false(self):
        left = deque([1, 2, 3])
        right = deque([1, 2, 3, 4])
        self.assertFalse(left.__eq__(right))

    def test_dunder_eq_with_non_deque_returns_not_implemented(self):
        left = deque([1, 2, 3])
        right = 5
        self.assertIs(left.__eq__(right), NotImplemented)

    def test_dunder_eq_with_identity_equal_but_inequal_elements_returns_true(self):
        nan = float("nan")
        left = deque([1, nan, 3])
        right = deque([1, nan, 3])
        self.assertTrue(left.__eq__(right))

    def test_dunder_eq_compares_item_equality_dunder_eq(self):
        class C:
            def __eq__(self, other):
                return True

        left = deque([1, 2, 3])
        right = deque([1, C(), 3])
        self.assertTrue(left.__eq__(right))

    def test_dunder_eq_compares_item_equality(self):
        left = deque([1, 2, 3])
        right = deque([1, 2, 3])
        self.assertTrue(left.__eq__(right))

    def test_dunder_iadd_calls_dunder_iter_on_rhs(self):
        class C:
            def __iter__(self):
                return [4, 5, 6].__iter__()

        left = deque([1, 2, 3])
        right = C()
        result = left.__iadd__(right)
        self.assertIs(result, left)
        self.assertEqual(list(left), [1, 2, 3, 4, 5, 6])

    def test_dunder_iadd_returns_left(self):
        left = deque([1, 2, 3])
        right = [4]
        result = left.__iadd__(right)
        self.assertIs(result, left)
        self.assertEqual(list(left), [1, 2, 3, 4])
        self.assertEqual(list(right), [4])

    def test_dunder_getitem_with_int_subclass_does_not_call_dunder_index(self):
        class C(int):
            def __index__(self):
                raise ValueError("foo")

        result = deque([1, 2, 3])
        self.assertEqual(deque.__getitem__(result, C(0)), 1)

    def test_dunder_getitem_with_raising_descriptor_propagates_exception(self):
        class Desc:
            def __get__(self, obj, type):
                raise AttributeError("foo")

        class C:
            __index__ = Desc()

        result = deque([1, 2, 3])
        with self.assertRaises(AttributeError) as context:
            result.__getitem__(C())
        self.assertEqual(str(context.exception), "foo")

    def test_dunder_getitem_with_non_int_raises_type_error(self):
        result = deque([1, 2, 3])
        with self.assertRaises(TypeError) as context:
            result.__getitem__("3")
        self.assertEqual(
            str(context.exception), "'str' object cannot be interpreted as an integer"
        )

    def test_dunder_getitem_with_dunder_index_calls_dunder_index(self):
        class C:
            def __index__(self):
                return 2

        result = deque([1, 2, 3])
        self.assertEqual(result.__getitem__(C()), 3)

    def test_dunder_getitem_index_too_small_raises_index_error(self):
        result = deque()
        with self.assertRaises(IndexError) as context:
            result.__getitem__(-1)
        self.assertEqual(str(context.exception), "deque index out of range")

    def test_dunder_getitem_index_too_large_raises_index_error(self):
        result = deque([1, 2, 3])
        with self.assertRaises(IndexError) as context:
            result.__getitem__(3)
        self.assertEqual(str(context.exception), "deque index out of range")

    def test_dunder_getitem_negative_index_relative_to_end(self):
        result = deque([1, 2, 3])
        self.assertEqual(result.__getitem__(-3), 1)

    def test_dunder_getitem_with_slice_raises_type_error(self):
        result = deque([1, 2, 3, 4, 5])
        self.assertRaises(TypeError, result.__getitem__, slice(2, -1))

    def test_dunder_hash_is_none(self):
        self.assertIsNone(deque.__hash__)

    def test_dunder_len_returns_length(self):
        result = deque([1, 2, 3])
        self.assertEqual(result.__len__(), 3)

    def test_dunder_repr(self):
        result = deque([1, "foo", 3])
        self.assertEqual(result.__repr__(), "deque([1, 'foo', 3])")

    def test_dunder_repr_recursive(self):
        result = deque([1, "foo", 3])
        result.append(result)
        self.assertEqual(result.__repr__(), "deque([1, 'foo', 3, [...]])")

    def test_dunder_repr_with_maxlen(self):
        result = deque([1, 2, 3], maxlen=4)
        self.assertEqual(result.__repr__(), "deque([1, 2, 3], maxlen=4)")

    def test_dunder_setitem_with_int_subclass_does_not_call_dunder_index(self):
        class C(int):
            def __index__(self):
                raise ValueError("foo")

        result = deque([1, 2, 3])
        deque.__setitem__(result, C(0), 4)
        self.assertEqual(list(result), [4, 2, 3])

    def test_dunder_setitem_with_raising_descriptor_propagates_exception(self):
        class Desc:
            def __get__(self, obj, type):
                raise AttributeError("foo")

        class C:
            __index__ = Desc()

        result = deque([1, 2, 3])
        with self.assertRaises(AttributeError) as context:
            result.__setitem__(C(), 10)
        self.assertEqual(str(context.exception), "foo")

    def test_dunder_setitem_with_non_int_raises_type_error(self):
        result = deque([1, 2, 3])
        with self.assertRaises(TypeError) as context:
            result.__setitem__("3", 10)
        self.assertEqual(
            str(context.exception), "'str' object cannot be interpreted as an integer"
        )

    def test_dunder_setitem_with_dunder_index_calls_dunder_index(self):
        class C:
            def __index__(self):
                return 2

        result = deque([1, 2, 3])
        result.__setitem__(C(), 10)
        self.assertEqual(list(result), [1, 2, 10])

    def test_dunder_setitem_index_too_small_raises_index_error(self):
        result = deque()
        with self.assertRaises(IndexError) as context:
            result.__setitem__(-1, 10)
        self.assertEqual(str(context.exception), "deque index out of range")

    def test_dunder_setitem_index_too_large_raises_index_error(self):
        result = deque([1, 2, 3])
        with self.assertRaises(IndexError) as context:
            result.__setitem__(3, 10)
        self.assertEqual(str(context.exception), "deque index out of range")

    def test_dunder_setitem_negative_index_relative_to_end(self):
        result = deque([1, 2, 3])
        result.__setitem__(-3, 10)
        self.assertEqual(list(result), [10, 2, 3])

    def test_dunder_setitem_with_slice_raises_type_error(self):
        result = deque([1, 2, 3, 4, 5])
        self.assertRaises(TypeError, result.__setitem__, slice(0, 1), [10])

    def test_append_adds_elements(self):
        result = deque()
        self.assertEqual(len(result), 0)
        result.append(1)
        result.append(2)
        result.append(3)
        self.assertEqual(len(result), 3)
        self.assertEqual(list(result), [1, 2, 3])

    def test_append_over_maxlen_removes_element_from_left(self):
        result = deque([1, 2, 3, 4, 5], maxlen=5)
        result.append("foo")
        self.assertEqual(list(result), [2, 3, 4, 5, "foo"])

    def test_appendleft_adds_elements_to_left(self):
        result = deque()
        self.assertEqual(len(result), 0)
        result.appendleft(1)
        result.appendleft(2)
        result.appendleft(3)
        self.assertEqual(len(result), 3)
        self.assertEqual(list(result), [3, 2, 1])

    def test_appendleft_over_maxlen_removes_element_from_right(self):
        result = deque([1, 2, 3], maxlen=3)
        result.appendleft("foo")
        self.assertEqual(list(result), ["foo", 1, 2])

    def test_clear_removes_elements(self):
        result = deque(iterable=[1, 2, 3])
        self.assertEqual(len(result), 3)
        result.clear()
        self.assertEqual(len(result), 0)
        self.assertNotIn(1, result)
        self.assertNotIn(2, result)
        self.assertNotIn(3, result)

    def test_extend_calls_dunder_iter_on_iterable(self):
        class C:
            def __iter__(self):
                raise UserWarning("foo")

        iterable = C()
        result = deque()
        with self.assertRaises(UserWarning):
            result.extend(iterable)

    def test_extend_with_self_copies_deque(self):
        result = deque([1, 2, 3])
        result.extend(result)
        self.assertEqual(list(result), [1, 2, 3, 1, 2, 3])

    def test_extend_adds_elements_from_iterable(self):
        class C:
            def __iter__(self):
                return [1, 2, 3].__iter__()

        result = deque()
        result.extend(C())
        self.assertEqual(list(result), [1, 2, 3])

    def test_extend_over_maxlen_removes_elements_from_right(self):
        result = deque([1, 2, 3], maxlen=3)
        result.extend([4])
        self.assertEqual(list(result), [2, 3, 4])

    def test_extendleft_calls_dunder_iter_on_iterable(self):
        class C:
            def __iter__(self):
                raise UserWarning("foo")

        iterable = C()
        result = deque()
        with self.assertRaises(UserWarning):
            result.extendleft(iterable)

    def test_extendleft_with_self_copies_deque(self):
        result = deque([1, 2, 3])
        result.extendleft(result)
        self.assertEqual(list(result), [3, 2, 1, 1, 2, 3])

    def test_extendleft_adds_elements_from_iterable(self):
        class C:
            def __iter__(self):
                return [1, 2, 3].__iter__()

        result = deque()
        result.extendleft(C())
        self.assertEqual(list(result), [3, 2, 1])

    def test_extendleft_over_maxlen_removes_elements_from_left(self):
        result = deque([1, 2, 3], maxlen=3)
        result.extendleft([4])
        self.assertEqual(list(result), [4, 1, 2])

    def test_pop_from_empty_deque_raises_index_error(self):
        result = deque()
        self.assertRaises(IndexError, result.pop)

    def test_pop_removes_element_from_right(self):
        result = deque([1, 2, 3])
        result.pop()
        self.assertEqual(list(result), [1, 2])

    def test_popleft_from_empty_deque_raises_index_error(self):
        result = deque()
        self.assertRaises(IndexError, result.popleft)

    def test_popleft_removes_element_from_left(self):
        result = deque([1, 2, 3])
        result.popleft()
        self.assertEqual(list(result), [2, 3])

    def test_count_counts_number_of_occurrences_with_dunder_eq(self):
        class C:
            def __eq__(self, other):
                return True

        result = deque([1, 2, 3])
        self.assertEqual(result.count(C()), 3)

    def test_count_counts_number_of_occurrences(self):
        result = deque([1, 2, 3, 2, 1])
        self.assertEqual(result.count(2), 2)

    def test_count_checks_identity(self):
        nan = float("nan")
        result = deque([1, nan, 3, nan])
        self.assertEqual(result.count(nan), 2)

    def test_remove_with_element_not_in_deque_raises_value_error(self):
        result = deque()
        self.assertRaises(ValueError, result.remove, 4)

    def test_remove_with_checks_identity(self):
        nan = float("nan")
        result = deque([1, nan, 3])
        result.remove(nan)
        self.assertEqual(list(result), [1, 3])

    def test_remove_removes_first_occurrence_of_element_from_left(self):
        result = deque([1, 2, 3, 2, 1])
        result.remove(2)
        self.assertEqual(list(result), [1, 3, 2, 1])

    def test_rotate_rotates_elements_right_by_one(self):
        result = deque([1, 2, 3])
        result.rotate()
        self.assertEqual(list(result), [3, 1, 2])

    def test_rotate_with_non_int_raises_type_error(self):
        result = deque()
        self.assertRaises(TypeError, result.rotate, "x")

    def test_rotate_rotates_elements_right_by_given_amount(self):
        result = deque([1, 2, 3, 4, 5])
        result.rotate(3)
        self.assertEqual(list(result), [3, 4, 5, 1, 2])

    def test_reverse_reverses_order_of_elements_in_place(self):
        result = deque([1, 2, 3])
        result.reverse()
        self.assertEqual(list(result), [3, 2, 1])


class NamedtupleTests(unittest.TestCase):
    def test_create_with_space_separated_field_names_splits_string(self):
        self.assertEqual(namedtuple("Foo", "a b")._fields, ("a", "b"))

    def test_create_with_rename_renames_bad_fields(self):
        result = namedtuple("Foo", ["a", "5", "b", "1"], rename=True)._fields
        self.assertEqual(result, ("a", "_1", "b", "_3"))

    def test_create_with_rename_renames_fields_starting_with_underscore(self):
        result = namedtuple("Foo", ["a", "5", "_b", "1"], rename=True)._fields
        self.assertEqual(result, ("a", "_1", "_2", "_3"))

    def test_repr_formats_fields(self):
        Foo = namedtuple("Foo", "a b")
        self.assertEqual(Foo(1, 2).__repr__(), "Foo(a=1, b=2)")

    def test_repr_formats_fields_with_different_str_repr(self):
        Foo = namedtuple("Foo", "a b")
        self.assertEqual(Foo(1, "bar").__repr__(), "Foo(a=1, b='bar')")

    def test_dunder_getattr_with_namedtuple_class_returns_descriptor(self):
        Foo = namedtuple("Foo", "a b")
        self.assertTrue(hasattr(Foo, "a"))
        self.assertTrue(hasattr(Foo.a, "__get__"))
        self.assertTrue(hasattr(Foo, "b"))
        self.assertTrue(hasattr(Foo.b, "__get__"))


if __name__ == "__main__":
    unittest.main()
