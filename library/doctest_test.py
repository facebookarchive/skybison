#!/usr/bin/env python3
"""
The next line is meant for DoctestTest.test_testmod:
>>> data.append(foo + 1)
"""
import doctest
import unittest


class DoctestTest(unittest.TestCase):
    def test_testmod(self):
        data = []
        doctest.testmod(globs={"data": data, "foo": 41})
        self.assertEqual(data, [42])


if __name__ == "__main__":
    unittest.main()
