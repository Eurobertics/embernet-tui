#include "ToolManager.hpp"
#include "Permission.hpp"
#include "Tools.hpp"
#include <algorithm>
#include <sstream>
#include <string>
#include <fstream>
#include <filesystem>
#include <unistd.h>
#include <sys/wait.h>
#include <array>
#include <vector>
#include <cstring>

ToolManager::ToolManager(const AppConfig& config)
    : config_(config)
{
}

void ToolManager::SetApprovalCallback(ApprovalCallback callback)
{
    approval_callback_ = callback;
}

ToolPermission ToolManager::RequiredPermission(const ToolCall& call) const
{
    if (call.name == "read_file") {
        return ToolPermission::Read;
    }

    if (call.name == "read_directory") {
        return ToolPermission::Read;
    }

    if (call.name == "write_file") {
        return ToolPermission::Write;
    }

    if (call.name == "create_directory") {
        return ToolPermission::Write;
    }

    if (call.name == "replace_text") {
        return ToolPermission::Modify;
    }

    if (call.name == "delete_file") {
        return ToolPermission::Dangerous;
    }

    if (call.name == "delete_directory") {
        return ToolPermission::Dangerous;
    }

    if (call.name == "exec_command") {
        return ToolPermission::Execute;
    }

    return ToolPermission::Dangerous;
}

PermissionDecision ToolManager::CheckPermission(const ToolCall& call) const
{
    ToolPermission required = RequiredPermission(call);

    if (config_.permission_mode == PermissionMode::ReadOnly) {
        if (required == ToolPermission::Read) {
            return PermissionDecision::Allow;
        }

        return PermissionDecision::Deny;
    }

    if (config_.permission_mode == PermissionMode::WorkspaceWrite) {
        if (required == ToolPermission::Read) {
            return PermissionDecision::Allow;
        }

        if (required == ToolPermission::Write) {
            return PermissionDecision::Ask;
        }

        if (required == ToolPermission::Modify) {
            return PermissionDecision::Ask;
        }

        if (required == ToolPermission::Dangerous) {
            return PermissionDecision::Ask;
        }

        if (required == ToolPermission::Execute) {
            return PermissionDecision::Ask;
        }

        return PermissionDecision::Deny;
    }

    if (config_.permission_mode == PermissionMode::Dangerous) {
        return PermissionDecision::Allow;
    }

    return PermissionDecision::Deny;
}

ToolResult ToolManager::Execute(const ToolCall& call)
{
    auto decision = CheckPermission(call);
    if (decision == PermissionDecision::Deny) {
        return {
            call.name,
            "Permission denied for tool: " + call.name,
            false
        };
    }

    if (decision == PermissionDecision::Ask) {
        if (!approval_callback_) {
            return {
                call.name,
                "Approval required but no approval callback is registered.",
                false
            };
        }

        decision = approval_callback_(call);

        if (decision != PermissionDecision::Allow) {
            return {
                call.name,
                "User denied tool execution: " + call.name,
                false
            };
        }
    }

    if (call.name == "read_file") {
        return ReadFile(call);
    }

    if (call.name == "read_directory") {
        return ReadDirectory(call);
    }

    if (call.name == "write_file") {
        return WriteFile(call);
    }

    if (call.name == "create_directory") {
        return CreateDirectory(call);
    }

    if (call.name == "replace_text") {
        return ReplaceText(call);
    }

    if (call.name == "delete_file") {
        return DeleteFile(call);
    }

    if (call.name == "delete_directory") {
        return DeleteDirectory(call);
    }

    if (call.name == "exec_command") {
        return ExecCommand(call);
    }

    return {
        call.name,
        "Unknown tool: " + call.name,
        false
    };
};

