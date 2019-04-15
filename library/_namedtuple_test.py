#!/usr/bin/env python3
import unittest

# TODO(T42627145): Enable collections.namedtuple
from _namedtuple import namedtuple


class NamedtupleTests(unittest.TestCase):
    def test_create_with_space_separated_field_names_splits_string(self):
        self.assertEqual(namedtuple("Foo", "a b")._fields, ("a", "b"))

    def test_create_with_rename_renames_bad_fields(self):
        result = namedtuple("Foo", ["a", "5", "b", "1"], rename=True)._fields
        self.assertEqual(result, ("a", "_1", "b", "_3"))

    def test_create_with_rename_renames_fields_starting_with_underscore(self):
        result = namedtuple("Foo", ["a", "5", "_b", "1"], rename=True)._fields
        self.assertEqual(result, ("a", "_1", "_2", "_3"))


if __name__ == "__main__":
    unittest.main()
