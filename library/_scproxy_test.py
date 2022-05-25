#!/usr/bin/env python3
import sys
import unittest


@unittest.skipIf(sys.platform != "darwin", "_scproxy is only valid on Darwin")
class UnderScproxyTest(unittest.TestCase):
    def test_it_imports(self):
        import _scproxy

        self.assertEqual(_scproxy.__name__, "_scproxy")


if __name__ == "__main__":
    unittest.main()
