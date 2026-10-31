#pragma once

#include <iostream>
#include <algorithm>
#include <string>
#include <vector>
#include <map>
#include <variant>
#include <stdexcept>
#include <memory>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string_view>

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
    int asInt() const {
        const double value = asDouble();
        if (value < static_cast<double>(std::numeric_limits<int>::min()) ||
            value > static_cast<double>(std::numeric_limits<int>::max())) {
            throw std::runtime_error("LFJSON: Number out of int range");
        }
        return static_cast<int>(value);
    }
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
        LFJSONValue root = parseValue(json, offset, 0);
        skipWS(json, offset);
        if (offset != json.size()) {
            throw std::runtime_error("LFJSON: Unexpected trailing content at index " + std::to_string(offset));
        }
        if (!root.isObject()) throw std::runtime_error("LFJSON: Root must be an object");
        return root.asObject();
    }
    
    static LFJSONObject::Ptr parse(const unsigned char* data, size_t len) {
        if (len == 0) throw std::invalid_argument("LFJSON: Empty input");
        return parse(std::string(reinterpret_cast<const char*>(data), len));
    }

private:
    static constexpr size_t kMaxDepth = 256;

    static void skipWS(const std::string& s, size_t& o) {
        while (o < s.size() &&
               (s[o] == ' ' || s[o] == '\t' || s[o] == '\n' || s[o] == '\r')) {
            ++o;
        }
    }

    static LFJSONValue parseValue(const std::string& s, size_t& o, size_t depth) {
        if (depth > kMaxDepth) {
            throw std::runtime_error("LFJSON: Maximum nesting depth exceeded");
        }

        skipWS(s, o);
        if (o >= s.size()) throw std::runtime_error("LFJSON: Unexpected end");

        char c = s[o];
        if (c == '{') return parseObject(s, o, depth + 1);
        if (c == '[') return parseArray(s, o, depth + 1);
        if (c == '"') return parseString(s, o);
        if ((c >= '0' && c <= '9') || c == '-') return parseNumber(s, o);
        if (s.compare(o, 4, "true") == 0) { o += 4; return true; }
        if (s.compare(o, 5, "false") == 0) { o += 5; return false; }
        if (s.compare(o, 4, "null") == 0) { o += 4; return nullptr; }
        
        throw std::runtime_error("LFJSON: Invalid token at index " + std::to_string(o));
    }

    static LFJSONValue parseObject(const std::string& s, size_t& o, size_t depth) {
        LFJSONMap m;
        o++; // skip {
        while (true) {
            skipWS(s, o);
            if (o < s.size() && s[o] == '}') { o++; break; }
            if (o >= s.size()) throw std::runtime_error("LFJSON: Unexpected end in object");

            std::string key = parseString(s, o).asString();
            skipWS(s, o);
            if (o >= s.size() || s[o] != ':') throw std::runtime_error("LFJSON: Expected ':'");
            o++; // skip :

            if (m.find(key) != m.end()) {
                throw std::runtime_error("LFJSON: Duplicate object key: " + key);
            }
            m.emplace(std::move(key), parseValue(s, o, depth));
            
            skipWS(s, o);
            if (o < s.size() && s[o] == ',') {
                ++o;
                skipWS(s, o);
                if (o < s.size() && s[o] == '}') {
                    throw std::runtime_error("LFJSON: Trailing comma in object");
                }
            }
            else if (o < s.size() && s[o] == '}') { o++; break; }
            else throw std::runtime_error("LFJSON: Expected ',' or '}'");
        }
        return m;
    }

    static LFJSONValue parseArray(const std::string& s, size_t& o, size_t depth) {
        LFJSONArray v;
        o++; // skip [
        while (true) {
            skipWS(s, o);
            if (o < s.size() && s[o] == ']') { o++; break; }
            if (o >= s.size()) throw std::runtime_error("LFJSON: Unexpected end in array");

            v.push_back(parseValue(s, o, depth));
            
            skipWS(s, o);
            if (o < s.size() && s[o] == ',') {
                ++o;
                skipWS(s, o);
                if (o < s.size() && s[o] == ']') {
                    throw std::runtime_error("LFJSON: Trailing comma in array");
                }
            }
            else if (o < s.size() && s[o] == ']') { o++; break; }
            else throw std::runtime_error("LFJSON: Expected ',' or ']'");
        }
        return v;
    }

    static int hexValue(char c) {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    }

    static uint32_t parseHexCodeUnit(const std::string& s, size_t& o) {
        if (o + 4 > s.size()) {
            throw std::runtime_error("LFJSON: Incomplete Unicode escape");
        }

        uint32_t value = 0;
        for (size_t i = 0; i < 4; ++i) {
            const int digit = hexValue(s[o + i]);
            if (digit < 0) {
                throw std::runtime_error("LFJSON: Invalid Unicode escape");
            }
            value = (value << 4) | static_cast<uint32_t>(digit);
        }
        o += 4;
        return value;
    }

    static void appendCodepoint(std::string& out, uint32_t codepoint) {
        if (codepoint <= 0x7F) {
            out.push_back(static_cast<char>(codepoint));
        } else if (codepoint <= 0x7FF) {
            out.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
            out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        } else if (codepoint <= 0xFFFF) {
            out.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
            out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        } else {
            out.push_back(static_cast<char>(0xF0 | (codepoint >> 18)));
            out.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        }
    }

    static LFJSONValue parseString(const std::string& s, size_t& o) {
        if (o >= s.size() || s[o] != '"') {
            throw std::runtime_error("LFJSON: Expected string");
        }
        o++; // skip "
        std::string res;
        // 预分配，避免频繁扩容
        res.reserve(16);
        bool terminated = false;
        while (o < s.size()) {
            char c = s[o++];
            if (c == '"') {
                terminated = true;
                break;
            }
            if (c == '\\') {
                if (o >= s.size()) {
                    throw std::runtime_error("LFJSON: Unexpected end in escape sequence");
                }
                const char escaped = s[o++];
                switch (escaped) {
                    case '"': res.push_back('"'); break;
                    case '\\': res.push_back('\\'); break;
                    case '/': res.push_back('/'); break;
                    case 'b': res.push_back('\b'); break;
                    case 'f': res.push_back('\f'); break;
                    case 'n': res.push_back('\n'); break;
                    case 'r': res.push_back('\r'); break;
                    case 't': res.push_back('\t'); break;
                    case 'u': {
                        uint32_t codepoint = parseHexCodeUnit(s, o);
                        if (codepoint >= 0xD800 && codepoint <= 0xDBFF) {
                            if (o + 2 > s.size() || s[o] != '\\' || s[o + 1] != 'u') {
                                throw std::runtime_error("LFJSON: Missing low surrogate");
                            }
                            o += 2;
                            const uint32_t low = parseHexCodeUnit(s, o);
                            if (low < 0xDC00 || low > 0xDFFF) {
                                throw std::runtime_error("LFJSON: Invalid low surrogate");
                            }
                            codepoint = 0x10000 +
                                        ((codepoint - 0xD800) << 10) +
                                        (low - 0xDC00);
                        } else if (codepoint >= 0xDC00 && codepoint <= 0xDFFF) {
                            throw std::runtime_error("LFJSON: Unexpected low surrogate");
                        }
                        appendCodepoint(res, codepoint);
                        break;
                    }
                    default:
                        throw std::runtime_error("LFJSON: Invalid escape sequence");
                }
            } else if (static_cast<unsigned char>(c) < 0x20) {
                throw std::runtime_error("LFJSON: Unescaped control character in string");
            } else {
                res.push_back(c);
            }
        }
        if (!terminated) {
            throw std::runtime_error("LFJSON: Unterminated string");
        }
        return res;
    }

    static LFJSONValue parseNumber(const std::string& s, size_t& o) {
        const size_t start = o;
        if (s[o] == '-') {
            ++o;
        }
        if (o >= s.size()) {
            throw std::runtime_error("LFJSON: Incomplete number");
        }

        if (s[o] == '0') {
            ++o;
        } else if (s[o] >= '1' && s[o] <= '9') {
            do {
                ++o;
            } while (o < s.size() && s[o] >= '0' && s[o] <= '9');
        } else {
            throw std::runtime_error("LFJSON: Invalid number at index " + std::to_string(start));
        }

        if (o < s.size() && s[o] == '.') {
            ++o;
            if (o >= s.size() || s[o] < '0' || s[o] > '9') {
                throw std::runtime_error("LFJSON: Invalid fraction at index " + std::to_string(o));
            }
            while (o < s.size() && s[o] >= '0' && s[o] <= '9') {
                ++o;
            }
        }

        if (o < s.size() && (s[o] == 'e' || s[o] == 'E')) {
            ++o;
            if (o < s.size() && (s[o] == '+' || s[o] == '-')) {
                ++o;
            }
            if (o >= s.size() || s[o] < '0' || s[o] > '9') {
                throw std::runtime_error("LFJSON: Invalid exponent at index " + std::to_string(o));
            }
            while (o < s.size() && s[o] >= '0' && s[o] <= '9') {
                ++o;
            }
        }

        size_t cursor = start;
        bool negative = false;
        if (s[cursor] == '-') {
            negative = true;
            ++cursor;
        }

        long double value = 0.0L;
        bool hasNonZeroDigit = false;
        long long fractionalDigits = 0;
        while (cursor < o && s[cursor] >= '0' && s[cursor] <= '9') {
            const int digit = s[cursor] - '0';
            hasNonZeroDigit = hasNonZeroDigit || digit != 0;
            value = value * 10.0L + static_cast<long double>(digit);
            ++cursor;
        }
        if (cursor < o && s[cursor] == '.') {
            ++cursor;
            while (cursor < o && s[cursor] >= '0' && s[cursor] <= '9') {
                const int digit = s[cursor] - '0';
                hasNonZeroDigit = hasNonZeroDigit || digit != 0;
                value = value * 10.0L + static_cast<long double>(digit);
                ++fractionalDigits;
                ++cursor;
            }
        }
        if (cursor < o && (s[cursor] == 'e' || s[cursor] == 'E')) {
            ++cursor;
            bool negativeExponent = false;
            if (s[cursor] == '+' || s[cursor] == '-') {
                negativeExponent = s[cursor] == '-';
                ++cursor;
            }

            long long exponent = 0;
            while (cursor < o && s[cursor] >= '0' && s[cursor] <= '9') {
                exponent = std::min<long long>(10000, exponent * 10 + (s[cursor] - '0'));
                ++cursor;
            }
            if (negativeExponent) {
                exponent = -exponent;
            }
            fractionalDigits -= exponent;
        }

        if (fractionalDigits > 10000 || fractionalDigits < -10000) {
            throw std::runtime_error("LFJSON: Invalid number at index " + std::to_string(start));
        }
        if (value != 0.0L && fractionalDigits != 0) {
            value *= std::pow(10.0L, -static_cast<long double>(fractionalDigits));
        }
        if (negative) {
            value = -value;
        }

        const double result = static_cast<double>(value);
        if (!std::isfinite(result) ||
            (!hasNonZeroDigit && result != 0.0) ||
            (hasNonZeroDigit && result == 0.0)) {
            throw std::runtime_error("LFJSON: Number out of range at index " + std::to_string(start));
        }
        return result;
    }
};
