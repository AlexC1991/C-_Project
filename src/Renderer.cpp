#define GLM_ENABLE_EXPERIMENTAL

#include "Renderer.h"
#include "Shader.h"
#include "Camera.h"
#include "Mesh.h"
#include "Textures.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <btBulletDynamicsCommon.h>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <filesystem>
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <memory>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include "tinyfiledialogs.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// Helper function to check for OpenGL errors
void checkGLError(const char* operation) {
    GLenum error;
    while ((error = glGetError()) != GL_NO_ERROR) {
        std::string errorStr;
        switch (error) {
            case GL_INVALID_ENUM:    errorStr = "GL_INVALID_ENUM";    break;
            case GL_INVALID_VALUE:   errorStr = "GL_INVALID_VALUE";   break;
            case GL_INVALID_OPERATION: errorStr = "GL_INVALID_OPERATION"; break;
            case GL_STACK_OVERFLOW:  errorStr = "GL_STACK_OVERFLOW";  break;
            case GL_STACK_UNDERFLOW: errorStr = "GL_STACK_UNDERFLOW"; break;
            default:                 errorStr = "Unknown GL error";   break;
        }
        std::cerr << "OpenGL error after " << operation << ": " << errorStr << " (" << error << ")" << std::endl;
    }
}

Camera* callbackCamera = nullptr;

// --- Constructor ---
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
      m_editorState(EditorState::EDITING),
      m_lockMouseInPlayMode(true),
      lastModelMatrix(glm::mat4(1.0f)),
      quadVAO(0), quadVBO(0),
      m_raymarchShader(nullptr),
      m_rasterShader(nullptr),
      m_cubeVAO(0), m_cubeVBO(0), m_cubeEBO(0), m_cubeIndexCount(0),
      m_raymarch_timeLoc(-1), m_raymarch_camPosLoc(-1), m_raymarch_invViewLoc(-1),
      m_raymarch_invProjLoc(-1), m_raymarch_lightDirLoc(-1), m_raymarch_lightColorLoc(-1),
      m_raymarch_ambientLoc(-1), m_raymarch_terrain_base_freqLoc(-1), m_raymarch_terrain_base_ampLoc(-1),
      m_raymarch_terrain_persistenceLoc(-1), m_raymarch_terrain_flatten_powerLoc(-1),
      m_raymarch_terrain_final_scaleLoc(-1), m_raymarch_terrain_octavesLoc(-1),
      m_raymarch_cloud_base_heightLoc(-1), m_raymarch_cloud_thicknessLoc(-1), m_raymarch_cloud_noise_scaleLoc(-1),
      m_raymarch_cloud_coverage_minLoc(-1), m_raymarch_cloud_coverage_maxLoc(-1), m_raymarch_cloud_density_factorLoc(-1),
      m_lightDirection(glm::normalize(glm::vec3(0.8f, 0.7f, -0.5f))),
      m_lightColor(glm::vec3(1.0f, 0.95f, 0.85f)),
      m_ambientStrength(0.15f),
      m_terrain_base_freq(0.2f), m_terrain_base_amp(1.5f), m_terrain_persistence(0.45f),
      m_terrain_flatten_power(1.8f), m_terrain_final_scale(2.5f), m_terrain_octaves(5),
      m_cloud_base_height(10.0f), m_cloud_thickness(12.0f), m_cloud_noise_scale(0.4f),
      m_cloud_coverage_min(0.6f), m_cloud_coverage_max(0.75f), m_cloud_density_factor(1.0f),
      m_selectedAsset(-1)

{
    callbackCamera = &camera;
}

Renderer::~Renderer() {
    ShutdownImGui();
    if (texture) { delete texture; texture = nullptr; }
    if (quadVBO != 0) { glDeleteBuffers(1, &quadVBO); quadVBO = 0; }
    if (quadVAO != 0) { glDeleteVertexArrays(1, &quadVAO); quadVAO = 0; }
    if (m_cubeEBO != 0) { glDeleteBuffers(1, &m_cubeEBO); m_cubeEBO = 0; }
    if (m_cubeVBO != 0) { glDeleteBuffers(1, &m_cubeVBO); m_cubeVBO = 0; }
    if (m_cubeVAO != 0) { glDeleteVertexArrays(1, &m_cubeVAO); m_cubeVAO = 0; }
    std::cout << "Cleaned up screen quad and physics meshes." << std::endl;
    glfwTerminate();
}

