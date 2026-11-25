#include "ui.hpp"
#include "font.hpp"
#include <algorithm>
#include <cstdio>
#include <vector>

namespace blackbox {
namespace {
constexpr int kVisibleRows = 10;
std::string clipped(const std::string& value, std::size_t max) {
    if (value.size() <= max) return value;
    if (max < 4) return value.substr(0, max);
    return value.substr(0, max - 3) + "...";
}
std::string value_or(const std::string& value, const char* fallback) {
    return value.empty() ? fallback : value;
}
void fill(SDL_Renderer* renderer, const SDL_Rect& rect, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &rect);
}
}

std::vector<std::size_t> Ui::favorite_indices() const {
    std::vector<std::size_t> result;
    const auto& entries = catalog_.entries();
    for (std::size_t i = 0; i < entries.size(); ++i) {
        if (storage_.is_favorite(entries[i].id))
            result.push_back(i);
    }
    return result;
}

void Ui::ensure_catalog_loaded() {
    if (catalog_loaded_) return;
    char* base = SDL_GetBasePath();
    std::string path = "/app0/data/catalog.json";
    if (base) {
        path = std::string(base) + "data/catalog.json";
        SDL_free(base);
    }
    if (!catalog_.load(path) && path != "/app0/data/catalog.json")
        catalog_.load("/app0/data/catalog.json");
    catalog_loaded_ = true;
}

void Ui::move_selection(int delta) {
    if (delta == 0) return;
    if (page_ == Page::Home) {
        selected_ = (selected_ + delta + 3) % 3;
        return;
    }

    if (page_ == Page::Catalog && !catalog_.entries().empty()) {
        const int last = static_cast<int>(catalog_.entries().size() - 1);
        int next = static_cast<int>(selected_item_) + delta;
        next = std::max(0, std::min(last, next));
        selected_item_ = static_cast<std::size_t>(next);
        if (selected_item_ < scroll_offset_) scroll_offset_ = selected_item_;
        if (selected_item_ >= scroll_offset_ + kVisibleRows)
            scroll_offset_ = selected_item_ - kVisibleRows + 1;
    } else if (page_ == Page::Favorites) {
        const auto indices = favorite_indices();
        if (!indices.empty()) {
            int next = static_cast<int>(selected_favorite_) + delta;
            next = std::max(0, std::min(static_cast<int>(indices.size() - 1), next));
            selected_favorite_ = static_cast<std::size_t>(next);
            if (selected_favorite_ < scroll_offset_) scroll_offset_ = selected_favorite_;
            if (selected_favorite_ >= scroll_offset_ + kVisibleRows)
                scroll_offset_ = selected_favorite_ - kVisibleRows + 1;
        }
    }
}

void Ui::activate() {
    if (page_ == Page::Home) {
        if (selected_ == 0) {
            ensure_catalog_loaded();
            selected_item_ = 0;
            scroll_offset_ = 0;
            page_ = Page::Catalog;
        } else if (selected_ == 1) {
            ensure_catalog_loaded();
            selected_favorite_ = 0;
            scroll_offset_ = 0;
            page_ = Page::Favorites;
        } else {
            page_ = Page::Settings;
        }
    } else if (page_ == Page::Catalog && !catalog_.entries().empty()) {
        details_origin_ = Page::Catalog;
        page_ = Page::Details;
    } else if (page_ == Page::Favorites) {
        const auto indices = favorite_indices();
        if (selected_favorite_ < indices.size()) {
            selected_item_ = indices[selected_favorite_];
            details_origin_ = Page::Favorites;
            page_ = Page::Details;
        }
    }
}

