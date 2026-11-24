#include "controller.hpp"
namespace blackbox {
void Controller::handle(const SDL_Event& event) {
    confirm_ = false;
    if (event.type == SDL_CONTROLLERBUTTONDOWN) {
        if (event.cbutton.button == SDL_CONTROLLER_BUTTON_A) confirm_ = true;
        if (event.cbutton.button == SDL_CONTROLLER_BUTTON_B ||
            event.cbutton.button == SDL_CONTROLLER_BUTTON_BACK) exit_ = true;
    }
    if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_SPACE)
            confirm_ = true;
        if (event.key.keysym.sym == SDLK_ESCAPE) exit_ = true;
    }
}
}
