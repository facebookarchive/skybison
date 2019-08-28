#!/usr/bin/env python3
import unittest


class SignalTest(unittest.TestCase):
    def test_import(self):
        import signal

        self.assertEqual(signal.__name__, "signal")


if __name__ == "__main__":
    unittest.main()
