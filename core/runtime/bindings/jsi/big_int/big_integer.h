// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
/*
 * defined BigInteger and related operator, such as "+ - * / %" and so on.
 */
#ifndef CORE_RUNTIME_BINDINGS_JSI_BIG_INT_BIG_INTEGER_H_
#define CORE_RUNTIME_BINDINGS_JSI_BIG_INT_BIG_INTEGER_H_

#include <climits>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace lynx {
namespace piper {

using ELEM_TYPE = int32_t;
using PRODUCT_TYPE = int64_t;
static constexpr ELEM_TYPE BASE = 1000000000;
static constexpr ELEM_TYPE UPPER_BOUND = 999999999;
static constexpr ELEM_TYPE DIGIT_COUNT = 9;

inline static div_t my_div(int num, int denom) {
  div_t result;
  result.quot = num / denom;
  result.rem = num - denom * result.quot;
  return result;
}

inline static ldiv_t my_ldiv(long num, long denom) {
  ldiv_t result;
  result.quot = num / denom;
  result.rem = num - denom * result.quot;
  return result;
}

inline static lldiv_t my_lldiv(long long num, long long denom) {
  lldiv_t result;
  result.quot = num / denom;
  result.rem = num - denom * result.quot;
  return result;
}

class BigInteger {
  friend std::ostream& operator<<(std::ostream& s, const BigInteger& n);

 public:
  /* constructors */
  BigInteger();
  BigInteger(const char* c);
  BigInteger(const std::string& s);
  BigInteger(int l);
  BigInteger(long l);
  BigInteger(long long l);
  BigInteger(unsigned int l);
  BigInteger(unsigned long l);
  BigInteger(unsigned long long l);
  BigInteger(const BigInteger& l);

  /* assignment operators */
  const BigInteger& operator=(const char* c);
  const BigInteger& operator=(const std::string& s);
  const BigInteger& operator=(int l);
  const BigInteger& operator=(long l);
  const BigInteger& operator=(long long l);
  const BigInteger& operator=(unsigned int l);
  const BigInteger& operator=(unsigned long l);
  const BigInteger& operator=(unsigned long long l);
  const BigInteger& operator=(const BigInteger& l);

  /* unary decrement operators, used for boundary conditions,  e.g. INT_MIN,
   * LONG_MIN*/
  const BigInteger& operator--();
  BigInteger operator--(int);

  /* operational assignments */
  const BigInteger& operator+=(const BigInteger& rhs);
  const BigInteger& operator-=(const BigInteger& rhs);
  const BigInteger& operator/=(const BigInteger& rhs);

  /* operations */
  BigInteger operator-() const;
  BigInteger operator+(const BigInteger& rhs) const;
  BigInteger operator-(const BigInteger& rhs) const;
  BigInteger operator*(const BigInteger& rhs) const;
  BigInteger operator/(const BigInteger& rhs) const;
  BigInteger operator%(const BigInteger& rhs) const;
  BigInteger operator*(ELEM_TYPE rhs) const;

  /* relational operations */
  bool operator==(const BigInteger& rhs) const;
  bool operator!=(const BigInteger& rhs) const;
  bool operator<(const BigInteger& rhs) const;
  bool operator<=(const BigInteger& rhs) const;
  bool operator>(const BigInteger& rhs) const;
  bool operator>=(const BigInteger& rhs) const;

  /* size in bytes */
  size_t size() const;

  /* string conversion */
  std::string toString() const;

 private:
  static ELEM_TYPE dInR(const BigInteger& R, const BigInteger& D);
  static void multiplyByDigit(ELEM_TYPE factor, std::vector<ELEM_TYPE>& val);

  void correct(bool justCheckLeadingZeros = false, bool hasValidSign = false);
  void fromString(const std::string& s);
  void truncateToBase();
  bool equalizeSigns();
  void removeLeadingZeros();

