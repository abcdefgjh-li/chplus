#include "../include/CHFormatter.h"
#include <algorithm>
#include <sstream>
#include <regex>

// 构造函数
CHFormatter::CHFormatter(const std::string& src)
    : source(src), inString(false), inComment(false), inBlockComment(false),
      quoteChar('\0'), currentIndent(0), lineNumber(1) {
    initializeKeywords();
}

// 格式化函数
std::string CHFormatter::format(bool autoFormat, bool noFormat) {
    if (noFormat) {
        return formatWithoutPreprocessing();
    } else {
        return formatWithPreprocessing();
    }
}

// 初始化关键字集合
void CHFormatter::initializeKeywords() {
    keywords = {
        "定义", "如果", "否则", "否则如果", "当", "对于", "返回", 
        "控制台输出", "控制台输入", "控制台换行", "导入", "系统命令行", "空类型", 
        "整型", "字符串", "小数", "布尔型", "字符型", "结构体"
    };
}

// 带预处理的格式化
std::string CHFormatter::formatWithPreprocessing() {
    std::string result = source;
    
    // 预处理：规范化空格和括号
    preprocessWhitespace(result);
    replaceChineseSymbols(result);
    
    // 应用格式化
    processFormatting(result);
    
    return result;
}

// 不带预处理的格式化
std::string CHFormatter::formatWithoutPreprocessing() {
    std::string result = source;
    
    // 只处理换行和缩进，不预处理空格
    applyIndentation(result);
    
    return result;
}

// 预处理空格
void CHFormatter::preprocessWhitespace(std::string& code) {
    // 移除多余的空格
    code = replacePattern(code, "  ", " ");
    code = replacePattern(code, "\t", " ");
    
    // 规范化括号周围的空格
    code = replacePattern(code, "( ", "(");
    code = replacePattern(code, " )", ")");
    code = replacePattern(code, "{ ", "{");
    code = replacePattern(code, " }", "}");
    code = replacePattern(code, "[ ", "[");
    code = replacePattern(code, " ]", "]");
    
    // 规范化逗号周围的空格
    code = replacePattern(code, " ,", ",");
    code = replacePattern(code, ", ", ",");
    
    // 规范化运算符周围的空格
    code = replacePattern(code, " +", "+");
    code = replacePattern(code, "+ ", "+");
    code = replacePattern(code, " -", "-");
    code = replacePattern(code, "- ", "-");
    code = replacePattern(code, " *", "*");
    code = replacePattern(code, "* ", "*");
    code = replacePattern(code, " /", "/");
    code = replacePattern(code, "/ ", "/");
    code = replacePattern(code, " =", "=");
    code = replacePattern(code, "= ", "=");
    code = replacePattern(code, " <=", "<=");
    code = replacePattern(code, "<= ", "<=");
    code = replacePattern(code, " >=", ">=");
    code = replacePattern(code, ">= ", ">=");
    code = replacePattern(code, " ==", "==");
    code = replacePattern(code, "== ", "==");
    code = replacePattern(code, " !=", "!=");
    code = replacePattern(code, "!= ", "!=");
    
    // 在关键字和括号之间添加空格
    for (const auto& keyword : keywords) {
        code = replacePattern(code, keyword + "(", keyword + " (");
    }
    
    // 在关键字和左大括号之间添加空格
    for (const auto& keyword : keywords) {
        code = replacePattern(code, keyword + "{", keyword + " {");
    }
}

// 替换中文符号
void CHFormatter::replaceChineseSymbols(std::string& code) {
    // 替换中文标点为英文标点
    code = replacePattern(code, "，", ",");
    code = replacePattern(code, "、", ",");
    code = replacePattern(code, "【", "[");
    code = replacePattern(code, "】", "]");
    code = replacePattern(code, "（", "(");
    code = replacePattern(code, "）", ")");
    code = replacePattern(code, "：", ":");
    code = replacePattern(code, "；", ";");
}

// 应用格式化规则
void CHFormatter::processFormatting(std::string& code) {
    std::vector<std::string> lines = splitLines(code);
    std::string result;
    int indentLevel = 0;
    
    for (size_t i = 0; i < lines.size(); ++i) {
        std::string line = lines[i];
        trim(line);
        
        // 跳过空行
        if (line.empty()) {
            continue;
        }
        
        // 处理右大括号（减少缩进）
        if (line[0] == '}') {
            indentLevel = std::max(0, indentLevel - 1);
        }
        
        // 添加缩进
        result += std::string(indentLevel * 4, ' ');
        
        // 添加行内容
        result += line;
        
        // 处理左大括号（增加缩进）
        if (!line.empty() && line.back() == '{') {
            indentLevel++;
        }
        
        // 添加换行（除了最后一行）
        if (i < lines.size() - 1) {
            result += "\n";
        }
    }
    
    code = result;
}

// 应用缩进（不修改其他内容）
void CHFormatter::applyIndentation(std::string& code) {
    std::vector<std::string> lines = splitLines(code);
    std::string result;
    int indentLevel = 0;
    
    for (size_t i = 0; i < lines.size(); ++i) {
        std::string line = lines[i];
        
        // 移除行首空格
        size_t firstNonSpace = line.find_first_not_of(" \t");
        if (firstNonSpace != std::string::npos) {
            line = line.substr(firstNonSpace);
        }
        
        // 跳过空行
        if (line.empty()) {
            result += "\n";
            continue;
        }
        
        // 处理右大括号（减少缩进）
        if (line[0] == '}') {
            indentLevel = std::max(0, indentLevel - 1);
        }
        
        // 添加缩进
        result += std::string(indentLevel * 4, ' ');
        
        // 添加行内容
        result += line;
        
        // 处理左大括号（增加缩进）
        if (!line.empty() && line.back() == '{') {
            indentLevel++;
        }
        
        // 添加换行（除了最后一行）
        if (i < lines.size() - 1) {
            result += "\n";
        }
    }
    
    code = result;
}

// 替换模式
std::string CHFormatter::replacePattern(const std::string& str, const std::string& pattern, const std::string& replacement) {
    std::string result = str;
    size_t pos = 0;
    
    while ((pos = result.find(pattern, pos)) != std::string::npos) {
        result.replace(pos, pattern.length(), replacement);
        pos += replacement.length();
    }
    
    return result;
}

// 分割行
std::vector<std::string> CHFormatter::splitLines(const std::string& str) {
    std::vector<std::string> lines;
    std::istringstream iss(str);
    std::string line;
    
    while (std::getline(iss, line)) {
        lines.push_back(line);
    }
    
    return lines;
}

// 去除首尾空格
void CHFormatter::trim(std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) {
        str = "";
        return;
    }
    
    size_t last = str.find_last_not_of(" \t\n\r");
    str = str.substr(first, last - first + 1);
}
