#include "../include/parser.h"
#include <stdexcept>

// 构造函数
Parser::Parser(const std::vector<Token>& tokens)
    : tokens(tokens), current(0) {}

// 检查当前token是否匹配指定类型
bool Parser::match(TokenType type) {
    if (isAtEnd()) {
        return false;
    }
    if (peek().type == type) {
        advance();
        return true;
    }
    return false;
}

// 检查当前token是否匹配指定类型之一
bool Parser::match(const std::vector<TokenType>& types) {
    if (isAtEnd()) {
        return false;
    }
    for (const auto& type : types) {
        if (peek().type == type) {
            advance();
            return true;
        }
    }
    return false;
}

// 前进到下一个token
Token Parser::advance() {
    if (!isAtEnd()) {
        current++;
    }
    return previous();
}

// 预览当前token
Token Parser::peek() const {
    if (isAtEnd()) {
        return Token(TokenType::EOF_TOKEN, "", 0, 0);
    }
    return tokens[current];
}

// 获取前一个token
Token Parser::previous() const {
    if (current > 0) {
        return tokens[current - 1];
    }
    return Token(TokenType::EOF_TOKEN, "", 0, 0);
}

// 检查是否到达文件末尾
bool Parser::isAtEnd() const {
    return current >= tokens.size() || tokens[current].type == TokenType::EOF_TOKEN;
}

// 消费指定类型的token，如果不匹配则抛出错误
Token Parser::consume(TokenType type, const std::string& message) {
    if (match(type)) {
        return previous();
    }
    throw std::runtime_error(message + " 在第 " + std::to_string(peek().line) + " 行, 第 " + std::to_string(peek().column) + " 列");
}

// 解析程序
std::unique_ptr<ProgramNode> Parser::parse() {
    return std::unique_ptr<ProgramNode>(static_cast<ProgramNode*>(parseProgram().release()));
}

// 解析程序
std::unique_ptr<ProgramNode> Parser::parseProgram() {
    auto program = std::make_unique<ProgramNode>(1, 1);
    
    while (!isAtEnd()) {
        program->statements.push_back(parseStatement());
    }
    
    return program;
}

// 解析语句
std::unique_ptr<ASTNode> Parser::parseStatement() {
    if (match(TokenType::DEFINE)) {
        return parseVariableDef();
    }
    
    // 检查是否是函数定义（以类型开头，后面跟着标识符和左括号）
    if (match(TokenType::INTEGER) || match(TokenType::STRING) || match(TokenType::CHAR) ||
        match(TokenType::VOID) || match(TokenType::DOUBLE) || match(TokenType::BOOLEAN) ||
        match(TokenType::STRUCT)) {
        // 类型已经被消费，检查下一个token是否是标识符或主函数关键字
        if (match(TokenType::IDENTIFIER) || match(TokenType::MAIN)) {
            // 检查是否是左括号
            if (match(TokenType::LPAREN)) {
                // 回退到类型token之前，使用parseFunctionDefCommon解析
                current -= 3;
                advance(); // 消费类型token
                Token typeToken = previous();
                advance(); // 消费函数名token
                Token nameToken = previous();
                advance(); // 消费左括号token
                
                // 使用parseFunctionDefCommon解析函数定义
                return parseFunctionDefCommon(typeToken.value, nameToken.value, typeToken.line, typeToken.column);
            } else {
                // 不是函数定义，是C风格的变量定义：类型 变量名 = 值;
                // 回退到类型token之前，使用parseCStyleVariableDef解析
                current -= 2;
                return parseCStyleVariableDef();
            }
        } else {
            throw std::runtime_error("类型声明后面必须跟着标识符 在第 " + std::to_string(previous().line) + " 行");
        }
    }
    
    if (match(TokenType::COUT)) {
        return parseCoutStatement();
    }
    if (match(TokenType::CIN)) {
        return parseCinStatement();
    }
    if (match(TokenType::LBRACE)) {
        return parseStatementList();
    }
    if (match(TokenType::WHILE)) {
        return parseWhileStatement();
    }
    if (match(TokenType::FOR)) {
        return parseForStatement();
    }
    if (match(TokenType::IF)) {
        return parseIfStatement();
    }
    if (match(TokenType::ELSE_IF)) {
        return parseIfStatement();
    }
    if (match(TokenType::RETURN)) {
        return parseReturnStatement();
    }
    if (match(TokenType::FILE_READ)) {
        return parseFileReadStatement();
    }
    if (match(TokenType::FILE_WRITE)) {
        return parseFileWriteStatement();
    }
    if (match(TokenType::FILE_APPEND)) {
        return parseFileAppendStatement();
    }
    if (match(TokenType::IMPORT)) {
        return parseImportStatement();
    }
    
    // 解析表达式语句
    auto expr = parseExpression();
    consume(TokenType::SEMICOLON, "语句必须以分号结束");
    return expr;
}

// 解析定义语句
std::unique_ptr<ASTNode> Parser::parseDefinition() {
    int line = previous().line;
    int column = previous().column;
    
    consume(TokenType::LPAREN, "定义必须以 '(' 开始");
    
    // 解析类型
    std::string type;
    if (match(TokenType::INTEGER)) {
        type = "整型";
    } else if (match(TokenType::STRING)) {
        type = "字符串";
    } else if (match(TokenType::CHAR)) {
        type = "字符型";
    } else if (match(TokenType::VOID)) {
        type = "空类型";
    } else if (match(TokenType::DOUBLE)) {
        type = "小数";
    } else if (match(TokenType::BOOLEAN)) {
        type = "布尔型";
    } else if (match(TokenType::STRUCT)) {
        // 这是结构体定义：定义(结构体) 结构体名 { ... };
        return parseStructDefinition(line, column);
    } else if (match(TokenType::IDENTIFIER)) {
        // 这是自定义类型（结构体类型）：定义(TypeName) 变量名 = ...
        type = previous().value;
    } else {
        std::cout << "未知类型错误: 当前token类型=" << static_cast<int>(peek().type) 
                  << ", 值='" << peek().value << "' 在第 " << peek().line << " 行" << std::endl;
        throw std::runtime_error("未知类型 在第 " + std::to_string(peek().line) + " 行");
    }
    
    consume(TokenType::RPAREN, "类型声明必须以 ')' 结束");
    
    // 检查下一个token
    Token nextToken = peek();
    std::string name;
    
    if (nextToken.type == TokenType::IDENTIFIER) {
        advance(); // 消费标识符
        name = nextToken.value;
    } else if (nextToken.type == TokenType::MAIN) {
        advance(); // 消费主函数关键字
        name = "主函数";
    } else {
        throw std::runtime_error("定义必须指定名称 在第 " + std::to_string(nextToken.line) + " 行");
    }
    
    // 检查是否是函数定义（后面跟着'('）
    if (match(TokenType::LPAREN)) {
        // 这是一个函数定义
        return parseFunctionDefCommon(type, name, line, column);
    } else {
        // 这是一个变量定义
        return parseVariableDefCommon(type, name, line, column);
    }
}

