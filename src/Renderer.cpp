// Define before including GLM headers that use experimental features
#define GLM_ENABLE_EXPERIMENTAL

#include "Renderer.h" // Should include necessary headers like GLFW/glew.h, glm, etc.
#include "Camera.h"   // For Camera class used in callbacks and members
#include "Mesh.h"     // For Mesh and Vertex types
#include "Shader.h"   // For Shader class
#include "Textures.h" // Corrected include name for Texture class
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp> // For glm::value_ptr
#include <glm/gtc/quaternion.hpp> // For quaternion to matrix conversion
#include <glm/gtx/quaternion.hpp> // For quaternion to matrix conversion
#include <GL/glew.h> // Make sure GLEW is included for OpenGL functions and types
#include <GLFW/glfw3.h> // Include GLFW header

// --- Bullet Includes ---
#include <btBulletDynamicsCommon.h>
// --- End Bullet Includes ---

#include <iostream>
#include <string> // For std::string
#include <vector> // For std::vector
#include <thread>
#include <filesystem> // For checking texture path and iterating directories
#include <algorithm> // For std::transform
#include <chrono>
#include <cstddef> // For offsetof
#include <memory> // For unique_ptr

#include <imgui.h> // Include ImGui
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

// --- Helper function to check for OpenGL errors ---
void checkGLError(const char* operation) {
    GLenum error;
    while ((error = glGetError()) != GL_NO_ERROR) {
        std::string errorStr;
        switch (error) {
            case GL_INVALID_ENUM:    errorStr = "GL_INVALID_ENUM";    break;
            case GL_INVALID_VALUE:  errorStr = "GL_INVALID_VALUE";  break;
            case GL_INVALID_OPERATION:errorStr = "GL_INVALID_OPERATION"; break;
            case GL_STACK_OVERFLOW:  errorStr = "GL_STACK_OVERFLOW";  break;
            case GL_STACK_UNDERFLOW: errorStr = "GL_STACK_UNDERFLOW"; break;
            default:                errorStr = "Unknown GL error";  break;
        }
        std::cerr << "OpenGL error after " << operation << ": " << errorStr << " (" << error << ")" << std::endl;
    }
}

// Static camera instance for callbacks
Camera* callbackCamera = nullptr;

// Constructor
Renderer::Renderer(int width, int height, const char* title)
    : width(width), height(height), title(title), window(nullptr),
      camera(glm::vec3(0.0f, 5.0f, 10.0f)),
      lastX(static_cast<float>(width) / 2.0f),
      lastY(static_cast<float>(height) / 2.0f),
      firstMouse(true), deltaTime(0.0f), lastFrame(0.0f), frameCount(0),
      fps(0.0f), fpsUpdateTime(0.0f),
      texture(nullptr),
      textureLoaded(false),
      needToRebindTexture(false),
      isPaused(false), // Initialize pause state
      lastModelMatrix(glm::mat4(1.0f)),
      quadVAO(0), quadVBO(0),
      m_rasterShader(nullptr), // Initialize shader pointer
      m_cubeVAO(0), m_cubeVBO(0), m_cubeEBO(0), m_cubeIndexCount(0)
       {
    callbackCamera = &camera;
}

// Destructor
Renderer::~Renderer() {
    ShutdownImGui();

    if (texture) {
        delete texture;
        texture = nullptr;
    }

    // Cleanup Quad
    if (quadVBO != 0) { glDeleteBuffers(1, &quadVBO); quadVBO = 0; }
    if (quadVAO != 0) { glDeleteVertexArrays(1, &quadVAO); quadVAO = 0; }
    std::cout << "Cleaned up screen quad." << std::endl;

    // Cleanup Physics Meshes
    if (m_cubeEBO != 0) { glDeleteBuffers(1, &m_cubeEBO); m_cubeEBO = 0; }
    if (m_cubeVBO != 0) { glDeleteBuffers(1, &m_cubeVBO); m_cubeVBO = 0; }
    if (m_cubeVAO != 0) { glDeleteVertexArrays(1, &m_cubeVAO); m_cubeVAO = 0; }
    std::cout << "Cleaned up physics meshes." << std::endl;

    // Shader unique_ptr handles its own deletion
    glfwTerminate();
}

// --- ImGui functions ---
void Renderer::InitImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
    if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) { std::cerr << "Failed to initialize ImGui GLFW backend" << std::endl; return; }
    if (!ImGui_ImplOpenGL3_Init("#version 330 core")) { std::cerr << "Failed to initialize ImGui OpenGL3 backend" << std::endl; return; }
     std::cout << "ImGui Initialized Successfully (Simple Mode)" << std::endl;
}

