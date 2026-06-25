#include "App.hpp"
#include "../tool/ToolFormat.hpp"
#include "SlashCommand.hpp"
#include "TerminalModeGuard.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <ftxui/dom/deprecated.hpp>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/screen/terminal.hpp>

#include <markdown/parser.hpp>
#include <markdown/viewer.hpp>

App::App(AppConfig config)
    : config_(std::move(config)),
    tools_(config_),
    ollama_(tools_, config_)
{
    tools_.SetApprovalCallback([this](const ToolCall& call) {
        return RequestApproval(call);
    });
}

std::string role_prefix(const Message& message) {
    if (message.role == "user") {
        return "Du: ";
    }

    if (message.role == "assistant") {
        return "AI: ";
    }

    return message.role + ": ";
}

void App::Run() {
    using namespace ftxui;

    TerminalModeGuard bracketed_paste(
        "\x1b[?2004h",
        "\x1b[?2004l"
    );

    LoadSession();

    auto screen = ScreenInteractive::Fullscreen();

    running_ = true;
    spinner_thread_ = std::thread([this, &screen]() {
        while (running_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            if (is_thinking_) {
                spinner_frame_ = (spinner_frame_ + 1) % spinner_.size();
                screen.PostEvent(Event::Custom);
            }
        }
    });

    renderer_ = Renderer([&] {
        ProcessEvents();

        auto content = vbox({
            RenderHead(),
            RenderChat(),
            RenderAgentState(),
            RenderInput(),
            RenderFooter()
        }) | flex;

        if (ApprovalPending()) {
            return dbox({content, center(RenderApprovalDialog())});
        }

        if (file_picker_open_) {
            return dbox({content, center(RenderFilePickerDialog())});
        }

        if (model_list_modal_open_) {
            return dbox({content, center(RenderModelListDialog())});
        }

        if (slash_command_active_) {
            return dbox({content, center(RenderSlashCommandDialog())});
        }

        return content;
    });

    screen.Loop(BuildAndHandle());

    running_ = false;

    if (spinner_thread_.joinable()) {
        spinner_thread_.join();
    }

    if (worker_.joinable()) {
        worker_.join();
    }
}

ftxui::Element App::RenderHead() {
    using namespace ftxui;

    return hbox({
        text(" EmberNet-TUI "),
        filler(),
        text(" Ollama local "),
    })
    | border
    | color(Color::Green);
}

ftxui::Element App::RenderChat() {
    using namespace ftxui;

    Elements chat_elements;

    for (const auto& message : messages_.All()) {
        chat_elements.push_back(RenderMessage(message));
    }

    return vbox(chat_elements)
        | focusPositionRelative(0.0, chat_scroll_offset_)
        | vscroll_indicator
        | yframe
        | border
        | flex;
}

ftxui::Element App::RenderMessage(const Message& message) {
    using namespace ftxui;

    return vbox({
        text(role_prefix(message)) | bold,
        RenderMessageContent(message.content)
    });
}

ftxui::Element App::RenderMessageContent(const std::string& content) {
    markdown_.SetContent(content);
    return markdown_.Render();
}

ftxui::Element App::RenderAgentState() {
    using namespace ftxui;

    return vbox({
        paragraph(agent_state_)
    });
}

