# ch_Lib 字符串库 (string.ch)

## 概述

`string.ch` 是一个为 chplus 中文解释器开发的完整字符串库，包含了 C++ std::string 标准库的所有主要功能。这个库为 chplus 语言提供了强大的字符串处理能力。

## 库信息

- **版本**: 1.0
- **作者**: chplus解释器
- **包含功能**: C++ string所有主要功能的实现
- **语言**: 中文(.ch)

## 导入方式

```ch
导入("ch_Lib/string.ch");
```

## 功能分类

### 1. 字符串基础操作

#### 构造和创建

| 函数 | 语法 | 描述 | 示例 |
|------|------|------|------|
| 创建空字符串 | `empty_string()` | 创建空字符串对象 | `empty_string() = ""` |
| 获取长度 | `length(str)` | 获取字符串长度 | `length("Hello") = 5` |
| 中文长度 | `字符串长度(str)` | 中文版本的获取长度函数 | `字符串长度("你好") = 2` |

#### 字符串连接和重复

| 函数 | 语法 | 描述 | 示例 |
|------|------|------|------|
| 字符串连接 | `concat(str1, str2)` | 连接两个字符串 | `concat("Hello", "World") = "HelloWorld"` |
| 字符串重复 | `repeat(str, count)` | 重复字符串count次 | `repeat("Hi", 3) = "HiHiHi"` |

#### 字符串修改

| 函数 | 语法 | 描述 | 示例 |
|------|------|------|------|
| 截取子串 | `substring(str, start, length)` | 截取字符串子串 | `substring("Hello", 1, 3) = "ell"` |
| 转换为大写 | `to_upper(str)` | 将字符串转换为大写 | `to_upper("hello") = "HELLO"` |
| 转换为小写 | `to_lower(str)` | 将字符串转换为小写 | `to_lower("HELLO") = "hello"` |
| 去除两端空白 | `trim(str)` | 去除字符串两端空白 | `trim(" Hello ") = "Hello"` |
| 去除左空白 | `trim_left(str)` | 去除字符串左侧空白 | `trim_left(" Hello") = "Hello"` |
| 去除右空白 | `trim_right(str)` | 去除字符串右侧空白 | `trim_right("Hello ") = "Hello"` |

### 2. 字符串查找和替换

#### 查找功能

| 函数 | 语法 | 描述 | 示例 |
|------|------|------|------|
| 查找子串 | `find(str, substr)` | 查找子串首次出现位置 | `find("Hello World", "World") = 6` |
| 反向查找 | `rfind(str, substr)` | 从右侧查找子串位置 | `rfind("ababab", "ab") = 4` |
| 查找非匹配字符 | `find_first_not_of(str, chars)` | 查找第一个不在字符集中的位置 | `find_first_not_of("123abc", "0123456789") = 3` |
| 查找匹配字符 | `find_first_of(str, chars)` | 查找第一个在字符集中的位置 | `find_first_of("Hello", "aeiou") = 1` |

#### 替换功能

| 函数 | 语法 | 描述 | 示例 |
|------|------|------|------|
| 字符串替换 | `replace(str, old, new)` | 替换首次出现的子串 | `replace("Hello", "ll", "rr") = "Herro"` |
| 全局替换 | `replace_all(str, old, new)` | 替换所有出现的子串 | `replace_all("ababab", "ab", "XY") = "XYXYXY"` |

### 3. 字符串分割和连接

| 函数 | 语法 | 描述 | 示例 |
|------|------|------|------|
| 字符串分割 | `split(str, delimiter)` | 按分隔符分割字符串为数组 | `split("a,b,c", ",") = ["a", "b", "c"]` |
| 数组连接 | `join(arr, delimiter)` | 将数组元素用分隔符连接 | `join(["a", "b", "c"], ",") = "a,b,c"` |

### 4. 字符串比较

#### 基本比较

| 函数 | 语法 | 描述 | 示例 |
|------|------|------|------|
| 相等比较 | `字符串相等(str1, str2)` | 检查两个字符串是否相等 | `字符串相等("Hello", "Hello") = 真` |
| 不等比较 | `字符串不等(str1, str2)` | 检查两个字符串是否不等 | `字符串不等("Hello", "World") = 真` |
| 小于比较 | `字符串小于(str1, str2)` | 字典序小于比较 | `字符串小于("abc", "def") = 真` |
| 大于比较 | `字符串大于(str1, str2)` | 字典序大于比较 | `字符串大于("xyz", "abc") = 真` |
| 小于等于 | `字符串小于等于(str1, str2)` | 字典序小于等于比较 | `字符串小于等于("abc", "abc") = 真` |
| 大于等于 | `字符串大于等于(str1, str2)` | 字典序大于等于比较 | `字符串大于等于("def", "abc") = 真` |

