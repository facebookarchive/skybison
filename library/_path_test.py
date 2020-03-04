#!/usr/bin/env python3
import unittest

from test_support import pyro_only


try:
    import _path
except ImportError:
    pass


@pyro_only
class UnderPathTest(unittest.TestCase):
    def test_dirname_with_non_str_raises_type_error(self):
        with self.assertRaises(TypeError):
            _path.dirname(1)

    def test_dirname_handles_empty_string(self):
        self.assertEqual(_path.dirname(""), "")

    def test_dirname_keeps_stripping_path_until_root(self):
        path = "/foo/bar"
        path = _path.dirname(path)
        self.assertEqual(path, "/foo")
        path = _path.dirname(path)
        self.assertEqual(path, "/")
        path = _path.dirname(path)
        self.assertEqual(path, "/")

    def test_join_with_non_str_path_raises_type_error(self):
        with self.assertRaises(TypeError):
            _path.join(1)

        with self.assertRaises(TypeError):
            _path.join("path", 1)

    def test_join_with_single_path_returns_path(self):
        path = "/"
        self.assertEqual(_path.join(path), path)

    def test_join_with_concats_multiple_paths(self):
        self.assertEqual(_path.join("foo/", "bar"), "foo/bar")
        self.assertEqual(_path.join("foo", "bar", "baz"), "foo/bar/baz")

    def test_join_with_root_path_starts_at_root_path(self):
        self.assertEqual(_path.join("foo", "/bar"), "/bar")
        self.assertEqual(_path.join("foo", "/bar", "baz"), "/bar/baz")
        self.assertEqual(_path.join("foo", "/bar", "/baz"), "/baz")


if __name__ == "__main__":
    unittest.main()
