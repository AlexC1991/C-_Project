#include <GL/glew.h>    // Include GLEW first
#include <GLFW/glfw3.h> // Then GLFW
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <vector>
#include <memory> // For smart pointers

#include "Renderer.h" // Your renderer class

// --- Bullet Includes ---
#include <btBulletDynamicsCommon.h>
// --- End Bullet Includes ---

// --- Physics Globals (Consider wrapping these in a PhysicsManager class later) ---
std::unique_ptr<btDefaultCollisionConfiguration> g_collisionConfiguration;
std::unique_ptr<btCollisionDispatcher> g_dispatcher;
std::unique_ptr<btBroadphaseInterface> g_overlappingPairCache;
std::unique_ptr<btSequentialImpulseConstraintSolver> g_solver;
std::unique_ptr<btDiscreteDynamicsWorld> g_dynamicsWorld;
// Store shapes and bodies separately for proper cleanup
std::vector<std::unique_ptr<btCollisionShape>> g_collisionShapes;
std::vector<btRigidBody*> g_physicsBodies; // Raw pointers okay if managed by world
// --- End Physics Globals ---

// --- Physics Initialization ---
void initPhysics() {
    std::cout << "Initializing Bullet Physics..." << std::endl;
    g_collisionConfiguration = std::make_unique<btDefaultCollisionConfiguration>();
    g_dispatcher = std::make_unique<btCollisionDispatcher>(g_collisionConfiguration.get());
    g_overlappingPairCache = std::make_unique<btDbvtBroadphase>();
    g_solver = std::make_unique<btSequentialImpulseConstraintSolver>();
    g_dynamicsWorld = std::make_unique<btDiscreteDynamicsWorld>(
        g_dispatcher.get(),
        g_overlappingPairCache.get(),
        g_solver.get(),
        g_collisionConfiguration.get()
    );

    g_dynamicsWorld->setGravity(btVector3(0, -9.81, 0)); // Set gravity

    // --- Create Ground Plane ---
    // Store shape in our vector using unique_ptr
    g_collisionShapes.push_back(std::make_unique<btStaticPlaneShape>(btVector3(0, 1, 0), 0));
    btCollisionShape* groundShape = g_collisionShapes.back().get(); // Get raw pointer for construction info

    btDefaultMotionState* groundMotionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, 0, 0)));
    btRigidBody::btRigidBodyConstructionInfo groundRigidBodyCI(0, groundMotionState, groundShape, btVector3(0, 0, 0));
    btRigidBody* groundRigidBody = new btRigidBody(groundRigidBodyCI);
    groundRigidBody->setRestitution(0.3f); // Ground has some bounce
    g_dynamicsWorld->addRigidBody(groundRigidBody);
    // Don't add static bodies to g_physicsBodies if we only track dynamic ones for rendering/cleanup

    // --- Create a Falling Cube ---
    float cubeHalfExtents = 0.5f;
    // Store shape
    g_collisionShapes.push_back(std::make_unique<btBoxShape>(btVector3(cubeHalfExtents, cubeHalfExtents, cubeHalfExtents)));
    btCollisionShape* fallShape = g_collisionShapes.back().get();

    btDefaultMotionState* fallMotionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, 20, 0))); // Start cube high
    btScalar mass = 1.f; btVector3 fallInertia(0, 0, 0);
    fallShape->calculateLocalInertia(mass, fallInertia);
    btRigidBody::btRigidBodyConstructionInfo fallRigidBodyCI(mass, fallMotionState, fallShape, fallInertia);
    btRigidBody* fallRigidBody = new btRigidBody(fallRigidBodyCI);
    fallRigidBody->setRestitution(0.6f); // Cube bounce
    g_dynamicsWorld->addRigidBody(fallRigidBody);
    g_physicsBodies.push_back(fallRigidBody); // Store pointer for rendering and cleanup

    // --- Create a Bouncing Sphere ---
    float sphereRadius = 0.6f; // Slightly larger than cube half-extent
    // Store shape
    g_collisionShapes.push_back(std::make_unique<btSphereShape>(sphereRadius));
    btCollisionShape* sphereShape = g_collisionShapes.back().get();

    // Start sphere next to the cube, slightly lower
    btDefaultMotionState* sphereMotionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), btVector3(2, 15, 0)));
    btScalar sphereMass = 1.5f; // Make it slightly heavier
    btVector3 sphereInertia(0, 0, 0);
    sphereShape->calculateLocalInertia(sphereMass, sphereInertia);
    btRigidBody::btRigidBodyConstructionInfo sphereRigidBodyCI(sphereMass, sphereMotionState, sphereShape, sphereInertia);
    btRigidBody* sphereRigidBody = new btRigidBody(sphereRigidBodyCI);
    sphereRigidBody->setRestitution(0.9f); // << Make sphere very bouncy
    sphereRigidBody->setFriction(0.1f);
    g_dynamicsWorld->addRigidBody(sphereRigidBody);
    g_physicsBodies.push_back(sphereRigidBody); // Store pointer for rendering and cleanup
    // --- End Bouncing Sphere ---


    std::cout << "Bullet Physics Initialized." << std::endl;
}

