#pragma once
#include <string>
#include <sstream>
#include <unordered_set>
#include <vector>

// CH 代码格式化工具类
class CHFormatter {
private:
    std::string source;
    std::ostringstream formatted;
    bool inString;
    bool inComment;
    bool inBlockComment;
    char quoteChar;
    int currentIndent;
    int lineNumber;
    std::unordered_set<std::string> keywords;

public:
    CHFormatter(const std::string& src);
    
    std::string format(bool autoFormat = true, bool noFormat = false);

private:
    void initializeKeywords();
    std::string formatWithPreprocessing();
    std::string formatWithoutPreprocessing();
    void preprocessWhitespace(std::string& code);
    std::string replacePattern(const std::string& code, const std::string& from, const std::string& to);
    void replaceChineseSymbols(std::string& code);
    void processFormatting(std::string& code);
    void applyIndentation(std::string& code);
    std::string processLineContent(const std::string& line, int indentLevel);
    std::string addOperatorSpaces(const std::string& line);
    std::string addCommaSpaces(const std::string& line);
    std::string addParenthesesSpaces(const std::string& line);
    void trim(std::string& str);
    std::vector<std::string> splitLines(const std::string& str);
    bool isNewline(char c) const;
    bool isQuote(char c) const;
    bool isWhitespace(char c) const;
    bool isAlpha(char c) const;
    bool isDigit(char c) const;
    bool isAlnum(char c) const;
    bool isPunct(char c) const;
    bool isControl(char c) const;
    bool isSpace(char c) const;
    bool isBrace(char c) const;
    bool isOperator(char c) const;
    bool isSeparator(char c) const;
    bool isKeyword(const std::string& word) const;
    bool startsWith(const std::string& str, const std::string& prefix) const;
    bool endsWith(const std::string& str, const std::string& suffix) const;
};