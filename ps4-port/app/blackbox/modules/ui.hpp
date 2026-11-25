#pragma once
#include <SDL2/SDL.h>
#include "catalog.hpp"
#include "storage.hpp"
#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

namespace blackbox {
class Ui {
public:
    Ui(SDL_Window* window, SDL_Renderer* renderer, Storage& storage);
    ~Ui();
    void process_event(const SDL_Event& event);
    void render();
    void move_selection(int delta);
    void activate();
    bool toggle_favorite();
    bool back();
    int selected_section() const { return selected_; }

private:
    enum class Page { Home, Catalog, Details, Favorites, Settings };
    void ensure_catalog_loaded();
    SDL_Texture* load_cover(const CatalogEntry& entry);
    std::vector<std::size_t> favorite_indices() const;
    void render_home();
    void render_catalog();
    void render_favorites();
    void render_details();
    void render_settings();
    void start_download(const CatalogEntry& entry);

    SDL_Window* window_;
    SDL_Renderer* renderer_;
    Storage& storage_;
    int selected_ = 0;
    int selected_setting_ = 0;
    int selected_detail_action_ = 0;
    Page page_ = Page::Home;
    Page details_origin_ = Page::Catalog;
    Catalog catalog_;
    bool catalog_loaded_ = false;
    std::size_t selected_item_ = 0;
    std::size_t selected_favorite_ = 0;
    std::size_t scroll_offset_ = 0;
    std::string download_message_;
    char search_query_[128] = {};
    std::string data_path_;
    std::unordered_map<std::string, SDL_Texture*> cover_cache_;
};
}
