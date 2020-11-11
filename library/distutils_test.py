#!/usr/bin/env python3
import _imp
import distutils.sysconfig
import importlib.machinery
import os
import tempfile
import unittest
from distutils.core import setup
from distutils.dist import Distribution
from distutils.extension import Extension


class DistutilsModuleTest(unittest.TestCase):
    def test_sysconfig_get_python_inc_returns_string(self):
        python_inc = distutils.sysconfig.get_python_inc()
        self.assertTrue(os.path.isdir(python_inc))
        self.assertTrue(os.path.isfile(os.path.join(python_inc, "Python.h")))

    def test_sysconfig_get_python_lib_returns_string(self):
        python_lib = distutils.sysconfig.get_python_lib()
        self.assertIn("site-packages", python_lib)

    def test_build_ext_with_c_file_creates_library(self):
        with tempfile.TemporaryDirectory() as dir_path:
            file_path = f"{dir_path}/foo.c"
            with open(file_path, "w") as fp:
                fp.write(
                    """\
#include "Python.h"
PyObject *func(PyObject *a0, PyObject *a1) {
    (void)a0;
    (void)a1;
    return PyUnicode_FromString("and now for something completely different");
}
static PyMethodDef methods[] = {
    {"announce", func, METH_NOARGS, "bar docu"},
    {NULL, NULL},
};
static PyModuleDef def = {
    PyModuleDef_HEAD_INIT,
    "foo",
    NULL,
    0,
    methods,
};
PyMODINIT_FUNC PyInit_foo(void) {
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
                script_args=["--quiet", "build_ext"],
                options={"build_ext": {"build_lib": dir_path}},
            )
            self.assertIsInstance(dist, Distribution)

            # Check directory contents
            dir_contents = sorted(os.listdir(dir_path))
            self.assertEqual(len(dir_contents), 2)
            self.assertTrue(dir_contents[0].endswith(".c"))
            self.assertTrue(dir_contents[1].endswith(".so"))

            lib_file = f"{dir_path}/{dir_contents[1]}"
            spec = importlib.machinery.ModuleSpec("foo", None, origin=lib_file)
            module = _imp.create_dynamic(spec)
            self.assertEqual(
                module.announce(), "and now for something completely different"
            )

    def test_build_ext_with_cpp_file_creates_library(self):
        with tempfile.TemporaryDirectory() as dir_path:
            file_path = f"{dir_path}/foo.cpp"
            with open(file_path, "w") as fp:
                fp.write(
                    """\
#include <sstream>
#include "Python.h"
PyObject* func(PyObject*, PyObject*) {
    std::stringstream ss;
    ss << "Hello" << ' ' << "world";
    return PyUnicode_FromString(ss.str().c_str());
}
static PyMethodDef methods[] = {
    {"greet", func, METH_NOARGS, "bar docu"},
    {nullptr, nullptr},
};
static PyModuleDef def = {
    PyModuleDef_HEAD_INIT,
    "foo",
    nullptr,
    0,
    methods,
};
PyMODINIT_FUNC PyInit_foo() {
    return PyModule_Create(&def);
}
"""
                )
            self.assertEqual(len(os.listdir(dir_path)), 1)

            # Setup compilation settings
            ext = Extension(name="foo", sources=[file_path])
            self.assertIsInstance(ext, Extension)

            dist = setup(
                name="test_so_compilation",
                ext_modules=[ext],
                script_args=["--quiet", "build_ext"],
                options={"build_ext": {"build_lib": dir_path}},
            )
            self.assertIsInstance(dist, Distribution)

            # Check directory contents
            dir_contents = sorted(os.listdir(dir_path))
            self.assertEqual(len(dir_contents), 2)
            self.assertTrue(dir_contents[0].endswith(".cpp"))
            self.assertTrue(dir_contents[1].endswith(".so"))

            lib_file = f"{dir_path}/{dir_contents[1]}"
            spec = importlib.machinery.ModuleSpec("foo", None, origin=lib_file)
            module = _imp.create_dynamic(spec)
            self.assertEqual(module.greet(), "Hello world")


if __name__ == "__main__":
    unittest.main()
