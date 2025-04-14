#ifndef RENDERER_H
#define RENDERER_H

#include <Camera.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string>
#include <memory> // For unique_ptr

// Forward declarations
struct GLFWwindow;
class Camera;
// class Shader; // << FORWARD DECLARE Shader instead of including Shader.h
class Texture;
class Mesh;
struct ImGuiContext;
class btRigidBody;
class btDiscreteDynamicsWorld;

// --- FORWARD DECLARE Shader ---
class Shader;
// ---

class Renderer {
public:
    Renderer(int width, int height, const char* title);
    ~Renderer(); // << Ensure Destructor is DECLARED here

    bool Initialize();
    void ProcessInput();
    void Update(float dt);
    void Render(btDiscreteDynamicsWorld* dynamicsWorld);

    // Getters
    GLFWwindow* getWindow() const { return window; }
    float getDeltaTime() const { return deltaTime; }
    bool isPaused = false; // Public for main loop check

private:
    // Window
    GLFWwindow* window;
    int width, height;
    const char* title;

    // Timing
    float deltaTime;
    float lastFrame;
    unsigned long long frameCount;
    double fps;
    double fpsUpdateTime;

    // Camera
    Camera camera;
    float lastX, lastY;
    bool firstMouse;

    // Rendering State
    glm::mat4 lastModelMatrix;

    // Raymarching specific
    unsigned int quadVAO;
    unsigned int quadVBO;

    // Texture
    Texture* texture;
    bool textureLoaded;
    bool needToRebindTexture;

    // Physics Rendering
    // Use unique_ptr with forward-declared Shader
    std::unique_ptr<Shader> m_rasterShader; // << This requires ~Renderer() definition in .cpp
    unsigned int m_cubeVAO;
    unsigned int m_cubeVBO;
    unsigned int m_cubeEBO;
    size_t m_cubeIndexCount;

    // Initialization Helpers
    void InitImGui();
    void ShutdownImGui();
    bool LoadTextureFromDirectories();
    void setupScreenQuad();
    void setupRasterShader();
    void setupPhysicsMeshes();

    // Rendering Helpers
    void RenderUI();
    void renderPhysicsObjects(btDiscreteDynamicsWorld* world, const glm::mat4& view, const glm::mat4& projection);
    glm::mat4 convertBtTransformToGlm(const class btTransform& trans);
    Mesh CreateCube();

    // Callbacks (static)
    static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
    static void mouse_callback(GLFWwindow* window, double xpos, double ypos);
    static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
};

#endif // RENDERER_H