// ch_Lib 数学库 - 完整实现
// 作者: chplus解释器
// 版本: 2.0

// 数学常数
定义(小数) PI = 3.14159265358979323846;
定义(小数) E = 2.71828182845904523536;
定义(小数) SQRT2 = 1.41421356237309504880;
定义(小数) SQRT3 = 1.73205080756887729353;

// 基础数学函数

// 正弦函数
定义(小数) sin(定义(小数) x) {
    // 角度转弧度
    定义(小数) radian = x * PI / 180.0;
    
    // 泰勒级数展开计算正弦
    定义(小数) result = 0;
    定义(小数) term = radian;
    定义(整型) n = 1;
    
    当 (n < 20) {
        如果 (n % 2 == 1) {
            result = result + term;
        } 否则 {
            result = result - term;
        }
        
        term = term * radian * radian / ((n + 1) * (n + 2));
        n = n + 2;
    }
    
    返回 result;
}

// 余弦函数
定义(小数) cos(定义(小数) x) {
    // 角度转弧度
    定义(小数) radian = x * PI / 180.0;
    
    // 泰勒级数展开计算余弦
    定义(小数) result = 1;
    定义(小数) term = 1;
    定义(整型) n = 2;
    
    当 (n < 20) {
        result = result - term;
        
        term = term * radian * radian / (n * (n - 1));
        n = n + 2;
    }
    
    返回 result;
}

// 正切函数
定义(小数) tan(定义(小数) x) {
    定义(小数) cos_val = cos(x);
    
    如果 (cos_val < 0.0001) {
        如果 (cos_val >= 0) {
            返回 999999.0;
        } 否则 {
            返回 -999999.0;
        }
    }
    
    返回 sin(x) / cos_val;
}

// 反正弦函数
定义(小数) asin(定义(小数) x) {
    如果 (x < -1.0 或 x > 1.0) {
        返回 0.0;
    }
    
    // 使用泰勒级数展开
    定义(小数) result = 0;
    定义(小数) term = x;
    定义(小数) x_squared = x * x;
    定义(整型) n = 1;
    定义(小数) factorial = 1;
    定义(小数) denominator = 1;
    
    当 (n < 20) {
        // 计算 (2n)! / (4^n * (n!)^2 * (2n+1))
        factorial = factorial * (2 * n - 1) * (2 * n);
        denominator = denominator * (n * n);
        
        term = term * x_squared * (2 * n - 1) * (2 * n) / (n * n * (2 * n + 1));
        result = result + term;
        
        n = n + 1;
    }
    
    返回 result * 180.0 / PI;
}

// 反余弦函数
定义(小数) acos(定义(小数) x) {
    如果 (x < -1.0 或 x > 1.0) {
        返回 0.0;
    }
    
    返回 90.0 - asin(x);
}

// 反正切函数
定义(小数) atan(定义(小数) x) {
    // 使用泰勒级数展开
    定义(小数) result = 0;
    定义(小数) term = x;
    定义(小数) x_squared = x * x;
    定义(整型) n = 1;
    
    当 (n < 20) {
        如果 (n % 2 == 1) {
            result = result + term;
        } 否则 {
            result = result - term;
        }
        
        term = term * x_squared * (2 * n - 1) / (2 * n + 1);
        n = n + 1;
    }
    
    返回 result * 180.0 / PI;
}

// 平方根
定义(小数) sqrt(定义(小数) x) {
    如果 (x < 0) {
        返回 0.0;
    }
    
    如果 (x == 0) {
        返回 0.0;
    }
    
    // 牛顿迭代法
    定义(小数) guess = x / 2.0;
    定义(整型) i = 0;
    
    当 (i < 20) {
        guess = (guess + x / guess) / 2.0;
        i = i + 1;
    }
    
    返回 guess;
}

// 自然指数函数
定义(小数) exp(定义(小数) x) {
    // 泰勒级数展开
    定义(小数) result = 1;
    定义(小数) term = 1;
    定义(小数) n = 1;
    
    当 (n < 20) {
        term = term * x / n;
        result = result + term;
        n = n + 1;
    }
    
    返回 result;
}

