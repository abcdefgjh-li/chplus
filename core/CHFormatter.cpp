#include "CHFormatter.h"
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
        "整型", "字符串", "小数", "布尔型", "字符型", "结构体",
        "真", "假", "空", "中断", "继续"
    };
}

// 带预处理的格式化
std::string CHFormatter::formatWithPreprocessing() {
    std::string result = source;
    
    // 预处理：规范化空格和符号
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
    // 移除多余的空格（超过2个连续空格）
    std::regex multiSpace(" {2,}");
    code = std::regex_replace(code, multiSpace, " ");
    
    // 移除制表符，替换为空格
    std::regex tabPattern("\\t+");
    code = std::regex_replace(code, tabPattern, " ");
    
    // 移除行尾空格
    std::regex trailingSpace("[ \t]+$");
    code = std::regex_replace(code, trailingSpace, "");
    
    // 移除多余的空行
    std::regex multiEmptyLine("\\n{3,}");
    code = std::regex_replace(code, multiEmptyLine, "\n\n");
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

// 应用格式化规则（参照Java样式）
void CHFormatter::processFormatting(std::string& code) {
    std::string result;
    std::string currentLine;
    int indentLevel = 0;
    bool inString = false;
    char stringChar = '\0';
    bool inComment = false;
    bool inBlockComment = false;
    bool inParentheses = false;
    int parenLevel = 0;
    
    for (size_t i = 0; i < code.length(); ++i) {
        char c = code[i];
        
        // 处理字符串
        if (!inComment && !inBlockComment && (c == '"' || c == '\'')) {
            if (!inString) {
                inString = true;
                stringChar = c;
            } else if (c == stringChar) {
                inString = false;
                stringChar = '\0';
            }
            currentLine += c;
            continue;
        }
        
        if (inString) {
            currentLine += c;
            continue;
        }
        
        // 处理注释
        if (c == '/' && i + 1 < code.length()) {
            if (code[i + 1] == '/' && !inBlockComment) {
                inComment = true;
                currentLine += c;
                continue;
            } else if (code[i + 1] == '*' && !inComment) {
                inBlockComment = true;
                currentLine += c;
                continue;
            }
        }
        
        if (inComment) {
            currentLine += c;
            if (c == '\n') {
                result += std::string(indentLevel * 4, ' ') + currentLine;
                currentLine = "";
                inComment = false;
            }
            continue;
        }
        
        if (inBlockComment) {
            currentLine += c;
            if (c == '*' && i + 1 < code.length() && code[i + 1] == '/') {
                currentLine += code[++i];
                if (c == '\n') {
                    result += std::string(indentLevel * 4, ' ') + currentLine;
                    currentLine = "";
                }
                inBlockComment = false;
            }
            continue;
        }
        
        // 处理分号（语句结束）
        if (c == ';') {
            currentLine += c;
            
            // 检查是否在循环语句中（对于、当）
            bool inLoopStatement = false;
            for (const auto& keyword : keywords) {
                if (keyword == "对于" || keyword == "当") {
                    if (currentLine.find(keyword) != std::string::npos) {
                        inLoopStatement = true;
                        break;
                    }
                }
            }
            
            // 如果不在循环语句中，才进行换行
            if (!inLoopStatement) {
                currentLine = addOperatorSpaces(currentLine);
                currentLine = addCommaSpaces(currentLine);
                trim(currentLine);
                if (!currentLine.empty()) {
                    result += std::string(indentLevel * 4, ' ') + currentLine + "\n";
                }
                currentLine = "";
            }
            continue;
        }
        
        // 处理左大括号
        if (c == '{') {
            currentLine += c;
            currentLine = addOperatorSpaces(currentLine);
            currentLine = addCommaSpaces(currentLine);
            trim(currentLine);
            if (!currentLine.empty()) {
                result += std::string(indentLevel * 4, ' ') + currentLine + "\n";
            }
            currentLine = "";
            indentLevel++;
            continue;
        }
        
        // 处理右大括号
        if (c == '}') {
            trim(currentLine);
            if (!currentLine.empty()) {
                indentLevel = std::max(0, indentLevel - 1);
                result += std::string(indentLevel * 4, ' ') + currentLine + "\n";
                result += std::string(indentLevel * 4, ' ') + "}\n";
            } else {
                indentLevel = std::max(0, indentLevel - 1);
                result += std::string(indentLevel * 4, ' ') + "}\n";
            }
            currentLine = "";
            continue;
        }
        
        // 处理换行
        if (c == '\n') {
            trim(currentLine);
            if (!currentLine.empty()) {
                result += std::string(indentLevel * 4, ' ') + currentLine + "\n";
            }
            currentLine = "";
            continue;
        }
        
        // 处理空格和制表符
        if (c == ' ' || c == '\t') {
            if (!currentLine.empty() && currentLine.back() != ' ') {
                currentLine += ' ';
            }
            continue;
        }
        
        currentLine += c;
    }
    
    // 处理最后一行
    trim(currentLine);
    if (!currentLine.empty()) {
        result += std::string(indentLevel * 4, ' ') + currentLine + "\n";
    }
    
    code = result;
}

// 处理单行内容
std::string CHFormatter::processLineContent(const std::string& line, int indentLevel) {
    std::string result = line;
    
    // 处理字符串
    if (inString) {
        return result;
    }
    
    // 处理关键字后的空格
    for (const auto& keyword : keywords) {
        size_t pos = result.find(keyword);
        if (pos != std::string::npos) {
            size_t afterKeyword = pos + keyword.length();
            if (afterKeyword < result.length() && 
                result[afterKeyword] != ' ' && 
                result[afterKeyword] != '(' && 
                result[afterKeyword] != '{' && 
                result[afterKeyword] != '\t') {
                result.insert(afterKeyword, " ");
            }
        }
    }
    
    // 处理运算符周围的空格
    result = addOperatorSpaces(result);
    
    // 处理逗号后的空格
    result = addCommaSpaces(result);
    
    // 处理括号周围的空格
    result = addParenthesesSpaces(result);
    
    return result;
}

// 添加运算符周围的空格
std::string CHFormatter::addOperatorSpaces(const std::string& line) {
    std::string result;
    bool inString = false;
    char stringChar = '\0';
    
    for (size_t i = 0; i < line.length(); ++i) {
        char c = line[i];
        
        // 处理字符串
        if ((c == '"' || c == '\'')) {
            if (!inString) {
                inString = true;
                stringChar = c;
            } else if (c == stringChar) {
                inString = false;
                stringChar = '\0';
            }
            result += c;
            continue;
        }
        
        if (inString) {
            result += c;
            continue;
        }
        
        // 处理运算符
        if (c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c == '^') {
            // 检查是否是负号或正号（前面是运算符或左括号）
            bool isUnary = false;
            if (!result.empty()) {
                char lastChar = result.back();
                if (lastChar == '+' || lastChar == '-' || lastChar == '*' || lastChar == '/' || 
                    lastChar == '%' || lastChar == '^' || lastChar == '=' || lastChar == '<' || 
                    lastChar == '>' || lastChar == '!' || lastChar == '(') {
                    isUnary = true;
                }
            }
            
            if (!isUnary && !result.empty() && result.back() != ' ') {
                result += ' ';
            }
            result += c;
            
            // 检查后面是否需要空格
            if (i + 1 < line.length() && line[i + 1] != ' ' && line[i + 1] != ')' && line[i + 1] != ']' && line[i + 1] != ';' && line[i + 1] != ',') {
                result += ' ';
            }
        } else if (c == '=' || c == '!' || c == '<' || c == '>' || c == '&') {
            // 处理多字符运算符
            std::string op;
            op += c;
            
            if (i + 1 < line.length() && (line[i + 1] == '=' || line[i + 1] == '&' || line[i + 1] == '|')) {
                op += line[i + 1];
                i++;
            }
            
            // 检查是否是比较运算符或赋值运算符
            if (op == "=" || op == "==" || op == "!=" || op == "<=" || op == ">=" || op == "<" || op == ">" || op == "&&" || op == "||") {
                // 运算符前添加空格
                if (!result.empty() && result.back() != ' ') {
                    result += ' ';
                }
                result += op;
                // 运算符后添加空格
                if (i + 1 < line.length() && line[i + 1] != ' ' && line[i + 1] != ')' && line[i + 1] != ']' && line[i + 1] != ';' && line[i + 1] != ',') {
                    result += ' ';
                }
            } else {
                result += op;
            }
        } else {
            result += c;
        }
    }
    
    return result;
}

// 添加逗号后的空格
std::string CHFormatter::addCommaSpaces(const std::string& line) {
    std::string result;
    bool inString = false;
    char stringChar = '\0';
    
    for (size_t i = 0; i < line.length(); ++i) {
        char c = line[i];
        
        // 处理字符串
        if ((c == '"' || c == '\'')) {
            if (!inString) {
                inString = true;
                stringChar = c;
            } else if (c == stringChar) {
                inString = false;
                stringChar = '\0';
            }
            result += c;
            continue;
        }
        
        if (inString) {
            result += c;
            continue;
        }
        
        // 处理逗号
        if (c == ',') {
            result += c;
            // 逗号后添加空格
            if (i + 1 < line.length() && line[i + 1] != ' ' && line[i + 1] != '\n') {
                result += ' ';
            }
        } else {
            result += c;
        }
    }
    
    return result;
}

// 添加括号周围的空格
std::string CHFormatter::addParenthesesSpaces(const std::string& line) {
    std::string result;
    bool inString = false;
    char stringChar = '\0';
    
    for (size_t i = 0; i < line.length(); ++i) {
        char c = line[i];
        
        // 处理字符串
        if ((c == '"' || c == '\'')) {
            if (!inString) {
                inString = true;
                stringChar = c;
            } else if (c == stringChar) {
                inString = false;
                stringChar = '\0';
            }
            result += c;
            continue;
        }
        
        if (inString) {
            result += c;
            continue;
        }
        
        // 处理左括号（检查前面是否是关键字）
        if (c == '(' && i > 0) {
            // 检查前面是否是关键字
            bool isKeyword = false;
            for (const auto& keyword : keywords) {
                if (i >= keyword.length()) {
                    std::string beforeParen = line.substr(i - keyword.length(), keyword.length());
                    if (beforeParen == keyword) {
                        isKeyword = true;
                        break;
                    }
                }
            }
            
            if (isKeyword) {
                // 关键字和左括号之间添加空格
                if (line[i - 1] != ' ') {
                    result += ' ';
                }
            }
            result += c;
        } else {
            result += c;
        }
    }
    
    return result;
}

// 检查是否为运算符
bool CHFormatter::isOperator(char c) const {
    return c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c == '^' || 
           c == '=' || c == '<' || c == '>' || c == '!' || c == '&';
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

// 辅助函数实现
bool CHFormatter::isNewline(char c) const {
    return c == '\n' || c == '\r';
}

bool CHFormatter::isQuote(char c) const {
    return c == '"' || c == '\'';
}

bool CHFormatter::isWhitespace(char c) const {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

bool CHFormatter::isAlpha(char c) const {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool CHFormatter::isDigit(char c) const {
    return c >= '0' && c <= '9';
}

bool CHFormatter::isAlnum(char c) const {
    return isAlpha(c) || isDigit(c);
}

bool CHFormatter::isPunct(char c) const {
    return c == ',' || c == ';' || c == ':' || c == '.';
}

bool CHFormatter::isControl(char c) const {
    return c < ' ';
}

bool CHFormatter::isSpace(char c) const {
    return c == ' ' || c == '\t';
}

bool CHFormatter::isBrace(char c) const {
    return c == '{' || c == '}' || c == '[' || c == ']';
}

bool CHFormatter::isSeparator(char c) const {
    return c == ',' || c == ';' || c == ':';
}

bool CHFormatter::isKeyword(const std::string& word) const {
    return keywords.find(word) != keywords.end();
}

bool CHFormatter::startsWith(const std::string& str, const std::string& prefix) const {
    if (str.length() < prefix.length()) {
        return false;
    }
    return str.substr(0, prefix.length()) == prefix;
}

bool CHFormatter::endsWith(const std::string& str, const std::string& suffix) const {
    if (str.length() < suffix.length()) {
        return false;
    }
    return str.substr(str.length() - suffix.length()) == suffix;
}