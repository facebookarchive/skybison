#!/usr/bin/env python3
import unittest

import generate_cpython_sources as gcs


class TestSymbolRegex(unittest.TestCase):
    def test_typedef_regex_returns_multiple_symbols(self):
        lines = """
typedef type1 Foo; // Comment

typedef void (*Bar)(void *);

typedef struct newtype {
  int foo_bar;
} Foo_Bar;

typedef struct Hello World;

typedef foo_bar Baz;

typedef PyObject *(*Func)(PyObject *, PyObject *);
"""
        symbols_dict = gcs.find_symbols_in_file(lines, gcs.HEADER_SYMBOL_REGEX)
        res = symbols_dict["typedef"]
        self.assertListEqual(res, ["World", "Foo", "Bar", "Baz", "Func", "Foo_Bar"])

    def test_struct_regex_returns_multiple_symbols(self):
        lines = """
struct Foo {
  Baz baz1; /* Comment */
  Baz baz2;
};

typedef int FooBar;

#define FooBaz 1,

struct Bar {
  Baz baz;
};
"""
        symbols_dict = gcs.find_symbols_in_file(lines, gcs.HEADER_SYMBOL_REGEX)
        res = symbols_dict["struct"]
        self.assertListEqual(res, ["Foo", "Bar"])

    def test_enum_regex_returns_multiple_symbols(self):
        lines = """
enum Foo {
  baz1 = 0, /* Comment */
  baz2 = 1,
};

typedef int FooBar;

#define FooBaz 1,

enum Bar {
  baz3 = 0;
};
"""
        symbols_dict = gcs.find_symbols_in_file(lines, gcs.HEADER_SYMBOL_REGEX)
        res = symbols_dict["enum"]
        self.assertListEqual(res, ["Foo", "Bar"])

    def test_macro_regex_returns_multiple_symbols(self):
        lines = """
#define Foo 0, // Comment

typedef int FooBar;

#define Bar(o) Foo;

#define FooBaz(o)       \\
    { Baz(type) },
"""
        symbols_dict = gcs.find_symbols_in_file(lines, gcs.HEADER_SYMBOL_REGEX)
        res = symbols_dict["macro"]
        self.assertListEqual(res, ["Foo", "Bar", "FooBaz"])

    def test_pytypeobject_regex_returns_multiple_symbols(self):
        lines = """
extern "C" PyTypeObject* Foo_Type_Ptr() {
  Thread* thread = Thread::currentThread();
}

extern "C" PyObject* Foo_Function(void) {
  // Some implementation
}

extern "C" PyTypeObject *Bar_Type_Ptr() {
  Thread* thread = Thread::currentThread();
}
"""
        symbols_dict = gcs.find_symbols_in_file(lines, gcs.SOURCE_SYMBOL_REGEX)
        res = symbols_dict["pytypeobject"]
        self.assertListEqual(res, ["Foo_Type", "Bar_Type"])

    def test_pytypeobject_macro_regex_returns_multiple_symbols(self):
        lines = """
#define Foo_Type (*Foo_Type_Ptr())

#define Foo       \\
    { Baz(type) },

typedef int FooBar;

#define Bar_Type (*Bar_Type_Ptr())

#define FooBaz(o) Foo,
"""
        symbols_dict = gcs.find_symbols_in_file(lines, gcs.HEADER_SYMBOL_REGEX)
        res = symbols_dict["pytypeobject_macro"]
        self.assertEqual(res, ["Foo_Type", "Bar_Type"])

    def test_pyexc_macro_regex_returns_multiple_symbols(self):
        lines = """
#define PyExc_FooError ((PyObject *)&(*PyExc_FooError_Ptr()))

#define Foo_Type (*Foo_Type_Ptr())
#define PyExc_BarError ((PyObject *)&(*PyExc_BarError_Ptr()))

typedef int FooBar;

#define FooBaz(o) Foo,
"""
        symbols_dict = gcs.find_symbols_in_file(lines, gcs.HEADER_SYMBOL_REGEX)
        res = symbols_dict["pyexc_macro"]
        self.assertEqual(res, ["PyExc_FooError", "PyExc_BarError"])

    def test_pyfunction_regex_returns_multiple_symbols(self):
        lines = """
PY_EXPORT type* foo_function() {
  // Implmementation
}

void Foo_Type_Init(void) {
  // Implementation
}

PY_EXPORT PyTypeObject *bar_function() {
  // Implmementation
}

PY_EXPORT PyObject *baz_function_with_many_args(PyObject *, PyObject *,
                                                PyObject *, PyObject *) {
  // Implementation
}
"""
        symbols_dict = gcs.find_symbols_in_file(lines, gcs.SOURCE_SYMBOL_REGEX)
        res = symbols_dict["pyfunction"]
        self.assertEqual(
            res, ["foo_function", "bar_function", "baz_function_with_many_args"]
        )


