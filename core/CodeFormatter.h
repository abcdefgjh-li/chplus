#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <sstream>

// 代码格式化工具类
class CodeFormatter {
private:
    std::string source;
    std::ostringstream formatted;
    int currentIndent;
    bool inString;
    bool inComment;
    bool inBlockComment;
    char quoteChar;
    int lineNumber;

public:
    CodeFormatter(const std::string& src) 
        : source(src), currentIndent(0), inString(false), 
          inComment(false), inBlockComment(false), quoteChar('\0'), lineNumber(1) {}

    std::string format();

private:
    bool isNewline(char c) const;
    bool isQuote(char c) const;
    bool isWhitespace(char c) const;
    bool isAlpha(char c) const;
    bool isDigit(char c) const;
    bool isAlphaNum(char c) const;
    bool isOperator(char c) const;

    void handleNewline();
    void handleString(char c, size_t& pos);
    void handleQuote(char c);
    void handleComment(char c, size_t& pos);
    void handleNormalChar(char c, size_t pos);
    void skipWhitespace(size_t& pos);
    void handleIndentAdjust(char c, size_t pos);
    void addSpacesAround();
};