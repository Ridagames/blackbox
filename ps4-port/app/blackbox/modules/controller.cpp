/*
 * DualShock 4 input for BLACKBOX PS4.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "controller.hpp"
#include <orbis/libkernel.h>
#include <orbis/Pad.h>
#include <orbis/UserService.h>
#include <time.h>
#include <SDL2/SDL_gamecontroller.h>

namespace blackbox {
namespace {
constexpr uint64_t kReconnectDelayMs = 500;
constexpr uint64_t kInitialRepeatDelayMs = 300;
constexpr uint64_t kRepeatIntervalMs = 85;
constexpr uint8_t kStickLow = 64;
constexpr uint8_t kStickHigh = 192;
constexpr int16_t kSdlStickThreshold = 12000;

uint64_t input_now() {
    struct timespec ts{};
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return 0;
    return static_cast<uint64_t>(ts.tv_sec) * 1000ULL +
           static_cast<uint64_t>(ts.tv_nsec) / 1000000ULL;
}
}

Controller::Controller() { SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK); open(); }
Controller::~Controller() { close(); }

void Controller::open() {
    started_ = true;
    if (SDL_NumJoysticks() > 0) {
        for (int i = 0; i < SDL_NumJoysticks(); ++i) {
            if (SDL_IsGameController(i)) { sdl_controller_ = SDL_GameControllerOpen(i); if (sdl_controller_) break; }
        }
    }
    seed_ = true;
    retry(input_now());
}

void Controller::retry(uint64_t now) {
    retry_at_ = now + kReconnectDelayMs;
    // UserService must be initialized before opening the controller.
    if (!user_service_initialized_) {
        OrbisUserServiceInitializeParams params{};
        params.priority = ORBIS_KERNEL_PRIO_FIFO_LOWEST;
        const int32_t user_rc = sceUserServiceInitialize(&params);
        // 0x80960003 means another component already initialized the service.
        if (user_rc != 0 && static_cast<uint32_t>(user_rc) != 0x80960003u) {
            error_ = user_rc;
            return;
        }
        user_service_initialized_ = true;
        user_service_owned_ = (user_rc == 0);
    }
    if (!pad_initialized_) {
        const int32_t rc = scePadInit();
        if (rc != 0) { error_ = rc; return; }
        pad_initialized_ = true;
    }
    if (pad_handle_ >= 0) return;
    int32_t user = -1;
    const int32_t user_rc = sceUserServiceGetInitialUser(&user);
    if (user_rc != 0) { error_ = user_rc; return; }
    pad_handle_ = scePadOpen(user, 0, 0, nullptr);
    seed_ = true;
    down_ = 0;
    if (pad_handle_ < 0) error_ = pad_handle_;
    else error_ = 0;
}

void Controller::handle(const SDL_Event& event) {
    if (event.type != SDL_KEYDOWN || event.key.repeat) return;
    switch (event.key.keysym.sym) {
        case SDLK_RETURN: case SDLK_SPACE: keyboard_confirm_ = true; break;
        case SDLK_f: keyboard_favorite_ = true; break;
        case SDLK_ESCAPE: case SDLK_BACKSPACE: keyboard_exit_ = true; break;
        case SDLK_LEFT: keyboard_navigation_ = -1; break;
        case SDLK_RIGHT: keyboard_navigation_ = 1; break;
        case SDLK_UP: keyboard_navigation_ = -6; break;
        case SDLK_DOWN: keyboard_navigation_ = 6; break;
        default: break;
    }
}

void Controller::poll() {
    confirm_ = favorite_ = exit_ = false;
    navigation_delta_ = 0;
    const uint64_t now = input_now();

    if (started_ && now >= retry_at_) {
        if (pad_handle_ < 0) {
            retry(now);
        } else {
            OrbisPadData pad{};
            const int32_t rc = scePadRead(pad_handle_, &pad, 1);
            if (rc != 0) {
                error_ = rc;
                connected_ = false;
                down_ = 0;
                seed_ = true;
                scePadClose(pad_handle_);
                // A failed read invalidates this handle; force a clean reopen.
                pad_handle_ = -1;
                retry_at_ = now + kReconnectDelayMs;
            } else if (!pad.connected) {
                connected_ = false;
                down_ = 0;
                seed_ = true;
                // Reopen after disconnect instead of polling a stale handle forever.
                scePadClose(pad_handle_);
                pad_handle_ = -1;
                retry_at_ = now + kReconnectDelayMs;
            } else {
                connected_ = true;
                error_ = 0;
                raw_buttons_ = pad.buttons;
                retry_at_ = 0;
                uint32_t held = pad.buttons;
                if (pad.leftStick.x < kStickLow) held |= ORBIS_PAD_BUTTON_LEFT;
                if (pad.leftStick.x > kStickHigh) held |= ORBIS_PAD_BUTTON_RIGHT;
                if (pad.leftStick.y < kStickLow) held |= ORBIS_PAD_BUTTON_UP;
                if (pad.leftStick.y > kStickHigh) held |= ORBIS_PAD_BUTTON_DOWN;

                const uint32_t nav_mask = ORBIS_PAD_BUTTON_UP | ORBIS_PAD_BUTTON_DOWN |
                    ORBIS_PAD_BUTTON_LEFT | ORBIS_PAD_BUTTON_RIGHT;
                uint32_t pressed = 0;
                if (seed_) {
                    seed_ = false;
                    down_ = held;
                    repeat_at_ = now + kInitialRepeatDelayMs;
                } else {
                    pressed = held & ~down_;
                    if (pressed & nav_mask) repeat_at_ = now + kInitialRepeatDelayMs;
                    else if ((held & nav_mask) && now >= repeat_at_) {
                        pressed |= held & nav_mask;
                        repeat_at_ = now + kRepeatIntervalMs;
                    }
                    down_ = held;
                }

                if (pressed & ORBIS_PAD_BUTTON_CROSS) confirm_ = true;
                if (pressed & ORBIS_PAD_BUTTON_SQUARE) favorite_ = true;
                if (pressed & (ORBIS_PAD_BUTTON_CIRCLE | ORBIS_PAD_BUTTON_OPTIONS))
                    exit_ = true;

                // Catalogue is a six-column grid: vertical movement skips one row.
                if (pressed & ORBIS_PAD_BUTTON_UP) navigation_delta_ = -6;
                else if (pressed & ORBIS_PAD_BUTTON_DOWN) navigation_delta_ = 6;
                else if (pressed & ORBIS_PAD_BUTTON_LEFT) navigation_delta_ = -1;
                else if (pressed & ORBIS_PAD_BUTTON_RIGHT) navigation_delta_ = 1;
                else if (pressed & ORBIS_PAD_BUTTON_L1) navigation_delta_ = -18;
                else if (pressed & ORBIS_PAD_BUTTON_R1) navigation_delta_ = 18;
            }
        }
    }

    // SDL fallback: used when the native scePad path is unavailable.
    // It supports DS4 mappings exposed by SDL without changing the native path.
    if (sdl_controller_ && SDL_GameControllerGetAttached(sdl_controller_)) {
        SDL_GameControllerUpdate();
        const auto button = [this](SDL_GameControllerButton b) {
            return SDL_GameControllerGetButton(sdl_controller_, b) != 0;
        };
        const Sint16 lx = SDL_GameControllerGetAxis(sdl_controller_, SDL_CONTROLLER_AXIS_LEFTX);
        const Sint16 ly = SDL_GameControllerGetAxis(sdl_controller_, SDL_CONTROLLER_AXIS_LEFTY);
        uint32_t held = 0;
        if (button(SDL_CONTROLLER_BUTTON_DPAD_UP) || ly < -kSdlStickThreshold) held |= ORBIS_PAD_BUTTON_UP;
        if (button(SDL_CONTROLLER_BUTTON_DPAD_DOWN) || ly > kSdlStickThreshold) held |= ORBIS_PAD_BUTTON_DOWN;
        if (button(SDL_CONTROLLER_BUTTON_DPAD_LEFT) || lx < -kSdlStickThreshold) held |= ORBIS_PAD_BUTTON_LEFT;
        if (button(SDL_CONTROLLER_BUTTON_DPAD_RIGHT) || lx > kSdlStickThreshold) held |= ORBIS_PAD_BUTTON_RIGHT;
        const uint32_t nav_mask = ORBIS_PAD_BUTTON_UP | ORBIS_PAD_BUTTON_DOWN |
            ORBIS_PAD_BUTTON_LEFT | ORBIS_PAD_BUTTON_RIGHT;
        uint32_t pressed = held & ~sdl_down_;
        if (pressed & nav_mask) sdl_repeat_at_ = now + kInitialRepeatDelayMs;
        else if ((held & nav_mask) && now >= sdl_repeat_at_) {
            pressed |= held & nav_mask;
            sdl_repeat_at_ = now + kRepeatIntervalMs;
        }
        sdl_down_ = held;
        if (button(SDL_CONTROLLER_BUTTON_A) && !sdl_confirm_down_) confirm_ = true;
        sdl_confirm_down_ = button(SDL_CONTROLLER_BUTTON_A);
        if (button(SDL_CONTROLLER_BUTTON_X) && !sdl_favorite_down_) favorite_ = true;
        sdl_favorite_down_ = button(SDL_CONTROLLER_BUTTON_X);
        if ((button(SDL_CONTROLLER_BUTTON_B) && !sdl_exit_down_) ||
            (button(SDL_CONTROLLER_BUTTON_START) && !sdl_exit_down_)) exit_ = true;
        sdl_exit_down_ = button(SDL_CONTROLLER_BUTTON_B) || button(SDL_CONTROLLER_BUTTON_START);
        // Keep native scePad navigation when present; SDL supplies a second input path.
        if (navigation_delta_ == 0) {
            if (pressed & ORBIS_PAD_BUTTON_UP) navigation_delta_ = -6;
            else if (pressed & ORBIS_PAD_BUTTON_DOWN) navigation_delta_ = 6;
            else if (pressed & ORBIS_PAD_BUTTON_LEFT) navigation_delta_ = -1;
            else if (pressed & ORBIS_PAD_BUTTON_RIGHT) navigation_delta_ = 1;
        }
    } else {
        sdl_down_ = 0;
        sdl_confirm_down_ = sdl_favorite_down_ = sdl_exit_down_ = false;
    }

    if (keyboard_confirm_) confirm_ = true;
    if (keyboard_favorite_) favorite_ = true;
    if (keyboard_exit_) exit_ = true;
    if (keyboard_navigation_ != 0 && navigation_delta_ == 0)
        navigation_delta_ = keyboard_navigation_;
    keyboard_confirm_ = keyboard_favorite_ = keyboard_exit_ = false;
    keyboard_navigation_ = 0;
}

void Controller::close() {
    started_ = false;
    if (sdl_controller_) { SDL_GameControllerClose(sdl_controller_); sdl_controller_ = nullptr; }
    SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK);
    connected_ = false;
    down_ = 0;
    if (pad_handle_ >= 0) {
        if (scePadClose(pad_handle_) == 0) pad_handle_ = -1;
    }
    if (user_service_owned_ && pad_handle_ < 0) {
        if (sceUserServiceTerminate() == 0) user_service_owned_ = false;
    }
}
}
