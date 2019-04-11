#!/usr/bin/env python3
import time
import unittest


class TimeTests(unittest.TestCase):
    def test_timeReturnsFloat(self):
        self.assertIsInstance(time.time(), float)


if __name__ == "__main__":
    unittest.main()
