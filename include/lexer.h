#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>
#include <iostream>

// Token类型枚举
enum class TokenType {
    // 关键字
    DEFINE,          // 定义
    INTEGER,         // 整型
    STRING,          // 字符串
    CHAR,            // 字符型
    VOID,            // 空类型
    MAIN,            // 主函数
    IF,              // 如果
    ELSE,            // 否则
    ELSE_IF,         // 否则如果
    COUT,            // 控制台输出
    CIN,             // 控制台输入
    COUT_NEWLINE,    // 控制台换行
    DOUBLE,          // 小数
    BOOLEAN,         // 布尔型
    STRUCT,          // 结构体
    ARRAY,           // 数组
    WHILE,           // 当（循环）
    FOR,             // 对于（循环）
    BREAK,           // 退出循环
    CONTINUE,        // 下一层循环
    RETURN,          // 返回
    
    // 文件操作关键字
    FILE_READ,       // 文件读取
    FILE_WRITE,      // 文件写入
    FILE_APPEND,     // 文件追加
    
    // 模块导入关键字
    IMPORT,          // 导入
    
    // 系统命令关键字
    SYSTEM_CMD,      // 系统命令行
    
    // 逻辑运算符
    LOGICAL_AND,     // 和 (&&)
    LOGICAL_OR,      // 或 (||)
    LOGICAL_NOT,     // 逻辑非 (!)
    
    // 标识符
    IDENTIFIER,      // 标识符
    
    // 字面量
    INTEGER_LITERAL, // 整数字面量
    NUMBER = INTEGER_LITERAL, // 兼容别名
    STRING_LITERAL,  // 字符串字面量
    CHAR_LITERAL,   // 字符字面量
    DOUBLE_LITERAL,   // 小数字面量
    BOOLEAN_LITERAL, // 布尔字面量
    
    // 运算符
    PLUS,            // +
    MINUS,           // -
    MULTIPLY,        // *
    DIVIDE,          // /
    ASSIGN,          // =
    MODULO,          // %
    POWER,           // ^
    LESS,            // <
    GREATER,         // >
    LESS_EQUAL,      // <=
    GREATER_EQUAL,   // >=
    EQUAL,           // ==
    NOT_EQUAL,       // !=
    
    // 复合赋值运算符
    COMPOUND_ADD,    // +=
    COMPOUND_SUB,    // -=
    COMPOUND_MUL,    // *=
    COMPOUND_DIV,    // /=
    COMPOUND_MOD,    // %=
    COMPOUND_POW,    // ^=
    
    // 自增自减运算符
    INCREMENT,       // ++
    DECREMENT,       // --
    
    // 分隔符
    LPAREN,          // (
    RPAREN,          // )
    LBRACE,          // {
    RBRACE,          // }
    LBRACKET,        // [
    RBRACKET,        // ]
    DOT,             // .
    COMMA,           // ,
    SEMICOLON,       // ;
    
    // 结束符
    EOF_TOKEN
};

// Token结构体
struct Token {
    TokenType type;
    std::string value;
    int line;
    int column;
    
    Token(TokenType type, const std::string& value, int line, int column)
        : type(type), value(value), line(line), column(column) {}
    
