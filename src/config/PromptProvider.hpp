#pragma once

#include <filesystem>
#include <optional>
#include <string>

class PromptProvider {
public:
    explicit PromptProvider(
        std::optional<std::filesystem::path> prompt_file = std::nullopt
    );

    std::string GetSystemPrompt() const;

private:
    std::optional<std::filesystem::path> prompt_file_;

    std::filesystem::path GetConfigPromptPath() const;
    std::string LoadFile(const std::filesystem::path& path) const;
    std::optional<std::string> TryLoadPrompt(
        const std::filesystem::path& path
    ) const;
};