void Renderer::InitImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui::StyleColorsDark();
    if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) { std::cerr << "Failed to initialize ImGui GLFW backend" << std::endl; return; }
    if (!ImGui_ImplOpenGL3_Init("#version 330 core")) { std::cerr << "Failed to initialize ImGui OpenGL3 backend" << std::endl; return; }
    std::cout << "ImGui Initialized Successfully (Docking Enabled)" << std::endl;
}

void Renderer::ShutdownImGui() {
    if (ImGui::GetCurrentContext()) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        std::cout << "ImGui Shutdown Successfully" << std::endl;
    }
}

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
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    std::cout << "Initializing GLEW..." << std::endl;
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) { std::cerr << "Failed to initialize GLEW: " << glewGetErrorString(err) << std::endl; glfwDestroyWindow(window); glfwTerminate(); return false; }
    while(glGetError() != GL_NO_ERROR);

    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;

    glEnable(GL_DEPTH_TEST);
    checkGLError("glEnable(GL_DEPTH_TEST)");

    InitImGui();

    // Load Shaders
    std::cout << "Loading Shaders..." << std::endl;
    m_raymarchShader = std::make_unique<Shader>("shaders/raymarch_vertex.glsl", "shaders/raymarch_fragment.glsl");
    if (!m_raymarchShader || !m_raymarchShader->isValid()) { std::cerr << "FATAL: Raymarch shader failed to load." << std::endl; m_raymarchShader = nullptr; }
    else {
        m_raymarchShader->use();
        m_raymarch_timeLoc = glGetUniformLocation(m_raymarchShader->ID, "u_time");
        m_raymarch_camPosLoc = glGetUniformLocation(m_raymarchShader->ID, "u_camPos");
        m_raymarch_invViewLoc = glGetUniformLocation(m_raymarchShader->ID, "u_invViewMatrix");
        m_raymarch_invProjLoc = glGetUniformLocation(m_raymarchShader->ID, "u_invProjMatrix");
        m_raymarch_lightDirLoc = glGetUniformLocation(m_raymarchShader->ID, "u_lightDir");
        m_raymarch_lightColorLoc = glGetUniformLocation(m_raymarchShader->ID, "u_lightColor");
        m_raymarch_ambientLoc = glGetUniformLocation(m_raymarchShader->ID, "u_ambientStrength");
        m_raymarch_terrain_base_freqLoc = glGetUniformLocation(m_raymarchShader->ID, "u_terrain_base_freq");
        m_raymarch_terrain_base_ampLoc = glGetUniformLocation(m_raymarchShader->ID, "u_terrain_base_amp");
        m_raymarch_terrain_persistenceLoc = glGetUniformLocation(m_raymarchShader->ID, "u_terrain_persistence");
        m_raymarch_terrain_flatten_powerLoc = glGetUniformLocation(m_raymarchShader->ID, "u_terrain_flatten_power");
        m_raymarch_terrain_final_scaleLoc = glGetUniformLocation(m_raymarchShader->ID, "u_terrain_final_scale");
        m_raymarch_terrain_octavesLoc = glGetUniformLocation(m_raymarchShader->ID, "u_terrain_octaves");
        m_raymarch_cloud_base_heightLoc = glGetUniformLocation(m_raymarchShader->ID, "u_cloud_base_height");
        m_raymarch_cloud_thicknessLoc = glGetUniformLocation(m_raymarchShader->ID, "u_cloud_thickness");
        m_raymarch_cloud_noise_scaleLoc = glGetUniformLocation(m_raymarchShader->ID, "u_cloud_noise_scale");
        m_raymarch_cloud_coverage_minLoc = glGetUniformLocation(m_raymarchShader->ID, "u_cloud_coverage_min");
        m_raymarch_cloud_coverage_maxLoc = glGetUniformLocation(m_raymarchShader->ID, "u_cloud_coverage_max");
        m_raymarch_cloud_density_factorLoc = glGetUniformLocation(m_raymarchShader->ID, "u_cloud_density_factor");
        glUseProgram(0);
    }
    m_rasterShader = std::make_unique<Shader>("shaders/vertex.glsl", "shaders/fragment.glsl");
    if (!m_rasterShader || !m_rasterShader->isValid()) { std::cerr << "ERROR: Failed to load raster shader!" << std::endl; m_rasterShader = nullptr; }

    if (!LoadTextureFromDirectories()) { std::cerr << "Initialization warning: Failed to load any texture." << std::endl; }

    setupScreenQuad();
    setupPhysicsMeshes();

    lastFrame = (float)glfwGetTime();
    std::cout << "Starting in Edit Mode. Cursor Enabled." << std::endl;

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
                        texture = new Texture(filePath.c_str()); textureLoaded = true;
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

