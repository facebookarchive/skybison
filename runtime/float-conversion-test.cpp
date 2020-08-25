#include "float-conversion.h"

#include <cmath>
#include <cstdlib>
#include <cstring>

#include "benchmark/benchmark.h"
#include "gtest/gtest.h"

#include "globals.h"
#include "runtime.h"
#include "test-utils.h"

namespace py {
namespace testing {

using FloatConversionTest = RuntimeFixture;

// Note: The testing here is not comprehensive; we have more testing for the
// corresponding formatting and parsing library functions.

TEST_F(FloatConversionTest, DoubleToStringWithZeroReturnsString) {
  unique_c_ptr<char> buf(
      doubleToString(0., 'g', 6, false, false, false, nullptr));
  EXPECT_EQ(std::strcmp(buf.get(), "0"), 0);
}

TEST_F(FloatConversionTest, DoubleToStringWithMinusZeroReturnsString) {
  unique_c_ptr<char> buf(
      doubleToString(-0., 'f', 2, false, false, false, nullptr));
  EXPECT_EQ(std::strcmp(buf.get(), "-0.00"), 0);
}

TEST_F(FloatConversionTest, DoubleToStringReturnsString) {
  unique_c_ptr<char> buf(doubleToString(-42.123456789, 'e', 5,
                                        /*skip_sign=*/true,
                                        /*add_dot_0=*/true,
                                        /*use_alt_formatting=*/true, nullptr));
  EXPECT_EQ(std::strcmp(buf.get(), "4.21235e+01"), 0);
}

TEST_F(FloatConversionTest, DoubleToStringWithNanReturnsString) {
  unique_c_ptr<char> buf(
      doubleToString(std::numeric_limits<double>::quiet_NaN(), 'r', 0, false,
                     false, false, nullptr));
  EXPECT_EQ(std::strcmp(buf.get(), "nan"), 0);
}

TEST_F(FloatConversionTest, DoubleToStringWithInfReturnsString) {
  unique_c_ptr<char> buf(doubleToString(std::numeric_limits<double>::infinity(),
                                        'e', 0, false, false, false, nullptr));
  EXPECT_EQ(std::strcmp(buf.get(), "inf"), 0);
}

TEST_F(FloatConversionTest, ParseFloatReturnsDouble) {
  const char* str = "-42.1234567890123456789ABC";
  char* endptr;
  ConversionResult result;
  double value = parseFloat(str, &endptr, &result);
  EXPECT_EQ(endptr - str, 23);
  EXPECT_EQ(result, ConversionResult::kSuccess);
  EXPECT_EQ(value, std::strtod("-0x1.50fcd6e9ba37bp+5", nullptr));
}

TEST_F(FloatConversionTest, ParseFloatWithNegativeExponentReturnsDouble) {
  const char* str = "+041524e-2";
  char* endptr;
  ConversionResult result;
  double value = parseFloat(str, &endptr, &result);
  EXPECT_EQ(endptr - str, 10);
  EXPECT_EQ(result, ConversionResult::kSuccess);
  EXPECT_EQ(value, std::strtod("0x1.9f3d70a3d70a4p+8", nullptr));
}

TEST_F(FloatConversionTest, ParseFloatWithNanReturnsDouble) {
  const char* str = "NaN";
  char* endptr;
  ConversionResult result;
  double value = parseFloat(str, &endptr, &result);
  EXPECT_EQ(endptr - str, 3);
  EXPECT_EQ(result, ConversionResult::kSuccess);
  EXPECT_TRUE(std::isnan(value));
}

TEST_F(FloatConversionTest, ParseFloatWithInfReturnsDouble) {
  const char* str = "InfABC";
  char* endptr;
  ConversionResult result;
  double value = parseFloat(str, &endptr, &result);
  EXPECT_EQ(endptr - str, 3);
  EXPECT_EQ(result, ConversionResult::kSuccess);
  EXPECT_TRUE(std::isinf(value));
}

TEST_F(FloatConversionTest, ParseFloatWithInfinityReturnsDouble) {
  const char* str = "-iNfInItY!";
  char* endptr;
  ConversionResult result;
  double value = parseFloat(str, &endptr, &result);
  EXPECT_EQ(endptr - str, 9);
  EXPECT_EQ(result, ConversionResult::kSuccess);
  EXPECT_TRUE(value < 0);
  EXPECT_TRUE(std::isinf(value));
}

}  // namespace testing
}  // namespace py
