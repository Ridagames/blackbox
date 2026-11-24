#pragma once
#include <SDL.h>
namespace blackbox {
class Controller {
public:
    Controller();
    ~Controller();
    Controller(const Controller&) = delete;
    Controller& operator=(const Controller&) = delete;
    void handle(const SDL_Event& event);
    bool confirm_pressed() const { return confirm_; }
    bool exit_requested() const { return exit_; }
    int navigation_delta() const { return navigation_delta_; }
private:
    SDL_GameController* game_controller_ = nullptr;
    bool confirm_ = false;
    bool exit_ = false;
    int navigation_delta_ = 0;
};
}
