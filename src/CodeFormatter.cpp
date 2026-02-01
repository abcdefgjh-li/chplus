#include "../include/CodeFormatter.h"
#include <algorithm>
#include <cctype>

bool CodeFormatter::isNewline(char c) const {
    return c == '\n' || c == '\r';
}

bool CodeFormatter::isQuote(char c) const {
    return c == '"' || c == '\'';
}

bool CodeFormatter::isWhitespace(char c) const {
    return c == ' ' || c == '\t' || c == '\v' || c == '\f';
}

bool CodeFormatter::isAlpha(char c) const {
    return std::isalpha(static_cast<unsigned char>(c));
}

bool CodeFormatter::isDigit(char c) const {
    return std::isdigit(static_cast<unsigned char>(c));
}

bool CodeFormatter::isAlphaNum(char c) const {
    return isAlpha(c) || isDigit(c);
}

std::string CodeFormatter::format() {
    formatted.str("");
    formatted.clear();
    currentIndent = 0;
    inString = false;
    inComment = false;
    inBlockComment = false;
    quoteChar = '\0';
    lineNumber = 1;

    size_t pos = 0;
    while (pos < source.length()) {
        char c = source[pos];
        
        if (isNewline(c)) {
            handleNewline();
            pos++;
            continue;
        }

        if (inComment || inBlockComment) {
            handleComment(c, pos);
            continue;
        }

        if (inString) {
            handleString(c, pos);
            continue;
        }

        if (isQuote(c)) {
            handleQuote(c);
            pos++;
            continue;
        }

        handleNormalChar(c, pos);
        pos++;
    }

    return formatted.str();
}

void CodeFormatter::handleNewline() {
    // 清除尾随空格
    std::string line = formatted.str();
    while (!line.empty() && (line.back() == ' ' || line.back() == '\t')) {
        line.pop_back();
    }
    formatted.str(line);
    formatted.seekp(0, std::ios::end);
    
    formatted << "\n";
    
    // 为下一行添加适当的缩进
    for (int i = 0; i < currentIndent * 4; i++) {
        formatted << " ";
    }
    
    lineNumber++;
}

void CodeFormatter::handleString(char c, size_t& pos) {
    formatted << c;
    
    // 检查转义字符
    if (c == '\\' && pos + 1 < source.length()) {
        formatted << source[pos + 1];
        pos++;
        return;
    }
    
    // 检查字符串结束
    if (c == quoteChar) {
        inString = false;
        quoteChar = '\0';
    }
}

void CodeFormatter::handleQuote(char c) {
    inString = true;
    quoteChar = c;
    formatted << c;
}

void CodeFormatter::handleComment(char c, size_t& pos) {
    formatted << c;
    
    if (inBlockComment) {
        if (c == '*' && pos + 1 < source.length() && source[pos + 1] == '/') {
            formatted << '/';
            pos++;
            inBlockComment = false;
        }
    } else if (inComment) {
        if (c == '\n') {
            inComment = false;
        }
    } else if (c == '/' && pos + 1 < source.length()) {
        if (source[pos + 1] == '/') {
            formatted << '/';
            pos++;
            inComment = true;
        } else if (source[pos + 1] == '*') {
            formatted << '*';
            pos++;
            inBlockComment = true;
        }
    }
}

void CodeFormatter::handleNormalChar(char c, size_t pos) {
    // 跳过多余的空格
    if (isWhitespace(c)) {
        skipWhitespace(pos);
        return;
    }

    // 检查注释
    if (c == '/' && pos + 1 < source.length()) {
        if (source[pos + 1] == '/' || source[pos + 1] == '*') {
            handleComment(c, pos);
            return;
        }
    }

    formatted << c;

    // 处理特殊符号的缩进
    handleIndentAdjust(c, pos);
}

void CodeFormatter::skipWhitespace(size_t& pos) {
    // 找到下一个非空白字符
    while (pos < source.length() && isWhitespace(source[pos])) {
        pos++;
    }
    pos--; // 回退一个，因为主循环会前进
    
    // 在非空白字符前添加单个空格
    if (pos + 1 < source.length() && !isWhitespace(source[pos + 1])) {
        formatted << " ";
    }
}

void CodeFormatter::handleIndentAdjust(char c, size_t pos) {
    // 在运算符前后添加空格
    if (isOperator(c)) {
        addSpacesAround();
    }
    
    // 处理括号缩进
    if (c == '{') {
        formatted << "\n";
        currentIndent++;
        for (int i = 0; i < currentIndent * 4; i++) {
            formatted << " ";
        }
    } else if (c == '}') {
        currentIndent--;
        if (currentIndent < 0) currentIndent = 0; // 防止负缩进
        
        // 如果不是行首，添加换行
        std::string line = formatted.str();
        if (!line.empty() && line.back() != '\n') {
            formatted << "\n";
        }
        
        for (int i = 0; i < currentIndent * 4; i++) {
            formatted << " ";
        }
        formatted << "}";
    }
}

bool CodeFormatter::isOperator(char c) const {
    return c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || 
           c == '=' || c == '<' || c == '>' || c == '&' || c == '|' ||
           c == '!' || c == '?' || c == ':' || c == ';' || c == ',';
}

void CodeFormatter::addSpacesAround() {
    std::string line = formatted.str();
    if (!line.empty() && !isWhitespace(line.back())) {
        formatted << " ";
    }
}