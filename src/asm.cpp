#include "../include/asm.h"
#include "../include/parser.h"
#include "../include/lexer.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <memory>
#include <stack>

using namespace HexAsm;

HexAsmParser::HexAsmParser() {
    initializeRegisterMap();
    initializeOpcodeMap();
    varRegisterCounter = 0;
    labelCounter = 0;
}

void HexAsmParser::initializeRegisterMap() {
    registerMap = {
        {"AX", 0x0}, {"BX", 0x1}, {"CX", 0x2}, {"DX", 0x3},
        {"IP", 0x4}, {"ST", 0x5}, {"BP", 0x6}, {"SR", 0x7}
    };
}

void HexAsmParser::initializeOpcodeMap() {
    opcodeMap = {
        {"LOAD", 0x01}, {"MOVE", 0x02}, {"PLUS", 0x03}, {"MINUS", 0x04},
        {"TIMES", 0x05}, {"DIVIDE", 0x06}, {"JUMP", 0x07}, {"JEQ", 0x08},
        {"JNE", 0x09}, {"FETCH", 0x0A}, {"SAVE", 0x0B}, {"CALL", 0x0C},
        {"TEST", 0x0D}, {"BITAND", 0x0E}, {"BITOR", 0x0F}
    };
}

bool HexAsmParser::parseAndGenerate(const std::string& asmCode, const std::string& outputFile) {
    errorMessage.clear();
    instructions.clear();
    symbolTable.clear();
    
    // 预处理代码
    std::string preprocessed = preprocessCode(asmCode);
    
    // 分割为行
    std::vector<std::string> lines = splitString(preprocessed, '\n');
    
    // 第一遍：解析标签
    if (!parseLabels(lines)) {
        return false;
    }
    
    // 第二遍：解析指令
    if (!parseInstructions(lines)) {
        return false;
    }
    
    // 生成二进制代码
    std::vector<uint8_t> binary = generateBinary();
    
    // 写入文件
    std::ofstream outFile(outputFile, std::ios::binary);
    if (!outFile) {
        errorMessage = "无法打开输出文件: " + outputFile;
        return false;
    }
    
    outFile.write(reinterpret_cast<const char*>(binary.data()), binary.size());
    outFile.close();
    
    return true;
}

bool HexAsmParser::compileCHToHexBinary(const std::string& chCode, const std::string& outputFile) {
    errorMessage.clear();
    varRegisterMap.clear();
    varRegisterCounter = 0;
    labelCounter = 0;
    stringLiterals.clear();
    
    // 将CH代码转换为16进制汇编
    std::string hexAsm = convertCHToHexAsm(chCode);
    
    // 调试输出
    std::cout << "[调试] 生成的汇编代码:" << std::endl;
    std::cout << hexAsm << std::endl;
    
    // 解析并生成二进制
    return parseAndGenerate(hexAsm, outputFile);
}

std::string HexAsmParser::preprocessCode(const std::string& code) {
    std::string result;
    std::istringstream iss(code);
    std::string line;
    
    while (std::getline(iss, line)) {
        // 移除注释（以#开头）
        size_t commentPos = line.find('#');
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }
        
        // 移除行尾空格
        line = trim(line);
        
        // 跳过空行
        if (!line.empty()) {
            result += line + "\n";
        }
    }
    
    return result;
}

bool HexAsmParser::parseLabels(const std::vector<std::string>& lines) {
    uint32_t address = 0;
    
    for (const auto& line : lines) {
        std::string trimmed = trim(line);
        
        // 跳过空行
        if (trimmed.empty()) continue;
        
        // 检查是否为标签（以冒号结尾）
        if (trimmed.back() == ':') {
            std::string label = trimmed.substr(0, trimmed.length() - 1);
            label = trim(label);
            
            if (symbolTable.find(label) != symbolTable.end()) {
                errorMessage = "重复的标签定义: " + label;
                return false;
            }
            
            symbolTable[label] = address;
        } else {
            // 普通指令，地址增加4字节
            address += 4;
        }
    }
    
    return true;
}

bool HexAsmParser::parseInstructions(const std::vector<std::string>& lines) {
    uint32_t address = 0;
    
    for (const auto& line : lines) {
        std::string trimmed = trim(line);
        
        // 跳过空行和标签定义
        if (trimmed.empty() || trimmed.back() == ':') {
            continue;
        }
        
        // 解析指令
        if (!parseInstruction(trimmed, address)) {
            return false;
        }
        
        address += 4; // 每条指令4字节
    }
    
    return true;
}

