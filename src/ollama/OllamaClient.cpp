#include "OllamaClient.hpp"

#include <cpr/api.h>
#include <cpr/cpr.h>

#include <cstdint>
#include <optional>
#include <nlohmann/json_fwd.hpp>
#include <string>

#include "../tool/Tools.hpp"
#include "../tool/ToolManager.hpp"
#include "../tool/ToolFormat.hpp"

namespace {

std::string Trim(const std::string& text)
{
    const auto start = text.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }

    const auto end = text.find_last_not_of(" \t\r\n");
    return text.substr(start, end - start + 1);
}

bool IsPrefixOf(const std::string& value, const std::string& target)
{
    return value.size() <= target.size() &&
        target.compare(0, value.size(), value) == 0;
}

bool IsPotentialTextToolCallPrefix(const std::string& content)
{
    std::string trimmed = Trim(content);

    if (trimmed.empty()) {
        return true;
    }

    if (trimmed[0] == '{') {
        return true;
    }

    if (trimmed[0] == '<') {
        return IsPrefixOf(trimmed, "<function") ||
            IsPrefixOf(trimmed, "<tool_call>") ||
            trimmed.starts_with("<function") ||
            trimmed.starts_with("<tool_call>");
    }

    return false;
}

std::string StripToolCallWrapper(const std::string& content)
{
    std::string stripped = Trim(content);

    const std::string open = "<tool_call>";
    const std::string close = "</tool_call>";

    if (stripped.starts_with(open)) {
        stripped = Trim(stripped.substr(open.size()));
    }

    if (stripped.ends_with(close)) {
        stripped = Trim(stripped.substr(0, stripped.size() - close.size()));
    }

    return stripped;
}

