#include "ui.hpp"
#include "font.hpp"
#include "downloader.hpp"
#include <algorithm>
#include <cstdio>
#include <sys/stat.h>
#include <cstdint>
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

Ui::Ui(SDL_Renderer* renderer, Storage& storage)
    : renderer_(renderer), storage_(storage) {
    char* base = SDL_GetBasePath();
    if (base) {
        data_path_ = base;
        SDL_free(base);
    } else {
        data_path_ = "/app0/";
    }
    data_path_ += "data/";

    auto load_bitmap = [this](const std::string& path) -> SDL_Texture* {
        SDL_Surface* surface = SDL_LoadBMP(path.c_str());
        if (!surface) return nullptr;
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
        SDL_FreeSurface(surface);
        return texture;
    };
    background_texture_ = load_bitmap(data_path_ + "ui-background.bmp");
    logo_texture_ = load_bitmap(data_path_ + "ui-logo.bmp");
}

Ui::~Ui() {
    if (cover_texture_) SDL_DestroyTexture(cover_texture_);
    if (logo_texture_) SDL_DestroyTexture(logo_texture_);
    if (background_texture_) SDL_DestroyTexture(background_texture_);
}

void Ui::draw_texture(SDL_Texture* texture, const SDL_Rect& destination, Uint8 alpha) {
    if (!texture) return;
    SDL_SetTextureAlphaMod(texture, alpha);
    SDL_RenderCopy(renderer_, texture, nullptr, &destination);
    SDL_SetTextureAlphaMod(texture, 255);
}

