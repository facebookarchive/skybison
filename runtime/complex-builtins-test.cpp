#include "gtest/gtest.h"

#include "complex-builtins.h"
#include "frame.h"
#include "object-builtins.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(ComplexBuiltinsTest, NewWithNoArgsReturnsZero) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Frame* frame = thread->openAndLinkFrame(0, 1, 0);
  frame->setLocal(0, runtime.typeAt(LayoutId::kComplex));
  Complex cmplx(&scope, ComplexBuiltins::dunderNew(thread, frame, 1));
  EXPECT_EQ(cmplx->real(), 0);
  EXPECT_EQ(cmplx->imag(), 0);
}

TEST(ComplexBuiltinsTest, NewWithOneNumberArgReturnsComplexWithReal) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);
  frame->setLocal(0, runtime.typeAt(LayoutId::kComplex));
  frame->setLocal(1, runtime.newInt(1));
  Complex cmplx(&scope, ComplexBuiltins::dunderNew(thread, frame, 2));
  EXPECT_EQ(cmplx->real(), 1.0);
  EXPECT_EQ(cmplx->imag(), 0);
}

TEST(ComplexBuiltinsTest, NewWithTwoNumberArgReturnsComplexWithReal) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Frame* frame = thread->openAndLinkFrame(0, 3, 0);
  frame->setLocal(0, runtime.typeAt(LayoutId::kComplex));
  frame->setLocal(1, runtime.newInt(1));
  frame->setLocal(2, runtime.newInt(2));
  Complex cmplx(&scope, ComplexBuiltins::dunderNew(thread, frame, 3));
  EXPECT_EQ(cmplx->real(), 1.0);
  EXPECT_EQ(cmplx->imag(), 2.0);
}

TEST(ComplexBuiltinsTest, NewWithComplexArgReturnsSameComplex) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);
  frame->setLocal(0, runtime.typeAt(LayoutId::kComplex));
  frame->setLocal(1, runtime.newComplex(1.0, 2.0));
  Complex cmplx(&scope, ComplexBuiltins::dunderNew(thread, frame, 2));
  EXPECT_EQ(cmplx->real(), 1.0);
  EXPECT_EQ(cmplx->imag(), 2.0);
}

}  // namespace python
