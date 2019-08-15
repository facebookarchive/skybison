#!/usr/bin/env python3
import unittest
from collections import namedtuple


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
