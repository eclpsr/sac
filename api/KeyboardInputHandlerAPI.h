#pragma once

#include <string>
#include <functional>
#include <map>

namespace KeyboardInputHandler {
    const std::map<std::string, int> keyNameToCodeValue = {
        { "azerty_space", 65 },
        { "azerty_a", 24 },
        { "azerty_b", 56 },
        { "azerty_c", 54 },
        { "azerty_d", 40 },
        { "azerty_e", 26 },
        { "azerty_f", 41 },
        { "azerty_g", 42 },
        { "azerty_h", 43 },
        { "azerty_i", 31 },
        { "azerty_j", 44 },
        { "azerty_k", 45 },
        { "azerty_l", 46 },
        { "azerty_m", 47 },
        { "azerty_n", 57 },
        { "azerty_o", 32 },
        { "azerty_p", 33 },
        { "azerty_q", 38 },
        { "azerty_r", 27 },
        { "azerty_s", 39 },
        { "azerty_t", 28 },
        { "azerty_u", 30 },
        { "azerty_v", 55 },
        { "azerty_w", 52 },
        { "azerty_x", 53 },
        { "azerty_y", 29 },
        { "azerty_z", 25 },
        { "azerty_$", 35 },
        { "azerty_*", 51 },
        { "azerty_enter", 36 },
        { "azerty_<", 94 },
        { "azerty_,", 58 },
        { "azerty_;", 59 },
        { "azerty_:", 60 },
        { "azerty_!", 61 },
        { "azerty_lshift", 62 },
    };
}

class KeyboardInputHandlerAPI {
    public:
        virtual void registerToKeyPress(int key, std::function<void()>) = 0;

        virtual void registerToKeyRelease(int key, std::function<void()>) = 0;

        virtual void update() = 0;

        virtual int eventSDL(const void* event) = 0;

        virtual bool isKeyPressed(int key) = 0;
};
