#ifndef MEMORYSTORAGE_H
#define MEMORYSTORAGE_H

#include <string>
#include <map>
#include <set>

class MemoryStorage {
private:
    std::map<std::string, std::string> cache;
    std::set<std::string> dirtyVariables;
    
public:
    MemoryStorage();
    ~MemoryStorage();
    
    void defineVariable(const std::string& name, const std::string& type, const std::string& value = "");
    void setVariable(const std::string& name, const std::string& value);
    std::string getVariable(const std::string& name) const;
    std::string getVariableType(const std::string& name) const;
    bool hasVariable(const std::string& name) const;
    void clear();
    void printAll() const;
};

#endif
