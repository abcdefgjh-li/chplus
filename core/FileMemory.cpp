#include "FileMemory.h"
#include <iostream>
#include <algorithm>
#include <sstream>
#include <climits>

void FileMemory::loadCache() {
    if (cacheLoaded) return;
    
    file.close();
    file.open(filename, std::ios::in);
    if (!file.is_open()) {
        cacheLoaded = true;
        return;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string name = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            
            if (value.find('{') == 0) {
                cache[name] = value;
                parseArrayString(name, value);
            } else {
                cache[name] = value;
            }
        }
    }
    
    file.close();
    cacheLoaded = true;
}

void FileMemory::parseArrayString(const std::string& arrayName, const std::string& arrayStr) {
    if (arrayStr.length() < 2 || arrayStr[0] != '{' || arrayStr[arrayStr.length() - 1] != '}') {
        return;
    }
    
    std::string content = arrayStr.substr(1, arrayStr.length() - 2);
    size_t pos = 0;
    
    while (pos < content.length()) {
        size_t colonPos = content.find(':', pos);
        if (colonPos == std::string::npos) break;
        
        std::string indexStr = content.substr(pos, colonPos - pos);
        size_t commaPos = content.find(',', colonPos + 1);
        if (commaPos == std::string::npos) {
            commaPos = content.length();
        }
        
        std::string valueStr = content.substr(colonPos + 1, commaPos - colonPos - 1);
        std::string elementName = arrayName + "[" + indexStr + "]";
        cache[elementName] = valueStr;
        
        pos = commaPos + 1;
    }
}

void FileMemory::flushCache() {
    if (dirtyVariables.empty()) return;
    
    file.close();
    
    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        throw std::runtime_error("无法写入内存文件: " + filename);
    }
    
    std::set<std::string> processedArrays;
    
    for (const auto& var : cache) {
        std::string name = var.first;
        std::string value = var.second;
        
        if (name.find('[') != std::string::npos) {
            std::string arrayName = name.substr(0, name.find('['));
            if (processedArrays.find(arrayName) != processedArrays.end()) {
                continue;
            }
            processedArrays.insert(arrayName);
            std::string arrayStr = buildArrayString(arrayName);
            outFile << arrayName << "=" << arrayStr << std::endl;
        } else {
            outFile << name << "=" << value << std::endl;
        }
    }
    
    outFile.close();
    dirtyVariables.clear();
}

std::string FileMemory::buildArrayString(const std::string& arrayName) {
    std::vector<std::pair<int, std::string>> elements;
    
    for (auto it = cache.begin(); it != cache.end(); ++it) {
        if (it->first.substr(0, arrayName.length()) == arrayName && 
            it->first.length() > arrayName.length() &&
            it->first[arrayName.length()] == '[') {
            
            std::string indexStr = it->first.substr(arrayName.length() + 1);
            size_t bracketPos = indexStr.find(']');
            if (bracketPos != std::string::npos) {
                indexStr = indexStr.substr(0, bracketPos);
            }
            
            try {
                int index = std::stoi(indexStr);
                elements.push_back(std::make_pair(index, it->second));
            } catch (...) {
                elements.push_back(std::make_pair(INT_MAX, it->second));
            }
        }
    }
    
    std::sort(elements.begin(), elements.end());
    
    std::string result = "{";
    for (size_t i = 0; i < elements.size(); ++i) {
        if (i > 0) {
            result += ",";
        }
        result += std::to_string(elements[i].first) + ":" + elements[i].second;
    }
    result += "}";
    
    return result;
}

FileMemory::FileMemory(const std::string& fname, bool keep) : filename(fname), keepFile(keep), cacheLoaded(false) {
    loadCache();
}

FileMemory::~FileMemory() {
    flushCache();
    if (!keepFile) {
        std::remove(filename.c_str());
    }
}

void FileMemory::defineVariable(const std::string& name, const std::string& type, const std::string& value) {
    loadCache();
    cache[name] = value;
    dirtyVariables.insert(name);
}

void FileMemory::setVariable(const std::string& name, const std::string& value) {
    loadCache();
    
    auto it = cache.find(name);
    if (it != cache.end() && it->second == value) {
        return;
    }
    
    cache[name] = value;
    dirtyVariables.insert(name);
}

std::string FileMemory::getVariable(const std::string& name) const {
    const_cast<FileMemory*>(this)->loadCache();
    
    auto it = cache.find(name);
    if (it != cache.end()) {
        return it->second;
    }
    
    if (name.find('[') != std::string::npos) {
        std::string arrayName = name.substr(0, name.find('['));
        auto arrayIt = cache.find(arrayName);
        if (arrayIt != cache.end()) {
            std::string arrayStr = arrayIt->second;
            if (arrayStr.find('{') == 0) {
                const_cast<FileMemory*>(this)->parseArrayString(arrayName, arrayStr);
                it = cache.find(name);
                if (it != cache.end()) {
                    return it->second;
                }
            }
            
            std::string indexStr = name.substr(arrayName.length() + 1);
            size_t bracketPos = indexStr.find(']');
            if (bracketPos != std::string::npos) {
                indexStr = indexStr.substr(0, bracketPos);
            }
            
            std::string searchStr = indexStr + ":";
            size_t searchPos = arrayStr.find(searchStr);
            if (searchPos != std::string::npos) {
                size_t valueStart = searchPos + searchStr.length();
                size_t valueEnd = arrayStr.find(",", valueStart);
                if (valueEnd == std::string::npos) {
                    valueEnd = arrayStr.find("}", valueStart);
                }
                if (valueEnd != std::string::npos) {
                    return arrayStr.substr(valueStart, valueEnd - valueStart);
                }
            }
        }
    }
    
    return "";
}

std::string FileMemory::getVariableType(const std::string& name) const {
    return "";
}

bool FileMemory::hasVariable(const std::string& name) const {
    const_cast<FileMemory*>(this)->loadCache();
    return cache.find(name) != cache.end();
}

void FileMemory::clear() {
    cache.clear();
    dirtyVariables.clear();
    cacheLoaded = true;
    
    file.close();
    std::ofstream outFile(filename, std::ios::trunc);
    outFile.close();
}

void FileMemory::printAll() const {
    const_cast<FileMemory*>(this)->loadCache();
    std::cout << "=== 文件内存内容 ===" << std::endl;
    for (const auto& var : cache) {
        std::cout << var.first << "=" << var.second << std::endl;
    }
    std::cout << "=====================" << std::endl;
}
