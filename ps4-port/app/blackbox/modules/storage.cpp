#include "storage.hpp"
#include <SDL.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>

namespace blackbox {
bool Storage::load() {
    char* base = SDL_GetPrefPath("Ridagames", "BlackBoxPS4");
    if (!base) return false;
    path_ = base;
    SDL_free(base);
    path_ += "settings.cfg";

    std::ifstream file(path_.c_str());
    if (!file) return true;

    std::string line;
    if (std::getline(file, line)) {
        char* end = nullptr;
        const long value = std::strtol(line.c_str(), &end, 10);
        if (end != line.c_str() && value >= 0 && value <= 2)
            last_section_ = static_cast<int>(value);
    }

    favorites_.clear();
    while (std::getline(file, line)) {
        if (line == "@show_title_ids=0") {
            show_title_ids_ = false;
            continue;
        }
        if (line == "@show_title_ids=1") {
            show_title_ids_ = true;
            continue;
        }
        if (line.rfind("@accent_theme=", 0) == 0) {
            const int value = std::atoi(line.c_str() + 14);
            if (value >= 0 && value <= 2) accent_theme_ = value;
            continue;
        }
        if (line.empty() || line.size() > 64 || line[0] == '@') continue;
        if (std::find(favorites_.begin(), favorites_.end(), line) == favorites_.end())
            favorites_.push_back(line);
    }
    return !file.bad();
}

bool Storage::save() const {
    if (path_.empty()) return false;
    FILE* file = std::fopen(path_.c_str(), "w");
    if (!file) return false;

    bool ok = std::fprintf(file, "%d\n", last_section_) > 0;
    if (std::fprintf(file, "@accent_theme=%d\n", accent_theme_) < 0) ok = false;
    if (std::fprintf(file, "@show_title_ids=%d\n", show_title_ids_ ? 1 : 0) < 0) ok = false;
    for (const auto& id : favorites_) {
        if (std::fprintf(file, "%s\n", id.c_str()) < 0) ok = false;
    }
    if (std::fclose(file) != 0) ok = false;
    return ok;
}

bool Storage::is_favorite(const std::string& id) const {
    return std::find(favorites_.begin(), favorites_.end(), id) != favorites_.end();
}

bool Storage::toggle_favorite(const std::string& id) {
    if (id.empty()) return false;
    const auto found = std::find(favorites_.begin(), favorites_.end(), id);
    if (found != favorites_.end()) {
        favorites_.erase(found);
        return false;
    }
    favorites_.push_back(id);
    return true;
}
}
