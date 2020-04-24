import types
import unittest


class NewClassTests(unittest.TestCase):
    def test_new_class_with_name_sets_name(self):
        result = types.new_class("foo")
        self.assertIsInstance(result, type)
        self.assertEqual(result.__name__, "foo")

    def test_new_class_with_non_str_name_raises_type_error(self):
        with self.assertRaises(TypeError):
            types.new_class(1, (object,), {})

    def test_new_class_with_non_tuple_bases_raises_type_error(self):
        with self.assertRaises(TypeError):
            types.new_class("X", [object], {})

    def test_new_class_with_non_dict_type_dict_raises_type_error(self):
        with self.assertRaises(TypeError):
            types.new_class("X", (object,), 1)

    def test_new_class_returns_type_instance(self):
        result = types.new_class("X", (object,), {})
        self.assertIsInstance(result, type)
        self.assertEqual(result.__bases__, (object,))


class ResolveBasesTests(unittest.TestCase):
    def test_resolve_bases_with_empty_returns_input(self):
        bases = []
        self.assertIs(types.resolve_bases(bases), bases)

    def test_resolve_bases_with_iterable_of_types_returns_input(self):
        class C:
            pass

        bases = [C, tuple, object]
        self.assertIs(types.resolve_bases(bases), bases)

    def test_resolve_bases_skips_items_without_dunder_mro_entries(self):
        class C:
            pass

        bases = [tuple, C(), object]
        result = types.resolve_bases(bases)
        self.assertIs(result, bases)

    def test_resolve_bases_with_non_tuple_dunder_mro_entries_raises_type_error(self):
        class C:
            def __mro_entries__(self, bases):
                return [1, 2, 3]

        bases = [tuple, C(), object]
        with self.assertRaises(TypeError) as context:
            types.resolve_bases(bases)
        self.assertEqual(str(context.exception), "__mro_entries__ must return a tuple")

    def test_resolve_bases_inserts_result_of_calling_dunder_mro_entries(self):
        class C:
            def __mro_entries__(self, bases):
                return (1, 2, 3)

        result = types.resolve_bases([tuple, C(), object])
        self.assertIs(type(result), tuple)
        self.assertEqual(result, (tuple, 1, 2, 3, object))


if __name__ == "__main__":
    unittest.main()