void Renderer::ShutdownImGui() {
    if (ImGui::GetCurrentContext()) { // Check if context exists
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        std::cout << "ImGui Shutdown Successfully (Simple Mode)" << std::endl;
    }
}
// --- End ImGui ---


bool Renderer::Initialize() {
    std::cout << "Initializing GLFW..." << std::endl;
    if (!glfwInit()) { std::cerr << "Failed to initialize GLFW" << std::endl; return false; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    #ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    std::cout << "Creating window..." << std::endl;
    window = glfwCreateWindow(width, height, title, NULL, NULL);
    if (!window) { std::cerr << "Failed to create GLFW window" << std::endl; glfwTerminate(); return false; }

    glfwSetWindowUserPointer(window, this);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    std::cout << "Initializing GLEW..." << std::endl;
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) { std::cerr << "Failed to initialize GLEW: " << glewGetErrorString(err) << std::endl; glfwDestroyWindow(window); glfwTerminate(); return false; }
    while(glGetError() != GL_NO_ERROR); // Clear GLEW error

    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;

    glEnable(GL_DEPTH_TEST);
    checkGLError("glEnable(GL_DEPTH_TEST)");

    InitImGui(); // Initialize ImGui

    // Load texture (optional)
    if (!LoadTextureFromDirectories()) {
        std::cerr << "Initialization warning: Failed to load any texture." << std::endl;
    }

    setupScreenQuad(); // Set up the quad geometry needed for ray marching
    setupRasterShader(); // Load the shader for physics objects
    setupPhysicsMeshes(); // Create meshes for physics objects

    // Initialize lastFrame time
    lastFrame = (float)glfwGetTime();

    return true;
}

// --- Helper function to find and load the first texture ---
bool Renderer::LoadTextureFromDirectories() {
    std::cout << "Searching for texture..." << std::endl;
    const std::vector<std::string> textureDirectories = { "textures/cube_textures", "textures", "textures/skybox" };
    const std::vector<std::string> extensions = { ".jpg", ".jpeg", ".png", ".bmp", ".tga" };
    if (texture) { delete texture; texture = nullptr; textureLoaded = false; }
    for (const std::string& dir : textureDirectories) {
        std::error_code ec; if (!std::filesystem::is_directory(dir, ec) || ec) continue;
        std::cout << "  Searching in directory: " << dir << std::endl;
        for (const auto& entry : std::filesystem::directory_iterator(dir)) {
            if (entry.is_regular_file()) {
                std::filesystem::path entryPath = entry.path(); std::string filePath = entryPath.string();
                std::string fileExtension = entryPath.extension().string();
                std::transform(fileExtension.begin(), fileExtension.end(), fileExtension.begin(), [](unsigned char c){ return std::tolower(c); });
                bool extensionMatch = false; for (const std::string& ext : extensions) { if (fileExtension == ext) { extensionMatch = true; break; } }
                if (extensionMatch) {
                    std::cout << "  Found potential texture file: " << filePath << std::endl;
                    try {
                        texture = new Texture(filePath.data()); textureLoaded = true;
                        std::cout << ">>> Successfully loaded texture: " << filePath << std::endl;
                        checkGLError("texture loading in LoadTextureFromDirectories"); return true;
                    } catch (const std::exception& e) {
                        std::cerr << "  Failed to load texture '" << filePath << "': " << e.what() << std::endl;
                        textureLoaded = false; if (texture) { delete texture; texture = nullptr; }
                    }
                }
            }
        }
    }
    std::cerr << "!!! No suitable texture found in specified directories." << std::endl;
    textureLoaded = false; return false;
}

// --- Helper to set up screen quad ---
void Renderer::setupScreenQuad() {
    float quadVertices[] = { -1.0f,  1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f,  1.0f, 1.0f, -1.0f, 1.0f,  1.0f };
    glGenVertexArrays(1, &quadVAO); glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO); glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0); glBindVertexArray(0);
    checkGLError("setupScreenQuad");
    std::cout << "Screen quad setup complete (VAO: " << quadVAO << ", VBO: " << quadVBO << ")" << std::endl;
}

// --- Physics Rendering Setup and Methods ---

