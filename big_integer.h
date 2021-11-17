#pragma once

#include <functional>
#include <iosfwd>
#include <string>
#include <vector>

struct big_integer
{
    using storage_t = std::vector<uint32_t>;

    big_integer();
    big_integer(big_integer const& other) = default;
    explicit big_integer(std::string const& str);
    ~big_integer() = default;
    void swap(big_integer& other);

    big_integer(int a);
    big_integer(unsigned a);
    big_integer(long a);
    big_integer(unsigned long a);
    big_integer(long long a);
    big_integer(unsigned long long a);

    big_integer& operator=(big_integer const& other) = default;

    big_integer& operator+=(big_integer const& rhs);
    big_integer& operator-=(big_integer const& rhs);
    big_integer& operator*=(big_integer rhs);
    big_integer& operator/=(big_integer const& rhs);
    big_integer& operator%=(big_integer const& rhs);

    big_integer& operator&=(big_integer const& rhs);
    big_integer& operator|=(big_integer const& rhs);
    big_integer& operator^=(big_integer const& rhs);

    big_integer& operator<<=(int rhs);
    big_integer& operator>>=(int rhs);

    big_integer operator+() const;
    big_integer operator-() const;
    big_integer operator~() const;

    big_integer& operator++();
    big_integer operator++(int);

    big_integer& operator--();
    big_integer operator--(int);

    friend bool operator==(big_integer const& a, big_integer const& b);
    friend bool operator!=(big_integer const& a, big_integer const& b);
    friend bool operator<(big_integer const& a, big_integer const& b);
    friend bool operator>(big_integer const& a, big_integer const& b);
    friend bool operator<=(big_integer const& a, big_integer const& b);
    friend bool operator>=(big_integer const& a, big_integer const& b);

    friend std::string to_string(big_integer const& a);

private:
    bool negative_;
    storage_t digits_;

    big_integer(bool neg, storage_t&&  dig);
    big_integer& mul_short(uint32_t b);
    uint32_t div_short(uint32_t b);
    big_integer& div_long(big_integer y);
    big_integer& mod_long(big_integer y);
    big_integer& bitwise(big_integer const& b, const std::function<uint32_t(uint32_t, uint32_t)>& op);
    big_integer& offset_sub(big_integer const& b, size_t offset);
    big_integer& negate();
    big_integer& abs();
    uint64_t get_digit(size_t idx) const;
    uint64_t get_inv_digit(size_t idx) const;
};

big_integer operator+(big_integer a, big_integer const& b);
big_integer operator-(big_integer a, big_integer const& b);
big_integer operator*(big_integer a, big_integer const& b);
big_integer operator/(big_integer a, big_integer const& b);
big_integer operator%(big_integer a, big_integer const& b);

big_integer operator&(big_integer a, big_integer const& b);
big_integer operator|(big_integer a, big_integer const& b);
big_integer operator^(big_integer a, big_integer const& b);

big_integer operator<<(big_integer a, int b);
big_integer operator>>(big_integer a, int b);

std::ostream& operator<<(std::ostream& s, big_integer const& a);
