#include "SlashCommand.hpp"

#include <sstream>

SlashCommand SlashCommandParser::Parse(const std::string& input)
{
    std::stringstream stream(input);

    std::string command;
    stream >> command;

    SlashCommand result;

    if (command == "/exit") {
        result.type = SlashCommandType::Exit;
    } else if (command == "/model") {
        result.type = SlashCommandType::Model;
    } else if (command == "/list-llms") {
        result.type = SlashCommandType::ModelList;
    } else {
        result.type = SlashCommandType::Unknown;
    }

    std::string arg;
    while (stream >> arg) {
        result.args.push_back(arg);
    }

    return result;
}

std::vector<SlashCommandInfo> SlashCommandParser::AvailableCommands(const std::string& current_model)
{
    return {
        {"/exit", "Beendet das Programm"},
        {"/model", "LLM Modellwahl (Aktuell: " + current_model + ")"},
        {"/list-llms", "Liste lokal verfügbarer LLMs"}
    };
}

std::vector<SlashCommandInfo> SlashCommandParser::FilterCommands(
    const std::string& input,
    const std::string& current_model
)
{
    std::vector<SlashCommandInfo> result;

    std::string command_part = input;

    auto space_pos = command_part.find(' ');
    if (space_pos != std::string::npos) {
        command_part = command_part.substr(0, space_pos);
    }

    for (const auto& command : AvailableCommands(current_model)) {
        if (command.name.starts_with(command_part)) {
            result.push_back(command);
        }
    }

    return result;
}

