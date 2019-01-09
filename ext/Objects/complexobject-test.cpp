#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

using namespace testing;

using ComplexExtensionApiTest = ExtensionApi;

TEST_F(ComplexExtensionApiTest, PyComplexFromDoublesReturnsComplex) {
  PyObjectPtr cmp(PyComplex_FromDoubles(0.0, 0.0));
  EXPECT_TRUE(PyComplex_CheckExact(cmp));
}

TEST_F(ComplexExtensionApiTest, PyComplexFromCComplexReturnsComplex) {
  Py_complex c = {1.0, 0.0};
  PyObjectPtr cmp(PyComplex_FromCComplex(c));
  EXPECT_TRUE(PyComplex_CheckExact(cmp));
}

}  // namespace python
