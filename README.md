# .ch 文件中文解释器

## 项目简介

这是一个使用 C++ 实现的 `.ch` 文件中文解释器，直接解析并执行 `.ch` 后缀文件，无需转换为其他语言。解释器采用类 C++ 的编译解析逻辑，所有关键字均使用中文，支持变量定义、函数定义、基本运算、控制台输出、数组操作、结构体、文件操作和流程控制等核心功能。

## 版本信息

### v2.0.0（最新版本）

- 新增16进制汇编语言解析器，支持将.ch文件编译为.chex二进制文件
- 新增-x编译参数：`chplus -x <输出文件> <输入文件>.ch`
- 新增-r执行参数：`chplus -r <文件>.chex`
- 实现完整的16进制指令集和虚拟机
- 实现完整的表达式评估和代码生成
- 支持所有语句类型：变量定义、赋值、输出、函数定义、返回、if、while、for等
- 支持完整的比较运算：==、!=、<、<=、>、>=
- 支持逻辑运算：&&、||、!
- 实现完整的系统调用处理：输出字符、输出整数、读取输入
- 修复标签生成问题，确保跳转指令正确
- 修复函数定义和返回语句的汇编代码生成
- 支持chex文件扩展名
- 移除执行成功的多余输出信息
- **Bug反馈**：有bug请tg @abcdefgjha反馈

## 开源协议

本项目采用 **GNU Affero General Public License v3.0 (AGPL v3.0) + Commons Clause 1.0** 协议。

### 核心许可声明

1. 本项目**仅允许个人非商业用途使用**，禁止任何形式的商业使用（包括但不限于：出售、出租、作为商业产品的组件、通过本项目获取商业收益等）。
2. 个人使用者可以自由修改、重构本项目代码，但修改后的衍生作品必须：
   - 遵守本许可证的全部条款（同样禁止商用）
   - 保留原项目的版权信息和许可证文本
   - 以相同的协议开源发布，且必须附带完整的源代码（包括修改记录）
   - 明确标注衍生作品与原项目的差异，并提供原项目的官方源库链接
3. 禁止删除、修改本许可证中的任何条款，禁止隐瞒原项目的作者信息和源库地址。

完整协议文本请查看 [LICENSE](LICENSE) 文件。

## 主要特性

- 完整的错误定位系统，所有错误都包含精确的行号信息
- 智能类型系统：整型运算返回整型，浮点数运算返回浮点数
- 多维数组支持：支持1-5维数组
- 动态数组大小支持：支持使用变量和表达式作为数组大小
- 文件读写操作：支持文件读取、写入和追加操作
- 结构体定义和成员访问：支持结构体定义、变量实例化和成员访问
- 函数重载：支持同名函数不同参数
- 函数返回类型检查：确保函数返回类型与定义一致
- 完整的流程控制：if-else if-else、while、for循环
- ASCII字符兼容：完全支持ASCII码表和中文字符
- 系统命令行：支持系统命令行执行功能
- 跨平台兼容：支持Windows和Linux系统
- 16进制汇编编译：支持将.ch文件编译为.chex二进制文件
- 16进制汇编执行：支持执行.chex二进制文件
- 完整的指令集：LOAD, MOVE, PLUS, MINUS, TIMES, DIVIDE, JUMP, JEQ, JNE, FETCH, SAVE, CALL, TEST, BITAND, BITOR
- 独特的寄存器系统：AX, BX, CX, DX, IP, ST, BP, SR

## 目录结构

```
chplus/
├── main.cpp          # 主程序入口
├── CMakeLists.txt    # CMake构建配置文件
├── README.md         # 项目文档
├── LICENSE           # 开源协议
├── include/          # 头文件目录
│   ├── lexer.h       # 词法分析器
│   ├── parser.h      # 语法分析器
│   ├── interpreter.h # 执行器
│   └── asm.h        # 16进制汇编器
├── src/              # 源代码目录
│   ├── lexer.cpp
│   ├── parser.cpp
│   ├── interpreter.cpp
│   ├── CodeFormatter.cpp
│   ├── CHFormatter.cpp
│   └── asm.cpp
├── jni/              # Android JNI 构建配置
│   ├── Android.mk
│   └── Application.mk
├── examples/         # 示例文件目录
└── ch_Lib/          # 标准库目录
    ├── math.ch       # 数学库
    ├── string.ch     # 字符串库
    └── file.ch       # 文件库
```

## 编译方法

### Windows平台编译

```bash
# 使用 g++ 编译
g++ -std=c++17 -I include main.cpp src/interpreter.cpp src/lexer.cpp src/parser.cpp src/CodeFormatter.cpp src/CHFormatter.cpp src/asm.cpp -o chplus.exe

# 使用 CMake 编译
mkdir build && cd build
cmake .. && cmake --build .
```

### Android JNI 编译

```bash
cd jni
ndk-build
```

**编译配置说明：**

- 目标平台: Android 21+ (arm64-v8a)
- C++ 标准: C++17
- STL 库: c++_static
- 日志支持: Android Logcat 集成
- 安全特性: 代码混淆和优化

**构建文件说明：**