ToolResult ToolManager::ReadFile(const ToolCall& call)
{
    std::string raw_path = call.arguments.value("path", "");
    auto path = ResolveWorkspacePath(raw_path);
    if (!path) {
        return {
            call.name,
            "Path rejected: outside workspace or invalid path: " + raw_path,
            false
        };
    }

    std::ifstream file(*path);

    if(!file.is_open()) {
        return {
            call.name,
            "Could not open file: " + path->string(),
            false
        };
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    return {
        call.name,
        buffer.str(),
        true
    };
}

ToolResult ToolManager::ReadDirectory(const ToolCall& call)
{
    namespace fs = std::filesystem;

    std::string raw_path = call.arguments.value("path", "");
    auto path = ResolveWorkspacePath(raw_path);
    if (!path) {
        return {
            call.name,
            "Path rejected: outside workspace or invalid path: " + raw_path,
            false
        };
    }
    
    std::string files;
    std::vector<fs::path> files_vector;
    
    try {
        for (const auto& entry : fs::directory_iterator(*path)) {
            files_vector.push_back(entry.path().filename());
        }
    } catch (const fs::filesystem_error& e) {
        return {
            call.name,
            e.what(),
            false
        };
    }

    std::sort(files_vector.begin(), files_vector.end());

    for (size_t i = 0; i < files_vector.size(); ++i) {
        files += files_vector[i].string();
        if (i < files_vector.size() - 1) {
            files += "\n";
        }
    }

    return {
        call.name,
        files,
        true
    };
}

ToolResult ToolManager::WriteFile(const ToolCall& call)
{
    std::string raw_path = call.arguments.value("path", "");
    auto path = ResolveWorkspacePath(raw_path);
    if (!path) {
        return {
            call.name,
            "Path rejected: outside workspace or invalid path: " + raw_path,
            false
        };
    }

    std::string content = call.arguments.value("content", "");
    std::ofstream file(path->string());

    if (!file.is_open()) {
        return {
            call.name,
            "Cannot create file: " + path->string(),
            false
        };
    }

    file << content;
    file.close();

    return {
        call.name,
        "File was successfully written: " + path->string(),
        true
    };
}

ToolResult ToolManager::CreateDirectory(const ToolCall& call)
{
    std::string raw_path = call.arguments.value("path", "");
    auto path = ResolveWorkspacePath(raw_path);
    if (!path) {
        return {
            call.name,
            "Path rejected: outside workspace or invalid path: " + raw_path,
            false
        };
    }

    try {
        if (std::filesystem::create_directories(*path)) {
            return { call.name, "Directory created: " + path->string(), true };
        }

        return { call.name, "Directory already exists: " + path->string(), true };
    } catch (const std::filesystem::filesystem_error& e) {
        return { call.name, e.what(), false };
    }
}
ToolResult ToolManager::ReplaceText(const ToolCall& call)
{
    std::string raw_path = call.arguments.value("path", "");
    auto path = ResolveWorkspacePath(raw_path);

    if (!path) {
        return {
            call.name,
            "Path rejected: outside workspace or invalid path: " + raw_path,
            false
        };
    }

    std::string old_text = call.arguments.value("old_text", "");
    std::string new_text = call.arguments.value("new_text", "");

    if (old_text.empty()) {
        return {
            call.name,
            "old_text must not be empty.",
            false
        };
    }

    std::ifstream file(*path);

    if (!file.is_open()) {
        return {
            call.name,
            "Could not open file: " + path->string(),
            false
        };
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    size_t first_pos = content.find(old_text);

    if (first_pos == std::string::npos) {
        return {
            call.name,
            "Text not found in file: " + path->string(),
            false
        };
    }

    size_t second_pos = content.find(old_text, first_pos + old_text.size());

    if (second_pos != std::string::npos) {
        return {
            call.name,
            "Text occurs multiple times. Refusing ambiguous replace.",
            false
        };
    }

    content.replace(first_pos, old_text.size(), new_text);

    std::ofstream out(*path);

    if (!out.is_open()) {
        return {
            call.name,
            "Could not write file: " + path->string(),
            false
        };
    }

    out << content;

    return {
        call.name,
        "Text replaced in file: " + path->string(),
        true
    };
}

ToolResult ToolManager::DeleteFile(const ToolCall& call)
{
    std::string raw_path = call.arguments.value("path", "");
    auto path = ResolveWorkspacePath(raw_path);

    if (!path) {
        return {
            call.name,
            "Path rejected: outside workspace or invalid path: " + raw_path,
            false
        };
    }

    if (!std::filesystem::exists(*path)) {
        return {
            call.name,
            "File does not exist: " + path->string(),
            false
        };
    }

    if (!std::filesystem::is_regular_file(*path)) {
        return {
            call.name,
            "Path is not a regular file: " + path->string(),
            false
        };
    }

    std::error_code ec;
    bool removed = std::filesystem::remove(*path, ec);

    if (ec) {
        return {
            call.name,
            "Could not delete file: " + ec.message(),
            false
        };
    }

    if (!removed) {
        return {
            call.name,
            "File was not deleted: " + path->string(),
            false
        };
    }

    return {
        call.name,
        "File deleted: " + path->string(),
        true
    };
}

ToolResult ToolManager::DeleteDirectory(const ToolCall& call)
{
    std::string raw_path = call.arguments.value("path", "");
    auto path = ResolveWorkspacePath(raw_path);

    if (!path) {
        return {
            call.name,
            "Path rejected: outside workspace or invalid path: " + raw_path,
            false
        };
    }

    if (!std::filesystem::exists(*path)) {
        return {
            call.name,
            "Directory does not exist: " + path->string(),
            false
        };
    }

    if (!std::filesystem::is_directory(*path)) {
        return {
            call.name,
            "Path is not a directory: " + path->string(),
            false
        };
    }

    if (!std::filesystem::is_empty(*path)) {
        return {
            call.name,
            "Directory is not empty: " + path->string(),
            false
        };
    }

    std::error_code ec;
    bool removed = std::filesystem::remove(*path, ec);

    if (ec) {
        return {
            call.name,
            "Could not delete directory: " + ec.message(),
            false
        };
    }

    if (!removed) {
        return {
            call.name,
            "Directory is not empty or could not be removed: "
                + path->string(),
            false
        };
    }

    return {
        call.name,
        "Directory deleted: " + path->string(),
        true
    };
}

ToolResult ToolManager::ExecCommand(const ToolCall& call)
{
    std::string command = call.arguments.value("command", "");

    std::vector<std::string> args;
    if (call.arguments.contains("args") && call.arguments["args"].is_array()) {
        args = call.arguments["args"].get<std::vector<std::string>>();
    }

    if (command.empty()) {
        return { call.name, "No command provided.", false };
    }

    int pipe_fd[2];

    if (pipe(pipe_fd) == -1) {
        return { call.name, "Failed to create pipe.", false };
    }

    pid_t pid = fork();

    if (pid == -1) {
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        return { call.name, "Failed to fork process.", false };
    }

    if (pid == 0) {
        // Child process

        close(pipe_fd[0]);

        dup2(pipe_fd[1], STDOUT_FILENO);
        dup2(pipe_fd[1], STDERR_FILENO);

        close(pipe_fd[1]);

        std::vector<char*> argv;
        argv.push_back(const_cast<char*>(command.c_str()));

        for (auto& arg : args) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }

        argv.push_back(nullptr);

        execvp(command.c_str(), argv.data());

        _exit(127);
    }

    // Parent process

    close(pipe_fd[1]);

    std::string output;
    std::array<char, 256> buffer;

    const size_t max_output = 20'000;

    ssize_t bytes_read;
    while ((bytes_read = read(pipe_fd[0], buffer.data(), buffer.size())) > 0) {
        output.append(buffer.data(), bytes_read);

        if (output.size() > max_output) {
            output += "\n[Output truncated]";
            break;
        }
    }

    close(pipe_fd[0]);

    int status = 0;
    waitpid(pid, &status, 0);

    int exit_code = -1;

    if (WIFEXITED(status)) {
        exit_code = WEXITSTATUS(status);
    }

    if (output.empty()) {
        output = "[No output]";
    }

    output += "\nExit code: " + std::to_string(exit_code);

    return {
        call.name,
        output,
        exit_code == 0
    };
}

std::optional<std::filesystem::path>
ToolManager::ResolveWorkspacePath(const std::string& raw_path) const
{
    namespace fs = std::filesystem;

    if (raw_path.empty()) {
        return std::nullopt;
    }

    try {
        fs::path workspace = fs::weakly_canonical(config_.workspace_root);
        fs::path target = fs::path(raw_path);

        if (target.is_relative()) {
            target = workspace / target;
        }

        target = fs::weakly_canonical(target);

        auto workspace_str = workspace.string();
        auto target_str = target.string();

        if (target_str == workspace_str) {
            return target;
        }

        if (target_str.rfind(workspace_str + "/", 0) == 0) {
            return target;
        }

        return std::nullopt;
    } catch (...) {
        return std::nullopt;
    }
}

