#include "gtest/gtest.h"

#include "capi-fixture.h"
#include "capi-testing.h"
#include "runtime.h"

namespace python {

TEST(FloatObject, FromDoubleReturnsFloat) {
  Runtime runtime;
  HandleScope scope;
  const double val = 15.4;
  PyObject* flt = PyFloat_FromDouble(val);
  Handle<Object> flt_obj(&scope, ApiHandle::fromPyObject(flt)->asObject());
  ASSERT_TRUE(flt_obj->isFloat());
  EXPECT_EQ(Float::cast(*flt_obj)->value(), val);
}

TEST(FloatObject, NegativeFromDoubleReturnsFloat) {
  Runtime runtime;
  HandleScope scope;
  const double val = -10000.123;
  PyObject* flt = PyFloat_FromDouble(val);
  Handle<Object> flt_obj(&scope, ApiHandle::fromPyObject(flt)->asObject());
  ASSERT_TRUE(flt_obj->isFloat());
  EXPECT_EQ(Float::cast(*flt_obj)->value(), val);
}

}  // namespace python
