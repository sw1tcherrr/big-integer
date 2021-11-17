#include "big_integer.h"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <functional>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <utility>

using storage_t = std::vector<uint32_t>;

constexpr uint32_t BASE_POW = 32u;
constexpr uint64_t BASE = 1ull << BASE_POW;
constexpr std::array<uint32_t, 10> TEN = {1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000};
const big_integer ZERO;
const big_integer ONE(1);

void big_integer::swap(big_integer& other) {
    using std::swap;
    swap(digits_, other.digits_);
    swap(negative_, other.negative_);
}

void shrink(storage_t &v, bool neg) {
    while (v.size() > 1 && v.back() == neg * UINT32_MAX) {
        v.pop_back();
    }
}

uint64_t big_integer::get_digit(size_t idx) const {
    return idx < digits_.size() ? digits_[idx] : negative_ * UINT32_MAX;
}

uint64_t big_integer::get_inv_digit(size_t idx) const {
    return idx < digits_.size() ? ~digits_[idx] : !negative_ * UINT32_MAX;
}

bool less(storage_t const& v1, storage_t const& v2, size_t offset = 0) {
    // Pre: v1.size() >= v2.size() + offset
    if (v1.size() > v2.size() + offset) {
        return false;
    }

    for (auto i = v1.rbegin(), j = v2.rbegin(); i != v1.rend() - offset && j != v2.rend(); ++i, ++j) {
        if (*i != *j) {
            return *i < *j;
        }
    }
    return false;
}

big_integer& big_integer::offset_sub(big_integer const& b, size_t offset) {
    // Pre:
    // *this > b > 0
    // digits_.size() >= b.digits_size() + offset
    uint64_t carry = 1;
    for (size_t i = offset; i < digits_.size(); ++i) {
        uint64_t s = get_digit(i) + b.get_inv_digit(i - offset) + carry;
        carry = s >> BASE_POW;

        digits_[i] = static_cast<uint32_t>(s);
    }
    shrink(digits_, false);

    return *this;
}

big_integer& big_integer::abs() {
    if (negative_) {
        return negate();
    }
    return *this;
}

big_integer::big_integer() : negative_(false), digits_(1, 0) {}

big_integer::big_integer(bool neg, storage_t&&  dig) : negative_(neg), digits_(std::move(dig)) {}

big_integer::big_integer(long long a) : negative_(a < 0), digits_(2) {
    digits_[0] = static_cast<uint32_t>(a);
    digits_[1] = static_cast<uint32_t>(a >> BASE_POW);
    shrink(digits_, negative_);
}

big_integer::big_integer(unsigned long long a) : negative_(false), digits_(2) {
    digits_[0] = static_cast<uint32_t>(a);
    digits_[1] = static_cast<uint32_t>(a >> BASE_POW);
    shrink(digits_, false);
}

big_integer::big_integer(int a)
    : big_integer(static_cast<long long>(a)) {}
big_integer::big_integer(unsigned a)
    : big_integer(static_cast<unsigned long long>(a)) {}
big_integer::big_integer(long a)
    : big_integer(static_cast<long long>(a)) {}
big_integer::big_integer(unsigned long a)
    : big_integer(static_cast<unsigned long long>(a)) {}

uint64_t strtoull(char const* s, size_t len) {
    uint64_t res = 0;
    for (size_t i = 0; i < len; ++i) {
        res = res * 10 + (s[i] - '0');
    }
    return res;
}

big_integer::big_integer(std::string const& str) : big_integer() {
    if (str.length() == 0) {
        throw std::invalid_argument("NaN string");
    }

    size_t start_i = (str[0] == '-' || str[0] == '+');
    auto start = str.begin() + start_i;
    if (str.end() - start == 0) {
        throw std::invalid_argument("NaN string");
    }

    constexpr size_t step = 9;
    digits_.reserve(str.length() / 9 + 1);
    for (size_t i = start_i; i < str.length(); i += step) {
        size_t len = i + step < str.length() ? step : str.length() - i;
        for (size_t c = i; c < i + len; ++c) {
            if (!('0' <= str[c] && str[c] <= '9')) {
                throw std::invalid_argument("NaN string");
            }
        }

        mul_short(TEN[len]) += strtoull(str.c_str() + i, len);
    }

    shrink(digits_, false);

    if (*this != ZERO && str[0] == '-') {
        negate();
    }
}

