#include "../include/interpreter.h"
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <cstdlib>
#include <chrono>
#include <cctype>
#include <cstring>
#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#endif

// Break和Continue异常类
class BreakException : public std::exception {
public:
    BreakException() : std::exception() {}
};

class ContinueException : public std::exception {
public:
    ContinueException() : std::exception() {}
};

// 简单的类型推断函数
std::string inferType(const std::string& value) {
    if (value == "真" || value == "假") {
        return "布尔型";
    } else if (value.find(".") != std::string::npos) {
        return "小数";
    } else if (value.length() > 0 && (value[0] == '"' || value[0] == '\'')) {
        return "字符串";
    } else if (value == "") {
        return "字符串"; // 空字符串明确返回字符串类型
    } else {
        // 对于函数返回值，保持其原始类型
        // 这里不尝试解析为整数，因为字符串类型的数字也应该被视为字符串
        return "字符串"; // 默认当作字符串
    }
}

// 符号表构造函数
SymbolTable::SymbolTable(SymbolTable* parent)
    : parent(parent) {}

// 符号表析构函数
SymbolTable::~SymbolTable() {
    // 清理子作用域
}

// 定义变量
void SymbolTable::defineVariable(const std::string& name, const std::string& type, const std::string& value, int line, bool initArrayElements) {
    if (variables.find(name) != variables.end()) {
        std::string lineInfo = line > 0 ? " 在第 " + std::to_string(line) + " 行" : "";
        throw std::runtime_error("变量已定义: " + name + lineInfo);
    }
    
    // 检查是否是数组参数（包含方括号）
    if (name.find("[") != std::string::npos && name.find("]") != std::string::npos && initArrayElements) {
        // 提取数组名和维度大小
        std::string arrayName = name.substr(0, name.find("["));
        std::string dimsStr = name.substr(name.find("["));
        
        // 解析所有维度大小
        std::vector<int> dimensions;
        size_t pos = 0;
        while (pos < dimsStr.length()) {
            if (dimsStr[pos] == '[') {
                size_t endPos = dimsStr.find(']', pos);
                if (endPos == std::string::npos) {
                    throw std::runtime_error("数组参数语法错误: " + name);
                }
                
                std::string sizeStr = dimsStr.substr(pos + 1, endPos - pos - 1);
                try {
                    int dimensionSize = std::stoi(sizeStr);
                    dimensions.push_back(dimensionSize);
                } catch (const std::exception& e) {
                    throw std::runtime_error("数组参数维度大小无效: " + sizeStr);
                }
                pos = endPos + 1;
            } else {
                pos++;
            }
        }
        
        // 定义数组本身
        variables[name] = std::make_pair(type, value);
        
        // 初始化所有数组元素
        std::vector<int> indices(dimensions.size(), 0);
        bool initialized = true;
        
        while (initialized) {
            // 构造元素名称
            std::string elementName = arrayName;
            for (int idx : indices) {
                elementName += "[" + std::to_string(idx) + "]";
            }
            
            // 根据类型初始化默认值
            std::string defaultValue = "0";
            if (type == "字符串") {
                defaultValue = "\"\"";
            } else if (type == "小数") {
                defaultValue = "0.0";
            } else if (type == "布尔型") {
                defaultValue = "假";
            } else if (type == "字符型") {
                defaultValue = "'";
            }
            
            // 定义数组元素
            variables[elementName] = std::make_pair(type, defaultValue);
            
            // 更新索引
            int dimIndex = dimensions.size() - 1;
            while (dimIndex >= 0) {
                if (dimIndex >= static_cast<int>(indices.size())) {
                    initialized = false;
                    break;
                }
                indices[dimIndex]++;
                if (dimIndex < static_cast<int>(dimensions.size()) && indices[dimIndex] < dimensions[dimIndex]) {
                    break;
                }
                indices[dimIndex] = 0;
                dimIndex--;
            }
            
            if (dimIndex < 0) {
                initialized = false;
            }
        }
    } else {
        variables[name] = std::make_pair(type, value);
    }
}

// 设置变量值
void SymbolTable::setVariable(const std::string& name, const std::string& value, int line) {
    if (variables.find(name) != variables.end()) {
        variables[name].second = value;
    } else if (parent) {
        parent->setVariable(name, value, line);
    } else {
        std::string lineInfo = line > 0 ? " 在第 " + std::to_string(line) + " 行" : "";
        throw std::runtime_error("变量未定义: " + name + lineInfo);
    }
}

// 获取变量值
std::string SymbolTable::getVariable(const std::string& name, int line) const {
    if (variables.find(name) != variables.end()) {
        return variables.at(name).second;
    } else if (parent) {
        return parent->getVariable(name, line);
    } else {
        std::string lineInfo = line > 0 ? " 在第 " + std::to_string(line) + " 行" : "";
        throw std::runtime_error("变量未定义: " + name + lineInfo);
    }
}

// 获取变量类型
std::string SymbolTable::getVariableType(const std::string& name) const {
    if (variables.find(name) != variables.end()) {
        return variables.at(name).first;
    } else if (parent) {
        return parent->getVariableType(name);
    } else {
        return "";
    }
}

// 检查变量是否存在
bool SymbolTable::hasVariable(const std::string& name) const {
    if (variables.find(name) != variables.end()) {
        return true;
    } else if (parent) {
        return parent->hasVariable(name);
    } else {
        return false;
    }
}

// 定义函数
void SymbolTable::defineFunction(FunctionDefNode* function) {
    std::string funcName = function->name;
    
    // 提取函数签名
    std::vector<std::string> paramTypes;
    for (const auto& param : function->parameters) {
        paramTypes.push_back(param.first);
    }
    
    // 检查是否有参数签名完全相同的函数
    for (const auto& existingFunc : functions[funcName]) {
        std::vector<std::string> existingParamTypes;
        for (const auto& param : existingFunc->parameters) {
            existingParamTypes.push_back(param.first);
        }
        
        if (paramTypes == existingParamTypes) {
            std::string lineInfo = function->line > 0 ? " 在第 " + std::to_string(function->line) + " 行" : "";
            throw std::runtime_error("函数已定义: " + funcName + " 带有相同参数类型" + lineInfo);
        }
    }
    
    functions[funcName].push_back(function);
}

// 获取函数
FunctionDefNode* SymbolTable::getFunction(const std::string& name, const std::vector<std::string>& argTypes, int line) const {
    auto it = functions.find(name);
    if (it != functions.end()) {
        // 调试信息：显示符号表中的函数定义
        // std::cout << "[调试] 符号表中有 " << it->second.size() << " 个 " << name << " 函数定义" << std::endl;
        
        // 查找匹配的函数定义
        for (const auto& func : it->second) {
            std::vector<std::string> paramTypes;
            for (const auto& param : func->parameters) {
                paramTypes.push_back(param.first);
            }
            
            // 调试信息：显示每个函数的参数类型
            // std::cout << "[调试] 检查函数: " << name << "(";
            // for (size_t i = 0; i < paramTypes.size(); ++i) {
            //     if (i > 0) std::cout << ", ";
            //     std::cout << paramTypes[i];
            // }
            // std::cout << ")" << std::endl;
            
            if (paramTypes == argTypes) {
                // std::cout << "[调试] 找到匹配的函数定义!" << std::endl;
                return func;
            }
        }
        
        // 没有找到匹配的函数
        std::string argTypesStr;
        for (size_t i = 0; i < argTypes.size(); ++i) {
            if (i > 0) argTypesStr += ", ";
            argTypesStr += argTypes[i];
        }
        
        std::string lineInfo = line > 0 ? " 在第 " + std::to_string(line) + " 行" : "";
        throw std::runtime_error("函数未定义: " + name + "(" + argTypesStr + ")" + lineInfo);
    } else if (parent) {
        return parent->getFunction(name, argTypes, line);
    } else {
        std::string lineInfo = line > 0 ? " 在第 " + std::to_string(line) + " 行" : "";
        throw std::runtime_error("函数未定义: " + name + lineInfo);
    }
}

// 检查函数是否存在
bool SymbolTable::hasFunction(const std::string& name) const {
    if (functions.find(name) != functions.end()) {
        return true;
    } else if (parent) {
        return parent->hasFunction(name);
    } else {
        return false;
    }
}

// 进入新作用域
SymbolTable* SymbolTable::enterScope() {
    return new SymbolTable(this);
}

// 获取父作用域
SymbolTable* SymbolTable::getParent() const {
    return parent;
}

// 判断是否是全局作用域
bool SymbolTable::isGlobalScope() const {
    return parent == nullptr;
}

// 定义结构体
void SymbolTable::defineStruct(const std::string& name, const std::vector<std::pair<std::string, std::string>>& members) {
    if (structs.find(name) != structs.end()) {
        throw std::runtime_error("结构体已定义: " + name);
    }
    structs[name] = StructInfo(name, members);
}

// 获取结构体定义
StructInfo SymbolTable::getStruct(const std::string& name, int line) const {
    auto it = structs.find(name);
    if (it != structs.end()) {
        return it->second;
    } else if (parent) {
        return parent->getStruct(name, line);
    } else {
        std::string lineInfo = line > 0 ? " 在第 " + std::to_string(line) + " 行" : "";
        throw std::runtime_error("结构体未定义: " + name + lineInfo);
    }
}

// 创建结构体实例
std::string SymbolTable::createStructInstance(const std::string& structName, int line) {
    StructInfo structInfo = getStruct(structName, line);
    std::string instance = structName + ":";
    
    for (size_t i = 0; i < structInfo.members.size(); ++i) {
        const auto& member = structInfo.members[i];
        
        // 根据成员类型设置默认值
        if (member.first == "整型") {
            instance += "0";
        } else if (member.first == "小数") {
            instance += "0.0";
        } else if (member.first == "布尔型") {
            instance += "false";
        } else if (member.first == "字符串") {
            instance += "";
        } else if (member.first == "字符型") {
            instance += "'";
        } else {
            // 如果是结构体类型，创建一个空实例
            instance += createStructInstance(member.first, line);
        }
        
        if (i < structInfo.members.size() - 1) {
            instance += ";";
        }
    }
    
    return instance;
}

// 检查结构体是否存在
bool SymbolTable::hasStruct(const std::string& name) const {
    if (structs.find(name) != structs.end()) {
        return true;
    } else if (parent) {
        return parent->hasStruct(name);
    } else {
        return false;
    }
}

// 检查结构体成员是否存在
bool SymbolTable::hasStructMember(const std::string& structName, const std::string& memberName, int line) const {
    try {
        StructInfo structInfo = getStruct(structName, line);
        for (const auto& member : structInfo.members) {
            if (member.second == memberName) {
                return true;
            }
        }
        return false;
    } catch (const std::runtime_error&) {
        return false;
    }
}

// 执行器构造函数
Interpreter::Interpreter(std::unique_ptr<ProgramNode> program, bool debug)
    : program(std::move(program)), globalScope(new SymbolTable()), debugMode(debug) {
    // 初始化全局作用域
}

// 执行器析构函数
Interpreter::~Interpreter() {
    delete globalScope;
}

