#pragma once
#include <SDL.h>
namespace blackbox {
class Ui {
public:
    explicit Ui(SDL_Renderer* renderer) : renderer_(renderer) {}
    void render(int selected);
    int selected_section() const { return selected_; }
private:
    SDL_Renderer* renderer_;
    int selected_ = 0;
};
}