// 解析变量定义
std::unique_ptr<ASTNode> Parser::parseVariableDef(bool consumeSemicolon) {
    int line = previous().line;
    int column = previous().column;
    
    consume(TokenType::LPAREN, "变量定义必须以 '(' 开始");
    
    // 解析类型
    std::string type;
    if (match(TokenType::INTEGER)) {
        type = "整型";
    } else if (match(TokenType::STRING)) {
        type = "字符串";
    } else if (match(TokenType::CHAR)) {
        type = "字符型";
    } else if (match(TokenType::VOID)) {
        type = "空类型";
    } else if (match(TokenType::DOUBLE)) {
        type = "小数";
    } else if (match(TokenType::BOOLEAN)) {
        type = "布尔型";
    } else if (match(TokenType::STRUCT)) {
        // 这是结构体定义：定义(结构体) 结构体名 { ... };
        return parseStructDefinition(line, column);
    } else if (match(TokenType::IDENTIFIER)) {
        // 支持自定义类型（结构体类型）
        type = previous().value;
    } else {
        // 尝试处理其他可能的类型标识符
        Token currentToken = peek();
        if (currentToken.type == TokenType::IDENTIFIER) {
            type = currentToken.value;
            advance(); // 消费标识符
        } else {
            std::cout << "未知类型错误: 当前token类型=" << static_cast<int>(peek().type) 
                      << ", 值='" << peek().value << "' 在第 " << peek().line << " 行" << std::endl;
            throw std::runtime_error("未知类型 在第 " + std::to_string(peek().line) + " 行");
        }
    }
    
    consume(TokenType::RPAREN, "类型声明必须以 ')' 结束");
    
    // 检查下一个token是否是标识符或主函数关键字
    Token nextToken = peek();
    std::string name;
    
    if (nextToken.type == TokenType::IDENTIFIER) {
        advance(); // 消费标识符
        name = nextToken.value;
    } else if (nextToken.type == TokenType::MAIN) {
        advance(); // 消费主函数关键字
        name = "主函数";
    } else {
        throw std::runtime_error("变量定义必须指定变量名 在第 " + std::to_string(nextToken.line) + " 行");
    }
    
    // 检查是否是函数定义（后面跟着'('）
    if (match(TokenType::LPAREN)) {
        // 这是一个函数定义
        
        // 解析参数列表
        std::vector<std::pair<std::string, std::string>> params;
        if (!match(TokenType::RPAREN)) {
            // 解析第一个参数 - 使用递归下降解析
            // 第一个参数应该是完整的定义语法：定义(整型) a
            // 我们需要解析定义(类型) 名字 的结构
            
            // 检查是否以"定义("开始
            Token paramToken = peek();
            if (paramToken.type == TokenType::DEFINE) {
                advance(); // 消费"定义"
                consume(TokenType::LPAREN, "参数定义必须以 '(' 开始");
                
                // 解析参数类型
                std::string paramType;
                Token typeToken = peek();
                if (typeToken.type == TokenType::INTEGER) {
                    paramType = "整型";
                } else if (typeToken.type == TokenType::STRING) {
                    paramType = "字符串";
                } else if (typeToken.type == TokenType::CHAR) {
                    paramType = "字符型";
                } else if (typeToken.type == TokenType::DOUBLE) {
                    paramType = "小数";
                } else if (typeToken.type == TokenType::BOOLEAN) {
                    paramType = "布尔型";
                } else {
                    throw std::runtime_error("无效的参数类型 在第 " + std::to_string(typeToken.line) + " 行");
                }
                advance(); // 消费类型token
                
                consume(TokenType::RPAREN, "参数类型声明必须以 ')' 结束");
                
                // 获取参数名
                Token paramNameToken = peek();
                if (paramNameToken.type != TokenType::IDENTIFIER) {
                    throw std::runtime_error("参数必须有名字 在第 " + std::to_string(paramNameToken.line) + " 行");
                }
                std::string paramName = paramNameToken.value;
                advance(); // 消费参数名token
                
                params.push_back({paramType, paramName});
            } else {
                throw std::runtime_error("函数参数必须使用 '定义(类型) 名字' 语法 在第 " + std::to_string(paramToken.line) + " 行");
            }
            
            // 解析后续参数
            while (match(TokenType::COMMA)) {
                // 解析下一个参数
                Token nextParamToken = peek();
                
                // 检查是否以"定义("开始
                if (nextParamToken.type == TokenType::DEFINE) {
                    advance(); // 消费"定义"
                    consume(TokenType::LPAREN, "参数定义必须以 '(' 开始");
                    
                    // 解析参数类型
                    std::string nextParamType;
                    Token nextTypeToken = peek();
                    if (nextTypeToken.type == TokenType::INTEGER) {
                        nextParamType = "整型";
                    } else if (nextTypeToken.type == TokenType::STRING) {
                        nextParamType = "字符串";
                    } else if (nextTypeToken.type == TokenType::CHAR) {
                        nextParamType = "字符型";
                    } else if (nextTypeToken.type == TokenType::DOUBLE) {
                        nextParamType = "小数";
                    } else if (nextTypeToken.type == TokenType::BOOLEAN) {
                        nextParamType = "布尔型";
                    } else {
                        throw std::runtime_error("无效的参数类型 在第 " + std::to_string(nextTypeToken.line) + " 行");
                    }
                    advance(); // 消费类型token
                    
                    consume(TokenType::RPAREN, "参数类型声明必须以 ')' 结束");
                    
                    // 获取参数名
                    Token nextParamNameToken = peek();
                    if (nextParamNameToken.type != TokenType::IDENTIFIER) {
                        throw std::runtime_error("参数必须有名字 在第 " + std::to_string(nextParamNameToken.line) + " 行");
                    }
                    std::string nextParamName = nextParamNameToken.value;
                    advance(); // 消费参数名token
                    
                    params.push_back({nextParamType, nextParamName});
                } else {
                    throw std::runtime_error("函数参数必须使用 '定义(类型) 名字' 语法 在第 " + std::to_string(nextParamToken.line) + " 行");
                }
            }
            consume(TokenType::RPAREN, "函数参数列表必须以 ')' 结束");
        }
        
        // 解析函数体
        consume(TokenType::LBRACE, "函数体必须以 '{' 开始");
        auto body = parseStatementList();
        
        return std::make_unique<FunctionDefNode>(type, name, params, std::move(body), line, column);
    } else {
        // 这是一个变量定义
        
        // 支持多个变量定义，用逗号分隔
        auto varDefList = std::make_unique<StatementListNode>(line, column);
        
        // 处理第一个变量
        bool isArray = false;
        int arraySize = 0;
        int dimensionCount = 0;
        if (match(TokenType::LBRACKET)) {
            isArray = true;
            // 支持多维数组定义
            dimensionCount++;
            
            // 获取数组大小的token值
            Token sizeToken = peek();
            if (sizeToken.type == TokenType::INTEGER_LITERAL) {
                advance(); // 消费数字字面量
                name += "[" + sizeToken.value + "]";
            } else {
                // 消费表达式（数组大小必须是常量表达式）
                parseExpression();
                throw std::runtime_error("数组大小必须是整数常量，不能是表达式 在第 " + std::to_string(sizeToken.line) + " 行");
            }
            consume(TokenType::RBRACKET, "数组定义必须以 ']' 结束");
            
            // 检查是否有多余的维度（最多5维）
            while (match(TokenType::LBRACKET)) {
                if (dimensionCount >= 5) {
                    throw std::runtime_error("数组维度不能超过5维 在第 " + std::to_string(peek().line) + " 行");
                }
                
                Token additionalSizeToken = peek();
                if (additionalSizeToken.type == TokenType::INTEGER_LITERAL) {
                    advance(); // 消费数字字面量
                    name += "[" + additionalSizeToken.value + "]";
                } else {
                    // 消费表达式（数组大小必须是常量表达式）
                    parseExpression();
                    throw std::runtime_error("数组大小必须是整数常量，不能是表达式 在第 " + std::to_string(additionalSizeToken.line) + " 行");
                }
                dimensionCount++;
                consume(TokenType::RBRACKET, "数组定义必须以 ']' 结束");
            }
        }
        
        std::unique_ptr<ASTNode> initializer = nullptr;
        if (match(TokenType::ASSIGN)) {
            // 检查是否是数组初始化（花括号列表）
            if (peek().type == TokenType::LBRACE) {
                advance(); // 消费左花括号
                // 简单处理：将花括号内的内容作为字符串处理
                std::string arrayValues = "";
                while (peek().type != TokenType::RBRACE && !isAtEnd()) {
                    if (peek().type == TokenType::INTEGER_LITERAL) {
                        arrayValues += previous().value;
                    } else if (peek().type == TokenType::COMMA) {
                        arrayValues += ",";
                    }
                    advance();
                }
                consume(TokenType::RBRACE, "数组初始化必须以 '}' 结束");
                // 创建一个字面量节点来表示数组初始化值
                initializer = std::make_unique<LiteralNode>(arrayValues, "数组", line, column);
            } else {
                initializer = parseExpression();
            }
        }
        varDefList->statements.push_back(std::make_unique<VariableDefNode>(type, name, std::move(initializer), line, column));
        
        // 处理后续变量
        while (match(TokenType::COMMA)) {
            // 解析下一个变量名
            if (peek().type == TokenType::IDENTIFIER) {
                advance(); // 消费标识符
                std::string varName = previous().value;
                
                // 检查是否是数组定义
                bool varIsArray = false;
                int varArraySize = 0;
                int varDimensionCount = 0;
                if (match(TokenType::LBRACKET)) {
                    varIsArray = true;
                    // 支持多维数组定义
                    varDimensionCount++;
                    
                    // 获取数组大小的token值
                    Token sizeToken = peek();
                    if (sizeToken.type == TokenType::INTEGER_LITERAL) {
                        advance(); // 消费数字字面量
                        varName += "[" + sizeToken.value + "]";
                    } else {
                        // 消费表达式（数组大小必须是常量表达式）
                        parseExpression();
                        throw std::runtime_error("数组大小必须是整数常量，不能是表达式 在第 " + std::to_string(sizeToken.line) + " 行");
                    }
                    consume(TokenType::RBRACKET, "数组定义必须以 ']' 结束");
                    
                    // 检查是否有多余的维度（最多5维）
                    while (match(TokenType::LBRACKET)) {
                        if (varDimensionCount >= 5) {
                            throw std::runtime_error("数组维度不能超过5维 在第 " + std::to_string(peek().line) + " 行");
                        }
                        
                        Token additionalSizeToken = peek();
                        if (additionalSizeToken.type == TokenType::INTEGER_LITERAL) {
                            advance(); // 消费数字字面量
                            varName += "[" + additionalSizeToken.value + "]";
                        } else {
                            // 消费表达式（数组大小必须是常量表达式）
                            parseExpression();
                            throw std::runtime_error("数组大小必须是整数常量，不能是表达式 在第 " + std::to_string(additionalSizeToken.line) + " 行");
                        }
                        varDimensionCount++;
                        consume(TokenType::RBRACKET, "数组定义必须以 ']' 结束");
                    }
                }
                
                // 解析初始化器
                std::unique_ptr<ASTNode> init = nullptr;
                if (match(TokenType::ASSIGN)) {
                    // 检查是否是数组初始化（花括号列表）
                    if (peek().type == TokenType::LBRACE) {
                        advance(); // 消费左花括号
                        // 简单处理：将花括号内的内容作为字符串处理
                        std::string arrayValues = "";
                        while (peek().type != TokenType::RBRACE && !isAtEnd()) {
                            if (peek().type == TokenType::INTEGER_LITERAL) {
                                arrayValues += previous().value;
                            } else if (peek().type == TokenType::COMMA) {
                                arrayValues += ",";
                            }
                            advance();
                        }
                        consume(TokenType::RBRACE, "数组初始化必须以 '}' 结束");
                        // 创建一个字面量节点来表示数组初始化值
                        init = std::make_unique<LiteralNode>(arrayValues, "数组", previous().line, previous().column);
                    } else {
                        init = parseExpression();
                    }
                }
                varDefList->statements.push_back(std::make_unique<VariableDefNode>(type, varName, std::move(init), previous().line, previous().column));
            } else {
                throw std::runtime_error("变量定义必须指定变量名 在第 " + std::to_string(peek().line) + " 行");
            }
        }
        
        if (consumeSemicolon) {
            consume(TokenType::SEMICOLON, "变量定义必须以分号结束");
        }
        
        return varDefList;
    }
}