big_integer& big_integer::operator+=(big_integer const& rhs) {
    digits_.resize(std::max(digits_.size(), rhs.digits_.size()) + 2, get_digit(digits_.size()));

    uint64_t  carry = 0;
    for (size_t i = 0; i < digits_.size(); ++i) {
        uint64_t s = get_digit(i) + rhs.get_digit(i) + carry;
        carry = s >> BASE_POW;

        digits_[i] = static_cast<uint32_t>(s);
    }
    negative_ = digits_.back() >> (BASE_POW - 1);
    shrink(digits_, negative_);

    return *this;
}

big_integer& big_integer::operator-=(big_integer const& rhs) {
    digits_.resize(std::max(digits_.size(), rhs.digits_.size()) + 2, get_digit(digits_.size()));

    uint64_t  carry = 1;
    for (size_t i = 0; i < digits_.size(); ++i) {
        uint64_t s = get_digit(i) + rhs.get_inv_digit(i) + carry;
        carry = s >> BASE_POW;

        digits_[i] = static_cast<uint32_t>(s);
    }
    negative_ = digits_.back() >> (BASE_POW - 1);
    shrink(digits_, negative_);

    return *this;
}

big_integer& big_integer::mul_short(uint32_t b) {
    // Pre: *this >= 0
    digits_.resize(digits_.size() + 1, 0);

    uint64_t  carry = 0;
    for (size_t i = 0; i < digits_.size(); ++i) {
        uint64_t cur = carry + get_digit(i) * b;
        digits_[i] = (cur & (BASE - 1));
        carry = cur >> BASE_POW;
    }
    shrink(digits_, negative_);
    return *this;
}

big_integer& big_integer::operator*=(big_integer rhs) {
    bool sign = negative_ ^ rhs.negative_;

    abs();
    rhs.abs();

    storage_t new_digits(digits_.size() + rhs.digits_.size(), 0);

    for (size_t i = 0; i < digits_.size(); ++i) {
        uint64_t  carry = 0;
        for (size_t j = 0; j < rhs.digits_.size() || carry; ++j) {
            uint64_t cur = new_digits[i + j] + get_digit(i) * rhs.get_digit(j) + carry;
            new_digits[i + j] = cur & (BASE - 1);
            carry = cur >> BASE_POW;
        }
    }
    std::swap(digits_, new_digits);
    shrink(digits_, false);
    if (sign) {
        negate();
    }
    return *this;
}

uint64_t div21(uint64_t x1, uint64_t x2, uint64_t y1) {
    return ((x1 << BASE_POW) | x2) / y1;
}

uint32_t big_integer::div_short(uint32_t b) {
    // Pre: *this > 0
    uint64_t carry = 0;
    for (auto digit = digits_.rbegin(); digit < digits_.rend(); ++digit) {
        uint64_t cur = *digit + carry * BASE;
        *digit = cur / b;
        carry = cur % b;
    }
    shrink(digits_, negative_);
    return carry;
}

big_integer& big_integer::div_long(big_integer y) {
    bool sign = negative_ ^ y.negative_;
    abs();
    y.abs();

    uint32_t f = BASE / (static_cast<uint64_t >(y.digits_.back()) + 1);
    mul_short(f);
    y.mul_short(f);
    int64_t n = y.digits_.size();
    int64_t m = digits_.size();
    int64_t k =  m - n;
    storage_t q(k + 1);

    for (; k >= 0 && digits_.size() >= n + k; --k) {
        q[k] = std::min(BASE - 1,
                        div21(get_digit(n + k), get_digit(n + k - 1),
                              y.get_digit(n - 1)));

        big_integer qy(y);
        qy.mul_short(q[k]);
        if (less(digits_, qy.digits_, k)) {
            q[k]--;
            qy -= y;
        }
        offset_sub(qy, k);
    }

    shrink(q, false);
    std::swap(digits_, q);
    if (sign) {
        negate();
    }
    return *this;
}