std::optional<ToolCall> ParseJsonTextToolCall(const std::string& content)
{
    try {
        auto parsed = nlohmann::json::parse(StripToolCallWrapper(content));

        if (!parsed.is_object() ||
            !parsed.contains("name") ||
            !parsed["name"].is_string() ||
            !parsed.contains("arguments") ||
            !parsed["arguments"].is_object()) {
            return std::nullopt;
        }

        return ToolCall{
            parsed["name"].get<std::string>(),
            parsed["arguments"]
        };
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<ToolCall> ParseFunctionTextToolCall(const std::string& content)
{
    const std::string text = StripToolCallWrapper(content);

    const std::string function_open = "<function=";
    const std::string parameter_open = "<parameter=";
    const std::string parameter_close = "</parameter>";
    const std::string function_close = "</function>";

    const auto function_pos = text.find(function_open);
    if (function_pos == std::string::npos) {
        return std::nullopt;
    }

    const auto name_start = function_pos + function_open.size();
    const auto name_end = text.find('>', name_start);
    if (name_end == std::string::npos) {
        return std::nullopt;
    }

    std::string name = Trim(text.substr(name_start, name_end - name_start));
    if ((name.starts_with('"') && name.ends_with('"')) ||
        (name.starts_with('\'') && name.ends_with('\''))) {
        name = name.substr(1, name.size() - 2);
    }

    if (name.empty()) {
        return std::nullopt;
    }

    nlohmann::json arguments = nlohmann::json::object();
    size_t search_pos = name_end + 1;

    while (true) {
        const auto param_pos = text.find(parameter_open, search_pos);
        if (param_pos == std::string::npos) {
            break;
        }

        const auto key_start = param_pos + parameter_open.size();
        const auto key_end = text.find('>', key_start);
        if (key_end == std::string::npos) {
            return std::nullopt;
        }

        std::string key = Trim(text.substr(key_start, key_end - key_start));
        if ((key.starts_with('"') && key.ends_with('"')) ||
            (key.starts_with('\'') && key.ends_with('\''))) {
            key = key.substr(1, key.size() - 2);
        }

        const auto value_start = key_end + 1;
        const auto value_end = text.find(parameter_close, value_start);
        if (value_end == std::string::npos) {
            return std::nullopt;
        }

        arguments[key] = Trim(text.substr(value_start, value_end - value_start));
        search_pos = value_end + parameter_close.size();
    }

    if (arguments.empty()) {
        return std::nullopt;
    }

    const auto close_pos = text.find(function_close, search_pos);
    if (close_pos == std::string::npos) {
        return std::nullopt;
    }

    return ToolCall{name, arguments};
}

std::optional<ToolCall> ParseTextToolCall(const std::string& content)
{
    if (auto call = ParseJsonTextToolCall(content)) {
        return call;
    }

    return ParseFunctionTextToolCall(content);
}

} // namespace

OllamaClient::OllamaClient(ToolManager& tools, const AppConfig& config)
    : tools_(tools),
    prompt_provider_(config.prompt_file)
{
    url_ = config.base_api_url + "/api/chat";
    url_model_list_ = config.base_api_url + "/api/tags";
}

OllamaClient::json OllamaClient::BuildMessagesJson(
    const std::vector<Message>& messages
) const {
    json result = json::array();

    result.push_back({
        {"role", "system"},
        {"content", prompt_provider_.GetSystemPrompt()}
    });

    for (const auto& message : messages) {
        result.push_back({
            {"role", message.role},
            {"content", message.content}
        });
    }

    return result;
}

nlohmann::json OllamaClient::BuildToolDefinitions() const
{
    json tools = json::array();

    for (const auto& tool : BuildReadFileToolDefinition()) {
        tools.push_back(tool);
    }

    for (const auto& tool : BuildReadDirectoryToolDefinition()) {
        tools.push_back(tool);
    }

    for (const auto& tool : BuildWriteFileToolDefinition()) {
        tools.push_back(tool);
    }

    for (const auto& tool : BuildCreateDirectoryToolDefinition()) {
        tools.push_back(tool);
    }

    for (const auto& tool : BuildReplaceTextToolDefinition()) {
        tools.push_back(tool);
    }

    for (const auto& tool : BuildDeleteFileToolDefinition()) {
        tools.push_back(tool);
    }

    for (const auto& tool : BuildDeleteDirectoryToolDefinition()) {
        tools.push_back(tool);
    }

    for (const auto& tool : BuildExecCommandToolDefinition()) {
        tools.push_back(tool);
    }

    return tools;
}

OllamaClient::json OllamaClient::BuildReadFileToolDefinition() const
{
    return json::array({
        {
            {"type", "function"},
            {"function", {
                {"name", "read_file"},
                {"description", "Read the contents of a text file from the local project directory."},
                {"parameters", {
                    {"type", "object"},
                    {"properties", {
                        {"path", {
                            {"type", "string"},
                            {"description", "Relative path to the file"}
                        }}
                    }},
                    {"required", json::array({"path"})}
                }}
            }}
        }
    });
}

OllamaClient::json OllamaClient::BuildReadDirectoryToolDefinition() const
{
    return json::array({
        {
            {"type", "function"},
            {"function", {
                {"name", "read_directory"},
                {"description", "Read the file list of a directory."},
                {"parameters", {
                    {"type", "object"},
                    {"properties", {
                        {"path", {
                            {"type", "string"},
                            {"description", "Relative path to the directory"}
                        }}
                    }},
                    {"required", json::array({"path"})}
                }}
            }}
        }
    });
}

OllamaClient::json OllamaClient::BuildWriteFileToolDefinition() const
{
    return json::array({
        {
            {"type", "function"},
            {"function", {
                {"name", "write_file"},
                {"description", "Write a file with content"},
                {"parameters", {
                    {"type", "object"},
                    {"properties", {
                        {"path", {
                            {"type", "string"},
                            {"description", "Relative path to the file"}
                        }},
                        {"content", {
                            {"type", "string"},
                            {"description", "The content to write into the file"}
                        }}
                    }},
                    {"required", json::array({"path", "content"})}
                }}
            }}
        }
    });
}

OllamaClient::json OllamaClient::BuildCreateDirectoryToolDefinition() const
{
    return json::array({
        {
            {"type", "function"},
            {"function", {
                {"name", "create_directory"},
                {"description", "Creates a single directory"},
                {"parameters", {
                    {"type", "object"},
                    {"properties", {
                        {"path", {
                            {"type", "string"},
                            {"description", "Relative path of the directory to be created"}
                        }}
                    }},
                    {"required", json::array({"path"})}
                }}
            }}
        }
    });
}

OllamaClient::json OllamaClient::BuildReplaceTextToolDefinition() const
{
    return json::array({
        {
            {"type", "function"},
            {"function", {
                {"name", "replace_text"},
                {"description", "Replace exactly one matching text fragment inside a file in the workspace. Use this for precise edits. The old_text must match exactly and must occur only once."},
                {"parameters", {
                    {"type", "object"},
                    {"properties", {
                        {"path", {
                            {"type", "string"},
                            {"description", "Relative path to the file"}
                        }},
                        {"old_text", {
                            {"type", "string"},
                            {"description", "Exact text to replace. Must occur exactly once."}
                        }},
                        {"new_text", {
                            {"type", "string"},
                            {"description", "Replacement text"}
                        }}
                    }},
                    {"required", json::array({"path", "old_text", "new_text"})}
                }}
            }}
        }
    });
}

OllamaClient::json OllamaClient::BuildDeleteFileToolDefinition() const
{
    return json::array({
        {
            {"type", "function"},
            {"function", {
                {"name", "delete_file"},
                {"description", "Delete one regular file inside the workspace. This is destructive and requires approval."},
                {"parameters", {
                    {"type", "object"},
                    {"properties", {
                        {"path", {
                            {"type", "string"},
                            {"description", "Relative path to the file to delete"}
                        }}
                    }},
                    {"required", json::array({"path"})}
                }}
            }}
        }
    });
}

OllamaClient::json OllamaClient::BuildDeleteDirectoryToolDefinition() const
{
    return json::array({
        {
            {"type", "function"},
            {"function", {
                {"name", "delete_directory"},
                {"description", "Deletes a directory with all its contents."},
                {"parameters", {
                    {"type", "object"},
                    {"properties", {
                        {"path", {
                            {"type", "string"},
                            {"description", "Relative path to the directory to delete"}
                        }}
                    }},
                    {"required", json::array({"path"})}
                }}
            }}
        }
    });
}

OllamaClient::json OllamaClient::BuildExecCommandToolDefinition() const
{
    return json::array({
        {
            {"type", "function"},
            {"function", {
                {"name", "exec_command"},
                {"description", "Executes an shell command, command is the shell command itself, args is an array of arguments."},
                {"parameters", {
                    {"type", "object"},
                    {"properties", {
                        {"command", {
                            {"type", "string"},
                            {"description", "Single executable name, without parameters, for example git, ls, pwd, cmake, make or g++"}
                        }},
                        {"args", {
                            {"type", "array"},
                            {"items", {
                                {"type", "string"}
                            }},
                            {"description", "Command arguments as separate strings, for example [\"status\", \"--short\"]"}
                        }}
                    }},
                    {"required", json::array({"command", "args"})}
                }}
            }}
        }
    });
}

void OllamaClient::PostStream(
    const json& request,
    std::function<void(const json&)> on_chunk,
    std::function<void(const std::string&)> on_error,
    std::function<bool()> should_cancel
) const {
    std::string buffer;

    auto parse_line = [&](const std::string& line) {
        if (line.empty()) {
            return;
        }

        try {
            json chunk = json::parse(line);
            on_chunk(chunk);
        } catch (...) {
            on_error("Fehler beim Parsen eines Stream-Chunks:\n" + line);
        }
    };

    auto response = cpr::Post(
        cpr::Url(url_),
        cpr::Body(request.dump()),
        cpr::Header{{"Content-Type", "application/json"}},
        cpr::WriteCallback([&](std::string_view data, intptr_t) {
            if (should_cancel && should_cancel()) {
                return false;
            }
            buffer.append(data.data(), data.size());

            size_t pos;
            while ((pos = buffer.find('\n')) != std::string::npos) {
                std::string line = buffer.substr(0, pos);
                buffer.erase(0, pos + 1);

                parse_line(line);
            }

            return true;
        })
    );

    if (should_cancel && should_cancel()) {
        on_error("Antwort abgebrochen.");
        return;
    }

    if (!buffer.empty()) {
        parse_line(buffer);
    }

    if (response.status_code != 200) {
        on_error("Ollama HTTP Fehler: " + std::to_string(response.status_code));
    }
}

Message OllamaClient::Chat(
    const std::string model,
    const std::vector<Message>& messages
) {
    json request;
    request["model"] = model;
    request["stream"] = false;
    request["messages"] = BuildMessagesJson(messages);

    auto response = cpr::Post(
        cpr::Url(url_),
        cpr::Body(request.dump()),
        cpr::Header{{"Content-Type", "application/json"}}
    );

    try {
        json response_json = json::parse(response.text);

        return {
            "assistant",
            response_json["message"]["content"]
        };
    } catch (...) {
        return {
            "System",
            "Es gab einen Fehler mit Ollama!"
        };
    }
}

void OllamaClient::ChatStream(
    const std::string& model,
    const std::vector<Message>& messages,
    std::function<void(const std::string&)> on_token,
    std::function<void(int, int)> on_done,
    std::function<void(const std::string&)> on_error,
    std::function<void(const std::string&)> on_aistate,
    std::function<bool()> should_cancel
) {
    json working_messages = BuildMessagesJson(messages);
    
    int total_input_tokens = 0;
    int total_output_tokens = 0;

    const int max_tool_steps = 40;

    for (int step = 0; step < max_tool_steps; ++step) {
        json request;
        request["model"] = model;
        request["stream"] = true;
        request["messages"] = working_messages;
        request["tools"] = BuildToolDefinitions();

        bool has_tool_call = false;
        json assistant_message;
        json tool_call_json;
        std::optional<ToolCall> text_tool_call;
        std::string assistant_content;
        bool buffering_possible_text_tool_call = true;

        int input_tokens = 0;
        int output_tokens = 0;

        PostStream(
            request,
            [&](const json& chunk) {
                if (chunk.contains("message")) {
                    const auto& message = chunk["message"];

                    if (message.contains("tool_calls") && !message["tool_calls"].empty()) {
                        has_tool_call = true;
                        assistant_message = message;
                        tool_call_json = message["tool_calls"][0];
                        return;
                    }

                    if (message.contains("content")) {
                        std::string content = message["content"].get<std::string>();

                        if (buffering_possible_text_tool_call) {
                            assistant_content += content;

                            if (IsPotentialTextToolCallPrefix(assistant_content)) {
                                return;
                            }

                            buffering_possible_text_tool_call = false;
                            on_token(assistant_content);
                            assistant_content.clear();
                            return;
                        }

                        on_token(content);
                    }
                }

                if (chunk.value("done", false)) {
                    input_tokens = chunk.value("prompt_eval_count", 0);
                    output_tokens = chunk.value("eval_count", 0);
                }
            },
            on_error,
            should_cancel
        );

        total_input_tokens += input_tokens;
        total_output_tokens += output_tokens;

        if (!assistant_content.empty()) {
            text_tool_call = ParseTextToolCall(assistant_content);

            if (!text_tool_call) {
                on_token(assistant_content);
            }
        }

        if (!has_tool_call) {
            if (!text_tool_call) {
                on_done(total_input_tokens, total_output_tokens);
                return;
            }

            has_tool_call = true;
            assistant_message = {
                {"role", "assistant"},
                {"content", assistant_content}
            };
        }

        ToolCall call = text_tool_call
            ? *text_tool_call
            : ToolCall{
                tool_call_json["function"]["name"].get<std::string>(),
                tool_call_json["function"]["arguments"]
            };

        std::string ai_state_string = FormatToolArguments(call.arguments);
        on_aistate("Executing " + call.name + ": " + ai_state_string);

        ToolResult tool_result = tools_.Execute(call);

        working_messages.push_back(assistant_message);
        
        working_messages.push_back({
            {"role", "tool"},
            {"tool_name", tool_result.name},
            {"content", tool_result.content}
        });
    }

    on_error("Maximale Anzahl an Tool-Schritten erreicht.");
    on_done(total_input_tokens, total_output_tokens);
}

std::vector<std::string> OllamaClient::ListModels()
{
    cpr::Response response = cpr::Get(
        cpr::Url{url_model_list_},
        cpr::Timeout{2000}
    );

    if (response.status_code != 200) {
        return {};
    }

    std::vector<std::string> model_list;

    try {
        json result = json::parse(response.text);

        if (!result.contains("models") || !result["models"].is_array()) {
            return {};
        }

        for (const auto& model : result["models"]) {
            if (model.contains("name") && model["name"].is_string()) {
                model_list.push_back(model["name"].get<std::string>());
            }
        }
    } catch (...) {
        return {};
    }

    return model_list;
}
