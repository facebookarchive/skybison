#!/usr/bin/env python3
import unittest

from test_support import supports_38_feature


class FloatTests(unittest.TestCase):
    def test_dunder_divmod_raises_type_error(self):
        with self.assertRaises(TypeError):
            float.__divmod__(1, 1.0)

    def test_dunder_divmod_with_non_number_as_other_returns_not_implemented(self):
        self.assertIs(float.__divmod__(1.0, "1"), NotImplemented)

    def test_dunder_divmod_with_zero_denominator_raises_zero_division_error(self):
        with self.assertRaises(ZeroDivisionError):
            float.__divmod__(1.0, 0.0)

    def test_dunder_divmod_with_negative_zero_denominator_raises_zero_division_error(
        self,
    ):
        with self.assertRaises(ZeroDivisionError):
            float.__divmod__(1.0, -0.0)

    def test_dunder_divmod_with_zero_numerator(self):
        floordiv, remainder = float.__divmod__(0.0, 4.0)
        self.assertEqual(floordiv, 0.0)
        self.assertEqual(remainder, 0.0)

    def test_dunder_divmod_with_positive_denominator_positive_numerator(self):
        floordiv, remainder = float.__divmod__(3.25, 1.0)
        self.assertEqual(floordiv, 3.0)
        self.assertEqual(remainder, 0.25)

    def test_dunder_divmod_with_negative_denominator_positive_numerator(self):
        floordiv, remainder = float.__divmod__(-3.25, 1.0)
        self.assertEqual(floordiv, -4.0)
        self.assertEqual(remainder, 0.75)

    def test_dunder_divmod_with_negative_denominator_negative_numerator(self):
        floordiv, remainder = float.__divmod__(-3.25, -1.0)
        self.assertEqual(floordiv, 3.0)
        self.assertEqual(remainder, -0.25)

    def test_dunder_divmod_with_positive_denominator_negative_numerator(self):
        floordiv, remainder = float.__divmod__(3.25, -1.0)
        self.assertEqual(floordiv, -4.0)
        self.assertEqual(remainder, -0.75)

    def test_dunder_divmod_with_nan_denominator(self):
        import math

        floordiv, remainder = float.__divmod__(3.25, float("nan"))
        self.assertTrue(math.isnan(floordiv))
        self.assertTrue(math.isnan(remainder))

    def test_dunder_divmod_with_nan_numerator(self):
        import math

        floordiv, remainder = float.__divmod__(float("nan"), 1.0)
        self.assertTrue(math.isnan(floordiv))
        self.assertTrue(math.isnan(remainder))

    def test_dunder_divmod_with_negative_nan_denominator(self):
        import math

        floordiv, remainder = float.__divmod__(3.25, float("-nan"))
        self.assertTrue(math.isnan(floordiv))
        self.assertTrue(math.isnan(remainder))

    def test_dunder_divmod_with_negative_nan_numerator(self):
        import math

        floordiv, remainder = float.__divmod__(float("-nan"), 1.0)
        self.assertTrue(math.isnan(floordiv))
        self.assertTrue(math.isnan(remainder))

    def test_dunder_divmod_with_inf_denominator(self):
        floordiv, remainder = float.__divmod__(3.25, float("inf"))
        self.assertEqual(floordiv, 0.0)
        self.assertEqual(remainder, 3.25)

    def test_dunder_divmod_with_inf_numerator(self):
        import math

        floordiv, remainder = float.__divmod__(float("inf"), 1.0)
        self.assertTrue(math.isnan(floordiv))
        self.assertTrue(math.isnan(remainder))

    def test_dunder_divmod_with_negative_inf_denominator(self):
        floordiv, remainder = float.__divmod__(3.25, float("-inf"))
        self.assertEqual(floordiv, -1.0)
        self.assertEqual(remainder, -float("inf"))

    def test_dunder_divmod_with_negative_inf_numerator(self):
        import math

        floordiv, remainder = float.__divmod__(float("-inf"), 1.0)
        self.assertTrue(math.isnan(floordiv))
        self.assertTrue(math.isnan(remainder))

    def test_dunder_divmod_with_big_numerator(self):
        floordiv, remainder = float.__divmod__(1e200, 1.0)
        self.assertEqual(floordiv, 1e200)
        self.assertEqual(remainder, 0.0)

    def test_dunder_divmod_with_big_denominator(self):
        floordiv, remainder = float.__divmod__(1.0, 1e200)
        self.assertEqual(floordiv, 0.0)
        self.assertEqual(remainder, 1.0)

    def test_dunder_divmod_with_negative_zero_numerator(self):
        floordiv, remainder = float.__divmod__(-0.0, 4.0)
        self.assertTrue(str(floordiv) == "-0.0")
        self.assertEqual(remainder, 0.0)

    def test_dunder_floordiv_raises_type_error(self):
        with self.assertRaises(TypeError):
            float.__floordiv__(1, 1.0)

    def test_dunder_floordiv_with_non_number_as_other_returns_not_implemented(self):
        self.assertIs(float.__floordiv__(1.0, "1"), NotImplemented)

    def test_dunder_floordiv_with_zero_denominator_raises_zero_division_error(self):
        with self.assertRaises(ZeroDivisionError):
            float.__floordiv__(1.0, 0.0)

    def test_dunder_floordiv_returns_floor_quotient(self):
        self.assertEqual(float.__floordiv__(3.25, -1.0), -4.0)

    def test_dunder_getformat_with_float_or_double_returns_format(self):
        import sys

        self.assertEqual(float.__getformat__("double"), f"IEEE, {sys.byteorder}-endian")
        self.assertEqual(float.__getformat__("float"), f"IEEE, {sys.byteorder}-endian")

    def test_dunder_getformat_with_non_float_or_double_raises_value_error(self):
        with self.assertRaises(ValueError):
            float.__getformat__("unknown")

    def test_dunder_int_with_non_float_raise_type_error(self):
        with self.assertRaises(TypeError) as context:
            float.__int__("not a float")
        self.assertIn("'__int__' requires a 'float' object", str(context.exception))

    def test_dunder_hash_from_non_finites_returns_well_known_values(self):
        import sys

        self.assertEqual(float.__hash__(float("inf")), sys.hash_info.inf)
        self.assertEqual(float.__hash__(float("-inf")), -(sys.hash_info.inf))
        self.assertEqual(float.__hash__(float("nan")), sys.hash_info.nan)

    def test_dunder_hash_returns_int(self):
        self.assertIsInstance(float.__hash__(0.0), int)
        self.assertIsInstance(float.__hash__(-1.0), int)
        self.assertIsInstance(float.__hash__(42.34532), int)
        self.assertIsInstance(float.__hash__(1.79769e308), int)

    def test_dunder_hash_matches_int_dunder_hash(self):
        self.assertEqual(float.__hash__(0.0), int.__hash__(0))
        self.assertEqual(float.__hash__(-0.0), int.__hash__(0))
        self.assertEqual(float.__hash__(1.0), int.__hash__(1))
        self.assertEqual(float.__hash__(-1.0), int.__hash__(-1))
        self.assertEqual(float.__hash__(42.0), int.__hash__(42))
        self.assertEqual(float.__hash__(-99.0), int.__hash__(-99))
        self.assertEqual(
            float.__hash__(9.313203665422767e55), int.__hash__(0x3CC58055CE060C << 132)
        )
        self.assertEqual(
            float.__hash__(-7.26682022207011e41), int.__hash__(-0x85786CAA960EE << 88)
        )

    def test_dunder_hash_with_negative_exponent_returns_int(self):
        self.assertEqual(float.__hash__(0.5), 1 << 60)

        import sys

        # the following only works for the given modulus.
        self.assertEqual(sys.hash_info.modulus, (1 << 61) - 1)
        self.assertEqual(float.__hash__(6.716542360700249e-22), 0xCAFEBABE00000)

    def test_dunder_hash_with_subnormals_returns_int(self):
        import sys

        # The following tests assume a specific modulus and need to be adapted
        # if that changes.
        self.assertEqual(sys.hash_info.modulus, (1 << 61) - 1)
        # This is the smallest number that is not a subnormal yet.
        self.assertEqual(float.__hash__(2.2250738585072014e-308), 1 << 15)
        # The following are subnormal numbers:
        self.assertEqual(float.__hash__(1.1125369292536007e-308), 1 << 14)
        self.assertEqual(float.__hash__(2.225073858507201e-308), 0x1FFFFFFFFF007FFF)
        self.assertEqual(float.__hash__(5e-324), 1 << 24)
        # This is the smallest number that is not a subnormal yet.
        self.assertEqual(float.__hash__(-2.2250738585072014e-308), -1 << 15)
        # The following are subnormal numbers:
        self.assertEqual(float.__hash__(-1.1125369292536007e-308), -1 << 14)
        self.assertEqual(float.__hash__(-2.225073858507201e-308), -0x1FFFFFFFFF007FFF)
        self.assertEqual(float.__hash__(-5e-324), -1 << 24)

    def test_dunder_hash_matches_bool_dunder_hash(self):
        self.assertEqual(float.__hash__(float(True)), bool.__hash__(True))
        self.assertEqual(float.__hash__(float(False)), bool.__hash__(False))

    def test_dunder_hash_with_float_subclass_returns_int(self):
        class C(float):
            pass

        self.assertEqual(float.__hash__(C(-77.0)), -77)

    def test_dunder_hash_with_non_float_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            self.assertEqual(float.__hash__("not a float"))
        self.assertIn("'__hash__' requires a 'float' object", str(context.exception))
        self.assertIn("'str'", str(context.exception))

    def test_dunder_mod_raises_type_error(self):
        with self.assertRaises(TypeError):
            float.__mod__(1, 1.0)

    def test_dunder_mod_with_non_number_as_other_returns_not_implemented(self):
        self.assertIs(float.__mod__(1.0, "1"), NotImplemented)

    def test_dunder_mod_with_zero_denominator_raises_zero_division_error(self):
        with self.assertRaises(ZeroDivisionError):
            float.__mod__(1.0, 0.0)

    def test_dunder_mod_returns_remainder(self):
        self.assertEqual(float.__mod__(3.25, -1.0), -0.75)

    def test_dunder_new_with_non_class_raises_type_error(self):
        with self.assertRaises(TypeError):
            float.__new__("not a type")

    def test_dunder_new_with_non_float_subclass_raises_type_error(self):
        with self.assertRaises(TypeError):
            float.__new__(int)

    def test_dunder_new_with_default_argument_returns_zero(self):
        self.assertEqual(float(), 0.0)

    def test_dunder_new_with_float_returns_same_value(self):
        self.assertEqual(float(1.0), 1.0)

    def test_dunder_new_with_invalid_str_raises_value_error(self):
        with self.assertRaises(ValueError):
            float("1880.3a01")

    def test_dunder_new_with_str_returns_float(self):
        self.assertEqual(float("1.0"), 1.0)

    def test_dunder_new_with_huge_positive_str_returns_inf(self):
        self.assertEqual(float("1.18973e+4932"), float("inf"))

    def test_dunder_new_with_huge_negative_str_returns_negative_inf(self):
        self.assertEqual(float("-1.18973e+4932"), float("-inf"))

    def test_dunder_new_with_invalid_bytes_raises_value_error(self):
        with self.assertRaises(ValueError):
            float(b"1880.3a01")

    def test_dunder_new_with_bytes_returns_float(self):
        self.assertEqual(float(b"1.0"), 1.0)

    def test_dunder_new_with_huge_positive_bytes_returns_inf(self):
        self.assertEqual(float(b"1.18973e+4932"), float("inf"))

    def test_dunder_new_with_huge_negative_bytes_returns_negative_inf(self):
        self.assertEqual(float(b"-1.18973e+4932"), float("-inf"))

    def test_dunder_new_with_float_subclass_calls_dunder_float(self):
        class C(float):
            def __float__(self):
                return 1.0

        c = C()
        self.assertEqual(c, 0.0)
        self.assertEqual(float(c), 1.0)

    @supports_38_feature
    def test_dunder_new_calls_type_index(self):
        class C:
            def __index__(self):
                return 42

        c = C()
        c.__index__ = "not callable"
        result = float.__new__(float, c)
        self.assertEqual(result, 42.0)

    @supports_38_feature
    def test_dunder_new_calls_float_when_index_and_float_exist(self):
        class C:
            def __float__(self):
                return 41.0

            def __index__(self):
                return 42

        c = C()
        c.__index__ = "not callable"
        result = float.__new__(float, c)
        self.assertEqual(result, 41.0)

    def test_dunder_new_with_str_subclass_calls_dunder_float(self):
        class C(str):
            def __float__(self):
                return 1.0

        c = C("0.0")
        self.assertEqual(float(c), 1.0)

    def test_dunder_new_with_int_calls_dunder_float(self):
        self.assertEqual(float(1), 1.0)

    def test_dunder_new_with_raising_descriptor_propagates_exception(self):
        class Desc:
            def __get__(self, obj, type):
                raise IndexError()

        class C:
            __float__ = Desc()

        self.assertRaises(IndexError, float, C())

    def test_dunder_new_without_dunder_float_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            float([])
        self.assertEqual(
            str(context.exception),
            "float() argument must be a string or a number, not 'list'",
        )

    def test_dunder_pow_with_str_returns_float(self):
        result = float.__pow__(2.0, 4)
        self.assertIs(type(result), float)
        self.assertEqual(result, 16.0)

    def test_dunder_pow_with_third_arg_int_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            float.__pow__(2.0, 4.0, 4.0)
        self.assertIn(
            "pow() 3rd argument not allowed unless all arguments are integers",
            str(context.exception),
        )

    def test_dunder_pow_with_third_arg_none_returns_power_of_first_two_args(self):
        self.assertEqual(float.__pow__(2.0, 4.0, None), 16.0)

    def test_dunder_rdivmod_raises_type_error(self):
        with self.assertRaises(TypeError):
            float.__rdivmod__(1, 1.0)

    def test_dunder_rdivmod_with_non_number_as_other_returns_not_implemented(self):
        self.assertIs(float.__rdivmod__(1.0, "1"), NotImplemented)

    def test_dunder_rdivmod_with_int_returns_same_result_as_divmod_with_reversed_args(
        self,
    ):
        self.assertEqual(float.__rdivmod__(1.0, 3), float.__divmod__(float(3), 1.0))

    def test_dunder_rdivmod_returns_same_result_as_divmod_with_reversed_args(self):
        self.assertEqual(float.__rdivmod__(1.0, 3.25), float.__divmod__(3.25, 1.0))

    def test_repr_with_infinity_returns_string(self):
        self.assertEqual(float.__repr__(float("inf")), "inf")
        self.assertEqual(float.__repr__(-float("inf")), "-inf")

    def test_repr_with_nan_returns_nan(self):
        self.assertEqual(float.__repr__(float("nan")), "nan")
        self.assertEqual(float.__repr__(float("-nan")), "nan")

    def test_repr_returns_string_without_exponent(self):
        self.assertEqual(float.__repr__(0.0), "0.0")
        self.assertEqual(float.__repr__(-0.0), "-0.0")
        self.assertEqual(float.__repr__(1.0), "1.0")
        self.assertEqual(float.__repr__(-1.0), "-1.0")
        self.assertEqual(float.__repr__(42.5), "42.5")
        self.assertEqual(float.__repr__(1.234567891234567), "1.234567891234567")
        self.assertEqual(float.__repr__(-1.234567891234567), "-1.234567891234567")
        self.assertEqual(float.__repr__(9.99999999999999e15), "9999999999999990.0")
        self.assertEqual(float.__repr__(0.0001), "0.0001")

    def test_repr_returns_string_with_exponent(self):
        self.assertEqual(float.__repr__(1e16), "1e+16")
        self.assertEqual(float.__repr__(0.00001), "1e-05")
        self.assertEqual(float.__repr__(1e100), "1e+100")
        self.assertEqual(float.__repr__(1e-88), "1e-88")
        self.assertEqual(
            float.__repr__(1.23456789123456789e123), "1.2345678912345679e+123"
        )
        self.assertEqual(
            float.__repr__(-1.23456789123456789e-123), "-1.2345678912345678e-123"
        )

    def test_dunder_rfloordiv_raises_type_error(self):
        with self.assertRaises(TypeError):
            float.__rfloordiv__(1, 1.0)

    def test_dunder_rfloordiv_with_non_number_as_other_returns_not_implemented(self):
        self.assertIs(float.__rfloordiv__(1.0, "1"), NotImplemented)

    def test_dunder_rfloordiv_with_int_returns_same_result_as_floordiv(self):
        self.assertEqual(float.__rfloordiv__(1.0, 3), float.__floordiv__(float(3), 1.0))

    def test_dunder_rfloordiv_returns_same_result_as_floordiv_for_float_other(self):
        self.assertEqual(float.__rfloordiv__(1.0, 3.25), float.__floordiv__(3.25, 1.0))

    def test_dunder_rmod_raises_type_error(self):
        with self.assertRaises(TypeError):
            float.__rmod__(1, 1.0)

    def test_dunder_rmod__with_non_number_as_other_returns_not_implemented(self):
        self.assertIs(float.__rmod__(1.0, "1"), NotImplemented)

    def test_dunder_rmod_with_int_returns_same_result_as_mod_with_reversed_args(self):
        self.assertEqual(float.__rmod__(1.0, 3), float.__mod__(float(3), 1.0))

    def test_dunder_rmod_returns_same_result_as_mod_for_float_other(self):
        self.assertEqual(float.__rmod__(1.0, 3.25), float.__mod__(3.25, 1.0))

    def test_dunder_round_with_one_arg_returns_int(self):
        self.assertEqual(float.__round__(0.0), 0)
        self.assertIsInstance(float.__round__(0.0), int)
        self.assertEqual(float.__round__(-0.0), 0)
        self.assertIsInstance(float.__round__(-0.0), int)
        self.assertEqual(float.__round__(1.0), 1)
        self.assertIsInstance(float.__round__(1.0), int)
        self.assertEqual(float.__round__(-1.0), -1)
        self.assertIsInstance(float.__round__(-1.0), int)
        self.assertEqual(float.__round__(42.42), 42)
        self.assertIsInstance(float.__round__(42.42), int)
        self.assertEqual(float.__round__(0.4), 0)
        self.assertEqual(float.__round__(0.5), 0)
        self.assertEqual(float.__round__(0.5000000000000001), 1)
        self.assertEqual(float.__round__(1.49), 1)
        self.assertEqual(float.__round__(1.5), 2)
        self.assertEqual(float.__round__(1.5000000000000001), 2)
        self.assertEqual(
            float.__round__(1.234567e200),
            123456699999999995062622360556161455756457158443485858665105941107312145749402909576243454437530421952327149599911208391362816498839992520580209467560546813973197632314335145381120371005964774514098176,  # noqa: B950
        )
        self.assertEqual(float.__round__(-13.4, None), -13)
        self.assertIsInstance(float.__round__(-13.4, None), int)

    def test_dunder_round_with_float_subclass_returns_int(self):
        class C(float):
            pass

        self.assertEqual(float.__round__(C(-7654321.7654321)), -7654322)

    def test_dunder_round_with_one_arg_raises_error(self):
        with self.assertRaises(ValueError) as context:
            float.__round__(float("nan"))
        self.assertEqual(str(context.exception), "cannot convert float NaN to integer")
        with self.assertRaises(OverflowError) as context:
            float.__round__(float("inf"))
        self.assertEqual(
            str(context.exception), "cannot convert float infinity to integer"
        )
        with self.assertRaises(OverflowError) as context:
            float.__round__(float("-inf"))
        self.assertEqual(
            str(context.exception), "cannot convert float infinity to integer"
        )

    def test_dunder_round_with_two_args_returns_float(self):
        self.assertEqual(float.__round__(0.0, 0), 0.0)
        self.assertIsInstance(float.__round__(0.0, 0), float)
        self.assertEqual(float.__round__(-0.0, 1), 0.0)
        self.assertIsInstance(float.__round__(-0.0, 1), float)
        self.assertEqual(float.__round__(1.0, 0), 1.0)
        self.assertIsInstance(float.__round__(1.0, 0), float)
        self.assertEqual(float.__round__(-77441.7, -2), -77400.0)
        self.assertIsInstance(float.__round__(-77441.7, -2), float)

        self.assertEqual(float.__round__(12.34567, -(1 << 200)), 0.0)
        self.assertEqual(float.__round__(12.34567, -50), 0.0)
        self.assertEqual(float.__round__(12.34567, -2), 0.0)
        self.assertEqual(float.__round__(12.34567, -1), 10.0)
        self.assertEqual(float.__round__(12.34567, 0), 12.0)
        self.assertEqual(float.__round__(12.34567, 1), 12.3)
        self.assertEqual(float.__round__(12.34567, 2), 12.35)
        self.assertEqual(float.__round__(12.34567, 3), 12.346)
        self.assertEqual(float.__round__(12.34567, 4), 12.3457)
        self.assertEqual(float.__round__(12.34567, 50), 12.34567)
        self.assertEqual(float.__round__(12.34567, 1 << 200), 12.34567)

        self.assertEqual(float("inf"), float("inf"))
        self.assertEqual(float("-inf"), -float("inf"))

        float_max = 1.7976931348623157e308
        self.assertEqual(float.__round__(float_max, -309), 0.0)
        self.assertEqual(float.__round__(float_max, -303), 1.79769e308)

    def test_dunder_round_with_two_args_returns_nan(self):
        import math

        self.assertTrue(math.isnan(float.__round__(float("nan"), 2)))

    def test_dunder_round_with_two_args_raises_error(self):
        float_max = 1.7976931348623157e308
        with self.assertRaises(OverflowError) as context:
            float.__round__(float_max, -308)
        self.assertEqual(str(context.exception), "rounded value too large to represent")

    def test_dunder_rpow_with_non_float_raises_type_error(self):
        with self.assertRaises(TypeError):
            float.__rpow__(None, 2.0)

    def test_dunder_rpow_with_float_float_returns_result_from_pow_with_swapped_args(
        self,
    ):
        self.assertEqual(float.__rpow__(2.0, 5.0), float.__pow__(5.0, 2.0))

    def test_dunder_rpow_with_float_int_returns_result_from_pow_with_swapped_args(self):
        result = float.__rpow__(2.0, 5)
        self.assertIsInstance(result, float)
        self.assertEqual(result, float.__pow__(5.0, 2.0))

    def test_dunder_rpow_with_third_arg_int_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            float.__rpow__(2.0, 4.0, 4.0)
        self.assertIn(
            "pow() 3rd argument not allowed unless all arguments are integers",
            str(context.exception),
        )

    def test_dunder_trunc_returns_int(self):
        self.assertEqual(float.__trunc__(0.0), 0)
        self.assertEqual(float.__trunc__(-0.0), 0)
        self.assertEqual(float.__trunc__(1.0), 1)
        self.assertEqual(float.__trunc__(-1.0), -1)
        self.assertEqual(float.__trunc__(42.12345), 42)
        self.assertEqual(float.__trunc__(1.6069380442589903e60), 1 << 200)
        self.assertEqual(float.__trunc__(1e-20), 0)
        self.assertEqual(float.__trunc__(-1e-20), 0)
        self.assertIsInstance(float.__trunc__(0.0), int)
        self.assertIsInstance(float.__trunc__(1.6069380442589903e60), int)
        self.assertIsInstance(float.__trunc__(1e-20), int)

    def test_dunder_trunc_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            float.__trunc__(float("nan"))
        self.assertEqual(str(context.exception), "cannot convert float NaN to integer")
        with self.assertRaises(OverflowError) as context:
            float.__trunc__(float("inf"))
        self.assertEqual(
            str(context.exception), "cannot convert float infinity to integer"
        )
        with self.assertRaises(OverflowError) as context:
            float.__trunc__(float("-inf"))
        self.assertEqual(
            str(context.exception), "cannot convert float infinity to integer"
        )

    def test_hex_with_positive_zero_returns_positive_zero(self):
        self.assertEqual(float(0).hex(), "0x0.0p+0")

    def test_hex_with_negative_zero_returns_negative_zero(self):
        self.assertEqual(float(-0.0).hex(), "-0x0.0p+0")

    def test_hex_with_positive_infinite_returns_inf(self):
        self.assertEqual(float("inf").hex(), "inf")

    def test_hex_with_negative_infinite_returns_minus_inf(self):
        self.assertEqual(float("-inf").hex(), "-inf")

    def test_hex_with_nan_returns_nan(self):
        self.assertEqual(float("nan").hex(), "nan")

    def test_hex_with_positive_decimal_returns_hex_string(self):
        self.assertEqual(float(3.14159).hex(), "0x1.921f9f01b866ep+1")

    def test_hex_with_negative_decimal_returns_hex_string(self):
        self.assertEqual(float(-0.1).hex(), "-0x1.999999999999ap-4")

    def test_hex_with_string_raises_type_error(self):
        with self.assertRaises(TypeError):
            float.hex("")

    def test_float_with_str_trims_whitespace(self):
        self.assertEqual(float("\t 14.2\n"), 14.2)
        self.assertEqual(float("\t 14"), 14.0)
        self.assertEqual(float(" 14.21    "), 14.21)
        self.assertEqual(float("1  "), 1.0)
        with self.assertRaises(ValueError):
            float("  14.2.1 ")
        with self.assertRaises(ValueError):
            float("")

    def test_float_with_bytes_trims_whitespace(self):
        self.assertEqual(float(b"\t 14.2\n"), 14.2)
        self.assertEqual(float(b" 14.21    "), 14.21)