  bool pos;                    // true if number is positive
  std::vector<ELEM_TYPE> val;  // number with base FACTOR
};

inline BigInteger::BigInteger() : pos(true) { val.push_back((ELEM_TYPE)0); }

inline BigInteger::BigInteger(const char* c) { fromString(c); }

inline BigInteger::BigInteger(const std::string& s) { fromString(s); }

inline BigInteger::BigInteger(int l) : pos(l >= 0) {
  bool subtractOne = false;
  if (l == INT_MIN) {
    subtractOne = true;
    ++l;
  }

  if (!pos) {
    l = -l;
  }
  do {
    div_t dt = my_div(l, BASE);
    val.push_back((ELEM_TYPE)dt.rem);
    l = dt.quot;
  } while (l > 0);

  if (subtractOne) {
    --*this;
  }
}

inline BigInteger::BigInteger(long l) : pos(l >= 0) {
  bool subtractOne = false;
  if (l == LONG_MIN) {
    subtractOne = true;
    ++l;
  }

  if (!pos) {
    l = -l;
  }
  do {
    ldiv_t dt = my_ldiv(l, BASE);
    val.push_back((ELEM_TYPE)dt.rem);
    l = dt.quot;
  } while (l > 0);

  if (subtractOne) {
    --*this;
  }
}

inline BigInteger::BigInteger(long long l) : pos(l >= 0) {
  bool subtractOne = false;
  if (l == LLONG_MIN) {
    subtractOne = true;
    ++l;
  }

  if (!pos) {
    l = -l;
  }
  do {
    lldiv_t dt = my_lldiv(l, BASE);
    val.push_back((ELEM_TYPE)dt.rem);
    l = dt.quot;
  } while (l > 0);

  if (subtractOne) {
    --*this;
  }
}

inline BigInteger::BigInteger(unsigned int l) : pos(true) {
  do {
    val.push_back((ELEM_TYPE)(l % BASE));
    l = l / BASE;
  } while (l > 0);
}

inline BigInteger::BigInteger(unsigned long l) : pos(true) {
  do {
    val.push_back((ELEM_TYPE)(l % BASE));
    l = l / BASE;
  } while (l > 0);
}

inline BigInteger::BigInteger(unsigned long long l) : pos(true) {
  do {
    val.push_back((ELEM_TYPE)(l % BASE));
    l = l / BASE;
  } while (l > 0);
}

inline BigInteger::BigInteger(const BigInteger& l) : pos(l.pos), val(l.val) {}

inline const BigInteger& BigInteger::operator=(const char* c) {
  fromString(c);
  return *this;
}

inline const BigInteger& BigInteger::operator=(const std::string& s) {
  fromString(s);
  return *this;
}

inline const BigInteger& BigInteger::operator=(int l) {
  bool subtractOne = false;
  if (l == INT_MIN) {
    subtractOne = true;
    ++l;
  }

  pos = l >= 0;
  val.clear();
  if (!pos) {
    l = -l;
  }
  do {
    div_t dt = my_div(l, BASE);
    val.push_back((ELEM_TYPE)dt.rem);
    l = dt.quot;
  } while (l > 0);

  return subtractOne ? --*this : *this;
}

inline const BigInteger& BigInteger::operator=(long l) {
  bool subtractOne = false;
  if (l == LONG_MIN) {
    subtractOne = true;
    ++l;
  }

  pos = l >= 0;
  val.clear();
  if (!pos) {
    l = -l;
  }
  do {
    ldiv_t dt = my_ldiv(l, BASE);
    val.push_back((ELEM_TYPE)dt.rem);
    l = dt.quot;
  } while (l > 0);

  return subtractOne ? --*this : *this;
}

inline const BigInteger& BigInteger::operator=(long long l) {
  bool subtractOne = false;
  if (l == LLONG_MIN) {
    subtractOne = true;
    ++l;
  }

  pos = l >= 0;
  val.clear();
  if (!pos) {
    l = -l;
  }
  do {
    lldiv_t dt = my_lldiv(l, BASE);
    val.push_back((ELEM_TYPE)dt.rem);
    l = dt.quot;
  } while (l > 0);

  return subtractOne ? --*this : *this;
}

inline const BigInteger& BigInteger::operator=(unsigned int l) {
  pos = true;
  val.clear();
  do {
    val.push_back((ELEM_TYPE)(l % BASE));
    l = l / BASE;
  } while (l > 0);
  return *this;
}

inline const BigInteger& BigInteger::operator=(unsigned long l) {
  pos = true;
  val.clear();
  do {
    val.push_back((ELEM_TYPE)(l % BASE));
    l = l / BASE;
  } while (l > 0);
  return *this;
}

inline const BigInteger& BigInteger::operator=(unsigned long long l) {
  pos = true;
  val.clear();
  do {
    val.push_back((ELEM_TYPE)(l % BASE));
    l = l / BASE;
  } while (l > 0);
  return *this;
}

inline const BigInteger& BigInteger::operator=(const BigInteger& l) {
  pos = l.pos;
  val = l.val;
  return *this;
}

inline const BigInteger& BigInteger::operator--() {
  val[0] -= (pos ? 1 : -1);
  this->correct(false, true);
  return *this;
}

inline BigInteger BigInteger::operator--(int) {
  BigInteger result = *this;
  val[0] -= (pos ? 1 : -1);
  this->correct(false, true);
  return result;
}

inline const BigInteger& BigInteger::operator+=(const BigInteger& rhs) {
  if (rhs.val.size() > val.size()) {
    val.resize(rhs.val.size(), 0);
  }
  for (size_t i = 0; i < val.size(); ++i) {
    val[i] = (pos ? val[i] : -val[i]) +
             (i < rhs.val.size() ? (rhs.pos ? rhs.val[i] : -rhs.val[i]) : 0);
  }
  correct();
  return *this;
}

inline const BigInteger& BigInteger::operator-=(const BigInteger& rhs) {
  if (rhs.val.size() > val.size()) {
    val.resize(rhs.val.size(), 0);
  }
  for (size_t i = 0; i < val.size(); ++i) {
    val[i] = (pos ? val[i] : -val[i]) -
             (i < rhs.val.size() ? (rhs.pos ? rhs.val[i] : -rhs.val[i]) : 0);
  }
  correct();
  return *this;
}

inline const BigInteger& BigInteger::operator/=(const BigInteger& rhs) {
  if (rhs == 0) {
    std::cerr << "Division by zero!" << std::endl;
    return *this;
  }
  BigInteger R, D = (rhs.pos ? rhs : -rhs), N = (pos ? *this : -*this);
  bool old_position = pos;
  std::fill(val.begin(), val.end(), 0);
  for (int i = (int)N.val.size() - 1; i >= 0; --i) {
    R.val.insert(R.val.begin(), N.val[i]);
    R.correct(true);
    ELEM_TYPE cnt = dInR(R, D);
    R -= D * cnt;
    val[i] += cnt;
  }
  correct();
  pos = (val.size() == 1 && val[0] == 0) ? true : (old_position == rhs.pos);
  return *this;
}

inline BigInteger BigInteger::operator-() const {
  BigInteger result = *this;
  result.pos = !pos;
  return result;
}

inline BigInteger BigInteger::operator+(const BigInteger& rhs) const {
  BigInteger result;
  result.val.resize(val.size() > rhs.val.size() ? val.size() : rhs.val.size(),
                    0);
  for (size_t i = 0; i < val.size() || i < rhs.val.size(); ++i) {
    result.val[i] =
        (i < val.size() ? (pos ? val[i] : -val[i]) : 0) +
        (i < rhs.val.size() ? (rhs.pos ? rhs.val[i] : -rhs.val[i]) : 0);
  }
  result.correct();
  return result;
}

inline BigInteger BigInteger::operator-(const BigInteger& rhs) const {
  BigInteger result;
  result.val.resize(val.size() > rhs.val.size() ? val.size() : rhs.val.size(),
                    0);
  for (size_t i = 0; i < val.size() || i < rhs.val.size(); ++i) {
    result.val[i] =
        (i < val.size() ? (pos ? val[i] : -val[i]) : 0) -
        (i < rhs.val.size() ? (rhs.pos ? rhs.val[i] : -rhs.val[i]) : 0);
  }
  result.correct();
  return result;
}

inline BigInteger BigInteger::operator*(const BigInteger& rhs) const {
  BigInteger result;
  result.val.resize(val.size() + rhs.val.size(), 0);
  PRODUCT_TYPE carry = 0;
  size_t digit = 0;
  for (;; ++digit) {
    lldiv_t dt = my_lldiv(carry, BASE);
    carry = dt.quot;
    result.val[digit] = (ELEM_TYPE)dt.rem;

    bool found = false;
    for (size_t i = digit < rhs.val.size() ? 0 : digit - rhs.val.size() + 1;
         i < val.size() && i <= digit; ++i) {
      PRODUCT_TYPE pval =
          result.val[digit] + val[i] * (PRODUCT_TYPE)rhs.val[digit - i];
      if (pval >= BASE || pval <= -BASE) {
        lldiv_t dt = my_lldiv(pval, BASE);
        carry += dt.quot;
        pval = dt.rem;
      }
      result.val[digit] = (ELEM_TYPE)pval;
      found = true;
    }
    if (!found) {
      break;
    }
  }
  for (; carry > 0; ++digit) {
    lldiv_t dt = my_lldiv(carry, BASE);
    result.val[digit] = (ELEM_TYPE)dt.rem;
    carry = dt.quot;
  }
  result.correct();
  result.pos =
      (result.val.size() == 1 && result.val[0] == 0) ? true : (pos == rhs.pos);
  return result;
}

inline BigInteger BigInteger::operator/(const BigInteger& rhs) const {
  if (rhs == 0) {
    std::cerr << "Division by zero!" << std::endl;
    return 0;
  }
  BigInteger Q, R, D = (rhs.pos ? rhs : -rhs), N = (pos ? *this : -*this);
  Q.val.resize(N.val.size(), 0);
  for (int i = (int)N.val.size() - 1; i >= 0; --i) {
    R.val.insert(R.val.begin(), N.val[i]);
    R.correct(true);
    ELEM_TYPE cnt = dInR(R, D);
    R -= D * cnt;
    Q.val[i] += cnt;
  }
  Q.correct();
  Q.pos = (Q.val.size() == 1 && Q.val[0] == 0) ? true : (pos == rhs.pos);
  return Q;
}

inline BigInteger BigInteger::operator%(const BigInteger& rhs) const {
  if (rhs == 0) {
    std::cerr << "Division by zero!" << std::endl;
    return 0;
  }
  BigInteger R, D = (rhs.pos ? rhs : -rhs), N = (pos ? *this : -*this);
  for (int i = (int)N.val.size() - 1; i >= 0; --i) {
    R.val.insert(R.val.begin(), N.val[i]);
    R.correct(true);
    R -= D * dInR(R, D);
  }
  R.correct();
  R.pos = (R.val.size() == 1 && R.val[0] == 0) ? true : pos;
  return R;
}

inline BigInteger BigInteger::operator*(ELEM_TYPE rhs) const {
  BigInteger result = *this;
  ELEM_TYPE factor = rhs < 0 ? -rhs : rhs;
  multiplyByDigit(factor, result.val);
  result.correct();
  result.pos = (result.val.size() == 1 && result.val[0] == 0)
                   ? true
                   : (pos == (rhs >= 0));
  return result;
}

inline bool BigInteger::operator==(const BigInteger& rhs) const {
  if (pos != rhs.pos || val.size() != rhs.val.size()) {
    return false;
  }
  for (int i = (int)val.size() - 1; i >= 0; --i) {
    if (val[i] != rhs.val[i]) {
      return false;
    }
  }
  return true;
}

inline bool BigInteger::operator!=(const BigInteger& rhs) const {
  if (pos != rhs.pos || val.size() != rhs.val.size()) {
    return true;
  }
  for (int i = (int)val.size() - 1; i >= 0; --i) {
    if (val[i] != rhs.val[i]) {
      return true;
    }
  }
  return false;
}

inline bool BigInteger::operator<(const BigInteger& rhs) const {
  if (pos && !rhs.pos) {
    return false;
  }
  if (!pos && rhs.pos) {
    return true;
  }
  if (val.size() > rhs.val.size()) {
    return pos ? false : true;
  }
  if (val.size() < rhs.val.size()) {
    return pos ? true : false;
  }
  for (int i = (int)val.size() - 1; i >= 0; --i) {
    if (val[i] < rhs.val[i]) {
      return pos ? true : false;
    }
    if (val[i] > rhs.val[i]) {
      return pos ? false : true;
    }
  }
  return false;
}

inline bool BigInteger::operator<=(const BigInteger& rhs) const {
  if (pos && !rhs.pos) {
    return false;
  }
  if (!pos && rhs.pos) {
    return true;
  }
  if (val.size() > rhs.val.size()) {
    return pos ? false : true;
  }
  if (val.size() < rhs.val.size()) {
    return pos ? true : false;
  }
  for (int i = (int)val.size() - 1; i >= 0; --i) {
    if (val[i] < rhs.val[i]) {
      return pos ? true : false;
    }
    if (val[i] > rhs.val[i]) {
      return pos ? false : true;
    }
  }
  return true;
}

inline bool BigInteger::operator>(const BigInteger& rhs) const {
  if (pos && !rhs.pos) {
    return true;
  }
  if (!pos && rhs.pos) {
    return false;
  }
  if (val.size() > rhs.val.size()) {
    return pos ? true : false;
  }
  if (val.size() < rhs.val.size()) {
    return pos ? false : true;
  }
  for (int i = (int)val.size() - 1; i >= 0; --i) {
    if (val[i] < rhs.val[i]) {
      return pos ? false : true;
    }
    if (val[i] > rhs.val[i]) {
      return pos ? true : false;
    }
  }
  return false;
}

inline bool BigInteger::operator>=(const BigInteger& rhs) const {
  if (pos && !rhs.pos) {
    return true;
  }
  if (!pos && rhs.pos) {
    return false;
  }
  if (val.size() > rhs.val.size()) {
    return pos ? true : false;
  }
  if (val.size() < rhs.val.size()) {
    return pos ? false : true;
  }
  for (int i = (int)val.size() - 1; i >= 0; --i) {
    if (val[i] < rhs.val[i]) {
      return pos ? false : true;
    }
    if (val[i] > rhs.val[i]) {
      return pos ? true : false;
    }
  }
  return true;
}

inline std::string BigInteger::toString() const {
  std::ostringstream oss;
  oss << *this;
  return oss.str();
}

inline size_t BigInteger::size() const {
  return val.size() * sizeof(ELEM_TYPE) + sizeof(bool);
}

inline void BigInteger::truncateToBase() {
  for (size_t i = 0; i < val.size(); ++i)  // truncate each
  {
    if (val[i] >= BASE || val[i] <= -BASE) {
      div_t dt = my_div(val[i], BASE);
      val[i] = dt.rem;
      if (i + 1 >= val.size()) {
        val.push_back(dt.quot);
      } else {
        val[i + 1] += dt.quot;
      }
    }
  }
}

inline bool BigInteger::equalizeSigns() {
  bool isPositive = true;
  int i = (int)((val.size())) - 1;
  for (; i >= 0; --i) {
    if (val[i] != 0) {
      isPositive = val[i--] > 0;
      break;
    }
  }

  if (isPositive) {
    for (; i >= 0; --i) {
      if (val[i] < 0) {
        int k = 0, index = i + 1;
        while (static_cast<size_t>(index) < val.size() && val[index] == 0) {
          ++k;
          ++index;
        }
        // count adjacent zeros on left. number on the left is positive
        val[index] -= 1;
        val[i] += BASE;
        for (; k > 0; --k) {
          val[i + k] = UPPER_BOUND;
        }
      }
    }
  } else {
    for (; i >= 0; --i) {
      if (val[i] > 0) {
        int k = 0, index = i + 1;
        while (static_cast<size_t>(index) < val.size() && val[index] == 0) {
          ++k;
          ++index;
        }
        // count adjacent zeros on right. number on the left is negative
        val[index] += 1;
        val[i] -= BASE;
        for (; k > 0; --k) {
          val[i + k] = -UPPER_BOUND;
        }
      }
    }
  }

  return isPositive;
}

inline void BigInteger::removeLeadingZeros() {
  for (int i = static_cast<int>(val.size()) - 1; i > 0; --i) {
    if (val[i] != 0) {
      return;
    } else {
      val.erase(val.begin() + i);
    }
  }
}

inline void BigInteger::correct(bool justCheckLeadingZeros, bool hasValidSign) {
  if (!justCheckLeadingZeros) {
    truncateToBase();

    if (equalizeSigns()) {
      pos = ((val.size() == 1 && val[0] == 0) || !hasValidSign) ? true : pos;
    } else {
      pos = hasValidSign ? !pos : false;
      for (size_t i = 0; i < val.size(); ++i) {
        val[i] = abs(val[i]);
      }
    }
  }

  removeLeadingZeros();
}

inline void BigInteger::fromString(const std::string& s) {
  pos = true;
  val.clear();
  val.reserve(s.size() / DIGIT_COUNT + 1);
  int i = (int)s.size() - DIGIT_COUNT;
  for (; i >= 0; i -= DIGIT_COUNT) {
    val.push_back(atoi(s.substr(i, DIGIT_COUNT).c_str()));
  }
  if (i > -DIGIT_COUNT) {
    std::string ss = s.substr(0, i + DIGIT_COUNT);
    if (ss.size() == 1 && ss[0] == '-') {
      pos = false;
    } else {
      val.push_back(atoi(ss.c_str()));
    }
  }
  if (val.back() < 0) {
    val.back() = -val.back();
    pos = false;
  }
  correct(true);
}

inline ELEM_TYPE BigInteger::dInR(const BigInteger& R, const BigInteger& D) {
  ELEM_TYPE min = 0, max = UPPER_BOUND;
  while (max > min) {
    ELEM_TYPE avg = max + min;
    div_t dt = my_div(avg, 2);
    avg = dt.rem ? (dt.quot + 1) : dt.quot;
    BigInteger prod = D * avg;
    if (R == prod) {
      return avg;
    } else if (R > prod) {
      min = avg;
    } else {
      max = avg - 1;
    }
  }
  return min;
}

inline void BigInteger::multiplyByDigit(ELEM_TYPE factor,
                                        std::vector<ELEM_TYPE>& val) {
  ELEM_TYPE carry = 0;
  for (size_t i = 0; i < val.size(); ++i) {
    PRODUCT_TYPE pval = val[i] * (PRODUCT_TYPE)factor + carry;
    if (pval >= BASE || pval <= -BASE) {
      lldiv_t dt = my_lldiv(pval, BASE);
      carry = (ELEM_TYPE)dt.quot;
      pval = dt.rem;
    } else {
      carry = 0;
    }
    val[i] = (ELEM_TYPE)pval;
  }
  if (carry > 0) {
    val.push_back(carry);
  }
}

inline std::ostream& operator<<(std::ostream& s, const BigInteger& n) {
  if (!n.pos) {
    s << '-';
  }
  bool first = true;
  for (int i = (int)n.val.size() - 1; i >= 0; --i) {
    if (first) {
      s << n.val[i];
      first = false;
    } else {
      s << std::setfill('0') << std::setw(DIGIT_COUNT) << n.val[i];
    }
  }
  return s;
}

}  // namespace piper
}  // namespace lynx
#endif  // CORE_RUNTIME_BINDINGS_JSI_BIG_INT_BIG_INTEGER_H_
