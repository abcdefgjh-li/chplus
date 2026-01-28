#include "../../include/interpreter.h"
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <filesystem>
#include <cstdlib>
#include <chrono>

// 简单的类型推断函数
std::string inferType(const std::string& value) {
    if (value == "真" || value == "假") {
        return "布尔型";
    } else if (value.find(".") != std::string::npos) {
        return "小数";
    } else if (value.length() > 0 && (value[0] == '"' || value[0] == '\'')) {
        return "字符串";
    } else {
        // 尝试解析为整数
        try {
            std::stoi(value);
            return "整型";
        } catch (...) {
            return "字符串"; // 默认当作字符串
        }
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
void SymbolTable::defineVariable(const std::string& name, const std::string& type, const std::string& value, int line) {
    if (variables.find(name) != variables.end()) {
        std::string lineInfo = line > 0 ? " 在第 " + std::to_string(line) + " 行" : "";
        throw std::runtime_error("变量已定义: " + name + lineInfo);
    }
    variables[name] = std::make_pair(type, value);
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
        // 查找匹配的函数定义
        for (const auto& func : it->second) {
            std::vector<std::string> paramTypes;
            for (const auto& param : func->parameters) {
                paramTypes.push_back(param.first);
            }
            
            if (paramTypes == argTypes) {
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
        instance += member.second + "=";
        
        // 根据成员类型设置默认值
        if (member.first == "整型") {
            instance += "0";
        } else if (member.first == "小数") {
            instance += "0.0";
        } else if (member.first == "布尔型") {
            instance += "false";
        } else if (member.first == "字符串") {
            instance += "";
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
Interpreter::Interpreter(std::unique_ptr<ProgramNode> program)
    : program(std::move(program)), globalScope(new SymbolTable()) {
    // 初始化全局作用域
}

// 执行器析构函数
Interpreter::~Interpreter() {
    delete globalScope;
}

// 运行程序
void Interpreter::run() {
    // 首先处理所有全局声明（函数定义、结构体定义、导入语句）
    for (const auto& stmt : program->statements) {
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
        }
    }
    
    // 然后查找并执行主函数
    if (globalScope->hasFunction("主函数")) {
        FunctionDefNode* mainFunc = globalScope->getFunction("主函数", std::vector<std::string>(), 0);
        try {
            // 创建主函数作用域
            SymbolTable* mainScope = globalScope->enterScope();
            executeStatement(mainFunc->body.get(), mainScope, "空类型");
        } catch (const std::runtime_error& e) {
            // 主函数中的返回语句异常不需要处理
            if (e.what() != "__return_from_function__") {
                throw;
            }
        }
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
            std::string value = "";
            if (varDef->initializer) {
                value = evaluate(varDef->initializer.get(), scope);
            }
            
            // 检查是否是结构体类型定义
            if (varDef->type != "整型" && varDef->type != "字符串" && varDef->type != "空类型" && 
                varDef->type != "小数" && varDef->type != "布尔型" && 
                varDef->type != "整型数组" && varDef->type != "字符串数组" && 
                varDef->type != "小数数组" && varDef->type != "布尔型数组") {
                // 这是一个结构体类型
                if (varDef->initializer) {
                    // 检查初始化表达式是否是标识符（变量）
                    if (varDef->initializer->type == NodeType::IDENTIFIER) {
                        IdentifierNode* ident = static_cast<IdentifierNode*>(varDef->initializer.get());
                        std::string sourceVarName = ident->name;
                        
                        // 检查源变量是否存在
                        if (!scope->hasVariable(sourceVarName)) {
                            throw std::runtime_error("变量未定义: " + sourceVarName + " 在第 " + std::to_string(node->line) + " 行");
                        }
                        
                        std::string sourceVarType = scope->getVariableType(sourceVarName);
                        
                        // 检查源变量是否也是结构体类型
                        if (sourceVarType == "整型" || sourceVarType == "字符串" || sourceVarType == "空类型" || 
                            sourceVarType == "小数" || sourceVarType == "布尔型") {
                            throw std::runtime_error("不能将非结构体类型赋值给结构体变量 在第 " + std::to_string(node->line) + " 行");
                        }
                        
                        // 检查结构体类型是否匹配
                        if (varDef->type != sourceVarType) {
                            throw std::runtime_error("结构体类型不匹配: 不能将 " + sourceVarType + " 赋值给 " + varDef->type + " 在第 " + std::to_string(node->line) + " 行");
                        }
                        
                        // 使用源变量的值
                        value = scope->getVariable(sourceVarName, node->line);
                    } else {
                        throw std::runtime_error("结构体变量只能从同类型的变量初始化 在第 " + std::to_string(node->line) + " 行");
                    }
                } else {
                    // 没有初始化器，创建默认实例
                    value = scope->createStructInstance(varDef->type, node->line);
                }
                
                // 定义结构体变量
                scope->defineVariable(varDef->name, varDef->type, value, varDef->line);
            }
            // 检查是否是数组定义（通过检查变量名是否包含方括号）
            else if (varDef->name.find("[") != std::string::npos && varDef->name.find("]") != std::string::npos) {
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
                            int dimensionSize = std::stoi(sizeStr);
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
                
                // 定义数组本身
                scope->defineVariable(varDef->name, varDef->type, value, varDef->line);
                
                // 初始化多维数组中的所有元素为0
                std::vector<int> indices(dimensions.size(), 0);
                bool initialized = true;
                
                while (initialized) {
                    // 构造元素名称
                    std::string elementName = arrayName;
                    for (int idx : indices) {
                        elementName += "[" + std::to_string(idx) + "]";
                    }
                    scope->defineVariable(elementName, varDef->type, "0", varDef->line);
                    
                    // 更新索引
                    int dim = dimensions.size() - 1;
                    while (dim >= 0) {
                        indices[dim]++;
                        if (indices[dim] < dimensions[dim]) {
                            break;
                        } else {
                            indices[dim] = 0;
                            dim--;
                        }
                    }
                    
                    // 检查是否完成所有元素的初始化
                    if (dim < 0) {
                        initialized = false;
                    }
                }
            } else {
                // 普通变量定义
                scope->defineVariable(varDef->name, varDef->type, value, varDef->line);
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
            std::string value = evaluate(coutStmt->expression.get(), scope);
            std::cout << value << std::endl;
            break;
        }
        case NodeType::CIN_STATEMENT: {
            CinStatementNode* cinStmt = static_cast<CinStatementNode*>(node);
            // 检查表达式类型
            if (cinStmt->expression->type != NodeType::IDENTIFIER) {
                throw std::runtime_error("控制台输入语句必须指定变量名 在第 " + std::to_string(node->line) + " 行");
            }
            IdentifierNode* ident = static_cast<IdentifierNode*>(cinStmt->expression.get());
            std::string name = ident->name;
            // 检查变量是否已定义
            if (!scope->hasVariable(name)) {
                throw std::runtime_error("变量未定义: " + name + " 在第 " + std::to_string(node->line) + " 行");
            }
            // 读取输入
            std::string input;
            std::cin >> input;
            // 将输入值存入变量
            scope->setVariable(name, input, node->line);
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
                if (varType != "整型" && varType != "字符串" && varType != "空类型" && 
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
                        if (sourceVarType == "整型" || sourceVarType == "字符串" || sourceVarType == "空类型" || 
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
        case NodeType::WHILE_STATEMENT: {
            WhileStatementNode* whileStmt = static_cast<WhileStatementNode*>(node);
            // 循环执行，直到条件为假
            while (true) {
                std::string conditionValue = evaluate(whileStmt->condition.get(), scope);
                if (conditionValue != "真") {
                    break;
                }
                executeStatement(whileStmt->body.get(), scope);
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
                executeStatement(forStmt->body.get(), scope);
                // 执行更新
                if (forStmt->update) {
                    executeStatement(forStmt->update.get(), scope);
                }
            }
            
            break;
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
                // 简单的类型推断（基于值的格式）
                argumentTypes.push_back(inferType(argValue));
            }
            
            // 获取匹配的函数定义
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
                funcScope->defineVariable(paramName, paramType, argumentValues[i], node->line);
            }
            
            // 创建一个返回值存储变量
            funcScope->defineVariable("__return_value", funcDef->returnType, "", node->line);
            
            // 执行函数体
            try {
                executeStatement(funcDef->body.get(), funcScope, funcDef->returnType);
            } catch (const std::runtime_error& e) {
                // 如果是返回语句异常，直接返回
                if (e.what() == "__return_from_function__") {
                    // 获取返回值
                    std::string returnValue = funcScope->getVariable("__return_value", node->line);
                    
                    // 检查返回类型是否匹配
                    if (funcDef->returnType != "空类型") {
                        std::string inferredType = inferType(returnValue);
                        if (inferredType != funcDef->returnType) {
                            throw std::runtime_error("函数 " + functionName + " 返回类型不匹配: 期望 " + funcDef->returnType + "，但实际返回 " + inferredType + " 在第 " + std::to_string(node->line) + " 行");
                        }
                    }
                    
                    return returnValue;
                } else {
                    throw;
                }
            }
            
            // 如果函数没有显式返回，检查函数是否应该有返回值
            if (funcDef->returnType != "空类型") {
                throw std::runtime_error("函数 " + functionName + " 没有返回预期的类型 " + funcDef->returnType + " 在第 " + std::to_string(node->line) + " 行");
            }
            
            // 如果函数是空类型，返回默认值
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
            
            // 构造多维数组元素的名称（例如：dp[1][2][3]）
            std::string elementName = arrayAccess->arrayName;
            for (int idx : indices) {
                elementName += "[" + std::to_string(idx) + "]";
            }
            
            // 检查变量是否已经在符号表中定义
            if (!scope->hasVariable(elementName)) {
                throw std::runtime_error("数组元素未定义: " + elementName + " 在第 " + std::to_string(node->line) + " 行");
            }
            return scope->getVariable(elementName, node->line);
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
            
            // 获取结构体变量名（member.access 应该是一个标识符）
            if (structAccess->structExpr->type != NodeType::IDENTIFIER) {
                throw std::runtime_error("结构体成员访问左侧必须是标识符 在第 " + std::to_string(node->line) + " 行");
            }
            
            IdentifierNode* structVar = static_cast<IdentifierNode*>(structAccess->structExpr.get());
            std::string structVarName = structVar->name;
            
            // 检查结构体变量是否存在
            if (!scope->hasVariable(structVarName)) {
                throw std::runtime_error("结构体变量未定义: " + structVarName + " 在第 " + std::to_string(node->line) + " 行");
            }
            
            // 获取结构体变量值，格式应该是 "structType:member1=value1;member2=value2"
            std::string structVarValue = scope->getVariable(structVarName, node->line);
            
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
            if (!scope->hasStructMember(structType, structAccess->memberName, node->line)) {
                throw std::runtime_error("结构体 " + structType + " 没有成员 " + structAccess->memberName + " 在第 " + std::to_string(node->line) + " 行");
            }
            
            // 解析结构体成员值
            std::string members = structVarValue.substr(colonPos + 1);
            std::string memberValue = "";
            
            // 解析成员值
            size_t startPos = 0;
            size_t semicolonPos = members.find(';');
            while (semicolonPos != std::string::npos) {
                std::string memberPair = members.substr(startPos, semicolonPos - startPos);
                size_t equalPos = memberPair.find('=');
                if (equalPos != std::string::npos) {
                    std::string currentMemberName = memberPair.substr(0, equalPos);
                    if (currentMemberName == structAccess->memberName) {
                        memberValue = memberPair.substr(equalPos + 1);
                        break;
                    }
                }
                startPos = semicolonPos + 1;
                semicolonPos = members.find(';', startPos);
            }
            
            // 检查最后一个成员
            if (memberValue.empty() && startPos < members.length()) {
                std::string memberPair = members.substr(startPos);
                size_t equalPos = memberPair.find('=');
                if (equalPos != std::string::npos) {
                    std::string currentMemberName = memberPair.substr(0, equalPos);
                    if (currentMemberName == structAccess->memberName) {
                        memberValue = memberPair.substr(equalPos + 1);
                    }
                }
            }
            
            return memberValue;
        }
        case NodeType::STRUCT_MEMBER_ASSIGNMENT: {
            StructMemberAssignmentNode* assign = static_cast<StructMemberAssignmentNode*>(node);
            
            // 获取结构体变量名
            if (assign->structExpr->type != NodeType::IDENTIFIER) {
                throw std::runtime_error("结构体成员赋值左侧必须是标识符 在第 " + std::to_string(node->line) + " 行");
            }
            
            IdentifierNode* structVar = static_cast<IdentifierNode*>(assign->structExpr.get());
            std::string structVarName = structVar->name;
            
            // 检查结构体变量是否存在
            if (!scope->hasVariable(structVarName)) {
                throw std::runtime_error("结构体变量未定义: " + structVarName + " 在第 " + std::to_string(node->line) + " 行");
            }
            
            // 获取结构体变量值
            std::string structVarValue = scope->getVariable(structVarName, node->line);
            
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
            
            // 解析并更新成员值
            size_t startPos = 0;
            size_t semicolonPos = members.find(';');
            bool memberFound = false;
            
            while (semicolonPos != std::string::npos) {
                std::string memberPair = members.substr(startPos, semicolonPos - startPos);
                size_t equalPos = memberPair.find('=');
                if (equalPos != std::string::npos) {
                    std::string currentMemberName = memberPair.substr(0, equalPos);
                    newStructValue += currentMemberName + "=";
                    
                    if (currentMemberName == assign->memberName) {
                        newStructValue += newValue;
                        memberFound = true;
                    } else {
                        newStructValue += memberPair.substr(equalPos + 1);
                    }
                }
                
                newStructValue += ";";
                startPos = semicolonPos + 1;
                semicolonPos = members.find(';', startPos);
            }
            
            // 检查最后一个成员
            if (startPos < members.length()) {
                std::string memberPair = members.substr(startPos);
                size_t equalPos = memberPair.find('=');
                if (equalPos != std::string::npos) {
                    std::string currentMemberName = memberPair.substr(0, equalPos);
                    newStructValue += currentMemberName + "=";
                    
                    if (currentMemberName == assign->memberName) {
                        newStructValue += newValue;
                        memberFound = true;
                    } else {
                        newStructValue += memberPair.substr(equalPos + 1);
                    }
                }
            }
            
            // 如果成员不存在，添加它
            if (!memberFound) {
                if (!members.empty()) {
                    newStructValue += ";";
                }
                newStructValue += assign->memberName + "=" + newValue;
            }
            
            // 更新结构体变量值
            scope->setVariable(structVarName, newStructValue, node->line);
            
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
                if (leftIsNumber && rightIsNumber) {
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
            if (binExpr->op == "&&" || binExpr->op == "和") {
                bool leftTrue = (left == "true" || left == "真" || (leftIsNumber && leftValue != 0));
                bool rightTrue = (right == "true" || right == "真" || (rightIsNumber && rightValue != 0));
                return (leftTrue && rightTrue) ? "真" : "假";
            } else if (binExpr->op == "||" || binExpr->op == "或") {
                bool leftTrue = (left == "true" || left == "真" || (leftIsNumber && leftValue != 0));
                bool rightTrue = (right == "true" || right == "真" || (rightIsNumber && rightValue != 0));
                return (leftTrue || rightTrue) ? "真" : "假";
            }
            
            // 对于其他操作符，如果任何一个操作数不是数字，抛出错误
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
                scope->defineVariable(elementName, "整型", value, node->line);
            } else {
                // 如果已经定义，更新它的值
                scope->setVariable(elementName, value, node->line);
            }
            return value;
        }
        default:
            throw std::runtime_error("无法计算的表达式类型 在第 " + std::to_string(node->line) + " 行");
    }
}

// 导入文件的实现
void Interpreter::importFile(const std::string& filePath, int line) {
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
            throw std::runtime_error("无法打开导入文件: " + filePath + " 在第 " + std::to_string(line) + " 行");
        }
        
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
            if (stmt->type == NodeType::VARIABLE_DEF) {
                VariableDefNode* varDef = static_cast<VariableDefNode*>(stmt.get());
                std::string value = "";
                if (varDef->initializer) {
                    // 避免在导入时执行函数调用或复杂表达式
                    throw std::runtime_error("导入文件中的变量不能有初始化表达式 在第 " + std::to_string(stmt->line) + " 行");
                }
                globalScope->defineVariable(varDef->name, varDef->type, value, stmt->line);
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
