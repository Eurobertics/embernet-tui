#pragma once

#include <vector>
#include <string>
#include "../models/Message.hpp"

class MessageStore {
    public:
        void AppendToLastMessage(const std::string& text);
        void AddMessage(const Message& message);
        void SetMessages(const std::vector<Message>& messages);
        void clear();

        const std::vector<Message>& All() const;
        std::vector<Message> Last(int count) const;

    private:
        std::vector<Message> messages_;
};

