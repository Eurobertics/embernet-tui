#pragma once

#include <string>
#include <vector>

enum class SlashCommandType {
    Unknown,
    Exit,
    Model,
    ModelList,
    SessionInfo
};

struct SlashCommand {
    SlashCommandType type = SlashCommandType::Unknown;
    std::vector<std::string> args;
};

struct SlashCommandInfo {
    std::string name;
    std::string description;
};

class SlashCommandParser {
public:
    static SlashCommand Parse(const std::string& input);
    static std::vector<SlashCommandInfo> AvailableCommands(const std::string& current_model);
    static std::vector<SlashCommandInfo> FilterCommands(
        const std::string& input,
        const std::string& current_model
    );
};

