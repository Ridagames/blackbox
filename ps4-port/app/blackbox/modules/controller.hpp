#pragma once
#include <SDL.h>
namespace blackbox {
class Controller {
public:
    void handle(const SDL_Event& event);
    bool confirm_pressed() const { return confirm_; }
    bool exit_requested() const { return exit_; }
private:
    bool confirm_ = false;
    bool exit_ = false;
};
}
