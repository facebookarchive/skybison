#!/usr/bin/env python3
import os
import sys
import tempfile
import unittest
from distutils.core import setup
from distutils.dist import Distribution
from distutils.extension import Extension

import _imp


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


if __name__ == "__main__":
    unittest.main()
