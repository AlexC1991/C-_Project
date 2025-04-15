#ifndef RENDERER_H
#define RENDERER_H

#include <Camera.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string>
#include <memory>
#include "Shader.h"
#include "Mesh.h"
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

struct GLFWwindow;
class Camera;
class Texture;
class Mesh;
class btRigidBody;
class btDiscreteDynamicsWorld;

enum class EditorState { EDITING, PLAYING };

// --- NEW: SceneAsset struct ---
struct SceneAsset {
    std::string name;
    glm::vec3 position;
    std::unique_ptr<Mesh> mesh; // Store the loaded mesh
};

class Renderer {
public:
    Renderer(int width, int height, const char* title);
    ~Renderer();

    bool Initialize();
    void ProcessInput();
    void Update(float dt);
    void Render(btDiscreteDynamicsWorld* dynamicsWorld);

    GLFWwindow* getWindow() const { return window; }
    float getDeltaTime() const { return deltaTime; }
    EditorState GetEditorState() const { return m_editorState; }

    bool isPaused = false;

private:
    GLFWwindow* window;
    int width, height;
    const char* title;
    float deltaTime;
    float lastFrame;
    unsigned long long frameCount;
    double fps;
    double fpsUpdateTime;
    Camera camera;
    float lastX, lastY;
    bool firstMouse;
    EditorState m_editorState = EditorState::EDITING;
    bool m_lockMouseInPlayMode = true;
    glm::mat4 lastModelMatrix;

    unsigned int quadVAO;
    unsigned int quadVBO;

    Texture* texture;
    bool textureLoaded;
    bool needToRebindTexture;

    std::unique_ptr<Shader> m_raymarchShader;
    std::unique_ptr<Shader> m_rasterShader;

    GLint m_raymarch_timeLoc, m_raymarch_camPosLoc, m_raymarch_invViewLoc, m_raymarch_invProjLoc;
    GLint m_raymarch_lightDirLoc, m_raymarch_lightColorLoc, m_raymarch_ambientLoc;
    GLint m_raymarch_terrain_base_freqLoc, m_raymarch_terrain_base_ampLoc, m_raymarch_terrain_persistenceLoc;
    GLint m_raymarch_terrain_flatten_powerLoc, m_raymarch_terrain_final_scaleLoc, m_raymarch_terrain_octavesLoc;
    GLint m_raymarch_cloud_base_heightLoc, m_raymarch_cloud_thicknessLoc, m_raymarch_cloud_noise_scaleLoc;
    GLint m_raymarch_cloud_coverage_minLoc, m_raymarch_cloud_coverage_maxLoc, m_raymarch_cloud_density_factorLoc;

    glm::vec3 m_lightDirection;
    glm::vec3 m_lightColor;
    float m_ambientStrength;

    float m_terrain_base_freq, m_terrain_base_amp, m_terrain_persistence, m_terrain_flatten_power, m_terrain_final_scale;
    int   m_terrain_octaves;
    float m_cloud_base_height, m_cloud_thickness, m_cloud_noise_scale, m_cloud_coverage_min, m_cloud_coverage_max, m_cloud_density_factor;

    unsigned int m_cubeVAO, m_cubeVBO, m_cubeEBO;
    size_t m_cubeIndexCount;

    std::vector<SceneAsset> m_sceneAssets;
    int m_selectedAsset = -1;

    void InitImGui();
    void ShutdownImGui();
    bool LoadTextureFromDirectories();
    void setupScreenQuad();
    void setupRasterShader();
    void setupPhysicsMeshes();

    void RenderUI();
    void RenderUIToolbar();
    void RenderUIStats();
    void RenderUISceneControls();

    void RenderUIHierarchy(btDiscreteDynamicsWorld *world);

    void RenderUIInspector();
    void renderPhysicsObjects(btDiscreteDynamicsWorld* world, const glm::mat4& view, const glm::mat4& projection);
    glm::mat4 convertBtTransformToGlm(const class btTransform& trans);
    Mesh CreateCube();

    std::unique_ptr<Mesh> LoadMeshFromFile(const std::string& path);

    static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
    static void mouse_callback(GLFWwindow* window, double xpos, double ypos);
    static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
};

#endif // RENDERER_H