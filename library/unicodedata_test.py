#!/usr/bin/env python3
import unicodedata
import unittest


class UnicodedataTests(unittest.TestCase):
    def test_UCD_dunder_new_raises_type_error(self):
        with self.assertRaises(TypeError):
            unicodedata.UCD()

    def test_UCD_bidirectional_uses_old_version(self):
        self.assertEqual(unicodedata.ucd_3_2_0.bidirectional(" "), "WS")
        self.assertEqual(unicodedata.ucd_3_2_0.bidirectional("+"), "ET")
        self.assertEqual(unicodedata.ucd_3_2_0.bidirectional("A"), "L")
        self.assertEqual(unicodedata.ucd_3_2_0.bidirectional("\uFFFE"), "")
        self.assertEqual(unicodedata.ucd_3_2_0.bidirectional("\U00020000"), "L")

    def test_UCD_category_uses_old_version(self):
        self.assertEqual(unicodedata.ucd_3_2_0.category("A"), "Lu")
        self.assertEqual(unicodedata.ucd_3_2_0.category("a"), "Ll")
        self.assertEqual(unicodedata.ucd_3_2_0.category("\u00A7"), "So")
        self.assertEqual(unicodedata.ucd_3_2_0.category("\uFFFE"), "Cn")
        self.assertEqual(unicodedata.ucd_3_2_0.category("\U0001012A"), "Cn")
        self.assertEqual(unicodedata.ucd_3_2_0.category("\U00020000"), "Lo")

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

    def test_bidirectional_uses_current_version(self):
        self.assertEqual(unicodedata.bidirectional(" "), "WS")
        self.assertEqual(unicodedata.bidirectional("+"), "ES")
        self.assertEqual(unicodedata.bidirectional("A"), "L")
        self.assertEqual(unicodedata.bidirectional("\uFFFE"), "")
        self.assertEqual(unicodedata.bidirectional("\U00020000"), "L")

    def test_category_uses_current_version(self):
        self.assertEqual(unicodedata.category("A"), "Lu")
        self.assertEqual(unicodedata.category("a"), "Ll")
        self.assertEqual(unicodedata.category("\u00A7"), "Po")
        self.assertEqual(unicodedata.category("\uFFFE"), "Cn")
        self.assertEqual(unicodedata.category("\U0001012A"), "No")
        self.assertEqual(unicodedata.category("\U00020000"), "Lo")

    def test_old_unidata_version(self):
        self.assertEqual(unicodedata.ucd_3_2_0.unidata_version, "3.2.0")

    def test_ucd_3_2_0_isinstance_of_UCD(self):
        self.assertIsInstance(unicodedata.ucd_3_2_0, unicodedata.UCD)


if __name__ == "__main__":
    unittest.main()