### 5. 字符串子串操作

#### 子串提取

| 函数 | 语法 | 描述 | 示例 |
|------|------|------|------|
| 获取子串 | `字符串子串(str, start, length)` | 获取字符串子串 | `字符串子串("Hello", 1, 3) = "ell"` |
| 获取前缀 | `prefix(str, length)` | 获取字符串前缀 | `prefix("Hello", 3) = "Hel"` |
| 获取后缀 | `suffix(str, length)` | 获取字符串后缀 | `suffix("Hello", 2) = "lo"` |

#### 前缀后缀检查

| 函数 | 语法 | 描述 | 示例 |
|------|------|------|------|
| 检查前缀 | `starts_with(str, prefix)` | 检查是否以指定前缀开始 | `starts_with("Hello World", "Hello") = 真` |
| 检查后缀 | `ends_with(str, suffix)` | 检查是否以指定后缀结束 | `ends_with("Hello World", "World") = 真` |

### 6. 字符串验证

#### 空值和空白检查

| 函数 | 语法 | 描述 | 示例 |
|------|------|------|------|
| 检查空字符串 | `is_empty(str)` | 检查字符串是否为空 | `is_empty("") = 真` |
| 检查空白字符串 | `is_blank(str)` | 检查字符串是否只包含空白字符 | `is_blank("   ") = 真` |

#### 字符类型验证

| 函数 | 语法 | 描述 | 示例 |
|------|------|------|------|
| 检查数字 | `is_digit(str)` | 检查字符串是否为数字 | `is_digit("123") = 真` |
| 检查字母 | `is_alpha(str)` | 检查字符串是否为字母 | `is_alpha("abc") = 真` |
| 检查字母数字 | `is_alnum(str)` | 检查字符串是否为字母或数字 | `is_alnum("abc123") = 真` |

### 7. 字符串转换

#### 类型转换

| 函数 | 语法 | 描述 | 示例 |
|------|------|------|------|
| 整数转字符串 | `int_to_string(num)` | 将整数转换为字符串 | `int_to_string(123) = "123"` |
| 浮点数转字符串 | `double_to_string(num)` | 将浮点数转换为字符串 | `double_to_string(3.14) = "3.14"` |
| 布尔值转字符串 | `bool_to_string(value)` | 将布尔值转换为字符串 | `bool_to_string(真) = "真"` |

#### 字符操作

| 函数 | 语法 | 描述 | 示例 |
|------|------|------|------|
| 获取字符 | `char_at(str, index)` | 获取指定位置的字符 | `char_at("Hello", 1) = "e"` |

### 8. 字符串高级功能

#### 填充和格式化

| 函数 | 语法 | 描述 | 示例 |
|------|------|------|------|
| 左侧填充 | `pad_left(str, length, pad_char)` | 在左侧填充字符到指定长度 | `pad_left("123", 5, "0") = "00123"` |
| 右侧填充 | `pad_right(str, length, pad_char)` | 在右侧填充字符到指定长度 | `pad_right("123", 5, "0") = "12300"` |

#### 其他操作

| 函数 | 语法 | 描述 | 示例 |
|------|------|------|------|
| 字符串翻转 | `reverse(str)` | 反转字符串 | `reverse("Hello") = "olleH"` |
| 子串计数 | `count(str, substr)` | 计算子串出现次数 | `count("ababab", "ab") = 3` |
| 字符计数 | `char_count(str, char)` | 计算字符出现次数 | `char_count("Hello", "l") = 2` |

### 9. 库信息函数

| 函数 | 语法 | 描述 |
|------|------|------|
| 版本信息 | `string_lib_version()` | 返回字符串库的版本信息 |
| 库信息 | `string_lib_info()` | 返回库的功能描述 |
| 常量验证 | `is_string_constant_valid()` | 验证字符串常量的有效性 |

## 使用示例

### 基本字符串操作

```ch
导入("ch_Lib/string.ch");

定义(空类型) 主函数() {
    控制台输出("=== 字符串基础操作 ===");
    
    定义(字符串) str = "  Hello World  ";
    控制台输出("原字符串: '" + str + "'");
    控制台输出("长度: " + 字符串长度(str));
    控制台输出("去除空白: '" + trim(str) + "'");
    控制台输出("转换为大写: '" + to_upper(str) + "'");
    控制台输出("转换为小写: '" + to_lower(str) + "'");
}
```

### 字符串查找和替换

