#!/usr/bin/env python3
import sys
import unittest


class QuickStringIO:
    def __init__(self):
        self.val = ""

    def write(self, text):
        self.val += text

    def getvalue(self):
        return self.val


class DisplayhookTest(unittest.TestCase):
    def test_displayhook_with_none_does_not_set_underscore(self):
        import builtins

        if hasattr(builtins, "_"):
            del builtins._

        orig_out = sys.stdout
        # TODO(T46541598): Test output with real StringIO
        out = QuickStringIO()
        sys.stdout = out
        sys.displayhook(None)
        self.assertEqual(out.getvalue(), "")
        self.assertTrue(not hasattr(builtins, "_"))
        sys.stdout = orig_out

    def test_displayhook_with_int_sets_underscore(self):
        import builtins

        orig_out = sys.stdout
        # TODO(T46541598): Test output with real StringIO
        out = QuickStringIO()
        sys.stdout = out
        sys.displayhook(42)
        self.assertEqual(out.getvalue(), "42\n")
        self.assertEqual(builtins._, 42)
        sys.stdout = orig_out


if __name__ == "__main__":
    unittest.main()
