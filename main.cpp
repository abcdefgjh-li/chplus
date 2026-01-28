#include "include/lexer.h"
#include "include/parser.h"
#include "include/interpreter.h"
#include <fstream>
#include <iostream>
#include <string>
#include <locale>

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
    
    // 在Windows上设置控制台编码为UTF-8
    system("chcp 65001");
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

int main(int argc, char* argv[]) {
    setChineseLocale();
    
    if (argc != 2) {
        std::cerr << "用法: chplus <文件名>.ch" << std::endl;
        return 1;
    }
    
    std::string filename = argv[1];
    
    try {
        // 读取文件内容
        std::string code = readFile(filename);
        
        // 词法分析
        Lexer lexer(code);
        std::vector<Token> tokens = lexer.tokenize();
        
        // 语法分析
        Parser parser(tokens);
        auto program = parser.parse();
        
        // 执行
        Interpreter interpreter(std::move(program));
        interpreter.run();
        
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