ftxui::Element App::RenderInput() {
    using namespace ftxui;

    int input_width = Terminal::Size().dimx - 4;

    if (input_width < 10) {
        input_width = 40;
    }

    auto visual_lines = input_.BuildVisualLines(input_width);
    auto cursor = input_.CursorVisualPosition(input_width);

    int visible_lines = 4;
    int scroll = std::max(0, cursor.line - visible_lines + 1);

    Elements input_elements;

    for (int i = 0; i < visible_lines; ++i) {
        int visual_index = scroll + i;

        if (visual_index < static_cast<int>(visual_lines.size())) {
            const std::string& line_text = visual_lines[visual_index].text;

            if (visual_index == cursor.line) {
                int column = std::min(
                        cursor.column,
                        static_cast<int>(line_text.size())
                        );

                std::string before = line_text.substr(0, column);
                std::string cursor_char = " ";
                std::string after;

                if (column < static_cast<int>(line_text.size())) {
                    cursor_char = line_text.substr(column, 1);
                    after = line_text.substr(column + 1);
                }

                input_elements.push_back(hbox({
                            text("> " + before),
                            text(cursor_char) | inverted,
                            text(after),
                            }));
            } else {
                input_elements.push_back(text("> " + line_text));
            }
        } else {
            input_elements.push_back(text("> "));
        }
    }

    return vbox(input_elements)
        | vscroll_indicator
        | frame
        | border
        | size(HEIGHT, EQUAL, 6);
}

ftxui::Element App::RenderFooter() {
    using namespace ftxui;

    std::string status = is_thinking_
        ? " THINKING " + spinner_[spinner_frame_] + " "
        : " READY ";

    return hbox({
            text(status),
            filler(),
            text(" Tokens: "),
            text(" IN: " + std::to_string(input_tokens_)),
            text(" OUT: " + std::to_string(output_tokens_)),
            filler(),
            text(" MODEL: " + model_ + " "),
            })
    | border
        | color(Color::LightSkyBlue1);
}