// 重载版本，默认消费分号
std::unique_ptr<ASTNode> Parser::parseVariableDef() {
    return parseVariableDef(true);
}

// 解析函数定义
std::unique_ptr<ASTNode> Parser::parseFunctionDef() {
    int line = previous().line;
    int column = previous().column;
    
    // 解析类型
    std::string type;
    if (match(TokenType::INTEGER)) {
        type = "整型";
    } else if (match(TokenType::STRING)) {
        type = "字符串";
    } else if (match(TokenType::CHAR)) {
        type = "字符型";
    } else if (match(TokenType::VOID)) {
        type = "空类型";
    } else if (match(TokenType::DOUBLE)) {
        type = "小数";
    } else if (match(TokenType::BOOLEAN)) {
        type = "布尔型";
    } else if (match(TokenType::STRUCT)) {
        type = previous().value;
    } else if (match(TokenType::IDENTIFIER)) {
        type = previous().value;
    } else {
        throw std::runtime_error("未知类型 在第 " + std::to_string(peek().line) + " 行");
    }
    
    // 解析函数名
    std::string name;
    Token nameToken = peek();
    if (nameToken.type == TokenType::IDENTIFIER) {
        advance();
        name = nameToken.value;
    } else if (nameToken.type == TokenType::MAIN) {
        advance();
        name = "主函数";
    } else {
        throw std::runtime_error("函数定义必须指定函数名 在第 " + std::to_string(nameToken.line) + " 行");
    }
    
    // 使用parseFunctionDefCommon解析函数定义
    return parseFunctionDefCommon(type, name, line, column);
}

