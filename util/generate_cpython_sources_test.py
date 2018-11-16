#!/usr/bin/env python3
import unittest
import generate_cpython_sources as gcs


class TestSymbolRegex(unittest.TestCase):
    def test_typedef_regex_returns_symbol(self):
        lines = """
typedef type Foo; // Comment
"""
        res = gcs.find_symbols_in_file(gcs.SYMBOL_REGEX, lines)["typedef"]
        self.assertListEqual(res, ["Foo"])

    def test_typedef_regex_returns_multiple_symbols(self):
        lines = """
typedef type1 Foo;

typedef void (*Bar)(void *);

typedef struct newtype {
  int foo_bar;
} Foo_Bar;

typedef foo_bar Baz;
"""
        res = gcs.find_symbols_in_file(gcs.SYMBOL_REGEX, lines)["typedef"]
        self.assertListEqual(res, ["Foo", "Bar", "Baz"])

    def test_multiline_typedef_regex_returns_symbol(self):
        lines = """
typedef struct {
  Bar bar;
  Baz baz; /* Comment */
} Foo;
"""
        res = gcs.find_symbols_in_file(gcs.SYMBOL_REGEX, lines)[
            "multiline_typedef"
        ]
        self.assertEqual(res, ["Foo"])

    def test_multiline_typedef_regex_returns_multiple_symbols(self):
        lines = """
typedef int Baz;

typedef struct {
  Foo *foo1;
  Foo *foo2;
} Foo;

typedef struct {
  int bar;
} Bar;

struct FooBarBaz {
  Foobar* foobar;
};
"""
        res = gcs.find_symbols_in_file(gcs.SYMBOL_REGEX, lines)[
            "multiline_typedef"
        ]
        self.assertListEqual(res, ["Foo", "Bar"])

    def test_struct_regex_returns_symbol(self):
        lines = """
struct Foo {
  Bar bar; /* Comment */
  Baz baz;
};
"""
        res = gcs.find_symbols_in_file(gcs.SYMBOL_REGEX, lines)["struct"]
        self.assertEqual(res, ["Foo"])

    def test_struct_regex_returns_multiple_symbols(self):
        lines = """
struct Foo {
  Baz baz;
};

typedef int FooBar;

#define FooBaz 1,

struct Bar {
  Baz baz;
};
"""
        res = gcs.find_symbols_in_file(gcs.SYMBOL_REGEX, lines)["struct"]
        self.assertListEqual(res, ["Foo", "Bar"])

    def test_macro_regex_returns_symbol(self):
        lines = """
#define Foo 0, // Comment
"""
        res = gcs.find_symbols_in_file(gcs.SYMBOL_REGEX, lines)["macro"]
        self.assertEqual(res, ["Foo"])

    def test_macro_regex_returns_multiple_symbols(self):
        lines = """
#define Foo 0,

typedef int FooBar;

#define Bar type *bar;
"""
        res = gcs.find_symbols_in_file(gcs.SYMBOL_REGEX, lines)["macro"]
        self.assertListEqual(res, ["Foo", "Bar"])


if __name__ == "__main__":
    unittest.main()
