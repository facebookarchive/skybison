#!/usr/bin/env python3
import unittest


class StrTests(unittest.TestCase):
    def test_splitlinesWithNonStrRaisesTypeError(self):
        with self.assertRaises(TypeError):
            str.splitlines(None)

    def test_splitlinesWithFloatKeependsRaisesTypeError(self):
        with self.assertRaises(TypeError):
            str.splitlines("hello", 0.4)

    def test_splitlinesWithNonIntCallsDunderIntNonBool(self):
        class C:
            def __int__(self):
                return 100

        self.assertEqual(str.splitlines("\n", C()), ["\n"])

    def test_splitlinesWithNonIntCallsDunderIntTrue(self):
        class C:
            def __int__(self):
                return 1

        self.assertEqual(str.splitlines("\n", C()), ["\n"])

    def test_splitlinesWithNonIntCallsDunderIntFalse(self):
        class C:
            def __int__(self):
                return 0

        self.assertEqual(str.splitlines("\n", C()), [""])


if __name__ == "__main__":
    unittest.main()