big_integer& big_integer::mod_long(big_integer y) {
    abs();
    y.abs();

    uint32_t f = BASE / (static_cast<uint64_t >(y.digits_.back()) + 1);
    mul_short(f);
    y.mul_short(f);
    int64_t n = y.digits_.size();
    int64_t m = digits_.size();
    int64_t k =  m - n;
    uint32_t q = 0;

    for (; k >= 0 && digits_.size() >= n + k; --k) {
        q = std::min(BASE - 1,
                        div21(get_digit(n + k), get_digit(n + k - 1),
                              y.get_digit(n - 1)));

        big_integer qy(y);
        qy.mul_short(q);
        if (less(digits_, qy.digits_, k)) {
            qy -= y;
        }
        offset_sub(qy, k);
    }

    div_short(f);
    return *this;
}

big_integer& big_integer::operator/=(big_integer const& rhs) {
    if (rhs.digits_.size() == 1) {
        if (rhs != ZERO) {
            bool sign = negative_ ^ rhs.negative_;
            abs();
            div_short(rhs.negative_ ? ~rhs.digits_[0] + 1 : rhs.digits_[0]);
            if (sign) {
                negate();
            }
        } else {
            throw std::invalid_argument("Division by zero");
        }
    } else if (rhs.digits_.size() > digits_.size()) {
        *this = ZERO;
    } else {
        div_long(rhs);
    }
    return *this;
}

big_integer& big_integer::operator%=(big_integer const& rhs) {
    bool sign = negative_;
    if (rhs.digits_.size() == 1) {
        if (rhs != ZERO) {
            abs();
            *this = div_short(rhs.negative_ ? ~rhs.digits_[0] + 1 : rhs.digits_[0]);
            if (sign) {
                negate();
            }
        }else {
            throw std::invalid_argument("Division by zero");
        }
    } else if (rhs.digits_.size() > digits_.size()) {
        // do nothing
    } else {
        mod_long(rhs);
        if (sign) {
            negate();
        }
    }
    return *this;
}

big_integer& big_integer::operator&=(big_integer const& rhs) {
    return bitwise(rhs, [](uint32_t a, uint32_t b) {return a & b;});
}

big_integer& big_integer::operator|=(big_integer const& rhs) {
    return bitwise(rhs, [](uint32_t a, uint32_t b) {return a | b;});
}

big_integer& big_integer::operator^=(big_integer const& rhs) {
    return bitwise(rhs, [](uint32_t a, uint32_t b) {return a ^ b;});
}

big_integer& big_integer::operator<<=(int rhs) {
    if (rhs < 0) {
        return *this >>= (-rhs);
    }

    size_t z_cnt = rhs / BASE_POW;
    digits_.reserve(z_cnt + digits_.size() + 1);
    digits_.insert(digits_.begin(), z_cnt, 0);
    uint32_t carry = 0;
    rhs %= BASE_POW;

    if (rhs != 0) {
        for (auto digit = digits_.begin() + z_cnt; digit < digits_.end(); ++digit) {
            uint32_t sec_half = *digit >> (BASE_POW - rhs);
            *digit = (*digit << rhs) + carry;
            carry = sec_half;
        }
        digits_.push_back((negative_ * (UINT32_MAX << rhs)) + carry);
    }

    shrink(digits_, negative_);
    return *this;
}

big_integer& big_integer::operator>>=(int rhs) {
    if (rhs < 0) {
        return *this <<= (-rhs);
    }

    digits_.erase(digits_.begin(), digits_.begin() + rhs / BASE_POW);
    rhs %= BASE_POW;
    uint32_t carry = negative_ * (static_cast<uint64_t>(UINT32_MAX) << (BASE_POW - rhs));
    if (rhs != 0) {
        for (auto digit = digits_.rbegin(); digit < digits_.rend(); ++digit) {
            uint32_t sec_half = *digit << (BASE_POW - rhs);
            *digit = (*digit >> rhs) + carry;
            carry = sec_half;
        }
    }
    if (digits_.empty()) {
        digits_.push_back(negative_ * UINT32_MAX);
    }
    shrink(digits_, negative_);
    return *this;
}

big_integer big_integer::operator+() const {
    return *this;
}

