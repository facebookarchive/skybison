#!/usr/bin/env python3
import unittest
import patch_cpython_sources as pcs


class TestTypeAccess(unittest.TestCase):
    def test_object_to_type(self):
        lines = """
type->ob_type
"""
        expected_lines = """
Py_TYPE(type)
"""
        res = pcs.object_to_type(lines, "")
        self.assertEqual(res, expected_lines)

    def test_object_to_ref_type(self):
        lines = """
type.ob_type
"""
        expected_lines = """
Py_TYPE(&type)
"""
        res = pcs.object_to_type(lines, "")
        self.assertEqual(res, expected_lines)

    def test_two_chain_object_to_type(self):
        lines = """
a->ob_type->tp_repr
"""
        expected_lines = """
Py_TYPE(a)->tp_repr
"""
        res = pcs.object_to_type(lines, "")
        self.assertEqual(res, expected_lines)

    def test_three_chain_object_to_type(self):
        lines = """
op->ob_ref->ob_type->tp_name
"""
        expected_lines = """
Py_TYPE(op->ob_ref)->tp_name
"""
        res = pcs.object_to_type(lines, "")
        self.assertEqual(res, expected_lines)

    def test_parenthesis_object_to_type(self):
        lines = """
((obj)->ob_type->tp_as_buffer != NULL)
"""
        expected_lines = """
(Py_TYPE(obj)->tp_as_buffer != NULL)
"""
        res = pcs.object_to_type(lines, "")
        self.assertEqual(res, expected_lines)

    def test_square_bracket_object_to_type(self):
        lines = """
lo.keys[0]->ob_type
"""
        expected_lines = """
Py_TYPE(lo.keys[0])
"""
        res = pcs.object_to_type(lines, "")
        self.assertEqual(res, expected_lines)

    def test_object_to_type_shamodule(self):
        lines = """
if (((PyObject*)self)->ob_type == &type) {
"""
        expected_lines = """
if (Py_TYPE(self) == &type) {
"""
        res = pcs.object_to_type(lines, "sha512module.c")
        self.assertEqual(res, expected_lines)


if __name__ == "__main__":
    unittest.main()
