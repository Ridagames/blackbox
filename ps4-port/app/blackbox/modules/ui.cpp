#include "ui.hpp"
#include "font.hpp"
namespace blackbox {
void Ui::move_selection(int delta) {
    if (delta == 0) return;
    selected_ = (selected_ + delta + 3) % 3;
}
void Ui::render(int selected) {
    selected_ = selected;
    SDL_SetRenderDrawColor(renderer_, 9, 12, 20, 255);
    SDL_RenderClear(renderer_);
    SDL_Rect header{0, 0, 1920, 150};
    SDL_SetRenderDrawColor(renderer_, 17, 23, 36, 255);
    SDL_RenderFillRect(renderer_, &header);
    SDL_Rect accent{80, 72, 9, 50};
    SDL_SetRenderDrawColor(renderer_, 0, 190, 255, 255);
    SDL_RenderFillRect(renderer_, &accent);
    draw_text(renderer_, 115, 60, 8, "BLACKBOX PS4", SDL_Color{238, 245, 255, 255});
    draw_text(renderer_, 116, 125, 3, "RIDAGAMES", SDL_Color{105, 145, 175, 255});

    const char* labels[3] = {"CATALOG", "FAVORITES", "SETTINGS"};
    const char* descriptions[3] = {"BROWSE METADATA", "SAVED ENTRIES", "APP PREFERENCES"};
    for (int i = 0; i < 3; ++i) {
        SDL_Rect card{80 + i * 590, 250, 540, 470};
        SDL_SetRenderDrawColor(renderer_, i == selected_ ? 27 : 18,
            i == selected_ ? 53 : 27, i == selected_ ? 76 : 42, 255);
        SDL_RenderFillRect(renderer_, &card);
        SDL_Rect marker{card.x, card.y, card.w, 8};
        SDL_SetRenderDrawColor(renderer_, i == selected_ ? 0 : 48,
            i == selected_ ? 190 : 66, i == selected_ ? 255 : 88, 255);
        SDL_RenderFillRect(renderer_, &marker);
        SDL_Rect inner{card.x + 40, card.y + 70, card.w - 80, 250};
        SDL_SetRenderDrawColor(renderer_, 13, 19, 30, 255);
        SDL_RenderFillRect(renderer_, &inner);
        draw_text(renderer_, card.x + 42, card.y + 350, 5, labels[i],
            SDL_Color{235, 242, 252, 255});
        draw_text(renderer_, card.x + 42, card.y + 400, 2, descriptions[i],
            SDL_Color{115, 153, 182, 255});
    }

    SDL_Rect footer{80, 900, 1760, 2};
    SDL_SetRenderDrawColor(renderer_, 45, 58, 76, 255);
    SDL_RenderFillRect(renderer_, &footer);
    draw_text(renderer_, 82, 930, 3, "D-PAD / STICK SELECT   CROSS OPEN   CIRCLE BACK",
        SDL_Color{140, 165, 190, 255});
    SDL_RenderPresent(renderer_);
}
}
