#include "ui.hpp"
namespace blackbox {
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

    for (int i = 0; i < 3; ++i) {
        SDL_Rect card{80 + i * 590, 250, 540, 470};
        SDL_SetRenderDrawColor(renderer_, i == selected_ ? 27 : 18,
            i == selected_ ? 53 : 27, i == selected_ ? 76 : 42, 255);
        SDL_RenderFillRect(renderer_, &card);
        SDL_Rect marker{card.x, card.y, card.w, 8};
        SDL_SetRenderDrawColor(renderer_, i == selected_ ? 0 : 48,
            i == selected_ ? 190 : 66, i == selected_ ? 255 : 88, 255);
        SDL_RenderFillRect(renderer_, &marker);
    }

    SDL_Rect footer{80, 900, 1760, 2};
    SDL_SetRenderDrawColor(renderer_, 45, 58, 76, 255);
    SDL_RenderFillRect(renderer_, &footer);
    SDL_RenderPresent(renderer_);
}
}
