#pragma once

#include <atomic>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>
#include <condition_variable>

#include <ftxui/component/component_base.hpp>
#include <ftxui/dom/elements.hpp>

#include "../store/MessageStore.hpp"
#include "../ollama/OllamaClient.hpp"
#include "InputBuffer.hpp"
#include "../markdown/Markdown.hpp"
#include "../config/AppConfig.hpp"
#include "../tool/ToolManager.hpp"
#include "../session/SessionStore.hpp"

class App {
public:
    explicit App(AppConfig config = {});
    void Run();

private:
    ftxui::Element RenderHead();
    ftxui::Element RenderChat();
    ftxui::Element RenderMessage(const Message& message);
    ftxui::Element RenderMessageContent(const std::string& content);
    ftxui::Element RenderAgentState();
    ftxui::Element RenderInput();
    ftxui::Element RenderFooter();
    ftxui::Component BuildAndHandle();

    AppConfig config_;

    ToolManager tools_;

    int input_tokens_ = 0;
    int output_tokens_ = 0;
    std::string model_ = "gemma4:e4b";

    ftxui::Component renderer_;
    MessageStore messages_;
    OllamaClient ollama_;
    InputBuffer input_;
    std::atomic<bool> cancel_request_ = false;

    double chat_scroll_offset_ = 1.0;

    bool bracketed_paste_active_ = false;

    struct AppEvent {
        enum class Type {
            Token,
            Done,
            Error,
            AiState
        };

        Type type;
        std::string text;
        int input_tokens = 0;
        int output_tokens = 0;
    };

    std::atomic<bool> is_thinking_ = false;
    std::thread worker_;
    std::mutex event_mutex_;
    std::queue<AppEvent> events_;

    void PushEvent(AppEvent event);
    void ProcessEvents();

    std::atomic<bool> running_ = true;
    std::thread spinner_thread_;
    int spinner_frame_ = 0;

    std::vector<std::string> spinner_ = {
        "⠋", "⠙", "⠹", "⠸",
        "⠼", "⠴", "⠦", "⠧",
        "⠇", "⠏"
    };

    Markdown markdown_;

    std::string agent_state_;

    // Approval UI
    ftxui::Element RenderApprovalDialog();

    PermissionDecision RequestApproval(const ToolCall& call);
    void AnswerApproval(PermissionDecision decisiont);
    bool ApprovalPending() const;

    mutable std::mutex approval_mutex_;
    std::condition_variable approval_cv_;

    bool approval_pending_ = false;
    bool approval_answered_ = false;
    PermissionDecision approval_decision_ = PermissionDecision::Deny;

    std::string approval_tool_;
    std::string approval_arguments_;

    // SlashCommands
    ftxui::Element RenderSlashCommandDialog();
    bool HandleSlashCommand();
    bool slash_command_active_ = false;

    // Available local LLMs
    ftxui::Element RenderModelListDialog();
    bool model_list_modal_open_ = false;
    std::vector<std::string> available_models_;
    std::string model_list_error_;

    // Dateiauswahl (Picker und @)
    struct FilePickerEntry
    {
        std::string name;
        bool is_directory = false;
        bool is_parent = false;
    };
    void RefreshFilePickerEntries();
    ftxui::Element RenderFilePickerDialog();
    bool file_picker_open_ = false;
    int file_picker_selected_ = 0;
    std::filesystem::path file_picker_current_dir_ = config_.workspace_root;
    std::vector<FilePickerEntry> file_picker_entries_;
    int file_picker_scroll_offset_ = 0;
    int file_picker_visible_rows_ = 12;

    std::vector<std::filesystem::path> ExtractAttachmentPaths(const std::string& text) const;
    std::string BuildPromptWithAttachments(const std::string& user_input) const;
    std::string BuildAttachmentInfoMessage(const std::vector<std::filesystem::path>& paths) const;

    // SessionStore
    SessionStore session_store_;
    void LoadSession();
    void SaveSession();
};