void Renderer::setupRasterShader() { /* Handled in Initialize */ }

void Renderer::setupPhysicsMeshes() {
    std::cout << "Setting up physics meshes..." << std::endl;
    Mesh tempCube = CreateCube();
    if (tempCube.vertices.empty() || tempCube.indices.empty()) { std::cerr << "ERROR: CreateCube returned empty data." << std::endl; return; }
    m_cubeIndexCount = tempCube.indices.size();
    glGenVertexArrays(1, &m_cubeVAO); glGenBuffers(1, &m_cubeVBO); glGenBuffers(1, &m_cubeEBO);
    glBindVertexArray(m_cubeVAO); glBindBuffer(GL_ARRAY_BUFFER, m_cubeVBO);
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
    glm::quat glmRot = glm::quat(rot.w(), rot.x(), rot.y(), rot.z());
    glm::vec3 glmPos(pos.x(), pos.y(), pos.z());
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glmPos);
    model = model * glm::toMat4(glmRot);
    return model;
}

void Renderer::renderPhysicsObjects(btDiscreteDynamicsWorld* world, const glm::mat4& view, const glm::mat4& projection) {
    if (!m_rasterShader || !m_rasterShader->isValid() || !world) return;
    m_rasterShader->use();
    m_rasterShader->setMat4("view", view); m_rasterShader->setMat4("projection", projection);
    bool textureWasBound = false;
    if (textureLoaded && texture) {
         glActiveTexture(GL_TEXTURE0); texture->Bind(0);
         m_rasterShader->setInt("texture1", 0); m_rasterShader->setBool("useTexture", true);
         textureWasBound = true;
    } else { m_rasterShader->setBool("useTexture", false); }
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
                modelMatrix = glm::scale(modelMatrix, glm::vec3(radius * 2.0f));
            }
            m_rasterShader->setMat4("model", modelMatrix);
            glBindVertexArray(m_cubeVAO);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_cubeIndexCount), GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }
    }
     if (textureWasBound && texture) { texture->Unbind(); }
     glUseProgram(0);
}

// --- End Physics Rendering ---

// --- UI Panel Rendering Functions ---

