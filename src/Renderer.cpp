#include "Renderer.h"
#include <iostream>
#include <thread>
#include <glm/gtc/matrix_transform.hpp>

// Static camera instance for callbacks
Camera* callbackCamera = nullptr;

Renderer::Renderer(int width, int height, const char* title)
    : width(width), height(height), title(title), window(nullptr),
      camera(glm::vec3(0.0f, 0.0f, 3.0f)), lastX(width / 2.0f), lastY(height / 2.0f),
      firstMouse(true), deltaTime(0.0f), lastFrame(0.0f), frameCount(0) {
    callbackCamera = &camera;
}

Renderer::~Renderer() {
    if (texture) {
        delete texture;
        texture = nullptr;
    }
    glfwTerminate();
}

bool Renderer::Initialize() {
    std::cout << "Initializing GLFW..." << std::endl;

    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    std::cout << "Creating window..." << std::endl;

    // Configure GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);

    // Create window
    window = glfwCreateWindow(width, height, title, NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    std::cout << "Setting up OpenGL context..." << std::endl;

    glfwMakeContextCurrent(window);

    // Enable vsync - CRITICAL for preventing flickering
    glfwSwapInterval(1);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // Tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    std::cout << "Initializing GLEW..." << std::endl;

    // Initialize GLEW
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW: " << glewGetErrorString(err) << std::endl;
        return false;
    }

    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);

    // Enable texture debugging
    Texture::EnableDebug(true);

    std::cout << "Loading texture..." << std::endl;

    try {
        // Create texture
        texture = new Texture("textures/container.jpg");
        textureLoaded = true;
        std::cout << "Texture loaded successfully" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Failed to load texture: " << e.what() << std::endl;
        textureLoaded = false;
        // Continue without texture
    }

    return true;
}

void Renderer::RenderLoop() {
    // Create shader
    Shader shader("shaders/vertex.glsl", "shaders/fragment.glsl");
    checkGLError("shader creation");

    // Create cube mesh
    Mesh cube = CreateCube();
    checkGLError("mesh creation");

    // Bind texture once at the beginning if available
    bool textureIsBound = false;
    if (textureLoaded && texture) {
        texture->Bind(0);
        shader.setInt("texture1", 0);
        shader.setBool("useTexture", true);
        textureIsBound = true;
        checkGLError("initial texture binding");
    } else {
        shader.setBool("useTexture", false);
    }

    // For tracking texture binding stats
    int bindCallsLastFrame = Texture::totalBindCalls;

    // Main render loop
    while (!glfwWindowShouldClose(window)) {
        // Per-frame time logic
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Process input
        ProcessInput();

        // Clear the screen
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        checkGLError("buffer clear");

        // Activate shader
        shader.use();
        checkGLError("shader use");

        // Create transformations
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
                                               (float)width / (float)height,
                                               0.1f, 100.0f);

        // Make the cube rotate over time
        model = glm::rotate(model, currentFrame, glm::vec3(0.5f, 1.0f, 0.0f));

        // Set uniforms
        shader.setMat4("model", model);
        shader.setMat4("view", view);
        shader.setMat4("projection", projection);
        checkGLError("setting uniforms");

        // No need to rebind texture every frame - it stays bound
        // Only update the useTexture flag if texture state changed
        if (textureLoaded && texture && !textureIsBound) {
            texture->Bind(0);
            shader.setInt("texture1", 0);
            shader.setBool("useTexture", true);
            textureIsBound = true;
            checkGLError("texture binding");
        } else if ((!textureLoaded || !texture) && textureIsBound) {
            shader.setBool("useTexture", false);
            textureIsBound = false;
        }

        // Draw cube
        cube.Draw();
        checkGLError("drawing cube");

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        checkGLError("buffer swap");

        glfwPollEvents();

        // Frame rate limiting (if vsync is disabled)
        const double FPS_LIMIT = 60.0;
        const double FRAME_TIME = 1.0 / FPS_LIMIT;

        double frameEndTime = glfwGetTime();
        double frameDuration = frameEndTime - currentFrame;

        if (frameDuration < FRAME_TIME) {
            // Sleep to maintain frame rate
            double sleepTime = FRAME_TIME - frameDuration;
            std::this_thread::sleep_for(std::chrono::duration<double>(sleepTime));
        }

        // Print debug stats periodically
        frameCount++;
        if (frameCount % 60 == 0) {
            std::cout << "Frame " << frameCount << ": "
                      << (Texture::totalBindCalls - bindCallsLastFrame) / 60.0f
                      << " texture binds per frame (avg)" << std::endl;
            Texture::PrintBindStats();
            bindCallsLastFrame = Texture::totalBindCalls;
        }
    }

    // Unbind texture before exiting
    if (textureLoaded && texture && textureIsBound) {
        texture->Unbind();
    }
}

void Renderer::ProcessInput() {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

Mesh Renderer::CreateCube() {
    // Define vertices with texture coordinates
    std::vector<Vertex> vertices = {
        // positions          // colors           // texture coords
        {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}, // Bottom-left
        {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},  // Bottom-right
        {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},   // Top-right
        {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},  // Top-left

        // Back face
        {{-0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},  // Bottom-left
        {{0.5f, -0.5f, 0.5f}, {0.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},   // Bottom-right
        {{0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},    // Top-right
        {{-0.5f, 0.5f, 0.5f}, {0.5f, 0.5f, 0.5f}, {1.0f, 1.0f}}    // Top-left
    };

    // Define indices (same as before)
    std::vector<unsigned int> indices = {
        0, 1, 2, 2, 3, 0, // Front face
        4, 5, 6, 6, 7, 4, // Back face
        0, 4, 7, 7, 3, 0, // Left face
        1, 5, 6, 6, 2, 1, // Right face
        3, 2, 6, 6, 7, 3, // Top face
        0, 1, 5, 5, 4, 0  // Bottom face
    };

    return Mesh(vertices, indices);
}

void Renderer::framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void Renderer::mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    static bool firstMouse = true;
    static float lastX = 800.0f / 2.0;
    static float lastY = 600.0 / 2.0;

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // Reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (callbackCamera)
        callbackCamera->ProcessMouseMovement(xoffset, yoffset);
}

void checkGLError(const char* operation) {
    GLenum error;
    while ((error = glGetError()) != GL_NO_ERROR) {
        std::string errorStr;
        switch (error) {
            case GL_INVALID_ENUM: errorStr = "GL_INVALID_ENUM"; break;
            case GL_INVALID_VALUE: errorStr = "GL_INVALID_VALUE"; break;
            case GL_INVALID_OPERATION: errorStr = "GL_INVALID_OPERATION"; break;
            case GL_STACK_OVERFLOW: errorStr = "GL_STACK_OVERFLOW"; break;
            case GL_STACK_UNDERFLOW: errorStr = "GL_STACK_UNDERFLOW"; break;
            case GL_OUT_OF_MEMORY: errorStr = "GL_OUT_OF_MEMORY"; break;
            default: errorStr = "Unknown"; break;
        }
        std::cerr << "OpenGL error after " << operation << ": " << errorStr << std::endl;
    }
}

void Renderer::scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if (callbackCamera)
        callbackCamera->ProcessMouseScroll(yoffset);
}