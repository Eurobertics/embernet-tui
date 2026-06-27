#pragma once

#include <optional>
#include <string>
#include <filesystem>
#include "../tool/Permission.hpp"

class AppConfig {
public:
    AppConfig();

    void SetConfigFile(const std::string& path);
    std::string GetConfigFile() const;

    // Default Configurations
    std::filesystem::path workspace_root = std::filesystem::current_path();
    std::string session_name;
    PermissionMode permission_mode = PermissionMode::WorkspaceWrite;
    std::optional<std::filesystem::path> prompt_file = std::nullopt;
    bool session_enabled = true;
    std::string llm_backend = "ollama";
    std::string base_api_url = "http://127.0.0.1:11434";
    std::string default_model = "gemma4:e4b";

private:
    std::string config_file_;

    bool ConfigExists(const std::filesystem::path& path) const;
    void CreateDefaultConfig(const std::filesystem::path& path);
    void LoadConfig();
};