void Renderer::RenderUIToolbar() {
    ImGui::Begin("Toolbar");
    if (m_editorState == EditorState::EDITING) {
        if (ImGui::Button("Play")) {
            m_editorState = EditorState::PLAYING;
            if (m_lockMouseInPlayMode) { glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); firstMouse = true; }
        }
    } else {
        if (ImGui::Button("Stop")) {
            m_editorState = EditorState::EDITING;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
    ImGui::SameLine(); ImGui::Separator(); ImGui::SameLine();
    if (ImGui::Button("Import Asset")) {
        char const * lFilterPatterns[2] = { "*.fbx", "*.obj" };
        char const * selectedFilePath = tinyfd_openFileDialog("Import 3D Model", "", 2, lFilterPatterns, "3D Models (.fbx, .obj)", 0);
        if (selectedFilePath) {
            std::cout << "Import Asset: Selected file: " << selectedFilePath << std::endl;
            auto mesh = LoadMeshFromFile(selectedFilePath);
            if (mesh) {
                m_sceneAssets.push_back({selectedFilePath, glm::vec3(0, 0, 0), std::move(mesh)});
            } else {
                std::cerr << "Failed to load mesh for asset: " << selectedFilePath << std::endl;
            }
        }
    }

    ImGui::SameLine(); ImGui::Checkbox("Lock Mouse in Play", &m_lockMouseInPlayMode);
    ImGui::End();
}

void Renderer::RenderUIInspector() {
    ImGui::Begin("Inspector");
    ImGui::Text("Scene Assets:");
    ImGui::Separator();
    for (int i = 0; i < m_sceneAssets.size(); ++i) {
        bool selected = (i == m_selectedAsset);
        if (ImGui::Selectable(m_sceneAssets[i].name.c_str(), selected)) {
            m_selectedAsset = i;
        }
        if (selected) {
            ImGui::Indent();
            ImGui::Text("Position: (%.2f, %.2f, %.2f)", m_sceneAssets[i].position.x, m_sceneAssets[i].position.y, m_sceneAssets[i].position.z);
            if (ImGui::Button("Focus Camera")) {
                camera.Position = m_sceneAssets[i].position + glm::vec3(0, 2, 5);
                camera.Yaw = -90.0f; camera.Pitch = 0.0f; camera.updateCameraVectors();
            }
            ImGui::Unindent();
        }
    }
    ImGui::End();
}

void Renderer::RenderUIStats() {
    ImGui::Begin("Stats");
    ImGui::Text("FPS: %.1f", fps); ImGui::Text("Frame Time: %.3f ms", deltaTime * 1000.0f);
    ImGui::End();
}

void Renderer::RenderUISceneControls() {
    ImGui::Begin("Scene Controls");
    if (ImGui::CollapsingHeader("Lighting", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::DragFloat3("Light Direction", &m_lightDirection.x, 0.01f)) { m_lightDirection = glm::normalize(m_lightDirection); }
        ImGui::ColorEdit3("Light Color", &m_lightColor.x);
        ImGui::DragFloat("Ambient Strength", &m_ambientStrength, 0.005f, 0.0f, 1.0f);
    }
    if (ImGui::CollapsingHeader("Camera Info")) {
        ImGui::Text("Pos: (%.1f, %.1f, %.1f)", camera.Position.x, camera.Position.y, camera.Position.z);
        ImGui::Text("Yaw: %.1f Pitch: %.1f", camera.Yaw, camera.Pitch);
        ImGui::DragFloat("Zoom", &camera.Zoom, 0.1f, 1.0f, 90.0f);
    }
    if (ImGui::CollapsingHeader("Terrain", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::DragFloat("Base Frequency", &m_terrain_base_freq, 0.01f, 0.01f, 2.0f);
        ImGui::DragFloat("Base Amplitude", &m_terrain_base_amp, 0.1f, 0.1f, 10.0f);
        ImGui::DragFloat("Persistence", &m_terrain_persistence, 0.01f, 0.1f, 1.0f);
        ImGui::DragFloat("Flatten Power", &m_terrain_flatten_power, 0.05f, 0.5f, 5.0f);
        ImGui::DragFloat("Final Scale", &m_terrain_final_scale, 0.1f, 0.1f, 10.0f);
        ImGui::Text("Octaves: %d (Requires recompile)", m_terrain_octaves);
    }
    // --- ADDED Cloud Controls ---
     if (ImGui::CollapsingHeader("Clouds", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::DragFloat("Base Height", &m_cloud_base_height, 0.1f, 0.0f, 50.0f);
        ImGui::DragFloat("Thickness", &m_cloud_thickness, 0.1f, 1.0f, 50.0f);
        ImGui::Separator();
        ImGui::DragFloat("Noise Scale", &m_cloud_noise_scale, 0.01f, 0.01f, 1.0f);
        ImGui::DragFloat("Coverage Min", &m_cloud_coverage_min, 0.01f, 0.0f, 1.0f);
        ImGui::DragFloat("Coverage Max", &m_cloud_coverage_max, 0.01f, 0.0f, 1.0f);
        // Ensure min is not greater than max
        m_cloud_coverage_max = glm::max(m_cloud_coverage_min + 0.01f, m_cloud_coverage_max);
        ImGui::Separator();
        ImGui::DragFloat("Density Factor", &m_cloud_density_factor, 0.05f, 0.0f, 5.0f);
     }
     // --- End Cloud Controls ---
    ImGui::End();
}

void Renderer::RenderUIHierarchy(btDiscreteDynamicsWorld* world) {
    ImGui::Begin("Hierarchy");
    ImGui::Text("Scene Objects:"); ImGui::Separator();
    if (world) {
        int numObjects = world->getNumCollisionObjects();
        for (int i = 0; i < numObjects; ++i) {
            btCollisionObject* obj = world->getCollisionObjectArray()[i];
            btRigidBody* body = btRigidBody::upcast(obj);
            std::string objectName = "Object " + std::to_string(i);
            if (body) { objectName += (body->getInvMass() == 0.0f) ? " (Static)" : " (Dynamic)";
                if (body->getCollisionShape()->getShapeType() == SPHERE_SHAPE_PROXYTYPE) objectName += " - Sphere";
                else if (body->getCollisionShape()->getShapeType() == BOX_SHAPE_PROXYTYPE) objectName += " - Box";
            } else { objectName += " (CollisionObject)"; }
            if (ImGui::Selectable(objectName.c_str())) { std::cout << "Selected: " << objectName << std::endl; }
        }
    } else { ImGui::Text("Physics world not available."); }
    ImGui::End();
}

// --- RenderUI (Main Setup) ---
void Renderer::RenderUI() {
    ImGui_ImplOpenGL3_NewFrame(); ImGui_ImplGlfw_NewFrame(); ImGui::NewFrame();

    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos); ImGui::SetNextWindowSize(viewport->WorkSize); ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f); ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) window_flags |= ImGuiWindowFlags_NoBackground;
    ImGui::Begin("DockSpace", nullptr, window_flags);
    ImGui::PopStyleVar(3);

    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    }

    RenderUIToolbar();
    RenderUIStats();
    RenderUISceneControls();
    RenderUIInspector(); // Inspector panel

    ImGui::End();
    ImGui::Render(); ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// --- Update Method ---
