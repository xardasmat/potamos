#pragma once

#include <numeric>

extern "C" {
#include <libavutil/rational.h>
}

namespace potamos {

template <typename T>
class Rational {
 public:
  Rational(T num, T den)
      : num_(num / std::gcd(num, den)), den_(den / std::gcd(num, den)) {}
  Rational(const AVRational& rational) : Rational(rational.num, rational.den) {}
  Rational operator+(Rational b) const {
    const Rational& a = *this;
    auto den = std::lcm(a.den_, b.den_);
    return Rational(a.num_ * (den / a.den_) + b.num_ * (den / b.den_), den);
  }
  Rational operator-(Rational b) const {
    const Rational& a = *this;
    auto den = std::lcm(a.den_, b.den_);
    return Rational(a.num_ * (den / a.den_) - b.num_ * (den / b.den_), den);
  }
  Rational operator*(Rational b) const {
    const Rational& a = *this;
    return Rational(a.num_ * b.num_, a.den_ * b.den_);
  }
  Rational operator/(Rational b) const {
    const Rational& a = *this;
    return Rational(a.num_ * b.den_, a.den_ * b.num_);
  }

  bool operator==(Rational b) const {
    const Rational& a = *this;
    return a.den_ == b.den_ && a.num_ == b.num_;
  }

  operator double() const { return double(num_) / den_; }

 private:
  T num_;
  T den_;
};

}  // namespace potamos