#include "gtest/gtest.h"

#include "Python.h"
#include "runtime.h"
#include "test-utils.h"

#include <cmath>
#include <limits>

namespace python {

TEST(FloatObject, FromDoubleReturnsFloat) {
  Runtime runtime;
  HandleScope scope;
  const double val = 15.4;
  PyObject* flt = PyFloat_FromDouble(val);
  Handle<Object> flt_obj(&scope, ApiHandle::fromPyObject(flt)->asObject());
  ASSERT_TRUE(flt_obj->isDouble());
  EXPECT_DOUBLE_EQ(Double::cast(*flt_obj)->value(), val);
}

TEST(FloatObject, NegativeFromDoubleReturnsFloat) {
  Runtime runtime;
  HandleScope scope;
  const double val = -10000.123;
  PyObject* flt = PyFloat_FromDouble(val);
  Handle<Object> flt_obj(&scope, ApiHandle::fromPyObject(flt)->asObject());
  ASSERT_TRUE(flt_obj->isDouble());
  EXPECT_DOUBLE_EQ(Double::cast(*flt_obj)->value(), val);
}

}  // namespace python
