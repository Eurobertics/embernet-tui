#include "SessionStore.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <vector>
#include <algorithm>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

std::filesystem::path SessionStore::SessionPath(const std::string& name) const
{
    const char* home = std::getenv("HOME");
    if (home == nullptr) {
        return {};
    }

    return std::filesystem::path(home)
        / ".config"
        / "embernet-tui"
        / "sessions"
        / (name + ".json");
}

bool SessionStore::Load(const std::string& name, MessageStore& messages)
{
    auto path = SessionPath(name);

    if (!std::filesystem::exists(path)) {
        std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    json data;
    try {
        file >> data;

        std::vector<Message> loaded_messages;

        for (const auto& item : data["messages"]) {
            loaded_messages.push_back({
                item.value("role", "system"),
                item.value("content", "")
            });
        }

        messages.SetMessages(loaded_messages);
        return true;
    } catch (...) {
        return false;
    }
}

bool SessionStore::Save(const std::string& name, const MessageStore& messages)
{
    try {
        auto path = SessionPath(name);
        std::filesystem::create_directories(path.parent_path());

        json data;
        data["version"] = 1;
        data["messages"] = json::array();

        for (const auto& message : messages.All()) {
            data["messages"].push_back({
                {"role", message.role},
                {"content", message.content}
            });
        }

        std::ofstream file(SessionPath(name));
        if (!file.is_open()) {
            return false;
        }

        file << data.dump(2);
        return true;
    } catch (...) {
        return false;
    }
}

std::filesystem::path SessionStore::SessionSavePath() const
{
    const char* home = std::getenv("HOME");
    if (home == nullptr) {
        return {};
    }

    return std::filesystem::path(home)
        / ".config"
        / "embernet-tui"
        / "sessions";
}

std::vector<std::string> SessionStore::ListSessions() const
{
    std::vector<std::string> sessions;

    auto dir = SessionSavePath();

    if (!std::filesystem::exists(dir)) {
        return sessions;
    }

    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        auto path = entry.path();

        if (path.extension() != ".json") {
            continue;
        }

        sessions.push_back(path.stem().string());
    }

    std::sort(sessions.begin(), sessions.end());

    return sessions;
}