// 运行程序
void Interpreter::run() {
    // 调试信息：开始执行程序
    debugOutput("开始执行程序");
    
    // 首先处理所有全局声明（函数定义、结构体定义、导入语句、变量定义）
    for (const auto& stmt : program->statements) {
        debugOutput("处理语句: 类型=" + std::to_string(static_cast<int>(stmt->type)) + ", 行号=" + std::to_string(stmt->line));
        if (stmt->type == NodeType::FUNCTION_DEF) {
            FunctionDefNode* func = static_cast<FunctionDefNode*>(stmt.get());
            globalScope->defineFunction(func);
        } else if (stmt->type == NodeType::STRUCT_DEF) {
            StructDefNode* structDef = static_cast<StructDefNode*>(stmt.get());
            globalScope->defineStruct(structDef->structName, structDef->members);
        } else if (stmt->type == NodeType::IMPORT_STATEMENT) {
            // 处理导入语句
            ImportStatementNode* importStmt = static_cast<ImportStatementNode*>(stmt.get());
            importFile(importStmt->filePath, stmt->line);
        } else if (stmt->type == NodeType::SYSTEM_CMD_STATEMENT) {
            // 处理全局系统命令语句
            SystemCmdStatementNode* cmdStmt = static_cast<SystemCmdStatementNode*>(stmt.get());
            executeSystemCommand(cmdStmt->commandExpr.get(), globalScope, stmt->line);
        } else if (stmt->type == NodeType::STATEMENT_LIST) {
            // 处理语句列表（可能包含多个变量定义）
            StatementListNode* stmtList = static_cast<StatementListNode*>(stmt.get());
            for (const auto& innerStmt : stmtList->statements) {
                if (innerStmt->type == NodeType::VARIABLE_DEF) {
                    // 处理全局变量定义
                    VariableDefNode* varDef = static_cast<VariableDefNode*>(innerStmt.get());
                    
                    // 检查是否是数组定义
                    if (varDef->isArray) {
                        // 处理数组定义
                        std::vector<int> dimensions;
                        
                        // 处理第一维
                        if (varDef->arraySizeExpr) {
                            std::string sizeValue = evaluate(varDef->arraySizeExpr.get(), globalScope);
                            try {
                                int size = std::stoi(sizeValue);
                                if (size <= 0) {
                                    throw std::runtime_error("数组大小必须为正整数 在第 " + std::to_string(stmt->line) + " 行");
                                }
                                if (size > 1000) {
                                    throw std::runtime_error("数组维度大小过大，最大允许1000 在第 " + std::to_string(stmt->line) + " 行");
                                }
                                dimensions.push_back(size);
                            } catch (const std::invalid_argument& e) {
                                throw std::runtime_error("数组大小必须是整数: " + sizeValue + " 在第 " + std::to_string(stmt->line) + " 行");
                            } catch (const std::out_of_range& e) {
                                throw std::runtime_error("数组大小超出范围: " + sizeValue + " 在第 " + std::to_string(stmt->line) + " 行");
                            }
                        } else {
                            // 空数组或未指定大小，使用默认值
                            dimensions.push_back(10); // 默认大小
                        }
                        
                        // 初始化数组值
                        std::string arrayValue = varDef->type + ":";
                        std::vector<int> indices(dimensions.size(), 0);
                        bool initialized = true;
                        
                        // 检查是否是大括号初始化列表
                        bool useBraceInit = varDef->initializer && varDef->initializer->type == NodeType::BRACE_INIT_LIST;
                        std::vector<std::string> braceInitValues;
                        if (useBraceInit) {
                            BraceInitListNode* braceInit = static_cast<BraceInitListNode*>(varDef->initializer.get());
                            // 递归解析嵌套的初始化列表
                            std::function<void(BraceInitListNode*, std::vector<std::string>&)> parseBraceInit = 
                                [&](BraceInitListNode* node, std::vector<std::string>& values) {
                                    for (const auto& element : node->elements) {
                                        if (element->type == NodeType::BRACE_INIT_LIST) {
                                            parseBraceInit(static_cast<BraceInitListNode*>(element.get()), values);
                                        } else {
                                            values.push_back(evaluate(element.get(), globalScope));
                                        }
                                    }
                                };
                            parseBraceInit(braceInit, braceInitValues);
                        }
                        
                        int elementIndex = 0;
                        while (initialized) {
                            // 根据类型初始化元素值
                            std::string elementValue;
                            if (varDef->type == "整型" || varDef->type == "小数" || varDef->type == "布尔型") {
                                // 基本类型：初始化为0
                                elementValue = "0";
                            } else if (varDef->type == "字符串") {
                                // 字符串类型：初始化为空字符串
                                elementValue = "";
                            } else if (varDef->type == "字符型") {
                                // 字符类型：初始化为空字符
                                elementValue = "'";
                            } else {
                                // 结构体类型：创建结构体实例
                                elementValue = globalScope->createStructInstance(varDef->type, stmt->line);
                                // 移除重复的类型前缀
                                size_t colonPos = elementValue.find(':');
                                if (colonPos != std::string::npos) {
                                    elementValue = elementValue.substr(colonPos + 1);
                                }
                            }
                            
                            // 如果是大括号初始化列表，使用列表中的值
                            if (useBraceInit && elementIndex < static_cast<int>(braceInitValues.size())) {
                                elementValue = braceInitValues[elementIndex];
                            }
                            
                            // 将元素值添加到数组值中
                            arrayValue += elementValue + ";";
                            
                            elementIndex++;
                            
                            // 更新索引
                            int dim = -1;
                            if (!dimensions.empty()) {
                                dim = dimensions.size() - 1;
                                while (dim >= 0) {
                                    if (dim >= static_cast<int>(indices.size())) {
                                        initialized = false;
                                        break;
                                    }
                                    indices[dim]++;
                                    if (dim < static_cast<int>(dimensions.size()) && indices[dim] < dimensions[dim]) {
                                        break;
                                    } else {
                                        indices[dim] = 0;
                                        dim--;
                                    }
                                }
                            } else {
                                initialized = false;
                            }
                            
                            // 检查是否完成所有元素的初始化
                            if (dim < 0) {
                                initialized = false;
                            }
                        }
                        
                        // 定义全局数组变量（不初始化数组元素，稍后手动初始化）
                        globalScope->defineVariable(varDef->name, varDef->type, arrayValue, stmt->line, false);
                        
                        // 手动初始化数组元素
                        indices.assign(dimensions.size(), 0);
                        initialized = true;
                        elementIndex = 0;
                        while (initialized) {
                            // 构造元素名称
                            std::string elementName = varDef->name;
                            for (int idx : indices) {
                                elementName += "[" + std::to_string(idx) + "]";
                            }
                            
                            // 根据类型初始化默认值
                            std::string elementValue;
                            if (varDef->type == "整型" || varDef->type == "小数" || varDef->type == "布尔型") {
                                elementValue = "0";
                            } else if (varDef->type == "字符串") {
                                elementValue = "\"\"";
                            } else if (varDef->type == "字符型") {
                                elementValue = "'";
                            } else {
                                elementValue = globalScope->createStructInstance(varDef->type, stmt->line);
                                size_t colonPos = elementValue.find(':');
                                if (colonPos != std::string::npos) {
                                    elementValue = elementValue.substr(colonPos + 1);
                                }
                            }
                            
                            // 如果是大括号初始化列表，使用列表中的值
                            if (useBraceInit && elementIndex < static_cast<int>(braceInitValues.size())) {
                                elementValue = braceInitValues[elementIndex];
                            }
                            
                            // 手动定义数组元素
                            globalScope->defineVariable(elementName, varDef->type, elementValue, stmt->line, false);
                            
                            elementIndex++;
                            
                            // 更新索引
                            int dim = dimensions.size() - 1;
                            while (dim >= 0) {
                                if (dim >= static_cast<int>(indices.size())) {
                                    initialized = false;
                                    break;
                                }
                                indices[dim]++;
                                if (dim < static_cast<int>(dimensions.size()) && indices[dim] < dimensions[dim]) {
                                    break;
                                } else {
                                    indices[dim] = 0;
                                    dim--;
                                }
                            }
                            
                            if (dim < 0) {
                                initialized = false;
                            }
                        }
                    } else {
                        // 处理普通变量定义
                        std::string value = "";
                        if (varDef->initializer) {
                            value = evaluate(varDef->initializer.get(), globalScope);
                        } else {
                            // 根据类型设置默认值
                            if (varDef->type == "整型") {
                                value = "0";
                            } else if (varDef->type == "小数") {
                                value = "0.0";
                            } else if (varDef->type == "布尔型") {
                                value = "假";
                            } else if (varDef->type == "字符串") {
                                value = "\"\"";
                            } else if (varDef->type == "字符型") {
                                value = "'";
                            }
                        }
                        
                        // 定义全局变量
                        globalScope->defineVariable(varDef->name, varDef->type, value, stmt->line, true);
                    }
                }
            }
        }
    }
    
    // 然后查找并执行主函数
    if (globalScope->hasFunction("主函数")) {
        FunctionDefNode* mainFunc = globalScope->getFunction("主函数", std::vector<std::string>(), 0);
        SymbolTable* mainScope = globalScope->enterScope();
        // 创建一个返回值存储变量
        mainScope->defineVariable("__return_value", mainFunc->returnType, "", 0, true);
        try {
            executeStatement(mainFunc->body.get(), mainScope, "空类型");
        } catch (const std::runtime_error& e) {
            // 主函数中的返回语句异常不需要处理
            if (std::string(e.what()) != "__return_from_function__") {
                delete mainScope;
                throw;
            }
            // 对于主函数，即使是正常的函数结束也不应该抛出异常
        }
        delete mainScope;
    } else {
        throw std::runtime_error("未找到主函数 在第 0 行");
    }
}

