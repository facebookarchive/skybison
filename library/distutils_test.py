#!/usr/bin/env python3
import os
import tempfile
import unittest
from distutils.core import setup
from distutils.dist import Distribution
from distutils.extension import Extension


class DistutilsModuleTest(unittest.TestCase):
    def test_so_compiles(self):
        # Create C file
        with tempfile.TemporaryDirectory() as dir_path:
            self.assertEqual(len(os.listdir(dir_path)), 0)
            file_path = f"{dir_path}/foo.c"
            with open(file_path, "w") as c_file:
                c_file.write(
                    """\
#include "Python.h"
PyObject* PyInit_foo() {
  static PyModuleDef def = {};
  def.m_name = "foo";
  return PyModule_Create(&def);
}
"""
                )
            self.assertEqual(len(os.listdir(dir_path)), 1)

            # Setup compilation settings
            ext = Extension(name="foo", sources=[file_path])
            self.assertIsInstance(ext, Extension)

            # Compile C File
            dist = setup(
                name="test_so_compilation",
                ext_modules=[ext],
                script_args=["build_ext"],
                options={"build_ext": {"build_lib": dir_path}},
            )
            self.assertIsInstance(dist, Distribution)

            # Check directory contents
            dir_contents = os.listdir(dir_path)
            self.assertEqual(len(dir_contents), 2)
            dir_contents[0].endswith(".so")
            dir_contents[1].endswith(".c")


if __name__ == "__main__":
    unittest.main()
