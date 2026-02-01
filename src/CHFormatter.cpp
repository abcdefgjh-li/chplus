#include "../include/CHFormatter.h"
#include <algorithm>
#include <cctype>
#include <functional>
#include <locale>

// 构造函数
CHFormatter::CHFormatter(const std::string& src) 
    : source(src), inString(false), inComment(false), inBlockComment(false), 
      quoteChar('\0'), currentIndent(0), lineNumber(1) {
    initializeKeywords();
}

// 格式化主函数
std::string CHFormatter::format(bool autoFormat, bool noFormat) {
    if (noFormat) {
        return source; // 不格式化，直接返回原代码
    }
    
    formatted.str("");
    formatted.clear();
    inString = false;
    inComment = false;
    inBlockComment = false;
    quoteChar = '\0';
    currentIndent = 0;
    lineNumber = 1;

    if (autoFormat) {
        return formatWithPreprocessing();
    } else {
        return formatWithoutPreprocessing();
    }
}

// 初始化关键字集合
void CHFormatter::initializeKeywords() {
    keywords = {
        "定义", "如果", "否则", "否则如果", "当", "对于", "返回", 
        "控制台输出", "控制台输入", "导入", "系统命令行", "空类型", 
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
    
    // 规范化分号周围的空格
    code = replacePattern(code, " ;", ";");
    code = replacePattern(code, "; ", ";");
}

// 替换模式
std::string CHFormatter::replacePattern(const std::string& code, const std::string& from, const std::string& to) {
    std::string result = code;
    size_t start_pos = 0;
    while ((start_pos = result.find(from, start_pos)) != std::string::npos) {
        result.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
    return result;
}

// 替换中文符号
void CHFormatter::replaceChineseSymbols(std::string& code) {
    // 替换中文逗号为英文逗号
    code = replacePattern(code, "，", ",");
    
    // 替换中文括号为英文括号
    code = replacePattern(code, "（", "(");
    code = replacePattern(code, "）", ")");
    code = replacePattern(code, "【", "[");
    code = replacePattern(code, "】", "]");
    code = replacePattern(code, "｛", "{");
    code = replacePattern(code, "｝", "}");
    
    // 替换中文分号为英文分号
    code = replacePattern(code, "；", ";");
    
    // 替换中文冒号为英文冒号
    code = replacePattern(code, "：", ":");
}

// 处理格式化
void CHFormatter::processFormatting(std::string& code) {
    formatted.str("");
    formatted.clear();
    
    // 应用缩进
    applyIndentation(code);
}

// 应用缩进
void CHFormatter::applyIndentation(std::string& code) {
    formatted.str("");
    formatted.clear();
    
    std::istringstream iss(code);
    std::string line;
    bool firstLine = true;
    
    while (std::getline(iss, line)) {
        trim(line);
        
        if (line.empty()) {
            formatted << "\n";
            continue;
        }
        
        // 计算缩进级别
        int indentLevel = currentIndent;
        
        // 添加换行（除了第一行）
        if (!firstLine) {
            formatted << "\n";
        }
        firstLine = false;
        
        // 输出带缩进的行
        for (int i = 0; i < indentLevel * 4; i++) {
            formatted << " ";
        }
        formatted << line;
        
        // 检查这行是否以左大括号结尾（且不是右大括号）
        if (endsWith(line, "{") && !startsWith(line, "}")) {
            currentIndent++;
        }
        
        // 检查这行是否以右大括号开头
        if (startsWith(line, "}")) {
            currentIndent = std::max(0, currentIndent - 1);
        }
    }
}

// 去除字符串两端的空白字符
void CHFormatter::trim(std::string& str) {
    // 去除左端空白
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), 
        [](unsigned char ch) { return !std::isspace(ch); }));
    
    // 去除右端空白
    str.erase(std::find_if(str.rbegin(), str.rend(), 
        [](unsigned char ch) { return !std::isspace(ch); }).base(), str.end());
}

// 辅助函数实现
bool CHFormatter::isNewline(char c) const { return c == '\n' || c == '\r'; }
bool CHFormatter::isQuote(char c) const { return c == '\"' || c == '\''; }
bool CHFormatter::isWhitespace(char c) const { return std::isspace(c); }
bool CHFormatter::isAlpha(char c) const { return std::isalpha(c); }
bool CHFormatter::isDigit(char c) const { return std::isdigit(c); }
bool CHFormatter::isAlnum(char c) const { return std::isalnum(c); }
bool CHFormatter::isPunct(char c) const { return std::ispunct(c); }
bool CHFormatter::isControl(char c) const { return std::iscntrl(c); }
bool CHFormatter::isSpace(char c) const { return std::isspace(c); }
bool CHFormatter::isBrace(char c) const { return c == '{' || c == '}'; }
bool CHFormatter::isOperator(char c) const { 
    return c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c == '=' || c == '<' || c == '>' || c == '!' || c == '&' || c == '|' || c == '^';
}
bool CHFormatter::isSeparator(char c) const { 
    return c == ',' || c == ';' || c == ':' || c == '.';
}
bool CHFormatter::isKeyword(const std::string& word) const {
    return keywords.find(word) != keywords.end();
}
bool CHFormatter::startsWith(const std::string& str, const std::string& prefix) const {
    return str.size() >= prefix.size() && str.compare(0, prefix.size(), prefix) == 0;
}
bool CHFormatter::endsWith(const std::string& str, const std::string& suffix) const {
    return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}