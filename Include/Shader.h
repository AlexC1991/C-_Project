#ifndef SHADER_H
#define SHADER_H

#include <GL/glew.h>
// #include <GLFW/glfw3.h> // Usually not needed directly in Shader.h
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp> // Often needed for uniform setters

#include <string>

class Shader {
public:
    // Program ID
    unsigned int ID;

    // Constructor reads and builds the shader
    Shader(const char* vertexPath, const char* fragmentPath);

    // Destructor (ADDED)
    ~Shader();

    // Use/activate the shader
    void use();

    // Method to check validity (ADDED)
    bool isValid() const;

    // Utility uniform functions
    void setBool(const std::string &name, bool value) const;
    void setInt(const std::string &name, int value) const;
    void setFloat(const std::string &name, float value) const;
    void setVec3(const std::string &name, const glm::vec3 &value) const;
    void setMat4(const std::string &name, const glm::mat4 &value) const;
    // Add any other uniform setters you have here

private:
    // Validity flag (ADDED)
    bool m_isValid = false;

    // Utility function for checking shader compilation errors.
    // Returns true on success, false on failure. (MODIFIED SIGNATURE)
    bool checkCompileErrors(GLuint shader, std::string type);

    // Utility function for checking shader linking errors. (ADDED)
    // Returns true on success, false on failure.
    bool checkLinkErrors(GLuint program);
};

#endif // SHADER_H