bool HexAsmParser::parseInstruction(const std::string& line, uint32_t address) {
    Instruction instr;
    instr.address = address;
    
    // 分割指令和操作数
    std::vector<std::string> parts = splitString(line, ' ');
    if (parts.empty()) {
        errorMessage = "空指令: " + line;
        return false;
    }
    
    std::string mnemonic = parts[0];
    
    // 查找操作码
    if (opcodeMap.find(mnemonic) == opcodeMap.end()) {
        errorMessage = "未知指令: " + mnemonic;
        return false;
    }
    
    instr.opcode = opcodeMap[mnemonic];
    
    // 根据操作码解析操作数
    switch (instr.opcode) {
        case 0x01: // LOAD Rd, imm
            if (parts.size() != 3) {
                errorMessage = "LOAD指令需要2个操作数: " + line;
                return false;
            }
            if (registerMap.find(parts[1]) == registerMap.end()) {
                errorMessage = "未知寄存器: " + parts[1];
                return false;
            }
            instr.reg1 = registerMap[parts[1]];
            if (!isHexNumber(parts[2])) {
                errorMessage = "无效的立即数: " + parts[2];
                return false;
            }
            instr.immediate = hexToInt(parts[2]);
            break;
            
        case 0x02: // MOVE Rd, Rs
        case 0x03: // PLUS Rd, Rs
        case 0x04: // MINUS Rd, Rs
        case 0x05: // TIMES Rd, Rs
        case 0x06: // DIVIDE Rd, Rs
        case 0x0D: // TEST Rd, Rs
        case 0x0E: // BITAND Rd, Rs
        case 0x0F: // BITOR Rd, Rs
            if (parts.size() != 3) {
                errorMessage = mnemonic + "指令需要2个操作数: " + line;
                return false;
            }
            if (registerMap.find(parts[1]) == registerMap.end()) {
                errorMessage = "未知寄存器: " + parts[1];
                return false;
            }
            if (registerMap.find(parts[2]) == registerMap.end()) {
                errorMessage = "未知寄存器: " + parts[2];
                return false;
            }
            instr.reg1 = registerMap[parts[1]];
            instr.reg2 = registerMap[parts[2]];
            break;
            
        case 0x07: // JUMP label
        case 0x08: // JEQ label
        case 0x09: // JNE label
            if (parts.size() != 2) {
                errorMessage = mnemonic + "指令需要1个操作数: " + line;
                return false;
            }
            instr.label = parts[1];
            break;
            
        case 0x0A: // FETCH Rd, [Rs+imm]
            if (parts.size() != 3) {
                errorMessage = "FETCH指令需要2个操作数: " + line;
                return false;
            }
            if (registerMap.find(parts[1]) == registerMap.end()) {
                errorMessage = "未知寄存器: " + parts[1];
                return false;
            }
            if (registerMap.find(parts[2]) == registerMap.end()) {
                errorMessage = "未知寄存器: " + parts[2];
                return false;
            }
            instr.reg1 = registerMap[parts[1]];
            instr.reg2 = registerMap[parts[2]];
            break;
            
        case 0x0B: // SAVE [Rd+imm], Rs
            if (parts.size() != 3) {
                errorMessage = "SAVE指令需要2个操作数: " + line;
                return false;
            }
            if (registerMap.find(parts[1]) == registerMap.end()) {
                errorMessage = "未知寄存器: " + parts[1];
                return false;
            }
            if (registerMap.find(parts[2]) == registerMap.end()) {
                errorMessage = "未知寄存器: " + parts[2];
                return false;
            }
            instr.reg1 = registerMap[parts[1]];
            instr.reg2 = registerMap[parts[2]];
            break;
            
        case 0x0C: // CALL callno
            if (parts.size() != 2) {
                errorMessage = "CALL指令需要1个操作数: " + line;
                return false;
            }
            if (!isHexNumber(parts[1])) {
                errorMessage = "无效的系统调用号: " + parts[1];
                return false;
            }
            instr.immediate = hexToInt(parts[1]);
            break;
            
        default:
            errorMessage = "未实现的指令: " + mnemonic;
            return false;
    }
    
    instructions.push_back(instr);
    return true;
}

std::vector<uint8_t> HexAsmParser::generateBinary() {
    std::vector<uint8_t> binary;
    
    for (const auto& instr : instructions) {
        // 指令格式：操作码(8位) | 寄存器1(4位) | 寄存器2(4位) | 立即数(16位)
        uint32_t encoded = 0;
        
        encoded |= (instr.opcode & 0xFF) << 24;
        encoded |= (instr.reg1 & 0xF) << 20;
        encoded |= (instr.reg2 & 0xF) << 16;
        encoded |= (instr.immediate & 0xFFFF);
        
        // 处理跳转指令的标签
        if (!instr.label.empty()) {
            if (symbolTable.find(instr.label) == symbolTable.end()) {
                errorMessage = "未定义的标签: " + instr.label;
                return {};
            }
            encoded = (encoded & 0xFFFF0000) | (symbolTable[instr.label] & 0xFFFF);
        }
        
        // 将32位指令分解为4个字节
        binary.push_back((encoded >> 24) & 0xFF);
        binary.push_back((encoded >> 16) & 0xFF);
        binary.push_back((encoded >> 8) & 0xFF);
        binary.push_back(encoded & 0xFF);
    }
    
    return binary;
}

std::string HexAsmParser::convertCHToHexAsm(const std::string& chCode) {
    // 使用新的基于解析器的转换方法
    return convertCHToHexAsmFromAST(chCode);
}

std::string HexAsmParser::convertCHToHexAsmFromAST(const std::string& chCode) {
    // 解析CH代码生成AST
    auto ast = parseCHCode(chCode);
    if (!ast) {
        std::cout << "[调试] AST解析失败: " << errorMessage << std::endl;
        // 如果解析失败，返回默认的简单程序
        std::stringstream asmCode;
        asmCode << "start:\n";
        asmCode << "LOAD AX 0x00\n";
        asmCode << "CALL 0x00\n";
        return asmCode.str();
    }
    
    std::cout << "[调试] AST解析成功，类型: " << static_cast<int>(ast->type) << std::endl;
    
    // 将AST转换为16进制汇编
    return astToHexAsm(std::move(ast));
}

std::unique_ptr<ASTNode> HexAsmParser::parseCHCode(const std::string& chCode) {
    // 创建词法分析器
    Lexer lexer(chCode);
    
    // 获取词法分析器的token列表
    std::vector<Token> tokens;
    try {
        tokens = lexer.tokenize();
    } catch (const std::exception& e) {
        errorMessage = "词法分析错误: " + std::string(e.what());
        return nullptr;
    }
    
    // 创建语法分析器
    Parser parser(tokens);
    
    try {
        // 解析代码生成AST
        return parser.parse();
    } catch (const std::exception& e) {
        errorMessage = "CH代码解析错误: " + std::string(e.what());
        return nullptr;
    }
}

