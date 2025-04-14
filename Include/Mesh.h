#ifndef MESH_H
#define MESH_H

#include <iostream> // For std::cout in print()
#include <vector>   // For std::vector
#include <string>   // Often useful, though not directly used here yet

// Include GLEW before GLFW (good practice)
#include <GL/glew.h>
// #include <GLFW/glfw3.h> // GLFW usually not needed directly in Mesh.h

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> // Often used with meshes, though not directly here
#include <glm/gtc/type_ptr.hpp>         // For passing data to OpenGL

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Color;     // Matches shader input `aColor`
    glm::vec2 TexCoords; // Matches shader input `aTexCoord`

    // --- ADDED CONSTRUCTOR ---
    // Constructor to initialize all members
    Vertex(const glm::vec3& pos, const glm::vec3& col, const glm::vec2& uv)
        : Position(pos), Color(col), TexCoords(uv) {}
    // -------------------------

    // Print function for debugging
    void print() const {
        std::cout << "Position: (" << Position.x << ", " << Position.y << ", " << Position.z << "), "
                  << "Color: (" << Color.r << ", " << Color.g << ", " << Color.b << "), "
                  << "TexCoords: (" << TexCoords.x << ", " << TexCoords.y << ")" << std::endl;
    }
}; // End of Vertex struct

class Mesh {
public:
    // Mesh data (public for easy access, consider getters if needed)
    std::vector<Vertex>       vertices;
    std::vector<unsigned int> indices;
    unsigned int              VAO; // Keep VAO public if Renderer needs to bind it directly

    // Constructor: Takes vertex and index data, then calls setupMesh
    Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);

    // Destructor (important for cleaning up OpenGL objects)
    ~Mesh(); // Added destructor declaration

    // Render the mesh (binds VAO and calls glDrawElements)
    void Draw();

private:
    // Render data - VBO (Vertex Buffer Object), EBO (Element Buffer Object)
    unsigned int VBO, EBO;

    // Initializes VAO, VBO, EBO, and sets up vertex attribute pointers
    void setupMesh();

    // Helper to delete OpenGL buffers (called by destructor)
    void cleanupMesh(); // Added cleanup helper declaration
};

#endif // MESH_H