big_integer& big_integer::negate() {
    if (*this == ZERO) {
        return *this;
    }
    digits_.resize(digits_.size() + 2, get_digit(digits_.size()));

    uint64_t  carry = 1;
    for (size_t i = 0; i < digits_.size(); ++i) {
        uint64_t s = get_inv_digit(i) + carry;

        carry = s >> BASE_POW;
        digits_[i] = static_cast<uint32_t>(s);
    }
    negative_ = !negative_;
    shrink(digits_, negative_);

    return *this;
}

big_integer big_integer::operator-() const {
    return big_integer(*this).negate();
}

big_integer big_integer::operator~() const {
    storage_t new_digits(digits_.size());
    for (size_t i = 0; i < new_digits.size(); ++i) {
        new_digits[i] = ~digits_[i];
    }
    return big_integer(!negative_, std::move(new_digits));
}

big_integer& big_integer::operator++() {
    return *this += ONE;
}

big_integer big_integer::operator++(int) {
    big_integer tmp(*this);
    ++*this;
    return tmp;
}

big_integer& big_integer::operator--() {
    return *this -= ONE;
}

big_integer big_integer::operator--(int) {
    big_integer tmp(*this);
    --*this;
    return tmp;
}

big_integer operator+(big_integer a, big_integer const& b) {
    return a += b;
}

big_integer operator-(big_integer a, big_integer const& b) {
    return a -= b;
}

big_integer operator*(big_integer a, big_integer const& b) {
    return a *= b;
}

big_integer operator/(big_integer a, big_integer const& b) {
    return a /= b;
}

big_integer operator%(big_integer a, big_integer const& b) {
    return a %= b;
}

big_integer& big_integer::bitwise(big_integer const& rhs, const std::function<uint32_t (uint32_t, uint32_t)>&op) {
    digits_.resize(std::max(digits_.size(), rhs.digits_.size()), get_digit(digits_.size()));
    for (size_t i = 0; i < digits_.size(); ++i) {
        digits_[i] = op(get_digit(i), rhs.get_digit(i));
    }

    negative_ = op(negative_, rhs.negative_);
    shrink(digits_, negative_);

    return *this;
}

big_integer operator&(big_integer a, big_integer const& b) {
    return a &= b;
}

big_integer operator|(big_integer a, big_integer const& b) {
    return a |= b;
}

big_integer operator^(big_integer a, big_integer const& b) {
    return a ^= b;
}

big_integer operator<<(big_integer a, int b) {
    return a <<= b;
}

big_integer operator>>(big_integer a, int b) {
   return a >>= b;
}

bool operator==(big_integer const& a, big_integer const& b) {
    return a.negative_ == b.negative_ && a.digits_ == b.digits_;
}

bool operator!=(big_integer const& a, big_integer const& b) {
    return !(a == b);
}

bool operator<(big_integer const& a, big_integer const& b) {
    if (a.negative_ != b.negative_) {
        return a.negative_ > b.negative_;
    }

    return (a.digits_.size() < b.digits_.size()) ||
           (a.digits_.size() == b.digits_.size() && less(a.digits_, b.digits_));
}

bool operator>(big_integer const& a, big_integer const& b) {
    return b < a;
}

bool operator<=(big_integer const& a, big_integer const& b) {
    return !(a > b);
}

bool operator>=(big_integer const& a, big_integer const& b) {
    return !(a < b);
}

void append_as_fixed_len_str(uint32_t x, std::string& str, size_t len) {
    size_t cnt = 0;
    while (x != 0) {
        str.push_back('0' + x % 10);
        x /= 10;
        ++cnt;
    }
    while (cnt != len) {
        str.push_back('0');
        ++cnt;
    }
}

std::string to_string(big_integer const& a) {
    if (a == ZERO) {
        return "0";
    }
    constexpr size_t step = 9;
    std::string str;
    big_integer tmp = a.negative_ ? -a : a;
    str.reserve(tmp.digits_.size() * 9 + 1);

    uint32_t rem;
    while (tmp != ZERO) {
        rem = tmp.div_short(TEN[step]);
        append_as_fixed_len_str(rem, str, step);
    }
    while (str.length() > 1 && str.back() == '0') {
        str.pop_back();
    }
    if (a.negative_) {
        str.push_back('-');
    }
    std::reverse(str.begin(), str.end());
    return str;
}

std::ostream& operator<<(std::ostream& s, big_integer const& a) {
    return s << to_string(a);
}