// 解析表达式（赋值运算符）
std::unique_ptr<ASTNode> Parser::parseExpression() {
    // 先解析左侧表达式（可能是标识符、数组访问、结构体成员访问等）
    auto expr = parseLogicalOrExpression();
    
    // 检查是否是赋值表达式
    if (match(TokenType::ASSIGN)) {
        std::string op = previous().value;
        auto right = parseExpression(); // 递归解析右侧表达式
        
        // 检查左侧表达式类型，创建相应的赋值节点
        if (expr->type == NodeType::IDENTIFIER) {
            // 简单变量赋值
            IdentifierNode* idNode = static_cast<IdentifierNode*>(expr.get());
            return std::make_unique<AssignmentNode>(idNode->name, std::move(right), expr->line, expr->column);
        } else if (expr->type == NodeType::ARRAY_ACCESS) {
            // 数组访问赋值
            ArrayAccessNode* arrayNode = static_cast<ArrayAccessNode*>(expr.get());
            return std::make_unique<ArrayAssignmentNode>(arrayNode->arrayName, std::move(arrayNode->indices), 
                                                         std::move(right), expr->line, expr->column);
        } else if (expr->type == NodeType::STRUCT_MEMBER_ACCESS) {
            // 结构体成员赋值
            StructMemberAccessNode* memberNode = static_cast<StructMemberAccessNode*>(expr.get());
            return std::make_unique<StructMemberAssignmentNode>(std::move(memberNode->structExpr), memberNode->memberName, 
                                                               std::move(right), expr->line, expr->column);
        } else {
            throw std::runtime_error("无效的赋值目标 在第 " + std::to_string(expr->line) + " 行");
        }
    }
    
    return expr;
}

// 解析逻辑或表达式
std::unique_ptr<ASTNode> Parser::parseLogicalOrExpression() {
    auto expr = parseLogicalAndExpression();
    
    while (match(TokenType::LOGICAL_OR)) {
        std::string op = previous().value;
        auto right = parseLogicalAndExpression();
        expr = std::make_unique<BinaryExpressionNode>(op, std::move(expr), std::move(right), previous().line, previous().column);
    }
    
    return expr;
}

// 解析逻辑与表达式
std::unique_ptr<ASTNode> Parser::parseLogicalAndExpression() {
    auto expr = parseComparisonExpression();
    
    while (match(TokenType::LOGICAL_AND)) {
        std::string op = previous().value;
        auto right = parseComparisonExpression();
        expr = std::make_unique<BinaryExpressionNode>(op, std::move(expr), std::move(right), previous().line, previous().column);
    }
    
    return expr;
}

// 解析比较表达式
std::unique_ptr<ASTNode> Parser::parseComparisonExpression() {
    auto expr = parseTerm();
    
    while (match(TokenType::LESS) || match(TokenType::GREATER) || 
           match(TokenType::LESS_EQUAL) || match(TokenType::GREATER_EQUAL) || 
           match(TokenType::EQUAL) || match(TokenType::NOT_EQUAL)) {
        std::string op = previous().value;
        auto right = parseTerm();
        expr = std::make_unique<BinaryExpressionNode>(op, std::move(expr), std::move(right), previous().line, previous().column);
    }
    
    return expr;
}

// 解析项（乘法、除法和取模）
std::unique_ptr<ASTNode> Parser::parseTerm() {
    auto expr = parseFactor();
    
    while (match(TokenType::MULTIPLY) || match(TokenType::DIVIDE) || match(TokenType::MODULO)) {
        std::string op = previous().value;
        auto right = parseFactor();
        expr = std::make_unique<BinaryExpressionNode>(op, std::move(expr), std::move(right), previous().line, previous().column);
    }
    
    return expr;
}

// 解析因子（加法和减法）
std::unique_ptr<ASTNode> Parser::parseFactor() {
    // 处理逻辑非运算符
    if (match(TokenType::LOGICAL_NOT)) {
        auto expr = parseFactor();
        return std::make_unique<UnaryExpressionNode>("!", std::move(expr), previous().line, previous().column);
    }
    
    auto expr = parsePrimary();
    
    while (match(TokenType::PLUS) || match(TokenType::MINUS)) {
        std::string op = previous().value;
        auto right = parsePrimary();
        expr = std::make_unique<BinaryExpressionNode>(op, std::move(expr), std::move(right), previous().line, previous().column);
    }
    
    return expr;
}

