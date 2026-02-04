#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <variant>
#include <stdexcept>

class LFJSONValue;
class LFJSONObject;

// 定义容器类型
using LFJSONArray     = std::vector<LFJSONValue>;
using LFJSONMap       = std::map<std::string, LFJSONValue>;

// 定义指针类型
using LFJSONArrayPtr  = std::shared_ptr<LFJSONArray>;
using LFJSONMapPtr    = std::shared_ptr<LFJSONMap>;

// 核心 Variant 定义
// 使用 shared_ptr 确保在 LFJSONValue 定义完成前，大小是固定的
using LFJSONVariant = std::variant<
    std::nullptr_t, 
    bool, 
    double, 
    std::string, 
    LFJSONArrayPtr, 
    LFJSONMapPtr
>;

class LFJSONObject {
private:
    LFJSONMapPtr _map;

public:
    using Ptr = std::shared_ptr<LFJSONObject>;

    // 构造函数只赋值指针，不涉及 map 内部结构，所以可以在这里写
    LFJSONObject(LFJSONMapPtr m) : _map(m) {}

    LFJSONValue& at(const std::string& key);
    bool contains(const std::string& key) const;
    size_t size() const;
    LFJSONMap& raw();
};

class LFJSONValue {
public:
    LFJSONVariant data;

    LFJSONValue() : data(nullptr) {}
    LFJSONValue(std::nullptr_t) : data(nullptr) {}
    LFJSONValue(bool v) : data(v) {}
    LFJSONValue(double v) : data(v) {}
    LFJSONValue(int v) : data(static_cast<double>(v)) {}
    LFJSONValue(std::string v) : data(std::move(v)) {}
    LFJSONValue(const char* v) : data(std::string(v)) {}

    // 容器构造
    LFJSONValue(LFJSONArray v) : data(std::make_shared<LFJSONArray>(std::move(v))) {}
    LFJSONValue(LFJSONMap v)   : data(std::make_shared<LFJSONMap>(std::move(v))) {}

    // 类型判断
    bool isNull()   const { return std::holds_alternative<std::nullptr_t>(data); }
    bool isBool()   const { return std::holds_alternative<bool>(data); }
    bool isNumber() const { return std::holds_alternative<double>(data); }
    bool isString() const { return std::holds_alternative<std::string>(data); }
    bool isArray()  const { return std::holds_alternative<LFJSONArrayPtr>(data); }
    bool isObject() const { return std::holds_alternative<LFJSONMapPtr>(data); }

    // 基础转换
    std::string asString() const { 
        if (!isString()) throw std::runtime_error("LFJSON: Not a string");
        return std::get<std::string>(data); 
    }
    double asDouble() const { 
        if (!isNumber()) throw std::runtime_error("LFJSON: Not a number");
        return std::get<double>(data); 
    }
    int asInt() const { return static_cast<int>(asDouble()); }
    bool asBool() const { 
        if (!isBool()) throw std::runtime_error("LFJSON: Not a boolean");
        return std::get<bool>(data); 
    }

    // 获取数组
    LFJSONArray& asArray() {
        if (!isArray()) throw std::runtime_error("LFJSON: Not an array");
        return *std::get<LFJSONArrayPtr>(data);
    }

    // 获取对象封装
    LFJSONObject::Ptr asObject() const {
        if (!isObject()) throw std::runtime_error("LFJSON: Not an object");
        return std::make_shared<LFJSONObject>(std::get<LFJSONMapPtr>(data));
    }

    // 链式调用 - 对象
    LFJSONValue& at(const std::string& key) {
        if (!isObject()) throw std::runtime_error("LFJSON: Not an object, cannot access key: " + key);
        auto& mapPtr = std::get<LFJSONMapPtr>(data);
        auto it = mapPtr->find(key);
        if (it == mapPtr->end()) throw std::runtime_error("LFJSON: Key not found: " + key);
        return it->second;
    }

    // 链式调用 - 数组
    LFJSONValue& at(size_t index) {
        if (!isArray()) throw std::runtime_error("LFJSON: Not an array");
        auto& arrPtr = std::get<LFJSONArrayPtr>(data);
        if (index >= arrPtr->size()) throw std::runtime_error("LFJSON: Index out of range");
        return arrPtr->at(index);
    }
};

