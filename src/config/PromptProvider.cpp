#include "PromptProvider.hpp"
#include "DefaultPrompt.hpp"

#include <cstdlib>
#include <fstream>
#include <sstream>

PromptProvider::PromptProvider(
    std::optional<std::filesystem::path> prompt_file
)
    : prompt_file_(std::move(prompt_file))
{
}

std::string PromptProvider::GetSystemPrompt() const
{
    if (prompt_file_.has_value()) {
        auto prompt = TryLoadPrompt(prompt_file_.value());

        if (prompt.has_value()) {
            return prompt.value();
        }
    }

    auto config_prompt = TryLoadPrompt(GetConfigPromptPath());

    if (config_prompt.has_value()) {
        return config_prompt.value();
    }

    return DEFAULT_SYSTEM_PROMPT;
}

std::filesystem::path PromptProvider::GetConfigPromptPath() const
{
    const char* home = std::getenv("HOME");

    if (home == nullptr) {
        return {};
    }

    return std::filesystem::path(home)
        / ".config"
        / "embernet-tui"
        / "system_prompt.md";
}

std::string PromptProvider::LoadFile(
    const std::filesystem::path& path
) const
{
    std::ifstream file(path);

    if (!file.is_open()) {
        return {};
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    return buffer.str();
}

std::optional<std::string> PromptProvider::TryLoadPrompt(
    const std::filesystem::path& path
) const
{
    if (path.empty()) {
        return std::nullopt;
    }

    if (!std::filesystem::exists(path)) {
        return std::nullopt;
    }

    auto prompt = LoadFile(path);

    if (prompt.empty()) {
        return std::nullopt;
    }

    return prompt;
}