class TestDefinitionRegex(unittest.TestCase):
    def test_pytypeobject_typedef_is_modified_to_struct_definition(self):
        original_lines = """
typedef struct _typeobject {
  int foo;
} PyTypeObject;
"""
        expected_lines = """
struct _typeobject {
  int foo;
};
"""
        symbols_to_replace = {"typedef": ["PyTypeObject"]}
        res = gcs.modify_file(
            original_lines, symbols_to_replace, gcs.HEADER_DEFINITIONS_REGEX
        )
        self.assertEqual(res, expected_lines)

    def test_multiple_typedef_definitions_are_replaced(self):
        original_lines = """
typedef type1 Foo; // Comment

typedef void (*Bar)(void *);

typedef struct newtype {
  int foo_bar;
} Foo_Bar;

typedef foo_bar Baz;

typedef struct Foo FooAlias;

typedef struct Bar BarAlias;

typedef PyObject *(*Foo)(PyObject *, PyObject *);
"""
        expected_lines = """
typedef struct newtype {
  int foo_bar;
} Foo_Bar;

typedef struct Foo FooAlias;
"""
        symbols_to_replace = {"typedef": ["Foo", "Bar", "Baz", "BarAlias"]}
        res = gcs.modify_file(
            original_lines, symbols_to_replace, gcs.HEADER_DEFINITIONS_REGEX
        )
        self.assertEqual(res, expected_lines)

    def test_multiple_multiline_typedef_definitions_are_replaced(self):
        original_lines = """
typedef int Baz;

typedef struct {
  Foo *foo1;
  Foo *foo2; /* Comment */
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
        symbols_to_replace = {"typedef": ["Foo", "Bar"]}
        res = gcs.modify_file(
            original_lines, symbols_to_replace, gcs.HEADER_DEFINITIONS_REGEX
        )
        self.assertEqual(res, expected_lines)

    def test_multiple_struct_definitions_are_replaced(self):
        original_lines = """
struct Foo {
  Baz baz1; /* Comment */
  Baz baz2;
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
        res = gcs.modify_file(
            original_lines, symbols_to_replace, gcs.HEADER_DEFINITIONS_REGEX
        )
        self.assertEqual(res, expected_lines)

    def test_multiple_macro_definitions_are_replaced(self):
        original_lines = """
#define Foo 0, // Comment

typedef int FooBar;

#define Bar(o) Foo;  /* Multi-line
                        comment */

#define FooBaz(o)       \\
    { Baz(type) },
"""
        expected_lines = """
// Comment

typedef int FooBar;

/* Multi-line
                        comment */

#define FooBaz(o)       \\
    { Baz(type) },
"""
        symbols_to_replace = {"macro": ["Foo", "Bar"]}
        res = gcs.modify_file(
            original_lines, symbols_to_replace, gcs.HEADER_DEFINITIONS_REGEX
        )
        self.assertEqual(res, expected_lines)

