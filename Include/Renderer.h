#ifndef RENDERER_H
#define RENDERER_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "Shader.h"
#include "Camera.h"
#include "Mesh.h"
#include <glm/glm.hpp>
#include "Textures.h"
#pragma once

void checkGLError(const char* operation);

class Renderer {
public:
    // Constructor
    Renderer(int width, int height, const char* title);

    // Destructor
    ~Renderer();

    // Initialize renderer
    bool Initialize();

    // Main render loop
    void RenderLoop();

    // Process input
    void ProcessInput();

    // Get window
    GLFWwindow* GetWindow() const { return window; }

    // Callback setup
    static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
    static void mouse_callback(GLFWwindow* window, double xpos, double ypos);
    static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

private:
    // Window properties
    int width, height;
    const char* title;
    GLFWwindow* window;
    int frameCount;
    // Camera
    Camera camera;
    float lastX, lastY;
    bool firstMouse;

    // Timing
    float deltaTime;
    float lastFrame;

    Texture* texture = nullptr;
    bool textureLoaded = false;

    // Create cube mesh
    Mesh CreateCube();
};

#endif