#!/usr/bin/env python3
import re
import unittest


class MatchTest(unittest.TestCase):
    def test_match_matches_ascii(self):
        a = re.match("abc", "abc")
        self.assertEqual(a.group(0), "abc")

    def test_match_matches_question_mark_and_match(self):
        a = re.match("a?", "aa")
        self.assertEqual(a.group(0), "a")

    def test_match_matches_question_mark_and_no_match(self):
        a = re.match("a?", "")
        self.assertEqual(a.group(0), "")

    def test_match_matches_star_mark_and_match(self):
        a = re.match("a*", "aa")
        self.assertEqual(a.group(0), "aa")

    def test_match_matches_star_mark_and_no_match(self):
        a = re.match("a*", "")
        self.assertEqual(a.group(0), "")

    def test_match_matches_plus_mark_and_match(self):
        a = re.match("a+", "aa")
        self.assertEqual(a.group(0), "aa")

    def test_match_matches_plus_mark_and_no_match(self):
        self.assertEqual(re.match("a+", ""), None)

    def test_match_only_matches_beginning_of_input(self):
        self.assertEqual(re.match("abc", "dabc"), None)


if __name__ == "__main__":
    unittest.main()