    def test_multiline_macro_definitions_are_replaced(self):
        original_lines = """
#define Foo       \\
    { Baz(1) },

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
        expected_lines = """

typedef int FooBar;

#define FooBaz(o) Foo,

"""
        symbols_to_replace = {"macro": ["Foo", "Bar"]}
        res = gcs.modify_file(
            original_lines, symbols_to_replace, gcs.HEADER_DEFINITIONS_REGEX
        )
        self.assertEqual(res, expected_lines)

    def test_pytypeobject_definitions_are_replaced(self):
        original_lines = """
PyObject *Foo_Function(void)
{
  // Some implementation
};

PyTypeObject Foo_Type = {
  // Some implementation
};

PyObject *Bar_Function(void)
{
  // Some implementation
};

PyTypeObject Bar_Type = {
  // Some implementation
};

PyTypeObject Baz_Type = {
  // Some implementation
};
"""
        expected_lines = """
PyObject *Foo_Function(void)
{
  // Some implementation
};


PyObject *Bar_Function(void)
{
  // Some implementation
};


PyTypeObject Baz_Type = {
  // Some implementation
};
"""
        symbols_to_replace = {"pytypeobject": ["Foo_Type", "Bar_Type"]}
        res = gcs.modify_file(
            original_lines, symbols_to_replace, gcs.SOURCE_DEFINITIONS_REGEX
        )
        self.assertEqual(res, expected_lines)

    def test_pytypeobject_macro_definitions_are_replaced(self):
        original_lines = """
PyAPI_DATA(PyTypeObject) Foo_Type;

#define Foo       \\
    { Baz(1) },

typedef int FooBar;

PyAPI_DATA(PyTypeObject) Bar_Type;

#define FooBaz(o) Foo,
"""
        expected_lines = """

#define Foo       \\
    { Baz(1) },

typedef int FooBar;


#define FooBaz(o) Foo,
"""
        symbols_to_replace = {"pytypeobject_macro": ["Foo_Type", "Bar_Type"]}
        res = gcs.modify_file(
            original_lines, symbols_to_replace, gcs.HEADER_DEFINITIONS_REGEX
        )
        self.assertEqual(res, expected_lines)

    def test_pyexc_macro_definitions_are_replaced(self):
        original_lines = """
# foo

PyAPI_DATA(PyObject *) PyExc_FooError;

#define Foo       \\
    { Baz(1) },

typedef int FooBar;

PyAPI_DATA(PyTypeObject) Bar_Type;

#define FooBaz(o) Foo,
"""
        expected_lines = """
# foo


#define Foo       \\
    { Baz(1) },

typedef int FooBar;

PyAPI_DATA(PyTypeObject) Bar_Type;

#define FooBaz(o) Foo,
"""
        symbols_to_replace = {"pyexc_macro": ["PyExc_FooError"]}
        res = gcs.modify_file(
            original_lines, symbols_to_replace, gcs.HEADER_DEFINITIONS_REGEX
        )
        self.assertEqual(res, expected_lines)

    def test_pyfunction_definitions_are_replaced(self):
        original_lines = """
static type *global_str = NULL;

type*
foo_function(type *arg1, type arg2, ...)
{
  // Implementation
}

Type foobar_function(
  type *arg1) {
  // Implementation
}

/* Function comments*/
static type bar_function(void) {
  // Implementation
}

static int forward_declare(PyTypeObject *);

Type baz_function(
  type *arg1,
  type arg2,
  type* arg3)
{
  // Implementation
}

static type foobaz_function(void)
{
  // Implementation
}

int counter = 1;
"""
        expected_lines = """
static type *global_str = NULL;



Type foobar_function(
  type *arg1) {
  // Implementation
}

/* Function comments*/


static int forward_declare(PyTypeObject *);



static type foobaz_function(void)
{
  // Implementation
}

int counter = 1;
"""
        symbols_to_replace = {
            "pyfunction": ["foo_function", "bar_function", "baz_function"]
        }
        res = gcs.modify_file(
            original_lines, symbols_to_replace, gcs.SOURCE_DEFINITIONS_REGEX
        )
        self.assertEqual(res, expected_lines)

    def test_pyfunction_extern_is_ignored(self):
        original_lines = """
#ifdef __cplusplus
extern "C" {
#endif

static type *global_str = NULL;

type*
foo_function(type *arg1, type arg2, ...)
{
  // Implementation
}
"""
        expected_lines = """
#ifdef __cplusplus
extern "C" {
#endif

static type *global_str = NULL;


"""
        symbols_to_replace = {"pyfunction": ["foo_function"]}
        res = gcs.modify_file(
            original_lines, symbols_to_replace, gcs.SOURCE_DEFINITIONS_REGEX
        )
        self.assertEqual(res, expected_lines)


if __name__ == "__main__":
    unittest.main()
