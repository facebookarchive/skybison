#!/usr/bin/env python3
# Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com)
import time
import unittest


class TimeTests(unittest.TestCase):
    def test_time_returns_float(self):
        self.assertIsInstance(time.time(), float)


if __name__ == "__main__":
    unittest.main()
