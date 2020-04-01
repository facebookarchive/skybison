#!/usr/bin/env python3
import grp
import time
import unittest

from test_support import pyro_only


try:
    from builtins import _structseq_new_type
except ImportError:
    pass


class StructSequenceTests(unittest.TestCase):
    def test_new_returns_instance(self):
        sseq_type = grp.struct_group
        instance = sseq_type(("foo", "bar", 42, "baz"))
        self.assertEqual(instance[0], "foo")
        self.assertEqual(instance[1], "bar")
        self.assertEqual(instance[2], 42)
        self.assertEqual(instance[3], "baz")
        self.assertEqual(instance.gr_name, "foo")
        self.assertEqual(instance.gr_passwd, "bar")
        self.assertEqual(instance.gr_gid, 42)
        self.assertEqual(instance.gr_mem, "baz")
        with self.assertRaises(IndexError):
            instance[4]

    def test_new_with_iteratble_returns_instance(self):
        sseq_type = grp.struct_group
        instance = sseq_type(range(20, 28, 2))
        self.assertEqual(instance, (20, 22, 24, 26))
        self.assertEqual(instance[2], 24)
        self.assertEqual(instance.gr_gid, 24)

    def test_new_with_min_len_returns_value(self):
        sseq_type = time.struct_time
        instance = sseq_type((9, 8, 7, 6, 5, 4, 3, 2, 1))
        self.assertEqual(instance, (9, 8, 7, 6, 5, 4, 3, 2, 1))
        self.assertIsNone(instance.tm_zone)
        self.assertIsNone(instance.tm_gmtoff)

    def test_new_with_more_than_min_len_returns_value(self):
        sseq_type = time.struct_time
        instance = sseq_type((10, 9, 8, 7, 6, 5, 4, 3, 2, 1))
        self.assertEqual(instance, (10, 9, 8, 7, 6, 5, 4, 3, 2))
        self.assertEqual(instance.tm_zone, 1)
        self.assertIsNone(instance.tm_gmtoff)

    def test_new_with_max_len_returns_value(self):
        sseq_type = time.struct_time
        instance = sseq_type((11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1))
        self.assertEqual(instance, (11, 10, 9, 8, 7, 6, 5, 4, 3))
        self.assertEqual(instance.tm_zone, 2)
        self.assertEqual(instance.tm_gmtoff, 1)

    def test_new_with_dict_returns_value(self):
        sseq_type = time.struct_time
        instance = sseq_type((1, 2, 3, 4, 5, 6, 7, 8, 9), {"tm_gmtoff": "foo"})
        self.assertEqual(instance, (1, 2, 3, 4, 5, 6, 7, 8, 9))
        self.assertIsNone(instance.tm_zone)
        self.assertEqual(instance.tm_gmtoff, "foo")

    def test_new_with_dict_with_extra_keys_returns_value(self):
        sseq_type = time.struct_time
        instance = sseq_type(
            (1, 2, 3, 4, 5, 6, 7, 8, 9, 10),
            {"tm_zone": "foo", "tm_gmtoff": "bar", "baz": 42, "tm_year": "ignored"},
        )
        self.assertEqual(instance, (1, 2, 3, 4, 5, 6, 7, 8, 9))
        self.assertEqual(instance.tm_zone, 10)
        self.assertEqual(instance.tm_gmtoff, "bar")

    def test_new_with_non_sequence_raises_type_error(self):
        sseq_type = grp.struct_group
        with self.assertRaises(TypeError):
            sseq_type(42)

    def test_new_with_too_few_elements_raises_type_error(self):
        sseq_type = grp.struct_group
        with self.assertRaisesRegex(TypeError, ".*4-sequence.*(2-sequence given)"):
            sseq_type((1, 2))

    def test_new_with_too_many_elements_raises_type_error(self):
        sseq_type = grp.struct_group
        with self.assertRaisesRegex(TypeError, ".*4-sequence.*(5-sequence given)"):
            sseq_type((1, 2, 3, 4, 5))

    def test_descriptor_on_other_instance_raises_type_error(self):
        sseq_type = grp.struct_group
        descriptor = sseq_type.gr_name
        with self.assertRaisesRegex(
            TypeError, "descriptor.*doesn't apply to 'tuple' object"
        ):
            descriptor.__get__((1, 2, 3, 4), tuple)

    @pyro_only
    def test_structseq_new_type_returns_type(self):
        tp = _structseq_new_type("foo.bar", ("f0", "f1", "f2"), 2)
        self.assertIsInstance(tp, type)
        self.assertTrue(issubclass(tp, tuple))
        self.assertEqual(tp.__module__, "foo")
        self.assertEqual(tp.__name__, "bar")
        self.assertEqual(tp.__qualname__, "bar")
        self.assertEqual(tp.n_sequence_fields, 2)
        self.assertEqual(tp.n_unnamed_fields, 0)
        self.assertEqual(tp.n_fields, 3)
        self.assertEqual(tp._structseq_field_names, ("f0", "f1", "f2"))

    @pyro_only
    def test_structseq_new_type_without_dot_in_name_does_not_set_module(self):
        tp = _structseq_new_type("foo", (), 0)
        self.assertNotIn("__module__", tp.__dict__)

    @pyro_only
    def test_structseq_new_type_with_unnamed_fields_returns_structseq_type(self):
        tp = _structseq_new_type("quux", ("foo", None, "bar", None, "baz"), 4)
        self.assertIsInstance(tp, type)
        self.assertTrue(issubclass(tp, tuple))
        self.assertEqual(tp.__name__, "quux")
        self.assertEqual(tp.n_sequence_fields, 4)
        self.assertEqual(tp.n_unnamed_fields, 2)
        self.assertEqual(tp.n_fields, 5)
        self.assertEqual(tp._structseq_field_names, ("foo", None, "bar", None, "baz"))
        instance = tp((1, 2, 3, 4, 5))
        self.assertEqual(instance, (1, 2, 3, 4))
        self.assertEqual(instance.bar, 3)
        self.assertEqual(instance.baz, 5)


if __name__ == "__main__":
    unittest.main()
