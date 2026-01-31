#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include <vector>
#include <memory>
#include <utility>

// 语法节点类型
enum class NodeType {
    PROGRAM,             // 程序
    VARIABLE_DEF,        // 变量定义
    FUNCTION_DEF,        // 函数定义
    FUNCTION_CALL,       // 函数调用
    RETURN_STATEMENT,    // 返回语句
    ASSIGNMENT,          // 赋值表达式
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
    WHILE_STATEMENT,     // while循环语句
    FOR_STATEMENT,       // for循环语句
    FILE_READ_STATEMENT,  // 文件读取语句
    FILE_WRITE_STATEMENT, // 文件写入语句
    FILE_APPEND_STATEMENT, // 文件追加语句
    IMPORT_STATEMENT,    // 导入语句
    SYSTEM_CMD_STATEMENT, // 系统命令行语句
    SYSTEM_CMD_EXPRESSION, // 系统命令行表达式
    STRUCT_DEF,          // 结构体定义
    STRUCT_MEMBER_ACCESS, // 结构体成员访问
    STRUCT_MEMBER_ASSIGNMENT // 结构体成员赋值
};

// 语法节点基类
class ASTNode {
public:
    NodeType type;
    int line;
    int column;
    
    ASTNode(NodeType type, int line, int column)
        : type(type), line(line), column(column) {}
    
    virtual ~ASTNode() = default;
};

// 程序节点
class ProgramNode : public ASTNode {
public:
    std::vector<std::unique_ptr<ASTNode>> statements;
    
    ProgramNode(int line, int column)
        : ASTNode(NodeType::PROGRAM, line, column) {}
};

// 变量定义节点
class VariableDefNode : public ASTNode {
public:
    std::string type;
    std::string name;
    bool isArray;
    std::unique_ptr<ASTNode> arraySizeExpr; // 支持动态数组大小表达式
    std::unique_ptr<ASTNode> initializer;
    
    VariableDefNode(const std::string& type, const std::string& name, bool isArray, std::unique_ptr<ASTNode> arraySizeExpr, std::unique_ptr<ASTNode> initializer, int line, int column)
        : ASTNode(NodeType::VARIABLE_DEF, line, column), type(type), name(name), isArray(isArray), arraySizeExpr(std::move(arraySizeExpr)), initializer(std::move(initializer)) {}
    
    // 兼容性构造函数（用于向后兼容）
    VariableDefNode(const std::string& type, const std::string& name, std::unique_ptr<ASTNode> initializer, int line, int column)
        : ASTNode(NodeType::VARIABLE_DEF, line, column), type(type), name(name), isArray(false), arraySizeExpr(nullptr), initializer(std::move(initializer)) {}
};

// 函数定义节点
class FunctionDefNode : public ASTNode {
public:
    std::string returnType;
    std::string name;
    std::vector<std::pair<std::string, std::string>> parameters;
    std::unique_ptr<ASTNode> body;
    
    FunctionDefNode(const std::string& returnType, const std::string& name, 
                   const std::vector<std::pair<std::string, std::string>>& parameters, 
                   std::unique_ptr<ASTNode> body, int line, int column)
        : ASTNode(NodeType::FUNCTION_DEF, line, column), 
          returnType(returnType), name(name), parameters(parameters), body(std::move(body)) {}
};

// 赋值表达式节点
class AssignmentNode : public ASTNode {
public:
    std::string name;
    std::unique_ptr<ASTNode> expression;
    
    AssignmentNode(const std::string& name, std::unique_ptr<ASTNode> expression, int line, int column)
        : ASTNode(NodeType::ASSIGNMENT, line, column), name(name), expression(std::move(expression)) {}
};

// 函数调用节点
class FunctionCallNode : public ASTNode {
public:
    std::string functionName;
    std::vector<std::unique_ptr<ASTNode>> arguments;
    
    FunctionCallNode(const std::string& functionName, std::vector<std::unique_ptr<ASTNode>> arguments, int line, int column)
        : ASTNode(NodeType::FUNCTION_CALL, line, column), functionName(functionName), arguments(std::move(arguments)) {}
};

// 返回语句节点
class ReturnStatementNode : public ASTNode {
public:
    std::unique_ptr<ASTNode> expression;
    
    ReturnStatementNode(std::unique_ptr<ASTNode> expression, int line, int column)
        : ASTNode(NodeType::RETURN_STATEMENT, line, column), expression(std::move(expression)) {}
};