// 解析基本表达式
std::unique_ptr<ASTNode> Parser::parsePrimary() {
    // 处理负数字面量
    if (match(TokenType::MINUS)) {
        if (match(TokenType::INTEGER_LITERAL)) {
            return std::make_unique<LiteralNode>("-" + previous().value, "整数", previous().line, previous().column);
        }
    }
    
    if (match(TokenType::INTEGER_LITERAL)) {
        return std::make_unique<LiteralNode>(previous().value, "整数", previous().line, previous().column);
    }
    if (match(TokenType::STRING_LITERAL)) {
        return std::make_unique<LiteralNode>(previous().value, "字符串", previous().line, previous().column);
    }
    if (match(TokenType::BOOLEAN_LITERAL)) {
        return std::make_unique<LiteralNode>(previous().value, "布尔型", previous().line, previous().column);
    }
    if (match(TokenType::CHAR_LITERAL)) {
        return std::make_unique<LiteralNode>(previous().value, "字符型", previous().line, previous().column);
    }
    if (match(TokenType::IDENTIFIER)) {
        std::string name = previous().value;
        
        // 检查是否是数组访问
        if (match(TokenType::LBRACKET)) {
            // 解析索引列表（支持多维数组）
            std::vector<std::unique_ptr<ASTNode>> indices;
            int dimensionCount = 0;
            
            // 解析第一个维度
            auto indexExpr = parseExpression();
            indices.push_back(std::move(indexExpr));
            dimensionCount++;
            consume(TokenType::RBRACKET, "数组访问必须以 ']' 结束");
            
            // 检查是否有多余的维度（最多5维）
            while (match(TokenType::LBRACKET)) {
                if (dimensionCount >= 5) {
                    throw std::runtime_error("数组维度不能超过5维 在第 " + std::to_string(peek().line) + " 行");
                }
                
                auto additionalIndex = parseExpression();
                indices.push_back(std::move(additionalIndex));
                dimensionCount++;
                consume(TokenType::RBRACKET, "数组访问必须以 ']' 结束");
            }
            
            // 创建数组访问节点
            auto arrayAccess = std::make_unique<ArrayAccessNode>(name, std::move(indices), previous().line, previous().column);
            
            // 检查是否是数组访问赋值
            if (match(TokenType::ASSIGN)) {
                auto expr = parseExpression();
                // 创建多维数组访问赋值节点
                return std::make_unique<ArrayAssignmentNode>(name, std::move(arrayAccess->indices), std::move(expr), previous().line, previous().column);
            }
            
            // 检查是否还有结构体成员访问
            if (match(TokenType::DOT)) {
                consume(TokenType::IDENTIFIER, "必须指定成员名称");
                std::string memberName = previous().value;
                return std::make_unique<StructMemberAccessNode>(std::move(arrayAccess), memberName, previous().line, previous().column);
            }
            
            return arrayAccess;
        }
        
        // 检查是否是函数调用
        if (match(TokenType::LPAREN)) {
            // 解析参数列表
            std::vector<std::unique_ptr<ASTNode>> arguments;
            if (!match(TokenType::RPAREN)) {
                // 解析第一个参数
                arguments.push_back(parseExpression());
                
                // 解析后续参数
                while (match(TokenType::COMMA)) {
                    arguments.push_back(parseExpression());
                }
                consume(TokenType::RPAREN, "函数调用必须以 ')' 结束");
            }
            return std::make_unique<FunctionCallNode>(name, std::move(arguments), previous().line, previous().column);
        }
        
        // 检查是否是结构体成员访问
        if (match(TokenType::DOT)) {
            consume(TokenType::IDENTIFIER, "必须指定成员名称");
            std::string memberName = previous().value;
            
            auto structExpr = std::make_unique<IdentifierNode>(name, previous().line, previous().column);
            return std::make_unique<StructMemberAccessNode>(std::move(structExpr), memberName, previous().line, previous().column);
        }
        
        // 检查是否是普通赋值
        if (match(TokenType::ASSIGN)) {
            auto expr = parseExpression();
            return std::make_unique<AssignmentNode>(name, std::move(expr), previous().line, previous().column);
        } else {
            return std::make_unique<IdentifierNode>(name, previous().line, previous().column);
        }
    }
    if (match(TokenType::LPAREN)) {
        auto expr = parseExpression();
        consume(TokenType::RPAREN, "表达式必须以 ')' 结束");
        return expr;
    }
    
    throw std::runtime_error("无效的表达式 在第 " + std::to_string(peek().line) + " 行");
}

// 解析语句列表
std::unique_ptr<ASTNode> Parser::parseStatementList() {
    int line = previous().line;
    int column = previous().column;
    
    auto list = std::make_unique<StatementListNode>(line, column);
    
    // 解析语句直到遇到右花括号
    while (!isAtEnd()) {
        Token current = peek();
        if (current.type == TokenType::RBRACE) {
            advance(); // 消费右花括号
            return list;
        }
        list->statements.push_back(parseStatement());
    }
    
    throw std::runtime_error("语句块必须以 '}' 结束");
}

// 解析控制台输出语句
std::unique_ptr<ASTNode> Parser::parseCoutStatement() {
    int line = previous().line;
    int column = previous().column;
    
    consume(TokenType::LPAREN, "控制台输出必须以 '(' 开始");
    auto expr = parseExpression();
    consume(TokenType::RPAREN, "控制台输出必须以 ')' 结束");
    consume(TokenType::SEMICOLON, "语句必须以分号结束");
    
    return std::make_unique<CoutStatementNode>(std::move(expr), line, column);
}

// 解析控制台输入语句
std::unique_ptr<ASTNode> Parser::parseCinStatement() {
    int line = previous().line;
    int column = previous().column;
    
    consume(TokenType::LPAREN, "控制台输入必须以 '(' 开始");
    auto expr = parseExpression();
    consume(TokenType::RPAREN, "控制台输入必须以 ')' 结束");
    consume(TokenType::SEMICOLON, "语句必须以分号结束");
    
    return std::make_unique<CinStatementNode>(std::move(expr), line, column);
}

// 解析while循环语句
std::unique_ptr<ASTNode> Parser::parseWhileStatement() {
    int line = previous().line;
    int column = previous().column;
    
    consume(TokenType::LPAREN, "循环语句必须以 '(' 开始");
    auto condition = parseExpression();
    consume(TokenType::RPAREN, "循环条件必须以 ')' 结束");
    
    // 循环体可以是单个语句或语句块
    auto body = parseStatement();
    
    // 创建while语句节点
    return std::make_unique<WhileStatementNode>(std::move(condition), std::move(body), line, column);
}

// 解析for循环语句
std::unique_ptr<ASTNode> Parser::parseForStatement() {
    int line = previous().line;
    int column = previous().column;
    
    consume(TokenType::LPAREN, "循环语句必须以 '(' 开始");
    
    // 解析初始化表达式（支持变量定义语句）
    std::unique_ptr<ASTNode> initialization = nullptr;
    if (!match(TokenType::SEMICOLON)) {
        // 检查是否是变量定义语句
        if (peek().type == TokenType::DEFINE) {
            // 消费DEFINE关键字
            advance();
            // 解析变量定义语句（不消费分号，由for循环处理）
            initialization = parseVariableDef(false);
        } else {
            // 解析表达式
            initialization = parseExpression();
        }
        consume(TokenType::SEMICOLON, "for循环初始化表达式必须以分号结束");
    }
    
    // 解析条件表达式
    auto condition = parseExpression();
    consume(TokenType::SEMICOLON, "for循环条件表达式必须以分号结束");
    
    // 解析更新表达式
    auto updateExpr = parseExpression();
    consume(TokenType::RPAREN, "for循环更新表达式必须以 ')' 结束");
    
    // 循环体可以是单个语句或语句块
    auto body = parseStatement();
    
    // 创建for语句节点
    return std::make_unique<ForStatementNode>(std::move(initialization), std::move(condition), 
                                             std::move(updateExpr), std::move(body), line, column);
}