class FloatDunderFormatTests(unittest.TestCase):
    def test_empty_format_returns_str(self):
        self.assertEqual(float.__format__(0.0, ""), "0.0")
        self.assertEqual(float.__format__(1.2, ""), "1.2")
        self.assertEqual(float.__format__(-1.6, ""), "-1.6")
        self.assertEqual(float("NaN").__format__(""), "nan")
        self.assertEqual(float("inf").__format__(""), "inf")

    def test_empty_format_with_subclass_calls_dunder_str(self):
        class C(float):
            def __str__(self):
                return "foobar"

        self.assertEqual(float.__format__(C(0.0), ""), "foobar")
        self.assertEqual(float.__format__(C("nan"), ""), "foobar")
        self.assertEqual(float.__format__(C("inf"), ""), "foobar")

    def test_nonempty_format_with_subclass_uses_float_value(self):
        class C(float):
            def __str__(self):
                return "foobar"

        self.assertEqual(float.__format__(C(0.0), "e"), "0.000000e+00")
        self.assertEqual(float.__format__(C("nan"), "F"), "NAN")
        self.assertEqual(float.__format__(C("inf"), "g"), "inf")

    def test_e_format_returns_str(self):
        self.assertEqual(float.__format__(0.0, "e"), "0.000000e+00")
        self.assertEqual(float.__format__(-0.0, "e"), "-0.000000e+00")
        self.assertEqual(float.__format__(0.0025, "e"), "2.500000e-03")
        self.assertEqual(float.__format__(-1000.0001, "e"), "-1.000000e+03")
        self.assertEqual(float.__format__(2.0 ** 64, "e"), "1.844674e+19")
        self.assertEqual(float("NAN").__format__("e"), "nan")
        self.assertEqual(float("-INF").__format__("e"), "-inf")

    def test_big_e_format_returns_str(self):
        self.assertEqual(float.__format__(0.0, "E"), "0.000000E+00")
        self.assertEqual(float.__format__(-0.0, "E"), "-0.000000E+00")
        self.assertEqual(float.__format__(0.0025, "E"), "2.500000E-03")
        self.assertEqual(float.__format__(-1000.0001, "E"), "-1.000000E+03")
        self.assertEqual(float.__format__(2.0 ** 64, "E"), "1.844674E+19")
        self.assertEqual(float("nan").__format__("E"), "NAN")
        self.assertEqual(float("-inf").__format__("E"), "-INF")

    def test_f_format_returns_str(self):
        self.assertEqual(float.__format__(0.0, "f"), "0.000000")
        self.assertEqual(float.__format__(-0.0, "f"), "-0.000000")
        self.assertEqual(float.__format__(0.0025, "f"), "0.002500")
        self.assertEqual(float.__format__(-1000.0001, "f"), "-1000.000100")
        self.assertEqual(
            float.__format__(2.0 ** 64, "f"), "18446744073709551616.000000"
        )
        self.assertEqual(float("NaN").__format__("f"), "nan")
        self.assertEqual(float("INF").__format__("f"), "inf")

    def test_big_f_format_returns_str(self):
        self.assertEqual(float.__format__(0.0, "F"), "0.000000")
        self.assertEqual(float.__format__(-0.0, "F"), "-0.000000")
        self.assertEqual(float.__format__(0.0025, "F"), "0.002500")
        self.assertEqual(float.__format__(-1000.0001, "F"), "-1000.000100")
        self.assertEqual(
            float.__format__(2.0 ** 64, "F"), "18446744073709551616.000000"
        )
        self.assertEqual(float("NaN").__format__("F"), "NAN")
        self.assertEqual(float("iNf").__format__("F"), "INF")

    def test_g_format_returns_str(self):
        self.assertEqual(float.__format__(0.0, "g"), "0")
        self.assertEqual(float.__format__(-0.0, "g"), "-0")
        self.assertEqual(float.__format__(0.00025, "g"), "0.00025")
        self.assertEqual(float.__format__(0.000025, "g"), "2.5e-05")
        self.assertEqual(float.__format__(-1000.0001, "g"), "-1000")
        self.assertEqual(float.__format__(123456.789, "g"), "123457")
        self.assertEqual(float.__format__(1234567.89, "g"), "1.23457e+06")
        self.assertEqual(float.__format__(2.0 ** 64, "g"), "1.84467e+19")
        self.assertEqual(float("NaN").__format__("g"), "nan")
        self.assertEqual(float("INF").__format__("g"), "inf")

    def test_big_g_format_returns_str(self):
        self.assertEqual(float.__format__(0.0, "G"), "0")
        self.assertEqual(float.__format__(-0.0, "G"), "-0")
        self.assertEqual(float.__format__(0.00025, "G"), "0.00025")
        self.assertEqual(float.__format__(0.000025, "G"), "2.5E-05")
        self.assertEqual(float.__format__(-1000.0001, "G"), "-1000")
        self.assertEqual(float.__format__(123456.789, "G"), "123457")
        self.assertEqual(float.__format__(1234567.89, "G"), "1.23457E+06")
        self.assertEqual(float.__format__(2.0 ** 64, "G"), "1.84467E+19")
        self.assertEqual(float("Nan").__format__("G"), "NAN")
        self.assertEqual(float("Inf").__format__("G"), "INF")

    # TODO(T52759101): test 'n' uses locale

    def test_percent_format_returns_str(self):
        self.assertEqual(float.__format__(0.0, "%"), "0.000000%")
        self.assertEqual(float.__format__(-0.0, "%"), "-0.000000%")
        self.assertEqual(float.__format__(0.0025, "%"), "0.250000%")
        self.assertEqual(float.__format__(-1000.0001, "%"), "-100000.010000%")
        self.assertEqual(float.__format__(2.0 ** 10, "%"), "102400.000000%")
        self.assertEqual(float("NaN").__format__("%"), "nan%")
        self.assertEqual(float("INF").__format__("%"), "inf%")

    def test_missing_precision_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            float.__format__(0.0, ".f")

        self.assertEqual("Format specifier missing precision", str(context.exception))

    def test_large_integer_precision_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            float.__format__(0.0, ".2147483648f")

        self.assertEqual("precision too big", str(context.exception))

    def test_precision_determines_remainder_length(self):
        self.assertEqual(float.__format__(0.0, ".0e"), "0e+00")
        self.assertEqual(float.__format__(123.0, ".0E"), "1E+02")
        self.assertEqual(float.__format__(4.56, ".0f"), "5")
        self.assertEqual(float.__format__(78.9, ".0F"), "79")
        self.assertEqual(float.__format__(71.0, ".0g"), "7e+01")
        self.assertEqual(float.__format__(0.025, ".0G"), "0.03")
        self.assertEqual(float.__format__(0.005, ".0%"), "0%")
        self.assertEqual(float.__format__(0.0051, ".0%"), "1%")

        self.assertEqual(float.__format__(-0.0, ".2e"), "-0.00e+00")
        self.assertEqual(float.__format__(123.0, ".1E"), "1.2E+02")
        self.assertEqual(float.__format__(4.56, ".10f"), "4.5600000000")
        self.assertEqual(float.__format__(0.00000000000789, ".8F"), "0.00000000")
        self.assertEqual(float.__format__(71.0, ".4g"), "71")
        self.assertEqual(float.__format__(0.000025, ".5G"), "2.5E-05")
        self.assertEqual(float.__format__(0.0005, ".4%"), "0.0500%")

        self.assertEqual(float.__format__(123.456, ".4"), "123.5")
        self.assertEqual(float.__format__(1234.56, ".4"), "1.235e+03")
        self.assertEqual(float.__format__(12345.6, ".4"), "1.235e+04")

    def test_grouping_by_thousands(self):
        self.assertEqual(float.__format__(12345678.9, "_e"), "1.234568e+07")
        self.assertEqual(float.__format__(123456.789, "_E"), "1.234568E+05")
        self.assertEqual(float.__format__(12345678.9, "_f"), "12_345_678.900000")
        self.assertEqual(float.__format__(123456.789, ",F"), "123,456.789000")
        self.assertEqual(float.__format__(12345678.9, ",g"), "1.23457e+07")
        self.assertEqual(float.__format__(123456.789, ",G"), "123,457")
        self.assertEqual(float.__format__(123456.789, "_%"), "12_345_678.900000%")

    def test_padding(self):
        self.assertEqual(float.__format__(123.456, "3e"), "1.234560e+02")
        self.assertEqual(float.__format__(0.0123456789, ">11.4E"), " 1.2346E-02")
        self.assertEqual(float.__format__(0.0, "2f"), "0.000000")
        self.assertEqual(float.__format__(-0.0, "7.2F"), "  -0.00")
        self.assertEqual(float("NaN").__format__("*^8g"), "**nan***")
        self.assertEqual(float.__format__(-10.02, "=8g"), "-  10.02")
        self.assertEqual(float.__format__(-1.234, "!<10G"), "-1.234!!!!")

    def test_alternate_returns_str(self):
        self.assertEqual(float.__format__(0.0, "#"), "0.0")
        self.assertEqual(float.__format__(-123.00, "#.0e"), "-1.e+02")
        self.assertEqual(float.__format__(-0.123, "#.0E"), "-1.E-01")
        self.assertEqual(float.__format__(12.021, "#.2f"), "12.02")
        self.assertEqual(float.__format__(12.021, "#.0F"), "12.")
        self.assertEqual(float.__format__(12.0, "#g"), "12.0000")
        self.assertEqual(float.__format__(2.0 ** 63, "#.1G"), "9.E+18")

    def test_with_sign_returns_str(self):
        self.assertEqual(float.__format__(1.0, " "), " 1.0")
        self.assertEqual(float.__format__(1.0, "+"), "+1.0")
        self.assertEqual(float.__format__(1.0, "-"), "1.0")
        self.assertEqual(float.__format__(-4.0, " "), "-4.0")
        self.assertEqual(float.__format__(-4.0, "+"), "-4.0")
        self.assertEqual(float.__format__(-4.0, "-"), "-4.0")

    def test_sign_aware_zero_padding_allows_zero_width(self):
        self.assertEqual(float.__format__(123.0, "00"), "123.0")
        self.assertEqual(float.__format__(123.34, "00f"), "123.340000")
        self.assertEqual(float.__format__(123.34, "00e"), "1.233400e+02")
        self.assertEqual(float.__format__(123.34, "00g"), "123.34")
        self.assertEqual(float.__format__(123.34, "00.10f"), "123.3400000000")
        self.assertEqual(float.__format__(123.34, "00.10e"), "1.2334000000e+02")
        self.assertEqual(float.__format__(123.34, "00.10g"), "123.34")
        self.assertEqual(float.__format__(123.34, "01f"), "123.340000")

        self.assertEqual(float.__format__(-123.0, "00"), "-123.0")
        self.assertEqual(float.__format__(-123.34, "00f"), "-123.340000")
        self.assertEqual(float.__format__(-123.34, "00e"), "-1.233400e+02")
        self.assertEqual(float.__format__(-123.34, "00g"), "-123.34")
        self.assertEqual(float.__format__(-123.34, "00.10f"), "-123.3400000000")
        self.assertEqual(float.__format__(-123.34, "00.10f"), "-123.3400000000")
        self.assertEqual(float.__format__(-123.34, "00.10e"), "-1.2334000000e+02")
        self.assertEqual(float.__format__(-123.34, "00.10g"), "-123.34")

    def test_unknown_format_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            float.__format__(42.0, "c")
        self.assertEqual(
            str(context.exception), "Unknown format code 'c' for object of type 'float'"
        )

    def test_with_non_float_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            float.__format__("not a float", "")
        self.assertIn("'__format__' requires a 'float' object", str(context.exception))
        self.assertIn("'str'", str(context.exception))

    def test_with_underscore_between_digits_returns_float(self):
        self.assertEqual(float(2_3.5_3), 23.53)

    def test_with_underscore_not_between_digits_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            float("_11")
        self.assertEqual(
            str(context.exception), "could not convert string to float: '_11'"
        )
        with self.assertRaises(ValueError) as context:
            float("4.4_")
        self.assertEqual(
            str(context.exception), "could not convert string to float: '4.4_'"
        )
        with self.assertRaises(ValueError) as context:
            float("2_3.5__3")
        self.assertEqual(
            str(context.exception), "could not convert string to float: '2_3.5__3'"
        )
        with self.assertRaises(ValueError) as context:
            float("4._4")
        self.assertEqual(
            str(context.exception), "could not convert string to float: '4._4'"
        )


if __name__ == "__main__":
    unittest.main()
