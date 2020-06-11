#!/usr/bin/env python3
import unittest
from collections import defaultdict, namedtuple


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

    def test_with_non_identifier_type_name_raises_value_error(self):
        with self.assertRaisesRegex(
            ValueError, "Type names and field names must be valid identifiers: '5'"
        ):
            namedtuple("5", ["a", "b"])

    def test_with_keyword_raises_value_error(self):
        with self.assertRaisesRegex(
            ValueError, "Type names and field names cannot be a keyword: 'from'"
        ):
            namedtuple("from", ["a", "b"])

    def test_with_field_starting_with_underscore_raises_value_error(self):
        with self.assertRaisesRegex(
            ValueError, "Field names cannot start with an underscore: '_a'"
        ):
            namedtuple("Foo", ["_a", "b"])

    def test_with_duplicate_field_name_raises_value_error(self):
        with self.assertRaisesRegex(
            ValueError, "Encountered duplicate field name: 'a'"
        ):
            namedtuple("Foo", ["a", "a"])

    def test_with_too_few_args_raises_type_error(self):
        Foo = namedtuple("Foo", ["a", "b"])
        with self.assertRaisesRegex(TypeError, "Expected 2 arguments, got 1"):
            Foo._make([1])

    def test_under_make_returns_new_instance(self):
        Foo = namedtuple("Foo", ["a", "b"])
        inst = Foo._make([1, 3])
        self.assertEqual(inst.a, 1)
        self.assertEqual(inst.b, 3)
        self.assertEqual(len(inst), 2)
        self.assertEqual(inst[0], 1)
        self.assertEqual(inst[1], 3)

    def test_under_replace_with_nonexistet_field_name_raises_value_error(self):
        Foo = namedtuple("Foo", ["a", "b"])
        with self.assertRaisesRegex(ValueError, "Got unexpected field names.*'x'.*"):
            Foo(1, 2)._replace(x=4)

    def test_under_replace_replaces_value_at_name(self):
        Foo = namedtuple("Foo", ["a", "b"])
        inst = Foo(1, 2)
        self.assertIs(inst.a, 1)
        self.assertIs(inst.b, 2)
        self.assertIs(inst[1], 2)
        inst = inst._replace(b=3)
        self.assertIs(inst.a, 1)
        self.assertIs(inst.b, 3)
        self.assertIs(inst[1], 3)


if __name__ == "__main__":
    unittest.main()
