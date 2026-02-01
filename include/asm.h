#ifndef ASM_H
#define ASM_H

#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <memory>
#include <sstream>
#include <iostream>

// 前向声明AST节点类型
class ASTNode;
class VariableDefNode;
class AssignmentNode;
class CoutStatementNode;
class FunctionDefNode;
class ReturnStatementNode;
class IfStatementNode;
class WhileStatementNode;
class ForStatementNode;
class StatementListNode;
class BinaryExpressionNode;
class UnaryExpressionNode;
class FunctionCallNode;

// 16进制汇编语言解析器
class HexAsmParser {
public:
    // 构造函数
    HexAsmParser();
    
    // 解析汇编代码并生成二进制文件
    bool parseAndGenerate(const std::string& asmCode, const std::string& outputFile);
    
    // 将CH代码编译为16进制汇编二进制文件
    bool compileCHToHexBinary(const std::string& chCode, const std::string& outputFile);
    
    // 解析16进制汇编二进制文件并执行
    bool parseAndExecuteHexBinary(const std::string& hexFile, bool debugMode = false);
    
    // 设置调试模式
    void setDebugMode(bool debug) { debugMode = debug; }
    
    // 获取错误信息
    std::string getError() const { return errorMessage; }
    
private:
    // 指令结构
    struct Instruction {
        uint8_t opcode;      // 操作码
        uint8_t reg1;        // 寄存器1
        uint8_t reg2;        // 寄存器2
        uint16_t immediate;  // 立即数/地址
        uint32_t address;    // 指令地址
        std::string label;   // 标签
    };
    
    // 符号表
    struct Symbol {
        std::string name;
        uint32_t address;
        uint16_t value;
    };
    
    // 错误信息
    std::string errorMessage;
    
    // 符号表
    std::map<std::string, uint32_t> symbolTable;
    
    // 指令列表
    std::vector<Instruction> instructions;
    
    // 寄存器映射
    std::map<std::string, uint8_t> registerMap;
    
    // 操作码映射
    std::map<std::string, uint8_t> opcodeMap;
    
    // 初始化寄存器映射
    void initializeRegisterMap();
    
    // 初始化操作码映射
    void initializeOpcodeMap();
    
    // 预处理：移除注释和空白
    std::string preprocessCode(const std::string& code);
    
    // 解析标签
    bool parseLabels(const std::vector<std::string>& lines);
    
    // 解析指令
    bool parseInstructions(const std::vector<std::string>& lines);
    
    // 解析单条指令
    bool parseInstruction(const std::string& line, uint32_t address);
    
    // 生成二进制代码
    std::vector<uint8_t> generateBinary();
    
public:
    // 将CH代码转换为16进制汇编（基于解析器）
    std::string convertCHToHexAsm(const std::string& chCode);
    
    // 基于AST的CH代码转换
    std::string convertCHToHexAsmFromAST(const std::string& chCode);

private:
    
    // 解析CH代码生成AST
    std::unique_ptr<ASTNode> parseCHCode(const std::string& chCode);
    
    // 将AST转换为16进制汇编
    std::string astToHexAsm(std::unique_ptr<ASTNode> ast);
    
    // 生成语句的汇编代码
    std::string generateStatementAsm(ASTNode* node);
    
    // 生成各种语句类型的汇编代码
    std::string generateVariableDefAsm(VariableDefNode* node);
    std::string generateAssignmentAsm(AssignmentNode* node);
    std::string generateCoutAsm(CoutStatementNode* node);
    std::string generateFunctionDefAsm(FunctionDefNode* node);
    std::string generateReturnAsm(ReturnStatementNode* node);
    std::string generateIfAsm(IfStatementNode* node);
    std::string generateWhileAsm(WhileStatementNode* node);
    std::string generateForAsm(ForStatementNode* node);
    std::string generateStatementListAsm(StatementListNode* node);
    
    // 生成输出代码
    std::string generateIntegerOutput(int value);
    std::string generateFloatOutput(double value);
    std::string generateBooleanOutput(bool value);
    