    void print() const {
        std::cout << "Token{type: ";
        switch (type) {
            case TokenType::DEFINE: std::cout << "DEFINE";
                break;
            case TokenType::INTEGER: std::cout << "INTEGER";
                break;
            case TokenType::STRING: std::cout << "STRING";
                break;
            case TokenType::VOID: std::cout << "VOID";
                break;
            case TokenType::MAIN: std::cout << "MAIN";
                break;
            case TokenType::IF: std::cout << "IF";
                break;
            case TokenType::ELSE: std::cout << "ELSE";
                break;
            case TokenType::ELSE_IF: std::cout << "ELSE_IF";
                break;
            case TokenType::COUT: std::cout << "COUT";
                break;
            case TokenType::IDENTIFIER: std::cout << "IDENTIFIER";
                break;
            case TokenType::INTEGER_LITERAL: std::cout << "INTEGER_LITERAL";
                break;
            case TokenType::STRING_LITERAL: std::cout << "STRING_LITERAL";
                break;
            case TokenType::PLUS: std::cout << "PLUS";
                break;
            case TokenType::MINUS: std::cout << "MINUS";
                break;
            case TokenType::MULTIPLY: std::cout << "MULTIPLY";
                break;
            case TokenType::DIVIDE: std::cout << "DIVIDE";
                break;
            case TokenType::ASSIGN: std::cout << "ASSIGN";
                break;
            case TokenType::LPAREN: std::cout << "LPAREN";
                break;
            case TokenType::RPAREN: std::cout << "RPAREN";
                break;
            case TokenType::LBRACE: std::cout << "LBRACE";
                break;
            case TokenType::RBRACE: std::cout << "RBRACE";
                break;
            case TokenType::LBRACKET: std::cout << "LBRACKET";
                break;
            case TokenType::RBRACKET: std::cout << "RBRACKET";
                break;
            case TokenType::DOT: std::cout << "DOT";
                break;
            case TokenType::COMMA: std::cout << "COMMA";
                break;
            case TokenType::SEMICOLON: std::cout << "SEMICOLON";
                break;
            case TokenType::BOOLEAN: std::cout << "BOOLEAN";
                break;
            case TokenType::STRUCT: std::cout << "STRUCT";
                break;
            case TokenType::ARRAY: std::cout << "ARRAY";
                break;
            case TokenType::WHILE: std::cout << "WHILE";
                break;
            case TokenType::FOR: std::cout << "FOR";
                break;
            case TokenType::BREAK: std::cout << "BREAK";
                break;
            case TokenType::CONTINUE: std::cout << "CONTINUE";
                break;
            case TokenType::RETURN: std::cout << "RETURN";
                break;
            case TokenType::FILE_READ: std::cout << "FILE_READ";
                break;
            case TokenType::FILE_WRITE: std::cout << "FILE_WRITE";
                break;
            case TokenType::FILE_APPEND: std::cout << "FILE_APPEND";
                break;
            case TokenType::BOOLEAN_LITERAL: std::cout << "BOOLEAN_LITERAL";
                break;
            case TokenType::MODULO: std::cout << "MODULO";
                break;
            case TokenType::LESS: std::cout << "LESS";
                break;
            case TokenType::GREATER: std::cout << "GREATER";
                break;
            case TokenType::LESS_EQUAL: std::cout << "LESS_EQUAL";
                break;
            case TokenType::GREATER_EQUAL: std::cout << "GREATER_EQUAL";
                break;
            case TokenType::EQUAL: std::cout << "EQUAL";
                break;
            case TokenType::NOT_EQUAL: std::cout << "NOT_EQUAL";
                break;
            case TokenType::COMPOUND_ADD: std::cout << "COMPOUND_ADD";
                break;
            case TokenType::COMPOUND_SUB: std::cout << "COMPOUND_SUB";
                break;
            case TokenType::COMPOUND_MUL: std::cout << "COMPOUND_MUL";
                break;
            case TokenType::COMPOUND_DIV: std::cout << "COMPOUND_DIV";
                break;
            case TokenType::COMPOUND_MOD: std::cout << "COMPOUND_MOD";
                break;
            case TokenType::COMPOUND_POW: std::cout << "COMPOUND_POW";
                break;
            case TokenType::INCREMENT: std::cout << "INCREMENT";
                break;
            case TokenType::DECREMENT: std::cout << "DECREMENT";
                break;
            case TokenType::LOGICAL_AND: std::cout << "LOGICAL_AND";
                break;
            case TokenType::LOGICAL_OR: std::cout << "LOGICAL_OR";
                break;
            case TokenType::LOGICAL_NOT: std::cout << "LOGICAL_NOT";
                break;
            case TokenType::CIN: std::cout << "CIN";
                break;
            case TokenType::DOUBLE: std::cout << "DOUBLE";
                break;
            case TokenType::EOF_TOKEN: std::cout << "EOF_TOKEN";
                break;
        }
        std::cout << ", value: '" << value << "', line: " << line << ", column: " << column << "}" << std::endl;
    }
};

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