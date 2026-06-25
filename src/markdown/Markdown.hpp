#pragma once

#include <ftxui/component/component_base.hpp>
#include <ftxui/dom/elements.hpp>
#include <markdown/parser.hpp>
#include <markdown/viewer.hpp>
#include <memory>

class Markdown {
    public:
        Markdown();
        void SetContent(const std::string& content);
        ftxui::Element Render();

    private:
        std::shared_ptr<markdown::Viewer> viewer_;
};
