// window.cpp
#include "window.hpp"
#include <algorithm>      // std::clamp
#include <stdexcept>

using std::array;
using std::function;
using std::string;


// ---------- define() overloads ----------
void Window::define(std::string name, Window::Scene scene) {
    scenes[name] = std::move(scene);
}
void Window::define(std::string name, Window::Transition tr) {
    transitions[name] = std::move(tr);
}
void Window::define(std::string name, Window::Popup popup) {
    popups[name] = std::move(popup);
}

// ---------- listen() overloads ----------
void Window::listen(WindowEvents event, std::function<void(std::array<float,2>, std::array<int,2>)> cb) {
    if (event == WindowEvents::Scale) gScaleListeners.push_back(std::move(cb));
}
void Window::listen(WindowEvents event, std::function<void(WindowStatus)> cb) {
    if (event == WindowEvents::Status) gStatusListeners.push_back(std::move(cb));
}

// ---------- actions ----------
void Window::navigate(std::string scene, std::string use_transition, bool /*freeze_scene*/) {
    SceneState.pending = std::move(scene);
    TransitionState.want_change = true;

    if (!use_transition.empty() && transitions.contains(use_transition)) {
        TransitionState.active_transition = std::move(use_transition);
    } else {
        TransitionState.active_transition.clear();
    }
}


void Window::show(std::string popup) {
    if (!popups.contains(popup)) return;
    PopupState.target  = std::move(popup);
    PopupState.state   = Popup_State::State::Show;
}

void Window::hide(std::string popup) {
    if (PopupState.current == popup) {
        PopupState.state = Popup_State::State::Hide;
    }
}

// ---------- init (owns window & loop) ----------
void Window::init(Options options) {
    // Window data
    WindowData.id = 0;
    WindowData.scale_width  = 1.0f;
    WindowData.scale_height = 1.0f;

    // Pull config
    const int W = options.general.width;
    const int H = options.general.height;
    const std::string TITLE = options.general.name;

    const std::string startScene     = options.scene.start_scene;
    const std::string fallbackScene  = options.scene.fallback_scene;
    const std::string defaultTrans   = options.scene.default_transition;

    // Choose starting scene
    if (scenes.contains(startScene)) {
        SceneState.current = startScene;
    } else if (!fallbackScene.empty() && scenes.contains(fallbackScene)) {
        SceneState.current = fallbackScene;
    } else {
        throw std::runtime_error("Scene " + startScene + " was not defined and no fallback was stated.");
    }

    // Prime transition state
    TransitionState.state = Transition_State::State::Inactive;
    TransitionState.time_accumulator = 0.0f;
    SceneState.target.clear();
    SceneState.pending.clear();

    // Window
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    SetTraceLogLevel(LOG_ERROR);
    InitWindow(W, H, TITLE.c_str());
    SetTargetFPS(60);
    for (auto &cb : gStatusListeners) cb(WindowStatus::Open);

    // First onLoad for start scene
    if (!SceneState.current.empty() && scenes.contains(SceneState.current) && scenes[SceneState.current].onLoad) {
        scenes[SceneState.current].onLoad();
    }

    // Create offscreen for popup compositing (kept separate from transition effect)
    ensureCanvas();

    // Track size to emit scale events
    int lastW = GetScreenWidth();
    int lastH = GetScreenHeight();
    const int baseW = W;
    const int baseH = H;

    // Transition bookkeeping
    string activeTransitionName = defaultTrans; // used when none specified by navigate()

    while (!WindowShouldClose()) {
        const float dt = GetFrameTime();

        // update scene
        if (!SceneState.current.empty() && scenes.contains(SceneState.current)) {
            if (scenes[SceneState.current].onUpdate) scenes[SceneState.current].onUpdate(dt);
        }

        tryStartTransition(options);
        advanceTransition(dt);                       // logic only; no drawing

        // resize handling (so scale is current for this frame)
        scaleCallbackExecution(lastW, lastH, baseW, baseH);

        BeginDrawing();
        ClearBackground(BLACK);

        // draw scene (text etc.) FIRST
        if (!SceneState.current.empty() && scenes.contains(SceneState.current)) {
            if (scenes[SceneState.current].onDraw) scenes[SceneState.current].onDraw();
        }

        // NOW draw the transition overlay on top of the scene
        if (TransitionState.render_phase != Transition_State::RenderPhase::None) {
            const float p = TransitionState.render_progress;
            if (TransitionState.render_phase == Transition_State::RenderPhase::Enter)
                callOnEnter(dt, p);
            else
                callOnExit(dt, p);
        }

        // finally popups on top of everything
        if (!PopupState.current.empty() && popups.contains(PopupState.current)) {
            if (popups[PopupState.current].onDraw) popups[PopupState.current].onDraw();
        }

        EndDrawing();
    }


    if (canvas.id != 0) UnloadRenderTexture(canvas);
    for (auto &cb : gStatusListeners) cb(WindowStatus::Close);
    CloseWindow();
}


// ---------- internals ----------
void Window::ensureCanvas() {
    const int sw = GetScreenWidth();
    const int sh = GetScreenHeight();
    if (sw <= 0 || sh <= 0) return;

    if (canvas.id == 0) {
        canvas = LoadRenderTexture(sw, sh);
    } else if (canvas.texture.width != sw || canvas.texture.height != sh) {
        UnloadRenderTexture(canvas);
        canvas = LoadRenderTexture(sw, sh);
    }
}

