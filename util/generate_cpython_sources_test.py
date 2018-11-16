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

#define Bar(o) Foo;

#define FooBaz(o)       \\
    { Baz(type) },
"""
        res = gcs.find_symbols_in_file(gcs.SYMBOL_REGEX, lines)["macro"]
        self.assertListEqual(res, ["Foo", "Bar"])

    def test_multiline_macro_regex_returns_symbol(self):
        lines = """
#define Foo(o)       \\
    { Baz(o) },
"""
        res = gcs.find_symbols_in_file(gcs.SYMBOL_REGEX, lines)[
            "multiline_macro"
        ]
        self.assertEqual(res, ["Foo"])

    def test_multiline_macro_regex_returns_multiple_symbols(self):
        lines = """
#define Foo       \\
    { Baz(type) },

typedef int FooBar;

#define FooBaz(o) Foo,

#define Bar(op)     \\
    do {            \\
        int a = 1;  \\
        if (a == 1) \\
          Foo       \\
        else        \\
          Baz(a)    \\
    } while (0)
"""
        res = gcs.find_symbols_in_file(gcs.SYMBOL_REGEX, lines)[
            "multiline_macro"
        ]
        self.assertListEqual(res, ["Foo", "Bar"])


class TestDefinitionRegex(unittest.TestCase):
    def test_typedef_definition_is_replaced(self):
        original_lines = """
typedef type Foo; // Comment
"""
        expected_lines = """
 // Comment
"""
        symbols_to_replace = {"typedef": ["Foo"]}
        res = gcs.modify_file(original_lines, symbols_to_replace)
        self.assertEqual(res, expected_lines)

    def test_multiple_typedef_definitions_are_replaced(self):
        original_lines = """
typedef type1 Foo;

typedef void (*Bar)(void *);

typedef struct newtype {
  int foo_bar;
} Foo_Bar;

typedef foo_bar Baz;
"""
        expected_lines = """




typedef struct newtype {
  int foo_bar;
} Foo_Bar;


"""
        symbols_to_replace = {"typedef": ["Foo", "Bar", "Baz"]}
        res = gcs.modify_file(original_lines, symbols_to_replace)
        self.assertEqual(res, expected_lines)

    def test_multiline_typedef_definition_is_replaced(self):
        original_lines = """
typedef struct {
  Bar bar;
  Baz baz; /* Comment */
} Foo;
"""
        expected_lines = """

"""
        symbols_to_replace = {"multiline_typedef": ["Foo"]}
        res = gcs.modify_file(original_lines, symbols_to_replace)
        self.assertEqual(res, expected_lines)

    def test_multiple_multiline_typedef_definitions_are_replaced(self):
        original_lines = """
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
        expected_lines = """
typedef int Baz;





struct FooBarBaz {
  Foobar* foobar;
};
"""
        symbols_to_replace = {"multiline_typedef": ["Foo", "Bar"]}
        res = gcs.modify_file(original_lines, symbols_to_replace)
        self.assertEqual(res, expected_lines)

    def test_struct_definition_is_replaced(self):
        original_lines = """
struct Foo {
  Bar bar; /* Comment */
  Baz baz;
};
"""
        expected_lines = """

"""
        symbols_to_replace = {"struct": ["Foo"]}
        res = gcs.modify_file(original_lines, symbols_to_replace)
        self.assertEqual(res, expected_lines)

    def test_multiple_struct_definitions_are_replaced(self):
        original_lines = """
struct Foo {
  Baz baz;
};

typedef int FooBar;

#define FooBaz 1,

struct Bar {
  Baz baz;
};
"""
        expected_lines = """


typedef int FooBar;

#define FooBaz 1,


"""
        symbols_to_replace = {"struct": ["Foo", "Bar"]}
        res = gcs.modify_file(original_lines, symbols_to_replace)
        self.assertEqual(res, expected_lines)

    def test_macro_definition_is_replaced(self):
        original_lines = """
#define Foo 0, // Comment
"""
        expected_lines = """
"""
        symbols_to_replace = {"macro": ["Foo"]}
        res = gcs.modify_file(original_lines, symbols_to_replace)
        self.assertEqual(res, expected_lines)

    def test_multiple_macro_definitions_are_replaced(self):
        original_lines = """
#define Foo 0,

typedef int FooBar;

#define Bar(o) Foo;

#define FooBaz(o)       \\
    { Baz(type) },
"""
        expected_lines = """
typedef int FooBar;

#define FooBaz(o)       \\
    { Baz(type) },
"""
        symbols_to_replace = {"macro": ["Foo", "Bar"]}
        res = gcs.modify_file(original_lines, symbols_to_replace)
        self.assertEqual(res, expected_lines)

    def test_multiline_macro_definition_is_replaced(self):
        original_lines = """
#define Foo(o)       \\
    { Baz(o) },
"""
        expected_lines = """

"""
        symbols_to_replace = {"multiline_macro": ["Foo"]}
        res = gcs.modify_file(original_lines, symbols_to_replace)
        self.assertEqual(res, expected_lines)

    def test_multiline_macro_definitions_are_replaced(self):
        original_lines = """
#define Foo       \\
    { Baz(type) },

typedef int FooBar;

#define FooBaz(o) Foo,

#define Bar(op)     \\
    do {            \\
        int a = 1;  \\
        if (a == 1) \\
          Foo    \\
        else        \\
          Baz(a)    \\
    } while (0)
"""
        expected_lines = """


typedef int FooBar;

#define FooBaz(o) Foo,


"""
        symbols_to_replace = {"multiline_macro": ["Foo", "Bar"]}
        res = gcs.modify_file(original_lines, symbols_to_replace)
        self.assertEqual(res, expected_lines)


if __name__ == "__main__":
    unittest.main()