std::string HexAsmParser::astToHexAsm(std::unique_ptr<ASTNode> ast) {
    std::stringstream asmCode;
    
    // 添加程序开始标签
    asmCode << "start:\n";
    
    // 根据AST类型生成不同的汇编代码
    if (ast->type == NodeType::PROGRAM) {
        auto program = static_cast<ProgramNode*>(ast.get());
        std::cout << "[调试] 程序节点，语句数量: " << program->statements.size() << std::endl;
        
        // 查找主函数
        bool hasMainFunction = false;
        for (const auto& statement : program->statements) {
            std::cout << "[调试] 语句类型: " << static_cast<int>(statement->type) << std::endl;
            if (statement->type == NodeType::FUNCTION_DEF) {
                auto funcDef = static_cast<FunctionDefNode*>(statement.get());
                std::cout << "[调试] 函数名: " << funcDef->name << std::endl;
                if (funcDef->name == "主函数") {
                    hasMainFunction = true;
                    // 跳转到主函数
                    asmCode << "JUMP 主函数\n";
                    break;
                }
            }
        }
        
        // 如果没有找到主函数，生成默认代码
        if (!hasMainFunction) {
            std::cout << "[调试] 没有找到主函数，生成默认代码" << std::endl;
            // 遍历程序中的所有语句
            for (const auto& statement : program->statements) {
                asmCode << generateStatementAsm(statement.get());
            }
        } else {
            std::cout << "[调试] 找到主函数，生成所有函数定义" << std::endl;
            // 生成所有函数定义
            for (const auto& statement : program->statements) {
                asmCode << generateStatementAsm(statement.get());
            }
        }
    } else {
        std::cout << "[调试] AST不是程序节点，类型: " << static_cast<int>(ast->type) << std::endl;
    }
    
    // 程序结束时添加退出系统调用
    asmCode << "LOAD AX 0x00\n";
    asmCode << "CALL 0x00\n";
    
    return asmCode.str();
}

// 生成语句的汇编代码
std::string HexAsmParser::generateStatementAsm(ASTNode* node) {
    std::stringstream asmCode;
    
    switch (node->type) {
        case NodeType::VARIABLE_DEF:
            asmCode << generateVariableDefAsm(static_cast<VariableDefNode*>(node));
            break;
            
        case NodeType::ASSIGNMENT:
            asmCode << generateAssignmentAsm(static_cast<AssignmentNode*>(node));
            break;
            
        case NodeType::COUT_STATEMENT:
            asmCode << generateCoutAsm(static_cast<CoutStatementNode*>(node));
            break;
            
        case NodeType::FUNCTION_DEF:
            asmCode << generateFunctionDefAsm(static_cast<FunctionDefNode*>(node));
            break;
            
        case NodeType::RETURN_STATEMENT:
            asmCode << generateReturnAsm(static_cast<ReturnStatementNode*>(node));
            break;
            
        case NodeType::IF_STATEMENT:
            asmCode << generateIfAsm(static_cast<IfStatementNode*>(node));
            break;
            
        case NodeType::WHILE_STATEMENT:
            asmCode << generateWhileAsm(static_cast<WhileStatementNode*>(node));
            break;
            
        case NodeType::FOR_STATEMENT:
            asmCode << generateForAsm(static_cast<ForStatementNode*>(node));
            break;
            
        case NodeType::STATEMENT_LIST:
            asmCode << generateStatementListAsm(static_cast<StatementListNode*>(node));
            break;
            
        default:
            // 对于不支持的语句类型，生成空代码
            break;
    }
    
    return asmCode.str();
}

// 生成语句列表的汇编代码
std::string HexAsmParser::generateStatementListAsm(StatementListNode* node) {
    std::stringstream asmCode;
    
    for (const auto& statement : node->statements) {
        asmCode << generateStatementAsm(statement.get());
    }
    
    return asmCode.str();
}

// 生成变量定义的汇编代码
std::string HexAsmParser::generateVariableDefAsm(VariableDefNode* node) {
    std::stringstream asmCode;
    
    if (node->initializer) {
        // 如果有初始化表达式，生成赋值代码
        std::string resultReg = evaluateExpression(node->initializer.get(), asmCode);
        asmCode << "MOVE " << getVariableRegister(node->name) << " " << resultReg << "\n";
    }
    
    return asmCode.str();
}

// 生成赋值语句的汇编代码
std::string HexAsmParser::generateAssignmentAsm(AssignmentNode* node) {
    std::stringstream asmCode;
    
    std::string resultReg = evaluateExpression(node->expression.get(), asmCode);
    asmCode << "MOVE " << getVariableRegister(node->name) << " " << resultReg << "\n";
    
    return asmCode.str();
}

// 生成输出语句的汇编代码
std::string HexAsmParser::generateCoutAsm(CoutStatementNode* node) {
    std::stringstream asmCode;
    
    if (!node->expression) {
        return asmCode.str();
    }
    
    auto expr = node->expression.get();
    
    // 处理不同类型的表达式
    if (expr->type == NodeType::LITERAL) {
        auto literal = static_cast<LiteralNode*>(expr);
        if (literal->literalType == "字符串") {
            // 输出字符串
            std::string str = literal->value;
            for (char c : str) {
                asmCode << "LOAD AX 0x01\n";
                asmCode << "LOAD BX 0x" << std::hex << static_cast<int>(static_cast<unsigned char>(c)) << std::dec << "\n";
                asmCode << "CALL 0x01\n";
            }
        } else if (literal->literalType == "整型") {
            // 输出整数
            int value = std::stoi(literal->value);
            asmCode << generateIntegerOutput(value);
        } else if (literal->literalType == "小数") {
            // 输出小数
            double value = std::stod(literal->value);
            asmCode << generateFloatOutput(value);
        } else if (literal->literalType == "布尔型") {
            // 输出布尔值
            bool value = (literal->value == "真");
            asmCode << generateBooleanOutput(value);
        } else if (literal->literalType == "字符型") {
            // 输出字符
            char c = literal->value.empty() ? '\0' : literal->value[0];
            asmCode << "LOAD AX 0x01\n";
            asmCode << "LOAD BX 0x" << std::hex << static_cast<int>(static_cast<unsigned char>(c)) << std::dec << "\n";
            asmCode << "CALL 0x01\n";
        }
    } else if (expr->type == NodeType::IDENTIFIER) {
        // 输出变量值
        auto identifier = static_cast<IdentifierNode*>(expr);
        std::string varReg = getVariableRegister(identifier->name);
        asmCode << "LOAD AX 0x02\n";
        asmCode << "MOVE BX " << varReg << "\n";
        asmCode << "CALL 0x01\n";
    } else if (expr->type == NodeType::BINARY_EXPRESSION) {
        // 输出表达式结果
        std::string resultReg = evaluateExpression(expr, asmCode);
        asmCode << "LOAD AX 0x02\n";
        asmCode << "MOVE BX " << resultReg << "\n";
        asmCode << "CALL 0x01\n";
    } else if (expr->type == NodeType::FUNCTION_CALL) {
        // 输出函数调用结果
        std::string resultReg = evaluateExpression(expr, asmCode);
        asmCode << "LOAD AX 0x02\n";
        asmCode << "MOVE BX " << resultReg << "\n";
        asmCode << "CALL 0x01\n";
    }
    
    // 输出换行符
    asmCode << "LOAD AX 0x01\n";
    asmCode << "LOAD BX 0x0A\n";
    asmCode << "CALL 0x01\n";
    
    return asmCode.str();
}