// 执行语句
void Interpreter::executeStatement(ASTNode* node, SymbolTable* scope, const std::string& expectedReturnType) {
    // 检查全局作用域限制：函数之外只能定义赋值，不允许其他操作
    if (scope->isGlobalScope()) {
        // 在全局作用域中，只允许变量定义、结构体定义和函数定义
        // 函数定义和结构体定义已经在run()函数中特殊处理
        // 变量定义可以在全局作用域中执行
        switch (node->type) {
            case NodeType::VARIABLE_DEF:
            case NodeType::STRUCT_DEF:
            case NodeType::FUNCTION_DEF:
                break; // 允许这些操作
            default:
                // 只在非特殊处理的语句中检查
                throw std::runtime_error("在全局作用域中不允许执行此操作，只能定义变量、结构体或函数 在第 " + std::to_string(node->line) + " 行");
        }
    }
    
    switch (node->type) {
        case NodeType::VARIABLE_DEF: {
            VariableDefNode* varDef = static_cast<VariableDefNode*>(node);
            
            // 如果是数组定义，处理动态数组大小
            if (varDef->isArray) {
                std::vector<int> dimensions;
                
                // 处理所有维度（最多5维）
                if (!varDef->arraySizeExprs.empty()) {
                    for (const auto& sizeExpr : varDef->arraySizeExprs) {
                        std::string sizeValue = evaluate(sizeExpr.get(), scope);
                        debugOutput("数组维度大小: " + sizeValue);
                        try {
                            int size = std::stoi(sizeValue);
                            if (size <= 0) {
                                throw std::runtime_error("数组大小必须为正整数 在第 " + std::to_string(node->line) + " 行");
                            }
                            if (size > 1000) {
                                throw std::runtime_error("数组维度大小过大，最大允许1000 在第 " + std::to_string(node->line) + " 行");
                            }
                            dimensions.push_back(size);
                        } catch (const std::invalid_argument& e) {
                            throw std::runtime_error("数组大小必须是整数: " + sizeValue + " 在第 " + std::to_string(node->line) + " 行");
                        } catch (const std::out_of_range& e) {
                            throw std::runtime_error("数组大小超出范围: " + sizeValue + " 在第 " + std::to_string(node->line) + " 行");
                        }
                    }
                } else if (varDef->arraySizeExpr) {
                    // 兼容旧代码：只处理第一维
                    std::string sizeValue = evaluate(varDef->arraySizeExpr.get(), scope);
                    try {
                        int size = std::stoi(sizeValue);
                        if (size <= 0) {
                            throw std::runtime_error("数组大小必须为正整数 在第 " + std::to_string(node->line) + " 行");
                        }
                        if (size > 1000) {
                            throw std::runtime_error("数组维度大小过大，最大允许1000 在第 " + std::to_string(node->line) + " 行");
                        }
                        dimensions.push_back(size);
                    } catch (const std::invalid_argument& e) {
                        throw std::runtime_error("数组大小必须是整数: " + sizeValue + " 在第 " + std::to_string(node->line) + " 行");
                    } catch (const std::out_of_range& e) {
                        throw std::runtime_error("数组大小超出范围: " + sizeValue + " 在第 " + std::to_string(node->line) + " 行");
                    }
                } else {
                    // 空数组或未指定大小，使用默认值
                    dimensions.push_back(10); // 默认大小
                }
                
                debugOutput("数组 " + varDef->name + " 维度数量: " + std::to_string(dimensions.size()));
                
                // 定义数组变量（不初始化数组元素，稍后手动初始化）
                std::string value = "";
                if (varDef->initializer) {
                    value = evaluate(varDef->initializer.get(), scope);
                }
                scope->defineVariable(varDef->name, varDef->type, value, varDef->line, false);
                
                // 初始化数组元素
                std::vector<int> indices(dimensions.size(), 0);
                bool initialized = true;
                
                // 检查是否是大括号初始化列表
                bool useBraceInit = varDef->initializer && varDef->initializer->type == NodeType::BRACE_INIT_LIST;
                std::vector<std::string> braceInitValues;
                if (useBraceInit) {
                    BraceInitListNode* braceInit = static_cast<BraceInitListNode*>(varDef->initializer.get());
                    // 递归解析嵌套的初始化列表
                    std::function<void(BraceInitListNode*, std::vector<std::string>&)> parseBraceInit = 
                        [&](BraceInitListNode* node, std::vector<std::string>& values) {
                            for (const auto& element : node->elements) {
                                if (element->type == NodeType::BRACE_INIT_LIST) {
                                    parseBraceInit(static_cast<BraceInitListNode*>(element.get()), values);
                                } else {
                                    values.push_back(evaluate(element.get(), scope));
                                }
                            }
                        };
                    parseBraceInit(braceInit, braceInitValues);
                }
                
                int elementIndex = 0;
                while (initialized) {
                    // 构造元素名称
                    std::string elementName = varDef->name;
                    for (int idx : indices) {
                        elementName += "[" + std::to_string(idx) + "]";
                    }
                    
                    debugOutput("定义数组元素: " + elementName);
                    
                    // 根据类型初始化默认值
                    std::string defaultValue = "0";
                    if (varDef->type == "字符串") {
                        defaultValue = "\"\"";
                    } else if (varDef->type == "小数") {
                        defaultValue = "0.0";
                    } else if (varDef->type == "布尔型") {
                        defaultValue = "假";
                    } else if (varDef->type == "字符型") {
                        defaultValue = "'";
                    }
                    
                    // 如果是大括号初始化列表，使用列表中的值
                    if (useBraceInit && elementIndex < static_cast<int>(braceInitValues.size())) {
                        defaultValue = braceInitValues[elementIndex];
                    }
                    
                    // 手动定义数组元素
                    scope->defineVariable(elementName, varDef->type, defaultValue, varDef->line, false);
                    
                    elementIndex++;
                    
                    // 更新索引
                    int dimIndex = dimensions.size() - 1;
                    while (dimIndex >= 0) {
                        if (dimIndex >= static_cast<int>(indices.size())) {
                            initialized = false;
                            break;
                        }
                        indices[dimIndex]++;
                        if (dimIndex < static_cast<int>(dimensions.size()) && indices[dimIndex] < dimensions[dimIndex]) {
                            break;
                        }
                        indices[dimIndex] = 0;
                        dimIndex--;
                    }
                    
                    if (dimIndex < 0) {
                        initialized = false;
                    }
                }
                
                // 数组定义不返回值
                return;
            }
            
            // 检查是否是数组定义（通过检查变量名是否包含方括号）
            if (varDef->name.find("[") != std::string::npos && varDef->name.find("]") != std::string::npos) {
                // 提取数组名和维度信息
                std::string arrayName = varDef->name.substr(0, varDef->name.find("["));
                std::string dimsStr = varDef->name.substr(varDef->name.find("["));
                
                // 解析所有维度大小
                std::vector<int> dimensions;
                size_t pos = 0;
                int dimensionCount = 0;
                
                while (pos < dimsStr.length() && dimensionCount < 5) {
                    if (dimsStr[pos] == '[') {
                        size_t endPos = dimsStr.find(']', pos);
                        if (endPos == std::string::npos) {
                            throw std::runtime_error("数组定义语法错误: " + varDef->name + " 在第 " + std::to_string(node->line) + " 行");
                        }
                        
                        std::string sizeStr = dimsStr.substr(pos + 1, endPos - pos - 1);
                        if (sizeStr.empty()) {
                            throw std::runtime_error("数组维度大小不能为空: " + varDef->name + " 在第 " + std::to_string(node->line) + " 行");
                        }
                        
                        try {
                            int dimensionSize;
                            
                            // 检查是否是纯数字（固定大小）
                            bool isNumeric = true;
                            for (char c : sizeStr) {
                                if (!std::isdigit(c) && c != '-' && c != '+') {
                                    isNumeric = false;
                                    break;
                                }
                            }
                            
                            if (isNumeric && !sizeStr.empty()) {
                                // 固定大小数组
                                dimensionSize = std::stoi(sizeStr);
                            } else {
                                // 动态大小数组：尝试从变量获取值
                                if (scope->hasVariable(sizeStr)) {
                                    std::string varValue = scope->getVariable(sizeStr, node->line);
                                    try {
                                        dimensionSize = std::stoi(varValue);
                                    } catch (const std::exception& e) {
                                        throw std::runtime_error("数组大小变量 '" + sizeStr + "' 的值 '" + varValue + "' 不是有效的整数 在第 " + std::to_string(node->line) + " 行");
                                    }
                                } else {
                                    throw std::runtime_error("数组大小必须是整数常量或已定义的变量: " + sizeStr + " 在第 " + std::to_string(node->line) + " 行");
                                }
                            }
                            
                            if (dimensionSize <= 0) {
                                throw std::runtime_error("数组维度大小必须为正整数: " + sizeStr);
                            }
                            if (dimensionSize > 1000) {
                                throw std::runtime_error("数组维度大小过大: " + sizeStr + "，最大允许1000");
                            }
                            dimensions.push_back(dimensionSize);
                            dimensionCount++;
                        } catch (const std::invalid_argument& e) {
                            throw std::runtime_error("无效的数组维度大小: " + sizeStr + " 在第 " + std::to_string(node->line) + " 行");
                        } catch (const std::out_of_range& e) {
                            throw std::runtime_error("数组维度大小超出范围: " + sizeStr + " 在第 " + std::to_string(node->line) + " 行");
                        }
                        
                        pos = endPos + 1;
                    } else {
                        pos++;
                    }
                }
                
                if (dimensions.empty()) {
                    throw std::runtime_error("数组定义语法错误: " + varDef->name + " 在第 " + std::to_string(node->line) + " 行");
                }
                
                // 定义数组本身（使用数组名，而不是带索引的名称）
                // 对于结构体数组，构建包含所有元素的数组值
                std::string value = varDef->type + ":";
                
                // 检查是否是大括号初始化列表
                bool useBraceInit = varDef->initializer && varDef->initializer->type == NodeType::BRACE_INIT_LIST;
                std::vector<std::string> braceInitValues;
                if (useBraceInit) {
                    BraceInitListNode* braceInit = static_cast<BraceInitListNode*>(varDef->initializer.get());
                    // 递归解析嵌套的初始化列表
                    std::function<void(BraceInitListNode*, std::vector<std::string>&)> parseBraceInit = 
                        [&](BraceInitListNode* node, std::vector<std::string>& values) {
                            for (const auto& element : node->elements) {
                                if (element->type == NodeType::BRACE_INIT_LIST) {
                                    parseBraceInit(static_cast<BraceInitListNode*>(element.get()), values);
                                } else {
                                    values.push_back(evaluate(element.get(), scope));
                                }
                            }
                        };
                    parseBraceInit(braceInit, braceInitValues);
                }
                
                // 初始化多维数组中的所有元素
                std::vector<int> indices(dimensions.size(), 0);
                bool initialized = true;
                int elementIndex = 0;
                
                while (initialized) {
                    // 构造元素名称
                    std::string elementName = arrayName;
                    for (int idx : indices) {
                        elementName += "[" + std::to_string(idx) + "]";
                    }
                    
                    // 根据类型初始化元素值
                    std::string elementValue;
                    if (varDef->type == "整型" || varDef->type == "小数" || varDef->type == "布尔型") {
                        // 基本类型：初始化为0
                        elementValue = "0";
                    } else if (varDef->type == "字符串") {
                        // 字符串类型：初始化为空字符串
                        elementValue = "";
                    } else if (varDef->type == "字符型") {
                        // 字符类型：初始化为空字符
                        elementValue = "'";
                    } else {
                        // 结构体类型：创建结构体实例
                        elementValue = scope->createStructInstance(varDef->type, varDef->line);
                        // 保留类型前缀，因为结构体成员访问需要类型信息
                    }
                    
                    // 如果是大括号初始化列表，使用列表中的值
                    if (useBraceInit && elementIndex < static_cast<int>(braceInitValues.size())) {
                        elementValue = braceInitValues[elementIndex];
                    }
                    
                    // 将元素值添加到数组值中
                    value += elementValue + ";";
                    
                    elementIndex++;
                    
                    // 更新索引
                    int dim = dimensions.size() - 1;
                    while (dim >= 0) {
                        if (dim >= static_cast<int>(indices.size())) {
                            initialized = false;
                            break;
                        }
                        indices[dim]++;
                        if (dim < static_cast<int>(dimensions.size()) && indices[dim] < dimensions[dim]) {
                            break;
                        } else {
                            indices[dim] = 0;
                            dim--;
                        }
                    }
                    
                    if (dim < 0) {
                        initialized = false;
                    }
                }
                
                // 定义数组变量（不初始化数组元素，稍后手动初始化）
                scope->defineVariable(arrayName, varDef->type, value, varDef->line, false);
                
                // 手动初始化数组元素
                indices.assign(dimensions.size(), 0);
                initialized = true;
                elementIndex = 0;
                while (initialized) {
                    // 构造元素名称
                    std::string elementName = arrayName;
                    for (int idx : indices) {
                        elementName += "[" + std::to_string(idx) + "]";
                    }
                    
                    // 根据类型初始化元素值
                    std::string elementValue;
                    if (varDef->type == "整型" || varDef->type == "小数" || varDef->type == "布尔型") {
                        // 基本类型：初始化为0
                        elementValue = "0";
                    } else if (varDef->type == "字符串") {
                        // 字符串类型：初始化为空字符串
                        elementValue = "";
                    } else if (varDef->type == "字符型") {
                        // 字符类型：初始化为空字符
                        elementValue = "'";
                    } else {
                        // 结构体类型：创建结构体实例
                        elementValue = scope->createStructInstance(varDef->type, varDef->line);
                        // 保留类型前缀，因为结构体成员访问需要类型信息
                    }
                    
                    // 如果是大括号初始化列表，使用列表中的值
                    if (useBraceInit && elementIndex < static_cast<int>(braceInitValues.size())) {
                        elementValue = braceInitValues[elementIndex];
                    }
                    
                    // 手动定义数组元素
                    scope->defineVariable(elementName, varDef->type, elementValue, varDef->line, false);
                    
                    elementIndex++;
                    
                    // 更新索引
                    int dim = dimensions.size() - 1;
                    while (dim >= 0) {
                        if (dim >= static_cast<int>(indices.size())) {
                            initialized = false;
                            break;
                        }
                        indices[dim]++;
                        if (dim < static_cast<int>(dimensions.size()) && indices[dim] < dimensions[dim]) {
                            break;
                        } else {
                            indices[dim] = 0;
                            dim--;
                        }
                    }
                    
                    if (dim < 0) {
                        initialized = false;
                    }
                }
            } else {
                // 普通变量定义
                std::string value = "";
                if (varDef->initializer) {
                    value = evaluate(varDef->initializer.get(), scope);
                } else {
                    // 如果没有初始化，根据类型设置默认值
                    if (varDef->type == "整型") {
                        value = "0";
                    } else if (varDef->type == "小数") {
                        value = "0.0";
                    } else if (varDef->type == "布尔型") {
                        value = "假";
                    } else if (varDef->type == "字符串") {
                        value = "";
                    } else if (varDef->type == "字符型") {
                        value = "'";
                    } else {
                        // 结构体类型：创建结构体实例
                        value = scope->createStructInstance(varDef->type, varDef->line);
                    }
                }
                
                scope->defineVariable(varDef->name, varDef->type, value, varDef->line, true);
            }
            break;
        }
        case NodeType::IF_STATEMENT: {
            IfStatementNode* ifStmt = static_cast<IfStatementNode*>(node);
            std::string conditionValue = evaluate(ifStmt->condition.get(), scope);
            if (conditionValue == "真") {
                executeStatement(ifStmt->thenBranch.get(), scope);
            } else if (ifStmt->elseBranch) {
                executeStatement(ifStmt->elseBranch.get(), scope);
            }
            break;
        }
        case NodeType::ELSE_IF_STATEMENT: {
            ElseIfStatementNode* elseIfStmt = static_cast<ElseIfStatementNode*>(node);
            std::string conditionValue = evaluate(elseIfStmt->condition.get(), scope);
            if (conditionValue == "真") {
                executeStatement(elseIfStmt->thenBranch.get(), scope);
            } else if (elseIfStmt->elseBranch) {
                executeStatement(elseIfStmt->elseBranch.get(), scope);
            }
            break;
        }
        case NodeType::FUNCTION_DEF: {
            FunctionDefNode* funcDef = static_cast<FunctionDefNode*>(node);
            scope->defineFunction(funcDef);
            break;
        }
        case NodeType::RETURN_STATEMENT: {
            ReturnStatementNode* returnStmt = static_cast<ReturnStatementNode*>(node);
            std::string returnValue = "";
            
            // 如果有返回值表达式
            if (returnStmt->expression) {
                returnValue = evaluate(returnStmt->expression.get(), scope);
                
                // 检查返回类型
                if (!expectedReturnType.empty()) {
                    std::string inferredType = inferType(returnValue);
                    if (expectedReturnType != "空类型" && inferredType != expectedReturnType) {
                        throw std::runtime_error("返回类型不匹配: 期望 " + expectedReturnType + "，但实际返回 " + inferredType + " 在第 " + std::to_string(node->line) + " 行");
                    }
                }
            } else {
                // 没有返回值，检查函数是否应该是空类型
                if (expectedReturnType != "空类型") {
                    throw std::runtime_error("返回类型不匹配: 期望 " + expectedReturnType + "，但没有返回值 在第 " + std::to_string(node->line) + " 行");
                }
            }
            
            // 设置返回值到作用域中
            scope->setVariable("__return_value", returnValue, node->line);
            
            // 抛出特殊异常来指示返回
            throw std::runtime_error("__return_from_function__");
        }
        case NodeType::STATEMENT_LIST: {
            StatementListNode* list = static_cast<StatementListNode*>(node);
            // 检查是否是变量定义列表（由 parseVariableDef 函数创建的）
            bool isVarDefList = true;
            for (const auto& stmt : list->statements) {
                if (stmt->type != NodeType::VARIABLE_DEF) {
                    isVarDefList = false;
                    break;
                }
            }
            // 如果是变量定义列表，就在当前作用域中执行
            if (isVarDefList) {
                for (const auto& stmt : list->statements) {
                    executeStatement(stmt.get(), scope);
                }
            } else {
                // 否则，创建新的块作用域
                SymbolTable* blockScope = scope->enterScope();
                for (const auto& stmt : list->statements) {
                    executeStatement(stmt.get(), blockScope);
                }
                delete blockScope;
            }
            break;
        }
        case NodeType::COUT_STATEMENT: {
            CoutStatementNode* coutStmt = static_cast<CoutStatementNode*>(node);
            for (const auto& expr : coutStmt->expressions) {
                std::string value = evaluate(expr.get(), scope);
                std::cout << value;
            }
            std::cout.flush();
            break;
        }
        case NodeType::COUT_NEWLINE_STATEMENT: {
            std::cout << std::endl;
            std::cout.flush();
            break;
        }
        case NodeType::CIN_STATEMENT: {
            CinStatementNode* cinStmt = static_cast<CinStatementNode*>(node);
            
            for (const auto& expr : cinStmt->expressions) {
                if (expr->type == NodeType::IDENTIFIER) {
                    IdentifierNode* ident = static_cast<IdentifierNode*>(expr.get());
                    std::string name = ident->name;
                    
                    if (!scope->hasVariable(name)) {
                        throw std::runtime_error("变量未定义: " + name + " 在第 " + std::to_string(node->line) + " 行");
                    }
                    
                    std::string input;
                    std::cin >> input;
                    scope->setVariable(name, input, node->line);
                } else if (expr->type == NodeType::ARRAY_ACCESS) {
                    ArrayAccessNode* arrayAccess = static_cast<ArrayAccessNode*>(expr.get());
                    std::string arrayName = arrayAccess->arrayName;
                    
                    // 找到数组定义所在的作用域
                    SymbolTable* arrayScope = scope;
                    while (arrayScope && !arrayScope->hasVariable(arrayName)) {
                        arrayScope = arrayScope->getParent();
                    }
                    if (!arrayScope) {
                        throw std::runtime_error("数组未定义: " + arrayName + " 在第 " + std::to_string(node->line) + " 行");
                    }
                    
                    // 计算索引值
                    std::vector<int> indices;
                    for (const auto& indexNode : arrayAccess->indices) {
                        std::string indexValue = evaluate(indexNode.get(), scope);
                        try {
                            double indexNum = std::stod(indexValue);
                            int intIndex = static_cast<int>(indexNum);
                            if (intIndex < 0) {
                                throw std::runtime_error("数组索引不能为负数: " + std::to_string(intIndex) + " 在第 " + std::to_string(node->line) + " 行");
                            }
                            indices.push_back(intIndex);
                        } catch (const std::invalid_argument& e) {
                            throw std::runtime_error("数组索引必须是数字: " + indexValue + " 在第 " + std::to_string(node->line) + " 行");
                        } catch (...) {
                            throw std::runtime_error("无效的数组索引: " + indexValue + " 在第 " + std::to_string(node->line) + " 行");
                        }
                    }
                    
                    // 构造数组元素名称
                    std::string elementName = arrayName;
                    for (int idx : indices) {
                        elementName += "[" + std::to_string(idx) + "]";
                    }
                    
                    // 读取输入
                    std::string input;
                    std::cin >> input;
                    
                    // 在数组所在作用域中定义或更新变量
                    if (arrayScope->hasVariable(elementName)) {
                        arrayScope->setVariable(elementName, input, node->line);
                    } else {
                        arrayScope->defineVariable(elementName, "整型", input, node->line, false);
                    }
                } else {
                    throw std::runtime_error("控制台输入语句必须指定变量名或数组元素 在第 " + std::to_string(node->line) + " 行");
                }
            }
            break;
        }
        case NodeType::FILE_READ_STATEMENT: {
            FileReadStatementNode* fileReadStmt = static_cast<FileReadStatementNode*>(node);
            
            // 第一个参数是文件名
            std::string filename = evaluate(fileReadStmt->filename.get(), scope);
            
            // 第二个参数必须是标识符（变量名）
            if (fileReadStmt->variableName->type != NodeType::IDENTIFIER) {
                throw std::runtime_error("文件读取的第二个参数必须是变量名 在第 " + std::to_string(node->line) + " 行");
            }
            IdentifierNode* ident = static_cast<IdentifierNode*>(fileReadStmt->variableName.get());
            std::string variableName = ident->name;
            
            // 检查变量是否存在
            if (!scope->hasVariable(variableName)) {
                throw std::runtime_error("变量未定义: " + variableName + " 在第 " + std::to_string(node->line) + " 行");
            }
            
            // 读取文件内容
            std::ifstream infile(filename);
            if (!infile.is_open()) {
                throw std::runtime_error("无法打开文件: " + filename + " 在第 " + std::to_string(node->line) + " 行");
            }
            
            std::string content((std::istreambuf_iterator<char>(infile)), std::istreambuf_iterator<char>());
            infile.close();
            
            // 将文件内容存入变量
            scope->setVariable(variableName, content, node->line);
            break;
        }
        case NodeType::FILE_WRITE_STATEMENT: {
            FileWriteStatementNode* fileWriteStmt = static_cast<FileWriteStatementNode*>(node);
            
            // 计算文件名和内容
            std::string filename = evaluate(fileWriteStmt->filename.get(), scope);
            std::string content = evaluate(fileWriteStmt->content.get(), scope);
            
            // 写入文件
            std::ofstream outfile(filename);
            if (!outfile.is_open()) {
                throw std::runtime_error("无法创建文件: " + filename + " 在第 " + std::to_string(node->line) + " 行");
            }
            
            outfile << content;
            outfile.close();
            break;
        }
        case NodeType::FILE_APPEND_STATEMENT: {
            FileAppendStatementNode* fileAppendStmt = static_cast<FileAppendStatementNode*>(node);
            
            // 计算文件名和内容
            std::string filename = evaluate(fileAppendStmt->filename.get(), scope);
            std::string content = evaluate(fileAppendStmt->content.get(), scope);
            
            // 追加到文件
            std::ofstream outfile(filename, std::ios::app);
            if (!outfile.is_open()) {
                throw std::runtime_error("无法打开文件进行追加: " + filename + " 在第 " + std::to_string(node->line) + " 行");
            }
            
            outfile << content;
            outfile.close();
            break;
        }
        case NodeType::IMPORT_STATEMENT: {
            ImportStatementNode* importStmt = static_cast<ImportStatementNode*>(node);
            
            // 调试信息
            debugOutput("执行导入语句: " + importStmt->filePath + " 在第 " + std::to_string(node->line) + " 行");
            
            // 读取并解析导入的文件
            importFile(importStmt->filePath, node->line);
            break;
        }
        case NodeType::SYSTEM_CMD_STATEMENT: {
            SystemCmdStatementNode* cmdStmt = static_cast<SystemCmdStatementNode*>(node);
            
            // 执行系统命令
            executeSystemCommand(cmdStmt->commandExpr.get(), scope, node->line);
            break;
        }
        case NodeType::ASSIGNMENT: {
            AssignmentNode* assign = static_cast<AssignmentNode*>(node);
            std::string value = evaluate(assign->expression.get(), scope);
            
            // 检查是否是结构体变量赋值
            if (scope->hasVariable(assign->name)) {
                std::string varType = scope->getVariableType(assign->name);
                
                // 如果是结构体类型，需要验证赋值类型匹配
                if (varType != "整型" && varType != "字符串" && varType != "字符型" && varType != "空类型" && 
                    varType != "小数" && varType != "布尔型") {
                    // 这是一个结构体类型
                    // 检查赋值表达式是否是标识符（变量）
                    if (assign->expression->type == NodeType::IDENTIFIER) {
                        IdentifierNode* ident = static_cast<IdentifierNode*>(assign->expression.get());
                        std::string sourceVarName = ident->name;
                        
                        // 检查源变量是否存在
                        if (!scope->hasVariable(sourceVarName)) {
                            throw std::runtime_error("变量未定义: " + sourceVarName + " 在第 " + std::to_string(node->line) + " 行");
                        }
                        
                        std::string sourceVarType = scope->getVariableType(sourceVarName);
                        
                        // 检查源变量是否也是结构体类型
                        if (sourceVarType == "整型" || sourceVarType == "字符串" || sourceVarType == "字符型" || sourceVarType == "空类型" || 
                            sourceVarType == "小数" || sourceVarType == "布尔型") {
                            throw std::runtime_error("不能将非结构体类型赋值给结构体变量 在第 " + std::to_string(node->line) + " 行");
                        }
                        
                        // 检查结构体类型是否匹配
                        if (varType != sourceVarType) {
                            throw std::runtime_error("结构体类型不匹配: 不能将 " + sourceVarType + " 赋值给 " + varType + " 在第 " + std::to_string(node->line) + " 行");
                        }
                    }
                }
            }
            
            // 检查是否是数组访问赋值
            if (assign->name.find("[") != std::string::npos && assign->name.find("]") != std::string::npos) {
                // 提取数组名和索引
                size_t bracketPos = assign->name.find("[");
                std::string arrayName = assign->name.substr(0, bracketPos);
                std::string indexStr = assign->name.substr(bracketPos + 1, assign->name.find("]") - bracketPos - 1);
                
                // 检查数组名是否定义
                if (!scope->hasVariable(arrayName)) {
                    throw std::runtime_error("数组未定义: " + arrayName + " 在第 " + std::to_string(node->line) + " 行");
                }
                
                // 尝试直接解析常量索引
                int index = -1;
                bool isConstantIndex = true;
                try {
                    // 尝试解析为整数
                    if (indexStr.empty()) {
                        throw std::runtime_error("空索引");
                    }
                    index = std::stoi(indexStr);
                    if (index < 0) {
                        throw std::runtime_error("数组索引不能为负数 在第 " + std::to_string(node->line) + " 行");
                    }
                } catch (const std::invalid_argument& e) {
                    // 如果不是常量，则视为变量索引，需要求值
                    isConstantIndex = false;
                } catch (...) {
                    // 其他异常
                    throw std::runtime_error("无效的数组索引: " + indexStr + " 在第 " + std::to_string(node->line) + " 行");
                }
                
                if (isConstantIndex) {
                    std::string elementName = arrayName + "[" + std::to_string(index) + "]";
                    scope->setVariable(elementName, value, node->line);
                } else {
                    // 动态索引，通过变量访问获取
                    std::string indexValue = scope->getVariable(indexStr, node->line);
                    try {
                        int intIndex = std::stoi(indexValue);
                        if (intIndex < 0) {
                            throw std::runtime_error("数组索引不能为负数 在第 " + std::to_string(node->line) + " 行");
                        }
                        std::string elementName = arrayName + "[" + std::to_string(intIndex) + "]";
                        scope->setVariable(elementName, value, node->line);
                    } catch (...) {
                        throw std::runtime_error("无效的数组索引: " + indexStr + " 在第 " + std::to_string(node->line) + " 行");
                    }
                }
            } else {
                scope->setVariable(assign->name, value, assign->line);
            }
            break;
        }
        case NodeType::COMPOUND_ASSIGNMENT: {
            CompoundAssignmentNode* compoundAssign = static_cast<CompoundAssignmentNode*>(node);
            std::string name = compoundAssign->name;
            std::string exprValue = evaluate(compoundAssign->expression.get(), scope);
            
            // 检查变量是否存在
            if (!scope->hasVariable(name)) {
                throw std::runtime_error("变量未定义: " + name + " 在第 " + std::to_string(node->line) + " 行");
            }
            
            // 获取变量当前值
            std::string currentValue = scope->getVariable(name, node->line);
            std::string varType = scope->getVariableType(name);
            
            // 执行复合赋值运算
            std::string resultValue;
            bool isNumeric = (varType == "整型" || varType == "小数");
            
            if (isNumeric) {
                // 数字类型处理
                double left = std::stod(currentValue);
                double right = std::stod(exprValue);
                double result;
                
                if (compoundAssign->op == "+") {
                    result = left + right;
                } else if (compoundAssign->op == "-") {
                    result = left - right;
                } else if (compoundAssign->op == "*") {
                    result = left * right;
                } else if (compoundAssign->op == "/") {
                    if (right == 0) {
                        throw std::runtime_error("除零错误 在第 " + std::to_string(node->line) + " 行");
                    }
                    result = left / right;
                } else if (compoundAssign->op == "%") {
                    result = static_cast<long long>(left) % static_cast<long long>(right);
                    resultValue = std::to_string(static_cast<long long>(result));
                    scope->setVariable(name, resultValue, node->line);
                    break;
                } else if (compoundAssign->op == "^") {
                    result = std::pow(left, right);
                    resultValue = std::to_string(result);
                    scope->setVariable(name, resultValue, node->line);
                    break;
                } else {
                    throw std::runtime_error("不支持的复合赋值运算符: " + compoundAssign->op + " 在第 " + std::to_string(node->line) + " 行");
                }
                
                // 根据类型格式化结果
                if (varType == "整型") {
                    resultValue = std::to_string(static_cast<long long>(result));
                } else {
                    resultValue = std::to_string(result);
                }
            } else {
                // 字符串类型处理（+=）
                if (compoundAssign->op == "+") {
                    resultValue = currentValue;
                    // 去掉两端的引号（如果有）
                    if (resultValue.length() >= 2 && resultValue.front() == '"' && resultValue.back() == '"') {
                        resultValue = resultValue.substr(1, resultValue.length() - 2);
                    }
                    std::string rightValue = exprValue;
                    if (rightValue.length() >= 2 && rightValue.front() == '"' && rightValue.back() == '"') {
                        rightValue = rightValue.substr(1, rightValue.length() - 2);
                    }
                    resultValue = "\"" + resultValue + rightValue + "\"";
                } else {
                    throw std::runtime_error("字符串类型不支持复合赋值运算符: " + compoundAssign->op + " 在第 " + std::to_string(node->line) + " 行");
                }
            }
            
            scope->setVariable(name, resultValue, node->line);
            break;
        }
        case NodeType::WHILE_STATEMENT: {
            WhileStatementNode* whileStmt = static_cast<WhileStatementNode*>(node);
            // 循环执行，直到条件为假
            while (true) {
                std::string conditionValue = evaluate(whileStmt->condition.get(), scope);
                if (conditionValue != "真") {
                    break;
                }
                try {
                    executeStatement(whileStmt->body.get(), scope);
                } catch (const ContinueException&) {
                    continue;
                } catch (const BreakException&) {
                    break;
                }
            }
            break;
        }
        case NodeType::FOR_STATEMENT: {
            ForStatementNode* forStmt = static_cast<ForStatementNode*>(node);
            
            // 执行初始化
            if (forStmt->initialization) {
                executeStatement(forStmt->initialization.get(), scope);
            }
            // 循环执行，直到条件为假
            while (true) {
                std::string conditionValue = evaluate(forStmt->condition.get(), scope);
                if (conditionValue != "真") {
                    break;
                }
                try {
                    executeStatement(forStmt->body.get(), scope);
                } catch (const ContinueException&) {
                    // 执行更新
                    if (forStmt->update) {
                        executeStatement(forStmt->update.get(), scope);
                    }
                    continue;
                } catch (const BreakException&) {
                    break;
                }
                // 执行更新
                if (forStmt->update) {
                    executeStatement(forStmt->update.get(), scope);
                }
            }
            
            break;
        }
        case NodeType::BREAK_STATEMENT: {
            throw BreakException();
        }
        case NodeType::CONTINUE_STATEMENT: {
            throw ContinueException();
        }
        
        default:
            // 其他类型的语句，如表达式语句
            evaluate(node, scope);
            break;
    }
}

