#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "parser.h"
#include "FileMemory.h"
#include "MemoryStorage.h"
#include "BigInt.h"
#include <memory>
#include <string>
#include <vector>
#include <set>
#include <map>

// 结构体定义信息
struct StructInfo {
    std::string structName;
    std::vector<std::pair<std::string, std::string>> members;  // 成员类型和成员名
    
    StructInfo() : structName(""), members() {}
    StructInfo(const std::string& name, const std::vector<std::pair<std::string, std::string>>& mems)
        : structName(name), members(mems) {}
    
    // 拷贝构造函数
    StructInfo(const StructInfo& other) 
        : structName(other.structName), members(other.members) {}
};

// 符号表
class SymbolTable {
private:
    std::map<std::string, std::pair<std::string, std::string>> variables;
    std::map<std::string, std::vector<FunctionDefNode*>> functions;
    std::map<std::string, StructInfo> structs;
    SymbolTable* parent;
    FileMemory* fileMemory; // 文件内存存储
    MemoryStorage* memoryStorage; // 纯内存存储
    bool useFileMemory; // 是否使用文件内存存储
    bool useMemoryStorage; // 是否使用纯内存存储
    
public:
    SymbolTable(SymbolTable* parent = nullptr, FileMemory* fileMem = nullptr, bool useFileMem = false, MemoryStorage* memStorage = nullptr, bool useMemStorage = false);
    ~SymbolTable();
    
    void defineVariable(const std::string& name, const std::string& type, const std::string& value = "", int line = 0);
    void setVariable(const std::string& name, const std::string& value, int line = 0);
    std::string getVariable(const std::string& name, int line = 0) const;
    std::string getVariableType(const std::string& name) const;
    bool hasVariable(const std::string& name) const;
    
    void defineFunction(FunctionDefNode* function);
    FunctionDefNode* getFunction(const std::string& name, const std::vector<std::string>& argTypes, int line = 0) const;
    bool hasFunction(const std::string& name) const;
    
    void defineStruct(const std::string& name, const std::vector<std::pair<std::string, std::string>>& members);
    StructInfo getStruct(const std::string& name, int line = 0) const;
    bool hasStruct(const std::string& name) const;
    bool hasStructMember(const std::string& structName, const std::string& memberName, int line = 0) const;
    std::string createStructInstance(const std::string& structName, int line = 0);
    
    SymbolTable* enterScope();
    SymbolTable* getParent() const;
    bool isGlobalScope() const;
    
    void defineVariable(const std::string& name, const std::string& type, const std::string& value, int line, bool initArrayElements = true);
};

// 执行器
class Interpreter {
private:
    std::unique_ptr<ProgramNode> program;
    SymbolTable* globalScope;
    std::set<std::string> importedFiles; // 跟踪已导入的文件，防止循环导入
    bool debugMode; // 调试模式标志
    FileMemory* fileMemory; // 文件内存存储
    MemoryStorage* memoryStorage; // 纯内存存储
    bool useFileMemory; // 是否使用文件内存存储
    bool useMemoryStorage; // 是否使用纯内存存储
    bool reserveMemory; // 是否保留内存文件
    
    std::string evaluate(ASTNode* node, SymbolTable* scope);
    void executeStatement(ASTNode* node, SymbolTable* scope, const std::string& expectedReturnType = "");
    void importFile(const std::string& filePath, int line);
    void executeSystemCommand(ASTNode* commandNode, SymbolTable* scope, int line);
    std::string executeSystemCommandExpression(ASTNode* commandNode, SymbolTable* scope, int line);
    std::string executeCommandWithOutput(const std::string& command);
    
    // 调试输出函数
    void debugOutput(const std::string& message);
    void debugTokenInfo(const std::vector<Token>& tokens);
    void debugASTInfo(ASTNode* node, int depth = 0);
    void debugSymbolTable(SymbolTable* scope, const std::string& scopeName = "当前作用域");
    
public:
    Interpreter(std::unique_ptr<ProgramNode> program, bool debug = false, bool useFileMem = false, bool useMemStorage = false, bool reserveMem = false);
    ~Interpreter();
    
    void run();
    void setDebugMode(bool debug);
};

#endif // INTERPRETER_H