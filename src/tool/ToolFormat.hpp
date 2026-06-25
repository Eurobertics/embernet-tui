#pragma once

#include <cstddef>
#include <string>

#include <nlohmann/json.hpp>

std::string ShortenText(const std::string& text, std::size_t max_length);

std::string FormatToolArguments(
    const nlohmann::json& arguments,
    std::size_t max_value_length = 120,
    std::string separator = ", "
);

