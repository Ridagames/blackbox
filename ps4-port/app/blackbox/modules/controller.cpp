/*
 * DualShock 4 input for BLACKBOX PS4.
 * Controller polling adapted directly from nueex/ps-mtp-explorer.
 * Original project: https://github.com/nueex/ps-mtp-explorer
 * Copyright (C) 2026 nueex; adaptation for BLACKBOX.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "controller.hpp"
#include <orbis/libkernel.h>
#include <orbis/Pad.h>
#include <orbis/UserService.h>
#include <time.h>

namespace blackbox {
namespace {
constexpr uint64_t kReconnectDelayMs = 500;
constexpr uint64_t kInitialRepeatDelayMs = 360;
constexpr uint64_t kRepeatIntervalMs = 90;

uint64_t input_now() {
    struct timespec ts{};
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return 0;
    return static_cast<uint64_t>(ts.tv_sec) * 1000ULL +
           static_cast<uint64_t>(ts.tv_nsec) / 1000000ULL;
}
}

Controller::Controller() {
    // Same ownership model as PS MTP Explorer: only scePad owns the DS4 port.
    // Do not initialize SDL's GameController subsystem or open the pad through SDL.
    started_ = true;
    seed_ = true;
    retry(input_now());
}

Controller::~Controller() { close(); }

void Controller::open() {
    started_ = true;
    seed_ = true;
    retry(input_now());
}

void Controller::retry(uint64_t now) {
    int32_t user = -1;
    int32_t rc;
    OrbisUserServiceInitializeParams params{};
    params.priority = ORBIS_KERNEL_PRIO_FIFO_LOWEST;
    retry_at_ = now + kReconnectDelayMs;

    // Keep the same initialization order as PS MTP Explorer.
    if (!pad_initialized_) {
        rc = scePadInit();
        if (rc != 0) { error_ = rc; return; }
        pad_initialized_ = true;
    }
    if (!user_service_initialized_) {
        rc = sceUserServiceInitialize(&params);
        if (rc == 0) user_service_owned_ = true;
        // 0x80960003 means another component already initialized UserService.
        else if (static_cast<uint32_t>(rc) != 0x80960003u) {
            error_ = rc;
            return;
        }
        user_service_initialized_ = true;
    }
    if (pad_handle_ >= 0) return;

    rc = sceUserServiceGetInitialUser(&user);
    if (rc != 0) { error_ = rc; return; }
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
    pressed_buttons_ = 0;
    const uint64_t now = input_now();

    if (started_ && now >= retry_at_) {
        if (pad_handle_ < 0) {
            retry(now);
        } else {
            OrbisPadData pad{};
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
                    if (pad.leftStick.y < 64) held |= ORBIS_PAD_BUTTON_UP;
                    if (pad.leftStick.y > 192) held |= ORBIS_PAD_BUTTON_DOWN;

                    const uint32_t nav_mask = ORBIS_PAD_BUTTON_UP | ORBIS_PAD_BUTTON_DOWN |
                        ORBIS_PAD_BUTTON_LEFT | ORBIS_PAD_BUTTON_RIGHT |
                        ORBIS_PAD_BUTTON_L1 | ORBIS_PAD_BUTTON_R1;
                    uint32_t pressed = 0;
                    if (seed_) {
                        seed_ = false;
                        down_ = held;
                        repeat_at_ = now + kInitialRepeatDelayMs;
                    } else {
                        pressed = held & ~down_;
                        if (pressed & nav_mask) repeat_at_ = now + kInitialRepeatDelayMs;
                        else if (nav_mask & held && now >= repeat_at_) {
                            pressed |= held & nav_mask;
                            repeat_at_ = now + kRepeatIntervalMs;
                        }
                        down_ = held;
                    }

                    raw_buttons_ = pad.buttons;
                    pressed_buttons_ = pressed;
                    if (pressed & ORBIS_PAD_BUTTON_CROSS) confirm_ = true;
                    if (pressed & ORBIS_PAD_BUTTON_SQUARE) favorite_ = true;
                    if (pressed & (ORBIS_PAD_BUTTON_CIRCLE | ORBIS_PAD_BUTTON_OPTIONS))
                        exit_ = true;

                    if (pressed & ORBIS_PAD_BUTTON_UP) navigation_delta_ = -6;
                    else if (pressed & ORBIS_PAD_BUTTON_DOWN) navigation_delta_ = 6;
                    else if (pressed & ORBIS_PAD_BUTTON_LEFT) navigation_delta_ = -1;
                    else if (pressed & ORBIS_PAD_BUTTON_RIGHT) navigation_delta_ = 1;
                    else if (pressed & ORBIS_PAD_BUTTON_L1) navigation_delta_ = -18;
                    else if (pressed & ORBIS_PAD_BUTTON_R1) navigation_delta_ = 18;
                }
            }
        }
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
    connected_ = false;
    down_ = 0;
    if (pad_handle_ >= 0) {
        if (scePadClose(pad_handle_) == 0) pad_handle_ = -1;
    }
    if (user_service_owned_ && pad_handle_ < 0) {
        if (sceUserServiceTerminate() == 0) {
            user_service_owned_ = false;
            user_service_initialized_ = false;
        }
    }
}
}
