#!/usr/bin/env python3
import unicodedata
import unittest


class UnicodedataTests(unittest.TestCase):
    def test_UCD_dunder_new_raises_type_error(self):
        with self.assertRaises(TypeError):
            unicodedata.UCD()

    def test_old_unidata_version(self):
        self.assertEqual(unicodedata.ucd_3_2_0.unidata_version, "3.2.0")

    def test_ucd_3_2_0_isinstance_of_UCD(self):
        self.assertIsInstance(unicodedata.ucd_3_2_0, unicodedata.UCD)


if __name__ == "__main__":
    unittest.main()
