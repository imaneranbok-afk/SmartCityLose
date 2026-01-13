/* minimal nlohmann/json single-header stub for parsing small config files
   This is a compact wrapper using the real nlohmann::json API but
   implemented with a tiny, permissive parser to avoid adding large files.
   It supports the subset needed: parse from string/file, object/array access,
   get<string>(), get<double>(), get<int>(), contains(), and operator[] access.
   NOTE: This is intentionally minimal and not a full replacement.
*/

#ifndef THIRD_PARTY_JSON_HPP
#define THIRD_PARTY_JSON_HPP

#include <string>
#include <map>
#include <vector>
#include <variant>
#include <sstream>
#include <fstream>
#include <cctype>
#include <stdexcept>

namespace nlohmann {

class json {
public:
    using object_t = std::map<std::string, json>;
    using array_t = std::vector<json>;
    using value_t = std::variant<std::nullptr_t, bool, double, std::string, object_t, array_t>;

    value_t value;

    json() : value(nullptr) {}
    json(std::nullptr_t) : value(nullptr) {}
    json(bool b) : value(b) {}
    json(double d) : value(d) {}
    json(int i) : value((double)i) {}
    json(const std::string& s) : value(s) {}
    json(const char* s) : value(std::string(s)) {}
    json(const object_t& o) : value(o) {}
    json(const array_t& a) : value(a) {}

    static json parse(const std::string& s) {
        size_t idx = 0;
        return parse_internal(s, idx);
    }

    static json parse_file(const std::string& path) {
        std::ifstream ifs(path);
        if (!ifs) throw std::runtime_error("Cannot open file: " + path);
        std::stringstream ss; ss << ifs.rdbuf();
        return parse(ss.str());
    }

    bool is_object() const { return std::holds_alternative<object_t>(value); }
    bool is_array() const { return std::holds_alternative<array_t>(value); }
    bool is_string() const { return std::holds_alternative<std::string>(value); }
    bool is_number() const { return std::holds_alternative<double>(value); }
    bool is_boolean() const { return std::holds_alternative<bool>(value); }
    bool is_null() const { return std::holds_alternative<std::nullptr_t>(value); }
    bool is_primitive() const { return is_null() || is_boolean() || is_number() || is_string(); }

    const json& operator[](const std::string& key) const {
        const auto& obj = std::get<object_t>(value);
        auto it = obj.find(key);
        if (it == obj.end()) throw std::out_of_range("key not found: " + key);
        return it->second;
    }
    json& operator[](const std::string& key) {
        if (!is_object()) value = object_t();
        return std::get<object_t>(value)[key];
    }
    json& operator[](size_t idx) {
        if (!is_array()) value = array_t();
        auto& arr = std::get<array_t>(value);
        if (idx >= arr.size()) throw std::out_of_range("index out of range");
        return arr[idx];
    }
    const json& operator[](size_t idx) const {
        const auto& arr = std::get<array_t>(value);
        if (idx >= arr.size()) throw std::out_of_range("index out of range");
        return arr[idx];
    }
    size_t size() const {
        if (is_array()) return std::get<array_t>(value).size();
        if (is_object()) return std::get<object_t>(value).size();
        return 0;
    }

    template<typename T>
    T get() const;

    std::string dump() const {
        if (is_null()) return "null";
        if (is_boolean()) return std::get<bool>(value) ? "true" : "false";
        if (is_number()) return std::to_string(std::get<double>(value));
        if (is_string()) return "\"" + std::get<std::string>(value) + "\"";
        if (is_object()) return "{...}";
        if (is_array()) return "[...]";
        return "";
    }

    bool contains(const std::string& key) const {
        if (!is_object()) return false;
        const auto& obj = std::get<object_t>(value);
        return obj.find(key) != obj.end();
    }

private:
    static void skip_ws(const std::string& s, size_t& i) {
        while (i < s.size() && std::isspace((unsigned char)s[i])) ++i;
    }

    static std::string parse_string(const std::string& s, size_t& i) {
        if (s[i] != '"') throw std::runtime_error("expected string");
        ++i;
        std::string out;
        while (i < s.size() && s[i] != '"') {
            if (s[i] == '\\') {
                ++i;
                if (i >= s.size()) break;
                out.push_back(s[i]);
            } else out.push_back(s[i]);
            ++i;
        }
        if (i >= s.size() || s[i] != '"') throw std::runtime_error("unterminated string");
        ++i;
        return out;
    }

    static double parse_number(const std::string& s, size_t& i) {
        size_t start = i;
        if (s[i] == '-') ++i;
        while (i < s.size() && (std::isdigit((unsigned char)s[i]) || s[i]=='.' || s[i]=='e' || s[i]=='E' || s[i]=='+' || s[i]=='-')) ++i;
        double v = std::stod(s.substr(start, i-start));
        return v;
    }

    static json parse_internal(const std::string& s, size_t& i) {
        skip_ws(s,i);
        if (i >= s.size()) return json();
        if (s[i] == '{') {
            ++i; skip_ws(s,i);
            object_t obj;
            while (i < s.size() && s[i] != '}') {
                skip_ws(s,i);
                std::string key = parse_string(s,i);
                skip_ws(s,i);
                if (s[i] != ':') throw std::runtime_error("expected :");
                ++i; skip_ws(s,i);
                json val = parse_internal(s,i);
                obj[key] = val;
                skip_ws(s,i);
                if (s[i] == ',') { ++i; skip_ws(s,i); }
                else break;
            }
            if (i >= s.size() || s[i] != '}') throw std::runtime_error("expected }");
            ++i;
            return json(obj);
        } else if (s[i] == '[') {
            ++i; skip_ws(s,i);
            array_t arr;
            while (i < s.size() && s[i] != ']') {
                json val = parse_internal(s,i);
                arr.push_back(val);
                skip_ws(s,i);
                if (s[i] == ',') { ++i; skip_ws(s,i); }
                else break;
            }
            if (i >= s.size() || s[i] != ']') throw std::runtime_error("expected ]");
            ++i;
            return json(arr);
        } else if (s[i] == '"') {
            std::string str = parse_string(s,i);
            return json(str);
        } else if (std::isdigit((unsigned char)s[i]) || s[i] == '-') {
            double num = parse_number(s,i);
            return json(num);
        } else if (s.compare(i,4,"null") == 0) { i += 4; return json(nullptr); }
        else if (s.compare(i,4,"true") == 0) { i += 4; return json(true); }
        else if (s.compare(i,5,"false") == 0) { i += 5; return json(false); }
        throw std::runtime_error(std::string("Unexpected token: ") + s[i]);
    }
};

// template specializations
template<> inline int json::get<int>() const { if (is_number()) return (int)std::get<double>(value); throw std::runtime_error("not a number"); }
template<> inline double json::get<double>() const { if (is_number()) return std::get<double>(value); throw std::runtime_error("not a number"); }
template<> inline std::string json::get<std::string>() const { if (is_string()) return std::get<std::string>(value); throw std::runtime_error("not a string"); }
template<> inline bool json::get<bool>() const { if (is_boolean()) return std::get<bool>(value); throw std::runtime_error("not a bool"); }

} // namespace nlohmann

#endif
