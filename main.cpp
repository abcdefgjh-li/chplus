#include "include/lexer.h"
#include "include/parser.h"
#include "include/interpreter.h"
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <locale>
#include <unordered_set>
#include <map>

// CHFormatter类定义
class CHFormatter {
private:
    std::string source;
    std::ostringstream formatted;
    bool inString;
    bool inComment;
    bool inBlockComment;
    char quoteChar;
    int lineNumber;
    int currentIndent;
    std::unordered_set<std::string> keywords;

public:
    CHFormatter(const std::string& src) 
        : source(src), inString(false), inComment(false), 
          inBlockComment(false), quoteChar('\0'), lineNumber(1), currentIndent(0) {
        initializeKeywords();
    }

    std::string format(bool autoFormat = true, bool noFormat = false);

private:
    void initializeKeywords();
    std::string formatWithPreprocessing();
    std::string formatWithoutPreprocessing();
    void preprocessWhitespace(std::string& code);
    std::string replacePattern(const std::string& code, const std::string& from, const std::string& to);
    void replaceChineseSymbols(std::string& code);
    void processFormatting(std::string& code);
    void applyIndentation(std::string& code);
    void trim(std::string& str);
};

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

// CHFormatter类的完整实现
std::string CHFormatter::format(bool autoFormat, bool noFormat) {
    if (noFormat) {
        return source; // 不格式化，直接返回原代码
    }
    
    formatted.str("");
    formatted.clear();
    inString = false;
    inComment = false;
    inBlockComment = false;
    quoteChar = '\0';
    lineNumber = 1;
    currentIndent = 0;

    if (autoFormat) {
        return formatWithPreprocessing();
    } else {
        return formatWithoutPreprocessing();
    }
}

void CHFormatter::initializeKeywords() {
    keywords = {
        "定义", "整型", "字符串", "字符型", "空类型", "主函数",
        "如果", "否则", "否则如果", "控制台输出", "控制台输入",
        "小数", "布尔型", "真", "假", "结构体", "数组",
        "当", "对于", "返回", "文件读取", "文件写入", "文件追加",
        "导入", "系统命令行", "和", "或", "或者"
    };
}

std::string CHFormatter::formatWithPreprocessing() {
    std::string result = source;
    
    // 预处理：规范化空格和括号
    preprocessWhitespace(result);
    
    // 处理换行和缩进
    processFormatting(result);
    
    return result;
}

std::string CHFormatter::formatWithoutPreprocessing() {
    std::string result = source;
    
    // 只处理换行和缩进，不预处理空格
    processFormatting(result);
    
    return result;
}

void CHFormatter::preprocessWhitespace(std::string& code) {
    // 使用简单字符串替换而不是正则表达式
    replaceChineseSymbols(code);
    
    // 移除函数调用中的多余空格
    code = replacePattern(code, "定义 (", "定义(");
    code = replacePattern(code, "控制台输出 (", "控制台输出(");
    code = replacePattern(code, "如果 (", "如果(");
    code = replacePattern(code, "对于 (", "对于(");
    code = replacePattern(code, "当 (", "当(");
    
    // 移除括号周围的多余空格
    code = replacePattern(code, " ( ", "(");
    code = replacePattern(code, " ) ", ")");
    
    // 移除语句结束的多余空格
    code = replacePattern(code, " ;", ";");
    code = replacePattern(code, "; ", ";");
    
    // 移除大括号前后的多余空格
    code = replacePattern(code, " {", "{");
    code = replacePattern(code, "{ ", "{");
    code = replacePattern(code, " }", "}");
    code = replacePattern(code, "} ", "}");
    
    // 清理多余的空格
    while (code.find("  ") != std::string::npos) {
        code.replace(code.find("  "), 2, " ");
    }
    
    // 清理行尾空格
    std::istringstream iss(code);
    std::string line;
    std::string result;
    
    while (std::getline(iss, line)) {
        // 移除行尾空格
        size_t end = line.length();
        while (end > 0 && (line[end - 1] == ' ' || line[end - 1] == '\t')) {
            end--;
        }
        if (end < line.length()) {
            line = line.substr(0, end);
        }
        result += line + "\n";
    }
    
    code = result;
}