// 解析if语句
std::unique_ptr<ASTNode> Parser::parseIfStatement() {
    int line = previous().line;
    int column = previous().column;
    
    consume(TokenType::LPAREN, "if语句必须以 '(' 开始");
    auto condition = parseExpression();
    consume(TokenType::RPAREN, "if条件必须以 ')' 结束");
    
    // 解析if语句体
    auto thenBranch = parseStatement();
    
    // 解析else分支（可选）
    std::unique_ptr<ASTNode> elseBranch = nullptr;
    
    // 支持else-if链
    if (match(TokenType::ELSE)) {
        // 检查是否是else-if（而不是简单的else）
        if (match(TokenType::IF) || match(TokenType::ELSE_IF)) {
            // 解析else-if语句（递归调用parseIfStatement）
            elseBranch = parseIfStatement();
        } else {
            // 简单的else语句
            elseBranch = parseStatement();
        }
    }
    
    // 创建if语句节点
    return std::make_unique<IfStatementNode>(std::move(condition), std::move(thenBranch), 
                                            std::move(elseBranch), line, column);
}

// 解析返回语句
std::unique_ptr<ASTNode> Parser::parseReturnStatement() {
    int line = previous().line;
    int column = previous().column;

    // 解析返回表达式
    auto expr = parseExpression();
    consume(TokenType::SEMICOLON, "返回语句必须以分号结束");

    // 创建返回语句节点
    return std::make_unique<ReturnStatementNode>(std::move(expr), line, column);
}

// 解析文件读取语句
std::unique_ptr<ASTNode> Parser::parseFileReadStatement() {
    int line = previous().line;
    int column = previous().column;
    
    consume(TokenType::LPAREN, "文件读取必须以 '(' 开始");
    auto filename = parseExpression();
    consume(TokenType::COMMA, "文件读取参数必须以逗号分隔");
    auto variableName = parseExpression();
    consume(TokenType::RPAREN, "文件读取必须以 ')' 结束");
    consume(TokenType::SEMICOLON, "语句必须以分号结束");
    
    return std::make_unique<FileReadStatementNode>(std::move(filename), std::move(variableName), line, column);
}

// 解析文件写入语句
std::unique_ptr<ASTNode> Parser::parseFileWriteStatement() {
    int line = previous().line;
    int column = previous().column;
    
    consume(TokenType::LPAREN, "文件写入必须以 '(' 开始");
    auto filename = parseExpression();
    consume(TokenType::COMMA, "文件写入参数必须以逗号分隔");
    auto content = parseExpression();
    consume(TokenType::RPAREN, "文件写入必须以 ')' 结束");
    consume(TokenType::SEMICOLON, "语句必须以分号结束");
    
    return std::make_unique<FileWriteStatementNode>(std::move(filename), std::move(content), line, column);
}

// 解析文件追加语句
std::unique_ptr<ASTNode> Parser::parseFileAppendStatement() {
    int line = previous().line;
    int column = previous().column;
    
    consume(TokenType::LPAREN, "文件追加必须以 '(' 开始");
    auto filename = parseExpression();
    consume(TokenType::COMMA, "文件追加参数必须以逗号分隔");
    auto content = parseExpression();
    consume(TokenType::RPAREN, "文件追加必须以 ')' 结束");
    consume(TokenType::SEMICOLON, "语句必须以分号结束");
    
    return std::make_unique<FileAppendStatementNode>(std::move(filename), std::move(content), line, column);
}

// 解析结构体定义
std::unique_ptr<ASTNode> Parser::parseStructDef() {
    int line = previous().line;
    int column = previous().column;
    
    // 消费结构体类型标记
    consume(TokenType::RPAREN, "结构体定义必须以 ')' 结束");
    
    // 消费结构体名称
    consume(TokenType::IDENTIFIER, "结构体定义必须指定结构体名称");
    std::string structName = previous().value;
    
    consume(TokenType::LBRACE, "结构体定义必须以 '{' 开始");
    
    std::vector<std::pair<std::string, std::string>> members;
    
    // 解析成员定义
    while (peek().type != TokenType::RBRACE && !isAtEnd()) {
        // 跳过分号
        if (peek().type == TokenType::SEMICOLON) {
            advance();
            continue;
        }
        
        // 解析成员类型
        std::string memberType;
        if (peek().type == TokenType::INTEGER) {
            memberType = "整型";
            advance();
        } else if (peek().type == TokenType::STRING) {
            memberType = "字符串";
            advance();
        } else if (peek().type == TokenType::DOUBLE) {
            memberType = "小数";
            advance();
        } else if (peek().type == TokenType::BOOLEAN) {
            memberType = "布尔型";
            advance();
        } else {
            // 如果不是有效的类型，跳过这个token
            advance();
            continue;
        }
        
        // 解析成员名称
        if (peek().type == TokenType::IDENTIFIER) {
            std::string memberName = peek().value;
            members.push_back(std::make_pair(memberType, memberName));
            advance();
            
            // 消费分号（如果有的话）
            if (peek().type == TokenType::SEMICOLON) {
                advance();
            }
        } else {
            // 如果没有找到成员名称，跳过
            advance();
        }
    }
    
    consume(TokenType::RBRACE, "结构体定义必须以 '}' 结束");
    consume(TokenType::SEMICOLON, "结构体定义必须以分号结束");
    
    return std::make_unique<StructDefNode>(structName, members, line, column);
}

// 解析结构体定义
std::unique_ptr<ASTNode> Parser::parseStructDefinition(int line, int column) {
    // 消费RPAREN（右括号）
    consume(TokenType::RPAREN, "结构体定义必须以 ')' 结束类型声明");
    
    // 消费结构体名称
    consume(TokenType::IDENTIFIER, "结构体定义必须指定结构体名称");
    std::string structName = previous().value;
    
    consume(TokenType::LBRACE, "结构体定义必须以 '{' 开始");
    
    std::vector<std::pair<std::string, std::string>> members;
    
    // 解析成员定义
    while (peek().type != TokenType::RBRACE && !isAtEnd()) {
        // 跳过分号
        if (peek().type == TokenType::SEMICOLON) {
            advance();
            continue;
        }
        
        // 解析成员类型
        std::string memberType;
        if (match(TokenType::INTEGER)) {
            memberType = "整型";
        } else if (match(TokenType::STRING)) {
            memberType = "字符串";
        } else if (match(TokenType::DOUBLE)) {
            memberType = "小数";
        } else if (match(TokenType::BOOLEAN)) {
            memberType = "布尔型";
        } else {
            // 如果不是有效的类型，跳过这个token
            advance();
            continue;
        }
        
        // 解析成员名称
        if (match(TokenType::IDENTIFIER)) {
            std::string memberName = previous().value;
            members.push_back(std::make_pair(memberType, memberName));
            consume(TokenType::SEMICOLON, "成员定义必须以分号结束");
        }
    }
    
    consume(TokenType::RBRACE, "结构体定义必须以 '}' 结束");
    consume(TokenType::SEMICOLON, "结构体定义必须以分号结束");
    
    return std::make_unique<StructDefNode>(structName, members, line, column);
}