void Renderer::setupRasterShader() {
    std::cout << "Setting up raster shader..." << std::endl;
    m_rasterShader = std::make_unique<Shader>("shaders/vertex.glsl", "shaders/fragment.glsl");
    if (!m_rasterShader || !m_rasterShader->isValid()) {
        std::cerr << "ERROR: Failed to load raster shader for physics objects!" << std::endl;
        m_rasterShader = nullptr;
    } else {
        std::cout << "Raster shader loaded successfully (ID: " << m_rasterShader->ID << ")." << std::endl;
    }
}

void Renderer::setupPhysicsMeshes() {
    std::cout << "Setting up physics meshes..." << std::endl;
    Mesh tempCube = CreateCube();
    if (tempCube.vertices.empty() || tempCube.indices.empty()) {
        std::cerr << "ERROR: CreateCube returned empty data for physics mesh setup." << std::endl;
        return;
    }
    m_cubeIndexCount = tempCube.indices.size();

    glGenVertexArrays(1, &m_cubeVAO); glGenBuffers(1, &m_cubeVBO); glGenBuffers(1, &m_cubeEBO);
    glBindVertexArray(m_cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, tempCube.vertices.size() * sizeof(Vertex), tempCube.vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_cubeEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, tempCube.indices.size() * sizeof(unsigned int), tempCube.indices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Position));
    glEnableVertexAttribArray(1); glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Color));
    glEnableVertexAttribArray(2); glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
    glBindVertexArray(0); glBindBuffer(GL_ARRAY_BUFFER, 0); glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    checkGLError("setupPhysicsMeshes (Cube)");
    std::cout << "Cube physics mesh setup complete (VAO: " << m_cubeVAO << ")" << std::endl;
}

glm::mat4 Renderer::convertBtTransformToGlm(const btTransform& trans) {
    btQuaternion rot = trans.getRotation(); btVector3 pos = trans.getOrigin();
    glm::quat glmRot = glm::quat(rot.w(), rot.x(), rot.y(), rot.z()); // GLM constructor order w,x,y,z
    glm::vec3 glmPos(pos.x(), pos.y(), pos.z());
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glmPos);
    model = model * glm::toMat4(glmRot);
    return model;
}

void Renderer::renderPhysicsObjects(btDiscreteDynamicsWorld* world, const glm::mat4& view, const glm::mat4& projection) {
    if (!m_rasterShader || !m_rasterShader->isValid() || !world) return;

    m_rasterShader->use();
    m_rasterShader->setMat4("view", view);
    m_rasterShader->setMat4("projection", projection);
    bool textureWasBound = false;
    if (textureLoaded && texture) {
         glActiveTexture(GL_TEXTURE0); texture->Bind(0);
         m_rasterShader->setInt("texture1", 0); m_rasterShader->setBool("useTexture", true);
         textureWasBound = true;
    } else { m_rasterShader->setBool("useTexture", false); }
    checkGLError("Setting raster shader uniforms");

    int numObjects = world->getNumCollisionObjects();
    for (int i = 0; i < numObjects; ++i) {
        btCollisionObject* obj = world->getCollisionObjectArray()[i];
        btRigidBody* body = btRigidBody::upcast(obj);
        if (body && body->getInvMass() != 0.0f && body->getMotionState()) {
            btTransform worldTrans; body->getMotionState()->getWorldTransform(worldTrans);
            glm::mat4 modelMatrix = convertBtTransformToGlm(worldTrans);
            btCollisionShape* shape = body->getCollisionShape();
            if (shape->getShapeType() == SPHERE_SHAPE_PROXYTYPE) {
                float radius = static_cast<const btSphereShape*>(shape)->getRadius();
                modelMatrix = glm::scale(modelMatrix, glm::vec3(radius * 2.0f)); // Scale cube mesh
            }
            m_rasterShader->setMat4("model", modelMatrix);
            checkGLError("Setting model matrix for physics object");

            // Draw using cube VAO for now
            glBindVertexArray(m_cubeVAO);
            checkGLError("glBindVertexArray(m_cubeVAO) in renderPhysicsObjects");
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_cubeIndexCount), GL_UNSIGNED_INT, 0);
            checkGLError("glDrawElements in renderPhysicsObjects");
            glBindVertexArray(0);
        }
    }
     if (textureWasBound && texture) { texture->Unbind(); }
     glUseProgram(0);
     checkGLError("End of renderPhysicsObjects");
}
// --- End Physics Rendering ---


