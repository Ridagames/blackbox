#include "modules/controller.hpp"
#include "modules/storage.hpp"
#include "modules/ui.hpp"
#include <SDL.h>
#include <cstdio>

int main(int, char**) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        std::fprintf(stderr, "BLACKBOX: SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
    SDL_Window* window = SDL_CreateWindow("BLACKBOX PS4",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1920, 1080, SDL_WINDOW_SHOWN);
    if (!window) { SDL_Quit(); return 2; }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer) { SDL_DestroyWindow(window); SDL_Quit(); return 3; }

    {
    blackbox::Controller controller;
    blackbox::Storage storage;
    storage.load();
    blackbox::Ui ui(window, renderer, storage);
    // Catalogue is the startup screen; begin on its first title.

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            ui.process_event(event);
            controller.handle(event);
        }
        controller.poll();
        ui.controller_input(controller.navigation_delta(), controller.confirm_pressed(), controller.exit_requested(), controller.favorite_pressed());
        if (controller.exit_requested() && !ui.back()) running = false;
        if (controller.favorite_pressed()) {
            ui.toggle_favorite();
            storage.save();
        }
        ui.render();
        SDL_Delay(8);
    }
    storage.set_last_section(ui.selected_section());
    storage.save();
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
