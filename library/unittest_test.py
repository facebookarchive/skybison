#!/usr/bin/env python3
import unittest


class BasicTests(unittest.TestCase):
    def test_assert_negative_true(self):
        self.assertTrue(1 == 1)

    def test_assert_false(self):
        self.assertFalse(1 == 2)

    def test_assert_equal(self):
        self.assertEqual(1, 1)

    def test_assert_not_equal(self):
        self.assertNotEqual("foo", "bar")

    def test_assert_raises(self):
        def raises():
            raise Exception("Testing 123")

        self.assertRaises(Exception, raises)

    def test_assert_compare(self):
        self.assertLess(1, 2)
        self.assertLessEqual(2, 2)
        self.assertGreater(3, 2)
        self.assertGreaterEqual(3, 3)

    def test_assert_is(self):
        self.assertIs(type([]), list)

    def test_assert_is_not(self):
        self.assertIsNot(type([]), tuple)

    def test_assert_is_instance(self):
        self.assertIsInstance([1, 2, 3], list)

    def test_assert_not_is_instance(self):
        self.assertNotIsInstance([1, 2, 3], str)


class ContainerTests(unittest.TestCase):
    def test_assert_sequence_equal(self):
        self.assertSequenceEqual([1, 2, 3], (1, 2, 3))

    def test_assert_list_equal(self):
        self.assertListEqual([1, 2, 3], [1, 2, 3])

    def test_assert_tuple_equal(self):
        self.assertTupleEqual(("a", "b"), ("a", "b"))

    def test_assert_in_equal(self):
        self.assertIn(1, [3, 2, 1, 2, 3])

    def test_assert_not_in_equal(self):
        self.assertNotIn(0, [3, 2, 1, 2, 3])

    def test_assert_dict_equal(self):
        self.assertDictEqual({"a": 2, 5: "foo"}, {"a": 2, 5: "foo"})

    def test_assert_multi_line_equal(self):
        self.assertMultiLineEqual(
            """ This is a big multiline
                                  to test if it works""",
            """ This is a big multiline
                                  to test if it works""",
        )


if __name__ == "__main__":
    unittest.main()
