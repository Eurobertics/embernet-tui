#pragma once

enum class PermissionMode {
    ReadOnly,
    WorkspaceWrite,
    Dangerous
};

enum class ToolPermission {
    Read,
    Write,
    Modify,
    Execute,
    Dangerous
};

enum class PermissionDecision {
    Allow,
    Deny,
    Ask
};

