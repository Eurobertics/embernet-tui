#pragma once

#include <filesystem>
#include <optional>
#include "../tool/Permission.hpp"

struct AppConfig {
    PermissionMode permission_mode = PermissionMode::WorkspaceWrite;
    std::filesystem::path workspace_root = std::filesystem::current_path();

    std::optional<std::filesystem::path> prompt_file = std::nullopt;

    bool session_enabled = true;
    std::string session_name;
};

