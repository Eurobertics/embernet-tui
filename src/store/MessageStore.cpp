#include "MessageStore.hpp"
#include <algorithm>
#include <vector>

void MessageStore::AppendToLastMessage(const std::string& text) {
    if (!messages_.empty()) {
        messages_.back().content += text;
    }
}

void MessageStore::AddMessage(const Message& message)
{
    messages_.push_back({message.role, message.content});
}

void MessageStore::SetMessages(const std::vector<Message>& messages)
{
    messages_ = messages;
}

void MessageStore::clear()
{
    messages_.clear();
}

const std::vector<Message>& MessageStore::All() const
{
    return messages_;
}

std::vector<Message> MessageStore::Last(int count) const
{
    int start = std::max(0, static_cast<int>(messages_.size()) - count);

    return std::vector<Message>(messages_.begin() + start, messages_.end());
}

