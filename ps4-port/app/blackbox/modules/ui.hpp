#pragma once
#include <SDL2/SDL.h>
#include "catalog.hpp"
#include "storage.hpp"
#include <cstddef>
#include <string>
#include <vector>

namespace blackbox {
class Ui {
public:
    Ui(SDL_Renderer* renderer, Storage& storage);
    ~Ui();
    void render();
    void move_selection(int delta);
    void activate();
    bool toggle_favorite();
    bool back();
    int selected_section() const { return selected_; }

private:
    enum class Page { Home, Catalog, Details, Favorites, Settings };

    void ensure_catalog_loaded();
    void load_cover(const CatalogEntry& entry);
    void draw_texture(SDL_Texture* texture, const SDL_Rect& destination, Uint8 alpha = 255);
    std::vector<std::size_t> favorite_indices() const;
    SDL_Color accent_color() const;
    void render_home();
    void render_catalog();
    void render_favorites();
    void render_details();
    void render_settings();
    void render_empty_page(const std::string& title, const std::string& message);
    void draw_header(const std::string& title, const std::string& subtitle);
    void draw_footer(const std::string& hint);

    SDL_Renderer* renderer_;
    Storage& storage_;
    int selected_ = 0;
    int selected_setting_ = 0;
    Page page_ = Page::Home;
    Page details_origin_ = Page::Catalog;
    Catalog catalog_;
    bool catalog_loaded_ = false;
    std::size_t selected_item_ = 0;
    std::size_t selected_favorite_ = 0;
    std::size_t scroll_offset_ = 0;
    std::string download_message_;
    std::string data_path_;
    std::string loaded_cover_id_;
    SDL_Texture* background_texture_ = nullptr;
    SDL_Texture* logo_texture_ = nullptr;
    SDL_Texture* cover_texture_ = nullptr;
};
}