// 数组访问赋值节点（支持多维）
class ArrayAssignmentNode : public ASTNode {
public:
    std::string arrayName;
    std::vector<std::unique_ptr<ASTNode>> indices;
    std::unique_ptr<ASTNode> expression;
    
    ArrayAssignmentNode(const std::string& arrayName, std::vector<std::unique_ptr<ASTNode>> indices,
                       std::unique_ptr<ASTNode> expression, int line, int column)
        : ASTNode(NodeType::ARRAY_ASSIGNMENT, line, column), 
          arrayName(arrayName), indices(std::move(indices)), expression(std::move(expression)) {}
};

// 一元表达式节点
class UnaryExpressionNode : public ASTNode {
public:
    std::string op;
    std::unique_ptr<ASTNode> operand;
    
    UnaryExpressionNode(const std::string& op, std::unique_ptr<ASTNode> operand, int line, int column)
        : ASTNode(NodeType::UNARY_EXPRESSION, line, column), op(op), operand(std::move(operand)) {}
};

// 二元表达式节点
class BinaryExpressionNode : public ASTNode {
public:
    std::string op;
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;
    
    BinaryExpressionNode(const std::string& op, std::unique_ptr<ASTNode> left, std::unique_ptr<ASTNode> right, int line, int column)
        : ASTNode(NodeType::BINARY_EXPRESSION, line, column), op(op), left(std::move(left)), right(std::move(right)) {}
};

// 字面量节点
class LiteralNode : public ASTNode {
public:
    std::string value;
    std::string literalType;
    
    LiteralNode(const std::string& value, const std::string& literalType, int line, int column)
        : ASTNode(NodeType::LITERAL, line, column), value(value), literalType(literalType) {}
};

// 标识符节点
class IdentifierNode : public ASTNode {
public:
    std::string name;
    
    IdentifierNode(const std::string& name, int line, int column)
        : ASTNode(NodeType::IDENTIFIER, line, column), name(name) {}
};

// 数组访问节点（支持多维）
class ArrayAccessNode : public ASTNode {
public:
    std::string arrayName;
    std::vector<std::unique_ptr<ASTNode>> indices;
    
    ArrayAccessNode(const std::string& arrayName, std::vector<std::unique_ptr<ASTNode>> indices, int line, int column)
        : ASTNode(NodeType::ARRAY_ACCESS, line, column), arrayName(arrayName), indices(std::move(indices)) {}
};

// 字符串索引访问节点
class StringAccessNode : public ASTNode {
public:
    std::string stringName;
    std::unique_ptr<ASTNode> index;
    
    StringAccessNode(const std::string& stringName, std::unique_ptr<ASTNode> index, int line, int column)
        : ASTNode(NodeType::STRING_ACCESS, line, column), stringName(stringName), index(std::move(index)) {}
};

// 导入语句节点
class ImportStatementNode : public ASTNode {
public:
    std::string filePath;
    
    ImportStatementNode(const std::string& filePath, int line, int column)
        : ASTNode(NodeType::IMPORT_STATEMENT, line, column), filePath(filePath) {}
};

// 系统命令行语句节点
class SystemCmdStatementNode : public ASTNode {
public:
    std::unique_ptr<ASTNode> commandExpr;
    
    SystemCmdStatementNode(std::unique_ptr<ASTNode> commandExpr, int line, int column)
        : ASTNode(NodeType::SYSTEM_CMD_STATEMENT, line, column), commandExpr(std::move(commandExpr)) {}
};

// 系统命令行表达式节点（返回命令结果）
class SystemCmdExpressionNode : public ASTNode {
public:
    std::unique_ptr<ASTNode> commandExpr;
    
    SystemCmdExpressionNode(std::unique_ptr<ASTNode> commandExpr, int line, int column)
        : ASTNode(NodeType::SYSTEM_CMD_EXPRESSION, line, column), commandExpr(std::move(commandExpr)) {}
};

// 语句列表节点
class StatementListNode : public ASTNode {
public:
    std::vector<std::unique_ptr<ASTNode>> statements;
    
    StatementListNode(int line, int column)
        : ASTNode(NodeType::STATEMENT_LIST, line, column) {}
};

// 控制台输出语句节点
class CoutStatementNode : public ASTNode {
public:
    std::unique_ptr<ASTNode> expression;
    
    CoutStatementNode(std::unique_ptr<ASTNode> expression, int line, int column)
        : ASTNode(NodeType::COUT_STATEMENT, line, column), expression(std::move(expression)) {}
};