- Android.mk: Android 构建脚本，定义模块配置和源文件
  - 模块名称: chplus
  - 源文件: main.cpp, lexer.cpp, parser.cpp, interpreter.cpp, CodeFormatter.cpp, CHFormatter.cpp, asm.cpp
  - 包含路径: ../include
  - 链接库: -llog
- Application.mk: 应用配置，指定目标平台和编译选项

## 使用方法

### 基本使用

```bash
# 直接执行.ch文件
chplus example.ch

# 编译为16进制汇编二进制
chplus -x output.chex input.ch

# 执行16进制汇编二进制
chplus -r output.chex
```

### 代码格式化

```bash
# 自动格式化并覆盖原文件
chplus -a example.ch

# 不自动格式化
chplus -n example.ch

# 启用调试模式
chplus -d example.ch
```

## 语法说明

### 变量定义

```ch
定义(整型) a = 10;
定义(字符串) b = "测试内容";
定义(小数) pi = 3.14;
定义(布尔型) isActive = 真;
定义(字符型) ch = 'A';

// 数组定义
定义(整型) arr[10];
定义(整型) matrix[3][4];
```

### 函数定义

```ch
定义(空类型) 主函数() {
    控制台输出("Hello World");
}

定义(整型) 求和(定义(整型) x, 定义(整型) y) {
    返回 x + y;
}
```

### 流程控制

```ch
如果 (条件) {
    // 条件为真时执行
} 否则如果 (其他条件) {
    // 第一个条件为假，其他条件为真时执行
} 否则 {
    // 所有条件为假时执行
}

对于 (定义(整型) i = 0; i < 10; i++) {
    // 循环体
}

当 (条件) {
    // 循环体
}
```

### 控制台输入输出

```ch
控制台输出("Hello World");
控制台输入(变量);

文件写入("file.txt", "内容");
文件读取("file.txt", 变量);
文件追加("file.txt", "追加内容");
```

### 结构体

```ch
定义(结构体) Point {
    整型 x;
    整型 y;
};

定义(Point) p;
p.x = 10;
p.y = 20;
```

### 模块化编程

```ch
导入("ch_Lib/math.ch");
导入("ch_Lib/string.ch");
导入("ch_Lib/file.ch");

定义(空类型) 主函数() {
    定义(小数) result = sin(30);
    定义(整型) len = 长度("Hello");
    写入文件("output.txt", "内容");
}
```

### 系统命令行

```ch
系统命令行("echo Hello");
系统命令行("dir");
```

## 标准库

### 数学库 (math.ch)

- 三角函数：sin, cos, tan, asin, acos, atan
- 双曲函数：sinh, cosh, tanh
- 指数对数：exp, log, log10, sqrt, cbrt
- 取整函数：ceil, floor, round, trunc
- 绝对值：abs, fabs
- 其他函数：fmod, gcd, lcm
- 数学常量：PI, E, SQRT2

### 字符串库 (string.ch)

- 基础操作：长度、子串、查找、替换
- 转换功能：转大写、转小写、去空白
- 类型转换：整数转字符串、小数转字符串、布尔值转字符串
- 字符操作：获取字符、翻转、计数
- 高级功能：填充、分割、连接

### 文件库 (file.ch)

- 文件操作：文件写入、文件读取、文件追加、文件存在
- 文件重定向：重定向标准输出、恢复标准输出
- 文件系统：获取大小、路径操作
- 目录操作：创建、删除、重命名
- 文本处理：逐行读取、分割、合并

## 16进制汇编

### 16进制汇编语法特性

- **独特的寄存器系统**：AX, BX, CX, DX, IP, ST, BP, SR
- **独特的指令集**：LOAD, MOVE, PLUS, MINUS, TIMES, DIVIDE, JUMP, JEQ, JNE, FETCH, SAVE, CALL, TEST, BITAND, BITOR
- **16进制编码**：所有指令和操作数都使用16进制表示
- **固定指令长度**：每条指令4字节（8位16进制数）

### 指令说明

- **LOAD Rd, imm**：加载立即数到寄存器
- **MOVE Rd, Rs**：寄存器之间传送数据
- **PLUS Rd, Rs**：加法运算
- **MINUS Rd, Rs**：减法运算
- **TIMES Rd, Rs**：乘法运算
- **DIVIDE Rd, Rs**：除法运算
- **JUMP address**：无条件跳转
- **JEQ address**：相等时跳转
- **JNE address**：不等时跳转
- **FETCH Rd, [Rs]**：从内存读取数据
- **SAVE [Rd], Rs**：保存数据到内存
- **CALL callno**：系统调用
- **TEST Rd, Rs**：测试比较
- **BITAND Rd, Rs**：按位与
- **BITOR Rd, Rs**：按位或

### 系统调用

- **0x00**：退出程序
- **0x01**：输出字符（BX寄存器）
- **0x02**：输出整数（BX寄存器）
- **0x03**：读取输入

## 命令行选项

- `-a` : 自动格式化并覆盖原文件
- `-d` : 启用调试模式，显示详细执行信息
- `-n` : 不自动格式化代码
- `-x <输出文件>` : 编译为16进制汇编二进制文件
- `-r` : 执行16进制汇编二进制文件
- `--help, -h` : 显示帮助信息

## 许可证

原项目源库：[https://github.com/abcdefgjh-li/chplus]
