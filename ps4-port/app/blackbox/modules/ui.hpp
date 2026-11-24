#pragma once
#include <SDL2/SDL.h>
namespace blackbox {
class Ui {
public:
    explicit Ui(SDL_Renderer* renderer) : renderer_(renderer) {}
    void render(int selected);
    void move_selection(int delta);
    int selected_section() const { return selected_; }
private:
    SDL_Renderer* renderer_;
    int selected_ = 0;
};
}