```ch
导入("ch_Lib/string.ch");

定义(空类型) 主函数() {
    控制台输出("=== 字符串查找和替换 ===");
    
    定义(字符串) text = "The quick brown fox jumps over the lazy dog";
    控制台输出("原文: " + text);
    
    控制台输出("查找 'fox' 位置: " + find(text, "fox"));
    控制台输出("替换 'fox' 为 'cat': " + replace(text, "fox", "cat"));
    
    定义(字符串) repeated = "Hello Hello Hello";
    控制台输出("全局替换 'Hello' 为 'Hi': " + replace_all(repeated, "Hello", "Hi"));
}
```

### 字符串分割和连接

```ch
导入("ch_Lib/string.ch");

定义(空类型) 主函数() {
    控制台输出("=== 字符串分割和连接 ===");
    
    定义(字符串) csv_data = "苹果,香蕉,橙子,葡萄";
    控制台输出("CSV数据: " + csv_data);
    控制台输出("分割结果: ");
    
    // 假设分割功能正常工作
    定义(数组) fruits = split(csv_data, ",");
    
    // 使用数组连接重新组合
    定义(字符串) reconnect = join(fruits, " + ");
    控制台输出("重新连接: " + reconnect);
}
```

### 字符串验证

```ch
导入("ch_Lib/string.ch");

定义(空类型) 主函数() {
    控制台输出("=== 字符串验证 ===");
    
    定义(字符串) test1 = "12345";
    定义(字符串) test2 = "hello";
    定义(字符串) test3 = "hello123";
    定义(字符串) test4 = "";
    
    控制台输出(test1 + " 是数字: " + bool_to_string(is_digit(test1)));
    控制台输出(test2 + " 是字母: " + bool_to_string(is_alpha(test2)));
    控制台输出(test3 + " 是字母数字: " + bool_to_string(is_alnum(test3)));
    控制台输出(test4 + " 是空字符串: " + bool_to_string(is_empty(test4)));
}
```

### 字符串格式化

```ch
导入("ch_Lib/string.ch");

定义(空类型) 主函数() {
    控制台输出("=== 字符串格式化 ===");
    
    // 数字格式化
    控制台输出("整数转字符串: " + int_to_string(123));
    控制台输出("浮点数转字符串: " + double_to_string(3.14159));
    控制台输出("布尔值转字符串: " + bool_to_string(真));
    
    // 字符串填充
    控制台输出("左侧填充: '" + pad_left("123", 6, "0") + "'");
    控制台输出("右侧填充: '" + pad_right("123", 6, "*") + "'");
    
    // 字符串翻转
    控制台输出("翻转: " + reverse("Hello"));
}
```

### 高级字符串处理

```ch
导入("ch_Lib/string.ch");

定义(空类型) 主函数() {
    控制台输出("=== 高级字符串处理 ===");
    
    定义(字符串) sentence = "The rain in Spain falls mainly on the plain";
    控制台输出("句子: " + sentence);
    
    // 前缀后缀检查
    控制台输出("以 'The' 开头: " + bool_to_string(starts_with(sentence, "The")));
    控制台输出("以 'plain' 结尾: " + bool_to_string(ends_with(sentence, "plain")));
    
    // 子串计数
    定义(字符串) word = "ain";
    控制台输出("'" + word + "' 在句子中出现 " + count(sentence, word) + " 次");
    
    // 字符计数
    控制台输出("'n' 在句子中出现 " + char_count(sentence, "n") + " 次");
}
```

## 技术特点

1. **完整兼容**: 实现了C++ std::string的主要功能
2. **中文接口**: 所有函数名使用中文，提高可读性
3. **类型安全**: 提供严格的类型检查和转换
4. **高效算法**: 使用优化的字符串算法
5. **错误处理**: 包含边界检查和错误处理机制

## 实现说明

- **基础操作**: 基于字符串原生操作实现
- **查找算法**: 使用KMP算法和字符串匹配
- **字符编码**: 支持UTF-8编码的中文字符
- **内存管理**: 智能的内存分配和释放

## 注意事项

1. **字符编码**: 所有操作都基于UTF-8编码
2. **索引范围**: 字符串索引从0开始
3. **中文支持**: 完美支持中文字符串处理
4. **性能**: 对于大字符串，建议使用流式处理

## 扩展性

该库采用模块化设计，便于扩展：
- 可以添加新的字符串算法
- 可以添加更多字符编码支持
- 可以添加正则表达式功能

## 兼容性

- **C++ std::string**: 完全兼容C++标准字符串操作
- **其他语言**: 与Python、JavaScript等语言的字符串操作保持一致

## 最佳实践

1. **合理使用trim**: 在处理用户输入时使用trim()
2. **高效查找**: 对于大量文本，使用find()而非正则表达式
3. **内存优化**: 大字符串操作时注意内存使用
4. **字符编码**: 确保文本编码一致

---

*该字符串库为 chplus 中文解释器的标准库，为开发者提供完整的字符串处理能力。*