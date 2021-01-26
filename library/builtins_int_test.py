#!/usr/bin/env python3
import unittest

from test_support import pyro_only, supports_38_feature

try:
    from _builtins import _int_ctor
except ImportError:
    pass


class IntTests(unittest.TestCase):
    def test_dunder_hash_with_small_number_returns_self(self):
        self.assertEqual(int.__hash__(0), 0)
        self.assertEqual(int.__hash__(1), 1)
        self.assertEqual(int.__hash__(-5), -5)
        self.assertEqual(int.__hash__(10000), 10000)
        self.assertEqual(int.__hash__(-10000), -10000)

    def test_dunder_hash_minus_one_returns_minus_two(self):
        self.assertEqual(int.__hash__(-1), -2)

        import sys

        factor = sys.hash_info.modulus + 1
        self.assertEqual(int.__hash__(-1 * factor), -2)
        self.assertEqual(int.__hash__(-1 * factor * factor * factor), -2)

    def test_dunder_hash_applies_modulus(self):
        import sys

        modulus = sys.hash_info.modulus

        self.assertEqual(int.__hash__(modulus - 1), modulus - 1)
        self.assertEqual(int.__hash__(modulus), 0)
        self.assertEqual(int.__hash__(modulus + 1), 1)
        self.assertEqual(int.__hash__(modulus + modulus + 13), 13)
        self.assertEqual(int.__hash__(-modulus + 1), -modulus + 1)
        self.assertEqual(int.__hash__(-modulus), 0)
        self.assertEqual(int.__hash__(-modulus - 2), -2)
        self.assertEqual(int.__hash__(-modulus - modulus - 13), -13)

    def test_dunder_hash_with_largeint_returns_int(self):
        self.assertEqual(int.__hash__(1 << 234), 1 << 51)
        self.assertEqual(int.__hash__(-1 << 234), -1 << 51)

        value = 0xB9F041FF6B5D18158ABA6BDAE4B582C01F55A792
        self.assertEqual(int.__hash__(value), 2278332794247153219)
        self.assertEqual(int.__hash__(-value), -2278332794247153219)

    def test_dunder_new_with_bool_class_raises_type_error(self):
        with self.assertRaisesRegex(
            TypeError, r"int\.__new__\(bool\) is not safe.*bool\.__new__\(\)"
        ):
            int.__new__(bool, 0)

    def test_dunder_new_with_dunder_int_subclass_warns(self):
        class Num(int):
            pass

        class Foo:
            def __int__(self):
                return Num(42)

        foo = Foo()
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(int.__new__(int, foo), 42)

    def test_dunder_new_uses_type_dunder_int(self):
        class Foo:
            def __int__(self):
                return 0

        foo = Foo()
        foo.__int__ = "not callable"
        self.assertEqual(int.__new__(int, foo), 0)

    def test_dunder_new_uses_type_dunder_trunc(self):
        class Foo:
            def __trunc__(self):
                return 0

        foo = Foo()
        foo.__trunc__ = "not callable"
        self.assertEqual(int.__new__(int, foo), 0)

    def test_dunder_new_with_raising_trunc_propagates_error(self):
        class Desc:
            def __get__(self, obj, type):
                raise AttributeError("failed")

        class Foo:
            __trunc__ = Desc()

        foo = Foo()
        with self.assertRaises(AttributeError) as context:
            int.__new__(int, foo)
        self.assertEqual(str(context.exception), "failed")

    def test_dunder_new_with_base_without_str_raises_type_error(self):
        with self.assertRaises(TypeError):
            int.__new__(int, base=8)

    def test_dunder_new_with_bool_returns_int(self):
        self.assertIs(int.__new__(int, False), 0)
        self.assertIs(int.__new__(int, True), 1)

    def test_dunder_new_with_bytearray_returns_int(self):
        self.assertEqual(int.__new__(int, bytearray(b"23")), 23)
        self.assertEqual(int.__new__(int, bytearray(b"-23"), 8), -0o23)
        self.assertEqual(int.__new__(int, bytearray(b"abc"), 16), 0xABC)
        self.assertEqual(int.__new__(int, bytearray(b"0xabc"), 0), 0xABC)

    def test_dunder_new_with_bytes_returns_int(self):
        self.assertEqual(int.__new__(int, b"-23"), -23)
        self.assertEqual(int.__new__(int, b"23", 8), 0o23)
        self.assertEqual(int.__new__(int, b"abc", 16), 0xABC)
        self.assertEqual(int.__new__(int, b"0xabc", 0), 0xABC)

    def test_dunder_new_strips_whitespace(self):
        self.assertEqual(int.__new__(int, " \t\n123\n\t "), 123)
        self.assertEqual(int.__new__(int, " \t\n123\n\t ", 10), 123)
        self.assertEqual(int.__new__(int, b" \t\n123\n\t "), 123)
        self.assertEqual(int.__new__(int, b" \t\n123\n\t ", 10), 123)
        self.assertEqual(int.__new__(int, bytearray(b" \t\n123\n\t "), 10), 123)

    def test_dunder_new_with_non_space_after_whitespace_raises_value_error(self):
        self.assertRaises(ValueError, int, " \t\n123\n\t 1")
        self.assertRaises(ValueError, int, " \t\n123\n\t 1", 10)
        self.assertRaises(ValueError, int, b" \t\n123\n\t 1")
        self.assertRaises(ValueError, int, b" \t\n123\n\t 1", 10)
        self.assertRaises(ValueError, int, bytearray(b" \t\n123\n\t 1"))
        self.assertRaises(ValueError, int, bytearray(b" \t\n123\n\t 1"), 10)

    def test_dunder_new_with_empty_bytearray_raises_value_error(self):
        with self.assertRaises(ValueError):
            int.__new__(int, bytearray())

    def test_dunder_new_with_empty_bytes_raises_value_error(self):
        with self.assertRaises(ValueError):
            int.__new__(int, b"")

    def test_dunder_new_with_empty_str_raises_value_error(self):
        with self.assertRaises(ValueError):
            int.__new__(int, "")

    def test_dunder_new_with_int_returns_int(self):
        self.assertEqual(int.__new__(int, 23), 23)

    def test_dunder_new_with_int_and_base_raises_type_error(self):
        with self.assertRaises(TypeError):
            int.__new__(int, 4, 5)

    def test_dunder_new_with_invalid_base_raises_value_error(self):
        with self.assertRaises(ValueError):
            int.__new__(int, "0", 1)

    def test_dunder_new_with_invalid_chars_raises_value_error(self):
        with self.assertRaises(ValueError):
            int.__new__(int, "&2*")

    def test_dunder_new_with_invalid_digits_raises_value_error(self):
        with self.assertRaises(ValueError):
            int.__new__(int, b"789", 6)

    def test_dunder_new_with_str_returns_int(self):
        self.assertEqual(int.__new__(int, "23"), 23)
        self.assertEqual(int.__new__(int, "-23", 8), -0o23)
        self.assertEqual(int.__new__(int, "-abc", 16), -0xABC)
        self.assertEqual(int.__new__(int, "0xabc", 0), 0xABC)

    def test_dunder_new_with_zero_args_returns_zero(self):
        self.assertIs(int.__new__(int), 0)

    def test_dunder_pow_with_zero_returns_one(self):
        self.assertEqual(int.__pow__(4, 0), 1)

    def test_dunder_pow_with_one_returns_self(self):
        self.assertEqual(int.__pow__(4, 1), 4)

    def test_dunder_pow_with_two_squares_number(self):
        self.assertEqual(int.__pow__(4, 2), 16)

    def test_dunder_pow_with_mod_equals_one_returns_zero(self):
        self.assertEqual(int.__pow__(4, 2, 1), 0)

    def test_dunder_pow_with_negative_power_and_mod_raises_value_error(self):
        with self.assertRaises(ValueError):
            int.__pow__(4, -2, 1)

    def test_dunder_pow_with_mod(self):
        self.assertEqual(int.__pow__(4, 8, 10), 6)

    def test_dunder_pow_with_large_mod(self):
        self.assertEqual(int.__pow__(10, 10 ** 1000, 3), 1)

    def test_dunder_pow_with_negative_base_calls_float_dunder_pow(self):
        self.assertLess((int.__pow__(2, -1) - 0.5).__abs__(), 0.00001)

    def test_dunder_pow_with_non_int_self_raises_type_error(self):
        with self.assertRaises(TypeError):
            int.__pow__(None, 1, 1)

    def test_dunder_pow_with_non_int_power_returns_not_implemented(self):
        self.assertEqual(int.__pow__(1, None), NotImplemented)

    @supports_38_feature
    def test_as_integer_ratio_with_non_int_raises_type_error(self):
        with self.assertRaises(TypeError) as ctx:
            int.as_integer_ratio("foo")
        exception_text = str(ctx.exception)
        self.assertIn("'as_integer_ratio'", exception_text)
        self.assertIn("'int'", exception_text)
        self.assertIn("'str'", exception_text)

    @supports_38_feature
    def test_as_integer_ratio_with_int_returns_tuple_with_same_object(self):
        i = 5
        result = i.as_integer_ratio()
        self.assertIs(type(result), tuple)
        self.assertEqual(len(result), 2)
        self.assertIs(result[0], i)
        self.assertEqual(result[1], 1)

    @supports_38_feature
    def test_as_integer_ratio_with_bool_returns_tuple_with_int(self):
        i = True
        result = i.as_integer_ratio()
        self.assertIs(type(result), tuple)
        self.assertEqual(len(result), 2)
        self.assertEqual(result[0], 1)
        self.assertEqual(result[1], 1)

    @supports_38_feature
    def test_as_integer_ratio_with_int_subclass_returns_tuple_with_underlying_int(self):
        class C(int):
            pass

        i = C(5)
        result = i.as_integer_ratio()
        self.assertIs(type(result), tuple)
        self.assertEqual(len(result), 2)
        self.assertIs(type(result[0]), int)
        self.assertEqual(result[0], 5)
        self.assertEqual(result[1], 1)

    def test_from_bytes_returns_int(self):
        self.assertEqual(int.from_bytes(b"\xca\xfe", "little"), 0xFECA)

    def test_from_bytes_with_kwargs_returns_int(self):
        self.assertEqual(
            int.from_bytes(bytes=b"\xca\xfe", byteorder="big", signed=True), -13570
        )

    def test_from_bytes_with_bytes_convertible_returns_int(self):
        class C:
            def __bytes__(self):
                return b"*"

        i = C()
        self.assertEqual(int.from_bytes(i, "big"), 42)

    def test_from_bytes_with_invalid_byteorder_raises_before_invalid_type(self):
        with self.assertRaisesRegex(
            ValueError, "byteorder must be either 'little' or 'big'"
        ):
            int.from_bytes("not a bytes object", byteorder="medium")

    def test_from_bytes_with_invalid_bytes_raises_type_error(self):
        with self.assertRaisesRegex(TypeError, "cannot convert 'str' object to bytes"):
            int.from_bytes("not a bytes object", "big")

    def test_from_bytes_with_invalid_byteorder_raises_value_error(self):
        with self.assertRaisesRegex(
            ValueError, "byteorder must be either 'little' or 'big'"
        ):
            int.from_bytes(b"*", byteorder="medium")

    def test_from_bytes_with_invalid_byteorder_raises_type_error(self):
        with self.assertRaises(TypeError):
            int.from_bytes(b"*", byteorder=42)

    def test_from_bytes_uses_type_dunder_bytes(self):
        class Foo:
            def __bytes__(self):
                return b"*"

        foo = Foo()
        foo.__bytes__ = lambda: b"123"
        self.assertEqual(int.from_bytes(foo, "big"), 42)