bool Ui::toggle_favorite() {
    ensure_catalog_loaded();
    if (page_ != Page::Catalog && page_ != Page::Details && page_ != Page::Favorites)
        return false;

    const auto& entries = catalog_.entries();
    if (page_ == Page::Favorites) {
        const auto before = favorite_indices();
        if (selected_favorite_ >= before.size()) return false;
        selected_item_ = before[selected_favorite_];
    }
    if (selected_item_ >= entries.size()) return false;
    const bool added = storage_.toggle_favorite(entries[selected_item_].id);

    if (page_ == Page::Favorites) {
        const auto indices = favorite_indices();
        if (indices.empty()) {
            selected_favorite_ = 0;
            scroll_offset_ = 0;
        } else if (selected_favorite_ >= indices.size()) {
            selected_favorite_ = indices.size() - 1;
        }
    }
    return added;
}

bool Ui::back() {
    if (page_ == Page::Details) {
        page_ = details_origin_;
        return true;
    }
    if (page_ != Page::Home) {
        page_ = Page::Home;
        return true;
    }
    return false;
}

void Ui::draw_header(const std::string& title, const std::string& subtitle) {
    SDL_Rect header{0, 0, 1920, 150};
    fill(renderer_, header, SDL_Color{17, 23, 36, 255});
    SDL_Rect accent{80, 72, 9, 50};
    fill(renderer_, accent, SDL_Color{0, 190, 255, 255});
    draw_text(renderer_, 115, 45, 7, title, SDL_Color{238, 245, 255, 255});
    draw_text(renderer_, 116, 112, 2, subtitle, SDL_Color{105, 145, 175, 255});
}

void Ui::draw_footer(const std::string& hint) {
    SDL_Rect line{80, 965, 1760, 2};
    fill(renderer_, line, SDL_Color{45, 58, 76, 255});
    draw_text(renderer_, 82, 990, 2, hint, SDL_Color{140, 165, 190, 255});
}

void Ui::render_home() {
    draw_header("BLACKBOX PS4", "RIDAGAMES  /  NATIVE CONSOLE INTERFACE");
    const char* labels[3] = {"CATALOG", "FAVORITES", "SETTINGS"};
    const char* descriptions[3] = {"BROWSE PS4 METADATA", "SAVED ENTRIES", "APP PREFERENCES"};
    for (int i = 0; i < 3; ++i) {
        SDL_Rect card{80 + i * 590, 250, 540, 470};
        fill(renderer_, card, i == selected_
            ? SDL_Color{27, 53, 76, 255} : SDL_Color{18, 27, 42, 255});
        SDL_Rect marker{card.x, card.y, card.w, 8};
        fill(renderer_, marker, i == selected_
            ? SDL_Color{0, 190, 255, 255} : SDL_Color{48, 66, 88, 255});
        SDL_Rect inner{card.x + 40, card.y + 70, card.w - 80, 250};
        fill(renderer_, inner, SDL_Color{13, 19, 30, 255});
        draw_text(renderer_, card.x + 42, card.y + 350, 5, labels[i],
            SDL_Color{235, 242, 252, 255});
        draw_text(renderer_, card.x + 42, card.y + 400, 2, descriptions[i],
            SDL_Color{115, 153, 182, 255});
    }
    draw_footer("D-PAD / LEFT STICK SELECT     CROSS OPEN     CIRCLE BACK");
}

