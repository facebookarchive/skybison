#include "gtest/gtest.h"

#include "handles.h"
#include "int-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(IntBuiltinsTest, CompareSmallIntegerEq) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCString(R"(
a = 1
b = 2
a_eq_b = a == b
a_eq_a = a == a
b_eq_b = b == b
)");

  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> a_eq_b(&scope, moduleAt(&runtime, main, "a_eq_b"));
  EXPECT_EQ(*a_eq_b, Boolean::falseObj());
  Handle<Object> a_eq_a(&scope, moduleAt(&runtime, main, "a_eq_a"));
  EXPECT_EQ(*a_eq_a, Boolean::trueObj());
  Handle<Object> b_eq_b(&scope, moduleAt(&runtime, main, "b_eq_b"));
  EXPECT_EQ(*b_eq_b, Boolean::trueObj());
}

TEST(IntBuiltinsTest, CompareSmallIntegerGe) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCString(R"(
a = 1
b = 2
a_ge_a = a >= a
a_ge_b = a >= b
b_ge_a = b >= a
b_ge_b = b >= b
)");

  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> a_ge_a(&scope, moduleAt(&runtime, main, "a_ge_a"));
  EXPECT_EQ(*a_ge_a, Boolean::trueObj());
  Handle<Object> a_ge_b(&scope, moduleAt(&runtime, main, "a_ge_b"));
  EXPECT_EQ(*a_ge_b, Boolean::falseObj());
  Handle<Object> b_ge_a(&scope, moduleAt(&runtime, main, "b_ge_a"));
  EXPECT_EQ(*b_ge_a, Boolean::trueObj());
  Handle<Object> b_ge_b(&scope, moduleAt(&runtime, main, "b_ge_b"));
  EXPECT_EQ(*b_ge_b, Boolean::trueObj());
}

TEST(IntBuiltinsTest, CompareSmallIntegerGt) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCString(R"(
a = 1
b = 2
a_gt_a = a > a
a_gt_b = a > b
b_gt_a = b > a
b_gt_b = b > b
)");

  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> a_gt_a(&scope, moduleAt(&runtime, main, "a_gt_a"));
  EXPECT_EQ(*a_gt_a, Boolean::falseObj());
  Handle<Object> a_gt_b(&scope, moduleAt(&runtime, main, "a_gt_b"));
  EXPECT_EQ(*a_gt_b, Boolean::falseObj());
  Handle<Object> b_gt_a(&scope, moduleAt(&runtime, main, "b_gt_a"));
  EXPECT_EQ(*b_gt_a, Boolean::trueObj());
  Handle<Object> b_gt_b(&scope, moduleAt(&runtime, main, "b_gt_b"));
  EXPECT_EQ(*b_gt_b, Boolean::falseObj());
}

TEST(IntBuiltinsTest, CompareSmallIntegerLe) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCString(R"(
a = 1
b = 2
a_le_a = a <= a
a_le_b = a <= b
b_le_a = b <= a
b_le_b = b <= b
)");

  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> a_le_a(&scope, moduleAt(&runtime, main, "a_le_a"));
  EXPECT_EQ(*a_le_a, Boolean::trueObj());
  Handle<Object> a_le_b(&scope, moduleAt(&runtime, main, "a_le_b"));
  EXPECT_EQ(*a_le_b, Boolean::trueObj());
  Handle<Object> b_le_a(&scope, moduleAt(&runtime, main, "b_le_a"));
  EXPECT_EQ(*b_le_a, Boolean::falseObj());
  Handle<Object> b_le_b(&scope, moduleAt(&runtime, main, "b_le_b"));
  EXPECT_EQ(*b_le_b, Boolean::trueObj());
}

TEST(IntBuiltinsTest, CompareSmallIntegerLt) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCString(R"(
a = 1
b = 2
a_lt_a = a < a
a_lt_b = a < b
b_lt_a = b < a
b_lt_b = b < b
)");

  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> a_lt_a(&scope, moduleAt(&runtime, main, "a_lt_a"));
  EXPECT_EQ(*a_lt_a, Boolean::falseObj());
  Handle<Object> a_lt_b(&scope, moduleAt(&runtime, main, "a_lt_b"));
  EXPECT_EQ(*a_lt_b, Boolean::trueObj());
  Handle<Object> b_lt_a(&scope, moduleAt(&runtime, main, "b_lt_a"));
  EXPECT_EQ(*b_lt_a, Boolean::falseObj());
  Handle<Object> b_lt_b(&scope, moduleAt(&runtime, main, "b_lt_b"));
  EXPECT_EQ(*b_lt_b, Boolean::falseObj());
}

TEST(IntBuiltinsTest, CompareSmallIntegerNe) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCString(R"(
a = 1
b = 2
a_ne_b = a != b
a_ne_a = a != a
b_ne_b = b != b
)");

  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> a_ne_b(&scope, moduleAt(&runtime, main, "a_ne_b"));
  EXPECT_EQ(*a_ne_b, Boolean::trueObj());
  Handle<Object> a_ne_a(&scope, moduleAt(&runtime, main, "a_ne_a"));
  EXPECT_EQ(*a_ne_a, Boolean::falseObj());
  Handle<Object> b_ne_b(&scope, moduleAt(&runtime, main, "b_ne_b"));
  EXPECT_EQ(*b_ne_b, Boolean::falseObj());
}