// 生成整数输出代码
std::string HexAsmParser::generateIntegerOutput(int value) {
    std::stringstream asmCode;
    
    // 转换为字符串
    std::string str = std::to_string(value);
    for (char c : str) {
        asmCode << "LOAD AX 0x01\n";
        asmCode << "LOAD BX 0x" << std::hex << static_cast<int>(static_cast<unsigned char>(c)) << std::dec << "\n";
        asmCode << "CALL 0x01\n";
    }
    
    return asmCode.str();
}

// 生成浮点数输出代码
std::string HexAsmParser::generateFloatOutput(double value) {
    std::stringstream asmCode;
    
    // 转换为字符串
    std::string str = std::to_string(value);
    for (char c : str) {
        asmCode << "LOAD AX 0x01\n";
        asmCode << "LOAD BX 0x" << std::hex << static_cast<int>(static_cast<unsigned char>(c)) << std::dec << "\n";
        asmCode << "CALL 0x01\n";
    }
    
    return asmCode.str();
}

// 生成布尔值输出代码
std::string HexAsmParser::generateBooleanOutput(bool value) {
    std::stringstream asmCode;
    
    std::string str = value ? "真" : "假";
    for (char c : str) {
        asmCode << "LOAD AX 0x01\n";
        asmCode << "LOAD BX 0x" << std::hex << static_cast<int>(static_cast<unsigned char>(c)) << std::dec << "\n";
        asmCode << "CALL 0x01\n";
    }
    
    return asmCode.str();
}

// 生成函数定义的汇编代码
std::string HexAsmParser::generateFunctionDefAsm(FunctionDefNode* node) {
    std::stringstream asmCode;
    
    // 添加函数标签
    asmCode << node->name << ":\n";
    
    // 生成函数体代码
    if (node->body) {
        asmCode << generateStatementAsm(node->body.get());
    }
    
    return asmCode.str();
}

// 生成返回语句的汇编代码
std::string HexAsmParser::generateReturnAsm(ReturnStatementNode* node) {
    std::stringstream asmCode;
    
    if (node->expression) {
        // 计算返回值并存储在AX寄存器
        std::string resultReg = evaluateExpression(node->expression.get(), asmCode);
        asmCode << "MOVE AX " << resultReg << "\n";
    }
    
    return asmCode.str();
}

// 生成if语句的汇编代码
std::string HexAsmParser::generateIfAsm(IfStatementNode* node) {
    std::stringstream asmCode;
    
    // 生成条件表达式
    std::string resultReg = evaluateExpression(node->condition.get(), asmCode);
    // 使用一个包含0的寄存器进行比较
    std::string zeroReg = getTempRegister(resultReg, "");
    asmCode << "LOAD " << zeroReg << " 0x00\n";
    asmCode << "TEST " << resultReg << " " << zeroReg << "\n";
    
    // 条件跳转
    std::string elseLabel = generateLabel("else");
    std::string endLabel = generateLabel("endif");
    
    asmCode << "JEQ " << elseLabel << "\n";
    
    // 生成if体
    if (node->thenBranch) {
        asmCode << generateStatementAsm(node->thenBranch.get());
    }
    
    asmCode << "JUMP " << endLabel << "\n";
    asmCode << elseLabel << ":\n";
    
    // 生成else体
    if (node->elseBranch) {
        asmCode << generateStatementAsm(node->elseBranch.get());
    }
    
    asmCode << endLabel << ":\n";
    
    return asmCode.str();
}

// 生成while循环的汇编代码
std::string HexAsmParser::generateWhileAsm(WhileStatementNode* node) {
    std::stringstream asmCode;
    
    std::string loopLabel = generateLabel("while");
    std::string endLabel = generateLabel("endwhile");
    
    asmCode << loopLabel << ":\n";
    
    // 生成条件表达式
    std::string resultReg = evaluateExpression(node->condition.get(), asmCode);
    asmCode << "TEST " << resultReg << " 0x00\n";
    
    // 条件跳转
    asmCode << "JEQ " << endLabel << "\n";
    
    // 生成循环体
    if (node->body) {
        asmCode << generateStatementAsm(node->body.get());
    }
    
    asmCode << "JUMP " << loopLabel << "\n";
    asmCode << endLabel << ":\n";
    
    return asmCode.str();
}

// 生成for循环的汇编代码
std::string HexAsmParser::generateForAsm(ForStatementNode* node) {
    std::stringstream asmCode;
    
    std::string loopLabel = generateLabel("for");
    std::string endLabel = generateLabel("endfor");
    
    // 生成初始化语句
    if (node->initialization) {
        asmCode << generateStatementAsm(node->initialization.get());
    }
    
    asmCode << loopLabel << ":\n";
    
    // 生成条件表达式
    if (node->condition) {
        std::string resultReg = evaluateExpression(node->condition.get(), asmCode);
        asmCode << "TEST " << resultReg << " 0x00\n";
        asmCode << "JEQ " << endLabel << "\n";
    }
    
    // 生成循环体
    if (node->body) {
        asmCode << generateStatementAsm(node->body.get());
    }
    
    // 生成更新语句
    if (node->update) {
        asmCode << generateStatementAsm(node->update.get());
    }
    
    asmCode << "JUMP " << loopLabel << "\n";
    asmCode << endLabel << ":\n";
    
    return asmCode.str();
}

