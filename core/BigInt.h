#ifndef BIGINT_H
#define BIGINT_H

#include <string>
#include <vector>
#include <algorithm>

class BigInt {
private:
    std::vector<int> digits;
    bool negative;
    
    void removeLeadingZeros();
    static BigInt addAbsolute(const BigInt& a, const BigInt& b);
    static BigInt subtractAbsolute(const BigInt& a, const BigInt& b);
    static BigInt multiplyAbsolute(const BigInt& a, const BigInt& b);
    static BigInt divideAbsolute(const BigInt& a, const BigInt& b);
    static BigInt modAbsolute(const BigInt& a, const BigInt& b);
    static int compareAbsolute(const BigInt& a, const BigInt& b);
    
public:
    BigInt();
    BigInt(const std::string& str);
    BigInt(long long value);
    BigInt(const BigInt& other);
    
    BigInt& operator=(const BigInt& other);
    
    std::string toString() const;
    
    BigInt operator+(const BigInt& other) const;
    BigInt operator-(const BigInt& other) const;
    BigInt operator*(const BigInt& other) const;
    BigInt operator/(const BigInt& other) const;
    BigInt operator%(const BigInt& other) const;
    BigInt operator^(const BigInt& other) const;
    
    bool operator<(const BigInt& other) const;
    bool operator>(const BigInt& other) const;
    bool operator<=(const BigInt& other) const;
    bool operator>=(const BigInt& other) const;
    bool operator==(const BigInt& other) const;
    bool operator!=(const BigInt& other) const;
    
    bool isZero() const;
    bool isNegative() const;
};

#endif