// 计算表达式
std::string Interpreter::evaluate(ASTNode* node, SymbolTable* scope) {
    switch (node->type) {
        case NodeType::LITERAL: {
            LiteralNode* literal = static_cast<LiteralNode*>(node);
            return literal->value;
        }
        case NodeType::IDENTIFIER: {
            IdentifierNode* ident = static_cast<IdentifierNode*>(node);
            std::string name = ident->name;
            // 检查变量是否已经在符号表中定义
            if (!scope->hasVariable(name)) {
                throw std::runtime_error("变量未定义: " + name + " 在第 " + std::to_string(node->line) + " 行");
            }
            return scope->getVariable(name, node->line);
        }
        case NodeType::FUNCTION_CALL: {
            FunctionCallNode* funcCall = static_cast<FunctionCallNode*>(node);
            std::string functionName = funcCall->functionName;
            
            // 检查内置函数
            if (functionName == "长度") {
                if (funcCall->arguments.size() != 1) {
                    throw std::runtime_error("长度函数需要一个参数 在第 " + std::to_string(node->line) + " 行");
                }
                std::string strValue = evaluate(funcCall->arguments[0].get(), scope);
                return std::to_string(strValue.length());
            }
            
            if (functionName == "子串") {
                if (funcCall->arguments.size() != 3) {
                    throw std::runtime_error("子串函数需要3个参数: 字符串, 开始位置, 长度 在第 " + std::to_string(node->line) + " 行");
                }
                std::string strValue = evaluate(funcCall->arguments[0].get(), scope);
                int start = std::stoi(evaluate(funcCall->arguments[1].get(), scope));
                int length = std::stoi(evaluate(funcCall->arguments[2].get(), scope));
                
                if (start < 0 || start >= static_cast<int>(strValue.length()) || length <= 0) {
                    return "";
                }
                return strValue.substr(start, length);
            }
            
            if (functionName == "查找") {
                if (funcCall->arguments.size() != 2) {
                    throw std::runtime_error("查找函数需要2个参数: 字符串, 子字符串 在第 " + std::to_string(node->line) + " 行");
                }
                std::string strValue = evaluate(funcCall->arguments[0].get(), scope);
                std::string substr = evaluate(funcCall->arguments[1].get(), scope);
                
                size_t pos = strValue.find(substr);
                if (pos == std::string::npos) {
                    return "-1";
                }
                return std::to_string(pos);
            }
            
            if (functionName == "转大写") {
                if (funcCall->arguments.size() != 1) {
                    throw std::runtime_error("转大写函数需要一个参数 在第 " + std::to_string(node->line) + " 行");
                }
                std::string strValue = evaluate(funcCall->arguments[0].get(), scope);
                std::string result = strValue;
                for (char& c : result) {
                    c = std::toupper(c);
                }
                return result;
            }
            
            if (functionName == "字符转整型") {
                if (funcCall->arguments.size() != 1) {
                    throw std::runtime_error("字符转整型函数需要一个参数 在第 " + std::to_string(node->line) + " 行");
                }
                std::string charStr = evaluate(funcCall->arguments[0].get(), scope);
                if (charStr.empty()) {
                    return "0";
                }
                // 返回字符的ASCII码值
                return std::to_string(static_cast<int>(charStr[0]));
            }
            
            if (functionName == "转小写") {
                if (funcCall->arguments.size() != 1) {
                    throw std::runtime_error("转小写函数需要一个参数 在第 " + std::to_string(node->line) + " 行");
                }
                std::string strValue = evaluate(funcCall->arguments[0].get(), scope);
                std::string result = strValue;
                for (char& c : result) {
                    c = std::tolower(c);
                }
                return result;
            }
            
            if (functionName == "去空白") {
                if (funcCall->arguments.size() != 1) {
                    throw std::runtime_error("去空白函数需要一个参数 在第 " + std::to_string(node->line) + " 行");
                }
                std::string strValue = evaluate(funcCall->arguments[0].get(), scope);
                
                // 移除首尾空白
                size_t start = 0;
                while (start < strValue.length() && std::isspace(strValue[start])) {
                    start++;
                }
                size_t end = strValue.length();
                while (end > start && std::isspace(strValue[end - 1])) {
                    end--;
                }
                return strValue.substr(start, end - start);
            }
            
            if (functionName == "重复") {
                if (funcCall->arguments.size() != 2) {
                    throw std::runtime_error("重复函数需要2个参数: 字符串, 次数 在第 " + std::to_string(node->line) + " 行");
                }
                std::string strValue = evaluate(funcCall->arguments[0].get(), scope);
                int times = std::stoi(evaluate(funcCall->arguments[1].get(), scope));
                
                std::string result;
                for (int i = 0; i < times; i++) {
                    result += strValue;
                }
                return result;
            }
            
            if (functionName == "整数转字符串") {
                if (funcCall->arguments.size() != 1) {
                    throw std::runtime_error("整数转字符串函数需要一个参数 在第 " + std::to_string(node->line) + " 行");
                }
                std::string intValue = evaluate(funcCall->arguments[0].get(), scope);
                return intValue;
            }
            
            if (functionName == "字符串拼接") {
                if (funcCall->arguments.size() != 2) {
                    throw std::runtime_error("字符串拼接函数需要2个参数 在第 " + std::to_string(node->line) + " 行");
                }
                std::string str1 = evaluate(funcCall->arguments[0].get(), scope);
                std::string str2 = evaluate(funcCall->arguments[1].get(), scope);
                return str1 + str2;
            }
            
            // 检查函数是否已定义
            if (!scope->hasFunction(functionName)) {
                throw std::runtime_error("函数未定义: " + functionName + " 在第 " + std::to_string(node->line) + " 行");
            }
            
            // 评估参数
            std::vector<std::string> argumentValues;
            std::vector<std::string> argumentTypes;
            for (const auto& argNode : funcCall->arguments) {
                std::string argValue = evaluate(argNode.get(), scope);
                argumentValues.push_back(argValue);
                
                // 改进的类型推断：优先使用变量的声明类型
                if (argNode->type == NodeType::IDENTIFIER) {
                    // 如果是变量引用，使用变量的声明类型
                    IdentifierNode* ident = static_cast<IdentifierNode*>(argNode.get());
                    if (scope->hasVariable(ident->name)) {
                        std::string varType = scope->getVariableType(ident->name);
                        argumentTypes.push_back(varType);
                    } else {
                        // 如果变量不存在，使用值的类型推断
                        argumentTypes.push_back(inferType(argValue));
                    }
                } else {
                    // 对于字面量等其他表达式，使用值的类型推断
                    argumentTypes.push_back(inferType(argValue));
                }
            }
            
            // 获取匹配的函数定义
            
            // 调试信息：显示函数名和参数类型
            // std::cout << "[调试] 查找函数: " << functionName << "(";
            // for (size_t i = 0; i < argumentTypes.size(); ++i) {
            //     if (i > 0) std::cout << ", ";
            //     std::cout << argumentTypes[i];
            // }
            // std::cout << ")" << std::endl;
            
            FunctionDefNode* funcDef = scope->getFunction(functionName, argumentTypes, node->line);
            
            // 创建函数作用域
            SymbolTable* funcScope = scope->enterScope();
            
            // 绑定参数
            if (funcDef->parameters.size() != argumentValues.size()) {
                throw std::runtime_error("函数 " + functionName + " 参数数量不匹配 在第 " + std::to_string(node->line) + " 行");
            }
            
            for (size_t i = 0; i < funcDef->parameters.size(); ++i) {
                std::string paramName = funcDef->parameters[i].second;
                std::string paramType = funcDef->parameters[i].first;
                
                // 检查是否是数组参数（包含方括号）
                bool isArrayParam = (paramName.find("[") != std::string::npos && paramName.find("]") != std::string::npos);
                
                // 如果是数组参数，不重新初始化数组元素
                funcScope->defineVariable(paramName, paramType, argumentValues[i], node->line, !isArrayParam);
            }
            
            // 创建一个返回值存储变量
            funcScope->defineVariable("__return_value", funcDef->returnType, "", node->line, true);
            
            // 执行函数体
            try {
                executeStatement(funcDef->body.get(), funcScope, funcDef->returnType);
            } catch (const std::runtime_error& e) {
                // 如果是返回语句异常，直接返回
                if (strcmp(e.what(), "__return_from_function__") == 0) {
                    // 获取返回值
                    std::string returnValue = funcScope->getVariable("__return_value", node->line);
                    
                    // 检查返回类型是否匹配
                    if (funcDef->returnType != "空类型") {
                        std::string inferredType = inferType(returnValue);
                        if (inferredType != funcDef->returnType) {
                            delete funcScope; // 清理作用域
                            throw std::runtime_error("函数 " + functionName + " 返回类型不匹配: 期望 " + funcDef->returnType + "，但实际返回 " + inferredType + " 在第 " + std::to_string(funcDef->line) + " 行");
                        }
                    }
                    
                    delete funcScope; // 清理作用域
                    return returnValue;
                } else {
                    // 其他异常重新抛出
                    delete funcScope; // 清理作用域
                    throw;
                }
            }
            
            // 如果函数没有显式返回且没有抛出返回异常，检查函数是否应该有返回值
            if (funcDef->returnType != "空类型") {
                delete funcScope; // 清理作用域
                throw std::runtime_error("函数 " + functionName + " 没有返回预期的类型 " + funcDef->returnType + " 在第 " + std::to_string(node->line) + " 行");
            }
            
            // 如果函数是空类型，正常返回而不抛出异常
            delete funcScope; // 清理作用域
            return "";
        }
        case NodeType::ARRAY_ACCESS: {
            ArrayAccessNode* arrayAccess = static_cast<ArrayAccessNode*>(node);
            
            // 计算多维索引
            std::vector<int> indices;
            for (const auto& indexNode : arrayAccess->indices) {
                std::string indexValue = evaluate(indexNode.get(), scope);
                // 将索引值转换为整数形式
                try {
                    double indexNum = std::stod(indexValue);
                    int intIndex = static_cast<int>(indexNum);
                    if (intIndex < 0) {
                        throw std::runtime_error("数组索引不能为负数: " + std::to_string(intIndex) + " 在第 " + std::to_string(node->line) + " 行");
                    }
                    indices.push_back(intIndex);
                } catch (const std::invalid_argument& e) {
                    // 如果转换失败，说明索引值不是数字
                    throw std::runtime_error("数组索引必须是数字: " + indexValue + " 在第 " + std::to_string(node->line) + " 行");
                } catch (...) {
                    // 其他异常
                    throw std::runtime_error("无效的数组索引: " + indexValue + " 在第 " + std::to_string(node->line) + " 行");
                }
            }
            
            // 检查维度数量（必须与数组定义匹配）
            if (indices.size() != arrayAccess->indices.size()) {
                throw std::runtime_error("数组维度不匹配 在第 " + std::to_string(node->line) + " 行");
            }
            
            // 构造完整的数组元素变量名：arrayName[index1][index2]...
            std::string elementName = arrayAccess->arrayName;
            for (int idx : indices) {
                elementName += "[" + std::to_string(idx) + "]";
            }
            
            // 检查变量类型，区分字符串访问和数组访问
            std::string varType = scope->getVariableType(arrayAccess->arrayName);
            
            if (varType == "字符串") {
                // 这是字符串访问，不是数组访问
                // 获取字符串变量的值
                std::string fullString = scope->getVariable(arrayAccess->arrayName, node->line);
                
                // 检查索引数量（字符串访问应该只有一个索引）
                if (indices.size() != 1) {
                    throw std::runtime_error("字符串访问只能有一个索引 在第 " + std::to_string(node->line) + " 行");
                }
                
                int intIndex = indices[0];
                if (intIndex < 0) {
                    throw std::runtime_error("字符串索引不能为负数: " + std::to_string(intIndex) + " 在第 " + std::to_string(node->line) + " 行");
                }
                
                // 检查索引是否超出字符串长度
                if (intIndex >= static_cast<int>(fullString.length())) {
                    throw std::runtime_error("字符串索引超出范围: " + std::to_string(intIndex) + " 在第 " + std::to_string(node->line) + " 行");
                }
                
                // 返回字符（作为字符串返回）
                std::string charStr(1, fullString[intIndex]);
                return charStr;
            } else {
                // 这是真正的数组访问
                // 检查数组元素变量是否存在
                if (!scope->hasVariable(elementName)) {
                    throw std::runtime_error("数组元素未定义: " + elementName + " 在第 " + std::to_string(node->line) + " 行");
                }
                
                // 直接返回数组元素的值
                return scope->getVariable(elementName, node->line);
            }
        }
        case NodeType::STRING_ACCESS: {
            StringAccessNode* stringAccess = static_cast<StringAccessNode*>(node);
            
            // 获取字符串变量的值
            if (!scope->hasVariable(stringAccess->stringName)) {
                throw std::runtime_error("字符串变量未定义: " + stringAccess->stringName + " 在第 " + std::to_string(node->line) + " 行");
            }
            
            std::string fullString = scope->getVariable(stringAccess->stringName, node->line);
            
            // 获取索引表达式的值
            std::string indexValue = evaluate(stringAccess->index.get(), scope);
            
            // 计算索引
            try {
                double indexNum = std::stod(indexValue);
                int intIndex = static_cast<int>(indexNum);
                if (intIndex < 0) {
                    throw std::runtime_error("字符串索引不能为负数: " + std::to_string(intIndex) + " 在第 " + std::to_string(node->line) + " 行");
                }
                
                // 检查索引是否超出字符串长度
                if (intIndex >= static_cast<int>(fullString.length())) {
                    throw std::runtime_error("字符串索引超出范围: " + std::to_string(intIndex) + " 在第 " + std::to_string(node->line) + " 行");
                }
                
                // 返回字符（作为字符串返回）
                std::string charStr(1, fullString[intIndex]);
                return charStr;
            } catch (const std::invalid_argument& e) {
                // 如果转换失败，说明索引值不是数字
                throw std::runtime_error("字符串索引必须是数字: " + indexValue + " 在第 " + std::to_string(node->line) + " 行");
            } catch (...) {
                // 其他异常
                throw std::runtime_error("无效的字符串索引: " + indexValue + " 在第 " + std::to_string(node->line) + " 行");
            }
        }
        case NodeType::STRUCT_MEMBER_ACCESS: {
            StructMemberAccessNode* structAccess = static_cast<StructMemberAccessNode*>(node);
            
            std::string structVarValue;
            std::string structVarName;
            
            // 获取结构体变量名（支持标识符和数组访问）
            if (structAccess->structExpr->type == NodeType::IDENTIFIER) {
                // 简单标识符：point.x
                IdentifierNode* structVar = static_cast<IdentifierNode*>(structAccess->structExpr.get());
                structVarName = structVar->name;
                
                // 检查结构体变量是否存在
                if (!scope->hasVariable(structVarName)) {
                    throw std::runtime_error("结构体变量未定义: " + structVarName + " 在第 " + std::to_string(node->line) + " 行");
                }
                
                // 获取结构体变量值
                structVarValue = scope->getVariable(structVarName, node->line);
            } else if (structAccess->structExpr->type == NodeType::ARRAY_ACCESS) {
                // 数组访问：points[0].x
                ArrayAccessNode* arrayAccess = static_cast<ArrayAccessNode*>(structAccess->structExpr.get());
                structVarName = arrayAccess->arrayName;
                
                // 检查数组变量是否存在
                if (!scope->hasVariable(structVarName)) {
                    throw std::runtime_error("数组变量未定义: " + structVarName + " 在第 " + std::to_string(node->line) + " 行");
                }
                
                // 获取数组变量值
                std::string arrayValue = scope->getVariable(structVarName, node->line);
                

                
                // 解析数组索引
                if (arrayAccess->indices.empty()) {
                    throw std::runtime_error("数组访问必须指定索引 在第 " + std::to_string(node->line) + " 行");
                }
                
                // 计算索引值
                std::string indexStr = evaluate(arrayAccess->indices[0].get(), scope);
                int index = std::stoi(indexStr);
                
                // 从数组值中提取指定索引的元素
                // 数组格式：类型:值1;值2;值3;...
                size_t colonPos = arrayValue.find(':');
                if (colonPos == std::string::npos) {
                    throw std::runtime_error("无效的数组变量格式 在第 " + std::to_string(node->line) + " 行");
                }
                
                std::string arrayType = arrayValue.substr(0, colonPos);
                std::string elements = arrayValue.substr(colonPos + 1);
                
                // 解析元素列表
                std::vector<std::string> elementList;
                size_t startPos = 0;
                size_t semicolonPos = elements.find(';');
                while (semicolonPos != std::string::npos) {
                    elementList.push_back(elements.substr(startPos, semicolonPos - startPos));
                    startPos = semicolonPos + 1;
                    semicolonPos = elements.find(';', startPos);
                }
                elementList.push_back(elements.substr(startPos));
                
                // 检查索引是否有效
                if (index < 0 || index >= static_cast<int>(elementList.size())) {
                    throw std::runtime_error("数组索引越界: " + std::to_string(index) + " 在第 " + std::to_string(node->line) + " 行");
                }
                
                // 为数组元素添加类型前缀，使其符合结构体变量格式
                structVarValue = arrayType + ":" + elementList[index];

            } else {
                throw std::runtime_error("不支持的结构体成员访问表达式 在第 " + std::to_string(node->line) + " 行");
            }
            
            // 验证成员访问格式
            
            // 检查结构体变量值是否为空或无效
            if (structVarValue.empty()) {
                throw std::runtime_error("结构体变量值为空 在第 " + std::to_string(node->line) + " 行");
            }
            
            size_t colonPos = structVarValue.find(':');
            if (colonPos == std::string::npos) {
                // 如果找不到冒号，尝试检查是否是未初始化的结构体变量
                if (structVarValue.find_first_not_of(" \t\n\r") == std::string::npos) {
                    // 空值，可能是未初始化的结构体变量
                    throw std::runtime_error("结构体变量未初始化 在第 " + std::to_string(node->line) + " 行");
                } else {
                    // 尝试修复格式：添加默认类型前缀
                    structVarValue = "Point:" + structVarValue;
                    colonPos = structVarValue.find(':');
                }
            }
            
            std::string structType = structVarValue.substr(0, colonPos);
            
            // 检查结构体类型是否存在
            if (!scope->hasStruct(structType)) {
                throw std::runtime_error("结构体类型未定义: " + structType + " 在第 " + std::to_string(node->line) + " 行");
            }
            
            // 检查成员是否存在
            if (!scope->hasStructMember(structType, structAccess->memberName, node->line)) {
                throw std::runtime_error("结构体 " + structType + " 没有成员 " + structAccess->memberName + " 在第 " + std::to_string(node->line) + " 行");
            }
            
            // 解析结构体成员值
            std::string members = structVarValue.substr(colonPos + 1);
            
            // 获取结构体信息以确定成员顺序
            StructInfo structInfo = scope->getStruct(structType, node->line);
            
            // 查找成员在结构体中的索引位置
            int memberIndex = -1;
            for (size_t i = 0; i < structInfo.members.size(); ++i) {
                if (structInfo.members[i].second == structAccess->memberName) {
                    memberIndex = i;
                    break;
                }
            }
            
            if (memberIndex == -1) {
                throw std::runtime_error("结构体 " + structType + " 没有成员 " + structAccess->memberName + " 在第 " + std::to_string(node->line) + " 行");
            }
            
            // 根据成员索引解析对应的值
            size_t startPos = 0;
            size_t semicolonPos = members.find(';');
            int currentIndex = 0;
            
            while (semicolonPos != std::string::npos && currentIndex < memberIndex) {
                startPos = semicolonPos + 1;
                semicolonPos = members.find(';', startPos);
                currentIndex++;
            }
            
            // 提取成员值
            std::string memberValue;
            if (semicolonPos != std::string::npos) {
                memberValue = members.substr(startPos, semicolonPos - startPos);
            } else {
                memberValue = members.substr(startPos);
            }
            
            return memberValue;
        }
        case NodeType::STRUCT_MEMBER_ASSIGNMENT: {
            StructMemberAssignmentNode* assign = static_cast<StructMemberAssignmentNode*>(node);
            
            // 获取结构体变量值（支持复杂表达式）
            std::string structVarValue;
            std::string structVarName;
            
            if (assign->structExpr->type == NodeType::IDENTIFIER) {
                // 简单标识符：point1.x = 10;
                IdentifierNode* structVar = static_cast<IdentifierNode*>(assign->structExpr.get());
                structVarName = structVar->name;
                
                // 检查结构体变量是否存在
                if (!scope->hasVariable(structVarName)) {
                    throw std::runtime_error("结构体变量未定义: " + structVarName + " 在第 " + std::to_string(node->line) + " 行");
                }
                
                // 获取结构体变量值
                structVarValue = scope->getVariable(structVarName, node->line);
            } else if (assign->structExpr->type == NodeType::ARRAY_ACCESS) {
                // 数组访问：points[0].x = 10;
                ArrayAccessNode* arrayAccess = static_cast<ArrayAccessNode*>(assign->structExpr.get());
                structVarName = arrayAccess->arrayName;
                
                // 检查数组变量是否存在
                if (!scope->hasVariable(structVarName)) {
                    throw std::runtime_error("数组变量未定义: " + structVarName + " 在第 " + std::to_string(node->line) + " 行");
                }
                
                // 获取数组变量值
                std::string arrayValue = scope->getVariable(structVarName, node->line);
                
                // 解析数组索引
                if (arrayAccess->indices.empty()) {
                    throw std::runtime_error("数组访问必须指定索引 在第 " + std::to_string(node->line) + " 行");
                }
                
                // 计算索引值
                std::string indexStr = evaluate(arrayAccess->indices[0].get(), scope);
                int index = std::stoi(indexStr);
                
                // 从数组值中提取指定索引的元素
                // 数组格式：类型:值1;值2;值3;...
                size_t colonPos = arrayValue.find(':');
                if (colonPos == std::string::npos) {
                    throw std::runtime_error("无效的数组变量格式 在第 " + std::to_string(node->line) + " 行");
                }
                
                std::string arrayType = arrayValue.substr(0, colonPos);
                std::string elements = arrayValue.substr(colonPos + 1);
                
                // 解析元素列表
                std::vector<std::string> elementList;
                size_t startPos = 0;
                size_t semicolonPos = elements.find(';');
                while (semicolonPos != std::string::npos) {
                    elementList.push_back(elements.substr(startPos, semicolonPos - startPos));
                    startPos = semicolonPos + 1;
                    semicolonPos = elements.find(';', startPos);
                }
                elementList.push_back(elements.substr(startPos));
                
                // 检查索引是否有效
                if (index < 0 || index >= static_cast<int>(elementList.size())) {
                    throw std::runtime_error("数组索引越界: " + std::to_string(index) + " 在第 " + std::to_string(node->line) + " 行");
                }
                
                structVarValue = elementList[index];
            } else {
                throw std::runtime_error("不支持的结构体成员赋值表达式 在第 " + std::to_string(node->line) + " 行");
            }
            
            // 验证成员访问格式
            size_t colonPos = structVarValue.find(':');
            if (colonPos == std::string::npos) {
                throw std::runtime_error("无效的结构体变量格式 在第 " + std::to_string(node->line) + " 行");
            }
            
            std::string structType = structVarValue.substr(0, colonPos);
            
            // 检查结构体类型是否存在
            if (!scope->hasStruct(structType)) {
                throw std::runtime_error("结构体类型未定义: " + structType + " 在第 " + std::to_string(node->line) + " 行");
            }
            
            // 检查成员是否存在
            if (!scope->hasStructMember(structType, assign->memberName, node->line)) {
                throw std::runtime_error("结构体 " + structType + " 没有成员 " + assign->memberName + " 在第 " + std::to_string(node->line) + " 行");
            }
            
            // 获取新值
            std::string newValue = evaluate(assign->expression.get(), scope);
            
            // 更新结构体成员值
            std::string members = structVarValue.substr(colonPos + 1);
            std::string newStructValue = structType + ":";
            
            // 获取结构体信息以确定成员顺序
            StructInfo structInfo = scope->getStruct(structType, node->line);
            
            // 查找成员在结构体中的索引位置
            int memberIndex = -1;
            for (size_t i = 0; i < structInfo.members.size(); ++i) {
                if (structInfo.members[i].second == assign->memberName) {
                    memberIndex = i;
                    break;
                }
            }
            
            if (memberIndex == -1) {
                throw std::runtime_error("结构体 " + structType + " 没有成员 " + assign->memberName + " 在第 " + std::to_string(node->line) + " 行");
            }
            
            // 解析并更新成员值（格式：值1;值2;值3）
            size_t startPos = 0;
            size_t semicolonPos = members.find(';');
            int currentIndex = 0;
            
            while (semicolonPos != std::string::npos) {
                std::string memberValue = members.substr(startPos, semicolonPos - startPos);
                
                if (currentIndex == memberIndex) {
                    // 更新当前成员的值
                    newStructValue += newValue;
                } else {
                    // 保留其他成员的值
                    newStructValue += memberValue;
                }
                
                newStructValue += ";";
                startPos = semicolonPos + 1;
                semicolonPos = members.find(';', startPos);
                currentIndex++;
            }
            
            // 处理最后一个成员
            if (startPos < members.length()) {
                std::string memberValue = members.substr(startPos);
                
                if (currentIndex == memberIndex) {
                    newStructValue += newValue;
                } else {
                    newStructValue += memberValue;
                }
            }
            
            // 更新结构体变量值
            if (assign->structExpr->type == NodeType::IDENTIFIER) {
                // 简单标识符：直接更新变量
                scope->setVariable(structVarName, newStructValue, node->line);
            } else if (assign->structExpr->type == NodeType::ARRAY_ACCESS) {
                // 数组访问：需要更新数组中的特定元素
                ArrayAccessNode* arrayAccess = static_cast<ArrayAccessNode*>(assign->structExpr.get());
                
                // 获取数组变量值
                std::string arrayValue = scope->getVariable(structVarName, node->line);
                
                // 解析数组索引
                std::string indexStr = evaluate(arrayAccess->indices[0].get(), scope);
                int index = std::stoi(indexStr);
                
                // 解析数组格式：类型:值1;值2;值3;...
                size_t colonPos = arrayValue.find(':');
                if (colonPos == std::string::npos) {
                    throw std::runtime_error("无效的数组变量格式 在第 " + std::to_string(node->line) + " 行");
                }
                
                std::string arrayType = arrayValue.substr(0, colonPos);
                std::string elements = arrayValue.substr(colonPos + 1);
                
                // 解析元素列表
                std::vector<std::string> elementList;
                size_t startPos = 0;
                size_t semicolonPos = elements.find(';');
                while (semicolonPos != std::string::npos) {
                    elementList.push_back(elements.substr(startPos, semicolonPos - startPos));
                    startPos = semicolonPos + 1;
                    semicolonPos = elements.find(';', startPos);
                }
                elementList.push_back(elements.substr(startPos));
                
                // 检查索引是否有效
                if (index < 0 || index >= static_cast<int>(elementList.size())) {
                    throw std::runtime_error("数组索引越界: " + std::to_string(index) + " 在第 " + std::to_string(node->line) + " 行");
                }
                
                // 更新指定索引的元素
                elementList[index] = newStructValue;
                
                // 重新构建数组值
                std::string newArrayValue = arrayType + ":";
                for (size_t i = 0; i < elementList.size(); ++i) {
                    if (i > 0) newArrayValue += ";";
                    newArrayValue += elementList[i];
                }
                
                // 更新数组变量
                scope->setVariable(structVarName, newArrayValue, node->line);
            }
            
            return newValue;
        }
        case NodeType::UNARY_EXPRESSION: {
            UnaryExpressionNode* unaryExpr = static_cast<UnaryExpressionNode*>(node);
            std::string operand = evaluate(unaryExpr->operand.get(), scope);
            
            // 处理逻辑非运算符
            if (unaryExpr->op == "!") {
                // 将操作数转换为布尔值
                bool operandIsTrue = false;
                if (operand == "true" || operand == "1" || operand == "真") {
                    operandIsTrue = true;
                } else if (operand == "false" || operand == "0" || operand == "假") {
                    operandIsTrue = false;
                } else {
                    // 尝试解析为数字
                    try {
                        double numValue = std::stod(operand);
                        operandIsTrue = (numValue != 0);
                    } catch (...) {
                        operandIsTrue = false;
                    }
                }
                return operandIsTrue ? "假" : "真";
            }
            
            // 处理一元负号运算符
            if (unaryExpr->op == "-") {
                // 尝试解析为数字
                try {
                    double numValue = std::stod(operand);
                    // 检查是否是小数
                    if (operand.find('.') != std::string::npos) {
                        return std::to_string(-numValue);
                    } else {
                        // 整数
                        return std::to_string(-static_cast<long long>(numValue));
                    }
                } catch (...) {
                    throw std::runtime_error("一元负号运算符只能应用于数字类型 在第 " + std::to_string(node->line) + " 行");
                }
            }
            
            // 处理一元正号运算符
            if (unaryExpr->op == "+") {
                return operand;
            }
            
            // 处理前缀自增运算符
            if (unaryExpr->op == "前缀++") {
                // 获取操作数（应该是标识符）
                if (unaryExpr->operand->type != NodeType::IDENTIFIER) {
                    throw std::runtime_error("自增运算符只能应用于变量 在第 " + std::to_string(node->line) + " 行");
                }
                IdentifierNode* ident = static_cast<IdentifierNode*>(unaryExpr->operand.get());
                std::string varName = ident->name;
                
                if (!scope->hasVariable(varName)) {
                    throw std::runtime_error("变量未定义: " + varName + " 在第 " + std::to_string(node->line) + " 行");
                }
                
                std::string varType = scope->getVariableType(varName);
                if (varType != "整型" && varType != "小数") {
                    throw std::runtime_error("自增运算符只能应用于数字类型 在第 " + std::to_string(node->line) + " 行");
                }
                
                // 获取当前值
                double currentValue = std::stod(operand);
                double newValue = currentValue + 1;
                
                // 设置新值
                std::string newValueStr;
                if (varType == "整型") {
                    newValueStr = std::to_string(static_cast<long long>(newValue));
                } else {
                    newValueStr = std::to_string(newValue);
                }
                scope->setVariable(varName, newValueStr, node->line);
                
                return newValueStr;
            }
            
            // 处理前缀自减运算符
            if (unaryExpr->op == "前缀--") {
                // 获取操作数（应该是标识符）
                if (unaryExpr->operand->type != NodeType::IDENTIFIER) {
                    throw std::runtime_error("自减运算符只能应用于变量 在第 " + std::to_string(node->line) + " 行");
                }
                IdentifierNode* ident = static_cast<IdentifierNode*>(unaryExpr->operand.get());
                std::string varName = ident->name;
                
                if (!scope->hasVariable(varName)) {
                    throw std::runtime_error("变量未定义: " + varName + " 在第 " + std::to_string(node->line) + " 行");
                }
                
                std::string varType = scope->getVariableType(varName);
                if (varType != "整型" && varType != "小数") {
                    throw std::runtime_error("自减运算符只能应用于数字类型 在第 " + std::to_string(node->line) + " 行");
                }
                
                // 获取当前值
                double currentValue = std::stod(operand);
                double newValue = currentValue - 1;
                
                // 设置新值
                std::string newValueStr;
                if (varType == "整型") {
                    newValueStr = std::to_string(static_cast<long long>(newValue));
                } else {
                    newValueStr = std::to_string(newValue);
                }
                scope->setVariable(varName, newValueStr, node->line);
                
                return newValueStr;
            }
            
            // 处理后置自增运算符
            if (unaryExpr->op == "后置++") {
                // 获取操作数（应该是标识符）
                if (unaryExpr->operand->type != NodeType::IDENTIFIER) {
                    throw std::runtime_error("自增运算符只能应用于变量 在第 " + std::to_string(node->line) + " 行");
                }
                IdentifierNode* ident = static_cast<IdentifierNode*>(unaryExpr->operand.get());
                std::string varName = ident->name;
                
                if (!scope->hasVariable(varName)) {
                    throw std::runtime_error("变量未定义: " + varName + " 在第 " + std::to_string(node->line) + " 行");
                }
                
                std::string varType = scope->getVariableType(varName);
                if (varType != "整型" && varType != "小数") {
                    throw std::runtime_error("自增运算符只能应用于数字类型 在第 " + std::to_string(node->line) + " 行");
                }
                
                // 获取当前值（作为返回值）
                std::string oldValue = operand;
                
                // 计算新值
                double currentValue = std::stod(operand);
                double newValue = currentValue + 1;
                
                // 设置新值
                std::string newValueStr;
                if (varType == "整型") {
                    newValueStr = std::to_string(static_cast<long long>(newValue));
                } else {
                    newValueStr = std::to_string(newValue);
                }
                scope->setVariable(varName, newValueStr, node->line);
                
                return oldValue;
            }
            
            // 处理后置自减运算符
            if (unaryExpr->op == "后置--") {
                // 获取操作数（应该是标识符）
                if (unaryExpr->operand->type != NodeType::IDENTIFIER) {
                    throw std::runtime_error("自减运算符只能应用于变量 在第 " + std::to_string(node->line) + " 行");
                }
                IdentifierNode* ident = static_cast<IdentifierNode*>(unaryExpr->operand.get());
                std::string varName = ident->name;
                
                if (!scope->hasVariable(varName)) {
                    throw std::runtime_error("变量未定义: " + varName + " 在第 " + std::to_string(node->line) + " 行");
                }
                
                std::string varType = scope->getVariableType(varName);
                if (varType != "整型" && varType != "小数") {
                    throw std::runtime_error("自减运算符只能应用于数字类型 在第 " + std::to_string(node->line) + " 行");
                }
                
                // 获取当前值（作为返回值）
                std::string oldValue = operand;
                
                // 计算新值
                double currentValue = std::stod(operand);
                double newValue = currentValue - 1;
                
                // 设置新值
                std::string newValueStr;
                if (varType == "整型") {
                    newValueStr = std::to_string(static_cast<long long>(newValue));
                } else {
                    newValueStr = std::to_string(newValue);
                }
                scope->setVariable(varName, newValueStr, node->line);
                
                return oldValue;
            }
            
            return operand;
        }
        case NodeType::BINARY_EXPRESSION: {
            BinaryExpressionNode* binExpr = static_cast<BinaryExpressionNode*>(node);
            std::string left = evaluate(binExpr->left.get(), scope);
            std::string right = evaluate(binExpr->right.get(), scope);
            
            // 检查是否是数字
            bool leftIsNumber = true;
            bool rightIsNumber = true;
            double leftValue = 0.0;
            double rightValue = 0.0;
            
            // 安全地解析左操作数
            try {
                leftValue = std::stod(left);
            } catch (...) {
                leftIsNumber = false;
            }
            
            // 安全地解析右操作数
            try {
                rightValue = std::stod(right);
            } catch (...) {
                rightIsNumber = false;
            }
            
            // + 操作符特殊处理：如果是数字则进行数字运算，否则进行字符串拼接
            if (binExpr->op == "+") {
                if (leftIsNumber && rightIsNumber && !left.empty() && !right.empty()) {
                    // 两个都是数字，进行数字运算
                    bool leftIsInt = (left.find('.') == std::string::npos);
                    bool rightIsInt = (right.find('.') == std::string::npos);
                    
                    if (leftIsInt && rightIsInt) {
                        // 都是整数
                        int leftInt = static_cast<int>(leftValue);
                        int rightInt = static_cast<int>(rightValue);
                        return std::to_string(leftInt + rightInt);
                    } else {
                        // 至少有一个是小数
                        return std::to_string(leftValue + rightValue);
                    }
                } else {
                    // 有非数字，进行字符串拼接
                    return left + right;
                }
            }
            
            // 逻辑运算符处理
            if (binExpr->op == "&&" || binExpr->op == "和" || binExpr->op == "且") {
                bool leftTrue = (left == "true" || left == "真" || (leftIsNumber && leftValue != 0));
                bool rightTrue = (right == "true" || right == "真" || (rightIsNumber && rightValue != 0));
                return (leftTrue && rightTrue) ? "真" : "假";
            } else if (binExpr->op == "||" || binExpr->op == "或") {
                bool leftTrue = (left == "true" || left == "真" || (leftIsNumber && leftValue != 0));
                bool rightTrue = (right == "true" || right == "真" || (rightIsNumber && rightValue != 0));
                return (leftTrue || rightTrue) ? "真" : "假";
            }
            
            // 比较运算符处理：支持字符串比较
            if (binExpr->op == "==" || binExpr->op == "!=" || binExpr->op == "<" || binExpr->op == "<=" || binExpr->op == ">" || binExpr->op == ">=") {
                // 如果两个操作数都是数字，进行数字比较
                if (leftIsNumber && rightIsNumber) {
                    bool result = false;
                    if (binExpr->op == "==") result = (leftValue == rightValue);
                    else if (binExpr->op == "!=") result = (leftValue != rightValue);
                    else if (binExpr->op == "<") result = (leftValue < rightValue);
                    else if (binExpr->op == "<=") result = (leftValue <= rightValue);
                    else if (binExpr->op == ">") result = (leftValue > rightValue);
                    else if (binExpr->op == ">=") result = (leftValue >= rightValue);
                    return result ? "真" : "假";
                } else {
                    // 否则进行字符串比较
                    bool result = false;
                    if (binExpr->op == "==") result = (left == right);
                    else if (binExpr->op == "!=") result = (left != right);
                    else if (binExpr->op == "<") result = (left < right);
                    else if (binExpr->op == "<=") result = (left <= right);
                    else if (binExpr->op == ">") result = (left > right);
                    else if (binExpr->op == ">=") result = (left >= right);
                    return result ? "真" : "假";
                }
            }
            
            // 对于其他算术操作符，如果任何一个操作数不是数字，抛出错误
            if (!leftIsNumber || !rightIsNumber) {
                throw std::runtime_error("非数字操作数: " + left + " " + binExpr->op + " " + right + " 在第 " + std::to_string(binExpr->line) + " 行");
            }
            
            // 检查是否是整数
            bool leftIsInt = (left.find('.') == std::string::npos);
            bool rightIsInt = (right.find('.') == std::string::npos);
            
            if (leftIsInt && rightIsInt) {
                // 两个操作数都是整数，使用整数计算
                int leftInt = static_cast<int>(leftValue);
                int rightInt = static_cast<int>(rightValue);
                
                if (binExpr->op == "-") {
                    return std::to_string(leftInt - rightInt);
                } else if (binExpr->op == "*") {
                    return std::to_string(leftInt * rightInt);
                } else if (binExpr->op == "/") {
                    if (rightInt == 0) {
                        throw std::runtime_error("除零错误 在第 " + std::to_string(binExpr->line) + " 行");
                    }
                    return std::to_string(leftInt / rightInt);
                } else if (binExpr->op == "%") {
                    if (rightInt == 0) {
                        throw std::runtime_error("取模除零错误 在第 " + std::to_string(binExpr->line) + " 行");
                    }
                    return std::to_string(leftInt % rightInt);
                } else if (binExpr->op == "^") {
                    // 整数乘方运算
                    int result = 1;
                    for (int i = 0; i < rightInt; ++i) {
                        // 防止整数溢出
                        if (result > 2147483647 / leftInt) {
                            throw std::runtime_error("整数乘方结果溢出 在第 " + std::to_string(binExpr->line) + " 行");
                        }
                        result *= leftInt;
                    }
                    return std::to_string(result);
                } else if (binExpr->op == "<") {
                    return leftInt < rightInt ? "真" : "假";
                } else if (binExpr->op == ">") {
                    return leftInt > rightInt ? "真" : "假";
                } else if (binExpr->op == "<=") {
                    return leftInt <= rightInt ? "真" : "假";
                } else if (binExpr->op == ">=") {
                    return leftInt >= rightInt ? "真" : "假";
                } else if (binExpr->op == "==") {
                    return leftInt == rightInt ? "真" : "假";
                } else if (binExpr->op == "!=") {
                    return leftInt != rightInt ? "真" : "假";
                }
            } else {
                // 至少有一个操作数是小数，使用浮点数计算
                if (binExpr->op == "-") {
                    return std::to_string(leftValue - rightValue);
                } else if (binExpr->op == "*") {
                    return std::to_string(leftValue * rightValue);
                } else if (binExpr->op == "/") {
                    if (rightValue == 0) {
                        throw std::runtime_error("除零错误 在第 " + std::to_string(binExpr->line) + " 行");
                    }
                    return std::to_string(leftValue / rightValue);
                } else if (binExpr->op == "^") {
                    // 浮点数乘方运算
                    if (leftValue == 0 && rightValue < 0) {
                        throw std::runtime_error("0的负数次方无意义 在第 " + std::to_string(binExpr->line) + " 行");
                    }
                    return std::to_string(std::pow(leftValue, rightValue));
                } else if (binExpr->op == "<") {
                    return leftValue < rightValue ? "真" : "假";
                } else if (binExpr->op == ">") {
                    return leftValue > rightValue ? "真" : "假";
                } else if (binExpr->op == "<=") {
                    return leftValue <= rightValue ? "真" : "假";
                } else if (binExpr->op == ">=") {
                    return leftValue >= rightValue ? "真" : "假";
                } else if (binExpr->op == "==") {
                    return leftValue == rightValue ? "真" : "假";
                } else if (binExpr->op == "!=") {
                    return leftValue != rightValue ? "真" : "假";
                }
            }
            
            throw std::runtime_error("不支持的运算符: " + binExpr->op + " 在第 " + std::to_string(binExpr->line) + " 行");
        }
        case NodeType::ASSIGNMENT: {
            AssignmentNode* assign = static_cast<AssignmentNode*>(node);
            std::string value = evaluate(assign->expression.get(), scope);
            std::string name = assign->name;
            // 检查变量是否已经在符号表中定义
            if (!scope->hasVariable(name)) {
                throw std::runtime_error("变量未定义: " + name + " 在第 " + std::to_string(node->line) + " 行");
            } else {
                // 如果已经定义，更新它的值
                scope->setVariable(name, value, node->line);
            }
            return value;
        }
        case NodeType::COMPOUND_ASSIGNMENT: {
            CompoundAssignmentNode* compoundAssign = static_cast<CompoundAssignmentNode*>(node);
            std::string name = compoundAssign->name;
            std::string exprValue = evaluate(compoundAssign->expression.get(), scope);
            
            // 检查变量是否存在
            if (!scope->hasVariable(name)) {
                throw std::runtime_error("变量未定义: " + name + " 在第 " + std::to_string(node->line) + " 行");
            }
            
            // 获取变量当前值
            std::string currentValue = scope->getVariable(name, node->line);
            std::string varType = scope->getVariableType(name);
            
            // 执行复合赋值运算
            std::string resultValue;
            bool isNumeric = (varType == "整型" || varType == "小数");
            
            if (isNumeric) {
                // 数字类型处理
                double left = std::stod(currentValue);
                double right = std::stod(exprValue);
                double result;
                
                if (compoundAssign->op == "+") {
                    result = left + right;
                } else if (compoundAssign->op == "-") {
                    result = left - right;
                } else if (compoundAssign->op == "*") {
                    result = left * right;
                } else if (compoundAssign->op == "/") {
                    if (right == 0) {
                        throw std::runtime_error("除零错误 在第 " + std::to_string(node->line) + " 行");
                    }
                    result = left / right;
                } else if (compoundAssign->op == "%") {
                    result = static_cast<long long>(left) % static_cast<long long>(right);
                    resultValue = std::to_string(static_cast<long long>(result));
                    scope->setVariable(name, resultValue, node->line);
                    return resultValue;
                } else if (compoundAssign->op == "^") {
                    result = std::pow(left, right);
                    resultValue = std::to_string(result);
                    scope->setVariable(name, resultValue, node->line);
                    return resultValue;
                } else {
                    throw std::runtime_error("不支持的复合赋值运算符: " + compoundAssign->op + " 在第 " + std::to_string(node->line) + " 行");
                }
                
                // 根据类型格式化结果
                if (varType == "整型") {
                    resultValue = std::to_string(static_cast<long long>(result));
                } else {
                    resultValue = std::to_string(result);
                }
            } else {
                // 字符串类型处理（+=）
                if (compoundAssign->op == "+") {
                    resultValue = currentValue;
                    // 去掉两端的引号（如果有）
                    if (resultValue.length() >= 2 && resultValue.front() == '"' && resultValue.back() == '"') {
                        resultValue = resultValue.substr(1, resultValue.length() - 2);
                    }
                    std::string rightValue = exprValue;
                    if (rightValue.length() >= 2 && rightValue.front() == '"' && rightValue.back() == '"') {
                        rightValue = rightValue.substr(1, rightValue.length() - 2);
                    }
                    resultValue = "\"" + resultValue + rightValue + "\"";
                } else {
                    throw std::runtime_error("字符串类型不支持复合赋值运算符: " + compoundAssign->op + " 在第 " + std::to_string(node->line) + " 行");
                }
            }
            
            scope->setVariable(name, resultValue, node->line);
            return resultValue;
        }
        case NodeType::ARRAY_ASSIGNMENT: {
            ArrayAssignmentNode* assign = static_cast<ArrayAssignmentNode*>(node);
            std::string value = evaluate(assign->expression.get(), scope);
            std::string arrayName = assign->arrayName;
            
            // 计算多维索引
            std::vector<int> indices;
            for (const auto& indexNode : assign->indices) {
                std::string indexValue = evaluate(indexNode.get(), scope);
                // 将索引值转换为整数形式
                try {
                    double indexNum = std::stod(indexValue);
                    int intIndex = static_cast<int>(indexNum);
                    if (intIndex < 0) {
                        throw std::runtime_error("数组索引不能为负数: " + std::to_string(intIndex) + " 在第 " + std::to_string(node->line) + " 行");
                    }
                    indices.push_back(intIndex);
                } catch (const std::invalid_argument& e) {
                    // 如果转换失败，说明索引值不是数字
                    throw std::runtime_error("数组索引必须是数字: " + indexValue + " 在第 " + std::to_string(node->line) + " 行");
                } catch (...) {
                    // 其他异常
                    throw std::runtime_error("无效的数组索引: " + indexValue + " 在第 " + std::to_string(node->line) + " 行");
                }
            }
            
            // 构造多维数组元素的名称（例如：arr[1][2][3]）
            std::string elementName = arrayName;
            for (int idx : indices) {
                elementName += "[" + std::to_string(idx) + "]";
            }
            
            // 在当前作用域中定义或更新变量
            if (!scope->hasVariable(elementName)) {
                // 如果没有定义，先定义它
                scope->defineVariable(elementName, "整型", value, node->line, true);
            } else {
                // 如果已经定义，更新它的值
                scope->setVariable(elementName, value, node->line);
            }
            return value;
        }
        case NodeType::SYSTEM_CMD_EXPRESSION: {
            // 执行系统命令行表达式，返回命令执行结果
            SystemCmdExpressionNode* cmdExpr = static_cast<SystemCmdExpressionNode*>(node);
            std::string result = executeSystemCommandExpression(cmdExpr->commandExpr.get(), scope, node->line);
            return result;
        }
        case NodeType::BRACE_INIT_LIST: {
            // 处理大括号初始化列表
            BraceInitListNode* braceInit = static_cast<BraceInitListNode*>(node);
            std::string result = "";
            
            for (size_t i = 0; i < braceInit->elements.size(); i++) {
                std::string elementValue = evaluate(braceInit->elements[i].get(), scope);
                
                // 检查是否是嵌套的大括号初始化列表
                if (braceInit->elements[i]->type == NodeType::BRACE_INIT_LIST) {
                    // 嵌套的初始化列表，用分号分隔
                    if (i > 0) {
                        result += ";";
                    }
                    result += elementValue;
                } else {
                    // 普通元素，用逗号分隔
                    if (i > 0) {
                        result += ",";
                    }
                    result += elementValue;
                }
            }
            
            return result;
        }
        default:
            throw std::runtime_error("无法计算的表达式类型 在第 " + std::to_string(node->line) + " 行");
    }
}