void Window::scaleCallbackExecution(int& lastW, int& lastH, int baseW, int baseH) {
    // inside the while (!WindowShouldClose()) loop, near the top (after dt):
    int curW = GetScreenWidth();
    int curH = GetScreenHeight();

    if (curW != lastW || curH != lastH || IsWindowResized()) {
        lastW = curW;
        lastH = curH;

        // update scale factors relative to your initial logical size
        WindowData.scale_width  = baseW > 0 ? (float)curW / (float)baseW : 1.0f;
        WindowData.scale_height = baseH > 0 ? (float)curH / (float)baseH : 1.0f;

        std::cout << WindowData.scale_width << std::endl;

        // recreate render target(s) for the new size
        ensureCanvas();

        // fire resize/scale listeners
        std::array<float,2> scale{ WindowData.scale_width, WindowData.scale_height };
        std::array<int,2>   size { curW, curH };
        for (auto &cb : gScaleListeners) cb(scale, size);
    }

}


// ---------------------- helpers: selection / math ----------------------
float Window::norm(float t, float dur) const {
    return std::clamp(t / std::max(0.0001f, dur), 0.0f, 1.0f);
}

std::string Window::pickTransitionName(const std::string& preferred,
                                       const std::string& fallback) const {
    if (!preferred.empty() && transitions.contains(preferred)) return preferred;
    if (!fallback.empty()  && transitions.contains(fallback))  return fallback;
    return {}; // none
}

float Window::pickTransitionDuration(const std::string& name) {
    if (!name.empty() && transitions.contains(name)) return transitions[name].duration;
    return 0.5f; // sensible default
}

// ---------------------- helpers: phase calls ----------------------
void Window::callOnEnter(float dt, float prog) {
    const auto& name = TransitionState.active_transition;
    if (!name.empty() && transitions.contains(name)) {
        auto& tr = transitions[name];
        if (tr.onEnter) tr.onEnter(dt, prog);
    } else {
        // fallback visual if desired
        drawDefaultWipe(prog);
    }
}

void Window::callOnExit(float dt, float prog) {
    const auto& name = TransitionState.active_transition;
    if (!name.empty() && transitions.contains(name)) {
        auto& tr = transitions[name];
        if (tr.onExit) tr.onExit(dt, prog);
    } else {
        // fallback visual if desired (reverse)
        drawDefaultWipe(prog);
    }
}

// Simple left->right wipe (optional)
void Window::drawDefaultWipe(float progress) const {
    int W = GetScreenWidth(), H = GetScreenHeight();
    int w = (int)(W * progress);
    DrawRectangle(0, 0, w, H, BLACK);
}

// ---------------------- helpers: state machine ----------------------
void Window::tryStartTransition(const Options& opts) {
    // Only kick off when inactive and a nav was requested
    if (!(TransitionState.want_change && TransitionState.state == Transition_State::State::Inactive))
        return;

    TransitionState.want_change = false;

    // Resolve scene target
    if (!SceneState.pending.empty() && scenes.contains(SceneState.pending)) {
        SceneState.target = std::move(SceneState.pending);
    } else if (!opts.scene.fallback_scene.empty() && scenes.contains(opts.scene.fallback_scene)) {
        SceneState.target = opts.scene.fallback_scene;
    } else {
        SceneState.target.clear();
    }

    // Choose transition: preferred (from navigate) or default (from options)
    const std::string chosen = pickTransitionName(
        TransitionState.active_transition,
        opts.scene.default_transition
    );
    TransitionState.active_transition = chosen;
    TransitionState.active_duration   = pickTransitionDuration(chosen);

    // Start only if we actually change scenes
    if (!SceneState.target.empty() && SceneState.target != SceneState.current) {
        beginEnterPhase();
    } else {
        // no-op; stay inactive
        TransitionState.active_transition.clear();
    }
}

void Window::beginEnterPhase() {
    TransitionState.state = Transition_State::State::Enter;
    TransitionState.time_accumulator = 0.0f;
}

void Window::beginExitPhase() {
    TransitionState.state = Transition_State::State::Exit;
    TransitionState.time_accumulator = TransitionState.active_duration;
}

void Window::endTransition() {
    TransitionState.state = Transition_State::State::Inactive;
    TransitionState.active_transition.clear();
    TransitionState.time_accumulator = 0.0f;
}

void Window::advanceTransition(float dt) {
    // default: nothing to render this frame
    TransitionState.render_phase    = Transition_State::RenderPhase::None;
    TransitionState.render_progress = 0.0f;

    switch (TransitionState.state) {
        case Transition_State::State::Inactive:
            break;

        case Transition_State::State::Enter: {
            TransitionState.time_accumulator += dt;
            const float prog = norm(TransitionState.time_accumulator, TransitionState.active_duration);

            // expose to renderer
            TransitionState.render_phase    = Transition_State::RenderPhase::Enter;
            TransitionState.render_progress = prog;

            if (prog >= 1.0f) {
                if (scenes.contains(SceneState.current) && scenes[SceneState.current].onUnload)
                    scenes[SceneState.current].onUnload();

                SceneState.current = SceneState.target;

                if (scenes.contains(SceneState.current) && scenes[SceneState.current].onLoad)
                    scenes[SceneState.current].onLoad();

                beginExitPhase();
            }
        } break;

        case Transition_State::State::Exit: {
            TransitionState.time_accumulator -= dt;
            const float prog = norm(TransitionState.time_accumulator, TransitionState.active_duration);

            // expose to renderer
            TransitionState.render_phase    = Transition_State::RenderPhase::Exit;
            TransitionState.render_progress = prog;

            if (prog <= 0.0f) {
                endTransition();
            }
        } break;
    }
}
