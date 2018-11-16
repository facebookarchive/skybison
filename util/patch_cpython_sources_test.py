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


class TestSlotAccess(unittest.TestCase):
    def test_two_chain_slot_to_macro(self):
        lines = """
foo->slot->nb_int
foo_bar->slot->sq_length(o)
foobaz->slot->mp_subscript
baz->slot->am_await(self)
"""
        expected_lines = """
PyType_GetSlot(foo, Py_nb_int)
PyType_GetSlot(foo_bar, Py_sq_length)(o)
PyType_GetSlot(foobaz, Py_mp_subscript)
PyType_GetSlot(baz, Py_am_await)(self)
"""
        res = pcs.slot_to_macro(lines, "")
        self.assertEqual(res, expected_lines)

    def test_two_chain_slot_to_ref_type(self):
        lines = """
PyFloat_Type.tp_as_number->nb_power(v, w, x);
"""
        expected_lines = """
((ternaryfunc)PyType_GetSlot(&PyFloat_Type, Py_nb_power))(v, w, x);
"""
        res = pcs.slot_to_macro(lines, "")
        self.assertEqual(res, expected_lines)

    def test_two_chain_slot_to_macro(self):
        lines = """
Py_TYPE(foo)->slot->nb_int
Py_TYPE(bar)->slot->sq_length(o)
Py_TYPE(baz)->slot->mp_subscript
Py_TYPE(foobar)->slot->am_await(self)
"""
        expected_lines = """
PyType_GetSlot(Py_TYPE(foo), Py_nb_int)
((lenfunc)PyType_GetSlot(Py_TYPE(bar), Py_sq_length))(o)
PyType_GetSlot(Py_TYPE(baz), Py_mp_subscript)
((unaryfunc)PyType_GetSlot(Py_TYPE(foobar), Py_am_await))(self)
"""
        res = pcs.slot_to_macro(lines, "")
        self.assertEqual(res, expected_lines)

    def test_slot_to_callable(self):
        lines = """
return PyType_GetSlot(&foo, Py_nb_invert)(v);
return PyType_GetSlot(&foo, Py_nb_lshift)(v, w);
return PyType_GetSlot(&foo, Py_nb_power)(v, w, z);
"""
        expected_lines = """
return ((unaryfunc)PyType_GetSlot(&foo, Py_nb_invert))(v);
return ((binaryfunc)PyType_GetSlot(&foo, Py_nb_lshift))(v, w);
return ((ternaryfunc)PyType_GetSlot(&foo, Py_nb_power))(v, w, z);
"""
        res = pcs.slot_to_macro(lines, "")
        self.assertEqual(res, expected_lines)

    def test_slot_to_deref_callable(self):
        lines = """
return *PyType_GetSlot(&foo, Py_nb_invert)(v);
return *PyType_GetSlot(&foo, Py_nb_lshift)(v, w);
return *PyType_GetSlot(&foo, Py_nb_power)(v, w, z);
"""
        expected_lines = """
return ((unaryfunc)PyType_GetSlot(&foo, Py_nb_invert))(v);
return ((binaryfunc)PyType_GetSlot(&foo, Py_nb_lshift))(v, w);
return ((ternaryfunc)PyType_GetSlot(&foo, Py_nb_power))(v, w, z);
"""
        res = pcs.slot_to_macro(lines, "")
        self.assertEqual(res, expected_lines)


class TestTypeSlotAccess(unittest.TestCase):
    def test_slot_to_macro(self):
        lines = """
Py_TYPE(foo)->tp_new
Py_TYPE(bar)->tp_free(o)
"""
        expected_lines = """
PyType_GetSlot(Py_TYPE(foo), Py_tp_new)
((freefunc)PyType_GetSlot(Py_TYPE(bar), Py_tp_free))(o)
"""
        res = pcs.slot_to_macro(lines, "")
        self.assertEqual(res, expected_lines)

    def test_slot_to_ref_type(self):
        lines = """
PyFloat_Type.tp_alloc(v, w);
(PyTypeObject *)PyType_Type.tp_new(v, w, x);
"""
        expected_lines = """
((allocfunc)PyType_GetSlot(&PyFloat_Type, Py_tp_alloc))(v, w);
(PyTypeObject *)((newfunc)PyType_GetSlot(&PyType_Type, Py_tp_new))(v, w, x);
"""
        res = pcs.slot_to_macro(lines, "")
        self.assertEqual(res, expected_lines)

    def test_slot_to_deref_callable(self):
        lines = """
return *PyType_GetSlot(&foo, Py_tp_free)(v);
"""
        expected_lines = """
return ((freefunc)PyType_GetSlot(&foo, Py_tp_free))(v);
"""
        res = pcs.slot_to_macro(lines, "")
        self.assertEqual(res, expected_lines)

    def test_type_to_special_case(self):
        lines = """
type->tp_name
type->tp_dict
"""
        expected_lines = """
((const char*)PyObject_GetAttrString(type, "__name__"))
PyObject_GetAttrString(type, "__dict__")
"""
        res = pcs.slot_to_macro(lines, "")
        self.assertEqual(res, expected_lines)

    def test_type_macro_to_special_case(self):
        lines = """
Py_TYPE(o)->tp_name
Py_TYPE(o)->tp_dict
"""
        expected_lines = """
((const char*)PyObject_GetAttrString(Py_TYPE(o), "__name__"))
PyObject_GetAttrString(Py_TYPE(o), "__dict__")
"""
        res = pcs.slot_to_macro(lines, "")
        self.assertEqual(res, expected_lines)


class TestCombinedAccess(unittest.TestCase):
    def test_four_chain_slot_to_macro(self):
        lines = """
res = (*v->ob_type->tp_as_sequence->sq_length)(v);
(obj)->ob_type->tp_as_number->nb_index != NULL
(o->ob_type->tp_as_sequence->sq_ass_item)
"""
        expected_lines = """
res = (((lenfunc)PyType_GetSlot(Py_TYPE(v), Py_sq_length)))(v);
PyType_GetSlot(Py_TYPE(obj), Py_nb_index) != NULL
(PyType_GetSlot(Py_TYPE(o), Py_sq_ass_item))
"""
        res = pcs.object_to_type(lines, "")
        res = pcs.slot_to_macro(res, "")
        self.assertEqual(res, expected_lines)


if __name__ == "__main__":
    unittest.main()