// 评估表达式并返回寄存器引用
std::string HexAsmParser::evaluateExpression(ASTNode* node, std::stringstream& asmCode) {
    if (node->type == NodeType::LITERAL) {
        auto literal = static_cast<LiteralNode*>(node);
        if (literal->literalType == "整型") {
            // 将字符串值转换为整数
            try {
                int value = std::stoi(literal->value);
                std::string tempReg = getTempRegister();
                asmCode << "LOAD " << tempReg << " 0x" << intToHex(value) << "\n";
                return tempReg;
            } catch (...) {
                return "AX";
            }
        } else if (literal->literalType == "字符串") {
            // 字符串字面量：存储到内存并返回地址
            std::string str = literal->value;
            uint32_t addr = allocateStringLiteral(str);
            std::string tempReg = getTempRegister();
            asmCode << "LOAD " << tempReg << " 0x" << intToHex(addr) << "\n";
            return tempReg;
        } else if (literal->literalType == "小数") {
            // 浮点数字面量
            try {
                double value = std::stod(literal->value);
                // 简化处理：转换为整数
                int intValue = static_cast<int>(value);
                std::string tempReg = getTempRegister();
                asmCode << "LOAD " << tempReg << " 0x" << intToHex(intValue) << "\n";
                return tempReg;
            } catch (...) {
                return "AX";
            }
        } else if (literal->literalType == "布尔型") {
            // 布尔值字面量
            bool value = (literal->value == "真");
            std::string tempReg = getTempRegister();
            asmCode << "LOAD " << tempReg << " 0x" << (value ? "01" : "00") << "\n";
            return tempReg;
        } else if (literal->literalType == "字符型") {
            // 字符字面量
            char c = literal->value.empty() ? '\0' : literal->value[0];
            std::string tempReg = getTempRegister();
            asmCode << "LOAD " << tempReg << " 0x" << intToHex(static_cast<int>(static_cast<unsigned char>(c))) << "\n";
            return tempReg;
        }
    } else if (node->type == NodeType::IDENTIFIER) {
        auto identifier = static_cast<IdentifierNode*>(node);
        return getVariableRegister(identifier->name);
    } else if (node->type == NodeType::BINARY_EXPRESSION) {
        auto binary = static_cast<BinaryExpressionNode*>(node);
        return evaluateBinaryExpression(binary, asmCode);
    } else if (node->type == NodeType::UNARY_EXPRESSION) {
        auto unary = static_cast<UnaryExpressionNode*>(node);
        return evaluateUnaryExpression(unary, asmCode);
    } else if (node->type == NodeType::FUNCTION_CALL) {
        auto funcCall = static_cast<FunctionCallNode*>(node);
        return evaluateFunctionCall(funcCall, asmCode);
    }
    
    return "AX"; // 默认返回AX寄存器
}

// 评估一元表达式
std::string HexAsmParser::evaluateUnaryExpression(UnaryExpressionNode* node, std::stringstream& asmCode) {
    std::string operandReg = evaluateExpression(node->operand.get(), asmCode);
    std::string resultReg = getTempRegister(operandReg, "");
    
    if (node->op == "-") {
        // 负号：resultReg = 0 - operandReg
        asmCode << "LOAD " << resultReg << " 0x00\n";
        asmCode << "MINUS " << resultReg << " " << resultReg << " " << operandReg << "\n";
    } else if (node->op == "!") {
        // 逻辑非
        asmCode << "TEST " << operandReg << " 0x00\n";
        asmCode << "LOAD " << resultReg << " 0x01\n";
        asmCode << "JEQ " << generateLabel("not_true") << "\n";
        asmCode << "LOAD " << resultReg << " 0x00\n";
        asmCode << generateLabel("not_true") << ":\n";
    }
    
    return resultReg;
}

// 评估函数调用
std::string HexAsmParser::evaluateFunctionCall(FunctionCallNode* node, std::stringstream& asmCode) {
    
    // 传递参数（简化处理：使用寄存器传递）
    for (size_t i = 0; i < node->arguments.size() && i < 4; i++) {
        std::string argReg = evaluateExpression(node->arguments[i].get(), asmCode);
        std::string paramReg = "AX";
        switch (i) {
            case 0: paramReg = "AX"; break;
            case 1: paramReg = "BX"; break;
            case 2: paramReg = "CX"; break;
            case 3: paramReg = "DX"; break;
        }
        asmCode << "MOVE " << paramReg << " " << argReg << "\n";
    }
    
    // 调用函数：使用CALL指令，调用号为0xFF表示用户函数调用
    // 注意：这里简化处理，实际应该使用函数地址
    // 暂时使用JUMP指令，因为CALL指令只用于系统调用
    asmCode << "JUMP " << node->functionName << "\n";
    
    // 返回值在AX寄存器中
    return "AX";
}

