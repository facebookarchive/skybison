#!/usr/bin/env python3
# WARNING: This is a temporary copy of code from the cpython library to
# facilitate bringup. Please file a task for anything you change!
# flake8: noqa
# fmt: off

import hmac
import unittest


class CompareDigestTestCase(unittest.TestCase):

    def test_compare_digest(self):
        # Testing input type exception handling
        a, b = 100, 200
        self.assertRaises(TypeError, hmac.compare_digest, a, b)
        a, b = 100, b"foobar"
        self.assertRaises(TypeError, hmac.compare_digest, a, b)
        a, b = b"foobar", 200
        self.assertRaises(TypeError, hmac.compare_digest, a, b)
        a, b = "foobar", b"foobar"
        self.assertRaises(TypeError, hmac.compare_digest, a, b)
        a, b = b"foobar", "foobar"
        self.assertRaises(TypeError, hmac.compare_digest, a, b)

        # Testing bytes of different lengths
        a, b = b"foobar", b"foo"
        self.assertFalse(hmac.compare_digest(a, b))
        a, b = b"\xde\xad\xbe\xef", b"\xde\xad"
        self.assertFalse(hmac.compare_digest(a, b))

        # Testing bytes of same lengths, different values
        a, b = b"foobar", b"foobaz"
        self.assertFalse(hmac.compare_digest(a, b))
        a, b = b"\xde\xad\xbe\xef", b"\xab\xad\x1d\xea"
        self.assertFalse(hmac.compare_digest(a, b))

        # Testing bytes of same lengths, same values
        a, b = b"foobar", b"foobar"
        self.assertTrue(hmac.compare_digest(a, b))
        a, b = b"\xde\xad\xbe\xef", b"\xde\xad\xbe\xef"
        self.assertTrue(hmac.compare_digest(a, b))

        # Testing bytearrays of same lengths, same values
        a, b = bytearray(b"foobar"), bytearray(b"foobar")
        self.assertTrue(hmac.compare_digest(a, b))

        # Testing bytearrays of different lengths
        a, b = bytearray(b"foobar"), bytearray(b"foo")
        self.assertFalse(hmac.compare_digest(a, b))

        # Testing bytearrays of same lengths, different values
        a, b = bytearray(b"foobar"), bytearray(b"foobaz")
        self.assertFalse(hmac.compare_digest(a, b))

        # Testing byte and bytearray of same lengths, same values
        a, b = bytearray(b"foobar"), b"foobar"
        self.assertTrue(hmac.compare_digest(a, b))
        self.assertTrue(hmac.compare_digest(b, a))

        # Testing byte bytearray of different lengths
        a, b = bytearray(b"foobar"), b"foo"
        self.assertFalse(hmac.compare_digest(a, b))
        self.assertFalse(hmac.compare_digest(b, a))

        # Testing byte and bytearray of same lengths, different values
        a, b = bytearray(b"foobar"), b"foobaz"
        self.assertFalse(hmac.compare_digest(a, b))
        self.assertFalse(hmac.compare_digest(b, a))

        # Testing str of same lengths
        a, b = "foobar", "foobar"
        self.assertTrue(hmac.compare_digest(a, b))

        # Testing str of different lengths
        a, b = "foo", "foobar"
        self.assertFalse(hmac.compare_digest(a, b))

        # Testing bytes of same lengths, different values
        a, b = "foobar", "foobaz"
        self.assertFalse(hmac.compare_digest(a, b))

        # Testing error cases
        a, b = "foobar", b"foobar"
        self.assertRaises(TypeError, hmac.compare_digest, a, b)
        a, b = b"foobar", 1
        self.assertRaises(TypeError, hmac.compare_digest, a, b)
        a, b = 100, 200
        self.assertRaises(TypeError, hmac.compare_digest, a, b)

        # subclasses are supported by ignore __eq__
        class mystr(str):
            def __eq__(self, other):
                return False

        a, b = mystr("foobar"), mystr("foobar")
        self.assertTrue(hmac.compare_digest(a, b))
        a, b = mystr("foobar"), "foobar"
        self.assertTrue(hmac.compare_digest(a, b))
        a, b = mystr("foobar"), mystr("foobaz")
        self.assertFalse(hmac.compare_digest(a, b))

        class mybytes(bytes):
            def __eq__(self, other):
                return False

        a, b = mybytes(b"foobar"), mybytes(b"foobar")
        self.assertTrue(hmac.compare_digest(a, b))
        a, b = mybytes(b"foobar"), b"foobar"
        self.assertTrue(hmac.compare_digest(a, b))
        a, b = mybytes(b"foobar"), mybytes(b"foobaz")
        self.assertFalse(hmac.compare_digest(a, b))


if __name__ == "__main__":
    unittest.main()
