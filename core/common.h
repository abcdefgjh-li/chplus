#ifndef COMMON_H
#define COMMON_H

#include <string>
#include <vector>
#include <iostream>
#include <stdexcept>

// 前向声明
class ASTNode;
class FunctionDefNode;

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

// 语法节点类型
enum class NodeType {
    PROGRAM,             // 程序
    VARIABLE_DEF,        // 变量定义
    FUNCTION_DEF,        // 函数定义
    FUNCTION_CALL,       // 函数调用
    RETURN_STATEMENT,    // 返回语句
    ASSIGNMENT,          // 赋值表达式
    COMPOUND_ASSIGNMENT, // 复合赋值表达式
    ARRAY_ASSIGNMENT,    // 数组访问赋值
    BINARY_EXPRESSION,   // 二元表达式
    UNARY_EXPRESSION,    // 一元表达式
    LITERAL,             // 字面量
    IDENTIFIER,          // 标识符
    ARRAY_ACCESS,        // 数组访问
    STRING_ACCESS,       // 字符串索引访问
    STATEMENT_LIST,      // 语句列表
    IF_STATEMENT,        // if语句
    ELSE_IF_STATEMENT,   // else if语句
    ELSE_STATEMENT,      // else语句
    COUT_STATEMENT,      // 控制台输出语句
    CIN_STATEMENT,       // 控制台输入语句
    COUT_NEWLINE_STATEMENT, // 控制台换行语句
    WHILE_STATEMENT,     // while循环语句
    FOR_STATEMENT,       // for循环语句
    BREAK_STATEMENT,     // 退出循环语句
    CONTINUE_STATEMENT,  // 下一层循环语句
    FILE_READ_STATEMENT,  // 文件读取语句
    FILE_WRITE_STATEMENT, // 文件写入语句
    FILE_APPEND_STATEMENT, // 文件追加语句
    IMPORT_STATEMENT,    // 导入语句
    SYSTEM_CMD_STATEMENT, // 系统命令行语句
    SYSTEM_CMD_EXPRESSION, // 系统命令行表达式
    STRUCT_DEF,          // 结构体定义
    STRUCT_MEMBER_ACCESS, // 结构体成员访问
    STRUCT_MEMBER_ASSIGNMENT, // 结构体成员赋值
    BRACE_INIT_LIST      // 大括号初始化列表
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
            case TokenType::DEFINE: std::cout << "DEFINE"; break;
            case TokenType::INTEGER: std::cout << "INTEGER"; break;
            case TokenType::STRING: std::cout << "STRING"; break;
            case TokenType::VOID: std::cout << "VOID"; break;
            case TokenType::MAIN: std::cout << "MAIN"; break;
            case TokenType::IF: std::cout << "IF"; break;
            case TokenType::ELSE: std::cout << "ELSE"; break;
            case TokenType::ELSE_IF: std::cout << "ELSE_IF"; break;
            case TokenType::COUT: std::cout << "COUT"; break;
            case TokenType::IDENTIFIER: std::cout << "IDENTIFIER"; break;
            case TokenType::INTEGER_LITERAL: std::cout << "INTEGER_LITERAL"; break;
            case TokenType::STRING_LITERAL: std::cout << "STRING_LITERAL"; break;
            case TokenType::PLUS: std::cout << "PLUS"; break;
            case TokenType::MINUS: std::cout << "MINUS"; break;
            case TokenType::MULTIPLY: std::cout << "MULTIPLY"; break;
            case TokenType::DIVIDE: std::cout << "DIVIDE"; break;
            case TokenType::ASSIGN: std::cout << "ASSIGN"; break;
            case TokenType::LPAREN: std::cout << "LPAREN"; break;
            case TokenType::RPAREN: std::cout << "RPAREN"; break;
            case TokenType::LBRACE: std::cout << "LBRACE"; break;
            case TokenType::RBRACE: std::cout << "RBRACE"; break;
            case TokenType::LBRACKET: std::cout << "LBRACKET"; break;
            case TokenType::RBRACKET: std::cout << "RBRACKET"; break;
            case TokenType::DOT: std::cout << "DOT"; break;
            case TokenType::COMMA: std::cout << "COMMA"; break;
            case TokenType::SEMICOLON: std::cout << "SEMICOLON"; break;
            case TokenType::BOOLEAN: std::cout << "BOOLEAN"; break;
            case TokenType::STRUCT: std::cout << "STRUCT"; break;
            case TokenType::ARRAY: std::cout << "ARRAY"; break;
            case TokenType::WHILE: std::cout << "WHILE"; break;
            case TokenType::FOR: std::cout << "FOR"; break;
            case TokenType::BREAK: std::cout << "BREAK"; break;
            case TokenType::CONTINUE: std::cout << "CONTINUE"; break;
            case TokenType::RETURN: std::cout << "RETURN"; break;
            case TokenType::FILE_READ: std::cout << "FILE_READ"; break;
            case TokenType::FILE_WRITE: std::cout << "FILE_WRITE"; break;
            case TokenType::FILE_APPEND: std::cout << "FILE_APPEND"; break;
            case TokenType::BOOLEAN_LITERAL: std::cout << "BOOLEAN_LITERAL"; break;
            case TokenType::MODULO: std::cout << "MODULO"; break;
            case TokenType::LESS: std::cout << "LESS"; break;
            case TokenType::GREATER: std::cout << "GREATER"; break;
            case TokenType::LESS_EQUAL: std::cout << "LESS_EQUAL"; break;
            case TokenType::GREATER_EQUAL: std::cout << "GREATER_EQUAL"; break;
            case TokenType::EQUAL: std::cout << "EQUAL"; break;
            case TokenType::NOT_EQUAL: std::cout << "NOT_EQUAL"; break;
            case TokenType::COMPOUND_ADD: std::cout << "COMPOUND_ADD"; break;
            case TokenType::COMPOUND_SUB: std::cout << "COMPOUND_SUB"; break;
            case TokenType::COMPOUND_MUL: std::cout << "COMPOUND_MUL"; break;
            case TokenType::COMPOUND_DIV: std::cout << "COMPOUND_DIV"; break;
            case TokenType::COMPOUND_MOD: std::cout << "COMPOUND_MOD"; break;
            case TokenType::COMPOUND_POW: std::cout << "COMPOUND_POW"; break;
            case TokenType::INCREMENT: std::cout << "INCREMENT"; break;
            case TokenType::DECREMENT: std::cout << "DECREMENT"; break;
            case TokenType::LOGICAL_AND: std::cout << "LOGICAL_AND"; break;
            case TokenType::LOGICAL_OR: std::cout << "LOGICAL_OR"; break;
            case TokenType::LOGICAL_NOT: std::cout << "LOGICAL_NOT"; break;
            case TokenType::CIN: std::cout << "CIN"; break;
            case TokenType::DOUBLE: std::cout << "DOUBLE"; break;
            case TokenType::EOF_TOKEN: std::cout << "EOF_TOKEN"; break;
        }
        std::cout << ", value: '" << value << "', line: " << line << ", column: " << column << "}" << std::endl;
    }
};