// 评估二元表达式
std::string HexAsmParser::evaluateBinaryExpression(BinaryExpressionNode* node, std::stringstream& asmCode) {
    std::string left = evaluateExpression(node->left.get(), asmCode);
    std::string right = evaluateExpression(node->right.get(), asmCode);
    
    // 如果left和right是同一个寄存器，需要复制其中一个
    if (left == right) {
        std::string newReg = getTempRegister(left, "");
        asmCode << "MOVE " << newReg << " " << left << "\n";
        right = newReg;
    }
    
    std::string resultReg = getTempRegister(left, right);
    
    // 生成运算代码
    if (node->op == "+") {
        asmCode << "MOVE " << resultReg << " " << left << "\n";
        asmCode << "PLUS " << resultReg << " " << right << "\n";
    } else if (node->op == "-") {
        asmCode << "MOVE " << resultReg << " " << left << "\n";
        asmCode << "MINUS " << resultReg << " " << right << "\n";
    } else if (node->op == "*") {
        asmCode << "MOVE " << resultReg << " " << left << "\n";
        asmCode << "TIMES " << resultReg << " " << right << "\n";
    } else if (node->op == "/") {
        asmCode << "MOVE " << resultReg << " " << left << "\n";
        asmCode << "DIVIDE " << resultReg << " " << right << "\n";
    } else if (node->op == "%") {
        // 取模运算
        asmCode << "MOVE CX " << left << "\n";
        asmCode << "DIVIDE CX " << right << "\n";
        asmCode << "TIMES CX " << right << "\n";
        asmCode << "MOVE " << resultReg << " " << left << "\n";
        asmCode << "MINUS " << resultReg << " CX\n";
    } else if (node->op == "==") {
        // 相等比较
        std::string neqLabel = generateLabel("neq_" + std::to_string(node->line));
        asmCode << "TEST " << left << " " << right << "\n";
        asmCode << "LOAD " << resultReg << " 0x00\n";
        asmCode << "JNE " << neqLabel << "\n";
        asmCode << "LOAD " << resultReg << " 0x01\n";
        asmCode << neqLabel << ":\n";
    } else if (node->op == "!=") {
        // 不等比较
        std::string eqLabel = generateLabel("eq_" + std::to_string(node->line));
        asmCode << "TEST " << left << " " << right << "\n";
        asmCode << "LOAD " << resultReg << " 0x01\n";
        asmCode << "JNE " << eqLabel << "\n";
        asmCode << "LOAD " << resultReg << " 0x00\n";
        asmCode << eqLabel << ":\n";
    } else if (node->op == "<") {
        // 小于比较
        std::string ltLabel = generateLabel("lt_" + std::to_string(node->line));
        asmCode << "MOVE " << resultReg << " " << left << "\n";
        asmCode << "MINUS " << resultReg << " " << right << "\n";
        // 使用一个包含0的寄存器进行比较（避免使用AX，因为它可能被占用）
        std::string zeroReg = getTempRegister(resultReg, "");
        asmCode << "LOAD " << zeroReg << " 0x00\n";
        asmCode << "TEST " << resultReg << " " << zeroReg << "\n";
        asmCode << "LOAD " << resultReg << " 0x00\n";
        asmCode << "JNE " << ltLabel << "\n";
        asmCode << "LOAD " << resultReg << " 0x01\n";
        asmCode << ltLabel << ":\n";
    } else if (node->op == "<=") {
        // 小于等于比较
        std::string leLabel = generateLabel("le_" + std::to_string(node->line));
        std::string le2Label = generateLabel("le2_" + std::to_string(node->line));
        asmCode << "MOVE " << resultReg << " " << left << "\n";
        asmCode << "MINUS " << resultReg << " " << right << "\n";
        // 使用一个包含0的寄存器进行比较（避免使用AX，因为它可能被占用）
        std::string zeroReg = getTempRegister(resultReg, "");
        asmCode << "LOAD " << zeroReg << " 0x00\n";
        asmCode << "TEST " << resultReg << " " << zeroReg << "\n";
        asmCode << "LOAD " << resultReg << " 0x01\n";
        asmCode << "JNE " << leLabel << "\n";
        asmCode << "TEST " << left << " " << right << "\n";
        asmCode << "LOAD " << resultReg << " 0x01\n";
        asmCode << "JNE " << le2Label << "\n";
        asmCode << "LOAD " << resultReg << " 0x00\n";
        asmCode << le2Label << ":\n";
        asmCode << leLabel << ":\n";
    } else if (node->op == ">") {
        // 大于比较
        std::string gtLabel = generateLabel("gt_" + std::to_string(node->line));
        asmCode << "MOVE " << resultReg << " " << right << "\n";
        asmCode << "MINUS " << resultReg << " " << left << "\n";
        // 使用一个包含0的寄存器进行比较（避免使用AX，因为它可能被占用）
        std::string zeroReg = getTempRegister(resultReg, "");
        asmCode << "LOAD " << zeroReg << " 0x00\n";
        asmCode << "TEST " << resultReg << " " << zeroReg << "\n";
        asmCode << "LOAD " << resultReg << " 0x00\n";
        asmCode << "JNE " << gtLabel << "\n";
        asmCode << "LOAD " << resultReg << " 0x01\n";
        asmCode << gtLabel << ":\n";
    } else if (node->op == ">=") {
        // 大于等于比较
        std::string geLabel = generateLabel("ge_" + std::to_string(node->line));
        std::string ge2Label = generateLabel("ge2_" + std::to_string(node->line));
        asmCode << "MOVE " << resultReg << " " << right << "\n";
        asmCode << "MINUS " << resultReg << " " << left << "\n";
        // 使用一个包含0的寄存器进行比较（避免使用AX，因为它可能被占用）
        std::string zeroReg = getTempRegister(resultReg, "");
        asmCode << "LOAD " << zeroReg << " 0x00\n";
        asmCode << "TEST " << resultReg << " " << zeroReg << "\n";
        asmCode << "LOAD " << resultReg << " 0x01\n";
        asmCode << "JNE " << geLabel << "\n";
        asmCode << "TEST " << left << " " << right << "\n";
        asmCode << "LOAD " << resultReg << " 0x01\n";
        asmCode << "JNE " << ge2Label << "\n";
        asmCode << "LOAD " << resultReg << " 0x00\n";
        asmCode << ge2Label << ":\n";
        asmCode << geLabel << ":\n";
    } else if (node->op == "&&") {
        // 逻辑与
        std::string andFalseLabel = generateLabel("and_false_" + std::to_string(node->line));
        std::string zeroReg = getTempRegister(left, right);
        asmCode << "LOAD " << zeroReg << " 0x00\n";
        asmCode << "TEST " << left << " " << zeroReg << "\n";
        asmCode << "LOAD " << resultReg << " 0x00\n";
        asmCode << "JEQ " << andFalseLabel << "\n";
        asmCode << "TEST " << right << " " << zeroReg << "\n";
        asmCode << "LOAD " << resultReg << " 0x00\n";
        asmCode << "JEQ " << andFalseLabel << "\n";
        asmCode << "LOAD " << resultReg << " 0x01\n";
        asmCode << andFalseLabel << ":\n";
    } else if (node->op == "||") {
        // 逻辑或
        std::string orTrueLabel = generateLabel("or_true_" + std::to_string(node->line));
        std::string zeroReg = getTempRegister(left, right);
        asmCode << "LOAD " << zeroReg << " 0x00\n";
        asmCode << "TEST " << left << " " << zeroReg << "\n";
        asmCode << "LOAD " << resultReg << " 0x01\n";
        asmCode << "JNE " << orTrueLabel << "\n";
        asmCode << "TEST " << right << " " << zeroReg << "\n";
        asmCode << "LOAD " << resultReg << " 0x01\n";
        asmCode << "JNE " << orTrueLabel << "\n";
        asmCode << "LOAD " << resultReg << " 0x00\n";
        asmCode << orTrueLabel << ":\n";
    }
    
    return resultReg;
}

