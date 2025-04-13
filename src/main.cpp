#include <iostream>
#include "Renderer.h"

int main() {
    std::cout << "Starting application..." << std::endl;

    // Create renderer
    Renderer renderer(800, 600, "Spinning Cube");

    std::cout << "Renderer created, initializing..." << std::endl;

    // Initialize renderer
    if (!renderer.Initialize()) {
        std::cerr << "Failed to initialize renderer" << std::endl;
        return -1;
    }

    std::cout << "Initialization successful, starting render loop..." << std::endl;

    // Start render loop
    renderer.RenderLoop();

    std::cout << "Render loop finished, exiting..." << std::endl;

    return 0;
}