ftxui::Component App::BuildAndHandle() {
    using namespace ftxui;

    return CatchEvent(renderer_, [&](Event event) {
            int input_width = Terminal::Size().dimx - 4;

            if (event.input() == "\x1b[200~") {
                bracketed_paste_active_ = true;
                return true;
            }

            if (event.input() == "\x1b[201~") {
                bracketed_paste_active_ = false;
                return true;
            }

            if (input_width < 10) {
            input_width = 40;
            }

            if (ApprovalPending()) {
                if (event == Event::Character('a') || event == Event::Character('A')) {
                AnswerApproval(PermissionDecision::Allow);
                return true;
            }

            if (event == Event::Character('d') || event == Event::Character('D') || event == Event::Escape) {
                AnswerApproval(PermissionDecision::Deny);
                return true;
            }

            return true;
            }

            if (file_picker_open_) {
                auto EnsureFilePickerSelectionVisible = [this]() {
                    if (file_picker_selected_ < file_picker_scroll_offset_) {
                        file_picker_scroll_offset_ = file_picker_selected_;
                    }

                    if (file_picker_selected_ >= file_picker_scroll_offset_ + file_picker_visible_rows_) {
                        file_picker_scroll_offset_ = file_picker_selected_ - file_picker_visible_rows_ + 1;
                    }
                };

                if (event == Event::Escape) {
                    file_picker_open_ = false;
                    return true;
                }

                if (event == Event::ArrowUp) {
                    if (file_picker_selected_ > 0) {
                        file_picker_selected_--;
                        EnsureFilePickerSelectionVisible();
                    }
                    return true;
                }

                if (event == Event::ArrowDown) {
                    if (file_picker_selected_ + 1 < static_cast<int>(file_picker_entries_.size())) {
                        file_picker_selected_++;
                        EnsureFilePickerSelectionVisible();
                    }
                    return true;
                }

                if (event == Event::Return) {
                    const auto& selected = file_picker_entries_[file_picker_selected_];

                    if (selected.is_parent) {
                        file_picker_current_dir_ = file_picker_current_dir_.parent_path();
                        RefreshFilePickerEntries();
                        return true;
                    }

                    if (selected.is_directory) {
                        file_picker_current_dir_ /= selected.name;
                        RefreshFilePickerEntries();
                        return true;
                    }

                    file_picker_open_ = false;
                    auto full_path = file_picker_current_dir_ / selected.name;
                    auto relative_path = std::filesystem::relative(full_path, config_.workspace_root);
                    input_.Insert(relative_path.string());

                    return true;
                }

                file_picker_open_ = false;
            }

            if (model_list_modal_open_ && event == Event::Escape) {
                model_list_modal_open_ = false;
                input_.Clear();
                return true;
            }

            if (bracketed_paste_active_) {
                if (event == Event::Return) {
                    input_.InsertNewline();
                    return true;
                }

                if (event.is_character()) {
                    input_.Insert(event.character());
                    return true;
                }

                return true;
            }

            if (event.is_character()) {
                agent_state_.clear();

                if (event.input() == "@") {
                    file_picker_current_dir_ = config_.workspace_root;
                    RefreshFilePickerEntries();
                    file_picker_open_ = true;
                    input_.Insert(event.character());
                    return true;
                }

                if (event.input() == "/" && input_.Empty()) {
                    slash_command_active_ = true;
                    input_.Insert(event.character());
                    return true;
                }

                if (event.input() == "\\") {
                    input_.InsertNewline();
                } else {
                    input_.Insert(event.character());
                }

                return true;
            }

            if (event == Event::Backspace) {
                input_.Backspace();

                if (input_.Empty()) {
                    slash_command_active_ = false;
                    model_list_modal_open_ = false;
                }

                return true;
            }

            if (event == Event::Escape) {
                if (is_thinking_) {
                    cancel_request_ = true;
                    agent_state_ = "Abbruch angefordert... ";
                }
                return true;
            }

            if (event == Event::ArrowRight) {
                input_.MoveRight();
                return true;
            }

            if (event == Event::ArrowLeft) {
                input_.MoveLeft();
                return true;
            }

            if (event == Event::ArrowUp) {
                input_.MoveUp(input_width);
                return true;
            }

            if (event == Event::ArrowDown) {
                input_.MoveDown(input_width);
                return true;
            }

            if (event == Event::Return) {
                if (slash_command_active_) {
                    bool handled = HandleSlashCommand();

                    slash_command_active_ = false;
                    input_.Clear();

                    if (model_list_modal_open_) {
                        input_.Insert("/model ");
                        slash_command_active_ = true;
                    }

                    return handled;
                }

                if (!input_.Empty() && !is_thinking_) {
                    std::string user_text = input_.Text();
                    auto attachments = ExtractAttachmentPaths(user_text);
                    std::string llm_prompt = BuildPromptWithAttachments(user_text);

                    messages_.AddMessage({"user", user_text});
                    SaveSession();

                    if (!attachments.empty()) {
                        messages_.AddMessage({
                            "system",
                            BuildAttachmentInfoMessage(attachments)
                        });
                    }

                    auto request_messages = messages_.All();

                    for (auto it = request_messages.rbegin(); it != request_messages.rend(); ++it) {
                        if (it->role == "user") {
                            it->content = llm_prompt;
                            break;
                        }
                    }
                    std::string prompt_model = model_;

                    input_.Clear();
                    is_thinking_ = true;
                    cancel_request_ = false;

                    if (worker_.joinable()) {
                        worker_.join();
                    }

                    auto screen = ScreenInteractive::Active();

                    messages_.AddMessage({"assistant", ""});
                    SaveSession();

                    worker_ = std::thread([this, prompt_model, request_messages, screen]() {
                            ollama_.ChatStream(
                                    prompt_model,
                                    request_messages,
                                    [this, screen](const std::string& token) {
                                    PushEvent({AppEvent::Type::Token, token});
                                    screen->PostEvent(Event::Custom);
                                    },
                                    [this, screen](int input, int output) {
                                    PushEvent({AppEvent::Type::Done, "", input, output});
                                    screen->PostEvent(Event::Custom);
                                    },
                                    [this, screen](const std::string& error) {
                                    PushEvent({AppEvent::Type::Error, error});
                                    screen->PostEvent(Event::Custom);
                                    },
                                    [this, screen](const std::string& aistate) {
                                    PushEvent({AppEvent::Type::AiState, aistate});
                                    screen->PostEvent(Event::Custom);
                                    },
                                    [this]() {
                                    return cancel_request_.load();
                                    }
                                    );
                    });
                }

                chat_scroll_offset_ = 1.0;
                return true;
            }

            if (event == Event::PageUp) {
                chat_scroll_offset_ -= 0.15;

                if (chat_scroll_offset_ < 0.0) {
                    chat_scroll_offset_ = 0.0;
                }

                return true;
            }

            if (event == Event::PageDown) {
                chat_scroll_offset_ += 0.15;

                if (chat_scroll_offset_ > 1.0) {
                    chat_scroll_offset_ = 1.0;
                }

                return true;
            }

            return false;
    });
}