std::string CHFormatter::replacePattern(const std::string& code, const std::string& from, const std::string& to) {
    std::string result = code;
    size_t start = 0;
    while ((start = result.find(from, start)) != std::string::npos) {
        result.replace(start, from.length(), to);
        start += to.length();
    }
    return result;
}

void CHFormatter::replaceChineseSymbols(std::string& code) {
    // 简化版本：只处理最常见的中文标点符号
    // 使用UTF-8字节序列进行替换
    replacePattern(code, "（", "(");
    replacePattern(code, "）", ")");
    replacePattern(code, "。", ".");
    replacePattern(code, "，", ",");
    replacePattern(code, "；", ";");
    replacePattern(code, "：", ":");
    replacePattern(code, "？", "?");
    replacePattern(code, "！", "!");
    replacePattern(code, "【", "[");
    replacePattern(code, "】", "]");
    replacePattern(code, "《", "<");
    replacePattern(code, "》", ">");
}

void CHFormatter::processFormatting(std::string& code) {
    formatted.str("");
    formatted.clear();
    
    // 应用缩进
    applyIndentation(code);
}

void CHFormatter::applyIndentation(std::string& code) {
    formatted.str("");
    formatted.clear();
    
    std::istringstream iss(code);
    std::string line;
    int indentLevel = 0;
    bool firstLine = true;
    
    while (std::getline(iss, line)) {
        // 移除行首尾空格
        trim(line);
        
        if (line.empty()) {
            formatted << "\n";
            continue;
        }
        
        // 检查这行是否是右大括号（且不是左大括号）
        bool isClosingBrace = line.find('}') != std::string::npos && line.find('{') == std::string::npos;
        
        // 如果是右大括号，减少缩进
        if (isClosingBrace) {
            indentLevel = std::max(0, indentLevel - 1);
        }
        
        // 添加换行（除了第一行）
        if (!firstLine) {
            formatted << "\n";
        }
        firstLine = false;
        
        // 输出带缩进的行
        for (int i = 0; i < indentLevel * 4; i++) {
            formatted << " ";
        }
        formatted << line;
        
        // 检查这行是否以左大括号结尾（且不是右大括号）
        bool endsWithOpeningBrace = line.find('{') != std::string::npos && line.find('}') == std::string::npos;
        
        // 如果以左大括号结尾，增加缩进
        if (endsWithOpeningBrace) {
            indentLevel++;
        }
    }
}

void CHFormatter::trim(std::string& str) {
    // 移除行首空格
    size_t start = 0;
    while (start < str.length() && (str[start] == ' ' || str[start] == '\t')) {
        start++;
    }
    
    // 移除行尾空格
    size_t end = str.length();
    while (end > start && (str[end - 1] == ' ' || str[end - 1] == '\t')) {
        end--;
    }
    
    if (start < end) {
        str = str.substr(start, end - start);
    } else {
        str = "";
    }
}

int main(int argc, char* argv[]) {
    setChineseLocale();
    
    // 解析命令行参数
    bool noFormat = false;
    bool autoFormat = false; // -a 参数
    bool debugMode = false;  // -d 参数
    std::string filename;
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--no-format" || arg == "-n") {
            noFormat = true;
        } else if (arg == "-a") {
            autoFormat = true;
        } else if (arg == "-d") {
            debugMode = true;
        } else if (arg.substr(0, 1) != "-") {
            filename = arg;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "用法: chplus [选项] <文件名>.ch" << std::endl;
            std::cout << "选项:" << std::endl;
            std::cout << "  -a               自动格式化并覆盖原文件" << std::endl;
            std::cout << "  -d               启用调试模式，显示详细执行信息" << std::endl;
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
    
    try {
        // 读取文件内容
        std::string code = readFile(filename);
        
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
    }
    
    return 0;
}