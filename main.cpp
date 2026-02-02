#include "include/lexer.h"
#include "include/parser.h"
#include "include/interpreter.h"
#include "include/CHFormatter.h"
#include "include/asm.h"
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <locale>
#include <unordered_set>
#include <map>

// 函数声明
void setChineseLocale();
std::string readFile(const std::string& filename);
void writeFile(const std::string& filename, const std::string& content);

// 乱码修复：设置中文locale
void setChineseLocale() {
    try {
        // 尝试设置UTF-8编码的中文locale
        std::locale::global(std::locale("zh_CN.UTF-8"));
        std::cout.imbue(std::locale());
        std::cerr.imbue(std::locale());
    } catch (...) {
        // 如果失败，尝试其他常见的中文locale
        try {
            std::locale::global(std::locale("Chinese"));
            std::cout.imbue(std::locale());
            std::cerr.imbue(std::locale());
        } catch (...) {
            // 忽略错误，使用默认locale
        }
    }
    
    // 只在Windows系统上设置控制台编码为UTF-8
#ifdef _WIN32
    system("chcp 65001 >nul 2>&1");
#endif
}

// 读取文件内容
std::string readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("无法打开文件: " + filename);
    }
    
    std::string content;
    std::string line;
    while (std::getline(file, line)) {
        content += line + "\n";
    }
    
    file.close();
    return content;
}

// 写入文件内容
void writeFile(const std::string& filename, const std::string& content) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("无法创建文件: " + filename);
    }
    
    file << content;
    file.close();
}

// 字符串替换函数
void replaceAll(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
}



int main(int argc, char* argv[]) {
    setChineseLocale();
    
    // 解析命令行参数
    bool noFormat = false;
    bool autoFormat = false; // -a 参数
    bool debugMode = false;  // -d 参数
    bool compileToHex = false; // -x 参数：编译为16进制汇编二进制文件
    bool executeHex = false; // -r 参数：执行16进制汇编二进制文件
    std::string filename;
    std::string outputFile;
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--no-format" || arg == "-n") {
            noFormat = true;
        } else if (arg == "-a") {
            autoFormat = true;
        } else if (arg == "-d") {
            debugMode = true;
        } else if (arg == "-x") {
            compileToHex = true;
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                outputFile = argv[++i];
            }
        } else if (arg == "-r") {
            executeHex = true;
        } else if (arg.substr(0, 1) != "-") {
            filename = arg;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "用法: chplus [选项] <文件名>" << std::endl;
            std::cout << "选项:" << std::endl;
            std::cout << "  -a               自动格式化并覆盖原文件" << std::endl;
            std::cout << "  -d               启用调试模式，显示详细执行信息" << std::endl;
            std::cout << "  -x <输出文件>    编译为16进制汇编二进制文件" << std::endl;
            std::cout << "  -r               执行16进制汇编二进制文件" << std::endl;
            std::cout << "  --no-format, -n 不自动格式化代码" << std::endl;
            std::cout << "  --help, -h      显示帮助信息" << std::endl;
            return 0;
        }
    }
    
    if (filename.empty()) {
        std::cerr << "用法: chplus [选项] <文件名>.ch" << std::endl;
        std::cerr << "使用 --help 查看更多信息" << std::endl;
        return 1;
    }
    
    // 检查文件扩展名
    if (filename.length() < 3) {
        std::cerr << "错误: 文件名太短" << std::endl;
        return 1;
    }
    
    std::string extension = filename.substr(filename.length() - 3);
    
    // 执行16进制汇编二进制文件
    if (executeHex) {
        if (extension != "hex" && extension != "bin" && extension != "chexbin") {
            std::cerr << "错误: -r 参数需要 .hex、.bin 或 .chexbin 文件" << std::endl;
            return 1;
        }
        
        try {
            HexAsmParser asmParser;
            if (!asmParser.parseAndExecuteHexBinary(filename)) {
                std::cerr << "执行失败: " << asmParser.getError() << std::endl;
                return 1;
            }
            return 0;
        } catch (const std::exception& e) {
            std::cerr << "错误: " << e.what() << std::endl;
            return 1;
        }
    }
    
    // 检查是否为.ch文件
    if (extension != ".ch") {
        std::cerr << "错误: 只支持 .ch 文件（或使用 -r 参数执行16进制文件）" << std::endl;
        return 1;
    }
    
    try {
        // 读取文件内容
        std::string code = readFile(filename);
        
        // 编译为16进制汇编二进制文件
        if (compileToHex) {
            if (outputFile.empty()) {
                // 如果没有指定输出文件，使用默认名称
                outputFile = filename.substr(0, filename.length() - 3) + ".hexbin";
            }
            
            HexAsmParser asmParser;
            if (asmParser.compileCHToHexBinary(code, outputFile)) {
                std::cout << "成功编译为16进制汇编二进制文件: " << outputFile << std::endl;
            } else {
                std::cerr << "编译失败: " << asmParser.getError() << std::endl;
                return 1;
            }
            return 0;
        }
        
        // 自动格式化并覆盖原文件
        if (autoFormat) {
            CHFormatter formatter(code);
            std::string formattedCode = formatter.format(true, false);
            writeFile(filename, formattedCode);
            std::cout << "文件已自动格式化: " << filename << std::endl;
            return 0; // 格式化后直接返回，不执行代码
        }
        
        // 编译时格式化（不修改原文件）
        if (!noFormat) {
            CHFormatter formatter(code);
            code = formatter.format(true, false);
        }
        
        // 词法分析
        Lexer lexer(code);
        std::vector<Token> tokens = lexer.tokenize();
        
        // 语法分析
        Parser parser(tokens);
        parser.setDebugMode(debugMode);
        auto program = parser.parse();
        
        // 执行
        Interpreter interpreter(std::move(program), debugMode);
        
        // 检查是否是标准库文件（不包含主函数的文件）
        bool hasMainFunction = false;
        for (const auto& token : tokens) {
            if (token.type == TokenType::MAIN) {
                hasMainFunction = true;
                break;
            }
        }
        
        if (hasMainFunction) {
            // 如果是程序文件（包含主函数），正常执行
            interpreter.run();
        } else {
            // 如果是库文件（不包含主函数），只解析不执行
            std::cout << "库文件已加载: " << filename << " (不包含主函数，跳过执行)" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "未知错误: 程序发生未捕获的异常（可能是内存访问错误）" << std::endl;
        return 1;
    }
    
    return 0;
}