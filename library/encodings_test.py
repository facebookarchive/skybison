#!/usr/bin/env python3
import codecs
import encodings  # noqa: F401
import unittest


class EncodingsModuleTest(unittest.TestCase):
    def test_utf_8_incremental_decoder_returns_str(self):
        inc = codecs.getincrementaldecoder("utf-8")()
        self.assertEqual(inc.decode(b"test"), "test")


if __name__ == "__main__":
    unittest.main()