void App::PushEvent(AppEvent event) {
    std::lock_guard<std::mutex> lock(event_mutex_);
    events_.push(event);
}

void App::ProcessEvents() {
    std::lock_guard<std::mutex> lock(event_mutex_);

    while (!events_.empty()) {
        AppEvent event = events_.front();
        events_.pop();

        if (event.type == AppEvent::Type::Token) {
            messages_.AppendToLastMessage(event.text);
        }

        if (event.type == AppEvent::Type::Done) {
            input_tokens_ = event.input_tokens;
            output_tokens_ = event.output_tokens;
            agent_state_.clear();
            is_thinking_ = false;
        }

        if (event.type == AppEvent::Type::Error) {
            messages_.AddMessage({"system", event.text});
            is_thinking_ = false;
        }

        if (event.type == AppEvent::Type::AiState) {
            agent_state_ = event.text;
        }
    }
}

PermissionDecision App::RequestApproval(const ToolCall& call)
{
    {
        std::lock_guard<std::mutex> lock(approval_mutex_);

        approval_tool_ = call.name;
        approval_arguments_ = FormatToolArguments(call.arguments, 120, "\n");
        approval_pending_ = true;
        approval_answered_ = false;
        approval_decision_ = PermissionDecision::Deny;
    }

    auto screen = ftxui::ScreenInteractive::Active();
    if (screen) {
        screen->PostEvent(ftxui::Event::Custom);
    }

    std::unique_lock<std::mutex> lock(approval_mutex_);
    approval_cv_.wait(lock, [this] {
            return approval_answered_;
            });

    return approval_decision_;
}

void App::AnswerApproval(PermissionDecision decision)
{
    {
        std::lock_guard<std::mutex> lock(approval_mutex_);

        approval_decision_ = decision;
        approval_answered_ = true;
        approval_pending_ = false;
    }

    approval_cv_.notify_one();

    auto screen = ftxui::ScreenInteractive::Active();
    if (screen) {
        screen->PostEvent(ftxui::Event::Custom);
    }
}

bool App::ApprovalPending() const
{
    std::lock_guard<std::mutex> lock(approval_mutex_);
    return approval_pending_;
}

ftxui::Element App::RenderApprovalDialog()
{
    using namespace ftxui;

    std::string tool;
    std::string arguments;

    {
        std::lock_guard<std::mutex> lock(approval_mutex_);
        tool = approval_tool_;
        arguments = approval_arguments_;
    }

    return vbox({
            text(" Approval required ") | bold,
            separator(),
            text("Tool: " + tool),
            paragraph(arguments),
            separator(),
            text("[A] Allow once    [D] Deny    [Esc] Deny")
            })
    | border
        | size(WIDTH, LESS_THAN, 80)
        | color(Color::Red)
        | bgcolor(Color::White);
}

