#include "storage.hpp"
#include <SDL.h>
#include <cstdio>
namespace blackbox {
bool Storage::load() {
    char* base = SDL_GetPrefPath("Ridagames", "BlackBoxPS4");
    if (!base) return false;
    path_ = base;
    SDL_free(base);
    path_ += "settings.cfg";
    FILE* file = std::fopen(path_.c_str(), "r");
    if (!file) return true;
    int value = 0;
    const bool parsed = std::fscanf(file, "%d", &value) == 1;
    const bool closed = std::fclose(file) == 0;
    if (parsed && value >= 0 && value <= 2) last_section_ = value;
    return parsed && closed;
}
bool Storage::save() const {
    if (path_.empty()) return false;
    FILE* file = std::fopen(path_.c_str(), "w");
    if (!file) return false;
    const int written = std::fprintf(file, "%d\n", last_section_);
    const bool closed = std::fclose(file) == 0;
    return written > 0 && closed;
}
}
