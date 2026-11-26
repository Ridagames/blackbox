#pragma once
#include <SDL2/SDL.h>
#include <cstdint>

namespace blackbox {
class Controller {
public:
    Controller();
    ~Controller();
    Controller(const Controller&) = delete;
    Controller& operator=(const Controller&) = delete;

    void handle(const SDL_Event& event);
    void poll();

    bool confirm_pressed() const { return confirm_; }
    bool favorite_pressed() const { return favorite_; }
    bool exit_requested() const { return exit_; }
    int navigation_delta() const { return navigation_delta_; }
    bool connected() const { return connected_; }
    uint32_t pressed_buttons() const { return pressed_buttons_; }

private:
    void retry(uint64_t now);
    void close();

    int pad_handle_ = -1;
    bool started_ = false;
    bool pad_initialized_ = false;
    bool user_service_owned_ = false;
    bool user_service_initialized_ = false;
    bool connected_ = false;
    bool seed_ = true;
    int failures_ = 0;
    int32_t error_ = 0;
    uint32_t down_ = 0;
    uint32_t pressed_buttons_ = 0;
    uint64_t retry_at_ = 0;
    uint64_t repeat_at_ = 0;

    bool keyboard_confirm_ = false;
    bool keyboard_favorite_ = false;
    bool keyboard_exit_ = false;
    int keyboard_navigation_ = 0;
    bool confirm_ = false;
    bool favorite_ = false;
    bool exit_ = false;
    int navigation_delta_ = 0;
};
}
