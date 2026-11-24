#pragma once
#include <SDL2/SDL.h>
#include "catalog.hpp"
#include <cstddef>
#include <string>

namespace blackbox {
class Ui {
public:
    explicit Ui(SDL_Renderer* renderer) : renderer_(renderer) {}
    void render();
    void move_selection(int delta);
    void activate();
    bool back();
    int selected_section() const { return selected_; }

private:
    enum class Page { Home, Catalog, Details, Favorites, Settings };

    void ensure_catalog_loaded();
    void render_home();
    void render_catalog();
    void render_details();
    void render_empty_page(const std::string& title, const std::string& message);
    void draw_header(const std::string& title, const std::string& subtitle);
    void draw_footer(const std::string& hint);

    SDL_Renderer* renderer_;
    int selected_ = 0;
    Page page_ = Page::Home;
    Catalog catalog_;
    bool catalog_loaded_ = false;
    std::size_t selected_item_ = 0;
    std::size_t scroll_offset_ = 0;
};
}