// 获取变量的寄存器映射
std::string HexAsmParser::getVariableRegister(const std::string& varName) {
    if (varRegisterMap.find(varName) == varRegisterMap.end()) {
        // 分配新寄存器，使用已定义的寄存器（AX, BX, CX, DX）
        std::string reg;
        if (varRegisterCounter == 0) {
            reg = "AX";
        } else if (varRegisterCounter == 1) {
            reg = "BX";
        } else if (varRegisterCounter == 2) {
            reg = "CX";
        } else if (varRegisterCounter == 3) {
            reg = "DX";
        } else {
            // 使用栈上的变量
            reg = "[BP+" + intToHex((varRegisterCounter - 4) * 4) + "]";
        }
        varRegisterMap[varName] = reg;
        varRegisterCounter++;
    }
    
    return varRegisterMap[varName];
}

// 获取临时寄存器
std::string HexAsmParser::getTempRegister() {
    static int tempCounter = 0;
    int idx = tempCounter++ % 4;
    if (idx == 0) return "AX";
    if (idx == 1) return "BX";
    if (idx == 2) return "CX";
    return "DX";
}

// 获取临时寄存器（避免指定的寄存器）
std::string HexAsmParser::getTempRegister(const std::string& avoid1, const std::string& avoid2) {
    for (int i = 0; i < 4; i++) {
        std::string reg;
        if (i == 0) reg = "AX";
        else if (i == 1) reg = "BX";
        else if (i == 2) reg = "CX";
        else reg = "DX";
        
        if (reg != avoid1 && reg != avoid2) {
            return reg;
        }
    }
    return "AX"; // 如果所有寄存器都被占用，返回AX
}

// 生成唯一标签
std::string HexAsmParser::generateLabel(const std::string& prefix) {
    return prefix + "_" + std::to_string(labelCounter++);
}

// 分配字符串字面量到内存
uint32_t HexAsmParser::allocateStringLiteral(const std::string& str) {
    static uint32_t stringAddr = 0x1000;
    uint32_t addr = stringAddr;
    stringLiterals[addr] = str;
    stringAddr += str.length() + 1;
    return addr;
}

// 工具函数实现
std::vector<std::string> HexAsmParser::splitString(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    
    while (std::getline(ss, token, delimiter)) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    
    return tokens;
}

std::string HexAsmParser::trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

bool HexAsmParser::isHexNumber(const std::string& str) {
    if (str.empty()) return false;
    
    // 检查是否以0x或0X开头
    size_t start = 0;
    if (str.length() >= 2 && str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        start = 2;
    }
    
    for (size_t i = start; i < str.length(); i++) {
        if (!isxdigit(str[i])) {
            return false;
        }
    }
    
    return true;
}

uint32_t HexAsmParser::hexToInt(const std::string& hexStr) {
    size_t start = 0;
    if (hexStr.length() >= 2 && hexStr[0] == '0' && (hexStr[1] == 'x' || hexStr[1] == 'X')) {
        start = 2;
    }
    
    uint32_t result = 0;
    for (size_t i = start; i < hexStr.length(); i++) {
        result = result * 16;
        char c = hexStr[i];
        if (c >= '0' && c <= '9') {
            result += (c - '0');
        } else if (c >= 'a' && c <= 'f') {
            result += (c - 'a' + 10);
        } else if (c >= 'A' && c <= 'F') {
            result += (c - 'A' + 10);
        }
    }
    
    return result;
}

std::string HexAsmParser::intToHex(uint32_t value, int width) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(width) << value;
    return ss.str();
}

bool HexAsmParser::parseAndExecuteHexBinary(const std::string& hexFile, bool debug) {
    errorMessage.clear();
    debugMode = debug;
    
    // 初始化虚拟机状态
    for (int i = 0; i < 8; i++) {
        registers[i] = 0;
    }
    programCounter = 0;
    running = true;
    
    // 初始化内存
    memory.clear();
    memory.resize(0x10000, 0); // 64KB内存
    
    // 加载字符串字面量到内存
    for (const auto& entry : stringLiterals) {
        uint32_t addr = entry.first;
        const std::string& str = entry.second;
        for (size_t i = 0; i < str.length() && addr + i < memory.size(); i++) {
            memory[addr + i] = static_cast<uint8_t>(str[i]);
        }
    }
    
    // 解析二进制文件
    if (!parseHexBinary(hexFile)) {
        return false;
    }
    
    // 执行程序
    return executeHexProgram();
}

