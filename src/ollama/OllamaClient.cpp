#include "OllamaClient.hpp"

#include <cpr/api.h>
#include <cpr/cpr.h>

#include <cstdint>
#include <nlohmann/json_fwd.hpp>
#include <string>

#include "../tool/Tools.hpp"
#include "../tool/ToolManager.hpp"
#include "../tool/ToolFormat.hpp"

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
                {"description", "Execute an allowlisted local development command without shell expansion. Use it to inspect or build the project. Prefer read_file and read_directory for reading files. Do not modify files unless explicitly requested by the user."},
                {"parameters", {
                    {"type", "object"},
                    {"properties", {
                        {"command", {
                            {"type", "string"},
                            {"description", "Executable name, for example git, ls, pwd, cmake, make or g++"}
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
                        on_token(message["content"].get<std::string>());
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

        if (!has_tool_call) {
            on_done(total_input_tokens, total_output_tokens);
            return;
        }

        ToolCall call{
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

