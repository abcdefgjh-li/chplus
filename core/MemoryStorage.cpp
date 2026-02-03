#include "MemoryStorage.h"
#include <iostream>

MemoryStorage::MemoryStorage() {
}

MemoryStorage::~MemoryStorage() {
}

void MemoryStorage::defineVariable(const std::string& name, const std::string& type, const std::string& value) {
    cache[name] = value;
    dirtyVariables.insert(name);
}

void MemoryStorage::setVariable(const std::string& name, const std::string& value) {
    auto it = cache.find(name);
    if (it != cache.end() && it->second == value) {
        return;
    }
    
    cache[name] = value;
    dirtyVariables.insert(name);
}

std::string MemoryStorage::getVariable(const std::string& name) const {
    auto it = cache.find(name);
    if (it != cache.end()) {
        return it->second;
    }
    
    return "";
}

std::string MemoryStorage::getVariableType(const std::string& name) const {
    return "";
}

bool MemoryStorage::hasVariable(const std::string& name) const {
    return cache.find(name) != cache.end();
}

void MemoryStorage::clear() {
    cache.clear();
    dirtyVariables.clear();
}

void MemoryStorage::printAll() const {
    std::cout << "=== 内存存储内容 ===" << std::endl;
    for (const auto& var : cache) {
        std::cout << var.first << "=" << var.second << std::endl;
    }
    std::cout << "===================" << std::endl;
}
