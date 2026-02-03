#ifndef FILEMEMORY_H
#define FILEMEMORY_H

#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <set>

class FileMemory {
private:
    std::string filename;
    bool keepFile;
    mutable std::fstream file;
    std::map<std::string, std::string> cache;
    std::set<std::string> dirtyVariables;
    bool cacheLoaded;
    
    void loadCache();
    void flushCache();
    void parseArrayString(const std::string& arrayName, const std::string& arrayStr);
    std::string buildArrayString(const std::string& arrayName);
    
public:
    FileMemory(const std::string& fname = "memory.txt", bool keep = false);
    ~FileMemory();
    
    void defineVariable(const std::string& name, const std::string& type, const std::string& value = "");
    void setVariable(const std::string& name, const std::string& value);
    std::string getVariable(const std::string& name) const;
    std::string getVariableType(const std::string& name) const;
    bool hasVariable(const std::string& name) const;
    void clear();
    void printAll() const;
};

#endif
