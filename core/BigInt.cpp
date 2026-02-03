#include "BigInt.h"
#include <stdexcept>
#include <sstream>

BigInt::BigInt() : negative(false) {
    digits.push_back(0);
}

BigInt::BigInt(const std::string& str) {
    if (str.empty()) {
        negative = false;
        digits.push_back(0);
        return;
    }
    
    size_t start = 0;
    if (str[0] == '-') {
        negative = true;
        start = 1;
    } else {
        negative = false;
    }
    
    for (size_t i = start; i < str.length(); i++) {
        if (str[i] >= '0' && str[i] <= '9') {
            digits.push_back(str[i] - '0');
        } else {
            throw std::invalid_argument("Invalid number format");
        }
    }
    
    std::reverse(digits.begin(), digits.end());
    removeLeadingZeros();
}

BigInt::BigInt(long long value) {
    if (value < 0) {
        negative = true;
        value = -value;
    } else {
        negative = false;
    }
    
    if (value == 0) {
        digits.push_back(0);
    } else {
        while (value > 0) {
            digits.push_back(value % 10);
            value /= 10;
        }
    }
}

BigInt::BigInt(const BigInt& other) : digits(other.digits), negative(other.negative) {
}

BigInt& BigInt::operator=(const BigInt& other) {
    if (this != &other) {
        digits = other.digits;
        negative = other.negative;
    }
    return *this;
}

void BigInt::removeLeadingZeros() {
    while (digits.size() > 1 && digits.back() == 0) {
        digits.pop_back();
    }
    if (digits.size() == 1 && digits[0] == 0) {
        negative = false;
    }
}

std::string BigInt::toString() const {
    std::string result;
    if (negative && !isZero()) {
        result += "-";
    }
    for (int i = digits.size() - 1; i >= 0; i--) {
        result += ('0' + digits[i]);
    }
    return result;
}

BigInt BigInt::addAbsolute(const BigInt& a, const BigInt& b) {
    BigInt result;
    result.digits.clear();
    result.negative = false;
    
    int carry = 0;
    int maxLen = std::max(a.digits.size(), b.digits.size());
    
    for (int i = 0; i < maxLen || carry > 0; i++) {
        int sum = carry;
        if (i < a.digits.size()) sum += a.digits[i];
        if (i < b.digits.size()) sum += b.digits[i];
        
        result.digits.push_back(sum % 10);
        carry = sum / 10;
    }
    
    result.removeLeadingZeros();
    return result;
}

BigInt BigInt::subtractAbsolute(const BigInt& a, const BigInt& b) {
    BigInt result;
    result.digits.clear();
    result.negative = false;
    
    int borrow = 0;
    int maxLen = std::max(a.digits.size(), b.digits.size());
    
    for (int i = 0; i < maxLen; i++) {
        int diff = borrow;
        if (i < a.digits.size()) diff += a.digits[i];
        if (i < b.digits.size()) diff -= b.digits[i];
        
        if (diff < 0) {
            diff += 10;
            borrow = -1;
        } else {
            borrow = 0;
        }
        
        result.digits.push_back(diff);
    }
    
    result.removeLeadingZeros();
    return result;
}

int BigInt::compareAbsolute(const BigInt& a, const BigInt& b) {
    if (a.digits.size() != b.digits.size()) {
        return a.digits.size() > b.digits.size() ? 1 : -1;
    }
    
    for (int i = a.digits.size() - 1; i >= 0; i--) {
        if (a.digits[i] != b.digits[i]) {
            return a.digits[i] > b.digits[i] ? 1 : -1;
        }
    }
    
    return 0;
}

BigInt BigInt::multiplyAbsolute(const BigInt& a, const BigInt& b) {
    BigInt result;
    result.digits.clear();
    result.negative = false;
    
    if (a.isZero() || b.isZero()) {
        result.digits.push_back(0);
        return result;
    }
    
    result.digits.resize(a.digits.size() + b.digits.size(), 0);
    
    for (size_t i = 0; i < a.digits.size(); i++) {
        int carry = 0;
        for (size_t j = 0; j < b.digits.size() || carry > 0; j++) {
            long long product = result.digits[i + j] + carry;
            if (j < b.digits.size()) {
                product += a.digits[i] * b.digits[j];
            }
            result.digits[i + j] = product % 10;
            carry = product / 10;
        }
    }
    
    result.removeLeadingZeros();
    return result;
}