bool HexAsmParser::parseHexBinary(const std::string& hexFile) {
    // 读取二进制文件
    std::vector<uint8_t> code = readBinaryFile(hexFile);
    if (code.empty()) {
        errorMessage = "无法读取二进制文件: " + hexFile;
        return false;
    }
    
    // 检查文件大小是否合法（应该是4的倍数）
    if (code.size() % 4 != 0) {
        errorMessage = "无效的二进制文件格式: 文件大小不是4的倍数";
        return false;
    }
    
    // 将代码加载到内存起始位置
    for (size_t i = 0; i < code.size() && i < memory.size(); i++) {
        memory[i] = code[i];
    }
    
    return true;
}

bool HexAsmParser::executeHexProgram() {
    if (debugMode) {
        std::cout << "[调试] 开始执行16进制汇编程序..." << std::endl;
    }
    
    while (running && programCounter < memory.size()) {
        // 读取指令（4字节）
        if (programCounter + 3 >= memory.size()) {
            errorMessage = "程序计数器越界";
            return false;
        }
        
        uint32_t instruction = (memory[programCounter] << 24) |
                              (memory[programCounter + 1] << 16) |
                              (memory[programCounter + 2] << 8) |
                              (memory[programCounter + 3]);
        
        if (debugMode) {
            std::cout << "[调试] PC=" << programCounter << " 指令=" << std::hex << instruction << std::dec << std::endl;
        }
        
        // 执行指令
        if (!executeInstruction(instruction)) {
            return false;
        }
        
        // 更新程序计数器（除非是跳转指令）
        programCounter += 4;
    }
    
    if (debugMode) {
        std::cout << "[调试] 程序执行完成" << std::endl;
    }
    return true;
}

bool HexAsmParser::executeInstruction(uint32_t instruction) {
    // 解码指令
    uint8_t opcode = (instruction >> 24) & 0xFF;
    uint8_t reg1 = (instruction >> 20) & 0xF;
    uint8_t reg2 = (instruction >> 16) & 0xF;
    uint16_t immediate = instruction & 0xFFFF;
    
    switch (opcode) {
        case 0x01: // LOAD Rd, imm
            registers[reg1] = immediate;
            break;
            
        case 0x02: // MOVE Rd, Rs
            registers[reg1] = registers[reg2];
            break;
            
        case 0x03: // PLUS Rd, Rs
            registers[reg1] += registers[reg2];
            break;
            
        case 0x04: // MINUS Rd, Rs
            registers[reg1] -= registers[reg2];
            break;
            
        case 0x05: // TIMES Rd, Rs
            registers[reg1] *= registers[reg2];
            break;
            
        case 0x06: // DIVIDE Rd, Rs
            if (registers[reg2] == 0) {
                errorMessage = "除零错误";
                return false;
            }
            registers[reg1] /= registers[reg2];
            break;
            
        case 0x07: // JUMP address
            programCounter = immediate - 4; // 减去4，因为执行后会+4
            break;
            
        case 0x08: // JEQ address
            if (registers[reg1] == 0) {
                programCounter = immediate - 4;
            }
            break;
            
        case 0x09: // JNE address
            if (registers[reg1] != 0) {
                programCounter = immediate - 4;
            }
            break;
            
        case 0x0A: // FETCH Rd, [Rs]
            {
                uint32_t addr = registers[reg2];
                if (addr + 3 < memory.size()) {
                    registers[reg1] = (memory[addr] << 24) | 
                                    (memory[addr + 1] << 16) | 
                                    (memory[addr + 2] << 8) | 
                                    memory[addr + 3];
                } else {
                    registers[reg1] = 0;
                }
            }
            break;
            
        case 0x0B: // SAVE [Rd], Rs
            {
                uint32_t addr = registers[reg1];
                if (addr + 3 < memory.size()) {
                    memory[addr] = (registers[reg2] >> 24) & 0xFF;
                    memory[addr + 1] = (registers[reg2] >> 16) & 0xFF;
                    memory[addr + 2] = (registers[reg2] >> 8) & 0xFF;
                    memory[addr + 3] = registers[reg2] & 0xFF;
                }
            }
            break;
            
        case 0x0C: // CALL callno
            handleSystemCall(immediate);
            break;
            
        case 0x0D: // TEST Rd, Rs
            // 设置状态寄存器
            if (registers[reg1] == registers[reg2]) {
                registers[7] = 1; // 相等
            } else {
                registers[7] = 0; // 不相等
            }
            break;
            
        case 0x0E: // BITAND Rd, Rs
            registers[reg1] &= registers[reg2];
            break;
            
        case 0x0F: // BITOR Rd, Rs
            registers[reg1] |= registers[reg2];
            break;
            
        default:
            errorMessage = "未知操作码: " + std::to_string(opcode);
            return false;
    }
    
    return true;
}

void HexAsmParser::handleSystemCall(uint8_t callNumber) {
    if (debugMode) {
        std::cout << "[调试] 系统调用: " << static_cast<int>(callNumber) << std::endl;
    }
    
    switch (callNumber) {
        case 0x00: // 退出程序
            running = false;
            if (debugMode) {
                std::cout << "[调试] 程序退出" << std::endl;
            }
            break;
            
        case 0x01: // 打印字符
            std::cout << static_cast<char>(registers[1]); // BX寄存器包含字符
            std::cout.flush();
            break;
            
        case 0x02: // 打印整数
            std::cout << registers[1]; // BX寄存器包含整数
            std::cout.flush();
            break;
            
        case 0x03: // 读取输入
            {
                std::string input;
                std::getline(std::cin, input);
                try {
                    registers[1] = std::stoi(input);
                } catch (...) {
                    registers[1] = 0;
                }
            }
            break;
            
        default:
            if (debugMode) {
                std::cout << "[调试] 未知系统调用: " << static_cast<int>(callNumber) << std::endl;
            }
            break;
    }
}

std::vector<uint8_t> HexAsmParser::readBinaryFile(const std::string& filename) {
    std::vector<uint8_t> data;
    std::ifstream file(filename, std::ios::binary);
    
    if (!file) {
        errorMessage = "无法打开文件: " + filename;
        return data;
    }
    
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    
    data.resize(fileSize);
    file.read(reinterpret_cast<char*>(data.data()), fileSize);
    file.close();
    
    return data;
}
