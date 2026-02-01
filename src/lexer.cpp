#include "../include/lexer.h"
#include <cctype>
#include <map>

// 关键字映射表
static std::map<std::string, TokenType> keywords = {
    {"定义", TokenType::DEFINE},
    {"整型", TokenType::INTEGER},
    {"字符串", TokenType::STRING},
    {"字符型", TokenType::CHAR},
    {"空类型", TokenType::VOID},
    {"主函数", TokenType::MAIN},
    {"如果", TokenType::IF},
    {"否则", TokenType::ELSE},
    {"否则如果", TokenType::ELSE_IF},
    {"控制台输出", TokenType::COUT},
    {"控制台输入", TokenType::CIN},
    {"小数", TokenType::DOUBLE},
    {"布尔型", TokenType::BOOLEAN},
    {"真", TokenType::BOOLEAN_LITERAL},
    {"假", TokenType::BOOLEAN_LITERAL},
    {"结构体", TokenType::STRUCT},
    {"当", TokenType::WHILE},
    {"对于", TokenType::FOR},
    {"返回", TokenType::RETURN},
    {"文件读取", TokenType::FILE_READ},
    {"文件写入", TokenType::FILE_WRITE},
    {"文件追加", TokenType::FILE_APPEND},
    {"导入", TokenType::IMPORT},
    {"数组", TokenType::ARRAY},
    {"系统命令行", TokenType::SYSTEM_CMD},
    {"和", TokenType::LOGICAL_AND},
    {"或", TokenType::LOGICAL_OR},
    {"或者", TokenType::LOGICAL_OR}
};

// 构造函数
Lexer::Lexer(const std::string& source)
    : source(source), position(0), line(1), column(1) {}

// 预览下一个字符
char Lexer::peek() const {
    if (position < source.length()) {
        return source[position];
    }
    return '\0';
}

// 前进到下一个字符
char Lexer::advance() {
    if (position < source.length()) {
        char c = source[position++];
        if (c == '\n') {
            line++;
            column = 1;
        } else {
            column++;
        }
        return c;
    }
    return '\0';
}

// 跳过空白字符和注释
void Lexer::skipWhitespace() {
    while (position < source.length()) {
        char c = source[position];
        
        // 跳过空白字符
        if (std::isspace(static_cast<unsigned char>(c))) {
            advance();
        }
        // 跳过单行注释
        else if (c == '/' && position + 1 < source.length() && source[position + 1] == '/') {
            // 跳过直到行尾
            while (position < source.length() && source[position] != '\n') {
                advance();
            }
        }
        else {
            break;
        }
    }
}

// 解析标识符或关键字
Token Lexer::identifier() {
    size_t start = position;
    int startLine = line;
    int startColumn = column;
    
    // 解析标识符（支持中文）
    while (position < source.length()) {
        char c = source[position];
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_' || 
            (static_cast<unsigned char>(c) >= 0x80)) { // 支持中文
            advance();
        } else {
            break;
        }
    }
    
    std::string text = source.substr(start, position - start);
    
    // 检查是否是关键字
    if (keywords.find(text) != keywords.end()) {
        return Token(keywords[text], text, startLine, startColumn);
    }
    
    // 特殊处理：检查是否是 "否则如果" 组合（不带空格的情况）
    if (text == "否则如果") {
        return Token(TokenType::ELSE_IF, "否则如果", startLine, startColumn);
    }
    
    // 特殊处理：检查是否是 "否则 如果" 组合（有空格的情况）
    if (text == "否则" && position < source.length()) {
        // 保存当前位置
        size_t savedPos = position;
        int savedLine = line;
        int savedColumn = column;
        
        // 跳过空白字符
        skipWhitespace();
        
        // 检查后面是否是 "如果"
        if (position + 2 <= source.length() && source.substr(position, 2) == "如果") {
            // 解析 "如果"
            position += 2;
            column += 2;
            return Token(TokenType::ELSE_IF, "否则如果", startLine, startColumn);
        } else {
            // 如果不是 "如果"，恢复位置
            position = savedPos;
            line = savedLine;
            column = savedColumn;
        }
    }
    
    return Token(TokenType::IDENTIFIER, text, startLine, startColumn);
}