inline LFJSONValue& LFJSONObject::at(const std::string& key) {
    auto it = _map->find(key);
    if (it == _map->end()) throw std::runtime_error("LFJSON: Key not found: " + key);
    return it->second;
}

inline bool LFJSONObject::contains(const std::string& key) const {
    return _map->find(key) != _map->end();
}

inline size_t LFJSONObject::size() const {
    return _map->size();
}

inline LFJSONMap& LFJSONObject::raw() {
    return *_map;
}

class LFJSONParser {
public:
    static LFJSONObject::Ptr parse(const std::string& json) {
        size_t offset = 0;
        LFJSONValue root = parseValue(json, offset);
        if (!root.isObject()) throw std::runtime_error("LFJSON: Root must be an object");
        return root.asObject();
    }
    
    static LFJSONObject::Ptr parse(const unsigned char* data, size_t len) {
        if (len == 0) throw std::invalid_argument("LFJSON: Empty input");
        return parse(std::string(reinterpret_cast<const char*>(data), len));
    }

private:
    static void skipWS(const std::string& s, size_t& o) {
        while (o < s.size() && (s[o] <= 32)) o++;
    }

    static LFJSONValue parseValue(const std::string& s, size_t& o) {
        skipWS(s, o);
        if (o >= s.size()) throw std::runtime_error("LFJSON: Unexpected end");

        char c = s[o];
        if (c == '{') return parseObject(s, o);
        if (c == '[') return parseArray(s, o);
        if (c == '"') return parseString(s, o);
        if (isdigit(c) || c == '-') return parseNumber(s, o);
        if (s.compare(o, 4, "true") == 0) { o += 4; return true; }
        if (s.compare(o, 5, "false") == 0) { o += 5; return false; }
        if (s.compare(o, 4, "null") == 0) { o += 4; return nullptr; }
        
        throw std::runtime_error("LFJSON: Invalid token at index " + std::to_string(o));
    }

    static LFJSONValue parseObject(const std::string& s, size_t& o) {
        LFJSONMap m;
        o++; // skip {
        while (true) {
            skipWS(s, o);
            if (o < s.size() && s[o] == '}') { o++; break; }
            
            std::string key = parseString(s, o).asString();
            skipWS(s, o);
            if (o >= s.size() || s[o] != ':') throw std::runtime_error("LFJSON: Expected ':'");
            o++; // skip :
            
            m[key] = parseValue(s, o);
            
            skipWS(s, o);
            if (o < s.size() && s[o] == ',') o++;
            else if (o < s.size() && s[o] == '}') { o++; break; }
            else throw std::runtime_error("LFJSON: Expected ',' or '}'");
        }
        return m;
    }

    static LFJSONValue parseArray(const std::string& s, size_t& o) {
        LFJSONArray v;
        o++; // skip [
        while (true) {
            skipWS(s, o);
            if (o < s.size() && s[o] == ']') { o++; break; }
            
            v.push_back(parseValue(s, o));
            
            skipWS(s, o);
            if (o < s.size() && s[o] == ',') o++;
            else if (o < s.size() && s[o] == ']') { o++; break; }
            else throw std::runtime_error("LFJSON: Expected ',' or ']'");
        }
        return v;
    }

    static LFJSONValue parseString(const std::string& s, size_t& o) {
        o++; // skip "
        std::string res;
        // 预分配，避免频繁扩容
        res.reserve(16); 
        while (o < s.size()) {
            char c = s[o++];
            if (c == '"') break;
            if (c == '\\') {
                if (o < s.size()) {
                    char escaped = s[o++];
                    if(escaped == 'n') res += '\n';
                    else if(escaped == 't') res += '\t';
                    else if(escaped == 'r') res += '\r';
                    else res += escaped;
                }
            } else {
                res += c;
            }
        }
        return res;
    }

    static LFJSONValue parseNumber(const std::string& s, size_t& o) {
        size_t end;
        // 注意：这里为了兼容性还是用 stod，生产环境建议自行实现 parseDouble 避免 locale 问题
        double d = std::stod(s.substr(o), &end);
        o += end;
        return d;
    }
};