// 解析函数定义的通用处理
std::unique_ptr<ASTNode> Parser::parseFunctionDefCommon(const std::string& type, const std::string& name, int line, int column) {
    // 调试信息
    std::cout << "[解析器调试] 解析函数定义: " << name << "(" << std::endl;
    
    // 解析参数列表
    std::vector<std::pair<std::string, std::string>> params;
    if (!match(TokenType::RPAREN)) {
        // 解析第一个参数 - 使用递归下降解析
        // 第一个参数应该是完整的定义语法：定义(整型) a
        // 我们需要解析定义(类型) 名字 的结构
        
        // 检查是否以"定义("开始
        Token paramToken = peek();
        if (paramToken.type == TokenType::DEFINE) {
            advance(); // 消费"定义"
            consume(TokenType::LPAREN, "参数定义必须以 '(' 开始");
            
            // 解析参数类型
            std::string paramType;
            Token typeToken = peek();
            if (typeToken.type == TokenType::INTEGER) {
                paramType = "整型";
            } else if (typeToken.type == TokenType::STRING) {
                paramType = "字符串";
            } else if (typeToken.type == TokenType::DOUBLE) {
                paramType = "小数";
            } else if (typeToken.type == TokenType::BOOLEAN) {
                paramType = "布尔型";
            } else if (typeToken.type == TokenType::IDENTIFIER) {
                // 支持自定义类型（结构体类型）
                paramType = peek().value;
                advance();
            } else {
                throw std::runtime_error("未知参数类型 在第 " + std::to_string(typeToken.line) + " 行");
            }
            advance(); // 消费类型token
            
            consume(TokenType::RPAREN, "参数类型声明必须以 ')' 结束");
            
            // 消费参数名
            Token paramNameToken = peek();
            if (paramNameToken.type != TokenType::IDENTIFIER) {
                throw std::runtime_error("参数必须有名字 在第 " + std::to_string(paramNameToken.line) + " 行");
            }
            std::string paramName = paramNameToken.value;
            advance(); // 消费参数名token
            
            params.push_back({paramType, paramName});
        } else {
            throw std::runtime_error("函数参数必须使用 '定义(类型) 名字' 语法 在第 " + std::to_string(paramToken.line) + " 行");
        }
        
        // 继续解析其他参数（用逗号分隔）
        while (match(TokenType::COMMA)) {
            // 解析下一个参数
            Token nextParamToken = peek();
            if (nextParamToken.type == TokenType::DEFINE) {
                advance(); // 消费"定义"
                consume(TokenType::LPAREN, "参数定义必须以 '(' 开始");
                
                // 解析参数类型
                std::string paramType;
                Token typeToken = peek();
                if (typeToken.type == TokenType::INTEGER) {
                    paramType = "整型";
                } else if (typeToken.type == TokenType::STRING) {
                    paramType = "字符串";
                } else if (typeToken.type == TokenType::DOUBLE) {
                    paramType = "小数";
                } else if (typeToken.type == TokenType::BOOLEAN) {
                    paramType = "布尔型";
                } else if (typeToken.type == TokenType::IDENTIFIER) {
                    // 支持自定义类型（结构体类型）
                    paramType = peek().value;
                    advance();
                } else {
                    throw std::runtime_error("未知参数类型 在第 " + std::to_string(typeToken.line) + " 行");
                }
                advance(); // 消费类型token
                
                consume(TokenType::RPAREN, "参数类型声明必须以 ')' 结束");
                
                // 消费参数名
                Token paramNameToken = peek();
                if (paramNameToken.type != TokenType::IDENTIFIER) {
                    throw std::runtime_error("参数必须有名字 在第 " + std::to_string(paramNameToken.line) + " 行");
                }
                std::string nextParamName = paramNameToken.value;
                advance(); // 消费参数名token
                
                params.push_back({paramType, nextParamName});
            } else {
                throw std::runtime_error("函数参数必须使用 '定义(类型) 名字' 语法 在第 " + std::to_string(nextParamToken.line) + " 行");
            }
        }
        consume(TokenType::RPAREN, "函数参数列表必须以 ')' 结束");
    }
    
    // 解析函数体
    consume(TokenType::LBRACE, "函数体必须以 '{' 开始");
    auto body = parseStatementList();
    
    return std::make_unique<FunctionDefNode>(type, name, params, std::move(body), line, column);
}