// 导入文件的实现
void Interpreter::importFile(const std::string& filePath, int line) {
    // 调试信息
    debugOutput("开始导入文件: " + filePath + " 在第 " + std::to_string(line) + " 行");
    
    // 检查是否已经导入过此文件（防止循环导入）
    if (importedFiles.find(filePath) != importedFiles.end()) {
        throw std::runtime_error("检测到循环导入: " + filePath + " 在第 " + std::to_string(line) + " 行");
    }
    
    // 将当前文件添加到已导入文件列表
    importedFiles.insert(filePath);
    
    try {
        // 读取文件内容
        std::ifstream file(filePath);
        if (!file.is_open()) {
            // 尝试相对路径
            std::string currentDir = "./";
            std::string relativePath = currentDir + filePath;
            file.open(relativePath);
            if (!file.is_open()) {
                throw std::runtime_error("无法打开导入文件: " + filePath + " (尝试了 " + relativePath + ") 在第 " + std::to_string(line) + " 行");
            }
        }
        
        // 调试信息
        debugOutput("成功打开文件: " + filePath);
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string fileContent = buffer.str();
        file.close();
        
        // 解析文件内容
        Lexer lexer(fileContent);
        std::vector<Token> tokens = lexer.tokenize();
        
        Parser parser(tokens);
        auto importedProgram = parser.parse();  // 使用parse()而不是parseProgram()
        
        // 将ASTNode转换为ProgramNode以访问statements
        ProgramNode* programNode = static_cast<ProgramNode*>(importedProgram.get());
        
        // 执行导入的程序（不运行主函数）
        for (const auto& stmt : programNode->statements) {
            try {
                if (stmt->type == NodeType::VARIABLE_DEF) {
                    VariableDefNode* varDef = static_cast<VariableDefNode*>(stmt.get());
                    
                    std::string value = "";
                    if (varDef->initializer) {
                        // 执行变量初始化表达式
                        try {
                            value = evaluate(varDef->initializer.get(), globalScope);
                        } catch (const std::exception& e) {
                            throw std::runtime_error("导入文件中的变量初始化失败: " + std::string(e.what()) + " 在第 " + std::to_string(stmt->line) + " 行");
                        }
                    }
                    
                    globalScope->defineVariable(varDef->name, varDef->type, value, stmt->line, true);
                } else if (stmt->type == NodeType::STATEMENT_LIST) {
                    // 处理语句列表（包含多个变量定义）
                    StatementListNode* stmtList = static_cast<StatementListNode*>(stmt.get());
                    
                    for (const auto& subStmt : stmtList->statements) {
                        if (subStmt->type == NodeType::VARIABLE_DEF) {
                            VariableDefNode* varDef = static_cast<VariableDefNode*>(subStmt.get());
                            
                            std::string value = "";
                            if (varDef->initializer) {
                                // 执行变量初始化表达式
                                try {
                                    value = evaluate(varDef->initializer.get(), globalScope);
                                } catch (const std::exception& e) {
                                    throw std::runtime_error("导入文件中的变量初始化失败: " + std::string(e.what()) + " 在第 " + std::to_string(subStmt->line) + " 行");
                                }
                            }
                            
                            globalScope->defineVariable(varDef->name, varDef->type, value, subStmt->line, true);
                        }
                    }
                } else if (stmt->type == NodeType::FUNCTION_DEF) {
                    // 执行函数定义，添加到全局符号表
                    FunctionDefNode* funcDef = static_cast<FunctionDefNode*>(stmt.get());
                    globalScope->defineFunction(funcDef);
                } else if (stmt->type == NodeType::STRUCT_DEF) {
                    // 执行结构体定义，添加到全局符号表
                    StructDefNode* structDef = static_cast<StructDefNode*>(stmt.get());
                    globalScope->defineStruct(structDef->structName, structDef->members);
                } else if (stmt->type == NodeType::SYSTEM_CMD_STATEMENT) {
                    // 执行系统命令行语句
                    executeSystemCommand(static_cast<SystemCmdStatementNode*>(stmt.get())->commandExpr.get(), globalScope, stmt->line);
                } else if (stmt->type == NodeType::IMPORT_STATEMENT) {
                    // 处理嵌套导入
                    ImportStatementNode* nestedImport = static_cast<ImportStatementNode*>(stmt.get());
                    importFile(nestedImport->filePath, stmt->line);
                }
                // 忽略其他类型的语句
            } catch (const std::exception& e) {
                std::cout << "处理语句时出错: " << e.what() << std::endl;
                throw; // 重新抛出异常
            }
        }
        
    } catch (const std::exception& e) {
        // 如果导入失败，从已导入列表中移除
        importedFiles.erase(filePath);
        throw std::runtime_error("导入文件失败: " + filePath + " - " + e.what() + " 在第 " + std::to_string(line) + " 行");
    }
}