TEST(IntBuiltinsTest, CompareOpSmallInteger) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCString(R"(
a = 1
b = 2
c = 1
a_lt_b = a < b
a_le_b = a <= b
a_eq_b = a == b
a_ge_b = a >= b
a_gt_b = a > b
a_is_c = a is c
a_is_not_c = a is not c
)");

  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> a_lt_b(&scope, moduleAt(&runtime, main, "a_lt_b"));
  EXPECT_EQ(*a_lt_b, Boolean::trueObj());
  Handle<Object> a_le_b(&scope, moduleAt(&runtime, main, "a_le_b"));
  EXPECT_EQ(*a_le_b, Boolean::trueObj());
  Handle<Object> a_eq_b(&scope, moduleAt(&runtime, main, "a_eq_b"));
  EXPECT_EQ(*a_eq_b, Boolean::falseObj());
  Handle<Object> a_ge_b(&scope, moduleAt(&runtime, main, "a_ge_b"));
  EXPECT_EQ(*a_ge_b, Boolean::falseObj());
  Handle<Object> a_gt_b(&scope, moduleAt(&runtime, main, "a_gt_b"));
  EXPECT_EQ(*a_gt_b, Boolean::falseObj());
  Handle<Object> a_is_c(&scope, moduleAt(&runtime, main, "a_is_c"));
  EXPECT_EQ(*a_is_c, Boolean::trueObj());
  Handle<Object> a_is_not_c(&scope, moduleAt(&runtime, main, "a_is_not_c"));
  EXPECT_EQ(*a_is_not_c, Boolean::falseObj());
}

TEST(IntBuiltinsTest, UnaryInvertSmallInteger) {
  Runtime runtime;
  HandleScope scope;

  const char* src = R"(
pos = 123
invert_pos = ~pos
neg = -456
invert_neg = ~neg
)";

  runtime.runFromCString(src);

  Handle<Module> main(&scope, findModule(&runtime, "__main__"));

  Handle<Object> invert_pos(&scope, moduleAt(&runtime, main, "invert_pos"));
  ASSERT_TRUE(invert_pos->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(*invert_pos)->value(), -124);

  Handle<Object> invert_neg(&scope, moduleAt(&runtime, main, "invert_neg"));
  ASSERT_TRUE(invert_neg->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(*invert_neg)->value(), 455);
}

TEST(IntBuiltinsTest, UnaryPositiveSmallInteger) {
  Runtime runtime;
  HandleScope scope;

  const char* src = R"(
pos = 123
plus_pos = +pos
neg = -123
plus_neg = +neg
)";

  runtime.runFromCString(src);

  Handle<Module> main(&scope, findModule(&runtime, "__main__"));

  Handle<Object> plus_pos(&scope, moduleAt(&runtime, main, "plus_pos"));
  ASSERT_TRUE(plus_pos->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(*plus_pos)->value(), 123);

  Handle<Object> plus_neg(&scope, moduleAt(&runtime, main, "plus_neg"));
  ASSERT_TRUE(plus_neg->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(*plus_neg)->value(), -123);
}

TEST(IntBuiltinsTest, UnaryNegateSmallInteger) {
  Runtime runtime;
  HandleScope scope;

  const char* src = R"(
pos = 123
minus_pos = -pos
neg = -123
minus_neg = -neg
)";

  runtime.runFromCString(src);

  Handle<Module> main(&scope, findModule(&runtime, "__main__"));

  Handle<Object> minus_pos(&scope, moduleAt(&runtime, main, "minus_pos"));
  ASSERT_TRUE(minus_pos->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(*minus_pos)->value(), -123);

  Handle<Object> minus_neg(&scope, moduleAt(&runtime, main, "minus_neg"));
  ASSERT_TRUE(minus_neg->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(*minus_neg)->value(), 123);
}

TEST(IntBuiltinsTest, TruthyIntPos) {
  const char* src = R"(
if 1:
  print("foo")
else:
  print("bar")
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "foo\n");
}

TEST(IntBuiltinsTest, TruthyIntNeg) {
  const char* src = R"(
if 0:
  print("foo")
else:
  print("bar")
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "bar\n");
}

TEST(IntBuiltinsTest, BinaryOps) {
  Runtime runtime;
  HandleScope scope;

  const char* src = R"(
a = 2
b = 3
c = 6
d = 7
print('a & b ==', a & b)
print('a ^ b ==', a ^ b)
print('a + b ==', a + b)

print('c // b ==', c // b)
print('d // b ==', d // b)

print('d % a ==', d % a)
print('d % b ==', d % b)

print('d * b ==', d * b)
print('c * b ==', c * b)

print('c - b ==', c - b)
print('b - c ==', b - c)

print('d * 0 ==', d * 0)
print('0 * d ==', 0 * d)
)";

  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, R"(a & b == 2
a ^ b == 1
a + b == 5
c // b == 2
d // b == 2
d % a == 1
d % b == 1
d * b == 21
c * b == 18
c - b == 3
b - c == -3
d * 0 == 0
0 * d == 0
)");
}