// 解析变量定义的通用处理
std::unique_ptr<ASTNode> Parser::parseVariableDefCommon(const std::string& type, const std::string& name, int line, int column) {
    // 支持多个变量定义，用逗号分隔
    auto varDefList = std::make_unique<StatementListNode>(line, column);
    
    // 处理第一个变量
    bool isArray = false;
    std::unique_ptr<ASTNode> arraySizeExpr = nullptr;
    int dimensionCount = 0;
    if (match(TokenType::LBRACKET)) {
        isArray = true;
        dimensionCount++;
        
        // 解析数组大小 - 支持动态表达式
        if (match(TokenType::INTEGER_LITERAL)) {
            // 兼容整数常量
            arraySizeExpr = std::make_unique<LiteralNode>(previous().value, "整数", previous().line, previous().column);
        } else if (peek().type == TokenType::RBRACKET) {
            // 空数组
            arraySizeExpr = nullptr;
        } else {
            // 支持动态表达式
            arraySizeExpr = parseExpression();
        }
        
        // 继续解析多维数组
        while (match(TokenType::LBRACKET)) {
            dimensionCount++;
            if (dimensionCount > 5) {
                throw std::runtime_error("数组最多支持5维 在第 " + std::to_string(previous().line) + " 行");
            }
            
            // 解析维度大小 - 支持动态表达式
            if (match(TokenType::INTEGER_LITERAL)) {
                // 兼容整数常量 - 创建一个LiteralNode而不是转换int
                // 这里不需要处理，因为主要是第一维的大小表达式
            } else if (peek().type == TokenType::RBRACKET) {
                // 空维度
            } else {
                // 支持动态表达式
                auto dimExpr = parseExpression();
                (void)dimExpr; // 忽略结果，因为这是维度大小
            }
            
            consume(TokenType::RBRACKET, "数组维度必须以 ']' 结束");
        }
    }
    
    // 检查是否有赋值
    std::unique_ptr<ASTNode> initializer = nullptr;
    if (match(TokenType::ASSIGN)) {
        initializer = parseExpression();
    }
    
    varDefList->statements.push_back(std::make_unique<VariableDefNode>(type, name, isArray, std::move(arraySizeExpr), std::move(initializer), line, column));
    
    // 继续解析其他变量定义（用逗号分隔）
    while (match(TokenType::COMMA)) {
        // 解析下一个变量名
        Token nextVarToken = peek();
        if (nextVarToken.type != TokenType::IDENTIFIER) {
            throw std::runtime_error("变量定义中意外的token 在第 " + std::to_string(nextVarToken.line) + " 行");
        }
        std::string nextVarName = nextVarToken.value;
        advance(); // 消费变量名
        
        // 检查是否有数组声明
        bool nextIsArray = false;
        std::unique_ptr<ASTNode> nextArraySizeExpr = nullptr;
        int nextDimensionCount = 0;
        if (match(TokenType::LBRACKET)) {
            nextIsArray = true;
            nextDimensionCount++;
            
            // 解析数组大小 - 支持动态表达式
            if (match(TokenType::INTEGER_LITERAL)) {
                // 兼容整数常量
                nextArraySizeExpr = std::make_unique<LiteralNode>(previous().value, "整数", previous().line, previous().column);
            } else if (peek().type == TokenType::RBRACKET) {
                // 空数组
                nextArraySizeExpr = nullptr;
            } else {
                // 支持动态表达式
                nextArraySizeExpr = parseExpression();
            }
            
            // 继续解析多维数组
            while (match(TokenType::LBRACKET)) {
                nextDimensionCount++;
                if (nextDimensionCount > 5) {
                    throw std::runtime_error("数组最多支持5维 在第 " + std::to_string(previous().line) + " 行");
                }
                
                // 解析维度大小 - 支持动态表达式
                if (match(TokenType::INTEGER_LITERAL)) {
                    // 兼容整数常量 - 创建一个LiteralNode而不是转换int
                    // 这里不需要处理，因为主要是第一维的大小表达式
                } else if (peek().type == TokenType::RBRACKET) {
                    // 空维度
                } else {
                    // 支持动态表达式
                    auto dimExpr = parseExpression();
                    (void)dimExpr; // 忽略结果，因为这是维度大小
                }
                
                consume(TokenType::RBRACKET, "数组维度必须以 ']' 结束");
            }
        }
        
        // 检查是否有赋值
        std::unique_ptr<ASTNode> nextInitializer = nullptr;
        if (match(TokenType::ASSIGN)) {
            nextInitializer = parseExpression();
        }
        
        varDefList->statements.push_back(std::make_unique<VariableDefNode>(type, nextVarName, nextIsArray, std::move(nextArraySizeExpr), std::move(nextInitializer), line, column));
    }
    
    return varDefList;
}

// 解析C风格的变量定义：类型 变量名 = 值;
std::unique_ptr<ASTNode> Parser::parseCStyleVariableDef() {
    int line = peek().line;
    int column = peek().column;
    
    // 解析类型
    std::string type;
    if (match(TokenType::INTEGER)) {
        type = "整型";
    } else if (match(TokenType::STRING)) {
        type = "字符串";
    } else if (match(TokenType::CHAR)) {
        type = "字符型";
    } else if (match(TokenType::VOID)) {
        type = "空类型";
    } else if (match(TokenType::DOUBLE)) {
        type = "小数";
    } else if (match(TokenType::BOOLEAN)) {
        type = "布尔型";
    } else if (match(TokenType::STRUCT)) {
        type = previous().value;
    } else if (match(TokenType::IDENTIFIER)) {
        type = previous().value;
    } else {
        throw std::runtime_error("未知类型 在第 " + std::to_string(peek().line) + " 行");
    }
    
    // 解析变量名
    Token nameToken = peek();
    if (nameToken.type != TokenType::IDENTIFIER) {
        throw std::runtime_error("变量定义必须指定变量名 在第 " + std::to_string(nameToken.line) + " 行");
    }
    std::string name = nameToken.value;
    advance(); // 消费变量名
    
    // 检查是否有数组声明
    bool isArray = false;
    std::unique_ptr<ASTNode> arraySizeExpr = nullptr;
    if (match(TokenType::LBRACKET)) {
        isArray = true;
        // 解析数组大小
        if (match(TokenType::INTEGER_LITERAL)) {
            arraySizeExpr = std::make_unique<LiteralNode>(previous().value, "整数", previous().line, previous().column);
        } else if (peek().type != TokenType::RBRACKET) {
            arraySizeExpr = parseExpression();
        }
        consume(TokenType::RBRACKET, "数组定义必须以 ']' 结束");
    }
    
    // 检查是否有赋值
    std::unique_ptr<ASTNode> initializer = nullptr;
    if (match(TokenType::ASSIGN)) {
        initializer = parseExpression();
    }
    
    consume(TokenType::SEMICOLON, "变量定义必须以分号结束");
    
    return std::make_unique<VariableDefNode>(type, name, isArray, std::move(arraySizeExpr), std::move(initializer), line, column);
}

// 解析结构体成员访问
std::unique_ptr<ASTNode> Parser::parseStructMemberAccess() {
    int line = previous().line;
    int column = previous().column;
    
    // 解析结构体表达式
    auto structExpr = parseExpression();
    
    consume(TokenType::DOT, "结构体成员访问必须使用 '.' 运算符");
    
    consume(TokenType::IDENTIFIER, "必须指定成员名称");
    std::string memberName = previous().value;
    
    return std::make_unique<StructMemberAccessNode>(std::move(structExpr), memberName, line, column);
}

// 解析导入语句
std::unique_ptr<ASTNode> Parser::parseImportStatement() {
    int line = previous().line;
    int column = previous().column;
    
    consume(TokenType::LPAREN, "导入语句必须以 '(' 开始");
    
    // 解析文件路径（字符串字面量）
    if (peek().type != TokenType::STRING_LITERAL) {
        throw std::runtime_error("导入语句必须包含字符串文件路径 在第 " + std::to_string(peek().line) + " 行");
    }
    
    std::string filePath = peek().value;
    advance(); // 消费字符串字面量
    
    consume(TokenType::RPAREN, "导入语句必须以 ')' 结束");
    consume(TokenType::SEMICOLON, "导入语句必须以分号结束");
    
    // 调试信息
    std::cout << "解析导入语句: " << filePath << " 在第 " << line << " 行" << std::endl;
    
    return std::make_unique<ImportStatementNode>(filePath, line, column);
}