bool App::HandleSlashCommand()
{
    auto command = SlashCommandParser::Parse(input_.Text());

    if (command.type == SlashCommandType::Exit) {
        running_ = false;

        auto screen = ftxui::ScreenInteractive::Active();
        if (screen) {
            screen->Exit();
        }

        return true;
    }

    if (command.type == SlashCommandType::Model) {
        if (command.args.empty()) {
            agent_state_ = "Aktuelles Modell: " + model_;
            model_list_modal_open_ = false;
            return true;
        }

        std::string selected_model = command.args[0];

        available_models_ = ollama_.ListModels();

        if (std::find(available_models_.begin(), available_models_.end(), selected_model) != available_models_.end()) {
            model_ = selected_model;
            agent_state_ = "Modell gewechselt: " + model_;
        } else {
            agent_state_ = "Modell nicht verfügbar: " + selected_model;
        }

        model_list_modal_open_ = false;
        return true;
    }

    if (command.type == SlashCommandType::ModelList) {
        available_models_ = ollama_.ListModels();
        model_list_error_.clear();

        if (available_models_.empty()) {
            model_list_error_ = "Keine Modelle gefunden oder Ollama nicht erreichbar.";
        }

        slash_command_active_ = false;
        model_list_modal_open_ = true;
        agent_state_ = std::to_string(available_models_.size()) + " Modelle gefunden";

        return true;
    }

    agent_state_ = "Unbekannter Slash Command: " + input_.Text();

    return true;
}

ftxui::Element App::RenderSlashCommandDialog()
{
    using namespace ftxui;

    auto commands = SlashCommandParser::FilterCommands(input_.Text(), model_);

    Elements command_elements;

    command_elements.push_back(text(" Slash Commands ") | bold);
    command_elements.push_back(separator());

    if (commands.empty()) {
        command_elements.push_back(text("Keine passenden Commands"));
    } else {
        for (const auto& command : commands) {
            command_elements.push_back(
                    hbox({
                        text("  " + command.name) | size(WIDTH, EQUAL, 16) | bold,
                        separatorEmpty(),
                        text(command.description) | dim
                        })
                    );
        }
    }

    command_elements.push_back(separator());
    command_elements.push_back(text("Enter ausführen"));

    return vbox(command_elements)
        | border
        | size(WIDTH, LESS_THAN, 70)
        | color(Color::LightSkyBlue1)
        | bgcolor(Color::Black);
}

ftxui::Element App::RenderModelListDialog()
{
    using namespace ftxui;

    Elements rows;

    rows.push_back(text(" Installierte Ollama-Modelle ") | bold);
    rows.push_back(separator());

    if (!model_list_error_.empty()) {
        rows.push_back(paragraph(model_list_error_) | color(Color::Red));
    } else if (available_models_.empty()) {
        rows.push_back(text("Keine Modelle gefunden"));
    } else {
        for (const auto& model : available_models_) {
            auto marker = model == model_ ? "* " : "  ";

            rows.push_back(
                    hbox({
                        text(marker + model)
                        })
                    );
        }
    }

    rows.push_back(separator());
    rows.push_back(text("Enter oder Esc schließen") | dim);

    return vbox(rows)
        | border
        | size(WIDTH, LESS_THAN, 60)
        | size(HEIGHT, LESS_THAN, 20);
}

void App::RefreshFilePickerEntries()
{
    namespace fs = std::filesystem;

    file_picker_entries_.clear();
    file_picker_selected_ = 0;
    file_picker_scroll_offset_ = 0;

    if (file_picker_current_dir_ != config_.workspace_root) {
        file_picker_entries_.push_back({
                "..",
                true,
                true
                });
    }

    try {
        for (const auto& entry : fs::directory_iterator(file_picker_current_dir_)) {
            FilePickerEntry item;
            item.name = entry.path().filename().string();
            item.is_directory = entry.is_directory();
            item.is_parent = false;

            file_picker_entries_.push_back(item);
        }

        std::sort(file_picker_entries_.begin(), file_picker_entries_.end(),
                [](const FilePickerEntry& a, const FilePickerEntry& b) {
                if (a.is_parent != b.is_parent) {
                return a.is_parent > b.is_parent;
                }

                if (a.is_directory != b.is_directory) {
                return a.is_directory > b.is_directory;
                }

                return a.name < b.name;
                }
                );
    } catch (const std::exception& e) {
        file_picker_entries_.push_back({
                std::string("Fehler: ") + e.what(),
                false
                });
    }
}

