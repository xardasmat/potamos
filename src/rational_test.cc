#include "rational.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

extern "C" {
#include <libavutil/rational.h>
}

namespace potamos {
TEST(RationalTest, RationalToDouble) {
  Rational<int> a(1, 3);
  EXPECT_EQ(double(a), 1.0 / 3.0);
}

TEST(RationalTest, FromAVRational) {
  Rational<int> a(av_make_q(1, 3));
  EXPECT_EQ(a, Rational(1, 3));
}

TEST(RationalTest, Sum) {
  Rational<int> a(1, 3);
  Rational<int> b(1, 2);
  EXPECT_EQ(a + b, Rational(5, 6));
}

TEST(RationalTest, Sub) {
  Rational<int> a(1, 3);
  Rational<int> b(1, 2);
  EXPECT_EQ(b - a, Rational(1, 6));
}

TEST(RationalTest, Mul) {
  Rational<int> a(2, 3);
  Rational<int> b(1, 2);
  EXPECT_EQ(a * b, Rational(2, 6));
}

TEST(RationalTest, Div) {
  Rational<int> a(2, 3);
  Rational<int> b(1, 2);
  EXPECT_EQ(a / b, Rational(4, 3));
}

}  // namespace potamos