// 执行系统命令的实现
void Interpreter::executeSystemCommand(ASTNode* commandNode, SymbolTable* scope, int line) {
    try {
        // 首先计算命令表达式的值
        std::string command = evaluate(commandNode, scope);
        
        // 直接使用system()执行命令
        int result = system(command.c_str());
        
        // 只在出错时输出信息
        if (result == -1) {
            std::cout << "系统命令执行失败: " << command << std::endl;
        } else if (result != 0) {
            std::cout << "系统命令执行完成 (返回码: " << result << ")" << std::endl;
        }
        // 成功时不输出任何信息
        
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("执行系统命令失败: ") + e.what() + " 在第 " + std::to_string(line) + " 行");
    }
}

// 执行系统命令行表达式（返回命令结果）
std::string Interpreter::executeSystemCommandExpression(ASTNode* commandNode, SymbolTable* scope, int line) {
    try {
        // 首先计算命令表达式的值
        std::string command = evaluate(commandNode, scope);
        
        // 使用管道执行命令并获取输出
        std::string result = executeCommandWithOutput(command);
        
        // 返回命令执行结果
        return result;
        
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("执行系统命令表达式失败: ") + e.what() + " 在第 " + std::to_string(line) + " 行");
    }
}

// 使用管道执行命令并获取输出
std::string Interpreter::executeCommandWithOutput(const std::string& command) {
    std::string result;
    
    // 在Windows上使用CreateProcess执行命令
    #ifdef _WIN32
        SECURITY_ATTRIBUTES saAttr;
        saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
        saAttr.bInheritHandle = TRUE;
        saAttr.lpSecurityDescriptor = NULL;
        
        HANDLE hChildStdoutRd, hChildStdoutWr;
        
        if (!CreatePipe(&hChildStdoutRd, &hChildStdoutWr, &saAttr, 0)) {
            return "创建管道失败";
        }
        
        if (!SetHandleInformation(hChildStdoutRd, HANDLE_FLAG_INHERIT, 0)) {
            return "设置句柄信息失败";
        }
        
        PROCESS_INFORMATION piProcInfo;
        STARTUPINFO siStartInfo;
        
        ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
        ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
        
        siStartInfo.cb = sizeof(STARTUPINFO);
        siStartInfo.hStdError = hChildStdoutWr;
        siStartInfo.hStdOutput = hChildStdoutWr;
        siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
        
        std::string cmd = "cmd.exe /c " + command;
        
        BOOL bSuccess = CreateProcess(NULL,
            const_cast<char*>(cmd.c_str()), // 命令行
            NULL,          // 进程安全属性
            NULL,          // 主线程安全属性
            TRUE,          // 句柄继承
            0,             // 创建标志
            NULL,          // 使用父进程环境块
            NULL,          // 使用父进程起始目录
            &siStartInfo,  // STARTUPINFO指针
            &piProcInfo);  // PROCESS_INFORMATION指针
        
        if (!bSuccess) {
            return "创建进程失败";
        }
        
        CloseHandle(hChildStdoutWr);
        
        DWORD dwRead;
        CHAR chBuf[4096];
        BOOL bSuccessRead = FALSE;
        
        for (;;) {
            bSuccessRead = ReadFile(hChildStdoutRd, chBuf, 4096, &dwRead, NULL);
            if (!bSuccessRead || dwRead == 0) break;
            
            std::string temp(chBuf, dwRead);
            result += temp;
        }
        
        WaitForSingleObject(piProcInfo.hProcess, INFINITE);
        
        CloseHandle(piProcInfo.hProcess);
        CloseHandle(piProcInfo.hThread);
        CloseHandle(hChildStdoutRd);
    #else
        // 在Unix/Linux上使用popen
        FILE* pipe = popen(command.c_str(), "r");
        
        if (!pipe) {
            return "命令执行失败";
        }
        
        char buffer[256];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
        
        pclose(pipe);
    #endif
    
    return result;
}