ftxui::Element App::RenderFilePickerDialog()
{
    using namespace ftxui;

    Elements rows;

    int end = std::min(
            file_picker_scroll_offset_ + file_picker_visible_rows_,
            static_cast<int>(file_picker_entries_.size())
            );

    for (int i = file_picker_scroll_offset_; i < end; ++i) {
        const auto& entry = file_picker_entries_[i];

        std::string label = entry.name;

        if (entry.is_parent) {
            label = "../";
        } else if (entry.is_directory) {
            label += "/";
        }

        std::string prefix = (i == file_picker_selected_) ? "> " : "  ";

        auto row = text(prefix + label);

        if (i == file_picker_selected_) {
            row = row | inverted;
        }

        rows.push_back(row);
    }

    return vbox({
            text(" File Picker " + file_picker_current_dir_.string()) | bold,
            separator(),
            vbox(rows),
            separator(),
            text(std::to_string(file_picker_selected_ + 1) + "/" + std::to_string(file_picker_entries_.size()) +
                    " [↑/↓] auswählen  [Enter] übernehmen  [Esc] schließen") | dim
            })
    | border
        | size(WIDTH, LESS_THAN, 70)
        | color(Color::LightSkyBlue1)
        | bgcolor(Color::Black);
}

std::vector<std::filesystem::path> App::ExtractAttachmentPaths(const std::string& text) const
{
    std::vector<std::filesystem::path> paths;

    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] != '@') {
            continue;
        }

        size_t start = i + 1;

        if (start >= text.size()) {
            continue;
        }

        size_t end = start;

        while (end < text.size()) {
            unsigned char c = static_cast<unsigned char>(text[end]);

            if (std::isspace(c)) {
                break;
            }

            if (text[end] == ',' ||
                    text[end] == ';' ||
                    text[end] == ':' ||
                    text[end] == ')' ||
                    text[end] == ']' ||
                    text[end] == '}') {
                break;
            }

            ++end;
        }

        if (end > start) {
            paths.emplace_back(text.substr(start, end - start));
        }

        i = end;
    }

    return paths;
}

std::string App::BuildAttachmentInfoMessage(const std::vector<std::filesystem::path>& paths) const
{
    if (paths.empty()) {
        return "";
    }

    std::ostringstream out;
    out << "Attached " << paths.size() << " file(s):\n";

    for (const auto& path : paths) {
        out << "- " << path.string() << "\n";
    }

    return out.str();
}

std::string App::BuildPromptWithAttachments(const std::string& user_input) const
{
    auto paths = ExtractAttachmentPaths(user_input);

    if (paths.empty()) {
        return user_input;
    }

    std::ostringstream prompt;

    prompt << "User message:\n";
    prompt << user_input << "\n\n";
    prompt << "Attached files:\n\n";

    for (const auto& path : paths) {
        auto full_path = config_.workspace_root / path;

        prompt << "--- attachment: " << path.string() << " ---\n";

        if (!std::filesystem::exists(full_path)) {
            prompt << "Status: ERROR\n";
            prompt << "Reason: File not found.\n";
            prompt << "This attachment could not be loaded.\n";
        } else if (!std::filesystem::is_regular_file(full_path)) {
            prompt << "Status: ERROR\n";
            prompt << "Reason: Path is not a regular file.\n";
            prompt << "This attachment could not be loaded.\n";
        } else {
            std::ifstream file(full_path);

            prompt << "Status: OK\n";
            prompt << "Content:\n";
            prompt << file.rdbuf();
        }

        prompt << "\n--- end attachment ---\n\n";
    }

    return prompt.str();
}

void App::LoadSession()
{
    if (!config_.session_enabled) {
        return;
    }

    session_store_.Load(config_.session_name, messages_);
}

void App::SaveSession()
{
    if (!config_.session_enabled) {
        return;
    }

    session_store_.Save(config_.session_name, messages_);
}