// --- Physics Cleanup ---
void cleanupPhysics() {
    std::cout << "Cleaning up Bullet Physics..." << std::endl;
    // Remove rigid bodies from the world
    // Iterate backwards safely
    for (int i = g_dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--) {
        btCollisionObject* obj = g_dynamicsWorld->getCollisionObjectArray()[i];
        btRigidBody* body = btRigidBody::upcast(obj);
        // Delete motion state ONLY if the body has one (static plane might not)
        if (body && body->getMotionState()) {
            delete body->getMotionState();
        }
        g_dynamicsWorld->removeCollisionObject(obj);
        // IMPORTANT: The rigid body itself is owned by the world,
        // removing it from the world DELETES the btRigidBody object.
        // Do NOT delete obj here.
    }
    g_physicsBodies.clear(); // Clear our vector of raw pointers

    // Collision shapes must be deleted AFTER rigid bodies are removed/deleted
    g_collisionShapes.clear(); // unique_ptrs handle deletion

    // Delete remaining Bullet components (handled by unique_ptrs)
    g_dynamicsWorld.reset();
    g_solver.reset();
    g_overlappingPairCache.reset();
    g_dispatcher.reset();
    g_collisionConfiguration.reset();

    std::cout << "Bullet Physics Cleaned up." << std::endl;
}


int main() {
    std::cout << "Starting application..." << std::endl;

    // --- Initialize Physics FIRST ---
    initPhysics();
    // ---

    // Create renderer (adjust size/title as needed)
    Renderer renderer(1280, 720, "Raymarching + Physics");
    std::cout << "Renderer created, initializing..." << std::endl;

    // Initialize renderer (GLFW, GLEW, ImGui, Shaders, Meshes)
    if (!renderer.Initialize()) {
        std::cerr << "Failed to initialize renderer!" << std::endl;
        cleanupPhysics(); // Cleanup physics even if renderer fails
        glfwTerminate(); // Ensure GLFW terminates if init failed partially
        return -1;
    }

    std::cout << "Initialization successful, starting render loop..." << std::endl;

    // --- Main Loop ---
    float lastFrameTime = (float)glfwGetTime();

    while (!glfwWindowShouldClose(renderer.getWindow())) {

        // --- Calculate Delta Time ---
        float currentFrameTime = (float)glfwGetTime();
        float deltaTime = currentFrameTime - lastFrameTime;
        lastFrameTime = currentFrameTime;
        // Clamp delta time
        if (deltaTime > 0.1f) { deltaTime = 0.1f; }
        if (deltaTime <= 0.0f) { deltaTime = 1.0f / 60.0f; }

        // --- Input ---
        glfwPollEvents();       // Poll events first
        renderer.ProcessInput(); // Handle input (updates renderer.isPaused)

        // --- Physics Update ---
        if (!renderer.isPaused) { // Use public member or getter
             g_dynamicsWorld->stepSimulation(deltaTime, 10, 1.0f/60.0f);
        }

        // --- Game Logic Update ---
        renderer.Update(deltaTime); // Call renderer's update function

        // --- Rendering ---
        renderer.Render(g_dynamicsWorld.get()); // Pass physics world to Render

    }
    // --- End Main Loop ---

    std::cout << "Render loop finished, exiting..." << std::endl;

    // --- Cleanup Physics LAST ---
    cleanupPhysics();
    // ---

    // Renderer destructor handles its own cleanup
    return 0;
}