// 调试输出函数
void Interpreter::debugOutput(const std::string& message) {
    if (debugMode) {
        std::cout << "[调试] " << message << std::endl;
    }
}

// 调试令牌信息
void Interpreter::debugTokenInfo(const std::vector<Token>& tokens) {
    if (!debugMode) return;
    
    std::cout << "\n=== 令牌信息 (共 " << tokens.size() << " 个令牌) ===" << std::endl;
    for (size_t i = 0; i < tokens.size(); i++) {
        const Token& token = tokens[i];
        std::cout << "令牌[" << i << "]: 类型=" << static_cast<int>(token.type) 
                  << ", 值='" << token.value << "', 行号=" << token.line << std::endl;
    }
    std::cout << "=== 令牌信息结束 ===\n" << std::endl;
}

// 调试AST信息
void Interpreter::debugASTInfo(ASTNode* node, int depth) {
    if (!debugMode) return;
    
    std::string indent(depth * 2, ' ');
    std::cout << indent << "AST节点: 类型=" << static_cast<int>(node->type) 
              << ", 行号=" << node->line << std::endl;
    
    // 根据节点类型输出详细信息
    switch (node->type) {
        case NodeType::LITERAL: {
            LiteralNode* literal = static_cast<LiteralNode*>(node);
            std::cout << indent << "  字面量: '" << literal->value << "'" << std::endl;
            break;
        }
        case NodeType::IDENTIFIER: {
            IdentifierNode* ident = static_cast<IdentifierNode*>(node);
            std::cout << indent << "  标识符: '" << ident->name << "'" << std::endl;
            break;
        }
        case NodeType::VARIABLE_DEF: {
            VariableDefNode* varDef = static_cast<VariableDefNode*>(node);
            std::cout << indent << "  变量定义: 名称='" << varDef->name 
                      << "', 类型='" << varDef->type << "'" << std::endl;
            if (varDef->initializer) {
                std::cout << indent << "  初始化器:" << std::endl;
                debugASTInfo(varDef->initializer.get(), depth + 1);
            }
            break;
        }
        case NodeType::FUNCTION_DEF: {
            FunctionDefNode* funcDef = static_cast<FunctionDefNode*>(node);
            std::cout << indent << "  函数定义: 名称='" << funcDef->name 
                      << "', 参数个数=" << funcDef->parameters.size() << std::endl;
            if (funcDef->body) {
                std::cout << indent << "  函数体:" << std::endl;
                debugASTInfo(funcDef->body.get(), depth + 1);
            }
            break;
        }
        default:
            // 其他节点类型不详细展开
            break;
    }
}

// 调试符号表信息
void Interpreter::debugSymbolTable(SymbolTable* scope, const std::string& scopeName) {
    if (!debugMode) return;
    
    std::cout << "\n=== 符号表信息 (" << scopeName << ") ===" << std::endl;
    
    // 这里可以添加符号表的具体调试信息
    // 由于SymbolTable的内部结构是私有的，我们只能通过公共接口获取信息
    // 在实际实现中，可能需要为SymbolTable添加调试接口
    
    std::cout << "=== 符号表信息结束 ===\n" << std::endl;
}

// 设置调试模式
void Interpreter::setDebugMode(bool debug) {
    debugMode = debug;
}
