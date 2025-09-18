#include <nlohmann/json.hpp>
#include <string>
#include <sstream>

#include <variant>
#include <stdexcept>
#include <vector>


using json = nlohmann::json;

// 定义路径项：可以是字符串（对象键）或整数（数组索引）

struct PathItem {
    enum Type { Key, Index } type;
    std::string key;
    int index;

    static PathItem makeKey(const std::string& k) {
        return { Key, k, 0 };
    }

    static PathItem makeIndex(int idx) {
        return { Index, "", idx };
    }
};

template<typename T>
T getNestedValue(const json& j, const std::initializer_list<std::string>& keys, const T& defaultVal) {
    const json* current = &j;
    for (const auto& key : keys) {
        if (!current->contains(key)) {
            return defaultVal;
        }

        const json& next = (*current)[key];
        if (next.is_null()) {
            return defaultVal;
        }

        current = &next;
    }

    try {
        // 尝试直接转换为目标类型
        return current->get<T>();
    }
    catch (const json::type_error&) {
        // 失败后尝试手动特殊处理
        if constexpr (std::is_same_v<T, int>) {
            if (current->is_string()) {
                try {
                    return static_cast<int>(std::stoi(current->get<std::string>()));
                }
                catch (...) {
                    return defaultVal;
                }
            }
            else if (current->is_number_float()) {
                return static_cast<int>(current->get<double>());
            }
        }
        else if constexpr (std::is_same_v<T, std::string>) {
            if (current->is_number()) {
                return current->dump();  // 自动数字转字符串
            }
            else if (current->is_boolean()) {
                return current->get<bool>() ? "true" : "false";
            }
        }

        return defaultVal;
    }
}

template<typename T>
T getNestedValuePlus(const json& j, const std::vector<PathItem>& path, const T& defaultVal) {
    const json* current = &j;

    for (const auto& item : path) {
        if (item.type == PathItem::Key) {
            if (!current->contains(item.key) || (*current)[item.key].is_null()) {
                return defaultVal;
            }
            current = &((*current)[item.key]);
        }
        else if (item.type == PathItem::Index) {
            if (!current->is_array() || item.index < 0 || item.index >= static_cast<int>(current->size())) {
                return defaultVal;
            }
            current = &((*current)[item.index]);
        }
    }

    try {
        return current->get<T>();
    }
    catch (const json::type_error&) {
        // 类型转换失败时的容错处理
        if constexpr (std::is_same_v<T, int>) {
            if (current->is_string()) {
                try {
                    return static_cast<int>(std::stoi(current->get<std::string>()));
                }
                catch (...) { return defaultVal; }
            }
            else if (current->is_number_float()) {
                return static_cast<int>(current->get<double>());
            }
        }
        else if constexpr (std::is_same_v<T, std::string>) {
            if (current->is_number()) {
                return current->dump();
            }
            else if (current->is_boolean()) {
                return current->get<bool>() ? "true" : "false";
            }
        }
        return defaultVal;
    }
}
