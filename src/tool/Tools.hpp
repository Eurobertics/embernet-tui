#pragma once

#include <string>
#include <nlohmann/json.hpp>

struct ToolCall {
    std::string name;
    nlohmann::json arguments;
};

struct ToolResult {
    std::string name;
    std::string content;
    bool success = true;
};

