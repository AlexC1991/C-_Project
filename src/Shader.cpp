#include "Shader.h" // Include the header file

#include <GL/glew.h> // Include GLEW for OpenGL types/functions
#include <glm/gtc/type_ptr.hpp> // For glm::value_ptr

#include <fstream>
#include <sstream>
#include <iostream>
#include <string>   // Included via Shader.h, but good practice
#include <vector>   // For dynamic info log buffer

// Constructor
Shader::Shader(const char* vertexPath, const char* fragmentPath)
    : ID(0), m_isValid(false) // Initialize ID to 0 and validity flag to false
{
    // 1. Retrieve the vertex/fragment source code from filePath
    std::string vertexCode;
    std::string fragmentCode;
    std::ifstream vShaderFile;
    std::ifstream fShaderFile;
    // Ensure ifstream objects can throw exceptions:
    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try {
        // Open files
        vShaderFile.open(vertexPath);
        fShaderFile.open(fragmentPath);
        std::stringstream vShaderStream, fShaderStream;
        // Read file's buffer contents into streams
        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();
        // Close file handlers
        vShaderFile.close();
        fShaderFile.close();
        // Convert stream into string
        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();
    } catch (std::ifstream::failure& e) {
        std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << std::endl;
        std::cerr << "  Vertex Path: " << (vertexPath ? vertexPath : "NULL") << std::endl;
        std::cerr << "  Fragment Path: " << (fragmentPath ? fragmentPath : "NULL") << std::endl;
        // m_isValid remains false, ID remains 0
        return; // Exit constructor on file read failure
    }
    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();

    // 2. Compile shaders
    unsigned int vertex, fragment;

    // Vertex shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    if (!checkCompileErrors(vertex, "VERTEX")) { // Use the bool returning function
        glDeleteShader(vertex); // Clean up failed shader
        return; // Exit constructor
    }

    // Fragment Shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    if (!checkCompileErrors(fragment, "FRAGMENT")) { // Use the bool returning function
        glDeleteShader(vertex); // Clean up vertex shader too
        glDeleteShader(fragment);
        return; // Exit constructor
    }

    // Shader Program Linking
    this->ID = glCreateProgram(); // Assign the member ID
    glAttachShader(this->ID, vertex);
    glAttachShader(this->ID, fragment);
    glLinkProgram(this->ID);
    if (!checkLinkErrors(this->ID)) { // Use the new linking check function
        glDeleteShader(vertex); // Clean up shaders
        glDeleteShader(fragment);
        glDeleteProgram(this->ID); // Clean up program
        this->ID = 0; // Reset ID
        return; // Exit constructor
    }

    // Delete the shaders as they're linked into our program now and no longer necessary
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    // If all steps succeeded, mark the shader as valid
    this->m_isValid = true;
    std::cout << "Shader Program (" << this->ID << ") created successfully from: "
              << vertexPath << ", " << fragmentPath << std::endl;
}

// Destructor (Implementation ADDED)
Shader::~Shader() {
    if (this->ID != 0) { // Check if ID is valid before deleting
        glDeleteProgram(this->ID);
        // std::cout << "Deleted Shader Program (" << this->ID << ")" << std::endl; // Optional debug message
    }
}


// Use/activate the shader
void Shader::use() {
    // Only use if valid (prevents using program 0 accidentally if creation failed)
    if (m_isValid) {
        glUseProgram(ID);
    }
}

// Method to check validity (Implementation ADDED)
bool Shader::isValid() const {
    return m_isValid;
}


// Utility function for checking shader compilation errors.
// Returns true if compilation succeeded, false otherwise. (MODIFIED)
bool Shader::checkCompileErrors(GLuint shader, std::string type) {
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint logLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<GLchar> infoLog(logLength > 0 ? logLength : 1); // Ensure buffer is not zero-size
        glGetShaderInfoLog(shader, logLength, NULL, infoLog.data());
        std::cerr << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n"
                  << (logLength > 0 ? infoLog.data() : "(No info log available)")
                  << "\n -- --------------------------------------------------- -- " << std::endl;
    }
    return success; // Return the success status (true or false)
}

// Utility function for checking shader linking errors.
// Returns true if linking succeeded, false otherwise. (ADDED)
bool Shader::checkLinkErrors(GLuint program) {
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLint logLength;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<GLchar> infoLog(logLength > 0 ? logLength : 1); // Ensure buffer is not zero-size
        glGetProgramInfoLog(program, logLength, NULL, infoLog.data());
        std::cerr << "ERROR::PROGRAM_LINKING_ERROR of type: PROGRAM\n"
                  << (logLength > 0 ? infoLog.data() : "(No info log available)")
                  << "\n -- --------------------------------------------------- -- " << std::endl;
    }
    return success; // Return the success status (true or false)
}


// --- Implementations for your uniform setters ---
// Added the m_isValid check to prevent errors if shader failed to build
void Shader::setBool(const std::string &name, bool value) const {
    if(m_isValid) glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
}

void Shader::setInt(const std::string &name, int value) const {
    if(m_isValid) glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::setFloat(const std::string &name, float value) const {
    if(m_isValid) glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::setVec3(const std::string &name, const glm::vec3 &value) const {
    // Check m_isValid before calling glGetUniformLocation and glUniform3fv
    if(m_isValid) glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, glm::value_ptr(value));
}

void Shader::setMat4(const std::string &name, const glm::mat4 &value) const {
    // Check m_isValid before calling glGetUniformLocation and glUniformMatrix4fv
    if(m_isValid) glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(value));
}

// Add implementations for any other setters you have, including the if(m_isValid) check