void Ui::render_catalog() {
    const auto& entries = catalog_.entries();
    draw_header("PS4 CATALOG", std::to_string(entries.size()) + " METADATA ENTRIES");
    SDL_Rect list_panel{80, 190, 1080, 735};
    SDL_Rect detail_panel{1190, 190, 650, 735};
    fill(renderer_, list_panel, SDL_Color{15, 22, 34, 255});
    fill(renderer_, detail_panel, SDL_Color{15, 22, 34, 255});
    draw_text(renderer_, 112, 215, 3, "AVAILABLE RELEASES",
        SDL_Color{110, 190, 220, 255});

    if (entries.empty()) {
        draw_text(renderer_, 125, 330, 4, "CATALOG NOT AVAILABLE",
            SDL_Color{255, 180, 120, 255});
        draw_text(renderer_, 125, 390, 2, "CHECK THE PACKAGED DATA FILE",
            SDL_Color{160, 175, 195, 255});
    } else {
        const std::size_t end = std::min(entries.size(), scroll_offset_ + kVisibleRows);
        for (std::size_t i = scroll_offset_; i < end; ++i) {
            const int row = static_cast<int>(i - scroll_offset_);
            const int y = 265 + row * 62;
            SDL_Rect item{105, y, 1030, 54};
            fill(renderer_, item, i == selected_item_
                ? SDL_Color{28, 63, 86, 255} : SDL_Color{20, 29, 43, 255});
            const SDL_Color title_color = i == selected_item_
                ? SDL_Color{245, 250, 255, 255} : SDL_Color{190, 205, 220, 255};
            draw_text(renderer_, 125, y + 17, 2, clipped(entries[i].title, 48), title_color);
            draw_text(renderer_, 850, y + 17, 2, entries[i].title_id,
                SDL_Color{105, 190, 220, 255});
            if (storage_.is_favorite(entries[i].id))
                draw_text(renderer_, 1110, y + 17, 2, "*", SDL_Color{255, 205, 95, 255});
        }

        const auto& entry = entries[selected_item_];
        draw_text(renderer_, 1230, 225, 3, "SELECTED ENTRY",
            SDL_Color{110, 190, 220, 255});
        draw_text(renderer_, 1230, 285, 3, clipped(entry.title, 45),
            SDL_Color{240, 246, 255, 255});
        draw_text(renderer_, 1230, 365, 2, "TITLE ID: " + entry.title_id,
            SDL_Color{170, 190, 210, 255});
        draw_text(renderer_, 1230, 415, 2, "REGION: " + value_or(entry.region, "N/A"),
            SDL_Color{170, 190, 210, 255});
        draw_text(renderer_, 1230, 465, 2, "VERSION: " + value_or(entry.version, "N/A"),
            SDL_Color{170, 190, 210, 255});
        draw_text(renderer_, 1230, 515, 2, "PUBLISHER: " + clipped(value_or(entry.publisher, "N/A"), 35),
            SDL_Color{170, 190, 210, 255});
        draw_text(renderer_, 1230, 565, 2, "RELEASE: " + value_or(entry.release_date, "N/A"),
            SDL_Color{170, 190, 210, 255});
        if (entry.size_bytes > 0) {
            const auto size_gb = static_cast<unsigned long long>(entry.size_bytes / 1073741824ULL);
            draw_text(renderer_, 1230, 615, 2, "SIZE: " + std::to_string(size_gb) + " GB",
                SDL_Color{170, 190, 210, 255});
        } else {
            draw_text(renderer_, 1230, 615, 2, "SIZE: N/A",
                SDL_Color{170, 190, 210, 255});
        }
        draw_text(renderer_, 1230, 720, 2,
            storage_.is_favorite(entry.id) ? "SAVED TO FAVORITES" : "NOT IN FAVORITES",
            storage_.is_favorite(entry.id) ? SDL_Color{255, 205, 95, 255} : SDL_Color{80, 210, 155, 255});
        draw_text(renderer_, 1230, 760, 2, "METADATA ONLY",
            SDL_Color{125, 145, 165, 255});
    }
    draw_footer("UP / DOWN SCROLL     CROSS DETAILS     SQUARE SAVE FAVORITE     CIRCLE BACK");
}

