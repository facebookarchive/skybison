#!/usr/bin/env python3
import _imp
import os
import sys
import tempfile
import unittest
from distutils.core import setup
from distutils.dist import Distribution
from distutils.extension import Extension

from test_support import pyro_only


class MockSpec:  # noqa: B903
    def __init__(self, name, origin):
        self.name = name
        self.origin = origin


class UnderImpModuleTest(unittest.TestCase):
    def compile_so(self, name, dir_path, file_path):
        ext = Extension(name=name, sources=[file_path])
        dist = setup(
            name="examples",
            ext_modules=[ext],
            script_args=["build_ext"],
            options={"build_ext": {"build_lib": dir_path}},
        )
        return dist

    def test_create_dynamic_invalid_shared_object_raises_exception(self):
        with tempfile.TemporaryDirectory() as dir_path:
            self.assertEqual(len(os.listdir(dir_path)), 0)
            file_path = f"{dir_path}/nonsharedobject.so"
            with open(file_path, "w") as c_file:
                c_file.write("\n")
            spec = MockSpec("nonsharedobject", file_path)
            with self.assertRaisesRegex(ImportError, "file too short"):
                _imp.create_dynamic(spec)

    def test_create_dynamic_without_init_raises_exception(self):
        with tempfile.TemporaryDirectory() as dir_path:
            self.assertEqual(len(os.listdir(dir_path)), 0)
            file_path = f"{dir_path}/nopyinit.c"
            with open(file_path, "w") as c_file:
                c_file.write(
                    """\
#include "Python.h"
PyObject* PyInit_returnsnull() {
  return NULL;
}
"""
                )
            self.assertEqual(len(os.listdir(dir_path)), 1)

            # Create shared object
            dist = self.compile_so("nopyinit", dir_path, file_path)
            self.assertIsInstance(dist, Distribution)

            # Check directory contents
            dir_contents = sorted(os.listdir(dir_path))
            self.assertEqual(len(dir_contents), 2)
            self.assertTrue(dir_contents[0].endswith(".c"))
            self.assertTrue(dir_contents[1].endswith(".so"))

            so_path = f"{dir_path}/{dir_contents[1]}"
            spec = MockSpec("nopyinit", so_path)
            with self.assertRaisesRegex(ImportError, "does not define"):
                _imp.create_dynamic(spec)

    def test_create_dynamic_init_returns_null_raises_exception(self):
        with tempfile.TemporaryDirectory() as dir_path:
            self.assertEqual(len(os.listdir(dir_path)), 0)
            file_path = f"{dir_path}/returnsnull.c"
            with open(file_path, "w") as c_file:
                c_file.write(
                    """\
#include "Python.h"
PyObject* PyInit_returnsnull() {
  return NULL;
}
"""
                )
            self.assertEqual(len(os.listdir(dir_path)), 1)

            # Create shared object
            dist = self.compile_so("returnsnull", dir_path, file_path)
            self.assertIsInstance(dist, Distribution)

            # Check directory contents
            dir_contents = sorted(os.listdir(dir_path))
            self.assertEqual(len(dir_contents), 2)
            self.assertTrue(dir_contents[0].endswith(".c"))
            self.assertTrue(dir_contents[1].endswith(".so"))

            sys.path.append(dir_path)
            with self.assertRaisesRegex(SystemError, "without raising"):
                import returnsnull  # noqa: F401
            sys.path.pop()

    def test_create_dynamic_returns_module(self):
        # Create C file
        with tempfile.TemporaryDirectory() as dir_path:
            self.assertEqual(len(os.listdir(dir_path)), 0)
            file_path = f"{dir_path}/imptestsoloads.c"
            with open(file_path, "w") as c_file:
                c_file.write(
                    """\
#include "Python.h"
PyObject* PyInit_imptestsoloads() {
  static PyModuleDef def;
  def.m_name = "imptestsoloads";
  return PyModule_Create(&def);
}
"""
                )
            self.assertEqual(len(os.listdir(dir_path)), 1)

            # Create shared object
            dist = self.compile_so("imptestsoloads", dir_path, file_path)
            self.assertIsInstance(dist, Distribution)

            # Check directory contents
            dir_contents = sorted(os.listdir(dir_path))
            self.assertEqual(len(dir_contents), 2)
            self.assertTrue(dir_contents[0].endswith(".c"))
            self.assertTrue(dir_contents[1].endswith(".so"))

            # Load shared_object
            sys.path.append(dir_path)
            try:
                import imptestsoloads
            except Exception:
                pass

            self.assertEqual(imptestsoloads.__name__, "imptestsoloads")
            sys.path.pop()

    def test_create_dynamic_adds_to_sys_modules(self):
        # Create C file
        with tempfile.TemporaryDirectory() as dir_path:
            self.assertEqual(len(os.listdir(dir_path)), 0)
            file_path = f"{dir_path}/addtomodules.c"
            with open(file_path, "w") as c_file:
                c_file.write(
                    """\
#include "Python.h"
PyObject* PyInit_addtomodules() {
  static PyModuleDef def;
  def.m_name = "addtomodules";
  return PyModule_Create(&def);
}
"""
                )
            self.assertEqual(len(os.listdir(dir_path)), 1)

            # Create shared object
            dist = self.compile_so("addtomodules", dir_path, file_path)
            self.assertIsInstance(dist, Distribution)

            # Check directory contents
            dir_contents = sorted(os.listdir(dir_path))
            self.assertEqual(len(dir_contents), 2)
            self.assertTrue(dir_contents[0].endswith(".c"))
            self.assertTrue(dir_contents[1].endswith(".so"))

            so_filename = os.path.join(dir_path, dir_contents[1])
            spec = MockSpec("foo.addtomodules", so_filename)
            self.assertNotIn("foo.addtomodules", sys.modules)
            self.assertNotIn("addtomodules", sys.modules)
            _imp.create_dynamic(spec)
            self.assertIn("foo.addtomodules", sys.modules)
            self.assertNotIn("addtomodules", sys.modules)

    def test_create_nested_dynamic_returns_module(self):
        # Create C file
        with tempfile.TemporaryDirectory() as dir_path:
            library_path = f"{dir_path}/nestedtest"
            os.mkdir(library_path)
            file_path = f"{library_path}/foo.c"
            with open(file_path, "w") as c_file:
                c_file.write(
                    """\
#include "Python.h"
PyObject* PyInit_foo() {
  static PyModuleDef def;
  def.m_name = "foo";
  return PyModule_Create(&def);
}
"""
                )

            # Create shared object
            dist = self.compile_so("foo", library_path, file_path)
            self.assertIsInstance(dist, Distribution)

            # Check directory contents
            dir_contents = sorted(os.listdir(library_path))
            self.assertEqual(len(dir_contents), 2)
            self.assertTrue(dir_contents[0].endswith(".c"))
            self.assertTrue(dir_contents[1].endswith(".so"))

            # Load shared_object
            sys.path.append(dir_path)
            try:
                import nestedtest.foo
            except Exception:
                pass

            self.assertIn("foo", nestedtest.foo.__name__)
            sys.path.pop()

    def test_create_dynamic_reading_global_variable(self):
        # Create C file
        with tempfile.TemporaryDirectory() as dir_path:
            self.assertEqual(len(os.listdir(dir_path)), 0)
            file_path = f"{dir_path}/foo.c"
            with open(file_path, "w") as c_file:
                c_file.write(
                    """\
#include "Python.h"
PyObject* PyInit_foo() {
  static PyModuleDef def;
  def.m_name = Py_FileSystemDefaultEncoding;
  return PyModule_Create(&def);
}
"""
                )
            self.assertEqual(len(os.listdir(dir_path)), 1)

            # Create shared object
            dist = self.compile_so("foo", dir_path, file_path)
            self.assertIsInstance(dist, Distribution)

            # Check directory contents
            dir_contents = sorted(os.listdir(dir_path))
            self.assertEqual(len(dir_contents), 2)
            self.assertTrue(dir_contents[0].endswith(".c"))
            self.assertTrue(dir_contents[1].endswith(".so"))

            # Load shared_object
            sys.path.append(dir_path)
            try:
                import foo

                self.assertEqual(foo.__name__, "utf-8")
                del sys.modules["foo"]
            finally:
                sys.path.pop()

    def test_create_dynamic_reading_PyOptimize_Flag(self):
        # Create C file
        with tempfile.TemporaryDirectory() as dir_path:
            self.assertEqual(len(os.listdir(dir_path)), 0)
            file_path = f"{dir_path}/foo.c"
            with open(file_path, "w") as c_file:
                c_file.write(
                    """\
#include "Python.h"
PyMODINIT_FUNC PyInit_foo() {
  static PyModuleDef def;
  def.m_name = "the foo module";
  def.m_size = Py_OptimizeFlag;
  return PyModule_Create(&def);
}
"""
                )
            self.assertEqual(len(os.listdir(dir_path)), 1)

            # Create shared object
            dist = self.compile_so("foo", dir_path, file_path)
            self.assertIsInstance(dist, Distribution)

            # Load shared_object
            sys.path.append(dir_path)
            try:
                import foo

                self.assertEqual(foo.__name__, "the foo module")
                del sys.modules["foo"]
            finally:
                sys.path.pop()

    def test_extension_suffixes_returns_list(self):
        result = _imp.extension_suffixes()
        self.assertIsInstance(result, list)
        self.assertTrue(len(result) > 0)

    def test_extension_suffixes_matches_sysconfig(self):
        import sysconfig

        suffixes = _imp.extension_suffixes()
        self.assertIn(sysconfig.get_config_var("EXT_SUFFIX"), suffixes)
        self.assertIn(".abi3" + sysconfig.get_config_var("SHLIB_SUFFIX"), suffixes)
        self.assertIn(sysconfig.get_config_var("SHLIB_SUFFIX"), suffixes)

    def test_fix_co_filename_updates_filenames_recursively(self):
        def foo():
            def bar():
                pass

        new_name = "foobar"
        foo_code = foo.__code__
        bar_code = foo_code.co_consts[1]
        self.assertIsInstance(bar_code, type(foo_code))
        self.assertNotEqual(foo_code.co_filename, new_name)
        self.assertNotEqual(bar_code.co_filename, new_name)
        _imp._fix_co_filename(foo_code, new_name)
        self.assertEqual(foo_code.co_filename, new_name)
        self.assertEqual(bar_code.co_filename, new_name)

    def test_fix_co_filename_with_str_subclass_returns_subclass(self):
        def foo():
            pass

        class C(str):
            pass

        new_name = C("foobar")
        foo_code = foo.__code__
        self.assertNotEqual(foo_code.co_filename, new_name)
        _imp._fix_co_filename(foo_code, new_name)
        self.assertIs(foo_code.co_filename, new_name)

    def test_source_hash_returns_bytes(self):
        result = _imp.source_hash(1234, b"foo")
        self.assertIs(type(result), bytes)
        self.assertEqual(len(result), 8)

    def test_source_hash_returns_same_value(self):
        result0 = _imp.source_hash(12345, b"foo bar")
        result1 = _imp.source_hash(12345, b"foo bar")
        self.assertEqual(result0, result1)

    def test_source_hash_with_long_input_returns_bytes(self):
        result = _imp.source_hash(1234, b"hello world foo bar baz bam long bytes!")
        self.assertIs(type(result), bytes)
        self.assertEqual(len(result), 8)

    def test_source_hash_with_different_key_returns_different_bytes(self):
        result0 = _imp.source_hash(1234, b"hello")
        result1 = _imp.source_hash(4321, b"hello")
        self.assertNotEqual(result0, result1)

    def test_source_hash_with_different_bytes_returns_different_bytes(self):
        result0 = _imp.source_hash(42, b"foo")
        result1 = _imp.source_hash(42, b"bar")
        self.assertNotEqual(result0, result1)

    @pyro_only
    def test_source_hash_returns_known_values(self):
        self.assertEqual(_imp.source_hash(123, b"foo"), b"\x8e\x13\x1cq\x0bZ\x1f;")
        self.assertEqual(
            _imp.source_hash(123, b"hello world foo bar baz bam long bytes!"),
            b"\x136g\xb0\xcf\xa9\x12R",
        )


if __name__ == "__main__":
    unittest.main()
