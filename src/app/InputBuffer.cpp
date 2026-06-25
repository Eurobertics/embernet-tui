#include "InputBuffer.hpp"
#include <algorithm>
#include <ftxui/dom/elements.hpp>
#include <sstream>
#include <algorithm>

bool InputBuffer::Empty() const {
    return text_.empty();
}

const std::string& InputBuffer::Text() const {
    return text_;
}

void InputBuffer::Clear() {
    text_.clear();
    cursor_ = 0;
}

void InputBuffer::Insert(const std::string& text) {
    text_.insert(cursor_, text);
    cursor_ += text.size();
}

void InputBuffer::InsertNewline() {
    Insert("\n");
}

void InputBuffer::MoveLeft() {
    if (cursor_ > 0) {
        cursor_--;
    }
}

void InputBuffer::MoveRight() {
    if (cursor_ < text_.size()) {
        cursor_++;
    }
}

void InputBuffer::MoveUp(int width) {
    auto cursor = CursorVisualPosition(width);

    if (cursor.line <= 0) {
        return;
    }

    cursor_ = CursorIndexFromVisualPosition(
        cursor.line - 1,
        cursor.column,
        width
    );
}

void InputBuffer::MoveDown(int width) {
    auto cursor = CursorVisualPosition(width);

    cursor_ = CursorIndexFromVisualPosition(
        cursor.line + 1,
        cursor.column,
        width
    );
}

void InputBuffer::Backspace() {
    if (cursor_ == 0) {
        return;
    }

    text_.erase(cursor_ -1, 1);
    cursor_--;
}

std::pair<int, int> InputBuffer::CursorLineColumn() const {
    int line = 0;
    int column = 0;

    for (size_t i = 0; i < cursor_ && i < text_.size(); ++i) {
        if (text_[i] == '\n') {
            line++;
            column = 0;
        } else {
            column++;
        }
    }

    return {line, column};
}

size_t InputBuffer::CursorIndexFromLineColumn(int target_line, int target_column) const {
    int line = 0;
    int column = 0;

    for (size_t i = 0; i < text_.size(); ++i) {
        if (line == target_line && column == target_column) {
            return i;
        }

        if (text_[i] == '\n') {
            if (line == target_line) {
                return i;
            }

            line++;
            column = 0;
        } else {
            column++;
        }
    }

    return text_.size();
}

std::vector<InputBuffer::VisualLine> InputBuffer::BuildVisualLines(int width) const {
   width = std::max(1, width);

   std::vector<VisualLine> visual_lines;

   std::stringstream stream(text_);
   std::string logical_line;

   int logical_idx = 0;

   while (std::getline(stream, logical_line)) {
       if (logical_line.empty()) {
           visual_lines.push_back({logical_idx, 0, ""});
       } else {
           int offset = 0;

           while (offset < static_cast<int>(logical_line.size())) {
               int chunk_size = std::min(width, static_cast<int>(logical_line.size()) - offset);

               visual_lines.push_back({
                   logical_idx,
                   offset,
                   logical_line.substr(offset, chunk_size)
                });

               offset += chunk_size;
           }
       }

       logical_idx++;
   }

   if (text_.empty() || text_.back() == '\n') {
       visual_lines.push_back({logical_idx, 0, ""});
   }

   return visual_lines;
}

InputBuffer::VisualCursor InputBuffer::CursorVisualPosition(int width) const {
    auto [cursor_line, cursor_column] = CursorLineColumn();

    auto visual_lines = BuildVisualLines(width);

    for (int i = 0; i < static_cast<int>(visual_lines.size()); ++i) {
        const auto& line = visual_lines[i];

        if (line.logical_idx != cursor_line) {
            continue;
        }

        int start = line.char_offset;
        int end = start + static_cast<int>(line.text.size());

        bool last_visual_line =
            i + 1 >= static_cast<int>(visual_lines.size()) ||
            visual_lines[i + 1].logical_idx != cursor_line;

        if (cursor_column >= start &&
            (cursor_column < end || last_visual_line))
        {
            return {
                i,
                cursor_column - start
            };
        }
    }

    return {0, 0};
}

size_t InputBuffer::CursorIndexFromVisualPosition(
    int visual_line,
    int visual_column,
    int width
) const {
    auto visual_lines = BuildVisualLines(width);

    if (visual_lines.empty()) {
        return 0;
    }

    visual_line = std::max(
        0,
        std::min(
            visual_line,
            static_cast<int>(visual_lines.size()) - 1
        )
    );

    const auto& line = visual_lines[visual_line];

    int column = std::min(
        visual_column,
        static_cast<int>(line.text.size())
    );

    return CursorIndexFromLineColumn(
        line.logical_idx,
        line.char_offset + column
    );
}

