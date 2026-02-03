#ifndef LEXER_H
#define LEXER_H

#include "common.h"

// 词法分析器类
class Lexer {
private:
    std::string source;
    size_t position;
    int line;
    int column;
    
    char peek() const;
    char advance();
    void skipWhitespace();
    Token identifier();
    Token number();
    Token string();
    Token character();
    
public:
    Lexer(const std::string& source);
    std::vector<Token> tokenize();
};

#endif // LEXER_H