// 控制台输入语句节点
class CinStatementNode : public ASTNode {
public:
    std::unique_ptr<ASTNode> expression;
    
    CinStatementNode(std::unique_ptr<ASTNode> expression, int line, int column)
        : ASTNode(NodeType::CIN_STATEMENT, line, column), expression(std::move(expression)) {}
};

// While循环语句节点
class WhileStatementNode : public ASTNode {
public:
    std::unique_ptr<ASTNode> condition;
    std::unique_ptr<ASTNode> body;
    
    WhileStatementNode(std::unique_ptr<ASTNode> condition, std::unique_ptr<ASTNode> body, int line, int column)
        : ASTNode(NodeType::WHILE_STATEMENT, line, column), condition(std::move(condition)), body(std::move(body)) {}
};

// For循环语句节点
class ForStatementNode : public ASTNode {
public:
    std::unique_ptr<ASTNode> initialization;
    std::unique_ptr<ASTNode> condition;
    std::unique_ptr<ASTNode> update;
    std::unique_ptr<ASTNode> body;
    
    ForStatementNode(std::unique_ptr<ASTNode> initialization, std::unique_ptr<ASTNode> condition, 
                    std::unique_ptr<ASTNode> update, std::unique_ptr<ASTNode> body, int line, int column)
        : ASTNode(NodeType::FOR_STATEMENT, line, column), 
          initialization(std::move(initialization)), condition(std::move(condition)), 
          update(std::move(update)), body(std::move(body)) {}
};

// If语句节点
class IfStatementNode : public ASTNode {
public:
    std::unique_ptr<ASTNode> condition;
    std::unique_ptr<ASTNode> thenBranch;
    std::unique_ptr<ASTNode> elseBranch;
    
    IfStatementNode(std::unique_ptr<ASTNode> condition, std::unique_ptr<ASTNode> thenBranch, 
                   std::unique_ptr<ASTNode> elseBranch, int line, int column)
        : ASTNode(NodeType::IF_STATEMENT, line, column), 
          condition(std::move(condition)), thenBranch(std::move(thenBranch)), 
          elseBranch(std::move(elseBranch)) {}
};

// ElseIf语句节点
class ElseIfStatementNode : public ASTNode {
public:
    std::unique_ptr<ASTNode> condition;
    std::unique_ptr<ASTNode> thenBranch;
    std::unique_ptr<ASTNode> elseBranch;
    
    ElseIfStatementNode(std::unique_ptr<ASTNode> condition, std::unique_ptr<ASTNode> thenBranch, 
                       std::unique_ptr<ASTNode> elseBranch, int line, int column)
        : ASTNode(NodeType::ELSE_IF_STATEMENT, line, column), 
          condition(std::move(condition)), thenBranch(std::move(thenBranch)), 
          elseBranch(std::move(elseBranch)) {}
};

// 文件读取语句节点
class FileReadStatementNode : public ASTNode {
public:
    std::unique_ptr<ASTNode> filename;      // 文件名（第一个参数）
    std::unique_ptr<ASTNode> variableName;  // 变量名（第二个参数）
    
    FileReadStatementNode(std::unique_ptr<ASTNode> filename, std::unique_ptr<ASTNode> variableName, int line, int column)
        : ASTNode(NodeType::FILE_READ_STATEMENT, line, column), filename(std::move(filename)), variableName(std::move(variableName)) {}
};

// 文件写入语句节点
class FileWriteStatementNode : public ASTNode {
public:
    std::unique_ptr<ASTNode> filename;
    std::unique_ptr<ASTNode> content;
    
    FileWriteStatementNode(std::unique_ptr<ASTNode> filename, std::unique_ptr<ASTNode> content, int line, int column)
        : ASTNode(NodeType::FILE_WRITE_STATEMENT, line, column), filename(std::move(filename)), content(std::move(content)) {}
};

// 文件追加语句节点
class FileAppendStatementNode : public ASTNode {
public:
    std::unique_ptr<ASTNode> filename;
    std::unique_ptr<ASTNode> content;
    
    FileAppendStatementNode(std::unique_ptr<ASTNode> filename, std::unique_ptr<ASTNode> content, int line, int column)
        : ASTNode(NodeType::FILE_APPEND_STATEMENT, line, column), filename(std::move(filename)), content(std::move(content)) {}
};

