#!/usr/bin/env python3
import unittest


class BasicTests(unittest.TestCase):
    def test_assertTrue(self):
        self.assertTrue(1 == 1)

    def test_assertFalse(self):
        self.assertFalse(1 == 2)

    def test_assertEqual(self):
        self.assertEqual(1, 1)

    def test_assertNotEqual(self):
        self.assertNotEqual("foo", "bar")

    def test_assertRaises(self):
        def raises():
            raise Exception("Testing 123")

        self.assertRaises(Exception, raises)

    def test_assertCompare(self):
        self.assertLess(1, 2)
        self.assertLessEqual(2, 2)
        self.assertGreater(3, 2)
        self.assertGreaterEqual(3, 3)

    def test_assertIs(self):
        self.assertIs(type([]), list)

    def test_assertIsNot(self):
        self.assertIsNot(type([]), tuple)

    def test_assertIsInstance(self):
        self.assertIsInstance([1, 2, 3], list)

    def test_assertNotIsInstance(self):
        self.assertNotIsInstance([1, 2, 3], str)


class ContainerTests(unittest.TestCase):
    def test_assertSequenceEqual(self):
        self.assertSequenceEqual([1, 2, 3], (1, 2, 3))

    def test_assertListEqual(self):
        self.assertListEqual([1, 2, 3], [1, 2, 3])

    def test_assertTupleEqual(self):
        self.assertTupleEqual(("a", "b"), ("a", "b"))

    def test_assertInEqual(self):
        self.assertIn(1, [3, 2, 1, 2, 3])

    def test_assertNotInEqual(self):
        self.assertNotIn(0, [3, 2, 1, 2, 3])

    def test_assertDictEqual(self):
        self.assertDictEqual({"a": 2, 5: "foo"}, {"a": 2, 5: "foo"})

    def test_assertMultiLineEqual(self):
        self.assertMultiLineEqual(
            """ This is a big multiline
                                  to test if it works""",
            """ This is a big multiline
                                  to test if it works""",
        )


if __name__ == "__main__":
    unittest.main()
