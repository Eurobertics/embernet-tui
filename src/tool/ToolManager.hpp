#pragma once

#include "Permission.hpp"
#include "Tools.hpp"
#include "../config/AppConfig.hpp"
#include <filesystem>
#include <functional>
#include <optional>

class ToolManager {
    public:
        using ApprovalCallback = std::function<PermissionDecision(const ToolCall&)>;
        explicit ToolManager(const AppConfig& config);
        void SetApprovalCallback(ApprovalCallback callback);
        ToolResult Execute(const ToolCall& call);

    private:
        const AppConfig& config_;
        ApprovalCallback approval_callback_;
        PermissionDecision CheckPermission(const ToolCall& call) const;
        ToolPermission RequiredPermission(const ToolCall& call) const;
        ToolResult ReadFile(const ToolCall& call);
        ToolResult ReadDirectory(const ToolCall& call);
        ToolResult WriteFile(const ToolCall& call);
        ToolResult CreateDirectory(const ToolCall& call);
        ToolResult ReplaceText(const ToolCall& call);
        ToolResult DeleteFile(const ToolCall& call);
        ToolResult DeleteDirectory(const ToolCall& call);
        ToolResult ExecCommand(const ToolCall& call);
        std::optional<std::filesystem::path> ResolveWorkspacePath(const std::string& raw_path) const;
};