class IntDunderFormatTests(unittest.TestCase):
    def test_empty_format_returns_str(self):
        self.assertEqual(int.__format__(0, ""), "0")
        self.assertEqual(int.__format__(1, ""), "1")
        self.assertEqual(int.__format__(-1, ""), "-1")
        self.assertEqual(
            int.__format__(0xFF1FD0035E55A752381015D7, ""),
            "78957136519217238320723531223",
        )

    def test_empty_format_calls_dunder_str(self):
        self.assertEqual(int.__format__(True, ""), "True")
        self.assertEqual(int.__format__(False, ""), "False")

        class C(int):
            def __str__(self):
                return "foobar"

        self.assertEqual(int.__format__(C(), ""), "foobar")

    def test_c_format_returns_str(self):
        self.assertEqual(int.__format__(0, "c"), "\0")
        self.assertEqual(int.__format__(False, "c"), "\0")
        self.assertEqual(int.__format__(True, "c"), "\01")
        self.assertEqual(int.__format__(80, "c"), "P")
        self.assertEqual(int.__format__(128013, "c"), "\U0001f40d")
        import sys

        self.assertEqual(int.__format__(sys.maxunicode, "c"), chr(sys.maxunicode))

    def test_d_format_returns_str(self):
        self.assertEqual(int.__format__(0, "d"), "0")
        self.assertEqual(int.__format__(False, "d"), "0")
        self.assertEqual(int.__format__(-1, "d"), "-1")
        self.assertEqual(int.__format__(1, "d"), "1")
        self.assertEqual(int.__format__(True, "d"), "1")
        self.assertEqual(int.__format__(42, "d"), "42")
        self.assertEqual(
            int.__format__(0x2B3EFBA733D579B55A9074934, "d"),
            "214143955543398893443684452660",
        )
        self.assertEqual(
            int.__format__(-0xF52A2EC166BD52D048CD1EA6C6E478B3, "d"),
            "-325879883749036333909592275151101393075",
        )

    def test_x_format_returns_str(self):
        self.assertEqual(int.__format__(0, "x"), "0")
        self.assertEqual(int.__format__(False, "x"), "0")
        self.assertEqual(int.__format__(-1, "x"), "-1")
        self.assertEqual(int.__format__(1, "x"), "1")
        self.assertEqual(int.__format__(True, "x"), "1")
        self.assertEqual(int.__format__(42, "x"), "2a")
        self.assertEqual(
            int.__format__(214143955543398893443684452660, "x"),
            "2b3efba733d579b55a9074934",
        )
        self.assertEqual(
            int.__format__(-32587988374903633390959227539375, "x"),
            "-19b51782f9224e54288bc2f8faf",
        )

    def test_big_x_format_returns_str(self):
        self.assertEqual(int.__format__(0, "X"), "0")
        self.assertEqual(int.__format__(-1, "X"), "-1")
        self.assertEqual(int.__format__(1, "X"), "1")
        self.assertEqual(int.__format__(42, "X"), "2A")
        self.assertEqual(
            int.__format__(214143955543398893443684452660, "X"),
            "2B3EFBA733D579B55A9074934",
        )

    def test_o_format_returns_str(self):
        self.assertEqual(int.__format__(0, "o"), "0")
        self.assertEqual(int.__format__(1, "o"), "1")
        self.assertEqual(int.__format__(-1, "o"), "-1")
        self.assertEqual(int.__format__(42, "o"), "52")
        self.assertEqual(
            int.__format__(0xFF1FD0035E55A752381015D7, "o"),
            "77617720006571255165107004012727",
        )
        self.assertEqual(
            int.__format__(-0x6D68DA9740D4105E240D77C587602, "o"),
            "-155321552272015202027422015357426073002",
        )

    def test_b_format_returns_str(self):
        self.assertEqual(int.__format__(0, "b"), "0")
        self.assertEqual(int.__format__(1, "b"), "1")
        self.assertEqual(int.__format__(-1, "b"), "-1")
        self.assertEqual(int.__format__(42, "b"), "101010")
        self.assertEqual(
            int.__format__(0x2D393E39DFD869D3DD18D24D6FE8C, "b"),
            "101101001110010011111000111001110111111101100001101001110100"
            "111101110100011000110100100100110101101111111010001100",
        )
        self.assertEqual(
            int.__format__(-0x52471A39A4C1BCB4D711249, "b"),
            "-10100100100011100011010001110011010010011000001101111001011"
            "01001101011100010001001001001001",
        )

    def test_float_format_returns_str(self):
        self.assertEqual(int.__format__(2 ** 64, "e"), "1.844674e+19")
        self.assertEqual(int.__format__(-12345, "E"), "-1.234500E+04")
        self.assertEqual(int.__format__(0, "f"), "0.000000")
        self.assertEqual(int.__format__(321, "F"), "321.000000")
        self.assertEqual(int.__format__(-1024, "g"), "-1024")
        self.assertEqual(int.__format__(10 ** 100, "G"), "1E+100")
        self.assertEqual(int.__format__(1, "%"), "100.000000%")

    def test_alternate_returns_str(self):
        self.assertEqual(int.__format__(0, "#"), "0")
        self.assertEqual(int.__format__(-123, "#"), "-123")
        self.assertEqual(int.__format__(42, "#d"), "42")
        self.assertEqual(int.__format__(-99, "#d"), "-99")
        self.assertEqual(int.__format__(77, "#b"), "0b1001101")
        self.assertEqual(int.__format__(-11, "#b"), "-0b1011")
        self.assertEqual(int.__format__(22, "#o"), "0o26")
        self.assertEqual(int.__format__(-33, "#o"), "-0o41")
        self.assertEqual(int.__format__(123, "#x"), "0x7b")
        self.assertEqual(int.__format__(-44, "#x"), "-0x2c")
        self.assertEqual(int.__format__(123, "#X"), "0X7B")
        self.assertEqual(int.__format__(-44, "#X"), "-0X2C")

    def test_with_sign_returns_str(self):
        self.assertEqual(int.__format__(7, " "), " 7")
        self.assertEqual(int.__format__(7, "+"), "+7")
        self.assertEqual(int.__format__(7, "-"), "7")
        self.assertEqual(int.__format__(-4, " "), "-4")
        self.assertEqual(int.__format__(-4, "+"), "-4")
        self.assertEqual(int.__format__(-4, "-"), "-4")

    def test_with_alignment_returns_str(self):
        self.assertEqual(int.__format__(-42, "0"), "-42")
        self.assertEqual(int.__format__(-42, "1"), "-42")
        self.assertEqual(int.__format__(-42, "5"), "  -42")
        self.assertEqual(int.__format__(-42, "05"), "-0042")
        self.assertEqual(int.__format__(-42, "0005"), "-0042")
        self.assertEqual(int.__format__(-42, "<5"), "-42  ")
        self.assertEqual(int.__format__(-42, ">5"), "  -42")
        self.assertEqual(int.__format__(-42, "=5"), "-  42")
        self.assertEqual(int.__format__(-42, "^6"), " -42  ")
        self.assertEqual(int.__format__(-42, "^^7"), "^^-42^^")
        self.assertEqual(int.__format__(-123, "#=#20x"), "-0x###############7b")
        self.assertEqual(
            int.__format__(-42, "\U0001f40d^6o"), "\U0001f40d-52\U0001f40d\U0001f40d"
        )

    def test_with_alternate_and_zero_returns_str(self):
        self.assertEqual(int.__format__(100, "#012b"), "0b0001100100")
        self.assertEqual(int.__format__(-42, "#08d"), "-0000042")
        self.assertEqual(int.__format__(88, "#013o"), "0o00000000130")
        self.assertEqual(int.__format__(99, "+#07x"), "+0x0063")
        self.assertEqual(int.__format__(33, "+#014X"), "+0X00000000021")

    def test_c_format_with_alignment_returns_str(self):
        self.assertEqual(int.__format__(65, ".^12c"), ".....A......")
        self.assertEqual(int.__format__(128013, ".^12c"), ".....\U0001f40d......")
        self.assertEqual(
            int.__format__(90, "\U0001f40d<4c"), "Z\U0001f40d\U0001f40d\U0001f40d"
        )
        self.assertEqual(
            int.__format__(90, "\U0001f40d>4c"), "\U0001f40d\U0001f40d\U0001f40dZ"
        )
        self.assertEqual(
            int.__format__(90, "\U0001f40d=4c"), "\U0001f40d\U0001f40d\U0001f40dZ"
        )

    def test_all_codes_with_alignment_returns_str(self):
        self.assertEqual(int.__format__(-123, "^12"), "    -123    ")
        self.assertEqual(int.__format__(-123, "^12d"), "    -123    ")
        self.assertEqual(int.__format__(-123, ".^#12b"), ".-0b1111011.")
        self.assertEqual(int.__format__(-123, ".^#12o"), "...-0o173...")
        self.assertEqual(int.__format__(-123, ".^#12x"), "...-0x7b....")
        self.assertEqual(int.__format__(-123, ".^#12X"), "...-0X7B....")

    def test_with_alignment_and_sign_returns_str(self):
        self.assertEqual(int.__format__(9, " 5"), "    9")
        self.assertEqual(int.__format__(9, "+5"), "   +9")
        self.assertEqual(int.__format__(9, "\t< 5"), " 9\t\t\t")
        self.assertEqual(int.__format__(9, "<+5"), "+9   ")
        self.assertEqual(int.__format__(9, ";= 5"), " ;;;9")
        self.assertEqual(int.__format__(9, "=+5"), "+   9")

    def test_works_with_subclass(self):
        class C(int):
            pass

        self.assertEqual(int.__format__(C(42), ""), "42")
        self.assertEqual(int.__format__(C(8), "*^#8b"), "*0b1000*")

        class D(str):
            pass

        self.assertEqual(int.__format__(42, D("")), "42")
        self.assertEqual(int.__format__(C(6), D("*^#8b")), "*0b110**")

    def test_precision_raises_error(self):
        with self.assertRaises(ValueError) as context:
            int.__format__(0, ".10")
        self.assertEqual(
            str(context.exception), "Precision not allowed in integer format specifier"
        )
        with self.assertRaises(ValueError) as context:
            int.__format__(0, ".10c")
        self.assertEqual(
            str(context.exception), "Precision not allowed in integer format specifier"
        )
        with self.assertRaises(ValueError) as context:
            int.__format__(0, ".10d")
        self.assertEqual(
            str(context.exception), "Precision not allowed in integer format specifier"
        )
        with self.assertRaises(ValueError) as context:
            int.__format__(0, ".10o")
        self.assertEqual(
            str(context.exception), "Precision not allowed in integer format specifier"
        )
        with self.assertRaises(ValueError) as context:
            int.__format__(0, ".1b")
        self.assertEqual(
            str(context.exception), "Precision not allowed in integer format specifier"
        )
        with self.assertRaises(ValueError) as context:
            int.__format__(0, ".10x")
        self.assertEqual(
            str(context.exception), "Precision not allowed in integer format specifier"
        )
        with self.assertRaises(ValueError) as context:
            int.__format__(0, ".10X")
        self.assertEqual(
            str(context.exception), "Precision not allowed in integer format specifier"
        )

    def test_c_format_raises_overflow_error(self):
        with self.assertRaises(OverflowError) as context:
            int.__format__(-1, "c")
        self.assertEqual(str(context.exception), "%c arg not in range(0x110000)")

        import sys

        with self.assertRaises(OverflowError) as context:
            int.__format__(sys.maxunicode + 1, "c")
        self.assertEqual(str(context.exception), "%c arg not in range(0x110000)")

    def test_c_format_with_sign_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            int.__format__(64, "+c")
        self.assertEqual(
            str(context.exception), "Sign not allowed with integer format specifier 'c'"
        )

    def test_c_format_alternate_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            int.__format__(64, "#c")
        self.assertEqual(
            str(context.exception),
            "Alternate form (#) not allowed with integer format specifier 'c'",
        )

    def test_unknown_format_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            int.__format__(42, "z")
        self.assertEqual(
            str(context.exception), "Unknown format code 'z' for object of type 'int'"
        )

    def test_with_non_int_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            int.__format__("not an int", "")
        self.assertIn("'__format__' requires a 'int' object", str(context.exception))
        self.assertIn("'str'", str(context.exception))

    @pyro_only
    def test_under_int_ctor_with_small_int_returns_int(self):
        actual = _int_ctor(int, 4)
        self.assertEqual(actual, 4)

    @pyro_only
    def test_under_int_ctor_with_bool_true_returns_int(self):
        actual = _int_ctor(int, True)
        self.assertEqual(actual, 1)

    @pyro_only
    def test_under_int_ctor_with_bool_false_returns_int(self):
        actual = _int_ctor(int, False)
        self.assertEqual(actual, 0)

    @pyro_only
    def test_under_int_ctor_with_float_returns_int(self):
        actual = _int_ctor(int, 3.14)
        self.assertEqual(actual, 3)

    @pyro_only
    def test_under_int_ctor_with_empty_small_str_raises_value_error(self):
        with self.assertRaises(ValueError):
            _int_ctor(int, "")

    @pyro_only
    def test_under_int_ctor_with_small_str_for_positive_number_returns_int(self):
        actual = _int_ctor(int, "213")
        self.assertEqual(actual, 213)

    @pyro_only
    def test_under_int_ctor_with_small_str_for_negative_number_returns_int(self):
        actual = _int_ctor(int, "-213")
        self.assertEqual(actual, -213)

    @pyro_only
    def test_under_int_ctor_with_no_argument_returns_int(self):
        actual = _int_ctor(int)
        self.assertEqual(actual, 0)

    @pyro_only
    def test_under_int_ctor_with_arguments_with_dunder_int_returns_int(self):
        class D:
            def __int__(self):
                return 12341234123412341234

        d = D()
        actual = _int_ctor(int, d)
        self.assertEqual(actual, 12341234123412341234)


if __name__ == "__main__":
    unittest.main()
