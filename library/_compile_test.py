#!/usr/bin/env python3
import dis
import io
import unittest

from test_support import pyro_only


def dis_str(code):
    with io.StringIO() as out:
        dis.dis(code, file=out)
        return out.getvalue()


@pyro_only
class PrintfTransformTests(unittest.TestCase):
    def test_simple(self):
        code = compile("'foo' % ()", "", "eval")
        self.assertEqual(
            dis_str(code),
            """\
  1           0 LOAD_CONST               0 ('foo')
              2 RETURN_VALUE
""",
        )
        self.assertEqual(eval(code), str.__mod__("foo", ()))  # noqa: P204

    def test_percent_a_constant_folded(self):
        code = compile("'%a' % (42,)", "", "eval")
        self.assertEqual(
            dis_str(code),
            """\
  1           0 LOAD_CONST               0 ('42')
              2 RETURN_VALUE
""",
        )
        self.assertEqual(eval(code), str.__mod__("%a", (42,)))  # noqa: P204

    def test_percent_a(self):
        code = compile("'%a' % (x,)", "", "eval")
        self.assertEqual(
            dis_str(code),
            """\
  1           0 LOAD_NAME                0 (x)
              2 FORMAT_VALUE             3 (ascii)
              4 RETURN_VALUE
""",
        )
        self.assertEqual(
            eval(code, locals={"x": 42}), str.__mod__("%a", (42,))  # noqa: P204
        )

    def test_percent_percent(self):
        code = compile("'%%' % ()", "", "eval")
        self.assertEqual(
            dis_str(code),
            """\
  1           0 LOAD_CONST               0 ('%')
              2 RETURN_VALUE
""",
        )
        self.assertEqual(eval(code), str.__mod__("%%", ()))  # noqa: P204

    def test_percent_r_constant_folded(self):
        code = compile("'%r' % ('bar',)", "", "eval")
        self.assertEqual(
            dis_str(code),
            """\
  1           0 LOAD_CONST               0 ("'bar'")
              2 RETURN_VALUE
""",
        )
        self.assertEqual(eval(code), str.__mod__("%r", ("bar",)))  # noqa: P204

    def test_percent_r(self):
        code = compile("'%r' % (x,)", "", "eval")
        self.assertEqual(
            dis_str(code),
            """\
  1           0 LOAD_NAME                0 (x)
              2 FORMAT_VALUE             2 (repr)
              4 RETURN_VALUE
""",
        )
        self.assertEqual(
            eval(code, locals={"x": "bar"}), str.__mod__("%r", ("bar",))  # noqa: P204
        )

    def test_percent_s_constant_folded(self):
        code = compile("'%s' % (42,)", "", "eval")
        self.assertEqual(
            dis_str(code),
            """\
  1           0 LOAD_CONST               0 ('42')
              2 RETURN_VALUE
""",
        )
        self.assertEqual(eval(code), str.__mod__("%s", (42,)))  # noqa: P204

    def test_percent_s(self):
        code = compile("'%s' % (x,)", "", "eval")
        self.assertEqual(
            dis_str(code),
            """\
  1           0 LOAD_NAME                0 (x)
              2 FORMAT_VALUE             1 (str)
              4 RETURN_VALUE
""",
        )
        self.assertEqual(
            eval(code, locals={"x": 42}), str.__mod__("%s", (42,))  # noqa: P204
        )

    def test_percent_d_i_u_constant_folded(self):
        expected = """\
  1           0 LOAD_CONST               0 ('-13')
              2 RETURN_VALUE
"""
        code0 = compile("'%d' % (-13,)", "", "eval")
        code1 = compile("'%i' % (-13,)", "", "eval")
        code2 = compile("'%u' % (-13,)", "", "eval")
        self.assertEqual(dis_str(code0), expected)
        self.assertEqual(dis_str(code1), expected)
        self.assertEqual(dis_str(code2), expected)
        self.assertEqual(eval(code0), str.__mod__("%d", (-13,)))  # noqa: P204
        self.assertEqual(eval(code1), str.__mod__("%i", (-13,)))  # noqa: P204
        self.assertEqual(eval(code2), str.__mod__("%u", (-13,)))  # noqa: P204

    @unittest.skipIf(True, "TODO(T78706522): Port printf transforms to compiler/")
    def test_percent_d_i_u(self):
        expected = """\
  1           0 LOAD_CONST               0 ('')
              2 LOAD_METHOD              0 (_mod_convert_number)
              4 LOAD_NAME                1 (x)
              6 CALL_METHOD              1
              8 FORMAT_VALUE             0
             10 RETURN_VALUE
"""
        code0 = compile("'%d' % (x,)", "", "eval")
        code1 = compile("'%i' % (x,)", "", "eval")
        code2 = compile("'%u' % (x,)", "", "eval")
        self.assertEqual(dis_str(code0), expected)
        self.assertEqual(dis_str(code1), expected)
        self.assertEqual(dis_str(code2), expected)
        self.assertEqual(
            eval(code0, locals={"x": -13}), str.__mod__("%d", (-13,))  # noqa: P204
        )
        self.assertEqual(
            eval(code1, locals={"x": -13}), str.__mod__("%i", (-13,))  # noqa: P204
        )
        self.assertEqual(
            eval(code2, locals={"x": -13}), str.__mod__("%u", (-13,))  # noqa: P204
        )

    @unittest.skipIf(True, "TODO(T78706522): Port printf transforms to compiler/")
    def test_percent_d_calls_dunder_int(self):
        class C:
            def __int__(self):
                return 7

        code = compile("'%d' % (x,)", "", "eval")
        self.assertNotIn("BINARY_MOD", dis_str(code))
        self.assertEqual(eval(code, None, {"x": C()}), "7")  # noqa: P204

    def test_percent_s_with_width_constant_folded(self):
        code = compile("'%13s' % (42,)", "", "eval")
        self.assertEqual(
            dis_str(code),
            """\
  1           0 LOAD_CONST               0 ('           42')
              2 RETURN_VALUE
""",
        )
        self.assertEqual(eval(code), str.__mod__("%13s", (42,)))  # noqa: P204

    @unittest.skipIf(True, "TODO(T78706522): Port printf transforms to compiler/")
    def test_percent_s_with_width(self):
        code = compile("'%13s' % (x,)", "", "eval")
        self.assertEqual(
            dis_str(code),
            """\
  1           0 LOAD_NAME                0 (x)
              2 LOAD_CONST               0 ('>13')
              4 FORMAT_VALUE             5 (str, with format)
              6 RETURN_VALUE
""",
        )
        self.assertEqual(
            eval(code, locals={"x": 42}), str.__mod__("%13s", (42,))  # noqa: P204
        )

    def test_percent_d_with_width_and_flags_constant_folded(self):
        code = compile("'%05d' % (-5,)", "", "eval")
        self.assertEqual(
            dis_str(code),
            """\
  1           0 LOAD_CONST               0 ('-0005')
              2 RETURN_VALUE
""",
        )
        self.assertEqual(eval(code), str.__mod__("%05d", (-5,)))  # noqa: P204

    @unittest.skipIf(True, "TODO(T78706522): Port printf transforms to compiler/")
    def test_percent_d_with_width_and_flags(self):
        code = compile("'%05d' % (x,)", "", "eval")
        self.assertEqual(
            dis_str(code),
            """\
  1           0 LOAD_CONST               0 ('')
              2 LOAD_METHOD              0 (_mod_convert_number)
              4 LOAD_NAME                1 (x)
              6 CALL_METHOD              1
              8 LOAD_CONST               1 ('05')
             10 FORMAT_VALUE             4 (with format)
             12 RETURN_VALUE
""",
        )
        self.assertEqual(
            eval(code, locals={"x": -5}), str.__mod__("%05d", (-5,))  # noqa: P204
        )

    def test_mixed_constant_folded(self):
        code = compile("'%s %% foo %r bar %a %s' % (1,2,3,4)", "", "eval")
        self.assertEqual(
            dis_str(code),
            """\
  1           0 LOAD_CONST               0 ('1 % foo 2 bar 3 4')
              2 RETURN_VALUE
""",
        )
        self.assertEqual(
            eval(code),  # noqa: P204
            str.__mod__("%s %% foo %r bar %a %s", (1, 2, 3, 4)),
        )

    def test_mixed(self):
        code = compile("'%s %% foo %r bar %a %s' % (a, b, c, d)", "", "eval")
        self.assertEqual(
            dis_str(code),
            """\
  1           0 LOAD_NAME                0 (a)
              2 FORMAT_VALUE             1 (str)
              4 LOAD_CONST               0 (' ')
              6 LOAD_CONST               1 ('% foo ')
              8 LOAD_NAME                1 (b)
             10 FORMAT_VALUE             2 (repr)
             12 LOAD_CONST               2 (' bar ')
             14 LOAD_NAME                2 (c)
             16 FORMAT_VALUE             3 (ascii)
             18 LOAD_CONST               0 (' ')
             20 LOAD_NAME                3 (d)
             22 FORMAT_VALUE             1 (str)
             24 BUILD_STRING             8
             26 RETURN_VALUE
""",
        )
        self.assertEqual(
            eval(code, locals={"a": 1, "b": 2, "c": 3, "d": 4}),  # noqa: P204
            str.__mod__("%s %% foo %r bar %a %s", (1, 2, 3, 4)),
        )

    @unittest.skipIf(True, "TODO(T78706522): Port printf transforms to compiler/")
    # Getting this working might require some modifications to the Dino
    # compiler.
    def test_with_invalid_ast_raises_type_error(self):
        import ast

        with self.assertRaises(TypeError):
            compile(ast.Module(), "test", "exec")

    def test_without_tuple_constant_folded(self):
        code = compile("'%s' % 5.5", "", "eval")
        self.assertEqual(
            dis_str(code),
            """\
  1           0 LOAD_CONST               0 ('5.5')
              2 RETURN_VALUE
""",
        )
        self.assertEqual(eval(code), str.__mod__("%s", 5.5))  # noqa: P204

    @unittest.skipIf(True, "TODO(T78706522): Port printf transforms to compiler/")
    def test_without_tuple(self):
        code = compile("'%s' % x", "", "eval")
        self.assertEqual(
            dis_str(code),
            """\
  1           0 LOAD_CONST               0 ('')
              2 LOAD_METHOD              0 (_mod_check_single_arg)
              4 LOAD_NAME                1 (x)
              6 CALL_METHOD              1
              8 LOAD_CONST               1 (0)
             10 BINARY_SUBSCR
             12 FORMAT_VALUE             1 (str)
             14 RETURN_VALUE
""",
        )
        self.assertEqual(
            eval(code, locals={"x": 5.5}), str.__mod__("%s", 5.5)  # noqa: P204
        )

    @unittest.skipIf(True, "TODO(T78706522): Port printf transforms to compiler/")
    def test_without_tuple_formats_value(self):
        code = compile("'%s' % x", "", "eval")
        self.assertNotIn("BINARY_MODULO", dis_str(code))
        self.assertEqual(
            eval(code, None, {"x": -8}), str.__mod__("%s", -8)  # noqa: P204
        )
        self.assertEqual(
            eval(code, None, {"x": (True,)}), str.__mod__("%s", (True,))  # noqa: P204
        )

    @unittest.skipIf(True, "TODO(T78706522): Port printf transforms to compiler/")
    def test_without_tuple_raises_type_error(self):
        code = compile("'%s' % x", "", "eval")
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
        code = compile("'foo%' % ()", "", "eval")
        self.assertIn("BINARY_MODULO", dis_str(code))

    def test_no_no_tuple(self):
        code = compile("'%s%s' % x", "", "eval")
        self.assertIn("BINARY_MODULO", dis_str(code))

    def test_no_mapping_key(self):
        code = compile("'%(foo)' % x", "", "eval")
        self.assertIn("BINARY_MODULO", dis_str(code))

        code = compile("'%(%s)' % x", "", "eval")
        self.assertIn("BINARY_MODULO", dis_str(code))

    def test_no_tuple_too_small(self):
        code = compile("'%s%s%s' % (1,2)", "", "eval")
        self.assertIn("BINARY_MODULO", dis_str(code))

    def test_no_tuple_too_big(self):
        code = compile("'%s%s%s' % (1,2,3,4)", "", "eval")
        self.assertIn("BINARY_MODULO", dis_str(code))

    def test_no_unknown_specifier(self):
        code = compile("'%Z' % (None,)", "", "eval")
        self.assertIn("BINARY_MODULO", dis_str(code))


if __name__ == "__main__":
    unittest.main()
