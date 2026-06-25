#include "Markdown.hpp"
#include "markdown/parser.hpp"
#include "markdown/viewer.hpp"
#include <ftxui/dom/elements.hpp>
#include <memory>

Markdown::Markdown() {
    viewer_ = std::make_shared<markdown::Viewer> (
        markdown::make_cmark_parser()
    );

    viewer_->set_embed(true);
    viewer_->show_scrollbar(false);
    viewer_->set_theme(markdown::theme_colorful());
}

void Markdown::SetContent(const std::string& content) {
    viewer_->set_content(content);
}

ftxui::Element Markdown::Render() {
    return viewer_->component()->Render();
}