// 错误消息定义
namespace ErrorMessages {
    const std::string TYPE_DECLARATION_MISSING_IDENTIFIER = "类型声明后面必须跟着标识符";
    const std::string UNKNOWN_TYPE = "未知类型";
    const std::string DEFINE_MISSING_NAME = "定义必须指定名称";
    const std::string EMPTY_DEFINITION_MISSING_ASSIGNMENT = "空定义必须赋值";
    const std::string VARIABLE_DEFINITION_MISSING_NAME = "变量定义必须指定变量名";
    const std::string INVALID_PARAMETER_TYPE = "无效的参数类型";
    const std::string PARAMETER_MISSING_NAME = "参数必须有名字";
    const std::string ARRAY_PARAMETER_MISSING_SIZE = "数组参数必须指定大小";
    const std::string FUNCTION_PARAMETER_SYNTAX = "函数参数必须使用 '定义(类型) 名字' 语法";
    const std::string ARRAY_SIZE_MUST_BE_CONSTANT = "数组大小必须是整数常量，不能是表达式";
    const std::string ARRAY_DIMENSION_EXCEEDED = "数组维度不能超过5维";
    const std::string UNEXPECTED_TOKEN = "意外的token";
    const std::string MISSING_SEMICOLON = "缺少分号";
    const std::string MISSING_CLOSING_PAREN = "缺少右括号";
    const std::string MISSING_CLOSING_BRACE = "缺少右大括号";
    const std::string MISSING_CLOSING_BRACKET = "缺少右方括号";
    const std::string INVALID_EXPRESSION = "无效的表达式";
    const std::string UNDEFINED_VARIABLE = "未定义的变量";
    const std::string UNDEFINED_FUNCTION = "未定义的函数";
    const std::string UNDEFINED_STRUCT = "未定义的结构体";
    const std::string TYPE_MISMATCH = "类型不匹配";
    const std::string INVALID_RETURN_TYPE = "无效的返回类型";
    const std::string MISSING_RETURN_STATEMENT = "缺少返回语句";
    const std::string BREAK_OUTSIDE_LOOP = "break语句只能在循环中使用";
    const std::string CONTINUE_OUTSIDE_LOOP = "continue语句只能在循环中使用";
    const std::string DUPLICATE_DEFINITION = "重复定义";
    const std::string INVALID_ARRAY_ACCESS = "无效的数组访问";
    const std::string INVALID_STRUCT_MEMBER = "无效的结构体成员";
    const std::string FILE_READ_ERROR = "文件读取错误";
    const std::string FILE_WRITE_ERROR = "文件写入错误";
    const std::string FILE_NOT_FOUND = "文件不存在";
    const std::string CIRCULAR_IMPORT = "循环导入";
    const std::string INVALID_SYSTEM_COMMAND = "无效的系统命令";
}

// 辅助函数：生成带行号和列号的错误消息
inline std::string formatError(const std::string& message, int line, int column = 0) {
    if (column > 0) {
        return message + " 在第 " + std::to_string(line) + " 行, 第 " + std::to_string(column) + " 列";
    }
    return message + " 在第 " + std::to_string(line) + " 行";
}

// 辅助函数：抛出运行时错误
inline void throwError(const std::string& message, int line, int column = 0) {
    throw std::runtime_error(formatError(message, line, column));
}

#endif // COMMON_H