void Ui::render_favorites() {
    const auto indices = favorite_indices();
    draw_header("FAVORITES", std::to_string(indices.size()) + " SAVED ENTRIES");
    SDL_Rect panel{80, 190, 1760, 735};
    fill(renderer_, panel, SDL_Color{15, 22, 34, 255});

    if (indices.empty()) {
        draw_text(renderer_, 140, 390, 4, "NO FAVORITES SAVED YET",
            SDL_Color{200, 215, 230, 255});
        draw_text(renderer_, 140, 465, 2, "OPEN THE CATALOG AND PRESS SQUARE TO SAVE A GAME",
            SDL_Color{130, 165, 190, 255});
    } else {
        const std::size_t end = std::min(indices.size(), scroll_offset_ + kVisibleRows);
        for (std::size_t i = scroll_offset_; i < end; ++i) {
            const auto& entry = catalog_.entries()[indices[i]];
            const int y = 250 + static_cast<int>(i - scroll_offset_) * 62;
            SDL_Rect item{110, y, 1690, 54};
            fill(renderer_, item, i == selected_favorite_
                ? SDL_Color{28, 63, 86, 255} : SDL_Color{20, 29, 43, 255});
            draw_text(renderer_, 140, y + 17, 2, clipped(entry.title, 72),
                i == selected_favorite_ ? SDL_Color{245, 250, 255, 255} : SDL_Color{190, 205, 220, 255});
            draw_text(renderer_, 1500, y + 17, 2, entry.title_id,
                SDL_Color{105, 190, 220, 255});
        }
    }
    draw_footer("UP / DOWN SELECT     CROSS DETAILS     SQUARE REMOVE     CIRCLE BACK");
}

void Ui::render_details() {
    const auto& entries = catalog_.entries();
    if (entries.empty() || selected_item_ >= entries.size()) {
        render_empty_page("ENTRY DETAILS", "NO CATALOG ENTRY SELECTED");
        return;
    }
    const auto& entry = entries[selected_item_];
    draw_header("ENTRY DETAILS", "PS4 CATALOG METADATA");
    SDL_Rect panel{100, 205, 1720, 700};
    fill(renderer_, panel, SDL_Color{15, 22, 34, 255});
    draw_text(renderer_, 150, 270, 5, clipped(entry.title, 48),
        SDL_Color{240, 246, 255, 255});
    draw_text(renderer_, 150, 390, 3, "TITLE ID: " + entry.title_id,
        SDL_Color{110, 190, 220, 255});
    draw_text(renderer_, 150, 455, 3, "REGION: " + value_or(entry.region, "N/A"),
        SDL_Color{185, 200, 218, 255});
    draw_text(renderer_, 150, 520, 3, "VERSION: " + value_or(entry.version, "N/A"),
        SDL_Color{185, 200, 218, 255});
    draw_text(renderer_, 150, 585, 3, "PUBLISHER: " + clipped(value_or(entry.publisher, "N/A"), 45),
        SDL_Color{185, 200, 218, 255});
    draw_text(renderer_, 150, 650, 3, "RELEASE: " + value_or(entry.release_date, "N/A"),
        SDL_Color{185, 200, 218, 255});
    draw_text(renderer_, 150, 715, 3, "GAME ID: " + clipped(entry.game_id, 55),
        SDL_Color{185, 200, 218, 255});
    draw_text(renderer_, 150, 805, 2,
        storage_.is_favorite(entry.id) ? "SQUARE: REMOVE FROM FAVORITES" : "SQUARE: ADD TO FAVORITES",
        SDL_Color{255, 205, 95, 255});
    draw_footer("CIRCLE BACK");
}

void Ui::render_empty_page(const std::string& title, const std::string& message) {
    draw_header(title, "BLACKBOX PS4");
    SDL_Rect panel{100, 230, 1720, 600};
    fill(renderer_, panel, SDL_Color{15, 22, 34, 255});
    draw_text(renderer_, 160, 400, 4, message, SDL_Color{200, 215, 230, 255});
    draw_footer("CIRCLE BACK");
}

void Ui::render() {
    SDL_SetRenderDrawColor(renderer_, 9, 12, 20, 255);
    SDL_RenderClear(renderer_);
    switch (page_) {
        case Page::Home: render_home(); break;
        case Page::Catalog: render_catalog(); break;
        case Page::Details: render_details(); break;
        case Page::Favorites: render_favorites(); break;
        case Page::Settings:
            render_empty_page("SETTINGS", "SETTINGS PANEL IN DEVELOPMENT");
            break;
    }
    SDL_RenderPresent(renderer_);
}
}