// 解析数字（支持整数和小数）
Token Lexer::number() {
    size_t start = position;
    int startLine = line;
    int startColumn = column;
    
    // 解析整数部分
    while (position < source.length() && std::isdigit(static_cast<unsigned char>(source[position]))) {
        advance();
    }
    
    // 解析小数部分
    if (position < source.length() && source[position] == '.') {
        advance();
        while (position < source.length() && std::isdigit(static_cast<unsigned char>(source[position]))) {
            advance();
        }
    }
    
    std::string text = source.substr(start, position - start);
    return Token(TokenType::INTEGER_LITERAL, text, startLine, startColumn);
}

// 解析字符串
Token Lexer::string() {
    size_t start = position;
    int startLine = line;
    int startColumn = column;
    
    advance(); // 跳过开始的引号
    
    std::string result = "";
    
    while (position < source.length() && source[position] != '"') {
        if (source[position] == '\\') {
            advance(); // 跳过反斜杠
            if (position < source.length()) {
                // 处理转义字符
                switch (source[position]) {
                    case 'n':
                        result += '\n';
                        break;
                    case 't':
                        result += '\t';
                        break;
                    case 'r':
                        result += '\r';
                        break;
                    case '\\':
                        result += '\\';
                        break;
                    case '"':
                        result += '"';
                        break;
                    default:
                        result += source[position];
                        break;
                }
                advance();
            }
        } else {
            result += source[position];
            advance();
        }
    }
    
    if (position < source.length()) {
        advance(); // 跳过结束的引号
    }
    
    return Token(TokenType::STRING_LITERAL, result, startLine, startColumn);
}

// 解析字符
Token Lexer::character() {
    size_t start = position;
    int startLine = line;
    int startColumn = column;
    
    advance(); // 跳过开始的单引号
    
    std::string result = "";
    
    if (position < source.length() && source[position] != '\'') {
        // 处理转义字符
        if (source[position] == '\\') {
            advance(); // 跳过反斜杠
            if (position < source.length()) {
                switch (source[position]) {
                    case 'n':
                        result += '\n';
                        break;
                    case 't':
                        result += '\t';
                        break;
                    case 'r':
                        result += '\r';
                        break;
                    case '\\':
                        result += '\\';
                        break;
                    case '\'':
                        result += '\'';
                        break;
                    default:
                        result += source[position];
                        break;
                }
                advance();
            }
        } else {
            result += source[position];
            advance();
        }
    }
    
    if (position < source.length() && source[position] == '\'') {
        advance(); // 跳过结束的单引号
    }
    
    return Token(TokenType::CHAR_LITERAL, result, startLine, startColumn);
}

