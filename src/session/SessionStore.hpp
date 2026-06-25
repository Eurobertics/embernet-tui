#pragma once

#include <filesystem>
#include <string>

#include "../store/MessageStore.hpp"

class SessionStore {
public:
    bool Load(const std::string& name, MessageStore& messages);
    bool Save(const std::string& name, const MessageStore& messages);
    std::vector<std::string> ListSessions() const;

private:
    std::filesystem::path SessionSavePath() const;
    std::filesystem::path SessionPath(const std::string& name) const;
};

