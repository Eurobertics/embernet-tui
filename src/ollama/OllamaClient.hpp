#pragma once

#include <functional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "../config/AppConfig.hpp"
#include "../models/Message.hpp"
#include "../tool/ToolManager.hpp"
#include "../config/PromptProvider.hpp"

class OllamaClient {
    public:
        explicit OllamaClient(
            ToolManager& tools,
            const AppConfig& config
        );
        Message Chat(const std::string model, const std::vector<Message>& messages);

        void ChatStream(
            const std::string& model,
            const std::vector<Message>& messages,
            std::function<void(const std::string&)> on_token,
            std::function<void(int, int)> on_done,
            std::function<void(const std::string&)> on_error,
            std::function<void(const std::string&)> on_aistate,
            std::function<bool()> should_cancel
        );

        std::vector<std::string> ListModels();

    private:
        using json = nlohmann::json;

        json BuildMessagesJson(const std::vector<Message>& messages) const;
        json BuildToolDefinitions() const;
        json BuildReadFileToolDefinition() const;
        json BuildReadDirectoryToolDefinition() const;
        json BuildWriteFileToolDefinition() const;
        json BuildCreateDirectoryToolDefinition() const;
        json BuildReplaceTextToolDefinition() const;
        json BuildDeleteFileToolDefinition() const;
        json BuildDeleteDirectoryToolDefinition() const;
        json BuildExecCommandToolDefinition() const;

        void PostStream(
            const json& request,
            std::function<void(const json&)> on_chunk,
            std::function<void(const std::string&)> on_error,
            std::function<bool()> should_cancel
        ) const;

        ToolManager& tools_;

        std::string url_ = "http://brainbox.fritz.box:11434/api/chat";
        std::string url_model_list_ = "http://brainbox.fritz.box:11434/api/tags";

        PromptProvider prompt_provider_;
};