TEST(IntBuiltinsTest, BinaryOverflowCheck) {
  Runtime runtime;
  HandleScope scope;

  const char* mul_src = R"(
a = 268435456
a = a * a * a
)";
  (void)mul_src;

  // Overflows in the multiplication itself.
  EXPECT_DEBUG_ONLY_DEATH(runtime.runFromCString(mul_src),
                          "small integer overflow");

  const char* add_src = R"(
a = 1048576
a *= 2048
a = a * a

a += a
)";
  (void)add_src;

  // No overflow per se, but result too large to store in a SmallInteger
  EXPECT_DEBUG_ONLY_DEATH(runtime.runFromCString(add_src),
                          "SmallInteger::isValid");
}

TEST(IntBuiltinsTest, InplaceAdd) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCString(R"(
a = 1
a += 0
b = a
a += 2
)");
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> a(&scope, moduleAt(&runtime, main, "a"));
  Handle<Object> b(&scope, moduleAt(&runtime, main, "b"));
  ASSERT_TRUE(a->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(*a)->value(), 3);
  ASSERT_TRUE(b->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(*b)->value(), 1);
}

TEST(IntBuiltinsTest, InplaceMultiply) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCString(R"(
a = 5
a *= 1
b = a
a *= 2
)");
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> a(&scope, moduleAt(&runtime, main, "a"));
  Handle<Object> b(&scope, moduleAt(&runtime, main, "b"));
  ASSERT_TRUE(a->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(*a)->value(), 10);
  ASSERT_TRUE(b->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(*b)->value(), 5);
}

TEST(IntBuiltinsTest, InplaceFloorDiv) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCString(R"(
a = 5
a //= 1
b = a
a //= 2
)");
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> a(&scope, moduleAt(&runtime, main, "a"));
  Handle<Object> b(&scope, moduleAt(&runtime, main, "b"));
  ASSERT_TRUE(a->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(*a)->value(), 2);
  ASSERT_TRUE(b->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(*b)->value(), 5);
}

TEST(IntBuiltinsTest, InplaceModulo) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCString(R"(
a = 10
a %= 7
b = a
a %= 2
)");
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> a(&scope, moduleAt(&runtime, main, "a"));
  Handle<Object> b(&scope, moduleAt(&runtime, main, "b"));
  ASSERT_TRUE(a->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(*a)->value(), 1);
  ASSERT_TRUE(b->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(*b)->value(), 3);
}

TEST(IntBuiltinsTest, InplaceSub) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCString(R"(
a = 10
a -= 0
b = a
a -= 7
)");
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> a(&scope, moduleAt(&runtime, main, "a"));
  Handle<Object> b(&scope, moduleAt(&runtime, main, "b"));
  ASSERT_TRUE(a->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(*a)->value(), 3);
  ASSERT_TRUE(b->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(*b)->value(), 10);
}

TEST(IntBuiltinsTest, InplaceXor) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCString(R"(
a = 0xFE
a ^= 0
b = a
a ^= 0x03
)");
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> a(&scope, moduleAt(&runtime, main, "a"));
  Handle<Object> b(&scope, moduleAt(&runtime, main, "b"));
  ASSERT_TRUE(a->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(*a)->value(), 0xFD);
  ASSERT_TRUE(b->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(*b)->value(), 0xFE);
}

TEST(IntBuiltinsTest, BitLength) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 1, 0);

  // (0).bit_length() == 0
  frame->setLocal(0, SmallInteger::fromWord(0));
  Handle<Object> bit_length(&scope,
                            builtinSmallIntegerBitLength(thread, frame, 1));
  ASSERT_TRUE(bit_length->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(*bit_length)->value(), 0);

  // (1).bit_length() == 1
  frame->setLocal(0, SmallInteger::fromWord(1));
  Handle<Object> bit_length1(&scope,
                             builtinSmallIntegerBitLength(thread, frame, 1));
  ASSERT_TRUE(bit_length1->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(*bit_length1)->value(), 1);

  // (-1).bit_length() == 1
  frame->setLocal(0, SmallInteger::fromWord(1));
  Handle<Object> bit_length2(&scope,
                             builtinSmallIntegerBitLength(thread, frame, 1));
  ASSERT_TRUE(bit_length2->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(*bit_length2)->value(), 1);

  // (SmallInteger::kMaxValue).bit_length() == 62
  frame->setLocal(0, SmallInteger::fromWord(SmallInteger::kMaxValue));
  Handle<Object> bit_length3(&scope,
                             builtinSmallIntegerBitLength(thread, frame, 1));
  ASSERT_TRUE(bit_length3->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(*bit_length3)->value(), 62);

  // (SmallInteger::kMinValue).bit_length() == 63
  frame->setLocal(0, SmallInteger::fromWord(SmallInteger::kMinValue));
  Handle<Object> bit_length4(&scope,
                             builtinSmallIntegerBitLength(thread, frame, 1));
  ASSERT_TRUE(bit_length4->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(*bit_length4)->value(), 63);
}

}  // namespace python
