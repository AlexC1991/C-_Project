#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <vector>
#include <memory>

#include "Renderer.h" // Your renderer class
#include <btBulletDynamicsCommon.h> // Bullet includes

// --- Physics Globals ---
std::unique_ptr<btDefaultCollisionConfiguration> g_collisionConfiguration;
std::unique_ptr<btCollisionDispatcher> g_dispatcher;
std::unique_ptr<btBroadphaseInterface> g_overlappingPairCache;
std::unique_ptr<btSequentialImpulseConstraintSolver> g_solver;
std::unique_ptr<btDiscreteDynamicsWorld> g_dynamicsWorld;
std::vector<std::unique_ptr<btCollisionShape>> g_collisionShapes;
std::vector<btRigidBody*> g_physicsBodies;
// --- End Physics Globals ---

// --- Physics Initialization ---
void initPhysics() {
    std::cout << "Initializing Bullet Physics..." << std::endl;
    g_collisionConfiguration = std::make_unique<btDefaultCollisionConfiguration>();
    g_dispatcher = std::make_unique<btCollisionDispatcher>(g_collisionConfiguration.get());
    g_overlappingPairCache = std::make_unique<btDbvtBroadphase>();
    g_solver = std::make_unique<btSequentialImpulseConstraintSolver>();
    g_dynamicsWorld = std::make_unique<btDiscreteDynamicsWorld>(
        g_dispatcher.get(), g_overlappingPairCache.get(), g_solver.get(), g_collisionConfiguration.get()
    );
    g_dynamicsWorld->setGravity(btVector3(0, -9.81, 0));

    // Ground Plane
    g_collisionShapes.push_back(std::make_unique<btStaticPlaneShape>(btVector3(0, 1, 0), 0));
    btCollisionShape* groundShape = g_collisionShapes.back().get();
    btDefaultMotionState* groundMotionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, 0, 0)));
    btRigidBody::btRigidBodyConstructionInfo groundRigidBodyCI(0, groundMotionState, groundShape, btVector3(0, 0, 0));
    btRigidBody* groundRigidBody = new btRigidBody(groundRigidBodyCI);
    groundRigidBody->setRestitution(0.3f);
    g_dynamicsWorld->addRigidBody(groundRigidBody);

    // Falling Cube
    float cubeHalfExtents = 0.5f;
    g_collisionShapes.push_back(std::make_unique<btBoxShape>(btVector3(cubeHalfExtents, cubeHalfExtents, cubeHalfExtents)));
    btCollisionShape* fallShape = g_collisionShapes.back().get();
    btDefaultMotionState* fallMotionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, 20, 0)));
    btScalar mass = 1.f; btVector3 fallInertia(0, 0, 0);
    fallShape->calculateLocalInertia(mass, fallInertia);
    btRigidBody::btRigidBodyConstructionInfo fallRigidBodyCI(mass, fallMotionState, fallShape, fallInertia);
    btRigidBody* fallRigidBody = new btRigidBody(fallRigidBodyCI);
    fallRigidBody->setRestitution(0.6f);
    g_dynamicsWorld->addRigidBody(fallRigidBody);
    g_physicsBodies.push_back(fallRigidBody);

    // Bouncing Sphere
    float sphereRadius = 0.6f;
    g_collisionShapes.push_back(std::make_unique<btSphereShape>(sphereRadius));
    btCollisionShape* sphereShape = g_collisionShapes.back().get();
    btDefaultMotionState* sphereMotionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), btVector3(2, 15, 0)));
    btScalar sphereMass = 1.5f; btVector3 sphereInertia(0, 0, 0);
    sphereShape->calculateLocalInertia(sphereMass, sphereInertia);
    btRigidBody::btRigidBodyConstructionInfo sphereRigidBodyCI(sphereMass, sphereMotionState, sphereShape, sphereInertia);
    btRigidBody* sphereRigidBody = new btRigidBody(sphereRigidBodyCI);
    sphereRigidBody->setRestitution(0.9f); sphereRigidBody->setFriction(0.1f);
    g_dynamicsWorld->addRigidBody(sphereRigidBody);
    g_physicsBodies.push_back(sphereRigidBody);

    std::cout << "Bullet Physics Initialized." << std::endl;
}

// --- Physics Cleanup ---
void cleanupPhysics() {
    std::cout << "Cleaning up Bullet Physics..." << std::endl;
    for (int i = g_dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--) {
        btCollisionObject* obj = g_dynamicsWorld->getCollisionObjectArray()[i];
        btRigidBody* body = btRigidBody::upcast(obj);
        if (body && body->getMotionState()) { delete body->getMotionState(); }
        g_dynamicsWorld->removeCollisionObject(obj);
    }
    g_physicsBodies.clear();
    g_collisionShapes.clear();
    g_dynamicsWorld.reset(); g_solver.reset(); g_overlappingPairCache.reset();
    g_dispatcher.reset(); g_collisionConfiguration.reset();
    std::cout << "Bullet Physics Cleaned up." << std::endl;
}


int main() {
    std::cout << "Starting application..." << std::endl;

    initPhysics();

    Renderer renderer(1280, 720, "Raymarching + Physics Editor"); // Updated Title
    std::cout << "Renderer created, initializing..." << std::endl;

    if (!renderer.Initialize()) {
        std::cerr << "Failed to initialize renderer!" << std::endl;
        cleanupPhysics();
        glfwTerminate();
        return -1;
    }

    std::cout << "Initialization successful, starting main loop..." << std::endl;

    // --- Main Loop ---
    float lastFrameTime = (float)glfwGetTime();

    while (!glfwWindowShouldClose(renderer.getWindow())) {

        // --- Calculate Delta Time ---
        float currentFrameTime = (float)glfwGetTime();
        float deltaTime = currentFrameTime - lastFrameTime;
        lastFrameTime = currentFrameTime;
        if (deltaTime > 0.1f) { deltaTime = 0.1f; } // Clamp dt
        if (deltaTime <= 0.0f) { deltaTime = 1.0f / 60.0f; }

        // --- Input ---
        glfwPollEvents();
        renderer.ProcessInput(); // Handles mode switching and game input

        // --- Physics Update (conditional) ---
        if (renderer.GetEditorState() == EditorState::PLAYING) { // Use getter
             g_dynamicsWorld->stepSimulation(deltaTime, 10, 1.0f/60.0f);
        }

        // --- Game Logic Update ---
        renderer.Update(deltaTime); // Update renderer internals (FPS, etc.)

        // --- Rendering ---
        renderer.Render(g_dynamicsWorld.get()); // Pass physics world

    }
    // --- End Main Loop ---

    std::cout << "Main loop finished, exiting..." << std::endl;

    cleanupPhysics();

    // Renderer destructor handles its own cleanup
    return 0;
}