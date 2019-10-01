#!/usr/bin/env python3

import hashlib
import unittest


class DigestTest(unittest.TestCase):
    DATA = b"The quick brown fox jumps over the lazy dog."

    def test_md5_digest(self):
        self.assertEqual(
            hashlib.md5(self.DATA).digest(),
            b'\xe4\xd9\t\xc2\x90\xd0\xfb\x1c\xa0h\xff\xad\xdf"\xcb\xd0',
        )

    def test_md5_hexdigest(self):
        self.assertEqual(
            hashlib.md5(self.DATA).hexdigest(), "e4d909c290d0fb1ca068ffaddf22cbd0"
        )

    def test_sha1_hexdigest(self):
        self.assertEqual(
            hashlib.sha1(self.DATA).hexdigest(),
            "408d94384216f890ff7a0c3528e8bed1e0b01621",
        )

    def test_sha256_hexdigest(self):
        self.assertEqual(
            hashlib.sha256(self.DATA).hexdigest(),
            "ef537f25c895bfa782526529a9b63d97aa631564d5d789c2b765448c8635fb6c",
        )

    def test_sha512_hexdigest(self):
        self.assertEqual(
            hashlib.sha512(self.DATA).hexdigest(),
            "91ea1245f20d46ae9a037a989f54f1f790f0a47607eeb8a14d12890cea77a1bb"
            "c6c7ed9cf205e67b7f2b8fd4c7dfd3a7a8617e45f3c463d481c7e586c39ac1ed",
        )


if __name__ == "__main__":
    unittest.main()
