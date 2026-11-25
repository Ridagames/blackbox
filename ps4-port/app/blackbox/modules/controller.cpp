/*
 * DualShock 4 input for BLACKBOX PS4.
 * Adapted from the proven PS4 controller handling in nueex/ps-mtp-explorer.
 * Original copyright (C) 2026 nueex.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Source: https://github.com/nueex/ps-mtp-explorer
 * Adaptation: 2026-11-25, Ridagames.
 */
#include "controller.hpp"
#include <orbis/libkernel.h>
#include <orbis/Pad.h>
#include <orbis/UserService.h>
#include <cstring>

namespace blackbox {
namespace {
constexpr uint64_t kReconnectDelayMs = 500;
constexpr uint64_t kInitialRepeatDelayMs = 360;
constexpr uint64_t kRepeatIntervalMs = 90;
constexpr uint8_t kStickLow = 64;
constexpr uint8_t kStickHigh = 192;

uint64_t input_now() {
    return static_cast<uint64_t>(SDL_GetTicks());
}
}

Controller::Controller() {
    open();
}

Controller::~Controller() {
    close();
}

void Controller::open() {
    started_ = true;
    seed_ = true;
    retry(input_now());
}

void Controller::retry(uint64_t now) {
    int32_t user = -1;
    int32_t rc = 0;
    OrbisUserServiceInitializeParams params = {ORBIS_KERNEL_PRIO_FIFO_LOWEST};
    retry_at_ = now + kReconnectDelayMs;

    if (!pad_initialized_) {
        rc = scePadInit();
        if (rc != 0) {
            error_ = rc;
            return;
        }
        pad_initialized_ = true;
        rc = sceUserServiceInitialize(&params);
        if (rc == 0) user_service_owned_ = true;
    }

    if (pad_handle_ >= 0) return;

    rc = sceUserServiceGetInitialUser(&user);
    if (rc != 0) {
        error_ = rc;
        return;
    }

    pad_handle_ = scePadOpen(user, 0, 0, nullptr);
    seed_ = true;
    if (pad_handle_ < 0) error_ = pad_handle_;
    else error_ = 0;
}

void Controller::handle(const SDL_Event& event) {
    if (event.type != SDL_KEYDOWN || event.key.repeat) return;

    switch (event.key.keysym.sym) {
        case SDLK_RETURN:
        case SDLK_SPACE:
            keyboard_confirm_ = true;
            break;
        case SDLK_f:
            keyboard_favorite_ = true;
            break;
        case SDLK_ESCAPE:
        case SDLK_BACKSPACE:
            keyboard_exit_ = true;
            break;
        case SDLK_LEFT:
        case SDLK_UP:
            keyboard_navigation_ = -1;
            break;
        case SDLK_RIGHT:
        case SDLK_DOWN:
            keyboard_navigation_ = 1;
            break;
        default:
            break;
    }
}

void Controller::poll() {
    confirm_ = false;
    favorite_ = false;
    exit_ = false;
    navigation_delta_ = 0;

    OrbisPadData pad{};
    const uint64_t now = input_now();

    if (started_ && now >= retry_at_) {
        if (pad_handle_ < 0) {
            retry(now);
        } else {
            const int32_t rc = scePadReadState(pad_handle_, &pad);
            if (rc < 0) {
                error_ = rc;
                connected_ = false;
                down_ = 0;
                seed_ = true;
                retry_at_ = now + kReconnectDelayMs;

                if (++failures_ >= 3) {
                    const int32_t close_rc = scePadClose(pad_handle_);
                    if (close_rc == 0) pad_handle_ = -1;
                    failures_ = 0;
                }
            } else {
                error_ = 0;
                failures_ = 0;

                if (!pad.connected) {
                    connected_ = false;
                    down_ = 0;
                    seed_ = true;
                    retry_at_ = now + kReconnectDelayMs;
                } else {
                    connected_ = true;
                    retry_at_ = 0;

                    uint32_t held = pad.buttons;
                    if (pad.leftStick.x < kStickLow) held |= ORBIS_PAD_BUTTON_LEFT;
                    if (pad.leftStick.x > kStickHigh) held |= ORBIS_PAD_BUTTON_RIGHT;
                    if (pad.leftStick.y < kStickLow) held |= ORBIS_PAD_BUTTON_UP;
                    if (pad.leftStick.y > kStickHigh) held |= ORBIS_PAD_BUTTON_DOWN;

                    const uint32_t nav_mask =
                        ORBIS_PAD_BUTTON_UP | ORBIS_PAD_BUTTON_DOWN |
                        ORBIS_PAD_BUTTON_LEFT | ORBIS_PAD_BUTTON_RIGHT |
                        ORBIS_PAD_BUTTON_L1 | ORBIS_PAD_BUTTON_R1;

                    uint32_t pressed = 0;
                    if (seed_) {
                        seed_ = false;
                        down_ = held;
                        repeat_at_ = now + kInitialRepeatDelayMs;
                    } else {
                        pressed = held & ~down_;
                        if (pressed & nav_mask) {
                            repeat_at_ = now + kInitialRepeatDelayMs;
                        } else if ((held & nav_mask) && now >= repeat_at_) {
                            pressed |= held & nav_mask;
                            repeat_at_ = now + kRepeatIntervalMs;
                        }
                        down_ = held;
                    }

                    if (pressed & ORBIS_PAD_BUTTON_CROSS) confirm_ = true;
                    if (pressed & ORBIS_PAD_BUTTON_SQUARE) favorite_ = true;
                    if (pressed & (ORBIS_PAD_BUTTON_CIRCLE | ORBIS_PAD_BUTTON_OPTIONS))
                        exit_ = true;

                    if (pressed & (ORBIS_PAD_BUTTON_LEFT | ORBIS_PAD_BUTTON_UP |
                                   ORBIS_PAD_BUTTON_L1)) {
                        navigation_delta_ = -1;
                    } else if (pressed & (ORBIS_PAD_BUTTON_RIGHT | ORBIS_PAD_BUTTON_DOWN |
                                          ORBIS_PAD_BUTTON_R1)) {
                        navigation_delta_ = 1;
                    }
                }
            }
        }
    }

    if (keyboard_confirm_) confirm_ = true;
    if (keyboard_favorite_) favorite_ = true;
    if (keyboard_exit_) exit_ = true;
    if (keyboard_navigation_ != 0 && navigation_delta_ == 0)
        navigation_delta_ = keyboard_navigation_;

    keyboard_confirm_ = false;
    keyboard_favorite_ = false;
    keyboard_exit_ = false;
    keyboard_navigation_ = 0;
}

void Controller::close() {
    started_ = false;
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
