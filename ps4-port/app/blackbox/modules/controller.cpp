#include "controller.hpp"
namespace blackbox {
Controller::Controller() {
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) {
            game_controller_ = SDL_GameControllerOpen(i);
            if (game_controller_) break;
        }
    }
}
Controller::~Controller() {
    if (game_controller_) SDL_GameControllerClose(game_controller_);
}
void Controller::handle(const SDL_Event& event) {
    confirm_ = false;
    navigation_delta_ = 0;
    if (event.type == SDL_CONTROLLERBUTTONDOWN) {
        if (event.cbutton.button == SDL_CONTROLLER_BUTTON_A) confirm_ = true;
        if (event.cbutton.button == SDL_CONTROLLER_BUTTON_B ||
            event.cbutton.button == SDL_CONTROLLER_BUTTON_BACK) exit_ = true;
        if (event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_LEFT) navigation_delta_ = -1;
        if (event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT) navigation_delta_ = 1;
    }
    if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_SPACE)
            confirm_ = true;
        if (event.key.keysym.sym == SDLK_ESCAPE) exit_ = true;
        if (event.key.keysym.sym == SDLK_LEFT) navigation_delta_ = -1;
        if (event.key.keysym.sym == SDLK_RIGHT) navigation_delta_ = 1;
    }
}
}
