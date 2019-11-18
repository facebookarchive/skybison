#!/usr/bin/env python3
import dis
import io
import unittest

from test_support import pyro_only


try:
    import _compile
except ImportError:
    pass


def dis_str(code):
    with io.StringIO() as out:
        dis.dis(code, file=out)
        return out.getvalue()


@pyro_only
class PrintfTransformTests(unittest.TestCase):
    def test_simple(self):
        code = _compile.compile("'foo' % ()", "", "eval")
        self.assertEqual(
            dis_str(code),
            """\
  1           0 LOAD_CONST               0 ('foo')
              2 RETURN_VALUE
""",
        )
        self.assertEqual(eval(code), str.__mod__("foo", ()))  # noqa: P204

    def test_percent_a(self):
        code = _compile.compile("'%a' % (42,)", "", "eval")
        self.assertEqual(
            dis_str(code),
            """\
  1           0 LOAD_CONST               0 (42)
              2 FORMAT_VALUE             3 (ascii)
              4 RETURN_VALUE
""",
        )
        self.assertEqual(eval(code), str.__mod__("%a", (42,)))  # noqa: P204

    def test_percent_percent(self):
        code = _compile.compile("'%%' % ()", "", "eval")
        self.assertEqual(
            dis_str(code),
            """\
  1           0 LOAD_CONST               0 ('%')
              2 RETURN_VALUE
""",
        )
        self.assertEqual(eval(code), str.__mod__("%%", ()))  # noqa: P204

    def test_percent_r(self):
        code = _compile.compile("'%r' % ('bar',)", "", "eval")
        self.assertEqual(
            dis_str(code),
            """\
  1           0 LOAD_CONST               0 ('bar')
              2 FORMAT_VALUE             2 (repr)
              4 RETURN_VALUE
""",
        )
        self.assertEqual(eval(code), str.__mod__("%r", ("bar",)))  # noqa: P204

    def test_percent_s(self):
        code = _compile.compile("'%s' % (42,)", "", "eval")
        self.assertEqual(
            dis_str(code),
            """\
  1           0 LOAD_CONST               0 (42)
              2 FORMAT_VALUE             1 (str)
              4 RETURN_VALUE
""",
        )
        self.assertEqual(eval(code), str.__mod__("%s", (42,)))  # noqa: P204

    def test_percent_d_i_u(self):
        expected = """\
  1           0 LOAD_CONST               0 ('')
              2 LOAD_ATTR                0 (_mod_convert_number)
              4 LOAD_CONST               2 (-13)
              6 CALL_FUNCTION            1
              8 FORMAT_VALUE             0
             10 RETURN_VALUE
"""
        code0 = _compile.compile("'%d' % (-13,)", "", "eval")
        code1 = _compile.compile("'%i' % (-13,)", "", "eval")
        code2 = _compile.compile("'%u' % (-13,)", "", "eval")
        self.assertEqual(dis_str(code0), expected)
        self.assertEqual(dis_str(code1), expected)
        self.assertEqual(dis_str(code2), expected)
        self.assertEqual(eval(code0), str.__mod__("%d", (-13,)))  # noqa: P204
        self.assertEqual(eval(code1), str.__mod__("%i", (-13,)))  # noqa: P204
        self.assertEqual(eval(code2), str.__mod__("%u", (-13,)))  # noqa: P204

    def test_percent_d_calls_dunder_int(self):
        class C:
            def __int__(self):
                return 7

        code = _compile.compile("'%d' % (x,)", "", "eval")
        self.assertNotIn("BINARY_MOD", dis_str(code))
        self.assertEqual(eval(code, None, {"x": C()}), "7")  # noqa: P204

    def test_percent_d_raises_type_error(self):
        class C:
            pass

        code = _compile.compile("'%d' % (x,)", "", "eval")
        self.assertNotIn("BINARY_MOD", dis_str(code))
        with self.assertRaisesRegex(TypeError, "format requires a number, not C"):
            eval(code, None, {"x": C()})  # noqa: P204

    def test_percent_s_with_width(self):
        code = _compile.compile("'%13s' % (42,)", "", "eval")
        self.assertEqual(
            dis_str(code),
            """\
  1           0 LOAD_CONST               0 (42)
              2 LOAD_CONST               1 ('>13')
              4 FORMAT_VALUE             5 (str, with format)
              6 RETURN_VALUE
""",
        )
        self.assertEqual(eval(code), str.__mod__("%13s", (42,)))  # noqa: P204

    def test_percent_d_with_width_and_flags(self):
        code = _compile.compile("'%05d' % (-5,)", "", "eval")
        self.assertEqual(
            dis_str(code),
            """\
  1           0 LOAD_CONST               0 ('')
              2 LOAD_ATTR                0 (_mod_convert_number)
              4 LOAD_CONST               3 (-5)
              6 CALL_FUNCTION            1
              8 LOAD_CONST               2 ('05')
             10 FORMAT_VALUE             4 (with format)
             12 RETURN_VALUE
""",
        )
        self.assertEqual(eval(code), str.__mod__("%05d", (-5,)))  # noqa: P204

    def test_mixed(self):
        code = _compile.compile("'%s %% foo %r bar %a %s' % (1,2,3,4)", "", "eval")
        self.assertEqual(
            dis_str(code),
            """\
  1           0 LOAD_CONST               0 (1)
              2 FORMAT_VALUE             1 (str)
              4 LOAD_CONST               1 (' ')
              6 LOAD_CONST               2 ('% foo ')
              8 LOAD_CONST               3 (2)
             10 FORMAT_VALUE             2 (repr)
             12 LOAD_CONST               4 (' bar ')
             14 LOAD_CONST               5 (3)
             16 FORMAT_VALUE             3 (ascii)
             18 LOAD_CONST               1 (' ')
             20 LOAD_CONST               6 (4)
             22 FORMAT_VALUE             1 (str)
             24 BUILD_STRING             8
             26 RETURN_VALUE
""",
        )
        self.assertEqual(
            eval(code),  # noqa: P204
            str.__mod__("%s %% foo %r bar %a %s", (1, 2, 3, 4)),
        )

    def test_without_tuple(self):
        code = _compile.compile("'%s' % 5.5", "", "eval")
        self.assertEqual(
            dis_str(code),
            """\
  1           0 LOAD_CONST               0 ('')
              2 LOAD_ATTR                0 (_mod_check_single_arg)
              4 LOAD_CONST               1 (5.5)
              6 CALL_FUNCTION            1
              8 LOAD_CONST               2 (0)
             10 BINARY_SUBSCR
             12 FORMAT_VALUE             1 (str)
             14 RETURN_VALUE
""",
        )
        self.assertEqual(eval(code), str.__mod__("%s", 5.5))  # noqa: P204

    def test_without_tuple_formats_value(self):
        code = _compile.compile("'%s' % x", "", "eval")
        self.assertNotIn("BINARY_MODULO", dis_str(code))
        self.assertEqual(
            eval(code, None, {"x": -8}), str.__mod__("%s", -8)  # noqa: P204
        )
        self.assertEqual(
            eval(code, None, {"x": (True,)}), str.__mod__("%s", (True,))  # noqa: P204
        )

    def test_without_tuple_raises_type_error(self):
        code = _compile.compile("'%s' % x", "", "eval")
        self.assertNotIn("BINARY_MODULO", dis_str(code))
        with self.assertRaisesRegex(
            TypeError, "not enough arguments for format string"
        ):
            eval(code, None, {"x": ()})  # noqa: P204
        with self.assertRaisesRegex(
            TypeError, "not all arguments converted during string formatting"
        ):
            eval(code, None, {"x": (1, 2)})  # noqa: P204

    def test_no_trailing_percent(self):
        code = _compile.compile("'foo%' % ()", "", "eval")
        self.assertIn("BINARY_MODULO", dis_str(code))

    def test_no_no_tuple(self):
        code = _compile.compile("'%s%s' % x", "", "eval")
        self.assertIn("BINARY_MODULO", dis_str(code))

    def test_no_mapping_key(self):
        code = _compile.compile("'%(foo)' % x", "", "eval")
        self.assertIn("BINARY_MODULO", dis_str(code))

        code = _compile.compile("'%(%s)' % x", "", "eval")
        self.assertIn("BINARY_MODULO", dis_str(code))

    def test_no_tuple_too_small(self):
        code = _compile.compile("'%s%s%s' % (1,2)", "", "eval")
        self.assertIn("BINARY_MODULO", dis_str(code))

    def test_no_tuple_too_big(self):
        code = _compile.compile("'%s%s%s' % (1,2,3,4)", "", "eval")
        self.assertIn("BINARY_MODULO", dis_str(code))

    def test_no_unknown_specifier(self):
        code = _compile.compile("'%Z' % (None,)", "", "eval")
        self.assertIn("BINARY_MODULO", dis_str(code))


if __name__ == "__main__":
    unittest.main()
