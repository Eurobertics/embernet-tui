#include "SessionStore.hpp"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <vector>
#include <algorithm>
#include <format>
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
    std::vector<std::filesystem::path> sessions_path;

    auto dir = SessionSavePath();

    if (!std::filesystem::exists(dir)) {
        return std::vector<std::string>{};
    }

    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        auto path = entry.path();

        if (path.extension() != ".json") {
            continue;
        }

        sessions_path.push_back(path);
    }

    std::sort(sessions_path.begin(), sessions_path.end(),
        [](
            const std::filesystem::path& a,
            const std::filesystem::path& b
        ) {
            auto time_a = std::filesystem::last_write_time(a);
            auto time_b = std::filesystem::last_write_time(b);
            return time_a < time_b;
        }
    );

    std::vector<std::string> sessions;
    for (const auto& s : sessions_path) {
        auto last_access_time = std::filesystem::last_write_time(s);
        std::string last_access_formatted = std::format("{:%F %R}", last_access_time);
        sessions.push_back(s.stem().string() + " Last used: " + last_access_formatted);
    }

    return sessions;
}