// 结构体定义节点
class StructDefNode : public ASTNode {
public:
    std::string structName;
    std::vector<std::pair<std::string, std::string>> members;  // 成员名和类型
    
    StructDefNode(const std::string& structName, const std::vector<std::pair<std::string, std::string>>& members, int line, int column)
        : ASTNode(NodeType::STRUCT_DEF, line, column), structName(structName), members(members) {}
};

// 结构体成员访问节点
class StructMemberAccessNode : public ASTNode {
public:
    std::unique_ptr<ASTNode> structExpr;
    std::string memberName;
    
    StructMemberAccessNode(std::unique_ptr<ASTNode> structExpr, const std::string& memberName, int line, int column)
        : ASTNode(NodeType::STRUCT_MEMBER_ACCESS, line, column), structExpr(std::move(structExpr)), memberName(memberName) {}
};

// 结构体成员赋值节点
class StructMemberAssignmentNode : public ASTNode {
public:
    std::unique_ptr<ASTNode> structExpr;
    std::string memberName;
    std::unique_ptr<ASTNode> expression;
    
    StructMemberAssignmentNode(std::unique_ptr<ASTNode> structExpr, const std::string& memberName, 
                             std::unique_ptr<ASTNode> expression, int line, int column)
        : ASTNode(NodeType::STRUCT_MEMBER_ASSIGNMENT, line, column), 
          structExpr(std::move(structExpr)), memberName(memberName), expression(std::move(expression)) {}
};

// 语法分析器类
class Parser {
private:
    std::vector<Token> tokens;
    size_t current;
    
    bool match(TokenType type);
    bool match(const std::vector<TokenType>& types);
    Token advance();
    Token peek() const;
    Token previous() const;
    bool isAtEnd() const;
    Token consume(TokenType type, const std::string& message);
    
    std::unique_ptr<ASTNode> parseStatement();
    std::unique_ptr<ASTNode> parseVariableDef();
    std::unique_ptr<ASTNode> parseVariableDef(bool consumeSemicolon);
    std::unique_ptr<ASTNode> parseFunctionDef();
    std::unique_ptr<ASTNode> parseExpression();
    std::unique_ptr<ASTNode> parseLogicalOrExpression();
    std::unique_ptr<ASTNode> parseLogicalAndExpression();
    std::unique_ptr<ASTNode> parseComparisonExpression();
    std::unique_ptr<ASTNode> parsePowerExpression();
    std::unique_ptr<ASTNode> parseTerm();
    std::unique_ptr<ASTNode> parseAdditiveExpression();
    std::unique_ptr<ASTNode> parseFactor();
    std::unique_ptr<ASTNode> parsePrimary();
    std::unique_ptr<ASTNode> parseStatementList();
    std::unique_ptr<ASTNode> parseCoutStatement();
    std::unique_ptr<ASTNode> parseCinStatement();
    std::unique_ptr<ASTNode> parseWhileStatement();
    std::unique_ptr<ASTNode> parseForStatement();
    std::unique_ptr<ASTNode> parseIfStatement();
    std::unique_ptr<ASTNode> parseElseIfStatement();
    std::unique_ptr<ASTNode> parseReturnStatement();
    std::unique_ptr<ASTNode> parseFileReadStatement();
    std::unique_ptr<ASTNode> parseFileWriteStatement();
    std::unique_ptr<ASTNode> parseFileAppendStatement();
    std::unique_ptr<ASTNode> parseImportStatement();
    std::unique_ptr<ASTNode> parseSystemCmdStatement();
    std::unique_ptr<ASTNode> parseSystemCmdExpression();
    std::unique_ptr<ASTNode> parseStructDef();
    std::unique_ptr<ASTNode> parseStructMemberAccess();
    std::unique_ptr<ASTNode> parseArrayAccess(const std::string& arrayName);
    
public:
    Parser(const std::vector<Token>& tokens);
    std::unique_ptr<ProgramNode> parse();
    
    // 公开的函数，用于导入文件时解析程序
    std::unique_ptr<ProgramNode> parseProgram();
    
    // 内部解析函数声明
    std::unique_ptr<ASTNode> parseDefinition();
    std::unique_ptr<ASTNode> parseStructDefinition(int line, int column);
    std::unique_ptr<ASTNode> parseFunctionDefCommon(const std::string& type, const std::string& name, int line, int column);
    std::unique_ptr<ASTNode> parseVariableDefCommon(const std::string& type, const std::string& name, int line, int column);
};

#endif // PARSER_H