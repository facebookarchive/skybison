#!/usr/bin/env python3
import unittest
from operator import attrgetter, itemgetter


class AttrGetterTests(unittest.TestCase):
    def test_dunder_repr_with_string_attr(self):
        self.assertEqual(repr(attrgetter("x")), "operator.attrgetter('x')")


class ItemGetterTests(unittest.TestCase):
    def test_dunder_repr_with_simple_items(self):
        self.assertEqual(
            repr(itemgetter("x", 1, [])), "operator.itemgetter('x', 1, [])"
        )

    def test_dunder_repr_with_recursion(self):
        container = []
        getter = itemgetter(container)
        container.append(getter)
        self.assertEqual(
            repr(getter), "operator.itemgetter([operator.itemgetter(...)])"
        )


if __name__ == "__main__":
    unittest.main()