void Ui::load_cover(const CatalogEntry& entry) {
    if (loaded_cover_id_ == entry.title_id) return;
    if (cover_texture_) {
        SDL_DestroyTexture(cover_texture_);
        cover_texture_ = nullptr;
    }
    loaded_cover_id_ = entry.title_id;
    std::string safe_id;
    for (unsigned char ch : entry.title_id) {
        if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
            (ch >= '0' && ch <= '9') || ch == '_' || ch == '-')
            safe_id.push_back(static_cast<char>(ch));
    }
    if (safe_id.empty()) return;
    SDL_Surface* surface = SDL_LoadBMP((data_path_ + "covers/" + safe_id + ".bmp").c_str());
    if (!surface) return;
    cover_texture_ = SDL_CreateTextureFromSurface(renderer_, surface);
    SDL_FreeSurface(surface);
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
    if (page_ == Page::Settings) {
        selected_setting_ = (selected_setting_ + delta + 2) % 2;
        return;
    }
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
    if (page_ == Page::Details) {
        const auto& entries = catalog_.entries();
        if (selected_item_ < entries.size()) {
            const CatalogEntry& entry = entries[selected_item_];
            if (entry.commercial || entry.content_type != "homebrew") {
                download_message_ = "Jeux commerciaux détecté ! Téléchargement annulé.";
                return;
            }
            if (entry.download_url.empty() || entry.filename.empty()) {
                download_message_ = "Source homebrew indisponible.";
                return;
            }

            char* pref = SDL_GetPrefPath("Ridagames", "BlackBoxPS4");
            if (!pref) {
                download_message_ = "Impossible de créer le dossier de téléchargement.";
                return;
            }
            const std::string directory = std::string(pref) + "downloads/";
            SDL_free(pref);
            mkdir(directory.c_str(), 0777);

            std::string filename = entry.filename;
            for (char& ch : filename) {
                if (ch == '/' || ch == '\\\\' || ch == ':' || ch == '\\n' || ch == '\\r')
                    ch = '_';
            }
            DownloadEntry request;
            request.url = entry.download_url;
            request.filename = filename;
            request.output_path = directory + filename;
            request.commercial = entry.commercial;
            request.homebrew = entry.content_type == "homebrew";

            download_message_ = "Connexion à la source homebrew...";
            render();
            const bool success = download_pkg(request, [this](const DownloadStatus& status) {
                if (status.total > 0) {
                    const unsigned int percent = static_cast<unsigned int>(
                        (status.downloaded * 100ULL) / status.total);
                    download_message_ = "Téléchargement : " + std::to_string(percent) + "%";
                } else if (status.downloaded > 0) {
                    download_message_ = "Téléchargement : " +
                        std::to_string(status.downloaded / 1048576ULL) + " Mo";
                } else if (!status.error.empty()) {
                    download_message_ = status.error;
                }
                render();
                SDL_PumpEvents();
            });
            download_message_ = success
                ? "Téléchargement terminé : " + filename
                : "Échec du téléchargement : source ou réseau.";
        }
    } else if (page_ == Page::Home) {
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
            selected_setting_ = 0;
            page_ = Page::Settings;
        }
    } else if (page_ == Page::Settings) {
        if (selected_setting_ == 0)
            storage_.set_accent_theme(storage_.accent_theme() + 1);
        else
            storage_.set_show_title_ids(!storage_.show_title_ids());
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

    if (page_ == Page::Favorites ||
        (page_ == Page::Details && details_origin_ == Page::Favorites)) {
        const auto indices = favorite_indices();
        if (indices.empty()) {
            selected_favorite_ = 0;
            scroll_offset_ = 0;
        } else {
            if (selected_favorite_ >= indices.size())
                selected_favorite_ = indices.size() - 1;
            const std::size_t max_scroll = indices.size() > kVisibleRows
                ? indices.size() - kVisibleRows : 0;
            if (scroll_offset_ > max_scroll) scroll_offset_ = max_scroll;
            if (selected_favorite_ < scroll_offset_)
                scroll_offset_ = selected_favorite_;
            if (selected_favorite_ >= scroll_offset_ + kVisibleRows)
                scroll_offset_ = selected_favorite_ - kVisibleRows + 1;
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

SDL_Color Ui::accent_color() const {
    switch (storage_.accent_theme()) {
        case 1: return SDL_Color{80, 220, 160, 255};
        case 2: return SDL_Color{190, 125, 255, 255};
        default: return SDL_Color{0, 190, 255, 255};
    }
}

void Ui::draw_header(const std::string& title, const std::string& subtitle) {
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
    SDL_Rect header{0, 0, 1920, 160};
    fill(renderer_, header, SDL_Color{7, 12, 23, 225});
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_NONE);
    if (logo_texture_) draw_texture(logo_texture_, SDL_Rect{70, 18, 310, 124});
    SDL_Rect accent{410, 48, 7, 70};
    fill(renderer_, accent, accent_color());
    draw_text(renderer_, 445, 42, 6, title, SDL_Color{238, 245, 255, 255});
    draw_text(renderer_, 447, 100, 2, subtitle, SDL_Color{125, 165, 195, 255});
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
            if (storage_.show_title_ids())
                draw_text(renderer_, 850, y + 17, 2, entries[i].title_id, accent_color());
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
            if (storage_.show_title_ids())
                draw_text(renderer_, 1500, y + 17, 2, entry.title_id, accent_color());
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
    fill(renderer_, panel, SDL_Color{11, 18, 31, 238});
    load_cover(entry);
    if (cover_texture_)
        draw_texture(cover_texture_, SDL_Rect{150, 310, 330, 330});
    else {
        SDL_Rect cover_fallback{150, 310, 330, 330};
        fill(renderer_, cover_fallback, SDL_Color{24, 39, 57, 255});
        draw_text(renderer_, 205, 450, 3, "NO COVER", SDL_Color{120, 155, 180, 255});
    }
    draw_text(renderer_, 535, 265, 5, clipped(entry.title, 48),
        SDL_Color{240, 246, 255, 255});
    draw_text(renderer_, 535, 365, 3, "TITLE ID: " + entry.title_id,
        SDL_Color{110, 190, 220, 255});
    draw_text(renderer_, 535, 430, 3, "REGION: " + value_or(entry.region, "N/A"),
        SDL_Color{185, 200, 218, 255});
    draw_text(renderer_, 535, 495, 3, "VERSION: " + value_or(entry.version, "N/A"),
        SDL_Color{185, 200, 218, 255});
    draw_text(renderer_, 535, 560, 3, "PUBLISHER: " + clipped(value_or(entry.publisher, "N/A"), 45),
        SDL_Color{185, 200, 218, 255});
    draw_text(renderer_, 535, 625, 3, "RELEASE: " + value_or(entry.release_date, "N/A"),
        SDL_Color{185, 200, 218, 255});
    draw_text(renderer_, 535, 690, 3, "GAME ID: " + clipped(entry.game_id, 55),
        SDL_Color{185, 200, 218, 255});
    draw_text(renderer_, 535, 755, 2,

        storage_.is_favorite(entry.id) ? "SQUARE: REMOVE FROM FAVORITES" : "SQUARE: ADD TO FAVORITES",
        SDL_Color{255, 205, 95, 255});

    // The download action is visible on every game details page.
    SDL_Rect download_button{150, 820, 390, 58};
    fill(renderer_, download_button, SDL_Color{0, 105, 155, 255});
    draw_text(renderer_, 185, 840, 3, "DOWNLOAD", SDL_Color{245, 250, 255, 255});
    if (!download_message_.empty()) {
        const bool blocked = entry.commercial;
        draw_text(renderer_, 580, 840, 2, clipped(download_message_, 76),
            blocked ? SDL_Color{255, 145, 130, 255} : SDL_Color{255, 205, 95, 255});
    }
    draw_footer("CROSS DOWNLOAD     SQUARE FAVORITE     CIRCLE BACK");
}

void Ui::render_settings() {
    draw_header("SETTINGS", "PREFERENCES ARE SAVED AUTOMATICALLY");
    SDL_Rect panel{100, 205, 1720, 700};
    fill(renderer_, panel, SDL_Color{15, 22, 34, 255});

    const char* theme_names[] = {"CYAN", "MINT", "VIOLET"};
    const char* labels[] = {"INTERFACE ACCENT", "SHOW TITLE IDS"};
    const std::string values[] = {
        theme_names[storage_.accent_theme()],
        storage_.show_title_ids() ? "ON" : "OFF"
    };
    for (int i = 0; i < 2; ++i) {
        const int y = 320 + i * 210;
        SDL_Rect row{150, y, 1620, 150};
        fill(renderer_, row, i == selected_setting_
            ? SDL_Color{28, 63, 86, 255} : SDL_Color{20, 29, 43, 255});
        SDL_Rect marker{150, y, 8, 150};
        fill(renderer_, marker, i == selected_setting_ ? accent_color()
            : SDL_Color{48, 66, 88, 255});
        draw_text(renderer_, 195, y + 42, 4, labels[i],
            SDL_Color{235, 242, 252, 255});
        draw_text(renderer_, 1330, y + 52, 4, values[i], accent_color());
    }
    draw_footer("UP / DOWN SELECT     CROSS CHANGE VALUE     CIRCLE BACK");
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
    if (background_texture_)
        draw_texture(background_texture_, SDL_Rect{0, 0, 1920, 1080});
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
    fill(renderer_, SDL_Rect{0, 0, 1920, 1080}, SDL_Color{5, 9, 17, 178});
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_NONE);
    switch (page_) {
        case Page::Home: render_home(); break;
        case Page::Catalog: render_catalog(); break;
        case Page::Details: render_details(); break;
        case Page::Favorites: render_favorites(); break;
        case Page::Settings: render_settings(); break;
    }
    SDL_RenderPresent(renderer_);
}
}