// 词法分析主函数
std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    
    while (position < source.length()) {
        skipWhitespace();
        
        if (position >= source.length()) {
            break;
        }
        
        char c = source[position];
        
        // 处理数字
        if (std::isdigit(static_cast<unsigned char>(c))) {
            tokens.push_back(number());
        }
        // 处理字符串
        else if (c == '"') {
            tokens.push_back(string());
        }
        // 处理字符
        else if (c == '\'') {
            tokens.push_back(character());
        }
        // 处理标识符或关键字
        else if (std::isalpha(static_cast<unsigned char>(c)) || c == '_' || 
                 (static_cast<unsigned char>(c) >= 0x80)) { // 支持中文
            tokens.push_back(identifier());
        }
        // 处理运算符
        else if (c == '=') {
            advance();
            if (position < source.length() && source[position] == '=') {
                tokens.push_back(Token(TokenType::EQUAL, "==", line, column));
                advance();
            } else {
                tokens.push_back(Token(TokenType::ASSIGN, "=", line, column));
            }
        }
        else if (c == '+') {
            tokens.push_back(Token(TokenType::PLUS, "+", line, column));
            advance();
        }
        else if (c == '-') {
            tokens.push_back(Token(TokenType::MINUS, "-", line, column));
            advance();
        }
        else if (c == '*') {
            tokens.push_back(Token(TokenType::MULTIPLY, "*", line, column));
            advance();
        }
        else if (c == '^') {
            tokens.push_back(Token(TokenType::POWER, "^", line, column));
            advance();
        }
        else if (c == '%') {
            tokens.push_back(Token(TokenType::MODULO, "%", line, column));
            advance();
        }
        else if (c == '/') {
            // 检查是否是注释
            if (position + 1 < source.length() && source[position + 1] == '/') {
                // 跳过注释
                while (position < source.length() && source[position] != '\n') {
                    advance();
                }
            } else {
                tokens.push_back(Token(TokenType::DIVIDE, "/", line, column));
                advance();
            }
        }
        else if (c == '<') {
            advance();
            if (position < source.length() && source[position] == '=') {
                tokens.push_back(Token(TokenType::LESS_EQUAL, "<=", line, column));
                advance();
            } else {
                tokens.push_back(Token(TokenType::LESS, "<", line, column));
            }
        }
        else if (c == '>') {
            advance();
            if (position < source.length() && source[position] == '=') {
                tokens.push_back(Token(TokenType::GREATER_EQUAL, ">=", line, column));
                advance();
            } else {
                tokens.push_back(Token(TokenType::GREATER, ">", line, column));
            }
        }
        else if (c == '!') {
            advance();
            if (position < source.length() && source[position] == '=') {
                tokens.push_back(Token(TokenType::NOT_EQUAL, "!=", line, column));
                advance();
            }
        }
        else if (c == '&') {
            advance();
            if (position < source.length() && source[position] == '&') {
                tokens.push_back(Token(TokenType::LOGICAL_AND, "&&", line, column));
                advance();
            }
        }
        else if (c == '|') {
            advance();
            if (position < source.length() && source[position] == '|') {
                tokens.push_back(Token(TokenType::LOGICAL_OR, "||", line, column));
                advance();
            }
        }
        // 处理分隔符
        else if (c == '(' || (position + 2 < source.length() && 
                 static_cast<unsigned char>(source[position]) == 0xEF && 
                 static_cast<unsigned char>(source[position+1]) == 0xBC && 
                 static_cast<unsigned char>(source[position+2]) == 0x88)) { // 全角左括号（
            tokens.push_back(Token(TokenType::LPAREN, "(", line, column));
            if (c == '(') {
                advance();
            } else {
                // 跳过UTF-8字符的3个字节
                position += 3;
                column += 3;
            }
        }
        else if (c == ')' || (position + 2 < source.length() && 
                 static_cast<unsigned char>(source[position]) == 0xEF && 
                 static_cast<unsigned char>(source[position+1]) == 0xBC && 
                 static_cast<unsigned char>(source[position+2]) == 0x89)) { // 全角右括号）
            tokens.push_back(Token(TokenType::RPAREN, ")", line, column));
            if (c == ')') {
                advance();
            } else {
                // 跳过UTF-8字符的3个字节
                position += 3;
                column += 3;
            }
        }
        else if (c == '{') {
            tokens.push_back(Token(TokenType::LBRACE, "{", line, column));
            advance();
        }
        else if (c == '}') {
            tokens.push_back(Token(TokenType::RBRACE, "}", line, column));
            advance();
        }
        else if (c == '[') {
            tokens.push_back(Token(TokenType::LBRACKET, "[", line, column));
            advance();
        }
        else if (c == ']') {
            tokens.push_back(Token(TokenType::RBRACKET, "]", line, column));
            advance();
        }
        else if (c == ',' || (position + 2 < source.length() && 
                 static_cast<unsigned char>(source[position]) == 0xEF && 
                 static_cast<unsigned char>(source[position+1]) == 0xBC && 
                 static_cast<unsigned char>(source[position+2]) == 0x8C)) { // 全角逗号，
            tokens.push_back(Token(TokenType::COMMA, ",", line, column));
            if (c == ',') {
                advance();
            } else {
                // 跳过UTF-8字符的3个字节
                position += 3;
                column += 3;
            }
        }
        else if (c == ';' || (position + 2 < source.length() && 
                 static_cast<unsigned char>(source[position]) == 0xEF && 
                 static_cast<unsigned char>(source[position+1]) == 0xBC && 
                 static_cast<unsigned char>(source[position+2]) == 0x9B)) { // 全角分号；
            tokens.push_back(Token(TokenType::SEMICOLON, ";", line, column));
            if (c == ';') {
                advance();
            } else {
                // 跳过UTF-8字符的3个字节
                position += 3;
                column += 3;
            }
        }
        else if (c == '.') {
            tokens.push_back(Token(TokenType::DOT, ".", line, column));
            advance();
        }
        // 处理其他字符（跳过）
        else {
            advance();
        }
    }
    
    // 添加EOF token
    tokens.push_back(Token(TokenType::EOF_TOKEN, "", line, column));
    
    return tokens;
}