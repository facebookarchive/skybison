#!/usr/bin/env python3

import hashlib
import unittest


class DigestTest(unittest.TestCase):
    DATA = b"The quick brown fox jumps over the lazy dog."

    def test_blake2b_hexdigest(self):
        self.assertEqual(
            hashlib.blake2b(self.DATA).hexdigest(),
            "87af9dc4afe5651b7aa89124b905fd214bf17c79af58610db86a0fb1e0194622a"
            "4e9d8e395b352223a8183b0d421c0994b98286cbf8c68a495902e0fe6e2bda2",
        )

    def test_blake2s_hexdigest(self):
        self.assertEqual(
            hashlib.blake2s(self.DATA).hexdigest(),
            "95bca6e1b761dca1323505cc629949a0e03edf11633cc7935bd8b56f393afcf2",
        )

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

    def test_sha3_224_hexdigest(self):
        self.assertEqual(
            hashlib.sha3_224(self.DATA).hexdigest(),
            "2d0708903833afabdd232a20201176e8b58c5be8a6fe74265ac54db0",
        )

    def test_sha3_256_hexdigest(self):
        self.assertEqual(
            hashlib.sha3_256(self.DATA).hexdigest(),
            "a80f839cd4f83f6c3dafc87feae470045e4eb0d366397d5c6ce34ba1739f734d",
        )

    def test_sha3_384_hexdigest(self):
        self.assertEqual(
            hashlib.sha3_384(self.DATA).hexdigest(),
            "1a34d81695b622df178bc74df7124fe12fac0f64ba5250b78b99c1273d4b08016"
            "8e10652894ecad5f1f4d5b965437fb9",
        )

    def test_sha3_512_hexdigest(self):
        self.assertEqual(
            hashlib.sha3_512(self.DATA).hexdigest(),
            "18f4f4bd419603f95538837003d9d254c26c23765565162247483f65c50303597"
            "bc9ce4d289f21d1c2f1f458828e33dc442100331b35e7eb031b5d38ba6460f8",
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
