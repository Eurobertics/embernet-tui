#include "ToolFormat.hpp"

std::string ShortenText(const std::string& text, std::size_t max_length)
{
    if (text.size() <= max_length) {
        return text;
    }

    if (max_length <= 3) {
        return text.substr(0, max_length);
    }

    return text.substr(0, max_length - 3) + "...";
}

std::string FormatToolArguments(
    const nlohmann::json& arguments,
    std::size_t max_value_length,
    std::string separator
) {
    std::string result;
    bool first = true;

    for (const auto& [key, value] : arguments.items()) {
        if (!first) {
            result += separator;
        }

        result += key + "|";

        std::string value_text;

        if (value.is_string()) {
            value_text = value.get<std::string>();
        } else if (value.is_array()) {
            value_text = "[";

            bool first_array_value = true;
            for (const auto& item : value) {
                if (!first_array_value) {
                    value_text += ", ";
                }

                if (item.is_string()) {
                    value_text += item.get<std::string>();
                } else {
                    value_text += item.dump();
                }

                first_array_value = false;
            }

            value_text += "]";
        } else {
            value_text = value.dump();
        }

        result += ShortenText(value_text, max_value_length);

        first = false;
    }

    return result;
}