    // 表达式评估
    std::string evaluateExpression(ASTNode* node, std::stringstream& asmCode);
    std::string evaluateBinaryExpression(BinaryExpressionNode* node, std::stringstream& asmCode);
    std::string evaluateUnaryExpression(UnaryExpressionNode* node, std::stringstream& asmCode);
    std::string evaluateFunctionCall(FunctionCallNode* node, std::stringstream& asmCode);
    std::string getVariableRegister(const std::string& varName);
    std::string getTempRegister();
    std::string getTempRegister(const std::string& avoid1, const std::string& avoid2);
    std::string generateLabel(const std::string& prefix);
    uint32_t allocateStringLiteral(const std::string& str);
    
    // 变量寄存器映射
    std::map<std::string, std::string> varRegisterMap;
    int varRegisterCounter;
    int labelCounter;
    std::map<uint32_t, std::string> stringLiterals;
    
    // 解析16进制二进制文件
    bool parseHexBinary(const std::string& hexFile);
    
    // 执行16进制汇编程序
    bool executeHexProgram();
    
    // 虚拟机执行指令
    bool executeInstruction(uint32_t instruction);
    
    // 系统调用处理
    void handleSystemCall(uint8_t callNumber);
    
    // 读取二进制文件
    std::vector<uint8_t> readBinaryFile(const std::string& filename);
    
    // 工具函数：分割字符串
    std::vector<std::string> splitString(const std::string& str, char delimiter);
    
    // 工具函数：去除空白字符
    std::string trim(const std::string& str);
    
    // 工具函数：检查是否为16进制数
    bool isHexNumber(const std::string& str);
    
    // 工具函数：16进制字符串转整数
    uint32_t hexToInt(const std::string& hexStr);
    
    // 工具函数：整数转16进制字符串
    std::string intToHex(uint32_t value, int width = 2);
    
    // 虚拟机状态
    uint32_t registers[8];  // AX, BX, CX, DX, IP, ST, BP, SR
    std::vector<uint8_t> memory;
    uint32_t programCounter;
    bool running;
    bool debugMode;
};

// 16进制汇编指令定义
namespace HexAsm {
    // 操作码定义
    constexpr uint8_t OP_LDI = 0x01;  // 加载立即数
    constexpr uint8_t OP_MOV = 0x02;  // 寄存器传输
    constexpr uint8_t OP_ADD = 0x03;  // 加法
    constexpr uint8_t OP_SUB = 0x04;  // 减法
    constexpr uint8_t OP_MUL = 0x05;  // 乘法
    constexpr uint8_t OP_DIV = 0x06;  // 除法
    constexpr uint8_t OP_JMP = 0x07;  // 跳转
    constexpr uint8_t OP_JZ  = 0x08;  // 为零跳转
    constexpr uint8_t OP_JNZ = 0x09;  // 非零跳转
    constexpr uint8_t OP_LD  = 0x0A;  // 内存加载
    constexpr uint8_t OP_ST  = 0x0B;  // 内存存储
    constexpr uint8_t OP_SYS = 0x0C;  // 系统调用
    constexpr uint8_t OP_CMP = 0x0D;  // 比较
    constexpr uint8_t OP_AND = 0x0E;  // 逻辑与
    constexpr uint8_t OP_OR  = 0x0F;  // 逻辑或
    
    // 寄存器定义
    constexpr uint8_t REG_R0 = 0x0;   // 通用寄存器0
    constexpr uint8_t REG_R1 = 0x1;   // 通用寄存器1
    constexpr uint8_t REG_R2 = 0x2;   // 通用寄存器2
    constexpr uint8_t REG_R3 = 0x3;   // 通用寄存器3
    constexpr uint8_t REG_PC = 0x4;   // 程序计数器
    constexpr uint8_t REG_SP = 0x5;   // 堆栈指针
    constexpr uint8_t REG_BP = 0x6;   // 基址指针
    constexpr uint8_t REG_FL = 0x7;   // 标志寄存器
    
    // 系统调用号
    constexpr uint8_t SYS_EXIT = 0x00;    // 退出程序
    constexpr uint8_t SYS_PRINT = 0x01;   // 打印字符
    constexpr uint8_t SYS_READ = 0x02;    // 读取输入
    constexpr uint8_t SYS_OPEN = 0x03;    // 打开文件
    constexpr uint8_t SYS_CLOSE = 0x04;   // 关闭文件
}

#endif // ASM_H