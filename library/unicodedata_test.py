#!/usr/bin/env python3
import unicodedata
import unittest


class UnicodedataTests(unittest.TestCase):
    def test_UCD_dunder_new_raises_type_error(self):
        with self.assertRaises(TypeError):
            unicodedata.UCD()

    def test_UCD_normalize_with_non_UCD_raises_type_error(self):
        with self.assertRaises(TypeError):
            unicodedata.UCD.normalize(1, "NFC", "foo")

    def test_UCD_normalize_with_non_str_form_raises_type_error(self):
        with self.assertRaises(TypeError):
            unicodedata.ucd_3_2_0.normalize(2, "foo")

    def test_UCD_normalize_with_non_str_src_raises_type_error(self):
        with self.assertRaises(TypeError):
            unicodedata.ucd_3_2_0.normalize("foo", 2)

    def test_UCD_normalize_with_empty_str_ignores_form(self):
        self.assertEqual(unicodedata.ucd_3_2_0.normalize("invalid", ""), "")

    def test_UCD_normalize_with_invalid_form_raises_value_error(self):
        with self.assertRaises(ValueError):
            unicodedata.ucd_3_2_0.normalize("invalid", "foo")

    def test_UCD_normalize_uses_old_version(self):
        self.assertEqual(
            unicodedata.ucd_3_2_0.normalize(
                "NFD", u"\U0002F868 \U0002F874 \U0002F91F \U0002F95F \U0002F9bF"
            ),
            u"\U0002136A \u5F33 \u43AB \u7AAE \u4D57",
        )

    def test_old_unidata_version(self):
        self.assertEqual(unicodedata.ucd_3_2_0.unidata_version, "3.2.0")

    def test_ucd_3_2_0_isinstance_of_UCD(self):
        self.assertIsInstance(unicodedata.ucd_3_2_0, unicodedata.UCD)


if __name__ == "__main__":
    unittest.main()