void Renderer::RenderUI() {
    ImGui_ImplOpenGL3_NewFrame(); ImGui_ImplGlfw_NewFrame(); ImGui::NewFrame();
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiCond_FirstUseEver);
    ImGui::Begin("Stats", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);
    ImGui::Text("FPS: %.1f", fps); ImGui::Text("Frame Time: %.3f ms", deltaTime * 1000.0f);
    ImGui::Separator(); ImGui::Text("Camera Pos: (%.1f, %.1f, %.1f)", camera.Position.x, camera.Position.y, camera.Position.z);
    ImGui::Text("Camera Yaw: %.1f Pitch: %.1f", camera.Yaw, camera.Pitch);
    ImGui::End();
    if (isPaused) {
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiCond_Appearing);
        ImGui::Begin("Pause Menu", &isPaused, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("Game Paused"); ImGui::Separator();
        if (ImGui::Button("Resume", ImVec2(-1, 0))) { isPaused = false; glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); std::cout << "Game Resumed (Button). Cursor Disabled." << std::endl; }
        if (ImGui::Button("Quit", ImVec2(-1, 0))) { glfwSetWindowShouldClose(window, true); }
        ImGui::End();
        if (!isPaused && glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_NORMAL) { glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); std::cout << "Game Resumed (Window Closed). Cursor Disabled." << std::endl; }
    }
    ImGui::Render(); ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// --- Update Method ---
void Renderer::Update(float dt) {
    this->deltaTime = dt; // Store delta time from main loop

    // Update FPS counter
    frameCount++;
    float currentTime = (float)glfwGetTime();
    if (currentTime - fpsUpdateTime >= 0.5f) {
        fps = static_cast<double>(frameCount) / (currentTime - fpsUpdateTime);
        fpsUpdateTime = currentTime;
        frameCount = 0;
    }
    // lastFrame update is handled by main loop now
}

// --- Render Method ---
void Renderer::Render(btDiscreteDynamicsWorld* dynamicsWorld) {
    // TODO: Optimize shader loading/uniform lookup - load/get once in Initialize
    Shader raymarchShader("shaders/raymarch_vertex.glsl", "shaders/raymarch_fragment.glsl");
    if (!raymarchShader.isValid()) { std::cerr << "ERROR: Raymarch shader invalid in Render." << std::endl; return; }
    raymarchShader.use();
    GLint timeLoc = glGetUniformLocation(raymarchShader.ID, "u_time");
    GLint camPosLoc = glGetUniformLocation(raymarchShader.ID, "u_camPos");
    GLint invViewLoc = glGetUniformLocation(raymarchShader.ID, "u_invViewMatrix");
    GLint invProjLoc = glGetUniformLocation(raymarchShader.ID, "u_invProjMatrix");
    glUseProgram(0);
    // ---

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    checkGLError("glClear");

    // --- 1. Render Raymarched Terrain ---
    raymarchShader.use();
    checkGLError("raymarchShader.use() inside Render");
    if (timeLoc != -1) glUniform1f(timeLoc, (float)glfwGetTime());
    if (camPosLoc != -1) glUniform3fv(camPosLoc, 1, glm::value_ptr(camera.Position));
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), static_cast<float>(width) / static_cast<float>(height), 0.1f, 200.0f);
    glm::mat4 invView = glm::inverse(view);
    glm::mat4 invProj = glm::inverse(projection);
    if (invViewLoc != -1) glUniformMatrix4fv(invViewLoc, 1, GL_FALSE, glm::value_ptr(invView));
    if (invProjLoc != -1) glUniformMatrix4fv(invProjLoc, 1, GL_FALSE, glm::value_ptr(invProj));
    checkGLError("after setting raymarch uniforms");
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    checkGLError("after drawing quad");
    glUseProgram(0);
    // --- End Terrain Rendering ---

    // --- 2. Render Physics Objects ---
    glEnable(GL_DEPTH_TEST);
    checkGLError("glEnable(GL_DEPTH_TEST) before physics objects");
    renderPhysicsObjects(dynamicsWorld, view, projection); // Pass world pointer
    checkGLError("after renderPhysicsObjects");
    // --- End Physics Object Rendering ---

    // --- 3. Render UI ---
    glUseProgram(0);
    RenderUI();
    checkGLError("after RenderUI");
    // --- End UI ---

    glfwSwapBuffers(window);
    checkGLError("glfwSwapBuffers");
}