void Renderer::Update(float dt) {
    this->deltaTime = dt; // Store delta time from main loop
    frameCount++;
    float currentTime = (float)glfwGetTime();
    if (currentTime - fpsUpdateTime >= 0.5f) {
        fps = static_cast<double>(frameCount) / (currentTime - fpsUpdateTime);
        fpsUpdateTime = currentTime;
        frameCount = 0;
    }
}

// --- Render Method ---
void Renderer::Render(btDiscreteDynamicsWorld* dynamicsWorld) {
    if (!m_raymarchShader || !m_raymarchShader->isValid()) {
        glClearColor(1.0f, 0.0f, 1.0f, 1.0f); glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        RenderUI(); glfwSwapBuffers(window); return;
    }

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // --- 1. Render Raymarched Terrain ---
    m_raymarchShader->use();
    if (m_raymarch_timeLoc != -1) glUniform1f(m_raymarch_timeLoc, (float)glfwGetTime());
    if (m_raymarch_camPosLoc != -1) glUniform3fv(m_raymarch_camPosLoc, 1, glm::value_ptr(camera.Position));
    if (m_raymarch_lightDirLoc != -1) glUniform3fv(m_raymarch_lightDirLoc, 1, glm::value_ptr(m_lightDirection));
    if (m_raymarch_lightColorLoc != -1) glUniform3fv(m_raymarch_lightColorLoc, 1, glm::value_ptr(m_lightColor));
    if (m_raymarch_ambientLoc != -1) glUniform1f(m_raymarch_ambientLoc, m_ambientStrength);
    if (m_raymarch_terrain_base_freqLoc != -1) glUniform1f(m_raymarch_terrain_base_freqLoc, m_terrain_base_freq);
    if (m_raymarch_terrain_base_ampLoc != -1) glUniform1f(m_raymarch_terrain_base_ampLoc, m_terrain_base_amp);
    if (m_raymarch_terrain_persistenceLoc != -1) glUniform1f(m_raymarch_terrain_persistenceLoc, m_terrain_persistence);
    if (m_raymarch_terrain_flatten_powerLoc != -1) glUniform1f(m_raymarch_terrain_flatten_powerLoc, m_terrain_flatten_power);
    if (m_raymarch_terrain_final_scaleLoc != -1) glUniform1f(m_raymarch_terrain_final_scaleLoc, m_terrain_final_scale);
    if (m_raymarch_terrain_octavesLoc != -1) glUniform1i(m_raymarch_terrain_octavesLoc, m_terrain_octaves);
    if (m_raymarch_cloud_base_heightLoc != -1) glUniform1f(m_raymarch_cloud_base_heightLoc, m_cloud_base_height);
    if (m_raymarch_cloud_thicknessLoc != -1) glUniform1f(m_raymarch_cloud_thicknessLoc, m_cloud_thickness);
    if (m_raymarch_cloud_noise_scaleLoc != -1) glUniform1f(m_raymarch_cloud_noise_scaleLoc, m_cloud_noise_scale);
    if (m_raymarch_cloud_coverage_minLoc != -1) glUniform1f(m_raymarch_cloud_coverage_minLoc, m_cloud_coverage_min);
    if (m_raymarch_cloud_coverage_maxLoc != -1) glUniform1f(m_raymarch_cloud_coverage_maxLoc, m_cloud_coverage_max);
    if (m_raymarch_cloud_density_factorLoc != -1) glUniform1f(m_raymarch_cloud_density_factorLoc, m_cloud_density_factor);

    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), static_cast<float>(width) / static_cast<float>(height), 0.1f, 200.0f);
    glm::mat4 invView = glm::inverse(view);
    glm::mat4 invProj = glm::inverse(projection);
    if (m_raymarch_invViewLoc != -1) glUniformMatrix4fv(m_raymarch_invViewLoc, 1, GL_FALSE, glm::value_ptr(invView));
    if (m_raymarch_invProjLoc != -1) glUniformMatrix4fv(m_raymarch_invProjLoc, 1, GL_FALSE, glm::value_ptr(invProj));

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    glUseProgram(0);
    // --- End Terrain Rendering ---

    // --- 2. Render Physics Objects ---
    glEnable(GL_DEPTH_TEST);
    renderPhysicsObjects(dynamicsWorld, view, projection);

    // --- Render Imported Assets ---
    for (const auto& asset : m_sceneAssets) {
        if (!asset.mesh) {
            std::cerr << "Asset mesh is null!" << std::endl;
            continue;
        }
        glm::mat4 model = glm::translate(glm::mat4(1.0f), asset.position);
        m_rasterShader->use();
        m_rasterShader->setMat4("model", model);
        m_rasterShader->setMat4("view", view);
        m_rasterShader->setMat4("projection", projection);
        std::cout << "Rendering asset: " << asset.name << " at position: " << asset.position.x << ", " << asset.position.y << ", " << asset.position.z << std::endl;
        asset.mesh->Draw();
    }

    // --- End Physics Object Rendering ---

    // --- 3. Render UI ---
    glUseProgram(0);
    RenderUI();
    // --- End UI ---

    glfwSwapBuffers(window);
}


