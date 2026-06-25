#pragma once

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

class InputBuffer {
    public:
        struct VisualLine {
            int logical_idx;
            int char_offset;
            std::string text;
        };

        struct VisualCursor {
            int line = 0;
            int column = 0;
        };

        void Insert(const std::string& text);
        void InsertNewline();
        void Backspace();
        void MoveLeft();
        void MoveRight();
        void MoveUp(int width);
        void MoveDown(int width);
        void Clear();

        bool Empty() const;
        const std::string& Text() const;

        std::vector<VisualLine> BuildVisualLines(int width) const;
        VisualCursor CursorVisualPosition(int width) const;

    private:
        std::pair<int, int> CursorLineColumn() const;
        size_t CursorIndexFromLineColumn(int target_line, int target_column) const;
        size_t CursorIndexFromVisualPosition(int visual_line, int visual_column, int width) const;

        std::string text_;
        size_t cursor_ = 0;
};

