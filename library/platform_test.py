#!/usr/bin/env python3
import unittest


class PlatformTest(unittest.TestCase):
    def test_import(self):
        import platform

        self.assertEqual(platform.__name__, "platform")


if __name__ == "__main__":
    unittest.main()