void Renderer::ProcessInput() {
    ImGuiIO& io = ImGui::GetIO();

    // --- Pause/Stop via Escape Key ---
    static bool escapePressedLastFrame = false;
    bool escapePressedThisFrame = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    if (m_editorState == EditorState::PLAYING && escapePressedThisFrame && !escapePressedLastFrame) {
        m_editorState = EditorState::EDITING;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        firstMouse = true;
        std::cout << "Game Stopped (Escape). Cursor Enabled." << std::endl;
    }
    escapePressedLastFrame = escapePressedThisFrame;
    // --- End Escape Key Logic ---

    // --- EDITOR MODE: Free-fly and Orbit Camera ---
    if (m_editorState == EditorState::EDITING) {
        // Free-fly camera: RMB + WASD + mouse look
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS && !io.WantCaptureMouse) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

            // WASD movement
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.ProcessKeyboard(FORWARD, deltaTime);
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.ProcessKeyboard(BACKWARD, deltaTime);
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.ProcessKeyboard(LEFT, deltaTime);
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.ProcessKeyboard(RIGHT, deltaTime);

            // Mouse look
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            static double lastX = width / 2.0, lastY = height / 2.0;
            static bool firstMouse = true;
            if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }
            float xoffset = xpos - lastX;
            float yoffset = lastY - ypos;
            lastX = xpos; lastY = ypos;
            camera.ProcessMouseMovement(xoffset, yoffset);
        }
        // Orbit camera: Alt + LMB
        else if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS &&
                 glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS &&
                 !io.WantCaptureMouse && m_selectedAsset >= 0 && m_selectedAsset < m_sceneAssets.size()) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            static double lastX = width / 2.0, lastY = height / 2.0;
            static bool firstOrbit = true;
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            if (firstOrbit) { lastX = xpos; lastY = ypos; firstOrbit = false; }
            float xoffset = xpos - lastX;
            float yoffset = lastY - ypos;
            lastX = xpos; lastY = ypos;
            glm::vec3 target = m_sceneAssets[m_selectedAsset].position;
            float radius = glm::length(camera.Position - target);
            float azimuth = atan2(camera.Position.z - target.z, camera.Position.x - target.x);
            float elevation = asin((camera.Position.y - target.y) / radius);
            azimuth -= xoffset * 0.005f;
            elevation += yoffset * 0.005f;
            elevation = glm::clamp(elevation, -glm::half_pi<float>() + 0.1f, glm::half_pi<float>() - 0.1f);
            camera.Position.x = target.x + radius * cos(elevation) * cos(azimuth);
            camera.Position.y = target.y + radius * sin(elevation);
            camera.Position.z = target.z + radius * cos(elevation) * sin(azimuth);
            camera.Yaw = glm::degrees(-azimuth) + 90.0f;
            camera.Pitch = glm::degrees(elevation);
            camera.updateCameraVectors();
        }
        else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
    // --- PLAYING MODE ---
    else if (m_editorState == EditorState::PLAYING) {
        static bool tabPressedLastFrame = false;
        bool tabPressedThisFrame = glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS;
        if (tabPressedThisFrame && !tabPressedLastFrame) {
            if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); firstMouse = true;
                std::cout << "Mouse Cursor Enabled (Tab)" << std::endl;
            } else {
                if (m_lockMouseInPlayMode) {
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                    std::cout << "Mouse Cursor Disabled (Tab)" << std::endl;
                }
            }
        }
        tabPressedLastFrame = tabPressedThisFrame;

        if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.ProcessKeyboard(FORWARD, deltaTime);
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.ProcessKeyboard(BACKWARD, deltaTime);
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.ProcessKeyboard(LEFT, deltaTime);
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.ProcessKeyboard(RIGHT, deltaTime);
        }

        static bool rPressedLastFrame = false;
        bool rPressedThisFrame = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;
        if (rPressedThisFrame && !rPressedLastFrame) {
            std::cout << "Reload key pressed, attempting texture reload..." << std::endl;
            if (LoadTextureFromDirectories()) { std::cout << "Texture reloaded/found successfully via keypress." << std::endl; }
            else { std::cout << "Texture reload/find via keypress failed." << std::endl; }
        }
        rPressedLastFrame = rPressedThisFrame;
    }
}