BigInt BigInt::divideAbsolute(const BigInt& a, const BigInt& b) {
    if (b.isZero()) {
        throw std::runtime_error("除零错误");
    }
    
    BigInt result;
    result.digits.clear();
    result.negative = false;
    
    if (compareAbsolute(a, b) < 0) {
        result.digits.push_back(0);
        return result;
    }
    
    BigInt current;
    current.digits.clear();
    current.negative = false;
    
    for (int i = a.digits.size() - 1; i >= 0; i--) {
        current.digits.insert(current.digits.begin(), a.digits[i]);
        current.removeLeadingZeros();
        
        int quotient = 0;
        while (compareAbsolute(current, b) >= 0) {
            current = subtractAbsolute(current, b);
            quotient++;
        }
        
        result.digits.insert(result.digits.begin(), quotient);
    }
    
    result.removeLeadingZeros();
    return result;
}

BigInt BigInt::modAbsolute(const BigInt& a, const BigInt& b) {
    if (b.isZero()) {
        throw std::runtime_error("取模除零错误");
    }
    
    if (compareAbsolute(a, b) < 0) {
        return a;
    }
    
    BigInt quotient = divideAbsolute(a, b);
    BigInt product = multiplyAbsolute(quotient, b);
    return subtractAbsolute(a, product);
}

BigInt BigInt::operator+(const BigInt& other) const {
    if (negative == other.negative) {
        BigInt result = addAbsolute(*this, other);
        result.negative = negative;
        return result;
    } else {
        int cmp = compareAbsolute(*this, other);
        if (cmp >= 0) {
            BigInt result = subtractAbsolute(*this, other);
            result.negative = negative;
            return result;
        } else {
            BigInt result = subtractAbsolute(other, *this);
            result.negative = other.negative;
            return result;
        }
    }
}

BigInt BigInt::operator-(const BigInt& other) const {
    BigInt temp = other;
    temp.negative = !temp.negative;
    return *this + temp;
}

BigInt BigInt::operator*(const BigInt& other) const {
    BigInt result = multiplyAbsolute(*this, other);
    result.negative = (negative != other.negative);
    return result;
}

BigInt BigInt::operator/(const BigInt& other) const {
    BigInt result = divideAbsolute(*this, other);
    result.negative = (negative != other.negative);
    return result;
}

BigInt BigInt::operator%(const BigInt& other) const {
    BigInt result = modAbsolute(*this, other);
    result.negative = negative;
    return result;
}

BigInt BigInt::operator^(const BigInt& other) const {
    if (other.negative) {
        throw std::runtime_error("负指数不支持");
    }
    
    BigInt result(1);
    BigInt base = *this;
    BigInt exp = other;
    
    while (!exp.isZero()) {
        if (exp.digits[0] % 2 == 1) {
            result = result * base;
        }
        base = base * base;
        exp = exp / BigInt(2);
    }
    
    return result;
}

bool BigInt::operator<(const BigInt& other) const {
    if (negative != other.negative) {
        return negative;
    }
    
    int cmp = compareAbsolute(*this, other);
    if (negative) {
        return cmp > 0;
    } else {
        return cmp < 0;
    }
}

bool BigInt::operator>(const BigInt& other) const {
    return other < *this;
}

bool BigInt::operator<=(const BigInt& other) const {
    return !(other < *this);
}

bool BigInt::operator>=(const BigInt& other) const {
    return !(*this < other);
}

bool BigInt::operator==(const BigInt& other) const {
    if (negative != other.negative) {
        return false;
    }
    return compareAbsolute(*this, other) == 0;
}

bool BigInt::operator!=(const BigInt& other) const {
    return !(*this == other);
}

bool BigInt::isZero() const {
    return digits.size() == 1 && digits[0] == 0;
}

bool BigInt::isNegative() const {
    return negative;
}
