# .ch 文件中文解释器 - 快速参考

## 项目概述

**chplus** - 使用 C++ 实现的 `.ch` 文件中文解释器，直接解析并执行中文语法代码。

## 快速开始

### 编译
```bash
# 使用 g++ 编译
g++ -std=c++17 -I include main.cpp src/executor/interpreter.cpp src/lexer/lexer.cpp src/parser/parser.cpp src/utils/CodeFormatter.cpp -o chplus.exe

# 使用 CMake
mkdir build && cd build
cmake .. && cmake --build .
```

### 运行示例
```bash
# 运行 Hello World
./chplus.exe examples/hello.ch

# 不格式化运行
./chplus.exe --no-format examples/hello.ch

# 查看帮助
./chplus.exe --help
```

## 核心语法

### 变量定义
```ch
定义(整型) a = 10;
定义(字符串) b = "测试内容";
定义(小数) pi = 3.14;
定义(布尔型) flag = 真;
定义(字符型) ch = 'A';
```

### 函数定义
```ch
定义(整型) 求和(定义(整型) x, 定义(整型) y) {
    返回 x + y;
}

定义(空类型) 主函数() {
    控制台输出("Hello World");
}
```

### 流程控制
```ch
如果 (条件) {
    // 条件为真
} 否则如果 (其他条件) {
    // 其他条件为真
} 否则 {
    // 所有条件为假
}

定义(整型) i = 0;
当 (i < 5) {
    控制台输出(i);
    i = i + 1;
}

对于 (定义(整型) j = 0; j < 3; j = j + 1) {
    控制台输出(j);
}
```

### 数组操作
```ch
定义(整型) arr[10];
arr[0] = 100;
控制台输出(arr[0]);

定义(整型) matrix[3][4];
matrix[1][2] = 42;
```

### 结构体
```ch
定义(结构体) Point {
    整型 x;
    整型 y;
};

定义(空类型) 主函数() {
    定义(Point) p1;
    p1.x = 100;
    p1.y = 200;
    控制台输出("x=" + p1.x + ", y=" + p1.y);
}
```

## 标准库使用

### 导入库文件
```ch
导入("ch_Lib/math.ch");
导入("ch_Lib/string.ch");
导入("ch_Lib/file.ch");
```

### 数学库示例
```ch
导入("ch_Lib/math.ch");
定义(空类型) 主函数() {
    定义(小数) angle = 45.0;
    定义(小数) sin_val = sin(angle);
    定义(小数) cos_val = cos(angle);
    控制台输出("sin(45°) = " + sin_val);
    控制台输出("cos(45°) = " + cos_val);
}
```

### 字符串库示例
```ch
导入("ch_Lib/string.ch");
定义(空类型) 主函数() {
    定义(字符串) text = "Hello World";
    控制台输出(长度(text));           // 输出: 11
    控制台输出(子串(text, 0, 5));    // 输出: Hello
    控制台输出(转大写(text));         // 输出: HELLO WORLD
}
```

### 文件库示例
```ch
导入("ch_Lib/file.ch");
定义(空类型) 主函数() {
    // 写入文件
    写入文件("test.txt", "Hello, File!");
    
    // 读取文件
    定义(字符串) content = 读取文件("test.txt");
    控制台输出("文件内容: " + content);
    
    // 文件重定向
    重定向标准输出("log.txt");
    控制台输出("这条信息写入文件");
    恢复标准输出();
}
```

## 系统命令行

```ch
定义(空类型) 主函数() {
    // 执行系统命令
    系统命令行("echo Hello World");
    系统命令行("dir");
    
    // 获取命令执行结果
    定义(字符串) result = 系统命令行("echo 测试");
    控制台输出("命令结果: " + result);
}
```

## 数据类型对照表

| 中文类型 | C++ 类型 | 说明 |
|---------|---------|------|
| 整型 | int | 整数类型 |
| 字符串 | string | 字符串类型 |
| 小数 | double | 小数类型 |
| 布尔型 | bool | 布尔类型 |
| 字符型 | char | 字符类型 |
| 空类型 | void | 无返回值类型 |

## 运算符对照表

| 运算符 | 功能 | 示例 |
|--------|------|------|
| +, -, *, / | 算术运算 | a + b, a * b |
| % | 取模 | a % b |
| ^ | 乘方 | a ^ b |
| &&, \|\| | 逻辑运算 | a && b, a \|\| b |
| ! | 逻辑非 | !a |
| ==, != | 相等比较 | a == b, a != b |
| <, >, <=, >= | 大小比较 | a < b, a >= b |

## 目录结构

```
chplus/
├── main.cpp              # 主程序入口
├── include/              # 头文件目录
│   ├── lexer.h           # 词法分析器
│   ├── parser.h          # 语法分析器
│   └── interpreter.h     # 解释器
├── src/                  # 源代码目录
│   ├── lexer/            # 词法分析实现
│   ├── parser/           # 语法分析实现
│   ├── executor/         # 解释器实现
│   └── utils/            # 工具函数
├── examples/             # 示例文件目录
├── ch_Lib/              # 标准库目录
│   ├── math.ch          # 数学函数库
│   ├── string.ch        # 字符串操作库
│   └── file.ch          # 文件操作库
└── tests/               # 测试文件目录
```

## 常用示例文件

- `examples/hello.ch` - Hello World 示例
- `examples/calculator.ch` - 基本运算示例
- `examples/conditions.ch` - 条件判断示例
- `examples/array_demo.ch` - 数组操作示例
- `examples/struct_demo.ch` - 结构体示例
- `examples/file_demo.ch` - 文件操作示例

## 错误处理

所有错误信息都包含精确的行号定位：

```
错误: 变量未定义: x 在第 5 行
错误: 函数未定义: sin(小数) 在第 10 行
错误: 除零错误 在第 15 行
```

## 格式化选项

```bash
# 自动格式化（默认）
./chplus.exe file.ch

# 不自动格式化
./chplus.exe --no-format file.ch

# 格式化并覆盖原文件
./chplus.exe -a file.ch

# 调试模式
./chplus.exe -d file.ch
```

## 版本信息

### v1.4.0（最新）
- 修复函数调用参数类型推断问题
- 修复文件库函数重复定义
- 增强跨平台兼容性

### v1.3.0
- 修复结构体数组语法
- 支持中文标点符号

## 注意事项

1. 使用 UTF-8 编码保存 `.ch` 文件
2. 结构体定义必须在主函数外部
3. 全局作用域只能定义变量和结构体
4. 函数重载支持同名函数不同参数类型

## 快速测试

```bash
# 测试所有示例文件
for file in examples/*.ch; do
    echo "测试: $file"
    ./chplus.exe "$file"
done
```

---

**项目维护**: @abcdefgjha  
**Bug反馈**: 有bug请tg @abcdefgjha反馈