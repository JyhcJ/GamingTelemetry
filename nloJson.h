#include <nlohmann/json.hpp>
#include <string>
#include <sstream>
#include <stdexcept>

using json = nlohmann::json;

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
        // ����ֱ��ת��ΪĿ������
        return current->get<T>();
    }
    catch (const json::type_error&) {
        // ʧ�ܺ����ֶ����⴦��
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
                return current->dump();  // �Զ�����ת�ַ���
            }
            else if (current->is_boolean()) {
                return current->get<bool>() ? "true" : "false";
            }
        }

        return defaultVal;
    }
}