void Renderer::ProcessInput() {
    static bool escapePressedLastFrame = false;
    bool escapePressedThisFrame = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    if (escapePressedThisFrame && !escapePressedLastFrame) {
        isPaused = !isPaused;
        if (isPaused) { glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); firstMouse = true; std::cout << "Game Paused. Cursor Enabled." << std::endl; }
        else { glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); std::cout << "Game Resumed. Cursor Disabled." << std::endl; }
    }
    escapePressedLastFrame = escapePressedThisFrame;

    if (!isPaused) {
        static bool tabPressedLastFrame = false;
        bool tabPressedThisFrame = glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS;
        if (tabPressedThisFrame && !tabPressedLastFrame) {
            if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) { glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); firstMouse = true; std::cout << "Mouse Cursor Enabled (Tab)" << std::endl; }
            else { glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); std::cout << "Mouse Cursor Disabled (Tab)" << std::endl; }
        }
        tabPressedLastFrame = tabPressedThisFrame;

        if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
            // Use the deltaTime member variable updated in Update()
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.ProcessKeyboard(FORWARD, deltaTime);
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.ProcessKeyboard(BACKWARD, deltaTime);
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.ProcessKeyboard(LEFT, deltaTime);
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.ProcessKeyboard(RIGHT, deltaTime);
        }

        static bool rPressedLastFrame = false;
        bool rPressedThisFrame = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;
        if (rPressedThisFrame && !rPressedLastFrame) {
            std::cout << "Reload key pressed, attempting texture reload..." << std::endl;
            if (LoadTextureFromDirectories()) { /* needToRebindTexture = true; */ std::cout << "Texture reloaded/found successfully via keypress." << std::endl; }
            else { std::cout << "Texture reload/find via keypress failed." << std::endl; }
        }
        rPressedLastFrame = rPressedThisFrame;
    }
}

// --- Static Callback Functions ---
void Renderer::framebuffer_size_callback(GLFWwindow* window, int newWidth, int newHeight) {
    if (newWidth <= 0 || newHeight <= 0) return;
    glViewport(0, 0, newWidth, newHeight); checkGLError("glViewport");
    Renderer* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
    if (renderer) { renderer->width = newWidth; renderer->height = newHeight; std::cout << "Window resized to: " << newWidth << "x" << newHeight << std::endl; }
}
void Renderer::mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    Renderer* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window)); if (!renderer) return;
    if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
        float xpos = static_cast<float>(xposIn); float ypos = static_cast<float>(yposIn);
        if (renderer->firstMouse) { renderer->lastX = xpos; renderer->lastY = ypos; renderer->firstMouse = false; }
        float xoffset = xpos - renderer->lastX; float yoffset = renderer->lastY - ypos;
        renderer->lastX = xpos; renderer->lastY = ypos;
        renderer->camera.ProcessMouseMovement(xoffset, yoffset);
    }
}
void Renderer::scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    Renderer* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
    if (renderer && !renderer->isPaused) { // Check pause state
        if (callbackCamera) { callbackCamera->ProcessMouseScroll(static_cast<float>(yoffset)); }
    }
}

// --- CreateCube Method (Used by setupPhysicsMeshes) ---
Mesh Renderer::CreateCube() {
    std::vector<Vertex> vertices = {
        Vertex(glm::vec3(-0.5f,-0.5f, 0.5f), glm::vec3(1.f,1.f,1.f), glm::vec2(0.f,0.f)), Vertex(glm::vec3( 0.5f,-0.5f, 0.5f), glm::vec3(1.f,1.f,1.f), glm::vec2(1.f,0.f)),
        Vertex(glm::vec3( 0.5f, 0.5f, 0.5f), glm::vec3(1.f,1.f,1.f), glm::vec2(1.f,1.f)), Vertex(glm::vec3(-0.5f, 0.5f, 0.5f), glm::vec3(1.f,1.f,1.f), glm::vec2(0.f,1.f)),
        Vertex(glm::vec3(-0.5f,-0.5f,-0.5f), glm::vec3(1.f,1.f,1.f), glm::vec2(1.f,0.f)), Vertex(glm::vec3( 0.5f,-0.5f,-0.5f), glm::vec3(1.f,1.f,1.f), glm::vec2(0.f,0.f)),
        Vertex(glm::vec3( 0.5f, 0.5f,-0.5f), glm::vec3(1.f,1.f,1.f), glm::vec2(0.f,1.f)), Vertex(glm::vec3(-0.5f, 0.5f,-0.5f), glm::vec3(1.f,1.f,1.f), glm::vec2(1.f,1.f))
    };
    std::vector<unsigned int> indices = {
        0,1,2, 2,3,0, 4,5,6, 6,7,4, 7,3,0, 0,4,7, 6,5,1, 1,2,6, 3,7,6, 6,2,3, 0,5,4, 0,1,5
    };
    // Return a temporary Mesh object containing the data
    return Mesh(vertices, indices);
}