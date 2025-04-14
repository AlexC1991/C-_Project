#include "Mesh.h" // Include the header file for the Mesh class definition
#include <GL/glew.h>
// #include <GLFW/glfw3.h> // Not usually needed in Mesh.cpp
#include <glm/glm.hpp>
// #include <glm/gtc/matrix_transform.hpp> // Not used here
#include <vector>   // Included via Mesh.h, but good to be explicit if needed elsewhere
#include <cstddef>  // Required for offsetof macro
#include <iostream> // For std::cout in cleanupMesh

// --- Constructor ---
// Uses member initializer list for efficiency.
// The assignments in the body were redundant and potentially less efficient.
Mesh::Mesh(const std::vector<Vertex>& verticesArg, const std::vector<unsigned int>& indicesArg)
    : vertices(verticesArg), indices(indicesArg), VAO(0), VBO(0), EBO(0) // Initialize buffer IDs to 0
{
    // Initializer list handles copying:
    // this->vertices = verticesArg; // Redundant, handled by initializer list
    // this->indices = indicesArg;  // Redundant, handled by initializer list

    // Now setup the OpenGL buffers
    setupMesh();
    std::cout << "Mesh created and setup." << std::endl; // Debug message
}

// --- Destructor Implementation (ADDED) ---
Mesh::~Mesh() {
    std::cout << "Mesh destructor called." << std::endl; // Debug message
    cleanupMesh(); // Call helper to delete buffers
}

// --- setupMesh Implementation ---
// Initializes VAO, VBO, EBO, and sets up vertex attribute pointers
void Mesh::setupMesh() {
    // 1. Create buffers/arrays
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    // Consider adding checkGLError calls here if you have that function available

    // 2. Bind VAO
    glBindVertexArray(VAO);

    // 3. Load data into vertex buffer (VBO)
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW); // Use .data()

    // 4. Load data into element buffer (EBO)
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW); // Use .data()

    // 5. Set the vertex attribute pointers
    // Vertex Positions (location = 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Position)); // Use offsetof for safety

    // Vertex Colors (location = 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Color));

    // Vertex Texture Coords (location = 2)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

    // 6. Unbind VAO (important!) - Unbinds VBO and EBO bindings associated with this VAO state
    glBindVertexArray(0);
    // Optional: Unbind buffers explicitly if needed elsewhere, but VAO unbind is key here
    // glBindBuffer(GL_ARRAY_BUFFER, 0);
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // Don't unbind EBO while VAO is bound
}

// --- Draw Implementation ---
// Binds VAO and calls glDrawElements
void Mesh::Draw() {
    // Bind the VAO containing all the buffer configuration
    glBindVertexArray(VAO);

    // Draw the mesh using indices
    // The EBO binding is remembered by the VAO
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0); // Cast size for safety

    // Unbind the VAO (good practice, prevents accidental modification)
    glBindVertexArray(0);
}


// --- Cleanup Implementation (ADDED) ---
// Helper function to delete OpenGL buffers
void Mesh::cleanupMesh() {
    std::cout << "Cleaning up mesh buffers (VAO: " << VAO << ", VBO: " << VBO << ", EBO: " << EBO << ")" << std::endl;
    // Check if buffers/arrays were generated (IDs > 0) before deleting
    if (EBO != 0) {
        glDeleteBuffers(1, &EBO);
        EBO = 0; // Reset ID after deletion
    }
    if (VBO != 0) {
        glDeleteBuffers(1, &VBO);
        VBO = 0; // Reset ID after deletion
    }
    if (VAO != 0) {
        glDeleteVertexArrays(1, &VAO);
        VAO = 0; // Reset ID after deletion
    }
    std::cout << "Mesh buffers cleanup finished." << std::endl;
}