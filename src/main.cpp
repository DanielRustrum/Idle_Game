#include <raylib.h>
#include <array>
#include <iostream>

#include "engine/window.hpp"

#include "scenes/title.cpp"

int main() {
    Window window;


    // Regular scenes
    window.define("menu", titleScene(window));
    window.define("game", gameScene(window));
    window.define( "blinds", transition());

    window.listen(Window::WindowEvents::Scale, [&window](std::array<float, 2>, std::array<int, 2> size){
        std::cout << size[0] << ", " << size[1] << std::endl;
        std::cout << window.WindowData.scale_width << std::endl;
    });

    // Start & get the per-frame lambda
    Window::Options options = {
        general: {
            width: 600,
            height: 600,
            name: "Hello!"
        },
        scene: {
            start_scene: "menu",
            default_transition: "blinds"
        }
    };
    window.init(options);

    
    return 0;
}