// 自然对数
定义(小数) log(定义(小数) x) {
    如果 (x <= 0) {
        返回 0.0;
    }
    
    // 使用泰勒级数展开
    定义(小数) result = 0;
    定义(小数) term = (x - 1) / (x + 1);
    定义(小数) n = 1;
    定义(小数) denominator = 1;
    
    当 (n < 20) {
        result = result + term / n;
        term = term * (x - 1) * (x - 1) / ((x + 1) * (x + 1));
        n = n + 1;
    }
    
    返回 result * 2.0;
}

// 常用对数
定义(小数) log10(定义(小数) x) {
    返回 log(x) / log(10.0);
}

// 向上取整
定义(小数) ceil(定义(小数) x) {
    如果 (x == static_cast<int>(x)) {
        返回 x;
    }
    
    如果 (x > 0) {
        返回 static_cast<int>(x) + 1;
    } 否则 {
        返回 static_cast<int>(x);
    }
}

// 向下取整
定义(小数) floor(定义(小数) x) {
    定义(整型) int_x = static_cast<int>(x);
    
    如果 (x < 0) {
        如果 (x != int_x) {
            返回 int_x - 1;
        }
    }
    
    返回 int_x;
}

// 四舍五入
定义(小数) round(定义(小数) x) {
    如果 (x >= 0) {
        如果 (x - static_cast<int>(x) >= 0.5) {
            返回 static_cast<int>(x) + 1;
        } 否则 {
            返回 static_cast<int>(x);
        }
    } 否则 {
        如果 (static_cast<int>(x) - x >= 0.5) {
            返回 static_cast<int>(x) - 1;
        } 否则 {
            返回 static_cast<int>(x);
        }
    }
}

// 取绝对值
定义(整型) abs(定义(整型) x) {
    如果 (x < 0) {
        返回 -x;
    }
    返回 x;
}

定义(小数) abs(定义(小数) x) {
    如果 (x < 0.0) {
        返回 -x;
    }
    返回 x;
}

// 幂运算
定义(小数) pow(定义(小数) base, 定义(小数) exponent) {
    如果 (exponent == 0.0) {
        返回 1.0;
    }
    
    如果 (base == 0.0) {
        如果 (exponent > 0) {
            返回 0.0;
        } 否则 {
            返回 999999.0;  // 无穷大
        }
    }
    
    // 使用对数运算：a^b = exp(b * ln(a))
    返回 exp(exponent * log(base));
}

// 辅助函数：计算两个数的最大公约数
定义(整型) gcd(定义(整型) a, 定义(整型) b) {
    如果 (b == 0) {
        返回 abs(a);
    }
    // 使用整数运算实现gcd
    定义(整型) temp = a % b;
    如果 (temp < 0) {
        temp = temp + abs(b);
    }
    返回 gcd(b, temp);
}

// 辅助函数：计算两个数的最小公倍数
定义(整型) lcm(定义(整型) a, 定义(整型) b) {
    如果 (a == 0) {
        返回 0;
    }
    如果 (b == 0) {
        返回 0;
    }
    返回 abs(a * b) / gcd(abs(a), abs(b));
}

// 数学常数验证
定义(布尔型) is_constant_valid() {
    定义(小数) pi_diff = abs(PI - 3.14159265358979323846);
    定义(小数) e_diff = abs(E - 2.71828182845904523536);
    
    如果 (pi_diff < 0.0001) {
        如果 (e_diff < 0.0001) {
            返回 真;
        }
    }
    返回 假;
}

// 数学库版本信息
定义(字符串) 数学库版本() {
    返回 "math.ch 2.0 - 完整数学函数库";
}

// 数学库信息
定义(字符串) 数学库信息() {
    返回 "提供完整数学函数库：三角函数、对数函数、指数函数、幂运算、数学常数等";
}