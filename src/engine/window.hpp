#pragma once
#include <raylib.h>
#include <string>
#include <functional>
#include <iostream>
#include <array>
#include <vector>

#include "map.hpp"

class Window {
    public:
        struct Scene {
            std::function<void()> onLoad = [] {};
            std::function<void()> onUnload = [] {};
            std::function<void(float)> onUpdate = [](float) {};
            std::function<void()> onDraw = [] {};
        };
        
        struct Transition {
            Color idle_color = BLACK;
            float duration = 0.5f;
            std::function<void(float, float)> onEnter = [](float, float) {};
            std::function<void(float, float)> onExit = [](float, float) {};
            std::function<void()> onIdle = []() {};
        };
        
        struct Popup {
            std::function<void(float, float)> onShow = [](float, float) {};
            std::function<void(float, float)> onHide = [](float, float) {};
            std::function<void()> onDraw = [] {};
            std::function<void(float)> onUpdate = [](float) {};
        };


        struct Options {
            struct  General {
                int width;
                int height;
                std::string name;
            } general;

            struct Scene {
                std::string start_scene;
                std::string fallback_scene = "";
                std::string default_transition = "";
            } scene;
        };

        struct Window_Data {
            int id; // ID Associated with the Window
            float scale_width; // percent scale from stated width 
            float scale_height; // percent scale from stated height 
        } WindowData;


        void define(std::string name, Scene scene);
        void define(std::string name, Transition transition);
        void define(std::string name, Popup popup);
        
        void init(Options options);

        enum class WindowEvents {
            Scale,
            Status
        };

        enum class WindowStatus {
            Close,
            Open,
            Minimize,
            Maximize,
            Focus,
            Blur,
            Fullscreen
        };

        void listen(WindowEvents event, std::function<void(std::array<float, 2>, std::array<int, 2>)> callback);
        void listen(WindowEvents event, std::function<void(WindowStatus)> callback);

        //* Actions 
        void navigate(std::string scene, std::string use_transtion = "", bool freeze_scene = false);
        void show(std::string popup);
        void hide(std::string popup);

    private:
        std::vector<std::function<void(std::array<float,2>, std::array<int,2>)>> gScaleListeners;
        std::vector<std::function<void(Window::WindowStatus)>> gStatusListeners;
        Dict<std::string, Scene> scenes;
        Dict<std::string, Transition> transitions;
        Dict<std::string, Popup> popups;

        struct Scene_State {
            std::string current;
            std::string target;
            std::string pending;
            std::string fallback;
        } SceneState;

        struct Transition_State {
            enum class State { Enter, Exit, Inactive } state = State::Inactive;
            bool         want_change = false;
            std::string  active_transition;
            float        time_accumulator = 0.0f;
            float        active_duration  = 0.5f;

            enum class RenderPhase { None, Enter, Exit } render_phase = RenderPhase::None;
            float render_progress = 0.0f;
        } TransitionState;

        struct Popup_State {
            enum class State{
                Show,
                Active,
                Hide,
                Inactive
            } state = State::Inactive;
            std::string current;
            std::string target;
        } PopupState;

        // Offscreen capture for fancy popups
        RenderTexture2D canvas{};

        // --- internals ---
        void ensureCanvas();

        void scaleCallbackExecution(int& lastW, int& lastH, int baseW, int baseH);

        // --- NEW: transition helpers ---
        void         tryStartTransition(const Options& opts);
        void         advanceTransition(float dt);
        void         beginEnterPhase();
        void         beginExitPhase();
        void         endTransition();
        float        norm(float t, float dur) const;
        std::string  pickTransitionName(const std::string& preferred,
                                        const std::string& fallback) const;
        float pickTransitionDuration(const std::string& name);
        void         callOnEnter(float dt, float prog);
        void         callOnExit (float dt, float prog);

        // Optional: default visual if a transition name isn’t found
        void         drawDefaultWipe(float progress) const;
};