// --- Static Callback Functions ---
void Renderer::framebuffer_size_callback(GLFWwindow* window, int newWidth, int newHeight) {
    if (newWidth <= 0 || newHeight <= 0) return;
    glViewport(0, 0, newWidth, newHeight);
    Renderer* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
    if (renderer) { renderer->width = newWidth; renderer->height = newHeight; }
}

void Renderer::mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    ImGuiIO& io = ImGui::GetIO(); if (io.WantCaptureMouse) return;
    Renderer* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window)); if (!renderer) return;
    if (renderer->m_editorState == EditorState::PLAYING && glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
        float xpos = static_cast<float>(xposIn); float ypos = static_cast<float>(yposIn);
        if (renderer->firstMouse) { renderer->lastX = xpos; renderer->lastY = ypos; renderer->firstMouse = false; }
        float xoffset = xpos - renderer->lastX; float yoffset = renderer->lastY - ypos;
        renderer->lastX = xpos; renderer->lastY = ypos;
        renderer->camera.ProcessMouseMovement(xoffset, yoffset);
    }
}

void Renderer::scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    ImGuiIO& io = ImGui::GetIO(); if (io.WantCaptureMouse) return;
    Renderer* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
    if (renderer && renderer->m_editorState == EditorState::PLAYING) {
        if (callbackCamera) { callbackCamera->ProcessMouseScroll(static_cast<float>(yoffset)); }
    }
}

// --- CreateCube Method ---
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
    return Mesh(vertices, indices);
}

// --- Helper function to load a mesh from file (very basic, only loads first mesh) ---
std::unique_ptr<Mesh> Renderer::LoadMeshFromFile(const std::string& path) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_JoinIdenticalVertices);
    if (!scene || !scene->HasMeshes()) {
        std::cerr << "Assimp failed to load mesh: " << path << std::endl;
        return nullptr;
    }
    const aiMesh* mesh = scene->mMeshes[0];
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
        glm::vec3 pos(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        glm::vec3 color(1.0f, 1.0f, 1.0f);
        glm::vec2 uv(0.0f, 0.0f);
        if (mesh->HasTextureCoords(0))
            uv = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
        vertices.emplace_back(pos, color, uv);
    }
    for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
        for (unsigned int j = 0; j < mesh->mFaces[i].mNumIndices; ++j)
            indices.push_back(mesh->mFaces[i].mIndices[j]);
    }
    std::cout << "Loaded mesh: " << path << std::endl;
    return std::make_unique<Mesh>(vertices, indices);
}