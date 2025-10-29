#include "../engine/window.hpp"
#include <algorithm>
#include <iostream>

inline Window::Scene titleScene(Window& sm) {
    return Window::Scene{
        .onLoad = [](){},
        .onUnload = [](){},
        .onUpdate = [&](float){ 
            if (IsKeyPressed(KEY_SPACE)) sm.navigate("game"); 
        },
        .onDraw = [&](){
            ClearBackground(DARKBLUE);
            float font = sm.WindowData.scale_width * 24.0f;
            float font_width = std::clamp(sm.WindowData.scale_width * 60.0f, 0.0f, 60.0f);
            float font_height = std::clamp(sm.WindowData.scale_height * 60.0f, 0.0f, 60.0f);
            DrawText("MENU — press SPACE", font_width, font_height, font, RAYWHITE);
        }
    };
}


inline Window::Scene gameScene(Window& sm) {
    return Window::Scene{
        .onUpdate = [&](float){ if (IsKeyPressed(KEY_SPACE)) sm.navigate("menu"); },
        .onDraw = [&](){
            ClearBackground(DARKGREEN);
            float font = sm.WindowData.scale_width * 24.0f;
            float font_width = std::clamp(sm.WindowData.scale_width * 60.0f, 0.0f, 60.0f);
            float font_height = std::clamp(sm.WindowData.scale_height * 60.0f, 0.0f, 60.0f);
            DrawText("MENU — press SPACE", font_width, font_height, font, RAYWHITE);
        }
    };
}


inline Window::Transition transition() {
    std::function<void(float, float)> transitionHelper = [](float, float progress){
        const int W = GetScreenWidth();
        const int H = GetScreenHeight();
        const int N = 16; // number of slats
        const float staggerSpan = 0.35f;

        const int slatW = (W + N - 1) / N; // ceil-div

        for (int i = 0; i < N; ++i) {
            const float stagger = (float)i / (float)N * staggerSpan;
            const float local   = std::clamp((progress - stagger) / (1.0f - staggerSpan), 0.0f, 1.0f);
            const int   hDraw   = (int)(H * local);
            const int   x       = i * slatW;
            DrawRectangle(x, 0, slatW, hDraw, BLACK);
        }
    };

    return Window::Transition {
        .duration = 0.5f,
        .onEnter = transitionHelper,
        .onExit = transitionHelper
    };
}